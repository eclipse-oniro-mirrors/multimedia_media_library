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

#include "picture_handle_service.h"

#include <cstdlib>
#include <fcntl.h>
#include <libexif/exif-entry.h>
#include <securec.h>
#include <sys/mman.h>
#include <unistd.h>

#include "ashmem.h"
#include "exif_metadata.h"
#include "image_type.h"
#include "metadata.h"
#include "medialibrary_errno.h"
#include "medialibrary_photo_operations.h"
#include "media_log.h"

namespace OHOS {
namespace Media {

bool PictureHandlerService::OpenPicture(const std::string &fileId, int32_t &fd)
{
    MEDIA_DEBUG_LOG("PictureHandlerService OpenPicture fileId: %{public}s", fileId.c_str());
    MessageParcel data;
    // 辅图数量
    uint32_t auxiliaryPictureSize = 0;
    WritePicture(std::atoi(fileId.c_str()), data, auxiliaryPictureSize);

    uint32_t dataSize = data.GetDataSize();

    // 消息长度
    uint32_t msgLen = 0;
    msgLen += UINT32_LEN; // msgLen长度
    msgLen += UINT32_LEN; // dataSize长度
    msgLen += UINT32_LEN; // auxiliaryPictureSize长度
    msgLen += dataSize; // data长度

    // 封装消息
    MessageParcel msgParcel;
    msgParcel.WriteUint32(msgLen);
    MEDIA_DEBUG_LOG("PictureHandlerService::OpenPicture msgLen: %{public}d", msgLen);
    msgParcel.WriteUint32(dataSize);
    MEDIA_DEBUG_LOG("PictureHandlerService::OpenPicture dataSize: %{public}d", dataSize);
    msgParcel.WriteUint32(auxiliaryPictureSize);
    MEDIA_DEBUG_LOG("PictureHandlerService::OpenPicture auxiliaryPictureSize: %{public}d", auxiliaryPictureSize);
    msgParcel.WriteBuffer((void*)data.GetData(), dataSize);

    // 创建共享内存
    std::string name = PICTURE_ASHMEM_NAME + fileId;
    fd = AshmemCreate(name.c_str(), msgParcel.GetDataSize());
    MEDIA_DEBUG_LOG("PictureHandlerService::OpenPicture fd:  %{public}d", fd);
    CHECK_AND_RETURN_RET_LOG(fd >= 0, false,
        "PictureHandlerService::OpenPicture AshmemCreate failed, name: %{public}s, fd: %{public}d", name.c_str(), fd);

    int result = AshmemSetProt(fd, PROT_READ | PROT_WRITE);
    if (result < 0) {
        MEDIA_ERR_LOG("PictureHandlerService::OpenPicture AshmemSetProt failed, result: %{public}d", result);
        close(fd);
        return false;
    }

    void *addr = mmap(nullptr, msgParcel.GetDataSize(), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        MEDIA_ERR_LOG("PictureHandlerService::OpenPicture mmap failed!");
        close(fd);
        return false;
    }

    if (memcpy_s(addr, msgParcel.GetDataSize(), (void*)msgParcel.GetData(), msgParcel.GetDataSize())) {
        MEDIA_ERR_LOG("PictureHandlerService::OpenPicture memcpy_s failed!");
        close(fd);
        munmap(addr, msgParcel.GetDataSize());
        return false;
    }
    munmap(addr, msgParcel.GetDataSize());
    MEDIA_INFO_LOG("PictureHandlerService::OpenPicture end");
    return true;
}

static bool WritePhotoQuality(MessageParcel &data, bool isHighQualityPicture)
{
    data.WriteBool(isHighQualityPicture);
    return true;
}

bool PictureHandlerService::WritePicture(const int32_t &fileId, MessageParcel &data,
    uint32_t &auxiliaryPictureSize)
{
    MEDIA_DEBUG_LOG("PictureHandlerService WritePicture enter, fileId: %{public}d", fileId);
    std::shared_ptr<Media::Picture> picture;
    std::string photoId;
    bool isHighQualityPicture = false;
    int32_t ret = MediaLibraryPhotoOperations::GetPicture(fileId, picture, false, photoId, isHighQualityPicture);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, false,
        "PictureHandlerService::GetPicture picture is not exist, fileId: %{public}d", fileId);

