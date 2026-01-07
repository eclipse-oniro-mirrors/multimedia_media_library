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
#define MLOG_TAG "CloneRestoreGroupPhoto"

#include "clone_restore_group_photo.h"

#include "media_log.h"
#include "backup_const.h"
#include "vision_column.h"
#include "medialibrary_errno.h"
#include "userfile_manager_types.h"
#include "backup_const_column.h"
#include "medialibrary_type_const.h"
#include "backup_file_utils.h"
#include "media_file_utils.h"
#include "result_set_utils.h"
#include "backup_database_utils.h"
#include "media_library_db_upgrade.h"
#include "medialibrary_unistore_manager.h"
#include "upgrade_restore_task_report.h"
#include "medialibrary_rdb_transaction.h"

namespace OHOS::Media {
const int32_t PAGE_SIZE = 200;
const std::string RESTORE_STATUS_SUCCESS = "1";

void CloneRestoreGroupPhoto::Init(int32_t sceneCode, const std::string &taskId, std::string restoreInfo,
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
    std::shared_ptr<NativeRdb::RdbStore> mediaRdb, bool isCloudRestoreSatisfied)
{
    sceneCode_ = sceneCode;
    taskId_ = taskId;
    mediaLibraryRdb_ = mediaLibraryRdb;
    mediaRdb_ = mediaRdb;
    analysisType_ = "";
    restoreInfo_ = restoreInfo;
    isCloudRestoreSatisfied_ = isCloudRestoreSatisfied;
    MEDIA_INFO_LOG("isCloudRestoreSatisfied_ is %{public}d", (int)isCloudRestoreSatisfied_);
}

void CloneRestoreGroupPhoto::Restore(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap)
{
    CHECK_AND_RETURN_LOG(mediaRdb_ != nullptr && mediaLibraryRdb_ != nullptr,
        "GroupPhotoRestore failed, rdbStore is nullptr");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    photoInfoMap_ = photoInfoMap;
    bool needCloneFlag = IsExistPortraitDataInOldDb();
    CHECK_AND_RETURN_LOG(needCloneFlag,
        "The old device does not have portrait album, so there is no need to clone the photo album to the new device");
    int32_t ret = DeleteGroupPhotoAlbumInfoInNewDb();
    CHECK_AND_RETURN_LOG(ret == E_OK, "fail to delete group photo album info in new db");
    ret = RestoreGroupPhotoAlbumInfo();
    CHECK_AND_RETURN_LOG(ret == E_OK, "fail to restore group photo album info");
    BackupDatabaseUtils::UpdateFaceGroupTagsUnion(mediaLibraryRdb_);
    ret = RestoreMaps();
    CHECK_AND_RETURN_LOG(ret == E_OK, "fail to update analysis photo map status");
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    restoreTimeCost_ = end - start;
    ReportCloneRestoreGroupPhotoTask();
    MEDIA_INFO_LOG("TimeCost: GroupPhotoRestore: %{public}" PRId64, end - start);
}

std::vector<GroupPhotoAlbumDfx> CloneRestoreGroupPhoto::QueryGroupPhotoAlbumInOldDb(int32_t& offset, int32_t& rowCount)
{
    std::vector<GroupPhotoAlbumDfx> result;
    result.reserve(QUERY_COUNT);

    const std::string querySql = "SELECT album_name, cover_uri, tag_id, group_tag, count "
        "FROM AnalysisAlbum "
        "WHERE album_type = ? "
        "AND album_subtype = ? "
        "LIMIT ?, ?";

    std::vector<NativeRdb::ValueObject> bindArgs = { SMART, PhotoAlbumSubType::GROUP_PHOTO, offset, QUERY_COUNT };
    CHECK_AND_RETURN_RET_LOG(this->mediaRdb_ != nullptr, result, "Media_Restore: mediaRdb_ is null.");
    auto resultSet = mediaRdb_->QuerySql(querySql, bindArgs);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, result, "Query resultSql is null.");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        GroupPhotoAlbumDfx dfxInfo;
        dfxInfo.albumName = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, ANALYSIS_COL_ALBUM_NAME);
        dfxInfo.coverUri = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, ANALYSIS_COL_COVER_URI);
        dfxInfo.tagId = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, ANALYSIS_COL_TAG_ID);
        dfxInfo.groupTag = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, ANALYSIS_COL_GROUP_TAG);
        dfxInfo.count = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, ANALYSIS_COL_COUNT);

        result.push_back(dfxInfo);
    }
    resultSet->GetRowCount(rowCount);
    resultSet->Close();
    return result;
}

