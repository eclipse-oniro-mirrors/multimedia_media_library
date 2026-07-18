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
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

#define MLOG_TAG "CloneReverseRestoreGeo"

#include "clone_reverse_restore_geo.h"

#include "backup_database_utils.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "result_set_utils.h"
#include "upgrade_restore_task_report.h"
#include "locale_config.h"
#include "location_column.h"
#include "vision_column_comm.h"

namespace OHOS::Media {

const std::string GEO = "geo";
const std::string MAP_ALBUM = "map_album";
const std::string MAP_ASSET = "map_asset";
const std::string INTEGER = "INTEGER";
const std::string TEXT = "TEXT";

// LCOV_EXCL_START
void CloneReverseRestoreGeo::Init(int32_t sceneCode,
    const std::string &taskId,
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
    std::shared_ptr<NativeRdb::RdbStore> mediaRdb)
{
    sceneCode_ = sceneCode;
    taskId_ = taskId;
    mediaLibraryRdb_ = mediaLibraryRdb;
    mediaRdb_ = mediaRdb;
    systemLanguage_ = Global::I18n::LocaleConfig::GetSystemLanguage();
}

void CloneReverseRestoreGeo::Restore()
{
    CHECK_AND_RETURN_LOG(mediaRdb_ != nullptr && mediaLibraryRdb_ != nullptr,
        "rdbStore is nullptr");

    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    maxAlbumIdOld_ = GetMaxAlbumIdOld();

    RestoreReverseGeoKnowledge();
    RestoreReverseAlbums();
    RestoreReverseMaps();
    UpdateReverseGeoStatus();

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    restoreTimeCost_ = end - start;
    ReportReverseRestoreTask();
    MEDIA_INFO_LOG("TimeCost: CloneReverseRestoreGeo: %{public}" PRId64, end - start);
}

int32_t CloneReverseRestoreGeo::GetMaxAlbumIdOld()
{
    return BackupDatabaseUtils::QueryMaxId(mediaRdb_, ANALYSIS_ALBUM_TABLE, ALBUM_ID);
}

void CloneReverseRestoreGeo::ReportReverseRestoreTask()
{
    RestoreTaskInfo info;
    info.type = "CLONE_REVERSE_RESTORE_GEO";
    info.errorCode = std::to_string(GEO_STATUS_SUCCESS);
    info.errorInfo = "timeCost: " + std::to_string(restoreTimeCost_) +
        ", geoKnowledge: " + std::to_string(successGeoKnowledgeCnt_.load()) +
        ", geoDictionary: " + std::to_string(successGeoDictionaryCnt_.load()) +
        ", albums: " + std::to_string(successAlbumCnt_.load()) +
        ", maps: " + std::to_string(successMapCnt_.load());
    info.successCount = successGeoKnowledgeCnt_.load() + successGeoDictionaryCnt_.load() +
        successAlbumCnt_.load() + successMapCnt_.load();
    info.failedCount = failedCnt_.load();

    UpgradeRestoreTaskReport()
        .SetSceneCode(sceneCode_)
        .SetTaskId(taskId_)
        .Report(info);
}

void CloneReverseRestoreGeo::RestoreReverseGeoKnowledge()
{
    MEDIA_INFO_LOG("restore reverse geo knowledge start.");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    std::vector<GeoCloneInfo> infos;
    QueryNewGeoKnowledge(infos);

    std::vector<GeoCloneInfo> insertInfos;
    std::vector<GeoCloneInfo> updateInfos;
    ClassifyGeoKnowledge(infos, insertInfos, updateInfos);

    InsertGeoKnowledgeToOld(insertInfos);
    UpdateGeoKnowledgeToOld(updateInfos);

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("TimeCost: RestoreReverseGeoKnowledge: %{public}" PRId64, end - start);
    MEDIA_INFO_LOG("restore reverse geo knowledge end.");
}

void CloneReverseRestoreGeo::QueryNewGeoKnowledge(std::vector<GeoCloneInfo> &infos)
{
    std::unordered_map<std::string, std::string> columns = {
        {"file_id", "INTEGER"}
    };
    bool hasRequiredColumns = CheckTableColumns(GEO_KNOWLEDGE_TABLE, columns, mediaLibraryRdb_);
    if (!hasRequiredColumns) {
        MEDIA_ERR_LOG("The tab_analysis_geo_knowledge does not contain file_id column.");
        ErrorInfo errorInfo(RestoreError::TABLE_LACK_OF_COLUMN,
            static_cast<int32_t>(columns.size()),
            "", "The tab_analysis_geo_knowledge does not contain file_id");
        UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).ReportError(errorInfo);
        return;
    }

