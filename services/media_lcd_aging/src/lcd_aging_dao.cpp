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
#define MLOG_TAG "Lcd_Aging"

#include "lcd_aging_dao.h"

#include <sys/stat.h>

#include "lcd_aging_utils.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_tracer.h"
#include "medialibrary_unistore_manager.h"
#include "photo_file_utils.h"
#include "photo_owner_album_id_operation.h"
#include "result_set_reader.h"
#include "ithumbnail_helper.h"
#include "thumbnail_generate_worker.h"
#include "thumbnail_service.h"
#include "thumbnail_source_loading.h"

namespace OHOS::Media {
using namespace OHOS::Media::CloudSync;

constexpr uint32_t LCD_TO_DOWNLOAD_MASK = 0x1;
// 30天内拍摄或访问的LCD图，不进行老化
constexpr int64_t LCD_USING_THIRTY_DAY = 30LL * 24 * 60 * 60 * 1000;

// 查询本地LCD数量: 1、纯本地图(position = 1)    2、云图(position = 2 OR 3)且LCD已下载(thumb_status为0或2)
const std::string SQL_GET_TOTAL_NUMBER_OF_LCD = "\
    SELECT count(1) AS count \
    FROM Photos \
    WHERE \
        (position = 1 OR \
        ((position = 2 OR position = 3) AND (thumb_status & 1) = 0)) AND \
        clean_flag = 0;";
const std::string SQL_QUERY_AGING_INFO_COLUMN =
    " file_id, data, cloud_id, media_type, orientation, exif_rotate, thumbnail_ready, date_modified, lcd_file_size ";

// 查询可以老化的图片: 纯云图、非收藏，需要排除以下图片: 最近拍摄的图片(LatestPhotos)、人像/合影相册封面、时刻封面
// 收藏的时刻、智慧分析标注的图片、其他原因不可老化的图片
const std::string SQL_QUERY_NOT_AGING_DATA = "\
    WITH LatestPhotos AS ( \
        SELECT file_id \
        FROM Photos \
        WHERE sync_status = 0 \
        AND clean_flag = 0 \
        AND time_pending = 0 \
        AND is_temp = 0 \
        ORDER BY date_taken DESC \
        LIMIT 4000 \
    ), \
    ExcludeFileId AS ( \
        SELECT DISTINCT CAST(SUBSTR(cover_uri, 20, INSTR(SUBSTR(cover_uri, 20), '/') - 1) AS INTEGER) AS file_id \
        FROM AnalysisAlbum \
        WHERE (album_subtype = 4102 OR album_subtype = 4103) AND cover_uri IS NOT NULL AND cover_uri <> '' \
        UNION \
        SELECT DISTINCT CAST( \
            SUBSTR( \
                cover_key, \
                INSTR(cover_key, 'file://media/Photo/') + LENGTH('file://media/Photo/'), \
                INSTR(SUBSTR(cover_key, \
                    INSTR(cover_key, 'file://media/Photo/') + LENGTH('file://media/Photo/')), '/') - 1 \
            ) AS INTEGER \
        ) AS file_id \
        FROM tab_highlight_cover_info \
        WHERE cover_key IS NOT NULL AND cover_key <> '' AND cover_key LIKE '%file://media/Photo/%' \
        UNION \
        SELECT DISTINCT map_asset AS file_id \
        FROM AnalysisPhotoMap \
        WHERE map_album IN ( \
            SELECT album_id FROM tab_highlight_album WHERE is_favorite <> 0 AND highlight_status = 1 \
        ) \
        UNION \
        SELECT photo_id AS file_id \
        FROM tab_photos_ext \
        WHERE lcd_using_status <> 0 \
        UNION \
        SELECT file_id \
        FROM LatestPhotos \
        UNION \
        SELECT file_id \
        FROM Photos \
        WHERE file_id IN ({0}) \
    ) ";

// 公共 WHERE 条件
const std::string SQL_AGING_WHERE_CONDITION = "\
    P.sync_status = 0 \
    AND P.clean_flag = 0 \
    AND P.time_pending = 0 \
    AND P.is_temp = 0 \
    AND P.position = 2 \
    AND P.is_favorite = 0 \
    AND (P.thumb_status & 1) = 0 \
    AND NOT EXISTS (SELECT 1 FROM ExcludeFileId WHERE file_id = P.file_id) \
    AND P.real_lcd_visit_time < ? \
    AND P.date_taken < ? ";

// 查询回收站可老化图片
const std::string SQL_QUERY_AGING_LCD_DATA_TRASHED = SQL_QUERY_NOT_AGING_DATA +
    " SELECT " + SQL_QUERY_AGING_INFO_COLUMN +
    " FROM Photos P WHERE " + SQL_AGING_WHERE_CONDITION + " AND P.date_trashed > 0 LIMIT ?;";

// 查询非回收站可老化图片
const std::string SQL_QUERY_AGING_LCD_DATA_NOT_TRASHED = SQL_QUERY_NOT_AGING_DATA +
    " SELECT " + SQL_QUERY_AGING_INFO_COLUMN +
    " FROM Photos P WHERE " + SQL_AGING_WHERE_CONDITION + " AND P.date_trashed = 0 LIMIT ?;";

// 统计可老化图片总数
const std::string SQL_COUNT_AGING_LCD_DATA = SQL_QUERY_NOT_AGING_DATA +
    " SELECT count(1) AS count FROM Photos P WHERE " + SQL_AGING_WHERE_CONDITION + ";";

const std::string SQL_QUERY_ANALYSIS_AGING_DATA =
    "WITH ExcludeFileId AS ( \
        SELECT DISTINCT CAST(SUBSTR(cover_uri, 20, INSTR(SUBSTR(cover_uri, 20), '/') - 1) AS INTEGER) AS file_id \
        FROM AnalysisAlbum \
        WHERE (album_subtype = 4102 OR album_subtype = 4103) AND cover_uri IS NOT NULL AND cover_uri <> '' \
        UNION \
        SELECT DISTINCT CAST( \
            SUBSTR( \
                cover_key, \
                INSTR(cover_key, 'file://media/Photo/') + LENGTH('file://media/Photo/'), \
                INSTR(SUBSTR(cover_key, \
                    INSTR(cover_key, 'file://media/Photo/') + LENGTH('file://media/Photo/')), '/') - 1 \
            ) AS INTEGER \
        ) AS file_id \
        FROM tab_highlight_cover_info \
        WHERE cover_key IS NOT NULL AND cover_key <> '' AND cover_key LIKE '%file://media/Photo/%' \
        UNION \
        SELECT DISTINCT map_asset AS file_id \
        FROM AnalysisPhotoMap \
        WHERE map_album IN ( \
            SELECT album_id FROM tab_highlight_album WHERE is_favorite <> 0 AND highlight_status = 1 \
        ) \
    ) ";

const std::string SQL_UPDATE_THUMB_STATUS = "\
    UPDATE Photos \
        SET thumb_status = thumb_status | ? \
    WHERE file_id IN ({0});";
const std::string SQL_REVERT_THUMB_STATUS = "\
    UPDATE Photos \
        SET thumb_status = thumb_status & ? \
    WHERE file_id IN ({0});";

int32_t LcdAgingDao::GetCurrentNumberOfLcd(int64_t &lcdNumber)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetCurrentNumberOfLcd");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "GetCurrentNumberOfLcd Failed to get rdbStore.");
    auto resultSet = rdbStore->QuerySql(SQL_GET_TOTAL_NUMBER_OF_LCD);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK, E_ERR,
        "GetCurrentNumberOfLcd Failed to query number of lcd");
    lcdNumber = GetInt64Val("count", resultSet);
    resultSet->Close();
    return E_OK;
}

