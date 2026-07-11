/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include <string>
#include <vector>

#include "cloud_sync_switch_observer.h"
#include "media_analysis_helper.h"
#include "medialibrary_unistore_manager.h"
#include "parameters.h"
#include "result_set_utils.h"
#include "lcd_download_operation.h"

namespace OHOS {
namespace Media {
using namespace std;

const std::string QUERY_URI = "datashareproxy://";
DataShare::CreateOptions options;
constexpr int32_t SYNC_INTERVAL = 20000;
// LCOV_EXCL_START
void CloudSyncSwitchObserver::OnChange()
{
    MEDIA_INFO_LOG("Cloud Sync Switch Status change");
    lock_guard<mutex> lock(syncMutex_);
        if (!isPending_) {
            MEDIA_INFO_LOG("CloudSyncSwitchObserver set timer handle index");
            std::thread([this]() {
                this->HandleIndex();
            }).detach();
            isPending_ = true;
        }
}

void CloudSyncSwitchObserver::HandleIndex()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(SYNC_INTERVAL));
    lock_guard<mutex> lock(syncMutex_);

    if (LcdDownloadOperation::GetInstance()->GetLcdDownloadStatus() == LcdDownloadStatus::DOWNLOADING) {
        MEDIA_INFO_LOG("Cloud sync switch changed, stop LCD download task");
        LcdDownloadOperation::GetInstance()->CancelDownload();
    }

    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(uniStore != nullptr, "uniStore is nullptr!");

    //delete index
    const std::string queryIdToDeleteIndex = "SELECT file_id FROM tab_analysis_search_index WHERE photo_status = -1";
    auto resultSet = uniStore->QuerySql(queryIdToDeleteIndex);
    CHECK_AND_PRINT_LOG(resultSet != nullptr, "resultSet is nullptr!");
    
    std::vector<std::string> idToDeleteIndex;
    while (resultSet != nullptr && resultSet->GoToNextRow() == NativeRdb::E_OK) {
        idToDeleteIndex.push_back(to_string(GetInt32Val("file_id", resultSet)));
    }
    MEDIA_INFO_LOG("idToDeleteIndex size: %{public}zu", idToDeleteIndex.size());
    if (!idToDeleteIndex.empty()) {
        MediaAnalysisHelper::AsyncStartMediaAnalysisService(
            static_cast<int32_t>(MediaAnalysisProxy::ActivateServiceType::START_DELETE_INDEX), idToDeleteIndex);
    }

    //update index
    const std::string queryIdToUpdateIndex = "SELECT file_id FROM tab_analysis_search_index WHERE photo_status = 2";
    auto resultSetUpdateIndex = uniStore->QuerySql(queryIdToUpdateIndex);
    CHECK_AND_RETURN_LOG(resultSetUpdateIndex != nullptr, "resultSetUpdateIndex is nullptr!");
    std::vector<std::string> idToUpdateIndex;
    while (resultSetUpdateIndex->GoToNextRow() == NativeRdb::E_OK) {
        idToUpdateIndex.push_back(to_string(GetInt32Val("file_id", resultSetUpdateIndex)));
    }
    MEDIA_INFO_LOG("idToUpdateIndex size: %{public}zu", idToUpdateIndex.size());
    if (!idToUpdateIndex.empty()) {
        MediaAnalysisHelper::AsyncStartMediaAnalysisService(
            static_cast<int32_t>(MediaAnalysisProxy::ActivateServiceType::START_UPDATE_INDEX), idToUpdateIndex);
    }
    isPending_ = false;
}

void CloudSyncSwitchManager::RegisterObserver()
{
    options.enabled_ = true;
    auto dataShareHelper = DataShare::DataShareHelper::Creator(QUERY_URI, options);
    CHECK_AND_RETURN_LOG(dataShareHelper != nullptr, "dataShareHelper is nullptr");

    const string photos = "persist.kernel.bundle_name.photos";
    const string clouddrive = "persist.kernel.bundle_name.clouddrive";
    const std::string GALLERY_BUNDLE_NAME = system::GetParameter(photos, "");
    const std::string CLOUDDRIVE_BUNDLE_NAME = system::GetParameter(clouddrive, "");
    CHECK_AND_RETURN_LOG(GALLERY_BUNDLE_NAME != "", "can't get gallery bundle name");
    CHECK_AND_RETURN_LOG(CLOUDDRIVE_BUNDLE_NAME != "", "can't get clouddrive bundle name");
    std::string queryUri = QUERY_URI + CLOUDDRIVE_BUNDLE_NAME + "/sync_switch?bundleName=" + GALLERY_BUNDLE_NAME;

    sptr<CloudSyncSwitchObserver> switchObserver(new (std::nothrow) CloudSyncSwitchObserver());
    CHECK_AND_RETURN(switchObserver != nullptr);
    Uri observerUri(queryUri);
    dataShareHelper->RegisterObserver(observerUri, switchObserver);
}

void CloudSyncSwitchManager::UnRegisterObserver()
{
    MEDIA_ERR_LOG("CloudSyncSwitchManager UnRegisterObserver");
}
// LCOV_EXCL_STOP
} // namespace Media
} // namespace OHOS
