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
#define MLOG_TAG "MediaLibraryFileInterwork"

#include "media_fileinterwork_util.h"

#include <filesystem>
#include <fstream>
#include <iostream>

#include "directory_ex.h"
#include "media_column.h"
#include "media_fileinterwork_column.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_rdbstore.h"
#include "media_file_utils.h"
#include "userfile_manager_types.h"
#include "preferences_helper.h"
#include "rdb_predicates.h"
#include "rdb_sql_utils.h"
#include "values_bucket.h"
#include "photo_album_column.h"
#include "settings_data_manager.h"
#include "dfx_anco_manager.h"
#include "dfx_reporter.h"

using namespace std;
namespace OHOS::Media {
namespace fs = std::filesystem;

constexpr const char* TASK_PROGRESS_XML = "/data/storage/el2/base/preferences/task_progress.xml";
constexpr const char* FILE_PROCESS_STATUS_KEY = "file_process_status";

const std::string FILE_LPATH_PREFIX = "/FromDocs";
const std::string FILE_ROOT_LPATH = "/FromDocs/";
const std::string FILE_ROOT_ALBUM = "根目录";

const std::string PHOTOS_ALL_ALBUM_UPLOAD_COMFIRMED = "photos_all_album_upload_comfirmed ";

constexpr int32_t TASK_STATUS_IDLE = 0;

int32_t MediaFileInterworkUtil::GetFileAlbumLPath(const string &path, string &lPath)
{
    std::error_code errorCode;
    fs::path canonicalPath = fs::canonical(path, errorCode);
    CHECK_AND_RETURN_RET_LOG(!errorCode, E_ERR, "Failed to canonicalize path");
    string realPath = canonicalPath.string();
    CHECK_AND_RETURN_RET_LOG(realPath.compare(0, MediaFileInterworkColumn::FILE_ROOT_DIR.size(),
        MediaFileInterworkColumn::FILE_ROOT_DIR) == 0, E_ERR, "invalid file path");
    size_t startPos = realPath.find(MediaFileInterworkColumn::FILE_ROOT_DIR);
    if (startPos == std::string::npos) {
        MEDIA_ERR_LOG("invalid file path");
        return E_ERR;
    }
    lPath = FILE_LPATH_PREFIX + realPath.substr(startPos + MediaFileInterworkColumn::FILE_ROOT_DIR.length());
    if (realPath == MediaFileInterworkColumn::FILE_ROOT_DIR) {
        lPath = FILE_ROOT_LPATH;
    }
    return E_OK;
}

std::string MediaFileInterworkUtil::GetLowerString(const string &str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

int32_t MediaFileInterworkUtil::InsertOrUpdateAlbum(const std::string &albumPath, int32_t &albumId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "RdbStore is nullptr");

    std::string albumName = MediaFileUtils::GetFileName(albumPath);
    std::string albumLPath = "";
    CHECK_AND_RETURN_RET_LOG(MediaFileInterworkUtil::GetFileAlbumLPath(albumPath, albumLPath) == E_OK,
        E_ERR, "invalid albumPath");
    if (albumPath == MediaFileInterworkColumn::FILE_ROOT_DIR) {
        albumName = FILE_ROOT_ALBUM;
    }
    std::string lowerLPath = GetLowerString(albumLPath);
    string querySql = "SELECT ALBUM_ID FROM PhotoAlbum WHERE LOWER(lpath) = ?";
    vector<string> bindArgs;
    bindArgs.push_back(lowerLPath);
    auto resultSet = rdbStore->QuerySql(querySql, bindArgs);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "resultSet is nullptr");
    if (resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        int32_t id = 0;
        if (resultSet->GetInt(0, id) == NativeRdb::E_OK) {
            albumId = id;
        }
        resultSet->Close();
        MEDIA_INFO_LOG("query album: %{public}s with id: %{public}d", albumName.c_str(), albumId);
        return E_OK;
    } else {
        resultSet->Close();
    }
    NativeRdb::ValuesBucket values;
    values.PutString(PhotoAlbumColumns::ALBUM_NAME, albumName);
    values.PutInt(PhotoAlbumColumns::ALBUM_TYPE, PhotoAlbumType::SOURCE);
    values.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILE_MANAGER);
    values.PutString(PhotoAlbumColumns::ALBUM_LPATH, albumLPath);
    
    int64_t rowId = 0;
    int32_t ret = rdbStore->Insert(rowId, PhotoAlbumColumns::TABLE, values);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Insert album failed for: %{public}s", albumPath.c_str());
        return E_HAS_DB_ERROR;
    }
    
    albumId = static_cast<int32_t>(rowId);
    SettingsDataManager::ComfirmUploadStatus();
    MEDIA_INFO_LOG("Created album: %{public}s with id: %{public}d", albumName.c_str(), albumId);
    return E_OK;
}

