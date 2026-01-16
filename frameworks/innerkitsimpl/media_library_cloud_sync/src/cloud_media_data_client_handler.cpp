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

#include "cloud_media_data_client_handler.h"

#include <string>
#include <vector>

#include "cloud_data_convert_to_vo.h"
#include "cloud_download_file_meta.h"
#include "cloud_media_operation_code.h"
#include "cloud_sync_unprepared_data_vo.h"
#include "media_itypes_utils.h"
#include "media_log.h"
#include "message_parcel.h"
#include "user_define_ipc_client.h"
#include "media_req_vo.h"
#include "media_resp_vo.h"
#include "medialibrary_errno.h"
#include "update_dirty_vo.h"
#include "update_position_vo.h"
#include "update_sync_status_vo.h"
#include "update_thm_status_vo.h"
#include "get_aging_file_vo.h"
#include "get_download_asset_vo.h"
#include "get_download_thm_vo.h"
#include "get_video_to_cache_vo.h"
#include "get_file_pos_stat_vo.h"
#include "get_cloud_thm_stat_vo.h"
#include "get_dirty_type_stat_vo.h"
#include "on_download_asset_vo.h"
#include "get_download_thm_num_vo.h"
#include "update_local_file_dirty_vo.h"
#include "get_download_thm_by_uri_vo.h"
#include "media_file_utils.h"
#include "media_operate_result_vo.h"
#include "query_data_vo.h"
#include "update_data_vo.h"
namespace OHOS::Media::CloudSync {
// LCOV_EXCL_START
void CloudMediaDataClientHandler::SetUserId(const int32_t &userId)
{
    this->userId_ = userId;
}

void CloudMediaDataClientHandler::SetTraceId(const std::string &traceId)
{
    this->traceId_ = traceId;
}

std::string CloudMediaDataClientHandler::GetTraceId() const
{
    return this->traceId_;
}

void CloudMediaDataClientHandler::SetCloudType(const int32_t cloudType)
{
    this->cloudType_ = cloudType;
}

int32_t CloudMediaDataClientHandler::UpdateDirty(const std::string &cloudId, DirtyTypes dirtyType)
{
    MEDIA_INFO_LOG("CloudMediaDataClientHandler::UpdateDirty begin");
    // request info.
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaOperationCode::CMD_UPDATE_DIRTY_FOR_CLOUD_CHECK);
    UpdateDirtyReqBody reqBody;
    reqBody.cloudId = cloudId;
    reqBody.dirtyType = static_cast<int32_t>(dirtyType);
    int32_t ret =
        IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
            .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}}).Post(operationCode, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to UpdateDirty, ret: %{public}d", ret);
    }
    return ret;
}

int32_t CloudMediaDataClientHandler::UpdatePosition(const std::vector<std::string> &cloudIds, int32_t position)
{
    MEDIA_INFO_LOG("UpdatePosition, cloudIds: %{public}zu, position: %{public}d", cloudIds.size(), position);
    CHECK_AND_RETURN_RET_LOG(!cloudIds.empty(), E_OK, "UpdatePosition: cloudIds is empty");
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaOperationCode::CMD_UPDATE_POSITION_FOR_CLOUD_CHECK);
    UpdatePositionReqBody reqBody;
    reqBody.cloudIds = cloudIds;
    reqBody.position = position;
    int32_t ret =
        IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
            .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}}).Post(operationCode, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to UpdatePosition, ret: %{public}d", ret);
    }
    return ret;
}

