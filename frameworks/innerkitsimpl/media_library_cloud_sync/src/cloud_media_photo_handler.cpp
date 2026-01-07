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

#include "cloud_media_photo_handler.h"

#include <string>

#include "cloud_file_data_convert.h"
#include "cloud_media_operation_code.h"
#include "cloud_mdkrecord_photos_vo.h"
#include "get_retey_records_vo.h"
#include "medialibrary_errno.h"
#include "media_log.h"
#include "media_req_vo.h"
#include "media_resp_vo.h"
#include "mdk_record_photos_data.h"
#include "on_copy_records_photos_vo.h"
#include "on_create_records_photos_vo.h"
#include "on_delete_records_photos_vo.h"
#include "on_modify_records_photos_vo.h"
#include "on_dentry_file_vo.h"
#include "on_fetch_records_vo.h"
#include "on_fetch_photos_vo.h"
#include "user_define_ipc_client.h"
#include "failed_size_resp_vo.h"
#include "get_check_records_vo.h"
#include "media_operate_result_vo.h"
#include "cloud_media_sync_utils.h"

namespace OHOS::Media::CloudSync {
void CloudMediaPhotoHandler::SetUserId(const int32_t &userId)
{
    this->userId_ = userId;
}

void CloudMediaPhotoHandler::SetTraceId(const std::string &traceId)
{
    this->traceId_ = traceId;
}

std::string CloudMediaPhotoHandler::GetTraceId() const
{
    return this->traceId_;
}

void CloudMediaPhotoHandler::SetCloudType(const int32_t cloudType)
{
    this->cloudType_ = cloudType;
}

int32_t CloudMediaPhotoHandler::OnFetchRecordsInner(
    const OnFetchRecordsReqBody &reqBody, OnFetchRecordsRespBody &respBody)
{
    std::vector<OnFetchRecordsReqBody> reqBodyList;
    reqBody.SplitBy20K(reqBodyList);
    int32_t ret = E_DATA;
    for (const auto &nodeReqBody : reqBodyList) {
        uint32_t operationCode = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_FETCH_RECORDS);
        OnFetchRecordsRespBody nodeRespBody;
        ret = IPC::UserDefineIPCClient()
                  .SetUserId(userId_)
                  .SetTraceId(this->traceId_)
                  .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}})
                  .Post(operationCode, nodeReqBody, nodeRespBody);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "OnFetchRecordsInner failed, ret: %{public}d", ret);
        for (auto data : nodeRespBody.fdirtyDatas) {
            if (data.attributesMediaType == static_cast<int32_t>(MediaType::MEDIA_TYPE_VIDEO)) {
                MEDIA_INFO_LOG("Need clear VideoCache");
                CloudMediaSyncUtils::InvalidVideoCache(data.localPath);
            }
        }
        respBody.MergeRespBody(nodeRespBody);
    }
    return ret;
}

/**
 * stats: 引用入参，按照以下顺序进行返回 [新增，合一，元数据修改，文件修改，删除]:
 * stats: [100, 30, 50, 10, 10]
 */
int32_t CloudMediaPhotoHandler::OnFetchRecords(const std::vector<MDKRecord> &records,
    std::vector<CloudMetaData> &newData, std::vector<CloudMetaData> &fdirtyData,
    std::vector<std::string> &failedRecords, std::vector<int32_t> &stats)
{
    MEDIA_INFO_LOG("OnFetchRecords, size: %{public}zu", records.size());
    CHECK_AND_RETURN_RET_LOG(!records.empty(), E_OK, "OnFetchRecords records is empty");
    int32_t ret;
    OnFetchRecordsReqBody reqBody;
    OnFetchRecordsRespBody respBody;
    CloudFileDataConvert dataConvertor{CloudOperationType::FILE_DATA_MODIFY, userId_};
    for (auto record : records) {
        OnFetchPhotosVo onFetchPhotoVo;
        // 需要从MDKRecord转换成vo 在vo中增加了两个属性
        if (dataConvertor.ConverMDKRecordToOnFetchPhotosVo(record, onFetchPhotoVo) != E_OK) {
            MEDIA_ERR_LOG("OnFetchRecords ConverMDKRecordToOnFetchPhotosVo error, recordId: %{public}s",
                record.GetRecordId().c_str());
            continue;
        }
        MEDIA_DEBUG_LOG("OnFetchPhotoVo: %{public}s", onFetchPhotoVo.ToString().c_str());
        reqBody.AddOnFetchPhotoData(onFetchPhotoVo);
    }
    ret = this->OnFetchRecordsInner(reqBody, respBody);
    failedRecords = respBody.failedRecords;
    stats = respBody.stats;
    newData = this->processor_.GetCloudNewData(respBody.newDatas);
    MEDIA_INFO_LOG("OnFetchRecords NewDataBody: %{public}s", respBody.ToString().c_str());
    fdirtyData = this->processor_.GetCloudFdirtyData(respBody.fdirtyDatas);
    return ret;
}

