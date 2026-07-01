/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "VideoRingtoneProcessor"

#include "video_ringtone_processor.h"

#include <string>

#include "media_log.h"
#include "media_file_utils.h"
#include "medialibrary_errno.h"
#include "media_old_photos_column.h"
#include "fetch_result.h"
#include "backup_file_utils.h"
#include "bundle_mgr_proxy.h"
#include "backup_database_utils.h"
#include "system_ability_definition.h"
#include "os_account_manager.h"
#include "result_set_utils.h"
#include "media_app_uri_permission_column.h"
#include "iservice_registry.h"

namespace OHOS {
namespace Media {

static const string SETTINGS_DATA_URI_BASE =
    "datashare:///com.ohos.settingsdata/entry/settingsdata/USER_SETTINGSDATA_";
static const string SETTINGS_DATA_FIELD_KEY = "KEYWORD";
static const string SETTINGS_DATA_FIELD_VAL = "VALUE";
static const string RINGTONE_PATH_KEY_SIM1 = "ringtone_path";
static const string RINGTONE_PATH_KEY_SIM2 = "ringtone2_path";
const std::string SOURCE_PATH_PREFIX = "/storage/emulated/0";
const std::string PHOTO_CLOUD_PATH_URI = "/storage/cloud/files/";
static const std::string VIDEO_EXTENSION_MP4 = "mp4";
static const std::string RINGTONE_LIBRARY_BUNDLE_NAME = "com.ohos.ringtonelibrary.ringtonelibrarydata";
static const int TYPE_PHOTOS = 1;
const int PERMISSION_PERSIST_READ = 1;
constexpr int BASE_USER_RANGE = 200000;
static std::vector<string> SETTINGS_COLUMNS = {SETTINGS_DATA_FIELD_VAL};

VideoRingtoneProcessor::~VideoRingtoneProcessor()
{
    if (dataShareHelper_ != nullptr) {
        dataShareHelper_->Release();
        dataShareHelper_ = nullptr;
    }
}

bool VideoRingtoneProcessor::GetActiveUserId()
{
    userId_ =  static_cast<int32_t>(getuid() / BASE_USER_RANGE);
    MEDIA_INFO_LOG("GetActiveUserId: %{public}d", userId_);
    return true;
}

int32_t VideoRingtoneProcessor::InitDataShareHelper()
{
    MEDIA_INFO_LOG("Initializing DataShareHelper");
    settingsDataUri_ = SETTINGS_DATA_URI_BASE + std::to_string(userId_);

    DataShare::CreateOptions options;
    dataShareHelper_ = DataShare::DataShareHelper::Creator(settingsDataUri_, options);
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr, E_FAIL, "Failed to create DataShareHelper");

    MEDIA_INFO_LOG("DataShareHelper initialized successfully");
    return E_OK;
}

string VideoRingtoneProcessor::QueryMp4RingtonePath(const string& key)
{
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr, "", "dataShareHelper_ is nullptr");
    string path = "";
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(SETTINGS_DATA_FIELD_KEY, key);

    Uri uri(settingsDataUri_);
    auto resultSet = dataShareHelper_->Query(uri, predicates, SETTINGS_COLUMNS);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Query result is null for key: %{public}s", key.c_str());
        if (resultSet != nullptr) {
            resultSet->Close();
        }
        return path;
    }
    string valueResult;
    int32_t columnIndex = 0;
    if (resultSet->GetColumnIndex(SETTINGS_DATA_FIELD_VAL, columnIndex) == NativeRdb::E_OK) {
        resultSet->GetString(columnIndex, valueResult);
    }
    resultSet->Close();
    if (IsMp4VideoFile(valueResult)) {
        path = valueResult;
        MEDIA_INFO_LOG("Found MP4 ringtone for key=%{public}s, path=%{public}s", key.c_str(),
            BackupFileUtils::GarbleFilePath(path, CLONE_RESTORE_ID, SOURCE_PATH_PREFIX).c_str());
    }
    return path;
}

