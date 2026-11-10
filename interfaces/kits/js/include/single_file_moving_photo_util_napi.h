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

#ifndef INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_SINGLE_FILE_MOVING_PHOTO_UTIL_NAPI_H
#define INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_SINGLE_FILE_MOVING_PHOTO_UTIL_NAPI_H

#include <string>

#include "napi_base_context.h"
#include "napi_error.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace OHOS::Media {
class SingleFileMovingPhotoUtil {
public:
    EXPORT static napi_value Init(napi_env env, napi_value exports);

private:
    EXPORT static napi_value Constructor(napi_env env, napi_callback_info info);
    EXPORT static void Destructor(napi_env env, void* nativeObject, void* finalizeHint);

    EXPORT static napi_value JSIsSingleFileMovingPhoto(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSUnpackingSingleFileMovingPhoto(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSPackingSingleFileMovingPhoto(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetVideoOffset(napi_env env, napi_callback_info info);

    static thread_local napi_ref constructor_;
};
} // namespace OHOS::Media
#endif // INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_SINGLE_FILE_MOVING_PHOTO_UTIL_NAPI_H
