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

#include "cloud_media_data_client.h"

#include <string>
#include <vector>

#include "medialibrary_errno.h"
#include "media_log.h"
#include "cloud_media_data_client_handler.h"
#include "cloud_media_thread_limiter.h"

namespace OHOS::Media::CloudSync {
CloudMediaDataClient::CloudMediaDataClient(const int32_t cloudType, const int32_t userId)
    : cloudType_(cloudType), userId_(userId)
{
    this->dataHandler_ = std::make_shared<CloudMediaDataClientHandler>();
    this->dataHandler_->SetCloudType(cloudType);
    this->dataHandler_->SetUserId(userId);
}

void CloudMediaDataClient::SetUserId(const int32_t &userId)
{
    this->userId_ = userId;
}

void CloudMediaDataClient::SetTraceId(const std::string &traceId)
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found!");
        return;
    }
    this->dataHandler_->SetTraceId(traceId);
}

std::string CloudMediaDataClient::GetTraceId() const
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found!");
        return "";
    }
    return this->dataHandler_->GetTraceId();
}

void CloudMediaDataClient::SetCloudType(const int32_t cloudType)
{
    this->cloudType_ = cloudType;
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found!");
        return;
    }
    this->dataHandler_->SetCloudType(cloudType);
}

int32_t CloudMediaDataClient::UpdateDirty(const std::string &cloudId, DirtyTypes dirtyType)
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found!");
        return E_IPC_INVAL_ARG;
    }
    CLOUD_DATA_CLIENT_WRITE_LOCK;
    return this->dataHandler_->UpdateDirty(cloudId, dirtyType);
}

int32_t CloudMediaDataClient::UpdatePosition(const std::vector<std::string> &cloudIds, int32_t position)
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found!");
        return E_IPC_INVAL_ARG;
    }
    CLOUD_DATA_CLIENT_WRITE_LOCK;
    return this->dataHandler_->UpdatePosition(cloudIds, position);
}

int32_t CloudMediaDataClient::UpdatePosWithType(const std::vector<std::string> &cloudIds,
    int32_t position, int32_t fileSourceType)
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found!");
        return E_IPC_INVAL_ARG;
    }
    CLOUD_DATA_CLIENT_WRITE_LOCK;
    return this->dataHandler_->UpdatePosWithType(cloudIds, position, fileSourceType);
}

int32_t CloudMediaDataClient::UpdateFileSourceType(
    const std::vector<std::string> &cloudIds, int32_t fileSourceType)
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found!");
        return E_IPC_INVAL_ARG;
    }
    CLOUD_DATA_CLIENT_WRITE_LOCK;
    return this->dataHandler_->UpdateFileSourceType(cloudIds, fileSourceType);
}

int32_t CloudMediaDataClient::UpdateSyncStatus(const std::string &cloudId, int32_t syncStatus)
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found!");
        return E_IPC_INVAL_ARG;
    }
    CLOUD_DATA_CLIENT_WRITE_LOCK;
    return this->dataHandler_->UpdateSyncStatus(cloudId, syncStatus);
}

int32_t CloudMediaDataClient::UpdateThmStatus(const std::string &cloudId, int32_t thmStatus)
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found!");
        return E_IPC_INVAL_ARG;
    }
    CLOUD_DATA_CLIENT_WRITE_LOCK;
    return this->dataHandler_->UpdateThmStatus(cloudId, thmStatus);
}

int32_t CloudMediaDataClient::GetAgingFile(
    const int64_t time, int32_t mediaType, int32_t sizeLimit, int32_t offset, std::vector<CloudMetaData> &metaData)
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found!");
        return E_IPC_INVAL_ARG;
    }
    return this->dataHandler_->GetAgingFile(time, mediaType, sizeLimit, offset, metaData);
}

int32_t CloudMediaDataClient::GetActiveAgingFile(
    const int64_t time, int32_t mediaType, int32_t sizeLimit, int32_t offset, std::vector<CloudMetaData> &metaData)
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found!");
        return E_IPC_INVAL_ARG;
    }
    return this->dataHandler_->GetActiveAgingFile(time, mediaType, sizeLimit, offset, metaData);
}

int32_t CloudMediaDataClient::GetDownloadAsset(
    const std::vector<std::string> &uris, std::vector<CloudMetaData> &cloudMetaDataVec)
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found!");
        return E_IPC_INVAL_ARG;
    }
    return this->dataHandler_->GetDownloadAsset(uris, cloudMetaDataVec);
}

int32_t CloudMediaDataClient::GetDownloadLakeAsset(
    const std::vector<std::string> &uris, std::vector<CloudDlFileMeta> &cloudMetaDataVec)
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found!");
        return E_IPC_INVAL_ARG;
    }
    return this->dataHandler_->GetDownloadLakeAsset(uris, cloudMetaDataVec);
}

