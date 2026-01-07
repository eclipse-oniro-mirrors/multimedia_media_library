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

#define MLOG_TAG "MediaLibraryCloneRestorePet"

#include "media_log.h"
#include "clone_restore_pet_album.h"
#include "media_smart_album_column.h"
#include "backup_database_utils.h"
#include "backup_dfx_utils.h"
#include "media_file_utils.h"
#include "backup_const_column.h"
#include "upgrade_restore_task_report.h"

using namespace std;
namespace OHOS::Media {
const int32_t PAGE_SIZE = 200;

void CloneRestorePet::Init(int32_t sceneCode, const std::string &taskId,
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
    std::shared_ptr<NativeRdb::RdbStore> mediaRdb,
    const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap, bool isCloudRestoreSatisfied)
{
    MEDIA_INFO_LOG("CloneRestorePet Init");
    this->sceneCode_ = sceneCode;
    this->taskId_ = taskId;
    this->mediaRdb_ = mediaRdb;
    this->mediaLibraryRdb_ = mediaLibraryRdb;
    this->photoInfoMap_ = photoInfoMap;
    this->isCloudRestoreSatisfied_ = isCloudRestoreSatisfied;
}

void CloneRestorePet::Preprocess()
{
    MEDIA_INFO_LOG("Preprocess");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    std::string querySql = "SELECT count(1) AS count FROM " + ANALYSIS_ALBUM_TABLE + " WHERE ";
    std::string whereClause = "(" + SMARTALBUM_DB_ALBUM_TYPE + " = " + std::to_string(SMART) + " AND " +
        "album_subtype" + " = " + std::to_string(PET) + ")";
    AppendExtraWhereClause(whereClause);
    querySql += whereClause;
    totalPetAlbumNumber_ = BackupDatabaseUtils::QueryInt(mediaRdb_, querySql, CUSTOM_COUNT);
    MEDIA_INFO_LOG("QueryPetAlbum totalNumber = %{public}d", totalPetAlbumNumber_);
    CHECK_AND_EXECUTE(!(totalPetAlbumNumber_ > 0), DeleteExistingPetInfos());

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    migratePetTotalTimeCost_ += end - start;
}

void CloneRestorePet::DeleteExistingPetInfos()
{
    MEDIA_INFO_LOG("DeleteExistingPetInfos");
    DeleteExistingPetAlbums();
    DeleteExistingPetInfo();
}

void CloneRestorePet::DeleteExistingPetAlbums()
{
    MEDIA_INFO_LOG("DeleteExistingPetAlbums");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    std::string deletePetAlbumsAql =
        "DELETE FROM AnalysisAlbum WHERE "
        "album_type = " + std::to_string(SMART) + " AND album_subtype = " + std::to_string(PET);
    BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, deletePetAlbumsAql);
    
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("DeleteExistingPetAlbums cost %{public}lld", (long long)(end - start));
}

void CloneRestorePet::DeleteExistingPetInfo()
{
    MEDIA_INFO_LOG("DeleteExistingPetInfo");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    std::string fileIdFilterClause = std::string("(") + "SELECT " + PET_FACE_COL_FILE_ID + " FROM " +
        VISION_PET_FACE_TABLE + ")";
    std::string fileIdCondition = PET_FACE_COL_FILE_ID + " IN " + fileIdFilterClause;

    MEDIA_INFO_LOG("Update TableAnalysisTotal");
    std::unique_ptr<NativeRdb::AbsRdbPredicates> totalTablePredicates =
        std::make_unique<NativeRdb::AbsRdbPredicates>(VISION_TOTAL_TABLE);
    std::string statusCondition = " status = 1";
    totalTablePredicates->SetWhereClause(fileIdCondition + " AND" + statusCondition);
    NativeRdb::ValuesBucket totalValues;
    totalValues.PutInt("pet", 0);
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
        "map_album IN (SELECT album_id FROM AnalysisAlbum WHERE album_type = " + std::to_string(SMART) +
        " AND album_subtype = " + std::to_string(PET) + ") "
        "AND map_asset IN" + fileIdFilterClause;
    BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, deleteAnalysisPhotoMapSql);

    MEDIA_INFO_LOG("Delete PetFaceTable");
    std::string deleteTagFaceSql = "DELETE FROM " + VISION_PET_FACE_TABLE;
    BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, deleteTagFaceSql);

    MEDIA_INFO_LOG("Delete PetTagTable");
    std::string deletePetTagSql = "DELETE FROM " + VISION_PET_TAG_TABLE;
    BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, deletePetTagSql);

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("DeleteExistingPetInfo cost %{public}lld", (long long)(end - start));
}

