/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "Media_Background"

#include "media_clean_all_dirty_files_task.h"

#include "dfx_utils.h"
#include "media_assets_service.h"
#include "media_file_utils.h"
#include "media_path_utils.h"
#include "media_log.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_bundle_manager.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "medialibrary_subscriber.h"
#include <nlohmann/json.hpp>
#include "preferences.h"
#include "preferences_helper.h"
#include "photo_file_utils.h"
#include "userfile_manager_types.h"
#include "rdb_predicates.h"
#include "result_set_utils.h"
#include "submit_cache_dto.h"
#include "thumbnail_const.h"
#include "thumbnail_service.h"
#include "mimetype_utils.h"

using namespace OHOS::NativeRdb;

namespace OHOS::Media::Background {

// LCOV_EXCL_START

static const int32_t prefsNullErrCode = -1;
static const int32_t scanBeginCode = 0;
static const int32_t scanDoneCode = -2;

const std::string ROOT_MEDIA_ORG_DIR = "/storage/cloud/files/Photo/";
const std::string ROOT_MEDIA_THUMBS_DIR = "/storage/cloud/files/.thumbs/Photo/";
const std::string ROOT_MEDIA_EDIT_DIR = "/storage/cloud/files/.editData/Photo/";

const std::string ROOT_MEDIA_LOCAL_ORG_DIR = "/storage/media/local/files/Photo/";
const std::string ROOT_MEDIA_LOCAL_THUMBS_DIR = "/storage/media/local/files/.thumbs/Photo/";
const std::string ROOT_MEDIA_LOCAL_EDIT_DIR = "/storage/media/local/files/.editData/Photo/";

const std::string THM_FILE_NAME = "THM.jpg";
const std::string LCD_FILE_NAME = "LCD.jpg";
const std::string THM_ASTC_FILE_NAME = "THM_ASTC.astc";

const std::string EDITDATA_FILE_NAME = "editdata";
const std::string EDITDATA_CAMERA_FILE_NAME = "editdata_camera";
const std::string EDITDATA_TRANSCODE_FILE_NAME = "transcode";
const std::string EXTRADATA_FILE_NAME = "extraData";
const std::string SOURCE_FILE_PREFIX_NAME = "source";
const std::string TEMP_FILE_PREFIX_NAME = "temp_";
const std::string TEMP_FILE_SUBFIX_NAME = "_tmp.";
const std::string SOURCE_FILE_VIDEO_NAME = "source.mp4";
const std::string MOVING_PHOTO_SUFFIX = ".mp4";

const std::string SLASH_STR = "/";
const std::string EXECUTE_TIME_STR = "executeTime";
const std::string START_FILE_ID_STR = "startFileId";
const std::string START_BUCKET_NUMBER_STR = "startBucketNumber";
const std::string QUERY_FILE_ID_STR = "q_id";

constexpr int64_t THREE_HOURS = 3 * 60 * 60;
constexpr int64_t FOUR_WEEK = 4 * 7 * 24 * 60 * 60;
constexpr int64_t THREE_DAY_MS = 72 * 60 * 60 * 1000;
constexpr int32_t CACHE_BATCH_SIZE = 30;
const std::string SPECIAL_EDIT_COMPATIBLE_FORMAT = "com.hmos.photos";
const std::string SPECIAL_EDIT_FORMAT_VERSION = "1.0";
const std::string SPECIAL_EDIT_EDIT_DATA = "{}";
const std::string SPECIAL_EDIT_APP_ID = "com.hmos.photos";
const string JPG_MIME_TYPE_FORMAT = "image/jpeg";

static bool Starts_With(const std::string& str, const std::string& prefix)
{
    return str.rfind(prefix, 0) == 0;  // 从位置 0 开始查找
}

static bool Contain_With(const std::string& str, const std::string& subfix)
{
    return str.find(subfix) != std::string::npos;
}

static std::vector<int32_t> ConvertBucketNameVector(const std::vector<std::string> &vecStr)
{
    std::vector<int32_t> vecInt;
    for (const auto& s : vecStr) {
        if (all_of(s.begin(), s.end(), ::isdigit)) {
            size_t processCount = 0;
            int32_t val = std::stoi(s, &processCount);
            if (processCount == s.length()) {
                vecInt.push_back(val);
            }
        } else {
            continue;
        }
    }
    return vecInt;
}

bool MediaCleanAllDirtyFilesTask::Accept()
{
    return MedialibrarySubscriber::IsCurrentStatusOn();
}

void MediaCleanAllDirtyFilesTask::Execute()
{
    if (!this->Accept()) { // 过程中中断
        MEDIA_ERR_LOG("MediaCleanAllDirtyFilesTask Interrupt");
        return;
    }
    {
        std::unique_lock<std::mutex> taskLock(taskRunningMutex_, std::defer_lock);
        CHECK_AND_RETURN_WARN_LOG(taskLock.try_lock(), "MediaCleanAllDirtyFilesTask is running");
    }
    std::thread([this] {
        std::unique_lock<std::mutex> taskLock(taskRunningMutex_, std::defer_lock);
        CHECK_AND_RETURN_WARN_LOG(taskLock.try_lock(), "MediaCleanAllDirtyFilesTask thread is running");
        this->HandleMediaAllDirtyFiles();
    }).detach();

    return;
}

void MediaCleanAllDirtyFilesTask::SetBatchExecuteTime()
{
    int32_t errCode = 0;
    std::shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(CLEAN_ALL_DIRTY_FILES_EVENT, errCode);
    MEDIA_DEBUG_LOG("clean_all_dirty_files_events prefs ErrCode: %{public}d", errCode);
    CHECK_AND_RETURN_LOG(prefs != nullptr, "Prefs Is Nullptr");
    int64_t time = MediaFileUtils::UTCTimeSeconds();
    prefs->PutLong(EXECUTE_TIME_STR, time);
    prefs->FlushSync();
    MEDIA_INFO_LOG("ExecuteTime Set To: %{public}" PRId64, time);
}

int64_t MediaCleanAllDirtyFilesTask::GetBatchExecuteTime()
{
    int32_t errCode = 0;
    std::shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(CLEAN_ALL_DIRTY_FILES_EVENT, errCode);
    MEDIA_DEBUG_LOG("clean_all_dirty_files_events prefs ErrCode: %{public}d", errCode);
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, prefsNullErrCode, "Prefs Is Nullptr");
    int64_t defaultVal = 0;
    int64_t time = prefs->GetLong(EXECUTE_TIME_STR, defaultVal);
    MEDIA_INFO_LOG("ExecuteTime Is %{public}" PRId64, time);
    return time;
}

void MediaCleanAllDirtyFilesTask::SetBatchProgressId(int32_t id, const std::string &column)
{
    int32_t errCode = 0;
    std::shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(CLEAN_ALL_DIRTY_FILES_EVENT, errCode);
    MEDIA_DEBUG_LOG("clean_all_dirty_files_events prefs ErrCode: %{public}d", errCode);
    CHECK_AND_RETURN_LOG(prefs != nullptr, "Prefs Is Nullptr");
    prefs->PutInt(column, id);
    prefs->FlushSync();
    MEDIA_INFO_LOG("%{public}s Set To: %{public}d", column.c_str(), id);
}

int32_t MediaCleanAllDirtyFilesTask::GetBatchProgressId(const std::string &column)
{
    int32_t errCode = 0;
    std::shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(CLEAN_ALL_DIRTY_FILES_EVENT, errCode);
    MEDIA_DEBUG_LOG("clean_all_dirty_files_events prefs ErrCode: %{public}d", errCode);
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, prefsNullErrCode, "Prefs Is Nullptr");
    int32_t defaultVal = 0;
    int32_t id = prefs->GetInt(column, defaultVal);
    MEDIA_INFO_LOG("%{public}s Set To: %{public}d", column.c_str(), id);
    return id;
}

int32_t MediaCleanAllDirtyFilesTask::GetMaxFileId()
{
    int32_t maxFileId = 0;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, maxFileId, "RdbStore Is Nullptr");
    std::string QUERY_MAX_FILE_ID = "SELECT MAX(file_id) as q_id FROM Photos";
    std::shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(QUERY_MAX_FILE_ID);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, maxFileId, "Query Result Set Fails");
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Go To First Row Fails");
        resultSet->Close();
        return maxFileId;
    }
    maxFileId = GetInt32Val(QUERY_FILE_ID_STR, resultSet);
    resultSet->Close();
    MEDIA_DEBUG_LOG("GetMaxFileId %{public}d", maxFileId);
    return maxFileId;
}

int32_t MediaCleanAllDirtyFilesTask::GetMinFileId()
{
    int32_t minFileId = 0;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, minFileId, "RdbStore Is Nullptr");
    std::string QUERY_MIN_FILE_ID = "SELECT MIN(file_id) as q_id FROM Photos WHERE " +
        PhotoColumn::PHOTO_FILE_SOURCE_TYPE + " = " +
        std::to_string(static_cast<int32_t>(FileSourceType::MEDIA)) + " AND " +
        PhotoColumn::PHOTO_POSITION  + " = " + to_string(static_cast<int32_t>(PhotoPositionType::LOCAL)) + " AND " +
        MediaColumn::MEDIA_TYPE + " = " + std::to_string(static_cast<int32_t>(MEDIA_TYPE_IMAGE)) + " AND " +
        PhotoColumn::PHOTO_IS_TEMP + " = 0 AND " +
        PhotoColumn::PHOTO_CLEAN_FLAG + " = " + std::to_string(static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN));
    std::shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(QUERY_MIN_FILE_ID);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, minFileId, "Query Result Set Fails");
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Go To First Row Fails");
        resultSet->Close();
        return minFileId;
    }
    minFileId = GetInt32Val(QUERY_FILE_ID_STR, resultSet);
    resultSet->Close();
    MEDIA_DEBUG_LOG("GetMinFileId %{public}d", minFileId);
    return minFileId;
}

int32_t MediaCleanAllDirtyFilesTask::QueryNextId(int32_t startFileId, int32_t &nextFileId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "RdbStore Is Nullptr");
    std::string sql = "SELECT file_id FROM Photos WHERE file_id > "+ to_string(startFileId) + " " +
        PhotoColumn::PHOTO_FILE_SOURCE_TYPE + " = " +
        std::to_string(static_cast<int32_t>(FileSourceType::MEDIA)) + " AND " +
        PhotoColumn::PHOTO_POSITION  + " = " + to_string(static_cast<int32_t>(PhotoPositionType::LOCAL)) + " AND " +
        MediaColumn::MEDIA_TYPE + " = " + std::to_string(static_cast<int32_t>(MEDIA_TYPE_IMAGE)) + " AND " +
        PhotoColumn::PHOTO_IS_TEMP + " = 0 AND " +
        PhotoColumn::PHOTO_CLEAN_FLAG + " = " + std::to_string(static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN)) +
        " ORDER BY file_id ASC LIMIT 1";
    // 除纯云图 动图? 除东湖和临时文管文件 clear_flag is_temp 排除掉
    std::shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(sql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RESULT_SET_NULL, "Query Not Match Data Fails");
    if (resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        nextFileId = GetInt32Val(PhotoColumn::MEDIA_ID, resultSet);
        MEDIA_DEBUG_LOG("QueryNextId NextId: %{public}d", nextFileId);
    } else {
        MEDIA_ERR_LOG("Go To First Row Fails StartId: %{public}d", startFileId);
    }
    resultSet->Close();
    return E_OK;
}

