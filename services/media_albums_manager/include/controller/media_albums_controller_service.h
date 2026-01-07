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

#ifndef OHOS_MEDIA_ALBUMS_CONTROLLER_SERVICE_H
#define OHOS_MEDIA_ALBUMS_CONTROLLER_SERVICE_H

#include "message_parcel.h"
#include "medialibrary_business_code.h"
#include "i_media_controller_service.h"
#include "user_define_ipc.h"
#include "medialibrary_errno.h"

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))
class EXPORT MediaAlbumsControllerService : public IPC::IMediaControllerService {
public:
    EXPORT int32_t DeleteHighlightAlbums(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t DeletePhotoAlbums(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t CreatePhotoAlbum(MessageParcel &data, MessageParcel &reply);
    int32_t SetSubtitle(MessageParcel &data, MessageParcel &reply);
    int32_t SetHighlightUserActionData(MessageParcel &data, MessageParcel &reply);
    int32_t ChangeRequestSetAlbumName(MessageParcel &data, MessageParcel &reply);
    int32_t ChangeRequestSetCoverUri(MessageParcel &data, MessageParcel &reply);
    int32_t ChangeRequestSetIsMe(MessageParcel &data, MessageParcel &reply);
    int32_t ChangeRequestSetDisplayLevel(MessageParcel &data, MessageParcel &reply);
    int32_t ChangeRequestDismiss(MessageParcel &data, MessageParcel &reply);
    int32_t ChangeRequestResetCoverUri(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t AddAssets(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t RemoveAssets(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t MoveAssets(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t RecoverAssets(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t DeleteAssets(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t DismissAssets(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t MergeAlbum(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t PlaceBefore(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t AlbumCommitModify(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t AlbumAddAssets(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t AlbumRemoveAssets(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t AlbumRecoverAssets(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t AlbumGetAssets(MessageParcel &data, MessageParcel &reply, OHOS::Media::IPC::IPCContext &context);
    EXPORT int32_t AlbumGetSelectAssets(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t QueryAlbums(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t QueryHiddenAlbums(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t GetAlbumsByIds(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t GetPhotoIndex(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t GetHighlightAlbumInfo(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t GetPhotoAlbumObject(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t UpdatePhotoAlbumOrder(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t QueryAlbumsLpaths(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t GetAlbumsLpathByIds(MessageParcel &data, MessageParcel &reply);
    int32_t ChangeRequestSetHighlightAttribute(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t GetClonedAlbumUris(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t ChangeRequestSetUploadStatus(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t GetAlbumIdByLpathOrBundleName(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t SmartMoveAssets(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t CreateAnalysisAlbum(MessageParcel &data, MessageParcel &reply);

public:
    virtual ~MediaAlbumsControllerService() = default;
    bool Accept(uint32_t code) override;
    int32_t OnRemoteRequest(
        uint32_t code, MessageParcel &data, MessageParcel &reply, OHOS::Media::IPC::IPCContext &context) override;
    int32_t GetPermissionPolicy(
        uint32_t code, std::vector<std::vector<PermissionType>> &permissionPolicy, bool &isBypass) override;
};
} // namespace OHOS::Media
#endif  // OHOS_MEDIA_ALBUMS_CONTROLLER_SERVICE_H