void CloneRestoreGroupPhoto::RecordOldGroupPhotoAlbumDfx()
{
    int32_t offset {0};
    int32_t rowCount {0};
    std::vector<GroupPhotoAlbumDfx> albums;

    do {
        auto batchResults = QueryGroupPhotoAlbumInOldDb(offset, rowCount);
        if (!batchResults.empty()) {
            albums.insert(albums.end(), batchResults.begin(), batchResults.end());
        }

        offset += QUERY_COUNT;
    } while (rowCount > 0);

    for (const auto& album : albums) {
        GroupPhotoAlbumDfx dfxInfo;
        if (album.albumName.has_value()) {
            dfxInfo.albumName = album.albumName.value();
        }
        if (album.coverUri.has_value()) {
            auto uriParts = BackupDatabaseUtils::SplitString(album.coverUri.value(), '/');
            if (uriParts.size() >= COVER_URI_NUM) {
                std::string fileName = uriParts[uriParts.size() - 1];
                dfxInfo.coverUri = BackupFileUtils::GarbleFileName(fileName);
            }
        }
        if (album.tagId.has_value()) {
            dfxInfo.tagId = album.tagId.value();
        }
        if (album.groupTag.has_value()) {
            dfxInfo.groupTag = album.groupTag.value();
        }
        if (album.count.has_value()) {
            dfxInfo.count = album.count.value();
        }

        groupPhotoAlbumDfx_.push_back(dfxInfo);
    }
}

std::unordered_set<std::string> CloneRestoreGroupPhoto::QueryAllGroupTag()
{
    std::unordered_set<std::string> result;
    std::vector<std::string> groupTags;
    for (const auto& oldAlbum : groupPhotoAlbumDfx_) {
        if (oldAlbum.groupTag.has_value()) {
            groupTags.push_back(oldAlbum.groupTag.value());
        }
    }

    CHECK_AND_RETURN_RET_LOG(!groupTags.empty(), result, "No valid group_tags found in old albums");

    std::string querySql = "SELECT group_tag FROM " + ANALYSIS_ALBUM_TABLE +
        " WHERE tag_id IN (" + BackupDatabaseUtils::JoinSQLValues<string>(groupTags, ", ") + ")";

    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaLibraryRdb_, querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, result, "Query resultSet is null.");

    std::string dfxInfo;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        dfxInfo =
            BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, ANALYSIS_COL_GROUP_TAG).value_or("");
        result.insert(dfxInfo);
    }

    resultSet->Close();
    return result;
}

void CloneRestoreGroupPhoto::LogGroupPhotoCloneDfx()
{
    std::vector<std::string> failedAlbums;
    std::unordered_set<std::string> existingGroupTags = QueryAllGroupTag();

    for (const auto& oldAlbum : groupPhotoAlbumDfx_) {
        if (!oldAlbum.groupTag.has_value()) {
            continue;
        }

        if (existingGroupTags.find(oldAlbum.groupTag.value()) == existingGroupTags.end()) {
            std::string albumInfo = "Album: " + oldAlbum.albumName.value_or("Unknown") +
                ", GroupTag: " + oldAlbum.groupTag.value() +
                ", Cover: " + oldAlbum.coverUri.value_or("Unknown") +
                ", Count: " + std::to_string(oldAlbum.count.value_or(0));
            failedAlbums.push_back(albumInfo);
        }
    }

    if (!failedAlbums.empty()) {
        MEDIA_ERR_LOG("Following group photo albums failed to clone completely:");
        for (const auto& failedAlbum : failedAlbums) {
            MEDIA_ERR_LOG("%{public}s", failedAlbum.c_str());
        }
    } else {
        MEDIA_INFO_LOG("All group photo albums cloned successfully");
    }

    MEDIA_INFO_LOG("Stat: Total albums: %{public}zu, Failed albums count: %{public}zu",
        groupPhotoAlbumDfx_.size(), failedAlbums.size());
}

