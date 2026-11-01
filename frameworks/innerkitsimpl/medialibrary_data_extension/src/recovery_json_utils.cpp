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
#include "recovery_json_utils.h"

#include <cmath>
#include <cstdint>
#include <fstream>
#include <limits>

namespace OHOS::Media {
RecoveryJsonStatus ReadRecoveryJsonFile(const std::string &path, nlohmann::json &value)
{
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        return RecoveryJsonStatus::READ_ERROR;
    }
    value = nlohmann::json::parse(input, nullptr, false);
    if (input.bad()) {
        return RecoveryJsonStatus::READ_ERROR;
    }
    return value.is_discarded() || !value.is_object() ? RecoveryJsonStatus::INVALID : RecoveryJsonStatus::OK;
}

// Invalid or absent fields are not added to the update bucket.
bool PutRecoveryJsonValue(NativeRdb::ValuesBucket &values, const std::string &name,
    const nlohmann::json &value, const std::string &type)
{
    if (type.find("INT") != std::string::npos && value.is_number_integer()) {
        if (value.is_number_unsigned() && value.get<uint64_t>() >
            static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
            return false;
        }
        values.PutLong(name, value.get<int64_t>());
        return true;
    }
    if ((type.find("CHAR") != std::string::npos || type.find("TEXT") != std::string::npos ||
        type.find("CLOB") != std::string::npos) && value.is_string()) {
        values.PutString(name, value.get<std::string>());
        return true;
    }
    if ((type.find("REAL") != std::string::npos || type.find("FLOAT") != std::string::npos ||
        type.find("DOUBLE") != std::string::npos) && value.is_number()) {
        double number = value.get<double>();
        if (std::isfinite(number)) {
            values.PutDouble(name, number);
            return true;
        }
    }
    return false;
}
} // namespace OHOS::Media
