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
#define MLOG_TAG "PhotoStorageOperation"
#include "photo_storage_operation.h"

#include "media_log.h"
#include "media_file_utils.h"
#include "medialibrary_tracer.h"
#include "medialibrary_type_const.h"
#include "preferences.h"
#include "preferences_helper.h"
#include "cloud_sync_manager.h"
#include "medialibrary_db_const.h"
#include "userfile_manager_types.h"
#include "result_set_utils.h"

using namespace OHOS::FileManagement::CloudSync;

namespace OHOS::Media {
std::shared_ptr<NativeRdb::ResultSet> PhotoStorageOperation::FindStorage(std::shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoStorageOperation::FindStorage");
    bool conn = rdbStore == nullptr;
    CHECK_AND_RETURN_RET_LOG(!conn, nullptr, "RdbStore is null");

    TotalThumbnailSizeResult totalThumbnailSizeResult = {};
    GetTotalThumbnailSize(rdbStore, totalThumbnailSizeResult);
    MEDIA_INFO_LOG("Thumbnail stats: total_size = %{public}" PRId64 " bytes, count = %{public}d",
        totalThumbnailSizeResult.totalThumbnailSize, totalThumbnailSizeResult.thumbnailCount);

    TotalEditdataSizeResult totalEditdataSizeRusult = {};
    GetTotalEditdataSize(rdbStore, totalEditdataSizeRusult);
    
    int64_t dfsSize = 0;
    int32_t ret = CloudSyncManager::GetInstance().GetDentryFileOccupy(dfsSize);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "Media_Storage: Failed to get dfsSize");
    MEDIA_INFO_LOG("Media_Storage: dfsSize = %{public}" PRId64 " bytes", dfsSize);
    
    MEDIA_INFO_LOG("Editdata stats: total_size = %{public}" PRId64 " bytes, count = %{public}d"
        ", highlightSize = %{public}" PRId64, totalEditdataSizeRusult.totalEditdataSize,
        totalEditdataSizeRusult.editdataCount, GetHighlightSizeFromPreferences());
    int64_t totalExtSize = GetCacheSize() + GetBackUpSize() + GetMetaSize() + GetAudioSize() + GetCameraSize() +
        GetPictureSize() + GetMediaVideoSize() + GetCustomSize() + GetDataSize() + GetHighlightSizeFromPreferences()
        + totalThumbnailSizeResult.totalThumbnailSize + totalEditdataSizeRusult.totalEditdataSize + dfsSize;
    int64_t totalImageSize = 0;
    int64_t totalVideoSize = 0;
    std::string sql = this->SQL_DB_STORAGE_QUERY;
    auto queryResultSet = rdbStore->QuerySql(sql);
    CHECK_AND_RETURN_RET_LOG(queryResultSet != nullptr, nullptr, "queryResultSet is null!");
    while (queryResultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t mediatype = GetInt32Val(MEDIA_DATA_DB_MEDIA_TYPE, queryResultSet);
        int64_t size = GetInt64Val(MEDIA_DATA_DB_SIZE, queryResultSet);
        MEDIA_INFO_LOG("media_type: %{public}d, size: %{public}lld", mediatype, static_cast<long long>(size));
        if (mediatype == static_cast<int32_t>(MEDIA_TYPE_IMAGE) || mediatype == -1) { // -1 thumbnailType
            totalImageSize = size;
        } else if (mediatype == static_cast<int32_t>(MEDIA_TYPE_VIDEO)) {
            totalVideoSize = size;
        }
    }
    MEDIA_INFO_LOG("Media_Storage: ext = %{public}" PRId64 ", Image = %{public}" PRId64 ", Video = %{public}" PRId64,
        totalExtSize, totalImageSize, totalVideoSize);
    std::vector<NativeRdb::ValueObject> params = {totalExtSize, totalImageSize, totalVideoSize};
    std::string sqlInfo = this->SQL_DB_STORAGE_INFO_QUERY;
    return rdbStore->QuerySql(sqlInfo, params);
}

int64_t PhotoStorageOperation::GetDataSize()
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoStorageOperation::GetDataSize");
    size_t totalSize = 0;
    MediaFileUtils::StatDirSize(MEDIA_DATA_DIR, totalSize);
    MEDIA_INFO_LOG("Media_Storage: DataSize = %{public}" PRId64, static_cast<int64_t>(totalSize));
    return static_cast<int64_t>(totalSize);
}