void CloneRestorePet::Restore()
{
    MEDIA_INFO_LOG("Start Restore");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    CHECK_AND_RETURN_LOG(mediaRdb_ != nullptr && mediaLibraryRdb_ != nullptr, "rdbStore is nullptr.");
    RestoreFromGalleryPetAlbum();
    RestorePetClusteringInfo();
    RestorePetFaceInfo();
    BackupDatabaseUtils::UpdateFaceAnalysisTblStatus(mediaLibraryRdb_);
    int32_t ret = RestoreMaps();
    CHECK_AND_RETURN_LOG(ret == E_OK, "fail to update analysis photo map status");
    RestoreAnalysisTotalFaceStatus();
    ReportPetCloneStat(sceneCode_);
    
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    migratePetTotalTimeCost_ += end - start;
}

void CloneRestorePet::RestoreAnalysisTotalFaceStatus()
{
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    CloneRestoreAnalysisTotal cloneRestoreAnalysisTotal;
    cloneRestoreAnalysisTotal.Init("pet", PAGE_SIZE, mediaRdb_, mediaLibraryRdb_);
    int32_t totalNumber = cloneRestoreAnalysisTotal.GetTotalNumber();
    for (int32_t offset = 0; offset < totalNumber; offset += PAGE_SIZE) {
        cloneRestoreAnalysisTotal.GetInfos(photoInfoMap_);
        cloneRestoreAnalysisTotal.UpdateDatabase();
    }
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("RestoreAnalysisTotalFaceStatus cost %{public}lld", (long long)(end - start));
}

void CloneRestorePet::RestoreFromGalleryPetAlbum()
{
    MEDIA_INFO_LOG("RestoreFromGalleryPetAlbum");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    RecordOldPetAlbumDfx();
    GetMaxAlbumId();
    std::vector<std::string> commonColumn = BackupDatabaseUtils::GetCommonColumnInfos(mediaRdb_, mediaLibraryRdb_,
        ANALYSIS_ALBUM_TABLE);
    std::vector<std::string> commonColumns = BackupDatabaseUtils::filterColumns(commonColumn,
        EXCLUDED_PET_TAG_COLUMNS);

    for (int32_t offset = 0; offset < totalPetAlbumNumber_; offset += QUERY_COUNT) {
        std::vector<AnalysisAlbumTbl> PetAlbumTbl = QueryPetAlbumTbl(offset, commonColumns);
        for (const auto& album : PetAlbumTbl) {
            if (album.tagId.has_value() && album.coverUri.has_value()) {
                coverUriInfo_.emplace_back(album.tagId.value(),
                    std::make_pair(album.coverUri.value(),
                    album.isCoverSatisfied.value_or(PET_INVALID_COVER_SATISFIED_STATUS)));
            }
        }
        InsertPetAlbum(PetAlbumTbl);
    }

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("RestoreFromGalleryPetAlbum cost %{public}lld", (long long)(end - start));

    LogPetCloneDfx();
}

void CloneRestorePet::RecordOldPetAlbumDfx()
{
    int32_t offset {0};
    int32_t rowCount {0};
    std::vector<PetAlbumDfx> albums;

    do {
        auto batchResults =  QueryAllPetAlbum(offset, rowCount);
        if (!batchResults.empty()) {
            albums.insert(albums.end(), batchResults.begin(), batchResults.end());
        }

        offset += QUERY_COUNT;
    } while (rowCount > 0);

    for (const auto& album : albums) {
        PetAlbumDfx dfxInfo;
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

        PetAlbumDfx_.push_back(dfxInfo);
    }
}

void CloneRestorePet::LogPetCloneDfx()
{
    std::vector<std::string> failedAlbums;
    std::unordered_set<std::string> existingTagIds = QueryAllPetAlbum();

    for (const auto& oldAlbum : PetAlbumDfx_) {
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
        MEDIA_ERR_LOG("Following Pet albums failed to clone completely:");
        for (const auto& failedAlbum : failedAlbums) {
            MEDIA_ERR_LOG("%{public}s", failedAlbum.c_str());
        }
    } else {
        MEDIA_INFO_LOG("All Pet albums cloned successfully");
    }

    MEDIA_INFO_LOG("Stat: Total albums: %{public}zu, Failed albums count: %{public}zu",
        PetAlbumDfx_.size(), failedAlbums.size());
}

