/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include "media_library_db_upgrade.h"

#include <string>
#include <vector>

#include "dfx_transaction.h"
#include "rdb_store.h"
#include "media_log.h"
#include "album_plugin_table_event_handler.h"
#include "db_upgrade_utils.h"
#include "medialibrary_data_manager_utils.h"
#include "medialibrary_rdb_transaction.h"
#include "parameter.h"
#include "parameters.h"
#include "photo_album_update_date_modified_operation.h"
#include "photo_day_month_year_operation.h"
#include "result_set_utils.h"
#include "classify_aggregate_types.h"
#include "media_file_utils.h"

namespace OHOS::Media {
namespace DataTransfer {
const std::string CATEGORY_REFRESH_ENABLE_TAG = "persist.multimedia.media_analysis_service.categoryfresh.enable";
const int32_t CATEGORY_REFRESH_ENABLE_VALUE = 1;
const int32_t CATEGORY_REFRESH_LENGTH = 3;
// LCOV_EXCL_START
/**
 * @brief Upgrade the database, before data restore or clone.
 */
int32_t MediaLibraryDbUpgrade::OnUpgrade(NativeRdb::RdbStore &store)
{
    int32_t ret = NativeRdb::E_OK;
    MEDIA_INFO_LOG("Media_Restore: MediaLibraryDbUpgrade::OnUpgrade start");
    ret = this->UpgradeAlbumPlugin(store);
    CHECK_AND_RETURN_RET(ret == NativeRdb::E_OK, ret);

    ret = this->UpgradePhotoAlbum(store);
    CHECK_AND_RETURN_RET(ret == NativeRdb::E_OK, ret);

    ret = this->UpgradePhotos(store);
    CHECK_AND_RETURN_RET(ret == NativeRdb::E_OK, ret);

    ret = this->UpgradePhotoMap(store);
    CHECK_AND_RETURN_RET(ret == NativeRdb::E_OK, ret);

    ret = this->MergeAlbumFromOldBundleNameToNewBundleName(store);
    CHECK_AND_RETURN_RET(ret == NativeRdb::E_OK, ret);

    ret = this->UpgradePhotosBelongsToAlbum(store);
    CHECK_AND_RETURN_RET(ret == NativeRdb::E_OK, ret);

    ret = this->ExecSqlWithRetry([&]() {
        return PhotoAlbumUpdateDateModifiedOperation::UpdateAlbumDateNeedFix(store);
    });
    CHECK_AND_RETURN_RET(ret == NativeRdb::E_OK, ret);

    this->AggregateClassifyAlbum(store);

    MEDIA_INFO_LOG("Media_Restore: MediaLibraryDbUpgrade::OnUpgrade end");
    return NativeRdb::E_OK;
}

int32_t MediaLibraryDbUpgrade::ExecSqlWithRetry(std::function<int32_t()> execSql)
{
    int currentTime = 0;
    int32_t busyRetryTime = 0;
    int32_t err = NativeRdb::E_OK;
    while (busyRetryTime < MAX_BUSY_TRY_TIMES && currentTime <= MAX_TRY_TIMES) {
        err = execSql();
        if (err == NativeRdb::E_OK) {
            break;
        } else if (err == NativeRdb::E_SQLITE_LOCKED) {
            std::this_thread::sleep_for(std::chrono::milliseconds(TRANSACTION_WAIT_INTERVAL));
            currentTime++;
            MEDIA_ERR_LOG("ExecuteSqlInternal busy, ret:%{public}d, time:%{public}d", err, currentTime);
        } else if (err == NativeRdb::E_SQLITE_BUSY || err == NativeRdb::E_DATABASE_BUSY) {
            busyRetryTime++;
            MEDIA_ERR_LOG("ExecuteSqlInternal busy, ret:%{public}d, busyRetryTime:%{public}d", err, busyRetryTime);
        } else {
            MEDIA_ERR_LOG("ExecuteSqlInternal faile, ret = %{public}d", err);
            break;
        }
    }
    return err;
}

static void GetAggregateMap(std::unordered_map<std::string, std::vector<std::string>> &newAlbumMaps)
{
    for (const auto &pair : AGGREGATE_MAPPING_TABLE) {
        int32_t oriAlbum = static_cast<int32_t>(pair.first);
        int32_t newAlbum = static_cast<int32_t>(pair.second);
        newAlbumMaps[std::to_string(newAlbum)].push_back(std::to_string(oriAlbum));
    }
}

static std::string BuildInClause(const std::vector<std::string> &values)
{
    std::string result = "(";
    for (size_t i = 0; i < values.size(); ++i) {
        result += "'" + values[i] + "'";
        if (i != values.size() - 1) {
            result += ", ";
        }
    }
    result += ")";
    return result;
}

void MediaLibraryDbUpgrade::SetAggregateBit(const int32_t &bitPosition)
{
    MEDIA_INFO_LOG("SetAggregateBit start, bitPosition: %{public}d", bitPosition);
    CHECK_AND_RETURN_LOG(bitPosition < CATEGORY_REFRESH_LENGTH && bitPosition >= 0,
        "bitPosition out of range");
    std::string flagStr = system::GetParameter(CATEGORY_REFRESH_ENABLE_TAG, "0");
    CHECK_AND_RETURN_INFO_LOG(MediaLibraryDataManagerUtils::IsNumber(flagStr),
        "categoryfresh flag is not number: %{public}s", flagStr.c_str());
    int32_t flag = std::stoi(flagStr);
    std::bitset<CATEGORY_REFRESH_LENGTH> binary(flag);
    binary.set(bitPosition, CATEGORY_REFRESH_ENABLE_VALUE);
    std::string refreshFlagStr = std::to_string(binary.to_ulong());
    int32_t ret = system::SetParameter(CATEGORY_REFRESH_ENABLE_TAG, refreshFlagStr);
    MEDIA_INFO_LOG("SetAggregateBit end, refreshFlagStr: %{public}s, ret: %{public}d",
        refreshFlagStr.c_str(), ret);
}
bool MediaLibraryDbUpgrade::CheckClassifyAlbumExist(const std::string &newAlbumName,
    NativeRdb::RdbStore &store, bool &isSetAggregateBit)
{
    std::vector<NativeRdb::ValueObject> params = {};
    params.push_back(NativeRdb::ValueObject(std::to_string(PhotoAlbumType::SMART)));
    params.push_back(NativeRdb::ValueObject(std::to_string(PhotoAlbumSubType::CLASSIFY)));
    params.push_back(NativeRdb::ValueObject(newAlbumName));
    auto resultSet = store.QuerySql(SQL_QUERY_CLASSIFY_ALBUM_EXIST, params);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "resultSet is nullptr");
    if (resultSet->GoToNextRow() == NativeRdb::E_OK && GetInt32Val("count", resultSet) > 0) {
        resultSet->Close();
        isSetAggregateBit = true;
        return true;
    }
    resultSet->Close();
    return false;
}

int32_t MediaLibraryDbUpgrade::CreateClassifyAlbum(const std::string &newAlbumName,
    NativeRdb::RdbStore &store)
{
    std::vector<NativeRdb::ValueObject> params = {};
    params.push_back(NativeRdb::ValueObject(std::to_string(PhotoAlbumType::SMART)));
    params.push_back(NativeRdb::ValueObject(std::to_string(PhotoAlbumSubType::CLASSIFY)));
    params.push_back(NativeRdb::ValueObject(newAlbumName));
    int32_t ret = ExecSqlWithRetry([&]() {
        return store.ExecuteSql(SQL_CREATE_CLASSIFY_ALBUM, params);
    });
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, ret,
        "Media_Restore: execute CreateClassifyAlbumSql failed, sql: %{public}s",
        SQL_CREATE_CLASSIFY_ALBUM.c_str());
    return ret;
}

