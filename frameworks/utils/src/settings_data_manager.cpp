/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
 
#define MLOG_TAG "SettingsDataManager"

#include "settings_data_manager.h"

#include <memory>

#include "datashare_helper.h"
#include "datashare_errno.h"
#include "datashare_result_set.h"
#include "iservice_registry.h"
#include "medialibrary_errno.h"
#include "media_log.h"
#include "media_uri_utils.h"
#ifdef MEDIA_LIBRARY_MEMOSPACE_SERVICE_SUPPORT
#include "hdc_device_id.h"
#endif
#include "medialibrary_tracer.h"

namespace OHOS::Media {
inline static const std::string SETTING_DATA_QUERY_URI = "datashareproxy://";
inline static const std::string SETTING_DATA_COMMON_URI =
    "datashare:///com.ohos.settingsdata/entry/settingsdata/SETTINGSDATA?Proxy=true";
inline static const std::string PHOTOS_USER_PROPERTY_INIT_URI =
    "datashare:///com.huawei.hmos.photos.provider.userPropertySettings?init";
inline static const std::string PHOTOS_SYNC_SWITCH_KEY = "photos_sync_options";
inline static const std::string PHOTOS_USER_PROPERTY_URI =
    "datashare:///com.huawei.hmos.photos.provider.userPropertySettings";
inline static const std::string USER_SETTINGS_ENABLE = "photos_user_property_settings_enable";
inline static const std::string PHOTOS_SYNC_SWITCH_USER_KEY = "photos_sync_options_user";
inline static const std::string ALL_PHOTOS_ALBUM_UPLOAD_USER = "photos_all_album_upload_user";
inline static const std::string ALL_PHOTOS_ALBUM_UPLOAD = "photos_all_album_upload";
inline static const std::string ALL_PHOTOS_ALBUM_UPLOAD_OFF = "0";
inline static const std::string PHOTOS_ALL_ALBUM_UPLOAD_COMFIRMED = "photos_all_album_upload_comfirmed";
constexpr int PHOTOS_STORAGE_MANAGER_ID = 5003;

inline static const std::string CLONE_SEARCH_STATUS = "clone_search_status";
inline static const std::string CLONE_SEARCH_INDEXING = "3";

class MediaSettingDataHelper {
public:
    MediaSettingDataHelper()
    {
        DataShare::CreateOptions options;
        options.enabled_ = true;
        dataShareHelper_ = DataShare::DataShareHelper::Creator(SETTING_DATA_QUERY_URI, options);
        if (dataShareHelper_ == nullptr) {
            MEDIA_ERR_LOG("dataShareHelper = nullptr");
            return;
        }
    }

    ~MediaSettingDataHelper()
    {
        if (dataShareHelper_) {
            if (!dataShareHelper_->Release()) {
                MEDIA_INFO_LOG("Release set data share helper failed");
                return;
            }
        }
    }

    explicit operator bool() const noexcept
    {
        return dataShareHelper_ == nullptr ? false : true;
    }

    DataShare::DataShareHelper& operator*() const
    {
        return *dataShareHelper_;
    }

