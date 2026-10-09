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
#define MLOG_TAG "Media_Client"

#include "mdk_record.h"
#include "mdk_database.h"
#include "json_helper.h"

namespace OHOS::Media::CloudSync {
// 获取记录id
std::string MDKRecord::GetRecordId() const
{
    return id_;
}
// 获取记录类型
std::string MDKRecord::GetRecordType() const
{
    return type_;
}
// 获取服务器上创建记录的信息，用于后续扩展，暂未使用
MDKRecordsResponse MDKRecord::GetRecordCreateInfo() const
{
    return createInfo_;
}
// 获取服务器上修改记录的信息，用于后续扩展，暂未使用
MDKRecordsResponse MDKRecord::GetRecordModifiedInfo() const
{
    return modifiedInfo_;
}
// 获取记录数据
void MDKRecord::GetRecordData(std::map<std::string, MDKRecordField> &fields) const
{
    fields = this->fields_;
}
// 记录是否被删除
bool MDKRecord::GetIsDelete() const
{
    return isDelete_;
}
// 是否为新增记录
bool MDKRecord::GetNewCreate() const
{
    return isNewCreate_;
}
// 获取记录的版本
int64_t MDKRecord::GetVersion() const
{
    return version_;
}
// 获取记录的创建时间
uint64_t MDKRecord::GetCreateTime() const
{
    return createdTime_;
}
// 获取记录的修改时间
uint64_t MDKRecord::GetEditedTime() const
{
    return editedTime_;
}
// 获取记录ownerID
std::string MDKRecord::GetOwnerId() const
{
    return ownerId_;
}
// 获取当前用户id
std::string MDKRecord::GetCurrentUserId() const
{
    return currentUserId_;
}
// 获取可操作权限,值参考：privilege flag value
uint32_t MDKRecord::GetPrivilege() const
{
    return privilege_;
}
// 获取分享uri
std::string MDKRecord::GetShareUri() const
{
    return shareUri_;
}
// 获取关联关系
void MDKRecord::GetRecordRelations(std::vector<MDKRelation> &relations) const
{
    relations = relations_;
}
// 获取记录共享状态
bool MDKRecord::GetShared() const
{
    return isShared_;
}

std::string MDKRecord::GetSrcRecordId() const
{
    return srcRecordId_;
}

// 内部接口,调用方不用
void MDKRecord::SetCreateInfo(MDKRecordsResponse create)
{
    createInfo_ = std::move(create);
}
// 内部接口,调用方不用
void MDKRecord::SetModifiedInfo(MDKRecordsResponse modified)
{
    modifiedInfo_ = std::move(modified);
}
// 设置记录id
void MDKRecord::SetRecordId(std::string id)
{
    id_ = id;
}
// 设置记录类型
void MDKRecord::SetRecordType(std::string type)
{
    type_ = type;
}
// 设置记录数据
void MDKRecord::SetRecordData(const std::map<std::string, MDKRecordField> &fields)
{
    this->fields_ = fields;
}
// 设置记录是否硬删除
void MDKRecord::SetDelete(bool isDelete)
{
    isDelete_ = isDelete;
}
// 设置记录现有的版本号
void MDKRecord::SetVersion(int64_t version)
{
    version_ = version;
}
// 设置记录是否为新增记录，不设置时默认为false
void MDKRecord::SetNewCreate(bool isNew)
{
    isNewCreate_ = isNew;
}
// 设置创建时间
void MDKRecord::SetCreateTime(uint64_t createdTime)
{
    createdTime_ = createdTime;
}
// 设置修改时间
void MDKRecord::SetEditedTime(uint64_t editedTime)
{
    editedTime_ = editedTime;
}
// 设置当前cursor版本目录软删除和硬删除才需要
void MDKRecord::SetBaseCursor(std::string cursor)
{
    baseCursor_ = std::move(cursor);
}
// 设置关联关系
void MDKRecord::SetRecordRelations(std::vector<MDKRelation> &relations)
{
    this->relations_ = relations;
}
// 设置记录ownerID
void MDKRecord::SetOwnerId(std::string ownerId)
{
    ownerId_ = std::move(ownerId);
}
// 设置当前用户id
void MDKRecord::SetCurrentUserId(std::string currentUserId)
{
    currentUserId_ = std::move(currentUserId);
}
// 设置记录是否共享
void MDKRecord::SetShared(bool isShared)
{
    isShared_ = isShared;
}

void MDKRecord::SetSrcRecordId(std::string srcRecordId)
{
    srcRecordId_ = srcRecordId;
}

Json::Value MDKRecord::AssetListToJsonValue(MDKRecordField &field)
{
    Json::Value jvData;
    std::vector<MDKRecordField> recordLst;
    field.GetRecordList(recordLst);
    for (auto &record : recordLst) {
        Json::Value jvAsset;
        jvAsset["asset"] = record.ToJsonValue();
        jvData.append(jvAsset);
    }
    return jvData;
}

Json::Value MDKRecord::ToJsonValue()
{
    Json::Value jvRecord;
    jvRecord["recordId"] = id_;
    jvRecord["recordType"] = type_;
    jvRecord["deleted"] = isDelete_;
    jvRecord["isNew"] = isNewCreate_;
    if (!baseCursor_.empty()) {
        jvRecord["baseCursor"] = baseCursor_;
    }
    jvRecord["baseVersion"] = version_;
    Json::Value jvFields;
    for (auto it = fields_.begin(); it != fields_.end(); it++) {
        if (type_ == "media" && it->first == "attachments") {
            if (it->second.GetType() == MDKRecordFieldType::FIELD_TYPE_LIST) {
                jvRecord[it->first] = AssetListToJsonValue(it->second);
                continue;
            }
        }
        jvRecord[it->first] = it->second.ToJsonValue();
    }
    if (createdTime_ != 0) {
        jvRecord["createdTime"] = createdTime_;
    }
    if (editedTime_ != 0) {
        jvRecord["editedTime"] = editedTime_;
    }
    Json::Value jvRelations;
    for (auto &relation : relations_) {
        Json::Value jvRelation;
        jvRelation["relationName"] = relation.relationName;
        jvRelation["recordType"] = relation.recordType;
        jvRelation["recordId"] = relation.recordId;
        jvRelations.append(jvRelation);
    }
    if (relations_.size() > 0) {
        jvRecord["relations"] = jvRelations;
    }
    if (!ownerId_.empty()) {
        jvRecord["ownerId"] = ownerId_;
    }
    if (isShared_) {
        jvRecord["isShared"] = isShared_;
    }
    return jvRecord;
}

void MDKRecord::ParseCreateInfoFromJson(const Json::Value &jvData)
{
    if (jvData.isMember("createInfo") && jvData["createInfo"].isObject()) {
        const Json::Value &jvCreate = jvData["createInfo"];
        createInfo_.appId = JsonHelper::GetStringFromJson(jvCreate, "appId");
        createInfo_.deviceName = JsonHelper::GetStringFromJson(jvCreate, "deviceName");
        createInfo_.time = JsonHelper::GetInt64FromJson(jvCreate, "time");
    }
}

void MDKRecord::ParseModifyInfoFromJson(const Json::Value &jvData)
{
    if (jvData.isMember("modifiedInfo") && jvData["modifiedInfo"].isObject()) {
        const Json::Value &jvModify = jvData["modifiedInfo"];
        modifiedInfo_.appId = JsonHelper::GetStringFromJson(jvModify, "appId");
        modifiedInfo_.deviceName = JsonHelper::GetStringFromJson(jvModify, "deviceName");
        modifiedInfo_.time = JsonHelper::GetInt64FromJson(jvModify, "time");
    }
}

void MDKRecord::ParseRelationsFromJson(const Json::Value &jvData)
{
    if (jvData.isMember("relations") && jvData["relations"].isArray()) {
        relations_.clear();
        Json::ArrayIndex maxCount = 500;
        for (Json::ArrayIndex j = 0; j < jvData["relations"].size() && j < maxCount; j++) {
            const Json::Value jvRelation = jvData["relations"][j];
            MDKRelation relation;
            relation.relationName = JsonHelper::GetStringFromJson(jvRelation, "relationName");
            relation.recordType = JsonHelper::GetStringFromJson(jvRelation, "recordType");
            relation.recordId = JsonHelper::GetStringFromJson(jvRelation, "recordId");
            relations_.push_back(std::move(relation));
        }
    }
}

bool MDKRecord::ParseFromJsonValue(const MDKSchema &schema, const Json::Value &jvData)
{
    if (!jvData.isObject()) {
        return false;
    }
    id_ = JsonHelper::GetStringFromJson(jvData, "recordId");
    type_ = JsonHelper::GetStringFromJson(jvData, "recordType");
    auto it = schema.recordTypes.find(type_);
    if (it == schema.recordTypes.end()) {
        return false;
    }
    const MDKSchemaNode &schemaNode = it->second;
    isDelete_ = JsonHelper::GetBoolFromJson(jvData, "deleted");
    isNewCreate_ = JsonHelper::GetBoolFromJson(jvData, "isNew");
    version_ = JsonHelper::GetInt64FromJson(jvData, "version");
    createdTime_ = JsonHelper::GetUInt64FromJson(jvData, "createdTime");
    editedTime_ = JsonHelper::GetUInt64FromJson(jvData, "editedTime");
    ownerId_ = JsonHelper::GetStringFromJson(jvData, "ownerId");
    shareUri_ = JsonHelper::GetStringFromJson(jvData, "shareUri");
    privilege_ = JsonHelper::GetUIntFromJson(jvData, "privilege");
    isShared_ = JsonHelper::GetBoolFromJson(jvData, "isShared");
    if (jvData.isMember("record") && jvData["record"].isObject()) {
        const Json::Value &jvRecordData = jvData["record"];
        auto mem = jvRecordData.getMemberNames();
        for (auto &key : mem) {
            MDKRecordField field;
            auto it = schemaNode.fields.find(key);
            if (it == schemaNode.fields.end()) {
                continue;
            }
            if (field.ParseFromJsonValue(it->second, jvRecordData[key])) {
                fields_[key] = field;
            }
        }
    }
    ParseCreateInfoFromJson(jvData);
    ParseModifyInfoFromJson(jvData);
    ParseRelationsFromJson(jvData);
    return true;
}
}  // namespace OHOS::Media::CloudSync