int32_t CloneRestoreGroupPhoto::QueryGroupPhotoAlbumTbl(const std::vector<std::string>& commonColumns)
{
    int32_t rowCount = 0;
    int32_t offset = 0;
    std::string queryColumns = BackupDatabaseUtils::JoinValues<string>(commonColumns, ", ");
    std::string querySql =
        "SELECT " + queryColumns +
        " FROM " + ANALYSIS_ALBUM_TABLE +
        " WHERE ";
    std::string whereClause = "(" +
        SMARTALBUM_DB_ALBUM_TYPE + " = " + std::to_string(SMART) + " AND " +
        "album_subtype" + " = " + std::to_string(GROUP_PHOTO) + ")";
    AppendExtraWhereClause(whereClause);
    querySql += whereClause;
    GetMaxAlbumId();
    int32_t albumIdNow = maxAnalysisAlbumId_;
    do {
        std::string querySqlEachRound = querySql + " LIMIT " +
            std::to_string(offset) + ", " + std::to_string(QUERY_COUNT);
        auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaRdb_, querySqlEachRound);
        CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "Query resultSql is null.");
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            AnalysisAlbumTbl groupPhotoAlbumTbl;
            ParseAlbumResultSet(resultSet, groupPhotoAlbumTbl);
            groupPhotoAlbumTbl.albumIdNew = std::make_optional<int32_t>(++albumIdNow);
            groupPhotoAlbumInfos_.emplace_back(groupPhotoAlbumTbl);
            if (groupPhotoAlbumTbl.tagId.has_value() && groupPhotoAlbumTbl.coverUri.has_value()) {
                coverUriInfo_.emplace_back(groupPhotoAlbumTbl.tagId.value(),
                    std::make_pair(groupPhotoAlbumTbl.coverUri.value(),
                    groupPhotoAlbumTbl.isCoverSatisfied.value_or(INVALID_COVER_SATISFIED_STATUS)));
            }
        }
        resultSet->GetRowCount(rowCount);
        resultSet->Close();
        offset += QUERY_COUNT;
    } while (rowCount == QUERY_COUNT);
    return E_OK;
}

int32_t CloneRestoreGroupPhoto::InsertGroupPhotoAlbum()
{
    size_t offset = 0;
    do {
        std::vector<NativeRdb::ValuesBucket> values;
        for (size_t index = 0; index < QUERY_COUNT && index + offset < groupPhotoAlbumInfos_.size(); index++) {
            NativeRdb::ValuesBucket value;
            GetAnalysisAlbumInsertValue(value, groupPhotoAlbumInfos_[index + offset]);
            values.emplace_back(value);
        }
        int64_t rowNum = 0;
        int32_t errCode = BatchInsertWithRetry("AnalysisAlbum", values, rowNum);
        if (errCode != E_OK || rowNum != static_cast<int64_t>(values.size())) {
            int64_t failNums = static_cast<int64_t>(values.size()) - rowNum;
            ErrorInfo errorInfo(RestoreError::INSERT_FAILED, 0, std::to_string(errCode),
                "insert into AnalysisAlbum fail, num:" + std::to_string(failNums));
            UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).ReportError(errorInfo);
            albumFailedCnt_ += failNums;
        }
        albumSuccessCnt_ += rowNum;
        offset += QUERY_COUNT;
    } while (offset < groupPhotoAlbumInfos_.size());
    return E_OK;
}

int32_t CloneRestoreGroupPhoto::RestoreGroupPhotoAlbumInfo()
{
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    RecordOldGroupPhotoAlbumDfx();

    std::string querySql =  "SELECT count(1) AS count FROM " + ANALYSIS_ALBUM_TABLE + " WHERE ";
    std::string whereClause = "(" + ANALYSIS_COL_ALBUM_TYPE + " = " + std::to_string(SMART) + " AND " +
        ANALYSIS_COL_ALBUM_SUBTYPE + " = " + std::to_string(GROUP_PHOTO) + ")";
    AppendExtraWhereClause(whereClause);
    querySql += whereClause;
    int32_t totalNumber = BackupDatabaseUtils::QueryInt(mediaRdb_, querySql, CUSTOM_COUNT);
    MEDIA_INFO_LOG("QueryGroupPhotoAlbum totalNumber = %{public}d", totalNumber);

    std::vector<std::string> commonColumn = BackupDatabaseUtils::GetCommonColumnInfos(mediaRdb_, mediaLibraryRdb_,
        ANALYSIS_ALBUM_TABLE);
    std::vector<std::string> commonColumns = BackupDatabaseUtils::filterColumns(commonColumn,
        EXCLUDED_GROUP_PHOTO_COLUMNS);
    int32_t ret = QueryGroupPhotoAlbumTbl(commonColumns);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "fail to query group photo record, ret is %{public}d", ret);
    ret = InsertGroupPhotoAlbum();
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "fail to insert group photo record, ret is %{public}d", ret);
    GenNewCoverUris(coverUriInfo_);
    LogGroupPhotoCloneDfx();
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    migrateGroupPhotoTotalTimeCost_ += end - start;
    return E_OK;
}