    DataShare::DataShareHelper* operator->() const noexcept
    {
        return dataShareHelper_.get();
    }

private:
    std::shared_ptr<DataShare::DataShareHelper> dataShareHelper_;
};

static int32_t GetUserId()
{
    int32_t uid = static_cast<int32_t>(getuid());
    constexpr uint32_t BASE_USER_RANGE = 200000;
    return uid / BASE_USER_RANGE;
}

static SwitchStatus StringToSwitchStatus(const std::string& value)
{
    static const std::unordered_map<std::string, SwitchStatus> STRING_SWITCH_STATUS_MAP = {
        { std::to_string(static_cast<int>(SwitchStatus::CLOSE)), SwitchStatus::CLOSE },
        { std::to_string(static_cast<int>(SwitchStatus::CLOUD)), SwitchStatus::CLOUD },
        { std::to_string(static_cast<int>(SwitchStatus::HDC)), SwitchStatus::HDC },
    };
    CHECK_AND_RETURN_RET_LOG(STRING_SWITCH_STATUS_MAP.count(value), SwitchStatus::NONE,
        "invalid SwitchStatus: %{public}s", value.c_str());
    return STRING_SWITCH_STATUS_MAP.at(value);
}

static int32_t QueryParamInDeviceSettingData(const std::string &key, std::string &value)
{
    MediaLibraryTracer tracer;
    tracer.Start("SettingsDataManager::QueryParamInDeviceSettingData");

    MediaSettingDataHelper dataShareHelper;
    if (!dataShareHelper) {
        MEDIA_ERR_LOG("failed to init dataShareHelper");
        return E_DB_FAIL;
    }

    MEDIA_DEBUG_LOG("Query key: %{public}s", key.c_str());
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo("KEYWORD", key);
    std::vector<std::string> columns = {"value"};
    std::string queryUri = SETTING_DATA_COMMON_URI + "&key=" + key;
    Uri uri(queryUri);

    std::shared_ptr<DataShare::DataShareResultSet> resultSet = dataShareHelper->Query(uri, predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_DB_FAIL, "resultSet == nullptr");
    auto ret = resultSet->GoToFirstRow();
    CHECK_AND_RETURN_RET_LOG(ret == DataShare::E_OK, ret, "GoToFirstRow failed, err: %{public}d", ret);

    int32_t columnIndex = 0;
    ret = resultSet->GetColumnIndex("value", columnIndex);
    CHECK_AND_RETURN_RET_LOG(ret == DataShare::E_OK, ret, "GetColumnIndex failed, err: %{public}d", ret);
    ret = resultSet->GetString(columnIndex, value);
    CHECK_AND_RETURN_RET_LOG(ret == DataShare::E_OK, ret, "GetString failed, err: %{public}d", ret);
    MEDIA_DEBUG_LOG("Query success, value: %{public}s", value.c_str());
    return E_OK;
}

static int32_t UpdateParamInDeviceSettingData(const std::string &key, const std::string &value)
{
    MediaSettingDataHelper dataShareHelper;
    if (!dataShareHelper) {
        MEDIA_ERR_LOG("failed to init dataShareHelper");
        return E_DB_FAIL;
    }

    MEDIA_INFO_LOG("Update device key: %{public}s", key.c_str());
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo("KEYWORD", key);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put("VALUE", value);

    std::string updateUri = SETTING_DATA_COMMON_URI + "&key=" + key;
    Uri uri(updateUri);
    auto ret = dataShareHelper->UpdateEx(uri, predicates, valuesBucket);
    CHECK_AND_RETURN_RET_LOG(ret.first == DataShare::E_OK, ret.first,
        "update device key failed, err: %{public}d", ret.first);
    return ret.first;
}

static int32_t InsertParamInDeviceSettingData(const std::string &key, const std::string &value)
{
    MediaSettingDataHelper dataShareHelper;
    if (!dataShareHelper) {
        MEDIA_ERR_LOG("failed to init dataShareHelper");
        return E_DB_FAIL;
    }

    MEDIA_INFO_LOG("Insert device key: %{public}s", key.c_str());
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put("KEYWORD", key);
    valuesBucket.Put("VALUE", value);

    std::string insertUri = SETTING_DATA_COMMON_URI + "&key=" + key;
    Uri uri(insertUri);
    auto ret = dataShareHelper->InsertEx(uri, valuesBucket);
    CHECK_AND_RETURN_RET_LOG(ret.first == DataShare::E_OK, ret.first,
        "insert device key failed, return: %{public}d", ret.first);
    return ret.first;
}

SwitchStatus SettingsDataManager::GetPhotosSyncSwitchDeviceStatus()
{
    std::string value;
    auto ret = QueryParamInDeviceSettingData(PHOTOS_SYNC_SWITCH_KEY, value);
    MEDIA_DEBUG_LOG("settingsData, ret: %{public}d, key: %{public}s, value: %{public}s",
        ret,
        PHOTOS_SYNC_SWITCH_KEY.c_str(),
        value.c_str());
    return StringToSwitchStatus(value);
}

int32_t SettingsDataManager::NotifyPhotosSyncSwitchInitByUpdate()
{
    MediaLibraryTracer tracer;
    tracer.Start("SettingsDataManager::NotifyPhotosSyncSwitchInitByUpdate");

    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    CHECK_AND_RETURN_RET_LOG(samgr != nullptr, E_DB_FAIL, "Failed to get SystemAbilityManagerClient");

    auto object = samgr->GetSystemAbility(PHOTOS_STORAGE_MANAGER_ID);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, E_DB_FAIL, "Failed to get system ability");