    std::string querySql = "SELECT * FROM " + GEO_KNOWLEDGE_TABLE;
    auto resultSet = BackupDatabaseUtils::QuerySql(mediaLibraryRdb_, querySql);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Query resultSql is null.");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        GeoCloneInfo info;
        GetGeoInfoFromResultSet(info, resultSet);
        infos.emplace_back(info);
    }
    resultSet->Close();
    MEDIA_INFO_LOG("query new device tab_analysis_geo_knowledge nums: %{public}zu", infos.size());
}

std::unordered_set<int32_t> CloneReverseRestoreGeo::GetExistingFileIdsOld()
{
    std::unordered_set<int32_t> existingFileIds;
    std::string querySql = "SELECT file_id FROM " + GEO_KNOWLEDGE_TABLE;
    auto resultSet = BackupDatabaseUtils::QuerySql(mediaRdb_, querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, existingFileIds, "Query resultSql is null.");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t fileId = GetInt32Val(FILE_ID, resultSet);
        existingFileIds.insert(fileId);
    }
    resultSet->Close();
    return existingFileIds;
}

void CloneReverseRestoreGeo::ClassifyGeoKnowledge(std::vector<GeoCloneInfo> &infos,
    std::vector<GeoCloneInfo> &insertInfos,
    std::vector<GeoCloneInfo> &updateInfos)
{
    existingFileIdsOld_ = GetExistingFileIdsOld();

    for (auto& info : infos) {
        CHECK_AND_CONTINUE(info.fileIdOld.has_value());
        int32_t fileId = info.fileIdOld.value();
        info.fileIdNew = fileId;

        if (existingFileIdsOld_.count(fileId) != 0) {
            updateInfos.emplace_back(info);
        } else {
            insertInfos.emplace_back(info);
        }
    }

    MEDIA_INFO_LOG("geo_knowledge insert: %{public}zu, update: %{public}zu",
        insertInfos.size(), updateInfos.size());
}

void CloneReverseRestoreGeo::InsertGeoKnowledgeToOld(std::vector<GeoCloneInfo> &insertInfos)
{
    CHECK_AND_RETURN(!insertInfos.empty());

    auto intersection = GetCommonColumns(GEO_KNOWLEDGE_TABLE, mediaLibraryRdb_, mediaRdb_);
    size_t offset = 0;

    while (offset < insertInfos.size()) {
        std::vector<NativeRdb::ValuesBucket> values;
        std::vector<int32_t> batchFileIds;
        size_t batchSize = std::min(static_cast<size_t>(PAGE_SIZE),
            insertInfos.size() - offset);

        for (size_t index = 0; index < batchSize; index++) {
            CHECK_AND_CONTINUE(insertInfos[index + offset].fileIdNew.has_value());
            int32_t fileId = insertInfos[index + offset].fileIdNew.value();
            batchFileIds.emplace_back(fileId);
            NativeRdb::ValuesBucket value;
            GetGeoInsertValue(value, insertInfos[index + offset], intersection);
            values.emplace_back(value);
        }

        MEDIA_INFO_LOG("Insert reverse geo knowledge values size: %{public}zu", values.size());
        int64_t rowNum = 0;
        int32_t errCode = BatchInsertWithRetry(GEO_KNOWLEDGE_TABLE, values, rowNum, mediaRdb_);

        if (errCode != E_OK || rowNum != static_cast<int64_t>(values.size())) {
            int64_t failNums = static_cast<int64_t>(values.size()) - rowNum;
            MEDIA_ERR_LOG("Insert reverse geo knowledge fail, num: %{public}" PRId64, failNums);
            failedCnt_ += failNums;
        } else {
            successGeoKnowledgeCnt_ += rowNum;
            for (size_t i = 0; i < static_cast<size_t>(rowNum) && i < batchFileIds.size(); i++) {
                successGeoFileIds_.emplace_back(batchFileIds[i]);
            }
        }
        offset += PAGE_SIZE;
    }
}

