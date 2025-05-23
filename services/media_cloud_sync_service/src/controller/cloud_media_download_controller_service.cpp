/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License"){return 0;}
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

#include "cloud_media_download_controller_service.h"

#include "media_log.h"
#include "media_column.h"
#include "user_define_ipc.h"
#include "media_req_vo.h"
#include "media_resp_vo.h"
#include "medialibrary_errno.h"
#include "get_download_thm_vo.h"
#include "get_download_thm_num_vo.h"
#include "get_download_thm_by_uri_vo.h"
#include "on_download_thms_vo.h"
#include "media_operate_result_vo.h"
#include "cloud_media_uri_utils.h"
#include "get_download_asset_vo.h"
#include "on_download_asset_vo.h"

namespace OHOS::Media::CloudSync {
void CloudMediaDownloadControllerService::GetDownloadThms(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter GetDownloadThms");
    GetDownloadThmReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    int32_t size = reqBody.size;
    MEDIA_INFO_LOG("GetDownloadThms size: %{public}d.", reqBody.size);
    if (size <= 0 || size > LIMIT_SIZE) {
        MEDIA_ERR_LOG("QueryThumbsToDownload param error, size: %{public}d", size);
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_MEDIA_CLOUD_ARGS_INVAILD);
    }
    DownloadThumbnailQueryDto queryDto = this->processor_.GetDownloadThumbnailQueryDto(reqBody);
    std::vector<PhotosDto> photosDtoVec;
    ret = this->service_.GetDownloadThms(queryDto, photosDtoVec);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetDownloadThms ret error, ret: %{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    std::vector<PhotosVo> photosVoList;
    for (auto &photosDto : photosDtoVec) {
        PhotosVo photosVo = this->processor_.ConvertPhotosDtoToPhotosVo(photosDto);
        MEDIA_INFO_LOG("GetDownloadThms Dto: %{public}s, Vo: %{public}s.",
            photosDto.ToString().c_str(),
            photosVo.ToString().c_str());
        photosVoList.push_back(photosVo);
    }
    GetDownloadThmRespBody respBody;
    respBody.photos = photosVoList;
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}

void CloudMediaDownloadControllerService::GetDownloadThmNum(MessageParcel &data, MessageParcel &reply)
{
    GetDownloadThmNumReqBody req;
    GetDownloadThmNumRespBody resp;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, req);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetDownloadThmNum Get Req Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    MEDIA_INFO_LOG("GetDownloadThmNum begin %{public}d", req.type);
    ret = this->service_.GetDownloadThmNum(req.type, resp.totalNum);
    MEDIA_INFO_LOG("GetDownloadThmNum end count %{public}d, ret %{public}d", resp.totalNum, ret);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetDownloadThmNum ret error, ret: %{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    return IPC::UserDefineIPC().WriteResponseBody(reply, resp, ret);
}

void CloudMediaDownloadControllerService::GetDownloadThmsByUri(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter GetDownloadThmsByUri");
    GetDownloadThmsByUriReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_INFO_LOG("GetDownloadThmsByUri Read Req Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    std::vector<int32_t> fileIds;
    ret = CloudMediaUriUtils::GetFileIds(reqBody.pathList, fileIds);
    if (ret != E_OK) {
        MEDIA_INFO_LOG("GetFileIds Error, ret = %{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    std::vector<PhotosVo> photosVoList;
    std::vector<PhotosDto> photosDtoList = this->service_.GetDownloadThmsByUri(fileIds, reqBody.thmType);
    MEDIA_INFO_LOG(
        "GetDownloadThmsByUri Query:%{public}zu, Result:%{public}zu", photosDtoList.size(), photosVoList.size());
    for (auto &photosDto : photosDtoList) {
        PhotosVo photosVo = this->processor_.ConvertPhotosDtoToPhotosVo(photosDto);
        MEDIA_INFO_LOG("GetDownloadThmsByUri PhotoVo: %{public}s", photosVo.ToString().c_str());
        photosVoList.push_back(photosVo);
    }
    GetDownloadThmsByUriRespBody respBody;
    respBody.photos = photosVoList;
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}

void CloudMediaDownloadControllerService::OnDownloadThms(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter OnDownloadThms");
    OnDownloadThmsReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    int32_t size = static_cast<int32_t>(reqBody.downloadThmsDataList.size());
    if (size <= 0 || size > LIMIT_SIZE) {
        MEDIA_ERR_LOG("OnDownloadThms param error, size: %{public}d", size);
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_MEDIA_CLOUD_ARGS_INVAILD);
    }
    MEDIA_INFO_LOG("downloadThumbnailMap: %{public}s", reqBody.ToString().c_str());
    std::unordered_map<std::string, int32_t> downloadThumbnailMap;
    for (size_t i = 0; i < reqBody.downloadThmsDataList.size(); i++) {
        OnDownloadThmsReqBody::DownloadThmsData downloadThmsData = reqBody.downloadThmsDataList[i];
        downloadThumbnailMap[downloadThmsData.cloudId] = downloadThmsData.thumbStatus;
    }
    MEDIA_INFO_LOG("downloadThumbnailMap: %{public}zu", downloadThumbnailMap.size());
    std::vector<MediaOperateResultDto> resultDtos;
    ret = this->service_.OnDownloadThms(downloadThumbnailMap, resultDtos);
    MediaOperateResultRespBody respBody;
    respBody.result = this->processor_.GetMediaOperateResult(resultDtos);
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}

void CloudMediaDownloadControllerService::GetDownloadAsset(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter CloudMediaDataControllerService::GetDownloadAsset");
    GetDownloadAssetReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_INFO_LOG("CloudMediaDataControllerService::GetDownloadAsset Read Req Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    std::vector<int32_t> fileIds;
    ret = CloudMediaUriUtils::GetFileIds(reqBody.pathList, fileIds);
    if (ret != E_OK) {
        MEDIA_INFO_LOG("GetFileIds Error, ret = %{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    std::vector<PhotosDto> photosDtoList = this->service_.GetDownloadAsset(fileIds);
    std::vector<PhotosVo> photosVoList;
    for (auto &photosDto : photosDtoList) {
        PhotosVo photosVo = this->processor_.ConvertPhotosDtoToPhotosVo(photosDto);
        MEDIA_INFO_LOG("GetDownloadAsset PhotoVo: %{public}s", photosVo.ToString().c_str());
        photosVoList.push_back(photosVo);
    }
    MEDIA_INFO_LOG("CloudMediaDataControllerService::GetDownloadAsset Query:%{public}zu, Result:%{public}zu",
        photosDtoList.size(),
        photosVoList.size());
    GetDownloadAssetRespBody respBody;
    respBody.photos = photosVoList;
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}

void CloudMediaDownloadControllerService::OnDownloadAsset(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter CloudMediaDataControllerService::OnDownloadAsset");
    OnDownloadAssetReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    MEDIA_INFO_LOG("OnDownloadAsset: %{public}s", reqBody.ToString().c_str());
    std::vector<MediaOperateResultDto> resultDtos;
    ret = this->service_.OnDownloadAsset(reqBody.cloudIds, resultDtos);
    MediaOperateResultRespBody respBody;
    respBody.result = this->processor_.GetMediaOperateResult(resultDtos);
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}
}  // namespace OHOS::Media::CloudSync