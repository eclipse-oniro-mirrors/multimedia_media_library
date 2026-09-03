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
#ifndef OHOS_MEDIA_CLOUD_SYNC_DK_RECORD_PARSER_H
#define OHOS_MEDIA_CLOUD_SYNC_DK_RECORD_PARSER_H

#include <string>
#include <limits>

#include "dk_asset.h"
#include "dk_database.h"
#include "dk_error.h"
#include "dk_record_field.h"
#include "dk_record.h"
#include "dk_reference.h"
#include "dk_participant.h"
#include "dk_scadetail.h"
#include "dk_result.h"

#include "mdk_asset.h"
#include "mdk_database.h"
#include "mdk_error.h"
#include "mdk_record_field.h"
#include "mdk_record.h"
#include "mdk_reference.h"
#include "mdk_participant.h"
#include "mdk_scadetail.h"
#include "mdk_result.h"

#define EXPORT __attribute__ ((visibility ("default")))

namespace OHOS::Media::CloudSync {
using namespace DriveKit;

template <typename ORIGIN_RECORD_TYPE, typename TARGET_RECORD_TYPE,              // DKRecord
    typename ORIGIN_FIELD_TYPE, typename TARGET_FIELD_TYPE,                      // DKRecordField
    typename ORIGIN_ASSET_TYPE, typename TARGET_ASSET_TYPE,                      // DKAsset
    typename ORIGIN_REFERENCE_TYPE, typename TARGET_REFERENCE_TYPE,              // DKReference
    typename ORIGIN_RELATION_TYPE, typename TARGET_RELATION_TYPE,                // DKRelation
    typename ORIGIN_RESPONSE_TYPE, typename TARGET_RESPONSE_TYPE,                // DKRecordsResponse
    typename ORIGIN_SCHEMA_TYPE, typename TARGET_SCHEMA_TYPE,                    // DKSchemaField
    typename ORIGIN_SCHEMA_RELATION_TYPE, typename TARGET_SCHEMA_RELATION_TYPE,  // DKSchemaRelation
    typename ORIGIN_SCHEMA_NODE_TYPE, typename TARGET_SCHEMA_NODE_TYPE,          // DKSchemaNode
    typename ORIGIN_ORDER_TABLE_TYPE, typename TARGET_ORDER_TABLE_TYPE,          // DKOrderTable
    typename ORIGIN_ERROR_DETAIL_TYPE, typename TARGET_ERROR_DETAIL_TYPE,        // DKErrorDetail
    typename ORIGIN_ERROR_TYPE, typename TARGET_ERROR_TYPE,                      // DKError
    typename ORIGIN_RESULT_TYPE, typename TARGET_RESULT_TYPE,                    // DKRecordOperResult
    typename ORIGIN_PARTICIPANT_TYPE, typename TARGET_PARTICIPANT_TYPE,          // Participant
    typename ORIGIN_SCADETAIL_TYPE, typename TARGET_SCADETAIL_TYPE              // DKScadetail
    >
class EXPORT DKRecordParser {
private:
    MDKRecordFieldType CDKRecordFieldType(const DKRecordFieldType &fieldType)
    {
        return static_cast<MDKRecordFieldType>(static_cast<int32_t>(fieldType));
    }

    DKRecordFieldType CDKRecordFieldType(const MDKRecordFieldType &fieldType)
    {
        return static_cast<DKRecordFieldType>(static_cast<int32_t>(fieldType));
    }
    TARGET_SCHEMA_TYPE CDKSchemaField(const ORIGIN_SCHEMA_TYPE &field)
    {
        TARGET_SCHEMA_TYPE dkField;
        dkField.name = field.name;                              // 名称
        dkField.type = CDKRecordFieldType(field.type);          // 数据类型
        dkField.primary = field.primary;                        // 是否为云端主键
        dkField.nullable = field.nullable;                      // 是否可为空
        dkField.sortable = field.sortable;                      // 是否支持排序
        dkField.searchable = field.searchable;                  // 是否支持搜索
        dkField.queryable = field.queryable;                    // 是否支持查询
        dkField.listType = CDKRecordFieldType(field.listType);  // 当type为list时，listType表示list的类型
        // 当type或listType为Reference类型时，该字段表示被引用的RecordType
        dkField.refRecordType = field.refRecordType;
        return dkField;
    }
    std::map<std::string, TARGET_SCHEMA_TYPE> CDKSchemaField(const std::map<std::string, ORIGIN_SCHEMA_TYPE> &fields)
    {
        std::map<std::string, TARGET_SCHEMA_TYPE> dkFields;
        for (auto &fieldPair : fields) {
            dkFields[fieldPair.first] = CDKSchemaField(fieldPair.second);
        }
        return dkFields;
    }
    TARGET_SCHEMA_RELATION_TYPE CDKSchemaRelation(const ORIGIN_SCHEMA_RELATION_TYPE &relation)
    {
        TARGET_SCHEMA_RELATION_TYPE dkRelation;
        dkRelation.relationName = relation.relationName;
        dkRelation.recordType = relation.recordType;
        dkRelation.refFields = relation.refFields;
        return dkRelation;
    }