void CloneReverseRestoreGeo::UpdateGeoKnowledgeToOld(std::vector<GeoCloneInfo> &updateInfos)
{
    CHECK_AND_RETURN(!updateInfos.empty());

    auto intersection = GetCommonColumns(GEO_KNOWLEDGE_TABLE, mediaLibraryRdb_, mediaRdb_);

    for (auto& info : updateInfos) {
        CHECK_AND_CONTINUE(info.fileIdNew.has_value());
        int32_t fileId = info.fileIdNew.value();

        NativeRdb::ValuesBucket updateValue;
        GetGeoInsertValue(updateValue, info, intersection);

        int32_t changedRows = 0;
        std::unique_ptr<NativeRdb::AbsRdbPredicates> predicates =
            std::make_unique<NativeRdb::AbsRdbPredicates>(GEO_KNOWLEDGE_TABLE);
        predicates->EqualTo(FILE_ID, std::to_string(fileId));

        int32_t errCode = BackupDatabaseUtils::Update(mediaRdb_, changedRows,
            updateValue, predicates);
        if (errCode != E_OK) {
            MEDIA_ERR_LOG("Update reverse geo knowledge fail, fileId: %{public}d", fileId);
            failedCnt_++;
        } else {
            successGeoKnowledgeCnt_++;
            successGeoFileIds_.emplace_back(fileId);
        }
    }
}

std::unordered_set<std::string> CloneReverseRestoreGeo::GetExistingCityIdsOld()
{
    std::unordered_set<std::string> existingCityIds;
    std::string querySql = "SELECT city_id FROM " + GEO_DICTIONARY_TABLE;
    auto resultSet = BackupDatabaseUtils::QuerySql(mediaRdb_, querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, existingCityIds, "Query resultSql is null.");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string cityId = GetStringVal(CITY_ID, resultSet);
        existingCityIds.insert(cityId);
    }
    resultSet->Close();
    return existingCityIds;
}

void CloneReverseRestoreGeo::RestoreReverseAlbums()
{
    MEDIA_INFO_LOG("restore reverse city albums start.");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    QueryNewCityAlbums(newAlbumInfos_);
    InsertCityAlbumsToOld(newAlbumInfos_);
    GenerateAlbumIdMap(newAlbumInfos_);

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("TimeCost: RestoreReverseAlbums: %{public}" PRId64, end - start);
    MEDIA_INFO_LOG("restore reverse city albums end.");
}

std::unordered_map<std::string, int32_t> CloneReverseRestoreGeo::QueryExistingCityAlbumNames()
{
    std::unordered_map<std::string, int32_t> existingAlbums;
    string querySql = "SELECT album_id, album_name FROM AnalysisAlbum WHERE album_name IS NOT NULL"
        " AND album_type = 4096 AND album_subtype = 4100";
    auto resultSet = BackupDatabaseUtils::QuerySql(mediaRdb_, querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, existingAlbums, "Query existing city albums failed.");
    
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t albumId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, ALBUM_ID).value_or(0);
        string albumName = BackupDatabaseUtils::GetOptionalValue<string>(resultSet, ALBUM_NAME).value_or("");
        if (!albumName.empty()) {
            existingAlbums[albumName] = albumId;
        }
    }
    resultSet->Close();
    MEDIA_INFO_LOG("QueryExistingCityAlbumNames found %{public}zu existing albums", existingAlbums.size());
    return existingAlbums;
}

void CloneReverseRestoreGeo::QueryNewCityAlbums(std::vector<AnalysisAlbumTbl> &albums)
{
    std::string querySql = "SELECT * FROM AnalysisAlbum WHERE album_type = 4096 AND album_subtype = 4100";

    auto resultSet = BackupDatabaseUtils::QuerySql(mediaLibraryRdb_, querySql);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Query resultSql is null.");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        AnalysisAlbumTbl album;
        album.albumId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, ALBUM_ID);
        album.albumName = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, ALBUM_NAME);
        album.albumType = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, ALBUM_TYPE);
        album.albumSubtype = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, ALBUM_SUBTYPE);
        albums.emplace_back(album);
    }
    resultSet->Close();
    MEDIA_INFO_LOG("query new device city albums nums: %{public}zu", albums.size());
}

