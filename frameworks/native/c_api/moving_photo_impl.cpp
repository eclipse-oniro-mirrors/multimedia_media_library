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

#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <securec.h>
#include "media_pure_file_utils.h"
#include <filesystem>

#include "moving_photo_impl.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "unique_fd.h"
#include "userfilemgr_uri.h"
#include "file_uri.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "userfile_client.h"
#include "dfx_utils.h"
#include "moving_photo_file_utils.h"

using namespace OHOS::Media;
using UniqueFd = OHOS::UniqueFd;
using Uri = OHOS::Uri;

namespace {
enum class MovingPhotoResourceType : int32_t {
    DEFAULT = 0,
    CLOUD_IMAGE_OR_LIVEPHOTO,
    CLOUD_VIDEO_OR_LIVEPHOTO,
};
}

std::shared_ptr<MovingPhoto> MovingPhotoFactory::CreateMovingPhoto(const std::string& uri)
{
    std::shared_ptr<MovingPhotoImpl> impl = std::make_shared<MovingPhotoImpl>(uri);
    CHECK_AND_PRINT_LOG(impl != nullptr, "Failed to create MovingPhotoImpl instance.");

    return impl;
}

MovingPhotoImpl::MovingPhotoImpl(const std::string& imageUri) : imageUri_(imageUri)
{
}

MovingPhotoImpl::~MovingPhotoImpl()
{
    if (arrayBufferData_ != nullptr) {
        free(arrayBufferData_);
        arrayBufferData_ = nullptr;
    }
}

MediaLibrary_ErrorCode MovingPhotoImpl::GetUri(const char** uri)
{
    *uri = imageUri_.c_str();
    MEDIA_INFO_LOG("Moving photo uri = %{public}s", imageUri_.c_str());
    return MEDIA_LIBRARY_OK;
}

MediaLibrary_ErrorCode MovingPhotoImpl::RequestContentWithUris(char* imageUri, char* videoUri)
{
    destImageUri_ = imageUri;
    destVideoUri_ = videoUri;
    MEDIA_DEBUG_LOG("Request content imageUri = %{public}s, video = %{public}s", imageUri, videoUri);
    int32_t ret = RequestContentToSandbox();
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED, "RequestContentToSandbox failed");

    return MEDIA_LIBRARY_OK;
}

MediaLibrary_ErrorCode MovingPhotoImpl::RequestContentWithUri(MediaLibrary_ResourceType resourceType, char* uri)
{
    resourceType_ = resourceType;
    if (resourceType == MEDIA_LIBRARY_IMAGE_RESOURCE) {
        destImageUri_ = uri;
        destVideoUri_ = nullptr;
        MEDIA_DEBUG_LOG("Request content with uri destImageUri_ = %{public}s", destImageUri_);
    } else if (resourceType == MEDIA_LIBRARY_VIDEO_RESOURCE) {
        destImageUri_ = nullptr;
        destVideoUri_ = uri;
        MEDIA_DEBUG_LOG("Request content with uri destVideoUri_ = %{public}s", destVideoUri_);
    } else {
        MEDIA_ERR_LOG("Request content with uri, invalid resourceType");
        destImageUri_ = nullptr;
        destVideoUri_ = nullptr;
        return MEDIA_LIBRARY_PARAMETER_ERROR;
    }
    int32_t ret = RequestContentToSandbox();
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED, "RequestContentToSandbox failed");

    return MEDIA_LIBRARY_OK;
}

MediaLibrary_ErrorCode MovingPhotoImpl::RequestContentWithBuffer(MediaLibrary_ResourceType resourceType,
    const uint8_t** buffer, uint32_t* size)
{
    resourceType_ = resourceType;
    int32_t ret = RequestContentToArrayBuffer();
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED, "RequestContentToArrayBuffer failed");

    if (arrayBufferLength_ <= 0) {
        MEDIA_ERR_LOG("arrayBufferLength equals 0,ivalid buffer length");
        return MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED;
    }

    *buffer = reinterpret_cast<const uint8_t*>(arrayBufferData_);
    *size = arrayBufferLength_;
    MEDIA_INFO_LOG("Request content buffer size = %{public}zu", arrayBufferLength_);
    return MEDIA_LIBRARY_OK;
}

