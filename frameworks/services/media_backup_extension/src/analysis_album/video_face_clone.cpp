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

#include "video_face_clone.h"

#include "backup_database_utils.h"
#include "backup_const.h"
#include "backup_const_column.h"
#include "backup_dfx_utils.h"
#include "backup_file_utils.h"
#include "backup_log_utils.h"
#include "database_report.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_library_db_upgrade.h"
#include "media_log.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_errno.h"
#include "medialibrary_type_const.h"
#include "rdb_store.h"
#include "result_set_utils.h"

namespace OHOS::Media {
VideoFaceClone::VideoFaceClone(
    const std::shared_ptr<NativeRdb::RdbStore>& sourceRdb,
    const std::shared_ptr<NativeRdb::RdbStore>& destRdb,
    const std::unordered_map<int32_t, PhotoInfo>& photoInfoMap
    )
    : sourceRdb_(sourceRdb),
      destRdb_(destRdb),
      photoInfoMap_(photoInfoMap)
{
}

bool VideoFaceClone::CloneVideoFaceInfo()
{
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    std::vector<int32_t> oldFileIds;
    oldFileIds.reserve(photoInfoMap_.size());
    std::vector<int32_t> newFileIds;
    newFileIds.reserve(photoInfoMap_.size());

    for (const auto& pair : photoInfoMap_) {
        oldFileIds.push_back(pair.first);
        newFileIds.push_back(pair.second.fileIdNew);
    }

    if (oldFileIds.empty()) {
        MEDIA_INFO_LOG("photoInfoMap_ is empty, no video face entries to clone.");
        migrateVideoFaceTotalTimeCost_ = MediaFileUtils::UTCTimeMilliSeconds() - start;
        return true;
    }

    std::string fileIdOldInClause = "(" + BackupDatabaseUtils::JoinValues<int>(oldFileIds, ", ") + ")";

    std::string querySql = QUERY_VIDEO_FACE_COUNT;
    querySql += " WHERE " + VIDEO_FACE_COL_FILE_ID + " IN " + fileIdOldInClause;
    int32_t totalNumber = BackupDatabaseUtils::QueryInt(sourceRdb_, querySql, CUSTOM_COUNT);
    MEDIA_INFO_LOG("QueryVideoFaceTotalNumber, totalNumber = %{public}d", totalNumber);
    if (totalNumber <= 0) {
        int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
        migrateVideoFaceTotalTimeCost_ += end - start;
        return true;
    }

    std::vector<std::string> commonColumn = BackupDatabaseUtils::GetCommonColumnInfos(sourceRdb_, destRdb_,
        VISION_VIDEO_FACE_TABLE);
    std::vector<std::string> commonColumns = BackupDatabaseUtils::filterColumns(commonColumn,
        EXCLUDED_VIDEO_FACE_COLUMNS);

    CHECK_AND_RETURN_RET_LOG(!commonColumns.empty(),
        false, "No common columns found for video face table after exclusion.");

    DeleteExistingVideoFaceData(newFileIds);

    for (int32_t offset = 0; offset < totalNumber; offset += QUERY_COUNT) {
        std::vector<VideoFaceTbl> videoFaceTbls = QueryVideoFaceTbl(offset, fileIdOldInClause, commonColumns);

        if (videoFaceTbls.empty()) {
            MEDIA_WARN_LOG("Query returned empty result for offset %{public}d", offset);
            continue;
        }

        std::vector<VideoFaceTbl> processedVideoFaces = ProcessVideoFaceTbls(videoFaceTbls);
        BatchInsertVideoFaces(processedVideoFaces);
    }
    UpdateAnalysisTotalTblVideoFaceStatus(destRdb_, newFileIds);
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    migrateVideoFaceTotalTimeCost_ += end - start;
    MEDIA_INFO_LOG("VideoFaceClone::CloneVideoFaceInfo completed. Migrated %{public}lld records. "
        "Total time: %{public}lld ms",
        (long long)migrateVideoFaceNum_, (long long)migrateVideoFaceTotalTimeCost_);
    return true;
}

std::vector<VideoFaceTbl> VideoFaceClone::QueryVideoFaceTbl(int32_t offset, std::string &fileIdClause,
    const std::vector<std::string> &commonColumns)
{
    std::vector<VideoFaceTbl> result;
    result.reserve(QUERY_COUNT);

    std::string inClause = BackupDatabaseUtils::JoinValues<std::string>(commonColumns, ", ");
    std::string querySql =
        "SELECT " + inClause +
        " FROM " + VISION_VIDEO_FACE_TABLE;
    querySql += " WHERE " + VIDEO_FACE_COL_FILE_ID + " IN " + fileIdClause;
    querySql += " LIMIT " + std::to_string(offset) + ", " + std::to_string(QUERY_COUNT);

    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(sourceRdb_, querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, result, "Query resultSet is null.");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        VideoFaceTbl videoFaceTbl;
        ParseVideoFaceResultSet(resultSet, videoFaceTbl);
        result.emplace_back(videoFaceTbl);
    }

