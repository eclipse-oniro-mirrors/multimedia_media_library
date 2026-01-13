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

#ifndef FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_IMAGE_FRAMEWORK_UTILS_H_
#define FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_IMAGE_FRAMEWORK_UTILS_H_

#include "image_source.h"
#include "picture.h"
#include "hdr_type.h"
#include "surface_buffer.h"

#include "exif_rotate_utils.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

using ImageSourcePtr = std::unique_ptr<ImageSource>;
using PicturePtr = std::shared_ptr<Picture>;
using PixelMapPtr = std::shared_ptr<PixelMap>;

class ThumbnailImageFrameWorkUtils {
public:
    EXPORT ThumbnailImageFrameWorkUtils() = delete;
    EXPORT virtual ~ThumbnailImageFrameWorkUtils() = delete;

    EXPORT static bool IsYuvPixelMap(std::shared_ptr<PixelMap> pixelMap);
    EXPORT static bool IsSupportCopyPixelMap(std::shared_ptr<PixelMap> pixelMap);
    EXPORT static std::shared_ptr<Picture> CopyPictureSource(std::shared_ptr<Picture> picture);
    EXPORT static std::shared_ptr<PixelMap> CopyPixelMapSource(std::shared_ptr<PixelMap> pixelMap);
    EXPORT static bool IsSupportGenAstc();
    EXPORT static bool IsPictureValid(const std::shared_ptr<Picture>& picture);
    EXPORT static bool IsPixelMapValid(const std::shared_ptr<PixelMap>& pixelMap);
    EXPORT static std::shared_ptr<Picture> CopyAndScalePicture(const std::shared_ptr<Picture>& picture,
        const Size& desiredSize);
    EXPORT static std::shared_ptr<PixelMap> CopyAndScalePixelMap(const std::shared_ptr<PixelMap>& pixelMap,
        const Size& desiredSize);
    EXPORT static bool FlipAndRotatePicture(std::shared_ptr<Picture> picture, int32_t exifRotate);
    EXPORT static bool FlipAndRotatePicture(std::shared_ptr<Picture> picture, const FlipAndRotateInfo &info);
    EXPORT static bool FlipAndRotatePixelMap(std::shared_ptr<PixelMap> pixelMap, int32_t exifRotate);
    EXPORT static bool FlipAndRotatePixelMap(std::shared_ptr<PixelMap> pixelMap, const FlipAndRotateInfo &info);
    EXPORT static bool ConvertPixelMapToSdrAndFormatRGBA8888(std::shared_ptr<PixelMap> &pixelMap);
    EXPORT static PicturePtr CreatePicture(const ImageSourcePtr &imageSource, const Size &targetSize);
    EXPORT static PixelMapPtr CreatePixelMap(const ImageSourcePtr &imageSource,
        const DecodeOptions &decodeOpts, uint32_t &err);
    EXPORT static bool ResizePicture(PicturePtr &picture, const Size &targetSize);
    EXPORT static bool IsAdaptedHdrType(const ImageHdrType hdrType);
    EXPORT static bool IsSinglePixelMapHdrType(const ImageHdrType hdrType);
private:
    EXPORT static std::shared_ptr<PixelMap> CopyNormalPixelmap(std::shared_ptr<PixelMap> pixelMap);
    EXPORT static std::shared_ptr<PixelMap> CopyYuvPixelmap(std::shared_ptr<PixelMap> pixelMap);
    EXPORT static std::shared_ptr<PixelMap> CopyYuvPixelmapWithSurfaceBuffer(std::shared_ptr<PixelMap> pixelMap);
    EXPORT static std::shared_ptr<PixelMap> CopyNoSurfaceBufferYuvPixelmap(std::shared_ptr<PixelMap> pixelMap);
    EXPORT static bool SetPixelMapYuvInfo(sptr<SurfaceBuffer> &surfaceBuffer,
        std::shared_ptr<PixelMap> pixelMap, bool isHdr);
    EXPORT static void CopySurfaceBufferInfo(sptr<SurfaceBuffer> &source, sptr<SurfaceBuffer> &dst);
    EXPORT static bool GetSbStaticMetadata(const sptr<SurfaceBuffer> &buffer, std::vector<uint8_t> &staticMetadata);
    EXPORT static bool GetSbDynamicMetadata(const sptr<SurfaceBuffer> &buffer, std::vector<uint8_t> &dynamicMetadata);
    EXPORT static bool SetSbStaticMetadata(sptr<SurfaceBuffer> &buffer, const std::vector<uint8_t> &staticMetadata);
    EXPORT static bool SetSbDynamicMetadata(sptr<SurfaceBuffer> &buffer, const std::vector<uint8_t> &dynamicMetadata);
};
} // namespace Media
} // namespace OHOS

#endif  // FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_IMAGE_FRAMEWORK_UTILS_H_
