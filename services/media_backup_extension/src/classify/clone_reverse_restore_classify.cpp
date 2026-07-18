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

#define MLOG_TAG "CloneReverseRestoreClassify"

#include "clone_reverse_restore_classify.h"

#include "backup_const_column.h"
#include "backup_database_utils.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "result_set_utils.h"
#include "upgrade_restore_task_report.h"
#include "media_library_db_upgrade.h"

namespace OHOS::Media {
// LCOV_EXCL_START
void CloneReverseRestoreClassify::Init(const ClassifyCloneRestoreConfig& config)
{
    sceneCode_ = config.sceneCode;
    taskId_ = config.taskId;
    mediaLibraryRdb_ = config.mediaLibraryRdb;
    mediaRdb_ = config.mediaRdb;
    externalScoreMaskMap_ = config.scoreMaskMap;
    duplicateMap_ = config.duplicateMap;
}

void CloneReverseRestoreClassify::Restore()
{
    CHECK_AND_RETURN_LOG(mediaRdb_ != nullptr && mediaLibraryRdb_ != nullptr,
        "rdbStore is nullptr");

    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    // 对旧机相册进行预处理
    DataTransfer::MediaLibraryDbUpgrade medialibraryDbUpgrade;
    medialibraryDbUpgrade.AggregateClassifyAlbum(*mediaRdb_);
    // 先迁移新机AanalysisAlbum中数据，再迁移AnalysisPhotoMap
    RestoreAlbum();
    RestoreMap();
    RestoreReverseByVersion();
    AddReverseSpecialAlbum();
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    restoreTimeCost_ = end - start;
    ReportReverseRestoreTask();
    MEDIA_INFO_LOG("TimeCost: CloneReverseRestoreClassify: %{public}" PRId64, end - start);
}

void CloneReverseRestoreClassify::InsertClassifyAlbumData()
{
    auto existingAlbums = QueryExistingAlbumNames();

    std::vector<NativeRdb::ValuesBucket> valuesBuckets;
    for (auto& album : classifyAlbumInfos_) {
        album.albumIdOld = album.albumId;

        if (album.albumName.has_value() && existingAlbums.count(album.albumName.value()) > 0) {
            int32_t duplicateAlbumId = existingAlbums[album.albumName.value()];
            MEDIA_INFO_LOG("Album '%{public}s' already exists, using existing id %{public}d",
                album.albumName.value().c_str(), duplicateAlbumId);

            // 更新 tab_old_albums 中的 album_id 为新机 album_id
            if (album.albumId.has_value()) {
                UpdateTabOldAlbumsId(album.albumId.value(), duplicateAlbumId);
            }

            // 更新 mediaRdb_ 中的重复相册数据
            DeleteDuplicateAlbum(duplicateAlbumId, album.albumId.value());
        }

        album.albumIdNew = album.albumIdOld;

        NativeRdb::ValuesBucket value;
        BackupDatabaseUtils::PutIfPresent(value, "album_id", album.albumIdNew);
        BackupDatabaseUtils::PutIfPresent(value, "album_name", album.albumName);
        BackupDatabaseUtils::PutIfPresent(value, "album_type", album.albumType);
        BackupDatabaseUtils::PutIfPresent(value, "album_subtype", album.albumSubType);
        valuesBuckets.emplace_back(value);

        if (album.albumId.has_value()) {
            albumIdMap_[album.albumId.value()] = album.albumIdNew.value();
        }
    }

    int64_t rowNum = 0;
    int32_t ret = E_OK;
    if (!valuesBuckets.empty()) {
        ret = BatchInsertWithRetry(ANALYSIS_ALBUM_TABLE, valuesBuckets, rowNum, mediaRdb_);
        CHECK_AND_RETURN_LOG(ret == E_OK, "Failed to batch insert portrait albums");
        MEDIA_INFO_LOG("InsertClassifyAlbumData inserted %{public}ld rows", rowNum);
    }
    MEDIA_INFO_LOG("InsertClassifyAlbumData total albums: %{public}zu, new: %{public}ld, existing: %{public}zu",
        classifyAlbumInfos_.size(), rowNum, classifyAlbumInfos_.size() - valuesBuckets.size());
}

void CloneReverseRestoreClassify::RestoreAlbum()
{
    string querySql = "SELECT * FROM AnalysisAlbum WHERE album_name IS NOT NULL AND album_subtype = 4097 ";
    auto resultSet = BackupDatabaseUtils::QuerySql(mediaLibraryRdb_, querySql);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Query resultSql is null.");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        ClassifyAlbumInfo info;
        ParseClassifyAlbumResultSet(info, resultSet);
        classifyAlbumInfos_.emplace_back(info);
    }
    resultSet->Close();
    
