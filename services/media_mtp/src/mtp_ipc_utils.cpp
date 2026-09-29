/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
#define MLOG_TAG "MtpIpcUtils"

#include "mtp_ipc_utils.h"
#include "user_inner_ipc_client.h"
#include "medialibrary_business_code.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"

#include "album_get_assets_vo.h"
#include "asset_change_vo.h"
#include "change_request_move_assets_vo.h"
#include "change_request_set_album_name_vo.h"
#include "create_album_vo.h"
#include "create_asset_vo.h"
#include "close_asset_vo.h"
#include "delete_albums_vo.h"
#include "delete_photos_completed_vo.h"
#include "query_albums_vo.h"

namespace OHOS {
namespace Media {

std::shared_ptr<DataShare::DataShareResultSet> MtpIpcUtils::GetAssets(ConstHelper &dataShareHelper,
    const DataShare::DataSharePredicates &predicates, const std::vector<std::string> &fetchColumns)
{
    MEDIA_DEBUG_LOG("GetAssets start");
    CHECK_AND_RETURN_RET_LOG(dataShareHelper != nullptr, nullptr, "dataShareHelper is null");

    AlbumGetAssetsReqBody req;
    req.predicates = predicates;
    req.columns = fetchColumns;
    AlbumGetAssetsRespBody resp;

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_ASSETS);
    int32_t ret = IPC::UserInnerIPCClient().SetDataShareHelper(dataShareHelper).Call(businessCode, req, resp);
    CHECK_AND_RETURN_RET_LOG(ret >= E_OK, nullptr, "after IPC Call, ret: %{public}d.", ret);
    MEDIA_DEBUG_LOG("GetAssets end");
    return resp.resultSet;
}

std::shared_ptr<DataShare::DataShareResultSet> MtpIpcUtils::GetAlbums(ConstHelper &dataShareHelper,
    const DataShare::DataSharePredicates &predicates, const std::vector<std::string> &fetchColumns)
{
    MEDIA_DEBUG_LOG("GetAlbums start");
    CHECK_AND_RETURN_RET_LOG(dataShareHelper != nullptr, nullptr, "dataShareHelper is null");

    QueryAlbumsReqBody req;
    QueryAlbumsRespBody resp;
    req.albumType = PhotoAlbumType::INVALID;
    req.albumSubType = PhotoAlbumSubType::ANY;
    req.columns = fetchColumns;
    req.predicates = predicates;

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_ALBUMS);
    int32_t ret = IPC::UserInnerIPCClient().SetDataShareHelper(dataShareHelper).Call(businessCode, req, resp);
    CHECK_AND_RETURN_RET_LOG(ret >= E_OK, nullptr, "after IPC Call, ret: %{public}d.", ret);
    MEDIA_DEBUG_LOG("GetAlbums end");
    return resp.resultSet;
}

int32_t MtpIpcUtils::CreateAsset(ConstHelper &dataShareHelper,
    const std::string &displayName, MediaType mediaType, int32_t &assetId)
{
    MEDIA_INFO_LOG("CreateAsset start, displayName: %{public}s", displayName.c_str());
    CHECK_AND_RETURN_RET_LOG(dataShareHelper != nullptr, E_ERR, "dataShareHelper is null");
    CHECK_AND_RETURN_RET_LOG(!displayName.empty(), E_ERR, "displayName is empty");

    CreateAssetReqBody req;
    req.mediaType = static_cast<int32_t>(mediaType);
    req.photoSubtype = static_cast<int32_t>(PhotoSubType::DEFAULT);
    req.displayName = displayName;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CREATE_ASSET);
    CreateAssetRespBody resp;
    int32_t ret = IPC::UserInnerIPCClient().SetDataShareHelper(dataShareHelper).Call(businessCode, req, resp);
    CHECK_AND_RETURN_RET_LOG(ret >= E_OK, ret, "after IPC Call, ret: %{public}d.", ret);
    assetId = resp.fileId;
    MEDIA_INFO_LOG("CreateAsset end, assetId: %{public}d", assetId);
    return ret;
}