std::vector<PetAlbumDfx> CloneRestorePet::QueryAllPetAlbum(int32_t& offset, int32_t& rowCount)
{
    std::vector<PetAlbumDfx> result;
    result.reserve(QUERY_COUNT);

    const std::string querySql = "SELECT album_name, cover_uri, tag_id, count "
        "FROM AnalysisAlbum "
        "WHERE album_type = ? "
        "AND album_subtype = ? "
        "LIMIT ?, ?";

    std::vector<NativeRdb::ValueObject> bindArgs = { SMART, PET, offset, QUERY_COUNT };
    CHECK_AND_RETURN_RET_LOG(this->mediaRdb_ != nullptr, result, "Media_Restore: mediaRdb_ is null.");
    auto resultSet = mediaRdb_->QuerySql(querySql, bindArgs);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, result, "Query resultSql is null.");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        PetAlbumDfx dfxInfo;
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

std::unordered_set<std::string> CloneRestorePet::QueryAllPetAlbum()
{
    std::unordered_set<std::string> result;
    std::vector<std::string> tagIds;
    for (const auto& oldAlbum : PetAlbumDfx_) {
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

vector<AnalysisAlbumTbl> CloneRestorePet::QueryPetAlbumTbl(int32_t offset,
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
        "album_subtype" + " = " + std::to_string(PET) + ")";
    AppendExtraWhereClause(whereClause);
    querySql += whereClause;
    querySql += " LIMIT " + std::to_string(offset) + ", " + std::to_string(QUERY_COUNT);

    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaRdb_, querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, result, "Query resultSql is null.");
    GetMaxAlbumId();
    int32_t albumIdNow = maxAnalysisAlbumId_ + 1;

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        AnalysisAlbumTbl PetAlbumTbl;
        ParseAlbumResultSet(resultSet, PetAlbumTbl);
        PetAlbumTbl.albumIdNew = std::make_optional<int32_t>(albumIdNow++);
        result.emplace_back(PetAlbumTbl);
    }
    resultSet->Close();
    return result;
}

void CloneRestorePet::InsertPetAlbum(std::vector<AnalysisAlbumTbl> &PetAlbumTbl)
{
    CHECK_AND_RETURN_LOG(mediaLibraryRdb_ != nullptr, "mediaLibraryRdb_ is null");
    CHECK_AND_RETURN_LOG(!PetAlbumTbl.empty(), "PetAlbumTbl are empty");

    std::vector<std::string> albumNames;
    std::vector<std::string> tagIds;

    for (const auto &album : PetAlbumTbl) {
        CHECK_AND_EXECUTE(!album.albumName.has_value(), albumNames.emplace_back(album.albumName.value()));
        CHECK_AND_EXECUTE(!album.tagId.has_value(), tagIds.emplace_back(album.tagId.value()));
    }
    MEDIA_INFO_LOG("InsertPetAlbum, total albums: %{public}zu, Albums with names: %{public}zu,"
        "Albums with tagIds: %{public}zu", PetAlbumTbl.size(), albumNames.size(), tagIds.size());

    int32_t albumRowNum = InsertPetAlbumByTable(PetAlbumTbl);
    CHECK_AND_PRINT_LOG(albumRowNum != E_ERR, "Failed to insert album");

    migratePetAlbumNumber_ += static_cast<uint64_t>(albumRowNum);
    return;
}

int32_t CloneRestorePet::InsertPetAlbumByTable(std::vector<AnalysisAlbumTbl> &PetAlbumTbl)
{
    std::vector<NativeRdb::ValuesBucket> valuesBuckets = GetInsertValues(PetAlbumTbl);
    int64_t rowNum = 0;
    int32_t ret = BatchInsertWithRetry(ANALYSIS_ALBUM_TABLE, valuesBuckets, rowNum);
    CHECK_AND_RETURN_RET(ret == E_OK, E_ERR);
    PetAlbumInfoMap_.insert(PetAlbumInfoMap_.end(), PetAlbumTbl.begin(), PetAlbumTbl.end());
    return rowNum;
}