bool MediaCleanAllDirtyFilesTask::QueryFileInfos(int32_t startFileId, DirtyFileInfo &dirtyFileInfo)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "RdbStore Is Nullptr");
    std::string sql = "SELECT data, time_pending, date_added, media_type FROM Photos WHERE " +
        PhotoColumn::PHOTO_FILE_SOURCE_TYPE + " = " +
        std::to_string(static_cast<int32_t>(FileSourceType::MEDIA)) + " AND " +
        PhotoColumn::PHOTO_POSITION  + " = " + to_string(static_cast<int32_t>(PhotoPositionType::LOCAL)) + " AND " +
        MediaColumn::MEDIA_ID + " = " + std::to_string(startFileId) + " AND " +
        MediaColumn::MEDIA_TYPE + " = " + std::to_string(static_cast<int32_t>(MEDIA_TYPE_IMAGE)) + " AND " +
        PhotoColumn::PHOTO_IS_TEMP + " = 0 AND " +
        PhotoColumn::PHOTO_CLEAN_FLAG + " = " + std::to_string(static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN));
    // 除纯云图 动图? 除东湖和临时文管文件 隐藏 等需要特殊适配 clear_flag is_temp 排除掉
    std::shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(sql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "Query not match data fails");
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Go To First Row Fails");
        resultSet->Close();
        return false;
    }
    dirtyFileInfo.fileId = startFileId;
    dirtyFileInfo.path = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
    dirtyFileInfo.pending = GetInt32Val(MediaColumn::MEDIA_TIME_PENDING, resultSet);
    dirtyFileInfo.addTime = GetInt64Val(MediaColumn::MEDIA_DATE_ADDED, resultSet);
    dirtyFileInfo.mediaType = GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet);
    dirtyFileInfo.localPath = MediaFileUtils::GetLocalPath(dirtyFileInfo.path);
    MEDIA_DEBUG_LOG("Handle Data %{public}s, Pending %{public}d, Mtype %{public}d, Addtime %{public}" PRId64,
        MediaFileUtils::DesensitizePath(dirtyFileInfo.path).c_str(), dirtyFileInfo.pending, dirtyFileInfo.mediaType,
        dirtyFileInfo.addTime);
    resultSet->Close();
    return true;
}

bool MediaCleanAllDirtyFilesTask::OriginSourceExist(std::string &path)
{
    CHECK_AND_RETURN_RET(!path.empty(), false);
    if (DealWithZeroSizeFile(path)) {
        return false;
    }
    return MediaFileUtils::IsFileExists(path);
}

bool MediaCleanAllDirtyFilesTask::DealWithZeroSizeFile(std::string &path)
{
    size_t size = 0;
    uint64_t currentTime = static_cast<uint64_t>(MediaFileUtils::UTCTimeSeconds());
    uint64_t addTime = currentTime;
    if (MediaFileUtils::GetFileSizeAndTime(path, size, addTime) && size == 0) {
        uint64_t interval = (currentTime > addTime) ? (currentTime - addTime) : 0;
        if (interval > FOUR_WEEK) {
            MEDIA_INFO_LOG("DirtyMediaHandler DealWithZeroSizeFile Interval: %{public}" PRId64,
                interval);
            MediaFileUtils::DeleteFileWithRetry(path);
            MEDIA_WARN_LOG("DirtyMediaHandler DealWithZeroSizeFile File: %{public}s",
                MediaFileUtils::DesensitizePath(path).c_str());
            return true;
        }
    }
    return false;
}

bool MediaCleanAllDirtyFilesTask::Is4WeekAgoFile(std::string &path)
{
    size_t size = 0;
    uint64_t currentTime = static_cast<uint64_t>(MediaFileUtils::UTCTimeSeconds());
    uint64_t addTime = currentTime;
    if (MediaFileUtils::GetFileSizeAndTime(path, size, addTime) && size != 0) {
        uint64_t interval = (currentTime > addTime) ? (currentTime - addTime) : 0;
        if (interval > FOUR_WEEK) {
            MEDIA_INFO_LOG("DirtyMediaHandler DealWithZeroSizeFile Interval: %{public}" PRId64 " File: %{public}s",
                interval, MediaFileUtils::DesensitizePath(path).c_str());
            return true;
        }
    }
    return false;
}

bool MediaCleanAllDirtyFilesTask::IsZeroSizeFile(std::string &path)
{
    size_t size = 0;
    if (MediaFileUtils::GetFileSize(path, size) && size == 0) {
        return true;
    }
    return false;
}

bool MediaCleanAllDirtyFilesTask::ThumbnailSourceExist(std::string &path)
{
    CHECK_AND_RETURN_RET(!path.empty(), false);
    std::string thumbnailPath = GetThumbnailPath(path, THUMBNAIL_THUMB_SUFFIX);
    DealWithZeroSizeFile(thumbnailPath);
    std::string lcdPath = GetThumbnailPath(path, THUMBNAIL_LCD_SUFFIX);
    DealWithZeroSizeFile(lcdPath);

    bool existThumbnail = MediaFileUtils::IsFileExists(thumbnailPath);
    bool existThumbnailLCD = MediaFileUtils::IsFileExists(lcdPath);
    if (!existThumbnail && !existThumbnailLCD) {
        return false;
    }
    return true;
}

int32_t MediaCleanAllDirtyFilesTask::UpdatePendingInfoByPath(int32_t fileId, int64_t modifyTime, int64_t pending)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL,
        "UpdatePendingInfoByPath Failed to get rdbStore.");
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    NativeRdb::ValuesBucket value;
    value.PutLong(PhotoColumn::PHOTO_META_DATE_MODIFIED, modifyTime);
    value.PutLong(PhotoColumn::MEDIA_TIME_PENDING, pending);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, fileId);
    int32_t changedRows = -1;
    int32_t ret = rdbStore->Update(changedRows, value, predicates);
    MEDIA_INFO_LOG("UpdatePendingAndByPath Ret: %{public}d, changedRows %{public}d", ret, changedRows);
    return ret;
}

void MediaCleanAllDirtyFilesTask::DealWithPendingToEffectFile(DirtyFileInfo &dirtyFileInfo)
{
    // pending文件 充电息屏且保存超过72小时 扫描原图转正
    if (dirtyFileInfo.pending != 0) {
        int64_t timeMs = MediaFileUtils::UTCTimeMilliSeconds();
        if (timeMs - dirtyFileInfo.addTime > THREE_DAY_MS) {
            // 转正
            MEDIA_INFO_LOG("DealWithPendingToEffectFile Id %{public}d", dirtyFileInfo.fileId);
            UpdatePendingInfoByPath(dirtyFileInfo.fileId, timeMs, 0L);
        }
    }
}

void MediaCleanAllDirtyFilesTask::HandleBothExistStrategy(DirtyFileInfo &dirtyFileInfo)
{
    // pending文件 充电息屏且保存超过72小时 扫描原图转正 DealWithPendingToEffectFile(dirtyFileInfo);
    MEDIA_INFO_LOG("HandleBothExistStrategy path %{public}s",
        MediaFileUtils::DesensitizePath(dirtyFileInfo.path).c_str());
}

void MediaCleanAllDirtyFilesTask::HandleOriginNotExistStrategy(DirtyFileInfo &dirtyFileInfo)
{
    // date_added距此超72小时使用缩略图填充原图并扫描刷新
    int64_t timeMs = MediaFileUtils::UTCTimeMilliSeconds();
    if (timeMs - dirtyFileInfo.addTime > THREE_DAY_MS) {
        std::string thumbnailPath = GetThumbnailPath(dirtyFileInfo.path, THUMBNAIL_THUMB_SUFFIX);
        bool existThumbnail = MediaFileUtils::IsFileExists(thumbnailPath);
        if (!existThumbnail) {
            std::string lcdPath = GetThumbnailPath(dirtyFileInfo.path, THUMBNAIL_LCD_SUFFIX);
            bool existThumbnailLCD = MediaFileUtils::IsFileExists(lcdPath);
            if (existThumbnailLCD && !MediaFileUtils::IsFileExists(dirtyFileInfo.path)
                && !MediaFileUtils::IsFileExists(dirtyFileInfo.localPath)) {
                MediaFileUtils::CopyFileUtil(lcdPath, dirtyFileInfo.localPath);
                MEDIA_INFO_LOG("DirtyMediaHandler HandleOriginNotExistStrategy Lcd Cp To Org %{public}s to Id"
                    " %{public}d", lcdPath.c_str(), dirtyFileInfo.fileId);
            }
            // SCAN 扫描更新size等column
            MediaAssetsService::GetInstance().ScanExistFileRecord(dirtyFileInfo.fileId, dirtyFileInfo.path);
            return;
        }
        if (!MediaFileUtils::IsFileExists(dirtyFileInfo.path) &&
            !MediaFileUtils::IsFileExists(dirtyFileInfo.localPath)) {
            MediaFileUtils::CopyFileUtil(thumbnailPath, dirtyFileInfo.localPath);
            MEDIA_INFO_LOG("DirtyMediaHandler HandleOriginNotExistStrategy Thumbnail Cp To Org %{public}s to Id"
                " %{public}d", MediaFileUtils::DesensitizePath(thumbnailPath).c_str(), dirtyFileInfo.fileId);
        }
        // SCAN 扫描更新size等column
        MediaAssetsService::GetInstance().ScanExistFileRecord(dirtyFileInfo.fileId, dirtyFileInfo.path);
        return;
    } else {
        MEDIA_WARN_LOG("Thumb Exist Org NotExist WithIn One Day %{public}s",
            MediaFileUtils::DesensitizePath(dirtyFileInfo.path).c_str());
    }
}

void MediaCleanAllDirtyFilesTask::HandleBothNotExistStrategy(DirtyFileInfo &dirtyFileInfo)
{
    // 危险操作 date_added距此超72小时删除元数据
    int64_t timeMs = MediaFileUtils::UTCTimeMilliSeconds();
    if (timeMs - dirtyFileInfo.addTime > THREE_DAY_MS) {
        MEDIA_INFO_LOG("HandleBothNotExistStrategy DeletePermanently FileId %{public}d", dirtyFileInfo.fileId);
        NativeRdb::RdbPredicates deletePhotoPredicates(PhotoColumn::PHOTOS_TABLE);
        deletePhotoPredicates.EqualTo(MediaColumn::MEDIA_ID, to_string(dirtyFileInfo.fileId));
        MediaLibraryAssetOperations::DeletePermanently(deletePhotoPredicates, true);
        return;
    }
}

void MediaCleanAllDirtyFilesTask::HandleOriginExistStrategy(DirtyFileInfo &dirtyFileInfo)
{
    // pending文件，充电息屏且保存超过72小时扫描原图转正 DealWithPendingToEffectFile(dirtyFileInfo);
    MEDIA_INFO_LOG("HandleOriginExistStrategy path %{public}s",
        MediaFileUtils::DesensitizePath(dirtyFileInfo.path).c_str());
}

void MediaCleanAllDirtyFilesTask::HandleSingleRecord(DirtyFileInfo &dirtyFileInfo)
{
    bool existOrigin = OriginSourceExist(dirtyFileInfo.path); // 原图判断
    bool existThumb = ThumbnailSourceExist(dirtyFileInfo.path); // 缩略图判断
    if (existOrigin && existThumb) {
        HandleBothExistStrategy(dirtyFileInfo);
    }
    if (!existOrigin && existThumb) {
        HandleOriginNotExistStrategy(dirtyFileInfo);
    }
    if (!existOrigin && !existThumb) {
        MEDIA_ERR_LOG("HandleBothNotExistStrategy path %{public}s",
            MediaFileUtils::DesensitizePath(dirtyFileInfo.path).c_str());
        // 都不存在 HandleBothNotExistStrategy(dirtyFileInfo);
    }
    if (existOrigin && !existThumb) {
        HandleOriginExistStrategy(dirtyFileInfo);
    }
}

void MediaCleanAllDirtyFilesTask::MoveToNextId(int32_t &startFileId)
{
    int32_t nextFileId = 0;
    if (QueryNextId(startFileId, nextFileId) == E_OK && nextFileId != 0 && startFileId != nextFileId) {
        MEDIA_INFO_LOG("HandleAllDirtyTable MoveToNext Id: %{public}d, Next: %{public}d", startFileId, nextFileId);
        startFileId = nextFileId;
    } else {
        MEDIA_INFO_LOG("HandleAllDirtyTable MoveToNext End Id: %{public}d, Next: %{public}d", startFileId,
            GetMaxFileId() + 1);
        startFileId = GetMaxFileId() + 1; // 拿不到下一个 超过退出
    }
}

