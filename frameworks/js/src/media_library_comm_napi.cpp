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

#define MLOG_TAG "MediaLibraryCommNapi"

#include "media_library_comm_napi.h"

#include <charconv>
#include "file_asset_napi.h"
#include "media_file_utils.h"
#include "photo_proxy.h"

using namespace std;

namespace OHOS {
namespace Media {
MediaLibraryCommNapi::MediaLibraryCommNapi() {}

MediaLibraryCommNapi::~MediaLibraryCommNapi() {}

// The current function is only provided to the camera framework.
napi_value MediaLibraryCommNapi::CreatePhotoAssetNapi(
    napi_env env, const string &uri, int32_t cameraShotType, const string &burstKey)
{
    if (uri.empty()) {
        NAPI_ERR_LOG("uri is empty");
        return nullptr;
    }
    shared_ptr<FileAsset> fileAsset = make_shared<FileAsset>();
    fileAsset->SetUri(uri);
    string fileId = MediaFileUtils::GetIdFromUri(uri);
    int32_t value = 0;
    auto [ptr, ec] = std::from_chars(fileId.data(), fileId.data() + fileId.size(), value);
    if (ec == std::errc{} && ptr == fileId.data() + fileId.size()) {
        fileAsset->SetId(value);
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
        NAPI_ERR_LOG("invalid cameraShotKey: %{public}d", cameraShotType);
    }
    fileAsset->SetResultNapiType(ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    return FileAssetNapi::CreatePhotoAsset(env, fileAsset);
}
} // namespace Media
} // namespace OHOS