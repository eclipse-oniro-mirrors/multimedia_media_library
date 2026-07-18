/*
 * Copyright (C) 2023-2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MediaLibraryBackupUtils"

#include "backup_database_utils.h"

#include <fcntl.h>
#include <nlohmann/json.hpp>
#include <safe_map.h>

#include "backup_const_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "parameters.h"
#include "vision_video_total_column.h"
#include "vision_image_face_column.h"
#include "vision_column_comm.h"
#include "media_string_utils.h"
#include "media_time_utils.h"
#include "media_uri_utils.h"
#include "backup_const.h"

namespace OHOS {
namespace Media {
const int32_t SCALE_FACTOR = 2;
const int32_t SCALE_MIN_SIZE = 1080;
const int32_t SCALE_MAX_SIZE = 2560;
const int32_t UPDATE_COUNT = 200;
const int32_t STAMP_PARAM = 4;
const int32_t INTEGRITY_CHECK_TABLE_COUNT = 2;
const float SCALE_DEFAULT = 0.25;
const size_t MIN_GARBLE_SIZE = 2;
const size_t GARBLE_START = 1;
const size_t XY_DIMENSION = 2;
const size_t BYTE_LEN = 4;
const size_t BYTE_BASE_OFFSET = 8;
const size_t LANDMARKS_SIZE = 5;
const std::string LANDMARK_X = "x";
const std::string LANDMARK_Y = "y";
const std::string COLUMN_INTEGRITY_CHECK = "quick_check";
const std::string SQL_QUOTES = "\"";
const std::string MEDIA_TYPE_VIDEO_VALUE = "2";
const std::string STATUS_DEFAULT_VALUE = "0";
const std::string STATUS_ANALYSIS_FINISH_VALUE = "1";
const std::string LABEL_DEFAULT_VALUE = "0";
const std::string FACE_DEFAULT_VALUE = "0";
const std::string FACE_DETECTION_VALUE = "1";
const std::string FACE_EXTRACT_FEATURE_VALUE = "2";
const std::string FACE_CLUSTERING_VALUE = "3";
const std::string FACE_CLUSTERING_FAIL_VALUE = "4";
const std::string TAGID_WAITING_FOR_ANALYSIS_VALUE = "-1";
const std::string TAGID_NO_NEED_TO_ANALYSIS_VALUE = "-2";
const std::string CONST_LOGSYSTEM_VERSIONTYPE = "const.logsystem.versiontype";
const std::string BETA_VERSIONTYPE = "beta";
const std::string PHOTOS_TABLE_NAME = "Photos";
const std::string GALLERY_MEDIA_TABLE_NAME = "gallery_media";
const std::string PHOTO_ALBUM_TABLE_NAME = "PhotoAlbum";
const std::string INTEGRITY_CHECK_OK = "ok";
const std::string INTEGRITY_CHECK_ERROR = "integrity error";

const std::vector<uint32_t> HEX_MAX = { 0xff, 0xffff, 0xffffff, 0xffffffff };
static SafeMap<int32_t, int32_t> fileIdOld2NewForCloudEnhancement;
const int32_t ARG_COUNT = 2;
const int32_t MAP_CODE_PARAM = 3;

static const std::unordered_map<int32_t, SouthDeviceType> INT_SOUTH_DEVICE_TYPE_MAP = {
    {static_cast<int32_t>(SouthDeviceType::SOUTH_DEVICE_NULL), SouthDeviceType::SOUTH_DEVICE_NULL},
    {static_cast<int32_t>(SouthDeviceType::SOUTH_DEVICE_CLOUD), SouthDeviceType::SOUTH_DEVICE_CLOUD},
    {static_cast<int32_t>(SouthDeviceType::SOUTH_DEVICE_HDC), SouthDeviceType::SOUTH_DEVICE_HDC},
};
// LCOV_EXCL_START
int32_t BackupDatabaseUtils::InitDb(std::shared_ptr<NativeRdb::RdbStore> &rdbStore, const std::string &dbName,
    const std::string &dbPath, const std::string &bundleName, bool isMediaLibrary, int32_t area, bool needSetSearchable)
{
    NativeRdb::RdbStoreConfig config(dbName);
    config.SetPath(dbPath);
    config.SetBundleName(bundleName);
    config.SetReadConSize(CONNECT_SIZE);
    config.SetSecurityLevel(NativeRdb::SecurityLevel::S3);
    config.SetHaMode(NativeRdb::HAMode::MANUAL_TRIGGER);
    config.SetAllowRebuild(true);
    config.SetWalLimitSize(WAL_LIMIT_SIZE);
    if (area != DEFAULT_AREA_VERSION) {
        config.SetArea(area);
    }
    if (isMediaLibrary) {
        config.SetScalarFunction("cloud_sync_func", 0, CloudSyncTriggerFunc);
        config.SetScalarFunction("is_caller_self_func", 0, IsCallerSelfFunc);
        config.SetScalarFunction("photo_album_notify_func", ARG_COUNT, PhotoAlbumNotifyFunc);
        config.SetScalarFunction("photo_map_code_func", MAP_CODE_PARAM, PhotoMapCodeFunc);
        config.SetScalarFunction("begin_generate_highlight_thumbnail", STAMP_PARAM, BeginGenerateHighlightThumbnail);
        bool enableOldPath =
            system::GetBoolParameter("persist.multimedia.media_analysis_service.search_oldpath_enable", true);
        if (needSetSearchable && !enableOldPath) {
            config.SetSearchable(true);
        }
    }
    int32_t err;
    RdbCallback cb;
    rdbStore = NativeRdb::RdbHelper::GetRdbStore(config, MEDIA_RDB_VERSION, cb, err);
    return err;
}

int32_t BackupDatabaseUtils::InitDbForOldVersion(std::shared_ptr<NativeRdb::RdbStore> &rdbStore,
    const std::string &dbName, const std::string &dbPath, const std::string &bundleName,
    bool isMediaLibrary, int32_t& oldVersion, int32_t area)
{
    NativeRdb::RdbStoreConfig config(dbName);
    config.SetPath(dbPath);
    config.SetCustomDir("../../../../../../../storage/media/local/files/.backup/restore/data/storage/el2/database/rdb");
    config.SetBundleName(bundleName);
    config.SetReadConSize(CONNECT_SIZE);
    config.SetSecurityLevel(NativeRdb::SecurityLevel::S3);
    config.SetHaMode(NativeRdb::HAMode::MANUAL_TRIGGER);
    config.SetAllowRebuild(true);
    config.SetWalLimitSize(WAL_LIMIT_SIZE);
    if (area != DEFAULT_AREA_VERSION) {
        config.SetArea(area);
    }
    if (isMediaLibrary) {
        config.SetScalarFunction("cloud_sync_func", 0, CloudSyncTriggerFunc);
        config.SetScalarFunction("is_caller_self_func", 0, IsCallerSelfFunc);
        config.SetScalarFunction("photo_album_notify_func", ARG_COUNT, PhotoAlbumNotifyFunc);
        config.SetScalarFunction("begin_generate_highlight_thumbnail", STAMP_PARAM, BeginGenerateHighlightThumbnail);
    }
    int32_t err;
    RdbCallback cb;
    rdbStore = NativeRdb::RdbHelper::GetRdbStore(config, MEDIA_RDB_VERSION, cb, err);
    oldVersion = cb.oldVersion_;
    MEDIA_INFO_LOG("InitDbForOldVersion oldVersion is %{public}d", oldVersion);
    return err;
}

int32_t BackupDatabaseUtils::InitReadOnlyRdb(std::shared_ptr<NativeRdb::RdbStore> &rdbStore,
    const std::string &dbName, const std::string &dbPath, const std::string &bundleName)
{
    NativeRdb::RdbStoreConfig config(dbName);
    config.SetPath(dbPath);
    config.SetBundleName(bundleName);
    config.SetReadConSize(CONNECT_SIZE);
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

std::string BackupDatabaseUtils::PhotoAlbumNotifyFunc(const std::vector<std::string> &args)
{
    return "";
}

std::string BackupDatabaseUtils::PhotoMapCodeFunc(const std::vector<std::string> &args)
{
    return "";
}

std::string BackupDatabaseUtils::BeginGenerateHighlightThumbnail(const std::vector<std::string> &args)
{
    return "";
}

static int32_t ExecSqlWithRetry(std::function<int32_t()> execSql)
{
    int32_t currentTime = 0;
    int32_t err = NativeRdb::E_OK;
    while (currentTime <= MAX_TRY_TIMES) {
        err = execSql();
        if (err == NativeRdb::E_OK) {
            break;
        } else if (err == NativeRdb::E_SQLITE_LOCKED || err == NativeRdb::E_DATABASE_BUSY ||
            err == NativeRdb::E_SQLITE_BUSY) {
            std::this_thread::sleep_for(std::chrono::milliseconds(TRANSACTION_WAIT_INTERVAL));
            currentTime++;
            MEDIA_ERR_LOG("execSql busy, err: %{public}d, currentTime: %{public}d", err, currentTime);
        } else {
            MEDIA_ERR_LOG("execSql failed, err: %{public}d, currentTime: %{public}d", err, currentTime);
            break;
        }
    }
    return err;
}

int32_t BackupDatabaseUtils::QueryInt(std::shared_ptr<NativeRdb::RdbStore> rdbStore, const std::string &sql,
    const std::string &column, const std::vector<NativeRdb::ValueObject> &args)
{
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdb_ is nullptr, Maybe init failed.");
        return 0;
    }
    auto resultSet = rdbStore->QuerySql(sql, args);
    if (resultSet == nullptr) {
        return 0;
    }
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return 0;
    }
    int32_t result = GetInt32Val(column, resultSet);
    resultSet->Close();
    return result;
}

int32_t BackupDatabaseUtils::Update(std::shared_ptr<NativeRdb::RdbStore> &rdbStore, int32_t &changeRows,
    NativeRdb::ValuesBucket &valuesBucket, std::unique_ptr<NativeRdb::AbsRdbPredicates> &predicates)
{
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdb_ is nullptr, Maybe init failed.");
        return E_FAIL;
    }
    return ExecSqlWithRetry([&]() { return rdbStore->Update(changeRows, valuesBucket, *predicates); });
}

int32_t BackupDatabaseUtils::Delete(NativeRdb::AbsRdbPredicates &predicates, int32_t &changeRows,
    std::shared_ptr<NativeRdb::RdbStore> &rdbStore)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_FAIL, "rdb is nullptr");
    return ExecSqlWithRetry([&]() { return rdbStore->Delete(changeRows, predicates); });
}

int32_t BackupDatabaseUtils::InitGarbageAlbum(std::shared_ptr<NativeRdb::RdbStore> galleryRdb,
    std::set<std::string> &cacheSet, std::unordered_map<std::string, std::string> &nickMap)
{
    CHECK_AND_RETURN_RET_LOG(galleryRdb != nullptr, E_FAIL, "Pointer rdb_ is nullptr, Maybe init failed.");
    const string querySql = "SELECT nick_dir, nick_name FROM garbage_album where type = 0";
    auto resultSet = galleryRdb->QuerySql(QUERY_GARBAGE_ALBUM);
    CHECK_AND_RETURN_RET(resultSet != nullptr, E_HAS_DB_ERROR);
    int32_t count = -1;
    int32_t err = resultSet->GetRowCount(count);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, E_FAIL, "Failed to get count, err: %{public}d", err);
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

void BackupDatabaseUtils::QueryGalleryDuplicateDataCount(std::shared_ptr<NativeRdb::RdbStore> galleryRdb,
    int32_t &count, int32_t &total)
{
    static string QUERY_GALLERY_DUPLICATE_DATA_COUNT = "SELECT count(DISTINCT _data) as count, count(1) as total"
        " FROM gallery_media WHERE _data IN (SELECT _data FROM gallery_media GROUP BY _data HAVING count(1) > 1)";
    auto resultSet = GetQueryResultSet(galleryRdb, QUERY_GALLERY_DUPLICATE_DATA_COUNT);
    bool cond = (resultSet == nullptr);
    CHECK_AND_RETURN(!cond);
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return;
    }
    count = GetInt32Val("count", resultSet);
    total = GetInt32Val("total", resultSet);
    resultSet->Close();
}

std::shared_ptr<NativeRdb::ResultSet> BackupDatabaseUtils::GetQueryResultSet(
    const std::shared_ptr<NativeRdb::RdbStore> &rdbStore, const std::string &querySql,
    const std::vector<std::string> &sqlArgs)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, nullptr, "rdbStore is nullptr");
    return rdbStore->QuerySql(querySql, sqlArgs);
}

std::unordered_map<std::string, std::string> BackupDatabaseUtils::GetColumnInfoMap(
    const std::shared_ptr<NativeRdb::RdbStore> &rdbStore, const std::string &tableName)
{
    std::unordered_map<std::string, std::string> columnInfoMap;
    std::string querySql = "SELECT name, type FROM pragma_table_info('" + tableName + "')";
    auto resultSet = GetQueryResultSet(rdbStore, querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, columnInfoMap, "resultSet is nullptr");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string columnName = GetStringVal(PRAGMA_TABLE_NAME, resultSet);
        std::string columnType = GetStringVal(PRAGMA_TABLE_TYPE, resultSet);
        if (columnName.empty() || columnType.empty()) {
            MEDIA_ERR_LOG("Empty column name or type: %{public}s, %{public}s", columnName.c_str(), columnType.c_str());
            continue;
        }
        columnInfoMap[columnName] = columnType;
    }
    resultSet->Close();
    return columnInfoMap;
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

void BackupDatabaseUtils::UpdateUniqueNumber(const std::shared_ptr<NativeRdb::RdbStore> &rdbStore, int32_t number,
    const std::string &type)
{
    const string updateSql =
        "UPDATE UniqueNumber SET unique_number = " + to_string(number) + " WHERE media_type = '" + type + "'";
    int32_t erroCode = BackupDatabaseUtils::ExecuteSQL(rdbStore, updateSql);
    CHECK_AND_PRINT_LOG(erroCode >= 0, "execute update unique number failed, ret=%{public}d", erroCode);
}

int32_t BackupDatabaseUtils::QueryUniqueNumber(const std::shared_ptr<NativeRdb::RdbStore> &rdbStore,
    const std::string &type)
{
    const string querySql = "SELECT unique_number FROM UniqueNumber WHERE media_type = '" + type + "'";
    return QueryInt(rdbStore, querySql, UNIQUE_NUMBER);
}

void BackupDatabaseUtils::UpdateSelection(std::string &selection, const std::string &selectionToAdd, bool needWrap)
{
    CHECK_AND_RETURN(!selectionToAdd.empty());
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

bool BackupDatabaseUtils::QueryThumbImage(NativeRdb::RdbStore &rdbStore,
    const std::string &keyValue, std::vector<uint8_t> &blob)
{
    std::string query = "SELECT v FROM general_kv where k = " + SQL_QUOTES + keyValue + SQL_QUOTES +";";
    auto resultSet = rdbStore.QueryByStep(query);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "Failed to QueryByStep");
    int32_t count = -1;
    int err = resultSet->GetRowCount(count);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, false,
        "Failed to get count, err: %{public}d, %{public}s", err, query.c_str());
    if (count != 1) {
        MEDIA_ERR_LOG("Failed to get count: %{public}d,", count);
        resultSet->Close();
        return false;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        resultSet->GetBlob(0, blob);
    }
    resultSet->Close();
    return true;
}

int32_t BackupDatabaseUtils::GetBlob(const std::string &columnName, std::shared_ptr<NativeRdb::ResultSet> resultSet,
    std::vector<uint8_t> &blobVal)
{
    int32_t columnIndex = 0;
    int32_t errCode = resultSet->GetColumnIndex(columnName, columnIndex);
    CHECK_AND_RETURN_RET_LOG(!errCode, E_FAIL, "Get column index errCode: %{public}d", errCode);
    CHECK_AND_RETURN_RET(resultSet->GetBlob(columnIndex, blobVal) == NativeRdb::E_OK, E_FAIL);
    return E_OK;
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
    int32_t errCode = BackupDatabaseUtils::ExecuteSQL(rdbStore, updateSql);
    CHECK_AND_PRINT_LOG(errCode >= 0, "execute update analysis total failed, ret=%{public}d", errCode);
}

void BackupDatabaseUtils::UpdateAnalysisFaceTagStatus(std::shared_ptr<NativeRdb::RdbStore> rdbStore)
{
    std::string updateSql = "UPDATE tab_analysis_face_tag SET count = (SELECT count(1) from tab_analysis_image_face \
        WHERE tab_analysis_image_face.tag_id = tab_analysis_face_tag.tag_id \
        AND tab_analysis_image_face.tag_id LIKE 'ser%')";
    int32_t errCode = BackupDatabaseUtils::ExecuteSQL(rdbStore, updateSql);
    if (errCode < 0) {
        MEDIA_ERR_LOG("execute update analysis face tag count failed, ret=%{public}d", errCode);
    }
}

void BackupDatabaseUtils::UpdateAnalysisTotalTblStatus(std::shared_ptr<NativeRdb::RdbStore> rdbStore,
    const std::vector<FileIdPair>& fileIdPair)
{
    auto [oldFileIds, newFileIds] = BackupDatabaseUtils::UnzipFileIdPairs(fileIdPair);
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

    int32_t errCode = BackupDatabaseUtils::ExecuteSQL(rdbStore, updateSql);
    CHECK_AND_PRINT_LOG(errCode >= 0, "execute update analysis total failed, ret=%{public}d", errCode);
}

void BackupDatabaseUtils::UpdateFaceAnalysisTblStatus(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb)
{
    BackupDatabaseUtils::UpdateAnalysisFaceTagStatus(mediaLibraryRdb);
}

bool BackupDatabaseUtils::SetTagIdNew(PortraitAlbumInfo &portraitAlbumInfo,
    std::unordered_map<std::string, std::string> &tagIdMap)
{
    portraitAlbumInfo.tagIdNew = portraitAlbumInfo.tagIdOld;
    tagIdMap[portraitAlbumInfo.tagIdOld] = portraitAlbumInfo.tagIdNew;
    return true;
}

bool BackupDatabaseUtils::SetFileIdNew(FaceInfo &faceInfo, const std::unordered_map<std::string, FileInfo> &fileInfoMap)
{
    bool cond = (faceInfo.hash.empty() || fileInfoMap.count(faceInfo.hash) == 0);
    CHECK_AND_RETURN_RET_LOG(!cond, false,
        "Set new file_id for face %{public}s failed, no such file hash", faceInfo.faceId.c_str());
    faceInfo.fileIdNew = fileInfoMap.at(faceInfo.hash).fileIdNew;
    CHECK_AND_RETURN_RET_LOG(faceInfo.fileIdNew > 0, false,
        "Set new file_id for face %{public}s failed, file_id %{public}d <= 0", faceInfo.faceId.c_str(),
        faceInfo.fileIdNew);
    return true;
}

bool BackupDatabaseUtils::SetTagIdNew(FaceInfo &faceInfo, const std::unordered_map<std::string, std::string> &tagIdMap)
{
    CHECK_AND_RETURN_RET_LOG(!faceInfo.tagIdOld.empty(), false,
        "Set new tag_id for face %{public}s failed, empty tag_id", faceInfo.faceId.c_str());
    if (tagIdMap.count(faceInfo.tagIdOld) == 0) {
        faceInfo.tagIdNew = TAG_ID_UNPROCESSED;
        return true;
    }
    faceInfo.tagIdNew = tagIdMap.at(faceInfo.tagIdOld);
    bool cond = (faceInfo.tagIdNew.empty() || !MediaStringUtils::StartsWith(faceInfo.tagIdNew, TAG_ID_PREFIX));
    CHECK_AND_RETURN_RET_LOG(!cond, false,
        "Set new tag_id for face %{public}s failed, new tag_id %{public}s empty or invalid",
        faceInfo.tagIdNew.c_str(), faceInfo.faceId.c_str());
    return true;
}

bool BackupDatabaseUtils::SetAlbumIdNew(FaceInfo &faceInfo, const std::unordered_map<std::string, int32_t> &albumIdMap)
{
    CHECK_AND_RETURN_RET(faceInfo.tagIdNew != TAG_ID_UNPROCESSED, true);
    CHECK_AND_RETURN_RET_LOG(albumIdMap.count(faceInfo.tagIdNew) != 0, false,
        "Set new album_id for face %{public}s failed, no such tag_id", faceInfo.faceId.c_str());

    faceInfo.albumIdNew = albumIdMap.at(faceInfo.tagIdNew);
    CHECK_AND_RETURN_RET_LOG(faceInfo.albumIdNew > 0, false,
        "Set new album_id for face %{public}s failed, album_id %{public}d <= 0", faceInfo.faceId.c_str(),
        faceInfo.albumIdNew);
    return true;
}

void BackupDatabaseUtils::PrintErrorLog(const std::string &errorLog, int64_t start)
{
    int64_t end = MediaTimeUtils::UTCTimeMilliSeconds();
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
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, columnInfoPairs, "resultSet is nullptr");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string columnName = GetStringVal(PRAGMA_TABLE_NAME, resultSet);
        std::string columnType = GetStringVal(PRAGMA_TABLE_TYPE, resultSet);
        if (columnName.empty() || columnType.empty()) {
            MEDIA_ERR_LOG("Empty column name or type: %{public}s, %{public}s", columnName.c_str(), columnType.c_str());
            continue;
        }
        columnInfoPairs.emplace_back(columnName, columnType);
    }

    resultSet->Close();
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

std::vector<std::string> BackupDatabaseUtils::FilterExcludedColumns(const std::vector<std::string>& allColumns,
    const std::vector<std::string>& excludedColumns)
{
    std::vector<std::string> filteredColumns;
    std::copy_if(allColumns.begin(), allColumns.end(), std::back_inserter(filteredColumns),
        [&excludedColumns](const std::string& column) {
            return std::find(excludedColumns.begin(), excludedColumns.end(), column) == excludedColumns.end();
        });
    return filteredColumns;
}

std::vector<std::string> BackupDatabaseUtils::FilterIncludedColumns(const std::vector<std::string>& allColumns,
    const std::vector<std::string>& includedColumns)
{
    std::vector<std::string> filteredColumns;
    std::copy_if(allColumns.begin(), allColumns.end(), std::back_inserter(filteredColumns),
        [&includedColumns](const std::string& column) {
            return std::find(includedColumns.begin(), includedColumns.end(), column) != includedColumns.end();
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

    int32_t ret = BackupDatabaseUtils::ExecuteSQL(rdbStore, insertSql);
    CHECK_AND_PRINT_LOG(ret >= 0, "execute update AnalysisPhotoMap failed, ret=%{public}d", ret);
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

    return {oldFileIds, newFileIds};
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

int64_t BackupDatabaseUtils::QueryLong(std::shared_ptr<NativeRdb::RdbStore> rdbStore, const std::string &sql,
    const std::string &columnName, const std::vector<NativeRdb::ValueObject> &args)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, 0, "RdbStore is null.");
    auto resultSet = rdbStore->QuerySql(sql, args);
    CHECK_AND_RETURN_RET(resultSet != nullptr, 0);
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return 0;
    }

    int64_t resultValue = GetInt64Val(columnName, resultSet);
    resultSet->Close();
    return resultValue;
}

int64_t BackupDatabaseUtils::QueryMaxId(std::shared_ptr<NativeRdb::RdbStore> rdbStore,
    const std::string& tableName, const std::string& idColumnName)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, 0, "RdbStore is null.");

    std::string querySql = "SELECT MAX(" + idColumnName + ") AS max_id FROM " + tableName;
    int64_t maxId = BackupDatabaseUtils::QueryLong(rdbStore, querySql, "max_id");
    MEDIA_INFO_LOG("QueryMaxId on table '%{public}s' column '%{public}s' return %{public}" PRId64,
        tableName.c_str(), idColumnName.c_str(), maxId);

    return maxId;
}

static bool DeleteDuplicateVisionFaceTags(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
    const std::string& selectTagIdsToDeleteSql)
{
    CHECK_AND_RETURN_RET_LOG(!selectTagIdsToDeleteSql.empty(), true, "No tag IDs to delete.");

    std::string deleteFaceTagSql = "DELETE FROM " + VISION_FACE_TAG_TABLE +
        " WHERE " + ANALYSIS_COL_TAG_ID + " IN (" + selectTagIdsToDeleteSql + ")";

    BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb, deleteFaceTagSql);
    return true;
}

static bool UpdateVisionTotalFaceStatus(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
    const std::vector<std::string>& affectedFileIds)
{
    CHECK_AND_RETURN_RET_LOG(mediaLibraryRdb != nullptr, false, "RdbStore is null.");

    auto totalPredicates = std::make_unique<NativeRdb::AbsRdbPredicates>(VISION_TOTAL_TABLE);
    totalPredicates->In(IMAGE_FACE_COL_FILE_ID, affectedFileIds);

    NativeRdb::ValuesBucket values;
    values.PutInt("face", TOTAL_TBL_FACE_ANALYSED);
    int32_t updatedRows = 0;
    int32_t ret = BackupDatabaseUtils::Update(mediaLibraryRdb, updatedRows, values, totalPredicates);
    if (ret < 0 || updatedRows < 0) {
        MEDIA_ERR_LOG("Update failed on VISION_TOTAL_TABLE, ret:%{public}d", ret);
        return false;
    }

    return true;
}

static bool UpdateDuplicateVisionImageFaces(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
    const std::string& selectTagIdsToDeleteSql)
{
    CHECK_AND_RETURN_RET_LOG(mediaLibraryRdb != nullptr, false, "RdbStore is null.");
    CHECK_AND_RETURN_RET_LOG(!selectTagIdsToDeleteSql.empty(), true, "No tag IDs to update.");

    std::string selectFileIdsSql = "SELECT DISTINCT " + IMAGE_FACE_COL_FILE_ID + " FROM " + VISION_IMAGE_FACE_TABLE +
                                   " WHERE " + FACE_TAG_COL_TAG_ID + " IN (" + selectTagIdsToDeleteSql + ")";

    std::vector<std::string> affectedFileIds;
    std::shared_ptr<NativeRdb::ResultSet> resultSet = mediaLibraryRdb->QuerySql(selectFileIdsSql);
    if (resultSet != nullptr) {
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            std::string fileId;
            int32_t columnIndex;
            if (resultSet->GetColumnIndex(IMAGE_FACE_COL_FILE_ID, columnIndex) == NativeRdb::E_OK) {
                resultSet->GetString(columnIndex, fileId);
                affectedFileIds.push_back(fileId);
            }
        }
    }

    std::string imageFaceUpdateWhereClause = FACE_TAG_COL_TAG_ID + " IN (" + selectTagIdsToDeleteSql + ")";
    std::unique_ptr<NativeRdb::AbsRdbPredicates> imageFacePredicates =
        std::make_unique<NativeRdb::AbsRdbPredicates>(VISION_IMAGE_FACE_TABLE);
    imageFacePredicates->SetWhereClause(imageFaceUpdateWhereClause);

    NativeRdb::ValuesBucket imageFaceValues;
    imageFaceValues.PutString(FACE_TAG_COL_TAG_ID, "-1");

    int32_t imageFaceUpdatedRows = 0;
    int32_t ret = BackupDatabaseUtils::Update(mediaLibraryRdb,
        imageFaceUpdatedRows, imageFaceValues, imageFacePredicates);
    bool imageFaceUpdateFailed = (imageFaceUpdatedRows < 0 || ret < 0);
    CHECK_AND_RETURN_RET_LOG(!imageFaceUpdateFailed, false, "Failed to update VISION_IMAGE_FACE_TABLE");

    if (!affectedFileIds.empty()) {
        if (!UpdateVisionTotalFaceStatus(mediaLibraryRdb, affectedFileIds)) {
            MEDIA_ERR_LOG("VISION_TOTAL_TABLE update failed");
            return false;
        }
    }

    return true;
}

static bool DeleteDuplicateAnalysisAlbums(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
    const std::string& finalWhereClause)
{
    CHECK_AND_RETURN_RET_LOG(!finalWhereClause.empty(), false, "finalWhereClause is empty, cannot delete.");

    std::string deleteAnalysisSql = "DELETE FROM " + ANALYSIS_ALBUM_TABLE +
        " WHERE " + finalWhereClause;

    BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb, deleteAnalysisSql);
    return true;
}

bool BackupDatabaseUtils::DeleteDuplicatePortraitAlbum(int64_t maxAlbumId, const std::vector<std::string> &albumNames,
    const std::vector<std::string> &tagIds, std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb)
{
    std::set<std::string> uniqueAlbums(albumNames.begin(), albumNames.end());
    std::vector<std::string> uniqueAlbumNames(uniqueAlbums.begin(), uniqueAlbums.end());
    MEDIA_INFO_LOG("DeleteDuplicatePortraitAlbum: Unique names %{public}zu", uniqueAlbumNames.size());
    std::string albumNameInClause;
    if (!uniqueAlbumNames.empty()) {
        albumNameInClause = ANALYSIS_COL_ALBUM_NAME + " IN (" +
            BackupDatabaseUtils::JoinSQLValues<string>(uniqueAlbumNames, ", ") + ")";
    }
    std::string tagIdInClause;
    if (!tagIds.empty()) {
        tagIdInClause = ANALYSIS_COL_TAG_ID + " IN (" +
            BackupDatabaseUtils::JoinSQLValues<string>(tagIds, ", ") + ")";
    }

    std::string analysisAlbumWhereClause;
    if (!albumNameInClause.empty() && !tagIdInClause.empty()) {
        analysisAlbumWhereClause = "(" + albumNameInClause + " OR " + tagIdInClause + ")";
    } else {
        analysisAlbumWhereClause = !albumNameInClause.empty() ? albumNameInClause : tagIdInClause;
    }
    if (analysisAlbumWhereClause.empty()) {
        MEDIA_INFO_LOG("DeleteDuplicatePortraitAlbum: Effective criteria empty.");
        return true;
    }

    std::string albumIdCondition = ANALYSIS_COL_ALBUM_ID + " <= " + std::to_string(maxAlbumId);
    std::string finalWhereClause = albumIdCondition + " AND (" + analysisAlbumWhereClause + ")";
    finalWhereClause += " AND (album_type = 4096 AND album_subtype = 4102)";

    std::string selectTagIdsToDeleteSql = "SELECT A." + ANALYSIS_COL_TAG_ID +
        " FROM " + ANALYSIS_ALBUM_TABLE + " AS A " + " WHERE " + finalWhereClause;

    bool success = true;
    success &= DeleteDuplicateVisionFaceTags(mediaLibraryRdb, selectTagIdsToDeleteSql);
    if (!success) {
        MEDIA_ERR_LOG("Failed during DeleteDuplicateVisionFaceTags step.");
    }

    success &= UpdateDuplicateVisionImageFaces(mediaLibraryRdb, selectTagIdsToDeleteSql);
    if (!success) {
        MEDIA_ERR_LOG("Failed during UpdateDuplicateVisionImageFaces step.");
    }

    success &= DeleteDuplicateAnalysisAlbums(mediaLibraryRdb, finalWhereClause);
    if (!success) {
        MEDIA_ERR_LOG("Failed during DeleteDuplicateAnalysisAlbums step.");
    }
    return success;
}

int BackupDatabaseUtils::ExecuteSQL(std::shared_ptr<NativeRdb::RdbStore> rdbStore, const std::string& sql,
    const std::vector<NativeRdb::ValueObject> &args)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_FAIL, "rdbStore is nullptr");
    return ExecSqlWithRetry([&]() { return rdbStore->ExecuteSql(sql, args); });
}

int32_t BackupDatabaseUtils::BatchInsert(std::shared_ptr<NativeRdb::RdbStore> rdbStore,
    const std::string &tableName, std::vector<NativeRdb::ValuesBucket> &value, int64_t &rowNum)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_FAIL, "rdbStore is nullptr");
    return ExecSqlWithRetry([&]() { return rdbStore->BatchInsert(rowNum, tableName, value); });
}

int32_t BackupDatabaseUtils::BatchInsert(std::shared_ptr<NativeRdb::RdbStore> rdbStore,
    const std::string &tableName, std::vector<NativeRdb::ValuesBucket> &value, int64_t &rowNum,
    NativeRdb::ConflictResolution conflictResolution)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_FAIL, "rdbStore is nullptr");
    return ExecSqlWithRetry([&]() {
        auto [errCode, insertedNum] = rdbStore->BatchInsert(tableName, NativeRdb::ValuesBuckets(value),
            conflictResolution);
        rowNum = insertedNum;
        return errCode;
    });
}

void BackupDatabaseUtils::DeleteExistingImageFaceData(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
    const std::vector<FileIdPair>& fileIdPair)
{
    auto [oldFileIds, newFileIds] = BackupDatabaseUtils::UnzipFileIdPairs(fileIdPair);
    std::vector<int> realNewFileIds;
    for (auto fileId: newFileIds) {
        CHECK_AND_EXECUTE(fileId == -1, realNewFileIds.emplace_back(fileId));
    }

    std::string fileIdNewFilterClause = "(" + BackupDatabaseUtils::JoinValues<int>(realNewFileIds, ", ") + ")";

    std::string deleteAnalysisPhotoMapSql =
        "DELETE FROM AnalysisPhotoMap WHERE "
        "map_album IN (SELECT album_id FROM AnalysisAlbum WHERE album_type = 4096 AND album_subtype = 4102) "
        "AND map_asset IN ("
        "SELECT " + IMAGE_FACE_COL_FILE_ID + " FROM " + VISION_IMAGE_FACE_TABLE +
        " WHERE " + IMAGE_FACE_COL_FILE_ID + " IN " + fileIdNewFilterClause +
        ") ";
    BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb, deleteAnalysisPhotoMapSql);

    std::string deleteFaceSql = "DELETE FROM " + VISION_IMAGE_FACE_TABLE +
        " WHERE " + IMAGE_FACE_COL_FILE_ID + " IN " + fileIdNewFilterClause;
    BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb, deleteFaceSql);

    const std::string updateTotalSql =
        "UPDATE " + VISION_TOTAL_TABLE +
        " SET face = 0, status = CASE WHEN status =1 THEN 0 ELSE status END " +
        " WHERE " + IMAGE_FACE_COL_FILE_ID + " IN " + fileIdNewFilterClause;
    int32_t totalRet = BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb, updateTotalSql);

    CHECK_AND_RETURN_LOG(totalRet >= 0, "Failed to update VISION_TOTAL_TABLE face field to 0");
}

void BackupDatabaseUtils::ParseFaceTagResultSet(const std::shared_ptr<NativeRdb::ResultSet>& resultSet,
    TagPairOpt& tagPair)
{
    tagPair.first = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, ANALYSIS_COL_TAG_ID);
    tagPair.second = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, ANALYSIS_COL_GROUP_TAG);
}

void BackupDatabaseUtils::ParseAlbumTagResultSet(const std::shared_ptr<NativeRdb::ResultSet>& resultSet,
    AlbumTagInfo& albumTag)
{
    albumTag.tagPair.first = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, ANALYSIS_COL_TAG_ID);
    albumTag.tagPair.second = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, ANALYSIS_COL_GROUP_TAG);
    albumTag.albumId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, ANALYSIS_COL_ALBUM_ID);
    albumTag.albumSubtype = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, ANALYSIS_COL_ALBUM_SUBTYPE);
}

std::vector<TagPairOpt> BackupDatabaseUtils::QueryTagInfo(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb)
{
    std::vector<TagPairOpt> result;
    std::string querySql = "SELECT " + ANALYSIS_COL_TAG_ID + ", " + ANALYSIS_COL_GROUP_TAG +
        " FROM " + ANALYSIS_ALBUM_TABLE +
        " WHERE " + ANALYSIS_COL_TAG_ID + " IS NOT NULL AND " +
        ANALYSIS_COL_TAG_ID + " != '' AND " + ANALYSIS_COL_ALBUM_SUBTYPE + " != " +
        std::to_string(PhotoAlbumSubType::GROUP_PHOTO);

    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaLibraryRdb, querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, result, "Query resultSet is null.");

    while (resultSet->GoToNextRow () == NativeRdb::E_OK) {
        TagPairOpt tagPair;
        ParseFaceTagResultSet(resultSet, tagPair);
        result.emplace_back(tagPair);
    }
    resultSet->Close();
    return result;
}

std::vector<AlbumTagInfo> BackupDatabaseUtils::QueryAlbumTagInfo(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb)
{
    std::vector<AlbumTagInfo> result;
    std::string querySql = "SELECT " + ANALYSIS_COL_ALBUM_ID + ", " + ANALYSIS_COL_TAG_ID + ", " +
        ANALYSIS_COL_GROUP_TAG + ", " + ANALYSIS_COL_ALBUM_SUBTYPE + " FROM " + ANALYSIS_ALBUM_TABLE + " WHERE " +
        ANALYSIS_COL_TAG_ID + " IS NOT NULL AND " + ANALYSIS_COL_TAG_ID + " != ''";

    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaLibraryRdb, querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, result, "Query resultSet is null.");

    while (resultSet->GoToNextRow () == NativeRdb::E_OK) {
        AlbumTagInfo albumTagInfo;
        ParseAlbumTagResultSet(resultSet, albumTagInfo);
        result.emplace_back(albumTagInfo);
    }
    resultSet->Close();
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
            bool cond = (updatedRows <= 0 || ret < 0);
            CHECK_AND_PRINT_LOG(!cond, "Failed to update group_tag for tag_id: %s", pair.first.value().c_str());
        }
    }
}

void BackupDatabaseUtils::UpdateTagsColumn(const std::vector<AlbumTagInfo> &UpdateAlbumTagInfo,
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb)
{
    for (const auto& info : UpdateAlbumTagInfo) {
        CHECK_AND_CONTINUE(info.tagPair.first.has_value() && info.tagPair.second.has_value() &&
            info.albumId.has_value());
        std::unique_ptr<NativeRdb::AbsRdbPredicates> predicates =
            std::make_unique<NativeRdb::AbsRdbPredicates>(ANALYSIS_ALBUM_TABLE);
        std::string whereClause = ANALYSIS_COL_ALBUM_ID + " = " + std::to_string(info.albumId.value());
        predicates->SetWhereClause(whereClause);

        int32_t updatedRows = 0;
        NativeRdb::ValuesBucket valuesBucket;
        valuesBucket.PutString(ANALYSIS_COL_GROUP_TAG, info.tagPair.second.value());
        valuesBucket.PutString(ANALYSIS_COL_TAG_ID, info.tagPair.first.value());

        int32_t ret = BackupDatabaseUtils::Update(mediaLibraryRdb, updatedRows, valuesBucket, predicates);
        bool cond = (updatedRows <= 0 || ret < 0);
        CHECK_AND_PRINT_LOG(!cond, "Failed to update group_tag or tag_id. tag_id: %s",
            info.tagPair.first.value().c_str());
    }
}

std::string RemoveNonExistentTags(const std::vector<std::string> allTagIds, const std::string originalTag)
{
    std::vector<std::string> newTagsBlock = {};
    std::vector<std::string> tagsBlock = BackupDatabaseUtils::SplitString(originalTag, ',');
    for (const auto& block : tagsBlock) {
        std::vector<std::string> TagUnit = BackupDatabaseUtils::SplitString(block, '|');
        TagUnit.erase(std::remove_if(TagUnit.begin(), TagUnit.end(),
            [&allTagIds](const std::string& tagId) {
            return std::find(allTagIds.begin(), allTagIds.end(), tagId) == allTagIds.end();
            }),
            TagUnit.end());

        std::string newTagBlock = BackupDatabaseUtils::JoinValues<std::string>(TagUnit, "|");
        newTagsBlock.push_back(newTagBlock);
    }
    std::string newTag = BackupDatabaseUtils::JoinValues<std::string>(newTagsBlock, ",");
    if (!originalTag.empty() && originalTag.back() == ',') {
        newTag = newTag + ',';
    }
    return newTag;
}

void BackupDatabaseUtils::UpdateFaceTagsUnion(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb)
{
    std::vector<AlbumTagInfo> albumTagInfos = QueryAlbumTagInfo(mediaLibraryRdb);
    std::vector<AlbumTagInfo> updateAlbumTagInfo;
    std::vector<std::string> allTagIds;
    for (const auto& info : albumTagInfos) {
        CHECK_AND_EXECUTE(!(info.tagPair.first.has_value() && (info.albumSubtype == PhotoAlbumSubType::PORTRAIT)),
            allTagIds.emplace_back(info.tagPair.first.value()));
    }
    MEDIA_INFO_LOG("get all TagId  %{public}zu", allTagIds.size());
    for (const auto& info : albumTagInfos) {
        CHECK_AND_CONTINUE(info.tagPair.second.has_value());
        MEDIA_INFO_LOG("old TagId: %{public}s, old GroupTags is: %{public}s",
            info.tagPair.first.value_or(std::string("-1")).c_str(), info.tagPair.second.value().c_str());
        std::string newTagId = RemoveNonExistentTags(allTagIds, info.tagPair.first.value());
        std::string newGroupTag = RemoveNonExistentTags(allTagIds, info.tagPair.second.value());
        if (newGroupTag != info.tagPair.second.value() || newTagId != info.tagPair.first.value()) {
            AlbumTagInfo albumTaginfo;
            albumTaginfo.albumId = info.albumId;
            albumTaginfo.tagPair.first = newTagId;
            albumTaginfo.tagPair.second = newGroupTag;
            updateAlbumTagInfo.emplace_back(albumTaginfo);
            MEDIA_INFO_LOG("TagId: %{public}s  GroupTags: %{public}s updated", info.tagPair.first.value().c_str(),
                info.tagPair.second.value().c_str());
        }
    }

    UpdateTagsColumn(updateAlbumTagInfo, mediaLibraryRdb);
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
        CHECK_AND_CONTINUE(!tagIds.empty());
        const std::string newGroupTag =
            (tagIds.size() > 1) ? BackupDatabaseUtils::JoinValues(tagIds, "|") : tagIds.front();
        CHECK_AND_EXECUTE(newGroupTag == groupTag, UpdateTagPairs(updatedPairs, newGroupTag, tagIds));
    }
}

void BackupDatabaseUtils::UpdateFaceGroupTagOfGallery(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb)
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
        bool cond = (fileInfo.associateFileId <= 0 || fileInfo.fileIdOld <= 0 || fileInfo.fileIdNew <= 0);
        CHECK_AND_CONTINUE(!cond);
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

void BackupDatabaseUtils::BatchUpdatePhotosToLocal(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
    const std::vector<std::string> &inColumn)
{
    CHECK_AND_RETURN(!inColumn.empty());

    int32_t changeRows = 0;
    std::unique_ptr<NativeRdb::AbsRdbPredicates> predicates =
        make_unique<NativeRdb::AbsRdbPredicates>(PhotoColumn::PHOTOS_TABLE);
    predicates->In(MediaColumn::MEDIA_ID, inColumn);
    NativeRdb::ValuesBucket updatePostBucket;
    updatePostBucket.Put(PhotoColumn::PHOTO_CLEAN_FLAG, 0);
    updatePostBucket.Put(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::LOCAL));
    updatePostBucket.Put(PhotoColumn::PHOTO_SOUTH_DEVICE_TYPE,
        static_cast<int32_t>(SouthDeviceType::SOUTH_DEVICE_NULL));
    updatePostBucket.PutNull(PhotoColumn::PHOTO_CLOUD_ID);
    updatePostBucket.PutNull(PhotoColumn::PHOTO_CLOUD_VERSION);
    updatePostBucket.Put(PhotoColumn::PHOTO_THUMBNAIL_READY, 0);
    updatePostBucket.Put(PhotoColumn::PHOTO_THUMB_STATUS, static_cast<int32_t>(PhotoThumbStatusType::NOT_DOWNLOADED));
    updatePostBucket.Put(PhotoColumn::PHOTO_LCD_VISIT_TIME, 0);
    updatePostBucket.Put(PhotoColumn::PHOTO_THUMBNAIL_VISIBLE, 0);

    BackupDatabaseUtils::Update(mediaLibraryRdb, changeRows, updatePostBucket, predicates);
    if (changeRows != static_cast<int32_t>(inColumn.size())) {
        MEDIA_ERR_LOG("update failed, UpdatePhotoToLocal, expected count %{public}d, but got %{public}d",
            static_cast<int32_t>(inColumn.size()), changeRows);
    }
}

static bool IsBetaVersionCheck()
{
    std::string versionType = system::GetParameter(CONST_LOGSYSTEM_VERSIONTYPE, "unknown");
    return versionType == BETA_VERSIONTYPE;
}

static std::string CheckSingleTableIntegrity(std::shared_ptr<NativeRdb::RdbStore> rdbStore,
                                             const std::string &tableName)
{
    std::string querySql = "PRAGMA quick_check(" + tableName + ")";
    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(rdbStore, querySql);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Query for table %{public}s failed, resultSet is null",
                      MediaFileUtils::DesensitizeName(tableName).c_str());
        return "query failed";
    }
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        MEDIA_ERR_LOG("No result row for table %{public}s", MediaFileUtils::DesensitizeName(tableName).c_str());
        return "no result";
    }
    std::string result = GetStringVal(COLUMN_INTEGRITY_CHECK, resultSet);
    resultSet->Close();
    return result;
}

static std::string CheckDualIntegrity(std::shared_ptr<NativeRdb::RdbStore> rdbStore)
{
    const std::string querySql = "PRAGMA " + COLUMN_INTEGRITY_CHECK;
    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(rdbStore, querySql);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG ("CheckDbIntegrity: Query resultSet is null or GoToFirstRow failed.");
        return "";
    }
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return "";
    }
    std::string result = GetStringVal(COLUMN_INTEGRITY_CHECK, resultSet);
    resultSet->Close();
    return result;
}

std::string BackupDatabaseUtils::CheckDbIntegrity(std::shared_ptr<NativeRdb::RdbStore> rdbStore, int32_t sceneCode,
                                                  const std::string &dbTag, ReverseRestoreReportInfo *reportInfo)
{
    if (sceneCode != CLONE_RESTORE_ID) {
        std::string result = CheckDualIntegrity(rdbStore);
        MEDIA_INFO_LOG("Check db integrity final result for dual: scene=%{public}d, tag=%{public}s, result=%{public}s",
                       sceneCode, dbTag.c_str(), result.c_str());
        return result;
    }

    if (!IsBetaVersionCheck()) {
        MEDIA_INFO_LOG("CheckDbIntegrity: Commercial version, skip integrity check, scene=%{public}d, tag=%{public}s",
                       sceneCode, dbTag.c_str());
        return INTEGRITY_CHECK_OK;
    }

    MEDIA_INFO_LOG("CheckDbIntegrity: Beta version, start integrity check for scene=%{public}d, tag=%{public}s",
                   sceneCode, dbTag.c_str());

    uint32_t successCount = 0;
    uint32_t expectedCount = INTEGRITY_CHECK_TABLE_COUNT;
    std::vector<std::string> tables = {PHOTOS_TABLE_NAME, PHOTO_ALBUM_TABLE_NAME};
    for (const auto &table : tables) {
        std::string result = CheckSingleTableIntegrity(rdbStore, table);
        if (result == INTEGRITY_CHECK_OK) {
            successCount++;
            MEDIA_INFO_LOG("Integrity check for table %{public}s: ok", MediaFileUtils::DesensitizeName(table).c_str());
        } else {
            MEDIA_ERR_LOG("Integrity check for table %{public}s failed: %{public}s",
                          MediaFileUtils::DesensitizeName(table).c_str(), result.c_str());
        }
    }

    std::string finalResult = (successCount == expectedCount) ? INTEGRITY_CHECK_OK : INTEGRITY_CHECK_ERROR;
    if (reportInfo != nullptr) {
        if (finalResult == INTEGRITY_CHECK_OK) {
            reportInfo->quickCheckResult = "CHECK PASS";
        } else {
            reportInfo->quickCheckResult = "CHECK FAILED";
        }
    }
    MEDIA_INFO_LOG("Check db integrity final result: scene=%{public}d, tag=%{public}s, result=%{public}s", sceneCode,
                    dbTag.c_str(), finalResult.c_str());
    return finalResult;
}

int32_t BackupDatabaseUtils::QueryLocalNoAstcCount(std::shared_ptr<NativeRdb::RdbStore> rdbStore)
{
    const std::string QUERY_LOCAL_NO_ASTC_COUNT = "SELECT count(1) AS count FROM Photos "
        "WHERE " + PhotoColumn::PHOTO_POSITION + " = 1 AND " + PhotoColumn::PHOTO_THUMBNAIL_VISIBLE + " = 0 " +
        "AND " + MediaColumn::MEDIA_DATE_TRASHED + " = 0 AND " + MediaColumn::MEDIA_TIME_PENDING +  "= 0 " +
        "AND " + MediaColumn::MEDIA_HIDDEN + " = 0 AND " + PhotoColumn::PHOTO_IS_TEMP + " = 0 " +
        "AND " + PhotoColumn::PHOTO_BURST_COVER_LEVEL + " = 1 AND " + PhotoColumn::PHOTO_CLEAN_FLAG + " = 0 " +
        "AND " + PhotoColumn::PHOTO_SYNC_STATUS + " = 0";
    return QueryInt(rdbStore, QUERY_LOCAL_NO_ASTC_COUNT, CUSTOM_COUNT);
}

int32_t BackupDatabaseUtils::QueryReadyAstcCount(std::shared_ptr<NativeRdb::RdbStore> rdbStore)
{
    const std::string QUERY_READY_ASTC_COUNT = "SELECT count(1) AS count FROM Photos WHERE " +
        PhotoColumn::PHOTO_THUMBNAIL_VISIBLE + " = 1";
    return QueryInt(rdbStore, QUERY_READY_ASTC_COUNT, CUSTOM_COUNT);
}

std::unordered_map<int32_t, int32_t> BackupDatabaseUtils::QueryMediaTypeCount(
    const std::shared_ptr<NativeRdb::RdbStore>& rdbStore, const std::string& querySql)
{
    std::unordered_map<int32_t, int32_t> mediaTypeCountMap;
    auto resultSet = GetQueryResultSet(rdbStore, querySql);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("resultSet is nullptr");
        return mediaTypeCountMap;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t mediaType = GetInt32Val(EXTERNAL_MEDIA_TYPE, resultSet);
        int32_t count = GetInt32Val(CUSTOM_COUNT, resultSet);
        mediaTypeCountMap[mediaType] = count;
    }
    resultSet->Close();
    return mediaTypeCountMap;
}

std::shared_ptr<NativeRdb::ResultSet> BackupDatabaseUtils::QuerySql(
    std::shared_ptr<NativeRdb::RdbStore> rdbStore, const std::string &querySql,
    const std::vector<NativeRdb::ValueObject> &params)
{
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdbStore is nullptr");
        return nullptr;
    }
    return rdbStore->QuerySql(querySql, params);
}

void BackupDatabaseUtils::UpdateBurstPhotos(const std::shared_ptr<NativeRdb::RdbStore> &rdbStore, int32_t maxId)
{
    const string updateBurstSql =
        "UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " +
        PhotoColumn::PHOTO_BURST_COVER_LEVEL + " = " + to_string(static_cast<int32_t>(BurstCoverLevelType::COVER)) +
        "," + PhotoColumn::PHOTO_BURST_KEY + " = NULL," +
        PhotoColumn::PHOTO_SUBTYPE + " = " + to_string(static_cast<int32_t>(PhotoSubType::DEFAULT)) +
        " WHERE " + SQL_SELECT_ERROR_BURST_PHOTOS +
        " AND " + " ((file_id IN (" + SQL_SELECT_CLONE_FILE_IDS + ")) OR (file_id > " + to_string(maxId)+"))";
    
    int32_t erroCode = BackupDatabaseUtils::ExecuteSQL(rdbStore, updateBurstSql);
    CHECK_AND_PRINT_LOG(erroCode >= 0, "execute update continuous shooting photos, ret=%{public}d", erroCode);

    const string updateBurstCoverSql =
        "UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " +
        PhotoColumn::PHOTO_BURST_KEY + " = NULL," +
        PhotoColumn::PHOTO_SUBTYPE + " = " + to_string(static_cast<int32_t>(PhotoSubType::DEFAULT)) +
        " WHERE " + SQL_SELECT_ERROR_BURST_COVER_PHOTOS +
        " AND " + " ((file_id IN (" + SQL_SELECT_CLONE_FILE_IDS + ")) OR (file_id > " + to_string(maxId)+"))";
    
    erroCode = BackupDatabaseUtils::ExecuteSQL(rdbStore, updateBurstCoverSql);
    CHECK_AND_PRINT_LOG(erroCode >= 0, "execute update error burst cover photo, ret=%{public}d", erroCode);
}

std::vector<int32_t> BackupDatabaseUtils::QueryIntVec(std::shared_ptr<NativeRdb::RdbStore> rdbStore,
    const std::string& sql, const std::string& columnName)
{
    std::vector<int32_t> results;
    if (rdbStore == nullptr) {
        return results;
    }

    auto resultSet = rdbStore->QuerySql(sql);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Failed to query SQL or resultSet is null");
        return results;
    }

    int32_t columnIndex = -1;
    if (resultSet->GetColumnIndex(columnName, columnIndex) != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Get column index error");
        resultSet->Close();
        return results;
    }

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t value;
        if (resultSet->GetInt(columnIndex, value) == NativeRdb::E_OK) {
            results.push_back(value);
        }
    }
    resultSet->Close();
    return results;
}

std::unordered_map<int32_t, int32_t> BackupDatabaseUtils::QueryIntMap(std::shared_ptr<NativeRdb::RdbStore> rdbStore,
    const std::string& sql, const std::string& keyColumnName, const std::string& valueColumnName)
{
    std::unordered_map<int32_t, int32_t> results;

    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdbStore is null");
        return results;
    }

    auto resultSet = rdbStore->QuerySql(sql);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Failed to query SQL or resultSet is null (SQL: %s)", sql.c_str());
        return results;
    }

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t key = GetInt32Val(keyColumnName, resultSet);
        int32_t value = GetInt32Val(valueColumnName, resultSet);
        results[key] = value;
    }
    resultSet->Close();
    return results;
}

std::vector<CloneVideoInfo> BackupDatabaseUtils::QueryVideoInfo(
    std::string tableName, std::string VideofileIdOldInClause, std::shared_ptr<NativeRdb::RdbStore> rdbStore)
{
    std::vector<CloneVideoInfo> results;
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdbStore is null");
        return results;
    }
    std::string sql =
        "SELECT file_id,status,label,face FROM " + tableName +
        " WHERE file_id IN " + VideofileIdOldInClause;
    auto resultSet = rdbStore->QuerySql(sql);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Failed to query SQL or resultSet is null (SQL: %s)", sql.c_str());
        return results;
    }

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t file_id = GetInt32Val("file_id", resultSet);
        int32_t status = GetInt32Val("status", resultSet);
        int32_t label = GetInt32Val("label", resultSet);
        int32_t face = GetInt32Val("face", resultSet);
        results.push_back({file_id, face, label, status});
    }
    resultSet->Close();
    return results;
}

std::unordered_map<int32_t, int32_t> BackupDatabaseUtils::QueryOldNoFaceStatus(
    std::shared_ptr<NativeRdb::RdbStore> oldRdbStore,
    const std::vector<int32_t>& oldFileIds)
{
    if (oldFileIds.empty()) {
        MEDIA_ERR_LOG("No old file IDs to process for no face status query.");
        return {};
    }

    std::string fileIdOldInClause = "(" + BackupDatabaseUtils::JoinValues<int>(oldFileIds, ", ") + ")";
    std::string queryOldNoFaceSql =
        "SELECT file_id, face FROM tab_analysis_total "
        "WHERE face < 0 AND file_id IN " + fileIdOldInClause;

    std::unordered_map<int32_t, int32_t> oldFileIdToFaceMap;
    auto resultSet = oldRdbStore->QuerySql(queryOldNoFaceSql);
    if (resultSet != nullptr) {
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            int32_t fileId = GetInt32Val("file_id", resultSet);
            int32_t faceValue = GetInt32Val("face", resultSet);
            oldFileIdToFaceMap[fileId] = faceValue;
        }
        resultSet->Close();
    }

    if (oldFileIdToFaceMap.empty()) {
        MEDIA_ERR_LOG("No old files found with negative face status to migrate.");
    }

    return oldFileIdToFaceMap;
}

void BackupDatabaseUtils::UpdateNewNoFaceStatus(std::shared_ptr<NativeRdb::RdbStore> newRdbStore,
    const std::unordered_map<int32_t, int32_t>& oldFileIdToFaceMap, const std::vector<FileIdPair>& fileIdPair)
{
    if (oldFileIdToFaceMap.empty()) {
        MEDIA_ERR_LOG("No old face status data to update.");
        return;
    }

    std::map<int32_t, int32_t> oldToNewIdMap;
    for (const auto& pair : fileIdPair) {
        oldToNewIdMap[pair.first] = pair.second;
    }

    std::unordered_map<int32_t, int32_t> newFileIdToFaceMap;
    for (const auto& [oldId, faceValue] : oldFileIdToFaceMap) {
        auto it = oldToNewIdMap.find(oldId);
        if (it != oldToNewIdMap.end()) {
            newFileIdToFaceMap[it->second] = faceValue;
        }
    }

    if (newFileIdToFaceMap.empty()) {
        MEDIA_ERR_LOG("No new file IDs found for old files with negative face status.");
        return;
    }

    for (const auto& [newId, faceValue] : newFileIdToFaceMap) {
        std::string updateSql =
            "UPDATE tab_analysis_total "
            "SET face = " + std::to_string(faceValue) + " "
            "WHERE file_id = " + std::to_string(newId);
        int32_t errCode = BackupDatabaseUtils::ExecuteSQL(newRdbStore, updateSql);
        CHECK_AND_PRINT_LOG(errCode >= 0, "execute update totalTbl for no face failed, ret=%{public}d", errCode);
    }
}

void BackupDatabaseUtils::UpdateAnalysisTotalTblNoFaceStatus(std::shared_ptr<NativeRdb::RdbStore> newRdbStore,
    std::shared_ptr<NativeRdb::RdbStore> oldRdbStore, const std::vector<FileIdPair>& fileIdPair)
{
    auto [oldFileIds, newFileIds] = BackupDatabaseUtils::UnzipFileIdPairs(fileIdPair);
    auto oldFileIdToFaceMap = QueryOldNoFaceStatus(oldRdbStore, oldFileIds);

    UpdateNewNoFaceStatus(newRdbStore, oldFileIdToFaceMap, fileIdPair);
}

bool BackupDatabaseUtils::isTableExist(const std::shared_ptr<NativeRdb::RdbStore> &rdbStore,
    const std::string &tableName, bool& result)
{
    std::string querySql = "SELECT name FROM sqlite_master WHERE type='table' AND name='" + tableName + "';";
    MEDIA_DEBUG_LOG("BackupDatabaseUtils::isTableExist sql: %{public}s", querySql.c_str());
    auto resultSet = GetQueryResultSet(rdbStore, querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "resultSet is nullptr");
    int32_t count = -1;
    int32_t err = resultSet->GetRowCount(count);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, false, "Failed to get count, err: %{public}d", err);
    resultSet->Close();
    result = (count > 0);
    return true;
}

BackupDatabaseUtils::ConfigInfoType BackupDatabaseUtils::QueryConfigInfo(
    const std::shared_ptr<NativeRdb::RdbStore> &rdbStore)
{
    ConfigInfoType configInfo;
    std::string querySql = "select * from " + ConfigInfoColumn::MEDIA_CONFIG_INFO_TABLE_NAME;
    auto resultSet = GetQueryResultSet(rdbStore, querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, configInfo, "resultSet is null.");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int sceneIdInt = GetInt32Val(ConfigInfoColumn::MEDIA_CONFIG_INFO_SCENE_ID, resultSet);
        std::string key = GetStringVal(ConfigInfoColumn::MEDIA_CONFIG_INFO_KEY, resultSet);
        std::string value = GetStringVal(ConfigInfoColumn::MEDIA_CONFIG_INFO_VALUE, resultSet);
        MEDIA_INFO_LOG("query backupInfo result: sceneId:%{public}d key:%{public}s value:%{public}s",
            sceneIdInt, key.c_str(), MediaFileUtils::DesensitizeName(value).c_str());
        CHECK_AND_CONTINUE_ERR_LOG(INT_CONFIG_INFO_SCENE_ID_MAP.count(sceneIdInt),
            "fail to parse SceneId:%{public}d", sceneIdInt);
        ConfigInfoSceneId sceneId = INT_CONFIG_INFO_SCENE_ID_MAP.at(sceneIdInt);
        configInfo[sceneId][key] = value;
    }
    resultSet->Close();
    return configInfo;
}

bool BackupDatabaseUtils::QueryPhotoUniqueSouthDeviceType(
    const std::shared_ptr<NativeRdb::RdbStore> &rdbStore, std::vector<SouthDeviceType>& uniqueSouthDeviceType)
{
    MEDIA_DEBUG_LOG("QueryPhotoUniqueSouthDeviceType sql: %{public}s",
        SQL_QUERY_PHOTO_UNIQUE_SOUTH_DEVICE_TYPE.c_str());
    auto resultSet = GetQueryResultSet(rdbStore, SQL_QUERY_PHOTO_UNIQUE_SOUTH_DEVICE_TYPE);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "resultSet in nullptr");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int southDeviceTypeInt = GetInt32Val(PhotoColumn::PHOTO_SOUTH_DEVICE_TYPE, resultSet);
        CHECK_AND_RETURN_RET_LOG(INT_SOUTH_DEVICE_TYPE_MAP.count(southDeviceTypeInt), false,
            "invalid SouthDeviceType value: %{public}d", southDeviceTypeInt);
        uniqueSouthDeviceType.push_back(INT_SOUTH_DEVICE_TYPE_MAP.at(southDeviceTypeInt));
        MEDIA_INFO_LOG("south_device_type: %{public}d", southDeviceTypeInt);
    }
    resultSet->Close();
    return true;
}

bool BackupDatabaseUtils::ClearConfigInfo(const std::shared_ptr<NativeRdb::RdbStore> &rdbStore)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore, false, "rdbStore is nullptr");
    std::string sqlStr = "DELETE FROM " + ConfigInfoColumn::MEDIA_CONFIG_INFO_TABLE_NAME + " WHERE " +
        ConfigInfoColumn::MEDIA_CONFIG_INFO_SCENE_ID + "=" +
        std::to_string(static_cast<int>(ConfigInfoSceneId::CLONE_RESTORE));
    MEDIA_DEBUG_LOG("Clear ConfigInfo sql: %{public}s", sqlStr.c_str());
    int32_t ret = BackupDatabaseUtils::ExecuteSQL(rdbStore, sqlStr);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, false,
        "fail to clear ConfigInfo, sql: %{private}s, ret:%{public}d", sqlStr.c_str(), ret);
    MEDIA_INFO_LOG("succeed to clear ConfigInfo");
    return true;
}
// LCOV_EXCL_STOP

void BackupDatabaseUtils::ClearAnalysisVideoTotalTable(
    const std::shared_ptr<NativeRdb::RdbStore> &rdbStore)
{
    CHECK_AND_RETURN_LOG(rdbStore, "rdbStore is nullptr");
    std::string updateFace_sql =
        "UPDATE " + VISION_VIDEO_TOTAL_TABLE + " " +
        " SET face = " + FACE_DEFAULT_VALUE + " ;";
    MEDIA_DEBUG_LOG("UpdateAnalysisVideoTotalTblLabelAndFace ClearAnalysisVideoTotalTable \
        sql = %{public}s", updateFace_sql.c_str());
    int32_t errCode = BackupDatabaseUtils::ExecuteSQL(rdbStore, updateFace_sql);
    CHECK_AND_RETURN_LOG(errCode >= 0, "execute ClearAnalysisVideoTotalTable updateFace_sql\
        failed, ret=%{public}d", errCode);
    std::string updateStatus_sql =
        "UPDATE " + VISION_VIDEO_TOTAL_TABLE +
        " SET status = " + STATUS_DEFAULT_VALUE +
        " WHERE status = 1 ;";
    MEDIA_DEBUG_LOG("UpdateAnalysisVideoTotalTblLabelAndFace ClearAnalysisVideoTotalTable \
        sql = %{public}s", updateStatus_sql.c_str());
    int32_t errCode_status = BackupDatabaseUtils::ExecuteSQL(rdbStore, updateStatus_sql);
    CHECK_AND_RETURN_LOG(errCode_status >= 0, "execute ClearAnalysisVideoTotalTable updateStatus_sql \
        failed, ret=%{public}d", errCode_status);
    MEDIA_DEBUG_LOG("succeed to ClearAnalysisVideoTotalTable");
}

void BackupDatabaseUtils::CheckFaceToAnalysisVideoTotalTable(
    const std::shared_ptr<NativeRdb::RdbStore> &rdbStore)
{
    CHECK_AND_RETURN_LOG(rdbStore, "rdbStore is nullptr");
    std::string checkFace_sql =
        "UPDATE " + VISION_VIDEO_TOTAL_TABLE + " SET " +
        FACE + " = 0, " +
        STATUS + " = CASE WHEN " + STATUS + " = 1 THEN 0 ELSE " + STATUS + " END "
        "WHERE NOT EXISTS ("
            "SELECT 1 FROM " + VISION_VIDEO_FACE_TABLE +
            " WHERE " + FILE_ID + " = " + VISION_VIDEO_TOTAL_TABLE + "." + FILE_ID +
        ");";
    MEDIA_DEBUG_LOG("CheckFaceToAnalysisVideoTotalTable \
        checkFace_sql sql = %{public}s", checkFace_sql.c_str());
    int32_t errCode = BackupDatabaseUtils::ExecuteSQL(rdbStore, checkFace_sql);
    CHECK_AND_RETURN_LOG(errCode >= 0, "execute \
        CheckFaceToAnalysisVideoTotalTable checkFace_sql failed, ret=%{public}d", errCode);
}

void BackupDatabaseUtils::UpdateFaceToAnalysisVideoTotalTable(
    const std::shared_ptr<NativeRdb::RdbStore> &rdbStore)
{
    CHECK_AND_RETURN_LOG(rdbStore, "rdbStore is nullptr");
    const string UpdateFace_sql =
        "UPDATE " + VISION_VIDEO_TOTAL_TABLE + " SET " + FACE + " = " + FACE_EXTRACT_FEATURE_VALUE
         + " WHERE " + FACE + " = "+ FACE_DETECTION_VALUE;
    MEDIA_DEBUG_LOG("UpdateAnalysisVideoTotalTblLabelAndFace UpdateFaceToAnalysisVideoTotalTable \
        sql = %{public}s", UpdateFace_sql.c_str());
    int32_t errCode = BackupDatabaseUtils::ExecuteSQL(rdbStore, UpdateFace_sql);
    CHECK_AND_RETURN_LOG(errCode >= 0, "execute UpdateFaceToAnalysisVideoTotalTable \
        failed, ret=%{public}d", errCode);
    MEDIA_DEBUG_LOG("succeed to UpdateFaceToAnalysisVideoTotalTable");
}

void BackupDatabaseUtils::UpdateStatusToAnalysisTable(
    const std::shared_ptr<NativeRdb::RdbStore> &rdbStore)
{
    CHECK_AND_RETURN_LOG(rdbStore, "rdbStore is nullptr");
    const string UpdateStatus_sql = "UPDATE " + VISION_TOTAL_TABLE +
        " SET " + STATUS + " = " + STATUS_DEFAULT_VALUE +" WHERE " + FILE_ID +" IN (SELECT " +
        FILE_ID + " FROM " + PHOTOS_TABLE + " WHERE media_type = " +
        MEDIA_TYPE_VIDEO_VALUE +") AND " + STATUS + " = " + STATUS_ANALYSIS_FINISH_VALUE;
    MEDIA_DEBUG_LOG("UpdateAnalysisVideoTotalTblLabelAndFace UpdateStatusToAnalysisTable \
        sql = %{public}s", UpdateStatus_sql.c_str());
    int32_t errCode = BackupDatabaseUtils::ExecuteSQL(rdbStore, UpdateStatus_sql);
    CHECK_AND_RETURN_LOG(errCode >= 0, "execute UpdateStatusToAnalysisTable \
        failed, ret=%{public}d", errCode);
    MEDIA_DEBUG_LOG("succeed to UpdateStatusToAnalysisTable");
}

void BackupDatabaseUtils::UpdateFaceToAnalysisTable(const shared_ptr<NativeRdb::RdbStore>& rdbStore,
    const std::vector<int32_t> &newFileIds)
{
    CHECK_AND_RETURN_LOG(rdbStore, "rdbStore is nullptr");
    CHECK_AND_RETURN_LOG(!newFileIds.empty(), "newFileIds is empty");
    std::string fileIdNewFilterClause = "(" + BackupDatabaseUtils::JoinValues<int>(newFileIds, ", ") + ")";

    const string sql = "UPDATE " + VISION_TOTAL_TABLE + " SET " + FACE + " = 0 WHERE " +
        FILE_ID + " IN ( SELECT " + FILE_ID + " FROM " + PHOTOS_TABLE + " WHERE media_type = 2 )" +
        " AND " + FILE_ID + " IN " + fileIdNewFilterClause;
    MEDIA_DEBUG_LOG("UpdateAnalysisVideoTotalTblLabelAndFace UpdateLabelAndFaceToAnalysisTable \
        sql = %{public}s", sql.c_str());
    int32_t errCode = BackupDatabaseUtils::ExecuteSQL(rdbStore, sql);
    CHECK_AND_RETURN_LOG(errCode >= 0, "execute UpdateLabelAndFaceToAnalysisTable \
        failed, ret=%{public}d", errCode);
    MEDIA_DEBUG_LOG("succeed to UpdateLabelAndFaceToAnalysisTable");
}

void BackupDatabaseUtils::DeleteDirtytagIdFromFaceTagTable(
    const std::shared_ptr<NativeRdb::RdbStore> &rdbStore)
{
    CHECK_AND_RETURN_LOG(rdbStore, "rdbStore is nullptr");
    const string DeleteDirtytagId_sql =
        "DELETE FROM " + VISION_FACE_TAG_TABLE +
        " WHERE EXISTS (SELECT 1 FROM " + VISION_VIDEO_FACE_TABLE
        + " WHERE " + VISION_FACE_TAG_TABLE + "." + TAG_ID
        + " = " + VISION_VIDEO_FACE_TABLE + "." + TAG_ID + " AND "
        + VISION_VIDEO_FACE_TABLE + "." + TAG_ID + " like 'ser%')"
        " AND NOT EXISTS  (SELECT 1 FROM " + VISION_IMAGE_FACE_TABLE
        + " WHERE " + VISION_FACE_TAG_TABLE + "." + TAG_ID +
        + " = " + VISION_IMAGE_FACE_TABLE + "." + TAG_ID +
        " AND " + VISION_IMAGE_FACE_TABLE + "." + TAG_ID + " like 'ser%');";
    MEDIA_DEBUG_LOG("UpdateAnalysisVideoTotalTblFaceAndTagId \
        DeleteDirtytagIdFromFaceTagTable sql = %{public}s", DeleteDirtytagId_sql.c_str());
    int32_t errCode = BackupDatabaseUtils::ExecuteSQL(rdbStore, DeleteDirtytagId_sql);
    CHECK_AND_RETURN_LOG(errCode >= 0, "execute DeleteDirtytagIdFromFaceTagTable \
        failed, ret=%{public}d", errCode);
    MEDIA_DEBUG_LOG("succeed to DeleteDirtytagIdFromFaceTagTable");
}

void BackupDatabaseUtils::UpdateVideoFaceTagId(
    const std::shared_ptr<NativeRdb::RdbStore> &rdbStore)
{
    CHECK_AND_RETURN_LOG(rdbStore, "rdbStore is nullptr");
    std::string UpdateVideoFaceTagId_sql =
        "UPDATE " + VISION_VIDEO_FACE_TABLE + " SET " + TAG_ID
        + " = " + TAGID_WAITING_FOR_ANALYSIS_VALUE + " WHERE "
        "NOT EXISTS (SELECT 1 FROM " + VISION_FACE_TAG_TABLE + " WHERE "
        + VISION_FACE_TAG_TABLE + "." + TAG_ID + " = " +
        VISION_VIDEO_FACE_TABLE + "." + TAG_ID + " AND "
        + VISION_VIDEO_FACE_TABLE + "." + TAG_ID + " like 'ser%') OR " +
        TAG_ID + " = " + TAGID_NO_NEED_TO_ANALYSIS_VALUE + ";";
    MEDIA_DEBUG_LOG("UpdateAnalysisVideoTotalTblFaceAndTagId \
        UpdateVideoFaceTagId sql = %{public}s", UpdateVideoFaceTagId_sql.c_str());
    int32_t errCode = BackupDatabaseUtils::ExecuteSQL(rdbStore, UpdateVideoFaceTagId_sql);
    CHECK_AND_RETURN_LOG(errCode >= 0, "execute UpdateVideoFaceTagId failed, ret=%{public}d", errCode);
    MEDIA_DEBUG_LOG("succeed to UpdateVideoFaceTagId");
}

void BackupDatabaseUtils::UpdateVideoTotalFaceId(
    const std::shared_ptr<NativeRdb::RdbStore> &rdbStore)
{
    CHECK_AND_RETURN_LOG(rdbStore, "rdbStore is nullptr");
    std::string UpdateVideoTotalFaceId_sql =
        "UPDATE " + VISION_VIDEO_TOTAL_TABLE + " SET " + FACE + " = ( SELECT CASE WHEN" +
        " COUNT(CASE WHEN " + TAG_ID + " LIKE 'ser%' THEN 1 END) > 0" +
        " AND COUNT(CASE WHEN " + TAG_ID + " NOT LIKE 'ser%' THEN 1 END) = 0 THEN "
        + FACE_CLUSTERING_VALUE +
        " WHEN COUNT(CASE WHEN " + TAG_ID + " LIKE 'ser%' THEN 1 END) > 0" +
        " AND COUNT(CASE WHEN " + TAG_ID + " NOT LIKE 'ser%' THEN 1 END) > 0 THEN "
        + FACE_CLUSTERING_FAIL_VALUE +
        " ELSE " + FACE_EXTRACT_FEATURE_VALUE + " END"
        " FROM " + VISION_VIDEO_FACE_TABLE + " WHERE " + VISION_VIDEO_FACE_TABLE + "." +
        FILE_ID + " = " + VISION_VIDEO_TOTAL_TABLE + "." + FILE_ID + " GROUP BY " + FILE_ID +
        " ) WHERE EXISTS (SELECT 1 FROM " + VISION_VIDEO_FACE_TABLE + " WHERE "
        + VISION_VIDEO_FACE_TABLE + "." + FILE_ID + " = " + VISION_VIDEO_TOTAL_TABLE + "." + FILE_ID + ")"
        " AND " + VISION_VIDEO_TOTAL_TABLE + "." + FACE + " = " + FACE_EXTRACT_FEATURE_VALUE + " ;";
    MEDIA_DEBUG_LOG("UpdateAnalysisVideoTotalTblFaceAndTagId \
        UpdateVideoTotalFaceId sql = %{public}s", UpdateVideoTotalFaceId_sql.c_str());
    int32_t errCode = BackupDatabaseUtils::ExecuteSQL(rdbStore, UpdateVideoTotalFaceId_sql);
    CHECK_AND_RETURN_LOG(errCode >= 0, "execute UpdateVideoTotalFaceId failed, ret=%{public}d", errCode);
    MEDIA_DEBUG_LOG("succeed to UpdateVideoTotalFaceId");
}

int32_t CloneFileInfoRestoreDbCallback::OnCreate(NativeRdb::RdbStore &rdbStore)
{
    MEDIA_INFO_LOG("CloneFileInfoRestoreDbCallback::OnCreate database should already exist, not create new");
    return NativeRdb::E_OK;
}

int32_t CloneFileInfoDbCallback::OnCreate(NativeRdb::RdbStore &rdbStore)
{
    MEDIA_INFO_LOG("CloneFileInfoDbCallback::OnCreate ancoFileListClone: %{public}d, fileManagerFileListClone: "
        "%{public}d", static_cast<int32_t>(ancoFileListClone_), static_cast<int32_t>(fileManagerFileListClone_));

    std::vector<std::string> tablesToCreate;
    if (ancoFileListClone_ == AncoFileListClone::ANCO_FILE_LIST_CLONE_SUPPORTED) {
        tablesToCreate.push_back(LAKE_FILE_INFO_TABLE);
    }
    if (fileManagerFileListClone_ != FileManagerFileListClone::FILE_MANAGER_FILE_LIST_CLONE_NONE) {
        tablesToCreate.push_back(FILE_MANAGER_INFO_TABLE);
    }
    for (const auto &tableName : tablesToCreate) {
        std::string createTableSql = MediaStringUtils::FillParams(SQL_CREATE_FILE_INFO_TABLE_FOR_CLONE, { tableName });
        int32_t ret = rdbStore.ExecuteSql(createTableSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("LakeClone: Failed to create %{public}s table, ret: %{public}d", tableName.c_str(), ret);
            return ret;
        }
        MEDIA_INFO_LOG("LakeClone: Create %{public}s table success", tableName.c_str());
    }
    MEDIA_INFO_LOG("LakeClone: Create cloneFileInfoDB success");
    return NativeRdb::E_OK;
}

void BackupDatabaseUtils::UpdateDuplicateCoverUris(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
    const std::unordered_map<int32_t, int32_t>& duplicateAssetMap,
    const std::unordered_map<int32_t, PhotoInfo>& photoInfoMap,
    const std::vector<int32_t>& albumSubtypes)
{
    CHECK_AND_RETURN_LOG(!duplicateAssetMap.empty(), "duplicateAssetMap is empty, skip update");
    CHECK_AND_RETURN_LOG(!albumSubtypes.empty(), "albumSubtypes is empty, skip update");
    MEDIA_INFO_LOG("update duplicate cover uris start.");

    std::string subtypeClause;
    for (size_t i = 0; i < albumSubtypes.size(); ++i) {
        if (i > 0) {
            subtypeClause += ", ";
        }
        subtypeClause += std::to_string(albumSubtypes[i]);
    }

    const std::string QUERY_SQL = "SELECT album_id, cover_uri FROM AnalysisAlbum "
        " WHERE album_subtype IN (" + subtypeClause + ") AND cover_uri IS NOT NULL ";
    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaLibraryRdb, QUERY_SQL);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "query AnalysisAlbum for cover_uri failed.");

    std::unordered_map<int32_t, std::string> albumIdToNewCoverUri;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        auto albumId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "album_id");
        auto coverUri = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, "cover_uri");
        CHECK_AND_CONTINUE(albumId.has_value() && coverUri.has_value());

        int32_t fileId = MediaUriUtils::GetFileId(coverUri.value());
        auto it = duplicateAssetMap.find(fileId);
        CHECK_AND_CONTINUE(it != duplicateAssetMap.end());

        int32_t newFileId = it->second;
        auto photoIt = photoInfoMap.find(newFileId);
        CHECK_AND_CONTINUE(photoIt != photoInfoMap.end());

        PhotoInfo photoInfo = photoIt->second;
        std::string newCoverUri = MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX,
            to_string(photoInfo.fileIdNew), MediaFileUtils::GetExtraUri(photoInfo.displayName, photoInfo.cloudPath));
        albumIdToNewCoverUri[albumId.value()] = newCoverUri;
    }
    resultSet->Close();

    if (!albumIdToNewCoverUri.empty()) {
        const std::string UPDATE_SQL = "UPDATE AnalysisAlbum SET cover_uri = ? WHERE album_id = ?";
        for (const auto &[albumId, newCoverUri] : albumIdToNewCoverUri) {
            BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb, UPDATE_SQL, { newCoverUri, albumId });
        }
        MEDIA_INFO_LOG("update duplicate cover uris completed, updated nums: %{public}zu", albumIdToNewCoverUri.size());
    } else {
        MEDIA_INFO_LOG("no duplicate cover uris found to update");
    }
}

int32_t BackupDatabaseUtils::GetDbPhotoCount(std::shared_ptr<NativeRdb::RdbStore> rdbStore, bool isLocal)
{
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("GetDbPhotoCount: rdb is nullptr");
        return -1;
    }

    std::string querySql;
    if (isLocal) {
        querySql = "SELECT COUNT(1) FROM Photos"
        " WHERE position IN (1, 3) AND sync_status = 0 AND clean_flag = 0 AND time_pending = 0 AND is_temp = 0";
    } else {
        querySql = "SELECT COUNT(1) FROM Photos"
        " WHERE sync_status = 0 AND clean_flag = 0 AND time_pending = 0 AND is_temp = 0";
    }

    auto resultSet = BackupDatabaseUtils::QuerySql(rdbStore, querySql, {});
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("GetDbPhotoCount: query failed");
        return -1;
    }

    int32_t err = resultSet->GoToFirstRow();
    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("query error");
        resultSet->Close();
        return err == NativeRdb::E_SQLITE_CORRUPT ? -NativeRdb::E_SQLITE_CORRUPT : -1;
    }

    int32_t count = 0;
    err = resultSet->GetInt(0, count);
    resultSet->Close();

    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("GetDbPhotoCount: get int failed, err=%{public}d", err);
        return -1;
    }

    MEDIA_INFO_LOG("GetDbPhotoCount: %{public}d", count);
    return count;
}
} // namespace Media
} // namespace OHOS
