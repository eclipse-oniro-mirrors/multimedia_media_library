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

#include "lcd_aging_service.h"
#include "lcd_aging_dao.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_tracer.h"
#include "medialibrary_unistore_manager.h"
#include "photo_owner_album_id_operation.h"
#include "result_set_utils.h"
#include "lcd_aging_worker.h"
#include "parameters.h"

namespace OHOS::Media {
const int32_t minLcdNums = 50000;
const int32_t thresholdLcdNums = 40000;
const int64_t LCD_DOWNLOAD_THIRTY_DAYS = 30 * 24 * 60 * 60 *1000LL;
static const std::string MEDIA_RESTORE_FLAG = "multimedia.medialibrary.restoreFlag";
static const std::string MEDIA_BACKUP_FLAG = "multimedia.medialibrary.backupFlag";
const std::string LcdAgingService::SQL_MARK_RECENT_LCD_PHOTOS = "\
        WITH HighlightAlbums AS ( \
            SELECT album_id \
            FROM tab_highlight_album \
            WHERE highlight_status = 1 \
            ORDER BY max_date_added DESC \
            LIMIT 50\
        ), \
        HighlightPhotos AS ( \
            SELECT DISTINCT map_asset AS file_id \
            FROM AnalysisPhotoMap \
            WHERE map_album IN (SELECT album_id FROM HighlightAlbums) \
        ), \
        AnalysisSelectionPhotos AS ( \
            SELECT file_id \
            FROM tab_analysis_selection \
            WHERE month_flag = 1 OR year_flag = 1 \
        ) \
        UPDATE tab_photos_ext \
        SET lcd_using_status = 16, lcd_file_modify_time = strftime('%s', 'now') * 1000 \
        WHERE photo_id IN ( \
            SELECT file_id FROM HighlightPhotos \
            UNION \
            SELECT file_id FROM AnalysisSelectionPhotos \
        );";
const std::string LcdAgingService::SQL_GET_TOTAL_NUMBER_OF_LCD = "\
        SELECT count(1) AS count \
        FROM Photos \
        WHERE \
            (position = 1 OR \
            ((position = 2 OR position = 3) AND (thumb_status & 1) = 0)) AND \
            clean_flag = 0;";
const std::string LcdAgingService::SQL_GET_CAN_OPTIMIZE_OF_LCD = "\
        WITH LatestPhotos AS ( \
            SELECT file_id \
            FROM Photos \
            WHERE sync_status = 0 \
            AND clean_flag = 0 \
            AND time_pending = 0 \
            AND is_temp = 0 \
            ORDER BY date_taken DESC \
            LIMIT 4000\
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
        ) \
        SELECT \
        file_id, data, cloud_id, media_type, orientation, exif_rotate, thumbnail_ready, date_modified, lcd_file_size \
        FROM Photos P \
        WHERE P.sync_status = 0 \
            AND P.clean_flag = 0 \
            AND P.time_pending = 0 \
            AND P.is_temp = 0 \
            AND P.position = 2 \
            AND P.is_favorite = 0 \
            AND (p.thumb_status & 1) = 0 \
            AND NOT EXISTS (SELECT 1 FROM ExcludeFileId WHERE file_id = P.file_id) \
            AND P.real_lcd_visit_time < ? \
            AND P.date_taken < ? ";

LcdAgingService &LcdAgingService::GetInstance()
{
    static LcdAgingService instance;
    return instance;
}

std::atomic<bool> LcdAgingService::isMarkingLcdStatus_(false);

void LcdAgingService::SetMarkingLcdStatus(bool status)
{
    isMarkingLcdStatus_.store(status, std::memory_order_relaxed);
    MEDIA_INFO_LOG("Set LCD marking status: %{public}d", status);
}

bool LcdAgingService::IsMarkingLcdStatus()
{
    return isMarkingLcdStatus_.load(std::memory_order_relaxed);
}

int32_t LcdAgingService::HandleCanPerformDeepOptimizeSpace(bool &result)
{
    MEDIA_INFO_LOG("Enter HandleCanPerformDeepOptimizeSpace");
    result = false;
    int64_t lcdCount = GetLcdImageCount();
    if (lcdCount < minLcdNums) {
        MEDIA_INFO_LOG("LCD count less than 50000, count: %{public}ld", (long)lcdCount);
        return E_OK;
    }

    if (IsCloningOrRestoring()) {
        MEDIA_INFO_LOG("Device is cloning or restoring");
        return E_OK;
    }

    if (IsCleaningLcd()) {
        MEDIA_INFO_LOG("LCD cleaning is in progress");
        return E_OK;
    }

    if (IsMarkingLcdStatus()) {
        MEDIA_INFO_LOG("LCD status marking is in progress");
        return E_OK;
    }

    bool hasReleasableLcd = HasReleasableLcdImages();
    result = hasReleasableLcd;
    
    MEDIA_INFO_LOG("Can perform deep optimize: %{public}d", result);
    return E_OK;
}