vector<NativeRdb::ValuesBucket> CloneRestorePet::GetInsertValues(std::vector<AnalysisAlbumTbl> &PetAlbumTbl)
{
    std::vector<NativeRdb::ValuesBucket> values;
    for (auto &PetAlbumInfo : PetAlbumTbl) {
        NativeRdb::ValuesBucket value;
        GetAnalysisAlbumInsertValue(value, PetAlbumInfo);
        values.emplace_back(value);
    }
    return values;
}

void CloneRestorePet::RestorePetClusteringInfo()
{
    MEDIA_INFO_LOG("RestorePetClusteringInfo");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    int32_t totalNumber = BackupDatabaseUtils::QueryInt(mediaRdb_, QUERY_PET_TAG_COUNT, CUSTOM_COUNT);
    MEDIA_INFO_LOG("QueryPetClustering totalNumber = %{public}d", totalNumber);

    std::vector<std::string> commonColumn = BackupDatabaseUtils::GetCommonColumnInfos(mediaRdb_, mediaLibraryRdb_,
        VISION_PET_TAG_TABLE);
    std::vector<std::string> commonColumns = BackupDatabaseUtils::filterColumns(commonColumn,
        EXCLUDED_PET_TAG_COLUMNS);
    BackupDatabaseUtils::LeftJoinValues<string>(commonColumns, "vft.");
    std::string inClause = BackupDatabaseUtils::JoinValues<string>(commonColumns, ", ");
    BackupDatabaseUtils::ExecuteSQL(mediaRdb_, CREATE_FACE_TAG_INDEX);
    for (int32_t offset = 0; offset < totalNumber; offset += QUERY_COUNT) {
        vector<PetTagTbl> petTagTbls = QueryPetTagTbl(offset, inClause);
        BatchInsertPetTags(petTagTbls);
        if (static_cast<std::int32_t>(petTagTbls.size()) < QUERY_COUNT) {
            break;
        }
    }

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("RestorePetClusteringInfo cost %{public}lld", (long long)(end - start));
}

std::vector<PetTagTbl> CloneRestorePet::QueryPetTagTbl(int32_t offset, const std::string &inClause)
{
    std::vector<PetTagTbl> result;

    std::string querySql = "SELECT DISTINCT " + inClause +
        " FROM " + VISION_PET_TAG_TABLE + " vft" +
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
        PetTagTbl petTagTbl;
        ParsePetTagResultSet(resultSet, petTagTbl);
        result.emplace_back(petTagTbl);
    }

    resultSet->Close();
    return result;
}

void CloneRestorePet::ParsePetTagResultSet(const std::shared_ptr<NativeRdb::ResultSet>& resultSet,
    PetTagTbl& petTagTbl)
{
    petTagTbl.tagId = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, PET_TAG_COL_TAG_ID);
    petTagTbl.petLabel = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, PET_TAG_COL_PET_LABEL);
    petTagTbl.centerFeatures = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        PET_TAG_COL_CENTER_FEATURES);
    petTagTbl.tagVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, PET_TAG_COL_TAG_VERSION);
    petTagTbl.count = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, PET_TAG_COL_COUNT);
    petTagTbl.dateModified = BackupDatabaseUtils::GetOptionalValue<int64_t>(resultSet, PET_TAG_COL_DATE_MODIFIED);
    petTagTbl.analysisVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        PET_TAG_COL_ANALYSIS_VERSION);
}

void CloneRestorePet::BatchInsertPetTags(const std::vector<PetTagTbl>& petTagTbls)
{
    std::vector<NativeRdb::ValuesBucket> valuesBuckets;
    for (const auto& petTagTbl : petTagTbls) {
        valuesBuckets.push_back(CreateValuesBucketFromPetTagTbl(petTagTbl));
    }

    int64_t rowNum = 0;
    int32_t ret = BatchInsertWithRetry(VISION_PET_TAG_TABLE, valuesBuckets, rowNum);
    migratePetFaceTagNumber_ += rowNum;
    CHECK_AND_RETURN_LOG(ret == E_OK, "Failed to batch insert pet tags");
}

