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
#ifndef OHOS_MEDIA_CLOUD_SYNC_DK_RECORD_PARSER_FACTORY_H
#define OHOS_MEDIA_CLOUD_SYNC_DK_RECORD_PARSER_FACTORY_H

#include "dk_record_parser.h"

#define EXPORT __attribute__ ((visibility ("default")))

namespace OHOS::Media::CloudSync {
class EXPORT CloudMediaFactory {
private:
    static DKRecordParser<DKRecord, MDKRecord, DKRecordField, MDKRecordField, DKAsset, MDKAsset, DKReference,
        MDKReference, DKRelation, MDKRelation, DKRecordsResponse, MDKRecordsResponse, DKSchemaField, MDKSchemaField,
        DKSchemaRelation, MDKSchemaRelation, DKSchemaNode, MDKSchemaNode, DKOrderTable, MDKOrderTable, DKErrorDetail,
        MDKErrorDetail, DKError, MDKError, DKRecordOperResult, MDKRecordOperResult, Participant, MDKParticipant,
        DKScadetail, MDKScadetail>
        DK_PARSER;
    static DKRecordParser<MDKRecord, DKRecord, MDKRecordField, DKRecordField, MDKAsset, DKAsset, MDKReference,
        DKReference, MDKRelation, DKRelation, MDKRecordsResponse, DKRecordsResponse, MDKSchemaField, DKSchemaField,
        MDKSchemaRelation, DKSchemaRelation, MDKSchemaNode, DKSchemaNode, MDKOrderTable, DKOrderTable, MDKErrorDetail,
        DKErrorDetail, MDKError, DKError, MDKRecordOperResult, DKRecordOperResult, MDKParticipant, Participant,
        MDKScadetail, DKScadetail>
        MDK_PARSER;

public:
    static DKRecordParser<DKRecord, MDKRecord, DKRecordField, MDKRecordField, DKAsset, MDKAsset, DKReference,
        MDKReference, DKRelation, MDKRelation, DKRecordsResponse, MDKRecordsResponse, DKSchemaField, MDKSchemaField,
        DKSchemaRelation, MDKSchemaRelation, DKSchemaNode, MDKSchemaNode, DKOrderTable, MDKOrderTable, DKErrorDetail,
        MDKErrorDetail, DKError, MDKError, DKRecordOperResult, MDKRecordOperResult, Participant, MDKParticipant,
        DKScadetail, MDKScadetail>
        GetDkParser()
    {
        return DK_PARSER;
    }

    static DKRecordParser<MDKRecord, DKRecord, MDKRecordField, DKRecordField, MDKAsset, DKAsset, MDKReference,
        DKReference, MDKRelation, DKRelation, MDKRecordsResponse, DKRecordsResponse, MDKSchemaField, DKSchemaField,
        MDKSchemaRelation, DKSchemaRelation, MDKSchemaNode, DKSchemaNode, MDKOrderTable, DKOrderTable, MDKErrorDetail,
        DKErrorDetail, MDKError, DKError, MDKRecordOperResult, DKRecordOperResult, MDKParticipant, Participant,
        MDKScadetail, DKScadetail>
        GetMdkParser()
    {
        return MDK_PARSER;
    }
};
}  // namespace OHOS::Media::CloudSync
#endif