    InsertClassifyAlbumData();
}

std::unordered_map<std::string, int32_t> CloneReverseRestoreClassify::QueryExistingAlbumNames()
{
    std::unordered_map<std::string, int32_t> existingAlbums;
    string querySql = "SELECT album_id, album_name FROM AnalysisAlbum WHERE album_name IS NOT NULL"
        " AND album_subtype = 4097";
    auto resultSet = BackupDatabaseUtils::QuerySql(mediaRdb_, querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, existingAlbums, "Query existing albums failed.");
    
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t albumId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "album_id").value_or(0);
        string albumName = BackupDatabaseUtils::GetOptionalValue<string>(resultSet, "album_name").value_or("");
        if (!albumName.empty()) {
            existingAlbums[albumName] = albumId;
        }
    }
    resultSet->Close();
    MEDIA_INFO_LOG("QueryExistingAlbumNames found %{public}zu existing albums", existingAlbums.size());
    return existingAlbums;
}

void CloneReverseRestoreClassify::QueryNewClassifyMaps()
{
    MEDIA_INFO_LOG("QueryNewClassifyMaps");
    CHECK_AND_RETURN_LOG(mediaLibraryRdb_ != nullptr, "mediaLibraryRdb_ is nullptr");

    std::string querySql = "SELECT count(1) AS count FROM AnalysisPhotoMap apm"
        " INNER JOIN AnalysisAlbum aa ON apm.map_album = aa.album_id "
        "WHERE aa.album_type = 4096 AND aa.album_subtype = 4097";
    int32_t totalNumber = BackupDatabaseUtils::QueryInt(mediaLibraryRdb_, querySql, "count");
    MEDIA_INFO_LOG("QueryNewClassifyMaps totalNumber = %{public}d", totalNumber);

    for (int32_t offset = 0; offset < totalNumber; offset += QUERY_COUNT) {
        std::string batchQuerySql = std::string("SELECT map_album, map_asset FROM AnalysisPhotoMap") +
            " WHERE map_album IN (SELECT album_id FROM AnalysisAlbum" +
            " WHERE album_type = 4096 AND album_subtype = 4097)" +
            " LIMIT " + std::to_string(offset) + ", " + std::to_string(QUERY_COUNT);

        auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaLibraryRdb_, batchQuerySql);
        CHECK_AND_RETURN_LOG(resultSet != nullptr, "Query resultSet is null.");

        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            ClassifyMapInfo mapInfo;
            mapInfo.mapAlbum = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "map_album");
            mapInfo.mapAsset = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "map_asset");
            classifyMapInfos_.emplace_back(mapInfo);
        }
        resultSet->Close();
    }

    MEDIA_INFO_LOG("QueryNewClassifyMaps count = %{public}zu", classifyMapInfos_.size());
}