void CloneReverseRestoreGeo::InsertCityAlbumsToOld(std::vector<AnalysisAlbumTbl> &albums)
{
    CHECK_AND_RETURN(!albums.empty());

    auto existingAlbums = QueryExistingCityAlbumNames();
    
    std::vector<NativeRdb::ValuesBucket> values;
    for (auto& album : albums) {
        CHECK_AND_CONTINUE(album.albumId.has_value());
        album.albumIdOld = album.albumId;
        album.albumIdNew = album.albumId;

        if (album.albumName.has_value() && existingAlbums.count(album.albumName.value()) > 0) {
            int32_t duplicateAlbumId = existingAlbums[album.albumName.value()];
            MEDIA_INFO_LOG("City album '%{public}s' already exists, using existing id %{public}d",
                album.albumName.value().c_str(), duplicateAlbumId);

            // 更新 tab_old_albums 中的 album_id 为新机 album_id
            UpdateTabOldAlbumsId(album.albumId.value(), duplicateAlbumId);

            // 更新 mediaRdb_ 中的重复相册数据
            DeleteDuplicateCityAlbum(duplicateAlbumId, album.albumId.value());
        }
        NativeRdb::ValuesBucket value;
        PutIfPresent(value, ALBUM_ID, album.albumIdNew);
        PutIfPresent(value, ALBUM_NAME, album.albumName);
        PutIfPresent(value, ALBUM_TYPE, album.albumType);
        PutIfPresent(value, ALBUM_SUBTYPE, album.albumSubtype);
        values.emplace_back(value);
    }

    int64_t rowNum = 0;
    int32_t errCode = E_OK;
    if (!values.empty()) {
        errCode = BatchInsertWithRetry(ANALYSIS_ALBUM_TABLE, values, rowNum, mediaRdb_);
        if (errCode != E_OK) {
            MEDIA_ERR_LOG("Insert reverse city albums fail");
            failedCnt_ += values.size();
        } else {
            successAlbumCnt_ += rowNum;
        }
    }
    MEDIA_INFO_LOG("InsertCityAlbumsToOld total albums processed: %{public}zu, new: %{public}ld, existing: %{public}zu",
        albums.size(), rowNum, albums.size() - values.size());
}

void CloneReverseRestoreGeo::GenerateAlbumIdMap(const std::vector<AnalysisAlbumTbl> &albums)
{
    for (const auto& album : albums) {
        if (album.albumIdOld.has_value() && album.albumIdNew.has_value()) {
            albumIdMap_[album.albumIdOld.value()] = album.albumIdNew.value();
        }
    }
    MEDIA_INFO_LOG("Generated albumIdMap size: %{public}zu", albumIdMap_.size());
}

void CloneReverseRestoreGeo::RestoreReverseMaps()
{
    MEDIA_INFO_LOG("restore reverse city maps start.");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    std::vector<CityMapInfo> maps;
    QueryNewCityMaps(maps);
    InsertCityMapsToOld(maps);

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("TimeCost: RestoreReverseMaps: %{public}" PRId64, end - start);
    MEDIA_INFO_LOG("restore reverse city maps end.");
}

void CloneReverseRestoreGeo::QueryNewCityMaps(std::vector<CityMapInfo> &maps)
{
    std::vector<int32_t> newAlbumIds;
    for (const auto& album : newAlbumInfos_) {
        if (album.albumIdOld.has_value()) {
            newAlbumIds.emplace_back(album.albumIdOld.value());
        }
    }
    CHECK_AND_RETURN(!newAlbumIds.empty());

    // 先查询总数
    std::string albumIdList;
    for (size_t i = 0; i < newAlbumIds.size(); i++) {
        albumIdList += std::to_string(newAlbumIds[i]);
        if (i < newAlbumIds.size() - 1) {
            albumIdList += ",";
        }
    }

    std::string countSql = "SELECT count(1) AS count FROM AnalysisPhotoMap WHERE map_album IN (" +
        albumIdList + ")";
    auto countResultSet = BackupDatabaseUtils::QuerySql(mediaLibraryRdb_, countSql);
    CHECK_AND_RETURN_LOG(countResultSet != nullptr, "Query countSql is null.");
    int32_t totalNumber = 0;
    if (countResultSet->GoToNextRow() == NativeRdb::E_OK) {
        totalNumber = BackupDatabaseUtils::GetOptionalValue<int32_t>(countResultSet, "count").value_or(0);
    }
    countResultSet->Close();
    MEDIA_INFO_LOG("QueryNewCityMaps totalNumber = %{public}d", totalNumber);

    // 分片查询
    for (int32_t offset = 0; offset < totalNumber; offset += PAGE_SIZE) {
        std::string querySql = "SELECT map_album, map_asset FROM AnalysisPhotoMap WHERE map_album IN (" +
            albumIdList + ") LIMIT " + std::to_string(offset) + ", " + std::to_string(PAGE_SIZE);

        auto resultSet = BackupDatabaseUtils::QuerySql(mediaLibraryRdb_, querySql);
        CHECK_AND_RETURN_LOG(resultSet != nullptr, "Query resultSql is null.");

        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            CityMapInfo mapInfo;
            mapInfo.mapAlbum = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, MAP_ALBUM);
            mapInfo.mapAsset = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, MAP_ASSET);
            maps.emplace_back(mapInfo);
        }
        resultSet->Close();
    }
    MEDIA_INFO_LOG("query new device city maps nums: %{public}zu", maps.size());
}

