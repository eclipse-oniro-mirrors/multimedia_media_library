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

#define MLOG_TAG "MediaLibraryCloneRestorePortrait"

#include "media_log.h"
#include "clone_restore_portrait_album.h"
#include "media_smart_album_column.h"
#include "backup_database_utils.h"
#include "backup_dfx_utils.h"
#include "media_file_utils.h"
#include "backup_const_column.h"
#include "upgrade_restore_task_report.h"

using namespace std;
namespace OHOS::Media {
const int32_t PAGE_SIZE = 200;

void CloneRestorePortrait::Init(int32_t sceneCode, const std::string &taskId,
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
    std::shared_ptr<NativeRdb::RdbStore> mediaRdb,
    const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap, bool isCloudRestoreSatisfied)
{
    MEDIA_INFO_LOG("CloneRestorePortrait Init");
    this->sceneCode_ = sceneCode;
    this->taskId_ = taskId;
    this->mediaRdb_ = mediaRdb;
    this->mediaLibraryRdb_ = mediaLibraryRdb;
    this->photoInfoMap_ = photoInfoMap;
    this->isCloudRestoreSatisfied_ = isCloudRestoreSatisfied;
}

void CloneRestorePortrait::Preprocess()
{
    MEDIA_INFO_LOG("Preprocess");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    CHECK_AND_RETURN_LOG(mediaRdb_ != nullptr && mediaLibraryRdb_ != nullptr, "rdbStore is nullptr.");
    std::string querySql = "SELECT count(1) AS count FROM " + ANALYSIS_ALBUM_TABLE + " WHERE ";
    std::string whereClause = "(" + SMARTALBUM_DB_ALBUM_TYPE + " = " + std::to_string(SMART) + " AND " +
        "album_subtype" + " = " + std::to_string(PORTRAIT) + ")";
    AppendExtraWhereClause(whereClause);
    querySql += whereClause;
    totalPortraitAlbumNumber_ = BackupDatabaseUtils::QueryInt(mediaRdb_, querySql, CUSTOM_COUNT);
    MEDIA_INFO_LOG("QueryPortraitAlbum totalNumber = %{public}d", totalPortraitAlbumNumber_);
    CHECK_AND_EXECUTE(!(totalPortraitAlbumNumber_ > 0), DeleteExistingPortraitInfos());
    
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    migratePortraitTotalTimeCost_ += end - start;
}

void CloneRestorePortrait::DeleteExistingPortraitInfos()
{
    MEDIA_INFO_LOG("DeleteExistingPortraitInfos");
    DeleteExistingImageFaceInfos();
    DeleteExistingPortraitAlbums();
    DeleteExistingCluseringInfo();
}

void CloneRestorePortrait::DeleteExistingPortraitAlbums()
{
    MEDIA_INFO_LOG("DeleteExistingPortraitAlbums");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    std::string deletePortraitAlbumSql =
        "DELETE FROM AnalysisAlbum WHERE "
        "album_type = 4096 AND album_subtype = 4102";
    BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, deletePortraitAlbumSql);
    
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("DeleteExistingPortraitAlbums cost %{public}lld", (long long)(end - start));
}

void CloneRestorePortrait::DeleteExistingCluseringInfo()
{
    MEDIA_INFO_LOG("DeleteClusteringInfo");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    std::string deleteFaceTagTblSql =
        "DELETE FROM " + VISION_FACE_TAG_TABLE;
    BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, deleteFaceTagTblSql);
    
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("DeleteExistingClusteringInfo cost %{public}lld", (long long)(end - start));
}

void CloneRestorePortrait::DeleteExistingImageFaceInfos()
{
    MEDIA_INFO_LOG("DeleteExistingImageFaceInfos");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    std::string fileIdFilterClause = std::string("(") + "SELECT " + IMAGE_FACE_COL_FILE_ID + " FROM " +
        VISION_IMAGE_FACE_TABLE + " UNION" + " SELECT " + VIDEO_FACE_COL_FILE_ID + " FROM " +
        VISION_VIDEO_FACE_TABLE + ")";
    std::string fileIdCondition = IMAGE_FACE_COL_FILE_ID + " IN " + fileIdFilterClause;

    MEDIA_INFO_LOG("Update TableAnalysisTotal");
    std::unique_ptr<NativeRdb::AbsRdbPredicates> totalTablePredicates =
        std::make_unique<NativeRdb::AbsRdbPredicates>(VISION_TOTAL_TABLE);
    std::string statusCondition = " status = 1";
    totalTablePredicates->SetWhereClause(fileIdCondition + " AND" + statusCondition);
    NativeRdb::ValuesBucket totalValues;
    totalValues.PutInt("face", 0);
    totalValues.PutInt("status", 0);
    int32_t totalUpdatedRows = 0;
    int32_t totalRet = BackupDatabaseUtils::Update(mediaLibraryRdb_,
        totalUpdatedRows, totalValues, totalTablePredicates);
    MEDIA_INFO_LOG("Update TableAnalysisTotal, updatedRows %{public}d", totalUpdatedRows);
    bool totalUpdateFailed = (totalUpdatedRows < 0 || totalRet < 0);
    CHECK_AND_RETURN_LOG(!totalUpdateFailed, "Failed to update VISION_TOTAL_TABLE face status field to 0");

    MEDIA_INFO_LOG("Update TableAnalysisSearchIndex");
    std::unique_ptr<NativeRdb::AbsRdbPredicates> searchIndexTablePredicates =
        std::make_unique<NativeRdb::AbsRdbPredicates>(ANALYSIS_SEARCH_INDEX_TABLE);
    statusCondition = " cv_status = 1";
    searchIndexTablePredicates->SetWhereClause(fileIdCondition + " AND" + statusCondition);
    NativeRdb::ValuesBucket searchIndexValues;
    searchIndexValues.PutInt("cv_status", 0);
    totalUpdatedRows = 0;
    totalRet = BackupDatabaseUtils::Update(mediaLibraryRdb_,
        totalUpdatedRows, searchIndexValues, searchIndexTablePredicates);
    MEDIA_INFO_LOG("Update TabAnalysisSearchIndex, updatedRows %{public}d", totalUpdatedRows);
    totalUpdateFailed = (totalUpdatedRows < 0 || totalRet < 0);
    CHECK_AND_RETURN_LOG(!totalUpdateFailed, "Failed to update ANALYSIS_SEARCH_INDEX_TABLE cv_status field to 0");

    MEDIA_INFO_LOG("Delete AnalysisPhotoMap");
    std::string deleteAnalysisPhotoMapSql =
        "DELETE FROM AnalysisPhotoMap WHERE "
        "map_album IN (SELECT album_id FROM AnalysisAlbum WHERE album_type = 4096 AND album_subtype = 4102) "
        "AND map_asset IN" + fileIdFilterClause;
    BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, deleteAnalysisPhotoMapSql);
    
    MEDIA_INFO_LOG("Delete ImageFaceTable");
    std::string deleteImageFaceSql = "DELETE FROM " + VISION_IMAGE_FACE_TABLE;
    BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, deleteImageFaceSql);
    
    MEDIA_INFO_LOG("Delete VideoFaceTable");
    std::string deleteVideoFaceSql = "DELETE FROM " + VISION_VIDEO_FACE_TABLE;
    BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, deleteVideoFaceSql);
    
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("DeleteExistingImageFaceInfo cost %{public}lld", (long long)(end - start));
}

