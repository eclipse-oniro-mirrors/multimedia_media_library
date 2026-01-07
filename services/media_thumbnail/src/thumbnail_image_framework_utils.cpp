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
#define MLOG_TAG "Thumbnail"

#include "thumbnail_image_framework_utils.h"

#include <securec.h>

#include "image_format_convert.h"
#include "v1_0/buffer_handle_meta_key_type.h"

#include "medialibrary_errno.h"
#include "medialibrary_tracer.h"
#include "media_image_framework_utils.h"
#include "media_log.h"
#include "thumbnail_const.h"

using namespace std;
using namespace OHOS::HDI::Display::Graphic::Common::V1_0;

namespace OHOS {
namespace Media {

static constexpr int32_t PLANE_Y = 0;
static constexpr int32_t PLANE_U = 1;
static constexpr int32_t PLANE_V = 2;
static constexpr uint8_t HDR_PIXEL_SIZE = 2;
static constexpr uint8_t SDR_PIXEL_SIZE = 1;

// LCOV_EXCL_START
bool ThumbnailImageFrameWorkUtils::IsYuvPixelMap(std::shared_ptr<PixelMap> pixelMap)
{
    CHECK_AND_RETURN_RET_LOG(pixelMap != nullptr, false, "PixelMap is nullptr");
    PixelFormat format = pixelMap->GetPixelFormat();
    return format == PixelFormat::NV21 || format == PixelFormat::NV12 ||
        format == PixelFormat::YCRCB_P010 || format == PixelFormat::YCBCR_P010;
}

bool ThumbnailImageFrameWorkUtils::IsSupportCopyPixelMap(std::shared_ptr<PixelMap> pixelMap)
{
    CHECK_AND_RETURN_RET_LOG(pixelMap != nullptr, false, "PixelMap is nullptr");
    if (!IsYuvPixelMap(pixelMap)) {
        return true;
    }
    PixelFormat format = pixelMap->GetPixelFormat();
    CHECK_AND_RETURN_RET_LOG(format == PixelFormat::NV21 || format == PixelFormat::NV12, false,
        "Not support copy pixelMap, format:%{public}d", format);
    return true;
}

std::shared_ptr<Picture> ThumbnailImageFrameWorkUtils::CopyPictureSource(std::shared_ptr<Picture> picture)
{
    CHECK_AND_RETURN_RET_LOG(picture != nullptr, nullptr, "Picture is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("CopyPictureSource");
    auto pixelMap = picture->GetMainPixel();
    auto gainMap = picture->GetGainmapPixelMap();
    CHECK_AND_RETURN_RET_LOG(pixelMap != nullptr && gainMap != nullptr, nullptr,
        "PixelMap or gainMap is nullptr");
    MEDIA_INFO_LOG("Picture information: pixelMap format:%{public}d isHdr:%{public}d allocatorType:%{public}d, "
        "gainMap format:%{public}d isHdr:%{public}d allocatorType:%{public}d, size: %{public}d * %{public}d",
        pixelMap->GetPixelFormat(), pixelMap->IsHdr(), pixelMap->GetAllocatorType(), gainMap->GetPixelFormat(),
        gainMap->IsHdr(), gainMap->GetAllocatorType(), pixelMap->GetWidth(), pixelMap->GetHeight());

    std::shared_ptr<PixelMap> copyPixelMap = CopyPixelMapSource(pixelMap);
    CHECK_AND_RETURN_RET_LOG(copyPixelMap != nullptr, nullptr, "Copy pixelMap failed");

    std::shared_ptr<PixelMap> copyGainMap = CopyPixelMapSource(gainMap);
    CHECK_AND_RETURN_RET_LOG(copyGainMap != nullptr, nullptr, "Copy gainMap failed");

    Size copyGainMapSize = {copyGainMap->GetWidth(), copyGainMap->GetHeight()};
    auto auxiliaryPicturePtr = AuxiliaryPicture::Create(copyGainMap, AuxiliaryPictureType::GAINMAP, copyGainMapSize);
    std::shared_ptr<AuxiliaryPicture> auxiliaryPicture = std::move(auxiliaryPicturePtr);
    CHECK_AND_RETURN_RET_LOG(auxiliaryPicture != nullptr, nullptr, "Create auxiliaryPicture failed");

    auto copySourcePtr = Picture::Create(copyPixelMap);
    std::shared_ptr<Picture> copySource = std::move(copySourcePtr);
    copySource->SetAuxiliaryPicture(auxiliaryPicture);
    return copySource;
}

std::shared_ptr<PixelMap> ThumbnailImageFrameWorkUtils::CopyPixelMapSource(std::shared_ptr<PixelMap> pixelMap)
{
    CHECK_AND_RETURN_RET_LOG(IsSupportCopyPixelMap(pixelMap), nullptr, "Not support copy pixelMap");
    if (IsYuvPixelMap(pixelMap)) {
        return CopyYuvPixelmap(pixelMap);
    }
    return CopyNormalPixelmap(pixelMap);
}

std::shared_ptr<PixelMap> ThumbnailImageFrameWorkUtils::CopyNormalPixelmap(std::shared_ptr<PixelMap> pixelMap)
{
    MediaLibraryTracer tracer;
    tracer.Start("CopyNormalPixelmap");
    Media::InitializationOptions pixelMapOpts = {
        .size = {pixelMap->GetWidth(), pixelMap->GetHeight()},
        .pixelFormat = pixelMap->GetPixelFormat(),
        .alphaType = pixelMap->GetAlphaType()
    };
    auto copyPixelMapPtr = PixelMap::Create(*pixelMap, pixelMapOpts);
    std::shared_ptr<PixelMap> copyPixelMap = std::move(copyPixelMapPtr);
    CHECK_AND_RETURN_RET_LOG(copyPixelMap != nullptr, nullptr, "CopyNormalPixelmap failed");
    return copyPixelMap;
}

std::shared_ptr<PixelMap> ThumbnailImageFrameWorkUtils::CopyYuvPixelmap(std::shared_ptr<PixelMap> pixelMap)
{
    if (pixelMap->GetAllocatorType() == AllocatorType::DMA_ALLOC) {
        return CopyYuvPixelmapWithSurfaceBuffer(pixelMap);
    }
    return CopyNoSurfaceBufferYuvPixelmap(pixelMap);
}

std::shared_ptr<PixelMap> ThumbnailImageFrameWorkUtils::CopyYuvPixelmapWithSurfaceBuffer(
    std::shared_ptr<PixelMap> pixelMap)
{
    MediaLibraryTracer tracer;
    tracer.Start("CopyYuvPixelmapWithSurfaceBuffer");
    CHECK_AND_RETURN_RET_LOG(pixelMap->GetFd() != nullptr, nullptr, "Get fd failed");
    sptr<SurfaceBuffer> surfaceBuffer = reinterpret_cast<SurfaceBuffer *>(pixelMap->GetFd());
    CHECK_AND_RETURN_RET_LOG(surfaceBuffer != nullptr, nullptr, "Get surfaceBuffer failed");
    sptr<SurfaceBuffer> dstSurfaceBuffer = SurfaceBuffer::Create();
    CHECK_AND_RETURN_RET_LOG(dstSurfaceBuffer != nullptr, nullptr, "Create surfaceBuffer failed");

    BufferRequestConfig requestConfig = {
        .width = surfaceBuffer->GetWidth(),
        .height = surfaceBuffer->GetHeight(),
        .strideAlignment = 0x2,
        .format = surfaceBuffer->GetFormat(),
        .usage = surfaceBuffer->GetUsage(),
        .timeout = 0,
    };
    GSError allocRes = dstSurfaceBuffer->Alloc(requestConfig);
    CHECK_AND_RETURN_RET_LOG(allocRes == 0, nullptr, "Alloc surfaceBuffer failed, err:%{public}d", allocRes);
    CopySurfaceBufferInfo(surfaceBuffer, dstSurfaceBuffer);
    int32_t copyRes = memcpy_s(dstSurfaceBuffer->GetVirAddr(), dstSurfaceBuffer->GetSize(),
                               surfaceBuffer->GetVirAddr(), surfaceBuffer->GetSize());
    CHECK_AND_RETURN_RET_LOG(copyRes == E_OK, nullptr,
        "Copy surface buffer pixels failed, copyRes %{public}d", copyRes);

    InitializationOptions opts;
    opts.size.width = pixelMap->GetWidth();
    opts.size.height = pixelMap->GetHeight();
    opts.srcPixelFormat = pixelMap->GetPixelFormat();
    opts.pixelFormat = pixelMap->GetPixelFormat();
    opts.useDMA = true;
    std::shared_ptr<PixelMap> copyPixelMap = PixelMap::Create(opts);
    CHECK_AND_RETURN_RET_LOG(copyPixelMap != nullptr, nullptr, "Create pixelMap failed");

    void* nativeBuffer = dstSurfaceBuffer.GetRefPtr();
    RefBase *ref = reinterpret_cast<RefBase *>(nativeBuffer);
    ref->IncStrongRef(ref);
    copyPixelMap->SetHdrType(pixelMap->GetHdrType());
    copyPixelMap->InnerSetColorSpace(pixelMap->InnerGetGrColorSpace());
    copyPixelMap->SetPixelsAddr(dstSurfaceBuffer->GetVirAddr(), dstSurfaceBuffer.GetRefPtr(),
        dstSurfaceBuffer->GetSize(), AllocatorType::DMA_ALLOC, nullptr);
    CHECK_AND_RETURN_RET_LOG(SetPixelMapYuvInfo(dstSurfaceBuffer, copyPixelMap, pixelMap->IsHdr()), nullptr,
        "SetPixelMapYuvInfo failed");
    return copyPixelMap;
}

std::shared_ptr<PixelMap> ThumbnailImageFrameWorkUtils::CopyNoSurfaceBufferYuvPixelmap(
    std::shared_ptr<PixelMap> pixelMap)
{
    MediaLibraryTracer tracer;
    tracer.Start("CopyNoSurfaceBufferYuvPixelmap");
    auto startPtr = pixelMap->GetPixels();
    InitializationOptions opts;
    opts.size.width = pixelMap->GetWidth();
    opts.size.height = pixelMap->GetHeight();
    opts.pixelFormat = pixelMap->GetPixelFormat();
    std::shared_ptr<PixelMap> copyPixelMap = PixelMap::Create(opts);
    CHECK_AND_RETURN_RET_LOG(copyPixelMap != nullptr, nullptr, "Create pixelMap failed");

    int32_t copyRes = memcpy_s(copyPixelMap->GetWritablePixels(), pixelMap->GetByteCount(),
        startPtr, pixelMap->GetByteCount());
    CHECK_AND_RETURN_RET_LOG(copyRes == E_OK, nullptr,
        "CopyNoSurfaceBufferYuvPixelmap failed, copyRes:%{public}d", copyRes);
    sptr<SurfaceBuffer> surfaceBuffer = nullptr;
    CHECK_AND_RETURN_RET_LOG(SetPixelMapYuvInfo(surfaceBuffer, copyPixelMap, false), nullptr,
        "SetPixelMapYuvInfo failed");
    return copyPixelMap;
}

bool ThumbnailImageFrameWorkUtils::SetPixelMapYuvInfo(sptr<SurfaceBuffer> &surfaceBuffer,
    std::shared_ptr<PixelMap> pixelMap, bool isHdr)
{
    CHECK_AND_RETURN_RET_LOG(pixelMap != nullptr, false, "PixelMap is nullptr");
    uint8_t ratio = isHdr ? HDR_PIXEL_SIZE : SDR_PIXEL_SIZE;
    int32_t srcWidth = pixelMap->GetWidth();
    int32_t srcHeight = pixelMap->GetHeight();
    YUVDataInfo yuvDataInfo = { .yWidth = srcWidth,
                                .yHeight = srcHeight,
                                .uvWidth = srcWidth / 2,
                                .uvHeight = srcHeight / 2,
                                .yStride = srcWidth,
                                .uvStride = srcWidth,
                                .uvOffset = srcWidth * srcHeight};

    if (surfaceBuffer == nullptr) {
        pixelMap->SetImageYUVInfo(yuvDataInfo);
        return true;
    }
    OH_NativeBuffer_Planes *planes = nullptr;
    GSError retVal = surfaceBuffer->GetPlanesInfo(reinterpret_cast<void**>(&planes));
    if (retVal != OHOS::GSERROR_OK || planes == nullptr) {
        pixelMap->SetImageYUVInfo(yuvDataInfo);
        return true;
    }

    auto format = pixelMap->GetPixelFormat();
    if (format == PixelFormat::NV12) {
        yuvDataInfo.yStride = planes->planes[PLANE_Y].columnStride / ratio;
        yuvDataInfo.uvStride = planes->planes[PLANE_U].columnStride / ratio;
        yuvDataInfo.yOffset = planes->planes[PLANE_Y].offset / ratio;
        yuvDataInfo.uvOffset = planes->planes[PLANE_U].offset / ratio;
    } else if (format == PixelFormat::NV21) {
        yuvDataInfo.yStride = planes->planes[PLANE_Y].columnStride / ratio;
        yuvDataInfo.uvStride = planes->planes[PLANE_V].columnStride / ratio;
        yuvDataInfo.yOffset = planes->planes[PLANE_Y].offset / ratio;
        yuvDataInfo.uvOffset = planes->planes[PLANE_V].offset / ratio;
    } else {
        MEDIA_ERR_LOG("Not support SetImageYUVInfo, format:%{public}d", format);
        return false;
    }

    pixelMap->SetImageYUVInfo(yuvDataInfo);
    return true;
}

void ThumbnailImageFrameWorkUtils::CopySurfaceBufferInfo(sptr<SurfaceBuffer> &source, sptr<SurfaceBuffer> &dst)
{
    MediaLibraryTracer tracer;
    tracer.Start("CopySurfaceBufferInfo");
    CHECK_AND_RETURN_LOG(source != nullptr && dst != nullptr,
        "CopySurfaceBufferInfo failed, source or dst is nullptr");
    std::vector<uint8_t> hdrMetadataTypeVec;
    std::vector<uint8_t> colorSpaceInfoVec;
    std::vector<uint8_t> staticData;
    std::vector<uint8_t> dynamicData;

    if (source->GetMetadata(ATTRKEY_HDR_METADATA_TYPE, hdrMetadataTypeVec) == GSERROR_OK) {
        dst->SetMetadata(ATTRKEY_HDR_METADATA_TYPE, hdrMetadataTypeVec);
    }
    if (source->GetMetadata(ATTRKEY_COLORSPACE_INFO, colorSpaceInfoVec) == GSERROR_OK) {
        dst->SetMetadata(ATTRKEY_COLORSPACE_INFO, colorSpaceInfoVec);
    }
    if (GetSbStaticMetadata(source, staticData) && (staticData.size() > 0)) {
        SetSbStaticMetadata(dst, staticData);
    }
    if (GetSbDynamicMetadata(source, dynamicData) && (dynamicData.size()) > 0) {
        SetSbDynamicMetadata(dst, dynamicData);
    }
}

bool ThumbnailImageFrameWorkUtils::GetSbStaticMetadata(const sptr<SurfaceBuffer> &buffer,
    std::vector<uint8_t> &staticMetadata)
{
    return buffer->GetMetadata(ATTRKEY_HDR_STATIC_METADATA, staticMetadata) == GSERROR_OK;
}

bool ThumbnailImageFrameWorkUtils::GetSbDynamicMetadata(const sptr<SurfaceBuffer> &buffer,
    std::vector<uint8_t> &dynamicMetadata)
{
    return buffer->GetMetadata(ATTRKEY_HDR_DYNAMIC_METADATA, dynamicMetadata) == GSERROR_OK;
}

bool ThumbnailImageFrameWorkUtils::SetSbStaticMetadata(sptr<SurfaceBuffer> &buffer,
    const std::vector<uint8_t> &staticMetadata)
{
    return buffer->SetMetadata(ATTRKEY_HDR_STATIC_METADATA, staticMetadata) == GSERROR_OK;
}

bool ThumbnailImageFrameWorkUtils::SetSbDynamicMetadata(sptr<SurfaceBuffer> &buffer,
    const std::vector<uint8_t> &dynamicMetadata)
{
    return buffer->SetMetadata(ATTRKEY_HDR_DYNAMIC_METADATA, dynamicMetadata) == GSERROR_OK;
}

bool ThumbnailImageFrameWorkUtils::IsSupportGenAstc()
{
    return ImageSource::IsSupportGenAstc();
}

bool ThumbnailImageFrameWorkUtils::IsPictureValid(const std::shared_ptr<Picture>& picture)
{
    CHECK_AND_RETURN_RET_LOG(picture != nullptr, false, "Picture is null");
    CHECK_AND_RETURN_RET_LOG(picture->GetMainPixel() != nullptr, false, "Picture main pixel is null");
    CHECK_AND_RETURN_RET_LOG(picture->GetGainmapPixelMap() != nullptr, false, "Picture gain pixel is null");
    auto mainWidth = picture->GetMainPixel()->GetWidth();
    auto mainHeight = picture->GetMainPixel()->GetHeight();
    CHECK_AND_RETURN_RET_LOG(mainWidth > 0 && mainHeight > 0, false,
        "Invalid main pixel map size: width %{public}d, height: %{public}d", mainWidth, mainHeight);
    return true;
}

bool ThumbnailImageFrameWorkUtils::IsPixelMapValid(const std::shared_ptr<PixelMap>& pixelMap)
{
    CHECK_AND_RETURN_RET_LOG(pixelMap != nullptr, false, "PixelMap is null");
    auto width = pixelMap->GetWidth();
    auto height = pixelMap->GetHeight();
    CHECK_AND_RETURN_RET_LOG(width > 0 && height > 0, false,
        "Invalid pixel map size: width %{public}d, height: %{public}d", width, height);
    return true;
}

std::shared_ptr<Picture> ThumbnailImageFrameWorkUtils::CopyAndScalePicture(const std::shared_ptr<Picture>& picture,
    const Size& desiredSize)
{
    std::shared_ptr<Picture> copyPicture = nullptr;
    CHECK_AND_RETURN_RET_LOG(desiredSize.width > 0 && desiredSize.height > 0, nullptr,
        "Invalid desired size: width: %{public}d, height: %{public}d", desiredSize.width, desiredSize.height);
    CHECK_AND_RETURN_RET_LOG(ThumbnailImageFrameWorkUtils::IsPictureValid(picture), nullptr, "picture is invalid");

    copyPicture = ThumbnailImageFrameWorkUtils::CopyPictureSource(picture);
    CHECK_AND_RETURN_RET_LOG(IsPictureValid(copyPicture), nullptr, "CopyPictureSource failed");

    float widthScale = (1.0f * desiredSize.width) / picture->GetMainPixel()->GetWidth();
    float heightScale = (1.0f * desiredSize.height) / picture->GetMainPixel()->GetHeight();
    copyPicture->GetMainPixel()->scale(widthScale, heightScale);
    copyPicture->GetGainmapPixelMap()->scale(widthScale, heightScale);
    return copyPicture;
}

std::shared_ptr<PixelMap> ThumbnailImageFrameWorkUtils::CopyAndScalePixelMap(const std::shared_ptr<PixelMap>& pixelMap,
    const Size& desiredSize)
{
    CHECK_AND_RETURN_RET_LOG(ThumbnailImageFrameWorkUtils::IsPixelMapValid(pixelMap), nullptr, "PixelMap is invalid");
    CHECK_AND_RETURN_RET_LOG(desiredSize.width > 0 && desiredSize.height > 0, nullptr,
        "Invalid desired size: width: %{public}d, height: %{public}d", desiredSize.width, desiredSize.height);
    auto copySource = ThumbnailImageFrameWorkUtils::CopyPixelMapSource(pixelMap);
    CHECK_AND_RETURN_RET_LOG(IsPixelMapValid(copySource), nullptr, "CopyPixelMapSource failed");
    float widthScale = (1.0f * desiredSize.width) / pixelMap->GetWidth();
    float heightScale = (1.0f * desiredSize.height) / pixelMap->GetHeight();
    copySource->scale(widthScale, heightScale);
    return copySource;
}

bool ThumbnailImageFrameWorkUtils::FlipAndRotatePicture(std::shared_ptr<Picture> picture, int32_t exifRotate)
{
    FlipAndRotateInfo info;
    CHECK_AND_RETURN_RET_LOG(ExifRotateUtils::GetFlipAndRotateInfo(exifRotate, info), false,
        "ExifRotate:%{public}d is invalid", exifRotate);
    return FlipAndRotatePicture(picture, info);
}

bool ThumbnailImageFrameWorkUtils::FlipAndRotatePicture(std::shared_ptr<Picture> picture, const FlipAndRotateInfo &info)
{
    CHECK_AND_RETURN_RET_LOG(IsPictureValid(picture), false, "Picture is invalid");
    auto pixelMap = picture->GetMainPixel();
    CHECK_AND_RETURN_RET_LOG(ThumbnailImageFrameWorkUtils::FlipAndRotatePixelMap(pixelMap, info),
        false, "FlipAndRotate pixelMap failed");

    auto gainMap = picture->GetGainmapPixelMap();
    CHECK_AND_RETURN_RET_LOG(ThumbnailImageFrameWorkUtils::FlipAndRotatePixelMap(gainMap, info),
        false, "FlipAndRotate gainMap failed");
    return true;
}

bool ThumbnailImageFrameWorkUtils::FlipAndRotatePixelMap(std::shared_ptr<PixelMap> pixelMap, int32_t exifRotate)
{
    FlipAndRotateInfo info;
    CHECK_AND_RETURN_RET_LOG(ExifRotateUtils::GetFlipAndRotateInfo(exifRotate, info), false,
        "GetFlipAndRotateInfo failed, exifRotate:%{public}d", exifRotate);
    return FlipAndRotatePixelMap(pixelMap, info);
}

bool ThumbnailImageFrameWorkUtils::FlipAndRotatePixelMap(std::shared_ptr<PixelMap> pixelMap,
    const FlipAndRotateInfo &info)
{
    CHECK_AND_RETURN_RET_LOG(pixelMap != nullptr, false, "PixelMap is nullptr");
    return MediaImageFrameWorkUtils::FlipAndRotatePixelMap(*(pixelMap.get()), info);
}

bool ThumbnailImageFrameWorkUtils::ConvertPixelMapToSdrAndFormatRGBA8888(std::shared_ptr<PixelMap> &pixelMap)
{
    CHECK_AND_RETURN_RET_LOG(pixelMap != nullptr, false, "PixelMap is nullptr");
    if (pixelMap->IsHdr()) {
        uint32_t ret = pixelMap->ToSdr();
        CHECK_AND_RETURN_RET_LOG(ret == 0, false, "Failed to transform to sdr");
    }

    ImageInfo imageInfo;
    pixelMap->GetImageInfo(imageInfo);
    if (imageInfo.pixelFormat != PixelFormat::RGBA_8888) {
        uint32_t ret = ImageFormatConvert::ConvertImageFormat(pixelMap, PixelFormat::RGBA_8888);
        CHECK_AND_RETURN_RET_LOG(ret == 0, false,
            "Failed to convert image format, format: %{public}d", imageInfo.pixelFormat);
    }
    return true;
}

bool ThumbnailImageFrameWorkUtils::ResizePicture(PicturePtr &picture, const Size &targetSize)
{
    CHECK_AND_RETURN_RET_LOG(IsPictureValid(picture), false, "Invalid picture");
    CHECK_AND_RETURN_RET_LOG(targetSize.width > 0 && targetSize.height > 0, false,
        "Invalid target size, weith: %{public}d height: %{public}d", targetSize.width, targetSize.height);
    auto mainMap = picture->GetMainPixel();
    auto gainMap = picture->GetGainmapPixelMap();
    float widthScale = (1.0f * targetSize.width) / mainMap->GetWidth();
    float heightScale = (1.0f * targetSize.height) / mainMap->GetHeight();
    MediaLibraryTracer tracer;
    tracer.Start("Main map resize");
    CHECK_AND_RETURN_RET_LOG(mainMap->resize(widthScale, heightScale), false, "Main map resize failed");
    tracer.Finish();
    tracer.Start("Gain map resize");
    CHECK_AND_RETURN_RET_LOG(gainMap->resize(widthScale, heightScale), false, "Gain map resize failed");
    tracer.Finish();
    return true;
}

PicturePtr ThumbnailImageFrameWorkUtils::CreatePicture(const ImageSourcePtr &imageSource, const Size &targetSize)
{
    CHECK_AND_RETURN_RET_LOG(imageSource != nullptr, nullptr, "Image source is null");
    CHECK_AND_RETURN_RET_LOG(targetSize.width > 0 && targetSize.height > 0, nullptr,
        "Invalid target size, weith: %{public}d height: %{public}d", targetSize.width, targetSize.height);
    DecodingOptionsForPicture pictureOpts;
    pictureOpts.desireAuxiliaryPictures = {AuxiliaryPictureType::GAINMAP};
    uint32_t err = E_OK;
    MediaLibraryTracer tracer;
    tracer.Start("ImageSource::CreatePicture");
    std::unique_ptr<Picture> uniquePtrPicture = imageSource->CreatePicture(pictureOpts, err);
    PicturePtr picture = move(uniquePtrPicture);
    tracer.Finish();
    CHECK_AND_RETURN_RET_LOG(err == E_OK, nullptr, "ImageSource::CreatePicture err: %{public}d", err);
    CHECK_AND_RETURN_RET_LOG(IsPictureValid(picture), nullptr, "Invalid picture");
    CHECK_AND_RETURN_RET_LOG(ResizePicture(picture, targetSize), nullptr, "ResizePicture failed");
    return picture;
}

PixelMapPtr ThumbnailImageFrameWorkUtils::CreatePixelMap(const ImageSourcePtr &imageSource,
    const DecodeOptions &decodeOpts, uint32_t &err)
{
    CHECK_AND_RETURN_RET_LOG(imageSource != nullptr, nullptr, "Image source is null");
    MediaLibraryTracer tracer;
    tracer.Start("ImageSource::CreatePixelMap");
    std::shared_ptr<PixelMap> pixelMap = imageSource->CreatePixelMap(decodeOpts, err);
    tracer.Finish();
    CHECK_AND_RETURN_RET_LOG(err == E_OK && pixelMap != nullptr, nullptr,
        "ImageSource::CreatePixelMap err: %{public}d", err);
    MEDIA_DEBUG_LOG("Is pixel map support hdr: %{public}d", pixelMap->IsHdr());
    return pixelMap;
}

bool ThumbnailImageFrameWorkUtils::IsAdaptedHdrType(const ImageHdrType hdrType)
{
    switch (hdrType) {
        case ImageHdrType::HDR_ISO_DUAL:
        case ImageHdrType::HDR_CUVA:
        case ImageHdrType::HDR_VIVID_DUAL:
        case ImageHdrType::HDR_LOG_DUAL:
        case ImageHdrType::HDR_VIVID_SINGLE:
        case ImageHdrType::HDR_ISO_SINGLE:
            return true;
        
        default:
            MEDIA_ERR_LOG("ImageHdrType: %{public}d not adapted", hdrType);
            return false;
    }
}

bool ThumbnailImageFrameWorkUtils::IsSinglePixelMapHdrType(const ImageHdrType hdrType)
{
    switch (hdrType) {
        case ImageHdrType::HDR_VIVID_SINGLE:
        case ImageHdrType::HDR_ISO_SINGLE:
            return true;
        default:
            return false;
    }
}

// LCOV_EXCL_STOP
} // namespace Media
} // namespace OHOS