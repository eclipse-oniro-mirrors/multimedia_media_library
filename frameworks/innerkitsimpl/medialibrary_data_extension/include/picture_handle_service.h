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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_PICTURE_HANDLE_SERVICE_H
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_PICTURE_HANDLE_SERVICE_H

#include "message_parcel.h"
#include "picture.h"
#include "surface_buffer.h"

namespace OHOS {
namespace Media {

static const int UINT32_LEN = sizeof(uint32_t);
static const std::string PICTURE_ASHMEM_NAME = "media_picture_ashmem_";
static const size_t AUXILIARY_PICTURE_TYPE_COUNT = static_cast<size_t>(AuxiliaryPictureType::FRAGMENT_MAP);

class PictureHandlerService {
public:
    static bool OpenPicture(const std::string &fileId, int32_t &fd);
    static int32_t RequestBufferHandlerFd(const std::string fd);
private:

    static bool WritePicture(const int32_t &fileId, MessageParcel &data, uint32_t &auxiliaryPictureSize);
    static bool WritePixelMap(MessageParcel &data, std::shared_ptr<PixelMap> &pixelMap);
    static bool WriteProperties(MessageParcel &data, std::shared_ptr<PixelMap> &pixelMap);
    static bool WriteImageInfo(MessageParcel &data, std::shared_ptr<PixelMap> &pixelMap, bool &isYuv);
    static bool WriteYuvDataInfo(MessageParcel &data, std::shared_ptr<PixelMap> &pixelMap);
    static bool WriteSurfaceBuffer(MessageParcel &data, std::shared_ptr<PixelMap> &pixelMap);
    static bool WriteBufferHandler(MessageParcel &data, BufferHandle &handle);
    static bool WriteExifMetadata(MessageParcel &data, std::shared_ptr<Media::Picture> &picture);
    static bool WriteMaintenanceData(MessageParcel &data, std::shared_ptr<Media::Picture> &picture);
    static bool WriteAuxiliaryPicture(MessageParcel &data, std::shared_ptr<AuxiliaryPicture> &auxiliaryPicture);
    static bool WriteAuxiliaryPictureInfo(MessageParcel &data, std::shared_ptr<AuxiliaryPicture> &auxiliaryPicture);
    static bool WriteAuxiliaryMetadata(MessageParcel &data, std::shared_ptr<AuxiliaryPicture> &auxiliaryPicture);
};
}
}
#endif