void LcdAgingDao::ReadLcdAgingInfoFromResultSet(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
    LcdAgingFileInfo &lcdAgingInfo)
{
    constexpr int32_t INDEX_FILE_ID = 0;
    constexpr int32_t INDEX_PATH = 1;
    constexpr int32_t INDEX_CLOUD_ID = 2;
    constexpr int32_t INDEX_MEDIA_TYPE = 3;
    constexpr int32_t INDEX_ORIENTATION = 4;
    constexpr int32_t INDEX_EXIF_ROTATE = 5;
    constexpr int32_t INDEX_THUMBNAIL_READY = 6;
    constexpr int32_t INDEX_DATE_MODIFIED = 7;
    constexpr int32_t INDEX_LCD_FILE_SIZE = 8;

    CHECK_AND_RETURN_LOG(resultSet != nullptr, "resultSet is nullptr");
    resultSet->GetInt(INDEX_FILE_ID, lcdAgingInfo.fileId);
    resultSet->GetString(INDEX_PATH, lcdAgingInfo.path);
    resultSet->GetString(INDEX_CLOUD_ID, lcdAgingInfo.cloudId);
    resultSet->GetInt(INDEX_MEDIA_TYPE, lcdAgingInfo.mediaType);
    resultSet->GetInt(INDEX_ORIENTATION, lcdAgingInfo.orientation);
    resultSet->GetInt(INDEX_EXIF_ROTATE, lcdAgingInfo.exifRotate);
    resultSet->GetLong(INDEX_THUMBNAIL_READY, lcdAgingInfo.thumbnailReady);
    resultSet->GetLong(INDEX_DATE_MODIFIED, lcdAgingInfo.dateModified);
    resultSet->GetInt(INDEX_LCD_FILE_SIZE, lcdAgingInfo.lcdFileSize);
}