void MediaCleanAllDirtyFilesTask::HandleAllDirtyTable(int32_t curStartFileId)
{
    // 处理表
    CHECK_AND_RETURN_INFO_LOG(curStartFileId >= 0, "HandleAllDirtyTable Finished");
    int32_t maxFileId = GetMaxFileId();
    int32_t minFileId = GetMinFileId();
    int32_t startFileId = (curStartFileId == scanBeginCode || curStartFileId < minFileId) ? minFileId : curStartFileId;
    MEDIA_INFO_LOG("HandleAllDirtyTable Start From Id: %{public}d, Min: %{public}d, Max: %{public}d", startFileId,
        minFileId, maxFileId);
    while (startFileId <= GetMaxFileId()) {
        CHECK_AND_RETURN_INFO_LOG(!IsCurrentTaskTimeOut(), "HandleAllDirtyTable Timeout");
        if (!this->Accept()) {
            MEDIA_ERR_LOG("MediaCleanAllDirtyFilesTask Exit");
            SetBatchProgressId(startFileId, START_FILE_ID_STR);
            return;
        }
        MEDIA_DEBUG_LOG("HandleAllDirtyTable Id: %{public}d", startFileId);
        if (ContainsFileIdsCacheSet(startFileId)) {
            MEDIA_INFO_LOG("HandleAllDirtyTable Skip Cache FileId: %{public}d", startFileId);
            MoveToNextId(startFileId);
            continue;
        }
        
        DirtyFileInfo dirtyFileInfo;
        if (QueryFileInfos(startFileId, dirtyFileInfo)) {
            HandleSingleRecord(dirtyFileInfo);
        }
        MoveToNextId(startFileId); // 一定需要拿到下一个 避免死循环
    }
    if (startFileId > maxFileId) { // Archiving
        MEDIA_INFO_LOG("DirtyMediaHandler Table Scan END");
        SetBatchProgressId(scanDoneCode, START_FILE_ID_STR);
        return;
    } else {
        MEDIA_INFO_LOG("DirtyMediaHandler Table Scan Part END FileId: %{public}d", startFileId);
        SetBatchProgressId(startFileId, START_FILE_ID_STR);
    }
}

bool MediaCleanAllDirtyFilesTask::GetFileNameWithSameNameOtherType(const std::string &path,
    const std::string &fileName, std::string &OtherFileName)
{
    string title = MediaFileUtils::GetTitleFromDisplayName(fileName) + DOT_STR;
    std::vector<std::string> fileNameVec;
    MediaFileUtils::GetAllFileNameListUnderPath(path, fileNameVec);
    for (std::string fileNameInside : fileNameVec) {
        if (fileNameInside != fileName && fileNameInside.size() > title.size()
            && fileNameInside.compare(0, title.size(), title) == 0) {
            OtherFileName = fileNameInside;
            return true;
        }
    }
    return false;
}

bool MediaCleanAllDirtyFilesTask::IsMovingPhotosInOrgFolder(int32_t curBucketNum, const std::string &fileName)
{
    // 同文件目录 存在其他类型的同title文件
    std::string originBucketFolder = ROOT_MEDIA_ORG_DIR + std::to_string(curBucketNum);
    MediaType mediaType = MediaFileUtils::GetMediaType(fileName);
    if (mediaType == MediaType::MEDIA_TYPE_VIDEO) {
        std::string OtherFileName;
        if (GetFileNameWithSameNameOtherType(originBucketFolder, fileName, OtherFileName)) {
            MediaType mediaTypeOther = MediaFileUtils::GetMediaType(OtherFileName);
            if (mediaTypeOther == MediaType::MEDIA_TYPE_IMAGE) {
                return true;
            } else { // 拿到了另一个文件 但不是另一个类型
                MEDIA_ERR_LOG("DirtyMediaHandler Invalid Path: %{public}s, File: %{public}s",
                    MediaFileUtils::DesensitizePath(originBucketFolder).c_str(), fileName.c_str());
            }
        }
    }
    if (mediaType == MediaType::MEDIA_TYPE_IMAGE) {
        std::string OtherFileName;
        if (GetFileNameWithSameNameOtherType(originBucketFolder, fileName, OtherFileName)) {
            MediaType mediaTypeOther = MediaFileUtils::GetMediaType(OtherFileName);
            if (mediaTypeOther == MediaType::MEDIA_TYPE_VIDEO) {
                return true;
            } else {
                MEDIA_ERR_LOG("DirtyMediaHandler Invalid Path: %{public}s, File: %{public}s",
                    MediaFileUtils::DesensitizePath(originBucketFolder).c_str(), fileName.c_str());
            }
        }
    }
    return false;
}

bool MediaCleanAllDirtyFilesTask::IsJpgImageFile(const std::string &fileName)
{
    MediaType mediaType = MediaFileUtils::GetMediaType(fileName);
    if (mediaType == MediaType::MEDIA_TYPE_IMAGE) {
        string mimeType = MimeTypeUtils::GetMimeTypeFromExtension(MediaFileUtils::GetExtensionFromPath(fileName));
        if (mimeType == JPG_MIME_TYPE_FORMAT) {
            return true;
        }
    }
    return false;
}

bool MediaCleanAllDirtyFilesTask::IsMovingPhotosInEditFolder(int32_t curBucketNum, const std::string &fileName)
{
    // 同文件目录 存在其他类型的同title文件 source.jpg  source.mp4
    std::string editBucketFolder = ROOT_MEDIA_EDIT_DIR + std::to_string(curBucketNum) +
        SLASH_STR + fileName; // 编辑文件目录名 包含editdata extraData source.jpg~heic source.mp4
    std::string extension = MediaPathUtils::GetExtension(fileName);
    std::string editOriginFile = editBucketFolder + SLASH_STR + SOURCE_FILE_PREFIX_NAME + DOT_STR + extension;
    std::string videoPath = editBucketFolder + SLASH_STR + SOURCE_FILE_VIDEO_NAME;
    MediaType mediaType = MediaFileUtils::GetMediaType(fileName);
    if (mediaType == MediaType::MEDIA_TYPE_IMAGE) {
        std::string OtherFileName;
        if (MediaFileUtils::IsFileExists(videoPath)) {
            MEDIA_INFO_LOG("DirtyMediaHandler Find Video Path: %{public}s",
                MediaFileUtils::DesensitizePath(videoPath).c_str());
            return true;
        } else {
            MEDIA_DEBUG_LOG("DirtyMediaHandler Cannot Find Video Path: %{public}s",
                MediaFileUtils::DesensitizePath(videoPath).c_str());
        }
    }
    return false;
}

bool MediaCleanAllDirtyFilesTask::ExistCloudAssetPathInDB(const std::string &path)
{
    // SELECT EXISTS(SELECT 1 FROM photos WHERE data = '/storage/cloud/files/Photo/1/IMG_X_001.jpg' and
    // (position != 1 or file_source_type != 0 or is_temp = 1 or clean_flag != 0)) AS recordExist
    // 纯云图和文管,临时文件 pending!=0 跳过
    bool recordExist = true; // 默认值true 避免因为数据库问题 多进行处理
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, recordExist, "RdbStore Is Nullptr");
    std::string querySql = "SELECT EXISTS(SELECT 1 FROM photos WHERE data = ? AND (" +
        PhotoColumn::PHOTO_POSITION  + " != " + to_string(static_cast<int32_t>(PhotoPositionType::LOCAL)) + " OR " +
        PhotoColumn::PHOTO_FILE_SOURCE_TYPE  + " != " + std::to_string(static_cast<int32_t>(FileSourceType::MEDIA)) +
        " OR " + PhotoColumn::PHOTO_CLEAN_FLAG + " != " +
        std::to_string(static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN)) +
        " OR " + MediaColumn::MEDIA_TIME_PENDING + " != 0 " +
        " OR " + PhotoColumn::PHOTO_IS_TEMP  + " = 1)" + ") AS recordExist";
    std::vector<NativeRdb::ValueObject> params = { path };
    std::shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(querySql, params);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, recordExist, "Query ExistCloudAssetPathInDB Fails");
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Go To First Row Fails");
        resultSet->Close();
        return true; // 默认要为true 避免异常误判 处理正常图片
    }
    recordExist = GetInt32Val("recordExist", resultSet) == 0 ? false : true;  // 默认要为true 否则可能误判
    resultSet->Close();
    return recordExist;
}

bool MediaCleanAllDirtyFilesTask::ExistPhotoPathInDB(const std::string &path)
{
    // SELECT EXISTS(SELECT 1 FROM photos WHERE data = '/storage/cloud/files/Photo/1/IMG_X_001.jpg') AS recordExist
    bool recordExist = true; // 默认值true 避免因为数据库问题 多进行处理
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, recordExist, "RdbStore Is Nullptr");
    std::string sql = "SELECT EXISTS(SELECT 1 FROM photos WHERE data = ?) AS recordExist";
    std::vector<NativeRdb::ValueObject> params = { path };
    std::shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(sql, params);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, recordExist, "Query ExistPhotoPathInDB Fails");
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Go To First Row Fails");
        resultSet->Close();
        return true;
    }
    recordExist = GetInt32Val("recordExist", resultSet) == 0 ? false : true;  // 默认要为true 否则可能误判
    resultSet->Close();
    return recordExist;
}

bool MediaCleanAllDirtyFilesTask::ExistMovingPhotoPathInDB(const std::string &path)
{
    // SELECT EXISTS(SELECT 1 FROM photos WHERE data = '/storage/cloud/files/Photo/1/IMG_X_001.jpg' AND subtype = 3)
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, true, "RdbStore Is Nullptr");
    std::string sql = "SELECT EXISTS(SELECT 1 FROM photos WHERE data = ? AND subtype = " +
        std::to_string(static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)) + ") AS recordExist";
    std::vector<NativeRdb::ValueObject> params = { path };
    std::shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(sql, params);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, true, "Query ExistMovingPhotoPathInDB Fails");
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Go To First Row Fails");
        resultSet->Close();
        return true;
    }
    bool recordExist = GetInt32Val("recordExist", resultSet) == 0 ? false : true;  // 默认要为true 否则可能误判
    resultSet->Close();
    return recordExist;
}

int32_t MediaCleanAllDirtyFilesTask::AddFileToTableWithFixedName(int32_t curBucketNum, const std::string &fileName)
{
    std::string path = ROOT_MEDIA_ORG_DIR + std::to_string(curBucketNum) + SLASH_STR + fileName;
    MEDIA_INFO_LOG("DirtyMediaHandler AddFileToTableWithFixedName Path: %{public}s",
        MediaFileUtils::DesensitizePath(path).c_str());
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsFileExists(path), E_NO_SUCH_FILE,
        "DirtyMediaHandler Photo Path: %{private}s Does Not Exist!", path.c_str());
    std::string extension = MediaPathUtils::GetExtension(fileName);
    MediaType mediaType = MediaFileUtils::GetMediaType(fileName);
    SubmitCacheDto dto;
    dto.isOriginalImageResource = false;
    dto.isWriteGpsAdvanced = false;
    NativeRdb::ValuesBucket values;
    values.Put(CONST_CACHE_FILE_NAME, fileName);
    values.Put(MEDIA_DATA_DB_NAME, fileName);
    values.Put(CONST_ASSET_EXTENTION, extension);
    values.Put(MEDIA_DATA_DB_MEDIA_TYPE, static_cast<int32_t>(mediaType));
    values.Put(CONST_MEDIA_DATA_DB_TITLE, MediaFileUtils::GetTitleFromDisplayName(fileName));
    values.Put(CONST_IMPORT_CACHE_FILE_PATH, path);
    
    dto.values = values;
    int32_t ret = MediaAssetsService::GetInstance().SubmitExistFileDBRecord(dto);
    MEDIA_INFO_LOG("DirtyMediaHandler SubmitExistFileDBRecord Ret: %{public}d", ret);
    return ret;
}