void MediaLibraryDbUpgrade::ProcessClassifyAlbum(const std::string &newAlbumName,
    const std::vector<std::string> &oriAlbumNames, NativeRdb::RdbStore &store, bool &isSetAggregateBitSecond)
{
    CHECK_AND_RETURN_INFO_LOG(!CheckClassifyAlbumExist(newAlbumName, store, isSetAggregateBitSecond),
        "Media_Restore: classify album: %{public}s already exist.", newAlbumName.c_str());
    int32_t ret = CreateClassifyAlbum(newAlbumName, store);
    CHECK_AND_RETURN_LOG(ret == NativeRdb::E_OK,
        "create classify album: %{public}s failed", newAlbumName.c_str());
    std::string subLabels = BuildInClause(oriAlbumNames);
    CHECK_AND_RETURN_LOG(subLabels != "()", "not meet query criteria");
    std::string insertMappingSql = SQL_INSERT_MAPPING_RESULT + subLabels + ";";
    std::vector<NativeRdb::ValueObject> params = {};
    params.push_back(NativeRdb::ValueObject(newAlbumName));
    params.push_back(NativeRdb::ValueObject(std::to_string(PhotoAlbumType::SMART)));
    params.push_back(NativeRdb::ValueObject(std::to_string(PhotoAlbumSubType::CLASSIFY)));
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    ret = ExecSqlWithRetry([&]() { return store.ExecuteSql(insertMappingSql, params); });
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK,
        "Media_Restore: execute insertMappingSql failed, sql: %{public}s", insertMappingSql.c_str());
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("ProcessClassifyAlbum cost: %{public}" PRId64, endTime - startTime);
}

