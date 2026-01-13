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
#include "property.h"
#include "media_log.h"
#include "mtp_packet_tools.h"
using namespace std;
namespace OHOS {
namespace Media {
static const std::map<uint32_t, std::string> FormMap = {
    { Property::Form::None, "None" },
    { Property::Form::Range, "Range" },
    { Property::Form::Enum, "Enum" },
    { Property::Form::DateTime, "DateTime" },
};

Property::Value::Value()
{
    bin_.ui128[OFFSET_0] = 0;
    bin_.ui128[OFFSET_1] = 0;
    bin_.ui128[OFFSET_2] = 0;
    bin_.ui128[OFFSET_3] = 0;
}

void Property::Value::Dump(uint32_t valueType)
{
    MEDIA_DEBUG_LOG("%{private}s", ToString(valueType).c_str());
}

std::string Property::Value::ToString(uint32_t valueType)
{
    std::string outStr;
    CHECK_AND_RETURN_RET(!StrToString(valueType, outStr), outStr);
    CHECK_AND_RETURN_RET(!BinToString(valueType, outStr), outStr);
    outStr.assign("unknown type ");
    outStr.append(std::to_string(valueType));
    return outStr;
}

bool Property::Value::BinToString(uint32_t valueType, std::string &outStr)
{
    std::string valueStr;

    bool res = false;
    if (valueType == MTP_TYPE_INT8_CODE) {
        res = MtpPacketTool::Int8ToString(bin_.i8, valueStr);
    } else if (valueType == MTP_TYPE_UINT8_CODE) {
        res = MtpPacketTool::UInt8ToString(bin_.ui8, valueStr);
    } else if (valueType == MTP_TYPE_INT16_CODE) {
        res = MtpPacketTool::Int16ToString(bin_.i16, valueStr);
    } else if (valueType == MTP_TYPE_UINT16_CODE) {
        res = MtpPacketTool::UInt16ToString(bin_.ui16, valueStr);
    } else if (valueType == MTP_TYPE_INT32_CODE) {
        res = MtpPacketTool::Int32ToString(bin_.i32, valueStr);
    } else if (valueType == MTP_TYPE_UINT32_CODE) {
        res = MtpPacketTool::UInt32ToString(bin_.ui32, valueStr);
    } else if (valueType == MTP_TYPE_INT64_CODE) {
        res = MtpPacketTool::Int64ToString(bin_.i64, valueStr);
    } else if (valueType == MTP_TYPE_UINT64_CODE) {
        res = MtpPacketTool::UInt64ToString(bin_.ui64, valueStr);
    } else if (valueType == MTP_TYPE_INT128_CODE) {
        res = MtpPacketTool::Int128ToString(bin_.i128, valueStr);
    } else if (valueType == MTP_TYPE_UINT128_CODE) {
        res = MtpPacketTool::UInt128ToString(bin_.ui128, valueStr);
    } else {
        return false;
    }

    if (!res) {
        outStr.assign("bin_={}");
        return true;
    }

    outStr.assign("bin_={");
    outStr.append("type=");
    outStr.append(MtpPacketTool::GetDataTypeName(valueType));
    outStr.append(", ");
    outStr.append(valueStr);
    outStr.append("}");
    return true;
}

bool Property::Value::StrToString(uint32_t valueType, std::string &outStr)
{
    CHECK_AND_RETURN_RET(valueType == MTP_TYPE_STRING_CODE, false);
    outStr.assign("str={");
    if (str_ == nullptr) {
        outStr.append("nullptr");
    } else {
        outStr.append(MtpPacketTool::StrToString(*str_));
    }
    outStr.append("}");
    return true;
}

Property::Property()
{
    defaultValue = std::make_shared<Value>();
    currentValue = std::make_shared<Value>();
    minValue = std::make_shared<Value>();
    maxValue = std::make_shared<Value>();
    stepSize = std::make_shared<Value>();
}

Property::Property(uint16_t propCode, uint16_t propType, bool propWriteable, int value)
    : code_(propCode), type_(propType), writeable_(propWriteable)
{
    defaultValue = std::make_shared<Value>();
    currentValue = std::make_shared<Value>();
    minValue = std::make_shared<Value>();
    maxValue = std::make_shared<Value>();
    stepSize = std::make_shared<Value>();

    if (value) {
        switch (type_) {
            case MTP_TYPE_INT8_CODE:
                defaultValue->bin_.i8 = static_cast<int8_t>(value);
                break;
            case MTP_TYPE_UINT8_CODE:
                defaultValue->bin_.ui8 = static_cast<uint8_t>(value);
                break;
            case MTP_TYPE_INT16_CODE:
                defaultValue->bin_.i16 = static_cast<int16_t>(value);
                break;
            case MTP_TYPE_UINT16_CODE:
                defaultValue->bin_.ui16 = static_cast<uint16_t>(value);
                break;
            case MTP_TYPE_INT32_CODE:
                defaultValue->bin_.i32 = static_cast<int32_t>(value);
                break;
            case MTP_TYPE_UINT32_CODE:
                defaultValue->bin_.ui32 = static_cast<uint32_t>(value);
                break;
            case MTP_TYPE_INT64_CODE:
                defaultValue->bin_.i64 = static_cast<int64_t>(value);
                break;
            case MTP_TYPE_UINT64_CODE:
                defaultValue->bin_.ui64 = static_cast<uint64_t>(value);
                break;
            default:
                MEDIA_ERR_LOG("Property::Property unknown type %{private}u", type_);
                break;
        }
    }
}

Property::~Property()
{
}

uint16_t Property::GetPropertyCode() const
{
    return code_;
}

uint16_t Property::GetDataType() const
{
    return type_;
}

bool Property::Read(const std::vector<uint8_t> &buffer, size_t &offset)
{
    CHECK_AND_RETURN_RET_LOG(MtpPacketTool::GetUInt16(buffer, offset, code_), false,
        "Property::read code error");
    CHECK_AND_RETURN_RET_LOG(MtpPacketTool::GetUInt16(buffer, offset, type_), false,
        "Property::read type error");
    uint8_t tmpVar = 0;
    CHECK_AND_RETURN_RET_LOG(MtpPacketTool::GetUInt8(buffer, offset, tmpVar), false,
        "Property::read tmpVar error");
    writeable_ = (tmpVar == 1);
    CHECK_AND_RETURN_RET_LOG(ReadValueData(buffer, offset), false,
        "Property::read valuedata error");
    bool deviceProp = IsDeviceProperty();
    bool cond = (!deviceProp && !MtpPacketTool::GetUInt32(buffer, offset, groupCode_));
    CHECK_AND_RETURN_RET_LOG(!cond, false, "Property::read group error");
    CHECK_AND_RETURN_RET_LOG(ReadFormData(buffer, offset), false, "Property::read formdata error");
    return true;
}

void Property::Write(std::vector<uint8_t> &buffer)
{
    MtpPacketTool::PutUInt16(buffer, code_);
    MtpPacketTool::PutUInt16(buffer, type_);
    MtpPacketTool::PutUInt8(buffer, writeable_ ? 1 : 0);

    WriteValueData(buffer);

    bool deviceProp = IsDeviceProperty();
    MEDIA_DEBUG_LOG("Property::write deviceProp=%{private}u", deviceProp);
    if (!deviceProp) {
        MtpPacketTool::PutUInt32(buffer, groupCode_);
    }

    WriteFormData(buffer);
}

void Property::SetDefaultValue(const std::shared_ptr<std::string> &str)
{
    defaultValue->str_ = str;
}

void Property::SetCurrentValue(const std::shared_ptr<std::string> &str)
{
    currentValue->str_ = str;
}

const std::shared_ptr<Property::Value> Property::GetCurrentValue()
{
    return currentValue;
}

void Property::SetFormRange(int min, int max, int step)
{
    formFlag_ = Form::Range;
    switch (type_) {
        case MTP_TYPE_INT8_CODE:
            minValue->bin_.i8 = static_cast<int8_t>(min);
            maxValue->bin_.i8 = static_cast<int8_t>(max);
            stepSize->bin_.i8 = static_cast<int8_t>(step);
            break;
        case MTP_TYPE_UINT8_CODE:
            minValue->bin_.ui8 = static_cast<uint8_t>(min);
            maxValue->bin_.ui8 = static_cast<uint8_t>(max);
            stepSize->bin_.ui8 = static_cast<uint8_t>(step);
            break;
        case MTP_TYPE_INT16_CODE:
            minValue->bin_.i16 = static_cast<int16_t>(min);
            maxValue->bin_.i16 = static_cast<int16_t>(max);
            stepSize->bin_.i16 = static_cast<int16_t>(step);
            break;
        case MTP_TYPE_UINT16_CODE:
            minValue->bin_.ui16 = static_cast<uint16_t>(min);
            maxValue->bin_.ui16 = static_cast<uint16_t>(max);
            stepSize->bin_.ui16 = static_cast<uint16_t>(step);
            break;
        case MTP_TYPE_INT32_CODE:
            minValue->bin_.i32 = static_cast<int32_t>(min);
            maxValue->bin_.i32 = static_cast<int32_t>(max);
            stepSize->bin_.i32 = static_cast<int32_t>(step);
            break;
        case MTP_TYPE_UINT32_CODE:
            minValue->bin_.ui32 = static_cast<uint32_t>(min);
            maxValue->bin_.ui32 = static_cast<uint32_t>(max);
            stepSize->bin_.ui32 = static_cast<uint32_t>(step);
            break;
        case MTP_TYPE_INT64_CODE:
            minValue->bin_.i64 = static_cast<int64_t>(min);
            maxValue->bin_.i64 = static_cast<int64_t>(max);
            stepSize->bin_.i64 = static_cast<int64_t>(step);
            break;
        case MTP_TYPE_UINT64_CODE:
            minValue->bin_.ui64 = static_cast<uint64_t>(min);
            maxValue->bin_.ui64 = static_cast<uint64_t>(max);
            stepSize->bin_.ui64 = static_cast<uint64_t>(step);
            break;
        default:
            MEDIA_ERR_LOG("Property::setFormRange unsupported type %{private}u", type_);
            break;
    }
}

void Property::SetFormEnum(const std::vector<int> &values)
{
    formFlag_ = Form::Enum;
    enumValues = std::make_shared<std::vector<Value>>();

    Value v;
    for (auto value : values) {
        switch (type_) {
            case MTP_TYPE_INT8_CODE:
                v.bin_.i8 = static_cast<int8_t>(value);
                break;
            case MTP_TYPE_UINT8_CODE:
                v.bin_.ui8 = static_cast<uint8_t>(value);
                break;
            case MTP_TYPE_INT16_CODE:
                v.bin_.i16 = static_cast<int16_t>(value);
                break;
            case MTP_TYPE_UINT16_CODE:
                v.bin_.ui16 = static_cast<uint16_t>(value);
                break;
            case MTP_TYPE_INT32_CODE:
                v.bin_.i32 = static_cast<int32_t>(value);
                break;
            case MTP_TYPE_UINT32_CODE:
                v.bin_.ui32 = static_cast<uint32_t>(value);
                break;
            case MTP_TYPE_INT64_CODE:
                v.bin_.i64 = static_cast<int64_t>(value);
                break;
            case MTP_TYPE_UINT64_CODE:
                v.bin_.ui64 = static_cast<uint64_t>(value);
                break;
            default:
                MEDIA_ERR_LOG("Property::setFormEnum unsupported type %{private}u", type_);
                break;
        }
        enumValues->push_back(v);
    }
}

void Property::SetFormDateTime()
{
    formFlag_ = Form::DateTime;
}

bool Property::IsDeviceProperty() const
{
    // bit values defined by protocol, check if code is device property
    return (((code_ & 0xF000) == 0x5000) || ((code_ & 0xF800) == 0xD000));
}

bool Property::IsArrayType() const
{
    return ((type_ >= MTP_DEVICE_PROP_DESC_TYPE_AINT8) && (type_ <= MTP_DEVICE_PROP_DESC_TYPE_AUINT128));
}

bool Property::ReadValueData(const std::vector<uint8_t> &buffer, size_t &offset)
{
    bool deviceProp = IsDeviceProperty();
    switch (type_) {
        case MTP_TYPE_AINT8_CODE:
        case MTP_TYPE_AUINT8_CODE:
        case MTP_TYPE_AINT16_CODE:
        case MTP_TYPE_AUINT16_CODE:
        case MTP_TYPE_AINT32_CODE:
        case MTP_TYPE_AUINT32_CODE:
        case MTP_TYPE_AINT64_CODE:
        case MTP_TYPE_AUINT64_CODE:
        case MTP_TYPE_AINT128_CODE:
        case MTP_TYPE_AUINT128_CODE: {
            CHECK_AND_RETURN_RET_LOG(ReadArrayValues(buffer, offset, defaultValues), false,
                "Property::readValueData defaultValues error");
            if (deviceProp) {
                CHECK_AND_RETURN_RET_LOG(ReadArrayValues(buffer, offset, currentValues), false,
                    "Property::readValueData currentValues error");
            }
            break;
        }
        default: {
            CHECK_AND_RETURN_RET_LOG(ReadValue(buffer, offset, *defaultValue), false,
                "Property::readValueData defaultValue error");
            if (deviceProp) {
                CHECK_AND_RETURN_RET_LOG(ReadValue(buffer, offset, *currentValue), false,
                    "Property::readValueData currentValues error");
            }
        }
    }
    return true;
}

bool Property::ReadFormData(const std::vector<uint8_t> &buffer, size_t &offset)
{
    CHECK_AND_RETURN_RET_LOG(MtpPacketTool::GetUInt8(buffer, offset, formFlag_), false,
        "Property::readFormData flag error");

    if (formFlag_ == Form::Range) {
        CHECK_AND_RETURN_RET_LOG(ReadValue(buffer, offset, *minValue), false, "Property::readFormData minValue error");
        CHECK_AND_RETURN_RET_LOG(ReadValue(buffer, offset, *maxValue), false, "Property::readFormData maxValue error");
        CHECK_AND_RETURN_RET_LOG(ReadValue(buffer, offset, *stepSize), false, "Property::readFormData stepSize error");
    } else if (formFlag_ == Form::Enum) {
        uint16_t len = 0;
        CHECK_AND_RETURN_RET_LOG(MtpPacketTool::GetUInt16(buffer, offset, len), false,
            "Property::readFormData len error");
        enumValues = std::make_shared<std::vector<Value>>();
        Value value;
        for (int i = 0; i < len; i++) {
            CHECK_AND_RETURN_RET_LOG(ReadValue(buffer, offset, value), false,
                "Property::readFormData i=%{private}u", i);
            enumValues->push_back(value);
        }
    }

    return true;
}

void Property::WriteValueData(std::vector<uint8_t> &buffer)
{
    switch (type_) {
        case MTP_TYPE_AINT8_CODE:
        case MTP_TYPE_AUINT8_CODE:
        case MTP_TYPE_AINT16_CODE:
        case MTP_TYPE_AUINT16_CODE:
        case MTP_TYPE_AINT32_CODE:
        case MTP_TYPE_AUINT32_CODE:
        case MTP_TYPE_AINT64_CODE:
        case MTP_TYPE_AUINT64_CODE:
        case MTP_TYPE_AINT128_CODE:
        case MTP_TYPE_AUINT128_CODE: {
            WriteArrayValues(buffer, defaultValues);
            if (IsDeviceProperty()) {
                WriteArrayValues(buffer, currentValues);
            }
            break;
        }
        default: {
            WriteValue(buffer, *defaultValue);
            if (IsDeviceProperty()) {
                WriteValue(buffer, *currentValue);
            }
        }
    }
}

void Property::WriteFormData(std::vector<uint8_t> &buffer)
{
    MtpPacketTool::PutUInt8(buffer, formFlag_);
    if (formFlag_ == Form::Range) {
        WriteValue(buffer, *minValue);
        WriteValue(buffer, *maxValue);
        WriteValue(buffer, *stepSize);
    } else if (formFlag_ == Form::Enum) {
        uint32_t valueSum = (enumValues == nullptr) ? 0 : enumValues->size();
        MtpPacketTool::PutUInt16(buffer, valueSum);
        for (uint32_t i = 0; i < valueSum; i++) {
            WriteValue(buffer, (*enumValues)[i]);
        }
    }
}

bool Property::ReadValue(const std::vector<uint8_t> &buffer, size_t &offset, Value &value)
{
    switch (type_) {
        case MTP_TYPE_INT8_CODE:
        case MTP_TYPE_AINT8_CODE:
            if (!MtpPacketTool::GetInt8(buffer, offset, value.bin_.i8)) {
                return false;
            }
            break;
        case MTP_TYPE_UINT8_CODE:
        case MTP_TYPE_AUINT8_CODE:
            if (!MtpPacketTool::GetUInt8(buffer, offset, value.bin_.ui8)) {
                return false;
            }
            break;
        case MTP_TYPE_INT16_CODE:
        case MTP_TYPE_AINT16_CODE:
            if (!MtpPacketTool::GetInt16(buffer, offset, value.bin_.i16)) {
                return false;
            }
            break;
        case MTP_TYPE_UINT16_CODE:
        case MTP_TYPE_AUINT16_CODE:
            if (!MtpPacketTool::GetUInt16(buffer, offset, value.bin_.ui16)) {
                return false;
            }
            break;
        case MTP_TYPE_INT32_CODE:
        case MTP_TYPE_AINT32_CODE:
            if (!MtpPacketTool::GetInt32(buffer, offset, value.bin_.i32)) {
                return false;
            }
            break;
        case MTP_TYPE_UINT32_CODE:
        case MTP_TYPE_AUINT32_CODE:
            if (!MtpPacketTool::GetUInt32(buffer, offset, value.bin_.ui32)) {
                return false;
            }
            break;
        default: {
            if (!ReadValueEx(buffer, offset, value)) {
                return false;
            }
            break;
        }
    }
    return true;
}

bool Property::ReadValueEx(const std::vector<uint8_t> &buffer, size_t &offset, Value &value)
{
    switch (type_) {
        case MTP_TYPE_INT64_CODE:
        case MTP_TYPE_AINT64_CODE: {
            CHECK_AND_RETURN_RET(MtpPacketTool::GetInt64(buffer, offset,
                value.bin_.i64), false);
            break;
        }
        case MTP_TYPE_UINT64_CODE:
        case MTP_TYPE_AUINT64_CODE: {
            CHECK_AND_RETURN_RET(MtpPacketTool::GetUInt64(buffer, offset,
                value.bin_.ui64), false);
            break;
        }
        case MTP_TYPE_INT128_CODE:
        case MTP_TYPE_AINT128_CODE: {
            CHECK_AND_RETURN_RET(MtpPacketTool::GetInt128(buffer, offset,
                value.bin_.i128), false);
            break;
        }
        case MTP_TYPE_UINT128_CODE:
        case MTP_TYPE_AUINT128_CODE: {
            CHECK_AND_RETURN_RET(MtpPacketTool::GetUInt128(buffer, offset,
                value.bin_.ui128), false);
            break;
        }
        case MTP_TYPE_STRING_CODE: {
            std::string str;
            CHECK_AND_RETURN_RET(MtpPacketTool::GetString(buffer, offset, str), false);
            value.str_ = std::make_shared<std::string>(str);
            break;
        }
        default:
            MEDIA_ERR_LOG("unknown type %{private}u in Property::ReadValue", type_);
            return false;
    }
    return true;
}

void Property::WriteValue(std::vector<uint8_t> &buffer, const Value &value)
{
    switch (type_) {
        case MTP_TYPE_INT8_CODE:
        case MTP_TYPE_AINT8_CODE:
            MtpPacketTool::PutUInt8(buffer, static_cast<uint8_t>(value.bin_.i8));
            break;
        case MTP_TYPE_UINT8_CODE:
        case MTP_TYPE_AUINT8_CODE:
            MtpPacketTool::PutUInt8(buffer, value.bin_.ui8);
            break;
        case MTP_TYPE_INT16_CODE:
        case MTP_TYPE_AINT16_CODE:
            MtpPacketTool::PutUInt16(buffer, static_cast<uint16_t>(value.bin_.i16));
            break;
        case MTP_TYPE_UINT16_CODE:
        case MTP_TYPE_AUINT16_CODE:
            MtpPacketTool::PutUInt16(buffer, value.bin_.ui16);
            break;
        case MTP_TYPE_INT32_CODE:
        case MTP_TYPE_AINT32_CODE:
            MtpPacketTool::PutUInt32(buffer, static_cast<uint32_t>(value.bin_.i32));
            break;
        case MTP_TYPE_UINT32_CODE:
        case MTP_TYPE_AUINT32_CODE:
            MtpPacketTool::PutUInt32(buffer, value.bin_.ui32);
            break;
        default: {
            WriteValueEx(buffer, value);
            break;
        }
    }
}

void Property::WriteValueEx(std::vector<uint8_t> &buffer, const Value &value)
{
    switch (type_) {
        case MTP_TYPE_INT64_CODE:
        case MTP_TYPE_AINT64_CODE:
            MtpPacketTool::PutUInt64(buffer, static_cast<uint64_t>(value.bin_.i64));
            break;
        case MTP_TYPE_UINT64_CODE:
        case MTP_TYPE_AUINT64_CODE:
            MtpPacketTool::PutUInt64(buffer, value.bin_.ui64);
            break;
        case MTP_TYPE_INT128_CODE:
        case MTP_TYPE_AINT128_CODE:
            MtpPacketTool::PutInt128(buffer, value.bin_.i128);
            break;
        case MTP_TYPE_UINT128_CODE:
        case MTP_TYPE_AUINT128_CODE:
            MtpPacketTool::PutUInt128(buffer, value.bin_.ui128);
            break;
        case MTP_TYPE_STRING_CODE:
            if (value.str_ == nullptr) {
                MtpPacketTool::PutUInt8(buffer, 0);
            } else {
                MtpPacketTool::PutString(buffer, *(value.str_));
            }
            break;
        default:
            MEDIA_ERR_LOG("Property::writeValue unknown type %{private}u", type_);
    }
}

bool Property::ReadArrayValues(const std::vector<uint8_t> &buffer, size_t &offset,
    std::shared_ptr<std::vector<Value>> &values)
{
    uint32_t length = 0;
    CHECK_AND_RETURN_RET(MtpPacketTool::GetUInt32(buffer, offset, length), false);
    bool cond = (length == 0 || (length >= (INT32_MAX / sizeof(Value))));
    CHECK_AND_RETURN_RET(!cond, false);
    if (values == nullptr) {
        values = std::make_shared<std::vector<Value>>();
    }

    values->clear();
    for (uint32_t i = 0; i < length; i++) {
        Value value;
        CHECK_AND_RETURN_RET(ReadValue(buffer, offset, value), false);
        values->push_back(value);
    }
    return true;
}

void Property::WriteArrayValues(std::vector<uint8_t> &buffer,
    const std::shared_ptr<std::vector<Value>> &values)
{
    uint32_t valueSum = (values == nullptr) ? 0 : values->size();
    MtpPacketTool::PutUInt32(buffer, valueSum);
    for (uint32_t i = 0; i < valueSum; i++) {
        WriteValue(buffer, (*values)[i]);
    }
}

void Property::Dump()
{
    int indent = 1;
    std::string indentStr = MtpPacketTool::GetIndentBlank(indent);

    MEDIA_DEBUG_LOG("handle=%{private}x", handle_);
    MEDIA_DEBUG_LOG("### Property {property=%{private}s(%{private}x)} begin ###",
        MtpPacketTool::GetObjectPropName(code_).c_str(), code_);
    MEDIA_DEBUG_LOG("%{private}stype=[%{private}s](%{private}x)}, writeable_=%{private}d",
        indentStr.c_str(), MtpPacketTool::GetDataTypeName(type_).c_str(), type_, writeable_);

    if (!IsArrayType()) {
        DumpValue(indent, defaultValue, "defaultValue");
        DumpValue(indent, currentValue, "currentValue");
    } else {
        DumpValues(indent, defaultValues, "defaultValues");
        DumpValues(indent, currentValues, "currentValues");
    }

    MEDIA_DEBUG_LOG("%{private}sgroupCode=%{private}u", indentStr.c_str(), groupCode_);
    DumpForm(indent);
    MEDIA_DEBUG_LOG("+++ Property end +++");
}

void Property::DumpValue(uint8_t indent, const std::shared_ptr<Value> &value, const std::string &name)
{
    std::string indentStr = MtpPacketTool::GetIndentBlank(indent);

    MEDIA_DEBUG_LOG("%{private}s%{private}s=%{private}s", indentStr.c_str(), name.c_str(),
        (value == nullptr) ? "nullptr" : value->ToString(type_).c_str());
}

void Property::DumpValues(uint8_t indent, const std::shared_ptr<std::vector<Value>> &values, const std::string &name)
{
    std::string indentStr = MtpPacketTool::GetIndentBlank(indent);

    if (values == nullptr) {
        MEDIA_DEBUG_LOG("%{private}s%{private}s=nullptr", indentStr.c_str(), name.c_str());
    } else {
        std::string indentStr2 = MtpPacketTool::GetIndentBlank(indent + 1);
        for (auto &v : (*values)) {
            MEDIA_DEBUG_LOG("%{private}s%{private}s", indentStr2.c_str(), v.ToString(type_).c_str());
        }
        MEDIA_DEBUG_LOG("%{private}s--- value end ---", indentStr.c_str());
    }
}

void Property::DumpForm(uint8_t indent)
{
    std::string indentStr = MtpPacketTool::GetIndentBlank(indent);

    MEDIA_DEBUG_LOG("%{private}sformFlag=%{private}s(%{private}u)",
        indentStr.c_str(), MtpPacketTool::CodeToStrByMap(formFlag_, FormMap).c_str(), formFlag_);

    if (formFlag_ == Form::Range) {
        DumpValue(indent + 1, minValue, "minValue");
        DumpValue(indent + 1, maxValue, "maxValue");
        DumpValue(indent + 1, stepSize, "stepSize");
    } else if (formFlag_ == Form::Enum) {
        DumpValues(indent + 1, enumValues, "enumValues");
    } else if (formFlag_ == Form::DateTime) {
        MEDIA_DEBUG_LOG("Form::DateTime");
    } else {
        MEDIA_DEBUG_LOG("unknow type");
    }
}
} // namespace Media
} // namespace OHOS