int32_t MediaCleanAllDirtyFilesTask::AddMovingPhotoFileToTableWithFixedName(int32_t curBucketNum,
    const std::string &fileName)
{
    MEDIA_INFO_LOG("DirtyMediaHandler AddMovingPhotoFileToTableWithFixedName: %{public}s", fileName.c_str());
    std::string path = ROOT_MEDIA_ORG_DIR + std::to_string(curBucketNum) + SLASH_STR + fileName;
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsFileExists(path), E_NO_SUCH_FILE,
        "DirtyMediaHandler Path: %{private}s does not exist!", path.c_str());
    std::string extension = MediaPathUtils::GetExtension(fileName);
    MediaType mediaType = MediaFileUtils::GetMediaType(fileName);
    MEDIA_INFO_LOG("DirtyMediaHandler MovingPhoto Path: %{public}s", MediaFileUtils::DesensitizePath(path).c_str());
    SubmitCacheDto dto;
    dto.isOriginalImageResource = false;
    dto.isWriteGpsAdvanced = false;
    NativeRdb::ValuesBucket values;
    values.Put(CONST_CACHE_FILE_NAME, fileName);
    values.Put(MEDIA_DATA_DB_NAME, fileName);
    values.Put(CONST_ASSET_EXTENTION, extension);
    values.Put(MEDIA_DATA_DB_MEDIA_TYPE, static_cast<int32_t>(mediaType));
    values.Put(CONST_MEDIA_DATA_DB_TITLE, MediaFileUtils::GetTitleFromDisplayName(fileName));
    values.Put(CONST_IMPORT_CACHE_FILE_PATH, path);
    std::string videoFileName; // 视频 是否存在
    if (!GetFileNameWithSameNameOtherType(ROOT_MEDIA_ORG_DIR + std::to_string(curBucketNum), fileName,
        videoFileName)) {
        MEDIA_ERR_LOG("DirtyMediaHandler Invalid Recog AS MovingPhoto: %{public}s", fileName.c_str());
        return E_FAIL; // Skip 不存在
    }
    values.Put(CONST_CACHE_MOVING_PHOTO_VIDEO_NAME, videoFileName);
    values.Put(CONST_COMPAT_FILE_SUBTYPE, static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    dto.values = values;
    int32_t ret = MediaAssetsService::GetInstance().SubmitExistFileDBRecord(dto);
    MEDIA_INFO_LOG("DirtyMediaHandler SubmitExistFileDBRecord Ret: %{public}d", ret);
    return ret;
}

bool MediaCleanAllDirtyFilesTask::DealOriginFileAndRecordNotExistPhotos(int32_t curBucketNum,
    const std::string &fileName)
{
    std::string path = ROOT_MEDIA_ORG_DIR + std::to_string(curBucketNum) + SLASH_STR + fileName;
    if (IsMovingPhotosInOrgFolder(curBucketNum, fileName)) { // 动图
        if (!ExistMovingPhotoPathInDB(path)) { // 加类型反查 记录不存在 insert
            AddMovingPhotoFileToTableWithFixedName(curBucketNum, fileName);
            MEDIA_INFO_LOG("DirtyMediaHandler File Insert MovingPhotos Bk:%{public}d, File: %{public}s",
                curBucketNum, fileName.c_str());
            AddToFilesCacheSet(path);
            return true;
        } else {
            MEDIA_ERR_LOG("DirtyMediaHandler Invalid Moving Photo Path: %{public}s",
                MediaFileUtils::DesensitizePath(path).c_str());
            return true;
        }
    }
    // 静图
    AddFileToTableWithFixedName(curBucketNum, fileName);
    AddToFilesCacheSet(path);
    MEDIA_INFO_LOG("DirtyMediaHandler File Insert FileAndRecordNotExist Bk:%{public}d, Path: %{public}s",
        curBucketNum, MediaFileUtils::DesensitizePath(path).c_str());
    return true;
}

bool MediaCleanAllDirtyFilesTask::HandleOriginFileNotExistAddToTable(int32_t curBucketNum, const std::string &fileName)
{
    std::string path = ROOT_MEDIA_ORG_DIR + std::to_string(curBucketNum) + SLASH_STR + fileName;
    // 反查数据库 原图存在 元数据是否存在
    std::string extension = MediaPathUtils::GetExtension(fileName);
    MediaType mediaType = MediaFileUtils::GetMediaType(fileName);
    CHECK_AND_RETURN_RET_LOG(!IsZeroSizeFile(path), true, "OriginFileNotExistAddToTable %{public}s file size is 0",
        MediaFileUtils::DesensitizePath(path).c_str()); // 0kb 跳过
    CHECK_AND_RETURN_RET_LOG(Is4WeekAgoFile(path), true, "OriginFileNotExistAddToTable %{public}s skip file in 4 week",
        MediaFileUtils::DesensitizePath(path).c_str()); // 判断是否4周前的文件 不是的话 不需要恢复逻辑 有temp文件
    CHECK_AND_RETURN_RET_LOG(!Starts_With(fileName, TEMP_FILE_PREFIX_NAME), true,
        "OriginFileNotExistAddToTable %{public}s skip temp file", MediaFileUtils::DesensitizePath(path).c_str());
    CHECK_AND_RETURN_RET_LOG(!Contain_With(fileName, TEMP_FILE_SUBFIX_NAME), true,
        "OriginFileNotExistAddToTable %{public}s skip end temp file", MediaFileUtils::DesensitizePath(path).c_str());
    if (mediaType == MediaType::MEDIA_TYPE_VIDEO) {
        if (!ExistPhotoPathInDB(path)) {
            // 扫描原图填充元数据 元数据不存在 insert 如果是动图 跳过视频处理, 因视频处理在数据库需反查模糊匹配 不精准
            if (IsMovingPhotosInOrgFolder(curBucketNum, fileName)) { // 处理视频时 跳过动图的视频处理
                MEDIA_DEBUG_LOG("DirtyMediaHandler File Skip MovingPhoto Path: %{public}s",
                    MediaFileUtils::DesensitizePath(path).c_str());
                return true;
            }
            // 视频
            AddFileToTableWithFixedName(curBucketNum, fileName);
            MEDIA_INFO_LOG("DirtyMediaHandler File Insert Video FileAndRecordNotExist Bk:%{public}d, Path: %{public}s",
                curBucketNum, MediaFileUtils::DesensitizePath(path).c_str());
            AddToFilesCacheSet(path);
            return true;
        }
        MEDIA_DEBUG_LOG("DirtyMediaHandler File Skip Video Bk:%{public}d, File: %{public}s", curBucketNum,
            fileName.c_str());
        return true;
    }
    if (mediaType == MediaType::MEDIA_TYPE_IMAGE) {
        if (!ExistPhotoPathInDB(path)) { // 扫描原图填充元数据 元数据不存在 insert 动图需要判断
            DealOriginFileAndRecordNotExistPhotos(curBucketNum, fileName);
            return true;
        }
        MEDIA_DEBUG_LOG("DirtyMediaHandler File Skip Bk:%{public}d, File: %{public}s", curBucketNum, fileName.c_str());
    }
    return true;
}

bool MediaCleanAllDirtyFilesTask::ExistEditFlagInDBByPath(const std::string &path)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "ExistEditFlagInDBByPath Failed to Get RdbStore.");
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_FILE_PATH, path);
    predicates.Limit(1);
    auto resultSet = rdbStore->Query(predicates, {PhotoColumn::PHOTO_EDIT_TIME, PhotoColumn::PHOTO_EDIT_DATA_EXIST});
    int64_t editTime = 0;
    int32_t editDataExist = 0;
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "ExistEditFlagInDBByPath Rs Is Null");
    if (resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        editTime = GetInt64Val(PhotoColumn::PHOTO_EDIT_TIME, resultSet);
        editDataExist = GetInt32Val(PhotoColumn::PHOTO_EDIT_DATA_EXIST, resultSet);
    }
    resultSet->Close();
    if (editTime != 0 || editDataExist != 0) { // 有一个不为零 就需要补
        return true;
    }
    return false;
}

bool MediaCleanAllDirtyFilesTask::DealOriginFileExistEditDataNotExist(int32_t curBucketNum, const std::string &fileName)
{
    std::string editBucketFolder = ROOT_MEDIA_EDIT_DIR + std::to_string(curBucketNum) +
        SLASH_STR + fileName; // 编辑目录名 editdata  source.jpg source.heic source.mp4
    std::string extension = MediaPathUtils::GetExtension(fileName);
    std::string editOriginFile = editBucketFolder + SLASH_STR + SOURCE_FILE_PREFIX_NAME + DOT_STR + extension;
    std::string editDataFile = editBucketFolder + SLASH_STR + EDITDATA_FILE_NAME;
    bool isOriginFileExist = MediaFileUtils::IsFileExists(editOriginFile);
    bool isEditdataFileExist = MediaFileUtils::IsFileExists(editDataFile);
    // 原图存在 编辑效果图存在 编辑数据不存在 填充空的编辑数据
    if (isOriginFileExist && !isEditdataFileExist && ExistEditFlagInDBByPath(editOriginFile)) {
        MEDIA_INFO_LOG("DirtyMediaHandler File Create Editdata OriginFileExistEditDataNotExist EditData: %{public}s",
            editDataFile.c_str());
        nlohmann::json editDataJson;
        editDataJson[CONST_COMPATIBLE_FORMAT] = SPECIAL_EDIT_COMPATIBLE_FORMAT;
        editDataJson[CONST_FORMAT_VERSION] = SPECIAL_EDIT_FORMAT_VERSION;
        editDataJson[CONST_EDIT_DATA] = SPECIAL_EDIT_EDIT_DATA;
        editDataJson[CONST_APP_ID] = SPECIAL_EDIT_APP_ID;
        string editDataContent = editDataJson.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateFile(editDataFile), E_HAS_FS_ERROR,
            "Failed To Create editdata File %{private}s", MediaFileUtils::DesensitizePath(editDataFile).c_str());
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::WriteStrToFile(editDataFile, editDataContent), E_HAS_FS_ERROR,
            "Failed to write editdata:%{private}s", editDataFile.c_str());
        std::string dataPath = ROOT_MEDIA_ORG_DIR + std::to_string(curBucketNum) + SLASH_STR + fileName;
        UpdateNoneEditTimeByPath(dataPath, MediaFileUtils::UTCTimeSeconds(), 1);
    }
    return true;
}

bool MediaCleanAllDirtyFilesTask::ProcessOriginFolderBatch(int32_t curBucketNum, const std::string &fileName)
{
    std::string path = ROOT_MEDIA_ORG_DIR + std::to_string(curBucketNum) + SLASH_STR + fileName;
    // 图片或视频 文件已经处理完了 跳过 动图
    if (!MediaFileUtils::IsFileExists(path)) { // 避免异常
        return true;
    }
    if (DealWithZeroSizeFile(path)) {
        return true;
    }
    bool finish = HandleOriginFileNotExistAddToTable(curBucketNum, fileName); // 反查数据库 原图存在 元数据不存在
    CHECK_AND_RETURN_RET_LOG(finish, false,
        "Failed to create HandleOriginFileNotExistAddToTable %{public}s", fileName.c_str());
    std::string extension = MediaPathUtils::GetExtension(fileName);
    std::string editOriginFile = ROOT_MEDIA_EDIT_DIR + std::to_string(curBucketNum) + SLASH_STR + fileName +
        SLASH_STR + SOURCE_FILE_PREFIX_NAME + DOT_STR + extension;
    if (MediaFileUtils::IsFileExists(editOriginFile)) { // 编辑图存在
        // 原图存在 编辑原图存在 编辑数据不存在 填充空的编辑数据 更新数据库
        DealOriginFileExistEditDataNotExist(curBucketNum, fileName);
    }
    return true;
}

bool MediaCleanAllDirtyFilesTask::HandleOriginBucketFolder(int32_t curBucketNum)
{
    std::string originBucketFolder = ROOT_MEDIA_LOCAL_ORG_DIR + std::to_string(curBucketNum);
    std::vector<std::string> fileNameVec;
    MediaFileUtils::GetAllFileNameListUnderPath(originBucketFolder, fileNameVec);
    for (const auto& fileName : fileNameVec) {
        if (!this->Accept()) {
            MEDIA_ERR_LOG("HandleOriginBucketFolder Failed End");
            return false;
        }
        CHECK_AND_CONTINUE_ERR_LOG(IsLegalMediaAsset(fileName), // 文件名合法性校验 Report point
            "DirtyMediaHandler Skip Illegal File While HandleOriginBucketFolder %{public}s", fileName.c_str());
        std::string dataPath = ROOT_MEDIA_ORG_DIR + std::to_string(curBucketNum) + SLASH_STR + fileName;
        CHECK_AND_CONTINUE_INFO_LOG(!ExistCloudAssetPathInDB(dataPath), // 纯云图跳过
            "DirtyMediaHandler Skip cloud File While HandleOriginBucketFolder %{public}s",
            MediaFileUtils::DesensitizePath(dataPath).c_str());
        bool finish = ProcessOriginFolderBatch(curBucketNum, fileName);
        CHECK_AND_RETURN_RET_LOG(finish, false,
            "Failed To Create ProcessOriginFolderBatch %{public}s", fileName.c_str());
    }
    return true;
}

