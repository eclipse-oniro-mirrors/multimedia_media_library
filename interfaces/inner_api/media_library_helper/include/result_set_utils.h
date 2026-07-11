/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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
#ifndef INTERFACES_INNERAPI_MEDIA_LIBRARY_HELPER_INCLUDE_RESULT_SET_UTILS_H_
#define INTERFACES_INNERAPI_MEDIA_LIBRARY_HELPER_INCLUDE_RESULT_SET_UTILS_H_

#include <string>
#include <variant>
#include <memory>
#include <vector>

#include "result_set.h"

#include "medialibrary_common_log.h"
// it will be optimized later
#include "medialibrary_type_const.h"

namespace OHOS {
namespace Media {
class ResultSetUtils {
public:
    template<typename T>
    static std::variant<int32_t, std::string, int64_t, double> GetValFromColumn(const std::string &columnName,
        T &resultSet, ResultSetDataType type)
    {
        if (resultSet == nullptr) {
            COMMON_ERR_LOG("resultSet is nullptr");
            return DefaultVariantVal(type);
        }

        int32_t err;
        int32_t index = 0;
        err = resultSet->GetColumnIndex(columnName, index);
        if (err) {
            COMMON_ERR_LOG("get column index err %{public}d, columnName: %{public}s", err, columnName.c_str());
            return DefaultVariantVal(type);
        }

        std::variant<int32_t, std::string, int64_t, double> data;
        switch (type) {
            case ResultSetDataType::TYPE_STRING: {
                data = GetStringValFromColumn(index, resultSet);
                break;
            }
            case ResultSetDataType::TYPE_INT32: {
                data = GetIntValFromColumn(index, resultSet);
                break;
            }
            case ResultSetDataType::TYPE_INT64: {
                data = GetLongValFromColumn(index, resultSet);
                break;
            }
            case ResultSetDataType::TYPE_DOUBLE: {
                data = GetDoubleValFromColumn(index, resultSet);
                break;
            }
            default: {
                break;
            }
        }

        return data;
    }

    template<typename T>
    static inline std::string GetStringValFromColumn(int index, T &resultSet)
    {
        std::string stringVal;
        if (resultSet->GetString(index, stringVal)) {
            return "";
        }
        return stringVal;
    }

    template<typename T>
    static inline int32_t GetIntValFromColumn(int index, T &resultSet)
    {
        int32_t integerVal;
        if (resultSet->GetInt(index, integerVal)) {
            return 0;
        }
        return integerVal;
    }

    template<typename T>
    static inline int64_t GetLongValFromColumn(int index, T &resultSet)
    {
        int64_t integer64Val;
        if (resultSet->GetLong(index, integer64Val)) {
            return 0;
        }
        return integer64Val;
    }

    template<typename T>
    static inline double GetDoubleValFromColumn(int index, T &resultSet)
    {
        double doubleVal;
        if (resultSet->GetDouble(index, doubleVal)) {
            return 0;
        }
        return doubleVal;
    }

private:
    static std::variant<int32_t, std::string, int64_t, double> DefaultVariantVal(ResultSetDataType type)
    {
        switch (type) {
            case ResultSetDataType::TYPE_STRING:
                return std::string();
            case ResultSetDataType::TYPE_INT32:
                return 0;
            case ResultSetDataType::TYPE_INT64:
                return static_cast<int64_t>(0);
            case ResultSetDataType::TYPE_DOUBLE:
                return static_cast<double>(0);
            default:
                COMMON_ERR_LOG("invalid data type %{public}d", type);
        }

        return 0;
    }
};

template<typename ResultSet>
static inline std::string GetStringVal(const std::string &field, const ResultSet &result)
{
    return std::get<std::string>(ResultSetUtils::GetValFromColumn(field, result, TYPE_STRING));
}
template<typename ResultSet>
static inline int32_t GetInt32Val(const std::string &field, const ResultSet &result)
{
    return std::get<int32_t>(ResultSetUtils::GetValFromColumn(field, result, TYPE_INT32));
}
template<typename ResultSet>
static inline int64_t GetInt64Val(const std::string &field, const ResultSet &result)
{
    return std::get<int64_t>(ResultSetUtils::GetValFromColumn(field, result, TYPE_INT64));
}
template<typename ResultSet>
static inline double GetDoubleVal(const std::string &field, const ResultSet &result)
{
    return std::get<double>(ResultSetUtils::GetValFromColumn(field, result, TYPE_DOUBLE));
}

template <typename ResultSet>
static inline bool TryToGoToFirstRow(const ResultSet &resultSet)
{
    if (resultSet == nullptr) {
        COMMON_ERR_LOG("resultSet is nullptr");
        return false;
    }

    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        COMMON_ERR_LOG("failed to go to first row");
        return false;
    }

    return true;
}
} // namespace Media
} // namespace  OHOS
#endif // INTERFACES_INNERAPI_MEDIA_LIBRARY_HELPER_INCLUDE_RESULT_SET_UTILS_H_