NativeRdb::ValuesBucket CloneRestorePet::CreateValuesBucketFromPetTagTbl(const PetTagTbl& petTagTbl)
{
    NativeRdb::ValuesBucket values;

    BackupDatabaseUtils::PutIfPresent(values, PET_TAG_COL_TAG_ID, petTagTbl.tagId);
    BackupDatabaseUtils::PutIfPresent(values, PET_TAG_COL_PET_LABEL, petTagTbl.petLabel);
    BackupDatabaseUtils::PutIfPresent(values, PET_TAG_COL_CENTER_FEATURES, petTagTbl.centerFeatures);
    BackupDatabaseUtils::PutIfPresent(values, PET_TAG_COL_TAG_VERSION, petTagTbl.tagVersion);
    BackupDatabaseUtils::PutIfPresent(values, PET_TAG_COL_COUNT, petTagTbl.count);
    BackupDatabaseUtils::PutIfPresent(values, PET_TAG_COL_DATE_MODIFIED, petTagTbl.dateModified);
    BackupDatabaseUtils::PutIfPresent(values, PET_TAG_COL_ANALYSIS_VERSION, petTagTbl.analysisVersion);

    return values;
}

template<typename T, typename U>
void CloneRestorePet::PutWithDefault(NativeRdb::ValuesBucket& values, const std::string& columnName,
    const std::optional<T>& optionalValue, const U& defaultValue)
{
    if (optionalValue.has_value()) {
        BackupDatabaseUtils::PutIfPresent(values, columnName, optionalValue);
    } else {
        BackupDatabaseUtils::PutIfPresent(values, columnName, std::optional<T>(static_cast<T>(defaultValue)));
    }
}

void CloneRestorePet::RestorePetFaceInfo()
{
    MEDIA_INFO_LOG("RestorePetFaceInfo");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    auto uniqueFileIdPairs = CollectFileIdPairs(photoInfoMap_);
    auto [oldFileIds, newFileIds] = BackupDatabaseUtils::UnzipFileIdPairs(uniqueFileIdPairs);

    std::string fileIdOldInClause = "(" + BackupDatabaseUtils::JoinValues<int>(oldFileIds, ", ") + ")";

    std::string querySql = QUERY_PET_FACE_COUNT;
    querySql += " WHERE " + PET_FACE_COL_FILE_ID + " IN " + fileIdOldInClause;
    int32_t totalNumber = BackupDatabaseUtils::QueryInt(mediaRdb_, querySql, CUSTOM_COUNT);
    MEDIA_INFO_LOG("QueryPetFaceTotalNumber, totalNumber = %{public}d", totalNumber);
    if (totalNumber == 0) {
        return;
    }

    std::vector<std::string> commonColumn = BackupDatabaseUtils::GetCommonColumnInfos(mediaRdb_, mediaLibraryRdb_,
        VISION_PET_FACE_TABLE);
    std::vector<std::string> commonColumns = BackupDatabaseUtils::filterColumns(commonColumn,
        EXCLUDED_PET_FACE_COLUMNS);

    BackupDatabaseUtils::DeleteExistingPetFaceData(mediaLibraryRdb_, uniqueFileIdPairs);
    for (int32_t offset = 0; offset < totalNumber; offset += QUERY_COUNT) {
        std::vector<PetFaceTbl> petFaceTbls = QueryPetFaceTbl(offset, fileIdOldInClause, commonColumns);
        auto petFaces = ProcessPetFaceTbls(petFaceTbls, uniqueFileIdPairs);
        BatchInsertPetFaces(petFaces);
    }

    GenNewCoverUris(coverUriInfo_);

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("RestorePetFaceInfo cost %{public}lld", (long long)(end - start));
}

std::vector<PetFaceTbl> CloneRestorePet::ProcessPetFaceTbls(const std::vector<PetFaceTbl>& petFaceTbls,
    const std::vector<FileIdPair>& fileIdPairs)
{
    CHECK_AND_RETURN_RET_LOG(!petFaceTbls.empty(), {}, "pet faces tbl empty");

    std::vector<PetFaceTbl> petFaceNewTbls;
    petFaceNewTbls.reserve(petFaceNewTbls.size());

    for (const auto& petFaceTbl : petFaceTbls) {
        if (petFaceTbl.fileId.has_value()) {
            int32_t oldFileId = petFaceTbl.fileId.value();
            auto it = std::find_if(fileIdPairs.begin(), fileIdPairs.end(),
                [oldFileId](const FileIdPair& pair) { return pair.first == oldFileId; });
            if (it != fileIdPairs.end()) {
                PetFaceTbl updatedFace = petFaceTbl;
                updatedFace.fileId = it->second;
                petFaceNewTbls.push_back(std::move(updatedFace));
            }
        }
    }

    return petFaceNewTbls;
}

