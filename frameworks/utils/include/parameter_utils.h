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

#ifndef MEDIALIBRARY_PARAMETER_UTILS_H
#define MEDIALIBRARY_PARAMETER_UTILS_H

#include <string>

#include "create_asset_vo.h"
#include "create_album_vo.h"
#include "modify_assets_vo.h"
#include "delete_highlight_albums_vo.h"
#include "asset_change_vo.h"
#include "restore_vo.h"
#include "cancel_request_vo.h"

namespace OHOS {
namespace Media {
class ParameterUtils {
public:
    static int32_t CheckFormIds(const std::vector<std::string>& formIds);
    static bool CheckHighlightAlbum(const DeleteHighLightAlbumsReqBody& reqBody,
        std::vector<std::string>& albumIds);
    static bool CheckEditDataLength(const std::string& editData);
    static bool CheckOpenUriLength(const std::string& fileUri);

    static int32_t CheckPublicCreateAsset(const CreateAssetReqBody &reqBody);
    static int32_t CheckSystemCreateAsset(const CreateAssetReqBody &reqBody);
    static int32_t CheckPublicCreateAssetForApp(const CreateAssetForAppReqBody &reqBody);
    static int32_t CheckSystemCreateAssetForApp(const CreateAssetForAppReqBody &reqBody);
    static int32_t CheckCreateAssetForAppWithAlbum(const CreateAssetForAppReqBody &reqBody);
    static int32_t CheckCreatePhotoAlbum(const CreateAlbumReqBody &reqBody);
    static int32_t CheckSetAssetTitle(const ModifyAssetsReqBody &reqBody);
    static int32_t CheckSetAssetPending(const ModifyAssetsReqBody &reqBody);
    static int32_t CheckSetAssetsFavorite(const ModifyAssetsReqBody &reqBody);
    static int32_t CheckSetAssetsHiddenStatus(const ModifyAssetsReqBody &reqBody);
    static int32_t CheckSetAssetsRecentShowStatus(const ModifyAssetsReqBody &reqBody);
    static int32_t CheckSetAssetsUserComment(const ModifyAssetsReqBody &reqBody);
    static int32_t CheckAddAssetVisitCount(int32_t fileId, int32_t visitType);

    static int32_t CheckCreateAssetSubtype(int32_t photoSubtype);
    static int32_t CheckCreateAssetTitle(const std::string &title, bool isSystem = false);
    static int32_t CheckCreateAssetMediaType(int32_t mediaType, const std::string &extension);
    static int32_t CheckCreateAssetCameraShotKey(int32_t photoSubtype, const std::string &cameraShotKey);
    static int32_t GetTitleAndExtension(const std::string &displayName, std::string &title, std::string &ext);
    static int32_t CheckTrashPhotos(const std::vector<std::string> &uris);
    static int32_t CheckDeletePhotosCompleted(const std::vector<std::string> &fileIds);
    static bool IsPhotoUri(const std::string& uri);
    static int32_t CheckUserComment(const AssetChangeReqBody &reqBody);
    static int32_t CheckCameraShotKey(const AssetChangeReqBody &reqBody);
    static int32_t CheckOrientation(const AssetChangeReqBody &reqBody);
    static int32_t CheckVideoEnhancementAttr(const AssetChangeReqBody &reqBody);
    static int32_t CheckWatermarkType(const AssetChangeReqBody &reqBody);
    static int32_t CheckCompositeDisplayMode(const AssetChangeReqBody &reqBody);
    static int32_t CheckWhereClause(const std::string &whereClause);
    static bool CheckPhotoUri(const std::string &uri);
    static int32_t CheckRestore(const RestoreReqBody &reqBody);
    static int32_t CheckCancelRequest(const CancelRequestReqBody &reqBody);
};
}  // namespace Media
}  // namespace OHOS
#endif // MEDIALIBRARY_PARAMETER_UTILS_H
