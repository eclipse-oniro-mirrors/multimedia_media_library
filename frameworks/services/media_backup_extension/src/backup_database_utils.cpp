/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "backup_database_utils.h"

#include <nlohmann/json.hpp>
#include <safe_map.h>

#include "backup_const_column.h"
#include "media_file_utils.h"
#include "media_log.h"

namespace OHOS {
namespace Media {
const int32_t SCALE_FACTOR = 2;
const int32_t SCALE_MIN_SIZE = 1080;
const int32_t SCALE_MAX_SIZE = 2560;
const int32_t UPDATE_COUNT = 200;
const float SCALE_DEFAULT = 0.25;
const size_t MIN_GARBLE_SIZE = 2;
const size_t GARBLE_START = 1;
const size_t XY_DIMENSION = 2;
const size_t BYTE_LEN = 4;
const size_t BYTE_BASE_OFFSET = 8;
const size_t LANDMARKS_SIZE = 5;
const std::string LANDMARK_X = "x";
const std::string LANDMARK_Y = "y";
const std::vector<uint32_t> HEX_MAX = { 0xff, 0xffff, 0xffffff, 0xffffffff };
static SafeMap<int32_t, int32_t> fileIdOld2NewForCloudEnhancement;

int32_t BackupDatabaseUtils::InitDb(std::shared_ptr<NativeRdb::RdbStore> &rdbStore, const std::string &dbName,
    const std::string &dbPath, const std::string &bundleName, bool isMediaLibrary, int32_t area)
{
    NativeRdb::RdbStoreConfig config(dbName);
    config.SetPath(dbPath);
    config.SetBundleName(bundleName);
    config.SetReadConSize(CONNECT_SIZE);
    config.SetSecurityLevel(NativeRdb::SecurityLevel::S3);
    config.SetHaMode(NativeRdb::HAMode::MANUAL_TRIGGER);
    config.SetAllowRebuild(true);
    if (area != DEFAULT_AREA_VERSION) {
        config.SetArea(area);
    }
    if (isMediaLibrary) {
        config.SetScalarFunction("cloud_sync_func", 0, CloudSyncTriggerFunc);
        config.SetScalarFunction("is_caller_self_func", 0, IsCallerSelfFunc);
    }
    int32_t err;
    RdbCallback cb;
    rdbStore = NativeRdb::RdbHelper::GetRdbStore(config, MEDIA_RDB_VERSION, cb, err);
    return err;
}

std::string BackupDatabaseUtils::CloudSyncTriggerFunc(const std::vector<std::string> &args)
{
    return "";
}

std::string BackupDatabaseUtils::IsCallerSelfFunc(const std::vector<std::string> &args)
{
    return "false";
}

int32_t BackupDatabaseUtils::QueryInt(std::shared_ptr<NativeRdb::RdbStore> rdbStore, const std::string &sql,
    const std::string &column)
{
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdb_ is nullptr, Maybe init failed.");
        return 0;
    }
    auto resultSet = rdbStore->QuerySql(sql);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        return 0;
    }
    int32_t result = GetInt32Val(column, resultSet);
    return result;
}

int32_t BackupDatabaseUtils::Update(std::shared_ptr<NativeRdb::RdbStore> &rdbStore, int32_t &changeRows,
    NativeRdb::ValuesBucket &valuesBucket, std::unique_ptr<NativeRdb::AbsRdbPredicates> &predicates)
{
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdb_ is nullptr, Maybe init failed.");
        return E_FAIL;
    }
    return rdbStore->Update(changeRows, valuesBucket, *predicates);
}

int32_t BackupDatabaseUtils::Delete(NativeRdb::AbsRdbPredicates &predicates, int32_t &changeRows,
    std::shared_ptr<NativeRdb::RdbStore> &rdbStore)
{
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdb is nullptr");
        return E_FAIL;
    }
    return rdbStore->Delete(changeRows, predicates);
}

int32_t BackupDatabaseUtils::InitGarbageAlbum(std::shared_ptr<NativeRdb::RdbStore> galleryRdb,
    std::set<std::string> &cacheSet, std::unordered_map<std::string, std::string> &nickMap)
{
    if (galleryRdb == nullptr) {
        MEDIA_ERR_LOG("Pointer rdb_ is nullptr, Maybe init failed.");
        return E_FAIL;
    }

    const string querySql = "SELECT nick_dir, nick_name FROM garbage_album where type = 0";
    auto resultSet = galleryRdb->QuerySql(QUERY_GARBAGE_ALBUM);
    if (resultSet == nullptr) {
        return E_HAS_DB_ERROR;
    }
    int32_t count = -1;
    int32_t err = resultSet->GetRowCount(count);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to get count, err: %{public}d", err);
        return E_FAIL;
    }
    MEDIA_INFO_LOG("garbageCount: %{public}d", count);
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t type;
        resultSet->GetInt(INDEX_TYPE, type);
        if (type == NICK) {
            string nickName;
            string nickDir;
            resultSet->GetString(INDEX_NICK_DIR, nickDir);
            resultSet->GetString(INDEX_NICK_NAME, nickName);
            nickMap[nickDir] = nickName;
        } else {
            string cacheDir;
            resultSet->GetString(INDEX_CACHE_DIR, cacheDir);
            cacheSet.insert(cacheDir);
        }
    }
    MEDIA_INFO_LOG("add map success!");
    resultSet->Close();
    return E_OK;
}

int32_t BackupDatabaseUtils::QueryGalleryAllCount(std::shared_ptr<NativeRdb::RdbStore> galleryRdb)
{
    static string QUERY_GALLERY_ALL_COUNT = "SELECT count(1) AS count FROM gallery_media";
    return QueryInt(galleryRdb, QUERY_GALLERY_ALL_COUNT, CUSTOM_COUNT);
}