int64_t LcdAgingService::GetLcdImageCount()
{
    MediaLibraryTracer tracer;
    tracer.Start("GetCurrentNumberOfLcd");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "GetCurrentNumberOfLcd Failed to get rdbStore");
    auto resultSet = rdbStore->QuerySql(SQL_GET_TOTAL_NUMBER_OF_LCD);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK, E_ERR,
        "GetCurrentNumberOfLcd Failed to query number of lcd");
    int64_t count = GetInt64Val("count", resultSet);
    resultSet->Close();
    return count;
}

bool LcdAgingService::IsCloningOrRestoring()
{
    return system::GetParameter(MEDIA_BACKUP_FLAG, "0") != "0" ||
        system::GetParameter(MEDIA_RESTORE_FLAG, "0") != "0";
}

bool LcdAgingService::IsCleaningLcd()
{
    return LcdAgingWorker::GetInstance().IsRunning();
}

bool LcdAgingService::HasReleasableLcdImages()
{
    MediaLibraryTracer tracer;
    tracer.Start("HasReleasableLcdImages");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "HasReleasableLcdImages Failed to get rdbStore");

    constexpr int64_t LCD_DOWNLOAD_THIRTY_DAYS = 30 * 24 * 60 * 60;
    int64_t currentTime = MediaFileUtils::UTCTimeSeconds();
    int64_t lcdFileTime = currentTime - LCD_DOWNLOAD_THIRTY_DAYS;

    std::vector<NativeRdb::ValueObject> bindArgs = { lcdFileTime, lcdFileTime };
    std::string execSql = PhotoOwnerAlbumIdOperation().FillParams(SQL_GET_CAN_OPTIMIZE_OF_LCD, { "" });
    execSql += " LIMIT 1 ";

    auto resultSet = rdbStore->QuerySql(execSql, bindArgs);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "HasReleasableLcdImages Failed to query");
    
    bool hasReleasable = (resultSet->GoToFirstRow() == NativeRdb::E_OK);
    resultSet->Close();

    MEDIA_INFO_LOG("Has releasable LCD images: %{public}d", hasReleasable);
    return hasReleasable;
}

int32_t LcdAgingService::HandleGetDeepOptimizableSpace(int64_t &space)
{
    MEDIA_INFO_LOG("Enter HandleGetDeepOptimizableSpace");

    int32_t lcdCount = GetLcdImageCount();
    int32_t canOptimizeLcdNums = lcdCount - thresholdLcdNums;

    MediaLibraryTracer tracer;
    tracer.Start("HandleGetDeepOptimizableSpace");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "HandleGetDeepOptimizableSpace Failed to get rdbStore");

    int64_t currentTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t lcdFileTime = currentTime - LCD_DOWNLOAD_THIRTY_DAYS;

    std::vector<NativeRdb::ValueObject> bindArgs = { lcdFileTime, lcdFileTime };
    std::string innerSql = PhotoOwnerAlbumIdOperation().FillParams(SQL_GET_CAN_OPTIMIZE_OF_LCD, { "" });
    innerSql += " LIMIT " + std::to_string(canOptimizeLcdNums);

    std::string execSql = "SELECT SUM(COALESCE(NULLIF(lcd_file_size, 0), 500000))" +
        std::string("AS total_size FROM (") + innerSql + ") AS releasable_lcd";
    
    auto resultSet = rdbStore->QuerySql(execSql, bindArgs);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "HandleGetDeepOptimizableSpace Failed to query");

    if (resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        space = GetInt64Val("total_size", resultSet);
    }
    resultSet->Close();

    MEDIA_INFO_LOG("Get deep optimizable space: %{public}ld", (long)space);
    return E_OK;
}

int32_t LcdAgingService::MarkRecentLcdPhotos(const std::shared_ptr<NativeRdb::RdbStore> &rdbStore)
{
    MEDIA_INFO_LOG("Enter MarkRecentLcdPhotos");
    SetMarkingLcdStatus(true);

    MediaLibraryTracer tracer;
    tracer.Start("MarkRecentLcdPhotos");
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("MarkRecentLcdPhotos Failed to get rdbStore");
        SetMarkingLcdStatus(false);
        return E_ERR;
    }

    int32_t result = rdbStore->ExecuteSql(SQL_MARK_RECENT_LCD_PHOTOS);
    if (result != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("MarkRecentLcdPhotos Failed to excute SQL: %{public}d", result);
        SetMarkingLcdStatus(false);
        return E_ERR;
    }

    SetMarkingLcdStatus(false);
    MEDIA_INFO_LOG("MarkRecentLcdPhotos completed successfully");
    return E_OK;
}
} //namespace OHOS::Media