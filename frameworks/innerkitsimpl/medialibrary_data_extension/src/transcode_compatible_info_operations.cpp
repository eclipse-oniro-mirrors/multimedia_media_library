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

#include "transcode_compatible_info_operations.h"

#include <list>
#include <mutex>
#include <unordered_map>

#include "rdb_store.h"
#include "rdb_errno.h"
#include "rdb_predicates.h"
#include "value_object.h"
#include "medialibrary_errno.h"
#include "media_log.h"
#include "media_compatible_info_column.h"
#include "result_set_utils.h"

using namespace std;
using namespace OHOS::NativeRdb;
using namespace OHOS::Media;

const string TranscodeCompatibleInfoOperation::ENCODINGS_SEPARATOR = ",";
constexpr int32_t INVALID_HIGH_RESOLUTION = -1;
std::unordered_map<std::string, CompatibleInfo> TranscodeCompatibleInfoOperation::compatibleInfoCache_ = {};

string TranscodeCompatibleInfoOperation::VectorToString(const std::vector<std::string> &vec)
{
    if (vec.empty()) {
        return "";
    }

    string result;
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0) {
            result += ENCODINGS_SEPARATOR;
        }
        result += vec[i];
    }
    return result;
}

std::vector<std::string> TranscodeCompatibleInfoOperation::StringToVector(const std::string &str)
{
    if (str.empty()) {
        return {};
    }

    vector<string> result;
    string current;

    for (char c: str) {
        if (c == ENCODINGS_SEPARATOR[0]) {
            if (!current.empty()) {
                result.push_back(current);
            }
            current.clear();
        } else {
            current += c;
        }
    }

    if (!current.empty()) {
        result.push_back(current);
    }
    return result;
}

int32_t TranscodeCompatibleInfoOperation::InsertCompatibleInfo(CompatibleInfo& compatibleInfo)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "rdbStore is null");
    CHECK_AND_RETURN_RET_LOG(!compatibleInfo.bundleName.empty(), E_INVALID_ARGUMENTS,
        "bundleName is empty");

    string sql = "INSERT OR REPLACE INTO " + TabCompatibleInfoColumn::TABLE + " (" +
                    TabCompatibleInfoColumn::BUNDLE_NAME + ", " +
                    TabCompatibleInfoColumn::HIGH_RESOLUTION + ", " +
                    TabCompatibleInfoColumn::ENCODINGS + ", " +
                    TabCompatibleInfoColumn::PREFERRED_COMPATIBLE_MODE + ") VALUES (?, ?, ?, ?)";

    vector<NativeRdb::ValueObject> values = {
        NativeRdb::ValueObject(compatibleInfo.bundleName),
        NativeRdb::ValueObject(to_string(compatibleInfo.highResolution)),
        NativeRdb::ValueObject(VectorToString(compatibleInfo.encodings)),
        NativeRdb::ValueObject(static_cast<int32_t>(compatibleInfo.preferredCompatibleMode))
    };
    int32_t ret = rdbStore->ExecuteSql(sql, values);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_DB_FAIL,
        "Insert compatibleInfo failed, ret : %{public}d", ret);
    {
        lock_guard<mutex> lock(compatibleInfoCacheMutex_);
        compatibleInfoCache_.erase(compatibleInfo.bundleName);
    }
    MEDIA_INFO_LOG("Insert compatibleInfo success");
    return E_OK;
}

int32_t TranscodeCompatibleInfoOperation::UpdataCompatibleInfo(CompatibleInfo& compatibleInfo)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "rdbStore is null");
    CHECK_AND_RETURN_RET_LOG(!compatibleInfo.bundleName.empty(), E_INVALID_ARGUMENTS,
        "bundleName is empty");
    
    ValuesBucket values;
    values.PutInt(TabCompatibleInfoColumn::HIGH_RESOLUTION, compatibleInfo.highResolution ? 1 : 0);
    values.PutString(TabCompatibleInfoColumn::ENCODINGS, VectorToString(compatibleInfo.encodings));

    AbsRdbPredicates predicates(TabCompatibleInfoColumn::TABLE);
    predicates.EqualTo(TabCompatibleInfoColumn::BUNDLE_NAME, compatibleInfo.bundleName);

    int32_t changedRows;
    int32_t ret = rdbStore->Update(changedRows, values, predicates);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_DB_FAIL,
        "Update compatibleInfo failed, ret : %{public}d", ret);
    
    if (changedRows == 0) {
        MEDIA_ERR_LOG("Update compatibleInfo no rows affected");
        return E_DB_FAIL;
    }

    MEDIA_INFO_LOG("Update compatibleInfo success");
    return E_OK;
}