static int32_t CopyFileFromMediaLibrary(int32_t srcFd, int32_t destFd)
{
    constexpr size_t bufferSize = 4096;
    char buffer[bufferSize];
    ssize_t bytesRead;
    ssize_t bytesWritten;
    while ((bytesRead = read(srcFd, buffer, bufferSize)) > 0) {
        bytesWritten = write(destFd, buffer, bytesRead);
        if (bytesWritten != bytesRead) {
            MEDIA_ERR_LOG("Failed to copy file from srcFd=%{public}d to destFd=%{public}d, errno=%{public}d",
                srcFd, destFd, errno);
            return E_HAS_FS_ERROR;
        }
    }

    if (bytesRead < 0) {
        MEDIA_ERR_LOG("Failed to read from srcFd=%{public}d, errno=%{public}d", srcFd, errno);
        return E_HAS_FS_ERROR;
    }
    MEDIA_INFO_LOG("Copy file from media library done");
    return E_OK;
}

static int32_t WriteToSandboxUri(int32_t srcFd, std::string& sandboxUri,
    MovingPhotoResourceType type)
{
    UniqueFd srcUniqueFd(srcFd);
    OHOS::AppFileService::ModuleFileUri::FileUri fileUri(sandboxUri);
    std::string destPath = fileUri.GetRealPath();
    MEDIA_INFO_LOG("Dest real path = %{public}s", destPath.c_str());
    if (!MediaPureFileUtils::IsFileExists(destPath) && !MediaFileUtils::CreateFile(destPath)) {
        MEDIA_ERR_LOG("Create empty dest file in sandbox failed, path:%{public}s", destPath.c_str());
        return E_HAS_FS_ERROR;
    }

    if (type == MovingPhotoResourceType::CLOUD_IMAGE_OR_LIVEPHOTO) {
        return MovingPhotoFileUtils::ConvertToMovingPhoto(srcUniqueFd.Get(), destPath, "", "");
    } else if (type == MovingPhotoResourceType::CLOUD_VIDEO_OR_LIVEPHOTO) {
        return MovingPhotoFileUtils::ConvertToMovingPhoto(srcUniqueFd.Get(), "", destPath, "");
    }

    int32_t destFd = MediaFileUtils::OpenFile(destPath, MEDIA_FILEMODE_READWRITE);
    if (destFd < 0) {
        MEDIA_ERR_LOG("Open dest file failed, error: %{public}d", errno);
        return E_HAS_FS_ERROR;
    }
    UniqueFd destUniqueFd(destFd);

    if (ftruncate(destUniqueFd.Get(), 0) == -1) {
        MEDIA_ERR_LOG("Truncate old file in sandbox failed, error:%{public}d", errno);
        return E_HAS_FS_ERROR;
    }
    return CopyFileFromMediaLibrary(srcUniqueFd.Get(), destUniqueFd.Get());
}

int32_t MovingPhotoImpl::RequestContentToSandbox()
{
    std::string movingPhotoUri = imageUri_;
    if (sourceMode_ == SourceMode::ORIGINAL_MODE) {
        MediaFileUtils::UriAppendKeyValue(movingPhotoUri, CONST_MEDIA_OPERN_KEYWORD, CONST_SOURCE_REQUEST);
    }

    if (destImageUri_ && strlen(destImageUri_) > 0) {
        MEDIA_DEBUG_LOG("Sandbox image movingPhotoUri = %{public}s, destImageUri_ = %{public}s",
            movingPhotoUri.c_str(), destImageUri_);
        int32_t imageFd = OpenReadOnlyFile(movingPhotoUri, true);
        CHECK_AND_RETURN_RET_LOG(HandleFd(imageFd), imageFd, "Open source image file failed");
        MovingPhotoResourceType resourceType = MovingPhotoFileUtils::IsLivePhoto(imageFd)
            ? MovingPhotoResourceType::CLOUD_IMAGE_OR_LIVEPHOTO
            : MovingPhotoResourceType::DEFAULT;
        std::string imageUri(destImageUri_);
        int32_t ret = WriteToSandboxUri(imageFd, imageUri, resourceType);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Write image to sandbox failed");
    }
    if (destVideoUri_ && strlen(destVideoUri_) > 0) {
        MEDIA_DEBUG_LOG("Sandbox video movingPhotoUri = %{public}s, destVideoUri_ = %{public}s",
            movingPhotoUri.c_str(), destVideoUri_);
        int32_t videoFd = OpenReadOnlyFile(movingPhotoUri, false);
        CHECK_AND_RETURN_RET_LOG(HandleFd(videoFd), videoFd, "Open source video file failed");
        MovingPhotoResourceType resourceType = MovingPhotoFileUtils::IsLivePhoto(videoFd)
            ? MovingPhotoResourceType::CLOUD_VIDEO_OR_LIVEPHOTO
            : MovingPhotoResourceType::DEFAULT;
        std::string videoUri(destVideoUri_);
        int32_t ret = WriteToSandboxUri(videoFd, videoUri, resourceType);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Write video to sandbox failed");
    }
    MEDIA_INFO_LOG("Request content to sandbox done");
    return E_OK;
}