std::vector<PetFaceTbl> CloneRestorePet::QueryPetFaceTbl(int32_t offset, std::string &fileIdClause,
    const std::vector<std::string> &commonColumns)
{
    std::vector<PetFaceTbl> result;
    result.reserve(QUERY_COUNT);

    std::string inClause = BackupDatabaseUtils::JoinValues<string>(commonColumns, ", ");
    std::string querySql =
        "SELECT " + inClause +
        " FROM " + VISION_PET_FACE_TABLE;
    querySql += " WHERE " + PET_FACE_COL_FILE_ID + " IN " + fileIdClause;
    querySql += " LIMIT " + std::to_string(offset) + ", " + std::to_string(QUERY_COUNT);

    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaRdb_, querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, result, "Query resultSet is null.");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        PetFaceTbl petFaceTbl;
        ParsePetFaceResultSet(resultSet, petFaceTbl);
        result.emplace_back(petFaceTbl);
    }

    resultSet->Close();
    return result;
}

void  CloneRestorePet::ParsePetFaceResultSet(const std::shared_ptr<NativeRdb::ResultSet>& resultSet,
    PetFaceTbl& petFaceTbl)
{
    petFaceTbl.fileId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet,
        PET_FACE_COL_FILE_ID);
    petFaceTbl.petId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet,
        PET_FACE_COL_PET_ID);
    petFaceTbl.prob = BackupDatabaseUtils::GetOptionalValue<double>(resultSet,
        PET_FACE_COL_PROB);
    petFaceTbl.petLabel = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet,
        PET_FACE_COL_PET_LABEL);
    petFaceTbl.petTotalFaces = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet,
        PET_FACE_COL_PET_TOTAL_FACES);
    petFaceTbl.features = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        PET_FACE_COL_FEATURES);
    petFaceTbl.petTagId = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        PET_FACE_COL_PET_TAG_ID);
    petFaceTbl.scaleX = BackupDatabaseUtils::GetOptionalValue<double>(resultSet,
        PET_FACE_COL_SCALE_X);
    petFaceTbl.scaleY = BackupDatabaseUtils::GetOptionalValue<double>(resultSet,
        PET_FACE_COL_SCALE_Y);
    petFaceTbl.scaleWidth = BackupDatabaseUtils::GetOptionalValue<double>(resultSet,
        PET_FACE_COL_SCALE_WIDTH);
    petFaceTbl.scaleHeight = BackupDatabaseUtils::GetOptionalValue<double>(resultSet,
        PET_FACE_COL_SCALE_HEIGHT);
    petFaceTbl.headVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        PET_FACE_COL_HEAD_VERSION);;
    petFaceTbl.petFeaturesVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        PET_FACE_COL_PET_FEATURE_VERSION);
    petFaceTbl.tagVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        PET_FACE_COL_TAG_VERSION);
    petFaceTbl.analysisVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        PET_FACE_COL_ANALYSIS_VERSION);
    petFaceTbl.jointBeautyBounderX = BackupDatabaseUtils::GetOptionalValue<double>(resultSet,
        PET_FACE_COL_BEAUTY_BOUNDER_X);
    petFaceTbl.jointBeautyBounderY = BackupDatabaseUtils::GetOptionalValue<double>(resultSet,
        PET_FACE_COL_BEAUTY_BOUNDER_Y);
    petFaceTbl.jointBeautyBounderWidth = BackupDatabaseUtils::GetOptionalValue<double>(resultSet,
        PET_FACE_COL_BEAUTY_BOUNDER_WIDTH);
    petFaceTbl.jointBeautyBounderHeight = BackupDatabaseUtils::GetOptionalValue<double>(resultSet,
        PET_FACE_COL_BEAUTY_BOUNDER_HEIGHT);
    petFaceTbl.dateModified = BackupDatabaseUtils::GetOptionalValue<int64_t>(resultSet,
        PET_FACE_COL_DATE_MODIFIED);
}

void CloneRestorePet::BatchInsertPetFaces(const std::vector<PetFaceTbl>& petFaceTbls)
{
    std::vector<NativeRdb::ValuesBucket> valuesBuckets;
    std::unordered_set<int32_t> fileIdSet;
    for (const auto& petFaceTbl : petFaceTbls) {
        valuesBuckets.push_back(CreateValuesBucketFromPetFaceTbl(petFaceTbl));
    }

    int64_t rowNum = 0;
    int32_t ret = BatchInsertWithRetry(VISION_PET_FACE_TABLE, valuesBuckets, rowNum);
    CHECK_AND_RETURN_LOG(ret == E_OK, "Failed to batch insert pet faces");

    for (const auto& petFaceTbl : petFaceTbls) {
        if (petFaceTbl.fileId.has_value()) {
            fileIdSet.insert(petFaceTbl.fileId.value());
        }
    }

    migratePetFaceNumber_ += rowNum;
    migratePetPhotoNumber_ += fileIdSet.size();
}

