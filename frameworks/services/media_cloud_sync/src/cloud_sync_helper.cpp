/*
 * Copyright (C) 2023-2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define MLOG_TAG "Cloud"

#include "cloud_sync_helper.h"

#include "medialibrary_all_album_refresh_processor.h"
#include "medialibrary_errno.h"
#include "media_log.h"
#include "parameters.h"
#include "post_event_utils.h"
#include "settings_data_manager.h"
#include "hi_audit.h"

namespace OHOS {
namespace Media {
using namespace std;
using namespace FileManagement::CloudSync;

shared_ptr<CloudSyncHelper> CloudSyncHelper::instance_ = nullptr;
mutex CloudSyncHelper::instanceMutex_;

shared_ptr<CloudSyncHelper> CloudSyncHelper::GetInstance()
{
    if (instance_ == nullptr) {
        lock_guard<mutex> guard(instanceMutex_);
        if (instance_ != nullptr) {
            return instance_;
        }
        auto *cloudSyncHelper = new (nothrow)CloudSyncHelper();
        if (cloudSyncHelper == nullptr) {
            MEDIA_ERR_LOG("Failed to new cloudSyncHelper");
        }
        instance_ = shared_ptr<CloudSyncHelper>(cloudSyncHelper);
    }

    return instance_;
}

CloudSyncHelper::~CloudSyncHelper()
{
    {
        lock_guard<mutex> lock(syncMutex_);
        isSyncStopped_ = true;
    }
    while (runningThreadNum_ != 0) {
        skipCond_.notify_all();
        this_thread::sleep_for(std::chrono::milliseconds(WAIT_INTERVAL));
    }
}

void CloudSyncHelper::StartSync()
{
    MEDIA_DEBUG_LOG("CloudSyncHelper StartSync");
    lock_guard<mutex> lock(syncMutex_);
    skipCond_.notify_all();
    if (!isThreadPending_ && !isSyncStopped_) {
        isThreadPending_ = true;
        runningThreadNum_++;
        std::thread([this]() {
            OnTimerCallback();
            {
                lock_guard<mutex> lock(syncMutex_);
                runningThreadNum_--;
            }
        }).detach();
    }
}

bool CloudSyncHelper::InitDataShareHelper()
{
    const string photos = "persist.kernel.bundle_name.photots";
    const string clouddrive = "persist.kernel.bundle_name.clouddrive";
    GALLERY_BUNDLE_NAME = system::GetParameter(photos, "");
    const auto CLOUDDRIVE_BUNDLE_NAME = system::GetParameter(clouddrive, "");
    if (GALLERY_BUNDLE_NAME == "") {
        MEDIA_ERR_LOG("can't get gallery bundle name");
        return false;
    }
    if (CLOUDDRIVE_BUNDLE_NAME == "") {
        MEDIA_ERR_LOG("can't get clouddrive bundle name");
        return false;
    }

    const int32_t INVALID_UID = -1;
    const int32_t BASE_USER_RANGE = 200000;
    int uid = IPCSkeleton::GetCallingUid();
    if (uid <= INVALID_UID) {
        MEDIA_ERR_LOG("Get INVALID_UID UID %{public}d", uid);
        return false;
    }
    int32_t userId = uid / BASE_USER_RANGE;

    uri_ = QUERY_URI + CLOUDDRIVE_BUNDLE_NAME +
        SYNC_SWITCH_SUFFIX +
        std::to_string(userId);
    DataShare::CreateOptions options;
    options.enabled_ = true;
    dataShareHelper_ = DataShare::DataShareHelper::Creator(uri_, options);
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr, false, "dataShareHelper is nullptr");
    return true;
}

bool CloudSyncHelper::IsSyncSwitchOpen()
{
    auto switchStatus = SettingsDataManager::GetPhotosSyncSwitchStatus();
    if (switchStatus != SwitchStatus::NONE) {
        MEDIA_DEBUG_LOG("GetPhotosSyncSwitchStatus success, switchStatus: %{public}d", static_cast<int>(switchStatus));
        return switchStatus != SwitchStatus::CLOSE;
    }
    MEDIA_DEBUG_LOG("GetPhotosSyncSwitchStatus fail, continue query old sync switch");

    shared_ptr<DataShare::DataShareResultSet> resultSet;
    {
        lock_guard<mutex> lock(uriMutex_);
        if (!InitDataShareHelper()) {
            return true;
        }

        Uri uri(uri_);
        DataShare::DataSharePredicates predicates;
        predicates.EqualTo(BUNDLE_NAME_KEY, GALLERY_BUNDLE_NAME);
        std::vector<std::string> columns = {SWITCH_STATUS_KEY};
        resultSet = dataShareHelper_->Query(uri, predicates, columns);
    }
    
    bool errConn = resultSet == nullptr;
    CHECK_AND_RETURN_RET_LOG(!errConn, false, "Media_Cloud: resultSet is null, maybe never login");

    int32_t rowCount = -1;
    int32_t ret = resultSet->GetRowCount(rowCount);
    errConn = ret != 0 || rowCount < 0;
    CHECK_AND_RETURN_RET_LOG(
        !errConn, false, "Media_Cloud: get cloud status fail ret is %{public}d, rowcount is %{public}d", ret, rowCount);
    errConn = rowCount == 0;
    CHECK_AND_RETURN_RET_LOG(!errConn, false, "Media_Cloud: rowCount is 0");

    int64_t status = 0;
    int32_t columnIndex = -1;
    ret = resultSet->GoToFirstRow();
    CHECK_AND_RETURN_RET_LOG(ret == 0, false, "Media_Cloud: goto first err");

    ret = resultSet->GetColumnIndex(SWITCH_STATUS_KEY, columnIndex);
    CHECK_AND_RETURN_RET_LOG(ret == 0, false, "Media_Cloud: Get Column index err");

    ret = resultSet->GetLong(columnIndex, status);
    CHECK_AND_RETURN_RET_LOG(ret == 0, false, "Media_Cloud: get long err");
    return status == 1;
}

void CloudSyncHelper::OnTimerCallback()
{
    {
        unique_lock<mutex> lock(skipMutex_);
        while (skipCond_.wait_for(lock, std::chrono::milliseconds(SYNC_INTERVAL)) == std::cv_status::no_timeout) {
            if (isSyncStopped_) {
                isThreadPending_ = false;
                return;
            }
        }
    }

    {
        lock_guard<mutex> lock(syncMutex_);
        isThreadPending_ = false;
    }

    if (!IsSyncSwitchOpen()) {
        return;
    }

    MEDIA_INFO_LOG("cloud sync manager start sync");
    auto callback = make_shared<MediaCloudSyncCallback>();
    int32_t ret = CloudSyncManager::GetInstance().StartSync(false, callback);
    CHECK_AND_PRINT_LOG(ret == 0, "cloud sync manager start sync err %{public}d", ret);
    HiAudit::GetInstance().WriteForTriggerSync("SWITCH_ON", ret == 0 ? "success" : "fail");
}

void MediaCloudSyncCallback::OnSyncStateChanged(SyncType type, SyncPromptState state)
{
    MEDIA_INFO_LOG("OnSyncStateChanged SyncType %{public}d, SyncPromptState %{public}d", type, state);
}

void MediaCloudSyncCallback::OnSyncStateChanged(CloudSyncState state, ErrorType error)
{
    MEDIA_INFO_LOG("OnSyncStateChanged CloudSyncState %{public}d, ErrorType %{public}d", state, error);
    unique_lock<mutex> lock(syncStateMutex_);
    bool isSyncing = (state == CloudSyncState::UPLOADING || state == CloudSyncState::DOWNLOADING);
    CHECK_AND_RETURN_LOG(isSyncing_ != isSyncing, "OnSyncStateChanged check failed");
    isSyncing_ = isSyncing;
    MediaLibraryAllAlbumRefreshProcessor::GetInstance()->OnCloudSyncStateChanged(isSyncing_);
}
} // namespace Media
} // namespace OHOS