    std::vector<TARGET_SCHEMA_RELATION_TYPE> CDKSchemaRelation(
        const std::vector<ORIGIN_SCHEMA_RELATION_TYPE> &relations)
    {
        std::vector<TARGET_SCHEMA_RELATION_TYPE> dkRelations;
        for (auto &relation : relations) {
            dkRelations.emplace_back(CDKSchemaRelation(relation));
        }
        return dkRelations;
    }

    TARGET_SCHEMA_NODE_TYPE CDKSchemaNode(const ORIGIN_SCHEMA_NODE_TYPE &node)
    {
        TARGET_SCHEMA_NODE_TYPE dkNode;
        dkNode.recordType = node.recordType;
        dkNode.tableName = node.tableName;
        dkNode.fields = CDKSchemaField(node.fields);
        dkNode.dupCheckFields = node.dupCheckFields;
        dkNode.sharedTableName = node.sharedTableName;
        dkNode.relations = CDKSchemaRelation(node.relations);
        return dkNode;
    }

    std::map<std::string, TARGET_SCHEMA_NODE_TYPE> CDKSchemaNode(
        const std::map<std::string, ORIGIN_SCHEMA_NODE_TYPE> &recordTypes)
    {
        std::map<std::string, TARGET_SCHEMA_NODE_TYPE> dkRecordTypes;
        for (auto &recordType : recordTypes) {
            dkRecordTypes[recordType.first] = CDKSchemaNode(recordType.second);
        }
        return dkRecordTypes;
    }

    TARGET_ORDER_TABLE_TYPE CDKOrderTable(const ORIGIN_ORDER_TABLE_TYPE &node)
    {
        TARGET_ORDER_TABLE_TYPE dkNode;
        dkNode.recordType = node.recordType;
        dkNode.tableName = node.tableName;
        return dkNode;
    }

    std::vector<TARGET_ORDER_TABLE_TYPE> CDKOrderTable(const std::vector<ORIGIN_ORDER_TABLE_TYPE> &orderTables)
    {
        std::vector<TARGET_ORDER_TABLE_TYPE> dkOrderTables;
        for (auto &orderTable : orderTables) {
            dkOrderTables.emplace_back(CDKOrderTable(orderTable));
        }
        return dkOrderTables;
    }

    MDKAssetOperType CDKAssetOperType(const DKAssetOperType &fieldType)
    {
        return static_cast<MDKAssetOperType>(static_cast<int32_t>(fieldType));
    }

    DKAssetOperType CDKAssetOperType(const MDKAssetOperType &fieldType)
    {
        return static_cast<DKAssetOperType>(static_cast<int32_t>(fieldType));
    }

    TARGET_ASSET_TYPE CDKAsset(const ORIGIN_ASSET_TYPE &asset)
    {
        TARGET_ASSET_TYPE dkAsset;
        dkAsset.uri = asset.uri;
        dkAsset.assetName = asset.assetName;
        dkAsset.operationType = CDKAssetOperType(asset.operationType);
        dkAsset.hash = asset.hash;
        dkAsset.version = asset.version;
        dkAsset.assetId = asset.assetId;
        dkAsset.subPath = asset.subPath;
        dkAsset.exCheckInfo = asset.exCheckInfo;
        dkAsset.size = asset.size;
        dkAsset.fd = asset.fd;
        return dkAsset;
    }
    TARGET_ERROR_DETAIL_TYPE CDKErrorDetail(const ORIGIN_ERROR_DETAIL_TYPE &errorDetail)
    {
        TARGET_ERROR_DETAIL_TYPE dkErrorDetail;
        dkErrorDetail.domain = errorDetail.domain;
        dkErrorDetail.reason = errorDetail.reason;
        dkErrorDetail.errorCode = errorDetail.errorCode;
        dkErrorDetail.description = errorDetail.description;
        dkErrorDetail.errorPos = errorDetail.errorPos;
        dkErrorDetail.errorParam = errorDetail.errorParam;
        dkErrorDetail.detailCode = errorDetail.detailCode;
        return dkErrorDetail;
    }