int32_t BackupDatabaseUtils::QueryGalleryImageCount(std::shared_ptr<NativeRdb::RdbStore> galleryRdb)
{
    static string QUERY_GALLERY_IMAGE_COUNT =
        "SELECT count(1) AS count FROM gallery_media WHERE media_type = 1 AND _size > 0";
    return QueryInt(galleryRdb, QUERY_GALLERY_IMAGE_COUNT, CUSTOM_COUNT);
}

int32_t BackupDatabaseUtils::QueryGalleryVideoCount(std::shared_ptr<NativeRdb::RdbStore> galleryRdb)
{
    static string QUERY_GALLERY_VIDEO_COUNT =
        "SELECT count(1) AS count FROM gallery_media WHERE media_type = 3 AND _size > 0";
    return QueryInt(galleryRdb, QUERY_GALLERY_VIDEO_COUNT, CUSTOM_COUNT);
}

int32_t BackupDatabaseUtils::QueryGalleryHiddenCount(std::shared_ptr<NativeRdb::RdbStore> galleryRdb)
{
    static string QUERY_GALLERY_HIDDEN_COUNT =
        "SELECT count(1) AS count FROM gallery_media WHERE local_media_id = -4 AND _size > 0";
    return QueryInt(galleryRdb, QUERY_GALLERY_HIDDEN_COUNT, CUSTOM_COUNT);
}

int32_t BackupDatabaseUtils::QueryGalleryTrashedCount(std::shared_ptr<NativeRdb::RdbStore> galleryRdb)
{
    static string QUERY_GALLERY_TRASHED_COUNT =
        "SELECT count(1) AS count FROM gallery_media WHERE local_media_id = 0 AND _size > 0";
    return QueryInt(galleryRdb, QUERY_GALLERY_TRASHED_COUNT, CUSTOM_COUNT);
}

int32_t BackupDatabaseUtils::QueryGalleryFavoriteCount(std::shared_ptr<NativeRdb::RdbStore> galleryRdb)
{
    static string QUERY_GALLERY_FAVORITE_COUNT =
        "SELECT count(1) AS count FROM gallery_media WHERE is_hw_favorite = 1 AND _size > 0 AND local_media_id != -1";
    return QueryInt(galleryRdb, QUERY_GALLERY_FAVORITE_COUNT, CUSTOM_COUNT);
}

int32_t BackupDatabaseUtils::QueryGalleryImportsCount(std::shared_ptr<NativeRdb::RdbStore> galleryRdb)
{
    static string QUERY_GALLERY_IMPORTS_COUNT =
        string("SELECT count(1) AS count FROM gallery_media WHERE ") +
        " _data LIKE '/storage/emulated/0/Pictures/cloud/Imports%' AND _size > 0 AND local_media_id != -1";
    return QueryInt(galleryRdb, QUERY_GALLERY_IMPORTS_COUNT, CUSTOM_COUNT);
}

int32_t BackupDatabaseUtils::QueryGalleryCloneCount(std::shared_ptr<NativeRdb::RdbStore> galleryRdb)
{
    static string QUERY_GALLERY_CLONE_COUNT =
        string("SELECT count(1) AS count FROM gallery_media WHERE local_media_id = -3 AND _size > 0 ") +
        "AND (storage_id IN (0, 65537)) AND relative_bucket_id NOT IN ( " +
        "SELECT DISTINCT relative_bucket_id FROM garbage_album WHERE type = 1)";
    return QueryInt(galleryRdb, QUERY_GALLERY_CLONE_COUNT, CUSTOM_COUNT);
}

int32_t BackupDatabaseUtils::QueryGallerySdCardCount(std::shared_ptr<NativeRdb::RdbStore> galleryRdb)
{
    static string QUERY_GALLERY_SD_CARD_COUNT =
        "SELECT count(1) AS count FROM gallery_media WHERE storage_id NOT IN (0, 65537) AND _size > 0";
    return QueryInt(galleryRdb, QUERY_GALLERY_SD_CARD_COUNT, CUSTOM_COUNT);
}

int32_t BackupDatabaseUtils::QueryGalleryScreenVideoCount(std::shared_ptr<NativeRdb::RdbStore> galleryRdb)
{
    static string QUERY_GALLERY_SCRENN_VIDEO_COUNT =
        "SELECT count(1) AS count FROM gallery_media \
        WHERE local_media_id = -3 AND bucket_id = 1028075469 AND _size > 0";
    return QueryInt(galleryRdb, QUERY_GALLERY_SCRENN_VIDEO_COUNT, CUSTOM_COUNT);
}

int32_t BackupDatabaseUtils::QueryGalleryCloudCount(std::shared_ptr<NativeRdb::RdbStore> galleryRdb)
{
    static string QUERY_GALLERY_CLOUD_COUNT =
        "SELECT count(1) AS count FROM gallery_media \
        WHERE local_media_id = -1 AND _size > 0";
    return QueryInt(galleryRdb, QUERY_GALLERY_CLOUD_COUNT, CUSTOM_COUNT);
}

int32_t BackupDatabaseUtils::QueryGalleryBurstCoverCount(std::shared_ptr<NativeRdb::RdbStore> galleryRdb)
{
    static string QUERY_GALLERY_BURST_COVER_COUNT =
        "SELECT count(1) AS count FROM gallery_media WHERE is_hw_burst = 1 AND _size > 0";
    return QueryInt(galleryRdb, QUERY_GALLERY_BURST_COVER_COUNT, CUSTOM_COUNT);
}

int32_t BackupDatabaseUtils::QueryGalleryBurstTotalCount(std::shared_ptr<NativeRdb::RdbStore> galleryRdb)
{
    static string QUERY_GALLERY_BURST_TOTAL_COUNT =
        "SELECT count(1) AS count FROM gallery_media WHERE is_hw_burst IN (1, 2) AND _size > 0";
    return QueryInt(galleryRdb, QUERY_GALLERY_BURST_TOTAL_COUNT, CUSTOM_COUNT);
}