int64_t PhotoStorageOperation::GetCustomSize()
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoStorageOperation::GetCustomSize");
    size_t totalSize = 0;
    MediaFileUtils::StatDirSize(MEDIA_CUSTOM_RESTORE_DIR, totalSize);
    MEDIA_INFO_LOG("Media_Storage: CustomSize = %{public}" PRId64, static_cast<int64_t>(totalSize));
    return static_cast<int64_t>(totalSize);
}

int64_t PhotoStorageOperation::GetMediaVideoSize()
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoStorageOperation::GetMediaVideoSize");
    size_t totalSize = 0;
    MediaFileUtils::StatDirSize(MEDIA_VIDEO_DIR, totalSize);
    MEDIA_INFO_LOG("Media_Storage: VideoSize = %{public}" PRId64, static_cast<int64_t>(totalSize));
    return static_cast<int64_t>(totalSize);
}

int64_t PhotoStorageOperation::GetPictureSize()
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoStorageOperation::GetPictureSize");
    size_t totalSize = 0;
    MediaFileUtils::StatDirSize(MEDIA_PICTURES_DIR, totalSize);
    MEDIA_INFO_LOG("Media_Storage: PictureSize = %{public}" PRId64, static_cast<int64_t>(totalSize));
    return static_cast<int64_t>(totalSize);
}

int64_t PhotoStorageOperation::GetCameraSize()
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoStorageOperation::GetCameraSize");
    size_t totalSize = 0;
    MediaFileUtils::StatDirSize(MEDIA_CAMERA_DIR, totalSize);
    MEDIA_INFO_LOG("Media_Storage: CameraSize = %{public}" PRId64, static_cast<int64_t>(totalSize));
    return static_cast<int64_t>(totalSize);
}

int64_t PhotoStorageOperation::GetAudioSize()
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoStorageOperation::GetAudioSize");
    size_t totalSize = 0;
    MediaFileUtils::StatDirSize(MEDIA_AUDIO_DIR, totalSize);
    size_t totalAudiosSize = 0;
    MediaFileUtils::StatDirSize(MEDIA_AUDIOS_DIR, totalAudiosSize);
    totalSize += totalAudiosSize;
    MEDIA_INFO_LOG("Media_Storage: AudioSize = %{public}" PRId64, static_cast<int64_t>(totalSize));
    return static_cast<int64_t>(totalSize);
}

int64_t PhotoStorageOperation::GetMetaSize()
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoStorageOperation::GetMetaSize");
    size_t totalSize = 0;
    MediaFileUtils::StatDirSize(MEDIA_META_DIR, totalSize);
    MEDIA_INFO_LOG("Media_Storage: MetaSize = %{public}" PRId64, static_cast<int64_t>(totalSize));
    return static_cast<int64_t>(totalSize);
}

int64_t PhotoStorageOperation::GetBackUpSize()
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoStorageOperation::GetBackUpSize");
    size_t totalSize = 0;
    MediaFileUtils::StatDirSize(MEDIA_BACKUP_DIR, totalSize);
    MEDIA_INFO_LOG("Media_Storage: BackupSize = %{public}" PRId64, static_cast<int64_t>(totalSize));
    return static_cast<int64_t>(totalSize);
}

int64_t PhotoStorageOperation::GetCacheSize()
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoStorageOperation::GetCacheSize");
    size_t totalSize = 0;
    MediaFileUtils::StatDirSize(MEDIA_CACHE_DIR, totalSize);
    MEDIA_INFO_LOG("Media_Storage: CacheSize = %{public}" PRId64, static_cast<int64_t>(totalSize));
    return static_cast<int64_t>(totalSize);
}

int64_t PhotoStorageOperation::GetHighlightSize()
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoStorageOperation::GetHighlightSize");
    size_t totalSize = 0;
    auto start = std::chrono::system_clock::now();
    MediaFileUtils::StatDirSize(MEDIA_HIGHLIGHT_DIR, totalSize);
    auto end = std::chrono::system_clock::now();
    MEDIA_INFO_LOG("Media_Storage: GetHighlightSize size = %{public}zu, cost time = %{public}lld ms",
        totalSize, std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
    this->SaveHighlightSizeToPreferences(totalSize);
    return static_cast<int64_t>(totalSize);
}

std::shared_ptr<NativeRdb::ResultSet> PhotoStorageOperation::QueryHighlightDirectorySize(
    std::shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    int64_t highlightSize = GetHighlightSize();
    std::string sql = "SELECT ? AS dir_size";
    std::vector<NativeRdb::ValueObject> params = {highlightSize};
    return rdbStore->QuerySql(sql, params);
}

