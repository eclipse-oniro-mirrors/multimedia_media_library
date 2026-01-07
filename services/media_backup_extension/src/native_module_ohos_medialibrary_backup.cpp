/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "native_module_ohos_medialibrary_backup.h"

namespace OHOS {
namespace Media {
/*
 * Function registering all props and functions of ohos.medialibrary module
 */
static napi_value RestoreServiceExport(napi_env env, napi_value exports)
{
    MediaLibraryBackupNapi::Init(env, exports);
    return exports;
}

/*
 * module define for mediabackup
 */
static napi_module g_module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = RestoreServiceExport,
    .nm_modname = "multimedia.mediabackup",
    .nm_priv = reinterpret_cast<void *>(0),
    .reserved = {0}
};

/*
 * module register for mediabackup
 */
extern "C" __attribute__((constructor)) void RegisterModule(void)
{
    napi_module_register(&g_module);
}
} // namespace Media
} // namespace OHOS
