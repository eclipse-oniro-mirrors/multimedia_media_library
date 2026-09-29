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

#ifndef OHOS_MEDIA_SHARE_ALBUM_LITE_ERRNO_H
#define OHOS_MEDIA_SHARE_ALBUM_LITE_ERRNO_H

#include <cstdint>

namespace OHOS::Media::ShareAlbum {

// 查询共享相册归属者的超时时间，单位为毫秒
constexpr int32_t SHARE_ALBUM_QUERY_TIMEOUT_MS = 500;

// 被查询的照片不存在，或查询结果为空
constexpr int32_t ERR_SHARE_ALBUM_NOT_FOUND = -1;

// 共享相册归属者查询未在 SHARE_ALBUM_QUERY_TIMEOUT_MS 内完成
constexpr int32_t ERR_SHARE_ALBUM_QUERY_TIMEOUT = -2;

// 被查询的照片不是共享相册照片
constexpr int32_t ERR_SHARE_ALBUM_RESULT_NOT_SHARED = -3;

}  // namespace OHOS::Media::ShareAlbum

#endif  // OHOS_MEDIA_SHARE_ALBUM_LITE_ERRNO_H