void CloneRestorePortrait::Restore()
{
    MEDIA_INFO_LOG("Start Restore");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    CHECK_AND_RETURN_LOG(mediaRdb_ != nullptr && mediaLibraryRdb_ != nullptr, "rdbStore is nullptr.");
    RestoreFromGalleryPortraitAlbum();
    RestorePortraitClusteringInfo();
    RestoreImageFaceInfo();
    BackupDatabaseUtils::UpdateFaceAnalysisTblStatus(mediaLibraryRdb_);
    int32_t ret = RestoreMaps();
    CHECK_AND_RETURN_LOG(ret == E_OK, "fail to update analysis photo map status");
    RestoreAnalysisTotalFaceStatus();
    ReportPortraitCloneStat(sceneCode_);
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    migratePortraitTotalTimeCost_ += end - start;
}

void CloneRestorePortrait::RestoreAnalysisTotalFaceStatus()
{
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    CloneRestoreAnalysisTotal cloneRestoreAnalysisTotal;
    cloneRestoreAnalysisTotal.Init("face", PAGE_SIZE, mediaRdb_, mediaLibraryRdb_);
    int32_t totalNumber = cloneRestoreAnalysisTotal.GetTotalNumber();
    for (int32_t offset = 0; offset < totalNumber; offset += PAGE_SIZE) {
        cloneRestoreAnalysisTotal.GetInfos(photoInfoMap_);
        cloneRestoreAnalysisTotal.UpdateDatabase();
    }
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("TimeCost: UpdateDatabase: %{public}" PRId64, end - start);
}

void CloneRestorePortrait::RestoreFromGalleryPortraitAlbum()
{
    MEDIA_INFO_LOG("RestoreFromGalleryPortraitAlbum");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    RecordOldPortraitAlbumDfx();
    GetMaxAlbumId();
    std::vector<std::string> commonColumn = BackupDatabaseUtils::GetCommonColumnInfos(mediaRdb_, mediaLibraryRdb_,
        ANALYSIS_ALBUM_TABLE);
    std::vector<std::string> commonColumns = BackupDatabaseUtils::filterColumns(commonColumn,
        EXCLUDED_PORTRAIT_COLUMNS);

    for (int32_t offset = 0; offset < totalPortraitAlbumNumber_; offset += QUERY_COUNT) {
        std::vector<AnalysisAlbumTbl> portraitAlbumTbl = QueryPortraitAlbumTbl(offset, commonColumns);
        for (const auto& album : portraitAlbumTbl) {
            if (album.tagId.has_value() && album.coverUri.has_value()) {
                coverUriInfo_.emplace_back(album.tagId.value(),
                    std::make_pair(album.coverUri.value(),
                    album.isCoverSatisfied.value_or(INVALID_COVER_SATISFIED_STATUS)));
            }
        }
        InsertPortraitAlbum(portraitAlbumTbl);
    }

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("RestoreFromGalleryPortraitAlbum cost %{public}lld", (long long)(end - start));

    LogPortraitCloneDfx();
}

void CloneRestorePortrait::RecordOldPortraitAlbumDfx()
{
    int32_t offset {0};
    int32_t rowCount {0};
    std::vector<PortraitAlbumDfx> albums;

    do {
        auto batchResults =  QueryAllPortraitAlbum(offset, rowCount);
        if (!batchResults.empty()) {
            albums.insert(albums.end(), batchResults.begin(), batchResults.end());
        }

        offset += QUERY_COUNT;
    } while (rowCount > 0);

    for (const auto& album : albums) {
        PortraitAlbumDfx dfxInfo;
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
        if (album.count.has_value()) {
            dfxInfo.count = album.count.value();
        }

        portraitAlbumDfx_.push_back(dfxInfo);
    }
}

void CloneRestorePortrait::LogPortraitCloneDfx()
{
    std::vector<std::string> failedAlbums;
    std::unordered_set<std::string> existingTagIds = QueryAllPortraitAlbum();

    for (const auto& oldAlbum : portraitAlbumDfx_) {
        if (!oldAlbum.tagId.has_value()) {
            continue;
        }

        if (existingTagIds.find(oldAlbum.tagId.value()) == existingTagIds.end()) {
            std::string albumInfo = "Album: " + oldAlbum.albumName.value_or("Unknown") +
                ", TagId: " + oldAlbum.tagId.value() +
                ", Cover: " + oldAlbum.coverUri.value_or("Unknown") +
                ", Count: " + std::to_string(oldAlbum.count.value_or(0));
            failedAlbums.push_back(albumInfo);
        }
    }

    if (!failedAlbums.empty()) {
        MEDIA_ERR_LOG("Following portrait albums failed to clone completely:");
        for (const auto& failedAlbum : failedAlbums) {
            MEDIA_ERR_LOG("%{public}s", failedAlbum.c_str());
        }
    } else {
        MEDIA_INFO_LOG("All portrait albums cloned successfully");
    }

    MEDIA_INFO_LOG("Stat: Total albums: %{public}zu, Failed albums count: %{public}zu",
        portraitAlbumDfx_.size(), failedAlbums.size());
}

std::vector<PortraitAlbumDfx> CloneRestorePortrait::QueryAllPortraitAlbum(int32_t& offset, int32_t& rowCount)
{
    std::vector<PortraitAlbumDfx> result;
    result.reserve(QUERY_COUNT);

    const std::string querySql = "SELECT album_name, cover_uri, tag_id, count "
        "FROM AnalysisAlbum "
        "WHERE album_type = ? "
        "AND album_subtype = ? "
        "LIMIT ?, ?";

    std::vector<NativeRdb::ValueObject> bindArgs = { SMART, PORTRAIT, offset, QUERY_COUNT };
    CHECK_AND_RETURN_RET_LOG(this->mediaRdb_ != nullptr, result, "Media_Restore: mediaRdb_ is null.");
    auto resultSet = mediaRdb_->QuerySql(querySql, bindArgs);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, result, "Query resultSql is null.");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        PortraitAlbumDfx dfxInfo;
        dfxInfo.albumName = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, ANALYSIS_COL_ALBUM_NAME);
        dfxInfo.coverUri = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, ANALYSIS_COL_COVER_URI);
        dfxInfo.tagId = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, ANALYSIS_COL_TAG_ID);
        dfxInfo.count = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, ANALYSIS_COL_COUNT);

        result.push_back(dfxInfo);
    }
    resultSet->GetRowCount(rowCount);
    resultSet->Close();
    return result;
}