void CloneReverseRestoreClassify::InsertClassifyMapsToOldDb()
{
    MEDIA_INFO_LOG("InsertClassifyMapsToOldDb");
    CHECK_AND_RETURN_LOG(mediaRdb_ != nullptr, "targetRdb_ is nullptr");
    CHECK_AND_RETURN_LOG(!classifyMapInfos_.empty(), "classifyMapInfos_ is empty");

    std::vector<NativeRdb::ValuesBucket> valuesBuckets;
    for (const auto& mapInfo : classifyMapInfos_) {
        if (!mapInfo.mapAlbum.has_value() || !mapInfo.mapAsset.has_value()) {
            continue;
        }

        int32_t oldAlbumId = mapInfo.mapAlbum.value();
        auto it = albumIdMap_.find(oldAlbumId);
        if (it == albumIdMap_.end()) {
            MEDIA_ERR_LOG("Album ID %{public}d not found in albumIdMap_", oldAlbumId);
            continue;
        }

        NativeRdb::ValuesBucket value;
        value.PutInt("map_album", it->second);
        value.PutInt("map_asset", mapInfo.mapAsset.value());
        valuesBuckets.emplace_back(value);
    }

    int64_t rowNum = 0;
    int32_t ret = BatchInsertWithRetry(ANALYSIS_PHOTO_MAP_TABLE, valuesBuckets, rowNum, mediaRdb_);
    CHECK_AND_RETURN_LOG(ret == E_OK, "Failed to batch insert portrait maps");
    MEDIA_INFO_LOG("InsertClassifyMapsToOldDb inserted %{public}ld rows", rowNum);
}

void CloneReverseRestoreClassify::RestoreMap()
{
    MEDIA_INFO_LOG("RestoreMap");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    QueryNewClassifyMaps();
    InsertClassifyMapsToOldDb();

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("RestoreMap cost %{public}lld", (long long)(end - start));
}

void CloneReverseRestoreClassify::RestoreReverseByVersion()
{
    BackupDatabaseUtils::isTableExist(mediaRdb_,
        ANALYSIS_VIDEO_TOTAL_TABLE, isRestoreFromNewVersion_);
    // 处理label和video_label表
    ReverseRestoreLabelAndTotalData();
}

void CloneReverseRestoreClassify::ReportReverseRestoreTask()
{
    RestoreTaskInfo info;
    info.type = "CLONE_REVERSE_RESTORE_CLASSIFY";
    info.successCount = successInsertLabelCnt_ + successUpdateLabelCnt_;
    info.failedCount = failInsertLabelCnt_ + failUpdateLabelCnt_;
    info.errorInfo = "timeCost: " + std::to_string(restoreTimeCost_);

    UpgradeRestoreTaskReport()
        .SetSceneCode(sceneCode_)
        .SetTaskId(taskId_)
        .Report(info);
}

void CloneReverseRestoreClassify::ReverseRestoreLabelAndTotalData()
{
    // 正向：重复以新机为准
    // 反向：重复用新机覆盖旧机 → 直接插入新机数据，删除已偏移旧机数据
    MEDIA_INFO_LOG("restore reverse classify albums start.");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    std::vector<ClassifyCloneInfo> imageClassifyInfos;
    std::vector<ClassifyVideoCloneInfo> videoClassifyInfos;
    // 查新机图片label表数据
    GetImageClassifyInfos(imageClassifyInfos);
    // 查新机视频label表数据
    GetVideoClassifyInfos(videoClassifyInfos);
    int64_t startInsert = MediaFileUtils::UTCTimeMilliSeconds();
    if (!imageClassifyInfos.empty()) {
        // 插入新机label表数据，预计不会重复or失败
        // todo 需要判断是否插入成功
        InsertImageLabelData(imageClassifyInfos);
        QueryAndUpdateTotal("tab_analysis_total", GetFileIdsStr(imageClassifyInfos));
    }

    if (!videoClassifyInfos.empty()) {
        InsertVideoLabelData(videoClassifyInfos);
        if (isRestoreFromNewVersion_) {
            QueryAndUpdateTotal("tab_analysis_video_total", GetFileIdsStr(videoClassifyInfos));
        } else {
            QueryAndUpdateTotal("tab_analysis_total", GetFileIdsStr(videoClassifyInfos));
        }
    }

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    restoreLabelTimeCost_ += end - start;
    MEDIA_INFO_LOG("TimeCost: GetReverse: %{public}" PRId64
        ", InsertUpdate: %{public}" PRId64 ", Total: %{public}" PRId64,
        startInsert - start, end - startInsert, end - start);
}

