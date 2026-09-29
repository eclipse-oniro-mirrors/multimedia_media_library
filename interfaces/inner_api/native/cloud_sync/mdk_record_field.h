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
#ifndef OHOS_MEDIA_CLOUD_SYNC_RECORD_FIELD_H
#define OHOS_MEDIA_CLOUD_SYNC_RECORD_FIELD_H
#include <map>
#include <string>
#include <variant>
#include <vector>

#include "mdk_asset.h"
#include "mdk_error.h"
#include "mdk_reference.h"
#include "mdk_participant.h"
#include "mdk_scadetail.h"
#include "json/json.h"

#define EXPORT __attribute__ ((visibility ("default")))

namespace OHOS::Media::CloudSync {
// 记录中值的类型
enum class MDKRecordFieldType {
    FIELD_TYPE_NULL = 0,
    FIELD_TYPE_INT,        // int64_t
    FIELD_TYPE_DOUBLE,     // double
    FIELD_TYPE_STRING,     // std::string
    FIELD_TYPE_BOOL,       // bool
    FIELD_TYPE_BYTES,       // std::vector<uint8_t>
    FIELD_TYPE_LIST,       // std::vector<MDKRecordField>
    FIELD_TYPE_MAP,        // std::map<std::string, MDKRecordField>
    FIELD_TYPE_ASSET,      // MDKAsset
    FIELD_TYPE_REFERENCE,  // MDKReference
    FIELD_TYPE_PARTICIPANT, // Participant
    FIELD_TYPE_SCADETAIL,  // MDKScadetail
};
struct MDKSchemaField;
class MDKRecordField;
using MDKFieldValue = std::variant<std::monostate, int64_t, double, std::string, bool, std::vector<uint8_t>,
    std::vector<MDKRecordField>, std::map<std::string, MDKRecordField>, MDKAsset, MDKReference, MDKParticipant,
    MDKScadetail>;
// MDKRecordField用来保存记录中的值

class EXPORT MDKRecordField {
public:
    MDKRecordField();
    ~MDKRecordField();
    MDKRecordField(MDKFieldValue fieldValue) noexcept;
    MDKRecordField(const MDKRecordField &recordField);
    explicit MDKRecordField(int val);
    explicit MDKRecordField(int64_t val);
    explicit MDKRecordField(double val);
    explicit MDKRecordField(bool val);
    explicit MDKRecordField(const char *val);
    explicit MDKRecordField(const std::string &val);
    explicit MDKRecordField(const std::vector<uint8_t> &val);
    explicit MDKRecordField(std::map<std::string, MDKRecordField> &val);
    explicit MDKRecordField(std::vector<MDKRecordField> &val);
    explicit MDKRecordField(MDKAsset &val);
    explicit MDKRecordField(MDKReference &val);
    explicit MDKRecordField(MDKParticipant &val);
    explicit MDKRecordField(MDKScadetail &val);
    MDKRecordField &operator=(const MDKRecordField &recordField);
    MDKRecordFieldType GetType() const;
    MDKFieldValue GetFieldValue() const;
    MDKLocalErrorCode GetInt(int &val) const;
    MDKLocalErrorCode GetLong(int64_t &val) const;
    MDKLocalErrorCode GetDouble(double &val) const;
    MDKLocalErrorCode GetBool(bool &val) const;
    MDKLocalErrorCode GetString(std::string &val) const;
    MDKLocalErrorCode GetBytes(std::vector<uint8_t> &val) const;
    MDKLocalErrorCode GetRecordList(std::vector<MDKRecordField> &val) const;
    MDKLocalErrorCode GetRecordMap(std::map<std::string, MDKRecordField> &val) const;
    MDKLocalErrorCode GetAsset(MDKAsset &val) const;
    MDKLocalErrorCode GetReference(MDKReference &val) const;
    MDKLocalErrorCode GetParticipant(MDKParticipant &val) const;
    MDKLocalErrorCode GetScadetail(MDKScadetail &val) const;

    operator int() const
    {
        return static_cast<int>(int64_t(std::get<int64_t>(value_)));
    }
    operator int64_t() const
    {
        return std::get<int64_t>(value_);
    }
    operator double() const
    {
        return std::get<double>(value_);
    }
    operator bool() const
    {
        return std::get<bool>(value_);
    }
    operator std::string() const
    {
        return std::get<std::string>(value_);
    }
    operator std::vector<uint8_t>() const
    {
        return std::get<std::vector<uint8_t>>(value_);
    }
    operator std::vector<MDKRecordField>() const
    {
        return std::get<std::vector<MDKRecordField>>(value_);
    }
    operator std::map<std::string, MDKRecordField>() const
    {
        return std::get<std::map<std::string, MDKRecordField>>(value_);
    }
    operator MDKAsset() const
    {
        return std::get<MDKAsset>(value_);
    }
    operator MDKReference() const
    {
        return std::get<MDKReference>(value_);
    }
    operator MDKParticipant() const
    {
        return std::get<MDKParticipant>(value_);
    }
    operator MDKScadetail() const
    {
        return std::get<MDKScadetail>(value_);
    }
    operator MDKFieldValue() const
    {
        return value_;
    }
    Json::Value ToJsonValue();
    bool ParseFromJsonValue(const MDKSchemaField &schemaField, const Json::Value &jvData);

private:
    Json::Value FieldListToJsonValue();
    Json::Value FieldMapToJsonValue();
    Json::Value AssetToJsonValue(const MDKAsset &asset);
    Json::Value FieldAssetToJsonValue();
    Json::Value FieldReferenceToJsonValue();
    Json::Value FieldParticipantToJsonValue();
    Json::Value FieldScadetailToJsonValue();
    bool ParseIntFromJson(const Json::Value &jvData);
    bool ParseDoubleFromJson(const Json::Value &jvData);
    bool ParseStringFromJson(const Json::Value &jvData);
    bool ParseBoolFromJson(const Json::Value &jvData);
    bool ParseBytesFromJson(const Json::Value &jvData);
    bool ParseListFromJson(MDKRecordFieldType listType, const Json::Value &jvData);
    bool ParseMapFromJson(const Json::Value &jvData);
    bool ParseAssetFromJson(const Json::Value &jvData);
    bool ParseReferenceFromJson(const Json::Value &jvData);
    bool ParseParticipantFromJson(const Json::Value &jvData);
    bool ParseScadetailFromJson(const Json::Value &jvData);

    MDKAsset ParseAssetFromJsonValue(const Json::Value &jvData);

private:
    MDKRecordFieldType type_;
    MDKFieldValue value_;
};
}  // namespace OHOS::Media::CloudSync
#endif