    auto dataShareHelper = DataShare::DataShareHelper::Creator(object, PHOTOS_USER_PROPERTY_URI);
    CHECK_AND_RETURN_RET_LOG(dataShareHelper != nullptr, E_DB_FAIL, "Failed to create dataShareHelper");

    DataShare::DataSharePredicates predicates;
    DataShare::DataShareValuesBucket valuesBucket;
    DataShare::DataShareValueObject valueObject(1);
    valuesBucket.Put("init", valueObject);
    Uri uri(PHOTOS_USER_PROPERTY_INIT_URI);
    auto ret = dataShareHelper->UpdateEx(uri, predicates, valuesBucket);
    dataShareHelper->Release();
    CHECK_AND_RETURN_RET_LOG(ret.first == DataShare::E_OK, ret.first,
        "notify photos init failed, err: %{public}d", ret.first);
    MEDIA_DEBUG_LOG("notify photos init completed, ret: %{public}d, second: %{public}d", ret.first, ret.second);
    return E_OK;
}

int32_t SettingsDataManager::QueryParamInSettingData(const std::string &key, std::string &value)
{
    MediaLibraryTracer tracer;
    tracer.Start("SettingsDataManager::QueryParamInSettingData::" + key);

    MediaSettingDataHelper dataShareHelper;
    CHECK_AND_RETURN_RET_LOG(dataShareHelper, E_DB_FAIL, "failed to init dataShareHelper");

    MEDIA_DEBUG_LOG("Query key: %{public}s", key.c_str());
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo("KEYWORD", key);
    std::vector<std::string> columns = {"value"};
    int32_t userId = GetUserId();
    std::string queryUri = "datashare:///com.ohos.settingsdata/entry/settingsdata/USER_SETTINGSDATA_" +
                           std::to_string(userId) + "?Proxy=true&key=" + key;
    Uri uri(queryUri);

    std::shared_ptr<DataShare::DataShareResultSet> resultSet = dataShareHelper->Query(uri, predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_DB_FAIL, "resultSet == nullptr");
    auto ret = resultSet->GoToFirstRow();
    CHECK_AND_RETURN_RET_LOG(ret == DataShare::E_OK, ret, "GoToFirstRow failed, err: %{public}d", ret);

    int32_t columnIndex = 0;
    ret = resultSet->GetColumnIndex("value", columnIndex);
    CHECK_AND_RETURN_RET_LOG(ret == DataShare::E_OK, ret, "GetColumnIndex failed, err: %{public}d", ret);
    ret = resultSet->GetString(columnIndex, value);
    CHECK_AND_RETURN_RET_LOG(ret == DataShare::E_OK, ret, "GetString failed, err: %{public}d", ret);
    MEDIA_DEBUG_LOG(
        "QueryParamInSettingData completed, key: %{public}s, value: %{public}s", key.c_str(), value.c_str());
    return E_OK;
}

bool SettingsDataManager::GetHdcDeviceId(std::string& deviceId)
{
    #ifdef MEDIA_LIBRARY_MEMOSPACE_SERVICE_SUPPORT
        int hdcRet = Memospace::GetHdcDeviceId(deviceId);
        if (hdcRet != 0) {
            MEDIA_ERR_LOG("fail to get real device Id, ret:%{public}d", hdcRet);
            deviceId = "";
            return false;
        }
        MEDIA_INFO_LOG("get hdc deviceId: %{public}s", deviceId.c_str());
        return true;
    #else
        MEDIA_WARN_LOG("hdc device is not supprted");
        deviceId = "";
        return false;
    #endif
}

static AlbumUploadSwitchStatus StringToAlbumUploadSwitchStatus(const std::string& value)
{
    static const std::unordered_map<std::string, AlbumUploadSwitchStatus> STRING_ALBUM_UPLOAD_MAP = {
        { std::to_string(static_cast<int>(AlbumUploadSwitchStatus::CLOSE)), AlbumUploadSwitchStatus::CLOSE },
        { std::to_string(static_cast<int>(AlbumUploadSwitchStatus::OPEN)), AlbumUploadSwitchStatus::OPEN },
    };
    CHECK_AND_RETURN_RET_LOG(STRING_ALBUM_UPLOAD_MAP.count(value), AlbumUploadSwitchStatus::NONE,
        "invalid AlbumUploadSwitchStatus: %{public}s", value.c_str());
    return STRING_ALBUM_UPLOAD_MAP.at(value);
}

