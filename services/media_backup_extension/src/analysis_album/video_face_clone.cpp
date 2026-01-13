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
        // The video status must be updated even if video_face is empty.
        StartCloneAnalysisVideoTotalTab(oldFileIds, newFileIds);
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
    StartCloneAnalysisVideoTotalTab(oldFileIds, newFileIds);
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("VideoFaceClone::CloneVideoFaceInfo completed. Migrated %{public}lld records. "
        "Total time: %{public}lld ms", (long long)migrateVideoFaceNum_, (long long)migrateVideoFaceTotalTimeCost_);
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
    videoFaceTbl.features = BackupDatabaseUtils::GetOptionalValue<std::vector<uint8_t>>(resultSet,
        VIDEO_FACE_COL_FEATURES);
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
    migrateVideoFaceFileNumber_ += static_cast<int64_t>(fileIdSet.size());
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

void VideoFaceClone::StartCloneAnalysisVideoTotalTab(const std::vector<int32_t> &oldFileIds,
    const std::vector<int32_t> &newFileIds)
{
    BackupDatabaseUtils::ClearAnalysisVideoTotalTable(destRdb_);
    bool isTableExist = false;
    CHECK_AND_RETURN_LOG(BackupDatabaseUtils::isTableExist(sourceRdb_, VISION_VIDEO_TOTAL_TABLE, isTableExist),
        "fail to check whether tableName exists");
    if (isTableExist) {
        CopyAnalysisVideoTotalTab(VISION_VIDEO_TOTAL_TABLE, oldFileIds);
    } else {
        UpdateAnalysisVideoTotalTblLabelAndFace(oldFileIds, newFileIds);
        UpdateAnalysisVideoTotalTblFaceAndTagId();
    }
}

void VideoFaceClone::UpdateAnalysisVideoTotalTblLabelAndFace(const std::vector<int32_t> &oldFileIds,
    const std::vector<int32_t> &newFileIds)
{
    MEDIA_INFO_LOG("UpdateAnalysisVideoTotalTblLabelAndFace");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    CopyAnalysisVideoTotalTab(VISION_TOTAL_TABLE, oldFileIds);
    BackupDatabaseUtils::UpdateFaceToAnalysisVideoTotalTable(destRdb_);
    BackupDatabaseUtils::UpdateStatusToAnalysisTable(destRdb_);
    BackupDatabaseUtils::UpdateFaceToAnalysisTable(destRdb_, newFileIds);

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("UpdateAnalysisVideoTotalTblLabelAndFace cost %{public}lld", static_cast<long long>(end - start));
}

void VideoFaceClone::UpdateAnalysisVideoTotalTblFaceAndTagId()
{
    MEDIA_INFO_LOG("UpdateAnalysisVideoTotalTblFaceAndTagId");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    BackupDatabaseUtils::DeleteDirtytagIdFromFaceTagTable(destRdb_);
    BackupDatabaseUtils::UpdateVideoFaceTagId(destRdb_);
    BackupDatabaseUtils::UpdateVideoTotalFaceId(destRdb_);
    BackupDatabaseUtils::CheckFaceToAnalysisVideoTotalTable(destRdb_);

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("UpdateAnalysisVideoTotalTblFaceAndTagId cost %{public}lld", static_cast<long long>(end - start));
}

static std::vector<int32_t> getVideoFileIds(std::vector<int32_t> FileIds,
    std::shared_ptr<NativeRdb::RdbStore> RdbStore)
{
    std::string fileIdInClause = "(" + BackupDatabaseUtils::JoinValues<int>(FileIds, ", ") + ")";
    std::string queryVideoFileIdSql =
        "SELECT file_id FROM " + PhotoColumn::PHOTOS_TABLE +
        " WHERE media_type = 2 AND file_id IN " + fileIdInClause;
    std::vector<int32_t> VideoFileIds =
        BackupDatabaseUtils::QueryIntVec(RdbStore, queryVideoFileIdSql, "file_id");
    return VideoFileIds;
}

static std::unordered_map<int32_t, int32_t> buildFileIdMap(const std::unordered_map<int32_t, PhotoInfo>& fileIdPairs)
{
    std::unordered_map<int32_t, int32_t> idMap;
    for (const auto& pair : fileIdPairs) {
        idMap[pair.first] = pair.second.fileIdNew;
    }
    return idMap;
}

static int32_t getNewFileId(std::unordered_map<int32_t, int32_t> idMap, int32_t oldid)
{
    auto it = idMap.find(oldid);
    if (it != idMap.end()) {
        return it->second;
    }
    return -1;
}

void VideoFaceClone::WriteDataToAnaVideoTotalTabSub(std::vector<CloneVideoInfo> &updateDataList)
{
    std::unordered_map<int32_t, int32_t> idMap = buildFileIdMap(photoInfoMap_);
    std::string updateSql = "UPDATE tab_analysis_video_total SET ";
    std::string faceCase = "face = CASE file_id ";
    std::string whereClause = " WHERE file_id IN (";
    for (size_t i = 0; i < updateDataList.size(); ++i) {
        const auto& data = updateDataList[i];
        int32_t newFileId = getNewFileId(idMap, data.file_id);
        int32_t face = data.face;

        faceCase += " WHEN " + std::to_string(newFileId) + " THEN " + std::to_string(face) + " ";
        whereClause += std::to_string(newFileId);
        if (i != updateDataList.size() - 1) {
            whereClause += ", ";
        }
    }

    faceCase += " END";
    whereClause += ")";

    updateSql += faceCase + whereClause + ";";

    int32_t errCode = BackupDatabaseUtils::ExecuteSQL(destRdb_, updateSql);
    CHECK_AND_PRINT_LOG(errCode >= 0,
        "execute batch update analysis total failed, ret=%{public}d", errCode);
}

void VideoFaceClone::WriteDataToAnaVideoTotalTab(std::vector<CloneVideoInfo> &updateDataList)
{
    if (updateDataList.empty()) {
        return;
    }
    size_t chunkSize = 200;
    size_t totalSize = updateDataList.size();
    size_t totalChunks = (totalSize + chunkSize - 1) / chunkSize;

    for (size_t i = 0; i < totalChunks; ++i) {
        size_t start = i * chunkSize;
        size_t end = std::min(start + chunkSize, totalSize);
        std::vector<CloneVideoInfo> currentBatch(updateDataList.begin() + start, updateDataList.begin() + end);
        WriteDataToAnaVideoTotalTabSub(currentBatch);
    }
}

bool VideoFaceClone::CopyAnalysisVideoTotalTab(const std::string &tableName, const std::vector<int32_t> &oldFileIds)
{
    MEDIA_INFO_LOG("CopyAnalysisVideoTotalTab");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    std::vector<int32_t> VideoFileIds = getVideoFileIds(oldFileIds, sourceRdb_);
    std::string VideofileIdOldInClause = "(" + BackupDatabaseUtils::JoinValues<int>(VideoFileIds, ", ") + ")";

    std::vector<CloneVideoInfo> updateDataList;
    updateDataList = BackupDatabaseUtils::QueryVideoInfo(tableName, VideofileIdOldInClause, sourceRdb_);
    WriteDataToAnaVideoTotalTab(updateDataList);
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("CopyAnalysisVideoTotalTab Cost %{public}lld", (long long)(end - start));
    return true;
}
} // namespace OHOS::Media