bool MediaCleanAllDirtyFilesTask::DealThumbsEffectAssetNotExist(int32_t curBucketNum, const std::string &folderName)
{ // 以缩略图填充原图 后续考虑旋转角度 ext目录
    std::string originBucketFolderFile = ROOT_MEDIA_ORG_DIR + std::to_string(curBucketNum) +
        SLASH_STR + folderName;
    std::string thumbsBucketFolderName = ROOT_MEDIA_THUMBS_DIR + std::to_string(curBucketNum) +
        SLASH_STR + folderName;
    std::string originLocalBucketFile = MediaFileUtils::GetLocalPath(originBucketFolderFile);
    std::string thumbsBucketFolderFile = thumbsBucketFolderName + SLASH_STR + THM_FILE_NAME;
    std::string thumbsBucketFolderLCDFile = thumbsBucketFolderName + SLASH_STR + LCD_FILE_NAME;
    if (MediaFileUtils::IsFileExists(thumbsBucketFolderLCDFile) &&
        !MediaFileUtils::IsFileExists(originBucketFolderFile) &&
        !MediaFileUtils::IsFileExists(originLocalBucketFile)) { // LCD 优先
        CHECK_AND_RETURN_RET_LOG(!IsZeroSizeFile(thumbsBucketFolderLCDFile), true, "%{public}s file size is 0",
            MediaFileUtils::DesensitizePath(thumbsBucketFolderLCDFile).c_str()); // 0kb 跳过
        MediaFileUtils::CopyFileUtil(thumbsBucketFolderLCDFile, originLocalBucketFile);
        MEDIA_INFO_LOG("DirtyMediaHandler DealThumbsEffectAssetNotExist lcd Cp To Org Bucket:%{public}d,"
            "Name: %{public}s", curBucketNum, MediaFileUtils::DesensitizePath(folderName).c_str());
        return true;
    } else if (MediaFileUtils::IsFileExists(thumbsBucketFolderFile) &&
        !MediaFileUtils::IsFileExists(originBucketFolderFile) &&
        !MediaFileUtils::IsFileExists(originLocalBucketFile)) {
        CHECK_AND_RETURN_RET_LOG(!IsZeroSizeFile(thumbsBucketFolderFile), true, "%{public}s file size is 0",
            MediaFileUtils::DesensitizePath(thumbsBucketFolderFile).c_str()); // 0kb 跳过
        MediaFileUtils::CopyFileUtil(thumbsBucketFolderFile, originLocalBucketFile);
        MEDIA_INFO_LOG("DirtyMediaHandler DealThumbsEffectAssetNotExist thumbs Cp To Org Bucket:%{public}d,"
            "Name: %{public}s", curBucketNum, MediaFileUtils::DesensitizePath(folderName).c_str());
        return true;
    }
    return false;
}

int32_t MediaCleanAllDirtyFilesTask::QueryPhotoAddTimeByPath(const std::string &path, int64_t &addTime)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "QueryPhotoAddTimeByPath Failed to Get RdbStore.");
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_FILE_PATH, path);
    predicates.Limit(1);
    auto resultSet = rdbStore->Query(predicates, {PhotoColumn::MEDIA_DATE_ADDED});
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RESULT_SET_NULL, "QueryPhotoAddTimeByPath Rs Is Null");
    if (resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        addTime = GetInt64Val(PhotoColumn::MEDIA_DATE_ADDED, resultSet);
        MEDIA_INFO_LOG("QueryPhotoAddTimeByPath addTime %{public}" PRId64, addTime);
    }
    resultSet->Close();
    return E_OK;
}

bool MediaCleanAllDirtyFilesTask::ProcessThumbsFolderBatch(int32_t curBucketNum, const std::string &folderName)
{
    CHECK_AND_RETURN_RET_INFO_LOG(!IsIllegalThumbFolderFile(curBucketNum, folderName), true,
        "DirtyMediaHandler Skip Illegal File While ProcessThumbsFolderBatch %{public}s",
        MediaFileUtils::DesensitizePath(folderName).c_str()); // 文件名合法性校验 Report point
    std::string originBucketFolderFile = ROOT_MEDIA_ORG_DIR + std::to_string(curBucketNum) +
        SLASH_STR + folderName; // 原图
    std::string thumbsBucketFolderName = ROOT_MEDIA_THUMBS_DIR + std::to_string(curBucketNum) +
        SLASH_STR + folderName;
    std::string thumbsBucketFolderFile = thumbsBucketFolderName + SLASH_STR + THM_FILE_NAME;
    std::string thumbsBucketFolderLCDFile = thumbsBucketFolderName + SLASH_STR + LCD_FILE_NAME;
    if (DealWithZeroSizeFile(thumbsBucketFolderFile) || DealWithZeroSizeFile(thumbsBucketFolderLCDFile)) {
        return true;
    }
    if (!MediaFileUtils::IsFileExists(originBucketFolderFile)) {
        // 原图不存在 以缩略图填充原图，扫描填充元数据
        if (!ExistPhotoPathInDB(originBucketFolderFile)) {
            MEDIA_INFO_LOG("ProcessThumbsFolderBatch Thumb Exist, Org And Record NotExist, Cp Thumb %{public}s",
                MediaFileUtils::DesensitizePath(originBucketFolderFile).c_str());
            DealThumbsEffectAssetNotExist(curBucketNum, folderName);
            AddFileToTableWithFixedName(curBucketNum, folderName);
            AddToFilesCacheSet(originBucketFolderFile);
        } else {
            // date_added距此超72小时使用缩略图填充原图并扫描刷新
            int64_t addTime = INT64_MAX;
            QueryPhotoAddTimeByPath(originBucketFolderFile, addTime);
            int64_t timeMs = MediaFileUtils::UTCTimeMilliSeconds();
            if (timeMs - addTime > THREE_DAY_MS) {
                MEDIA_INFO_LOG("ProcessThumbsFolderBatch Thumb, Record Exist, Org NotExist, Cp Thumb %{public}s",
                    MediaFileUtils::DesensitizePath(originBucketFolderFile).c_str());
                DealThumbsEffectAssetNotExist(curBucketNum, folderName);
                MediaAssetsService::GetInstance().ScanExistFileRecord(-1, originBucketFolderFile);
                AddToFilesCacheSet(originBucketFolderFile);
            } else {
                MEDIA_WARN_LOG("Thumb Exist Org NotExist WithIn One Day %{public}s",
                    MediaFileUtils::DesensitizePath(originBucketFolderFile).c_str());
            }
        }
    }
    return true;
}

// 目录内非法名称识别 LCD.jpg  THM.jpg THM_ASTC.astc
bool MediaCleanAllDirtyFilesTask::IsIllegalThumbFolderFile(int32_t curBucketNum, const std::string &folderName)
{
    bool containIllegal = false;
    std::string thumbsBucketFolderName = ROOT_MEDIA_THUMBS_DIR + std::to_string(curBucketNum) +
        SLASH_STR + folderName;
    std::vector<std::string> fileNameVec;
    MediaFileUtils::GetAllFileNameListUnderPath(thumbsBucketFolderName, fileNameVec);
    for (const auto& fileName : fileNameVec) {
        if (!Starts_With(fileName, THM_FILE_NAME) && !Starts_With(fileName, LCD_FILE_NAME) &&
            !Starts_With(fileName, THM_ASTC_FILE_NAME)) {
            MEDIA_ERR_LOG("IsIllegalThumbFolderFile Folder: %{public}s, Name: %{public}s",
                MediaFileUtils::DesensitizePath(thumbsBucketFolderName).c_str(), fileName.c_str());
            containIllegal = true;
        }
    }
    return containIllegal;
}

bool MediaCleanAllDirtyFilesTask::HandleThumbsBucketFolder(int32_t curBucketNum)
{
    std::vector<std::string> folderNameVec;
    std::string thumbsBucketFolder = ROOT_MEDIA_LOCAL_THUMBS_DIR + std::to_string(curBucketNum);
    MediaFileUtils::GetFolderListUnderPath(thumbsBucketFolder, folderNameVec);
    for (const auto& folderName : folderNameVec) {
        if (!this->Accept()) {
            MEDIA_ERR_LOG("HandleThumbsBucketFolder Failed End");
            return false;
        }
        MEDIA_DEBUG_LOG("HandleThumbsBucketFolder Name: %{public}s",
            MediaFileUtils::DesensitizePath(folderName).c_str());
        CHECK_AND_CONTINUE_ERR_LOG(IsLegalMediaAsset(folderName), // 文件名合法性校验 Report point 空文件夹跳过 删除等
            "DirtyMediaHandler Skip Illegal File While HandleThumbsBucketFolder %{public}s",
            MediaFileUtils::DesensitizePath(folderName).c_str());
        std::string dataPath = ROOT_MEDIA_ORG_DIR + std::to_string(curBucketNum) + SLASH_STR + folderName;
        CHECK_AND_CONTINUE_INFO_LOG(!ExistCloudAssetPathInDB(dataPath), // 纯云图跳过
            "DirtyMediaHandler Skip cloud File While HandleThumbsBucketFolder %{public}s",
            MediaFileUtils::DesensitizePath(dataPath).c_str());
        CHECK_AND_CONTINUE_INFO_LOG(IsJpgImageFile(folderName), // 非JPG静态图跳过缩略图处理
            "DirtyMediaHandler Skip video File While HandleThumbsBucketFolder %{public}s",
            MediaFileUtils::DesensitizePath(dataPath).c_str());
        bool finish = ProcessThumbsFolderBatch(curBucketNum, folderName);
        CHECK_AND_RETURN_RET_LOG(finish, false,
            "Failed to create ProcessThumbsFolderBatch %{public}s",
            MediaFileUtils::DesensitizePath(folderName).c_str());
    }
    return true;
}

// s1 恢复编辑过原图的为效果图 多一个应用效果过程 扫描原图填充元数据 元数据不存在 insert
bool MediaCleanAllDirtyFilesTask::DealEditedEffectFileNotExistInEditFolder(int32_t curBucketNum,
    const std::string &fileName, DirtyFilePathInfo& dirtyFilePathInfo)
{
    if (!MediaFileUtils::IsFileExists(dirtyFilePathInfo.effectFolder)) {
        if (!MediaFileUtils::CreateDirectory(dirtyFilePathInfo.effectFolder)) {
            MEDIA_ERR_LOG("Create DestPath:%{private}s failed", dirtyFilePathInfo.effectFolder.c_str());
            return true; // failed but finish
        }
    }
    if (MediaFileUtils::IsFileExists(dirtyFilePathInfo.effectFolderFile) ||
        MediaFileUtils::IsFileExists(dirtyFilePathInfo.localEffectFile)) { // 再查一次 避免误操作
        MEDIA_ERR_LOG("DirtyMediaHandler DealEditedEffectFileNotExistInEditFolder exist: %{public}s",
            MediaFileUtils::DesensitizePath(dirtyFilePathInfo.localEffectFile).c_str());
        return true;
    }
    // 静图
    bool cpSucc = MediaFileUtils::CopyFileUtil(dirtyFilePathInfo.editOriginFile, dirtyFilePathInfo.localEffectFile);
    MEDIA_INFO_LOG("DirtyMediaHandler DealEditedEffectFileNotExistInEditFolder Cp To Org: %{public}s, Ret: %{public}d",
        MediaFileUtils::DesensitizePath(dirtyFilePathInfo.effectFolderFile).c_str(), cpSucc);
    if (cpSucc && !ExistPhotoPathInDB(dirtyFilePathInfo.effectFolderFile)) {
        AddFileToTableWithFixedName(curBucketNum, fileName);
        int32_t ret = MediaAssetsService::GetInstance().ApplyEditEffectToFile(curBucketNum, fileName);
        MEDIA_INFO_LOG("DirtyMediaHandler ApplyEditEffectToFile Ret: %{public}d", ret);
        MediaAssetsService::GetInstance().ScanExistFileRecord(-1, dirtyFilePathInfo.effectFolderFile);
    } else if (cpSucc) {
        int32_t ret = MediaAssetsService::GetInstance().ApplyEditEffectToFile(curBucketNum, fileName);
        MEDIA_INFO_LOG("DirtyMediaHandler No Record ApplyEditEffectToFile Ret: %{public}d", ret);
        // ReScan file, size may change
        MediaAssetsService::GetInstance().ScanExistFileRecord(-1, dirtyFilePathInfo.effectFolderFile);
    }
    return true;
}

