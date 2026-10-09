/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#define LOG_TAG "verify_strategy"
#include <regex>

#include "verify_strategy.h"

#include "media_log.h"
#include "medialibrary_errno.h"

namespace OHOS {
namespace Media {
using OperationType = DataShare::OperationType;

// ---- Global whitelist (exact match) ----
const std::set<std::string> ValidatorRegistry::WHITELIST_ = {
    "lower(lpath)",
};

// ---- Regex patterns (migrated from original) ----
// colName or (colName) or [colName] or "colName"
static const std::regex COLNAME_OPTIONAL_BRACKETS(
    "^\\s*([a-zA-Z0-9_]+)\\s*$|"
    "^\\s*\\(([a-zA-Z0-9_]+)\\)\\s*$|"
    "^\\s*\\[([a-zA-Z0-9_]+)\\]\\s*$|"
    "^\\s*\"([a-zA-Z0-9_]+)\"\\s*$"
);

// tableName.colName or (tableName.colName) or [tableName.colName] or "tableName.colName"
static const std::regex TABLENAME_DOT_COLNAME_OPTIONAL_BRACKETS(
    "^\\s*([a-zA-Z0-9_]+\\.[a-zA-Z0-9_]+)\\s*$|"
    "^\\s*\\(([a-zA-Z0-9_]+\\.[a-zA-Z0-9_]+)\\)\\s*$|"
    "^\\s*\\[([a-zA-Z0-9_]+\\.[a-zA-Z0-9_]+)\\]\\s*$|"
    "^\\s*\"([a-zA-Z0-9_]+\\.[a-zA-Z0-9_]+)\"\\s*$"
);

// $.colName or ($.colName) or [$.colName] or "$.colName"
static const std::regex AMPERSAND_DOT_COLNAME_OPTIONAL_BRACKETS(
    "^\\s*(\\$\\.[a-zA-Z0-9_]+)\\s*$|"
    "^\\s*\\((\\$\\.[a-zA-Z0-9_]+)\\)\\s*$|"
    "^\\s*\\[(\\$\\.[a-zA-Z0-9_]+)\\]\\s*$|"
    "^\\s*\"(\\$\\.[a-zA-Z0-9_]+)\"\\s*$"
);

// store.table.colName or (store.table.colName) or [store.table.colName] or "store.table.colName"
static const std::regex STORE_TABLE_COLNAME_OPTIONAL_BRACKETS(
    "^\\s*([a-zA-Z0-9_]+\\.[a-zA-Z0-9_]+\\.[a-zA-Z0-9_]+)\\s*$|"
    "^\\s*\\(([a-zA-Z0-9_]+\\.[a-zA-Z0-9_]+\\.[a-zA-Z0-9_]+)\\)\\s*$|"
    "^\\s*\\[([a-zA-Z0-9_]+\\.[a-zA-Z0-9_]+\\.[a-zA-Z0-9_]+)\\]\\s*$|"
    "^\\s*\"([a-zA-Z0-9_]+\\.[a-zA-Z0-9_]+\\.[a-zA-Z0-9_]+)\"\\s*$"
);

// ---- WhitelistStrategy ----
WhitelistStrategy::WhitelistStrategy(const std::set<std::string> &whitelist) : whitelist_(whitelist) {}

int32_t WhitelistStrategy::Validate(const DataShare::OperationItem &item)
{
    // Single-param operations: field is GetSingle(0)
    if (!item.singleParams.empty()) {
        if (whitelist_.find(item.GetSingle(0)) != whitelist_.end()) {
            return E_OK; // Short-circuit: whitelist hit, skip all subsequent strategies
        }
    }
    // Multi-param operations: fields are from multiParams[0]
    if (!item.multiParams.empty()) {
        std::vector<std::string> fields = DataShare::MutliValue(item.multiParams[0]);
        for (const auto &field : fields) {
            if (whitelist_.find(field) != whitelist_.end()) {
                return E_OK;
            }
        }
    }
    return E_VERIFY_CONTINUE; // No whitelist hit, continue to next strategy
}

// ---- FieldFormatStrategy ----
FieldFormatStrategy::FieldFormatStrategy(FieldCheckMode mode) : mode_(mode) {}

static bool VerifyField(const std::string &field)
{
    if (field.empty()) {
        MEDIA_WARN_LOG("field is empty");
        return true;
    }
    return (std::regex_match(field, COLNAME_OPTIONAL_BRACKETS) ||
        std::regex_match(field, TABLENAME_DOT_COLNAME_OPTIONAL_BRACKETS) ||
        std::regex_match(field, AMPERSAND_DOT_COLNAME_OPTIONAL_BRACKETS) ||
        std::regex_match(field, STORE_TABLE_COLNAME_OPTIONAL_BRACKETS));
}

int32_t FieldFormatStrategy::ValidateSingleParam(const DataShare::OperationItem &item)
{
    if (item.singleParams.empty()) {
        return E_VERIFY_CONTINUE;
    }
    std::string field = item.GetSingle(0);
    if (!VerifyField(field)) {
        MEDIA_ERR_LOG("FieldFormatStrategy(SINGLE_PARAM) failed, field: %{public}s", field.c_str());
        return E_SQL_CHECK_FAIL;
    }
    return E_VERIFY_CONTINUE;
}

int32_t FieldFormatStrategy::ValidateMultiParam(const DataShare::OperationItem &item)
{
    if (item.multiParams.empty()) {
        return E_VERIFY_CONTINUE;
    }
    std::vector<std::string> fields = DataShare::MutliValue(item.multiParams[0]);
    for (const auto &field : fields) {
        if (!VerifyField(field)) {
            MEDIA_ERR_LOG("FieldFormatStrategy(MULTI_PARAM) failed, field: %{public}s", field.c_str());
            return E_SQL_CHECK_FAIL;
        }
    }
    return E_VERIFY_CONTINUE;
}

int32_t FieldFormatStrategy::Validate(const DataShare::OperationItem &item)
{
    if (mode_ == FieldCheckMode::SINGLE_PARAM) {
        return ValidateSingleParam(item);
    }
    return ValidateMultiParam(item);
}

// ---- NumericFieldStrategy ----
static const std::regex PURE_NUMERIC("^\\d+$");

int32_t NumericFieldStrategy::Validate(const DataShare::OperationItem &item)
{
    if (!item.singleParams.empty()) {
        std::string field = item.GetSingle(0);
        if (std::regex_match(field, PURE_NUMERIC)) {
            MEDIA_ERR_LOG("NumericFieldStrategy failed, field is pure numeric: %{public}s", field.c_str());
            return E_SQL_CHECK_FAIL;
        }
    }
    return E_VERIFY_CONTINUE;
}

// ---- ValueCheckStrategy ----
int32_t ValueCheckStrategy::Validate(const DataShare::OperationItem &item)
{
    // Placeholder for future value-level validation
    // E.g. BETWEEN must have exactly 2 values, LIKE wildcard legality, etc.
    return E_VERIFY_CONTINUE;
}

// ---- ValidatorRegistry ----
ValidatorRegistry::ValidatorRegistry()
{
    // 2-param public operations
    for (auto type : { OperationType::ORDER_BY_ASC, OperationType::ORDER_BY_DESC }) {
        Register(type, std::make_unique<WhitelistStrategy>(WHITELIST_));
        Register(type, std::make_unique<FieldFormatStrategy>());
    }
    // 3-param public operations
    for (auto type : { OperationType::EQUAL_TO }) {
        Register(type, std::make_unique<WhitelistStrategy>(WHITELIST_));
        Register(type, std::make_unique<FieldFormatStrategy>());
        Register(type, std::make_unique<NumericFieldStrategy>());
    }
    // 2-param system operations
    for (auto type : { OperationType::IS_NULL, OperationType::IS_NOT_NULL,
                      OperationType::INDEXED_BY, OperationType::KEY_PREFIX }) {
        Register(type, std::make_unique<WhitelistStrategy>(WHITELIST_));
        Register(type, std::make_unique<FieldFormatStrategy>());
    }
    // 3-param system operations
    for (auto type : { OperationType::NOT_EQUAL_TO, OperationType::GREATER_THAN,
                      OperationType::LESS_THAN, OperationType::GREATER_THAN_OR_EQUAL_TO,
                      OperationType::LESS_THAN_OR_EQUAL_TO, OperationType::NOT_IN,
                      OperationType::LIKE, OperationType::UNLIKE,
                      OperationType::BEGIN_WITH, OperationType::END_WITH,
                      OperationType::CONTAINS, OperationType::GLOB,
                      OperationType::BETWEEN, OperationType::NOTBETWEEN }) {
        Register(type, std::make_unique<WhitelistStrategy>(WHITELIST_));
        Register(type, std::make_unique<FieldFormatStrategy>());
        Register(type, std::make_unique<NumericFieldStrategy>());
    }
    // Multi-param system operations (field is in multiParams[0])
    for (auto type : { OperationType::IN_KEY, OperationType::GROUP_BY }) {
        Register(type, std::make_unique<WhitelistStrategy>(WHITELIST_));
        Register(type, std::make_unique<FieldFormatStrategy>(FieldCheckMode::MULTI_PARAM));
    }
}

void ValidatorRegistry::Register(DataShare::OperationType type, std::unique_ptr<IVerifyStrategy> strategy)
{
    strategies_[type].push_back(std::move(strategy));
}

int32_t ValidatorRegistry::Validate(DataShare::OperationType type, const DataShare::OperationItem &item)
{
    auto it = strategies_.find(type);
    if (it == strategies_.end()) {
        return E_OK; // No strategies registered for this type, pass by default
    }
    for (size_t i = 0; i < it->second.size(); i++) {
        int32_t result = it->second[i]->Validate(item);
        if (result == E_OK) {
            return E_OK; // Short-circuit (whitelist hit) or all pass
        }
        if (result != E_VERIFY_CONTINUE) {
            MEDIA_ERR_LOG("Validate failed, operation: %{public}d, strategy index: %{public}zu, errCode: %{public}d",
                static_cast<int32_t>(type), i, result);
            return result; // Error code, stop and return
        }
        // E_VERIFY_CONTINUE: proceed to next strategy
    }
    return E_OK; // All strategies passed
}

} // namespace Media
} // namespace OHOS