int32_t CloudMediaPhotoHandler::OnDentryFileInsertInner(
    const OnDentryFileReqBody &reqBody, OnDentryFileRespBody &respBody)
{
    std::vector<OnDentryFileReqBody> reqBodyList;
    reqBody.SplitBy20K(reqBodyList);
    int32_t ret = E_DATA;
    for (const auto &nodeReqBody : reqBodyList) {
        uint32_t operationCode = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_DENTRY_FILE_INSERT);
        OnDentryFileRespBody nodeRespBody;
        ret = IPC::UserDefineIPCClient()
                  .SetUserId(userId_)
                  .SetTraceId(this->traceId_)
                  .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}})
                  .Post(operationCode, nodeReqBody, nodeRespBody);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "OnFetchRecordsInner failed, ret: %{public}d", ret);
        respBody.MergeRespBody(nodeRespBody);
    }
    return ret;
}

int32_t CloudMediaPhotoHandler::OnDentryFileInsert(
    std::vector<MDKRecord> &records, std::vector<std::string> &failedRecords)
{
    MEDIA_INFO_LOG("OnDentryFileInsert, size: %{public}zu", records.size());
    CHECK_AND_RETURN_RET_LOG(!records.empty(), E_OK, "OnDentryFileInsert records is empty");
    int32_t ret;
    OnDentryFileReqBody reqBody;
    OnDentryFileRespBody respBody;
    CloudFileDataConvert dataConvertor{CloudOperationType::FILE_DATA_MODIFY, userId_};
    for (auto record : records) {
        OnFetchPhotosVo onDentryRecord;
        if (dataConvertor.ConverMDKRecordToOnFetchPhotosVo(record, onDentryRecord) != E_OK) {
            MEDIA_ERR_LOG("OnDentryFileInsert ConverMDKRecordToOnFetchPhotosVo error");
            continue;
        }
        MEDIA_INFO_LOG("OnDentryFileInsert OnFetchPhotosVo: %{public}s", onDentryRecord.ToString().c_str());
        reqBody.AddOnDentryFileRecord(onDentryRecord);
    }
    ret = this->OnDentryFileInsertInner(reqBody, respBody);
    failedRecords = respBody.failedRecords;
    MEDIA_INFO_LOG("OnDentryFileInsert end, ret: %{public}d, respBody: %{public}s", ret, respBody.ToString().c_str());
    return ret;
}

int32_t CloudMediaPhotoHandler::GetRetryRecords(std::vector<std::string> &records)
{
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_GET_RETRY_RECORDS);
    GetRetryRecordsRespBody respBody;
    int32_t ret = IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
        .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}})
        .Get(operationCode, respBody);
    records = respBody.cloudIds;
    MEDIA_INFO_LOG("GetRetryRecords completed, result-size: %{public}zu", records.size());
    return ret;
}