int32_t BackupDatabaseUtils::QueryExternalImageCount(std::shared_ptr<NativeRdb::RdbStore> externalRdb)
{
    static string QUERY_EXTERNAL_IMAGE_COUNT =
        "SELECT count(1) AS count FROM files WHERE  media_type = 1 AND _size > 0";
    return QueryInt(externalRdb, QUERY_EXTERNAL_IMAGE_COUNT, CUSTOM_COUNT);
}

int32_t BackupDatabaseUtils::QueryExternalVideoCount(std::shared_ptr<NativeRdb::RdbStore> externalRdb)
{
    static string QUERY_EXTERNAL_VIDEO_COUNT =
        "SELECT count(1) AS count FROM files WHERE  media_type = 3 AND _size > 0";
    return QueryInt(externalRdb, QUERY_EXTERNAL_VIDEO_COUNT, CUSTOM_COUNT);
}

std::shared_ptr<NativeRdb::ResultSet> BackupDatabaseUtils::GetQueryResultSet(
    const std::shared_ptr<NativeRdb::RdbStore> &rdbStore, const std::string &querySql,
    const std::vector<std::string> &sqlArgs)
{
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdbStore is nullptr");
        return nullptr;
    }
    return rdbStore->QuerySql(querySql, sqlArgs);
}

std::unordered_map<std::string, std::string> BackupDatabaseUtils::GetColumnInfoMap(
    const std::shared_ptr<NativeRdb::RdbStore> &rdbStore, const std::string &tableName)
{
    std::unordered_map<std::string, std::string> columnInfoMap;
    std::string querySql = "SELECT name, type FROM pragma_table_info('" + tableName + "')";
    auto resultSet = GetQueryResultSet(rdbStore, querySql);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("resultSet is nullptr");
        return columnInfoMap;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string columnName = GetStringVal(PRAGMA_TABLE_NAME, resultSet);
        std::string columnType = GetStringVal(PRAGMA_TABLE_TYPE, resultSet);
        if (columnName.empty() || columnType.empty()) {
            MEDIA_ERR_LOG("Empty column name or type: %{public}s, %{public}s", columnName.c_str(), columnType.c_str());
            continue;
        }
        columnInfoMap[columnName] = columnType;
    }
    return columnInfoMap;
}

void BackupDatabaseUtils::UpdateUniqueNumber(const std::shared_ptr<NativeRdb::RdbStore> &rdbStore, int32_t number,
    const std::string &type)
{
    const string updateSql =
        "UPDATE UniqueNumber SET unique_number = " + to_string(number) + " WHERE media_type = '" + type + "'";
    int32_t erroCode = rdbStore->ExecuteSql(updateSql);
    if (erroCode < 0) {
        MEDIA_ERR_LOG("execute update unique number failed, ret=%{public}d", erroCode);
    }
}

int32_t BackupDatabaseUtils::QueryUniqueNumber(const std::shared_ptr<NativeRdb::RdbStore> &rdbStore,
    const std::string &type)
{
    const string querySql = "SELECT unique_number FROM UniqueNumber WHERE media_type = '" + type + "'";
    return QueryInt(rdbStore, querySql, UNIQUE_NUMBER);
}

std::string BackupDatabaseUtils::GarbleInfoName(const string &infoName)
{
    std::string garbledInfoName = infoName;
    if (infoName.size() <= MIN_GARBLE_SIZE) {
        return garbledInfoName;
    }
    size_t garbledSize = infoName.size() - MIN_GARBLE_SIZE;
    garbledInfoName.replace(GARBLE_START, garbledSize, GARBLE);
    return garbledInfoName;
}

int32_t BackupDatabaseUtils::QueryExternalAudioCount(std::shared_ptr<NativeRdb::RdbStore> externalRdb)
{
    static string QUERY_EXTERNAL_AUDIO_COUNT = "SELECT count(1) as count FROM files WHERE media_type = 2 AND _size > 0 \
        AND _data LIKE '/storage/emulated/0/Music%'";
    return QueryInt(externalRdb, QUERY_EXTERNAL_AUDIO_COUNT, CUSTOM_COUNT);
}

void BackupDatabaseUtils::UpdateSelection(std::string &selection, const std::string &selectionToAdd, bool needWrap)
{
    if (selectionToAdd.empty()) {
        return;
    }
    std::string wrappedSelectionToAdd = needWrap ? "'" + selectionToAdd + "'" : selectionToAdd;
    selection += selection.empty() ? wrappedSelectionToAdd : ", " + wrappedSelectionToAdd;
}

void BackupDatabaseUtils::UpdateSdWhereClause(std::string &querySql, bool shouldIncludeSd)
{
    if (shouldIncludeSd) {
        return;
    }
    querySql += " AND " + EXCLUDE_SD;
}

int32_t BackupDatabaseUtils::GetBlob(const std::string &columnName, std::shared_ptr<NativeRdb::ResultSet> resultSet,
    std::vector<uint8_t> &blobVal)
{
    int32_t columnIndex = 0;
    int32_t errCode = resultSet->GetColumnIndex(columnName, columnIndex);
    if (errCode) {
        MEDIA_ERR_LOG("Get column index errCode: %{public}d", errCode);
        return E_FAIL;
    }
    if (resultSet->GetBlob(columnIndex, blobVal) != NativeRdb::E_OK) {
        return E_FAIL;
    }
    return E_OK;
}

std::string BackupDatabaseUtils::GetLandmarksStr(const std::string &columnName,
    std::shared_ptr<NativeRdb::ResultSet> resultSet)
{
    std::vector<uint8_t> blobVal;
    if (GetBlob(columnName, resultSet, blobVal) != E_OK) {
        MEDIA_ERR_LOG("Get blob failed");
        return "";
    }
    return GetLandmarksStr(blobVal);
}