int32_t MtpIpcUtils::CreateFileManagerAsset(ConstHelper &dataShareHelper,
    const std::string &displayName, uint32_t ownerAlbumId, int32_t &assetId)
{
    MEDIA_INFO_LOG("CreateFileManagerAsset start, displayName: %{public}s", displayName.c_str());
    CHECK_AND_RETURN_RET_LOG(dataShareHelper != nullptr, E_ERR, "dataShareHelper is null");
    CHECK_AND_RETURN_RET_LOG(!displayName.empty(), E_ERR, "displayName is empty");
    CHECK_AND_RETURN_RET_LOG(ownerAlbumId > 0, E_ERR, "ownerAlbumId is invalid");

    CreateFileMgrAssetReqBody req;
    req.displayName = displayName;
    req.ownerAlbumId = std::to_string(ownerAlbumId);

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CREATE_FILE_MANAGER_ASSET);
    CreateAssetRespBody resp;
    int32_t ret = IPC::UserInnerIPCClient().SetDataShareHelper(dataShareHelper).Call(businessCode, req, resp);
    CHECK_AND_RETURN_RET_LOG(ret >= E_OK, ret, "after IPC Call, ret: %{public}d.", ret);
    assetId = resp.fileId;
    MEDIA_INFO_LOG("CreateAsset end, assetId: %{public}d", assetId);
    return ret;
}

int32_t MtpIpcUtils::ChangeAssetTitle(ConstHelper &dataShareHelper, int32_t assetId, const std::string &title)
{
    MEDIA_INFO_LOG("ChangeAssetTitle start, assetId: %{public}d, title: %{public}s", assetId, title.c_str());
    CHECK_AND_RETURN_RET_LOG(dataShareHelper != nullptr, E_ERR, "dataShareHelper is null");
    CHECK_AND_RETURN_RET_LOG(!title.empty(), E_ERR, "title is empty");
    CHECK_AND_RETURN_RET_LOG(assetId > 0, E_ERR, "assetId is not valid");

    AssetChangeReqBody req;
    req.fileId = assetId;
    req.title = title;

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_SET_TITLE);
    int32_t ret = IPC::UserInnerIPCClient().SetDataShareHelper(dataShareHelper).Call(businessCode, req);
    CHECK_AND_RETURN_RET_LOG(ret >= E_OK, ret, "after IPC Call, ret: %{public}d.", ret);
    MEDIA_INFO_LOG("ChangeAssetTitle end");
    return MTP_SUCCESS;
}

int32_t MtpIpcUtils::CreateAlbum(ConstHelper &dataShareHelper, const std::string &albumName, int32_t &albumId)
{
    MEDIA_INFO_LOG("CreateAlbum start, albumName: %{public}s", albumName.c_str());
    CHECK_AND_RETURN_RET_LOG(dataShareHelper != nullptr, E_ERR, "dataShareHelper is null");
    CHECK_AND_RETURN_RET_LOG(!albumName.empty(), E_ERR, "albumName is empty");

    CreateAlbumReqBody req;
    req.albumName = albumName;

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CREATE_ALBUM);
    int32_t ret = IPC::UserInnerIPCClient().SetDataShareHelper(dataShareHelper).Call(businessCode, req);
    CHECK_AND_RETURN_RET_LOG(ret >= E_OK, ret, "after IPC Call, ret: %{public}d.", ret);
    albumId = ret;
    MEDIA_INFO_LOG("CreateAlbum end, albumId: %{public}d", albumId);
    return MTP_SUCCESS;
}

int32_t MtpIpcUtils::ChangeAlbumName(ConstHelper &dataShareHelper, const std::string &albumId,
    const std::string &albumName, int32_t albumType, int32_t albumSubType)
{
    MEDIA_INFO_LOG("ChangeAlbumName start, albumId: %{public}s, albumName: %{public}s, albumSubType: %{public}d",
        albumId.c_str(), albumName.c_str(), albumSubType);
    CHECK_AND_RETURN_RET_LOG(dataShareHelper != nullptr, E_ERR, "dataShareHelper is null");
    CHECK_AND_RETURN_RET_LOG(!albumId.empty(), E_ERR, "albumId is empty");
    CHECK_AND_RETURN_RET_LOG(!albumName.empty(), E_ERR, "albumName is empty");

    ChangeRequestSetAlbumNameReqBody req;
    req.albumId = albumId;
    req.albumName = albumName;
    req.albumType = albumType;
    req.albumSubType = albumSubType;

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_ALBUM_NAME);
    int32_t ret = IPC::UserInnerIPCClient().SetDataShareHelper(dataShareHelper).Call(businessCode, req);
    CHECK_AND_RETURN_RET_LOG(ret >= E_OK, ret, "after IPC Call, ret: %{public}d.", ret);
    MEDIA_INFO_LOG("ChangeAlbumName end");
    return MTP_SUCCESS;
}