std::unordered_set<std::string> CloneRestorePortrait::QueryAllPortraitAlbum()
{
    std::unordered_set<std::string> result;
    std::vector<std::string> tagIds;
    for (const auto& oldAlbum : portraitAlbumDfx_) {
        if (oldAlbum.tagId.has_value()) {
            tagIds.push_back(oldAlbum.tagId.value());
        }
    }

    CHECK_AND_RETURN_RET_LOG(!tagIds.empty(), result, "No valid tag_ids found in old albums");

    std::string querySql = "SELECT tag_id FROM " + ANALYSIS_ALBUM_TABLE +
        " WHERE tag_id IN (" + BackupDatabaseUtils::JoinSQLValues<string>(tagIds, ", ") + ")";

    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaLibraryRdb_, querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, result, "Query resultSql is null.");

    std::string dfxInfo;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        dfxInfo =
            BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, ANALYSIS_COL_TAG_ID).value_or("");
        result.insert(dfxInfo);
    }

    resultSet->Close();
    return result;
}

vector<AnalysisAlbumTbl> CloneRestorePortrait::QueryPortraitAlbumTbl(int32_t offset,
    const std::vector<std::string>& commonColumns)
{
    vector<AnalysisAlbumTbl> result;
    result.reserve(QUERY_COUNT);

    std::string inClause = BackupDatabaseUtils::JoinValues<string>(commonColumns, ", ");
    std::string querySql =
        "SELECT " + inClause +
        " FROM " + ANALYSIS_ALBUM_TABLE +
        " WHERE ";
    std::string whereClause = "(" +
        SMARTALBUM_DB_ALBUM_TYPE + " = " + std::to_string(SMART) + " AND " +
        "album_subtype" + " = " + std::to_string(PORTRAIT) + ")";
    AppendExtraWhereClause(whereClause);
    querySql += whereClause;
    querySql += " LIMIT " + std::to_string(offset) + ", " + std::to_string(QUERY_COUNT);

    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaRdb_, querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, result, "Query resultSql is null.");
    GetMaxAlbumId();
    int32_t albumIdNow = maxAnalysisAlbumId_ + 1;

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        AnalysisAlbumTbl portraitAlbumTbl;
        ParseAlbumResultSet(resultSet, portraitAlbumTbl);
        portraitAlbumTbl.albumIdNew = std::make_optional<int32_t>(albumIdNow++);
        result.emplace_back(portraitAlbumTbl);
    }
    resultSet->Close();
    return result;
}

void CloneRestorePortrait::InsertPortraitAlbum(std::vector<AnalysisAlbumTbl> &portraitAlbumTbl)
{
    CHECK_AND_RETURN_LOG(mediaLibraryRdb_ != nullptr, "mediaLibraryRdb_ is null");
    CHECK_AND_RETURN_LOG(!portraitAlbumTbl.empty(), "portraitAlbumTbl are empty");

    std::vector<std::string> albumNames;
    std::vector<std::string> tagIds;

    for (const auto &album : portraitAlbumTbl) {
        CHECK_AND_EXECUTE(!album.albumName.has_value(), albumNames.emplace_back(album.albumName.value()));
        CHECK_AND_EXECUTE(!album.tagId.has_value(), tagIds.emplace_back(album.tagId.value()));
    }
    MEDIA_INFO_LOG("InsertPortraitAlbum, total albums: %{public}zu, Albums with names: %{public}zu,"
        "Albums with tagIds: %{public}zu", portraitAlbumTbl.size(), albumNames.size(), tagIds.size());

    int32_t albumRowNum = InsertPortraitAlbumByTable(portraitAlbumTbl);
    CHECK_AND_PRINT_LOG(albumRowNum != E_ERR, "Failed to insert album");

    migratePortraitAlbumNumber_ += static_cast<uint64_t>(albumRowNum);
    return;
}

void CloneRestorePortrait::UpdatePortraitTblAlbumId(std::vector<AnalysisAlbumTbl> &portraitAlbumTbl, int32_t minId)
{
    const std::string QUERY_SQL = "SELECT " + ANALYSIS_COL_ALBUM_ID + ", " + ANALYSIS_COL_TAG_ID + " FROM " +
        ANALYSIS_ALBUM_TABLE + " WHERE " + ANALYSIS_COL_ALBUM_SUBTYPE + " = " + std::to_string(PORTRAIT) +
        " AND " + ANALYSIS_COL_ALBUM_ID + " > ?";
    std::vector<NativeRdb::ValueObject> params = { minId };
    auto resultSet = BackupDatabaseUtils::QuerySql(mediaLibraryRdb_, QUERY_SQL, params);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "query resultSet is null");
    int resultRowCount = 0;
    int32_t errCode = resultSet->GetRowCount(resultRowCount);
    if (errCode != NativeRdb::E_OK || resultRowCount < 0) {
        MEDIA_ERR_LOG("resultSet GetRowCount failed");
        resultSet->Close();
        return;
    }
    std::unordered_map<std::string, int32_t> portraitAlbumIdMap;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t albumId = GetInt32Val(ANALYSIS_COL_ALBUM_ID, resultSet);
        std::string tagId = GetStringVal(ANALYSIS_COL_TAG_ID, resultSet);
        bool cond = (albumId <= 0 || tagId.empty());
        CHECK_AND_CONTINUE(!cond);
        portraitAlbumIdMap[tagId] = albumId;
    }
    resultSet->Close();
    for (auto it = portraitAlbumTbl.begin(); it != portraitAlbumTbl.end(); ++it) {
        it->albumIdNew = std::nullopt;
        CHECK_AND_CONTINUE(it->tagId.has_value() &&
            portraitAlbumIdMap.find(it->tagId.value()) != portraitAlbumIdMap.end());
        it->albumIdNew = portraitAlbumIdMap[it->tagId.value()];
    }
}

int32_t CloneRestorePortrait::InsertPortraitAlbumByTable(std::vector<AnalysisAlbumTbl> &portraitAlbumTbl)
{
    std::vector<NativeRdb::ValuesBucket> valuesBuckets = GetInsertValues(portraitAlbumTbl);
    int64_t rowNum = 0;
    int32_t maxAnalysisAlbumIdBeforePortraitInsert =
        BackupDatabaseUtils::QueryMaxId(mediaLibraryRdb_, ANALYSIS_ALBUM_TABLE, ANALYSIS_COL_ALBUM_ID);
    int32_t ret = BatchInsertWithRetry(ANALYSIS_ALBUM_TABLE, valuesBuckets, rowNum);
    CHECK_AND_RETURN_RET(ret == E_OK, E_ERR);
    UpdatePortraitTblAlbumId(portraitAlbumTbl, maxAnalysisAlbumIdBeforePortraitInsert);
    portraitAlbumInfoMap_.insert(portraitAlbumInfoMap_.end(), portraitAlbumTbl.begin(), portraitAlbumTbl.end());
    return rowNum;
}