std::string BackupDatabaseUtils::GetLandmarksStr(const std::vector<uint8_t> &bytes)
{
    if (bytes.size() != LANDMARKS_SIZE * XY_DIMENSION * BYTE_LEN) {
        MEDIA_ERR_LOG("Get landmarks bytes size: %{public}zu, not %{public}zu", bytes.size(),
            LANDMARKS_SIZE * XY_DIMENSION * BYTE_LEN);
        return "";
    }
    nlohmann::json landmarksJson;
    for (size_t index = 0; index < bytes.size(); index += XY_DIMENSION * BYTE_LEN) {
        nlohmann::json landmarkJson;
        landmarkJson[LANDMARK_X] = GetUint32ValFromBytes(bytes, index);
        landmarkJson[LANDMARK_Y] = GetUint32ValFromBytes(bytes, index + BYTE_LEN);
        landmarksJson.push_back(landmarkJson);
    }
    return landmarksJson.dump();
}

uint32_t BackupDatabaseUtils::GetUint32ValFromBytes(const std::vector<uint8_t> &bytes, size_t start)
{
    uint32_t uint32Val = 0;
    for (size_t index = 0; index < BYTE_LEN; index++) {
        uint32Val |= static_cast<uint32_t>(bytes[start + index]) << (index * BYTE_BASE_OFFSET);
        uint32Val &= HEX_MAX[index];
    }
    return uint32Val;
}

void BackupDatabaseUtils::UpdateAnalysisTotalStatus(std::shared_ptr<NativeRdb::RdbStore> rdbStore)
{
    std::string updateSql = "UPDATE tab_analysis_total SET face = CASE WHEN EXISTS \
        (SELECT 1 FROM tab_analysis_image_face WHERE tab_analysis_image_face.file_id = tab_analysis_total.file_id \
        AND tag_id = '-1') THEN 2 ELSE 3 END WHERE EXISTS (SELECT 1 FROM tab_analysis_image_face WHERE \
        tab_analysis_image_face.file_id = tab_analysis_total.file_id)";
    int32_t errCode = rdbStore->ExecuteSql(updateSql);
    if (errCode < 0) {
        MEDIA_ERR_LOG("execute update analysis total failed, ret=%{public}d", errCode);
    }
}

void BackupDatabaseUtils::UpdateAnalysisFaceTagStatus(std::shared_ptr<NativeRdb::RdbStore> rdbStore)
{
    std::string updateSql = "UPDATE tab_analysis_face_tag SET count = (SELECT count(1) from tab_analysis_image_face \
        WHERE tab_analysis_image_face.tag_id = tab_analysis_face_tag.tag_id)";
    int32_t errCode = rdbStore->ExecuteSql(updateSql);
    if (errCode < 0) {
        MEDIA_ERR_LOG("execute update analysis face tag count failed, ret=%{public}d", errCode);
    }
}

void BackupDatabaseUtils::UpdateAnalysisTotalTblStatus(std::shared_ptr<NativeRdb::RdbStore> rdbStore,
    const std::vector<FileIdPair>& fileIdPair)
{
    std::string fileIdNewFilterClause = GetFileIdNewFilterClause(rdbStore, fileIdPair);
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
                      "WHERE tab_analysis_image_face.file_id = tab_analysis_total.file_id "
                      "AND " + IMAGE_FACE_COL_FILE_ID + " IN " + fileIdNewFilterClause + ")";

    int32_t errCode = rdbStore->ExecuteSql(updateSql);
    if (errCode < 0) {
        MEDIA_ERR_LOG("execute update analysis total failed, ret=%{public}d", errCode);
    }
}

void BackupDatabaseUtils::UpdateFaceAnalysisTblStatus(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb)
{
    BackupDatabaseUtils::UpdateAnalysisFaceTagStatus(mediaLibraryRdb);
}

bool BackupDatabaseUtils::SetTagIdNew(PortraitAlbumInfo &portraitAlbumInfo,
    std::unordered_map<std::string, std::string> &tagIdMap)
{
    portraitAlbumInfo.tagIdNew = TAG_ID_PREFIX + std::to_string(MediaFileUtils::UTCTimeNanoSeconds());
    tagIdMap[portraitAlbumInfo.tagIdOld] = portraitAlbumInfo.tagIdNew;
    return true;
}

bool BackupDatabaseUtils::SetLandmarks(FaceInfo &faceInfo, const std::unordered_map<std::string, FileInfo> &fileInfoMap)
{
    if (faceInfo.hash.empty() || fileInfoMap.count(faceInfo.hash) == 0) {
        MEDIA_ERR_LOG("Set landmarks for face %{public}s failed, no such file hash", faceInfo.faceId.c_str());
        return false;
    }
    FileInfo fileInfo = fileInfoMap.at(faceInfo.hash);
    if (fileInfo.width == 0 || fileInfo.height == 0) {
        MEDIA_ERR_LOG("Set landmarks for face %{public}s failed, invalid width %{public}d or height %{public}d",
            faceInfo.faceId.c_str(), fileInfo.width, fileInfo.height);
        return false;
    }
    float scale = GetLandmarksScale(fileInfo.width, fileInfo.height);
    if (scale == 0) {
        MEDIA_ERR_LOG("Set landmarks for face %{public}s failed, scale = 0", faceInfo.faceId.c_str());
        return false;
    }
    nlohmann::json landmarksJson = nlohmann::json::parse(faceInfo.landmarks, nullptr, false);
    if (landmarksJson.is_discarded()) {
        MEDIA_ERR_LOG("Set landmarks for face %{public}s failed, parse landmarks failed", faceInfo.faceId.c_str());
        return false;
    }
    for (auto &landmark : landmarksJson) {
        if (!landmark.contains(LANDMARK_X) || !landmark.contains(LANDMARK_Y)) {
            MEDIA_ERR_LOG("Set landmarks for face %{public}s failed, lack of x or y", faceInfo.faceId.c_str());
            return false;
        }
        landmark[LANDMARK_X] = static_cast<float>(landmark[LANDMARK_X]) / fileInfo.width / scale;
        landmark[LANDMARK_Y] = static_cast<float>(landmark[LANDMARK_Y]) / fileInfo.height / scale;
        if (IsLandmarkValid(faceInfo, landmark[LANDMARK_X], landmark[LANDMARK_Y])) {
            continue;
        }
        MEDIA_WARN_LOG("Given landmark may be invalid, (%{public}f, %{public}f), rect TL: (%{public}f, %{public}f), "
            "rect BR: (%{public}f, %{public}f)", static_cast<float>(landmark[LANDMARK_X]),
            static_cast<float>(landmark[LANDMARK_Y]), faceInfo.scaleX, faceInfo.scaleY,
            faceInfo.scaleX + faceInfo.scaleWidth, faceInfo.scaleY + faceInfo.scaleHeight);
    }
    faceInfo.landmarks = landmarksJson.dump();
    return true;
}

