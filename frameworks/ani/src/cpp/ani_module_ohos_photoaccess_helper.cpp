/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "cloud_enhancement_ani.h"
#include "cloud_enhancement_task_state_ani.h"
#include "file_asset_ani.h"
#include "fetch_result_ani.h"
#include "highlight_album_ani.h"
#include "media_album_change_request_ani.h"
#include "media_asset_change_request_ani.h"
#include "media_assets_change_request_ani.h"
#include "media_asset_edit_data_ani.h"
#include "media_asset_manager_ani.h"
#include "media_library_ani.h"
#include "medialibrary_ani_log.h"
#include "moving_photo_ani.h"
#include "photo_album_ani.h"
#include "photo_asset_custom_record_manager_ani.h"
#include "photo_proxy_ani.h"
#include "cloud_media_asset_manager_ani.h"
#include "cloud_media_asset_status_ani.h"

using namespace OHOS::Media;

namespace OHOS::Media::Ani {
ani_status GlobalFunctionInit(ani_env *env)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    const char *namespaceName = "@ohos.file.photoAccessHelper.photoAccessHelper";
    ani_namespace ns;
    if (ANI_OK != env->FindNamespace(namespaceName, &ns)) {
        ANI_ERR_LOG("Not found namespace: %{public}s", namespaceName);
        return ANI_ERROR;
    }

    std::array staticMethods = {
        ani_native_function {"getPhotoAccessHelperInner", nullptr,
            reinterpret_cast<void *>(MediaLibraryAni::GetPhotoAccessHelperInner)},
        ani_native_function {"getPhotoAccessHelperWithUserIdInner", nullptr,
            reinterpret_cast<void *>(MediaLibraryAni::GetPhotoAccessHelperWithUserIdInner)},
    };

    if (ANI_OK != env->Namespace_BindNativeFunctions(ns, staticMethods.data(), staticMethods.size())) {
        ANI_ERR_LOG("Cannot bind native methods to namespace: %{public}s", namespaceName);
        return ANI_ERROR;
    }

    ANI_INFO_LOG("GlobalFunctionInit ok");
    return ANI_OK;
}
} // namespace OHOS::Media::Ani

ANI_EXPORT ani_status ANI_Constructor(ani_vm *vm, uint32_t *result)
{
    ANI_INFO_LOG("ANI_Constructor start");
    CHECK_COND_RET(vm != nullptr, ANI_ERROR, "vm is nullptr");
    ani_env *env;
    if (ANI_OK != vm->GetEnv(ANI_VERSION_1, &env)) {
        ANI_ERR_LOG("Unsupported %{public}d", ANI_VERSION_1);
        return ANI_ERROR;
    }
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");

    CHECK_STATUS_RET(OHOS::Media::Ani::GlobalFunctionInit(env), "GlobalFunctionInit fail");
    CHECK_STATUS_RET(MediaLibraryAni::PhotoAccessHelperInit(env), "PhotoAccessHelperInit fail");
    CHECK_STATUS_RET(FetchFileResultAni::PhotoAccessHelperInit(env), "FetchFileResult init fail");
    CHECK_STATUS_RET(FileAssetAni::PhotoAccessHelperInit(env), "FileAssetAni init fail");
    CHECK_STATUS_RET(PhotoAlbumAni::PhotoAccessInit(env), "PhotoAccessInit fail");
    CHECK_STATUS_RET(HighlightAlbumAni::Init(env), "HighlightAlbumAni init fail");
    CHECK_STATUS_RET(HighlightAlbumAni::AnalysisAlbumInit(env), "AnalysisAlbumInit fail");
    CHECK_STATUS_RET(MediaAssetEditDataAni::Init(env), "MediaAssetEditDataAni init fail");
    CHECK_STATUS_RET(MediaAssetChangeRequestAni::Init(env), "MediaAssetChangeRequestAni init fail");
    CHECK_STATUS_RET(MediaAssetsChangeRequestAni::Init(env), "MediaAssetsChangeRequestAni init fail");
    CHECK_STATUS_RET(MediaAlbumChangeRequestAni::Init(env), "MediaAlbumChangeRequestAni init fail");
    CHECK_STATUS_RET(MediaAlbumChangeRequestAni::MediaAnalysisAlbumChangeRequestInit(env),
        "MediaAnalysisAlbumChangeRequestAni init fail");
    CHECK_STATUS_RET(MediaAssetManagerAni::Init(env), "MediaAssetManagerAni init fail");
    CHECK_STATUS_RET(MovingPhotoAni::Init(env), "MovingPhoto init fail");
    CHECK_STATUS_RET(CloudEnhancementAni::Init(env), "CloudEnhancementAni init fail");
    CHECK_STATUS_RET(CloudEnhancementTaskStateAni::Init(env), "CloudEnhancementTaskStateAni init fail");
    CHECK_STATUS_RET(CloudMediaAssetManagerAni::Init(env), "CloudMediaAssetManagerAni init fail");
    CHECK_STATUS_RET(CloudMediaAssetStatusAni::Init(env), "CloudMediaAssetStatusAni init fail");
    CHECK_STATUS_RET(PhotoAssetCustomRecordManagerAni::Init(env), "PhotoAssetCustomRecordManagerAni init fail");

    *result = ANI_VERSION_1;
    return ANI_OK;
}