void MediaLibraryDbUpgrade::ProcessOcrClassifyAlbum(const std::string &newAlbumName,
    const std::vector<std::string> &ocrText, NativeRdb::RdbStore &store)
{
    bool isSetAggregateBit;
    CHECK_AND_RETURN_INFO_LOG(!CheckClassifyAlbumExist(newAlbumName, store, isSetAggregateBit),
        "Media_Restore: classify album: %{public}s already exist.", newAlbumName.c_str());
    int32_t ret = CreateClassifyAlbum(newAlbumName, store);
    CHECK_AND_RETURN_LOG(ret == NativeRdb::E_OK,
        "create classify album: %{public}s failed", newAlbumName.c_str());

    std::string subOcrSql = "";
    for (size_t i = 0; i < ocrText.size(); i++) {
        subOcrSql += "tab_analysis_ocr.ocr_text LIKE '%" + ocrText[i] + "%'";
        if (i != ocrText.size() - 1) {
            subOcrSql += " AND ";
        }
    }
    std::string selectOcrSql = SQL_SELECT_CLASSIFY_OCR + subOcrSql + ")) ";
    std::string insertMappingSql = selectOcrSql +
        "INSERT INTO AnalysisPhotoMap (map_album, map_asset) "
        "SELECT album_id, map_asset FROM TempResult;";
    std::vector<NativeRdb::ValueObject> params = {};
    params.push_back(NativeRdb::ValueObject(newAlbumName));
    params.push_back(NativeRdb::ValueObject(std::to_string(PhotoAlbumType::SMART)));
    params.push_back(NativeRdb::ValueObject(std::to_string(PhotoAlbumSubType::CLASSIFY)));
    params.push_back(NativeRdb::ValueObject(std::to_string(static_cast<int32_t>(PhotoLabel::ID_CARD))));
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    ret = ExecSqlWithRetry([&]() { return store.ExecuteSql(insertMappingSql, params); });
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK,
        "Media_Restore: execute insertMappingSql failed, sql: %{public}s", insertMappingSql.c_str());
    
    std::string updateSql = selectOcrSql +
        "UPDATE tab_analysis_label SET sub_label = "
        "CASE WHEN sub_label = '[]' THEN '[" + newAlbumName + "]' "
        "ELSE SUBSTR(sub_label,1,LENGTH(sub_label)-1)||'," + newAlbumName + "]' END "
        "WHERE file_id IN(SELECT map_asset FROM TempResult);";
    ret = ExecSqlWithRetry([&]() { return store.ExecuteSql(updateSql, params); });
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK,
        "Media_Restore: execute updateSql failed, sql: %{public}s", updateSql.c_str());
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("ProcessOcrClassifyAlbum cost: %{public}" PRId64, endTime - startTime);
}