void CloneReverseRestoreGeo::InsertCityMapsToOld(std::vector<CityMapInfo> &maps)
{
    CHECK_AND_RETURN(!maps.empty());

    std::vector<NativeRdb::ValuesBucket> insertValues;
    std::vector<CityMapInfo> updateMaps;

    for (auto& map : maps) {
        CHECK_AND_CONTINUE(map.mapAlbum.has_value() && map.mapAsset.has_value());

        int32_t oldAlbumId = map.mapAlbum.value();
        auto it = albumIdMap_.find(oldAlbumId);
        CHECK_AND_CONTINUE(it != albumIdMap_.end());

        int32_t fileId = map.mapAsset.value();
        if (existingFileIdsOld_.count(fileId) != 0) {
            updateMaps.emplace_back(map);
        } else {
            NativeRdb::ValuesBucket value;
            value.PutInt(MAP_ALBUM, it->second);
            value.PutInt(MAP_ASSET, fileId);
            insertValues.emplace_back(value);
        }
    }

    if (!insertValues.empty()) {
        int64_t rowNum = 0;
        int32_t errCode = BatchInsertWithRetry(ANALYSIS_PHOTO_MAP_TABLE,
            insertValues, rowNum, mediaRdb_);
        if (errCode != E_OK) {
            MEDIA_ERR_LOG("Insert reverse city maps fail");
            failedCnt_ += insertValues.size();
        } else {
            successMapCnt_ += rowNum;
        }
    }

    UpdateCityMapsToOld(updateMaps);
}

void CloneReverseRestoreGeo::UpdateReverseGeoStatus()
{
    MEDIA_INFO_LOG("Update reverse geo status start.");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    if (successGeoFileIds_.empty()) {
        MEDIA_INFO_LOG("No successful geo file ids to update.");
        int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
        MEDIA_INFO_LOG("TimeCost: UpdateReverseGeoStatus: %{public}" PRId64, end - start);
        return;
    }

    std::string querySql = "SELECT file_id, " + GEO + " FROM " + GEO_TOTAL_TABLE +
        " WHERE file_id IN (" + BackupDatabaseUtils::JoinSQLValues(successGeoFileIds_, ",") + ")";
    auto resultSet = BackupDatabaseUtils::QuerySql(mediaLibraryRdb_, querySql);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Query new device geo status is null.");

    std::unordered_map<int32_t, int32_t> fileIdGeoStatusMap;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t fileId = GetInt32Val(FILE_ID, resultSet);
        int32_t geoStatus = GetInt32Val(GEO, resultSet);
        fileIdGeoStatusMap[fileId] = geoStatus;
    }
    resultSet->Close();

    for (const auto& [fileId, geoStatus] : fileIdGeoStatusMap) {
        int32_t updatedRows = 0;
        NativeRdb::ValuesBucket valuesBucket;
        valuesBucket.PutInt(GEO, geoStatus);
        std::unique_ptr<NativeRdb::AbsRdbPredicates> updatePredicates =
            std::make_unique<NativeRdb::AbsRdbPredicates>(GEO_TOTAL_TABLE);
        updatePredicates->EqualTo(FILE_ID, std::to_string(fileId));
        int32_t errCode = BackupDatabaseUtils::Update(mediaRdb_, updatedRows,
            valuesBucket, updatePredicates);

        if (errCode != E_OK) {
            MEDIA_ERR_LOG("Update reverse geo status fail, fileId: %{public}d, errCode: %{public}d",
                fileId, errCode);
            failedCnt_++;
        }
    }

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("TimeCost: UpdateReverseGeoStatus: %{public}" PRId64, end - start);
}

