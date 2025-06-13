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
    EXPORT void DeleteHighlightAlbums(MessageParcel &data, MessageParcel &reply);
    EXPORT void DeletePhotoAlbums(MessageParcel &data, MessageParcel &reply);
    EXPORT void CreatePhotoAlbum(MessageParcel &data, MessageParcel &reply);
    void SetSubtitle(MessageParcel &data, MessageParcel &reply);
    void SetHighlightUserActionData(MessageParcel &data, MessageParcel &reply);
    void ChangeRequestSetAlbumName(MessageParcel &data, MessageParcel &reply);
    void ChangeRequestSetCoverUri(MessageParcel &data, MessageParcel &reply);
    void ChangeRequestSetIsMe(MessageParcel &data, MessageParcel &reply);
    void ChangeRequestSetDisplayLevel(MessageParcel &data, MessageParcel &reply);
    void ChangeRequestDismiss(MessageParcel &data, MessageParcel &reply);
    EXPORT void AddAssets(MessageParcel &data, MessageParcel &reply);
    EXPORT void RemoveAssets(MessageParcel &data, MessageParcel &reply);
    EXPORT void MoveAssets(MessageParcel &data, MessageParcel &reply);
    EXPORT void RecoverAssets(MessageParcel &data, MessageParcel &reply);
    EXPORT void DeleteAssets(MessageParcel &data, MessageParcel &reply);
    EXPORT void DismissAssets(MessageParcel &data, MessageParcel &reply);
    EXPORT void MergeAlbum(MessageParcel &data, MessageParcel &reply);
    EXPORT void PlaceBefore(MessageParcel &data, MessageParcel &reply);
    EXPORT void SetOrderPosition(MessageParcel &data, MessageParcel &reply);
    EXPORT void AlbumCommitModify(MessageParcel &data, MessageParcel &reply);
    EXPORT void AlbumAddAssets(MessageParcel &data, MessageParcel &reply);
    EXPORT void AlbumRemoveAssets(MessageParcel &data, MessageParcel &reply);
    EXPORT void AlbumRecoverAssets(MessageParcel &data, MessageParcel &reply);
    EXPORT void AlbumGetAssets(MessageParcel &data, MessageParcel &reply, OHOS::Media::IPC::IPCContext &context);
    EXPORT void QueryAlbums(MessageParcel &data, MessageParcel &reply);
    EXPORT void QueryHiddenAlbums(MessageParcel &data, MessageParcel &reply);
    EXPORT void GetAlbumsByIds(MessageParcel &data, MessageParcel &reply);
    EXPORT void GetOrderPosition(MessageParcel &data, MessageParcel &reply);
    EXPORT void GetFaceId(MessageParcel &data, MessageParcel &reply);
    EXPORT void GetPhotoIndex(MessageParcel &data, MessageParcel &reply);
    EXPORT void GetAnalysisProcess(MessageParcel &data, MessageParcel &reply);
    EXPORT void GetHighlightAlbumInfo(MessageParcel &data, MessageParcel &reply);
public:
    virtual ~MediaAlbumsControllerService() = default;
    bool Accept(uint32_t code) override;
    void OnRemoteRequest(
        uint32_t code, MessageParcel &data, MessageParcel &reply, OHOS::Media::IPC::IPCContext &context) override;
    int32_t GetPermissionPolicy(
        uint32_t code, std::vector<std::vector<PermissionType>> &permissionPolicy, bool &isBypass) override;
};
} // namespace OHOS::Media
#endif  // OHOS_MEDIA_ALBUMS_CONTROLLER_SERVICE_H