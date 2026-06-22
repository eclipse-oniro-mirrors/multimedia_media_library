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

#include "save_camera_photo_dto.h"

namespace OHOS::Media {

SaveCameraPhotoDto SaveCameraPhotoDto::Create(const SaveCameraPhotoReqBody &req)
{
    SaveCameraPhotoDto dto;
    dto.fileId = req.fileId;
    dto.mediaType = req.mediaType;
    dto.photoSubType = req.photoSubType;
    dto.imageFileType = req.imageFileType;
    dto.supportedWatermarkType = req.supportedWatermarkType;
    dto.discardHighQualityPhoto = req.discardHighQualityPhoto;
    dto.needScan = req.needScan;
    dto.path = req.path;
    dto.cameraShotKey = req.cameraShotKey;
    dto.containsAddResource = req.containsAddResource;
    return dto;
}

}  // namespace OHOS::Media