int32_t SettingsDataManager::UpdateParamInSettingData(const std::string &key, const std::string &value)
{
    MediaSettingDataHelper dataShareHelper;
    if (!dataShareHelper) {
        return E_DB_FAIL;
    }

    MEDIA_INFO_LOG("Update key: %{public}s", key.c_str());
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo("KEYWORD", key);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put("VALUE", value); // static_cast<int32_t>(value)

    int32_t userId = GetUserId();
    std::string updateUri = "datashare:///com.ohos.settingsdata/entry/settingsdata/USER_SETTINGSDATA_" +
                            std::to_string(userId) + "?Proxy=true&key=" + key;
    Uri uri(updateUri);
    auto ret = dataShareHelper->UpdateEx(uri, predicates, valuesBucket);
    CHECK_AND_RETURN_RET_LOG(ret.first == DataShare::E_OK, ret.first, "update failed, err: %{public}d", ret.first);
    return ret.first;
}

int32_t SettingsDataManager::InsertParamInSettingData(const std::string &key, const std::string &value)
{
    MediaSettingDataHelper dataShareHelper;
    if (!dataShareHelper) {
        return E_DB_FAIL;
    }

    MEDIA_INFO_LOG("Insert key: %{public}s", key.c_str());
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put("KEYWORD", key);
    valuesBucket.Put("VALUE", value); // static_cast<int32_t>(value)

    int32_t userId = GetUserId();
    std::string insertUri = "datashare:///com.ohos.settingsdata/entry/settingsdata/USER_SETTINGSDATA_" +
                            std::to_string(userId) + "?Proxy=true&key=" + key;
    Uri uri(insertUri);
    std::string result;
    auto ret = dataShareHelper->InsertEx(uri, valuesBucket);
    CHECK_AND_RETURN_RET_LOG(ret.first == DataShare::E_OK, ret.first, "insert failed, return: %{public}d", ret.first);
    return ret.first;
}

int32_t SettingsDataManager::UpdateOrInsertAllPhotosAlbumUpload()
{
    AlbumUploadSwitchStatus ret = GetAllAlbumUploadStatus();
    if (ret != AlbumUploadSwitchStatus::NONE) {
        int32_t updateRet = UpdateParamInSettingData(ALL_PHOTOS_ALBUM_UPLOAD_USER, ALL_PHOTOS_ALBUM_UPLOAD_OFF);
        if (updateRet == E_OK) {
            return updateRet;
        }
        MEDIA_WARN_LOG("update album upload user key failed, fallback to device key, ret: %{public}d", updateRet);
        return UpdateParamInDeviceSettingData(ALL_PHOTOS_ALBUM_UPLOAD, ALL_PHOTOS_ALBUM_UPLOAD_OFF);
    }
    int32_t insertRet = InsertParamInSettingData(ALL_PHOTOS_ALBUM_UPLOAD_USER, ALL_PHOTOS_ALBUM_UPLOAD_OFF);
    if (insertRet == E_OK) {
        return insertRet;
    }
    MEDIA_WARN_LOG("insert album upload user key failed, fallback to device key, ret: %{public}d", insertRet);
    return InsertParamInDeviceSettingData(ALL_PHOTOS_ALBUM_UPLOAD, ALL_PHOTOS_ALBUM_UPLOAD_OFF);
}

int32_t SettingsDataManager::UpdateOrInsertCloneSearchStatus()
{
    std::string values;
    auto ret = QueryParamInSettingData(CLONE_SEARCH_STATUS, values);
    if (ret != E_OK) {
        return InsertParamInSettingData(CLONE_SEARCH_STATUS, CLONE_SEARCH_INDEXING);
    }
    return UpdateParamInSettingData(CLONE_SEARCH_STATUS, CLONE_SEARCH_INDEXING);
}

