/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "moving_photo_impl.h"

#include <fcntl.h>
#include <unistd.h>

#include "directory_ex.h"
#include "file_uri.h"
#include "media_file_utils.h"
#include "medialibrary_errno.h"
#include "userfile_client.h"
#include "userfile_manager_types.h"

using namespace std;

namespace OHOS {
namespace Media {
FfiMovingPhotoImpl::FfiMovingPhotoImpl(const string& photoUri, SourceMode sourceMode)
{
    this->photoUri_ = photoUri;
    this->sourceMode_ = sourceMode;
}

string FfiMovingPhotoImpl::GetUri()
{
    return photoUri_;
}

SourceMode FfiMovingPhotoImpl::GetSourceMode()
{
    return sourceMode_;
}

void FfiMovingPhotoImpl::SetSourceMode(SourceMode sourceMode)
{
    this->sourceMode_ = sourceMode;
}

static int32_t OpenReadOnlyImage(const std::string& imageUri, bool isMediaLibUri)
{
    if (isMediaLibUri) {
        Uri uri(imageUri);
        return UserFileClient::OpenFile(uri, MEDIA_FILEMODE_READONLY);
    }
    AppFileService::ModuleFileUri::FileUri fileUri(imageUri);
    std::string realPath = fileUri.GetRealPath();
    int32_t fd = open(realPath.c_str(), O_RDONLY);
    if (fd < 0) {
        LOGE("Failed to open read only image file");
        return E_ERR;
    }
    return fd;
}

static int32_t OpenReadOnlyVideo(const std::string& videoUri, bool isMediaLibUri)
{
    if (isMediaLibUri) {
        std::string openVideoUri = videoUri;
        MediaFileUtils::UriAppendKeyValue(openVideoUri, CONST_MEDIA_MOVING_PHOTO_OPRN_KEYWORD,
            CONST_OPEN_MOVING_PHOTO_VIDEO);
        Uri uri(openVideoUri);
        return UserFileClient::OpenFile(uri, MEDIA_FILEMODE_READONLY);
    }
    AppFileService::ModuleFileUri::FileUri fileUri(videoUri);
    std::string realPath = fileUri.GetRealPath();
    int32_t fd = open(realPath.c_str(), O_RDONLY);
    if (fd < 0) {
        LOGE("Failed to open read only video file");
        return E_ERR;
    }
    return fd;
}

static bool HandleFd(int32_t& fd)
{
    if (fd == E_ERR) {
        fd = E_HAS_FS_ERROR;
        return false;
    } else if (fd < 0) {
        LOGE("Open failed due to OpenFile failure, error: %{public}d", fd);
        return false;
    }
    return true;
}

int32_t FfiMovingPhotoImpl::OpenReadOnlyFile(const string& uri, bool isReadImage)
{
    if (uri.empty()) {
        LOGE("Failed to open read only file, uri is empty");
        return E_ERR;
    }
    std::string curUri = uri;
    bool isMediaLibUri = MediaFileUtils::IsMediaLibraryUri(uri);
    if (!isMediaLibUri) {
        std::vector<std::string> uris;
        if (!MediaFileUtils::SplitMovingPhotoUri(uri, uris)) {
            LOGE("Failed to open read only file, split moving photo failed");
            return E_ERR;
        }
        curUri = uris[isReadImage ? MOVING_PHOTO_IMAGE_POS : MOVING_PHOTO_VIDEO_POS];
    }
    return isReadImage ? OpenReadOnlyImage(curUri, isMediaLibUri) : OpenReadOnlyVideo(curUri, isMediaLibUri);
}

int32_t FfiMovingPhotoImpl::OpenReadOnlyLivePhoto(const string& destLivePhotoUri)
{
    if (destLivePhotoUri.empty()) {
        LOGE("Failed to open read only file, uri is empty");
        return E_ERR;
    }
    if (MediaFileUtils::IsMediaLibraryUri(destLivePhotoUri)) {
        string livePhotoUri = destLivePhotoUri;
        MediaFileUtils::UriAppendKeyValue(livePhotoUri, CONST_MEDIA_MOVING_PHOTO_OPRN_KEYWORD,
            CONST_OPEN_PRIVATE_LIVE_PHOTO);
        Uri uri(livePhotoUri);
        return UserFileClient::OpenFile(uri, MEDIA_FILEMODE_READONLY);
    }
    return E_ERR;
}

static int32_t CopyFileFromMediaLibrary(int32_t srcFd, int32_t destFd)
{
    size_t bufferSize = 4096;
    char buffer[bufferSize];
    ssize_t bytesRead;
    ssize_t bytesWritten;
    while ((bytesRead = read(srcFd, buffer, bufferSize)) > 0) {
        bytesWritten = write(destFd, buffer, bytesRead);
        if (bytesWritten != bytesRead) {
            LOGE("Failed to copy file from srcFd=%{public}d to destFd=%{public}d, errno=%{public}d",
                srcFd, destFd, errno);
            return E_HAS_FS_ERROR;
        }
    }

    if (bytesRead < 0) {
        LOGE("Failed to read from srcFd=%{public}d, errno=%{public}d", srcFd, errno);
        return E_HAS_FS_ERROR;
    }
    return E_OK;
}

static int32_t WriteToSandboxUri(int32_t srcFd, const string& sandboxUri)
{
    UniqueFd srcUniqueFd(srcFd);
    AppFileService::ModuleFileUri::FileUri fileUri(sandboxUri);
    string destPath = fileUri.GetRealPath();
    if (!MediaFileUtils::IsFileExists(destPath) && !MediaFileUtils::CreateFile(destPath)) {
        LOGE("Create empty dest file in sandbox failed, path:%{private}s", destPath.c_str());
        return E_HAS_FS_ERROR;
    }
    int32_t destFd = MediaFileUtils::OpenFile(destPath, MEDIA_FILEMODE_READWRITE);
    if (destFd < 0) {
        LOGE("Open dest file failed, error: %{public}d", errno);
        return E_HAS_FS_ERROR;
    }
    UniqueFd destUniqueFd(destFd);
    if (ftruncate(destUniqueFd.Get(), 0) == -1) {
        LOGE("Truncate old file in sandbox failed, error:%{public}d", errno);
        return E_HAS_FS_ERROR;
    }
    return CopyFileFromMediaLibrary(srcUniqueFd.Get(), destUniqueFd.Get());
}

static int32_t RequestContentToSandbox(const string &destImageUri, const string &destVideoUri,
    string movingPhotoUri, SourceMode sourceMode)
{
    if (sourceMode == SourceMode::ORIGINAL_MODE) {
        MediaFileUtils::UriAppendKeyValue(movingPhotoUri, CONST_MEDIA_OPERN_KEYWORD, CONST_SOURCE_REQUEST);
    }
    if (!destImageUri.empty()) {
        int32_t imageFd = FfiMovingPhotoImpl::OpenReadOnlyFile(movingPhotoUri, true);
        if (!HandleFd(imageFd)) {
            LOGE("Open source image file failed");
            return imageFd;
        }
        int32_t ret = WriteToSandboxUri(imageFd, destImageUri);
        if (ret != E_OK) {
            LOGE("Write image to sandbox failed");
            return ret;
        }
    }
    if (!destVideoUri.empty()) {
        int32_t videoFd = FfiMovingPhotoImpl::OpenReadOnlyFile(movingPhotoUri, false);
        if (!HandleFd(videoFd)) {
            LOGE("Open source video file failed");
            return videoFd;
        }
        int32_t ret = WriteToSandboxUri(videoFd, destVideoUri);
        if (ret != E_OK) {
            LOGE("Write video to sandbox failed");
            return ret;
        }
    }
    return E_OK;
}

static bool IsValidResourceType(int32_t resourceType)
{
    // public API only support IMAGE_RESOURCE/VIDEO_RESOURCE
    return (resourceType == static_cast<int>(ResourceType::IMAGE_RESOURCE) ||
        resourceType == static_cast<int>(ResourceType::VIDEO_RESOURCE));
}

void FfiMovingPhotoImpl::RequestContent(char* imageFileUri, char* videoFileUri, int32_t &errCode)
{
    // write both image and video to sandbox
    string destImageUri(imageFileUri);
    string destVideoUri(videoFileUri);
    int32_t ret = RequestContentToSandbox(destImageUri, destVideoUri, photoUri_, sourceMode_);
    if (ret != E_OK) {
        errCode = static_cast<int32_t>(MediaLibraryNapiUtils::TransErrorCode("RequestContent", ret));
    }
}

void FfiMovingPhotoImpl::RequestContent(int32_t resourceType, char* fileUri, int32_t &errCode)
{
    string destImageUri = "";
    string destVideoUri = "";
    if (!IsValidResourceType(resourceType)) {
        LOGE("Invalid resource type");
        errCode = OHOS_INVALID_PARAM_CODE;
        return;
    }
    if (resourceType == static_cast<int>(ResourceType::IMAGE_RESOURCE)) {
        destImageUri = string(fileUri);
    } else {
        destVideoUri = string(fileUri);
    }
    int32_t ret = RequestContentToSandbox(destImageUri, destVideoUri, photoUri_, sourceMode_);
    if (ret != E_OK) {
        errCode = static_cast<int32_t>(MediaLibraryNapiUtils::TransErrorCode("RequestContent", ret));
    }
}

static int32_t AcquireFdForArrayBuffer(string movingPhotoUri, SourceMode sourceMode, ResourceType resourceType)
{
    int32_t fd = 0;
    if (sourceMode == SourceMode::ORIGINAL_MODE) {
        MediaFileUtils::UriAppendKeyValue(movingPhotoUri, CONST_MEDIA_OPERN_KEYWORD, CONST_SOURCE_REQUEST);
    }
    switch (resourceType) {
        case ResourceType::IMAGE_RESOURCE:
            fd = FfiMovingPhotoImpl::OpenReadOnlyFile(movingPhotoUri, true);
            if (!HandleFd(fd)) {
                LOGE("Open source image file failed");
            }
            return fd;
        case ResourceType::VIDEO_RESOURCE:
            fd = FfiMovingPhotoImpl::OpenReadOnlyFile(movingPhotoUri, false);
            if (!HandleFd(fd)) {
                LOGE("Open source video file failed");
            }
            return fd;
        default:
            LOGE("Invalid resource type: %{public}d", static_cast<int32_t>(resourceType));
            return -EINVAL;
    }
}

static int32_t RequestContentToArrayBuffer(int32_t resourceType, string movingPhotoUri,
    SourceMode sourceMode, CArrUI8 &result)
{
    int32_t fd = AcquireFdForArrayBuffer(movingPhotoUri, sourceMode, static_cast<ResourceType>(resourceType));
    if (fd < 0) {
        return fd;
    }
    UniqueFd uniqueFd(fd);
    off_t fileLen = lseek(uniqueFd.Get(), 0, SEEK_END);
    if (fileLen < 0) {
        LOGE("Failed to get file length, error: %{public}d", errno);
        return E_HAS_FS_ERROR;
    }
    off_t ret = lseek(uniqueFd.Get(), 0, SEEK_SET);
    if (ret < 0) {
        LOGE("Failed to reset file offset, error: %{public}d", errno);
        return E_HAS_FS_ERROR;
    }
    size_t fileSize = static_cast<size_t>(fileLen);
    void* arrayBufferData = malloc(fileSize);
    if (!arrayBufferData) {
        LOGE("Failed to malloc array buffer data, moving photo uri is %{public}s, resource type is %{public}d",
            movingPhotoUri.c_str(), resourceType);
        return E_HAS_FS_ERROR;
    }
    size_t readBytes = static_cast<size_t>(read(uniqueFd.Get(), arrayBufferData, fileSize));
    if (readBytes != fileSize) {
        LOGE("read file failed, read bytes is %{public}zu, actual length is %{public}zu, "
            "error: %{public}d", readBytes, fileSize, errno);
        free(arrayBufferData);
        return E_HAS_FS_ERROR;
    }
    if (fileSize > 0) {
        if (arrayBufferData == nullptr) {
            LOGE("get arrayBuffer failed.");
            return E_HAS_FS_ERROR;
        }
        result.head = static_cast<uint8_t*>(arrayBufferData);
        result.size = static_cast<int64_t>(fileSize);
    }
    return E_OK;
}

CArrUI8 FfiMovingPhotoImpl::RequestContent(int32_t resourceType, int32_t &errCode)
{
    CArrUI8 result = {
        .head = nullptr,
        .size  = 0
    };
    if (!IsValidResourceType(resourceType)) {
        LOGE("Invalid resource type");
        errCode = OHOS_INVALID_PARAM_CODE;
        return result;
    }
    int32_t ret = RequestContentToArrayBuffer(resourceType, photoUri_, sourceMode_, result);
    if (ret != E_OK) {
        errCode = static_cast<int32_t>(MediaLibraryNapiUtils::TransErrorCode("RequestContent", ret));
    }
    return result;
}
}
}