bool BackupDatabaseUtils::SetFileIdNew(FaceInfo &faceInfo, const std::unordered_map<std::string, FileInfo> &fileInfoMap)
{
    if (faceInfo.hash.empty() || fileInfoMap.count(faceInfo.hash) == 0) {
        MEDIA_ERR_LOG("Set new file_id for face %{public}s failed, no such file hash", faceInfo.faceId.c_str());
        return false;
    }
    faceInfo.fileIdNew = fileInfoMap.at(faceInfo.hash).fileIdNew;
    if (faceInfo.fileIdNew <= 0) {
        MEDIA_ERR_LOG("Set new file_id for face %{public}s failed, file_id %{public}d <= 0", faceInfo.faceId.c_str(),
            faceInfo.fileIdNew);
        return false;
    }
    return true;
}

bool BackupDatabaseUtils::SetTagIdNew(FaceInfo &faceInfo, const std::unordered_map<std::string, std::string> &tagIdMap)
{
    if (faceInfo.tagIdOld.empty()) {
        MEDIA_ERR_LOG("Set new tag_id for face %{public}s failed, empty tag_id", faceInfo.faceId.c_str());
        return false;
    }
    if (tagIdMap.count(faceInfo.tagIdOld) == 0) {
        faceInfo.tagIdNew = TAG_ID_UNPROCESSED;
        return true;
    }
    faceInfo.tagIdNew = tagIdMap.at(faceInfo.tagIdOld);
    if (faceInfo.tagIdNew.empty() || !MediaFileUtils::StartsWith(faceInfo.tagIdNew, TAG_ID_PREFIX)) {
        MEDIA_ERR_LOG("Set new tag_id for face %{public}s failed, new tag_id %{public}s empty or invalid",
            faceInfo.tagIdNew.c_str(), faceInfo.faceId.c_str());
        return false;
    }
    return true;
}

bool BackupDatabaseUtils::SetAlbumIdNew(FaceInfo &faceInfo, const std::unordered_map<std::string, int32_t> &albumIdMap)
{
    if (faceInfo.tagIdNew == TAG_ID_UNPROCESSED) {
        return true;
    }
    if (albumIdMap.count(faceInfo.tagIdNew) == 0) {
        MEDIA_ERR_LOG("Set new album_id for face %{public}s failed, no such tag_id", faceInfo.faceId.c_str());
        return false;
    }
    faceInfo.albumIdNew = albumIdMap.at(faceInfo.tagIdNew);
    if (faceInfo.albumIdNew <= 0) {
        MEDIA_ERR_LOG("Set new album_id for face %{public}s failed, album_id %{public}d <= 0", faceInfo.faceId.c_str(),
            faceInfo.albumIdNew);
        return false;
    }
    return true;
}

void BackupDatabaseUtils::PrintErrorLog(const std::string &errorLog, int64_t start)
{
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("%{public}s, cost %{public}ld", errorLog.c_str(), (long)(end - start));
}

float BackupDatabaseUtils::GetLandmarksScale(int32_t width, int32_t height)
{
    float scale = 1;
    int32_t minWidthHeight = width <= height ? width : height;
    if (minWidthHeight >= SCALE_MIN_SIZE * SCALE_FACTOR) {
        minWidthHeight = static_cast<int32_t>(minWidthHeight * SCALE_DEFAULT);
        scale = SCALE_DEFAULT;
        if (minWidthHeight < SCALE_MIN_SIZE) {
            minWidthHeight *= SCALE_FACTOR;
            scale *= SCALE_FACTOR;
        }
        if (minWidthHeight < SCALE_MIN_SIZE) {
            scale = 1;
        }
    }
    width = static_cast<int32_t>(width * scale);
    height = static_cast<int32_t>(height * scale);
    int32_t maxWidthHeight = width >= height ? width : height;
    scale *= maxWidthHeight >= SCALE_MAX_SIZE ? static_cast<float>(SCALE_MAX_SIZE) / maxWidthHeight : 1;
    return scale;
}

bool BackupDatabaseUtils::IsLandmarkValid(const FaceInfo &faceInfo, float landmarkX, float landmarkY)
{
    return IsValInBound(landmarkX, faceInfo.scaleX, faceInfo.scaleX + faceInfo.scaleWidth) &&
        IsValInBound(landmarkY, faceInfo.scaleY, faceInfo.scaleY + faceInfo.scaleHeight);
}

bool BackupDatabaseUtils::IsValInBound(float val, float minVal, float maxVal)
{
    return val >= minVal && val <= maxVal;
}

std::vector<std::pair<std::string, std::string>> BackupDatabaseUtils::GetColumnInfoPairs(
    const std::shared_ptr<NativeRdb::RdbStore> &rdbStore, const std::string &tableName)
{
    std::vector<std::pair<std::string, std::string>> columnInfoPairs;
    std::string querySql = "SELECT name, type FROM pragma_table_info('" + tableName + "')";
    auto resultSet = GetQueryResultSet(rdbStore, querySql);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("resultSet is nullptr");
        return columnInfoPairs;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string columnName = GetStringVal(PRAGMA_TABLE_NAME, resultSet);
        std::string columnType = GetStringVal(PRAGMA_TABLE_TYPE, resultSet);
        if (columnName.empty() || columnType.empty()) {
            MEDIA_ERR_LOG("Empty column name or type: %{public}s, %{public}s", columnName.c_str(), columnType.c_str());
            continue;
        }
        columnInfoPairs.emplace_back(columnName, columnType);
    }

    return columnInfoPairs;
}