    std::vector<TARGET_ERROR_DETAIL_TYPE> CDKErrorDetail(const std::vector<ORIGIN_ERROR_DETAIL_TYPE> &errorDetails)
    {
        std::vector<TARGET_ERROR_DETAIL_TYPE> dkErrorDetails;
        for (auto &errorDetail : errorDetails) {
            dkErrorDetails.emplace_back(CDKErrorDetail(errorDetail));
        }
        return dkErrorDetails;
    }

    MDKLocalErrorCode CDKLocalErrorCode(const DKLocalErrorCode &localErrorCode)
    {
        return static_cast<MDKLocalErrorCode>(static_cast<int32_t>(localErrorCode));
    }

    DKLocalErrorCode CDKLocalErrorCode(const MDKLocalErrorCode &localErrorCode)
    {
        return static_cast<DKLocalErrorCode>(static_cast<int32_t>(localErrorCode));
    }

    MDKErrorType CDKErrorType(const DKErrorType &errorType)
    {
        return static_cast<MDKErrorType>(static_cast<int32_t>(errorType));
    }

    DKErrorType CDKErrorType(const MDKErrorType &errorType)
    {
        return static_cast<DKErrorType>(static_cast<int32_t>(errorType));
    }

    TARGET_ERROR_TYPE CDKError(const ORIGIN_ERROR_TYPE &error)
    {
        TARGET_ERROR_TYPE dkError;
        dkError.isLocalError = error.isLocalError;
        dkError.dkErrorCode = CDKLocalErrorCode(error.dkErrorCode);
        dkError.isServerError = error.isServerError;
        dkError.serverErrorCode = error.serverErrorCode;
        dkError.reason = error.reason;
        dkError.errorDetails = CDKErrorDetail(error.errorDetails);
        dkError.retryAfter = error.retryAfter;
        dkError.errorType = CDKErrorType(error.errorType);
        return dkError;
    }
    TARGET_REFERENCE_TYPE CDKReference(const ORIGIN_REFERENCE_TYPE &reference)
    {
        TARGET_REFERENCE_TYPE dkReference;
        dkReference.recordType = reference.recordType;
        dkReference.recordId = reference.recordId;
        return dkReference;
    }

    MDKRoleType CDKRoleType(const Roletype &roleType)
    {
        return static_cast<MDKRoleType>(static_cast<int32_t>(roleType));
    }

    MDKRoleType CDKRoleType(const MDKRoleType &roleType)
    {
        return static_cast<MDKRoleType>(static_cast<int32_t>(roleType));
    }

    MDKUserType CDKUserType(const UserType &userType)
    {
        return static_cast<MDKUserType>(static_cast<int32_t>(userType));
    }

    UserType CDKUserType(const MDKUserType &userType)
    {
        return static_cast<UserType>(static_cast<int32_t>(userType));
    }

    MDKAccountType CDKAccountType(const AccountType &accountType)
    {
        return static_cast<MDKAccountType>(static_cast<int32_t>(accountType));
    }

    AccountType CDKAccountType(const MDKAccountType &accountType)
    {
        return static_cast<AccountType>(static_cast<int32_t>(accountType));
    }

    MDKShareStatus CDKShareStatus(const ShareStatus&shareStatus)
    {
        return static_cast<MDKShareStatus>(static_cast<int32_t>(shareStatus));
    }

    ShareStatus CDKShareStatus(const MDKShareStatus &shareStatus)
    {
        return static_cast<ShareStatus>(static_cast<int32_t>(shareStatus));
    }

    MDKCategory CDKCategoryType(const Category &category)
    {
        return static_cast<MDKCategory>(static_cast<int32_t>(category));
    }

    Category CDKCategoryType(const MDKCategory &category)
    {
        return static_cast<Category>(static_cast<int32_t>(category));
    }

