/*
 * Copyright (C) 2021-2022 Huawei Device Co., Ltd.
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

#include "native_module_ohos_medialibrary.h"
extern const char _binary_medialibraryinf_js_start[];
extern const char _binary_medialibraryinf_js_end[];
extern const char _binary_medialibraryinf_abc_start[];
extern const char _binary_medialibraryinf_abc_end[];

namespace OHOS {
namespace Media {
/*
 * Function registering all props and functions of ohos.medialibrary module
 */
static napi_value Export(napi_env env, napi_value exports)
{
    FileAssetNapi::Init(env, exports);
    FetchFileResultNapi::Init(env, exports);
    AlbumNapi::Init(env, exports);
    SmartAlbumNapi::Init(env, exports);
    MediaLibraryNapi::Init(env, exports);
    return exports;
}

extern "C" __attribute__((visibility("default"))) void NAPI_multimedia_mediaLibrary_GetJSCode(const char** buf,
    int* bufLen)
{
    if (buf != nullptr) {
        *buf = _binary_medialibraryinf_js_start;
    }

    if (bufLen != nullptr) {
        *bufLen = _binary_medialibraryinf_js_end - _binary_medialibraryinf_js_start;
    }
}

extern "C" __attribute__((visibility("default"))) void NAPI_multimedia_mediaLibrary_GetABCCode(const char** buf,
    int* bufLen)
{
    if (buf != nullptr) {
        *buf = _binary_medialibraryinf_abc_start;
    }

    if (bufLen != nullptr) {
        *bufLen = _binary_medialibraryinf_abc_end - _binary_medialibraryinf_abc_start;
    }
}

/*
 * module define
 */
static napi_module g_module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Export,
    .nm_modname = "multimedia.mediaLibrary",
    .nm_priv = reinterpret_cast<void *>(0),
    .reserved = {0}
};

/*
 * module register
 */
extern "C" __attribute__((constructor)) void RegisterModule(void)
{
    napi_module_register(&g_module);
}
} // namespace Media
} // namespace OHOS