bool VideoRingtoneProcessor::IsMp4VideoFile(const string& path)
{
    string ext = MediaFileUtils::GetExtensionFromPath(path);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return ext == VIDEO_EXTENSION_MP4;
}

int32_t VideoRingtoneProcessor::ConvertOldUriToNewUri(const string& oldUri, string& newUri)
{
    int32_t mediaID = -1;
    newUri = GetUrisByOldUrisInner(oldUri, mediaID);
    MEDIA_INFO_LOG("oldUri: %{public}s, newUri: %{public}s, mediaID=%{public}d",
        BackupFileUtils::GarbleFilePath(oldUri, CLONE_RESTORE_ID, SOURCE_PATH_PREFIX).c_str(),
        BackupFileUtils::GarbleFilePath(newUri, CLONE_RESTORE_ID, PHOTO_CLOUD_PATH_URI).c_str(), mediaID);
    
    if (newUri.empty() || mediaID <= 0) {
        MEDIA_ERR_LOG("Invalid mediaID or newUri, mediaID=%{public}d", mediaID);
        return E_FAIL;
    }

    std::string appId;
    uint32_t tokenId = 0;
    int32_t ret = GetAppIdAndTokenId(RINGTONE_LIBRARY_BUNDLE_NAME, userId_, appId, tokenId);
    if (ret != E_OK || appId.empty() || tokenId == 0) {
        MEDIA_ERR_LOG("Invalid appId or tokenId, appId=%{public}s, tokenId=%{public}u",
            appId.c_str(), tokenId);
        return E_FAIL;
    }

    return SetPermissionForFile(appId, tokenId, mediaID);
}

void VideoRingtoneProcessor::SetVideoFilePermission(const string& oldUri)
{
    CHECK_AND_RETURN_LOG(!oldUri.empty(), "oldUri is empty");
    string newUri;
    int32_t ret = ConvertOldUriToNewUri(oldUri, newUri);
    if (ret != E_OK || newUri.empty()) {
        MEDIA_ERR_LOG("ConvertOldUriToNewUri failed for path=%{public}s",
            BackupFileUtils::GarbleFilePath(oldUri, CLONE_RESTORE_ID, SOURCE_PATH_PREFIX).c_str());
        return;
    }
}

void VideoRingtoneProcessor::ProcessVideoRingtones(std::shared_ptr<NativeRdb::RdbStore> &rdbStore)
{
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("ProcessVideoRingtones Start");
    
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "rdbStore is nullptr");
    rdbStore_ = rdbStore;
    if (!GetActiveUserId()) {
        MEDIA_ERR_LOG("Failed to get active userId");
        return;
    }

    int32_t ret = InitDataShareHelper();
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to initialize DataShareHelper");
        return;
    }
    
    std::vector<std::string> ringtoneKeys = {RINGTONE_PATH_KEY_SIM1, RINGTONE_PATH_KEY_SIM2};
    for (const auto& key : ringtoneKeys) {
        std::string path = QueryMp4RingtonePath(key);
        SetVideoFilePermission(path);
    }

    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("ProcessVideoRingtones End, cost=%{public}" PRId64 "ms",
        endTime - startTime);
}

sptr<AppExecFwk::IBundleMgr> GetSysBundleManager()
{
    auto systemAbilityMgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    CHECK_AND_RETURN_RET_LOG(systemAbilityMgr != nullptr, nullptr, "Failed to get SystemAbilityManager.");

    auto bundleObj = systemAbilityMgr->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    CHECK_AND_RETURN_RET_LOG(bundleObj != nullptr, nullptr, "Remote object is nullptr.");

    auto bundleMgr = iface_cast<AppExecFwk::IBundleMgr>(bundleObj);
    CHECK_AND_RETURN_RET_LOG(bundleMgr != nullptr, nullptr, "Failed to iface_cast");
    return bundleMgr;
}