int32_t CloudMediaDataClient::GetDownloadThmsByUri(
    const std::vector<std::string> &uri, int32_t type, std::vector<CloudMetaData> &metaData)
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found!");
        return E_IPC_INVAL_ARG;
    }
    return this->dataHandler_->GetDownloadThmsByUri(uri, type, metaData);
}

int32_t CloudMediaDataClient::OnDownloadAsset(
    const std::vector<std::string> &cloudIds, std::vector<MediaOperateResult> &result)
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found!");
        return E_IPC_INVAL_ARG;
    }
    CLOUD_DATA_CLIENT_WRITE_LOCK;
    return this->dataHandler_->OnDownloadAsset(cloudIds, result);
}

int32_t CloudMediaDataClient::OnDownloadLakeAsset(
    const std::unordered_map<std::string, AdditionFileInfo> &lakeInfos, std::vector<MediaOperateResult> &result)
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found!");
        return E_IPC_INVAL_ARG;
    }
    CLOUD_DATA_CLIENT_WRITE_LOCK;
    return this->dataHandler_->OnDownloadLakeAsset(lakeInfos, result);
}

int32_t CloudMediaDataClient::GetDownloadThms(
    std::vector<CloudMetaData> &cloudMetaDataVec, const DownloadThumPara &param)
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found!");
        return E_IPC_INVAL_ARG;
    }
    return this->dataHandler_->GetDownloadThms(cloudMetaDataVec, param);
}

int32_t CloudMediaDataClient::OnDownloadThms(const std::unordered_map<std::string, int32_t> &resMap, int32_t &failSize)
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found!");
        return E_IPC_INVAL_ARG;
    }
    CLOUD_DATA_CLIENT_WRITE_LOCK;
    return this->dataHandler_->OnDownloadThms(resMap, failSize);
}

int32_t CloudMediaDataClient::GetVideoToCache(std::vector<CloudMetaData> &cloudMetaDataVec, int32_t size)
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found!");
        return E_IPC_INVAL_ARG;
    }
    return this->dataHandler_->GetVideoToCache(cloudMetaDataVec, size);
}

int32_t CloudMediaDataClient::GetFilePosStat(std::vector<uint64_t> &filePosStat)
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found!");
        return E_IPC_INVAL_ARG;
    }
    return this->dataHandler_->GetFilePosStat(filePosStat);
}

int32_t CloudMediaDataClient::GetCloudThmStat(std::vector<uint64_t> &cloudThmStat)
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found!");
        return E_IPC_INVAL_ARG;
    }
    return this->dataHandler_->GetCloudThmStat(cloudThmStat);
}

int32_t CloudMediaDataClient::GetDirtyTypeStat(std::vector<uint64_t> &dirtyTypeStat)
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found!");
        return E_IPC_INVAL_ARG;
    }
    return this->dataHandler_->GetDirtyTypeStat(dirtyTypeStat);
}

int32_t CloudMediaDataClient::GetDownloadThmNum(int32_t &totalNum, int32_t type)
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found!");
        return E_IPC_INVAL_ARG;
    }
    return this->dataHandler_->GetDownloadThmNum(totalNum, type);
}

int32_t CloudMediaDataClient::UpdateLocalFileDirty(std::vector<MDKRecord> &records)
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found!");
        return E_IPC_INVAL_ARG;
    }
    CLOUD_DATA_CLIENT_WRITE_LOCK;
    return this->dataHandler_->UpdateLocalFileDirty(records);
}

int32_t CloudMediaDataClient::GetCloudSyncUnPreparedData(int32_t &result)
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found!");
        return E_IPC_INVAL_ARG;
    }
    return this->dataHandler_->GetCloudSyncUnPreparedData(result);
}

int32_t CloudMediaDataClient::SubmitCloudSyncPreparedDataTask()
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found!");
        return E_IPC_INVAL_ARG;
    }
    return this->dataHandler_->SubmitCloudSyncPreparedDataTask();
}

int32_t CloudMediaDataClient::CheckAndFixAlbum()
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found!");
        return E_IPC_INVAL_ARG;
    }
    return this->dataHandler_->CheckAndFixAlbum();
}

int32_t CloudMediaDataClient::QueryData(const DataShare::DataSharePredicates &predicates,
                                        const std::vector<std::string> &columnNames,
                                        const std::string &tableName,
                                        std::vector<std::unordered_map<std::string, std::string>> &results)
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found!");
        return E_IPC_INVAL_ARG;
    }
    return this->dataHandler_->QueryData(predicates, columnNames, tableName, results);
}

int32_t CloudMediaDataClient::UpdateData(const std::string &tableName, const DataShare::DataSharePredicates &predicates,
    const DataShare::DataShareValuesBucket &value, const std::string &operateName)
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found!");
        return E_IPC_INVAL_ARG;
    }
    return this->dataHandler_->UpdateData(tableName, predicates, value, operateName);
}
}  // namespace OHOS::Media::CloudSync