int32_t TranscodeCompatibleInfoOperation::UpsertCompatibleInfo(const std::string &bundleName, bool highResolution,
    const std::vector<std::string> &encodings)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "rdbStore is null");
    CHECK_AND_RETURN_RET_LOG(!bundleName.empty(), E_INVALID_ARGUMENTS, "bundleName is empty");

    string sql = "INSERT INTO " + TabCompatibleInfoColumn::TABLE + " (" +
                    TabCompatibleInfoColumn::BUNDLE_NAME + ", " +
                    TabCompatibleInfoColumn::HIGH_RESOLUTION + ", " +
                    TabCompatibleInfoColumn::ENCODINGS + ", " +
                    TabCompatibleInfoColumn::PREFERRED_COMPATIBLE_MODE + ") VALUES (?, ?, ?, ?) " +
                    "ON CONFLICT(" + TabCompatibleInfoColumn::BUNDLE_NAME + ") DO UPDATE SET " +
                    TabCompatibleInfoColumn::HIGH_RESOLUTION + " = excluded." +
                    TabCompatibleInfoColumn::HIGH_RESOLUTION + ", " +
                    TabCompatibleInfoColumn::ENCODINGS + " = CASE WHEN excluded." +
                    TabCompatibleInfoColumn::ENCODINGS + " = '' THEN " +
                    TabCompatibleInfoColumn::TABLE + "." + TabCompatibleInfoColumn::ENCODINGS +
                    " ELSE excluded." + TabCompatibleInfoColumn::ENCODINGS + " END";

    vector<NativeRdb::ValueObject> values = {
        NativeRdb::ValueObject(bundleName),
        NativeRdb::ValueObject(highResolution ? 1 : 0),
        NativeRdb::ValueObject(VectorToString(encodings)),
        NativeRdb::ValueObject(static_cast<int32_t>(PreferredCompatibleMode::DEFAULT))
    };
    int32_t ret = rdbStore->ExecuteSql(sql, values);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_DB_FAIL,
        "Upsert compatibleInfo failed, ret : %{public}d", ret);

    CompatibleInfo compatibleInfo;
    compatibleInfo.bundleName = bundleName;
    compatibleInfo.highResolution = highResolution;
    compatibleInfo.encodings = encodings;
    {
        lock_guard<mutex> lock(compatibleInfoCacheMutex_);
        compatibleInfoCache_.erase(bundleName);
    }
    MEDIA_INFO_LOG("Upsert compatibleInfo success");
    return E_OK;
}

int32_t TranscodeCompatibleInfoOperation::UpsertPreferredCompatibleMode(const std::string &bundleName,
    PreferredCompatibleMode preferredCompatibleMode)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "rdbStore is null");
    CHECK_AND_RETURN_RET_LOG(!bundleName.empty(), E_INVALID_ARGUMENTS, "bundleName is empty");

    string sql = "INSERT INTO " + TabCompatibleInfoColumn::TABLE + " (" +
                    TabCompatibleInfoColumn::BUNDLE_NAME + ", " +
                    TabCompatibleInfoColumn::HIGH_RESOLUTION + ", " +
                    TabCompatibleInfoColumn::ENCODINGS + ", " +
                    TabCompatibleInfoColumn::PREFERRED_COMPATIBLE_MODE + ") VALUES (?, ?, ?, ?) " +
                    "ON CONFLICT(" + TabCompatibleInfoColumn::BUNDLE_NAME + ") DO UPDATE SET " +
                    TabCompatibleInfoColumn::PREFERRED_COMPATIBLE_MODE + " = excluded." +
                    TabCompatibleInfoColumn::PREFERRED_COMPATIBLE_MODE;

    vector<NativeRdb::ValueObject> values = {
        NativeRdb::ValueObject(bundleName),
        NativeRdb::ValueObject(INVALID_HIGH_RESOLUTION),
        NativeRdb::ValueObject(""),
        NativeRdb::ValueObject(static_cast<int32_t>(preferredCompatibleMode))
    };
    int32_t ret = rdbStore->ExecuteSql(sql, values);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_DB_FAIL,
        "Upsert preferredCompatibleMode failed, ret : %{public}d", ret);

    CompatibleInfo compatibleInfo;
    compatibleInfo.bundleName = bundleName;
    compatibleInfo.preferredCompatibleMode = preferredCompatibleMode;
    {
        lock_guard<mutex> lock(compatibleInfoCacheMutex_);
        compatibleInfoCache_.erase(bundleName);
    }
    MEDIA_INFO_LOG("Upsert preferredCompatibleMode success");
    return E_OK;
}

