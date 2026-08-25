/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_CLOUD_SYNC_I_CLOUD_MEDIA_DATA_HANDLER_H
#define OHOS_MEDIA_CLOUD_SYNC_I_CLOUD_MEDIA_DATA_HANDLER_H

#include <map>
#include <vector>

#include "cloud_check_data.h"
#include "cloud_meta_data.h"
#include "mdk_record.h"
#include "mdk_reference.h"
#include "mdk_database.h"
#include "media_operate_result.h"

namespace OHOS::Media::CloudSync {
#define EXPORT __attribute__ ((visibility ("default")))
class EXPORT ICloudMediaDataHandler {
public:  // getter & setter
    virtual void SetTraceId(const std::string &traceId) = 0;
    virtual std::string GetTraceId() const = 0;
    virtual void SetUserId(const int32_t &userId) = 0;
    virtual void SetCloudType(const int32_t cloudType) = 0;
    virtual void SetCloudSpaceFull(bool isCloudSpaceFull) = 0;
    virtual bool IsCloudSpaceFull() = 0;
    virtual void SetSceneType(int32_t sceneType) = 0;

public:
    virtual int32_t GetCheckRecords(const std::vector<std::string> &cloudIds,
        std::unordered_map<std::string, CloudCheckData> &checkRecords) = 0;
    virtual int32_t GetCreatedRecords(std::vector<MDKRecord> &records, int32_t size) = 0;
    virtual int32_t GetMetaModifiedRecords(std::vector<MDKRecord> &records, int32_t size) = 0;
    virtual int32_t GetFileModifiedRecords(std::vector<MDKRecord> &records, int32_t size) = 0;
    virtual int32_t GetDeletedRecords(std::vector<MDKRecord> &records, int32_t size) = 0;
    virtual int32_t GetCopyRecords(std::vector<MDKRecord> &records, int32_t size) = 0;
    virtual int32_t OnCreateRecords(const std::map<std::string, MDKRecordOperResult> &map, int32_t &failSize) = 0;
    virtual int32_t OnMdirtyRecords(const std::map<std::string, MDKRecordOperResult> &map, int32_t &failSize) = 0;
    virtual int32_t OnFdirtyRecords(const std::map<std::string, MDKRecordOperResult> &map, int32_t &failSize) = 0;
    virtual int32_t OnDeleteRecords(const std::map<std::string, MDKRecordOperResult> &map, int32_t &failSize) = 0;
    virtual int32_t OnCopyRecords(const std::map<std::string, MDKRecordOperResult> &map, int32_t &failSize) = 0;
    virtual int32_t OnFetchRecords(const std::vector<MDKRecord> &records, std::vector<CloudMetaData> &newData,
        std::vector<CloudMetaData> &fdirtyData, std::vector<std::string> &failedRecords,
        std::vector<int32_t> &stats) = 0;
    virtual int32_t OnDentryFileInsert(std::vector<MDKRecord> &records, std::vector<std::string> &failedRecords) = 0;
    virtual int32_t GetRetryRecords(std::vector<std::string> &records) = 0;
    virtual int32_t OnStartSync() = 0;
    virtual int32_t OnCompleteSync() = 0;
    virtual int32_t OnCompletePull(const MediaOperateResult &optRet) = 0;
    virtual int32_t OnCompletePush() = 0;
    virtual int32_t OnCompleteCheck() = 0;
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_I_CLOUD_MEDIA_DATA_HANDLER_H