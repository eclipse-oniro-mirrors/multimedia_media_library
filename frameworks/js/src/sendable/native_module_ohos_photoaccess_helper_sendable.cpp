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

#include "native_module_ohos_medialibrary_sendable.h"

namespace OHOS {
namespace Media {
/*
 * Function registering all props and functions of userfilemanager module
 */
static napi_value PhotoAccessHelperExport(napi_env env, napi_value exports)
{
    SendablePhotoAccessHelper::Init(env, exports);
    SendablePhotoAlbumNapi::PhotoAccessInit(env, exports);
    SendableFileAssetNapi::PhotoAccessHelperInit(env, exports);
    SendableFetchFileResultNapi::PhotoAccessHelperInit(env, exports);
    return exports;
}

/*
 * module define
 */
static napi_module g_photoAccessHelperModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = PhotoAccessHelperExport,
    .nm_modname = "file.sendablePhotoAccessHelper",
    .nm_priv = reinterpret_cast<void *>(0),
    .reserved = {0}
};

/*
 * module register
 */
extern "C" __attribute__((constructor)) void RegisterPhotoAccessHelper(void)
{
    napi_module_register(&g_photoAccessHelperModule);
}
} // namespace Media
} // namespace OHOS