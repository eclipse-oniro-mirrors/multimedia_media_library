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

#define MLOG_TAG "Media_Cloud_Controller"

#include "cloud_media_album_controller_service.h"

#include "media_log.h"
#include "media_column.h"
#include "cloud_media_data_controller_processor.h"
#include "media_req_vo.h"
#include "media_resp_vo.h"
#include "cloud_media_data_service.h"
#include "medialibrary_errno.h"
#include "cloud_mdkrecord_photo_album_vo.h"
#include "on_create_records_album_vo.h"
#include "on_delete_records_album_vo.h"
#include "on_delete_albums_vo.h"
#include "on_mdirty_records_album_vo.h"
#include "failed_size_resp_vo.h"

namespace OHOS::Media::CloudSync {
int32_t CloudMediaAlbumControllerService::OnFetchRecords(MessageParcel &data, MessageParcel &reply)
{
    OnFetchRecordsAlbumReqBody req;
    OnFetchRecordsAlbumRespBody resp;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, req);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, IPC::UserDefineIPC().WriteResponseBody(reply, resp, ret),
        "OnFetchRecords Read Req Error");
    if (req.albums.empty()) {
        MEDIA_ERR_LOG("OnFetchRecords Param Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, resp);
    }
    MEDIA_INFO_LOG("OnFetchRecords, size: %{public}zu, data: %{public}s", req.albums.size(), req.ToString().c_str());
    std::vector<PhotoAlbumDto> albumDtoList;
    for (const auto &album : req.albums) {
        PhotoAlbumDto albumDto;
        albumDto.albumId = album.albumId;
        albumDto.cloudId = album.cloudId;
        albumDto.lPath = album.localPath;
        albumDto.albumName = album.albumName;
        albumDto.bundleName = album.albumBundleName;
        albumDto.localLanguage = album.localLanguage;
        albumDto.priority = album.priority;
        albumDto.albumType = album.albumType;
        albumDto.albumSubType = album.albumSubType;
        albumDto.albumDateCreated = album.albumDateCreated;
        albumDto.albumDateAdded = album.albumDateAdded;
        albumDto.albumDateModified = album.albumDateModified;
        albumDto.isDelete = album.isDelete;
        albumDto.coverUriSource = album.coverUriSource;
        albumDto.coverCloudId = album.coverCloudId;
        albumDtoList.emplace_back(albumDto);
        MEDIA_DEBUG_LOG("OnFetchRecords albumDto: %{public}s", albumDto.ToString().c_str());
        MEDIA_DEBUG_LOG("OnFetchRecords album: %{public}s", album.ToString().c_str());
    }
    ret = this->albumService_.OnFetchRecords(albumDtoList, resp);
    MEDIA_INFO_LOG("OnFetchRecords Resp: %{public}s", resp.ToString().c_str());
    return IPC::UserDefineIPC().WriteResponseBody(reply, resp, ret);
}

