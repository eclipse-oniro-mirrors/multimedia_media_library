/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "PortraitFriendIdHandler"

#include "portrait_friend_id_handler.h"

#include "medialibrary_album_operations.h"
#include "medialibrary_errno.h"
#include "media_log.h"

namespace OHOS::Media {
const AnalysisAlbumAttributeSpec &PortraitFriendIdHandler::GetSpec()
{
    return ANALYSIS_ALBUM_FRIEND_ID_SPEC;
}

int32_t PortraitFriendIdHandler::ValidateTarget(const std::shared_ptr<PhotoAlbum> &photoAlbum)
{
    CHECK_AND_RETURN_RET_LOG(photoAlbum != nullptr, E_INVALID_VALUES,
        "photoAlbum is nullptr");
    CHECK_AND_RETURN_RET_LOG(IsPortraitAlbumAttributeTarget(photoAlbum), E_PARAM_CONVERT_FORMAT,
        "only portrait album can operate friend_id, albumId: %{public}d", photoAlbum->GetAlbumId());
    return E_OK;
}

int32_t PortraitFriendIdHandler::Execute(const std::shared_ptr<PhotoAlbum> &photoAlbum,
    const AnalysisAlbumOperation &operation)
{
    CHECK_AND_RETURN_RET_LOG(photoAlbum != nullptr, E_PARAM_CONVERT_FORMAT,
        "photoAlbum is nullptr");
    CHECK_AND_RETURN_RET_LOG(ValidateTarget(photoAlbum) == E_OK, E_PARAM_CONVERT_FORMAT,
        "photoAlbum is not a valid target");
    CHECK_AND_RETURN_RET_LOG(operation.attr == GetSpec().attr, E_PARAM_CONVERT_FORMAT,
        "unsupported attribute for portrait friend_id handler: %{public}s", operation.attr.c_str());
    return MediaLibraryAlbumOperations::OperatePortraitAlbumFriendId(std::to_string(photoAlbum->GetAlbumId()),
        operation.values);
}

int32_t PortraitFriendIdHandler::GetAttributeExecute(const std::shared_ptr<PhotoAlbum> &photoAlbum,
    std::string &friendId)
{
    CHECK_AND_RETURN_RET_LOG(photoAlbum != nullptr, E_INNER_FAIL,
        "photoAlbum is nullptr");
    return MediaLibraryAlbumOperations::GetPortraitAlbumFriendId(photoAlbum->GetAlbumId(),
        friendId);
}
} // namespace OHOS::Media
