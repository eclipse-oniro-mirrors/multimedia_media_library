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
#define MLOG_TAG "Media_Operation"

#include "photo_album_upload_status_operation.h"

#include "media_log.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "photo_album_column.h"
#include "result_set_utils.h"
#include "settings_data_manager.h"
#include "parameters.h"
#include "preferences.h"
#include "preferences_helper.h"

using namespace std;
using namespace OHOS::NativeRdb;

namespace OHOS::Media {
static const std::string ABILITY_ENABLE_XML = "/data/storage/el2/base/preferences/ability_enable.xml";
const std::string HISTORY_UPLOAD_ALBUM_ENABLE = "history_upload_album_enable";
const std::string PHOTO_UPLOAD_ALBUM_ENABLE = "const.photo.upload.album.enable";
const static std::string SQL_QUERY_ALBUM_ALL_UPLOAD_STATUS = "\
    SELECT \
        CASE \
            WHEN NOT EXISTS (SELECT 1 FROM PhotoAlbum WHERE album_type IN (0, 2048) AND dirty <> 4) \
                THEN 0 \
            WHEN EXISTS \
                (SELECT 1 FROM PhotoAlbum WHERE album_type IN (0, 2048) AND dirty <> 4 AND upload_status = 0) \
                THEN 0 \
            ELSE 1 \
        END AS upload_status;";

int32_t PhotoAlbumUploadStatusOperation::GetAlbumUploadStatus()
{
    AlbumUploadSwitchStatus ret = SettingsDataManager::GetAllAlbumUploadStatus();
    if (ret != AlbumUploadSwitchStatus::NONE) {
        MEDIA_INFO_LOG("GetAlbumUploadStatus %{public}d", static_cast<int32_t>(ret));
        return static_cast<int32_t>(ret);
    }
    return static_cast<int32_t>(IsAllAlbumUploadOnInDb());
}

std::string PhotoAlbumUploadStatusOperation::ToLower(const std::string &str)
{
    std::string lowerStr;
    std::transform(
        str.begin(), str.end(), std::back_inserter(lowerStr), [](unsigned char c) { return std::tolower(c); });
    return lowerStr;
}

int32_t PhotoAlbumUploadStatusOperation::GetAlbumUploadStatusWithLpath(const std::string lpath)
{
    constexpr int32_t UPLOAD_STATUS_ON = 1;
    const std::string lpathLower = ToLower(lpath);
    if (lpathLower == ToLower(PhotoAlbumColumns::LPATH_CAMERA) ||
        lpathLower == ToLower(PhotoAlbumColumns::LPATH_SCREENSHOT) ||
        lpathLower == ToLower(PhotoAlbumColumns::LPATH_SCREENRECORD)) {
        return UPLOAD_STATUS_ON;
    }
    return GetAlbumUploadStatus();
}

bool PhotoAlbumUploadStatusOperation::IsAllAlbumUploadOnInDb()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "Failed to get rdbStore");
    std::shared_ptr<ResultSet> resultSet = rdbStore->QuerySql(SQL_QUERY_ALBUM_ALL_UPLOAD_STATUS);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "queryResultSet is null!");
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == E_OK, false, "Failed to GoToFirstRow");
    int32_t uploadStatus = GetInt32Val(PhotoAlbumColumns::UPLOAD_STATUS, resultSet);
    resultSet->Close();
    return uploadStatus == 1;
}

bool PhotoAlbumUploadStatusOperation::IsSupportUploadStatus()
{
    // 读不到时默认图库支持指定相册上云版本
    bool isSupport = system::GetBoolParameter(PHOTO_UPLOAD_ALBUM_ENABLE, true);
    MEDIA_INFO_LOG("IsSupportUploadStatus: %{public}d", isSupport);
    return isSupport;
}

int32_t PhotoAlbumUploadStatusOperation::JudgeUploadAlbumEnable()
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(ABILITY_ENABLE_XML, errCode);
    CHECK_AND_RETURN_RET_LOG(prefs, E_ERR, "Get preferences error: %{public}d", errCode);
    int32_t historyEnable = prefs->GetInt(HISTORY_UPLOAD_ALBUM_ENABLE, -1);
    MEDIA_INFO_LOG("historyUploadAlbumEnable: %{public}d", historyEnable);
    if (historyEnable == static_cast<int32_t>(EnableUploadStatus::DEFAULT) && !IsSupportUploadStatus()) {
        prefs->PutInt(HISTORY_UPLOAD_ALBUM_ENABLE, static_cast<int32_t>(EnableUploadStatus::OFF));
        prefs->FlushSync();
    }
    if (historyEnable == static_cast<int32_t>(EnableUploadStatus::OFF) && IsSupportUploadStatus()) {
        prefs->PutInt(HISTORY_UPLOAD_ALBUM_ENABLE, static_cast<int32_t>(EnableUploadStatus::ON));
        prefs->FlushSync();
        EnableUploadAlbumInDb();
    }
    return E_OK;
}

int32_t PhotoAlbumUploadStatusOperation::EnableUploadAlbumInDb()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "Failed to get rdbStore");
    std::string uploadSql = "UPDATE PhotoAlbum SET upload_status = 1 WHERE album_type IN (0, 2048)";
    int32_t ret = rdbStore->ExecuteSql(uploadSql);
    MEDIA_INFO_LOG("EnableUploadAlbumInDb, ret: %{public}d", ret);
    return ret;
}
}  // namespace OHOS::Media