int32_t CloudMediaAlbumControllerService::OnDentryFileInsert(MessageParcel &data, MessageParcel &reply)
{
    int32_t ret = this->albumService_.OnDentryFileInsert();
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t CloudMediaAlbumControllerService::GetCreatedRecords(MessageParcel &data, MessageParcel &reply)
{
    CloudMdkRecordPhotoAlbumReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("ReadRequestBody failed, ret: %{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    int32_t size = reqBody.size;
    if (size <= 0 || size > LIMIT_SIZE) {
        MEDIA_ERR_LOG("GetCreatedRecords param error, size: %{public}d", size);
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_MEDIA_CLOUD_ARGS_INVAILD);
    }
    std::vector<PhotoAlbumPo> photoAlbumPoList;
    ret = this->albumService_.GetCreatedRecords(reqBody.size, reqBody.isCloudSpaceFull, photoAlbumPoList);
    if (ret != E_OK) {
        MEDIA_INFO_LOG("GetCreatedRecords process error, ret: %{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    std::vector<CloudMdkRecordPhotoAlbumVo> recordsList;
    for (const auto &record : photoAlbumPoList) {
        CloudMdkRecordPhotoAlbumVo recordVo = this->processor_.ConvertRecordPoToVo(record);
        recordsList.push_back(recordVo);
    }
    CloudMdkRecordPhotoAlbumRespBody respBody{recordsList};
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}

int32_t CloudMediaAlbumControllerService::GetMetaModifiedRecords(MessageParcel &data, MessageParcel &reply)
{
    CloudMdkRecordPhotoAlbumReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    int32_t size = reqBody.size;
    if (size <= 0 || size > LIMIT_SIZE) {
        MEDIA_ERR_LOG("GetMetaModifiedRecords param error, size: %{public}d", size);
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_MEDIA_CLOUD_ARGS_INVAILD);
    }
    std::vector<PhotoAlbumPo> photoAlbumPoList = this->albumService_.GetAlbumMetaModifiedRecords(reqBody.size);
    std::vector<CloudMdkRecordPhotoAlbumVo> recordsList;
    for (const auto &record : photoAlbumPoList) {
        CloudMdkRecordPhotoAlbumVo recordVo = this->processor_.ConvertRecordPoToVo(record);
        recordsList.push_back(recordVo);
    }
    MEDIA_INFO_LOG("CloudMediaAlbumControllerService::GetMetaModifiedRecords size: %{public}zu, %{public}zu",
        photoAlbumPoList.size(),
        recordsList.size());
    CloudMdkRecordPhotoAlbumRespBody respBody{recordsList};
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}

int32_t CloudMediaAlbumControllerService::GetDeletedRecords(MessageParcel &data, MessageParcel &reply)
{
    CloudMdkRecordPhotoAlbumReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("get album deleted records error %{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    int32_t size = reqBody.size;
    if (size <= 0 || size > LIMIT_SIZE) {
        MEDIA_ERR_LOG("GetDeletedRecords param error, size: %{public}d", size);
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_MEDIA_CLOUD_ARGS_INVAILD);
    }
    std::vector<PhotoAlbumPo> photoAlbumPoList = this->albumService_.GetAlbumDeletedRecords(reqBody.size);
    std::vector<CloudMdkRecordPhotoAlbumVo> recordsList;
    for (const auto &record : photoAlbumPoList) {
        CloudMdkRecordPhotoAlbumVo recordVo = this->processor_.ConvertRecordPoToVo(record);
        recordsList.push_back(recordVo);
    }
    CloudMdkRecordPhotoAlbumRespBody respBody{recordsList};
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}

int32_t CloudMediaAlbumControllerService::OnCreateRecords(MessageParcel &data, MessageParcel &reply)
{
    OnCreateRecordsAlbumReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("OnCreateRecords Album Read Req Error ret: %{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    MEDIA_INFO_LOG("OnCreateRecords %{public}zu", reqBody.albums.size());
    std::vector<PhotoAlbumDto> albumDtoList;
    for (const auto &album : reqBody.albums) {
        PhotoAlbumDto albumDto = this->processor_.ConvertToPhotoAlbumDto(album);
        MEDIA_DEBUG_LOG("OnCreateRecords record:%{public}s", albumDto.ToString().c_str());
        albumDtoList.emplace_back(albumDto);
    }
    FailedSizeResp resp;
    ret = this->albumService_.OnCreateRecords(albumDtoList, resp.failedSize);
    MEDIA_INFO_LOG("OnCreateRecords Album ret: %{public}d, failedSize:%{public}d", ret, resp.failedSize);
    return IPC::UserDefineIPC().WriteResponseBody(reply, resp, ret);
}

int32_t CloudMediaAlbumControllerService::OnMdirtyRecords(MessageParcel &data, MessageParcel &reply)
{
    OnMdirtyRecordsAlbumReqBody reqBody;
    OnMdirtyRecordsAlbumRespBody respBody;
    respBody.failSize = 0;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CloudMediaAlbumControllerService::OnMdirtyRecords Read Req Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    std::vector<OnMdirtyAlbumRecord> records = reqBody.GetMdirtyRecords();
    std::vector<PhotoAlbumDto> albumDtoList;
    for (const auto &record : records) {
        PhotoAlbumDto albumDto = this->processor_.ConvertToPhotoAlbumDto(record);
        albumDtoList.emplace_back(albumDto);
        MEDIA_DEBUG_LOG("OnMdirtyRecords OnModifyRecord: %{public}s", record.ToString().c_str());
    }
    ret = this->albumService_.OnMdirtyRecords(albumDtoList, respBody.failSize);
    MEDIA_INFO_LOG("CloudMediaAlbumControllerService::OnMdirtyRecords end ret: %{public}d, failSize: %{public}d",
        ret,
        respBody.failSize);
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}

int32_t CloudMediaAlbumControllerService::OnFdirtyRecords(MessageParcel &data, MessageParcel &reply)
{
    int32_t ret = this->albumService_.OnFdirtyRecords();
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t CloudMediaAlbumControllerService::OnDeleteRecords(MessageParcel &data, MessageParcel &reply)
{
    OnDeleteRecordsAlbumReqBody reqBody;
    OnDeleteRecordsAlbumRespBody respBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CloudMediaAlbumControllerService::OnDeleteRecords Read Req Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    std::vector<PhotoAlbumDto> albumDtoList;
    for (const auto &album : reqBody.albums) {
        PhotoAlbumDto albumDto;
        albumDto.cloudId = album.cloudId;
        albumDto.isSuccess = album.isSuccess;
        albumDtoList.emplace_back(albumDto);
    }
    ret = this->albumService_.OnDeleteRecords(albumDtoList, respBody.failSize);
    MEDIA_INFO_LOG("CloudMediaAlbumControllerService::OnDeleteRecords ret: %{public}d, failSize: %{public}d",
        ret,
        respBody.failSize);
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}

int32_t CloudMediaAlbumControllerService::OnCopyRecords(MessageParcel &data, MessageParcel &reply)
{
    int32_t ret = this->albumService_.OnCopyRecords();
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t CloudMediaAlbumControllerService::OnStartSync(MessageParcel &data, MessageParcel &reply)
{
    int32_t ret = this->albumService_.OnStartSync();
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t CloudMediaAlbumControllerService::OnCompleteSync(MessageParcel &data, MessageParcel &reply)
{
    int32_t ret = this->albumService_.OnCompleteSync();
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t CloudMediaAlbumControllerService::OnCompletePull(MessageParcel &data, MessageParcel &reply)
{
    MediaOperateResultRespBodyResultNode reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CloudMediaAlbumControllerService::OnCompletePull Read Req Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    MediaOperateResult optRet;
    optRet.cloudId = reqBody.cloudId;
    optRet.errorCode = reqBody.errorCode;
    optRet.errorMsg = reqBody.errorMsg;
    MEDIA_INFO_LOG("album OnCompletePull: %{public}s", reqBody.ToString().c_str());
    ret = this->albumService_.OnCompletePull(optRet);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t CloudMediaAlbumControllerService::OnCompletePush(MessageParcel &data, MessageParcel &reply)
{
    int32_t ret = this->albumService_.OnCompletePush();
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t CloudMediaAlbumControllerService::OnCompleteCheck(MessageParcel &data, MessageParcel &reply)
{
    int32_t ret = this->albumService_.OnCompleteCheck();
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}
}  // namespace OHOS::Media::CloudSync