std::vector<std::string> BackupDatabaseUtils::GetCommonColumnInfos(std::shared_ptr<NativeRdb::RdbStore> mediaRdb,
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb, std::string tableName)
{
    std::vector<std::string> commonColumns;
    auto mediaRdbColumnInfoPairs = BackupDatabaseUtils::GetColumnInfoPairs(mediaRdb, tableName);
    auto mediaLibraryRdbColumnInfoPairs = BackupDatabaseUtils::GetColumnInfoPairs(mediaLibraryRdb, tableName);

    for (const auto &pair : mediaRdbColumnInfoPairs) {
        auto it = std::find_if(mediaLibraryRdbColumnInfoPairs.begin(), mediaLibraryRdbColumnInfoPairs.end(),
            [&](const std::pair<std::string, std::string> &p) {
                return p.first == pair.first && p.second == pair.second;
            });
        if (it != mediaLibraryRdbColumnInfoPairs.end()) {
            commonColumns.emplace_back(pair.first);
        }
    }

    return commonColumns;
}

std::vector<std::string> BackupDatabaseUtils::filterColumns(const std::vector<std::string>& allColumns,
    const std::vector<std::string>& excludedColumns)
{
    std::vector<std::string> filteredColumns;
    std::copy_if(allColumns.begin(), allColumns.end(), std::back_inserter(filteredColumns),
        [&excludedColumns](const std::string& column) {
            return std::find(excludedColumns.begin(), excludedColumns.end(), column) == excludedColumns.end();
        });
    return filteredColumns;
}

void BackupDatabaseUtils::UpdateAnalysisPhotoMapStatus(std::shared_ptr<NativeRdb::RdbStore> rdbStore)
{
    std::string insertSql =
        "INSERT OR REPLACE INTO AnalysisPhotoMap (map_album, map_asset) "
        "SELECT AnalysisAlbum.album_id, tab_analysis_image_face.file_id "
        "FROM AnalysisAlbum "
        "INNER JOIN tab_analysis_image_face ON AnalysisAlbum.tag_id = tab_analysis_image_face.tag_id";

    int32_t ret = rdbStore->ExecuteSql(insertSql);
    if (ret < 0) {
        MEDIA_ERR_LOG("execute update AnalysisPhotoMap failed, ret=%{public}d", ret);
    }
}

std::vector<FileIdPair> BackupDatabaseUtils::CollectFileIdPairs(const std::vector<FileInfo>& fileInfos)
{
    std::set<FileIdPair> uniquePairs;

    for (const auto& fileInfo : fileInfos) {
        uniquePairs.emplace(fileInfo.fileIdOld, fileInfo.fileIdNew);
    }

    return std::vector<FileIdPair>(uniquePairs.begin(), uniquePairs.end());
}

std::pair<std::vector<int32_t>, std::vector<int32_t>> BackupDatabaseUtils::UnzipFileIdPairs(
    const std::vector<FileIdPair>& pairs)
{
    std::vector<int32_t> oldFileIds;
    std::vector<int32_t> newFileIds;

    for (const auto& pair : pairs) {
        oldFileIds.push_back(pair.first);
        newFileIds.push_back(pair.second);
    }

    return {std::move(oldFileIds), std::move(newFileIds)};
}

std::vector<std::string> BackupDatabaseUtils::SplitString(const std::string& str, char delimiter)
{
    std::vector<std::string> elements;
    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, delimiter)) {
        if (!item.empty()) {
            elements.emplace_back(item);
        }
    }
    return elements;
}

void BackupDatabaseUtils::PrintQuerySql(const std::string& querySql)
{
    MEDIA_INFO_LOG("Generated SQL Query:");
    MEDIA_INFO_LOG("--------------------");
    MEDIA_INFO_LOG("%{public}s", querySql.c_str());
    MEDIA_INFO_LOG("--------------------");
}

bool BackupDatabaseUtils::DeleteDuplicatePortraitAlbum(const std::vector<std::string> &albumNames,
    const std::vector<std::string> tagIds, std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb)
{
    std::set<std::string> uniqueAlbums(albumNames.begin(), albumNames.end());
    std::vector<std::string> uniqueAlbumNames(uniqueAlbums.begin(), uniqueAlbums.end());
    MEDIA_INFO_LOG("unique AlbumName %{public}zu", uniqueAlbumNames.size());

    std::string inClause = BackupDatabaseUtils::JoinSQLValues<string>(uniqueAlbumNames, ", ");
    std::string tagIdClause;
    if (!tagIds.empty()) {
        tagIdClause = "(" + BackupDatabaseUtils::JoinSQLValues<string>(tagIds, ", ") + ")";
    }
    // 删除 VisionFaceTag 表中的记录
    std::string deleteFaceTagSql = "DELETE FROM " + VISION_FACE_TAG_TABLE +
                                   " WHERE tag_id IN (SELECT A.tag_id FROM " + ANALYSIS_ALBUM_TABLE + " AS A, " +
                                   VISION_FACE_TAG_TABLE + " AS B WHERE A.tag_id = B.tag_id AND " +
                                   ANALYSIS_COL_ALBUM_NAME + " IN (" + inClause + "))";
    ExecuteSQL(mediaLibraryRdb, deleteFaceTagSql);

    std::string imageFaceClause = "tag_id IN (SELECT A.tag_id FROM " + ANALYSIS_ALBUM_TABLE + " AS A, " +
        VISION_IMAGE_FACE_TABLE + " AS B WHERE A.tag_id = B.tag_id AND " +
        ANALYSIS_COL_ALBUM_NAME + " IN (" + inClause + "))";

    std::unique_ptr<NativeRdb::AbsRdbPredicates> updatePredicates =
            make_unique<NativeRdb::AbsRdbPredicates>(VISION_IMAGE_FACE_TABLE);
    updatePredicates->SetWhereClause(imageFaceClause);
    int32_t deletedRows = 0;
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutString(FACE_TAG_COL_TAG_ID, std::string("-1"));

    int32_t ret = BackupDatabaseUtils::Update(mediaLibraryRdb, deletedRows, valuesBucket, updatePredicates);
    if (deletedRows < 0 || ret < 0) {
        MEDIA_ERR_LOG("Failed to update tag_id colum value");
        return false;
    }

    /* 删除 AnalysisAlbum 表中的记录 */
    std::string deleteAnalysisSql = "DELETE FROM " + ANALYSIS_ALBUM_TABLE +
                                    " WHERE " + ANALYSIS_COL_ALBUM_NAME + " IN (" + inClause + ")";
    if (!tagIds.empty()) {
        deleteAnalysisSql += " OR ";
        deleteAnalysisSql += "(" + ANALYSIS_COL_TAG_ID + " IN " + tagIdClause + ")";
    }
    ExecuteSQL(mediaLibraryRdb, deleteAnalysisSql);

    return true;
}