    TARGET_PARTICIPANT_TYPE CDKParticipant(const ORIGIN_PARTICIPANT_TYPE &participant)
    {
        TARGET_PARTICIPANT_TYPE dkParticipant;
        dkParticipant.userId = participant.userId;
        dkParticipant.role = CDKRoleType(participant.role);
        dkParticipant.type = CDKUserType(participant.type);
        dkParticipant.userAccount = participant.userAccount;
        dkParticipant.displayName = participant.displayName;
        dkParticipant.profilePhotoLink = participant.profilePhotoLink;
        dkParticipant.accountType = CDKAccountType(participant.accountType);
        dkParticipant.status = CDKShareStatus(participant.status);
        dkParticipant.id = participant.id;
        dkParticipant.properties = participant.properties;
        dkParticipant.createdTime = participant.createdTime;
        dkParticipant.modifiedTime = participant.modifiedTime;
        dkParticipant.expirationTime = participant.expirationTime;
        dkParticipant.category = CDKCategoryType(participant.category);
        return dkParticipant;
    }

    TARGET_SCADETAIL_TYPE CDKScadetail(const ORIGIN_SCADETAIL_TYPE &scadetail)
    {
        TARGET_SCADETAIL_TYPE dkScadetail;
        dkScadetail.usage = scadetail.usage;
        dkScadetail.scaState = scadetail.scaState;
        dkScadetail.scaRank = scadetail.scaRank;
        dkScadetail.scaVersion = scadetail.scaVersion;
        dkScadetail.scaAttributes = scadetail.scaAttributes;
        dkScadetail.riskResult = scadetail.riskResult;
        return dkScadetail;
    }

    bool CDKRecordFieldInt(const ORIGIN_FIELD_TYPE &field, TARGET_FIELD_TYPE &value)
    {
        int32_t type = static_cast<int32_t>(field.GetType());
        if (type == static_cast<int32_t>(MDKRecordFieldType::FIELD_TYPE_INT)) {
            int64_t intVal;
            field.GetLong(intVal);
            value = TARGET_FIELD_TYPE(intVal);
            return true;
        }
        return false;
    }

    bool CDKRecordFieldDouble(const ORIGIN_FIELD_TYPE &field, TARGET_FIELD_TYPE &value)
    {
        int32_t type = static_cast<int32_t>(field.GetType());
        if (type == static_cast<int32_t>(MDKRecordFieldType::FIELD_TYPE_DOUBLE)) {
            double doubleVal;
            field.GetDouble(doubleVal);
            value = TARGET_FIELD_TYPE(doubleVal);
            return true;
        }
        return false;
    }

    bool CDKRecordFieldString(const ORIGIN_FIELD_TYPE &field, TARGET_FIELD_TYPE &value)
    {
        int32_t type = static_cast<int32_t>(field.GetType());
        if (type == static_cast<int32_t>(MDKRecordFieldType::FIELD_TYPE_STRING)) {
            std::string stringVal;
            field.GetString(stringVal);
            value = TARGET_FIELD_TYPE(stringVal);
            return true;
        }
        return false;
    }

    bool CDKRecordFieldBool(const ORIGIN_FIELD_TYPE &field, TARGET_FIELD_TYPE &value)
    {
        int32_t type = static_cast<int32_t>(field.GetType());
        if (type == static_cast<int32_t>(MDKRecordFieldType::FIELD_TYPE_BOOL)) {
            bool boolVal;
            field.GetBool(boolVal);
            value = TARGET_FIELD_TYPE(boolVal);
            return true;
        }
        return false;
    }

    bool CDKRecordFieldBlob(const ORIGIN_FIELD_TYPE &field, TARGET_FIELD_TYPE &value)
    {
        int32_t type = static_cast<int32_t>(field.GetType());
        if (type == static_cast<int32_t>(MDKRecordFieldType::FIELD_TYPE_BYTES)) {
            std::vector<uint8_t> blobVal;
            field.GetBytes(blobVal);
            value = TARGET_FIELD_TYPE(blobVal);
            return true;
        }
        return false;
    }

    std::map<std::string, TARGET_FIELD_TYPE> CDKRecordField(const std::map<std::string, ORIGIN_FIELD_TYPE> &fields)
    {
        std::map<std::string, TARGET_FIELD_TYPE> dkFields;
        for (auto &fieldPair : fields) {
            dkFields[fieldPair.first] = CDKRecordField(fieldPair.second);
        }
        return dkFields;
    }