int32_t VideoRingtoneProcessor::GetAppIdAndTokenId(const string &bundleName, int32_t userId,
    string &appId, uint32_t &tokenId)
{
    auto bms = GetSysBundleManager();
    CHECK_AND_RETURN_RET_LOG(bms != nullptr, E_FAIL, "Failed to get bundle manager");

    AppExecFwk::BundleInfo bundleInfo;
    int32_t flags = static_cast<int32_t>(AppExecFwk::GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_APPLICATION);
    auto ret = bms->GetBundleInfoV9(bundleName, flags, bundleInfo, userId);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_FAIL, "Failed to get bundle info, ret: %{public}d", ret);

    appId = bundleInfo.appId;
    tokenId = bundleInfo.applicationInfo.accessTokenId;
    MEDIA_INFO_LOG("GetAppIdAndTokenId, bundleName: %{public}s, appId: %{public}s, tokenId: %{public}u",
        bundleName.c_str(), appId.c_str(), tokenId);
    return E_OK;
}

string VideoRingtoneProcessor::GetUrisByOldUrisInner(const string &oldUris, int32_t &mediaId)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore_ != nullptr, "", "rdbStore is nullptr");
    NativeRdb::AbsRdbPredicates predicates(TabOldPhotosColumn::OLD_PHOTOS_TABLE);
    predicates.EqualTo(TabOldPhotosColumn::MEDIA_OLD_FILE_PATH, oldUris);
    std::vector<std::string> columns = {TabOldPhotosColumn::MEDIA_ID, TabOldPhotosColumn::MEDIA_FILE_PATH};
    auto resultSet = rdbStore_->Query(predicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Query matched data fails for oldUris: %{private}s", oldUris.c_str());
        if (resultSet != nullptr) {
            resultSet->Close();
        }
        return "";
    }

    auto mediaIdOpt = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, TabOldPhotosColumn::MEDIA_ID);
    auto newUrisOpt = BackupDatabaseUtils::GetOptionalValue<string>(resultSet, TabOldPhotosColumn::MEDIA_FILE_PATH);
    if (!newUrisOpt.has_value()) {
        MEDIA_ERR_LOG("newUrisOpt is empty");
        resultSet->Close();
        return "";
    }
    mediaId = mediaIdOpt.value_or(0);
    string newUris = *newUrisOpt;
    resultSet->Close();
    return newUris;
}

int32_t VideoRingtoneProcessor::SetPermissionForFile(const std::string& appId, uint32_t tokenId, int32_t mediaId)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore_ != nullptr, E_FAIL, "rdbStore is nullptr");

    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutString(AppUriPermissionColumn::APP_ID, appId);
    valuesBucket.PutLong(AppUriPermissionColumn::TARGET_TOKENID, static_cast<int64_t>(tokenId));
    valuesBucket.PutLong(AppUriPermissionColumn::SOURCE_TOKENID, static_cast<int64_t>(tokenId));
    valuesBucket.PutInt(AppUriPermissionColumn::FILE_ID, mediaId);
    valuesBucket.PutInt(AppUriPermissionColumn::URI_TYPE, TYPE_PHOTOS);
    valuesBucket.PutInt(AppUriPermissionColumn::PERMISSION_TYPE, PERMISSION_PERSIST_READ);
    valuesBucket.PutLong(AppUriPermissionColumn::DATE_MODIFIED, MediaFileUtils::UTCTimeMilliSeconds());

    int64_t outRowId = 0;
    std::vector<NativeRdb::ValuesBucket> values = {valuesBucket};
    auto ret = rdbStore_->BatchInsert(outRowId, AppUriPermissionColumn::APP_URI_PERMISSION_TABLE, values);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("BatchInsert failed, ret: %{public}d", ret);
        return ret;
    }

    MEDIA_INFO_LOG("SetPermissionForFile success, tokenId=%{private}u, mediaId=%{public}d", tokenId, mediaId);
    return E_OK;
}
} // namespace Media
} // namespace OHOS