void CloneReverseRestoreClassify::GetImageClassifyInfos(
    std::vector<ClassifyCloneInfo> &classifyInfos)
{
    std::string querySql = "SELECT * FROM tab_analysis_label";
    auto resultSet = BackupDatabaseUtils::QuerySql(mediaLibraryRdb_, querySql);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Query resultSql is null.");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        ClassifyCloneInfo info;
        GetClassifyInfoFromResultSet(info, resultSet);
        classifyInfos.emplace_back(info);
    }
    resultSet->Close();
    MEDIA_INFO_LOG("query new device tab_analysis_label nums: %{public}zu",
        classifyInfos.size());
}

void CloneReverseRestoreClassify::InsertImageLabelData(
    std::vector<ClassifyCloneInfo> &insertInfos)
{
    CHECK_AND_RETURN(!insertInfos.empty());

    auto intersection = GetCommonColumns(ANALYSIS_LABEL_TABLE);
    InsertClassifyInfosBatch(insertInfos, intersection);
}

void CloneReverseRestoreClassify::InsertClassifyInfosBatch(
    std::vector<ClassifyCloneInfo> &insertInfos,
    const std::unordered_set<std::string> &intersection)
{
    size_t offset = 0;
    while (offset < insertInfos.size()) {
        InsertSingleBatch(insertInfos, offset, intersection);
        offset += PAGE_SIZE;
    }
}

void CloneReverseRestoreClassify::InsertSingleBatch(
    std::vector<ClassifyCloneInfo> &insertInfos,
    size_t offset,
    const std::unordered_set<std::string> &intersection)
{
    std::vector<NativeRdb::ValuesBucket> values;
    size_t batchSize = std::min(static_cast<size_t>(PAGE_SIZE),
        insertInfos.size() - offset);

    for (size_t index = 0; index < batchSize; index++) {
        BuildInsertValue(insertInfos[index + offset], intersection, values);
    }

    MEDIA_INFO_LOG("Insert reverse classify albums, values size: %{public}zu",
        values.size());
    int64_t rowNum = 0;
    // todo 假设旧机偏移失败，需确认此处是否会覆盖
    int32_t errCode = BatchInsertWithRetry(ANALYSIS_LABEL_TABLE, values, rowNum, mediaRdb_);
    if (errCode != E_OK || rowNum != static_cast<int64_t>(values.size())) {
        int64_t failNums = static_cast<int64_t>(values.size()) - rowNum;
        MEDIA_ERR_LOG("Insert reverse classify albums fail, num: %{public}" PRId64,
            failNums);
        failInsertLabelCnt_ += static_cast<int32_t>(failNums);
    } else {
        successInsertLabelCnt_ += rowNum;
    }
}

void CloneReverseRestoreClassify::BuildInsertValue(
    ClassifyCloneInfo &info,
    const std::unordered_set<std::string> &intersection,
    std::vector<NativeRdb::ValuesBucket> &values)
{
    CHECK_AND_RETURN(info.fileIdOld.has_value());
    NativeRdb::ValuesBucket value;
    GetMapInsertValue(value, info, intersection);
    values.emplace_back(value);
}

void CloneReverseRestoreClassify::GetVideoClassifyInfos(
    std::vector<ClassifyVideoCloneInfo> &classifyVideoInfos)
{
    std::string querySql = "SELECT * FROM " + ANALYSIS_VIDEO_LABEL_TABLE;
    auto resultSet = BackupDatabaseUtils::QuerySql(mediaLibraryRdb_, querySql);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Query resultSql is null.");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        ClassifyVideoCloneInfo info;
        GetClassifyVideoInfoFromResultSet(info, resultSet);
        classifyVideoInfos.emplace_back(info);
    }
    resultSet->Close();
}

