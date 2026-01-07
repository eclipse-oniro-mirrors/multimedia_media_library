/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MediaCameraCharacterService"

#include "media_camera_character_service.h"

#include "media_log.h"
#include "medialibrary_errno.h"
#include "multistages_photo_capture_manager.h"
#include "multistages_video_capture_manager.h"
#include "userfile_manager_types.h"

using namespace std;

namespace OHOS::Media {
MediaCameraCharacterService &MediaCameraCharacterService::GetInstance()
{
    static MediaCameraCharacterService service;
    return service;
}

int32_t MediaCameraCharacterService::AddProcessVideo(const AddProcessVideoDto &dto)
{
    MultiStagesVideoCaptureManager::GetInstance().AddVideo(dto);
    return E_OK;
}

int32_t MediaCameraCharacterService::CancelRequest(const CancelRequestDto &dto)
{
    switch (dto.mediaType) {
        case static_cast<int32_t>(MEDIA_TYPE_IMAGE): {
            MultiStagesPhotoCaptureManager::GetInstance().CancelProcessRequest(dto.photoId);
            break;
        }
        case static_cast<int32_t>(MEDIA_TYPE_VIDEO): {
            MultiStagesVideoCaptureManager::GetInstance().CancelProcessRequest(dto.photoId);
            break;
        }
        default: {
            MEDIA_ERR_LOG("unsupported media_type");
            return E_ERR;
        }
    }
    return E_OK;
}

int32_t MediaCameraCharacterService::ProcessVideo(const ProcessVideoDto &dto)
{
    MultiStagesVideoCaptureManager::GetInstance().ProcessVideo(dto);
    return E_OK;
}

int32_t MediaCameraCharacterService::GetProgressCallback(GetProgressCallbackRespBody &respbody)
{
    MultiStagesVideoCaptureManager::GetInstance().GetProgressCallback(respbody);
    return E_OK;
}
} // namespace OHOS::Media