int32_t MovingPhotoImpl::OpenReadOnlyFile(const std::string& uri, bool isReadImage)
{
    CHECK_AND_RETURN_RET_LOG(!uri.empty(), E_ERR, "Failed to open read only file, uri is empty");

    std::string curUri = uri;
    bool isMediaLibUri = MediaFileUtils::IsMediaLibraryUri(uri);
    MEDIA_DEBUG_LOG("isMediaLibUri = %{public}d, isReadImage = %{public}d", isMediaLibUri, isReadImage);
    if (!isMediaLibUri) {
        std::vector<std::string> uris;
        if (!MediaFileUtils::SplitMovingPhotoUri(uri, uris)) {
            MEDIA_ERR_LOG("Failed to open read only file, split moving photo failed");
            return E_ERR;
        }
        if (uris.size() < MOVING_PHOTO_URIS_SIZE) {
            MEDIA_ERR_LOG("Failed to check uris size, size = %{public}zu", uris.size());
            return E_ERR;
        }
        curUri = uris[isReadImage ? MOVING_PHOTO_IMAGE_POS : MOVING_PHOTO_VIDEO_POS];
    }
    return isReadImage ? OpenReadOnlyImage(curUri, isMediaLibUri) : OpenReadOnlyVideo(curUri, isMediaLibUri);
}

static std::string NormalizePath(const std::string& path)
{
    std::error_code errorCode;
    std::filesystem::path canonicalPath = std::filesystem::canonical(path, errorCode);
    CHECK_AND_RETURN_RET_LOG(!errorCode, "", "Failed to canonicalize path: %{public}s, message: %{public}s",
        DfxUtils::GetSafePath(path).c_str(), errorCode.message().c_str());
    return canonicalPath.string();
}

int32_t MovingPhotoImpl::OpenReadOnlyImage(const std::string& imageUri, bool isMediaLibUri)
{
    if (isMediaLibUri) {
        Uri uri(imageUri);
        return UserFileClient::OpenFile(uri, MEDIA_FILEMODE_READONLY);
    }
    OHOS::AppFileService::ModuleFileUri::FileUri fileUri(imageUri);
    std::string realPath = fileUri.GetRealPath();
    std::string normalizedRealPath = NormalizePath(realPath);
    CHECK_AND_RETURN_RET_LOG(!normalizedRealPath.empty(), E_ERR, "Failed to normalize path");
    int32_t fd = open(normalizedRealPath.c_str(), O_RDONLY);
    CHECK_AND_RETURN_RET_LOG(fd >= 0, E_ERR, "Failed to open read only image file");

    return fd;
}

bool MovingPhotoImpl::HandleFd(int32_t& fd)
{
    if (fd == E_ERR) {
        fd = E_HAS_FS_ERROR;
        return false;
    } else if (fd < 0) {
        MEDIA_ERR_LOG("Open failed due to OpenFile failure, error: %{public}d", fd);
        return false;
    }
    return true;
}

int32_t MovingPhotoImpl::OpenReadOnlyVideo(const std::string& videoUri, bool isMediaLibUri)
{
    if (isMediaLibUri) {
        std::string openVideoUri = videoUri;
        MediaFileUtils::UriAppendKeyValue(openVideoUri, CONST_MEDIA_MOVING_PHOTO_OPRN_KEYWORD,
            CONST_OPEN_MOVING_PHOTO_VIDEO);
        Uri uri(openVideoUri);
        return UserFileClient::OpenFile(uri, MEDIA_FILEMODE_READONLY);
    }
    OHOS::AppFileService::ModuleFileUri::FileUri fileUri(videoUri);
    std::string realPath = fileUri.GetRealPath();
    int32_t fd = open(realPath.c_str(), O_RDONLY);
    if (fd < 0) {
        MEDIA_ERR_LOG("Failed to open read only video file, errno: %{public}d", errno);
        return -1;
    }
    return fd;
}

int32_t MovingPhotoImpl::AcquireFdForArrayBuffer()
{
    int32_t fd = 0;
    std::string movingPhotoUri = imageUri_;
    switch (resourceType_) {
        case MediaLibrary_ResourceType::MEDIA_LIBRARY_IMAGE_RESOURCE: {
            fd = OpenReadOnlyFile(movingPhotoUri, true);
            CHECK_AND_RETURN_RET_LOG(HandleFd(fd), fd, "Open source image file failed");
            return fd;
        }
        case MediaLibrary_ResourceType::MEDIA_LIBRARY_VIDEO_RESOURCE: {
            fd = OpenReadOnlyFile(movingPhotoUri, false);
            CHECK_AND_RETURN_RET_LOG(HandleFd(fd), fd, "Open source video file failed");
            return fd;
        }
        default:
            MEDIA_ERR_LOG("Invalid resource type: %{public}d", static_cast<int32_t>(resourceType_));
            return -EINVAL;
    }
}

