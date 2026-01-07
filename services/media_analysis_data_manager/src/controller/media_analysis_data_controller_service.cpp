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
 
#define MLOG_TAG "Analysis_Data_Controller"
 
#include "media_analysis_data_controller_service.h"
 
#include "media_analysis_data_service.h"
#include "medialibrary_business_code.h"
#include "dfx_timer.h"
#include "media_log.h"
#include "start_asset_analysis_dto.h"
#include "start_asset_analysis_vo.h"
#include "get_asset_analysis_data_vo.h"
#include "get_asset_analysis_data_dto.h"
#include "get_index_construct_progress_vo.h"
#include "change_request_set_order_position_vo.h"
#include "change_request_set_order_position_dto.h"
#include "photo_album.h"
#include "change_request_set_relationship_vo.h"
#include "get_analysis_process_vo.h"
#include "query_result_vo.h"
#include "get_face_id_vo.h"
 
namespace OHOS::Media::AnalysisData {
using namespace std;
bool MediaAnalysisDataControllerService::Accept(uint32_t code)
{
    return HANDLERS.find(code) != HANDLERS.end();
}
 
int32_t MediaAnalysisDataControllerService::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, OHOS::Media::IPC::IPCContext &context)
{
    auto handlersIt = HANDLERS.find(code);
    if (handlersIt != HANDLERS.end()) {
        return (this->*(handlersIt->second))(data, reply);
    }
    return IPC::UserDefineIPC().WriteResponseBody(reply, E_IPC_SEVICE_NOT_FOUND);
}

static inline PhotoAlbumType GetPhotoAlbumType(int32_t albumType)
{
    return static_cast<PhotoAlbumType>(albumType);
}

static inline PhotoAlbumSubType GetPhotoAlbumSubType(int32_t albumSubType)
{
    return static_cast<PhotoAlbumSubType>(albumSubType);
}
 
int32_t MediaAnalysisDataControllerService::GetAssetAnalysisData(MessageParcel &data, MessageParcel &reply)
{
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_ASSET_ANALYSIS_DATA);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    GetAssetAnalysisDataReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetAssetAnalysisData Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
 
    GetAssetAnalysisDataDto dto;
    dto.fileId = reqBody.fileId;
    dto.language = reqBody.language;
    dto.analysisType = reqBody.analysisType;
    dto.analysisTotal = reqBody.analysisTotal;
    ret = MediaAnalysisDataService::GetInstance().GetAssetAnalysisData(dto);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetAssetAnalysisData failed, ret:%{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
 
    GetAssetAnalysisDataRespBody respBody;
    respBody.resultSet = std::move(dto.resultSet);
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}
 
int32_t MediaAnalysisDataControllerService::GetIndexConstructProgress(MessageParcel &data, MessageParcel &reply)
{
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::GET_INDEX_CONSTRUCT_PROGRESS);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    std::string indexProgress;
    auto ret = MediaAnalysisDataService::GetInstance().GetIndexConstructProgress(indexProgress);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("get index construct progress failed, ret: %{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    GetIndexConstructProgressRespBody respBody;
    respBody.indexProgress = indexProgress;
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}
 
int32_t MediaAnalysisDataControllerService::StartAssetAnalysis(MessageParcel &data, MessageParcel &reply)
{
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_START_ASSET_ANALYSIS);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    StartAssetAnalysisReqBody reqBody;
    StartAssetAnalysisRespBody respBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("StartAssetAnalysis Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    StartAssetAnalysisDto dto;
    dto.uri = reqBody.uri;
    dto.predicates = reqBody.predicates;
    ret = MediaAnalysisDataService::GetInstance().StartAssetAnalysis(dto, respBody);
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}

