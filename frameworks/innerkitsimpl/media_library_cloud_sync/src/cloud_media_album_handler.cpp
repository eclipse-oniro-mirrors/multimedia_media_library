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

#include "cloud_media_album_handler.h"

#include <string>

#include "media_log.h"
#include "user_define_ipc_client.h"
#include "media_req_vo.h"
#include "media_resp_vo.h"
#include "cloud_media_operation_code.h"
#include "medialibrary_errno.h"
#include "cloud_mdkrecord_photo_album_vo.h"
#include "on_create_records_album_vo.h"
#include "on_fetch_records_album_vo.h"
#include "on_delete_records_album_vo.h"
#include "on_delete_albums_vo.h"
#include "on_mdirty_records_album_vo.h"
#include "mdk_record_album_data.h"
#include "cloud_album_data_convert.h"
#include "failed_size_resp_vo.h"
#include "media_operate_result_vo.h"

namespace OHOS::Media::CloudSync {
void CloudMediaAlbumHandler::SetUserId(const int32_t &userId)
{
    this->userId_ = userId;
}

void CloudMediaAlbumHandler::SetTraceId(const std::string &traceId)
{
    this->traceId_ = traceId;
}

std::string CloudMediaAlbumHandler::GetTraceId() const
{
    return this->traceId_;
}

void CloudMediaAlbumHandler::SetCloudType(const int32_t cloudType)
{
    this->cloudType_ = cloudType;
}

static void InitAlbumReqData(MDKRecordAlbumData &albumData, OnFetchRecordsAlbumReqBody::AlbumReqData &data)
{
    auto lpathOpt = albumData.GetlPath();
    if (lpathOpt.has_value()) {
        data.localPath = lpathOpt.value();
    }
    auto albumTypeOpt = albumData.GetAlbumType();
    if (albumTypeOpt.has_value()) {
        data.albumType = albumTypeOpt.value();
    } else {
        data.albumType = PhotoAlbumType::INVALID;
    }
    auto albumSubTypeOpt = albumData.GetAlbumSubType();
    if (albumSubTypeOpt.has_value()) {
        data.albumSubType = albumSubTypeOpt.value();
    }
    auto albumDateAddedOpt = albumData.GetDateAdded();
    if (albumDateAddedOpt.has_value()) {
        data.albumDateAdded = albumDateAddedOpt.value();
    }
    auto albumDateModifiedOpt = albumData.GetDateModified();
    if (albumDateModifiedOpt.has_value()) {
        data.albumDateModified = albumDateModifiedOpt.value();
    }
    auto albumNameOpt = albumData.GetAlbumName();
    if (albumNameOpt.has_value()) {
        data.albumName = albumNameOpt.value();
    }
    auto albumBundleNameOpt = albumData.GetBundleName();
    if (albumBundleNameOpt.has_value()) {
        data.albumBundleName = albumBundleNameOpt.value();
    }
    std::optional<std::string> cloudIdOpt = albumData.GetCloudId();
    if (cloudIdOpt.has_value()) {
        data.cloudId = cloudIdOpt.value();
    }
    auto albumsCoverUriSource = albumData.GetCoverUriSource();
    if (albumsCoverUriSource.has_value()) {
        data.coverUriSource = albumsCoverUriSource.value();
    }
    auto albumsCoverCloudId = albumData.GetCoverCloudId();
    if (albumsCoverCloudId.has_value()) {
        data.coverCloudId = albumsCoverCloudId.value();
    }
}

/**
 * stats: 引用入参，按照以下顺序进行返回 [新增，合一，元数据修改，文件修改，删除]:
 * stats: [100, 30, 50, 10, 10]
 */
int32_t CloudMediaAlbumHandler::OnFetchRecords(const std::vector<MDKRecord> &records,
    std::vector<CloudMetaData> &newData, std::vector<CloudMetaData> &fdirtyData,
    std::vector<std::string> &failedRecords, std::vector<int32_t> &stats)
{
    CHECK_AND_RETURN_RET_LOG(!records.empty(), E_OK, "OnFetchRecords param error, records is empty");
    OnFetchRecordsAlbumReqBody req;
    OnFetchRecordsAlbumRespBody resp;
    MEDIA_INFO_LOG("OnFetchRecords %{public}zu records", records.size());
    for (auto record : records) {
        auto cloudId = record.GetRecordId();
        OnFetchRecordsAlbumReqBody::AlbumReqData data;
        data.cloudId = cloudId;
        MDKRecordAlbumData albumData = MDKRecordAlbumData(record);
        InitAlbumReqData(albumData, data);
        data.albumDateCreated = static_cast<int64_t>(record.GetCreateTime());
        data.isDelete = record.GetIsDelete();
        req.albums.emplace_back(data);
        MEDIA_INFO_LOG("OnFetchRecords AlbumReqData:%{public}s", data.ToString().c_str());
    }
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_FETCH_RECORDS);
    int32_t ret =
        IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
            .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}}).Post(operationCode, req, resp);
    stats = resp.stats;
    failedRecords = resp.failedRecords;
    return ret;
}