void MediaLibraryDbUpgrade::AggregateClassifyAlbum(NativeRdb::RdbStore &store)
{
    MEDIA_INFO_LOG("Media_Restore: MediaLibraryDbUpgrade::AggregateClassifyAlbum start");
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    std::unordered_map<std::string, std::vector<std::string>> newAlbumMaps;
    bool isSetAggregateBitSecond = false;
    GetAggregateMap(newAlbumMaps);
    for (const auto &pair : newAlbumMaps) {
        std::string newAlbumName = pair.first;
        std::vector<std::string> oriAlbumNames = newAlbumMaps[newAlbumName];
        ProcessClassifyAlbum(newAlbumName, oriAlbumNames, store, isSetAggregateBitSecond);
    }
    int32_t bitPosition = 1;
    CHECK_AND_EXECUTE(!isSetAggregateBitSecond, SetAggregateBit(bitPosition));
    for (const auto &pair : OCR_AGGREGATE_MAPPING_TABLE) {
        int32_t newAlbum = static_cast<int32_t>(pair.first);
        std::vector<std::string> ocrText = pair.second;
        ProcessOcrClassifyAlbum(std::to_string(newAlbum), ocrText, store);
    }
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("Media_Restore: MediaLibraryDbUpgrade::AggregateClassifyAlbum end, cost: %{public}" PRId64,
        endTime - startTime);
}

int32_t MediaLibraryDbUpgrade::MergeAlbumFromOldBundleNameToNewBundleName(NativeRdb::RdbStore &store)
{
    MEDIA_INFO_LOG("Media_Restore: MediaLibraryDbUpgrade::MergeAlbumFromOldBundleNameToNewBundleName start");
    int32_t ret = NativeRdb::E_OK;
    std::vector<NativeRdb::ValueObject> args;
    for (auto &executeSql : SQL_MERGE_ALBUM_FROM_OLD_BUNDLE_NAME_TO_NEW_BUNDLE_NAME) {
        ret = ExecSqlWithRetry([&]() { return store.ExecuteSql(executeSql); });
        CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, ret,
            "Media_Restore: executeSql failed, sql: %{public}s", executeSql.c_str());
    }
    MEDIA_INFO_LOG("Media_Restore: MediaLibraryDbUpgrade::MergeAlbumFromOldBundleNameToNewBundleName end");
    return NativeRdb::E_OK;
}

int32_t MediaLibraryDbUpgrade::UpgradePhotosBelongsToAlbum(NativeRdb::RdbStore &store)
{
    MEDIA_INFO_LOG("Media_Restore: MediaLibraryDbUpgrade::UpgradePhotosBelongsToAlbum start");
    int32_t ret = NativeRdb::E_OK;
    std::vector<NativeRdb::ValueObject> args;
    for (const auto &executeSql : this->SQL_PHOTOS_NEED_TO_BELONGS_TO_ALBUM) {
        ret = ExecSqlWithRetry([&]() { return store.ExecuteSql(executeSql); });
        CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, ret,
            "Media_Restore: executeSql failed, sql: %{public}s", executeSql.c_str());
    }
    MEDIA_INFO_LOG("Media_Restore: MediaLibraryDbUpgrade::UpgradePhotosBelongsToAlbum end");
    return NativeRdb::E_OK;
}

/**
 * @brief Upgrade album_plugin table.
 */
int32_t MediaLibraryDbUpgrade::UpgradeAlbumPlugin(NativeRdb::RdbStore &store)
{
    CHECK_AND_RETURN_RET(!(this->dbUpgradeUtils_.IsTableExists(store, "album_plugin")), NativeRdb::E_OK);
    AlbumPluginTableEventHandler handler;
    return handler.OnUpgrade(store, 0, 0);
}

