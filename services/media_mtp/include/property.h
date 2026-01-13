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
#ifndef FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_PROPERTY_H_
#define FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_PROPERTY_H_
#include <memory>
#include <stdint.h>
#include <time.h>
#include <string>
#include <vector>

#include "mtp_constants.h"

namespace OHOS {
namespace Media {
class Property {
public:
    class Value {
    public:
        Value();

        void Dump(uint32_t valueType);
        std::string ToString(uint32_t valueType);

        union {
            uint128_t ui128;
            int128_t i128;
            uint64_t ui64;
            int64_t i64;
            uint32_t ui32;
            int32_t i32;
            uint16_t ui16;
            int16_t i16;
            uint8_t ui8;
            int8_t i8;
        } bin_;
        std::shared_ptr<std::string> str_;

    private:
        bool BinToString(uint32_t valueType, std::string &outStr);
        bool StrToString(uint32_t valueType, std::string &outStr);
    };

    Property();
    Property(uint16_t propCode, uint16_t propType, bool propWriteable = false, int value = 0);
    virtual ~Property();

    uint16_t GetPropertyCode() const;
    uint16_t GetDataType() const;

    bool Read(const std::vector<uint8_t> &buffer, size_t &offset);
    void Write(std::vector<uint8_t> &buffer);
    void SetDefaultValue(const std::shared_ptr<std::string> &str);
    void SetCurrentValue(const std::shared_ptr<std::string> &str);
    const std::shared_ptr<Value> GetCurrentValue();
    void SetFormRange(int min, int max, int step);
    void SetFormEnum(const std::vector<int> &values);
    void SetFormDateTime();

    bool IsDeviceProperty() const;
    bool IsArrayType() const;
    void Dump();

    uint32_t handle_ {0}; // not a element for property
    uint16_t code_ {0};
    uint16_t type_ {MTP_TYPE_UNDEFINED_CODE};
    bool writeable_ {false};
    std::shared_ptr<Value> defaultValue;
    std::shared_ptr<Value> currentValue;
    std::shared_ptr<std::vector<Value>> defaultValues; // for array types
    std::shared_ptr<std::vector<Value>> currentValues; // for array types

    enum Form : uint8_t {
        None = 0,
        Range = 1,
        Enum = 2,
        DateTime = 3,
    };

    uint32_t groupCode_ {0};
    uint8_t formFlag_ {Form::None};
    std::shared_ptr<Value> minValue; // for range form
    std::shared_ptr<Value> maxValue;
    std::shared_ptr<Value> stepSize;
    std::shared_ptr<std::vector<Value>> enumValues; // for enum form

private:
    bool ReadValueData(const std::vector<uint8_t> &buffer, size_t &offset);
    bool ReadFormData(const std::vector<uint8_t> &buffer, size_t &offset);
    void WriteValueData(std::vector<uint8_t> &buffer);
    void WriteFormData(std::vector<uint8_t> &buffer);

    bool ReadValue(const std::vector<uint8_t> &buffer, size_t &offset, Value &value);
    bool ReadValueEx(const std::vector<uint8_t> &buffer, size_t &offset, Value &value);
    void WriteValue(std::vector<uint8_t> &buffer, const Value &value);
    void WriteValueEx(std::vector<uint8_t> &buffer, const Value &value);

    bool ReadArrayValues(const std::vector<uint8_t> &buffer, size_t &offset,
        std::shared_ptr<std::vector<Value>> &values);
    void WriteArrayValues(std::vector<uint8_t> &buffer, const std::shared_ptr<std::vector<Value>> &values);

    void DumpValue(uint8_t indent, const std::shared_ptr<Value> &value, const std::string &name);
    void DumpValues(uint8_t indent, const std::shared_ptr<std::vector<Value>> &values, const std::string &name);
    void DumpForm(uint8_t indent);
};
} // namespace Media
} // namespace OHOS
#endif // FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_PROPERTY_H_