int32_t CloudMediaAlbumHandler::OnDentryFileInsert(
    std::vector<MDKRecord> &records, std::vector<std::string> &failedRecords)
{
    MEDIA_INFO_LOG("OnDentryFileInsert, records size: %{public}zu", records.size());
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_DENTRY_FILE_INSERT);
    return IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
        .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}}).Post(operationCode);
}

// album does not handle this operation <GetRetryRecords>.
int32_t CloudMediaAlbumHandler::GetRetryRecords(std::vector<std::string> &records)
{
    MEDIA_INFO_LOG("OnDentryFileInsert, records size: %{public}zu", records.size());
    return E_OK;
}

// album does not handle this operation <GetCheckRecords>.
int32_t CloudMediaAlbumHandler::GetCheckRecords(
    const std::vector<std::string> &cloudIds, std::unordered_map<std::string, CloudCheckData> &checkRecords)
{
    MEDIA_INFO_LOG("album does not handle this operation <GetCheckRecords>.");
    return E_OK;
}

int32_t CloudMediaAlbumHandler::GetCreatedRecords(std::vector<MDKRecord> &records, int32_t size)
{
    CloudMdkRecordPhotoAlbumReqBody reqBody;
    reqBody.size = size;
    reqBody.isCloudSpaceFull = this->IsCloudSpaceFull();
    CloudMdkRecordPhotoAlbumRespBody respBody;
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_GET_CREATED_RECORDS);
    int32_t ret = IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
        .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}})
        .Post(operationCode, reqBody, respBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetCreatedRecords fail to call service function");
        return E_ERR;
    }
    std::vector<CloudMdkRecordPhotoAlbumVo> createdRecord = respBody.GetPhotoAlbumRecords();
    CloudAlbumDataConvert dataConvertor{CloudAlbumOperationType::PHOTO_ALBUM_CREATE};
    // Set CloudSpaceFull flag to CloudAlbumDataConvert
    dataConvertor.SetCloudSpaceFull(this->IsCloudSpaceFull());
    std::map<std::string, MDKRecordField> data;
    for (auto it = createdRecord.begin(); it != createdRecord.end(); ++it) {
        std::shared_ptr<MDKRecord> dkRecord = dataConvertor.ConvertToMdkRecord(*it);
        if (dkRecord != nullptr) {
            dkRecord->GetRecordData(data);
            data["albumId"] = MDKRecordField(dkRecord->GetRecordId());
            dkRecord->SetRecordData(data);
            records.push_back(*dkRecord);
        } else {
            MEDIA_ERR_LOG("CloudMediaAlbumHandler::GetCreatedRecords ConvertToMdkRecord Error");
        }
        dkRecord = nullptr;
    }
    MEDIA_INFO_LOG("GetCreatedRecords completed, "
        "query-size: %{public}d, result-size: %{public}zu",
        size,
        records.size());
    return E_OK;
}

int32_t CloudMediaAlbumHandler::GetMetaModifiedRecords(std::vector<MDKRecord> &records, int32_t size, int32_t dirtyType)
{
    CloudMdkRecordPhotoAlbumReqBody reqBody;
    reqBody.size = size;
    CloudMdkRecordPhotoAlbumRespBody respBody;
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_GET_META_MODIFIED_RECORDS);
    int32_t ret = IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
        .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}})
        .Post(operationCode, reqBody, respBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetMetaModifiedRecords fail to call service function");
        return E_ERR;
    }
    std::vector<CloudMdkRecordPhotoAlbumVo> createdRecord = respBody.GetPhotoAlbumRecords();
    CloudAlbumDataConvert dataConvertor{CloudAlbumOperationType::PHOTO_ALBUM_METADATA_MODIF};
    for (auto it = createdRecord.begin(); it != createdRecord.end(); ++it) {
        std::shared_ptr<MDKRecord> dkRecord = dataConvertor.ConvertToMdkRecord(*it);
        if (dkRecord != nullptr) {
            records.push_back(*dkRecord);
            dkRecord = nullptr;
        }
    }
    MEDIA_INFO_LOG("GetMetaModifiedRecords completed, "
        "query-size: %{public}d, result-size: %{public}zu",
        size,
        records.size());
    return E_OK;
}
// album does not handle this operation <GetFileModifiedRecords>.
int32_t CloudMediaAlbumHandler::GetFileModifiedRecords(std::vector<MDKRecord> &records, int32_t size)
{
    return E_OK;
}