int32_t CloudMediaPhotoHandler::GetCheckRecords(
    const std::vector<std::string> &cloudIds, std::unordered_map<std::string, CloudCheckData> &checkRecords)
{
    MEDIA_INFO_LOG("enter CloudMediaPhotoHandler::GetCheckRecords %{public}zu", cloudIds.size());
    CHECK_AND_RETURN_RET_LOG(!cloudIds.empty(), E_OK, "GetCheckRecords cloudIds is empty");
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_GET_CHECK_RECORDS);
    GetCheckRecordsReqBody reqBody;
    reqBody.cloudIds = cloudIds;
    GetCheckRecordsRespBody respBody;
    int32_t ret = IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
        .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}})
        .Post(operationCode, reqBody, respBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to GetCheckRecords");
        return ret;
    }
    MEDIA_INFO_LOG("GetCheckRecords GetCheckRecordsRespBody: %{public}s", respBody.ToString().c_str());
    checkRecords.clear();
    for (auto &[cloudId, info] : respBody.checkDataList) {
        CloudCheckData cloudCheckData;
        cloudCheckData.cloudId = cloudId;
        cloudCheckData.size = info.size;
        cloudCheckData.path = info.data;
        cloudCheckData.fileName = info.fileName;
        cloudCheckData.type = info.mediaType;
        cloudCheckData.version = info.cloudVersion;
        cloudCheckData.position = info.position;
        cloudCheckData.modifiedTime = info.dateModified;
        cloudCheckData.dirtyType = info.dirty;
        cloudCheckData.syncStatus = info.syncStatus;
        cloudCheckData.thmStatus = info.thmStatus;
        cloudCheckData.fileSourceType = info.fileSourceType;
        cloudCheckData.storagePath = info.storagePath;
        for (auto &[key, value] : info.attachment) {
            CloudFileData fileData;
            fileData.fileName = value.fileName;
            fileData.filePath = value.filePath;
            fileData.size = value.size;
            cloudCheckData.attachment[key] = fileData;
        }
        checkRecords[cloudId] = cloudCheckData;
        MEDIA_DEBUG_LOG("GetCheckRecords CloudCheckData: %{public}s", cloudCheckData.ToString().c_str());
    }
    return E_OK;
}

int32_t CloudMediaPhotoHandler::GetCreatedRecords(std::vector<MDKRecord> &records, int32_t size)
{
    CloudMdkRecordPhotosReqBody reqBody;
    reqBody.size = size;
    CloudMdkRecordPhotosRespBody respBody;
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_GET_CREATED_RECORDS);
    int32_t ret = IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
        .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}})
        .Post(operationCode, reqBody, respBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetCreatedRecords fail to call service function");
        return ret;
    }
    std::vector<CloudMdkRecordPhotosVo> createdRecords = respBody.GetPhotosRecords();
    CloudFileDataConvert dataConvertor{CloudOperationType::FILE_CREATE, userId_};
    for (auto &record : createdRecords) {
        MDKRecord dkRecord;
        ret = dataConvertor.ConvertToMdkRecord(record, dkRecord);
        if (ret == E_OK) {
            records.push_back(dkRecord);
        } else {
            MEDIA_ERR_LOG("GetCreatedRecords ReportFailure, ret: %{public}d, photosVo: %{public}s",
                ret,
                record.ToString().c_str());
            this->ReportFailure(
                ReportFailureReqBody()
                    .SetApiCode(static_cast<int32_t>(CloudMediaPhotoOperationCode::CMD_GET_CREATED_RECORDS))
                    .SetErrorCode(ret)
                    .SetFileId(record.fileId)
                    .SetCloudId(record.cloudId));
        }
    }
    MEDIA_INFO_LOG("GetCreatedRecords completed, "
        "query-size: %{public}d, result-size: %{public}zu",
        size,
        records.size());
    return E_OK;
}

