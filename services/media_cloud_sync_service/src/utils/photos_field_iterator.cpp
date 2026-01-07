/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "Media_Cloud_Sync"

#include "photos_field_iterator.h"
#include "media_log.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "rdb_predicates.h"

#include <algorithm>
#include <charconv>
#include <system_error>

namespace OHOS::Media::CloudSync {
std::vector<ColumnInfo> PhotosFieldIterator::all_columns_;
std::once_flag PhotosFieldIterator::init_flag_;
const int SQL_QUOTE_START_OFFSET = 1;
const int SQL_QUOTES_TOTAL_LENGTH_TO_REMOVE = 2;

void PhotosFieldIterator::Initialize()
{
    MEDIA_INFO_LOG("Initializing Photos table schema cache.");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("Failed to get RdbStore for schema initialization.");
        return;
    }
    std::string querySql = "PRAGMA table_info('" + PhotoColumn::PHOTOS_TABLE + "')";
    auto resultSet = rdbStore->QuerySql(querySql);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("PRAGMA table_info query failed for table %{public}s.", PhotoColumn::PHOTOS_TABLE.c_str());
        return;
    }
    int32_t rowCount = 0;
    resultSet->GetRowCount(rowCount);
    if (rowCount == 0) {
        MEDIA_ERR_LOG("No columns found for table %{public}s.", PhotoColumn::PHOTOS_TABLE.c_str());
        resultSet->Close();
        return;
    }
    all_columns_.reserve(rowCount);
    int32_t nameIndex;
    int32_t typeIndex;
    int32_t dfltValueIndex;
    resultSet->GetColumnIndex("name", nameIndex);
    resultSet->GetColumnIndex("type", typeIndex);
    resultSet->GetColumnIndex("dflt_value", dfltValueIndex);
    for (int32_t i = 0; i < rowCount; i++) {
        resultSet->GoToRow(i);
        std::string name;
        std::string type;
        std::string dflt_value;
        resultSet->GetString(nameIndex, name);
        resultSet->GetString(typeIndex, type);
        bool isDefaultNull = false;
        resultSet->IsColumnNull(dfltValueIndex, isDefaultNull);
        if (!isDefaultNull) {
            resultSet->GetString(dfltValueIndex, dflt_value);
        }
        std::transform(type.begin(), type.end(), type.begin(), ::toupper);
        all_columns_.push_back({name, type, dflt_value});
    }

    resultSet->Close();
    MEDIA_INFO_LOG("Cached %{public}zu columns for Photos table.", all_columns_.size());
}

static int64_t SafeStoll(const std::string &str)
{
    int64_t value;
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);

    if (ec != std::errc()) {
        MEDIA_ERR_LOG("Failed to convert string to int64_t: %{public}s", str.c_str());
        return 0;
    }
    return value;
}

void PhotosFieldIterator::PutDefaultValue(NativeRdb::ValuesBucket &values, const ColumnInfo &column)
{
    if (column.dflt_value.empty() || column.dflt_value == "NULL") {
        values.PutNull(column.name);
        return;
    }

    if (column.type.find("INT") != std::string::npos) {
        values.PutLong(column.name, SafeStoll(column.dflt_value));
    } else if (column.type == "TEXT") {
        std::string textValue = column.dflt_value;
        if (textValue.front() == '\'' && textValue.back() == '\'') {
            textValue = textValue.substr(SQL_QUOTE_START_OFFSET,
                textValue.length() - SQL_QUOTES_TOTAL_LENGTH_TO_REMOVE);
        }
        values.PutString(column.name, textValue);
    } else if (column.type == "REAL" || column.type == "DOUBLE" || column.type == "FLOAT") {
        values.PutLong(column.name, SafeStoll(column.dflt_value));
    } else {
        MEDIA_ERR_LOG("Unhandled column type '%{public}s' for column '%{public}s'. Setting to NULL.",
            column.type.c_str(), column.name.c_str());
        values.PutNull(column.name);
    }
}

void PhotosFieldIterator::ResetLocalFields(NativeRdb::ValuesBucket &values)
{
    std::call_once(init_flag_, Initialize);

    if (all_columns_.empty()) {
        return;
    }

    for (const auto &column : all_columns_) {
        if (values.HasColumn(column.name)) {
            continue;
        }

        PutDefaultValue(values, column);
    }
}

} // namespace OHOS::Media::CloudSync