int32_t CloudMediaDataClientHandler::UpdatePosWithType(const std::vector<std::string> &cloudIds,
    int32_t position, int32_t fileSourceType)
{
    MEDIA_INFO_LOG("UpdatePosWithType, cloudIds: %{public}zu, position: %{public}d, fileSourceType: %{public}d",
        cloudIds.size(), position, fileSourceType);
    CHECK_AND_RETURN_RET_LOG(!cloudIds.empty(), E_OK, "UpdatePosWithType: cloudIds is empty");
    uint32_t operationCode = static_cast<uint32_t>(
        CloudMediaOperationCode::CMD_UPDATE_FILE_SOURCE_TYPE);
    UpdatePositionReqBody reqBody;
    reqBody.cloudIds = cloudIds;
    reqBody.position = position;
    reqBody.fileSourceType = fileSourceType;
    int32_t ret =
        IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_).Post(operationCode, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to UpdatePosWithType, ret: %{public}d", ret);
    }
    return ret;
}

int32_t CloudMediaDataClientHandler::UpdateFileSourceType(const std::vector<std::string> &cloudIds,
    int32_t fileSourceType)
{
    MEDIA_INFO_LOG("UpdateFileSourceType, cloudIds: %{public}zu, fileSourceType: %{public}d",
        cloudIds.size(), fileSourceType);
    CHECK_AND_RETURN_RET_LOG(!cloudIds.empty(), E_OK, "UpdateFileSourceType: cloudIds is empty");
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaOperationCode::CMD_UPDATE_FILE_SOURCE_TYPE);
    UpdatePositionReqBody reqBody;
    reqBody.cloudIds = cloudIds;
    reqBody.fileSourceType = fileSourceType;
    int32_t ret =
        IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_).Post(operationCode, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to UpdateFileSourceType, ret: %{public}d", ret);
    }
    return ret;
}

int32_t CloudMediaDataClientHandler::UpdateSyncStatus(const std::string &cloudId, int32_t syncStatus)
{
    MEDIA_INFO_LOG(
        "CloudMediaDataClientHandler::UpdateSyncStatus begin %{public}s, %{public}d", cloudId.c_str(), syncStatus);
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaOperationCode::CMD_UPDATE_SYNC_STATUS);
    UpdateSyncStatusReqBody reqBody;
    reqBody.cloudId = cloudId;
    reqBody.syncStatus = syncStatus;
    int32_t ret =
        IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
            .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}}).Post(operationCode, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to UpdateSyncStatus, ret: %{public}d", ret);
    }
    return ret;
}

int32_t CloudMediaDataClientHandler::UpdateThmStatus(const std::string &cloudId, int32_t thmStatus)
{
    MEDIA_INFO_LOG("CloudMediaDataClientHandler::UpdateThmStatus begin");
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaOperationCode::CMD_UPDATE_THM_STATUS_FOR_CLOUD_CHECK);
    UpdateThmStatusReqBody reqBody;
    reqBody.cloudId = cloudId;
    reqBody.thmStatus = thmStatus;
    int32_t ret = IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
            .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}}).Post(operationCode, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to UpdateThmStatus, ret: %{public}d", ret);
    }
    return ret;
}

int32_t CloudMediaDataClientHandler::GetAgingFile(
    const int64_t time, int32_t mediaType, int32_t sizeLimit, int32_t offset, std::vector<CloudMetaData> &metaData)
{
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_AGING_ASSET);
    GetAgingFileReqBody reqBody;
    reqBody.time = time;
    reqBody.mediaType = mediaType;
    reqBody.sizeLimit = sizeLimit;
    reqBody.offset = offset;
    int32_t ret = this->GetAgingFile(operationCode, reqBody, metaData);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to GetAgingFile, ret: %{public}d", ret);
    }
    return ret;
}

int32_t CloudMediaDataClientHandler::GetAgingFile(
    uint32_t operationCode, GetAgingFileReqBody &reqBody, std::vector<CloudMetaData> &metaData)
{
    MEDIA_INFO_LOG("CloudMediaDataClientHandler::GetAgingFile begin, operationCode: %{public}d", operationCode);
    GetAgingFileRespBody respBody;
    int32_t ret =
        IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
            .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}}).Post(operationCode, reqBody, respBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to GetAgingFile, ret: %{public}d", ret);
        return ret;
    }
    for (auto &photosVo : respBody.photos) {
        CloudMetaData cloudMetaData = CloudDataConvertToVo::ConvertPhotosVoToCloudMetaData(photosVo);
        metaData.push_back(cloudMetaData);
    }
    MEDIA_INFO_LOG("CloudMediaDataClientHandler::GetAgingFile end, operationCode: %{public}d", operationCode);
    return E_OK;
}