int32_t MtpIpcUtils::MoveAsset(ConstHelper &dataShareHelper, uint32_t assetId, int32_t srcAlbumId, int32_t targAlbumId)
{
    MEDIA_INFO_LOG("MtpIpcUtils::MoveAsset start, assetId: %{public}u", assetId);
    CHECK_AND_RETURN_RET_LOG(dataShareHelper != nullptr, E_ERR, "dataShareHelper is null");

    ChangeRequestMoveAssetsReqBody req;
    ChangeRequestMoveAssetsRespBody resp;
    req.assets.push_back(std::to_string(assetId));
    req.albumId = srcAlbumId;
    req.targetAlbumId = targAlbumId;
    // CHANGE_REQUEST_MOVE_ASSETS
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_MOVE_ASSETS);
    int32_t errCode = IPC::UserInnerIPCClient().SetDataShareHelper(dataShareHelper).Call(businessCode, req, resp);
    CHECK_AND_RETURN_RET_LOG(errCode >= E_OK, errCode, "after IPC Call, errCode: %{public}d.", errCode);
    MEDIA_INFO_LOG("MoveAsset end, albumCount: %{public}d", resp.albumCount);
    return MTP_SUCCESS;
}

int32_t MtpIpcUtils::DeletePhotos(ConstHelper &dataShareHelper, const std::vector<std::string> &photoIds)
{
    MEDIA_INFO_LOG("MtpIpcUtils::DeletePhotos start, photoIds size: %{public}zu", photoIds.size());
    CHECK_AND_RETURN_RET_LOG(!photoIds.empty(), MTP_SUCCESS, "photoIds is empty, no need delete");
    CHECK_AND_RETURN_RET_LOG(dataShareHelper != nullptr, E_ERR, "dataShareHelper is null");

    DeletePhotosCompletedReqBody req;
    req.fileIds = photoIds;

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::DELETE_ASSETS_PERMANENTLY_WITH_URI);
    int32_t errCode = IPC::UserInnerIPCClient().SetDataShareHelper(dataShareHelper).Call(businessCode, req);
    CHECK_AND_RETURN_RET_LOG(errCode >= E_OK, errCode, "after IPC Call, errCode: %{public}d.", errCode);
    MEDIA_INFO_LOG("DeletePhotos end");
    return MTP_SUCCESS;
}

int32_t MtpIpcUtils::DeleteAlbums(ConstHelper &dataShareHelper, const std::vector<std::string> &deleteAlbumIds)
{
    MEDIA_INFO_LOG("MtpIpcUtils::DeleteAlbums start, albumIds size: %{public}zu", deleteAlbumIds.size());
    CHECK_AND_RETURN_RET_LOG(dataShareHelper != nullptr, E_ERR, "dataShareHelper is null");
    CHECK_AND_RETURN_RET_LOG(!deleteAlbumIds.empty(), E_ERR, "albumIds is empty");

    DeleteAlbumsReqBody req;
    req.albumIds = deleteAlbumIds;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_DELETE_ALBUMS);
    int32_t errCode = IPC::UserInnerIPCClient().SetDataShareHelper(dataShareHelper).Call(businessCode, req);
    CHECK_AND_RETURN_RET_LOG(errCode >= E_OK, errCode, "after IPC Call, errCode: %{public}d.", errCode);
    MEDIA_INFO_LOG("MtpIpcUtils::DeleteAlbums end");
    return MTP_SUCCESS;
}
} // namespace Media
} // namespace OHOS