NativeRdb::ValuesBucket CloneRestoreGroupPhoto::GetMapInsertValue(int32_t albumId, int32_t fileId,
    std::optional<int32_t> &order)
{
    NativeRdb::ValuesBucket value;
    value.PutInt("map_album", albumId);
    value.PutInt("map_asset", fileId);
    CHECK_AND_EXECUTE(!order.has_value(), value.PutInt("order_position", order.value()));
    return value;
}

void CloneRestoreGroupPhoto::ParseMapInsertValues(std::vector<NativeRdb::ValuesBucket> &values,
    std::shared_ptr<NativeRdb::ResultSet> resultSet)
{
    lastIdOfMap_ = GetInt32Val("rowid", resultSet);

    std::optional<int32_t> oldFileId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "map_asset");
    bool exceptCond = oldFileId.has_value() && photoInfoMap_.count(oldFileId.value()) > 0;
    CHECK_AND_RETURN_LOG(exceptCond, "the query oldFileId is invalid!");
    PhotoInfo photoInfo = photoInfoMap_.at(oldFileId.value());

    std::optional<int32_t> oldAlbumId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "map_album");
    CHECK_AND_RETURN_LOG(oldAlbumId.has_value(), "the query oldAlbumId is invalid!");
    auto it = std::find_if(groupPhotoAlbumInfos_.begin(), groupPhotoAlbumInfos_.end(),
        [oldAlbumId](const AnalysisAlbumTbl& info) {
            return info.albumIdOld.has_value() && info.albumIdOld.value() == oldAlbumId.value();
        });
    CHECK_AND_RETURN_LOG(it != groupPhotoAlbumInfos_.end() && it->albumIdNew.has_value(),
        "not find the needed album info, oldAlbumId: %{public}d", oldAlbumId.value());

    std::optional<int32_t> order = std::nullopt;
    CHECK_AND_EXECUTE(!isMapOrder_,
        order = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "order_position"));
    values.emplace_back(GetMapInsertValue(it->albumIdNew.value(), photoInfo.fileIdNew, order));
    std::lock_guard<mutex> lock(counterMutex_);
    std::string reportAlbumName = std::to_string(it->albumIdNew.value());
    CHECK_AND_EXECUTE(!it->albumName.has_value(), reportAlbumName += "_" + it->albumName.value());
    CHECK_AND_EXECUTE(albumPhotoCounter_.count(reportAlbumName) != 0,
        albumPhotoCounter_[reportAlbumName] = 0);
    albumPhotoCounter_[reportAlbumName]++;
}

void CloneRestoreGroupPhoto::UpdateMapInsertValues(std::vector<NativeRdb::ValuesBucket> &values)
{
    const std::string QUERY_SQL = "SELECT map.rowid, map.* FROM AnalysisPhotoMap AS map "
        " INNER JOIN AnalysisAlbum AS a ON map.map_album = a.album_id "
        " WHERE a.album_subtype = 4103 "
        " AND map.rowid > ? ORDER BY map.rowid LIMIT ? ";
    std::vector<NativeRdb::ValueObject> params = { lastIdOfMap_, PAGE_SIZE };
    auto resultSet = BackupDatabaseUtils::QuerySql(mediaRdb_, QUERY_SQL, params);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "query AnalysisPhotoMap err!");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        ParseMapInsertValues(values, resultSet);
    }
    resultSet->Close();
}