NativeRdb::ValuesBucket CloneRestorePet::CreateValuesBucketFromPetFaceTbl(const PetFaceTbl& petFaceTbl)
{
    NativeRdb::ValuesBucket values;

    BackupDatabaseUtils::PutIfPresent(values, PET_FACE_COL_FILE_ID, petFaceTbl.fileId);
    BackupDatabaseUtils::PutIfPresent(values, PET_FACE_COL_PET_ID, petFaceTbl.petId);
    BackupDatabaseUtils::PutIfPresent(values, PET_FACE_COL_PROB, petFaceTbl.prob);
    BackupDatabaseUtils::PutIfPresent(values, PET_FACE_COL_PET_LABEL, petFaceTbl.petLabel);
    BackupDatabaseUtils::PutIfPresent(values, PET_FACE_COL_PET_TOTAL_FACES, petFaceTbl.petTotalFaces);
    BackupDatabaseUtils::PutIfPresent(values, PET_FACE_COL_FEATURES, petFaceTbl.features);
    BackupDatabaseUtils::PutIfPresent(values, PET_FACE_COL_PET_TAG_ID, petFaceTbl.petTagId);
    BackupDatabaseUtils::PutIfPresent(values, PET_FACE_COL_SCALE_X, petFaceTbl.scaleX);
    BackupDatabaseUtils::PutIfPresent(values, PET_FACE_COL_SCALE_Y, petFaceTbl.scaleY);
    BackupDatabaseUtils::PutIfPresent(values, PET_FACE_COL_SCALE_WIDTH, petFaceTbl.scaleWidth);
    BackupDatabaseUtils::PutIfPresent(values, PET_FACE_COL_SCALE_HEIGHT, petFaceTbl.scaleHeight);
    BackupDatabaseUtils::PutIfPresent(values, PET_FACE_COL_HEAD_VERSION, petFaceTbl.headVersion);
    BackupDatabaseUtils::PutIfPresent(values, PET_FACE_COL_PET_FEATURE_VERSION, petFaceTbl.petFeaturesVersion);
    BackupDatabaseUtils::PutIfPresent(values, PET_FACE_COL_TAG_VERSION, petFaceTbl.tagVersion);
    BackupDatabaseUtils::PutIfPresent(values, PET_FACE_COL_ANALYSIS_VERSION, petFaceTbl.analysisVersion);
    BackupDatabaseUtils::PutIfPresent(values, PET_FACE_COL_BEAUTY_BOUNDER_X, petFaceTbl.jointBeautyBounderX);
    BackupDatabaseUtils::PutIfPresent(values, PET_FACE_COL_BEAUTY_BOUNDER_Y, petFaceTbl.jointBeautyBounderY);
    BackupDatabaseUtils::PutIfPresent(values, PET_FACE_COL_BEAUTY_BOUNDER_WIDTH, petFaceTbl.jointBeautyBounderWidth);
    BackupDatabaseUtils::PutIfPresent(values, PET_FACE_COL_BEAUTY_BOUNDER_HEIGHT, petFaceTbl.jointBeautyBounderHeight);
    BackupDatabaseUtils::PutIfPresent(values, PET_FACE_COL_DATE_MODIFIED, petFaceTbl.dateModified);
    return values;
}

int32_t CloneRestorePet::RestoreMaps()
{
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    
    const std::string QUERY_TOTAL_COUNT_SQL = "SELECT count(1) AS count FROM AnalysisPhotoMap AS map "
        " INNER JOIN AnalysisAlbum AS a ON map.map_album = a.album_id "
        " WHERE a.album_subtype = 4106 AND (a.is_removed != 1 OR a.is_removed IS NULL)";
    int32_t totalNumber = BackupDatabaseUtils::QueryInt(mediaRdb_, QUERY_TOTAL_COUNT_SQL, "count");
    MEDIA_INFO_LOG("totalNumber: %{public}d", totalNumber);
    for (int32_t offset = 0; offset < totalNumber; offset += QUERY_COUNT) {
        RestoreMapsBatch();
    }

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("RestoreMaps cost %{public}lld", (long long)(end - start));

    return E_OK;
}