vector<NativeRdb::ValuesBucket> CloneRestorePortrait::GetInsertValues(std::vector<AnalysisAlbumTbl> &portraitAlbumTbl)
{
    std::vector<NativeRdb::ValuesBucket> values;
    for (auto &portraitAlbumInfo : portraitAlbumTbl) {
        NativeRdb::ValuesBucket value;
        GetAnalysisAlbumInsertValue(value, portraitAlbumInfo);
        values.emplace_back(value);
    }
    return values;
}

void CloneRestorePortrait::RestorePortraitClusteringInfo()
{
    MEDIA_INFO_LOG("RestorePortraitClusteringInfo");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    int32_t totalNumber = BackupDatabaseUtils::QueryInt(mediaRdb_, QUERY_FACE_TAG_COUNT, CUSTOM_COUNT);
    MEDIA_INFO_LOG("QueryPortraitClustering totalNumber = %{public}d", totalNumber);

    std::vector<std::string> commonColumn = BackupDatabaseUtils::GetCommonColumnInfos(mediaRdb_, mediaLibraryRdb_,
        VISION_FACE_TAG_TABLE);
    std::vector<std::string> commonColumns = BackupDatabaseUtils::filterColumns(commonColumn,
        EXCLUDED_FACE_TAG_COLUMNS);
    BackupDatabaseUtils::LeftJoinValues<string>(commonColumns, "vft.");
    std::string inClause = BackupDatabaseUtils::JoinValues<string>(commonColumns, ", ");
    BackupDatabaseUtils::ExecuteSQL(mediaRdb_, CREATE_FACE_TAG_INDEX);
    for (int32_t offset = 0; offset < totalNumber; offset += QUERY_COUNT) {
        vector<FaceTagTbl> faceTagTbls = QueryFaceTagTbl(offset, inClause);
        BatchInsertFaceTags(faceTagTbls);
        if (static_cast<std::int32_t>(faceTagTbls.size()) < QUERY_COUNT) {
            break;
        }
    }

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("RestorePortraitClusteringInfo cost %{public}lld", (long long)(end - start));
}

std::vector<FaceTagTbl> CloneRestorePortrait::QueryFaceTagTbl(int32_t offset, const std::string &inClause)
{
    std::vector<FaceTagTbl> result;

    std::string querySql = "SELECT DISTINCT " + inClause +
        " FROM " + VISION_FACE_TAG_TABLE + " vft" +
        " LEFT JOIN AnalysisAlbum aa ON aa.tag_id = vft.tag_id" +
        " LEFT JOIN AnalysisPhotoMap apm ON aa.album_id = apm.map_album" +
        " LEFT JOIN Photos ph ON ph.file_id = apm.map_asset" +
        " WHERE " +
        (isCloudRestoreSatisfied_ ? "ph.position IN (1, 2, 3)" : "ph.position IN (1, 3)");

    querySql += " LIMIT " + std::to_string(offset) + ", " + std::to_string(QUERY_COUNT);

    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaRdb_, querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, result, "Query resultSet is null.");
    int resultRowCount = 0;
    resultSet->GetRowCount(resultRowCount);
    result.reserve(resultRowCount);
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        FaceTagTbl faceTagTbl;
        ParseFaceTagResultSet(resultSet, faceTagTbl);
        result.emplace_back(faceTagTbl);
    }

    resultSet->Close();
    return result;
}

void CloneRestorePortrait::ParseFaceTagResultSet(const std::shared_ptr<NativeRdb::ResultSet>& resultSet,
    FaceTagTbl& faceTagTbl)
{
    faceTagTbl.tagId = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, FACE_TAG_COL_TAG_ID);
    faceTagTbl.tagName = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, FACE_TAG_COL_TAG_NAME);
    faceTagTbl.groupTag = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, FACE_TAG_COL_GROUP_TAG);
    faceTagTbl.centerFeatures = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        FACE_TAG_COL_CENTER_FEATURES);
    faceTagTbl.tagVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, FACE_TAG_COL_TAG_VERSION);
    faceTagTbl.analysisVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        FACE_TAG_COL_ANALYSIS_VERSION);
    faceTagTbl.age = BackupDatabaseUtils::GetOptionalValue<double>(resultSet, FACE_TAG_COL_AGE);
    faceTagTbl.gender = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, FACE_TAG_COL_GENDER);
}

void CloneRestorePortrait::BatchInsertFaceTags(const std::vector<FaceTagTbl>& faceTagTbls)
{
    std::vector<NativeRdb::ValuesBucket> valuesBuckets;
    for (const auto& faceTagTbl : faceTagTbls) {
        valuesBuckets.push_back(CreateValuesBucketFromFaceTagTbl(faceTagTbl));
    }

    int64_t rowNum = 0;
    int32_t ret = BatchInsertWithRetry(VISION_FACE_TAG_TABLE, valuesBuckets, rowNum);
    migratePortraitFaceTagNumber_ += rowNum;
    CHECK_AND_RETURN_LOG(ret == E_OK, "Failed to batch insert face tags");
}

NativeRdb::ValuesBucket CloneRestorePortrait::CreateValuesBucketFromFaceTagTbl(const FaceTagTbl& faceTagTbl)
{
    NativeRdb::ValuesBucket values;

    BackupDatabaseUtils::PutIfPresent(values, FACE_TAG_COL_TAG_ID, faceTagTbl.tagId);
    BackupDatabaseUtils::PutIfPresent(values, FACE_TAG_COL_TAG_NAME, faceTagTbl.tagName);
    BackupDatabaseUtils::PutIfPresent(values, FACE_TAG_COL_CENTER_FEATURES, faceTagTbl.centerFeatures);
    BackupDatabaseUtils::PutIfPresent(values, FACE_TAG_COL_TAG_VERSION, faceTagTbl.tagVersion);
    BackupDatabaseUtils::PutIfPresent(values, FACE_TAG_COL_ANALYSIS_VERSION, faceTagTbl.analysisVersion);
    BackupDatabaseUtils::PutIfPresent(values, FACE_TAG_COL_AGE, faceTagTbl.age);
    BackupDatabaseUtils::PutIfPresent(values, FACE_TAG_COL_GENDER, faceTagTbl.gender);

    return values;
}

template<typename T, typename U>
void CloneRestorePortrait::PutWithDefault(NativeRdb::ValuesBucket& values, const std::string& columnName,
    const std::optional<T>& optionalValue, const U& defaultValue)
{
    if (optionalValue.has_value()) {
        BackupDatabaseUtils::PutIfPresent(values, columnName, optionalValue);
    } else {
        BackupDatabaseUtils::PutIfPresent(values, columnName, std::optional<T>(static_cast<T>(defaultValue)));
    }
}