int32_t CloudMediaAlbumHandler::GetDeletedRecords(std::vector<MDKRecord> &records, int32_t size)
{
    CloudMdkRecordPhotoAlbumReqBody reqBody;
    reqBody.size = size;
    CloudMdkRecordPhotoAlbumRespBody respBody;
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_GET_DELETED_RECORDS);
    int32_t ret = IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
        .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}})
        .Post(operationCode, reqBody, respBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetDeletedRecords fail to call service function");
        return E_ERR;
    }
    std::vector<CloudMdkRecordPhotoAlbumVo> createdRecord = respBody.GetPhotoAlbumRecords();
    CloudAlbumDataConvert dataConvertor{CloudAlbumOperationType::PHOTO_ALBUM_DELETE};
    for (auto it = createdRecord.begin(); it != createdRecord.end(); ++it) {
        std::shared_ptr<MDKRecord> dkRecord = dataConvertor.ConvertToMdkRecord(*it);
        if (dkRecord != nullptr) {
            records.push_back(*dkRecord);
            dkRecord = nullptr;
        }
    }
    MEDIA_INFO_LOG("GetDeletedRecords completed, "
        "query-size: %{public}d, result-size: %{public}zu",
        size,
        records.size());
    return E_OK;
}
// album does not handle this operation <GetRetryRecords>.
int32_t CloudMediaAlbumHandler::GetCopyRecords(std::vector<MDKRecord> &records, int32_t size)
{
    return E_OK;
}

int32_t CloudMediaAlbumHandler::OnCreateRecords(
    const std::map<std::string, MDKRecordOperResult> &map, int32_t &failSize)
{
    MEDIA_INFO_LOG("enter CloudMediaAlbumHandler::OnCreateRecords %{public}zu", map.size());
    CHECK_AND_RETURN_RET_LOG(!map.empty(), E_OK, "OnCreateRecords Album param error");
    OnCreateRecordsAlbumReqBody reqBody;
    CloudAlbumDataConvert dataConvertor{CloudAlbumOperationType::PHOTO_ALBUM_CREATE};
    for (auto &entry : map) {
        if (entry.first.empty()) {
            MEDIA_INFO_LOG("OnCreateRecords is failed");
            continue;
        }
        OnCreateRecordsAlbumReqBodyAlbumData record;
        if (dataConvertor.ConvertToOnCreateRecord(entry.first, entry.second, record) != E_OK) {
            MEDIA_ERR_LOG("OnCreateRecords Album ConvertToOnCreateRecord error");
            continue;
        }
        MEDIA_INFO_LOG("OnCreateRecords Album:%{public}s", record.ToString().c_str());
        reqBody.albums.emplace_back(record);
    }
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_CREATE_RECORDS);
    FailedSizeResp resp;
    resp.failedSize = 0;
    int32_t ret = IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
        .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}})
        .Post(operationCode, reqBody, resp);
    failSize = resp.failedSize;
    MEDIA_INFO_LOG("OnCreateRecords Album Resp:%{public}s Ret:%{public}d", resp.ToString().c_str(), ret);
    return ret;
}

int32_t CloudMediaAlbumHandler::OnMdirtyRecords(
    const std::map<std::string, MDKRecordOperResult> &map, int32_t &failSize)
{
    MEDIA_INFO_LOG("enter CloudMediaAlbumHandler::OnMdirtyRecords %{public}zu", map.size());
    CHECK_AND_RETURN_RET_LOG(!map.empty(), E_OK, "OnMdirtyRecords Album param error");
    OnMdirtyRecordsAlbumReqBody reqBody;
    CloudAlbumDataConvert dataConvertor{CloudAlbumOperationType::PHOTO_ALBUM_METADATA_MODIF};
    for (auto &entry : map) {
        OnMdirtyAlbumRecord record;
        if (dataConvertor.BuildModifyRecord(entry.first, entry.second, record) != E_OK) {
            MEDIA_ERR_LOG("OnMdirtyRecords Album BuildModifyRecord error");
            continue;
        }
        MEDIA_INFO_LOG("OnMdirtyRecords Album Record:%{public}s", record.ToString().c_str());
        reqBody.AddMdirtyRecord(record);
    }
    OnMdirtyRecordsAlbumRespBody respBody;
    respBody.failSize = 0;
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_MDIRTY_RECORDS);
    int32_t ret = IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
        .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}})
        .Post(operationCode, reqBody, respBody);
    failSize = respBody.failSize;
    return ret;
}