// s2 恢复存的原图为效果图 扫描原图填充元数据 元数据不存在 insert/存在rescan
bool MediaCleanAllDirtyFilesTask::DealEffectFileNotExistInEditFolder(int32_t curBucketNum,
    const std::string &fileName, DirtyFilePathInfo& dirtyFilePathInfo)
{
    if (!MediaFileUtils::IsFileExists(dirtyFilePathInfo.effectFolder)) {
        if (!MediaFileUtils::CreateDirectory(dirtyFilePathInfo.effectFolder)) {
            MEDIA_ERR_LOG("Create DestPath:%{private}s Failed", dirtyFilePathInfo.effectFolder.c_str());
            return true; // failed but finish
        }
    }
    if (MediaFileUtils::IsFileExists(dirtyFilePathInfo.effectFolderFile) ||
        MediaFileUtils::IsFileExists(dirtyFilePathInfo.localEffectFile)) { // 再查一次 避免误操作
        MEDIA_ERR_LOG("DirtyMediaHandler DealEffectFileNotExistInEditFolder Cp To Org: %{public}s",
            MediaFileUtils::DesensitizePath(dirtyFilePathInfo.localEffectFile).c_str());
        return true;
    }
    // 静图
    bool cpSucc = MediaFileUtils::CopyFileUtil(dirtyFilePathInfo.editOriginFile, dirtyFilePathInfo.localEffectFile);
    MEDIA_INFO_LOG("DirtyMediaHandler DealEffectFileNotExistInEditFolder Cp To Org: %{public}s, Ret: %{public}d",
        MediaFileUtils::DesensitizePath(dirtyFilePathInfo.effectFolderFile).c_str(), cpSucc);
    if (cpSucc && !ExistPhotoPathInDB(dirtyFilePathInfo.effectFolderFile)) {
        AddFileToTableWithFixedName(curBucketNum, fileName);
        // ReScan file, size may change
        MediaAssetsService::GetInstance().ScanExistFileRecord(-1, dirtyFilePathInfo.effectFolderFile);
    } else if (cpSucc) {
        // ReScan file, size may change 没编辑过 但是带水印拍的 也需要恢复效果
        int32_t ret = MediaAssetsService::GetInstance().ApplyEditEffectToFile(curBucketNum, fileName);
        MEDIA_INFO_LOG("DirtyMediaHandler No Record ApplyEditEffectToFile Ret: %{public}d", ret);
        MediaAssetsService::GetInstance().ScanExistFileRecord(-1, dirtyFilePathInfo.effectFolderFile);
    }
    return true;
}

// s3 恢复编辑过动图的为效果图 多一个应用效果过程
bool MediaCleanAllDirtyFilesTask::DealEditedEffectMovingPhotoNotExistInEditFolder(int32_t curBucketNum,
    const std::string &fileName, DirtyFilePathInfo& dirtyFilePathInfo)
{
    if (!MediaFileUtils::IsFileExists(dirtyFilePathInfo.effectFolder)) {
        if (!MediaFileUtils::CreateDirectory(dirtyFilePathInfo.effectFolder)) {
            MEDIA_ERR_LOG("Create DestPath:%{private}s failed", dirtyFilePathInfo.effectFolder.c_str());
            return true; // failed but finish
        }
    }
    if (MediaFileUtils::IsFileExists(dirtyFilePathInfo.effectFolderFile) ||
        MediaFileUtils::IsFileExists(dirtyFilePathInfo.localEffectFile)) { // 再查一次 避免误操作
        MEDIA_ERR_LOG("DirtyMediaHandler DealEditedEffectMovingPhotoNotExistInEditFolder Cp To Org: %{public}s",
            MediaFileUtils::DesensitizePath(dirtyFilePathInfo.localEffectFile).c_str());
        return true;
    }
    // 动图图片
    bool cpSucc = MediaFileUtils::CopyFileUtil(dirtyFilePathInfo.editOriginFile, dirtyFilePathInfo.localEffectFile);
    // 动图视频
    std::string videoPath = dirtyFilePathInfo.effectFolder + SLASH_STR +
        MediaFileUtils::GetTitleFromDisplayName(dirtyFilePathInfo.fileName) + MOVING_PHOTO_SUFFIX;
    std::string localVideoPath = MediaFileUtils::GetLocalPath(videoPath);
    bool cpSuccV = true;
    if (!MediaFileUtils::IsFileExists(videoPath) &&
        !MediaFileUtils::IsFileExists(localVideoPath)) { // 不存在就拷贝一份 存在直接跳过
        cpSuccV = MediaFileUtils::CopyFileUtil(dirtyFilePathInfo.editOriginMovingPhotoVideo, localVideoPath);
    }
    MEDIA_INFO_LOG("DirtyMediaHandler DealEditedEffectMovingPhotoNotExistInEditFolder Cp To Org: %{public}s, "
        "Ret: %{public}d, RetV: %{public}d",
        MediaFileUtils::DesensitizePath(dirtyFilePathInfo.localEffectFile).c_str(), cpSucc, cpSuccV);
    if (cpSucc && cpSuccV && !ExistPhotoPathInDB(dirtyFilePathInfo.effectFolderFile)) {
        AddMovingPhotoFileToTableWithFixedName(curBucketNum, fileName);
        int32_t ret = MediaAssetsService::GetInstance().ApplyEditEffectToFile(curBucketNum, fileName);
        MEDIA_INFO_LOG("DirtyMediaHandler ApplyEditEffectToFile Ret: %{public}d", ret);
        MediaAssetsService::GetInstance().ScanExistFileRecord(-1, dirtyFilePathInfo.effectFolderFile);
    } else {
        // ReScan file, size may change
        MediaAssetsService::GetInstance().ScanExistFileRecord(-1, dirtyFilePathInfo.effectFolderFile);
    }
    return true;
}

// s4 恢复存的动图 原图为效果图 扫描原图填充元数据 元数据不存在 insert/存在rescan
bool MediaCleanAllDirtyFilesTask::DealEffectMovingPhotoNotExistInEditFolder(int32_t curBucketNum,
    const std::string &fileName, DirtyFilePathInfo& dirtyFilePathInfo)
{
    if (!MediaFileUtils::IsFileExists(dirtyFilePathInfo.effectFolder)) {
        if (!MediaFileUtils::CreateDirectory(dirtyFilePathInfo.effectFolder)) {
            MEDIA_ERR_LOG("Create DestPath:%{private}s Failed", dirtyFilePathInfo.effectFolder.c_str());
            return true; // failed but finish
        }
    }
    if (MediaFileUtils::IsFileExists(dirtyFilePathInfo.effectFolderFile) ||
        MediaFileUtils::IsFileExists(dirtyFilePathInfo.localEffectFile)) { // 再查一次 避免误操作
        MEDIA_ERR_LOG("DirtyMediaHandler DealEffectMovingPhotoNotExistInEditFolder MovingPhoto Cp To Org: %{public}s",
            MediaFileUtils::DesensitizePath(dirtyFilePathInfo.localEffectFile).c_str());
        return true;
    }
    // 动图图片
    bool cpSucc = MediaFileUtils::CopyFileUtil(dirtyFilePathInfo.editOriginFile, dirtyFilePathInfo.localEffectFile);
    // 动图视频
    std::string videoPath = dirtyFilePathInfo.effectFolder + SLASH_STR +
        MediaFileUtils::GetTitleFromDisplayName(dirtyFilePathInfo.fileName) + MOVING_PHOTO_SUFFIX;
    std::string localVideoPath = MediaFileUtils::GetLocalPath(videoPath);
    bool cpSuccV = true;
    if (!MediaFileUtils::IsFileExists(videoPath) &&
        !MediaFileUtils::IsFileExists(localVideoPath)) { // 不存在就拷贝一份 存在直接跳过
        cpSuccV = MediaFileUtils::CopyFileUtil(dirtyFilePathInfo.editOriginMovingPhotoVideo, localVideoPath);
    }
    MEDIA_INFO_LOG("DirtyMediaHandler DealEffectMovingPhotoNotExistInEditFolder Cp To Org: %{public}s, "
        "Ret: %{public}d, RetV: %{public}d",
        MediaFileUtils::DesensitizePath(dirtyFilePathInfo.effectFolderFile).c_str(), cpSucc, cpSuccV);
    if (cpSucc && cpSuccV && !ExistPhotoPathInDB(dirtyFilePathInfo.effectFolderFile)) {
        AddMovingPhotoFileToTableWithFixedName(curBucketNum, fileName);
    } else {
        // ReScan file, size may change
        MediaAssetsService::GetInstance().ScanExistFileRecord(-1, dirtyFilePathInfo.effectFolderFile);
    }
    return true;
}

bool MediaCleanAllDirtyFilesTask::ProcessEditFolderBatchMovingPhotos(int32_t curBucketNum,
    const std::string &folderName, DirtyFilePathInfo& dirtyFilePathInfo)
{
    bool isEditdataFileExist = MediaFileUtils::IsFileExists(dirtyFilePathInfo.editDataFile);
    if (isEditdataFileExist) { // 原图存在 效果图不存在的 原图应用效果 并搬迁至效果图位置
        MEDIA_INFO_LOG("DirtyMediaHandler Edit Generate Movingphoto Effect File:%{public}s",
            MediaFileUtils::DesensitizePath(dirtyFilePathInfo.effectFolderFile).c_str());
        bool finish = DealEditedEffectMovingPhotoNotExistInEditFolder(curBucketNum, folderName, dirtyFilePathInfo);
        CHECK_AND_RETURN_RET_LOG(finish, false,
            "Failed To Create DealEditedEffectMovingPhotoNotExistInEditFolder %{public}s",
            MediaFileUtils::DesensitizePath(folderName).c_str()); // 数据库edit_time有值则不变，无值则刷新为当前时间
        UpdateNoneEditTimeByPath(dirtyFilePathInfo.effectFolderFile, MediaFileUtils::UTCTimeSeconds(), 1);
        AddToFilesCacheSet(dirtyFilePathInfo.effectFolderFile);
    } else { // 原图存在 效果图不存在的 原图搬迁至效果图位置
        MEDIA_INFO_LOG("DirtyMediaHandler Edit Move To Movingphoto Effect Folder:%{public}s",
            MediaFileUtils::DesensitizePath(dirtyFilePathInfo.effectFolderFile).c_str());
        bool finish = DealEffectMovingPhotoNotExistInEditFolder(curBucketNum, folderName, dirtyFilePathInfo);
        CHECK_AND_RETURN_RET_LOG(finish, false,
            "Failed To Create DealEffectMovingPhotoNotExistInEditFolder %{public}s",
            MediaFileUtils::DesensitizePath(folderName).c_str());
        UpdateEditTimeByPath(dirtyFilePathInfo.effectFolderFile, 0L, 0);
        AddToFilesCacheSet(dirtyFilePathInfo.effectFolderFile);
    }
    return true;
}

bool MediaCleanAllDirtyFilesTask::ProcessEditFolderBatchNormalPhotos(int32_t curBucketNum,
    const std::string &folderName, DirtyFilePathInfo& dirtyFilePathInfo)
{
    bool isEditdataFileExist = MediaFileUtils::IsFileExists(dirtyFilePathInfo.editDataFile);
    if (isEditdataFileExist) { // 原图存在 效果图不存在的 原图应用效果 并搬迁至效果图位置
        MEDIA_INFO_LOG("DirtyMediaHandler Edit Generate Effect File:%{public}s",
            MediaFileUtils::DesensitizePath(dirtyFilePathInfo.effectFolderFile).c_str());
        bool finish = DealEditedEffectFileNotExistInEditFolder(curBucketNum, folderName, dirtyFilePathInfo);
        CHECK_AND_RETURN_RET_LOG(finish, false,
            "Failed To Create DealEditedEffectFileNotExistInEditFolder %{public}s",
            MediaFileUtils::DesensitizePath(folderName).c_str()); // 数据库edit_time有值则不变
        UpdateNoneEditTimeByPath(dirtyFilePathInfo.effectFolderFile, MediaFileUtils::UTCTimeSeconds(), 1);
        AddToFilesCacheSet(dirtyFilePathInfo.effectFolderFile);
    } else { // 原图存在 效果图不存在的 编辑数据不存在 原图搬迁至效果图位置
        MEDIA_INFO_LOG("DirtyMediaHandler Edit Move To Effect Folder:%{public}s",
            MediaFileUtils::DesensitizePath(dirtyFilePathInfo.effectFolderFile).c_str());
        bool finish = DealEffectFileNotExistInEditFolder(curBucketNum, folderName, dirtyFilePathInfo);
        CHECK_AND_RETURN_RET_LOG(finish, false,
            "Failed To Create DealEffectFileNotExistInEditFolder %{public}s",
            MediaFileUtils::DesensitizePath(folderName).c_str());
        UpdateEditTimeByPath(dirtyFilePathInfo.effectFolderFile, 0L, 0);
        AddToFilesCacheSet(dirtyFilePathInfo.effectFolderFile);
    }
    return true;
}

