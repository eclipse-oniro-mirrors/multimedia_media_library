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

#define MLOG_TAG "MediaAlbumsControllerService"

#include "media_albums_controller_service.h"
#include "media_albums_service.h"

#include "media_log.h"
#include "parameter_utils.h"
#include "delete_albums_vo.h"
#include "create_album_vo.h"
#include "set_subtitle_vo.h"
#include "photo_album.h"
#include "photo_album_column.h"
#include "change_request_set_album_name_vo.h"
#include "change_request_set_cover_uri_vo.h"
#include "change_request_set_album_name_dto.h"
#include "medialibrary_data_manager_utils.h"
#include "change_request_add_assets_vo.h"
#include "change_request_remove_assets_vo.h"
#include "change_request_move_assets_vo.h"
#include "change_request_recover_assets_vo.h"
#include "change_request_delete_assets_vo.h"
#include "album_commit_modify_vo.h"
#include "album_commit_modify_dto.h"
#include "album_add_assets_vo.h"
#include "album_add_assets_dto.h"
#include "album_remove_assets_vo.h"
#include "album_remove_assets_dto.h"
#include "album_recover_assets_vo.h"
#include "album_recover_assets_dto.h"
#include "album_photo_query_vo.h"
#include "query_albums_vo.h"
#include "query_albums_dto.h"
#include "get_order_position_dto.h"
#include "get_order_position_vo.h"
#include "permission_common.h"
#include "get_albums_by_ids_vo.h"
#include "get_photo_album_object_vo.h"
#include "set_photo_album_order_vo.h"
#include "set_photo_album_order_dto.h"
#include "change_request_move_assets_dto.h"
#include "change_request_add_assets_dto.h"
#include "change_request_recover_assets_dto.h"
#include "change_request_delete_assets_dto.h"
#include "change_request_remove_assets_dto.h"
#include "dfx_timer.h"
#include "dfx_const.h"
#include "get_albums_lpath_by_ids_vo.h"
#include "change_request_set_upload_status_vo.h"
#include "change_request_set_upload_status_dto.h"
#include "get_albumid_by_lpath_dto.h"
#include "create_analysis_album_dto.h"
#include "change_request_set_display_level_vo.h"
#include "change_request_dismiss_vo.h"

namespace OHOS::Media {
using namespace std;
using SpecialRequestHandle = int32_t (MediaAlbumsControllerService::*)(
    MessageParcel &, MessageParcel &, OHOS::Media::IPC::IPCContext &);

using RequestHandle = int32_t (MediaAlbumsControllerService::*)(MessageParcel &, MessageParcel &);

const std::map<uint32_t, SpecialRequestHandle> SPECIAL_HANDLERS = {
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::ALBUM_SYS_GET_ASSETS),
        &MediaAlbumsControllerService::AlbumGetAssets
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::ALBUM_GET_ASSETS),
        &MediaAlbumsControllerService::AlbumGetAssets
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_ASSETS),
        &MediaAlbumsControllerService::AlbumGetAssets
    },
};