int32_t CloudMediaDataClientHandler::GetActiveAgingFile(
    const int64_t time, int32_t mediaType, int32_t sizeLimit, int32_t offset, std::vector<CloudMetaData> &metaData)
{
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_ACTIVE_AGING_ASSET);
    GetAgingFileReqBody reqBody;
    reqBody.time = time;
    reqBody.mediaType = mediaType;
    reqBody.sizeLimit = sizeLimit;
    reqBody.offset = offset;
    int32_t ret = this->GetAgingFile(operationCode, reqBody, metaData);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to GetActiveAgingFile, ret: %{public}d", ret);
    }
    return ret;
}

int32_t CloudMediaDataClientHandler::GetDownloadAsset(
    const std::vector<std::string> &uris, std::vector<CloudMetaData> &cloudMetaDataVec)
{
    MEDIA_INFO_LOG("GetDownloadAsset, uris: %{public}zu", uris.size());
    CHECK_AND_RETURN_RET_LOG(!uris.empty(), E_OK, "GetDownloadAsset: uris is empty");
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_DOWNLOAD_ASSET);
    GetDownloadAssetReqBody reqBody;
    reqBody.pathList = uris;
    // parcel data.
    GetDownloadAssetRespBody respBody;
    int32_t ret =
        IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
            .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}}).Post(operationCode, reqBody, respBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to GetDownloadAsset, ret: %{public}d", ret);
        return ret;
    }
    for (auto &photosVo : respBody.photos) {
        CloudMetaData cloudMetaData = CloudDataConvertToVo::ConvertPhotosVoToCloudMetaData(photosVo);
        MEDIA_INFO_LOG("GetDownloadAsset MetaData: %{public}s", cloudMetaData.ToString().c_str());
        cloudMetaDataVec.push_back(cloudMetaData);
    }
    MEDIA_INFO_LOG("CloudMediaDataClientHandler::GetDownloadAsset end");
    return E_OK;
}

int32_t CloudMediaDataClientHandler::GetDownloadLakeAsset(
    const std::vector<std::string> &uris, std::vector<CloudDlFileMeta> &cloudMetaDataVec)
{
    MEDIA_INFO_LOG("GetDownloadLakeAsset, uris: %{public}zu", uris.size());
    CHECK_AND_RETURN_RET_LOG(!uris.empty(), E_OK, "GetDownloadAsset: uris is empty");
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_DOWNLOAD_ASSET);
    GetDownloadAssetReqBody reqBody;
    reqBody.pathList = uris;
    // parcel data.
    GetDownloadAssetRespBody respBody;
    int32_t ret =
        IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_).Post(operationCode, reqBody, respBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to GetDownloadAsset, ret: %{public}d", ret);
        return ret;
    }
    for (auto &photosVo : respBody.photos) {
        CloudDlFileMeta cloudMetaData = CloudDataConvertToVo::ConvertPhotosVoToLakeCloudMetaData(photosVo);
        MEDIA_INFO_LOG("GetDownloadAsset MetaData: %{public}s", cloudMetaData.ToString().c_str());
        cloudMetaDataVec.push_back(cloudMetaData);
    }
    MEDIA_INFO_LOG("CloudMediaDataClientHandler::GetDownloadAsset end");
    return E_OK;
}