void CloneRestorePet::RestoreMapsBatch()
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

void CloneRestorePet::UpdateMapInsertValues(std::vector<NativeRdb::ValuesBucket> &values)
{
    const std::string QUERY_SQL = "SELECT map.rowid, map.* FROM AnalysisPhotoMap AS map "
        " INNER JOIN AnalysisAlbum AS a ON map.map_album = a.album_id "
        " WHERE a.album_subtype = 4106 "
        " AND map.rowid > ? ORDER BY map.rowid LIMIT ? ";
    std::vector<NativeRdb::ValueObject> params = { lastIdOfMap_, PET_BATCH_SIZE };
    auto resultSet = BackupDatabaseUtils::QuerySql(mediaRdb_, QUERY_SQL, params);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "query AnalysisPhotoMap err!");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        UpdateMapInsertValuesByAlbumId(values, resultSet);
    }
    resultSet->Close();
}

void CloneRestorePet::UpdateMapInsertValuesByAlbumId(std::vector<NativeRdb::ValuesBucket> &values,
    std::shared_ptr<NativeRdb::ResultSet> resultSet)
{
    lastIdOfMap_ = GetInt32Val("rowid", resultSet);

    std::optional<int32_t> oldFileId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "map_asset");
    bool exceptCond = oldFileId.has_value() && photoInfoMap_.count(oldFileId.value()) > 0;
    CHECK_AND_RETURN_LOG(exceptCond, "the query oldFileId is invalid!");
    PhotoInfo photoInfo = photoInfoMap_.at(oldFileId.value());

    std::optional<int32_t> oldAlbumId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "map_album");
    CHECK_AND_RETURN_LOG(oldAlbumId.has_value(), "the query oldAlbumId is invalid!");
    auto it = std::find_if(PetAlbumInfoMap_.begin(), PetAlbumInfoMap_.end(),
        [oldAlbumId](const AnalysisAlbumTbl& info) {
            return info.albumIdOld.has_value() && info.albumIdOld.value() == oldAlbumId.value();
        });
    CHECK_AND_RETURN_LOG(it != PetAlbumInfoMap_.end() && it->albumIdNew.has_value(),
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

NativeRdb::ValuesBucket CloneRestorePet::GetMapInsertValue(int32_t albumId, int32_t fileId,
    std::optional<int32_t> &order)
{
    NativeRdb::ValuesBucket value;
    value.PutInt("map_album", albumId);
    value.PutInt("map_asset", fileId);
    CHECK_AND_EXECUTE(!order.has_value(), value.PutInt("order_position", order.value()));
    return value;
}

void CloneRestorePet::InsertAnalysisPhotoMap(std::vector<NativeRdb::ValuesBucket> &values)
{
    int64_t rowNum = 0;
    int32_t errCode = BatchInsertWithRetry("AnalysisPhotoMap", values, rowNum);
    migratePetAnalysisPhotoMapNumber_ += rowNum;
    if (errCode != E_OK || rowNum != static_cast<int64_t>(values.size())) {
        int64_t failNums = static_cast<int64_t>(values.size()) - rowNum;
        ErrorInfo errorInfo(RestoreError::INSERT_FAILED, 0, std::to_string(errCode),
            "insert into AnalysisPhotoMap fail, num:" + std::to_string(failNums));
        UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).ReportError(errorInfo);
        mapFailedCnt_ += failNums;
    }
    mapSuccessCnt_ += rowNum;
}

void CloneRestorePet::ReportPetCloneStat(int32_t sceneCode)
{
    CHECK_AND_RETURN_LOG(sceneCode == CLONE_RESTORE_ID, "err scenecode %{public}d", sceneCode);

    MEDIA_INFO_LOG("PetStat: analysisAlbum %{public}lld, petTag %{public}lld"
        ", imageFace %{public}lld, photoMap %{public}lld, cost %{public}lld",
        (long long)migratePetAlbumNumber_, (long long)migratePetFaceTagNumber_,
        (long long)migratePetFaceNumber_, (long long)migratePetAnalysisPhotoMapNumber_,
        (long long)migratePetTotalTimeCost_);

    BackupDfxUtils::PostPetStat(static_cast<uint32_t>(migratePetAlbumNumber_), migratePetPhotoNumber_,
        migratePetFaceNumber_, migratePetTotalTimeCost_);
}
}