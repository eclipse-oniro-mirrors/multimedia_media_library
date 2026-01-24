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

#include "picture_handle_client.h"

#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <libexif/exif-entry.h>
#include <securec.h>
#include <sys/mman.h>
#include <unistd.h>

#include "exif_metadata.h"
#include "image_type.h"
#include "image_utils.h"
#include "medialibrary_ani_utils.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "pixel_yuv.h"
#include "pixel_yuv_ext.h"
#include "userfilemgr_uri.h"
#include "userfile_client.h"

namespace OHOS {
namespace Media {
const int32_t MAX_VALUE = 100000000;
constexpr uint32_t MAX_SOURCE_SIZE = 300 * 1024 * 1024;
std::shared_ptr<Media::Picture> PictureHandlerClient::RequestPicture(const int32_t &fileId)
{
    ANI_DEBUG_LOG("PictureHandlerClient::RequestPicture fileId: %{public}d", fileId);
    std::string uri = PhotoColumn::PHOTO_REQUEST_PICTURE;
    MediaFileUtils::UriAppendKeyValue(uri, MediaColumn::MEDIA_ID, std::to_string(fileId));
    Uri requestUri(uri);
    int32_t fd = UserFileClient::OpenFile(requestUri, MEDIA_FILEMODE_READONLY);
    if (fd < 0) {
        ANI_DEBUG_LOG("PictureHandlerClient::RequestPicture picture not exist");
        return nullptr;
    }
    std::shared_ptr<Media::Picture> picture = nullptr;
    ReadPicture(fd, fileId, picture);
    FinishRequestPicture(fileId);
    return picture;
}

void PictureHandlerClient::FinishRequestPicture(const int32_t &fileId)
{
    ANI_DEBUG_LOG("PictureHandlerClient::FinishRequestPicture fileId: %{public}d", fileId);
    std::string uri = CONST_PAH_FINISH_REQUEST_PICTURE;
    MediaLibraryAniUtils::UriAppendKeyValue(uri, API_VERSION, std::to_string(MEDIA_API_VERSION_V10));
    Uri finishRequestPictureUri(uri);

    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(PhotoColumn::MEDIA_ID, fileId);
    UserFileClient::Insert(finishRequestPictureUri, valuesBucket);
}

int32_t GetMessageLength(const int32_t &fd, uint32_t &msgLen)
{
    void* msgLenAddr = mmap(nullptr, UINT32_LEN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (msgLenAddr == MAP_FAILED) {
        ANI_ERR_LOG("mmap msgLen failed");
        close(fd);
        return E_ERR;
    }

    uint32_t* pMsgLen = static_cast<uint32_t*>(msgLenAddr);
    if (pMsgLen == nullptr) {
        ANI_ERR_LOG("GetMessageLength failed, mmap return nullptr");
        munmap(msgLenAddr, UINT32_LEN);
        close(fd);
        return E_ERR;
    }
    msgLen = *pMsgLen;
    munmap(msgLenAddr, UINT32_LEN);
    ANI_DEBUG_LOG("msgLen: %{public}u", msgLen);
    return E_OK;
}

int32_t MapMessageData(const int32_t &fd, uint32_t msgLen, uint8_t*& addr)
{
    addr = static_cast<uint8_t*>(mmap(nullptr, msgLen, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    if (addr == MAP_FAILED) {
        ANI_ERR_LOG("mmap addr failed");
        close(fd);
        return E_ERR;
    }
    return E_OK;
}

uint32_t ReadDataSize(const uint8_t* addr, uint32_t& offset)
{
    if (addr == nullptr) {
        ANI_ERR_LOG("ReadDataSize failed, aadr is nullptr!");
        return 0;
    }
    uint32_t dataSize = *reinterpret_cast<const uint32_t*>(addr + offset);
    offset += UINT32_LEN;
    ANI_DEBUG_LOG("dataSize: %{public}u", dataSize);
    return dataSize;
}

int32_t ParseMainPicture(const uint8_t* addr, uint32_t& offset, uint32_t dataSize, MessageParcel& parcel)
{
    uint32_t auxCount = *reinterpret_cast<const uint32_t*>(addr + offset);
    offset += UINT32_LEN;
    ANI_DEBUG_LOG("auxiliaryPictureSize: %{public}u", auxCount);

    if (dataSize == 0 || dataSize > MAX_SOURCE_SIZE) {
        return E_ERR;
    }

    uint8_t* parcelData = static_cast<uint8_t*>(malloc(dataSize));
    if (!parcelData) {
        return E_ERR;
    }

    if (memcpy_s(parcelData, dataSize, addr + offset, dataSize) != 0) {
        free(parcelData);
        return E_ERR;
    }
    offset += dataSize;

    if (!parcel.ParseFrom(reinterpret_cast<uintptr_t>(parcelData), dataSize)) {
        ANI_ERR_LOG("parcelData parse failed!");
        free(parcelData);
        return E_ERR;
    }
    free(parcelData);
    return E_OK;
}

uint32_t ReadAuxiliaryPictureCount(const uint8_t* addr, uint32_t& offset)
{
    uint32_t count = *reinterpret_cast<const uint32_t*>(addr + offset);
    offset += UINT32_LEN;
    return count;
}

int32_t PictureHandlerClient::ReadPicture(const int32_t &fd, const int32_t &fileId,
    std::shared_ptr<Media::Picture> &picture)
{
    ANI_DEBUG_LOG("PictureHandlerClient::ReadPicture fd: %{public}d", fd);

    uint32_t msgLen = 0;
    if (GetMessageLength(fd, msgLen) != E_OK) {
        return E_ERR;
    }

    uint8_t* addr = nullptr;
    if (MapMessageData(fd, msgLen, addr) != E_OK) {
        return E_ERR;
    }
    close(fd);
    uint32_t readOffset = UINT32_LEN;

    uint32_t dataSize = ReadDataSize(addr, readOffset);
    if (dataSize == 0) {
        ANI_DEBUG_LOG("Picture not exists");
        munmap(addr, msgLen);
        return E_NO_SUCH_FILE;
    }

    MessageParcel pictureParcel;
    std::unique_ptr<Media::Picture> picturePtr;
    if (ParseMainPicture(addr, readOffset, dataSize, pictureParcel) != E_OK) {
        munmap(addr, msgLen);
        return E_ERR;
    }
    std::shared_ptr<PixelMap> pixelMap = PictureHandlerClient::ReadPixelMap(pictureParcel);
    picturePtr = Picture::Create(pixelMap);
    if (picturePtr == nullptr) {
        ANI_ERR_LOG("PictureHandlerService::ReadPicture picturePtr is nullptr!");
        munmap(addr, msgLen);
        return E_ERR;
    }

    ReadExifMetadata(pictureParcel, picturePtr);
    ReadMaintenanceData(pictureParcel, picturePtr);

    uint32_t auxiliaryPictureSize = ReadAuxiliaryPictureCount(addr, readOffset);
    for (size_t i = 1; i <= auxiliaryPictureSize; i++) {
        ANI_DEBUG_LOG("PictureHandlerClient::ReadPicture read auxiliaryPicture, index:%{public}zu", i);
        ReadAuxiliaryPicture(pictureParcel, picturePtr);
    }
    picture.reset(picturePtr.get());
    (void)picturePtr.release();
    munmap(addr, msgLen);
    return E_OK;
}

std::shared_ptr<PixelMap> PictureHandlerClient::ReadPixelMap(MessageParcel &data)
{
    ImageInfo imageInfo;
    ReadImageInfo(data, imageInfo);

    bool isYuv = data.ReadBool();
    ANI_DEBUG_LOG("PictureHandlerClient::ReadPixelMap isYuv:%{public}d", isYuv);
    YUVDataInfo yuvInfo;
    if (isYuv) {
        ReadYuvDataInfo(data, yuvInfo);
    }

    bool editable = data.ReadBool();
    ANI_DEBUG_LOG("PictureHandlerClient::ReadPixelMap editable:%{public}d", editable);

    std::unique_ptr<PixelMap> pixelMap;
    if (isYuv) {
#ifdef EXT_PIXEL
        pixelMap = std::make_unique<PixelYuvExt>();
#else
        pixelMap = std::make_unique<PixelYuv>();
#endif
    } else {
        pixelMap = std::make_unique<PixelMap>();
    }
    CHECK_COND_RET(pixelMap != nullptr, nullptr, "pixelMap is nullptr");
    pixelMap->SetImageInfo(imageInfo);
    pixelMap->SetImageYUVInfo(yuvInfo);

    ANI_DEBUG_LOG("PictureHandlerClient::ReadPixelMap read surface buffer");
    ReadSurfaceBuffer(data, pixelMap);
    return pixelMap;
}

bool PictureHandlerClient::ReadAuxiliaryPicture(MessageParcel &data, std::unique_ptr<Media::Picture> &picture)
{
    AuxiliaryPictureInfo auxiliaryPictureInfo;
    ReadAuxiliaryPictureInfo(data, auxiliaryPictureInfo);

    std::shared_ptr<PixelMap> pixelMap = ReadPixelMap(data);
    CHECK_COND_RET(pixelMap != nullptr, false, "pixelMap is nullptr");
    std::unique_ptr<AuxiliaryPicture> uptr = AuxiliaryPicture::Create(pixelMap,
        auxiliaryPictureInfo.auxiliaryPictureType, auxiliaryPictureInfo.size);
    CHECK_COND_RET(uptr != nullptr, false, "uptr is nullptr");
    std::shared_ptr<AuxiliaryPicture> auxiliaryPicture;
    CHECK_COND_RET(auxiliaryPicture != nullptr, false, "auxiliaryPicture is nullptr");
    auxiliaryPicture.reset(uptr.get());
    (void)uptr.release();

    auxiliaryPicture->SetAuxiliaryPictureInfo(auxiliaryPictureInfo);

    int32_t metadataSize = 0;
    if (data.ReadInt32(metadataSize) && metadataSize >= 0 && metadataSize < MAX_VALUE) {
        ANI_DEBUG_LOG("PictureHandlerClient::ReadAuxiliaryPicture metadataSize: %{public}d", metadataSize);
        for (int i = 0; i < metadataSize; i++) {
            MetadataType type = static_cast<MetadataType>(data.ReadInt32());
            ANI_DEBUG_LOG("PictureHandlerClient::ReadAuxiliaryPicture type: %{public}d", type);
            std::shared_ptr<ImageMetadata> metadataPtr(nullptr);
            metadataPtr.reset(ExifMetadata::Unmarshalling(data));
            auxiliaryPicture->SetMetadata(type, metadataPtr);
        }
    } else {
        ANI_ERR_LOG("PictureHandlerClient::ReadAuxiliaryPicture metadataSize failed");
    }
    CHECK_COND_RET(picture != nullptr, false, "picture is nullptr");
    picture->SetAuxiliaryPicture(auxiliaryPicture);
    ANI_DEBUG_LOG("PictureHandler::ReadAuxiliaryPicture end");
    return true;
}

bool PictureHandlerClient::ReadAuxiliaryPictureInfo(MessageParcel &data, AuxiliaryPictureInfo &auxiliaryPictureInfo)
{
    auxiliaryPictureInfo.auxiliaryPictureType = static_cast<AuxiliaryPictureType>(data.ReadInt32());
    ANI_DEBUG_LOG("PictureHandlerClient::ReadAuxiliaryPictureInfo auxiliaryPictureType: %{public}d",
        auxiliaryPictureInfo.auxiliaryPictureType);

    auxiliaryPictureInfo.colorSpace = static_cast<ColorSpace>(data.ReadInt32());
    ANI_DEBUG_LOG("PictureHandlerClient::ReadAuxiliaryPictureInfo colorSpace: %{public}d",
        auxiliaryPictureInfo.colorSpace);

    auxiliaryPictureInfo.pixelFormat = static_cast<PixelFormat>(data.ReadInt32());
    ANI_DEBUG_LOG("PictureHandlerClient::ReadAuxiliaryPictureInfo pixelFormat: %{public}d",
        auxiliaryPictureInfo.pixelFormat);

    auxiliaryPictureInfo.rowStride = static_cast<uint32_t>(data.ReadInt32());
    ANI_DEBUG_LOG("PictureHandlerClient::ReadAuxiliaryPictureInfo rowStride: %{public}d",
        auxiliaryPictureInfo.rowStride);

    auxiliaryPictureInfo.size.height = data.ReadInt32();
    ANI_DEBUG_LOG("PictureHandlerClient::ReadAuxiliaryPictureInfo height: %{public}d",
        auxiliaryPictureInfo.size.height);

    auxiliaryPictureInfo.size.width = data.ReadInt32();
    ANI_DEBUG_LOG("PictureHandlerClient::ReadAuxiliaryPictureInfo width: %{public}d",
        auxiliaryPictureInfo.size.width);
    return true;
}

bool PictureHandlerClient::ReadImageInfo(MessageParcel &data, ImageInfo &imageInfo)
{
    imageInfo.size.width = data.ReadInt32();
    ANI_DEBUG_LOG("PictureHandlerClient::ReadImageInfo width: %{public}d", imageInfo.size.width);
    imageInfo.size.height = data.ReadInt32();
    ANI_DEBUG_LOG("PictureHandlerClient::ReadImageInfo height: %{public}d", imageInfo.size.height);
    imageInfo.pixelFormat = static_cast<PixelFormat>(data.ReadInt32());
    ANI_DEBUG_LOG("PictureHandlerClient::ReadImageInfo pixelFormat: %{public}d", imageInfo.pixelFormat);
    imageInfo.colorSpace = static_cast<ColorSpace>(data.ReadInt32());
    ANI_DEBUG_LOG("PictureHandlerClient::ReadImageInfo colorSpace: %{public}d", imageInfo.colorSpace);
    imageInfo.alphaType = static_cast<AlphaType>(data.ReadInt32());
    ANI_DEBUG_LOG("PictureHandlerClient::ReadImageInfo alphaType: %{public}d", imageInfo.alphaType);
    imageInfo.baseDensity = data.ReadInt32();
    ANI_DEBUG_LOG("PictureHandlerClient::ReadImageInfo baseDensity: %{public}d", imageInfo.baseDensity);
    return true;
}

bool PictureHandlerClient::ReadYuvDataInfo(MessageParcel &data, YUVDataInfo &info)
{
    info.imageSize.width = data.ReadInt32();
    ANI_DEBUG_LOG("PictureHandlerClient::ReadYuvDataInfo width: %{public}d", info.imageSize.width);
    info.imageSize.height = data.ReadInt32();
    ANI_DEBUG_LOG("PictureHandlerClient::ReadYuvDataInfo height: %{public}d", info.imageSize.height);
    info.yWidth = data.ReadUint32();
    ANI_DEBUG_LOG("PictureHandlerClient::ReadYuvDataInfo yWidth: %{public}d", info.yWidth);
    info.yHeight = data.ReadUint32();
    ANI_DEBUG_LOG("PictureHandlerClient::ReadYuvDataInfo yHeight: %{public}d", info.yHeight);
    info.uvWidth = data.ReadUint32();
    ANI_DEBUG_LOG("PictureHandlerClient::ReadYuvDataInfo uvWidth: %{public}d", info.uvWidth);
    info.uvHeight = data.ReadUint32();
    ANI_DEBUG_LOG("PictureHandlerClient::ReadYuvDataInfo uvHeight: %{public}d", info.uvHeight);
    info.yStride = data.ReadUint32();
    ANI_DEBUG_LOG("PictureHandlerClient::ReadYuvDataInfo yStride: %{public}d", info.yStride);
    info.uStride = data.ReadUint32();
    ANI_DEBUG_LOG("PictureHandlerClient::ReadYuvDataInfo uStride: %{public}d", info.uStride);
    info.vStride = data.ReadUint32();
    ANI_DEBUG_LOG("PictureHandlerClient::ReadYuvDataInfo vStride: %{public}d", info.vStride);
    info.uvStride = data.ReadUint32();
    ANI_DEBUG_LOG("PictureHandlerClient::ReadYuvDataInfo uvStride: %{public}d", info.uvStride);
    info.yOffset = data.ReadUint32();
    ANI_DEBUG_LOG("PictureHandlerClient::ReadYuvDataInfo yOffset: %{public}d", info.yOffset);
    info.uOffset = data.ReadUint32();
    ANI_DEBUG_LOG("PictureHandlerClient::ReadYuvDataInfo uOffset: %{public}d", info.uOffset);
    info.vOffset = data.ReadUint32();
    ANI_DEBUG_LOG("PictureHandlerClient::ReadYuvDataInfo vOffset: %{public}d", info.vOffset);
    info.uvOffset = data.ReadUint32();
    ANI_DEBUG_LOG("PictureHandlerClient::ReadYuvDataInfo uvOffset: %{public}d", info.uvOffset);
    return true;
}

bool PictureHandlerClient::ReadSurfaceBuffer(MessageParcel &data, std::unique_ptr<PixelMap> &pixelMap)
{
    CHECK_COND_RET(pixelMap != nullptr, false, "pixelMap is nullptr");
    bool hasBufferHandle = data.ReadBool();
    ANI_DEBUG_LOG("PictureHandlerClient::ReadSurfaceBuffer hasBufferHandle: %{public}d", hasBufferHandle);
    if (!hasBufferHandle) {
        return false;
    }
    sptr<SurfaceBuffer> surfaceBuffer = SurfaceBuffer::Create();
    CHECK_COND_RET(surfaceBuffer != nullptr, false, "surfaceBuffer creation failed");
    ReadBufferHandle(data, surfaceBuffer);
    void* nativeBuffer = surfaceBuffer.GetRefPtr();
    OHOS::RefBase *ref = reinterpret_cast<OHOS::RefBase *>(nativeBuffer);
    CHECK_COND_RET(ref != nullptr, false, "ref is nullptr");
    ref->IncStrongRef(ref);
    pixelMap->SetPixelsAddr(static_cast<uint8_t*>(surfaceBuffer->GetVirAddr()), nativeBuffer,
        pixelMap->GetByteCount(), AllocatorType::DMA_ALLOC, nullptr);
    ANI_DEBUG_LOG("PictureHandlerClient::ReadSurfaceBuffer end");
    return true;
}

void setHandleFromData(BufferHandle *handle, MessageParcel &data)
{
    CHECK_NULL_PTR_RETURN_VOID(handle, "handle is null");
    handle->width = data.ReadInt32();
    ANI_DEBUG_LOG("PictureHandlerClient::ReadBufferHandle width: %{public}d", handle->width);
    handle->stride = data.ReadInt32();
    ANI_DEBUG_LOG("PictureHandlerClient::ReadBufferHandle stride: %{public}d", handle->stride);
    handle->height = data.ReadInt32();
    ANI_DEBUG_LOG("PictureHandlerClient::ReadBufferHandle height: %{public}d", handle->height);
    handle->size = data.ReadInt32();
    ANI_DEBUG_LOG("PictureHandlerClient::ReadBufferHandle size: %{public}d", handle->size);
    handle->format = data.ReadInt32();
    ANI_DEBUG_LOG("PictureHandlerClient::ReadBufferHandle format: %{public}d", handle->format);
    handle->usage = data.ReadUint64();
    handle->phyAddr = data.ReadUint64();
}

bool PictureHandlerClient::ReadBufferHandle(MessageParcel &data, sptr<SurfaceBuffer> &surfaceBuffer)
{
    CHECK_COND_RET(surfaceBuffer != nullptr, false, "surfaceBuffer is nullptr");
    uint32_t reserveFds = 0;
    bool readReserveFdsRet = data.ReadUint32(reserveFds);
    if (reserveFds < 0 || reserveFds > static_cast<uint32_t>(MAX_VALUE)) {
        return false;
    }
    uint32_t reserveInts = 0;
    bool reserveIntsRet = data.ReadUint32(reserveInts);
    if (reserveInts < 0 || reserveInts > static_cast<uint32_t>(MAX_VALUE)) {
        return false;
    }

    size_t handleSize = sizeof(BufferHandle) + (sizeof(int32_t) * (reserveFds + reserveInts));
    BufferHandle *handle = static_cast<BufferHandle *>(malloc(handleSize));
    if (handle == nullptr) {
        ANI_ERR_LOG("PictureHandlerClient::ReadBufferHandle malloc BufferHandle failed");
        return false;
    }
    memset_s(handle, handleSize, 0, handleSize);

    handle->reserveFds = reserveFds;
    handle->reserveInts = reserveInts;
    setHandleFromData(handle, data);

    int32_t fd = RequestBufferHandlerFd(data.ReadInt32());
    ANI_DEBUG_LOG("PictureHandlerClient::ReadBufferHandle fd: %{public}d", fd);
    handle->fd = dup(fd);
    close(fd);
    ANI_DEBUG_LOG("PictureHandlerClient::ReadBufferHandle handle->fd: %{public}d", handle->fd);
    if (readReserveFdsRet) {
        for (uint32_t i = 0; i < reserveFds; i++) {
            int32_t reserveFd = RequestBufferHandlerFd(data.ReadInt32());
            ANI_DEBUG_LOG("PictureHandlerClient::ReadBufferHandle reserve[%{public}d]: %{public}d", i, reserveFd);
            handle->reserve[i] = dup(reserveFd);
            close(reserveFd);
        }
    }

    if (reserveIntsRet) {
        for (uint32_t j = 0; j < handle->reserveInts; j++) {
            handle->reserve[reserveFds + j] = data.ReadInt32();
        }
    }
    surfaceBuffer->SetBufferHandle(handle);
    return true;
}

bool PictureHandlerClient::ReadExifMetadata(MessageParcel &data, std::unique_ptr<Media::Picture> &picture)
{
    CHECK_COND_RET(picture != nullptr, false, "picture is nullptr");
    bool hasExifMetadata = data.ReadBool();
    ANI_DEBUG_LOG("PictureHandlerClient::ReadExifMetadata hasExifMetadata:%{public}d", hasExifMetadata);
    if (!hasExifMetadata) {
        return true;
    }
    ExifMetadata *exifMetadataPtr = ExifMetadata::Unmarshalling(data);
    auto exifMetadata = std::shared_ptr<ExifMetadata>(exifMetadataPtr);
    picture->SetExifMetadata(exifMetadata);
    return true;
}

bool PictureHandlerClient::ReadMaintenanceData(MessageParcel &data, std::unique_ptr<Media::Picture> &picture)
{
    CHECK_COND_RET(picture != nullptr, false, "picture is nullptr");
    bool hasMaintenanceData = data.ReadBool();
    ANI_DEBUG_LOG("PictureHandlerClient::ReadMaintenanceData hasMaintenanceData:%{public}d", hasMaintenanceData);
    if (!hasMaintenanceData) {
        return true;
    }
    sptr<SurfaceBuffer> surfaceBuffer = SurfaceBuffer::Create();
    ReadBufferHandle(data, surfaceBuffer);
    return picture->SetMaintenanceData(surfaceBuffer);
}

int32_t PictureHandlerClient::RequestBufferHandlerFd(const int32_t &fd)
{
    std::string uri = PhotoColumn::PHOTO_REQUEST_PICTURE_BUFFER;
    MediaFileUtils::UriAppendKeyValue(uri, "fd", std::to_string(fd));
    ANI_DEBUG_LOG("PictureHandlerClient::RequestBufferHandlerFd uri: %{public}s", uri.c_str());
    Uri requestUri(uri);
    return UserFileClient::OpenFile(requestUri, MEDIA_FILEMODE_READONLY);
}
} // namespace Media
} // namespace OHOS