/**
 * @brief Upgrade PhotoAlbum table.
 */
int32_t MediaLibraryDbUpgrade::UpgradePhotoAlbum(NativeRdb::RdbStore &store)
{
    int32_t ret = NativeRdb::E_OK;
    ret = this->dbUpgradeUtils_.DropAllTriggers(store, "PhotoAlbum");
    CHECK_AND_RETURN_RET(ret == NativeRdb::E_OK, ret);

    ret = this->dbUpgradeUtils_.DropAllUniqueIndex(store, "PhotoAlbum");
    CHECK_AND_RETURN_RET(ret == NativeRdb::E_OK, ret);

    ret = this->AddlPathColumn(store);
    CHECK_AND_RETURN_RET(ret == NativeRdb::E_OK, ret);
    ret = this->UpdatelPathColumn(store);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "Media_Restore: Update lpath column failed, ret=%{public}d", ret);
    ret = this->UpgradePhotoAlbumAddUploadStatusColumn(store);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "Media_Restore: Add upload_status column failed, ret=%{public}d", ret);
    ret = this->AddPhotoAlbumChangeTime(store);
    CHECK_AND_RETURN_RET(ret == NativeRdb::E_OK, ret);
    return ret;
}

/**
 * @brief Upgrade the lpath column in PhotoAlbum table.
 */
int32_t MediaLibraryDbUpgrade::UpdatelPathColumn(NativeRdb::RdbStore &store)
{
    std::string executeSql = this->SQL_PHOTO_ALBUM_TABLE_UPDATE_LPATH_COLUMN;
    std::vector<NativeRdb::ValueObject> args;
    int32_t ret = ExecSqlWithRetry([&]() { return store.ExecuteSql(executeSql); });
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, ret,
        "Media_Restore: MediaLibraryDbUpgrade::UpdatelPathColumn failed, ret=%{public}d, sql=%{public}s",
        ret, executeSql.c_str());
    MEDIA_INFO_LOG("Media_Restore: MediaLibraryDbUpgrade::UpdatelPathColumn success");
    return ret;
}

/**
 * @brief Add owner_album_id column to Photos table.
 */
int32_t MediaLibraryDbUpgrade::AddOwnerAlbumIdColumn(NativeRdb::RdbStore &store)
{
    CHECK_AND_RETURN_RET(!this->dbUpgradeUtils_.IsColumnExists(store, "Photos", "owner_album_id"), NativeRdb::E_OK);
    std::vector<NativeRdb::ValueObject> args;
    std::string sql = this->SQL_PHOTOS_TABLE_ADD_OWNER_ALBUM_ID;
    int32_t ret = ExecSqlWithRetry([&]() { return store.ExecuteSql(sql, args); });
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK,
        "Media_Restore: MediaLibraryDbUpgrade::AddOwnerAlbumIdColumn failed, ret=%{public}d, sql=%{public}s",
        ret, sql.c_str());
    MEDIA_INFO_LOG("Media_Restore: MediaLibraryDbUpgrade::AddOwnerAlbumIdColumn success");
    return ret;
}

/**
 * @brief Add file_source_type column to Photos table.
 */
int32_t MediaLibraryDbUpgrade::AddFileSourceTypeColumn(NativeRdb::RdbStore &store)
{
    CHECK_AND_RETURN_RET(!this->dbUpgradeUtils_.IsColumnExists(store, "Photos", "file_source_type"), NativeRdb::E_OK);
    std::string sql = this->SQL_PHOTOS_TABLE_ADD_FILE_SOURCE_TYPE;
    int32_t ret = ExecSqlWithRetry([&]() { return store.ExecuteSql(sql); });
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK,
        "Media_Restore: MediaLibraryDbUpgrade::AddFileSourceTypeColumn failed, ret=%{public}d, sql=%{public}s",
        ret, sql.c_str());
    MEDIA_INFO_LOG("Media_Restore: MediaLibraryDbUpgrade::AddFileSourceTypeColumn success");
    return ret;
}

