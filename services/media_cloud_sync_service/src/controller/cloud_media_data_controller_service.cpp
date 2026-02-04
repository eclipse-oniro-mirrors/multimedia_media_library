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

#include "cloud_media_data_controller_service.h"

#include "cloud_sync_unprepared_data_vo.h"
#include "media_log.h"
#include "media_column.h"
#include "user_define_ipc.h"
#include "media_req_vo.h"
#include "media_resp_vo.h"
#include "medialibrary_errno.h"
#include "update_dirty_vo.h"
#include "update_position_vo.h"
#include "update_sync_status_vo.h"
#include "update_thm_status_vo.h"
#include "get_aging_file_vo.h"
#include "get_video_to_cache_vo.h"
#include "get_file_pos_stat_vo.h"
#include "get_cloud_thm_stat_vo.h"
#include "get_dirty_type_stat_vo.h"
#include "cloud_mdkrecord_photos_vo.h"
#include "update_local_file_dirty_vo.h"
#include "media_operate_result_vo.h"
#include "query_data_vo.h"
#include "update_data_vo.h"

namespace OHOS::Media::CloudSync {
int32_t CloudMediaDataControllerService::UpdateDirty(MessageParcel &data, MessageParcel &reply)
{
    UpdateDirtyReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("UpdateDirtyForCloudCheck Read Req Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = this->dataService_.UpdateDirty(reqBody.cloudId, reqBody.dirtyType);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t CloudMediaDataControllerService::UpdatePosition(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter UpdatePositionForCloudCheck");
    UpdatePositionReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("UpdatePositionForCloudCheck Read Req Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = this->dataService_.UpdatePosition(reqBody.cloudIds, reqBody.position);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t CloudMediaDataControllerService::UpdatePosWithType(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter UpdatePosWithTypeForCloudCheck");
    UpdatePositionReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("UpdatePositionForCloudCheck Read Req Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = this->dataService_.UpdatePosWithType(reqBody.cloudIds, reqBody.position, reqBody.fileSourceType);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t CloudMediaDataControllerService::UpdateFileSourceType(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter UpdateFileSourceType");
    UpdatePositionReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("UpdateFileSourceType Read Req Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = this->dataService_.UpdateFileSourceType(reqBody.cloudIds, reqBody.fileSourceType);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t CloudMediaDataControllerService::UpdateSyncStatus(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter UpdateSyncStatus");
    UpdateSyncStatusReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("UpdateSyncStatus Read Req Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = this->dataService_.UpdateSyncStatus(reqBody.cloudId, reqBody.syncStatus);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t CloudMediaDataControllerService::UpdateThmStatus(MessageParcel &data, MessageParcel &reply)
{
    UpdateThmStatusReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = this->dataService_.UpdateThmStatus(reqBody.cloudId, reqBody.thmStatus);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t CloudMediaDataControllerService::GetAgingFile(MessageParcel &data, MessageParcel &reply)
{
    GetAgingFileReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    int32_t sizeLimit = reqBody.sizeLimit;
    if (sizeLimit <= 0 || sizeLimit > LIMIT_SIZE) {
        MEDIA_ERR_LOG("GetAgingFile param error, sizeLimit = %{public}d", sizeLimit);
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_MEDIA_CLOUD_ARGS_INVAILD);
    }
    AgingFileQueryDto queryDto;
    this->processor_.GetAgingFileQueryDto(reqBody, queryDto);
    std::vector<PhotosDto> photosDtoList;
    ret = this->dataService_.GetAgingFile(queryDto, photosDtoList);
    if (ret != E_OK) {
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    std::vector<PhotosVo> photosVoList;
    for (const auto &photosDto : photosDtoList) {
        PhotosVo photosVo = this->processor_.ConvertPhotosDtoToPhotosVo(photosDto);
        photosVoList.push_back(photosVo);
    }
    GetAgingFileRespBody respBody;
    respBody.photos = photosVoList;
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}

// 主动老化
int32_t CloudMediaDataControllerService::GetActiveAgingFile(MessageParcel &data, MessageParcel &reply)
{
    GetAgingFileReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    int32_t sizeLimit = reqBody.sizeLimit;
    if (sizeLimit <= 0 || sizeLimit > LIMIT_SIZE) {
        MEDIA_ERR_LOG("GetActiveAgingFile param error, sizeLimit = %{public}d", sizeLimit);
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_MEDIA_CLOUD_ARGS_INVAILD);
    }
    AgingFileQueryDto queryDto;
    this->processor_.GetAgingFileQueryDto(reqBody, queryDto);
    std::vector<PhotosDto> photosDtoList;
    ret = this->dataService_.GetActiveAgingFile(queryDto, photosDtoList);
    if (ret != E_OK) {
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    std::vector<PhotosVo> photosVoList;
    for (const auto &photosDto : photosDtoList) {
        PhotosVo photosVo = this->processor_.ConvertPhotosDtoToPhotosVo(photosDto);
        photosVoList.push_back(photosVo);
    }
    GetAgingFileRespBody respBody;
    respBody.photos = photosVoList;
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}

int32_t CloudMediaDataControllerService::GetVideoToCache(MessageParcel &data, MessageParcel &reply)
{
    std::vector<PhotosDto> photosDtoVec;
    int32_t ret = this->dataService_.GetVideoToCache(photosDtoVec);
    if (ret != E_OK) {
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    std::vector<PhotosVo> photosVoList;
    for (const auto &photosDto : photosDtoVec) {
        PhotosVo photosVo = this->processor_.ConvertPhotosDtoToPhotosVo(photosDto);
        photosVoList.push_back(photosVo);
    }
    GetVideoToCacheRespBody respBody;
    respBody.photos = photosVoList;
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}

int32_t CloudMediaDataControllerService::GetFilePosStat(MessageParcel &data, MessageParcel &reply)
{
    std::vector<uint64_t> statList = this->dataService_.GetFilePosStat();
    GetFilePosStatRespBody respBody;
    respBody.statList = statList;
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}

int32_t CloudMediaDataControllerService::GetCloudThmStat(MessageParcel &data, MessageParcel &reply)
{
    std::vector<uint64_t> statList = this->dataService_.GetCloudThmStat();
    GetCloudThmStatRespBody respBody;
    respBody.statList = statList;
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}

int32_t CloudMediaDataControllerService::GetDirtyTypeStat(MessageParcel &data, MessageParcel &reply)
{
    std::vector<uint64_t> statList;
    int32_t ret = this->dataService_.GetDirtyTypeStat(statList);
    if (ret != E_OK) {
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    GetDirtyTypeStatRespBody respBody;
    respBody.statList = statList;
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}

int32_t CloudMediaDataControllerService::UpdateLocalFileDirty(MessageParcel &data, MessageParcel &reply)
{
    UpdateLocalFileDirtyReqBody req;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, req);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("UpdateLocalFileDirty Get Req Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = this->dataService_.UpdateLocalFileDirty(req.cloudIds);
    MEDIA_INFO_LOG("UpdateLocalFileDirty %{public}zu, %{public}d", req.cloudIds.size(), ret);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t CloudMediaDataControllerService::GetCloudSyncUnPreparedData(MessageParcel &data, MessageParcel &reply)
{
    CloudSyncUnPreparedDataRespBody respBody;
    int32_t ret = this->enhanceService_.GetCloudSyncUnPreparedData(respBody.count);
    MEDIA_INFO_LOG("GetCloudSyncUnPreparedData %{public}d, %{public}d", respBody.count, ret);
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}

int32_t CloudMediaDataControllerService::SubmitCloudSyncPreparedDataTask(MessageParcel &data, MessageParcel &reply)
{
    int32_t ret = this->enhanceService_.SubmitCloudSyncPreparedDataTask();
    MEDIA_INFO_LOG("SubmitCloudSyncPreparedDataTask, %{public}d", ret);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t CloudMediaDataControllerService::CheckAndFixAlbum(MessageParcel &data, MessageParcel &reply)
{
    int32_t ret = this->dataService_.CheckAndFixAlbum();
    MEDIA_INFO_LOG("CheckAndFixAlbum, %{public}d", ret);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t CloudMediaDataControllerService::QueryData(MessageParcel &data, MessageParcel &reply)
{
    QueryDataReqBody req;
    QueryDataRespBody resp;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, req);
    CHECK_AND_RETURN_RET_LOG(
        ret == E_OK,
        IPC::UserDefineIPC().WriteResponseBody(reply, resp, ret),
        "QueryData Read Req Error");
    DataShare::DataSharePredicates predicates = req.predicates;
    std::vector<std::string> columnNames = req.columnNames;
    std::string tableName = req.tableName;
    std::vector<std::unordered_map<std::string, std::string>> results;
    ret = this->dataService_.QueryData(predicates, columnNames, tableName, results);
    resp.queryResults = results;
    MEDIA_INFO_LOG("QueryData Resp : %{public}s", resp.ToString().c_str());
    return IPC::UserDefineIPC().WriteResponseBody(reply, resp, ret);
}

int32_t CloudMediaDataControllerService::UpdateData(MessageParcel &data, MessageParcel &reply)
{
    UpdateDataReqBody req;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, req);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("UpdateData Get Req Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    DataShare::DataSharePredicates predicates = req.predicates;
    std::string tableName = req.tableName;
    DataShare::DataShareValuesBucket value = req.value;
    std::string operateName = req.operateName;
    ret = this->dataService_.UpdateData(tableName, predicates, value, operateName);
    MEDIA_INFO_LOG("UpdateData %{public}d", ret);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}
}  // namespace OHOS::Media::CloudSync