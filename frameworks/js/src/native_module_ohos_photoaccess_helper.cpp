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

#include "album_order_napi.h"
#include "cloud_enhancement_task_state_napi.h"
#include "cloud_enhancement_napi.h"
#include "cloud_media_asset_manager_napi.h"
#include "cloud_media_asset_status_napi.h"
#include "highlight_album_napi.h"
#include "media_album_change_request_napi.h"
#include "media_share_album_change_request_napi.h"
#include "media_asset_change_request_napi.h"
#include "media_asset_edit_data_napi.h"
#include "media_asset_manager_napi.h"
#include "media_assets_change_request_napi.h"
#include "napi/native_node_api.h"
#include "native_module_ohos_medialibrary.h"
#include "photo_proxy_napi.h"
#include "photo_asset_custom_record_manager_napi.h"
#include "cloud_media_download_resources_status_napi.h"
#include "result_set_napi.h"
#include "task_signal_napi.h"

namespace OHOS {
namespace Media {
/*
 * Function registering all props and functions of userfilemanager module
 */
static napi_value PhotoAccessHelperExport(napi_env env, napi_value exports)
{
    MediaLibraryNapi::PhotoAccessHelperInit(env, exports);
    FetchFileResultNapi::PhotoAccessHelperInit(env, exports);
    FileAssetNapi::PhotoAccessHelperInit(env, exports);
    AlbumNapi::PhotoAccessHelperInit(env, exports);
    PhotoAlbumNapi::PhotoAccessInit(env, exports);
    AlbumOrderNapi::PhotoAccessInit(env, exports);
    HighlightAlbumNapi::AnalysisAlbumInit(env, exports);
    HighlightAlbumNapi::Init(env, exports);
    ResultSetNapi::Init(env, exports);
    MediaAssetEditDataNapi::Init(env, exports);
    MediaAssetChangeRequestNapi::Init(env, exports);
    MediaAssetsChangeRequestNapi::Init(env, exports);
    MediaAlbumChangeRequestNapi::Init(env, exports);
    MediaAlbumChangeRequestNapi::MediaAnalysisAlbumChangeRequestInit(env, exports);
    MediaAlbumChangeRequestNapi::MediaHighlightAlbumChangeRequestInit(env, exports);
    MediaShareAlbumChangeRequestNapi::Init(env, exports);
    MediaAssetManagerNapi::Init(env, exports);
    MovingPhotoNapi::Init(env, exports);
    PhotoProxyNapi::Init(env, exports);
    CloudEnhancementNapi::Init(env, exports);
    CloudEnhancementTaskStateNapi::Init(env, exports);
    CloudMediaAssetManagerNapi::Init(env, exports);
    CloudMediaAssetStatusNapi::Init(env, exports);
    PhotoAssetCustomRecordManager::Init(env, exports);
    PhotoAssetCustomRecordNapi::Init(env, exports);
    CloudMediaDownloadResourcesStatusNapi::Init(env, exports);
    TaskSignalNapi::Init(env, exports);
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
    .nm_modname = "file.photoAccessHelper",
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