/**
 * @brief Add lpath column to PhotoAlbum table.
 */
int32_t MediaLibraryDbUpgrade::AddlPathColumn(NativeRdb::RdbStore &store)
{
    CHECK_AND_RETURN_RET(!(this->dbUpgradeUtils_.IsColumnExists(store, "PhotoAlbum", "lpath")), NativeRdb::E_OK);
    std::vector<NativeRdb::ValueObject> args;
    std::string sql = this->SQL_PHOTO_ALBUM_TABLE_ADD_LPATH_COLUMN;
    return ExecSqlWithRetry([&]() { return store.ExecuteSql(sql, args); });
}

/**
 * @brief Move the single relationship from PhotoMap table to Photos table, stored in owner_album_id.
 */
int32_t MediaLibraryDbUpgrade::MoveSingleRelationshipToPhotos(NativeRdb::RdbStore &store)
{
    std::vector<std::string> executeSqls = {this->SQL_TEMP_PHOTO_MAP_TABLE_DROP, this->SQL_TEMP_PHOTO_MAP_TABLE_CREATE};
    executeSqls.insert(executeSqls.end(),
        this->SQL_TEMP_PHOTO_MAP_TABLE_CREATE_INDEX_ARRAY.begin(),
        this->SQL_TEMP_PHOTO_MAP_TABLE_CREATE_INDEX_ARRAY.end());
    executeSqls.push_back(this->SQL_PHOTOS_TABLE_UPDATE_ALBUM_ID);
    executeSqls.push_back(this->SQL_PHOTO_MAP_TABLE_DELETE_SINGLE_RELATIONSHIP);
    executeSqls.push_back(this->SQL_TEMP_PHOTO_MAP_TABLE_DROP);
    int ret = NativeRdb::E_OK;
    MEDIA_INFO_LOG("MoveSingleRelationshipToPhotos begin");
    auto [errCode, transaction] = store.CreateTransaction(OHOS::NativeRdb::Transaction::DEFERRED);
    DfxTransaction reporter{__func__};
    if (errCode != NativeRdb::E_OK || transaction == nullptr) {
        reporter.ReportError(DfxTransaction::AbnormalType::CREATE_ERROR, errCode);
        MEDIA_ERR_LOG("transaction failed, err:%{public}d", errCode);
        return errCode;
    }
    for (const std::string &executeSql : executeSqls) {
        auto res = transaction->Execute(executeSql);
        ret = res.first;
        if (ret != NativeRdb::E_OK) {
            reporter.ReportError(DfxTransaction::AbnormalType::EXECUTE_ERROR, ret);
            MEDIA_ERR_LOG("Media_Restore: MoveSingleRelationshipToPhotos failed, ret=%{public}d, sql=%{public}s",
                ret,
                executeSql.c_str());
            transaction->Rollback();
            return ret;
        }
    }
    ret = transaction->Commit();
    if (ret != NativeRdb::E_OK) {
        reporter.ReportError(DfxTransaction::AbnormalType::COMMIT_ERROR, ret);
        MEDIA_ERR_LOG("MoveSingleRelationshipToPhotos: tans finish fail!, ret:%{public}d", ret);
        transaction->Rollback();
    } else {
        reporter.ReportIfTimeout();
    }
    return NativeRdb::E_OK;
}

/**
 * @brief Add change_time column to Photos table.
 */
int32_t MediaLibraryDbUpgrade::AddPhotosChangeTime(NativeRdb::RdbStore &store)
{
    CHECK_AND_RETURN_RET(!this->dbUpgradeUtils_.IsColumnExists(store, "Photos", "change_time"), NativeRdb::E_OK);
    int32_t ret = NativeRdb::E_OK;
    for (const auto &executeSql : this->SQL_PHOTOS_CHANGE_TIME) {
        ret = ExecSqlWithRetry([&]() { return store.ExecuteSql(executeSql); });
        CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, ret,
            "Media_Restore: AddPhotosChangeTime failed, sql: %{public}s", executeSql.c_str());
    }
    return NativeRdb::E_OK;
}