int32_t LcdAgingDao::QueryAgingLcdDataInternal(const int32_t size, const std::vector<std::string> &notAgingFileIds,
    const std::string &sql, const char *logPrefix, std::vector<LcdAgingFileInfo> &lcdAgingFileInfoList)
{
    MediaLibraryTracer tracer;
    tracer.Start("QueryAgingLcdDataInternal");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_DB_FAIL, "%{public}s Failed to get rdbStore.", logPrefix);

    int64_t lcdFilterTime = MediaFileUtils::UTCTimeMilliSeconds() - LCD_USING_THIRTY_DAY;
    std::vector<NativeRdb::ValueObject> bindArgs = { lcdFilterTime, lcdFilterTime, size };
    std::string fileIdNotIn = PhotoOwnerAlbumIdOperation().ToStringWithCommaAndQuote(notAgingFileIds);
    std::string execSql = PhotoOwnerAlbumIdOperation().FillParams(sql, {fileIdNotIn});
    auto resultSet = rdbStore->QuerySql(execSql, bindArgs);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_DB_FAIL, "%{public}s Failed to query.", logPrefix);

    FillLcdAgingInfoListFromResultSet(resultSet, lcdAgingFileInfoList);
    resultSet->Close();

    MEDIA_INFO_LOG("size of lcdAgingFileInfoList (%{public}s) is %{public}zu", logPrefix, lcdAgingFileInfoList.size());
    return E_OK;
}

bool LcdAgingDao::CheckLocalLcd(LcdAgingFileInfo &agingFileInfo)
{
    MediaLibraryTracer tracer;
    tracer.Start("CheckLocalLcd");
    std::string localLcdPath = agingFileInfo.hasExThumbnail ? agingFileInfo.localLcdExPath : agingFileInfo.localLcdPath;
    MEDIA_DEBUG_LOG("CheckLocalLcd, path: %{public}s", localLcdPath.c_str());
    struct stat statInfo = { 0 };
    bool isValid = !localLcdPath.empty();
    isValid = isValid && (stat(localLcdPath.c_str(), &statInfo) == E_SUCCESS);
    CHECK_AND_RETURN_RET_LOG(isValid, false,
        "local lcd not exist, path: %{public}s", localLcdPath.c_str());
    agingFileInfo.lcdFileSize = statInfo.st_size;
    RegenerateAstcWithLocal(agingFileInfo);
    return true;
}

int32_t LcdAgingDao::RegenerateAstcWithLocal(const LcdAgingFileInfo &agingFileInfo)
{
    MediaLibraryTracer tracer;
    tracer.Start("RegenerateAstcWithLocal");
    bool isValid = agingFileInfo.thumbnailReady == static_cast<int64_t>(ThumbnailReady::THUMB_NEED_REGENERATE_ASTC);
    CHECK_AND_RETURN_RET(isValid, E_ERR);
    std::string astcPath = GetThumbnailPath(agingFileInfo.path, THUMBNAIL_THUMB_ASTC_SUFFIX);
    CHECK_AND_RETURN_RET_LOG(!astcPath.empty(), E_ERR, "astcPath is empty, fileId: %{public}d", agingFileInfo.fileId);
    isValid = !MediaFileUtils::IsFileExists(astcPath);
    CHECK_AND_RETURN_RET(isValid, E_ERR);
    auto thumbnailService = ThumbnailService::GetInstance();
    CHECK_AND_RETURN_RET_LOG(thumbnailService != nullptr, E_ERR, "thumbnailService is null");
    MEDIA_INFO_LOG("begin to sync regenerate astc with local, fileId: %{public}d", agingFileInfo.fileId);
    return thumbnailService->SyncRegenerateAstcWithLocal(std::to_string(agingFileInfo.fileId));
}

