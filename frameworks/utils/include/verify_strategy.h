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

#ifndef VERIFY_STRATEGY_H
#define VERIFY_STRATEGY_H

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "datashare_predicates.h"

namespace OHOS {
namespace Media {

// Special return value: continue to next strategy
constexpr int32_t E_VERIFY_CONTINUE = 1;

// Strategy interface for extensible validation rules
class IVerifyStrategy {
public:
    virtual ~IVerifyStrategy() = default;
    // Returns E_OK for pass-and-done (short-circuit), E_VERIFY_CONTINUE for pass-and-continue,
    // or an error code (e.g. E_SQL_CHECK_FAIL) for failure
    virtual int32_t Validate(const DataShare::OperationItem &item) = 0;
};

// Whitelist strategy: exact-match field against a global whitelist
// Registered as the FIRST strategy for each OperationType; short-circuits on match
class WhitelistStrategy : public IVerifyStrategy {
public:
    explicit WhitelistStrategy(const std::set<std::string> &whitelist);
    int32_t Validate(const DataShare::OperationItem &item) override;
private:
    const std::set<std::string> &whitelist_;
};

// Field check mode: which parameter slot contains the field name(s)
enum class FieldCheckMode {
    SINGLE_PARAM, // field is in singleParams[0] (e.g. EqualTo, Like, OrderBy)
    MULTI_PARAM,  // fields are in multiParams[0] (e.g. InKeys, GroupBy)
};

// Field format strategy: validates field format via regex (migrated from original logic)
class FieldFormatStrategy : public IVerifyStrategy {
public:
    explicit FieldFormatStrategy(FieldCheckMode mode = FieldCheckMode::SINGLE_PARAM);
    int32_t Validate(const DataShare::OperationItem &item) override;
private:
    int32_t ValidateSingleParam(const DataShare::OperationItem &item);
    int32_t ValidateMultiParam(const DataShare::OperationItem &item);
    FieldCheckMode mode_;
};

// Pure numeric field strategy: rejects field that is purely numeric
// Registered for specific OperationTypes (e.g. EQUAL_TO)
class NumericFieldStrategy : public IVerifyStrategy {
public:
    int32_t Validate(const DataShare::OperationItem &item) override;
};

// Value check strategy: validates value-level constraints (extensible placeholder)
// E.g. BETWEEN must have exactly 2 values, LIKE wildcard legality
class ValueCheckStrategy : public IVerifyStrategy {
public:
    int32_t Validate(const DataShare::OperationItem &item) override;
};

// Registry: maps OperationType → ordered list of strategies
class ValidatorRegistry {
public:
    ValidatorRegistry();
    // Register a strategy for a given OperationType; strategies execute in registration order
    void Register(DataShare::OperationType type, std::unique_ptr<IVerifyStrategy> strategy);
    // Execute all registered strategies for the given OperationType
    // Returns E_OK if all pass, or the first error code encountered
    int32_t Validate(DataShare::OperationType type, const DataShare::OperationItem &item);
private:
    std::map<DataShare::OperationType, std::vector<std::unique_ptr<IVerifyStrategy>>> strategies_;
    static const std::set<std::string> WHITELIST_;
};

} // namespace Media
} // namespace OHOS
#endif // VERIFY_STRATEGY_H