int64_t PhotoStorageOperation::GetHighlightSizeFromPreferences()
{
    int32_t errCode;
    std::shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(HIGHLIGHT_CONFIG, errCode);
    CHECK_AND_RETURN_RET_WARN_LOG(prefs != nullptr && errCode == NativePreferences::E_OK, 0,
        "get highlight preferences error: %{public}d", errCode);
    return prefs->GetLong(HIGHLIGHT_DIRECTORY_SIZE, 0);
}

void PhotoStorageOperation::SaveHighlightSizeToPreferences(int64_t size)
{
    int32_t errCode;
    std::shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(HIGHLIGHT_CONFIG, errCode);
    CHECK_AND_RETURN_WARN_LOG(prefs != nullptr && errCode == NativePreferences::E_OK,
        "get highlight preferences error: %{public}d", errCode);
    int32_t output = prefs->PutLong(HIGHLIGHT_DIRECTORY_SIZE, size);
    prefs->FlushSync();
    MEDIA_INFO_LOG("SaveHighlightSizeToPreferences output is %{public}d", output);
}

void PhotoStorageOperation::GetTotalThumbnailSize(std::shared_ptr<MediaLibraryRdbStore> rdbStore,
    TotalThumbnailSizeResult &totalThumbnailSizeResult)
{
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "RdbStore is null");

    std::string sql = "SELECT "
                      "SUM(thumbnail_size) AS total_thumbnail_size, "
                      "COUNT(thumbnail_size) AS thumbnail_count, "
                      "SUM(CASE WHEN thumbnail_size = 0 THEN 1 ELSE 0 END) AS zero_thumbnail_count "
                      "FROM tab_photos_ext";

    // 此处统计为文件逻辑占用大小
    auto statsResult = rdbStore->QuerySql(sql);
    int32_t zeroThumbSizeCount = 0;
    constexpr int32_t ZERO_THUMB_SIZE_COUNT_COLUMN_NUMBER = 2;
    if (statsResult && (statsResult->GoToFirstRow() == NativeRdb::E_OK)) {
        statsResult->GetLong(0, totalThumbnailSizeResult.totalThumbnailSize);
        statsResult->GetInt(1, totalThumbnailSizeResult.thumbnailCount);
        statsResult->GetInt(ZERO_THUMB_SIZE_COUNT_COLUMN_NUMBER, zeroThumbSizeCount);
    }

    // 此接口需返回磁盘占用大小，为每张图片增加12KB大小（经验值），缩略图大小为0的除外
    constexpr int64_t EXTRA_SIZE_PER_PHOTO = 12 * 1024;
    totalThumbnailSizeResult.totalThumbnailSize +=
        (totalThumbnailSizeResult.thumbnailCount - zeroThumbSizeCount) * EXTRA_SIZE_PER_PHOTO;
}

void PhotoStorageOperation::GetTotalEditdataSize(std::shared_ptr<MediaLibraryRdbStore> rdbStore,
    TotalEditdataSizeResult &totalEditdataSizeResult)
{
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "RdbStore is null");

    std::string sql = "SELECT "
                      "SUM(editdata_size) AS total_editdata_size, "
                      "COUNT(editdata_size) AS editdata_count "
                      "FROM tab_photos_ext";

    auto statsResult = rdbStore->QuerySql(sql);
    if (statsResult && (statsResult->GoToFirstRow() == NativeRdb::E_OK)) {
        statsResult->GetLong(0, totalEditdataSizeResult.totalEditdataSize);
        statsResult->GetInt(1, totalEditdataSizeResult.editdataCount);
    }
}

void PhotoStorageOperation::GetLocalPhotoSize(std::shared_ptr<MediaLibraryRdbStore> rdbStore,
    LocalPhotoSizeResult &localPhotoSizeResult)
{
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "RdbStore is null");

    std::string sql = this->SQL_DB_STORAGE_QUERY;
    auto statsResult = rdbStore->QuerySql(sql);
    int32_t mediaType = MediaType::MEDIA_TYPE_DEFAULT;
    while (statsResult && (statsResult->GoToNextRow() == NativeRdb::E_OK)) {
        statsResult->GetInt(0, mediaType);
        if (mediaType == MediaType::MEDIA_TYPE_IMAGE) {
            statsResult->GetLong(1, localPhotoSizeResult.localImageSize);
        } else if (mediaType == MediaType::MEDIA_TYPE_VIDEO) {
            statsResult->GetLong(1, localPhotoSizeResult.localVideoSize);
        }
    }
    statsResult->Close();
}
}  // namespace OHOS::Media