int32_t MediaFileInterworkUtil::GetScannerTaskStatus()
{
    int32_t errCode = 0;
    std::shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(TASK_PROGRESS_XML, errCode);
    
    if (errCode != E_OK || prefs == nullptr) {
        MEDIA_ERR_LOG("Failed to get preferences, errCode = %{public}d", errCode);
        return TASK_STATUS_IDLE;
    }

    int32_t status = prefs->GetInt(FILE_PROCESS_STATUS_KEY, TASK_STATUS_IDLE);
    MEDIA_INFO_LOG("Current task status: %{public}d", status);
    return status;
}

int32_t MediaFileInterworkUtil::SetScannerTaskStatus(int32_t status)
{
    int32_t errCode = 0;
    std::shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(TASK_PROGRESS_XML, errCode);
    
    if (errCode != E_OK || prefs == nullptr) {
        MEDIA_ERR_LOG("Failed to get preferences, errCode = %{public}d", errCode);
        return E_ERR;
    }

    prefs->PutInt(FILE_PROCESS_STATUS_KEY, status);
    prefs->Flush();
    MEDIA_INFO_LOG("Set task status to: %{public}d", status);
    return E_OK;
}

int32_t MediaFileInterworkUtil::SetLoadFirstTime()
{
    int32_t errCode = 0;
    std::shared_ptr<NativePreferences::Preferences> prefs =
    NativePreferences::PreferencesHelper::GetPreferences(DFX_COMMON_XML, errCode);

    if (errCode != E_OK || prefs == nullptr) {
    MEDIA_ERR_LOG("Failed to get preferences, errCode = %{public}d", errCode);
    return E_ERR;
    }
    int32_t loadType = prefs->GetInt(SCAN_FILEMANAGER_LOAD_TYPE, 0);
    if (loadType != static_cast<int32_t>(LoadType::FILEMANAGER_CLONE_FIRST_LOAD)) {
        prefs->PutInt(SCAN_FILEMANAGER_LOAD_TYPE, LoadType::FILEMANAGER_FIRST_LOAD);
        prefs->PutBool(IS_INVENTORY_LOADING, true);
    }
    int64_t startTime = prefs->GetLong(SCAN_FM_START_TIME, 0);
    if (startTime == 0) {
    int64_t curTime = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("fileManager first load, set first load time is:%{public}" PRId64, curTime);
    prefs->PutLong(SCAN_FM_START_TIME, curTime);
    }

    prefs->FlushSync();
    return E_OK;
}

int32_t MediaFileInterworkUtil::ReportFileManagerFirstLoad()
{
    int32_t errCode = 0;
    std::shared_ptr<NativePreferences::Preferences> prefs =
    NativePreferences::PreferencesHelper::GetPreferences(DFX_COMMON_XML, errCode);

    if (errCode != E_OK || prefs == nullptr) {
    MEDIA_ERR_LOG("Failed to get preferences, errCode = %{public}d", errCode);
    return E_ERR;
    }

    prefs->PutLong(SCAN_FM_END_TIME, MediaFileUtils::UTCTimeMilliSeconds());
    prefs->FlushSync();
    AncoDfxManager::GetInstance().ReportFileManagerFirstLoad();
    return E_OK;
}

int32_t MediaFileInterworkUtil::AddImageAndVideoCount(int32_t imageCount, int32_t videoCount)
{
    int32_t errCode = 0;
    std::shared_ptr<NativePreferences::Preferences> prefs =
    NativePreferences::PreferencesHelper::GetPreferences(DFX_COMMON_XML, errCode);

    if (errCode != E_OK || prefs == nullptr) {
    MEDIA_ERR_LOG("Failed to get preferences, errCode = %{public}d", errCode);
    return E_ERR;
    }

    int32_t curImageCount = prefs->GetInt(SCAN_FM_IMAGE_COUNT, 0);
    prefs->PutInt(SCAN_FM_IMAGE_COUNT, curImageCount + imageCount);
    int32_t curVideoCount = prefs->GetInt(SCAN_FM_VIDEO_COUNT, 0);
    prefs->PutInt(SCAN_FM_VIDEO_COUNT, curVideoCount + videoCount);
    prefs->FlushSync();
    return E_OK;
}

int32_t MediaFileInterworkUtil::AddAlbumCount(int32_t albumCount)
{
    int32_t errCode = 0;
    std::shared_ptr<NativePreferences::Preferences> prefs =
    NativePreferences::PreferencesHelper::GetPreferences(DFX_COMMON_XML, errCode);

    if (errCode != E_OK || prefs == nullptr) {
    MEDIA_ERR_LOG("Failed to get preferences, errCode = %{public}d", errCode);
    return E_ERR;
    }

    int32_t curAlbumCount = prefs->GetInt(SCAN_FM_ALBUM_COUNT, 0);
    prefs->PutInt(SCAN_FM_ALBUM_COUNT, curAlbumCount + albumCount);
    prefs->FlushSync();
    return E_OK;
}
}