    bool CDKRecordFieldList(const ORIGIN_FIELD_TYPE &field, TARGET_FIELD_TYPE &value)
    {
        int32_t type = static_cast<int32_t>(field.GetType());
        if (type == static_cast<int32_t>(MDKRecordFieldType::FIELD_TYPE_LIST)) {
            std::vector<ORIGIN_FIELD_TYPE> fieldListVal;
            field.GetRecordList(fieldListVal);
            std::vector<TARGET_FIELD_TYPE> dkFieldList;
            for (auto &field : fieldListVal) {
                TARGET_FIELD_TYPE dkField = CDKRecordField(field);
                dkFieldList.emplace_back(dkField);
            }
            value = TARGET_FIELD_TYPE(dkFieldList);
            return true;
        }
        return false;
    }

    bool CDKRecordFieldMap(const ORIGIN_FIELD_TYPE &field, TARGET_FIELD_TYPE &value)
    {
        int32_t type = static_cast<int32_t>(field.GetType());
        if (type == static_cast<int32_t>(MDKRecordFieldType::FIELD_TYPE_MAP)) {
            std::map<std::string, ORIGIN_FIELD_TYPE> fieldMapVal;
            field.GetRecordMap(fieldMapVal);
            std::map<std::string, TARGET_FIELD_TYPE> dkFields;
            for (auto &fieldPair : fieldMapVal) {
                TARGET_FIELD_TYPE dkField = CDKRecordField(fieldPair.second);
                dkFields[fieldPair.first] = dkField;
            }
            value = TARGET_FIELD_TYPE(dkFields);
            return true;
        }
        return false;
    }

    bool CDKRecordFieldAsset(const ORIGIN_FIELD_TYPE &field, TARGET_FIELD_TYPE &value)
    {
        int32_t type = static_cast<int32_t>(field.GetType());
        if (type == static_cast<int32_t>(MDKRecordFieldType::FIELD_TYPE_ASSET)) {
            ORIGIN_ASSET_TYPE assetVal;
            field.GetAsset(assetVal);
            value = TARGET_FIELD_TYPE(CDKAsset(assetVal));
            return true;
        }
        return false;
    }

    bool CDKRecordFieldReference(const ORIGIN_FIELD_TYPE &field, TARGET_FIELD_TYPE &value)
    {
        int32_t type = static_cast<int32_t>(field.GetType());
        if (type == static_cast<int32_t>(MDKRecordFieldType::FIELD_TYPE_REFERENCE)) {
            ORIGIN_REFERENCE_TYPE referenceVal;
            field.GetReference(referenceVal);
            value = TARGET_FIELD_TYPE(CDKReference(referenceVal));
            return true;
        }
        return false;
    }

    bool CDKRecordFieldParticipant(const ORIGIN_FIELD_TYPE &field, TARGET_FIELD_TYPE &value)
    {
        int32_t type = static_cast<int32_t>(field.GetType());
        if (type == static_cast<int32_t>(MDKRecordFieldType::FIELD_TYPE_PARTICIPANT)) {
            ORIGIN_PARTICIPANT_TYPE participant;
            field.GetParticipant(participant);
            value = TARGET_FIELD_TYPE(CDKParticipant(participant));
            return true;
        }
        return false;
    }

    bool CDKRecordFieldScadetail(const ORIGIN_FIELD_TYPE &field, TARGET_FIELD_TYPE &value)
    {
        int32_t type = static_cast<int32_t>(field.GetType());
        if (type == static_cast<int32_t>(MDKRecordFieldType::FIELD_TYPE_SCADETAIL)) {
            ORIGIN_SCADETAIL_TYPE scadetail;
            field.GetScadetail(scadetail);
            value = TARGET_FIELD_TYPE(CDKScadetail(scadetail));
            return true;
        }
        return false;
    }

