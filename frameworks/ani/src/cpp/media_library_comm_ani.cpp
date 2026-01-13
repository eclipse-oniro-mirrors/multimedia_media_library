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

#define MLOG_TAG "MediaLibraryCommAni"

#include "media_library_comm_ani.h"

#include <ani.h>
#include "file_asset_ani.h"
#include "media_file_utils.h"
#include "medialibrary_ani_utils.h"
#include "photo_proxy.h"
//LCOV_EXCL_START
namespace OHOS {
namespace Media {
MediaLibraryCommAni::MediaLibraryCommAni() {}

MediaLibraryCommAni::~MediaLibraryCommAni() {}

ani_object MakePhotoAssetAni(ani_env *env, shared_ptr<FileAsset> fileAsset, bool IsCapture, int32_t captureId = -1)
{
    FileAssetAniMethod fileAssetAniMethod;
    if (ANI_OK != FileAssetAni::InitFileAssetAniMethod(env, ResultNapiType::TYPE_PHOTOACCESS_HELPER,
        fileAssetAniMethod)) {
        ANI_ERR_LOG("InitFileAssetAniMethod failed");
        return nullptr;
    }
    ani_object ret = FileAssetAni::Wrap(env, FileAssetAni::CreatePhotoAsset(env, fileAsset), fileAssetAniMethod);
    if (!IsCapture) {
        ANI_DEBUG_LOG("Not using captureId");
        return ret;
    }
    ani_method setCaptureId;
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    CHECK_COND_RET(env->Class_FindMethod(fileAssetAniMethod.cls, "<set>captureId", nullptr, &setCaptureId),
        nullptr, "No <set>captureId");
    CHECK_COND_RET(env->Object_CallMethod_Void(ret, setCaptureId, static_cast<ani_double>(captureId)), nullptr,
        "<set>photoType fail");
    return ret;
}

// The current function is only provided to the camera framework.
ani_object MediaLibraryCommAni::CreatePhotoAssetAni(ani_env *env, const std::string &uri,
    int32_t cameraShotType, const std::string &burstKey)
{
    if (uri.empty()) {
        ANI_ERR_LOG("uri is empty");
        return nullptr;
    }
    shared_ptr<FileAsset> fileAsset = make_shared<FileAsset>();
    CHECK_COND_RET(fileAsset != nullptr, nullptr, "fileAsset is nullptr");
    fileAsset->SetUri(uri);
    string fileId = MediaFileUtils::GetIdFromUri(uri);
    if (MediaFileUtils::IsValidInteger(fileId)) {
        fileAsset->SetId(std::atoi(fileId.c_str()));
    }

    fileAsset->SetDisplayName(MediaFileUtils::GetFileName(uri));
    if (cameraShotType == static_cast<int32_t>(CameraShotType::IMAGE)) {
        fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::CAMERA));
        fileAsset->SetMediaType(MediaType::MEDIA_TYPE_IMAGE);
    } else if (cameraShotType == static_cast<int32_t>(CameraShotType::MOVING_PHOTO)) {
        fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
        fileAsset->SetMediaType(MediaType::MEDIA_TYPE_IMAGE);
    } else if (cameraShotType == static_cast<int32_t>(CameraShotType::BURST)) {
        fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::BURST));
        fileAsset->SetMediaType(MediaType::MEDIA_TYPE_IMAGE);
        fileAsset->SetBurstKey(burstKey);
    } else if (cameraShotType == static_cast<int32_t>(CameraShotType::VIDEO)) {
        fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::CAMERA));
        fileAsset->SetMediaType(MediaType::MEDIA_TYPE_VIDEO);
    } else {
        ANI_ERR_LOG("invalid cameraShotKey: %{public}d", cameraShotType);
    }
    fileAsset->SetResultNapiType(ResultNapiType::TYPE_PHOTOACCESS_HELPER);

    return MakePhotoAssetAni(env, fileAsset, false);
}

// The current function is only provided to the camera framework.
ani_object MediaLibraryCommAni::CreatePhotoAssetAni(ani_env *env, const std::string &uri,
    int32_t cameraShotType, int32_t captureId, const std::string &burstKey)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    CHECK_COND_RET(!uri.empty(), nullptr, "uri is empty");

    shared_ptr<FileAsset> fileAsset = make_shared<FileAsset>();
    CHECK_COND_RET(fileAsset != nullptr, nullptr, "fileAsset is nullptr");

    fileAsset->SetUri(uri);
    string fileId = MediaFileUtils::GetIdFromUri(uri);
    if (MediaFileUtils::IsValidInteger(fileId)) {
        fileAsset->SetId(std::atoi(fileId.c_str()));
    }

    fileAsset->SetDisplayName(MediaFileUtils::GetFileName(uri));
    if (cameraShotType == static_cast<int32_t>(CameraShotType::IMAGE)) {
        fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::CAMERA));
        fileAsset->SetMediaType(MediaType::MEDIA_TYPE_IMAGE);
    } else if (cameraShotType == static_cast<int32_t>(CameraShotType::MOVING_PHOTO)) {
        fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
        fileAsset->SetMediaType(MediaType::MEDIA_TYPE_IMAGE);
    } else if (cameraShotType == static_cast<int32_t>(CameraShotType::BURST)) {
        fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::BURST));
        fileAsset->SetMediaType(MediaType::MEDIA_TYPE_IMAGE);
        fileAsset->SetBurstKey(burstKey);
    } else if (cameraShotType == static_cast<int32_t>(CameraShotType::VIDEO)) {
        fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::CAMERA));
        fileAsset->SetMediaType(MediaType::MEDIA_TYPE_VIDEO);
    } else {
        ANI_ERR_LOG("invalid cameraShotKey: %{public}d", cameraShotType);
    }
    fileAsset->SetResultNapiType(ResultNapiType::TYPE_PHOTOACCESS_HELPER);

    return MakePhotoAssetAni(env, fileAsset, true, captureId);
}

} // namespace Media
} // namespace OHOS
//LCOV_EXCL_STOP