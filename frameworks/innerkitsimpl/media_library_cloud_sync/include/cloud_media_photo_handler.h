/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_PHOTO_HANDLER_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_PHOTO_HANDLER_H

#include <string>
#include <vector>

#include "i_cloud_media_data_handler.h"
#include "cloud_check_data.h"
#include "cloud_meta_data.h"
#include "mdk_record.h"
#include "mdk_reference.h"
#include "mdk_database.h"
#include "cloud_media_data_client_handler_processor.h"
#include "report_failure_vo.h"
#include "cloud_media_define.h"
#include "on_dentry_file_vo.h"
#include "on_fetch_records_vo.h"

namespace OHOS::Media::CloudSync {
class EXPORT CloudMediaPhotoHandler : public ICloudMediaDataHandler {
public:
    CloudMediaPhotoHandler() = default;
    virtual ~CloudMediaPhotoHandler() = default;

public:  // getter & setter
    void SetTraceId(const std::string &traceId) override;
    std::string GetTraceId() const override;
    void SetUserId(const int32_t &userId) override;
    void SetCloudType(const int32_t cloudType) override;
    void SetCloudSpaceFull(bool isCloudSpaceFull) override;
    bool IsCloudSpaceFull() override;

public:
    int32_t GetCheckRecords(const std::vector<std::string> &cloudIds,
        std::unordered_map<std::string, CloudCheckData> &checkRecords) override;
    int32_t GetCreatedRecords(std::vector<MDKRecord> &records, int32_t size) override;
    int32_t GetMetaModifiedRecords(std::vector<MDKRecord> &records, int32_t size, int32_t dirtyType = 2) override;
    int32_t GetFileModifiedRecords(std::vector<MDKRecord> &records, int32_t size) override;
    int32_t GetDeletedRecords(std::vector<MDKRecord> &records, int32_t size) override;
    int32_t GetCopyRecords(std::vector<MDKRecord> &records, int32_t size) override;
    int32_t OnCreateRecords(const std::map<std::string, MDKRecordOperResult> &map, int32_t &failSize) override;
    int32_t OnMdirtyRecords(const std::map<std::string, MDKRecordOperResult> &map, int32_t &failSize) override;
    int32_t OnFdirtyRecords(const std::map<std::string, MDKRecordOperResult> &map, int32_t &failSize) override;
    int32_t OnDeleteRecords(const std::map<std::string, MDKRecordOperResult> &map, int32_t &failSize) override;
    int32_t OnCopyRecords(const std::map<std::string, MDKRecordOperResult> &map, int32_t &failSize) override;
    int32_t OnFetchRecords(const std::vector<MDKRecord> &records, std::vector<CloudMetaData> &newData,
        std::vector<CloudMetaData> &fdirtyData, std::vector<std::string> &failedRecords,
        std::vector<int32_t> &stats) override;
    int32_t OnDentryFileInsert(std::vector<MDKRecord> &records, std::vector<std::string> &failedRecords) override;
    int32_t GetRetryRecords(std::vector<std::string> &records) override;
    int32_t OnStartSync() override;
    int32_t OnCompleteSync() override;
    int32_t OnCompletePull(const MediaOperateResult &optRet) override;
    int32_t OnCompletePush() override;
    int32_t OnCompleteCheck() override;

private:
    int32_t ReportFailure(const ReportFailureReqBody &reqBody);
    void DeleteTempLivePhotoFile(std::string &livePhotoCachePath);
    int32_t OnFetchRecordsInner(const OnFetchRecordsReqBody &reqBody, OnFetchRecordsRespBody &respBody);
    int32_t OnDentryFileInsertInner(const OnDentryFileReqBody &reqBody, OnDentryFileRespBody &respBody);
private:
    CloudMediaDataClientHandlerProcessor processor_;
    std::string traceId_;
    int32_t userId_;
    int32_t cloudType_;
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_PHOTO_HANDLER_H
