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
#define MLOG_TAG "Media_Client"

#include "cloud_media_data_handler.h"

#include <string>

#include "media_log.h"
#include "user_define_ipc_client.h"
#include "userfile_client.h"
#include "media_req_vo.h"
#include "media_resp_vo.h"
#include "media_empty_obj_vo.h"
#include "cloud_media_operation_code.h"
#include "medialibrary_errno.h"
#include "cloud_media_data_handler_factory.h"
#include "cloud_media_thread_limiter.h"

namespace OHOS::Media::CloudSync {
CloudMediaDataHandler::CloudMediaDataHandler(const std::string &tableName, int32_t cloudType, int32_t userId)
    : cloudType_(cloudType), userId_(userId), tableName_(tableName)
{
    this->dataHandler_ = CloudMediaDataHandlerFactory().GetDataHandler(tableName, cloudType, userId);
    MEDIA_INFO_LOG("media-ipc userId: %{public}d", userId);
}

int32_t CloudMediaDataHandler::GetCloudType() const
{
    return this->cloudType_;
}

void CloudMediaDataHandler::SetUserId(const int32_t &userId)
{
    this->userId_ = userId;
}

int32_t CloudMediaDataHandler::GetUserId() const
{
    return this->userId_;
}

void CloudMediaDataHandler::SetCloudType(const int32_t cloudType)
{
    this->cloudType_ = cloudType;
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found! tableName: %{public}s", this->tableName_.c_str());
        return;
    }
    this->dataHandler_->SetCloudType(cloudType);
}

std::string CloudMediaDataHandler::GetTableName() const
{
    return this->tableName_;
}

void CloudMediaDataHandler::SetTableName(const std::string &tableName)
{
    this->tableName_ = tableName;
    this->dataHandler_ = CloudMediaDataHandlerFactory().GetDataHandler(tableName, cloudType_, userId_);
}

void CloudMediaDataHandler::SetTraceId(const std::string &traceId)
{
    this->traceId_ = traceId;
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found! tableName: %{public}s", this->tableName_.c_str());
        return;
    }
    this->dataHandler_->SetTraceId(traceId);
}

std::string CloudMediaDataHandler::GetTraceId() const
{
    return this->traceId_;
}

int32_t CloudMediaDataHandler::GetCheckRecords(
    const std::vector<std::string> &cloudIds, std::unordered_map<std::string, CloudCheckData> &checkRecords)
{
    MEDIA_INFO_LOG("OnCompletePush enter, cloudIds: %{public}zu", cloudIds.size());
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found! tableName: %{public}s", this->tableName_.c_str());
        return E_IPC_INVAL_ARG;
    }
    return this->dataHandler_->GetCheckRecords(cloudIds, checkRecords);
}

int32_t CloudMediaDataHandler::GetCreatedRecords(std::vector<MDKRecord> &records, int32_t size)
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found! tableName: %{public}s", this->tableName_.c_str());
        return E_IPC_INVAL_ARG;
    }
    return this->dataHandler_->GetCreatedRecords(records, size);
}

int32_t CloudMediaDataHandler::GetMetaModifiedRecords(std::vector<MDKRecord> &records, int32_t size, int32_t dirtyType)
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found! tableName: %{public}s", this->tableName_.c_str());
        return E_IPC_INVAL_ARG;
    }
    return this->dataHandler_->GetMetaModifiedRecords(records, size, dirtyType);
}

int32_t CloudMediaDataHandler::GetFileModifiedRecords(std::vector<MDKRecord> &records, int32_t size)
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found! tableName: %{public}s", this->tableName_.c_str());
        return E_IPC_INVAL_ARG;
    }
    return this->dataHandler_->GetFileModifiedRecords(records, size);
}

int32_t CloudMediaDataHandler::GetDeletedRecords(std::vector<MDKRecord> &records, int32_t size)
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found! tableName: %{public}s", this->tableName_.c_str());
        return E_IPC_INVAL_ARG;
    }
    return this->dataHandler_->GetDeletedRecords(records, size);
}

int32_t CloudMediaDataHandler::GetCopyRecords(std::vector<MDKRecord> &records, int32_t size)
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found! tableName: %{public}s", this->tableName_.c_str());
        return E_IPC_INVAL_ARG;
    }
    return this->dataHandler_->GetCopyRecords(records, size);
}

int32_t CloudMediaDataHandler::OnCreateRecords(const std::map<std::string, MDKRecordOperResult> &map, int32_t &failSize)
{
    MEDIA_INFO_LOG("OnCreateRecords enter, map: %{public}zu", map.size());
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found! tableName: %{public}s", this->tableName_.c_str());
        return E_IPC_INVAL_ARG;
    }
    CLOUD_SYNC_HANDLER_WRITE_LOCK;
    return this->dataHandler_->OnCreateRecords(map, failSize);
}

int32_t CloudMediaDataHandler::OnMdirtyRecords(const std::map<std::string, MDKRecordOperResult> &map, int32_t &failSize)
{
    MEDIA_INFO_LOG("OnMdirtyRecords enter, map: %{public}zu", map.size());
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found! tableName: %{public}s", this->tableName_.c_str());
        return E_IPC_INVAL_ARG;
    }
    CLOUD_SYNC_HANDLER_WRITE_LOCK;
    return this->dataHandler_->OnMdirtyRecords(map, failSize);
}