int32_t CloudMediaDataClientHandler::GetDownloadThmsByUri(
    const std::vector<std::string> &uri, int32_t type, std::vector<CloudMetaData> &metaData)
{
    MEDIA_INFO_LOG("GetDownloadThmsByUri, uris: %{public}zu, type: %{public}d", uri.size(), type);
    CHECK_AND_RETURN_RET_LOG(!uri.empty(), E_OK, "GetDownloadThmsByUri: uris is empty");
    GetDownloadThmsByUriReqBody reqBody;
    reqBody.pathList = uri;
    reqBody.thmType = type;
    GetDownloadThmsByUriRespBody respBody;
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_DOWNLOAD_THM_BY_URI);
    int32_t ret =
        IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
            .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}}).Post(operationCode, reqBody, respBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to GetDownloadThmsByUri, ret: %{public}d", ret);
        return ret;
    }
    for (auto &photosVo : respBody.photos) {
        CloudMetaData cloudMetaData = CloudDataConvertToVo::ConvertPhotosVoToCloudMetaData(photosVo);
        MEDIA_INFO_LOG("GetDownloadThmsByUri MetaData: %{public}s", cloudMetaData.ToString().c_str());
        metaData.push_back(cloudMetaData);
    }
    return ret;
}

int32_t CloudMediaDataClientHandler::OnDownloadAsset(
    const std::vector<std::string> &cloudIds, std::vector<MediaOperateResult> &result)
{
    MEDIA_INFO_LOG("OnDownloadAsset, cloudIds: %{public}zu", cloudIds.size());
    CHECK_AND_RETURN_RET_LOG(!cloudIds.empty(), E_OK, "OnDownloadAsset: cloudIds is empty");
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaOperationCode::CMD_ON_DOWNLOAD_ASSET);
    OnDownloadAssetReqBody reqBody;
    reqBody.cloudIds = cloudIds;
    MediaOperateResultRespBody respBody;
    int32_t ret =
        IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
            .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}}).Post(operationCode, reqBody, respBody);
    result.clear();
    for (auto &resultVo : respBody.result) {
        MEDIA_INFO_LOG(
            "CloudMediaDataClientHandler::OnDownloadAsset, mediaResult: %{public}s", resultVo.ToString().c_str());
        MediaOperateResult mediaResult;
        mediaResult.cloudId = resultVo.cloudId;
        mediaResult.errorCode = resultVo.errorCode;
        mediaResult.errorMsg = resultVo.errorMsg;
        result.emplace_back(mediaResult);
    }
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to OnDownloadAsset, ret: %{public}d", ret);
    }
    return ret;
}

int32_t CloudMediaDataClientHandler::OnDownloadLakeAsset(
    const std::unordered_map<std::string, AdditionFileInfo> &lakeInfos,
    std::vector<MediaOperateResult> &result)
{
    MEDIA_INFO_LOG("OnDownloadLakeAsset, cloudIds: %{public}zu", lakeInfos.size());
    CHECK_AND_RETURN_RET_LOG(!lakeInfos.empty(), E_OK, "OnDownloadAsset: lakeInfos is empty");
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaOperationCode::CMD_ON_DOWNLOAD_LAKE_ASSET);
    OnDownloadAssetReqBody reqBody;
    reqBody.lakeInfos = lakeInfos;
    MediaOperateResultRespBody respBody;
    int32_t ret =
        IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_).Post(operationCode, reqBody, respBody);
    result.clear();
    for (auto &resultVo : respBody.result) {
        MEDIA_INFO_LOG(
            "CloudMediaDataClientHandler::OnDownloadLakeAsset, mediaResult: %{public}s", resultVo.ToString().c_str());
        MediaOperateResult mediaResult;
        mediaResult.cloudId = resultVo.cloudId;
        mediaResult.errorCode = resultVo.errorCode;
        mediaResult.errorMsg = resultVo.errorMsg;
        result.emplace_back(mediaResult);
    }
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to OnDownloadLakeAsset, ret: %{public}d", ret);
    }
    return ret;
}

