/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_PHOTO_ASSET_PROXY_SCAN_CAMERA_FILE_DTO_H
#define OHOS_PHOTO_ASSET_PROXY_SCAN_CAMERA_FILE_DTO_H

#include "scan_camera_file_vo.h"

namespace OHOS::Media {
class ScanCameraFileDto {
public:
    int32_t fileId{0};
    bool needUpdateAlbum{false};
    bool needGenerateThumbnail{false};
    int32_t pathType{0};

public:
    static ScanCameraFileDto Create(const ScanCameraFileReqBody &req);
    std::string ToString() const;
};
}  // namespace OHOS::Media
#endif  // OHOS_PHOTO_ASSET_PROXY_SCAN_CAMERA_FILE_DTO_H