    resultSet->Close();
    return result;
}

void VideoFaceClone::ParseVideoFaceResultSet(const std::shared_ptr<NativeRdb::ResultSet>& resultSet,
    VideoFaceTbl& videoFaceTbl)
{
    videoFaceTbl.file_id = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, VIDEO_FACE_COL_FILE_ID);
    videoFaceTbl.face_id = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, VIDEO_FACE_COL_FACE_ID);
    videoFaceTbl.tag_id = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, VIDEO_FACE_COL_TAG_ID);
    videoFaceTbl.scale_x = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, VIDEO_FACE_COL_SCALE_X);
    videoFaceTbl.scale_y = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, VIDEO_FACE_COL_SCALE_Y);
    videoFaceTbl.scale_width = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        VIDEO_FACE_COL_SCALE_WIDTH);
    videoFaceTbl.scale_height = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        VIDEO_FACE_COL_SCALE_HEIGHT);
    videoFaceTbl.landmarks = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, VIDEO_FACE_COL_LANDMARKS);
    videoFaceTbl.pitch = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, VIDEO_FACE_COL_PITCH);
    videoFaceTbl.yaw = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, VIDEO_FACE_COL_YAW);
    videoFaceTbl.roll = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, VIDEO_FACE_COL_ROLL);
    videoFaceTbl.prob = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, VIDEO_FACE_COL_PROB);
    videoFaceTbl.total_faces = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, VIDEO_FACE_COL_TOTAL_FACES);
    videoFaceTbl.frame_id = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, VIDEO_FACE_COL_FRAME_ID);
    videoFaceTbl.frame_timestamp = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        VIDEO_FACE_COL_FRAME_TIMESTAMP);
    videoFaceTbl.tracks = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, VIDEO_FACE_COL_TRACKS);
    videoFaceTbl.algo_version = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        VIDEO_FACE_COL_ALGO_VERSION);
    videoFaceTbl.features = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, VIDEO_FACE_COL_FEATURES);
    videoFaceTbl.analysis_version = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        VIDEO_FACE_COL_ANALYSIS_VERSION);
}

std::vector<VideoFaceTbl> VideoFaceClone::ProcessVideoFaceTbls(const std::vector<VideoFaceTbl>& videoFaceTbls)
{
    CHECK_AND_RETURN_RET_LOG(!videoFaceTbls.empty(), {}, "video faces tbl empty");

    std::vector<VideoFaceTbl> videoFaceNewTbls;
    videoFaceNewTbls.reserve(videoFaceTbls.size());

    for (const auto& videoFaceTbl : videoFaceTbls) {
        if (videoFaceTbl.file_id.has_value()) {
            int32_t oldFileId = videoFaceTbl.file_id.value();
            const auto it = photoInfoMap_.find(oldFileId);
            if (it != photoInfoMap_.end()) {
                VideoFaceTbl updatedFace = videoFaceTbl;
                updatedFace.file_id = it->second.fileIdNew;
                videoFaceNewTbls.push_back(std::move(updatedFace));
            } else {
                MEDIA_WARN_LOG("Original file_id %{public}d not found in photoInfoMap_, skipping.", oldFileId);
            }
        }
    }
    return videoFaceNewTbls;
}

void VideoFaceClone::BatchInsertVideoFaces(const std::vector<VideoFaceTbl>& videoFaceTbls)
{
    std::vector<NativeRdb::ValuesBucket> valuesBuckets;
    std::unordered_set<int32_t> fileIdSet;
    valuesBuckets.reserve(videoFaceTbls.size());
    for (const auto& videoFaceTbl : videoFaceTbls) {
        valuesBuckets.push_back(CreateValuesBucketFromVideoFaceTbl(videoFaceTbl));
    }

    int64_t rowNum = 0;
    int32_t ret = BatchInsertWithRetry(VISION_VIDEO_FACE_TABLE, valuesBuckets, rowNum);
    CHECK_AND_RETURN_LOG(ret == E_OK, "Failed to batch insert video faces");

    for (const auto& videoFaceTbl : videoFaceTbls) {
        if (videoFaceTbl.file_id.has_value()) {
            fileIdSet.insert(videoFaceTbl.file_id.value());
        }
    }

    migrateVideoFaceNum_ += rowNum;
    migrateVideoFaceFileNumber_ += fileIdSet.size();
}