int32_t CloudMediaPhotoHandler::GetMetaModifiedRecords(std::vector<MDKRecord> &records, int32_t size, int32_t dirtyType)
{
    CloudMdkRecordPhotosReqBody reqBody;
    reqBody.size = size;
    reqBody.dirtyType = dirtyType;
    CloudMdkRecordPhotosRespBody respBody;
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_GET_META_MODIFIED_RECORDS);
    int32_t ret =
        IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
        .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}})
        .Post(operationCode, reqBody, respBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CloudMediaPhotoHandler::GetMetaModifiedRecords Call IPC Error");
        return ret;
    }
    std::vector<CloudMdkRecordPhotosVo> metaModifiedRecord = respBody.GetPhotosRecords();
    CloudFileDataConvert dataConvertor{CloudOperationType::FILE_METADATA_MODIFY, userId_};
    for (auto &record : metaModifiedRecord) {
        MDKRecord dkRecord;
        MEDIA_INFO_LOG("SetUpdateSourceAlbum CloudMdkRecordPhotosVo: %{public}s", record.ToString().c_str());
        ret = dataConvertor.ConvertToMdkRecord(record, dkRecord);
        if (ret == E_OK) {
            records.push_back(dkRecord);
            if (!record.removeAlbumCloudId.empty()) {
                ret = dataConvertor.InsertAlbumIdChanges(dkRecord, records, record);
            }
        } else {
            MEDIA_ERR_LOG("GetMetaModifiedRecords ReportFailure, ret: %{public}d, photosVo: %{public}s",
                ret,
                record.ToString().c_str());
            this->ReportFailure(
                ReportFailureReqBody()
                    .SetApiCode(static_cast<int32_t>(CloudMediaPhotoOperationCode::CMD_GET_META_MODIFIED_RECORDS))
                    .SetErrorCode(ret)
                    .SetFileId(record.fileId)
                    .SetCloudId(record.cloudId));
        }
    }
    MEDIA_INFO_LOG("GetMetaModifiedRecords completed, "
        "query-size: %{public}d, result-size: %{public}zu",
        size,
        records.size());
    return E_OK;
}

int32_t CloudMediaPhotoHandler::GetFileModifiedRecords(std::vector<MDKRecord> &records, int32_t size)
{
    CloudMdkRecordPhotosReqBody reqBody;
    reqBody.size = size;
    CloudMdkRecordPhotosRespBody respBody;
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_GET_FILE_MODIFIED_RECORDS);
    int32_t ret = IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
        .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}})
        .Post(operationCode, reqBody, respBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetFileModifiedRecords fail to call service function");
        return ret;
    }
    std::vector<CloudMdkRecordPhotosVo> fileModifiedRecord = respBody.GetPhotosRecords();
    CloudFileDataConvert dataConvertor{CloudOperationType::FILE_DATA_MODIFY, userId_};
    for (auto &record : fileModifiedRecord) {
        MDKRecord dkRecord;
        ret = dataConvertor.ConvertToMdkRecord(record, dkRecord);
        if (ret == E_OK) {
            records.push_back(dkRecord);
            if (!record.removeAlbumCloudId.empty()) {
                ret = dataConvertor.InsertAlbumIdChanges(dkRecord, records, record);
            }
        } else {
            MEDIA_ERR_LOG("GetFileModifiedRecords ReportFailure, ret: %{public}d, photosVo: %{public}s",
                ret,
                record.ToString().c_str());
            this->ReportFailure(
                ReportFailureReqBody()
                    .SetApiCode(static_cast<int32_t>(CloudMediaPhotoOperationCode::CMD_GET_FILE_MODIFIED_RECORDS))
                    .SetErrorCode(ret)
                    .SetFileId(record.fileId)
                    .SetCloudId(record.cloudId));
        }
    }
    MEDIA_INFO_LOG("GetFileModifiedRecords completed, "
        "query-size: %{public}d, result-size: %{public}zu",
        size,
        records.size());
    return E_OK;
}