int32_t CloudMediaDataClientHandler::GetDownloadThms(
    std::vector<CloudMetaData> &cloudMetaDataVec, const DownloadThumPara &param)
{
    MEDIA_INFO_LOG("enter GetDownloadThms");
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_DOWNLOAD_THM);
    GetDownloadThmReqBody reqBody;
    reqBody.size = param.size;
    reqBody.type = param.type;
    reqBody.offset = param.offset;
    reqBody.isDownloadDisplayFirst = param.isDownloadDisplayFirst;
    GetDownloadThmRespBody respBody;
    int32_t ret =
        IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
            .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}}).Post(operationCode, reqBody, respBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetDownloadThms IPC Err, ret: %{public}d", ret);
        return ret;
    }
    for (auto &photosVo : respBody.photos) {
        MEDIA_INFO_LOG("GetDownloadThm %{public}s.", photosVo.ToString().c_str());
        CloudMetaData cloudMetaData = CloudDataConvertToVo::ConvertPhotosVoToCloudMetaData(photosVo);
        cloudMetaDataVec.push_back(cloudMetaData);
        MEDIA_INFO_LOG("GetDownloadThms MetaData: %{public}s", cloudMetaData.ToString().c_str());
    }
    MEDIA_INFO_LOG("GetDownloadThms end");
    return E_OK;
}

int32_t CloudMediaDataClientHandler::OnDownloadThmsInner(
    std::vector<OnDownloadThmsReqBody::DownloadThmsData> &downloadThmsDataList, int32_t &failSize)
{
    MEDIA_INFO_LOG("enter CloudMediaDataClientHandler::OnDownloadThmsInner");
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaOperationCode::CMD_ON_DOWNLOAD_THMS);
    OnDownloadThmsReqBody reqBody;
    reqBody.downloadThmsDataList = downloadThmsDataList;
    MEDIA_INFO_LOG("OnDownloadThmsReqBody: %{public}zu", reqBody.downloadThmsDataList.size());
    MediaOperateResultRespBody respBody;
    int32_t ret =
        IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
            .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}}).Post(operationCode, reqBody, respBody);
    failSize = respBody.GetFailSize();
    CHECK_AND_PRINT_LOG(ret == E_OK, "Failed to OnDownloadThms, ret: %{public}d", ret);
    return ret;
}

int32_t CloudMediaDataClientHandler::OnDownloadThms(
    const std::unordered_map<std::string, int32_t> &resMap, int32_t &failSize)
{
    MEDIA_INFO_LOG("OnDownloadThms, resMap: %{public}zu", resMap.size());
    CHECK_AND_RETURN_RET_LOG(!resMap.empty(), E_OK, "OnDownloadThms: resMap is empty");
    std::vector<OnDownloadThmsReqBody::DownloadThmsData> downloadThmsDataList;
    for (const auto &res : resMap) {
        OnDownloadThmsReqBody::DownloadThmsData downloadThmsData;
        downloadThmsData.cloudId = res.first;
        downloadThmsData.thumbStatus = res.second;
        downloadThmsDataList.emplace_back(downloadThmsData);
    }
    std::vector<std::vector<OnDownloadThmsReqBody::DownloadThmsData>> splitedDataList;
    this->processor_.SplitVector(
        downloadThmsDataList, static_cast<size_t>(this->MAX_DOWNLOAD_THMS_SIZE), splitedDataList);
    MEDIA_INFO_LOG("OnDownloadThms, total size: %{public}zu, split size: %{public}zu",
        downloadThmsDataList.size(),
        splitedDataList.size());
    int32_t ret = E_OK;
    int32_t subFailSize = 0;
    for (auto &dataSubList : splitedDataList) {
        ret = this->OnDownloadThmsInner(dataSubList, subFailSize);
        failSize += subFailSize;
        CHECK_AND_BREAK_ERR_LOG(ret == E_OK, "OnDownloadThmsInner failed, ret: %{public}d", ret);
    }
    return ret;
}