void SettingsDataManager::ComfirmUploadStatus()
{
    // check has second comfirmed
    std::string value;
    QueryParamInSettingData(PHOTOS_ALL_ALBUM_UPLOAD_COMFIRMED, value);
    AlbumUploadSwitchStatus comfirmStatus = StringToAlbumUploadSwitchStatus(value);
    if (comfirmStatus == AlbumUploadSwitchStatus::OPEN) {
        return;
    }
    AlbumUploadSwitchStatus uploadStatus = GetAllAlbumUploadStatus();
    // swtich upload status
    if (uploadStatus == AlbumUploadSwitchStatus::OPEN) {
        MEDIA_INFO_LOG("Off upload switch");
        UpdateOrInsertAllPhotosAlbumUpload();
    }
}

int32_t SettingsDataManager::CheckAndInitUserSettings()
{
    std::string configValue;
    auto ret = QueryParamInSettingData(USER_SETTINGS_ENABLE, configValue);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_IPC_ERR, "Query user settings enable failed, ret: %{public}d", ret);
    MEDIA_DEBUG_LOG("settingsData, ret: %{public}d, key: %{public}s, value: %{public}s",
        ret,
        USER_SETTINGS_ENABLE.c_str(),
        configValue.c_str());

    // 1 means user settings has been initialized, the other means not initialized, need to init by notify.
    const std::string CONFIG_EXISTS = "1";
    const bool isUserSettingsEnable = (configValue == CONFIG_EXISTS);
    CHECK_AND_EXECUTE(isUserSettingsEnable, NotifyPhotosSyncSwitchInitByUpdate());
    return E_OK;
}

SwitchStatus SettingsDataManager::GetPhotosSyncSwitchUserStatus()
{
    CheckAndInitUserSettings();

    std::string value;
    auto ret = QueryParamInSettingData(PHOTOS_SYNC_SWITCH_USER_KEY, value);
    MEDIA_DEBUG_LOG("settingsData, ret: %{public}d, key: %{public}s, value: %{public}s",
        ret,
        PHOTOS_SYNC_SWITCH_USER_KEY.c_str(),
        value.c_str());
    return StringToSwitchStatus(value);
}

SwitchStatus SettingsDataManager::GetPhotosSyncSwitchStatus()
{
    MediaLibraryTracer tracer;
    tracer.Start("SettingsDataManager::GetPhotosSyncSwitchStatus");

    SwitchStatus userStatus = GetPhotosSyncSwitchUserStatus();
    CHECK_AND_RETURN_RET(userStatus == SwitchStatus::NONE, userStatus);

    return GetPhotosSyncSwitchDeviceStatus();
}

AlbumUploadSwitchStatus SettingsDataManager::GetAllAlbumUploadUserStatus()
{
    CheckAndInitUserSettings();

    std::string value;
    auto ret = QueryParamInSettingData(ALL_PHOTOS_ALBUM_UPLOAD_USER, value);
    MEDIA_DEBUG_LOG("settingsData, ret: %{public}d, key: %{public}s, value: %{public}s",
        ret,
        ALL_PHOTOS_ALBUM_UPLOAD_USER.c_str(),
        value.c_str());
    return StringToAlbumUploadSwitchStatus(value);
}

AlbumUploadSwitchStatus SettingsDataManager::GetAllAlbumUploadDeviceStatus()
{
    std::string value;
    auto ret = QueryParamInDeviceSettingData(ALL_PHOTOS_ALBUM_UPLOAD, value);
    MEDIA_DEBUG_LOG("settingsData, ret: %{public}d, key: %{public}s, value: %{public}s",
        ret,
        ALL_PHOTOS_ALBUM_UPLOAD.c_str(),
        value.c_str());
    return StringToAlbumUploadSwitchStatus(value);
}

AlbumUploadSwitchStatus SettingsDataManager::GetAllAlbumUploadStatus()
{
    MediaLibraryTracer tracer;
    tracer.Start("SettingsDataManager::GetAllAlbumUploadStatus");

    AlbumUploadSwitchStatus userStatus = GetAllAlbumUploadUserStatus();
    CHECK_AND_RETURN_RET(userStatus == AlbumUploadSwitchStatus::NONE, userStatus);

    return GetAllAlbumUploadDeviceStatus();
}
} // namespace OHOS::Media