int32_t LcdAgingDao::QueryAgingLcdDataTrashed(const int32_t size,
    const std::vector<std::string> &notAgingFileIds, std::vector<LcdAgingFileInfo> &lcdAgingFileInfoList)
{
    return QueryAgingLcdDataInternal(size, notAgingFileIds, SQL_QUERY_AGING_LCD_DATA_TRASHED,
        "trashed", lcdAgingFileInfoList);
}

int32_t LcdAgingDao::QueryAgingLcdDataNotTrashed(const int32_t size,
    const std::vector<std::string> &notAgingFileIds, std::vector<LcdAgingFileInfo> &lcdAgingFileInfoList)
{
    return QueryAgingLcdDataInternal(size, notAgingFileIds, SQL_QUERY_AGING_LCD_DATA_NOT_TRASHED,
        "notTrashed", lcdAgingFileInfoList);
}

int32_t LcdAgingDao::SetLcdNotDownloadStatus(const std::vector<std::string> &fileIds)
{
    CHECK_AND_RETURN_RET_LOG(!fileIds.empty(), E_ERR, "fileIds is empty");
    MediaLibraryTracer tracer;
    tracer.Start("SetLcdNotDownloadStatus");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_DB_FAIL, "SetLcdNotDownloadStatus Failed to get rdbStore.");
    
    std::vector<std::string> params = { PhotoOwnerAlbumIdOperation().ToStringWithComma(fileIds) };
    std::string execSql = PhotoOwnerAlbumIdOperation().FillParams(SQL_UPDATE_THUMB_STATUS, params);
    std::vector<NativeRdb::ValueObject> bindArgs = {static_cast<int32_t>(LCD_TO_DOWNLOAD_MASK)};
    int32_t ret = rdbStore->ExecuteSql(execSql, bindArgs);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_ERR, "Failed to SetLcdNotDownloadStatus.");
    return E_OK;
}

int32_t LcdAgingDao::RevertToLcdDownloadStatus(const std::vector<std::string> &fileIds)
{
    CHECK_AND_RETURN_RET_LOG(!fileIds.empty(), E_ERR, "fileIds is empty");
    MediaLibraryTracer tracer;
    tracer.Start("RevertToLcdDownloadStatus");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_DB_FAIL, "RevertToLcdDownloadStatus Failed to get rdbStore.");

    std::vector<std::string> params = { PhotoOwnerAlbumIdOperation().ToStringWithComma(fileIds) };
    std::string execSql = PhotoOwnerAlbumIdOperation().FillParams(SQL_REVERT_THUMB_STATUS, params);
    std::vector<NativeRdb::ValueObject> bindArgs = {static_cast<int32_t>(~LCD_TO_DOWNLOAD_MASK)};
    int32_t ret = rdbStore->ExecuteSql(execSql, bindArgs);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_ERR, "Failed to RevertToLcdDownloadStatus.");
    return E_OK;
}

int32_t LcdAgingDao::UpdateLcdFileSize(const std::vector<LcdAgingFileInfo> &agingFileInfos)
{
    MediaLibraryTracer tracer;
    tracer.Start("UpdateLcdFileSize");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_DB_FAIL, "UpdateLcdFileSize Failed to get rdbStore.");

    std::string updateSql = "UPDATE Photos SET lcd_file_size = CASE file_id ";
    std::string fileIdList = "";
    for (const auto &agingFileInfo : agingFileInfos) {
        CHECK_AND_CONTINUE(agingFileInfo.needFixLcdFileSize);
        updateSql +=
            " WHEN " + std::to_string(agingFileInfo.fileId) + " THEN " + std::to_string(agingFileInfo.lcdFileSize);
        fileIdList += std::to_string(agingFileInfo.fileId) + ",";
    }
    CHECK_AND_RETURN_RET(!fileIdList.empty(), E_OK);
    fileIdList.pop_back();
    updateSql += " ELSE lcd_file_size END WHERE file_id IN ( " + fileIdList + " );";

    int32_t ret = rdbStore->ExecuteSql(updateSql);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "failed to execute sql: %{public}s, ret: %{public}d",
        updateSql.c_str(), ret);
    return ret;
}