int32_t CloudMediaPhotoHandler::GetDeletedRecords(std::vector<MDKRecord> &records, int32_t size)
{
    CloudMdkRecordPhotosReqBody reqBody;
    reqBody.size = size;
    CloudMdkRecordPhotosRespBody respBody;
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_GET_DELETED_RECORDS);
    int32_t ret = IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
        .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}})
        .Post(operationCode, reqBody, respBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetDeletedRecords fail to call service function");
        return ret;
    }
    std::vector<CloudMdkRecordPhotosVo> deletedRecord = respBody.GetPhotosRecords();
    CloudFileDataConvert dataConvertor{CloudOperationType::FILE_DELETE, userId_};
    for (auto &record : deletedRecord) {
        MDKRecord dkRecord;
        ret = dataConvertor.ConvertToMdkRecord(record, dkRecord);
        if (ret == E_OK) {
            records.push_back(dkRecord);
        } else {
            MEDIA_ERR_LOG("GetDeletedRecords ReportFailure, ret: %{public}d, photosVo: %{public}s",
                ret,
                record.ToString().c_str());
            this->ReportFailure(
                ReportFailureReqBody()
                    .SetApiCode(static_cast<int32_t>(CloudMediaPhotoOperationCode::CMD_GET_DELETED_RECORDS))
                    .SetErrorCode(ret)
                    .SetFileId(record.fileId)
                    .SetCloudId(record.cloudId));
        }
    }
    MEDIA_INFO_LOG("GetDeletedRecords completed, "
        "query-size: %{public}d, result-size: %{public}zu",
        size,
        records.size());
    return E_OK;
}

int32_t CloudMediaPhotoHandler::GetCopyRecords(std::vector<MDKRecord> &records, int32_t size)
{
    CloudMdkRecordPhotosReqBody reqBody;
    reqBody.size = size;
    CloudMdkRecordPhotosRespBody respBody;
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_GET_COPY_RECORDS);
    int32_t ret = IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
        .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}})
        .Post(operationCode, reqBody, respBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetCopyRecords fail to call service function");
        return ret;
    }
    std::vector<CloudMdkRecordPhotosVo> copyRecord = respBody.GetPhotosRecords();
    CloudFileDataConvert dataConvertor{CloudOperationType::FILE_DATA_MODIFY, userId_};
    for (auto &record : copyRecord) {
        MDKRecord dkRecord;
        ret = dataConvertor.ConvertToMdkRecord(record, dkRecord);
        if (ret == E_OK) {
            dkRecord.SetSrcRecordId(record.originalAssetCloudId);
            records.push_back(dkRecord);
        } else {
            MEDIA_ERR_LOG(
                "GetCopyRecords ReportFailure, ret: %{public}d, photosVo: %{public}s", ret, record.ToString().c_str());
            this->ReportFailure(
                ReportFailureReqBody()
                    .SetApiCode(static_cast<int32_t>(CloudMediaPhotoOperationCode::CMD_GET_COPY_RECORDS))
                    .SetErrorCode(ret)
                    .SetFileId(record.fileId)
                    .SetCloudId(record.cloudId));
        }
    }
    MEDIA_INFO_LOG("GetCopyRecords completed, "
        "query-size: %{public}d, result-size: %{public}zu",
        size,
        records.size());
    return E_OK;
}

void CloudMediaPhotoHandler::DeleteTempLivePhotoFile(std::string &livePhotoCachePath)
{
    CHECK_AND_RETURN_LOG(!livePhotoCachePath.empty(), "livePhotoCachePath is empty");
    int result = unlink(livePhotoCachePath.c_str());
    CHECK_AND_RETURN_LOG(result == 0, "unlink err: %{public}d, result: %{public}d, livePhotoCachePath: %{public}s",
        errno, result, livePhotoCachePath.c_str());
}

int32_t CloudMediaPhotoHandler::OnCreateRecords(
    const std::map<std::string, MDKRecordOperResult> &map, int32_t &failSize)
{
    MEDIA_INFO_LOG("OnCreateRecords, size: %{public}zu", map.size());
    CHECK_AND_RETURN_RET_LOG(!map.empty(), E_OK, "OnCreateRecords records is empty");
    OnCreateRecordsPhotosReqBody req;
    CloudFileDataConvert dataConvertor{CloudOperationType::FILE_CREATE, userId_};
    for (auto &entry : map) {
        OnCreateRecord record;
        if (dataConvertor.ConvertToOnCreateRecord(entry.first, entry.second, record) != E_OK) {
            MEDIA_ERR_LOG("OnCreateRecords ConvertToOnCreateRecord error");
            continue;
        }
        MEDIA_INFO_LOG("OnCreateRecords Record:%{public}s", record.ToString().c_str());
        req.records.emplace_back(record);
        DeleteTempLivePhotoFile(record.livePhotoCachePath);
        DeleteTempLivePhotoFile(record.sourceLivePhoto);
    }
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_CREATE_RECORDS);
    FailedSizeResp resp;
    resp.failedSize = 0;
    int32_t ret = IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
        .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}})
        .Post(operationCode, req, resp);
    failSize = resp.failedSize;
    MEDIA_INFO_LOG("OnCreateRecords Resp:%{public}s", resp.ToString().c_str());
    return ret;
}