void CloneReverseRestoreClassify::InsertVideoLabelData(
    std::vector<ClassifyVideoCloneInfo> &insertInfos)
{
    CHECK_AND_RETURN(!insertInfos.empty());
    auto intersection = GetCommonColumns(ANALYSIS_VIDEO_LABEL_TABLE);

    std::vector<NativeRdb::ValuesBucket> values;
    for (auto& info : insertInfos) {
        CHECK_AND_CONTINUE(info.fileIdOld.has_value());
        NativeRdb::ValuesBucket value;
        GetVideoMapInsertValue(value, info, intersection);
        values.emplace_back(value);
    }

    int64_t rowNum = 0;
    int32_t errCode = BatchInsertWithRetry(ANALYSIS_VIDEO_LABEL_TABLE, values, rowNum, mediaRdb_);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("Insert reverse video label albums fail");
        failInsertLabelCnt_ += static_cast<int32_t>(values.size());
    } else {
        successInsertLabelCnt_ += rowNum;
    }
}

string CloneReverseRestoreClassify::GetFileIdsStr(const std::vector<ClassifyCloneInfo> &imageInfos)
{
    std::stringstream fileIdStr;
    fileIdStr << "(";
    for (size_t i = 0; i < imageInfos.size(); i++) {
        if (!imageInfos[i].fileIdOld.has_value()) {
            continue;
        }
        if (i != imageInfos.size() - 1) {
            fileIdStr << imageInfos[i].fileIdOld.value() << ",";
        } else {
            fileIdStr << imageInfos[i].fileIdOld.value();
        }
    }
    fileIdStr << ")";
    return fileIdStr.str();
}

string CloneReverseRestoreClassify::GetFileIdsStr(const std::vector<ClassifyVideoCloneInfo> &videoInfos)
{
    std::stringstream fileIdStr;
    fileIdStr << "(";
    for (size_t i = 0; i < videoInfos.size(); i++) {
        if (!videoInfos[i].fileIdOld.has_value()) {
            continue;
        }
        if (i != videoInfos.size() - 1) {
            fileIdStr << videoInfos[i].fileIdOld.value() << ",";
        } else {
            fileIdStr << videoInfos[i].fileIdOld.value();
        }
    }
    fileIdStr << ")";
    return fileIdStr.str();
}

void CloneReverseRestoreClassify::QueryAndUpdateTotal(const string& tableName, const string& fileIdClause)
{
    std::string querySql = "SELECT file_id, label FROM " + tableName +
        " WHERE file_id IN " + fileIdClause;
    // 查询新机
    auto resultSet = BackupDatabaseUtils::QuerySql(mediaLibraryRdb_, querySql);
    CHECK_AND_RETURN(resultSet != nullptr);
    unordered_map<int32_t, vector<int32_t>> labelMap;

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t fileId = GetInt32Val("file_id", resultSet);
        int32_t label = GetInt32Val("label", resultSet);
        labelMap[label].emplace_back(fileId);
    }
    resultSet->Close();

    for (auto& item : labelMap) {
        int32_t updatedRows = 0;
        NativeRdb::ValuesBucket valuesBucket;
        valuesBucket.PutInt("label", item.first);
        std::unique_ptr<NativeRdb::AbsRdbPredicates> updatePredicates =
            std::make_unique<NativeRdb::AbsRdbPredicates>(tableName);
        std::vector<NativeRdb::ValueObject> fileIds;
        for (auto& fileId : item.second) {
            fileIds.emplace_back(fileId);
        }
        updatePredicates->In("file_id", fileIds);
        BackupDatabaseUtils::Update(mediaRdb_, updatedRows, valuesBucket, updatePredicates);
    }
}

