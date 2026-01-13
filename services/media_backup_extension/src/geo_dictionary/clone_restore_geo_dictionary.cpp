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
#include "clone_restore_geo_dictionary.h"

#include "backup_database_utils.h"
#include "medialibrary_data_manager_utils.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "result_set_utils.h"
#include "locale_config.h"
#include "upgrade_restore_task_report.h"
#include "medialibrary_unistore_manager.h"

namespace OHOS::Media {
const int32_t PAGE_SIZE = 200;
const string NOT_MATCH = "NOT MATCH";
const string CITY_ID = "city_id";
const string LANGUAGE = "language";
const string CITY_NAME = "city_name";

const string TEXT = "TEXT";
const string SINGLE_CH = "zh-Hans";
const string GEO_DICTIONARY_TABLE = "tab_analysis_geo_dictionary";

const unordered_map<string, unordered_set<string>> COMPARED_COLUMNS_MAP = {
    { "tab_analysis_geo_dictionary",
        {
            "city_id",
            "language",
            "city_name"
        }
    }
};

template<typename Key, typename Value>
Value GetValueFromMap(const unordered_map<Key, Value> &map, const Key &key, const Value &defaultValue = Value())
{
    auto it = map.find(key);
    CHECK_AND_RETURN_RET(it == map.end(), it->second);
    return defaultValue;
}

void CloneRestoreGeoDictionary::Init(int32_t sceneCode, const std::string &taskId,
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb, std::shared_ptr<NativeRdb::RdbStore> mediaRdb)
{
    sceneCode_ = sceneCode;
    taskId_ = taskId;
    mediaLibraryRdb_ = mediaLibraryRdb;
    mediaRdb_ = mediaRdb;
    successInsertCnt_ = 0;
    failInsertCnt_ = 0;
    systemLanguage_ = Global::I18n::LocaleConfig::GetSystemLanguage();
}

void CloneRestoreGeoDictionary::RestoreAlbums()
{
    bool cond = (mediaRdb_ == nullptr || mediaLibraryRdb_ == nullptr);
    CHECK_AND_RETURN_LOG(!cond, "rdbStore is nullptr");
    MEDIA_INFO_LOG("restore geo dictionary albums start.");
    GetGeoDictionaryInfos();
    InsertIntoGeoDictionaryAlbums();
}

void CloneRestoreGeoDictionary::GetGeoDictionaryInfos()
{
    std::unordered_map<std::string, std::string> columns;
    columns[CITY_ID] = TEXT;
    columns[LANGUAGE] = TEXT;
    bool hasRequiredColumns = CheckTableColumns(GEO_DICTIONARY_TABLE, columns);
    if (!hasRequiredColumns) {
        MEDIA_ERR_LOG("The tab_analysis_geo_dictionary does not contain the required columns.");
        ErrorInfo errorInfo(RestoreError::TABLE_LACK_OF_COLUMN, static_cast<int32_t>(columns.size()),
            "", "The tab_analysis_geo_dictionary does not contain city_id or language");
        UpgradeRestoreTaskReport().SetSceneCode(this->sceneCode_).SetTaskId(this->taskId_).ReportError(errorInfo);
        return;
    }
    const std::string querySql = "SELECT * FROM " + GEO_DICTIONARY_TABLE + " LIMIT ?, ?";
    int32_t rowCount = 0;
    int32_t offset = 0;
    do {
        std::vector<NativeRdb::ValueObject> params = {offset, PAGE_SIZE};
        auto resultSet = BackupDatabaseUtils::QuerySql(mediaRdb_, querySql, params);
        if (resultSet == nullptr) {
            MEDIA_ERR_LOG("Query resultSql is null.");
            break;
        }
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            GeoDictionaryCloneInfo info;
            GetGeoDictionaryInfo(info, resultSet);
            geoDictionaryInfos_.emplace_back(info);
        }
        resultSet->GetRowCount(rowCount);
        offset += PAGE_SIZE;
        resultSet->Close();
    } while (rowCount > 0);
    MEDIA_INFO_LOG("query tab_analysis_geo_dictionary nums: %{public}zu", geoDictionaryInfos_.size());
}

void CloneRestoreGeoDictionary::InsertIntoGeoDictionaryAlbums()
{
    GeoDictionaryDeduplicate();
    std::unordered_set<std::string> intersection = GetCommonColumns(GEO_DICTIONARY_TABLE);
    size_t offset = 0;
    do {
        std::vector<NativeRdb::ValuesBucket> values;
        for (size_t index = 0; index < PAGE_SIZE && index + offset < geoDictionaryInfos_.size(); index++) {
            if (!geoDictionaryInfos_[index + offset].cityId.has_value() ||
                !geoDictionaryInfos_[index + offset].language.has_value() ||
                !geoDictionaryInfos_[index + offset].cityName.has_value()) {
                continue;
            }
            NativeRdb::ValuesBucket value;
            GetGeoDictionaryInsertValue(value, geoDictionaryInfos_[index + offset], intersection);
            values.emplace_back(value);
        }
        int64_t rowNum = 0;
        int32_t errCode = BatchInsertWithRetry(GEO_DICTIONARY_TABLE, values, rowNum);
        if (errCode != E_OK || rowNum != static_cast<long>(values.size())) {
            int32_t failNums = static_cast<long>(values.size()) - rowNum;
            MEDIA_ERR_LOG("insert into tab_analysis_geo_dictionary fail, num: %{public}d", failNums);
            ErrorInfo errorInfo(RestoreError::INSERT_FAILED, 0, std::to_string(errCode),
                "insert into tab_analysis_geo_dictionary fail, num:" + std::to_string(failNums));
            failInsertCnt_ +=  failNums;
            UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).ReportError(errorInfo);
        }
        offset += PAGE_SIZE;
        successInsertCnt_ += rowNum;
    } while (offset < geoDictionaryInfos_.size());
}

