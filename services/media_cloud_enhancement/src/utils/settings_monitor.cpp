/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "settings_monitor.h"
#include "enhancement_manager.h"
#include "medialibrary_type_const.h"
#include "media_log.h"

namespace OHOS {
namespace Media {
constexpr const char *SETTINGS_DATA_EXT_URI = "datashare:///com.ohos.settingsdata.DataAbility";
const std::string SETTING_KEY = "KEYWORD";
const std::string SETTING_VALUE = "VALUE";
const std::string WATER_MARK_ENABLED = "true";
constexpr int PHOTOS_STORAGE_MANAGER_ID = 5003;

std::shared_ptr<DataShare::DataShareHelper> SettingsMonitor::CreateNonBlockDataShareHelper()
{
    sptr<ISystemAbilityManager> saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (saManager == nullptr) {
        MEDIA_ERR_LOG("CreateNonBlockDataShareHelper: GetSystemAbilityManager failed.");
        return nullptr;
    }
    sptr<IRemoteObject> remoteObj = saManager->GetSystemAbility(PHOTOS_STORAGE_MANAGER_ID);
    if (remoteObj == nullptr) {
        MEDIA_ERR_LOG("CreateNonBlockDataShareHelper: GetSystemAbility Service Failed.");
        return nullptr;
    }
    auto [ret, helper] = DataShare::DataShareHelper::Create(remoteObj, SETTINGS_DATASHARE_URI, SETTINGS_DATA_EXT_URI);
    if (ret == DataShare::E_OK) {
        return helper;
    } else if (ret == DataShare::E_DATA_SHARE_NOT_READY) {
        MEDIA_ERR_LOG("CreateNonBlockDataShareHelper: datashare not ready.");
        return nullptr;
    } else {
        MEDIA_ERR_LOG("CreateNonBlockDataShareHelper: create datashare fail, ret = %{public}d.", ret);
        return nullptr;
    }
}

std::shared_ptr<DataShare::DataShareHelper> SettingsMonitor::CreateDataShareHelper()
{
    sptr<ISystemAbilityManager> saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (saManager == nullptr) {
        MEDIA_ERR_LOG("CreateDataShareHelper, GetSystemAbilityManager failed.");
        return nullptr;
    }
    sptr<IRemoteObject> remoteObj = saManager->GetSystemAbility(PHOTOS_STORAGE_MANAGER_ID);

    if (remoteObj == nullptr) {
        MEDIA_ERR_LOG("CreateDataShareHelper, GetSystemAbility Service Failed.");
        return nullptr;
    }
    return DataShare::DataShareHelper::Creator(remoteObj, SETTINGS_DATASHARE_URI, SETTINGS_DATA_EXT_URI);
}

void SettingsMonitor::RegisterSettingsObserver(const Uri &uri,
    const sptr<AAFwk::IDataAbilityObserver> &dataObserver)
{
    auto settingHelper = CreateDataShareHelper();
    if (settingHelper == nullptr) {
        MEDIA_ERR_LOG("RegisterSettingsObserver, settingHelper is nullptr");
        return;
    }
    settingHelper->RegisterObserver(uri, dataObserver);
    settingHelper->Release();
    return;
}

void SettingsMonitor::UnregisterSettingsObserver(const Uri &uri,
    const sptr<AAFwk::IDataAbilityObserver> &dataObserver)
{
    auto settingHelper = CreateDataShareHelper();
    if (settingHelper == nullptr) {
        MEDIA_ERR_LOG("UnregisterSettingsObserver, settingHelper is nullptr");
        return;
    }
    settingHelper->UnregisterObserver(uri, dataObserver);
    settingHelper->Release();
    return;
}

int32_t SettingsMonitor::Insert(Uri uri, const std::string &key, const std::string &value)
{
    std::shared_ptr<DataShare::DataShareHelper> settingHelper = CreateDataShareHelper();
    if (settingHelper == nullptr) {
        MEDIA_ERR_LOG("settingHelper is null");
        return 0;
    }
    DataShare::DataShareValuesBucket valuesBucket;
    DataShare::DataShareValueObject keyObj(key);
    DataShare::DataShareValueObject valueObj(value);
    valuesBucket.Put(SETTING_KEY, keyObj);
    valuesBucket.Put(SETTING_VALUE, valueObj);
    int32_t result = settingHelper->Insert(uri, valuesBucket);
    if (result == -1) {
        settingHelper->Release();
        return 0;
    }
    MEDIA_ERR_LOG("insert success");
    settingHelper->NotifyChange(uri);
    settingHelper->Release();
    return 0;
}

int32_t SettingsMonitor::Query(Uri uri, const std::string &key, std::string &value)
{
    std::shared_ptr<DataShare::DataShareHelper> settingHelper = CreateNonBlockDataShareHelper();
    if (settingHelper == nullptr) {
        MEDIA_ERR_LOG("settingHelper is null");
        return 0;
    }

    std::vector<std::string> columns;
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(SETTING_KEY, key);
    auto result = settingHelper->Query(uri, predicates, columns);
    if (result == nullptr) {
        MEDIA_ERR_LOG("SettingsMonitor: query error, result is null");
        settingHelper->Release();
        return 0;
    }

    if (result->GoToFirstRow() != DataShare::E_OK) {
        MEDIA_ERR_LOG("SettingsMonitor: query error, go to first row error");
        result->Close();
        settingHelper->Release();
        return 0;
    }

    int columnIndex = 0;
    result->GetColumnIndex(SETTING_VALUE, columnIndex);
    result->GetString(columnIndex, value);
    result->Close();
    settingHelper->Release();
    MEDIA_ERR_LOG("SettingsMonitor: query success");
    return 0;
}

std::string SettingsMonitor::QueryPhotosAutoOption()
{
    Uri uri(SETTINGS_DATASHARE_AUTO_OPTION_URI);
    std::string key = "persist.photos.ce.auto.option";
    std::string value = "";
    SettingsMonitor::Query(uri, key, value);
    MEDIA_INFO_LOG("QueryPhotosAutoOption value: %{public}s", value.c_str());
    return value;
}

bool SettingsMonitor::QueryPhotosWaterMark()
{
    Uri uri(SETTINGS_DATASHARE_WATER_MARK_URI);
    std::string key = "persist.photos.ce.watermark.enable";
    std::string value = "";
    SettingsMonitor::Query(uri, key, value);
    MEDIA_INFO_LOG("QueryPhotosWaterMark value: %{public}s", value.c_str());
    return value == WATER_MARK_ENABLED;
}

void PhotosAutoOptionObserver::OnChange()
{
    MEDIA_INFO_LOG("PhotosAutoOptionObserver OnChange");
    EnhancementManager::GetInstance().HandlePhotosAutoOptionChange(SettingsMonitor::QueryPhotosAutoOption());
}

void PhotosWaterMarkObserver::OnChange()
{
    MEDIA_INFO_LOG("PhotosWaterMarkObserver OnChange");
    EnhancementManager::GetInstance().HandlePhotosWaterMarkChange(SettingsMonitor::QueryPhotosWaterMark());
}

} // namespace Media
} // namespace OHOS