void CloneReverseRestoreGeo::UpdateCityMapsToOld(std::vector<CityMapInfo> &maps)
{
    CHECK_AND_RETURN(!maps.empty());

    for (auto& map : maps) {
        CHECK_AND_CONTINUE(map.mapAlbum.has_value() && map.mapAsset.has_value());

        int32_t oldAlbumId = map.mapAlbum.value();
        auto it = albumIdMap_.find(oldAlbumId);
        CHECK_AND_CONTINUE(it != albumIdMap_.end());

        int32_t fileId = map.mapAsset.value();
        int32_t newAlbumId = it->second;

        NativeRdb::ValuesBucket updateValue;
        updateValue.PutInt(MAP_ALBUM, newAlbumId);

        int32_t changedRows = 0;
        std::unique_ptr<NativeRdb::AbsRdbPredicates> predicates =
            std::make_unique<NativeRdb::AbsRdbPredicates>(ANALYSIS_PHOTO_MAP_TABLE);
        predicates->EqualTo(MAP_ASSET, std::to_string(fileId));

        int32_t errCode = BackupDatabaseUtils::Update(mediaRdb_, changedRows,
            updateValue, predicates);
        if (errCode != E_OK) {
            MEDIA_ERR_LOG("Update reverse city maps fail, fileId: %{public}d", fileId);
            failedCnt_++;
        } else {
            successMapCnt_++;
        }
    }
}

void CloneReverseRestoreGeo::DeleteDuplicateCityAlbum(int32_t oldAlbumId, int32_t newAlbumId)
{
    MEDIA_INFO_LOG("DeleteDuplicateCityAlbum start, oldAlbumId=%{public}d, newAlbumId=%{public}d",
        oldAlbumId, newAlbumId);

    // 更新 AnalysisPhotoMap 中 map_album 从 oldAlbumId 改为 newAlbumId
    NativeRdb::ValuesBucket updateMapValues;
    updateMapValues.PutInt("map_album", newAlbumId);
    std::unique_ptr<NativeRdb::AbsRdbPredicates> updateMapPredicates =
        std::make_unique<NativeRdb::AbsRdbPredicates>(ANALYSIS_PHOTO_MAP_TABLE);
    updateMapPredicates->EqualTo("map_album", std::to_string(oldAlbumId));
    int32_t updatedMapRows = 0;
    int32_t ret = BackupDatabaseUtils::Update(mediaRdb_, updatedMapRows, updateMapValues, updateMapPredicates);
    if (ret == E_OK) {
        MEDIA_INFO_LOG("DeleteDuplicateCityAlbum updated %{public}d rows in AnalysisPhotoMap",
            updatedMapRows);
    } else {
        MEDIA_ERR_LOG("DeleteDuplicateCityAlbum failed to update AnalysisPhotoMap, ret=%{public}d",
            ret);
    }

    // 删除 AnalysisAlbum 中 album_id = oldAlbumId 的数据
    NativeRdb::AbsRdbPredicates deleteAlbumPredicates("AnalysisAlbum");
    deleteAlbumPredicates.EqualTo("album_id", std::to_string(oldAlbumId));
    int32_t deletedAlbumRows = 0;
    ret = BackupDatabaseUtils::Delete(deleteAlbumPredicates, deletedAlbumRows, mediaRdb_);
    if (ret == E_OK) {
        MEDIA_INFO_LOG("DeleteDuplicateCityAlbum deleted %{public}d rows from AnalysisAlbum",
            deletedAlbumRows);
    } else {
        MEDIA_ERR_LOG("DeleteDuplicateCityAlbum failed to delete from AnalysisAlbum, ret=%{public}d",
            ret);
    }

    MEDIA_INFO_LOG("DeleteDuplicateCityAlbum end, deleted %{public}d album rows and updated %{public}d map rows",
        deletedAlbumRows, updatedMapRows);
}

void CloneReverseRestoreGeo::UpdateTabOldAlbumsId(int32_t oldAlbumId, int32_t newAlbumId)
{
    MEDIA_INFO_LOG("UpdateTabOldAlbumsId start, oldAlbumId=%{public}d, newAlbumId=%{public}d",
                   oldAlbumId, newAlbumId);

    if (mediaRdb_ == nullptr) {
        MEDIA_ERR_LOG("mediaRdb_ is null");
        return;
    }

    std::string updateQuery = "UPDATE " + TAB_OLD_ALBUMS + " SET " + ALBUM_ID_COL +
                             " = " + std::to_string(newAlbumId) +
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