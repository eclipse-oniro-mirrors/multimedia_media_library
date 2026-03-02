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

#ifndef FRAMEWORKS_ANI_SRC_INCLUDE_PICTURE_HANDLE_CLIENT_H
#define FRAMEWORKS_ANI_SRC_INCLUDE_PICTURE_HANDLE_CLIENT_H

#include <memory>
#include "message_parcel.h"
#include "picture.h"
#include "surface_buffer.h"

namespace OHOS {
namespace Media {

static const int UINT32_LEN = sizeof(uint32_t);

class PictureHandlerClient {
public:
    static std::shared_ptr<Media::Picture> RequestPicture(const int32_t &fileId, bool &isHighQuality);

private:
    static void FinishRequestPicture(const int32_t &fileId);
    static int32_t ReadPicture(const int32_t &fd, const int32_t &fileId, std::shared_ptr<Media::Picture> &picture,
        bool &isHighQuality);
    static std::shared_ptr<PixelMap> ReadPixelMap(MessageParcel &data);
    static bool ReadAuxiliaryPicture(MessageParcel &data, std::unique_ptr<Media::Picture> &picture);
    static bool ReadAuxiliaryPictureInfo(MessageParcel &data, AuxiliaryPictureInfo &auxiliaryPictureInfo);
    static bool ReadImageInfo(MessageParcel &data, ImageInfo &imageInfo);
    static bool ReadYuvDataInfo(MessageParcel &data, YUVDataInfo &yuvInfo);
    static bool ReadSurfaceBuffer(MessageParcel &data, std::unique_ptr<PixelMap> &pixelMap);
    static bool ReadBufferHandle(MessageParcel &data, sptr<SurfaceBuffer> &surfaceBuffer);
    static bool ReadExifMetadata(MessageParcel &data, std::unique_ptr<Media::Picture> &picture);
    static bool ReadMaintenanceData(MessageParcel &data, std::unique_ptr<Media::Picture> &picture);
    static int32_t RequestBufferHandlerFd(const int32_t &fd);
};

} // namespace Media
} // namespace OHOS
#endif // FRAMEWORKS_ANI_SRC_INCLUDE_PICTURE_HANDLE_CLIENT_H