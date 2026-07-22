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

#ifndef OHOS_MEDIA_PORTRAIT_FRIEND_ID_HANDLER_H
#define OHOS_MEDIA_PORTRAIT_FRIEND_ID_HANDLER_H

#include <memory>

#include "analysis_album_attribute_request_utils.h"
#include "analysis_album_attribute_spec.h"
#include "photo_album.h"

namespace OHOS::Media {
class PortraitFriendIdHandler {
public:
    static const AnalysisAlbumAttributeSpec &GetSpec();
    static int32_t ValidateTarget(const std::shared_ptr<PhotoAlbum> &photoAlbum);
    static int32_t Execute(const std::shared_ptr<PhotoAlbum> &photoAlbum,
        const AnalysisAlbumOperation &operation);
    static int32_t GetAttributeExecute(const std::shared_ptr<PhotoAlbum> &photoAlbum, std::string &friendId);
};
} // namespace OHOS::Media

#endif // OHOS_MEDIA_PORTRAIT_FRIEND_ID_HANDLER_H