void CloneRestorePortrait::RestoreImageFaceInfo()
{
    MEDIA_INFO_LOG("RestoreImageFaceInfo");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    auto uniqueFileIdPairs = CollectFileIdPairs(photoInfoMap_);
    auto [oldFileIds, newFileIds] = BackupDatabaseUtils::UnzipFileIdPairs(uniqueFileIdPairs);

    std::string fileIdOldInClause = "(" + BackupDatabaseUtils::JoinValues<int>(oldFileIds, ", ") + ")";

    std::string querySql = QUERY_IMAGE_FACE_COUNT;
    querySql += " WHERE " + IMAGE_FACE_COL_FILE_ID + " IN " + fileIdOldInClause;
    int32_t totalNumber = BackupDatabaseUtils::QueryInt(mediaRdb_, querySql, CUSTOM_COUNT);
    MEDIA_INFO_LOG("QueryImageFaceTotalNumber, totalNumber = %{public}d", totalNumber);
    if (totalNumber == 0) {
        return;
    }

    std::vector<std::string> commonColumn = BackupDatabaseUtils::GetCommonColumnInfos(mediaRdb_, mediaLibraryRdb_,
        VISION_IMAGE_FACE_TABLE);
    std::vector<std::string> commonColumns = BackupDatabaseUtils::filterColumns(commonColumn,
        EXCLUDED_IMAGE_FACE_COLUMNS);

    BackupDatabaseUtils::DeleteExistingImageFaceData(mediaLibraryRdb_, uniqueFileIdPairs);
    for (int32_t offset = 0; offset < totalNumber; offset += QUERY_COUNT) {
        std::vector<ImageFaceTbl> imageFaceTbls = QueryImageFaceTbl(offset, fileIdOldInClause, commonColumns);
        auto imageFaces = ProcessImageFaceTbls(imageFaceTbls, uniqueFileIdPairs);
        BatchInsertImageFaces(imageFaces);
    }

    GenNewCoverUris(coverUriInfo_);

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("RestoreImageFaceInfo cost %{public}lld", (long long)(end - start));
}

std::vector<ImageFaceTbl> CloneRestorePortrait::ProcessImageFaceTbls(const std::vector<ImageFaceTbl>& imageFaceTbls,
    const std::vector<FileIdPair>& fileIdPairs)
{
    CHECK_AND_RETURN_RET_LOG(!imageFaceTbls.empty(), {}, "image faces tbl empty");

    std::vector<ImageFaceTbl> imageFaceNewTbls;
    imageFaceNewTbls.reserve(imageFaceTbls.size());

    for (const auto& imageFaceTbl : imageFaceTbls) {
        if (imageFaceTbl.fileId.has_value()) {
            int32_t oldFileId = imageFaceTbl.fileId.value();
            auto it = std::find_if(fileIdPairs.begin(), fileIdPairs.end(),
                [oldFileId](const FileIdPair& pair) { return pair.first == oldFileId; });
            if (it != fileIdPairs.end()) {
                ImageFaceTbl updatedFace = imageFaceTbl;
                updatedFace.fileId = it->second;
                imageFaceNewTbls.push_back(std::move(updatedFace));
            }
        }
    }

    return imageFaceNewTbls;
}

std::vector<ImageFaceTbl> CloneRestorePortrait::QueryImageFaceTbl(int32_t offset, std::string &fileIdClause,
    const std::vector<std::string> &commonColumns)
{
    std::vector<ImageFaceTbl> result;
    result.reserve(QUERY_COUNT);

    std::string inClause = BackupDatabaseUtils::JoinValues<string>(commonColumns, ", ");
    std::string querySql =
        "SELECT " + inClause +
        " FROM " + VISION_IMAGE_FACE_TABLE;
    querySql += " WHERE " + IMAGE_FACE_COL_FILE_ID + " IN " + fileIdClause;
    querySql += " LIMIT " + std::to_string(offset) + ", " + std::to_string(QUERY_COUNT);

    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaRdb_, querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, result, "Query resultSet is null.");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        ImageFaceTbl imageFaceTbl;
        ParseImageFaceResultSet(resultSet, imageFaceTbl);
        result.emplace_back(imageFaceTbl);
    }

    resultSet->Close();
    return result;
}

void CloneRestorePortrait::ParseImageFaceResultSet(const std::shared_ptr<NativeRdb::ResultSet>& resultSet,
    ImageFaceTbl& imageFaceTbl)
{
    imageFaceTbl.fileId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, IMAGE_FACE_COL_FILE_ID);
    imageFaceTbl.faceId = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, IMAGE_FACE_COL_FACE_ID);
    imageFaceTbl.tagId = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, IMAGE_FACE_COL_TAG_ID);
    imageFaceTbl.scaleX = BackupDatabaseUtils::GetOptionalValue<double>(resultSet, IMAGE_FACE_COL_SCALE_X);
    imageFaceTbl.scaleY = BackupDatabaseUtils::GetOptionalValue<double>(resultSet, IMAGE_FACE_COL_SCALE_Y);
    imageFaceTbl.scaleWidth = BackupDatabaseUtils::GetOptionalValue<double>(resultSet, IMAGE_FACE_COL_SCALE_WIDTH);
    imageFaceTbl.scaleHeight = BackupDatabaseUtils::GetOptionalValue<double>(resultSet, IMAGE_FACE_COL_SCALE_HEIGHT);
    imageFaceTbl.landmarks = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, IMAGE_FACE_COL_LANDMARKS);
    imageFaceTbl.pitch = BackupDatabaseUtils::GetOptionalValue<double>(resultSet, IMAGE_FACE_COL_PITCH);
    imageFaceTbl.yaw = BackupDatabaseUtils::GetOptionalValue<double>(resultSet, IMAGE_FACE_COL_YAW);
    imageFaceTbl.roll = BackupDatabaseUtils::GetOptionalValue<double>(resultSet, IMAGE_FACE_COL_ROLL);
    imageFaceTbl.prob = BackupDatabaseUtils::GetOptionalValue<double>(resultSet, IMAGE_FACE_COL_PROB);
    imageFaceTbl.totalFaces = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, IMAGE_FACE_COL_TOTAL_FACES);
    imageFaceTbl.faceVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        IMAGE_FACE_COL_FACE_VERSION);
    imageFaceTbl.featuresVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        IMAGE_FACE_COL_FEATURES_VERSION);
    imageFaceTbl.features = BackupDatabaseUtils::GetOptionalValue<std::vector<uint8_t>>(resultSet,
        IMAGE_FACE_COL_FEATURES);
    imageFaceTbl.faceOcclusion = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet,
        IMAGE_FACE_COL_FACE_OCCLUSION);
    imageFaceTbl.analysisVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        IMAGE_FACE_COL_ANALYSIS_VERSION);
    imageFaceTbl.beautyBounderX = BackupDatabaseUtils::GetOptionalValue<double>(resultSet,
        IMAGE_FACE_COL_BEAUTY_BOUNDER_X);
    imageFaceTbl.beautyBounderY = BackupDatabaseUtils::GetOptionalValue<double>(resultSet,
        IMAGE_FACE_COL_BEAUTY_BOUNDER_Y);
    imageFaceTbl.beautyBounderWidth = BackupDatabaseUtils::GetOptionalValue<double>(resultSet,
        IMAGE_FACE_COL_BEAUTY_BOUNDER_WIDTH);
    imageFaceTbl.beautyBounderHeight = BackupDatabaseUtils::GetOptionalValue<double>(resultSet,
        IMAGE_FACE_COL_BEAUTY_BOUNDER_HEIGHT);
    imageFaceTbl.aestheticsScore = BackupDatabaseUtils::GetOptionalValue<double>(resultSet,
        IMAGE_FACE_COL_AESTHETICS_SCORE);
    imageFaceTbl.beautyBounderVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        IMAGE_FACE_COL_BEAUTY_BOUNDER_VERSION);
    imageFaceTbl.isExcluded = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, IMAGE_FACE_COL_IS_EXCLUDED);
    imageFaceTbl.faceClarity = BackupDatabaseUtils::GetOptionalValue<double>(resultSet, IMAGE_FACE_COL_FACE_CLARITY);
    imageFaceTbl.faceLuminance = BackupDatabaseUtils::GetOptionalValue<double>(resultSet,
        IMAGE_FACE_COL_FACE_LUMINANCE);
    imageFaceTbl.faceSaturation = BackupDatabaseUtils::GetOptionalValue<double>(resultSet,
        IMAGE_FACE_COL_FACE_SATURATION);
    imageFaceTbl.faceEyeClose = BackupDatabaseUtils::GetOptionalValue<double>(resultSet,
        IMAGE_FACE_COL_FACE_EYE_CLOSE);
    imageFaceTbl.faceExpression = BackupDatabaseUtils::GetOptionalValue<double>(resultSet,
        IMAGE_FACE_COL_FACE_EXPRESSION);
    ParseImageFaceResultSet1(resultSet, imageFaceTbl);
}