NativeRdb::ValuesBucket VideoFaceClone::CreateValuesBucketFromVideoFaceTbl(const VideoFaceTbl& videoFaceTbl)
{
    NativeRdb::ValuesBucket values;
    PutIfPresent(values, VIDEO_FACE_COL_FILE_ID, videoFaceTbl.file_id);
    PutIfPresent(values, VIDEO_FACE_COL_FACE_ID, videoFaceTbl.face_id);
    PutIfPresent(values, VIDEO_FACE_COL_TAG_ID, videoFaceTbl.tag_id);
    PutIfPresent(values, VIDEO_FACE_COL_SCALE_X, videoFaceTbl.scale_x);
    PutIfPresent(values, VIDEO_FACE_COL_SCALE_Y, videoFaceTbl.scale_y);
    PutIfPresent(values, VIDEO_FACE_COL_SCALE_WIDTH, videoFaceTbl.scale_width);
    PutIfPresent(values, VIDEO_FACE_COL_SCALE_HEIGHT, videoFaceTbl.scale_height);
    PutIfPresent(values, VIDEO_FACE_COL_LANDMARKS, videoFaceTbl.landmarks);
    PutIfPresent(values, VIDEO_FACE_COL_PITCH, videoFaceTbl.pitch);
    PutIfPresent(values, VIDEO_FACE_COL_YAW, videoFaceTbl.yaw);
    PutIfPresent(values, VIDEO_FACE_COL_ROLL, videoFaceTbl.roll);
    PutIfPresent(values, VIDEO_FACE_COL_PROB, videoFaceTbl.prob);
    PutIfPresent(values, VIDEO_FACE_COL_TOTAL_FACES, videoFaceTbl.total_faces);
    PutIfPresent(values, VIDEO_FACE_COL_FRAME_ID, videoFaceTbl.frame_id);
    PutIfPresent(values, VIDEO_FACE_COL_FRAME_TIMESTAMP, videoFaceTbl.frame_timestamp);
    PutIfPresent(values, VIDEO_FACE_COL_TRACKS, videoFaceTbl.tracks);
    PutIfPresent(values, VIDEO_FACE_COL_ALGO_VERSION, videoFaceTbl.algo_version);
    PutIfPresent(values, VIDEO_FACE_COL_FEATURES, videoFaceTbl.features);
    PutIfPresent(values, VIDEO_FACE_COL_ANALYSIS_VERSION, videoFaceTbl.analysis_version);

    return values;
}

int32_t VideoFaceClone::BatchInsertWithRetry(const std::string &tableName,
    std::vector<NativeRdb::ValuesBucket> &values, int64_t &rowNum)
{
    CHECK_AND_RETURN_RET(!values.empty(), E_OK);
    int32_t errCode = E_ERR;
    TransactionOperations trans{ __func__ };
    trans.SetBackupRdbStore(destRdb_);
    std::function<int(void)> func = [&]()->int {
        errCode = trans.BatchInsert(rowNum, tableName, values);
        CHECK_AND_PRINT_LOG(errCode == E_OK, "InsertSql failed, errCode: %{public}d, rowNum: %{public}ld.",
            errCode, (long)rowNum);
        return errCode;
    };
    errCode = trans.RetryTrans(func, true);
    CHECK_AND_PRINT_LOG(errCode == E_OK, "BatchInsertWithRetry: tans finish fail!, ret:%{public}d", errCode);
    return errCode;
}

void VideoFaceClone::DeleteExistingVideoFaceData(const std::vector<int32_t>& newFileIds)
{
    if (newFileIds.empty()) {
        MEDIA_INFO_LOG("No new file IDs to delete video face data for.");
        return;
    }

    std::string fileIdNewFilterClause = "(" + BackupDatabaseUtils::JoinValues<int>(newFileIds, ", ") + ")";

    std::string deleteAnalysisVideoMapSql =
        "DELETE FROM AnalysisPhotoMap WHERE "
        "map_album IN (SELECT album_id FROM AnalysisAlbum WHERE album_type = 4096 AND album_subtype = 4102) "
        "AND map_asset IN ("
        "SELECT " + VIDEO_FACE_COL_FILE_ID + " FROM " + VISION_VIDEO_FACE_TABLE +
        " WHERE " + VIDEO_FACE_COL_FILE_ID + " IN " + fileIdNewFilterClause +
        ") ";

    BackupDatabaseUtils::ExecuteSQL(destRdb_, deleteAnalysisVideoMapSql);

    std::string deleteFaceSql = "DELETE FROM " + VISION_VIDEO_FACE_TABLE +
        " WHERE " + VIDEO_FACE_COL_FILE_ID + " IN " + fileIdNewFilterClause;
    BackupDatabaseUtils::ExecuteSQL(destRdb_, deleteFaceSql);
}

void VideoFaceClone::UpdateAnalysisTotalTblVideoFaceStatus(std::shared_ptr<NativeRdb::RdbStore> rdbStore,
    std::vector<int32_t> newFileIds)
{
    std::string fileIdNewFilterClause = "(" + BackupDatabaseUtils::JoinValues<int>(newFileIds, ", ") + ")";

    std::string updateSql =
        "UPDATE tab_analysis_total "
        "SET face = 1 "
        "WHERE EXISTS (SELECT 1 FROM tab_analysis_video_face "
                      "WHERE tab_analysis_video_face.file_id = tab_analysis_total.file_id) "
        "AND " + VIDEO_FACE_COL_FILE_ID + " IN " + fileIdNewFilterClause;

    int32_t errCode = BackupDatabaseUtils::ExecuteSQL(rdbStore, updateSql);
    CHECK_AND_PRINT_LOG(errCode >= 0, "execute update analysis total failed, ret=%{public}d", errCode);
}
} // namespace OHOS::Media