int32_t CloudMediaPhotoHandler::OnMdirtyRecords(
    const std::map<std::string, MDKRecordOperResult> &map, int32_t &failSize)
{
    MEDIA_INFO_LOG("OnMdirtyRecords, size: %{public}zu", map.size());
    CHECK_AND_RETURN_RET_LOG(!map.empty(), E_OK, "OnMdirtyRecords records is empty");
    OnModifyRecordsPhotosReqBody req;
    CloudFileDataConvert dataConvertor{CloudOperationType::FILE_DATA_MODIFY, userId_};
    for (auto &entry : map) {
        OnModifyRecord record;
        if (dataConvertor.BuildModifyRecord(entry.first, entry.second, record) != E_OK) {
            MEDIA_ERR_LOG("OnMdirtyRecords BuildModifyRecord error");
            continue;
        }
        MEDIA_INFO_LOG("enter OnMdirtyRecords Record:%{public}s", record.ToString().c_str());
        req.AddModifyRecord(record);
    }
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_MDIRTY_RECORDS);
    FailedSizeResp resp;
    resp.failedSize = 0;
    int32_t ret = IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
        .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}})
        .Post(operationCode, req, resp);
    failSize = resp.failedSize;
    MEDIA_INFO_LOG("OnMdirtyRecords Resp:%{public}s", resp.ToString().c_str());
    return ret;
}

int32_t CloudMediaPhotoHandler::OnFdirtyRecords(
    const std::map<std::string, MDKRecordOperResult> &map, int32_t &failSize)
{
    MEDIA_INFO_LOG("OnFdirtyRecords, size: %{public}zu", map.size());
    CHECK_AND_RETURN_RET_LOG(!map.empty(), E_OK, "OnFdirtyRecords records is empty");
    OnFileDirtyRecordsReqBody req;
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_FDIRTY_RECORDS);
    CloudFileDataConvert dataConvertor{CloudOperationType::FILE_DATA_MODIFY, userId_};
    for (const auto &entry : map) {
        OnFileDirtyRecord record;
        if (dataConvertor.ConvertFdirtyRecord(entry.first, entry.second, record) != E_OK) {
            MEDIA_ERR_LOG("OnFdirtyRecords ConvertFdirtyRecord Error");
            continue;
        }
        MEDIA_INFO_LOG("OnFdirtyRecords Record: %{public}s", record.ToString().c_str());
        req.AddRecord(record);
        DeleteTempLivePhotoFile(record.livePhotoCachePath);
        DeleteTempLivePhotoFile(record.sourceLivePhoto);
    }
    FailedSizeResp resp;
    resp.failedSize = 0;
    int32_t ret = IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
        .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}})
        .Post(operationCode, req, resp);
    failSize = resp.failedSize;
    MEDIA_INFO_LOG("OnFdirtyRecords Resp:%{public}s", resp.ToString().c_str());
    return ret;
}

int32_t CloudMediaPhotoHandler::OnDeleteRecords(
    const std::map<std::string, MDKRecordOperResult> &map, int32_t &failSize)
{
    MEDIA_INFO_LOG("OnDeleteRecords, size: %{public}zu", map.size());
    CHECK_AND_RETURN_RET_LOG(!map.empty(), E_OK, "OnDeleteRecords records is empty");
    OnDeleteRecordsPhotosReqBody reqBody;
    OnDeleteRecordsPhotosRespBody resp;
    resp.failSize = 0;
    CloudFileDataConvert dataConvertor{CloudOperationType::FILE_DATA_MODIFY, userId_};
    for (auto &entry : map) {
        const MDKRecordOperResult &result = entry.second;
        OnDeleteRecordsPhoto deleteRecord;
        deleteRecord.dkRecordId = entry.first;
        deleteRecord.cloudId = entry.first;
        deleteRecord.isSuccess = result.IsSuccess();
        reqBody.AddDeleteRecord(deleteRecord);
        MEDIA_INFO_LOG("CloudMediaPhotoHandler::OnDeleteRecords ID: %{public}s", deleteRecord.cloudId.c_str());
    }
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_DELETE_RECORDS);
    int32_t ret = IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
        .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}})
        .Post(operationCode, reqBody, resp);
    failSize = resp.failSize;
    return ret;
}

