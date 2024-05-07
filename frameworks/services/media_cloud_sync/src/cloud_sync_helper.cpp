/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "medialibrary_errno.h"
#include "media_log.h"
#include "parameters.h"
#include "post_event_utils.h"
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
        instance_ = shared_ptr<CloudSyncHelper>(new (nothrow)CloudSyncHelper());
    }

    return instance_;
}

CloudSyncHelper::CloudSyncHelper() : timer_("CloudSync")
{
    timer_.Setup();
}

CloudSyncHelper::~CloudSyncHelper()
{
    timer_.Unregister(timerId_);
    timer_.Shutdown();
}

void CloudSyncHelper::StartSync()
{
    lock_guard<mutex> lock(syncMutex_);
    if (isPending_) {
        /* cancel the previous timer */
        timer_.Unregister(timerId_);
    } else {
        isPending_ = true;
    }
    timerId_ = timer_.Register(bind(&CloudSyncHelper::OnTimerCallback, this),
        SYNC_INTERVAL, true);
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
    if (dataShareHelper_ == nullptr) {
        MEDIA_ERR_LOG("dataShareHelper is nullptr");
        return false;
    }
    return true;
}

bool CloudSyncHelper::IsSyncSwitchOpen()
{
    if (!InitDataShareHelper()) {
        return true;
    }

    Uri uri(uri_);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(BUNDLE_NAME_KEY, GALLERY_BUNDLE_NAME);
    std::vector<std::string> columns = {SWITCH_STATUS_KEY};
    auto resultSet = dataShareHelper_->Query(uri, predicates, columns);
    if (resultSet == nullptr) {
        MEDIA_INFO_LOG("resultSet is null, maybe never login");
        return false;
    }

    int32_t rowCount = -1;
    int32_t ret = resultSet->GetRowCount(rowCount);
    if (ret != 0 || rowCount < 0) {
        MEDIA_ERR_LOG("get cloud status fail ret is %{public}d, rowcount is %{public}d", ret, rowCount);
        return true;
    } else if (rowCount == 0) {
        MEDIA_INFO_LOG("rowCount is 0");
        return true;
    }

    int64_t status = 0;
    int32_t columnIndex = -1;
    ret = resultSet->GoToFirstRow();
    if (ret != 0) {
        MEDIA_ERR_LOG("goto first err");
        return true;
    }
    ret = resultSet->GetColumnIndex(SWITCH_STATUS_KEY, columnIndex);
    if (ret != 0) {
        MEDIA_ERR_LOG("Get Column index err");
        return true;
    }
    ret = resultSet->GetLong(columnIndex, status);
    if (ret != 0) {
        MEDIA_ERR_LOG("get long err");
        return true;
    }
    return status == 1;
}

void CloudSyncHelper::OnTimerCallback()
{
    if (!IsSyncSwitchOpen()) {
        return;
    }

    unique_lock<mutex> lock(syncMutex_);
    isPending_ = false;
    lock.unlock();

    MEDIA_INFO_LOG("cloud sync manager start sync");
    auto callback = make_shared<MediaCloudSyncCallback>();
    int32_t ret = CloudSyncManager::GetInstance().StartSync(false, callback);
    if (ret != 0) {
        MEDIA_ERR_LOG("cloud sync manager start sync err %{public}d", ret);
    }
}

void MediaCloudSyncCallback::OnSyncStateChanged(SyncType type, SyncPromptState state)
{
    MEDIA_INFO_LOG("sync type %{public}d, state %{public}d", type, state);
}

} // namespace Media
} // namespace OHOS