int32_t CloudMediaAlbumHandler::OnFdirtyRecords(
    const std::map<std::string, MDKRecordOperResult> &map, int32_t &failSize)
{
    MEDIA_INFO_LOG("CloudMediaAlbumHandler::OnFdirtyRecords, size: %{public}zu", map.size());
    CHECK_AND_RETURN_RET_LOG(!map.empty(), E_OK, "OnFdirtyRecords Album param error");
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_FDIRTY_RECORDS);
    return IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
        .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}})
        .Post(operationCode);
}

int32_t CloudMediaAlbumHandler::OnDeleteRecords(
    const std::map<std::string, MDKRecordOperResult> &map, int32_t &failSize)
{
    MEDIA_INFO_LOG("enter CloudMediaAlbumHandler::OnDeleteRecords, size: %{public}zu", map.size());
    CHECK_AND_RETURN_RET_LOG(!map.empty(), E_OK, "OnDeleteRecords Album param error");
    OnDeleteRecordsAlbumReqBody reqBody;
    OnDeleteRecordsAlbumRespBody respBody;
    respBody.failSize = 0;
    for (auto &entry : map) {
        const MDKRecordOperResult &result = entry.second;
        OnDeleteAlbumData album;
        album.cloudId = entry.first;
        album.isSuccess = result.IsSuccess();
        reqBody.AddSuccessResult(album);
        MEDIA_INFO_LOG("OnDeleteRecords, IsSuccess: %{public}d, ErrorCode: %{public}d, cloudId: %{public}s",
            result.IsSuccess(),
            result.GetDKError().serverErrorCode,
            entry.first.c_str());
    }
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_DELETE_RECORDS);
    int32_t ret = IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
        .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}})
        .Post(operationCode, reqBody, respBody);
    failSize = respBody.failSize;
    return ret;
}

int32_t CloudMediaAlbumHandler::OnCopyRecords(const std::map<std::string, MDKRecordOperResult> &map, int32_t &failSize)
{
    MEDIA_INFO_LOG("OnCopyRecords, map size: %{public}zu", map.size());
    CHECK_AND_RETURN_RET_LOG(!map.empty(), E_OK, "OnCopyRecords Album param error");
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_COPY_RECORDS);
    return IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
        .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}})
        .Post(operationCode);
}

int32_t CloudMediaAlbumHandler::OnStartSync()
{
    MEDIA_INFO_LOG("OnStartSync enter");
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_START_SYNC);
    return IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
        .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}})
        .Post(operationCode);
}

int32_t CloudMediaAlbumHandler::OnCompleteSync()
{
    MEDIA_DEBUG_LOG("OnCompleteSync enter");
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_COMPLETE_SYNC);
    return IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
        .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}})
        .Post(operationCode);
}

int32_t CloudMediaAlbumHandler::OnCompletePull(const MediaOperateResult &optRet)
{
    MEDIA_INFO_LOG("OnCompletePull enter");
    MediaOperateResultRespBodyResultNode reqBody;
    reqBody.cloudId = optRet.cloudId;
    reqBody.errorCode = optRet.errorCode;
    reqBody.errorMsg = optRet.errorMsg;
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_COMPLETE_PULL);
    return IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
        .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}})
        .Post(operationCode, reqBody);
}

int32_t CloudMediaAlbumHandler::OnCompletePush()
{
    MEDIA_INFO_LOG("OnCompletePush enter");
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_COMPLETE_PUSH);
    return IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
        .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}})
        .Post(operationCode);
}

int32_t CloudMediaAlbumHandler::OnCompleteCheck()
{
    MEDIA_INFO_LOG("CloudMediaDataClient::OnCompleteCheck begin");
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_COMPLETE_CHECK);
    return IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
        .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}})
        .Post(operationCode);
}

void CloudMediaAlbumHandler::SetCloudSpaceFull(bool isCloudSpaceFull)
{
    this->isCloudSpaceFull_ = isCloudSpaceFull;
}

bool CloudMediaAlbumHandler::IsCloudSpaceFull()
{
    return this->isCloudSpaceFull_;
}
}  // namespace OHOS::Media::CloudSync