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

#ifndef OHOS_MEDIA_ASSETS_MANAGER_SAVE_CAMERA_PHOTO_VO_H
#define OHOS_MEDIA_ASSETS_MANAGER_SAVE_CAMERA_PHOTO_VO_H

#include "i_media_parcelable.h"
#include "userfile_manager_types.h"

namespace OHOS::Media {
class SaveCameraPhotoReqBody : public IPC::IMediaParcelable {
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

public:  // functions of Parcelable.
    bool Unmarshalling(MessageParcel &parcel) override;

    bool Marshalling(MessageParcel &parcel) const override;
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_ASSETS_MANAGER_SAVE_CAMERA_PHOTO_VO_H