int32_t MediaAnalysisDataControllerService::SetOrderPosition(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter PlaceBefore");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_ORDER_POSITION);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    ChangeRequestSetOrderPositionReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("PlaceBefore Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ChangeRequestSetOrderPositionDto dto;
    dto.FromVo(reqBody);
    ret = MediaAnalysisDataService::GetInstance().SetOrderPosition(dto);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAnalysisDataControllerService::GetOrderPosition(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter GetOrderPosition");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_ORDER_POSITION);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    GetOrderPositionReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetOrderPosition Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    PhotoAlbumType albumType = GetPhotoAlbumType(reqBody.albumType);
    PhotoAlbumSubType albumSubtype = GetPhotoAlbumSubType(reqBody.albumSubType);
    if (!PhotoAlbum::IsAnalysisAlbum(albumType, albumSubtype)) {
        MEDIA_ERR_LOG("Only analysis album can get asset order positions");
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_INVALID_VALUES);
    }

    if (reqBody.assetIdArray.size() <= 0) {
        MEDIA_ERR_LOG("needs at least one asset id");
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_INVALID_VALUES);
    }
    std::set<std::string> idSet(reqBody.assetIdArray.begin(), reqBody.assetIdArray.end());
    if (reqBody.assetIdArray.size() != idSet.size()) {
        MEDIA_ERR_LOG("has same assets");
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_INVALID_VALUES);
    }

    GetOrderPositionRespBody respBody;
    GetOrderPositionDto getOrderPositionDto;
    getOrderPositionDto.albumId = reqBody.albumId;
    getOrderPositionDto.assetIdArray = reqBody.assetIdArray;

    ret = MediaAnalysisDataService::GetInstance().GetOrderPosition(getOrderPositionDto, respBody);
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}

int32_t MediaAnalysisDataControllerService::SetRelationship(MessageParcel &data, MessageParcel &reply)
{
    ChangeRequestSetRelationshipReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("SetRelationship Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    PhotoAlbumType albumType = GetPhotoAlbumType(reqBody.albumType);
    PhotoAlbumSubType albumSubtype = GetPhotoAlbumSubType(reqBody.albumSubType);
    bool cond = PhotoAlbum::IsSmartPortraitPhotoAlbum(albumType, albumSubtype);
    if (!cond) {
        MEDIA_ERR_LOG("params is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_INVALID_VALUES);
    }
    ret = MediaAnalysisDataService::GetInstance().SetPortraitRelationship(
        reqBody.albumId, reqBody.relationship, reqBody.isMe);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAnalysisDataControllerService::GetRelationship(MessageParcel &data, MessageParcel &reply)
{
    GetRelationshipReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetRelationship Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    PhotoAlbumType albumType = GetPhotoAlbumType(reqBody.albumType);
    PhotoAlbumSubType albumSubtype = GetPhotoAlbumSubType(reqBody.albumSubType);
    bool cond = PhotoAlbum::IsSmartPortraitPhotoAlbum(albumType, albumSubtype);
    if (!cond) {
        MEDIA_ERR_LOG("params is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_INVALID_VALUES);
    }

    GetRelationshipRespBody respBody;
    ret = MediaAnalysisDataService::GetInstance().GetPortraitRelationship(reqBody.albumId, respBody);
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}

int32_t MediaAnalysisDataControllerService::GetAnalysisProcess(MessageParcel &data, MessageParcel &reply)
{
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::GET_ANALYSIS_PROCESS);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    GetAnalysisProcessReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetAnalysisProcess Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    QueryResultRespBody respBody;
    ret = MediaAnalysisDataService::GetInstance().GetAnalysisProcess(reqBody, respBody);
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}

int32_t MediaAnalysisDataControllerService::GetFaceId(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter GetFaceId");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::GET_FACE_ID);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    GetFaceIdReqBody reqBody;
    GetFaceIdRespBody respBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetFaceId Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    if (reqBody.albumSubType != PhotoAlbumSubType::PORTRAIT && reqBody.albumSubType != PhotoAlbumSubType::GROUP_PHOTO) {
        MEDIA_WARN_LOG("albumSubType: %{public}d, not support getFaceId", reqBody.albumSubType);
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_INVALID_VALUES);
    }

    string groupTag;
    ret = MediaAnalysisDataService::GetInstance().GetFaceId(reqBody.albumId, groupTag);
    respBody.groupTag = groupTag;
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}
} // namespace OHOS::Media::AnalysisData