DirtyFilePathInfo MediaCleanAllDirtyFilesTask::BuildDirtyFilePathInfo(int32_t curBucketNum,
    const std::string &folderName)
{
    DirtyFilePathInfo dirtyFilePathInfo;
    dirtyFilePathInfo.curBucketNum = curBucketNum;
    dirtyFilePathInfo.fileName = folderName;
    dirtyFilePathInfo.extension = MediaPathUtils::GetExtension(folderName);
    dirtyFilePathInfo.effectFolder = ROOT_MEDIA_ORG_DIR + std::to_string(curBucketNum);
    // 编辑过 原图目录放的效果图
    dirtyFilePathInfo.effectFolderFile = dirtyFilePathInfo.effectFolder + SLASH_STR + folderName;
    dirtyFilePathInfo.editBucketFolder = ROOT_MEDIA_EDIT_DIR + std::to_string(curBucketNum) +
        SLASH_STR + folderName; // 编辑文件目录名 包含editdata extraData source.jpg~heic source.mp4
    dirtyFilePathInfo.editDataFile = dirtyFilePathInfo.editBucketFolder + SLASH_STR + EDITDATA_FILE_NAME;
    dirtyFilePathInfo.editDataCameraFile = dirtyFilePathInfo.editBucketFolder + SLASH_STR + EDITDATA_CAMERA_FILE_NAME;
    dirtyFilePathInfo.editOriginFile = dirtyFilePathInfo.editBucketFolder + SLASH_STR + SOURCE_FILE_PREFIX_NAME +
        DOT_STR + dirtyFilePathInfo.extension;
    dirtyFilePathInfo.editOriginMovingPhotoVideo = dirtyFilePathInfo.editBucketFolder + SLASH_STR +
        SOURCE_FILE_VIDEO_NAME;
    dirtyFilePathInfo.localEffectFile = MediaFileUtils::GetLocalPath(dirtyFilePathInfo.effectFolderFile);
    return dirtyFilePathInfo;
}

// 目录内非法名称识别 editdata  extraData  source.* jpg/heif  source.mp4
bool MediaCleanAllDirtyFilesTask::IsIllegalEditFolderFile(int32_t curBucketNum, const std::string &folderName)
{
    bool containIllegal = false;
    std::string editBucketFolder = ROOT_MEDIA_EDIT_DIR + std::to_string(curBucketNum) +
        SLASH_STR + folderName; // 编辑文件目录名 包含editdata extraData source.jpg~heic source.mp4
    std::vector<std::string> fileNameVec;
    MediaFileUtils::GetAllFileNameListUnderPath(editBucketFolder, fileNameVec);
    if (fileNameVec.size() == 0) {
        MEDIA_ERR_LOG("IsLegalEditFolderFile Empty Folder: %{public}s",
            MediaFileUtils::DesensitizePath(editBucketFolder).c_str());
    }
    for (const auto& fileName : fileNameVec) {
        if (!Starts_With(fileName, EDITDATA_FILE_NAME) && !Starts_With(fileName, EXTRADATA_FILE_NAME) &&
            !Starts_With(fileName, SOURCE_FILE_PREFIX_NAME) && !Starts_With(fileName, EDITDATA_CAMERA_FILE_NAME)
            && !Starts_With(fileName, EDITDATA_TRANSCODE_FILE_NAME)) {
            MEDIA_ERR_LOG("IsLegalEditFolderFile Folder: %{public}s, Name: %{public}s",
                MediaFileUtils::DesensitizePath(editBucketFolder).c_str(),
                fileName.c_str());
            containIllegal = true;
        }
    }
    return containIllegal;
}

int32_t MediaCleanAllDirtyFilesTask::GetFileIdByPathsFromDB(std::vector<std::string> &paths,
    std::set<int32_t> &fileIdSet)
{
    CHECK_AND_RETURN_RET_LOG(!paths.empty(), E_OK, "Paths Is Empty.");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "RdbStore Is Null.");
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(PhotoColumn::MEDIA_FILE_PATH, paths);
    auto resultSet = rdbStore->Query(predicates, {PhotoColumn::MEDIA_ID});
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RESULT_SET_NULL, "Rs Is Null");
    if (resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        int32_t fileId = GetInt32Val(PhotoColumn::MEDIA_ID, resultSet);
        MEDIA_DEBUG_LOG("GetFileIdByPathsFromDB FileId %{public}d", fileId);
        fileIdSet.insert(fileId);
    }
    resultSet->Close();
    return E_OK;
}

int32_t MediaCleanAllDirtyFilesTask::UpdateNoneEditTimeByPath(std::string &path, int64_t editTime,
    int32_t editDataExist)
{
    CHECK_AND_RETURN_RET_LOG(!path.empty(), E_ERR, "UpdateNoneEditTimeByPath Empty");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL,
        "UpdateNoneEditTimeByPath Failed to get rdbStore.");
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_FILE_PATH, path);
    predicates.Limit(1);
    auto resultSet = rdbStore->Query(predicates, {PhotoColumn::PHOTO_EDIT_TIME, PhotoColumn::PHOTO_EDIT_DATA_EXIST});
    int64_t editTimeDefault = 0;
    int32_t editDataExistDefault = 0;
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "ExistEditFlagInDBByPath Rs Is Null");
    if (resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        editTimeDefault = GetInt64Val(PhotoColumn::PHOTO_EDIT_TIME, resultSet);
        editDataExistDefault = GetInt32Val(PhotoColumn::PHOTO_EDIT_DATA_EXIST, resultSet);
    }
    resultSet->Close();

    NativeRdb::AbsRdbPredicates predicatesUpdate(PhotoColumn::PHOTOS_TABLE);
    NativeRdb::ValuesBucket value;
    if (editTimeDefault <= 0) { // 数据库edit_time有值则不变
        value.PutLong(PhotoColumn::PHOTO_EDIT_TIME, editTime);
    }
    value.PutInt(PhotoColumn::PHOTO_EDIT_DATA_EXIST, editDataExist);
    predicatesUpdate.EqualTo(PhotoColumn::MEDIA_FILE_PATH, path);
    int32_t changedRows = -1;
    int32_t ret = rdbStore->Update(changedRows, value, predicatesUpdate);
    MEDIA_INFO_LOG("UpdateNoneEditTimeByPath Ret: %{public}d, ChangedRows %{public}d", ret, changedRows);
    return ret;
}

int32_t MediaCleanAllDirtyFilesTask::UpdateEditTimeByPath(std::string &path, int64_t editTime, int32_t editDataExist)
{
    CHECK_AND_RETURN_RET_LOG(!path.empty(), E_ERR, "UpdateEditTimeByPath Empty");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL,
        "UpdateEditTimeByPath Failed to get rdbStore.");
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    NativeRdb::ValuesBucket value;
    if (editTime >= 0) {
        value.PutLong(PhotoColumn::PHOTO_EDIT_TIME, editTime);
    }
    if (editDataExist >= 0) {
        value.PutInt(PhotoColumn::PHOTO_EDIT_DATA_EXIST, editDataExist);
    }
    predicates.EqualTo(PhotoColumn::MEDIA_FILE_PATH, path);
    int32_t changedRows = -1;
    int32_t ret = rdbStore->Update(changedRows, value, predicates);
    MEDIA_INFO_LOG("UpdateEditTimeByPath Ret: %{public}d, ChangedRows %{public}d", ret, changedRows);
    return ret;
}

bool MediaCleanAllDirtyFilesTask::ProcessMovingPhotosInEditFolder(int32_t curBucketNum, const std::string &folderName,
    DirtyFilePathInfo &dirtyFilePathInfo)
{
    bool isOriginFileExist = MediaFileUtils::IsFileExists(dirtyFilePathInfo.editOriginFile);
    bool isEffectFileExist = MediaFileUtils::IsFileExists(dirtyFilePathInfo.effectFolderFile);
    bool isEditdataFileExist = MediaFileUtils::IsFileExists(dirtyFilePathInfo.editDataFile);
    MEDIA_INFO_LOG("DirtyMediaHandler ProcessMovingPhotosInEditFolder File:%{public}s",
        MediaFileUtils::DesensitizePath(dirtyFilePathInfo.effectFolderFile).c_str());
    if (isOriginFileExist && !isEffectFileExist) {
        return ProcessEditFolderBatchMovingPhotos(curBucketNum, folderName, dirtyFilePathInfo);
    }
    if (!isOriginFileExist && isEffectFileExist && isEditdataFileExist) {
        // 原图不存在 效果图存在的 删除编辑数据，标记图片未被编辑 清时间戳
        MediaFileUtils::DeleteFileWithRetry(dirtyFilePathInfo.editDataFile);
        UpdateEditTimeByPath(dirtyFilePathInfo.effectFolderFile, 0L, 0);
        MEDIA_INFO_LOG("Delete MovingPhoto editDataFile: %{public}s",
            MediaFileUtils::DesensitizePath(dirtyFilePathInfo.editDataFile).c_str());
        AddToFilesCacheSet(dirtyFilePathInfo.effectFolderFile);
        return true;
    }
    if (!isOriginFileExist && !isEffectFileExist && isEditdataFileExist) {
        // 原图不存在 效果图不存在的 删除编辑数据 删除目录记录等
        MediaFileUtils::DeleteFileWithRetry(dirtyFilePathInfo.editDataFile);
        UpdateEditTimeByPath(dirtyFilePathInfo.effectFolderFile, 0L, 0);
        MediaFileUtils::DeleteFileWithRetry(dirtyFilePathInfo.editBucketFolder);
        MEDIA_INFO_LOG("Delete MovingPhoto editData File And Dir: %{public}s",
            MediaFileUtils::DesensitizePath(dirtyFilePathInfo.editDataFile).c_str());
        AddToFilesCacheSet(dirtyFilePathInfo.effectFolderFile);
        return true;
    }
    // 编辑原图存在 效果图存在的 不处理
    return true;
}

bool MediaCleanAllDirtyFilesTask::ProcessEditFolderBatch(int32_t curBucketNum, const std::string &folderName)
{
    CHECK_AND_RETURN_RET_INFO_LOG(!IsIllegalEditFolderFile(curBucketNum, folderName), true,
        "DirtyMediaHandler Skip Illegal File While ProcessEditFolderBatch %{public}s",
        MediaFileUtils::DesensitizePath(folderName).c_str()); // 清理除固定名称文件之外的文件
    DirtyFilePathInfo dirtyFilePathInfo = BuildDirtyFilePathInfo(curBucketNum, folderName);
    MEDIA_DEBUG_LOG("ProcessEditFolderBatch FileName:%{public}s", MediaFileUtils::DesensitizePath(folderName).c_str());
    if (DealWithZeroSizeFile(dirtyFilePathInfo.editOriginFile)) {
        return true;
    }
    CHECK_AND_RETURN_RET_LOG(!IsZeroSizeFile(dirtyFilePathInfo.editOriginFile), true, "%{public}s file size is 0",
        MediaFileUtils::DesensitizePath(dirtyFilePathInfo.editOriginFile).c_str()); // 0kb 跳过
    if (IsMovingPhotosInEditFolder(curBucketNum, folderName)) { // 动图
        return ProcessMovingPhotosInEditFolder(curBucketNum, folderName, dirtyFilePathInfo);
    } else { // 静图
        bool isOriginFileExist = MediaFileUtils::IsFileExists(dirtyFilePathInfo.editOriginFile);
        bool isEffectFileExist = MediaFileUtils::IsFileExists(dirtyFilePathInfo.effectFolderFile);
        bool isEditdataFileExist = MediaFileUtils::IsFileExists(dirtyFilePathInfo.editDataFile);
        if (isOriginFileExist && !isEffectFileExist) { // 效果图不存在的
            return ProcessEditFolderBatchNormalPhotos(curBucketNum, folderName, dirtyFilePathInfo);
        }
        if (!isOriginFileExist && isEffectFileExist && isEditdataFileExist) {
            // 原图不存在 效果图存在的 删除编辑数据，标记图片未被编辑 清时间戳
            MediaFileUtils::DeleteFileWithRetry(dirtyFilePathInfo.editDataFile);
            UpdateEditTimeByPath(dirtyFilePathInfo.effectFolderFile, 0L, 0);
            MEDIA_INFO_LOG("Delete editDataFile: %{public}s",
                MediaFileUtils::DesensitizePath(dirtyFilePathInfo.editDataFile).c_str());
            AddToFilesCacheSet(dirtyFilePathInfo.effectFolderFile);
            return true;
        }
        if (!isOriginFileExist && !isEffectFileExist && isEditdataFileExist) {
            // 原图不存在 效果图不存在的 删除编辑数据 删除目录记录等
            MediaFileUtils::DeleteFileWithRetry(dirtyFilePathInfo.editDataFile);
            UpdateEditTimeByPath(dirtyFilePathInfo.effectFolderFile, 0L, 0);
            MediaFileUtils::DeleteFileWithRetry(dirtyFilePathInfo.editBucketFolder);
            MEDIA_INFO_LOG("Delete editData File and Dir: %{public}s",
                MediaFileUtils::DesensitizePath(dirtyFilePathInfo.editDataFile).c_str());
            AddToFilesCacheSet(dirtyFilePathInfo.effectFolderFile);
            return true;
        }
        // 编辑原图存在 效果图存在的 不处理
    }
    return true;
}

