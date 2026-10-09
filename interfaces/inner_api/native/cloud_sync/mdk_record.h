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
#ifndef OHOS_MEDIA_CLOUD_SYNC_RECORD_H
#define OHOS_MEDIA_CLOUD_SYNC_RECORD_H

#include <map>

#include "mdk_record_field.h"
#include "mdk_reference.h"
#include "json/json.h"

#define EXPORT __attribute__ ((visibility ("default")))

namespace OHOS::Media::CloudSync {
struct MDKRelation {
    std::string relationName;
    std::string recordType;
    std::string recordId;
};

struct MDKRecordsResponse {
    // 参考服务端的公共接口 遗留
    int64_t time;
    std::string deviceName;
    std::string appId;
};

struct MDKSchema;
class EXPORT MDKRecord {
public:
    // 获取记录id
    std::string GetRecordId() const;
    // 获取记录类型
    std::string GetRecordType() const;
    // 获取服务器上创建记录的信息，用于后续扩展，暂未使用
    MDKRecordsResponse GetRecordCreateInfo() const;
    // 获取服务器上修改记录的信息，用于后续扩展，暂未使用
    MDKRecordsResponse GetRecordModifiedInfo() const;
    // 获取记录数据
    void GetRecordData(std::map<std::string, MDKRecordField> &fields) const;
    // 记录是否被删除
    bool GetIsDelete() const;
    // 是否为新增记录
    bool GetNewCreate() const;
    // 获取记录的版本
    int64_t GetVersion() const;
    // 获取记录的创建时间
    uint64_t GetCreateTime() const;
    // 获取记录的修改时间
    uint64_t GetEditedTime() const;
    // 获取记录ownerID
    std::string GetOwnerId() const;
    // 获取当前用户id
    std::string GetCurrentUserId() const;
    // 获取可操作权限,值参考：privilege flag value
    uint32_t GetPrivilege() const;
    // 获取分享uri
    std::string GetShareUri() const;
    // 获取关联关系
    void GetRecordRelations(std::vector<MDKRelation> &relations) const;
    // 获取记录共享状态
    bool GetShared() const;
    std::string GetSrcRecordId() const;
    // 内部接口,调用方不用
    void SetCreateInfo(MDKRecordsResponse create);
    // 内部接口,调用方不用
    void SetModifiedInfo(MDKRecordsResponse modified);
    // 设置记录id
    void SetRecordId(std::string id);
    // 设置记录类型
    void SetRecordType(std::string type);
    // 设置记录数据
    void SetRecordData(const std::map<std::string, MDKRecordField> &fields);
    // 设置记录是否硬删除
    void SetDelete(bool isDelete);
    // 设置记录现有的版本号
    void SetVersion(int64_t version);
    // 设置记录是否为新增记录，不设置时默认为false
    void SetNewCreate(bool isNew);
    // 设置创建时间
    void SetCreateTime(uint64_t createdTime);
    // 设置修改时间
    void SetEditedTime(uint64_t editedTime);
    // 设置当前cursor版本目录软删除和硬删除才需要
    void SetBaseCursor(std::string cursor);
    // 设置关联关系
    void SetRecordRelations(std::vector<MDKRelation> &relations);
    // 设置记录ownerID
    void SetOwnerId(std::string ownerId);
    // 设置当前用户id
    void SetCurrentUserId(std::string currentUserId);
    // 设置记录是否共享
    void SetShared(bool isShared);
    void SetSrcRecordId(std::string srcRecordId);
    Json::Value ToJsonValue();
    bool ParseFromJsonValue(const MDKSchema &schema, const Json::Value &jvData);
    void ParseCreateInfoFromJson(const Json::Value &jvData);
    void ParseModifyInfoFromJson(const Json::Value &jvData);
    void ParseRelationsFromJson(const Json::Value &jvData);

private:
    Json::Value AssetListToJsonValue(MDKRecordField &field);

private:
    std::string id_;
    std::string type_;
    MDKRecordsResponse createInfo_;
    MDKRecordsResponse modifiedInfo_;
    std::map<std::string, MDKRecordField> fields_;
    std::vector<MDKRelation> relations_;
    bool isDelete_ = false;
    bool isNewCreate_ = false;
    int64_t version_ = 0;
    uint64_t createdTime_ = 0;
    uint64_t editedTime_ = 0;
    std::string ownerId_;
    std::string currentUserId_;
    std::string shareUri_;
    uint32_t privilege_ = 0;  // 被共享者具备对该记录的操作能力，值参考：privilege flag value
    std::string baseCursor_;  // 目录软删除和硬删除才需要
    bool isShared_ = false;
    std::string srcRecordId_;
};
}  // namespace OHOS::Media::CloudSync
#endif