int32_t TranscodeCompatibleInfoOperation::DeleteCompatibleInfo(const std::string &bundleName)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "rdbStore is null");
    CHECK_AND_RETURN_RET_LOG(!bundleName.empty(), E_INVALID_ARGUMENTS,
        "bundleName is empty");
    
    AbsRdbPredicates predicates(TabCompatibleInfoColumn::TABLE);
    predicates.EqualTo(TabCompatibleInfoColumn::BUNDLE_NAME, bundleName);

    int32_t deletedRows;
    int32_t ret = rdbStore->Delete(deletedRows, predicates);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_DB_FAIL,
        "Delete compatibleInfo failed, ret : %{public}d", ret);
    return E_OK;
}

int32_t TranscodeCompatibleInfoOperation::QueryCompatibleInfo(
    const std::string &bundleName, CompatibleInfo& compatibleInfo)
{
    {
        lock_guard<mutex> lock(compatibleInfoCacheMutex_);
        auto it = compatibleInfoCache_.find(bundleName);
        if (it != compatibleInfoCache_.end()) {
            compatibleInfo = it->second;
            MEDIA_INFO_LOG("Query compatibleInfo success");
            return E_OK;
        }
    }
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "rdbStore is null");
    CHECK_AND_RETURN_RET_LOG(!bundleName.empty(), E_INVALID_ARGUMENTS,
        "bundleName is empty");
    
    AbsRdbPredicates predicates(TabCompatibleInfoColumn::TABLE);
    predicates.EqualTo(TabCompatibleInfoColumn::BUNDLE_NAME, bundleName);

    vector<string> columns = {
        TabCompatibleInfoColumn::BUNDLE_NAME,
        TabCompatibleInfoColumn::HIGH_RESOLUTION,
        TabCompatibleInfoColumn::ENCODINGS,
        TabCompatibleInfoColumn::PREFERRED_COMPATIBLE_MODE
    };

    auto resultSet = rdbStore->Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_DB_FAIL,
        "Query compatibleInfo failed, resultSet is null");

    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_INFO_LOG("Query compatibleInfo not found");
        resultSet->Close();
        return E_OK;
    }

    compatibleInfo.bundleName = GetStringVal(TabCompatibleInfoColumn::BUNDLE_NAME, resultSet);
    compatibleInfo.highResolution = GetInt32Val(TabCompatibleInfoColumn::HIGH_RESOLUTION, resultSet);
    compatibleInfo.encodings = StringToVector(GetStringVal(TabCompatibleInfoColumn::ENCODINGS, resultSet));
    compatibleInfo.preferredCompatibleMode =
        static_cast<PreferredCompatibleMode>(GetInt32Val(TabCompatibleInfoColumn::PREFERRED_COMPATIBLE_MODE,
            resultSet));
    resultSet->Close();
    {
        lock_guard<mutex> lock(compatibleInfoCacheMutex_);
        compatibleInfoCache_.emplace(compatibleInfo.bundleName, compatibleInfo);
    }
    MEDIA_INFO_LOG("Query compatibleInfo success");

    return E_OK;
}