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

#include "save_camera_photo_vo.h"

#include "itypes_util.h"

namespace OHOS::Media {

bool SaveCameraPhotoReqBody::Unmarshalling(MessageParcel &parcel)
{
    return ITypesUtil::Unmarshal(parcel,
        fileId,
        mediaType,
        photoSubType,
        imageFileType,
        supportedWatermarkType,
        discardHighQualityPhoto,
        needScan,
        path,
        cameraShotKey,
        containsAddResource);
}

bool SaveCameraPhotoReqBody::Marshalling(MessageParcel &parcel) const
{
    return ITypesUtil::Marshal(parcel,
        fileId,
        mediaType,
        photoSubType,
        imageFileType,
        supportedWatermarkType,
        discardHighQualityPhoto,
        needScan,
        path,
        cameraShotKey,
        containsAddResource);
}
}  // namespace OHOS::Media