bool MediaCleanAllDirtyFilesTask::IsLegalMediaAsset(const std::string &fileName)
{
    MediaType mediaTypeOther = MediaFileUtils::GetMediaType(fileName);
    if (mediaTypeOther == MediaType::MEDIA_TYPE_VIDEO || mediaTypeOther == MEDIA_TYPE_IMAGE) {
        return true;
    } else {
        return false;
    }
}

bool MediaCleanAllDirtyFilesTask::HandleEditBucketFolder(int32_t curBucketNum)
{
    std::vector<std::string> folderNameVec;
    std::string editBucketFolder = ROOT_MEDIA_LOCAL_EDIT_DIR + std::to_string(curBucketNum);
    MediaFileUtils::GetFolderListUnderPath(editBucketFolder, folderNameVec);
    for (const auto& folderName : folderNameVec) {
        if (!this->Accept()) {
            MEDIA_ERR_LOG("HandleEditBucketFolder Failed End");
            return false;
        }
        MEDIA_DEBUG_LOG("HandleEditBucketFolder Loop Name: %{public}s",
            MediaFileUtils::DesensitizePath(folderName).c_str());
        CHECK_AND_CONTINUE_ERR_LOG(IsLegalMediaAsset(folderName), // 文件名合法性校验 Report point
            "DirtyMediaHandler Skip Illegal File While HandleEditBucketFolder %{public}s",
            MediaFileUtils::DesensitizePath(folderName).c_str());
        std::string dataPath = ROOT_MEDIA_ORG_DIR + std::to_string(curBucketNum) + SLASH_STR + folderName;
        CHECK_AND_CONTINUE_INFO_LOG(!ExistCloudAssetPathInDB(dataPath), // 纯云图跳过
            "DirtyMediaHandler Skip cloud File While HandleEditBucketFolder %{public}s",
            MediaFileUtils::DesensitizePath(dataPath).c_str());
        bool finish = ProcessEditFolderBatch(curBucketNum, folderName);
        CHECK_AND_RETURN_RET_LOG(finish, false,
            "Failed To Create ProcessEditFolderBatch %{public}s",
            MediaFileUtils::DesensitizePath(folderName).c_str());
    }
    return true;
}

bool MediaCleanAllDirtyFilesTask::HandleHandleAllDirtyFoldersInner(int32_t curBucketNum)
{
    if (!this->Accept()) {
        MEDIA_ERR_LOG("HandleHandleAllDirtyFoldersInner Failed End");
        return false;
    }
    if (curBucketNum == 0) { // 东湖的0桶 跳过
        return true;
    }
    // 1 处理编辑目录 多出的图片
    if (!HandleEditBucketFolder(curBucketNum)) {
        return false;
    }
    // 2 处理缩略图目录 多出的图片
    if (!HandleThumbsBucketFolder(curBucketNum)) {
        return false;
    }
    // 3 处理原图目录 多出的图片
    if (!HandleOriginBucketFolder(curBucketNum)) {
        return false;
    }

    return true;
}

void MediaCleanAllDirtyFilesTask::AddToFilesCacheSet(const std::string &val)
{
    std::lock_guard<std::mutex> lock(filesCacheSetMtx_);
    filesCacheSet_.insert(val);
}

void MediaCleanAllDirtyFilesTask::ClearFilesCacheSet()
{
    std::lock_guard<std::mutex> lock(filesCacheSetMtx_);
    filesCacheSet_.clear();
}

void MediaCleanAllDirtyFilesTask::ClearFileIdsCacheSet()
{
    std::lock_guard<std::mutex> lock(fileIdsCacheSetMtx_);
    fileIdsCacheSet_.clear();
}

bool MediaCleanAllDirtyFilesTask::ContainsFileIdsCacheSet(int32_t val)
{
    std::lock_guard<std::mutex> lock(fileIdsCacheSetMtx_);
    return fileIdsCacheSet_.count(val) > 0;
}

std::set<int32_t> MediaCleanAllDirtyFilesTask::ProcessCacheSet(const std::set<std::string>& cacheSet,
    int32_t batchSize)
{
    std::set<int32_t> finalIds;
    // 预分配内存，减少 vector 扩容开销
    std::vector<std::string> batchPaths;
    batchPaths.reserve(batchSize);
    auto it = cacheSet.begin();
    while (it != cacheSet.end()) {
        batchPaths.clear();
        // 1. 每次拿出一部分 (Batch)
        for (int32_t i = 0; i < batchSize && it != cacheSet.end(); ++i, ++it) {
            batchPaths.push_back(*it);
        }
        // 2. 去数据库中查 IDs
        std::set<int32_t> batchIds;
        GetFileIdByPathsFromDB(batchPaths, batchIds);

        // 3. 将结果并入最终集 (使用 insert(range) 或 move)
        finalIds.insert(batchIds.begin(), batchIds.end());
    }
    return finalIds;
}

void MediaCleanAllDirtyFilesTask::SaveCacheSetToCacheDB()
{
    {
        std::lock_guard<std::mutex> lock(filesCacheSetMtx_);
        std::set<int32_t> fileIdsCacheSet = ProcessCacheSet(filesCacheSet_, CACHE_BATCH_SIZE);
        {
            std::lock_guard<std::mutex> lock(fileIdsCacheSetMtx_);
            fileIdsCacheSet_ = fileIdsCacheSet;
        }
        MEDIA_INFO_LOG("DirtyMediaHandler SaveCacheSetToCacheDB Id: %{public}zu", fileIdsCacheSet.size());
    }
    ClearFilesCacheSet();
}


void MediaCleanAllDirtyFilesTask::HandleAllDirtyFolders(int32_t curStartBucketId)
{
    CHECK_AND_RETURN_INFO_LOG(curStartBucketId >= 0, "HandleAllDirtyFolders Finished");
    int32_t startBucketId = curStartBucketId == scanBeginCode ? 1 : curStartBucketId;
    std::vector<std::string> bucketsVec;
    MediaFileUtils::GetFolderListUnderPath(ROOT_MEDIA_LOCAL_ORG_DIR, bucketsVec);
    CHECK_AND_RETURN_INFO_LOG(!bucketsVec.empty(), "HandleAllDirtyFolders BucketsVec Empty");
    std::vector<int32_t> bucketsIntVec = ConvertBucketNameVector(bucketsVec);
    std::sort(bucketsIntVec.begin(), bucketsIntVec.end());
    for (const auto& curBucketNum : bucketsIntVec) {
        CHECK_AND_RETURN_INFO_LOG(!IsCurrentTaskTimeOut(), "HandleAllDirtyFolders Timeout");
        if (curBucketNum < startBucketId || curBucketNum == 0) { // 扫过的 和 东湖的0桶 跳过
            continue;
        }
        int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
        if (!this->Accept()) {
            MEDIA_ERR_LOG("DirtyMediaHandler HandleAllDirtyFolders failed End");
            SetBatchProgressId(curStartBucketId, START_BUCKET_NUMBER_STR);
            return;
        }
        MEDIA_DEBUG_LOG("DirtyMediaHandler HandleAllDirtyFolders Bucket: %{public}d", curBucketNum);
        // 处理原图目录 缩略图目录 编辑目录
        ClearFilesCacheSet();
        bool finish = HandleHandleAllDirtyFoldersInner(curBucketNum); // 内部入口
        SaveCacheSetToCacheDB();
        int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
        MEDIA_INFO_LOG("DirtyMediaHandler Folder Scan END Bucket :%{public}d, Duration: %{public}" PRId64,
            curBucketNum, endTime - startTime);
        if (finish) {
            curStartBucketId = curBucketNum; // Archiving
            MEDIA_DEBUG_LOG("DirtyMediaHandler Folder Scan Part END Bucket: %{public}d", curBucketNum);
            SetBatchProgressId(curBucketNum, START_BUCKET_NUMBER_STR);
        } else {
            return;
        }
    }
    MEDIA_INFO_LOG("DirtyMediaHandler Folder Scan END");
    SetBatchProgressId(scanDoneCode, START_BUCKET_NUMBER_STR);
}

void MediaCleanAllDirtyFilesTask::HandleAllTableAndFolder(int32_t curStartFileId, int32_t curStartBucketId)
{
    MEDIA_INFO_LOG("DirtyMediaHandler HandleAllTableAndFolder START");
    HandleAllDirtyFolders(curStartBucketId); // 先扫目录
    if (!this->Accept()) { // 过程中中断
        MEDIA_ERR_LOG("HandleAllTableAndFolder Interrupt");
        SetBatchExecuteTime();
        return;
    }
    HandleAllDirtyTable(curStartFileId);
    SetBatchExecuteTime(); // 保存处理进度
    MEDIA_INFO_LOG("DirtyMediaHandler HandleAllTableAndFolder END");
}

bool MediaCleanAllDirtyFilesTask::IsCurrentTaskTimeOut()
{
    int64_t timeSpend = MediaFileUtils::UTCTimeSeconds() - triggerTime_;
    if (timeSpend >= THREE_HOURS) {
        return true;
    }
    return false;
}

void MediaCleanAllDirtyFilesTask::HandleMediaAllDirtyFiles()
{
    int64_t lastExecuteTime = GetBatchExecuteTime();
    triggerTime_ = MediaFileUtils::UTCTimeSeconds(); // 当前时间
    MEDIA_INFO_LOG("DirtyMediaHandler Start LastExecuteTime: %{public}" PRId64, lastExecuteTime);
    if (lastExecuteTime > 0) { // 判断接续 执行过 接续执行
        int32_t curStartFileId = GetBatchProgressId(START_FILE_ID_STR);
        int32_t curStartBucketId = GetBatchProgressId(START_BUCKET_NUMBER_STR);
        if (curStartBucketId > 0) {
            // 接续处理文件系统
            HandleAllTableAndFolder(scanBeginCode, curStartBucketId); // -1 finish
            return;
        }
        if (curStartFileId >= 0 && curStartBucketId == scanDoneCode) {
            // 接续处理表
            HandleAllTableAndFolder(curStartFileId, scanDoneCode);
            return; // 避免重复执行
        }
        int64_t timeWindow = triggerTime_ - lastExecuteTime;
        MEDIA_INFO_LOG("DirtyMediaHandler Continue Fid: %{public}d, Bucket: %{public}d, Interval: %{public}" PRId64,
            curStartFileId, curStartBucketId, timeWindow);
        if (curStartFileId <= 0 && curStartBucketId <= 0 && timeWindow > FOUR_WEEK) { // 执行完成过 重新全量执行
            ClearFileIdsCacheSet();
            HandleAllTableAndFolder(scanBeginCode, scanBeginCode);
            return;
        } else {
            MEDIA_INFO_LOG("DirtyMediaHandler Skip Task Execute Period FileId: %{public}d, Bucket: %{public}d,"
                " LastExecuteTime %{public}" PRId64, curStartFileId, curStartBucketId, lastExecuteTime);
        }
    } else { // 未执行过 全量执行
        ClearFileIdsCacheSet();
        HandleAllTableAndFolder(scanBeginCode, scanBeginCode);
        return;
    }
}
// LCOV_EXCL_STOP
} // namespace OHOS::Media::Background