void BackupDatabaseUtils::ExecuteSQL(std::shared_ptr<NativeRdb::RdbStore> rdbStore, const std::string& sql)
{
    int ret = rdbStore->ExecuteSql(sql);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to execute SQL: %{public}s", sql.c_str());
    }
}

std::string BackupDatabaseUtils::GetFileIdNewFilterClause(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
    const std::vector<FileIdPair>& fileIdPair)
{
    std::vector<int32_t> result;
    auto [oldFileIds, newFileIds] = BackupDatabaseUtils::UnzipFileIdPairs(fileIdPair);
    std::string fileIdNewInClause = "(" + BackupDatabaseUtils::JoinValues<int>(newFileIds, ", ") + ")";
    std::string querySql = "SELECT " + IMAGE_FACE_COL_FILE_ID +
        " FROM " + VISION_IMAGE_FACE_TABLE +
        " WHERE " + IMAGE_FACE_COL_FILE_ID + " IN " + fileIdNewInClause;

    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaLibraryRdb, querySql);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Query resultSet is null.");
        return "()";
    }

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t value;
        int32_t columnIndex;
        int32_t err = resultSet->GetColumnIndex(IMAGE_FACE_COL_FILE_ID, columnIndex);
        if (err == E_OK) {
            resultSet->GetInt(columnIndex, value);
            result.emplace_back(value);
        }
    }

    std::vector<int32_t> newFileIdsToDelete;
    for (const auto& fileId : result) {
        auto it = std::find_if(fileIdPair.begin(), fileIdPair.end(),
            [fileId](const FileIdPair& pair) { return pair.second == fileId; });
        if (it != fileIdPair.end()) {
            newFileIdsToDelete.push_back(it->second);
        }
    }

    return "(" + BackupDatabaseUtils::JoinValues<int>(newFileIdsToDelete, ", ") + ")";
}

void BackupDatabaseUtils::DeleteExistingImageFaceData(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
    const std::vector<FileIdPair>& fileIdPair)
{
    std::string fileIdNewFilterClause = GetFileIdNewFilterClause(mediaLibraryRdb, fileIdPair);

    std::string deleteFaceSql = "DELETE FROM " + VISION_IMAGE_FACE_TABLE +
        " WHERE " + IMAGE_FACE_COL_FILE_ID + " IN " + fileIdNewFilterClause;
    BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb, deleteFaceSql);
}

void BackupDatabaseUtils::ParseFaceTagResultSet(const std::shared_ptr<NativeRdb::ResultSet>& resultSet,
    TagPairOpt& tagPair)
{
    tagPair.first = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, ANALYSIS_COL_TAG_ID);
    tagPair.second = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, ANALYSIS_COL_GROUP_TAG);
}

std::vector<TagPairOpt> BackupDatabaseUtils::QueryTagInfo(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb)
{
    std::vector<TagPairOpt> result;
    std::string querySql = "SELECT " + ANALYSIS_COL_TAG_ID + ", " +
        ANALYSIS_COL_GROUP_TAG +
        " FROM " + ANALYSIS_ALBUM_TABLE +
        " WHERE " + ANALYSIS_COL_TAG_ID + " IS NOT NULL AND " +
        ANALYSIS_COL_TAG_ID + " != ''";

    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaLibraryRdb, querySql);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG ("Query resultSet is null.");
        return result;
    }
    while (resultSet->GoToNextRow () == NativeRdb::E_OK) {
        TagPairOpt tagPair;
        ParseFaceTagResultSet(resultSet, tagPair);
        result.emplace_back(tagPair);
    }
    return result;
}

void BackupDatabaseUtils::UpdateGroupTagColumn(const std::vector<TagPairOpt>& updatedPairs,
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb)
{
    for (const auto& pair : updatedPairs) {
        if (pair.first.has_value() && pair.second.has_value()) {
            std::unique_ptr<NativeRdb::AbsRdbPredicates> predicates =
                std::make_unique<NativeRdb::AbsRdbPredicates>(ANALYSIS_ALBUM_TABLE);
            std::string whereClause = ANALYSIS_COL_TAG_ID + " = '" + pair.first.value() + "'";
            predicates->SetWhereClause(whereClause);

            int32_t updatedRows = 0;
            NativeRdb::ValuesBucket valuesBucket;
            valuesBucket.PutString(ANALYSIS_COL_GROUP_TAG, pair.second.value());

            int32_t ret = BackupDatabaseUtils::Update(mediaLibraryRdb, updatedRows, valuesBucket, predicates);
            if (updatedRows <= 0 || ret < 0) {
                MEDIA_ERR_LOG("Failed to update group_tag for tag_id: %s", pair.first.value().c_str());
            }
        }
    }
}