const std::map<uint32_t, RequestHandle> HANDLERS = {
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_DELETE_PHOTO_ALBUMS),
        &MediaAlbumsControllerService::DeletePhotoAlbums
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_CREATE_ALBUM),
        &MediaAlbumsControllerService::CreatePhotoAlbum
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_ALBUM_NAME),
        &MediaAlbumsControllerService::ChangeRequestSetAlbumName
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_COVER_URI),
        &MediaAlbumsControllerService::ChangeRequestSetCoverUri
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_RESET_COVER_URI),
        &MediaAlbumsControllerService::ChangeRequestResetCoverUri
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_ADD_ASSETS),
        &MediaAlbumsControllerService::AddAssets
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_REMOVE_ASSETS),
        &MediaAlbumsControllerService::RemoveAssets
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_MOVE_ASSETS),
        &MediaAlbumsControllerService::MoveAssets
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_RECOVER_ASSETS),
        &MediaAlbumsControllerService::RecoverAssets
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_DELETE_ASSETS),
        &MediaAlbumsControllerService::DeleteAssets
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_COMMIT_MODIFY),
        &MediaAlbumsControllerService::AlbumCommitModify
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_ADD_ASSETS),
        &MediaAlbumsControllerService::AlbumAddAssets
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_REMOVE_ASSETS),
        &MediaAlbumsControllerService::AlbumRemoveAssets
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_RECOVER_ASSETS),
        &MediaAlbumsControllerService::AlbumRecoverAssets
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SET_COVER_URI),
        &MediaAlbumsControllerService::AlbumCommitModify
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_QUERY_PHOTO_ALBUMS),
        &MediaAlbumsControllerService::QueryAlbums
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_PAH_QUERY_PHOTO_ALBUMS),
        &MediaAlbumsControllerService::QueryAlbumsLpaths
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_QUERY_HIDDEN_ALBUMS),
        &MediaAlbumsControllerService::QueryHiddenAlbums
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_QUERY_GET_ALBUMS_BY_IDS),
        &MediaAlbumsControllerService::GetAlbumsByIds
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_PAH_QUERY_GET_ALBUMS_BY_IDS),
        &MediaAlbumsControllerService::GetAlbumsLpathByIds
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::GET_PHOTO_INDEX),
        &MediaAlbumsControllerService::GetPhotoIndex
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_PHOTO_ALBUMS),
        &MediaAlbumsControllerService::GetPhotoAlbumObject
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_PHOTO_ALBUM_ORDER),
        &MediaAlbumsControllerService::GetPhotoAlbumObject
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SET_PHOTO_ALBUM_ORDER),
        &MediaAlbumsControllerService::UpdatePhotoAlbumOrder
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_HIGHLIGHT_ATTRIBUTE),
        &MediaAlbumsControllerService::ChangeRequestSetHighlightAttribute
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::ALBUM_SYS_GET_SELECTED_ASSETS),
        &MediaAlbumsControllerService::AlbumGetSelectAssets
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::GET_CLONED_ALBUM_URIS),
        &MediaAlbumsControllerService::GetClonedAlbumUris
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_ALBUMS),
        &MediaAlbumsControllerService::QueryAlbums
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_DELETE_ALBUMS),
        &MediaAlbumsControllerService::DeletePhotoAlbums
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CREATE_ALBUM),
        &MediaAlbumsControllerService::CreatePhotoAlbum
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_MOVE_ASSETS),
        &MediaAlbumsControllerService::MoveAssets
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_UPLOAD_STATUS),
        &MediaAlbumsControllerService::ChangeRequestSetUploadStatus
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_ALBUM_BY_LPATH),
        &MediaAlbumsControllerService::GetAlbumIdByLpathOrBundleName
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_ALBUM_BY_BUNDLENAME),
        &MediaAlbumsControllerService::GetAlbumIdByLpathOrBundleName
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SMART_MOVE_ASSETS),
        &MediaAlbumsControllerService::SmartMoveAssets
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_CREATE_ANALYSIS_ALBUM),
        &MediaAlbumsControllerService::CreateAnalysisAlbum
    },
};

bool MediaAlbumsControllerService::Accept(uint32_t code)
{
    return HANDLERS.find(code) != HANDLERS.end() || SPECIAL_HANDLERS.find(code) != SPECIAL_HANDLERS.end();
}

