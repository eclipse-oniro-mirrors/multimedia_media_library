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

#ifndef OHOS_MEDIA_ASSETS_MANAGER_SAVE_CAMERA_PHOTO_DTO_H
#define OHOS_MEDIA_ASSETS_MANAGER_SAVE_CAMERA_PHOTO_DTO_H

#include "save_camera_photo_vo.h"

namespace OHOS::Media {
class SaveCameraPhotoDto {
public:
    int32_t fileId{INT32_MIN};
    int32_t mediaType = static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE);
    int32_t photoSubType{INT32_MIN};
    int32_t imageFileType{INT32_MIN};
    int32_t supportedWatermarkType{INT32_MIN};
    bool discardHighQualityPhoto{false};
    bool needScan{false};
    std::string path;
    std::string cameraShotKey{"NotSet"};
    bool containsAddResource{false};

public:
    static SaveCameraPhotoDto Create(const SaveCameraPhotoReqBody &req);
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_ASSETS_MANAGER_SAVE_CAMERA_PHOTO_DTO_H