    TARGET_FIELD_TYPE CDKRecordField(const ORIGIN_FIELD_TYPE &field)
    {
        TARGET_FIELD_TYPE dkField;
        bool isValid = CDKRecordFieldInt(field, dkField);
        isValid = isValid || CDKRecordFieldDouble(field, dkField);
        isValid = isValid || CDKRecordFieldString(field, dkField);
        isValid = isValid || CDKRecordFieldBool(field, dkField);
        isValid = isValid || CDKRecordFieldBlob(field, dkField);
        isValid = isValid || CDKRecordFieldAsset(field, dkField);
        isValid = isValid || CDKRecordFieldReference(field, dkField);
        isValid = isValid || CDKRecordFieldParticipant(field, dkField);
        isValid = isValid || CDKRecordFieldScadetail(field, dkField);
        isValid = isValid || CDKRecordFieldList(field, dkField);
        isValid = isValid || CDKRecordFieldMap(field, dkField);
        return dkField;
    }

    TARGET_RESPONSE_TYPE CDKRecordsResponse(const ORIGIN_RESPONSE_TYPE &response)
    {
        TARGET_RESPONSE_TYPE dkResponse;
        dkResponse.time = response.time;
        dkResponse.deviceName = response.deviceName;
        dkResponse.appId = response.appId;
        return dkResponse;
    }

    TARGET_RELATION_TYPE CDKRelation(const ORIGIN_RELATION_TYPE &relation)
    {
        TARGET_RELATION_TYPE dkRelation;
        dkRelation.relationName = relation.relationName;
        dkRelation.recordType = relation.recordType;
        dkRelation.recordId = relation.recordId;
        return dkRelation;
    }

    std::vector<TARGET_RELATION_TYPE> CDKRelation(const std::vector<ORIGIN_RELATION_TYPE> &relations)
    {
        std::vector<TARGET_RELATION_TYPE> dkRelations;
        for (auto &relation : relations) {
            dkRelations.emplace_back(CDKRelation(relation));
        }
        return dkRelations;
    }

public:
    TARGET_RECORD_TYPE CDKRecord(const ORIGIN_RECORD_TYPE &record)
    {
        TARGET_RECORD_TYPE dkRecord;
        dkRecord.SetRecordId(record.GetRecordId());
        dkRecord.SetRecordType(record.GetRecordType());
        dkRecord.SetCreateInfo(CDKRecordsResponse(record.GetRecordCreateInfo()));
        dkRecord.SetModifiedInfo(CDKRecordsResponse(record.GetRecordModifiedInfo()));
        std::map<std::string, ORIGIN_FIELD_TYPE> recordFields;
        record.GetRecordData(recordFields);
        dkRecord.SetRecordData(CDKRecordField(recordFields));
        std::vector<ORIGIN_RELATION_TYPE> relations;
        record.GetRecordRelations(relations);
        std::vector<TARGET_RELATION_TYPE> targetRelations = CDKRelation(relations);
        dkRecord.SetRecordRelations(targetRelations);
        dkRecord.SetDelete(record.GetIsDelete());
        dkRecord.SetNewCreate(record.GetNewCreate());
        dkRecord.SetVersion(record.GetVersion());
        dkRecord.SetCreateTime(record.GetCreateTime());
        dkRecord.SetEditedTime(record.GetEditedTime());
        dkRecord.SetOwnerId(record.GetOwnerId());
        // shareUri_ & privilege_ & baseCursor_: no setter.
        dkRecord.SetShared(record.GetShared());
        return dkRecord;
    }

    std::vector<TARGET_RECORD_TYPE> CDKRecord(const std::vector<ORIGIN_RECORD_TYPE> &records)
    {
        std::vector<TARGET_RECORD_TYPE> result;
        for (auto &record : records) {
            result.emplace_back(CDKRecord(record));
        }
        return result;
    }

    TARGET_RESULT_TYPE CDKResult(const ORIGIN_RESULT_TYPE &result)
    {
        TARGET_RESULT_TYPE dkResult;
        TARGET_RECORD_TYPE mdkRecord = CDKRecord(result.GetDKRecord());
        dkResult.SetDKRecord(mdkRecord);
        TARGET_ERROR_TYPE mdkError = CDKError(result.GetDKError());
        dkResult.SetDKError(mdkError);
        return dkResult;
    }

    std::map<std::string, TARGET_RESULT_TYPE> CDKResult(const std::map<std::string, ORIGIN_RESULT_TYPE> &results)
    {
        std::map<std::string, TARGET_RESULT_TYPE> dkResults;
        for (auto &resultPair : results) {
            dkResults[resultPair.first] = CDKResult(resultPair.second);
        }
        return dkResults;
    }
};
}  // namespace OHOS::Media::CloudSync
#endif