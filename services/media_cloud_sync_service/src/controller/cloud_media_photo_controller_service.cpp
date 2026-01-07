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

#define MLOG_TAG "Media_Cloud_Controller"

#include "cloud_media_photo_controller_service.h"

#include "media_column.h"
#include "cloud_media_data_controller_processor.h"
#include "user_define_ipc.h"
#include "media_req_vo.h"
#include "media_resp_vo.h"
#include "cloud_media_photos_service.h"
#include "cloud_media_data_service.h"
#include "medialibrary_errno.h"
#include "cloud_mdkrecord_photos_vo.h"
#include "on_delete_records_photos_vo.h"
#include "on_copy_records_photos_vo.h"
#include "on_create_records_photos_vo.h"
#include "on_modify_records_photos_vo.h"
#include "on_fetch_records_vo.h"
#include "on_modify_file_dirty_vo.h"
#include "get_retey_records_vo.h"
#include "on_dentry_file_vo.h"
#include "on_fetch_photos_vo.h"
#include "failed_size_resp_vo.h"
#include "get_check_records_vo.h"
#include "report_failure_vo.h"

namespace OHOS::Media::CloudSync {
int32_t CloudMediaPhotoControllerService::OnFetchRecords(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter CloudMediaPhotoControllerService::OnFetchRecords");
    OnFetchRecordsReqBody reqBody;
    OnFetchRecordsRespBody respBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("OnFetchRecords Read Req Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    // 获取端云请求中传递过来的数据
    std::vector<OnFetchPhotosVo> onFetchPhotoDatas = reqBody.GetOnFetchPhotoData();
    std::vector<std::string> cloudIds;
    std::map<std::string, CloudMediaPullDataDto> cloudIdRelativeMap;
    std::vector<PhotosDto> newData;
    std::vector<PhotosDto> fdirtyData;
    std::vector<int32_t> stats{0, 0, 0, 0, 0};
    std::vector<std::string> failedRecords;
    MEDIA_INFO_LOG("OnFetchRecords onFetchPhotoDatas size: %{public}zu", onFetchPhotoDatas.size());
    for (auto onFetchPhotoData : onFetchPhotoDatas) {
        cloudIds.emplace_back(onFetchPhotoData.cloudId);
        // 从vo对象转成数据库的dto对象 在dto中增加attributesFileSourceType 和 attributesStoragePath 两个属性
        CloudMediaPullDataDto pullData = this->processor_.ConvertToCloudMediaPullData(onFetchPhotoData);
        cloudIdRelativeMap[onFetchPhotoData.cloudId] = pullData;
        MEDIA_DEBUG_LOG("OnFetchRecords CloudMediaPullData: %{public}s", pullData.ToString().c_str());
    }
    ret = this->photosService_.OnFetchRecords(cloudIds, cloudIdRelativeMap, newData, fdirtyData, stats, failedRecords);
    respBody.stats = stats;
    respBody.failedRecords = failedRecords;
    respBody.newDatas = this->processor_.SetNewDataVoFromDto(newData);
    respBody.fdirtyDatas = this->processor_.SetFdirtyDataVoFromDto(fdirtyData);
    MEDIA_INFO_LOG("OnFetchRecords Resp: %{public}s, size:%{public}zu", respBody.ToString().c_str(), newData.size());
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}

int32_t CloudMediaPhotoControllerService::OnDentryFileInsert(MessageParcel &data, MessageParcel &reply)
{
    OnDentryFileReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("OnDentryFileInsert Read Req Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    OnDentryFileRespBody respBody;
    std::vector<std::string> failedRecords;
    std::vector<OnFetchPhotosVo> onDentryRecords = reqBody.GetOnDentryFileRecord();
    std::vector<CloudMediaPullDataDto> pullDatas;
    for (auto onDentryRecord : onDentryRecords) {
        CloudMediaPullDataDto pullData = this->processor_.ConvertToCloudMediaPullData(onDentryRecord);
        pullDatas.emplace_back(pullData);
        MEDIA_DEBUG_LOG("OnDentryFileInsert PullData: %{public}s", pullData.ToString().c_str());
    }
    ret = this->photosService_.OnDentryFileInsert(pullDatas, failedRecords);
    respBody.failedRecords = failedRecords;
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}

int32_t CloudMediaPhotoControllerService::GetCheckRecords(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter CloudMediaPhotoControllerService::GetCheckRecords");
    GetCheckRecordsReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetCheckRecords Read Req Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    int32_t size = static_cast<int32_t>(reqBody.cloudIds.size());
    if (size <= 0 || size > LIMIT_SIZE) {
        MEDIA_ERR_LOG("GetCheckRecords param error, size: %{public}d", size);
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_MEDIA_CLOUD_ARGS_INVAILD);
    }
    MEDIA_INFO_LOG("GetCheckRecords reqBody: %{public}s", reqBody.ToString().c_str());
    std::vector<PhotosDto> photosDtoVec = this->photosService_.GetCheckRecords(reqBody.cloudIds);
    GetCheckRecordsRespBody respBody;
    respBody.checkDataList = this->processor_.GetCheckRecordsRespBody(photosDtoVec);
    MEDIA_INFO_LOG("GetCheckRecords RespBody: %{public}s", respBody.ToString().c_str());
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}

int32_t CloudMediaPhotoControllerService::GetCreatedRecords(MessageParcel &data, MessageParcel &reply)
{
    CloudMdkRecordPhotosReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CloudMediaPhotoControllerService::GetCreatedRecords Read Req Error %{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    int32_t size = reqBody.size;
    if (size <= 0 || size > LIMIT_SIZE) {
        MEDIA_ERR_LOG("GetCreatedRecords param error, size: %{public}d", size);
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_MEDIA_CLOUD_ARGS_INVAILD);
    }
    std::vector<PhotosPo> createdRecordsPoList;
    ret = this->photosService_.GetCreatedRecords(reqBody.size, createdRecordsPoList);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetCreatedRecords process error, ret: %{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    std::vector<CloudMdkRecordPhotosVo> createdRecordsList;
    for (const auto &createdRecord : createdRecordsPoList) {
        createdRecordsList.emplace_back(this->processor_.ConvertRecordPoToVo(createdRecord));
    }
    CloudMdkRecordPhotosRespBody respBody{createdRecordsList};
    respBody.TruncateDataBy200K();
    MEDIA_INFO_LOG("GetCreatedRecords completed, dataSize: %{public}zu", respBody.GetDataSize());
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}

int32_t CloudMediaPhotoControllerService::GetMetaModifiedRecords(MessageParcel &data, MessageParcel &reply)
{
    CloudMdkRecordPhotosReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_INFO_LOG("CloudMediaPhotoControllerService::GetMetaModifiedRecords Read Req Err");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    int32_t size = reqBody.size;
    if (size <= 0 || size > LIMIT_SIZE) {
        MEDIA_ERR_LOG("GetMetaModifiedRecords param error, size: %{public}d", size);
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_MEDIA_CLOUD_ARGS_INVAILD);
    }
    bool isInvalidType = reqBody.dirtyType != static_cast<int32_t>(DirtyType::TYPE_MDIRTY) &&
                         reqBody.dirtyType != static_cast<int32_t>(DirtyType::TYPE_SDIRTY);
    if (isInvalidType) {
        MEDIA_ERR_LOG("GetMetaModifiedRecords param error, dirtyType: %{public}d", reqBody.dirtyType);
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_MEDIA_CLOUD_ARGS_INVAILD);
    }
    std::vector<PhotosPo> metaModifiedRecordsPoList;
    ret = this->photosService_.GetMetaModifiedRecords(reqBody.size, metaModifiedRecordsPoList, reqBody.dirtyType);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetMetaModifiedRecords process error, ret: %{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    std::vector<CloudMdkRecordPhotosVo> metaModifiedRecordsList;
    for (const auto &metaModifiedRecord : metaModifiedRecordsPoList) {
        metaModifiedRecordsList.emplace_back(this->processor_.ConvertRecordPoToVo(metaModifiedRecord));
    }
    CloudMdkRecordPhotosRespBody respBody{metaModifiedRecordsList};
    respBody.TruncateDataBy200K();
    MEDIA_INFO_LOG("GetMetaModifiedRecords completed, dataSize: %{public}zu", respBody.GetDataSize());
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}

int32_t CloudMediaPhotoControllerService::GetFileModifiedRecords(MessageParcel &data, MessageParcel &reply)
{
    CloudMdkRecordPhotosReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetFileModifiedRecords Read Req ERR");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    int32_t size = reqBody.size;
    if (size <= 0 || size > LIMIT_SIZE) {
        MEDIA_ERR_LOG("GetFileModifiedRecords param error, size: %{public}d", size);
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_MEDIA_CLOUD_ARGS_INVAILD);
    }
    std::vector<PhotosPo> fileModifiedRecordsPoList;
    ret = this->photosService_.GetFileModifiedRecords(reqBody.size, fileModifiedRecordsPoList);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetFileModifiedRecords process error, ret: %{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    std::vector<CloudMdkRecordPhotosVo> fileModifiedRecordsList;
    for (const auto &fileModifiedRecord : fileModifiedRecordsPoList) {
        fileModifiedRecordsList.emplace_back(this->processor_.ConvertRecordPoToVo(fileModifiedRecord));
    }
    CloudMdkRecordPhotosRespBody respBody{fileModifiedRecordsList};
    respBody.TruncateDataBy200K();
    MEDIA_INFO_LOG("GetFileModifiedRecords completed, dataSize: %{public}zu", respBody.GetDataSize());
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}

int32_t CloudMediaPhotoControllerService::GetDeletedRecords(MessageParcel &data, MessageParcel &reply)
{
    CloudMdkRecordPhotosReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetDeletedRecords ReadRequestBody error %{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    int32_t size = reqBody.size;
    if (size <= 0 || size > LIMIT_SIZE) {
        MEDIA_ERR_LOG("GetDeletedRecords param error, size: %{public}d", size);
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_MEDIA_CLOUD_ARGS_INVAILD);
    }
    std::vector<PhotosPo> deletedRecordsPoList = this->photosService_.GetDeletedRecords(reqBody.size);
    std::vector<CloudMdkRecordPhotosVo> deletedRecordsList;
    for (const auto &deletedRecord : deletedRecordsPoList) {
        deletedRecordsList.emplace_back(this->processor_.ConvertRecordPoToVo(deletedRecord));
    }
    CloudMdkRecordPhotosRespBody respBody{deletedRecordsList};
    respBody.TruncateDataBy200K();
    MEDIA_INFO_LOG("GetDeletedRecords completed, dataSize: %{public}zu", respBody.GetDataSize());
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}

int32_t CloudMediaPhotoControllerService::GetCopyRecords(MessageParcel &data, MessageParcel &reply)
{
    CloudMdkRecordPhotosReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    int32_t size = reqBody.size;
    if (size <= 0 || size > LIMIT_SIZE) {
        MEDIA_ERR_LOG("GetCopyRecords param error, size: %{public}d", size);
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_MEDIA_CLOUD_ARGS_INVAILD);
    }
    std::vector<PhotosPo> copyRecordsPoList;
    ret = this->photosService_.GetCopyRecords(reqBody.size, copyRecordsPoList);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetCopyRecords process error, ret: %{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    std::vector<CloudMdkRecordPhotosVo> copyRecordsList;
    for (const auto &copyRecord : copyRecordsPoList) {
        copyRecordsList.emplace_back(this->processor_.ConvertRecordPoToVo(copyRecord));
    }
    CloudMdkRecordPhotosRespBody respBody{copyRecordsList};
    respBody.TruncateDataBy200K();
    MEDIA_INFO_LOG("GetCopyRecords completed, dataSize: %{public}zu", respBody.GetDataSize());
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}

int32_t CloudMediaPhotoControllerService::OnCreateRecords(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter OnCreateRecords");
    OnCreateRecordsPhotosReqBody req;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, req);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("OnCreateRecords Read Req Err");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    std::vector<PhotosDto> photos;
    for (const auto &record : req.records) {
        PhotosDto photo = this->processor_.ConvertToPhotoDto(record);
        MEDIA_DEBUG_LOG("OnCreateRecords record: %{public}s", record.ToString().c_str());
        photos.emplace_back(photo);
    }
    FailedSizeResp resp;
    ret = this->photosService_.OnCreateRecords(photos, resp.failedSize);
    MEDIA_INFO_LOG("OnCreateRecords ret: %{public}d, failedSize:%{public}d", ret, resp.failedSize);
    return IPC::UserDefineIPC().WriteResponseBody(reply, resp, ret);
}

int32_t CloudMediaPhotoControllerService::OnMdirtyRecords(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter OnMdirtyRecords");
    OnModifyRecordsPhotosReqBody req;
    FailedSizeResp resp;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, req);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("OnMdirtyRecords Read Req Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    std::vector<OnModifyRecord> records = req.GetModifyRecords();
    std::vector<PhotosDto> photos;
    for (const auto &record : records) {
        PhotosDto photo;
        this->processor_.ConvertToPhotosDto(record, photo);
        photos.emplace_back(photo);
        MEDIA_DEBUG_LOG("OnMdirtyRecords OnModifyRecord: %{public}s", record.ToString().c_str());
    }
    ret = this->photosService_.OnMdirtyRecords(photos, resp.failedSize);
    return IPC::UserDefineIPC().WriteResponseBody(reply, resp, ret);
}

int32_t CloudMediaPhotoControllerService::OnFdirtyRecords(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("OnFdirtyRecords enter");
    OnFileDirtyRecordsReqBody req;
    FailedSizeResp resp;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, req);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("OnFdirtyRecords Read Req Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    std::vector<PhotosDto> photos;
    for (const auto &entry : req.records) {
        PhotosDto photo;
        this->processor_.ConvertToPhotosDto(entry, photo);
        MEDIA_INFO_LOG(
            "OnFdirtyRecords Photo: %{public}s, entry: %{public}s", photo.ToString().c_str(), entry.ToString().c_str());
        photos.emplace_back(photo);
    }
    ret = this->photosService_.OnFdirtyRecords(photos, resp.failedSize);
    MEDIA_INFO_LOG("OnFdirtyRecords end Ret:%{public}d", ret);
    return IPC::UserDefineIPC().WriteResponseBody(reply, resp, ret);
}

int32_t CloudMediaPhotoControllerService::OnDeleteRecords(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("CloudMediaPhotoControllerService::OnDeleteRecords");
    OnDeleteRecordsPhotosReqBody reqBody;
    OnDeleteRecordsPhotosRespBody resp;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CloudMediaPhotoControllerService::OnDeleteRecords Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    std::vector<PhotosDto> photoDtoList;
    for (const auto &photo : reqBody.records) {
        PhotosDto photoDto;
        photoDto.cloudId = photo.cloudId;
        photoDto.dkRecordId = photo.dkRecordId;
        photoDto.isSuccess = photo.isSuccess;
        photoDtoList.emplace_back(photoDto);
    }
    MEDIA_INFO_LOG("CloudMediaPhotoControllerService::OnDeleteRecords size: %{public}zu", photoDtoList.size());
    ret = this->photosService_.OnDeleteRecords(photoDtoList, resp.failSize);
    return IPC::UserDefineIPC().WriteResponseBody(reply, resp, ret);
}

int32_t CloudMediaPhotoControllerService::OnCopyRecords(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter CloudMediaPhotoControllerService::OnCopyRecords");
    OnCopyRecordsPhotosReqBody req;
    FailedSizeResp resp;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, req);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("OnCopyRecords Read Req Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    std::vector<OnCopyRecord> records = req.GetRecords();
    std::vector<PhotosDto> photos;
    for (const auto &record : records) {
        PhotosDto photo;
        photo.fileId = record.fileId;
        photo.fileType = record.fileType;
        photo.size = record.size;
        photo.createTime = record.createTime;
        photo.modifiedTime = record.modifyTime;
        photo.cloudVersion = record.version;
        photo.rotation = record.rotation;
        photo.cloudId = record.cloudId;
        photo.path = record.path;
        photo.fileName = record.fileName;
        photo.sourcePath = record.sourcePath;
        photo.isSuccess = record.isSuccess;
        photo.errorType = record.errorType;
        photo.serverErrorCode = record.serverErrorCode;
        photo.errorDetails = record.errorDetails;
        photos.emplace_back(photo);
        MEDIA_DEBUG_LOG("OnCopyRecords record: %{public}s", record.ToString().c_str());
    }
    ret = this->photosService_.OnCopyRecords(photos, resp.failedSize);
    return IPC::UserDefineIPC().WriteResponseBody(reply, resp, ret);
}

int32_t CloudMediaPhotoControllerService::GetRetryRecords(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter GetRetryRecords");
    GetRetryRecordsRespBody respBody;
    int32_t ret = this->photosService_.GetRetryRecords(respBody.cloudIds);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetRetryRecords ret error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}

int32_t CloudMediaPhotoControllerService::OnStartSync(MessageParcel &data, MessageParcel &reply)
{
    int32_t ret = this->photosService_.OnStartSync();
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t CloudMediaPhotoControllerService::OnCompleteSync(MessageParcel &data, MessageParcel &reply)
{
    int32_t ret = this->photosService_.OnCompleteSync();
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t CloudMediaPhotoControllerService::OnCompletePull(MessageParcel &data, MessageParcel &reply)
{
    MediaOperateResultRespBodyResultNode reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CloudMediaPhotoControllerService::OnCompletePull Read Req Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    MediaOperateResult optRet;
    optRet.cloudId = reqBody.cloudId;
    optRet.errorCode = reqBody.errorCode;
    optRet.errorMsg = reqBody.errorMsg;
    MEDIA_INFO_LOG("photo OnCompletePull: %{public}s", reqBody.ToString().c_str());
    ret = this->photosService_.OnCompletePull(optRet);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t CloudMediaPhotoControllerService::OnCompletePush(MessageParcel &data, MessageParcel &reply)
{
    int32_t ret = this->photosService_.OnCompletePush();
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t CloudMediaPhotoControllerService::OnCompleteCheck(MessageParcel &data, MessageParcel &reply)
{
    int32_t ret = this->photosService_.OnCompleteCheck();
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t CloudMediaPhotoControllerService::ReportFailure(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter CloudMediaPhotoControllerService::ReportFailure");
    ReportFailureReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("ReportFailure Read Req Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = this->photosService_.ReportFailure(this->processor_.GetReportFailureDto(reqBody));
    return IPC::UserDefineIPC().WriteResponseBody(reply);
}
}  // namespace OHOS::Media::CloudSync