static int32_t ProcessLivePhotoArrayBuffer(int32_t fd, int32_t resourceType,
    void*& arrayBufferData, size_t& arrayBufferLength)
{
    int64_t imageSize = 0;
    int64_t videoSize = 0;
    int64_t extraDataSize = 0;
    int32_t err = MovingPhotoFileUtils::GetMovingPhotoDetailedSize(
        fd, imageSize, videoSize, extraDataSize);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, E_HAS_FS_ERROR, "Failed to get moving photo detailed size");

    size_t fileSize = 0;
    off_t readOffset = 0;
    switch (resourceType) {
        case MediaLibrary_ResourceType::MEDIA_LIBRARY_IMAGE_RESOURCE:
            fileSize = static_cast<size_t>(imageSize);
            readOffset = 0;
            break;
        case MediaLibrary_ResourceType::MEDIA_LIBRARY_VIDEO_RESOURCE:
            fileSize = static_cast<size_t>(videoSize);
            readOffset = imageSize;
            break;
        default:
            MEDIA_ERR_LOG("Invalid resource type: %{public}d", static_cast<int32_t>(resourceType));
            return E_HAS_FS_ERROR;
    }

    off_t ret = lseek(fd, readOffset, SEEK_SET);
    if (ret < 0) {
        MEDIA_ERR_LOG("Failed to seek to offset %{public}lld, error: %{public}d",
            static_cast<long long>(readOffset), errno);
        return E_HAS_FS_ERROR;
    }

    arrayBufferData = malloc(fileSize);
    if (!arrayBufferData) {
        MEDIA_ERR_LOG("Failed to malloc array buffer, resource type is %{public}d",
            static_cast<int32_t>(resourceType));
        return E_HAS_FS_ERROR;
    }
    size_t readBytes = static_cast<size_t>(read(fd, arrayBufferData, fileSize));
    if (readBytes != fileSize) {
        MEDIA_ERR_LOG("read file failed, read bytes is %{public}zu, actual length is %{public}zu,"
            " error: %{public}d", readBytes, fileSize, errno);
        free(arrayBufferData);
        arrayBufferData = nullptr;
        return E_HAS_FS_ERROR;
    }
    arrayBufferLength = fileSize;
    return E_OK;
}

int32_t MovingPhotoImpl::RequestContentToArrayBuffer()
{
    int32_t fd = AcquireFdForArrayBuffer();
    CHECK_AND_RETURN_RET_LOG(fd >= 0, fd, "Acquire fd for arraybuffer failed");

    UniqueFd uniqueFd(fd);

    if (MovingPhotoFileUtils::IsLivePhoto(uniqueFd.Get())) {
        return ProcessLivePhotoArrayBuffer(uniqueFd.Get(), resourceType_,
            arrayBufferData_, arrayBufferLength_);
    } else {
        off_t fileLen = lseek(uniqueFd.Get(), 0, SEEK_END);
        if (fileLen < 0) {
            MEDIA_ERR_LOG("Failed to get file length, error: %{public}d", errno);
            return E_HAS_FS_ERROR;
        }

        off_t ret = lseek(uniqueFd.Get(), 0, SEEK_SET);
        if (ret < 0) {
            MEDIA_ERR_LOG("Failed to reset file offset, error: %{public}d", errno);
            return E_HAS_FS_ERROR;
        }

        if (static_cast<uint64_t>(fileLen) > static_cast<uint64_t>(SIZE_MAX)) {
            MEDIA_ERR_LOG("File length is too large to fit in a size_t, length: %{public}zu",
                static_cast<size_t>(fileLen));
            return E_HAS_FS_ERROR;
        }

        size_t fileSize = static_cast<size_t>(fileLen);
        arrayBufferData_ = malloc(fileSize);
        if (!arrayBufferData_) {
            MEDIA_ERR_LOG("Failed to malloc array buffer, moving photo uri is %{public}s, resource type is %{public}d",
                imageUri_.c_str(), static_cast<int32_t>(resourceType_));
            return E_HAS_FS_ERROR;
        }

        size_t readBytes = static_cast<size_t>(read(uniqueFd.Get(), arrayBufferData_, fileSize));
        if (readBytes != fileSize) {
            MEDIA_ERR_LOG("read file failed, read bytes is %{public}zu, actual length is %{public}zu,"
                " error: %{public}d", readBytes, fileSize, errno);
            free(arrayBufferData_);
            arrayBufferData_ = nullptr;
            return E_HAS_FS_ERROR;
        }
        arrayBufferLength_ = fileSize;
    }
    return E_OK;
}