int32_t CloudMediaPhotoHandler::OnCopyRecords(const std::map<std::string, MDKRecordOperResult> &map, int32_t &failSize)
{
    MEDIA_INFO_LOG("OnCopyRecords, size: %{public}zu", map.size());
    CHECK_AND_RETURN_RET_LOG(!map.empty(), E_OK, "OnCopyRecords records is empty");
    OnCopyRecordsPhotosReqBody req;
    CloudFileDataConvert dataConvertor{CloudOperationType::FILE_COPY, userId_};
    for (auto &entry : map) {
        OnCopyRecord copyRecord;
        if (dataConvertor.BuildCopyRecord(entry.first, entry.second, copyRecord) != E_OK) {
            MEDIA_ERR_LOG("OnCopyRecords BuildCopyRecord error");
            continue;
        }
        MEDIA_INFO_LOG("OnCopyRecords Record JSON:%{public}s", copyRecord.ToString().c_str());
        req.AddCopyRecord(copyRecord);
    }
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_COPY_RECORDS);
    FailedSizeResp resp;
    resp.failedSize = 0;
    int32_t ret = IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
        .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}})
        .Post(operationCode, req, resp);
    failSize = resp.failedSize;
    MEDIA_INFO_LOG("OnCopyRecords Resp:%{public}s", resp.ToString().c_str());
    return ret;
}

int32_t CloudMediaPhotoHandler::OnStartSync()
{
    MEDIA_INFO_LOG("OnStartSync enter");
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_START_SYNC);
    return IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
        .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}})
        .Post(operationCode);
}

int32_t CloudMediaPhotoHandler::OnCompleteSync()
{
    MEDIA_DEBUG_LOG("OnCompleteSync enter");
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_COMPLETE_SYNC);
    return IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
        .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}})
        .Post(operationCode);
}

int32_t CloudMediaPhotoHandler::OnCompletePull(const MediaOperateResult &optRet)
{
    MEDIA_INFO_LOG("OnCompletePull enter");
    MediaOperateResultRespBodyResultNode reqBody;
    reqBody.cloudId = optRet.cloudId;
    reqBody.errorCode = optRet.errorCode;
    reqBody.errorMsg = optRet.errorMsg;
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_COMPLETE_PULL);
    return IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
        .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}})
        .Post(operationCode, reqBody);
}

int32_t CloudMediaPhotoHandler::OnCompletePush()
{
    MEDIA_INFO_LOG("OnCompletePush enter");
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_COMPLETE_PUSH);
    return IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
        .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}})
        .Post(operationCode);
}

int32_t CloudMediaPhotoHandler::OnCompleteCheck()
{
    MEDIA_INFO_LOG("OnCompleteCheck begin");
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_COMPLETE_CHECK);
    return IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
        .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}})
        .Post(operationCode);
}

int32_t CloudMediaPhotoHandler::ReportFailure(const ReportFailureReqBody &reqBody)
{
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_REPORT_FAILURE);
    MEDIA_INFO_LOG("ReportFailure begin, reqBody: %{public}s", reqBody.ToString().c_str());
    return IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
        .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}})
        .Post(operationCode, reqBody);
}

void CloudMediaPhotoHandler::SetCloudSpaceFull(bool isCloudSpaceFull)
{
    return;
}

bool CloudMediaPhotoHandler::IsCloudSpaceFull()
{
    return false;
}
}  // namespace OHOS::Media::CloudSync