/**
 * @brief Add change_time column to PhotoAlbum table.
 */
int32_t MediaLibraryDbUpgrade::AddPhotoAlbumChangeTime(NativeRdb::RdbStore &store)
{
    CHECK_AND_RETURN_RET(!this->dbUpgradeUtils_.IsColumnExists(store, "PhotoAlbum", "change_time"), NativeRdb::E_OK);
    int32_t ret = NativeRdb::E_OK;
    for (const auto &executeSql : this->SQL_PHOTO_ALBUM_CHANGE_TIME) {
        ret = ExecSqlWithRetry([&]() { return store.ExecuteSql(executeSql); });
        CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, ret,
            "Media_Restore: AddPhotoAlbumChangeTime failed, sql: %{public}s", executeSql.c_str());
    }
    return NativeRdb::E_OK;
}

/**
 * @brief Upgrade Photos table.
 */
int32_t MediaLibraryDbUpgrade::UpgradePhotos(NativeRdb::RdbStore &store)
{
    int32_t ret = NativeRdb::E_OK;
    ret = this->dbUpgradeUtils_.DropAllTriggers(store, "Photos");
    CHECK_AND_RETURN_RET(ret == NativeRdb::E_OK, ret);

    ret = this->AddOwnerAlbumIdColumn(store);
    CHECK_AND_RETURN_RET(ret == NativeRdb::E_OK, ret);

    ret = this->AddFileSourceTypeColumn(store);
    CHECK_AND_RETURN_RET(ret == NativeRdb::E_OK, ret);
    ret = this->AddPhotosEffectModeColumn(store);
    CHECK_AND_RETURN_RET(ret == NativeRdb::E_OK, ret);

    ret = this->AddPhotosChangeTime(store);
    CHECK_AND_RETURN_RET(ret == NativeRdb::E_OK, ret);

    ret = ExecSqlWithRetry([&]() { return PhotoDayMonthYearOperation::UpdatePhotosDate(store); });
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, ret, "update photos date failed, ret=%{public}d", ret);

    return NativeRdb::E_OK;
}

/**
 * @brief Upgrade PhotoMap table.
 */
int32_t MediaLibraryDbUpgrade::UpgradePhotoMap(NativeRdb::RdbStore &store)
{
    int32_t ret = NativeRdb::E_OK;
    ret = this->dbUpgradeUtils_.DropAllTriggers(store, "PhotoMap");
    CHECK_AND_RETURN_RET(ret == NativeRdb::E_OK, ret);

    ret = this->MoveSingleRelationshipToPhotos(store);
    CHECK_AND_RETURN_RET(ret == NativeRdb::E_OK, ret);
    return NativeRdb::E_OK;
}

/**
 * @brief Upgrade PhotoAlbum table.
 */
int32_t MediaLibraryDbUpgrade::UpgradePhotoAlbumAddUploadStatusColumn(NativeRdb::RdbStore &store)
{
    CHECK_AND_RETURN_RET(
        !(this->dbUpgradeUtils_.IsColumnExists(store, "PhotoAlbum", "upload_status")), NativeRdb::E_OK);
    return ExecSqlWithRetry([&]() { return store.ExecuteSql(this->SQL_PHOTO_ALBUM_TABLE_ADD_UPDATE_STATUS_COLUMN); });
}

int32_t MediaLibraryDbUpgrade::AddPhotosEffectModeColumn(NativeRdb::RdbStore &store)
{
    CHECK_AND_RETURN_RET(
        !this->dbUpgradeUtils_.IsColumnExists(store, "Photos", PhotoColumn::MOVING_PHOTO_EFFECT_MODE), NativeRdb::E_OK);
    return ExecSqlWithRetry([&]() { return store.ExecuteSql(SQL_PHOTOS_TABLE_ADD_EFFECT_MODE); });
}
// LCOV_EXCL_STOP
}  // namespace DataTransfer
}  // namespace OHOS::Media