void  CloneRestorePortrait::ParseImageFaceResultSet1(const std::shared_ptr<NativeRdb::ResultSet>& resultSet,
    ImageFaceTbl& imageFaceTbl)
{
    imageFaceTbl.preferredGrade = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        IMAGE_FACE_COL_PREFERRED_GRADE);
    imageFaceTbl.jointBeautyBounderX = BackupDatabaseUtils::GetOptionalValue<double>(resultSet,
        IMAGE_FACE_COL_JOINT_BEAUTY_BOUNDER_X);
    imageFaceTbl.jointBeautyBounderY = BackupDatabaseUtils::GetOptionalValue<double>(resultSet,
        IMAGE_FACE_COL_JOINT_BEAUTY_BOUNDER_Y);
    imageFaceTbl.jointBeautyBounderWidth = BackupDatabaseUtils::GetOptionalValue<double>(resultSet,
        IMAGE_FACE_COL_JOINT_BEAUTY_BOUNDER_WIDTH);
    imageFaceTbl.jointBeautyBounderHeight = BackupDatabaseUtils::GetOptionalValue<double>(resultSet,
        IMAGE_FACE_COL_JOINT_BEAUTY_BOUNDER_HEIGHT);
    imageFaceTbl.groupVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        IMAGE_FACE_COL_GROUP_VERSION);
    imageFaceTbl.faceDetailVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        IMAGE_FACE_COL_FACE_DETAIL_VERSION);
    imageFaceTbl.age = BackupDatabaseUtils::GetOptionalValue<double>(resultSet,
        IMAGE_FACE_COL_AGE);
    imageFaceTbl.gender = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet,
        IMAGE_FACE_COL_GENDER);
}

void CloneRestorePortrait::BatchInsertImageFaces(const std::vector<ImageFaceTbl>& imageFaceTbls)
{
    std::vector<NativeRdb::ValuesBucket> valuesBuckets;
    std::unordered_set<int32_t> fileIdSet;
    for (const auto& imageFaceTbl : imageFaceTbls) {
        valuesBuckets.push_back(CreateValuesBucketFromImageFaceTbl(imageFaceTbl));
    }

    int64_t rowNum = 0;
    int32_t ret = BatchInsertWithRetry(VISION_IMAGE_FACE_TABLE, valuesBuckets, rowNum);
    CHECK_AND_RETURN_LOG(ret == E_OK, "Failed to batch insert image faces");

    for (const auto& imageFaceTbl : imageFaceTbls) {
        if (imageFaceTbl.fileId.has_value()) {
            fileIdSet.insert(imageFaceTbl.fileId.value());
        }
    }

    migratePortraitFaceNumber_ += rowNum;
    migratePortraitPhotoNumber_ += fileIdSet.size();
}