void CloneRestoreGroupPhoto::InsertAnalysisPhotoMap(std::vector<NativeRdb::ValuesBucket> &values)
{
    int64_t rowNum = 0;
    int32_t errCode = BatchInsertWithRetry("AnalysisPhotoMap", values, rowNum);
    if (errCode != E_OK || rowNum != static_cast<int64_t>(values.size())) {
        int64_t failNums = static_cast<int64_t>(values.size()) - rowNum;
        ErrorInfo errorInfo(RestoreError::INSERT_FAILED, 0, std::to_string(errCode),
            "insert into AnalysisPhotoMap fail, num:" + std::to_string(failNums));
        UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).ReportError(errorInfo);
        mapFailedCnt_ += failNums;
    }
    mapSuccessCnt_ += rowNum;
}

void CloneRestoreGroupPhoto::RestoreMapsBatch()
{
    std::vector<NativeRdb::ValuesBucket> values;
    int64_t startUpdateTime = MediaFileUtils::UTCTimeMilliSeconds();
    UpdateMapInsertValues(values);
    int64_t startInsertTime = MediaFileUtils::UTCTimeMilliSeconds();
    InsertAnalysisPhotoMap(values);
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("TimeCost: UpdateMapInsertValues: %{public}" PRId64 ", InsertAnalysisPhotoMap: %{public}" PRId64,
        startInsertTime - startUpdateTime, endTime - startInsertTime);
}

int32_t CloneRestoreGroupPhoto::RestoreMaps()
{
    int64_t startCloneTime = MediaFileUtils::UTCTimeMilliSeconds();
    isMapOrder_ = IsMapColumnOrderExist();
    const std::string QUERY_TOTAL_COUNT_SQL = "SELECT count(1) AS count FROM AnalysisPhotoMap AS map "
        " INNER JOIN AnalysisAlbum AS a ON map.map_album = a.album_id "
        " WHERE a.album_subtype = 4103 AND (a.is_removed != 1 OR a.is_removed IS NULL)";
    int32_t totalNumber = BackupDatabaseUtils::QueryInt(mediaRdb_, QUERY_TOTAL_COUNT_SQL, "count");
    for (int32_t offset = 0; offset < totalNumber; offset += QUERY_COUNT) {
        RestoreMapsBatch();
    }
    int64_t endCloneTime = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("Restore maps timecost of group photo album is %{public}" PRId64 "ms",
        endCloneTime - startCloneTime);
    return E_OK;
}

void CloneRestoreGroupPhoto::ReportRestoreTaskOfTotal()
{
    std::stringstream totalReport;
    totalReport << "timeCost: " << restoreTimeCost_ <<
        "; GROUP_PHOTO_ALBUM: max_id: " << maxAnalysisAlbumId_ << ", success: " << albumSuccessCnt_ <<
        ", deletedAlbum: " << albumDeleteCnt_ << ", failed: " << albumFailedCnt_ <<
        "; MAP: success: " << mapSuccessCnt_ << ", failed: " << mapFailedCnt_;
    UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_)
        .Report("CLONE_RESTORE_GROUP_PHOTO_TOTAL", RESTORE_STATUS_SUCCESS, totalReport.str());
}

void CloneRestoreGroupPhoto::ReportRestoreTaskOfAlbumStats()
{
    int32_t maxCnt = 0;
    int32_t totalCnt = 0;
    for (const auto &counter : albumPhotoCounter_) {
        maxCnt = maxCnt > counter.second ? maxCnt : counter.second;
        totalCnt += counter.second;
    }
    double meanCnt = albumPhotoCounter_.size() == 0 ? 0 : (double) totalCnt / albumPhotoCounter_.size();
    std::stringstream albumStatsReport;
    albumStatsReport << "AlbumNumber: " << albumPhotoCounter_.size() << ", maxAssetCount: " << maxCnt
        << ", meanAssetCount: " << meanCnt << ", totalAssetCount: " << totalCnt;
    UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_)
        .Report("CLONE_RESTORE_GROUP_PHOTO_ALBUM_STATS", RESTORE_STATUS_SUCCESS, albumStatsReport.str());
}

