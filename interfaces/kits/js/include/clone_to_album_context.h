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

#ifndef INTERFACES_KITS_JS_INCLUDE_CLONE_TO_ALBUM_CONTEXT_H
#define INTERFACES_KITS_JS_INCLUDE_CLONE_TO_ALBUM_CONTEXT_H

#include <string>
#include <vector>
#include <atomic>

#include "clone_to_album_callback_napi.h"

#include "task_signal_napi.h"

namespace OHOS {
namespace Media {

struct CloneToAlbumContext {
    napi_ref sizeProgressListener {nullptr};
    napi_ref countProgressListener {nullptr};
    napi_ref resultListener {nullptr};

    sptr<CloneToAlbumCallbackNapi> callback;

    std::vector<std::string> fileUris;

    int32_t albumId {0};
    int32_t mode {0};
    std::string targetDir;
    int32_t requestId {0};
    std::string owner;
    int64_t shareGroup {0};
};

} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIA_CLONE_TO_ALBUM_CONTEXT_H