NativeRdb::ValuesBucket CloneRestorePortrait::CreateValuesBucketFromImageFaceTbl(const ImageFaceTbl& imageFaceTbl)
{
    NativeRdb::ValuesBucket values;

    BackupDatabaseUtils::PutIfPresent(values, IMAGE_FACE_COL_FILE_ID, imageFaceTbl.fileId);
    BackupDatabaseUtils::PutIfPresent(values, IMAGE_FACE_COL_FACE_ID, imageFaceTbl.faceId);
    BackupDatabaseUtils::PutIfPresent(values, IMAGE_FACE_COL_TAG_ID, imageFaceTbl.tagId);
    BackupDatabaseUtils::PutIfPresent(values, IMAGE_FACE_COL_SCALE_X, imageFaceTbl.scaleX);
    BackupDatabaseUtils::PutIfPresent(values, IMAGE_FACE_COL_SCALE_Y, imageFaceTbl.scaleY);
    BackupDatabaseUtils::PutIfPresent(values, IMAGE_FACE_COL_SCALE_WIDTH, imageFaceTbl.scaleWidth);
    BackupDatabaseUtils::PutIfPresent(values, IMAGE_FACE_COL_SCALE_HEIGHT, imageFaceTbl.scaleHeight);
    BackupDatabaseUtils::PutIfPresent(values, IMAGE_FACE_COL_LANDMARKS, imageFaceTbl.landmarks);
    BackupDatabaseUtils::PutIfPresent(values, IMAGE_FACE_COL_PITCH, imageFaceTbl.pitch);
    BackupDatabaseUtils::PutIfPresent(values, IMAGE_FACE_COL_YAW, imageFaceTbl.yaw);
    BackupDatabaseUtils::PutIfPresent(values, IMAGE_FACE_COL_ROLL, imageFaceTbl.roll);
    BackupDatabaseUtils::PutIfPresent(values, IMAGE_FACE_COL_PROB, imageFaceTbl.prob);
    BackupDatabaseUtils::PutIfPresent(values, IMAGE_FACE_COL_TOTAL_FACES, imageFaceTbl.totalFaces);
    BackupDatabaseUtils::PutIfPresent(values, IMAGE_FACE_COL_FACE_VERSION, imageFaceTbl.faceVersion);
    BackupDatabaseUtils::PutIfPresent(values, IMAGE_FACE_COL_FEATURES_VERSION, imageFaceTbl.featuresVersion);
    BackupDatabaseUtils::PutIfPresent(values, IMAGE_FACE_COL_FEATURES, imageFaceTbl.features);
    BackupDatabaseUtils::PutIfPresent(values, IMAGE_FACE_COL_FACE_OCCLUSION, imageFaceTbl.faceOcclusion);
    BackupDatabaseUtils::PutIfPresent(values, IMAGE_FACE_COL_ANALYSIS_VERSION, imageFaceTbl.analysisVersion);
    BackupDatabaseUtils::PutIfPresent(values, IMAGE_FACE_COL_BEAUTY_BOUNDER_X, imageFaceTbl.beautyBounderX);
    BackupDatabaseUtils::PutIfPresent(values, IMAGE_FACE_COL_BEAUTY_BOUNDER_Y, imageFaceTbl.beautyBounderY);
    BackupDatabaseUtils::PutIfPresent(values, IMAGE_FACE_COL_BEAUTY_BOUNDER_WIDTH, imageFaceTbl.beautyBounderWidth);
    BackupDatabaseUtils::PutIfPresent(values, IMAGE_FACE_COL_BEAUTY_BOUNDER_HEIGHT, imageFaceTbl.beautyBounderHeight);
    BackupDatabaseUtils::PutIfPresent(values, IMAGE_FACE_COL_AESTHETICS_SCORE, imageFaceTbl.aestheticsScore);
    BackupDatabaseUtils::PutIfPresent(values, IMAGE_FACE_COL_BEAUTY_BOUNDER_VERSION, imageFaceTbl.beautyBounderVersion);
    PutWithDefault<int>(values, IMAGE_FACE_COL_IS_EXCLUDED, imageFaceTbl.isExcluded, 0);
    BackupDatabaseUtils::PutIfPresent(values, IMAGE_FACE_COL_FACE_CLARITY, imageFaceTbl.faceClarity);
    BackupDatabaseUtils::PutIfPresent(values, IMAGE_FACE_COL_FACE_LUMINANCE, imageFaceTbl.faceLuminance);
    BackupDatabaseUtils::PutIfPresent(values, IMAGE_FACE_COL_FACE_SATURATION, imageFaceTbl.faceSaturation);
    BackupDatabaseUtils::PutIfPresent(values, IMAGE_FACE_COL_FACE_EYE_CLOSE, imageFaceTbl.faceEyeClose);
    BackupDatabaseUtils::PutIfPresent(values, IMAGE_FACE_COL_FACE_EXPRESSION, imageFaceTbl.faceExpression);
    BackupDatabaseUtils::PutIfPresent(values, IMAGE_FACE_COL_AGE, imageFaceTbl.age);
    BackupDatabaseUtils::PutIfPresent(values, IMAGE_FACE_COL_GENDER, imageFaceTbl.gender);
    BackupDatabaseUtils::PutIfPresent(values, IMAGE_FACE_COL_PREFERRED_GRADE, imageFaceTbl.preferredGrade);
    BackupDatabaseUtils::PutIfPresent(values, IMAGE_FACE_COL_JOINT_BEAUTY_BOUNDER_X, imageFaceTbl.jointBeautyBounderX);
    BackupDatabaseUtils::PutIfPresent(values, IMAGE_FACE_COL_JOINT_BEAUTY_BOUNDER_Y, imageFaceTbl.jointBeautyBounderY);
    BackupDatabaseUtils::PutIfPresent(values, IMAGE_FACE_COL_JOINT_BEAUTY_BOUNDER_WIDTH,
        imageFaceTbl.jointBeautyBounderWidth);
    BackupDatabaseUtils::PutIfPresent(values, IMAGE_FACE_COL_JOINT_BEAUTY_BOUNDER_HEIGHT,
        imageFaceTbl.jointBeautyBounderHeight);
    BackupDatabaseUtils::PutIfPresent(values, IMAGE_FACE_COL_FACE_DETAIL_VERSION, imageFaceTbl.faceDetailVersion);
    return values;
}

void CloneRestorePortrait::UpdateAnalysisTotalTblNoFaceStatus()
{
    MEDIA_INFO_LOG("UpdateAnalysisTotalTblNoFaceStatus");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    auto fileIdPairs = CollectFileIdPairs(photoInfoMap_);
    auto [oldFileIds, newFileIds] = BackupDatabaseUtils::UnzipFileIdPairs(fileIdPairs);

    if (oldFileIds.empty()) {
        MEDIA_ERR_LOG("No old file IDs to process for no face status update.");
        return;
    }

    std::string fileIdOldInClause = "(" + BackupDatabaseUtils::JoinValues<int>(oldFileIds, ", ") + ")";
    std::string queryOldNoFaceSql =
        "SELECT file_id FROM tab_analysis_total "
        "WHERE face = -2 AND file_id IN " + fileIdOldInClause;

    std::vector<int32_t> oldNoFaceFileIds = BackupDatabaseUtils::QueryIntVec(mediaRdb_, queryOldNoFaceSql, "file_id");

    if (oldNoFaceFileIds.empty()) {
        MEDIA_ERR_LOG("No old files found with face = -2 status to migrate.");
        return;
    }

    std::vector<int32_t> newNoFaceFileIds;
    std::map<int32_t, int32_t> oldToNewIdMap;
    for (const auto& pair : fileIdPairs) {
        oldToNewIdMap[pair.first] = pair.second;
    }

    for (int32_t oldId : oldNoFaceFileIds) {
        auto it = oldToNewIdMap.find(oldId);
        if (it != oldToNewIdMap.end()) {
            newNoFaceFileIds.push_back(it->second);
        }
    }

    if (newNoFaceFileIds.empty()) {
        MEDIA_ERR_LOG("No corresponding new file IDs found for old files with face = -2 status.");
        return;
    }

    std::string fileIdNewFilterClause = "(" + BackupDatabaseUtils::JoinValues<int>(newNoFaceFileIds, ", ") + ")";
    std::string updateSql =
        "UPDATE tab_analysis_total "
        "SET face = -2 "
        "WHERE file_id IN " + fileIdNewFilterClause;

    int32_t errCode = BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, updateSql);
    CHECK_AND_PRINT_LOG(errCode >= 0, "execute update analysis total for no face failed, ret=%{public}d", errCode);
    
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("UpdateAnalysisTotalTblNoFaceStatus cost %{public}lld", (long long)(end - start));
}