void CloneRestoreGroupPhoto::ReportRestoreTaskOfAlbumInfo()
{
    for (const auto &counter : albumPhotoCounter_) {
        std::stringstream albumInfoReport;
        albumInfoReport << "albumName: " << counter.first << ", photo count: " << counter.second;
        UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_)
            .ReportInAudit("CLONE_RESTORE_GROUP_PHOTO_ALBUM_INFO", RESTORE_STATUS_SUCCESS, albumInfoReport.str());
    }
}

void CloneRestoreGroupPhoto::ReportCloneRestoreGroupPhotoTask()
{
    ReportRestoreTaskOfTotal();
    ReportRestoreTaskOfAlbumStats();
    ReportRestoreTaskOfAlbumInfo();
}

bool CloneRestoreGroupPhoto::IsExistPortraitDataInOldDb()
{
    int index;
    int32_t value = -1;
    std::string querySql = "SELECT COUNT(1) FROM AnalysisAlbum WHERE album_type = 4096 AND album_subtype = 4102";
    AppendExtraWhereClause(querySql);
    auto resultSet = BackupDatabaseUtils::QuerySql(mediaRdb_, querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "query analysis portrait album err!");
    int32_t ret = resultSet->GoToFirstRow();
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, false, "fail to go to first row!");
    ret = resultSet->GetColumnIndex("COUNT(1)", index);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, false, "fail to get colunm index!");
    ret = resultSet->GetInt(index, value);
    resultSet->Close();
    return value > 0;
}

int32_t CloneRestoreGroupPhoto::DeleteGroupPhotoAlbumInNewDb(const std::vector<std::string> &deletedAlbumIds)
{
    CHECK_AND_RETURN_RET(!deletedAlbumIds.empty(), E_ERR);
    int32_t deleteRows = 0;
    NativeRdb::AbsRdbPredicates deletePredicates("AnalysisAlbum");
    deletePredicates.In("album_id", deletedAlbumIds);
    BackupDatabaseUtils::Delete(deletePredicates, deleteRows, mediaLibraryRdb_);
    albumDeleteCnt_ = deleteRows;
    MEDIA_INFO_LOG("delete exist analysis group photo album, delete nums: %{public}d", deleteRows);
    return deleteRows;
}

int32_t CloneRestoreGroupPhoto::DeleteGroupPhotoMapInNewDb(const std::vector<std::string> &deletedAlbumIds)
{
    CHECK_AND_RETURN_RET(!deletedAlbumIds.empty(), E_ERR);
    int32_t deleteRows = 0;
    NativeRdb::AbsRdbPredicates deletePredicates("AnalysisPhotoMap");
    deletePredicates.In("map_album", deletedAlbumIds);
    BackupDatabaseUtils::Delete(deletePredicates, deleteRows, mediaLibraryRdb_);
    MEDIA_INFO_LOG("delete exist analysis group photo album map, delete nums: %{public}d", deleteRows);
    return deleteRows;
}

int32_t CloneRestoreGroupPhoto::DeleteGroupPhotoAlbumInfoInNewDb()
{
    std::vector<string> deletedGroupPhotoAlbumIds;
    const std::string querySql = "SELECT album_id FROM AnalysisAlbum WHERE album_type = 4096 AND album_subtype = 4103";
    auto resultSet = BackupDatabaseUtils::QuerySql(mediaLibraryRdb_, querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "query analysis portrait album err!");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int albumId = get<int32_t>(ResultSetUtils::GetValFromColumn(MEDIA_DATA_DB_ALBUM_ID,
            resultSet, TYPE_INT32));
        deletedGroupPhotoAlbumIds.push_back(std::to_string(albumId));
    }
    if (deletedGroupPhotoAlbumIds.size() == 0) {
        MEDIA_INFO_LOG("The group photo album does not need to be deleted in new database");
        return E_OK;
    }
    int32_t ret = DeleteGroupPhotoMapInNewDb(deletedGroupPhotoAlbumIds);
    CHECK_AND_RETURN_RET_LOG(ret >= 0, E_ERR, "fail to delete group photo map in new medialibrary database");
    ret = DeleteGroupPhotoAlbumInNewDb(deletedGroupPhotoAlbumIds);
    CHECK_AND_RETURN_RET_LOG(ret >= 0, E_ERR, "fail to delete group photo album in new medialibrary database");
    return E_OK;
}
}   // OHOS::Media