    std::shared_ptr<PixelMap> mainPixel = picture->GetMainPixel();
    CHECK_AND_RETURN_RET_LOG(mainPixel != nullptr, false,
        "PictureHandlerService::GetPicture mainPixel is not exist, fileId: %{public}d", fileId);

    WritePhotoQuality(isHighQualityPicture);
    WritePixelMap(data, mainPixel);

    WriteExifMetadata(data, picture);
    WriteMaintenanceData(data, picture);

    for (size_t i = 0; i <= AUXILIARY_PICTURE_TYPE_COUNT; i++) {
        if (!picture->HasAuxiliaryPicture(static_cast<AuxiliaryPictureType>(i))) {
            continue;
        }
        AuxiliaryPictureType type = static_cast<AuxiliaryPictureType>(i);
        MEDIA_DEBUG_LOG("PictureHandlerService::WriteAuxiliaryPicture type: %{public}d", type);
        std::shared_ptr<AuxiliaryPicture> auxiliaryPicture = picture->GetAuxiliaryPicture(
            static_cast<AuxiliaryPictureType>(i));
        if (auxiliaryPicture == nullptr) {
            MEDIA_DEBUG_LOG("PictureHandlerService::WritePicture auxiliaryPicture is null, type: %{public}d", type);
            continue;
        }
        
        auxiliaryPictureSize ++;
        WriteAuxiliaryPicture(data, auxiliaryPicture);
    }
    return true;
}

bool PictureHandlerService::WritePixelMap(MessageParcel &data, std::shared_ptr<PixelMap> &pixelMap)
{
    WriteProperties(data, pixelMap);
    MEDIA_DEBUG_LOG("PictureHandlerService WritePixelMap write surface buffer");
    WriteSurfaceBuffer(data, pixelMap);
    return true;
}

bool PictureHandlerService::WriteProperties(MessageParcel &data, std::shared_ptr<PixelMap> &pixelMap)
{
    bool isYuv = false;
    WriteImageInfo(data, pixelMap, isYuv);
    MEDIA_DEBUG_LOG("PictureHandlerService::WriteProperties isYuv:%{public}d", isYuv);
    data.WriteBool(isYuv);
    if (isYuv) {
        WriteYuvDataInfo(data, pixelMap);
    }
    MEDIA_DEBUG_LOG("PictureHandlerService::WriteProperties editable:%{public}d", pixelMap->IsEditable());
    data.WriteBool(pixelMap->IsEditable());
    return true;
}

bool PictureHandlerService::WriteImageInfo(MessageParcel &data, std::shared_ptr<PixelMap> &pixelMap, bool &isYuv)
{
    ImageInfo imageInfo;
    pixelMap->GetImageInfo(imageInfo);

    MEDIA_DEBUG_LOG("PictureHandlerService::WriteImageInfo width: %{public}d", imageInfo.size.width);
    data.WriteInt32(imageInfo.size.width);

    MEDIA_DEBUG_LOG("PictureHandlerService::WriteImageInfo height: %{public}d", imageInfo.size.height);
    data.WriteInt32(imageInfo.size.height);

    MEDIA_DEBUG_LOG("PictureHandlerService::WriteImageInfo pixelFormat: %{public}d", imageInfo.pixelFormat);
    data.WriteInt32(static_cast<int32_t>(imageInfo.pixelFormat));
    isYuv = (imageInfo.pixelFormat == PixelFormat::NV21 || imageInfo.pixelFormat == PixelFormat::NV12);

    MEDIA_DEBUG_LOG("PictureHandlerService::WriteImageInfo colorSpace: %{public}d", imageInfo.colorSpace);
    data.WriteInt32(static_cast<int32_t>(imageInfo.colorSpace));

    MEDIA_DEBUG_LOG("PictureHandlerService::WriteImageInfo alphaType: %{public}d", imageInfo.alphaType);
    data.WriteInt32(static_cast<int32_t>(imageInfo.alphaType));

    MEDIA_DEBUG_LOG("PictureHandlerService::WriteImageInfo baseDensity: %{public}d", imageInfo.baseDensity);
    data.WriteInt32(imageInfo.baseDensity);
    return true;
}

bool PictureHandlerService::WriteYuvDataInfo(MessageParcel &data, std::shared_ptr<PixelMap> &pixelMap)
{
    YUVDataInfo yuvInfo;
    pixelMap->GetImageYUVInfo(yuvInfo);

    MEDIA_DEBUG_LOG("PictureHandlerService::WriteYuvDataInfo width: %{public}d", yuvInfo.imageSize.width);
    data.WriteInt32(static_cast<int32_t>(yuvInfo.imageSize.width));

    MEDIA_DEBUG_LOG("PictureHandlerService::WriteYuvDataInfo height: %{public}d", yuvInfo.imageSize.height);
    data.WriteInt32(static_cast<int32_t>(yuvInfo.imageSize.height));

    MEDIA_DEBUG_LOG("PictureHandlerService::WriteYuvDataInfo yWidth: %{public}d", yuvInfo.yWidth);
    data.WriteInt32(static_cast<int32_t>(yuvInfo.yWidth));

    MEDIA_DEBUG_LOG("PictureHandlerService::WriteYuvDataInfo yHeight: %{public}d", yuvInfo.yHeight);
    data.WriteInt32(static_cast<int32_t>(yuvInfo.yHeight));

    MEDIA_DEBUG_LOG("PictureHandlerService::WriteYuvDataInfo uvWidth: %{public}d", yuvInfo.uvWidth);
    data.WriteInt32(static_cast<int32_t>(yuvInfo.uvWidth));

    MEDIA_DEBUG_LOG("PictureHandlerService::WriteYuvDataInfo uvHeight: %{public}d", yuvInfo.uvHeight);
    data.WriteInt32(static_cast<int32_t>(yuvInfo.uvHeight));

    MEDIA_DEBUG_LOG("PictureHandlerService::WriteYuvDataInfo yStride: %{public}d", yuvInfo.yStride);
    data.WriteInt32(static_cast<int32_t>(yuvInfo.yStride));

    MEDIA_DEBUG_LOG("PictureHandlerService::WriteYuvDataInfo uStride: %{public}d", yuvInfo.uStride);
    data.WriteInt32(static_cast<int32_t>(yuvInfo.uStride));

    MEDIA_DEBUG_LOG("PictureHandlerService::WriteYuvDataInfo vStride: %{public}d", yuvInfo.vStride);
    data.WriteInt32(static_cast<int32_t>(yuvInfo.vStride));

    MEDIA_DEBUG_LOG("PictureHandlerService::WriteYuvDataInfo uvStride: %{public}d", yuvInfo.uvStride);
    data.WriteInt32(static_cast<int32_t>(yuvInfo.uvStride));

    MEDIA_DEBUG_LOG("PictureHandlerService::WriteYuvDataInfo yOffset: %{public}d", yuvInfo.yOffset);
    data.WriteInt32(static_cast<int32_t>(yuvInfo.yOffset));

    MEDIA_DEBUG_LOG("PictureHandlerService::WriteYuvDataInfo uOffset: %{public}d", yuvInfo.uOffset);
    data.WriteInt32(static_cast<int32_t>(yuvInfo.uOffset));

    MEDIA_DEBUG_LOG("PictureHandlerService::WriteYuvDataInfo vOffset: %{public}d", yuvInfo.vOffset);
    data.WriteInt32(static_cast<int32_t>(yuvInfo.vOffset));

    MEDIA_DEBUG_LOG("PictureHandlerService::WriteYuvDataInfo uvOffset: %{public}d", yuvInfo.uvOffset);
    data.WriteInt32(static_cast<int32_t>(yuvInfo.uvOffset));

    return true;
}

bool PictureHandlerService::WriteSurfaceBuffer(MessageParcel &data, std::shared_ptr<PixelMap> &pixelMap)
{
    // surfaceBuffer 序列化
    SurfaceBuffer *surfaceBuffer = reinterpret_cast<SurfaceBuffer*> (pixelMap->GetFd());
    if (surfaceBuffer == nullptr) {
        MEDIA_DEBUG_LOG("PictureHandlerService::WriteSurfaceBuffer surfaceBuffer is null");
        return false;
    }

    BufferHandle *handle = surfaceBuffer->GetBufferHandle();
    bool hasBufferHandle = (handle != nullptr);
    MEDIA_DEBUG_LOG("PictureHandlerService::WriteSurfaceBuffer hasBufferHandle: %{public}d", hasBufferHandle);
    data.WriteBool(hasBufferHandle);
    CHECK_AND_RETURN_RET(hasBufferHandle, false);
    return WriteBufferHandler(data, *handle);
}

bool PictureHandlerService ::WriteBufferHandler(MessageParcel &data, BufferHandle &handle)
{
    MEDIA_DEBUG_LOG("PictureHandlerService::WriteBufferHandler reserveFds: %{public}d", handle.reserveFds);
    data.WriteUint32(handle.reserveFds);

    MEDIA_DEBUG_LOG("PictureHandlerService::WriteBufferHandler reserveInts: %{public}d", handle.reserveInts);
    data.WriteUint32(handle.reserveInts);

    MEDIA_DEBUG_LOG("PictureHandlerService::WriteBufferHandler width: %{public}d", handle.width);
    data.WriteInt32(handle.width);

    MEDIA_DEBUG_LOG("PictureHandlerService::WriteBufferHandler stride: %{public}d", handle.stride);
    data.WriteInt32(handle.stride);

    MEDIA_DEBUG_LOG("PictureHandlerService::WriteBufferHandler height: %{public}d", handle.height);
    data.WriteInt32(handle.height);

    MEDIA_DEBUG_LOG("PictureHandlerService::WriteBufferHandler size: %{public}d", handle.size);
    data.WriteInt32(handle.size);

    MEDIA_DEBUG_LOG("PictureHandlerService::WriteBufferHandler format: %{public}d", handle.format);
    data.WriteInt32(handle.format);

    data.WriteUint64(handle.usage);

    data.WriteUint64(handle.phyAddr);

    MEDIA_DEBUG_LOG("PictureHandlerService::WriteBufferHandler fd: %{public}d.", handle.fd);
    data.WriteInt32(handle.fd);

    for (uint32_t i = 0; i < handle.reserveFds; i++) {
        MEDIA_DEBUG_LOG("PictureHandlerService::WriteBufferHandler reserve[%{public}d]: %{public}d",
            i, handle.reserve[i]);
        data.WriteInt32(handle.reserve[i]);
    }
    for (uint32_t j = 0; j < handle.reserveInts; j++) {
        data.WriteInt32(handle.reserve[handle.reserveFds + j]);
    }

    return true;
}

bool PictureHandlerService::WriteExifMetadata(MessageParcel &data, std::shared_ptr<Media::Picture> &picture)
{
    // 序列化ExifMetadata
    std::shared_ptr<ExifMetadata> exifMetadata = picture->GetExifMetadata();
    bool hasExifMetadata = (exifMetadata != nullptr);
    MEDIA_DEBUG_LOG("PictureHandlerService WriteExifMetadata hasExifMetadata :%{public}d", hasExifMetadata);
    data.WriteBool(hasExifMetadata);
    CHECK_AND_RETURN_RET(hasExifMetadata, true);
    return exifMetadata->Marshalling(data);
}

bool PictureHandlerService::WriteMaintenanceData(MessageParcel &data, std::shared_ptr<Media::Picture> &picture)
{
    sptr<SurfaceBuffer> surfaceBuffer = picture->GetMaintenanceData();
    bool hasMaintenanceData = (surfaceBuffer != nullptr);
    MEDIA_DEBUG_LOG("PictureHandlerService WriteMaintenanceData hasMaintenanceData :%{public}d", hasMaintenanceData);
    data.WriteBool(hasMaintenanceData);
    CHECK_AND_RETURN_RET(hasMaintenanceData, true);
    BufferHandle *handle = surfaceBuffer->GetBufferHandle();
    return WriteBufferHandler(data, *handle);
}

bool PictureHandlerService::WriteAuxiliaryPicture(MessageParcel &data,
    std::shared_ptr<AuxiliaryPicture> &auxiliaryPicture)
{
    MEDIA_DEBUG_LOG("PictureHandlerService WriteAuxiliaryPicture enter");

    WriteAuxiliaryPictureInfo(data, auxiliaryPicture);

    std::shared_ptr<PixelMap> pixelMap = auxiliaryPicture->GetContentPixel();
    if (pixelMap != nullptr) {
        WritePixelMap(data, pixelMap);
    }

    WriteAuxiliaryMetadata(data, auxiliaryPicture);

    return true;
}

bool PictureHandlerService::WriteAuxiliaryPictureInfo(MessageParcel &data,
    std::shared_ptr<AuxiliaryPicture> &auxiliaryPicture)
{
    AuxiliaryPictureInfo auxiliaryPictureInfo = auxiliaryPicture->GetAuxiliaryPictureInfo();

    MEDIA_DEBUG_LOG("PictureHandlerService::WriteAuxiliaryPictureInfo auxiliaryPictureType: %{public}d",
        auxiliaryPictureInfo.auxiliaryPictureType);
    data.WriteInt32(static_cast<int32_t>(auxiliaryPictureInfo.auxiliaryPictureType));

    MEDIA_DEBUG_LOG("PictureHandlerService::WriteAuxiliaryPictureInfo colorSpace: %{public}d",
        auxiliaryPictureInfo.colorSpace);
    data.WriteInt32(static_cast<int32_t>(auxiliaryPictureInfo.colorSpace));

    MEDIA_DEBUG_LOG("PictureHandlerService::WriteAuxiliaryPictureInfo pixelFormat: %{public}d",
        auxiliaryPictureInfo.pixelFormat);
    data.WriteInt32(static_cast<int32_t>(auxiliaryPictureInfo.pixelFormat));

    MEDIA_DEBUG_LOG("PictureHandlerService::WriteAuxiliaryPictureInfo rowStride: %{public}d",
        auxiliaryPictureInfo.rowStride);
    data.WriteInt32(auxiliaryPictureInfo.rowStride);

    MEDIA_DEBUG_LOG("PictureHandlerService::WriteAuxiliaryPictureInfo height: %{public}d",
        auxiliaryPictureInfo.size.height);
    data.WriteInt32(auxiliaryPictureInfo.size.height);

    MEDIA_DEBUG_LOG("PictureHandlerService::WriteAuxiliaryPictureInfo width: %{public}d",
        auxiliaryPictureInfo.size.width);
    data.WriteInt32(auxiliaryPictureInfo.size.width);

    return true;
}

bool PictureHandlerService::WriteAuxiliaryMetadata(MessageParcel &data,
    std::shared_ptr<AuxiliaryPicture> &auxiliaryPicture)
{
    int32_t metadataSize = 0;
    bool hasExif = auxiliaryPicture->HasMetadata(MetadataType::EXIF);
    bool hasFragment = auxiliaryPicture->HasMetadata(MetadataType::FRAGMENT);
    if (hasExif) {
        metadataSize ++;
    }
    if (hasFragment) {
        metadataSize ++;
    }
    MEDIA_DEBUG_LOG("PictureHandlerService::WriteAuxiliaryMetadata metadataSize: %{public}d", metadataSize);
    data.WriteInt32(metadataSize);
    if (hasExif) {
        data.WriteInt32(static_cast<int32_t>(MetadataType::EXIF));
        if (auxiliaryPicture->GetMetadata(MetadataType::EXIF) != nullptr) {
            auxiliaryPicture->GetMetadata(MetadataType::EXIF)->Marshalling(data);
        }
    }
    if (hasFragment) {
        data.WriteInt32(static_cast<int32_t>(MetadataType::FRAGMENT));
        if (auxiliaryPicture->GetMetadata(MetadataType::FRAGMENT) != nullptr) {
            auxiliaryPicture->GetMetadata(MetadataType::FRAGMENT)->Marshalling(data);
        }
    }
    return true;
}

int32_t PictureHandlerService::RequestBufferHandlerFd(const std::string fd)
{
    MEDIA_DEBUG_LOG("PictureHandlerService RequestBufferHandlerFd fd: %{public}s", fd.c_str());
    int dupFd = dup(std::atoi(fd.c_str()));
    MEDIA_DEBUG_LOG("PictureHandlerService::RequestBufferHandlerFd dupFd: %{public}d", dupFd);
    return dupFd;
}
}
}