void CloneRestorePortrait::UpdateAnalysisTotalTblStatus()
{
    MEDIA_INFO_LOG("UpdateAnalysisTotalTblStatus");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    auto fileIdPairs = CollectFileIdPairs(photoInfoMap_);
    auto [oldFileIds, newFileIds] = BackupDatabaseUtils::UnzipFileIdPairs(fileIdPairs);
    std::string fileIdNewFilterClause = "(" + BackupDatabaseUtils::JoinValues<int>(newFileIds, ", ") + ")";

    std::string updateSql =
        "UPDATE tab_analysis_total "
        "SET face = CASE "
            "WHEN EXISTS (SELECT 1 FROM tab_analysis_image_face "
                         "WHERE tab_analysis_image_face.file_id = tab_analysis_total.file_id "
                         "AND tag_id = '-1') THEN 2 "
            "WHEN EXISTS (SELECT 1 FROM tab_analysis_image_face "
                         "WHERE tab_analysis_image_face.file_id = tab_analysis_total.file_id "
                         "AND tag_id = '-2') THEN 4 "
            "ELSE 3 "
        "END "
        "WHERE EXISTS (SELECT 1 FROM tab_analysis_image_face "
                      "WHERE tab_analysis_image_face.file_id = tab_analysis_total.file_id) "
        "AND " + IMAGE_FACE_COL_FILE_ID + " IN " + fileIdNewFilterClause;

    int32_t errCode = BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, updateSql);
    CHECK_AND_PRINT_LOG(errCode >= 0, "execute update analysis total failed, ret=%{public}d", errCode);

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("UpdateAnalysisTotalTblStatus cost %{public}lld", (long long)(end - start));
}

int32_t CloneRestorePortrait::RestoreMaps()
{
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    
    const std::string QUERY_TOTAL_COUNT_SQL = "SELECT count(1) AS count FROM AnalysisPhotoMap AS map "
        " INNER JOIN AnalysisAlbum AS a ON map.map_album = a.album_id "
        " WHERE a.album_subtype = 4102 AND (a.is_removed != 1 OR a.is_removed IS NULL)";
    int32_t totalNumber = BackupDatabaseUtils::QueryInt(mediaRdb_, QUERY_TOTAL_COUNT_SQL, "count");
    MEDIA_INFO_LOG("totalNumber: %{public}d", totalNumber);
    for (int32_t offset = 0; offset < totalNumber; offset += QUERY_COUNT) {
        RestoreMapsBatch();
    }

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("RestoreMaps cost %{public}lld", (long long)(end - start));

    return E_OK;
}

void CloneRestorePortrait::RestoreMapsBatch()
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

void CloneRestorePortrait::UpdateMapInsertValues(std::vector<NativeRdb::ValuesBucket> &values)
{
    const std::string QUERY_SQL = "SELECT map.rowid, map.* FROM AnalysisPhotoMap AS map "
        " INNER JOIN AnalysisAlbum AS a ON map.map_album = a.album_id "
        " WHERE a.album_subtype = 4102 "
        " AND map.rowid > ? ORDER BY map.rowid LIMIT ? ";
    std::vector<NativeRdb::ValueObject> params = { lastIdOfMap_, BATCH_SIZE };
    auto resultSet = BackupDatabaseUtils::QuerySql(mediaRdb_, QUERY_SQL, params);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "query AnalysisPhotoMap err!");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        UpdateMapInsertValuesByAlbumId(values, resultSet);
    }
    resultSet->Close();
}

void CloneRestorePortrait::UpdateMapInsertValuesByAlbumId(std::vector<NativeRdb::ValuesBucket> &values,
    std::shared_ptr<NativeRdb::ResultSet> resultSet)
{
    lastIdOfMap_ = GetInt32Val("rowid", resultSet);

    std::optional<int32_t> oldFileId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "map_asset");
    bool exceptCond = oldFileId.has_value() && photoInfoMap_.count(oldFileId.value()) > 0;
    CHECK_AND_RETURN_LOG(exceptCond, "the query oldFileId is invalid!");
    PhotoInfo photoInfo = photoInfoMap_.at(oldFileId.value());

    std::optional<int32_t> oldAlbumId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "map_album");
    CHECK_AND_RETURN_LOG(oldAlbumId.has_value(), "the query oldAlbumId is invalid!");
    auto it = std::find_if(portraitAlbumInfoMap_.begin(), portraitAlbumInfoMap_.end(),
        [oldAlbumId](const AnalysisAlbumTbl& info) {
            return info.albumIdOld.has_value() && info.albumIdOld.value() == oldAlbumId.value();
        });
    CHECK_AND_RETURN_LOG(it != portraitAlbumInfoMap_.end() && it->albumIdNew.has_value(),
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

NativeRdb::ValuesBucket CloneRestorePortrait::GetMapInsertValue(int32_t albumId, int32_t fileId,
    std::optional<int32_t> &order)
{
    NativeRdb::ValuesBucket value;
    value.PutInt("map_album", albumId);
    value.PutInt("map_asset", fileId);
    CHECK_AND_EXECUTE(!order.has_value(), value.PutInt("order_position", order.value()));
    return value;
}

void CloneRestorePortrait::InsertAnalysisPhotoMap(std::vector<NativeRdb::ValuesBucket> &values)
{
    int64_t rowNum = 0;
    int32_t errCode = BatchInsertWithRetry("AnalysisPhotoMap", values, rowNum);
    migratePortraitAnalysisPhotoMapNumber_ += rowNum;
    if (errCode != E_OK || rowNum != static_cast<int64_t>(values.size())) {
        int64_t failNums = static_cast<int64_t>(values.size()) - rowNum;
        ErrorInfo errorInfo(RestoreError::INSERT_FAILED, 0, std::to_string(errCode),
            "insert into AnalysisPhotoMap fail, num:" + std::to_string(failNums));
        UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).ReportError(errorInfo);
        mapFailedCnt_ += failNums;
    }
    mapSuccessCnt_ += rowNum;
}

void CloneRestorePortrait::ReportPortraitCloneStat(int32_t sceneCode)
{
    CHECK_AND_RETURN_LOG(sceneCode == CLONE_RESTORE_ID, "err scenecode %{public}d", sceneCode);

    MEDIA_INFO_LOG("PortraitStat: analysisAlbum %{public}lld, faceTag %{public}lld"
        ", imageFace %{public}lld, photoMap %{public}lld, cost %{public}lld",
        (long long)migratePortraitAlbumNumber_, (long long)migratePortraitFaceTagNumber_,
        (long long)migratePortraitFaceNumber_, (long long)migratePortraitAnalysisPhotoMapNumber_,
        (long long)migratePortraitTotalTimeCost_);

    BackupDfxUtils::PostPortraitStat(static_cast<uint32_t>(migratePortraitAlbumNumber_), migratePortraitPhotoNumber_,
        migratePortraitFaceNumber_, migratePortraitTotalTimeCost_);
}
}