void CloneReverseRestoreClassify::AddReverseSpecialAlbum()
{
    MEDIA_INFO_LOG("AddReverseSpecialAlbum start");
    AddReverseSelfieAlbum();
    AddReverseUserCommentAlbum();
}

void CloneReverseRestoreClassify::AddReverseSelfieAlbum()
{
    // 暂不需要
}

void CloneReverseRestoreClassify::AddReverseUserCommentAlbum()
{
    // 暂不需要
}

void CloneReverseRestoreClassify::DeleteDuplicateAlbum(int32_t oldAlbumId, int32_t newAlbumId)
{
    MEDIA_INFO_LOG("DeleteDuplicateAlbum start, oldAlbumId=%{public}d, newAlbumId=%{public}d",
        oldAlbumId, newAlbumId);

    // 更新 AnalysisPhotoMap 中 map_album 从 oldAlbumId 改为 newAlbumId
    NativeRdb::ValuesBucket updateMapValues;
    updateMapValues.PutInt("map_album", newAlbumId);
    std::unique_ptr<NativeRdb::AbsRdbPredicates> updateMapPredicates =
        std::make_unique<NativeRdb::AbsRdbPredicates>("AnalysisPhotoMap");
    updateMapPredicates->EqualTo("map_album", std::to_string(oldAlbumId));
    int32_t updatedMapRows = 0;
    int32_t ret = BackupDatabaseUtils::Update(mediaRdb_, updatedMapRows, updateMapValues, updateMapPredicates);
    if (ret == E_OK) {
        MEDIA_INFO_LOG("DeleteDuplicateAlbum updated %{public}d rows in AnalysisPhotoMap",
            updatedMapRows);
    } else {
        MEDIA_ERR_LOG("DeleteDuplicateAlbum failed to update AnalysisPhotoMap, ret=%{public}d",
            ret);
    }

    // 删除 AnalysisAlbum 中 album_id = oldAlbumId 的数据
    NativeRdb::AbsRdbPredicates deleteAlbumPredicates("AnalysisAlbum");
    deleteAlbumPredicates.EqualTo("album_id", oldAlbumId);
    int32_t deletedAlbumRows = 0;
    ret = BackupDatabaseUtils::Delete(deleteAlbumPredicates, deletedAlbumRows, mediaRdb_);
    if (ret == E_OK) {
        MEDIA_INFO_LOG("DeleteDuplicateAlbum deleted %{public}d rows from AnalysisAlbum",
            deletedAlbumRows);
    } else {
        MEDIA_ERR_LOG("DeleteDuplicateAlbum failed to delete from AnalysisAlbum, ret=%{public}d",
            ret);
    }

    MEDIA_INFO_LOG("DeleteDuplicateAlbum end, deleted %{public}d album rows and updated %{public}d map rows",
        deletedAlbumRows, updatedMapRows);
}

void CloneReverseRestoreClassify::UpdateTabOldAlbumsId(int32_t oldAlbumId, int32_t newAlbumId)
{
    MEDIA_INFO_LOG("UpdateTabOldAlbumsId start, oldAlbumId=%{public}d, newAlbumId=%{public}d",
                   oldAlbumId, newAlbumId);

    if (mediaRdb_ == nullptr) {
        MEDIA_ERR_LOG("mediaRdb_ is null");
        return;
    }

    std::string updateQuery = "UPDATE tab_old_albums SET album_id = " + std::to_string(newAlbumId) +
                             " WHERE album_type = 4096 AND album_id = " + std::to_string(oldAlbumId);

    int32_t ret = mediaRdb_->ExecuteSql(updateQuery);
    if (ret == NativeRdb::E_OK) {
        MEDIA_INFO_LOG("UpdateTabOldAlbumsId successfully updated album_id from %{public}d to %{public}d",
                       oldAlbumId, newAlbumId);
    } else {
        MEDIA_ERR_LOG("UpdateTabOldAlbumsId failed to update album_id, ret=%{public}d", ret);
    }
}
// LCOV_EXCL_STOP
} // namespace OHOS::Media