int32_t CloudMediaDataHandler::OnFdirtyRecords(const std::map<std::string, MDKRecordOperResult> &map, int32_t &failSize)
{
    MEDIA_INFO_LOG("OnFdirtyRecords enter, map: %{public}zu", map.size());
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found! tableName: %{public}s", this->tableName_.c_str());
        return E_IPC_INVAL_ARG;
    }
    CLOUD_SYNC_HANDLER_WRITE_LOCK;
    return this->dataHandler_->OnFdirtyRecords(map, failSize);
}

int32_t CloudMediaDataHandler::OnDeleteRecords(const std::map<std::string, MDKRecordOperResult> &map, int32_t &failSize)
{
    MEDIA_INFO_LOG("OnDeleteRecords enter, map: %{public}zu", map.size());
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found! tableName: %{public}s", this->tableName_.c_str());
        return E_IPC_INVAL_ARG;
    }
    CLOUD_SYNC_HANDLER_WRITE_LOCK;
    return this->dataHandler_->OnDeleteRecords(map, failSize);
}

int32_t CloudMediaDataHandler::OnCopyRecords(const std::map<std::string, MDKRecordOperResult> &map, int32_t &failSize)
{
    MEDIA_INFO_LOG("OnCopyRecords enter, map: %{public}zu", map.size());
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found! tableName: %{public}s", this->tableName_.c_str());
        return E_IPC_INVAL_ARG;
    }
    CLOUD_SYNC_HANDLER_WRITE_LOCK;
    return this->dataHandler_->OnCopyRecords(map, failSize);
}

int32_t CloudMediaDataHandler::OnFetchRecords(const std::vector<MDKRecord> &records,
    std::vector<CloudMetaData> &newData, std::vector<CloudMetaData> &fdirtyData,
    std::vector<std::string> &failedRecords, std::vector<int32_t> &stats)
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found! tableName: %{public}s", this->tableName_.c_str());
        return E_IPC_INVAL_ARG;
    }
    CLOUD_SYNC_HANDLER_WRITE_LOCK;
    return this->dataHandler_->OnFetchRecords(records, newData, fdirtyData, failedRecords, stats);
}

int32_t CloudMediaDataHandler::OnDentryFileInsert(
    std::vector<MDKRecord> &records, std::vector<std::string> &failedRecords)
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found! tableName: %{public}s", this->tableName_.c_str());
        return E_IPC_INVAL_ARG;
    }
    CLOUD_SYNC_HANDLER_WRITE_LOCK;
    return this->dataHandler_->OnDentryFileInsert(records, failedRecords);
}

int32_t CloudMediaDataHandler::GetRetryRecords(std::vector<std::string> &records)
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found! tableName: %{public}s", this->tableName_.c_str());
        return E_IPC_INVAL_ARG;
    }
    return this->dataHandler_->GetRetryRecords(records);
}

int32_t CloudMediaDataHandler::OnStartSync()
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found! tableName: %{public}s", this->tableName_.c_str());
        return E_IPC_INVAL_ARG;
    }
    CLOUD_SYNC_HANDLER_WRITE_LOCK;
    return this->dataHandler_->OnStartSync();
}

int32_t CloudMediaDataHandler::OnCompleteSync()
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found! tableName: %{public}s", this->tableName_.c_str());
        return E_IPC_INVAL_ARG;
    }
    CLOUD_SYNC_HANDLER_WRITE_LOCK;
    return this->dataHandler_->OnCompleteSync();
}

int32_t CloudMediaDataHandler::OnCompletePull(const MediaOperateResult &optRet)
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found! tableName: %{public}s", this->tableName_.c_str());
        return E_IPC_INVAL_ARG;
    }
    CLOUD_SYNC_HANDLER_WRITE_LOCK;
    return this->dataHandler_->OnCompletePull(optRet);
}

int32_t CloudMediaDataHandler::OnCompletePush()
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found! tableName: %{public}s", this->tableName_.c_str());
        return E_IPC_INVAL_ARG;
    }
    CLOUD_SYNC_HANDLER_WRITE_LOCK;
    return this->dataHandler_->OnCompletePush();
}

int32_t CloudMediaDataHandler::OnCompleteCheck()
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found! tableName: %{public}s", this->tableName_.c_str());
        return E_IPC_INVAL_ARG;
    }
    CLOUD_SYNC_HANDLER_WRITE_LOCK;
    return this->dataHandler_->OnCompleteCheck();
}

void CloudMediaDataHandler::SetCloudSpaceFull(bool isCloudSpaceFull)
{
    CHECK_AND_RETURN_LOG(
        this->dataHandler_ != nullptr, "No data handler found! tableName: %{public}s", this->tableName_.c_str());
    this->dataHandler_->SetCloudSpaceFull(isCloudSpaceFull);
}

bool CloudMediaDataHandler::IsCloudSpaceFull()
{
    CHECK_AND_RETURN_RET_LOG(
        this->dataHandler_ != nullptr, false, "No data handler found! tableName: %{public}s", this->tableName_.c_str());
    return this->dataHandler_->IsCloudSpaceFull();
}
}  // namespace OHOS::Media::CloudSync