int32_t CloudMediaDataClientHandler::GetVideoToCache(std::vector<CloudMetaData> &cloudMetaDataVec, int32_t size)
{
    MEDIA_INFO_LOG("CloudMediaDataClientHandler::GetVideoToCache begin");
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_VIDEO_TO_CACHE);
    GetVideoToCacheRespBody respBody;
    int32_t ret =
        IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
            .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}}).Get(operationCode, respBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to GetAgingFile, ret: %{public}d", ret);
        return ret;
    }
    for (auto &photosVo : respBody.photos) {
        CloudMetaData cloudMetaData = CloudDataConvertToVo::ConvertPhotosVoToCloudMetaData(photosVo);
        cloudMetaDataVec.push_back(cloudMetaData);
    }
    MEDIA_INFO_LOG("CloudMediaDataClientHandler::GetVideoToCache end");
    return E_OK;
}

int32_t CloudMediaDataClientHandler::GetFilePosStat(std::vector<uint64_t> &filePosStat)
{
    MEDIA_INFO_LOG("CloudMediaDataClientHandler::GetFilePosStat begin");
    if (filePosStat.size() != SIZE_FILE_POSITION_LEN) {
        MEDIA_ERR_LOG("GetFilePosStat file position stat size is wrong with %{public}zu", filePosStat.size());
        return E_DATA;
    }
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_FILE_POS_STAT);
    GetFilePosStatRespBody respBody;
    int32_t ret =
        IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
            .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}}).Get(operationCode, respBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetFilePosStat Failed to GetAgingFile, ret: %{public}d", ret);
        return ret;
    }
    filePosStat = respBody.statList;
    return E_OK;
}

int32_t CloudMediaDataClientHandler::GetCloudThmStat(std::vector<uint64_t> &cloudThmStat)
{
    MEDIA_INFO_LOG("CloudMediaDataClientHandler::GetCloudThmStat begin");
    if (cloudThmStat.size() != SIZE_CLOUD_THM_STAT_LEN) {
        MEDIA_ERR_LOG("cloud thm stat size is wrong with %{public}zu", cloudThmStat.size());
        return E_DATA;
    }
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_CLOUD_THM_STAT);
    GetCloudThmStatRespBody respBody;
    int32_t ret =
        IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
            .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}}).Get(operationCode, respBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to GetCloudThmStat, ret: %{public}d", ret);
        return ret;
    }
    cloudThmStat = respBody.statList;
    return E_OK;
}

int32_t CloudMediaDataClientHandler::GetDirtyTypeStat(std::vector<uint64_t> &dirtyTypeStat)
{
    MEDIA_INFO_LOG("CloudMediaDataClientHandler::GetDirtyTypeStat begin");
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_DIRTY_TYPE_STAT);
    GetDirtyTypeStatRespBody respBody;
    int32_t ret = IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
            .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}}).Get(operationCode, respBody);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to GetDirtyTypeStat, ret: %{public}d", ret);
    dirtyTypeStat = respBody.statList;
    return E_OK;
}

int32_t CloudMediaDataClientHandler::GetDownloadThmNum(int32_t &totalNum, int32_t type)
{
    MEDIA_INFO_LOG("CloudMediaDataClientHandler::GetDownloadThmNum begin");
    GetDownloadThmNumReqBody req;
    req.type = type;
    GetDownloadThmNumRespBody respBody;
    respBody.totalNum = 0;
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_DOWNLOAD_THM_NUM);
    int32_t ret =
        IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
            .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}}).Post(operationCode, req, respBody);
    totalNum = respBody.totalNum;
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to GetDownloadThmNum, ret: %{public}d", ret);
    }
    return ret;
}

int32_t CloudMediaDataClientHandler::UpdateLocalFileDirty(std::vector<MDKRecord> &records)
{
    MEDIA_INFO_LOG("CloudMediaDataClientHandler::UpdateLocalFileDirty begin");
    std::vector<std::string> cloudIds;
    for (auto &record : records) {
        if (!record.GetRecordId().empty()) {
            MEDIA_INFO_LOG("UpdateLocalFileDirty CloudId: %{public}s", record.GetRecordId().c_str());
            cloudIds.emplace_back(record.GetRecordId());
        }
    }
    CHECK_AND_RETURN_RET_LOG(!cloudIds.empty(), E_OK, "UpdateLocalFileDirty: cloudIds is empty");
    UpdateLocalFileDirtyReqBody req;
    req.cloudIds = cloudIds;
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaOperationCode::CMD_UPDATE_LOCAL_FILE_DIRTY);
    int32_t ret = IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
            .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}}).Post(operationCode, req);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to UpdateLocalFileDirty, ret: %{public}d", ret);
    }
    return ret;
}

