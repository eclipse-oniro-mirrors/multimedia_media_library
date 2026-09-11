/*
 * Copyright (C) 2024-2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "Media_CloudDrive"

#include "cloud_sync_utils.h"

#include "datashare_helper.h"
#include "iservice_registry.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "settings_data_manager.h"

using namespace std;
using namespace OHOS::DataShare;

namespace OHOS {
namespace Media {
static const std::string CLOUD_BASE_URI = "datashareproxy://generic.cloudstorage";
static const std::string CLOUD_DATASHARE_URI = CLOUD_BASE_URI + "/cloud_sp";
static const std::string CLOUD_SYNC_SWITCH_URI = CLOUD_BASE_URI + "/sync_switch";
static const std::string MOBILE_NETWORK_STATUS_ON = "1";

static std::shared_ptr<DataShare::DataShareHelper> GetCloudHelper(const std::string &uri)
{
    if (uri.empty()) {
        MEDIA_ERR_LOG("uri is empty.");
        return nullptr;
    }
    CreateOptions options;
    options.enabled_ = true;
    return DataShare::DataShareHelper::Creator(uri, options);
}

bool CloudSyncUtils::IsUnlimitedTrafficStatusOn()
{
    std::shared_ptr<DataShare::DataShareHelper> cloudHelper = GetCloudHelper(CLOUD_BASE_URI);
    if (cloudHelper == nullptr) {
        MEDIA_INFO_LOG("cloudHelper is null");
        return false;
    }

    DataShare::DataSharePredicates predicates;
    predicates.EqualTo("key", "useMobileNetworkData");
    Uri cloudUri(CLOUD_DATASHARE_URI);
    vector<string> columns = { "value" };
    shared_ptr<DataShare::DataShareResultSet> resultSet = cloudHelper->Query(cloudUri, predicates, columns);
    if (resultSet == nullptr) {
        MEDIA_INFO_LOG("resultSet is nullptr");
        return false;
    }
    string switchOn = "0";
    if (resultSet->GoToNextRow() == E_OK) {
        resultSet->GetString(0, switchOn);
    }
    return switchOn == MOBILE_NETWORK_STATUS_ON;
}

bool CloudSyncUtils::IsCloudSyncSwitchOn()
{
    auto switchStatus = SettingsDataManager::GetPhotosSyncSwitchStatus();
    if (switchStatus != SwitchStatus::NONE) {
        MEDIA_DEBUG_LOG("GetPhotosSyncSwitchStatus success, switchStatus: %{public}d", static_cast<int>(switchStatus));
        return switchStatus != SwitchStatus::CLOSE;
    }
    MEDIA_DEBUG_LOG("GetPhotosSyncSwitchStatus fail, continue query old sync switch");

    std::shared_ptr<DataShare::DataShareHelper> cloudHelper = GetCloudHelper(CLOUD_BASE_URI);
    CHECK_AND_RETURN_RET_LOG(cloudHelper != nullptr, false, "cloudHelper is null");

    DataShare::DataSharePredicates predicates;
    predicates.EqualTo("bundleName", "generic.cloudstorage");
    Uri cloudUri(CLOUD_SYNC_SWITCH_URI);
    vector<string> columns = { "isSwitchOn" };
    shared_ptr<DataShare::DataShareResultSet> resultSet = cloudHelper->Query(cloudUri, predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "resultSet is null");

    string switchOn = "0";
    if (resultSet->GoToNextRow() == E_OK) {
        resultSet->GetString(0, switchOn);
    }
    return switchOn == MOBILE_NETWORK_STATUS_ON;
}

bool CloudSyncUtils::IsCloudDataAgingPolicyOn()
{
    std::shared_ptr<DataShare::DataShareHelper> cloudHelper = GetCloudHelper(CLOUD_BASE_URI);
    CHECK_AND_RETURN_RET_LOG(cloudHelper != nullptr, false, "cloudHelper is null");
    
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo("key", "dataAgingPolicy");
    Uri cloudUri(CLOUD_DATASHARE_URI);
    vector<string> columns = { "value" };
    shared_ptr<DataShare::DataShareResultSet> resultSet = cloudHelper->Query(cloudUri, predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "resultSet is nullptr");
    string switchOn = "0";
    if (resultSet->GoToNextRow() == E_OK) {
        resultSet->GetString(0, switchOn);
    }
    return switchOn == MOBILE_NETWORK_STATUS_ON;
}

bool CloudSyncUtils::IsSwitchOn(const std::string &bundleName)
{
    std::shared_ptr<DataShare::DataShareHelper> cloudHelper = GetCloudHelper(CLOUD_BASE_URI);
    CHECK_AND_RETURN_RET_LOG(cloudHelper != nullptr, false, "cloudHelper is null");

    DataShare::DataSharePredicates predicates;
    predicates.EqualTo("bundleName", bundleName);
    Uri uri(CLOUD_SYNC_SWITCH_URI);
    std::vector<std::string> columns = {"isSwitchOn"};
    std::shared_ptr<DataShare::DataShareResultSet> resultSet = cloudHelper->Query(uri, predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "resultSet is null");

    std::string switchOn = "0";
    if (resultSet->GoToNextRow() == E_OK) {
        resultSet->GetString(0, switchOn);
    }
    resultSet->Close();
    return switchOn == MOBILE_NETWORK_STATUS_ON;
}

bool CloudSyncUtils::IsSharedAlbumCloudSyncSwitchOn()
{
    const static std::string shareAlbumBundleName = "com.ohos.photos.shared";
    const bool isSwitchOn = IsSwitchOn(shareAlbumBundleName);
    MEDIA_DEBUG_LOG("IsSharedAlbumCloudSyncSwitchOn, isSwitchOn: %{public}d", isSwitchOn);
    return isSwitchOn;
}
} // namespace Media
} // namespace OHOS