void BackupDatabaseUtils::UpdateFaceGroupTagsUnion(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb)
{
    std::vector<TagPairOpt> tagPairs = QueryTagInfo(mediaLibraryRdb);
    std::vector<TagPairOpt> updatedPairs;
    std::vector<std::string> allTagIds;
    for (const auto& pair : tagPairs) {
        if (pair.first.has_value()) {
            allTagIds.emplace_back(pair.first.value());
        }
    }
    MEDIA_INFO_LOG("get all TagId  %{public}zu", allTagIds.size());
    for (const auto& pair : tagPairs) {
        if (pair.second.has_value()) {
            std::vector<std::string> groupTags = BackupDatabaseUtils::SplitString(pair.second.value(), '|');
            MEDIA_INFO_LOG("TagId: %{public}s, old GroupTags is: %{public}s",
                           pair.first.value_or(std::string("-1")).c_str(), pair.second.value().c_str());
            groupTags.erase(std::remove_if(groupTags.begin(), groupTags.end(),
                [&allTagIds](const std::string& tagId) {
                return std::find(allTagIds.begin(), allTagIds.end(), tagId) == allTagIds.end();
                }),
                groupTags.end());

            std::string newGroupTag = BackupDatabaseUtils::JoinValues<std::string>(groupTags, "|");
            if (newGroupTag != pair.second.value()) {
                updatedPairs.emplace_back(pair.first, newGroupTag);
                MEDIA_INFO_LOG("TagId: %{public}s  GroupTags updated", pair.first.value().c_str());
            }
        }
    }

    UpdateGroupTagColumn(updatedPairs, mediaLibraryRdb);
}

void BackupDatabaseUtils::UpdateTagPairs(std::vector<TagPairOpt>& updatedPairs, const std::string& newGroupTag,
    const std::vector<std::string>& tagIds)
{
    for (const auto& tagId : tagIds) {
        updatedPairs.emplace_back(tagId, newGroupTag);
    }
}

void BackupDatabaseUtils::UpdateGroupTags(std::vector<TagPairOpt>& updatedPairs,
    const std::unordered_map<std::string, std::vector<std::string>>& groupTagMap)
{
    for (auto &[groupTag, tagIds] : groupTagMap) {
        if (tagIds.empty()) {
            continue;
        }

        const std::string newGroupTag =
            (tagIds.size() > 1) ? BackupDatabaseUtils::JoinValues(tagIds, "|") : tagIds.front();
        if (newGroupTag != groupTag) {
            UpdateTagPairs(updatedPairs, newGroupTag, tagIds);
        }
    }
}

    /* 双框架的group_id是合并相册之一的某一 tag_id */
void BackupDatabaseUtils::UpdateFaceGroupTagOfDualFrame(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb)
{
    std::vector<TagPairOpt> tagPairs = QueryTagInfo(mediaLibraryRdb);
    std::vector<TagPairOpt> updatedPairs;
    std::unordered_map<std::string, std::vector<std::string>> groupTagMap;

    for (const auto& pair : tagPairs) {
        if (pair.first.has_value() && pair.second.has_value()) {
            groupTagMap[pair.second.value()].push_back(pair.first.value());
        } else {
            MEDIA_INFO_LOG("Found tag_id without group_tag: %{public}s", pair.first.value().c_str());
        }
    }

    UpdateGroupTags(updatedPairs, groupTagMap);
    UpdateGroupTagColumn(updatedPairs, mediaLibraryRdb);
}

void BackupDatabaseUtils::UpdateAssociateFileId(std::shared_ptr<NativeRdb::RdbStore> rdbStore,
    const std::vector<FileInfo> &fileInfos)
{
    for (const FileInfo &fileInfo : fileInfos) {
        if (fileInfo.associateFileId <= 0 || fileInfo.fileIdOld <= 0 || fileInfo.fileIdNew <= 0) {
            continue;
        }
        int32_t updateAssociateId = -1;
        bool ret = fileIdOld2NewForCloudEnhancement.Find(fileInfo.associateFileId, updateAssociateId);
        if (!ret) {
            fileIdOld2NewForCloudEnhancement.Insert(fileInfo.fileIdOld, fileInfo.fileIdNew);
            continue;
        }
        int32_t changeRows = 0;
        NativeRdb::ValuesBucket updatePostBucket;
        updatePostBucket.Put(PhotoColumn::PHOTO_ASSOCIATE_FILE_ID, updateAssociateId);
        std::unique_ptr<NativeRdb::AbsRdbPredicates> predicates =
            make_unique<NativeRdb::AbsRdbPredicates>(PhotoColumn::PHOTOS_TABLE);
        predicates->SetWhereClause("file_id=?");
        predicates->SetWhereArgs({ to_string(fileInfo.fileIdNew) });
        BackupDatabaseUtils::Update(rdbStore, changeRows, updatePostBucket, predicates);
        if (changeRows > 0) {
            MEDIA_INFO_LOG("update, old:%{public}d, new:%{public}d, old_associate:%{public}d, new_associate:%{public}d",
                fileInfo.fileIdOld, fileInfo.fileIdNew, fileInfo.associateFileId, updateAssociateId);
        }

        NativeRdb::ValuesBucket updatePreBucket;
        updatePreBucket.Put(PhotoColumn::PHOTO_ASSOCIATE_FILE_ID, fileInfo.fileIdNew);
        predicates->SetWhereArgs({ to_string(updateAssociateId) });
        BackupDatabaseUtils::Update(rdbStore, changeRows, updatePreBucket, predicates);
        if (changeRows > 0) {
            MEDIA_INFO_LOG("update, old:%{public}d, new:%{public}d, new_associate:%{public}d",
                fileInfo.associateFileId, updateAssociateId, fileInfo.fileIdNew);
        }
        fileIdOld2NewForCloudEnhancement.Erase(fileInfo.associateFileId);
    }
}
} // namespace Media
} // namespace OHOS