int32_t CloudMediaDataClientHandler::GetCloudSyncUnPreparedData(int32_t &result)
{
    MEDIA_INFO_LOG("CloudMediaDataClientHandler::GetCloudSyncUnPreparedData begin");
    // request info.
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_CLOUD_SYNC_UNPREPARED_DATA);
    CloudSyncUnPreparedDataRespBody respBody;
    respBody.count = 0;
    int32_t ret = IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
            .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}}).Get(operationCode, respBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to GetCloudSyncUnPreparedData, ret: %{public}d", ret);
    }
    result = respBody.count;
    return ret;
}

int32_t CloudMediaDataClientHandler::SubmitCloudSyncPreparedDataTask()
{
    MEDIA_INFO_LOG("CloudMediaDataClientHandler::SubmitCloudSyncPreparedDataTask begin");
    // request info.
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaOperationCode::CMD_SUBMIT_CLOUD_SYNC_UNPREPARED_DATA_TASK);
    int32_t ret = IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
            .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}}).Post(operationCode);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to SubmitCloudSyncPreparedDataTask, ret: %{public}d", ret);
    }
    return ret;
}

int32_t CloudMediaDataClientHandler::CheckAndFixAlbum()
{
    MEDIA_INFO_LOG("CloudMediaDataClientHandler::CheckAndFixAlbum begin");
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaOperationCode::CMD_CHECK_AND_FIX_ALBUM);
    int32_t ret = IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
            .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}}).Post(operationCode);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to CheckAndFixAlbum, ret: %{public}d", ret);
    }
    return ret;
}

int32_t CloudMediaDataClientHandler::QueryData(const DataShare::DataSharePredicates &predicates,
                                               const std::vector<std::string> &columnNames,
                                               const std::string &tableName,
                                               std::vector<std::unordered_map<std::string, std::string>> &results)
{
    MEDIA_INFO_LOG("enter CloudMediaDataClientHandler::QueryData");
    QueryDataReqBody reqBody;
    QueryDataRespBody respBody;
    reqBody.predicates = predicates;
    reqBody.columnNames = columnNames;
    reqBody.tableName = tableName;
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaOperationCode::CMD_QUERY_DATA);
    int32_t ret = IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
            .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}}).Post(operationCode, reqBody, respBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to QueryData, ret: %{public}d", ret);
    } else {
        results = respBody.queryResults;
        MEDIA_INFO_LOG("QueryData Client result: %{public}s", respBody.ToString().c_str());
    }
    return ret;
}

int32_t CloudMediaDataClientHandler::QueryData(const std::string &tableName,
                                               const DataShare::DataSharePredicates &predicates,
                                               const DataShare::DataShareValuesBucket &value,
                                               const std::string &operateName)
{
    MEDIA_INFO_LOG("enter CloudMediaDataClientHandler::UpdateData");
    UpdateDataReqBody reqBody;
    reqBody.tableName = tableName;
    reqBody.predicates = predicates;
    reqBody.value = value;
    reqBody.operateName = operateName;
    int32_t ret = 0;
    uint32_t operationCode = static_cast<uint32_t>(CloudMediaOperationCode::CMD_UPDATE_DATA);
    int32_t ret = IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(this->traceId_)
            .SetHeader({{PhotoColumn::CLOUD_TYPE, to_string(cloudType_)}}).Post(operationCode, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to UpdateData, ret: %{public}d", ret);
    }
    return ret;
}
// LCOV_EXCL_STOP
}  // namespace OHOS::Media::CloudSync