int32_t LcdAgingDao::QueryAgingLcdDataByFileIds(const std::vector<int64_t> &fileIds,
    std::vector<LcdAgingFileInfo> &lcdAgingFileInfoList)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_DB_FAIL, "QueryAgingLcdDataByFileIds Failed to get rdbStore.");
    CHECK_AND_RETURN_RET_LOG(!fileIds.empty(), E_OK, "fileIds is empty");

    std::vector<std::string> fileIdStrs;
    for (auto fileId : fileIds) {
        fileIdStrs.push_back(std::to_string(fileId));
    }
    std::string fileIdStr = PhotoOwnerAlbumIdOperation().ToStringWithComma(fileIdStrs);
    std::string querySql = SQL_QUERY_ANALYSIS_AGING_DATA + " SELECT " + SQL_QUERY_AGING_INFO_COLUMN +
        " FROM Photos P \
        WHERE P.file_id IN (" + fileIdStr + ") \
            AND P.sync_status = 0 \
            AND P.clean_flag = 0 \
            AND P.time_pending = 0 \
            AND P.is_temp = 0 \
            AND P.position = 2 \
            AND P.is_favorite = 0 \
            AND (P.thumb_status & 1) = 0 \
            AND NOT EXISTS (SELECT 1 FROM ExcludeFileId WHERE file_id = P.file_id);";

    auto resultSet = rdbStore->QuerySql(querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_DB_FAIL, "QueryAgingLcdDataByFileIds Failed to query.");

    FillLcdAgingInfoListFromResultSet(resultSet, lcdAgingFileInfoList);
    resultSet->Close();

    MEDIA_INFO_LOG("QueryAgingLcdDataByFileIds found %{public}zu files", lcdAgingFileInfoList.size());
    return E_OK;
}

void LcdAgingDao::FillLcdAgingInfoListFromResultSet(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
    std::vector<LcdAgingFileInfo> &lcdAgingFileInfoList)
{
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "resultSet is nullptr");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        LcdAgingFileInfo lcdAgingInfo;
        ReadLcdAgingInfoFromResultSet(resultSet, lcdAgingInfo);
        lcdAgingInfo.needFixLcdFileSize = (lcdAgingInfo.lcdFileSize <= 0);
        lcdAgingInfo.hasExThumbnail = LcdAgingUtils::HasExThumbnail(lcdAgingInfo);
        lcdAgingInfo.localLcdPath = PhotoFileUtils::GetLocalLcdPath(lcdAgingInfo.path);
        lcdAgingInfo.localLcdExPath = lcdAgingInfo.hasExThumbnail ?
            PhotoFileUtils::GetLocalLcdExPath(lcdAgingInfo.path) : "";
        if (lcdAgingInfo.lcdFileSize <= 0 ||
            lcdAgingInfo.thumbnailReady == static_cast<int64_t>(ThumbnailReady::THUMB_NEED_REGENERATE_ASTC)) {
            this->CheckLocalLcd(lcdAgingInfo);
        }
        lcdAgingFileInfoList.emplace_back(lcdAgingInfo);
    }
}

int64_t LcdAgingDao::GetAgingLcdCount()
{
    MediaLibraryTracer tracer;
    tracer.Start("GetAgingLcdCount");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "GetAgingLcdCount Failed to get rdbStore.");

    int64_t lcdFilterTime = MediaFileUtils::UTCTimeMilliSeconds() - LCD_USING_THIRTY_DAY;
    std::vector<NativeRdb::ValueObject> bindArgs = { lcdFilterTime, lcdFilterTime };
    std::string execSql = PhotoOwnerAlbumIdOperation().FillParams(SQL_COUNT_AGING_LCD_DATA, {"-1"});
    auto resultSet = rdbStore->QuerySql(execSql, bindArgs);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK, E_ERR,
        "GetAgingLcdCount Failed to query.");

    int64_t count = GetInt64Val("count", resultSet);
    resultSet->Close();
    MEDIA_INFO_LOG("GetAgingLcdCount count: %{public}" PRId64, count);
    return count;
}
}  // namespace OHOS::Media