void CloneRestoreGeoDictionary::GeoDictionaryDeduplicate()
{
    const std::string querySql = "SELECT * FROM " + GEO_DICTIONARY_TABLE + " LIMIT ?, ?";
    int32_t rowCount = 0;
    int32_t offset = 0;
    do {
        std::vector<NativeRdb::ValueObject> params = {offset, PAGE_SIZE};
        auto resultSet = BackupDatabaseUtils::QuerySql(mediaLibraryRdb_, querySql, params);
        if (resultSet == nullptr) {
            MEDIA_ERR_LOG("Query resultSql is null.");
            break;
        }
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            GeoDictionaryCloneInfo info;
            GetGeoDictionaryInfo(info, resultSet);
            dstGeoDictionaryInfos_.emplace_back(info);
        }
        resultSet->GetRowCount(rowCount);
        offset += PAGE_SIZE;
        resultSet->Close();
    } while (rowCount > 0);

    auto itr = geoDictionaryInfos_.begin();
    while (itr != geoDictionaryInfos_.end()) {
        GeoDictionaryCloneInfo info = *itr;
        auto it = std::find_if(dstGeoDictionaryInfos_.begin(), dstGeoDictionaryInfos_.end(),
            [info](const GeoDictionaryCloneInfo& dstInfo) {
                return info.cityId == dstInfo.cityId;
            });
        if (it != dstGeoDictionaryInfos_.end()) {
            itr = geoDictionaryInfos_.erase(itr);
        } else {
            ++itr;
        }
    }
}

bool CloneRestoreGeoDictionary::CheckTableColumns(const std::string& tableName,
    std::unordered_map<std::string, std::string>& columns)
{
    std::unordered_map<std::string, std::string> srcColumnInfoMap =
        BackupDatabaseUtils::GetColumnInfoMap(mediaRdb_, tableName);
    for (auto it = columns.begin(); it != columns.end(); ++it) {
        if (srcColumnInfoMap.find(it->first) != srcColumnInfoMap.end()) {
            continue;
        }
        return false;
    }
    return true;
}

std::unordered_set<std::string> CloneRestoreGeoDictionary::GetCommonColumns(const string &tableName)
{
    std::unordered_map<std::string, std::string> srcColumnInfoMap =
        BackupDatabaseUtils::GetColumnInfoMap(mediaRdb_, tableName);
    std::unordered_map<std::string, std::string> dstColumnInfoMap =
        BackupDatabaseUtils::GetColumnInfoMap(mediaLibraryRdb_, tableName);
    std::unordered_set<std::string> result;
    auto comparedColumns = GetValueFromMap(COMPARED_COLUMNS_MAP, tableName);
    for (auto it = dstColumnInfoMap.begin(); it != dstColumnInfoMap.end(); ++it) {
        if (srcColumnInfoMap.find(it->first) != srcColumnInfoMap.end() && comparedColumns.count(it->first) > 0) {
            result.insert(it->first);
        }
    }
    return result;
}

void CloneRestoreGeoDictionary::GetGeoDictionaryInfo(GeoDictionaryCloneInfo &info,
    std::shared_ptr<NativeRdb::ResultSet> resultSet)
{
    info.cityId = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, CITY_ID);
    info.language = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, LANGUAGE);
    info.cityName = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, CITY_NAME);
}

void CloneRestoreGeoDictionary::GetGeoDictionaryInsertValue(NativeRdb::ValuesBucket &value,
    const GeoDictionaryCloneInfo &info, const std::unordered_set<std::string> &intersection)
{
    PutIfInIntersection(value, CITY_ID, info.cityId, intersection);
    PutIfInIntersection(value, LANGUAGE, info.language, intersection);
    PutIfInIntersection(value, CITY_NAME, info.cityName, intersection);
}

int32_t CloneRestoreGeoDictionary::BatchInsertWithRetry(const std::string &tableName,
    std::vector<NativeRdb::ValuesBucket> &values, int64_t &rowNum)
{
    CHECK_AND_RETURN_RET(!values.empty(), E_OK);
    int32_t errCode = E_ERR;
    TransactionOperations trans{ __func__ };
    trans.SetBackupRdbStore(mediaLibraryRdb_);
    std::function<int(void)> func = [&]()->int {
        errCode = trans.BatchInsert(rowNum, tableName, values);
        CHECK_AND_PRINT_LOG(errCode == E_OK, "InsertSql failed, errCode: %{public}d,"
            " rowNum: %{public}ld.", errCode, (long)rowNum);
        return errCode;
    };
    errCode = trans.RetryTrans(func, true);
    CHECK_AND_PRINT_LOG(errCode == E_OK, "BatchInsertWithRetry: tans finish fail!, ret:%{public}d", errCode);
    return errCode;
}

void CloneRestoreGeoDictionary::ReportGeoRestoreTask()
{
    const int32_t GEO_STATUS_SUCCESS = 1;
    MEDIA_INFO_LOG("GeoDictionary Insert successInsertCnt_: %{public}d, failInsertCnt_: %{public}d, "
        "Current System Language: %{public}s",
        successInsertCnt_.load(), failInsertCnt_.load(), systemLanguage_.c_str());
    UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_)
        .Report("GeoDictionary Restore", std::to_string(GEO_STATUS_SUCCESS),
        "successInsertCnt_: " + std::to_string(successInsertCnt_) +
        ", failInsertCnt_: " + std::to_string(failInsertCnt_) +
        ", Current System Language: " + systemLanguage_);
}
} // namespace OHOS::Media