int32_t MediaAlbumsControllerService::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, OHOS::Media::IPC::IPCContext &context)
{
    auto handlersIt = HANDLERS.find(code);
    if (handlersIt != HANDLERS.end()) {
        return (this->*(handlersIt->second))(data, reply);
    }
    auto specialHandlersIt = SPECIAL_HANDLERS.find(code);
    if (specialHandlersIt != SPECIAL_HANDLERS.end()) {
        return (this->*(specialHandlersIt->second))(data, reply, context);
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

int32_t MediaAlbumsControllerService::DeletePhotoAlbums(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter DeletePhotoAlbums");
    DeleteAlbumsReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("DeletePhotoAlbums Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    if (reqBody.albumIds.empty()) {
        MEDIA_ERR_LOG("DeletePhotoAlbums albumIds is empty");
        return IPC::UserDefineIPC().WriteResponseBody(reply, -EINVAL);
    }
    ret = MediaAlbumsService::GetInstance().DeletePhotoAlbums(reqBody.albumIds);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAlbumsControllerService::CreatePhotoAlbum(MessageParcel &data, MessageParcel &reply)
{
    CreateAlbumReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CreatePhotoAlbum Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = ParameterUtils::CheckCreatePhotoAlbum(reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CheckCreatePhotoAlbum ret:%{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = MediaAlbumsService::GetInstance().CreatePhotoAlbum(reqBody.albumName);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAlbumsControllerService::ChangeRequestSetAlbumName(MessageParcel &data, MessageParcel &reply)
{
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_ALBUM_NAME);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    ChangeRequestSetAlbumNameReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("ChangeRequestSetAlbumName Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    PhotoAlbumType albumType = GetPhotoAlbumType(reqBody.albumType);
    PhotoAlbumSubType albumSubtype = GetPhotoAlbumSubType(reqBody.albumSubType);
    bool cond = (MediaFileUtils::CheckAlbumName(reqBody.albumName) == E_OK);
    cond = (PhotoAlbum::IsUserPhotoAlbum(albumType, albumSubtype) ||
        PhotoAlbum::IsSmartPortraitPhotoAlbum(albumType, albumSubtype) ||
        PhotoAlbum::IsSmartGroupPhotoAlbum(albumType, albumSubtype) ||
        PhotoAlbum::IsHighlightAlbum(albumType, albumSubtype) ||
        PhotoAlbum::IsSourceAlbum(albumType, albumSubtype) ||
        PhotoAlbum::IsPetAlbum(albumType, albumSubtype)) && cond;
    cond = cond && !reqBody.albumName.empty() && !reqBody.albumId.empty() &&
        MediaLibraryDataManagerUtils::IsNumber(reqBody.albumId);
    if (!cond) {
        MEDIA_ERR_LOG("params is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_INVALID_VALUES);
    }
    ChangeRequestSetAlbumNameDto dto;
    dto.albumSubtype = GetPhotoAlbumSubType(reqBody.albumSubType);
    dto.albumName = reqBody.albumName;
    dto.albumId = reqBody.albumId;
    ret = MediaAlbumsService::GetInstance().ChangeRequestSetAlbumName(dto);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAlbumsControllerService::ChangeRequestSetCoverUri(MessageParcel &data, MessageParcel &reply)
{
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_COVER_URI);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    ChangeRequestSetCoverUriReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("ChangeRequestSetCoverUri Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    PhotoAlbumType albumType = GetPhotoAlbumType(reqBody.albumType);
    PhotoAlbumSubType albumSubtype = GetPhotoAlbumSubType(reqBody.albumSubType);
    bool cond = PhotoAlbum::IsUserPhotoAlbum(albumType, albumSubtype) ||
        PhotoAlbum::IsSmartPortraitPhotoAlbum(albumType, albumSubtype) ||
        PhotoAlbum::IsSmartGroupPhotoAlbum(albumType, albumSubtype) ||
        PhotoAlbum::IsHighlightAlbum(albumType, albumSubtype) ||
        PhotoAlbum::IsPetAlbum(albumType, albumSubtype);
    cond = cond && !reqBody.coverUri.empty() && !reqBody.albumId.empty() &&
        MediaLibraryDataManagerUtils::IsNumber(reqBody.albumId);
    if (!cond) {
        MEDIA_ERR_LOG("params is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_INVALID_VALUES);
    }
    ChangeRequestSetCoverUriDto dto;
    dto.albumSubtype = GetPhotoAlbumSubType(reqBody.albumSubType);
    dto.coverUri = reqBody.coverUri;
    dto.albumId = reqBody.albumId;
    ret = MediaAlbumsService::GetInstance().ChangeRequestSetCoverUri(dto);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAlbumsControllerService::ChangeRequestResetCoverUri(MessageParcel &data, MessageParcel &reply)
{
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_RESET_COVER_URI);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    ChangeRequesDismissReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("ChangeRequestResetCoverUri Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    PhotoAlbumType albumType = GetPhotoAlbumType(reqBody.albumType);
    PhotoAlbumSubType albumSubtype = GetPhotoAlbumSubType(reqBody.albumSubType);
    int32_t albumId = atoi(reqBody.albumId.c_str());
    bool cond = PhotoAlbum::IsUserPhotoAlbum(albumType, albumSubtype) ||
        (PhotoAlbum::IsSystemAlbum(albumType) && !PhotoAlbum::IsHiddenAlbum(albumType, albumSubtype)) ||
        PhotoAlbum::IsSourceAlbum(albumType, albumSubtype);
    cond = cond && !reqBody.albumId.empty() && MediaLibraryDataManagerUtils::IsNumber(reqBody.albumId);
    if (!cond) {
        MEDIA_ERR_LOG("params is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_INVALID_VALUES);
    }

    ret = MediaAlbumsService::GetInstance().ChangeRequestResetCoverUri(albumId, albumSubtype);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAlbumsControllerService::AddAssets(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter AddAssets");
    ChangeRequestAddAssetsReqBody reqBody;
    ChangeRequestAddAssetsRespBody respBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("AddAssets Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ChangeRequestAddAssetsDto dto;
    dto.FromVo(reqBody);

    ret = MediaAlbumsService::GetInstance().AddAssets(dto, respBody);
    return  IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}

int32_t MediaAlbumsControllerService::RemoveAssets(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter RemoveAssets");
    ChangeRequestRemoveAssetsReqBody reqBody;
    ChangeRequestRemoveAssetsRespBody respBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("RemoveAssets Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ChangeRequestRemoveAssetsDto dto;
    dto.FromVo(reqBody);
    ret = MediaAlbumsService::GetInstance().RemoveAssets(dto, respBody);
    return  IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}

int32_t MediaAlbumsControllerService::MoveAssets(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter MoveAssets");
    ChangeRequestMoveAssetsReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("MoveAssets Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ChangeRequestMoveAssetsDto dto;
    dto.FromVo(reqBody);
    ret = MediaAlbumsService::GetInstance().MoveAssets(dto);
    ChangeRequestMoveAssetsRespBody respBody;
    respBody.albumCount = dto.albumCount;
    respBody.albumImageCount = dto.albumImageCount;
    respBody.albumVideoCount = dto.albumVideoCount;
    respBody.targetAlbumCount = dto.targetAlbumCount;
    respBody.targetAlbumImageCount = dto.targetAlbumImageCount;
    respBody.targetAlbumVideoCount = dto.targetAlbumVideoCount;
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}

int32_t MediaAlbumsControllerService::SmartMoveAssets(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter SmartMoveAssets");
    ChangeRequestMoveAssetsReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("SmartMoveAssets Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ChangeRequestMoveAssetsDto dto;
    dto.FromVo(reqBody);
    ret = MediaAlbumsService::GetInstance().MoveAssets(dto);
    ChangeRequestMoveAssetsRespBody respBody;
    respBody.albumCount = dto.albumCount;
    respBody.albumImageCount = dto.albumImageCount;
    respBody.albumVideoCount = dto.albumVideoCount;
    respBody.targetAlbumCount = dto.targetAlbumCount;
    respBody.targetAlbumImageCount = dto.targetAlbumImageCount;
    respBody.targetAlbumVideoCount = dto.targetAlbumVideoCount;
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}

int32_t MediaAlbumsControllerService::RecoverAssets(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter RecoverAssets");
    ChangeRequestRecoverAssetsReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("RecoverAssets Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ChangeRequestRecoverAssetsDto dto;
    dto.assets = reqBody.assets;
    ret = MediaAlbumsService::GetInstance().RecoverAssets(dto);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAlbumsControllerService::DeleteAssets(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter DeleteAssets");
    ChangeRequestDeleteAssetsReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("DeleteAssets Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ChangeRequestDeleteAssetsDto dto;
    dto.assets = reqBody.assets;
    ret = MediaAlbumsService::GetInstance().DeleteAssets(dto);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAlbumsControllerService::AlbumCommitModify(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter CommitModify");
    AlbumCommitModifyReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CommitModify Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    if (reqBody.businessCode == static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_COMMIT_MODIFY)) {
        PhotoAlbumType albumType = GetPhotoAlbumType(reqBody.albumType);
        PhotoAlbumSubType albumSubtype = GetPhotoAlbumSubType(reqBody.albumSubType);

        bool cond = PhotoAlbum::IsUserPhotoAlbum(albumType, albumSubtype) &&
            MediaFileUtils::CheckAlbumName(reqBody.albumName) == E_OK;
        if (!cond) {
            MEDIA_ERR_LOG("params is invalid");
            return IPC::UserDefineIPC().WriteResponseBody(reply, E_INVALID_VALUES);
        }
    }

    if (reqBody.businessCode == static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SET_COVER_URI) &&
        reqBody.coverUri.empty()) {
        MEDIA_ERR_LOG("params is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_INVALID_VALUES);
    }

    AlbumCommitModifyDto commitModifyDto;
    commitModifyDto.albumName = reqBody.albumName;
    commitModifyDto.coverUri = reqBody.coverUri;
    commitModifyDto.albumId = reqBody.albumId;

    ret = MediaAlbumsService::GetInstance().AlbumCommitModify(commitModifyDto, reqBody.businessCode);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAlbumsControllerService::AlbumAddAssets(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter AddAssets");
    AlbumAddAssetsReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("AddAssets Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    PhotoAlbumType albumType = GetPhotoAlbumType(reqBody.albumType);
    PhotoAlbumSubType albumSubtype = GetPhotoAlbumSubType(reqBody.albumSubType);
    if (!PhotoAlbum::IsUserPhotoAlbum(albumType, albumSubtype)) {
        MEDIA_ERR_LOG("params is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_INVALID_VALUES);
    }

    if (reqBody.assetsArray.empty()) {
        MEDIA_ERR_LOG("params is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_INVALID_VALUES);
    }

    AlbumPhotoQueryRespBody respBody;
    AlbumAddAssetsDto addAssetsDto;
    addAssetsDto.albumId = reqBody.albumId;
    addAssetsDto.isSmartAlbum = reqBody.albumType == PhotoAlbumType::SMART;
    addAssetsDto.assetsArray = reqBody.assetsArray;

    ret = MediaAlbumsService::GetInstance().AlbumAddAssets(addAssetsDto, respBody);
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}

int32_t MediaAlbumsControllerService::AlbumRemoveAssets(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter RemoveAssets");
    AlbumRemoveAssetsReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("AddAssets Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    PhotoAlbumType albumType = GetPhotoAlbumType(reqBody.albumType);
    PhotoAlbumSubType albumSubtype = GetPhotoAlbumSubType(reqBody.albumSubType);
    if (!PhotoAlbum::IsUserPhotoAlbum(albumType, albumSubtype)) {
        MEDIA_ERR_LOG("params is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_INVALID_VALUES);
    }

    if (reqBody.assetsArray.empty()) {
        MEDIA_ERR_LOG("params is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_INVALID_VALUES);
    }

    AlbumRemoveAssetsDto removeAssetsDto;
    removeAssetsDto.albumId = reqBody.albumId;
    removeAssetsDto.isSmartAlbum = albumType == PhotoAlbumType::SMART;
    removeAssetsDto.assetsArray = reqBody.assetsArray;
    AlbumPhotoQueryRespBody respBody;

    ret = MediaAlbumsService::GetInstance().AlbumRemoveAssets(removeAssetsDto, respBody);
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}

int32_t MediaAlbumsControllerService::AlbumRecoverAssets(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter RecoverAssets");
    AlbumRecoverAssetsReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("AddAssets Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    PhotoAlbumType albumType = GetPhotoAlbumType(reqBody.albumType);
    PhotoAlbumSubType albumSubtype = GetPhotoAlbumSubType(reqBody.albumSubType);
    if (!PhotoAlbum::IsTrashAlbum(albumType, albumSubtype)) {
        MEDIA_ERR_LOG("params is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_INVALID_VALUES);
    }

    if (reqBody.uris.empty()) {
        MEDIA_ERR_LOG("params is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_INVALID_VALUES);
    }

    AlbumRecoverAssetsDto recoverAssetsDto;
    recoverAssetsDto.uris = reqBody.uris;
    ret = MediaAlbumsService::GetInstance().AlbumRecoverAssets(recoverAssetsDto);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAlbumsControllerService::QueryAlbums(MessageParcel &data, MessageParcel &reply)
{
    QueryAlbumsReqBody reqBody;
    QueryAlbumsRespBody respBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CreateAssetForApp Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
    }

    QueryAlbumsDto dto;
    dto.albumType = reqBody.albumType;
    dto.albumSubType = reqBody.albumSubType;
    dto.columns = reqBody.columns;
    dto.predicates = reqBody.predicates;
    ret = MediaAlbumsService::GetInstance().QueryAlbums(dto);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("QueryAlbums failed, ret:%{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
    }

    respBody.resultSet = dto.resultSet;
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}

int32_t MediaAlbumsControllerService::QueryHiddenAlbums(MessageParcel &data, MessageParcel &reply)
{
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_QUERY_HIDDEN_ALBUMS);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    QueryAlbumsReqBody reqBody;
    QueryAlbumsRespBody respBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CreateAssetForApp Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
    }

    QueryAlbumsDto dto;
    dto.columns = reqBody.columns;
    dto.predicates = reqBody.predicates;
    dto.hiddenAlbumFetchMode = reqBody.hiddenAlbumFetchMode;
    ret = MediaAlbumsService::GetInstance().QueryHiddenAlbums(dto);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("QueryHiddenAlbums failed, ret:%{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
    }

    respBody.resultSet = dto.resultSet;
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}

int32_t MediaAlbumsControllerService::GetAlbumsByIds(MessageParcel &data, MessageParcel &reply)
{
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_QUERY_GET_ALBUMS_BY_IDS);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    GetAlbumsByIdsReqBody reqBody;
    GetAlbumsByIdsRespBody respBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetAlbumsByIds Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    GetAlbumsByIdsDto dto;
    dto.FromVo(reqBody);
    ret = MediaAlbumsService::GetInstance().GetAlbumsByIds(dto, respBody);
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}

int32_t MediaAlbumsControllerService::GetClonedAlbumUris(MessageParcel &data, MessageParcel &reply)
{
    GetClonedAlbumUrisReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = ParameterUtils::CheckWhereClause(reqBody.predicates.GetWhereClause());
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CheckWhereClause fialed");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    GetClonedAlbumUrisDto dto = GetClonedAlbumUrisDto::Create(reqBody);
    auto resultSet = MediaAlbumsService::GetInstance().GetClonedAlbumUris(dto);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("resultSet is null");
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_FAIL);
    }
    GetClonedAlbumUrisRespBody respBody;
    respBody.resultSet = resultSet;
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}

int32_t MediaAlbumsControllerService::GetPhotoIndex(MessageParcel &data, MessageParcel &reply)
{
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::GET_PHOTO_INDEX);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    GetPhotoIndexReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetPhotoIndex Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    QueryResultRespBody respBody;
    ret = MediaAlbumsService::GetInstance().GetPhotoIndex(reqBody, respBody);
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}

int32_t MediaAlbumsControllerService::AlbumGetAssets(
    MessageParcel &data, MessageParcel &reply, OHOS::Media::IPC::IPCContext &context)
{
    MEDIA_INFO_LOG("enter");
    AlbumGetAssetsReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = ParameterUtils::CheckWhereClause(reqBody.predicates.GetWhereClause());
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CheckWhereClause fialed");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    AlbumGetAssetsDto dto = AlbumGetAssetsDto::Create(reqBody);
    if (context.GetByPassCode() == E_PERMISSION_DB_BYPASS) {
        string clientAppId = GetClientAppId();
        if (clientAppId.empty()) {
            MEDIA_ERR_LOG("clientAppId is empty");
            return IPC::UserDefineIPC().WriteResponseBody(reply, Media::E_PERMISSION_DENIED);
        }
        dto.predicates.And()->EqualTo("owner_appid", clientAppId);
    }

    auto resultSet = MediaAlbumsService::GetInstance().AlbumGetAssets(dto);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("resultSet is null");
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_FAIL);
    }
    AlbumGetAssetsRespBody respBody;
    respBody.resultSet = resultSet;
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}

int32_t MediaAlbumsControllerService::GetPhotoAlbumObject(MessageParcel &data, MessageParcel &reply)
{
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_PHOTO_ALBUMS);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    GetPhotoAlbumObjectReqBody reqBody;
    GetPhotoAlbumObjectRespBody respBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetPhotoAlbumObject Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    GetPhotoAlbumObjectDto dto;
    dto.FromVo(reqBody);
    ret = MediaAlbumsService::GetInstance().GetPhotoAlbumObject(dto, respBody);
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}

int32_t MediaAlbumsControllerService::UpdatePhotoAlbumOrder(MessageParcel &data, MessageParcel &reply)
{
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SET_PHOTO_ALBUM_ORDER);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    SetPhotoAlbumOrderReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("UpdatePhotoAlbumOrder Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    SetPhotoAlbumOrderDto setPhotoAlbumOrderDto;
    setPhotoAlbumOrderDto.albumOrderColumn = reqBody.albumOrderColumn;
    setPhotoAlbumOrderDto.orderSectionColumn = reqBody.orderSectionColumn;
    setPhotoAlbumOrderDto.orderTypeColumn = reqBody.orderTypeColumn;
    setPhotoAlbumOrderDto.orderStatusColumn = reqBody.orderStatusColumn;
    setPhotoAlbumOrderDto.albumIds = reqBody.albumIds;
    setPhotoAlbumOrderDto.albumOrders = reqBody.albumOrders;
    setPhotoAlbumOrderDto.orderSection = reqBody.orderSection;
    setPhotoAlbumOrderDto.orderType = reqBody.orderType;
    setPhotoAlbumOrderDto.orderStatus = reqBody.orderStatus;
    MEDIA_INFO_LOG("Update photo album order include: %{public}s", setPhotoAlbumOrderDto.ToString().c_str());

    ret = MediaAlbumsService::GetInstance().UpdatePhotoAlbumOrder(setPhotoAlbumOrderDto);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAlbumsControllerService::QueryAlbumsLpaths(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_DEBUG_LOG("MediaAlbumsControllerService::QueryAlbumsLpaths Start");
    QueryAlbumsReqBody reqBody;
    QueryAlbumsRespBody respBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("QueryAlbumsLpaths Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
    }

    QueryAlbumsDto dto;
    dto.albumType = reqBody.albumType;
    dto.albumSubType = reqBody.albumSubType;
    dto.columns = reqBody.columns;
    dto.predicates = reqBody.predicates;
    ret = MediaAlbumsService::GetInstance().QueryAlbumsLpaths(dto);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("QueryAlbumsLpaths failed, ret: %{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
    }
    respBody.resultSet = dto.resultSet;
    MEDIA_DEBUG_LOG("MediaAlbumsControllerService::QueryAlbumsLpaths End");
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}

int32_t MediaAlbumsControllerService::GetAlbumsLpathByIds(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_DEBUG_LOG("MediaAlbumsControllerService::GetAlbumsLpathByIds Start");
    GetAlbumsLpathByIdsReqBody reqBody;
    GetAlbumsLpathByIdsRespBody respBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetAlbumsLpathByIds Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, std::to_string(reqBody.albumId))
        ->And()
        ->BeginWrap()
        ->EqualTo(PhotoAlbumColumns::ALBUM_TYPE, std::to_string(PhotoAlbumType::USER))
        ->Or()
        ->EqualTo(PhotoAlbumColumns::ALBUM_TYPE, std::to_string(PhotoAlbumType::SOURCE))
        ->EndWrap();
    
    QueryAlbumsDto dto;
    dto.columns = { PhotoAlbumColumns::ALBUM_LPATH };
    dto.predicates = predicates;
    ret = MediaAlbumsService::GetInstance().QueryAlbumsLpath(dto);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetAlbumsLpathByIds failed, ret: %{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
    }

    CHECK_AND_RETURN_RET_LOG(dto.resultSet != nullptr && dto.resultSet->GoToFirstRow() == NativeRdb::E_OK, E_FAIL,
        "GetAlbumsLpathByIds query failed");
    CHECK_AND_RETURN_RET_LOG(dto.resultSet->GetString(0, respBody.lpath) == NativeRdb::E_OK, E_FAIL,
        "GetAlbumsLpathByIds cannot get lpath");
    
    MEDIA_DEBUG_LOG("MediaAlbumsControllerService::GetAlbumsLpathByIds End. lpath: %{public}s",
        respBody.lpath.c_str());
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}

int32_t MediaAlbumsControllerService::ChangeRequestSetHighlightAttribute(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("MediaAlbumsControllerService::ChangeRequestSetHighlightAttribute start");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_HIGHLIGHT_ATTRIBUTE);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    ChangeRequestSetHighlightAttributeReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("ChangeRequestSetHighlightAttribute Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    PhotoAlbumType albumType = GetPhotoAlbumType(reqBody.albumType);
    PhotoAlbumSubType albumSubType = GetPhotoAlbumSubType(reqBody.albumSubType);
    bool cond = PhotoAlbum::IsHighlightAlbum(albumType, albumSubType);
    cond = cond && (reqBody.albumId != -1) && (!reqBody.highlightAlbumChangeAttributeValue.empty());
    if (!cond) {
        MEDIA_ERR_LOG("params is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_PARAM_CONVERT_FORMAT);
    }
    ChangeRequestSetHighlightAttributeDto dto;
    dto.albumId = reqBody.albumId;
    dto.highlightAlbumChangeAttribute = reqBody.highlightAlbumChangeAttribute;
    dto.highlightAlbumChangeAttributeValue = reqBody.highlightAlbumChangeAttributeValue;
    dto.albumType = reqBody.albumType;
    dto.albumSubType = reqBody.albumSubType;
    ret = MediaAlbumsService::GetInstance().ChangeRequestSetHighlightAttribute(dto);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAlbumsControllerService::AlbumGetSelectAssets(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter AlbumGetSelectAssets");
    AlbumGetSelectedAssetsReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
 
    ret = ParameterUtils::CheckWhereClause(reqBody.predicates.GetWhereClause());
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CheckWhereClause fialed");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    
    AlbumGetSelectedAssetsDto dto = AlbumGetSelectedAssetsDto::Create(reqBody);
    auto resultSet = MediaAlbumsService::GetInstance().AlbumGetSelectedAssets(dto);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("resultSet is null");
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_FAIL);
    }
    
    AlbumGetSelectedAssetsRespBody respBody;
    respBody.resultSet = resultSet;
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}

int32_t MediaAlbumsControllerService::ChangeRequestSetUploadStatus(MessageParcel &data, MessageParcel &reply)
{
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_UPLOAD_STATUS);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    ChangeRequestSetUploadStatusReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("ChangeRequestSetUploadStatusReqBody Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
 
    ChangeRequestSetUploadStatusDto dto;
    dto.FromVo(reqBody);
    ret = MediaAlbumsService::GetInstance().ChangeRequestSetUploadStatus(dto);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAlbumsControllerService::GetAlbumIdByLpathOrBundleName(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("GetAlbumIdByLpathOrBundleName start");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_ALBUM_BY_LPATH);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    GetAlbumIdByLpathReqBody reqBody;
    GetAlbumIdByLpathRespBody respBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetAlbumIdByLpathOrBundleName Read Request Error: %{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    GetAlbumIdByLpathDto dto;
    dto.FromVo(reqBody);
    ret = MediaAlbumsService::GetInstance().GetAlbumIdByLpathOrBundleName(dto, respBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetAlbumIdByLpathOrBundleName Read Request Error: %{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}

int32_t MediaAlbumsControllerService::CreateAnalysisAlbum(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("CreateAnalysisAlbum start");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_CREATE_ANALYSIS_ALBUM);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    CreateAnalysisAlbumReqBody reqBody;
    CreateAnalysisAlbumRespBody respBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CreateAnalysisAlbum Read Request Error: %{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    CHECK_AND_RETURN_RET_LOG(reqBody.subType == static_cast<int32_t>(PORTRAIT), E_INNER_FAIL, "subtype failed");
    CreateAnalysisAlbumDto dto;
    dto.FromVo(reqBody);
    ret = MediaAlbumsService::GetInstance().CreateAnalysisAlbum(dto, respBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CreateAnalysisAlbum Error: %{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}
} // namespace OHOS::Media