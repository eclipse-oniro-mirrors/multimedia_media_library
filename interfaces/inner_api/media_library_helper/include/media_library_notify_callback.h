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

#ifndef INTERFACES_INNER_API_MEDIA_LIBRARY_HELPER_INCLUDE_MEDIA_LIBRARY_NOTIFY_CALLBACK_H_
#define INTERFACES_INNER_API_MEDIA_LIBRARY_HELPER_INCLUDE_MEDIA_LIBRARY_NOTIFY_CALLBACK_H_

#define EXPORT __attribute__ ((visibility ("default")))

#include "media_library_notify_types.h"

namespace OHOS {
namespace Media {
class PhotoAlbumChangeCallback {
public:
    PhotoAlbumChangeCallback() = default;
    EXPORT virtual ~PhotoAlbumChangeCallback() = default;
    EXPORT virtual void OnChange(const AlbumChangeInfos &changeInfos) = 0;
};

class PhotoAssetChangeCallback {
public:
    PhotoAssetChangeCallback() = default;
    EXPORT virtual ~PhotoAssetChangeCallback() = default;
    EXPORT virtual void OnChange(const PhotoAssetChangeInfos &changeInfos) = 0;
};
} // namespace Media
} // namespace OHOS

#endif // INTERFACES_INNER_API_MEDIA_LIBRARY_HELPER_INCLUDE_MEDIA_LIBRARY_NOTIFY_CALLBACK_H_
