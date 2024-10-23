/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_PHOTO_PROXY_NAPI_H
#define INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_PHOTO_PROXY_NAPI_H

#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "photo_proxy.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
static const char PHOTO_PROXY_NAPI_CLASS_NAME[] = "PhotoProxy";
class PhotoProxyNapi {
public:
    EXPORT PhotoProxyNapi();
    EXPORT ~PhotoProxyNapi();

    EXPORT static napi_value Init(napi_env env, napi_value exports);

    EXPORT sptr<PhotoProxy> photoProxy_;
    EXPORT static thread_local sptr<PhotoProxy> sPhotoProxy_;

private:
    static void PhotoProxyNapiDestructor(napi_env env, void* nativeObject, void* finalize_hint);
    static napi_value PhotoProxyNapiConstructor(napi_env env, napi_callback_info info);

    static thread_local napi_ref sConstructor_;

    napi_env env_;
    napi_ref wrapper_;
};
} // Media
} // OHOS
#endif // INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_PHOTO_PROXY_NAPI_H