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
#define MLOG_TAG "MovingPhotoAni"
#include "moving_photo_ani.h"

#include <cstddef>
#include <iostream>
#include <array>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <securec.h>

#include "access_token.h"
#include "accesstoken_kit.h"
#include "ani_class_name.h"
#include "file_uri.h"
#include "ipc_skeleton.h"
#include "medialibrary_ani_utils.h"
#include "medialibrary_business_code.h"
#include "media_asset_rdbstore.h"
#include "media_file_uri.h"
#include "media_file_utils.h"
#include "moving_photo_call_transcoder.h"
#include "moving_photo_file_utils.h"
#include "permission_utils.h"
#include "progress_handler.h"
#include "request_content_vo.h"
#include "unique_fd.h"
#include "user_define_ipc_client.h"
#include "userfile_client.h"
#include "userfile_manager_types.h"
#include "ability_context.h"
#include "application_context.h"
#include "media_call_transcode.h"
#include "interop_js/arkts_esvalue.h"
#include "interop_js/arkts_interop_js_api.h"
#include "transfer_utils.h"
#include "ani_transfer_lib_manager.h"
namespace OHOS {
namespace Media {
using CreateTransferMovingPhotoFn = napi_value (*)(napi_env,
    std::string, int, OHOS::Media::TransferUtils::TransferMovingPhotoParam);
using GetPropertyMovingPhotoNapiFn = bool (*)(OHOS::Media::MovingPhotoNapi*, std::string&, int&,
    OHOS::Media::TransferUtils::TransferMovingPhotoParam&);
std::mutex LibManager::mutex_;
std::shared_ptr<LibHandle> LibManager::instance_ = nullptr;
namespace {
static const std::string URI_TYPE = "uriType";
static const std::string TYPE_PHOTOS = "1";
static const std::string MULTI_USER_URI_FLAG = "user=";
enum class MovingPhotoResourceType : int32_t {
    DEFAULT = 0,
    CLOUD_IMAGE,
    CLOUD_VIDEO,
    CLOUD_LIVE_PHOTO,
    CLOUD_METADATA,
};
} // namespace

ani_status MovingPhotoAni::Init(ani_env *env)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    static const char *className = PAH_ANI_CLASS_MOVING_PHOTO_HANDLE.c_str();
    ani_class cls;
    ani_status status = env->FindClass(className, &cls);
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to find class: %{public}s", className);
        return status;
    }
    std::array methods = {
        ani_native_function {"requestContentByImageFileAndVideoFile", nullptr,
            reinterpret_cast<void *>(MovingPhotoAni::RequestContentByImageFileAndVideoFile)},
        ani_native_function {"requestContentByResourceTypeAndFile", nullptr,
            reinterpret_cast<void *>(MovingPhotoAni::RequestContentByResourceTypeAndFile)},
        ani_native_function {"requestContentByResourceType", nullptr,
            reinterpret_cast<void *>(MovingPhotoAni::RequestContentByResourceType)},
        ani_native_function {"getUri", nullptr, reinterpret_cast<void *>(MovingPhotoAni::GetUri)},
        ani_native_function {"isVideoReadyInner", nullptr, reinterpret_cast<void *>(MovingPhotoAni::IsVideoReady)},
    };

    status = env->Class_BindNativeMethods(cls, methods.data(), methods.size());
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to bind native methods to: %{public}s", className);
        return status;
    }

    std::array staticMethods = {
        ani_native_function {"transferToDynamicMovingPhoto", nullptr,
            reinterpret_cast<void *>(TransferToDynamicMovingPhoto)},
        ani_native_function {"transferToStaticMovingPhoto", nullptr,
            reinterpret_cast<void *>(TransferToStaticMovingPhoto)},
    };
    status = env->Class_BindStaticNativeMethods(cls, staticMethods.data(), staticMethods.size());
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to bind static native methods to: %{public}s", className);
        return status;
    }
    return ANI_OK;
}

ani_object MovingPhotoAni::NewMovingPhotoAni(ani_env *env, const string& photoUri, SourceMode sourceMode,
    MovingPhotoParam &movingPhotoParam)
{
    ani_string photoUriAni;
    if (ANI_OK != MediaLibraryAniUtils::ToAniString(env, photoUri, photoUriAni)) {
        ANI_ERR_LOG("ToAniString photoUri fail");
        return nullptr;
    }
    ani_object movingPhotoObject = Constructor(env, nullptr, photoUriAni);
    MovingPhotoAni* movingPhotoAni = Unwrap(env, movingPhotoObject);
    if (movingPhotoAni == nullptr) {
        ANI_ERR_LOG("movingPhotoAni is nullptr");
        return nullptr;
    }
    movingPhotoAni->SetSourceMode(sourceMode);
    movingPhotoAni->SetRequestId(movingPhotoParam.requestId);
    movingPhotoAni->SetCompatibleMode(movingPhotoParam.compatibleMode);
    movingPhotoAni->SetProgressHandlerRef(movingPhotoParam.progressHandlerRef);
    movingPhotoAni->SetThreadsafeFunction(movingPhotoParam.threadsafeFunction);
    ani_vm *etsVm {};
    if (env == nullptr || env->GetVM(&etsVm) != ANI_OK) {
        ANI_ERR_LOG("Failed to get ani_vm");
        return nullptr;
    }
    movingPhotoAni->SetEtsVm(etsVm);
    return movingPhotoObject;
}

void BufferTranscodeRequestContent(int32_t fd, MovingPhotoAsyncContext* context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    UniqueFd uniqueFd(fd);
    off_t fileLen = lseek(uniqueFd.Get(), 0, SEEK_END);
    if (fileLen < 0) {
        ANI_ERR_LOG("Failed to get file length, error: %{public}d", errno);
        context->SaveError(E_HAS_FS_ERROR);
        return;
    }

    off_t ret = lseek(uniqueFd.Get(), 0, SEEK_SET);
    if (ret < 0) {
        ANI_ERR_LOG("Failed to reset file offset, error: %{public}d", errno);
        context->SaveError(E_HAS_FS_ERROR);
        return;
    }

    size_t fileSize = static_cast<size_t>(fileLen);
    context->arrayBufferData = malloc(fileSize);
    if (!context->arrayBufferData) {
        ANI_ERR_LOG("Failed to malloc array buffer data, moving photo uri is %{public}s, resource type is %{public}d",
            context->movingPhotoUri.c_str(), static_cast<int32_t>(context->resourceType));
            context->error = JS_INNER_FAIL;
        return;
    }
    size_t readBytes = static_cast<size_t>(read(uniqueFd.Get(), context->arrayBufferData, fileSize));
    if (readBytes != fileSize) {
        ANI_ERR_LOG("read file failed, read bytes is %{public}zu, actual length is %{public}zu, "
            "error: %{public}d", readBytes, fileSize, errno);
        free(context->arrayBufferData);
        context->arrayBufferData = nullptr;
        context->SaveError(E_HAS_FS_ERROR);
        return;
    }
    context->arrayBufferLength = fileSize;
    return;
}

void MovingPhotoAni::SubRequestContent(int32_t fd, MovingPhotoAsyncContext* context)
{
    if (context->position == POSITION_CLOUD) {
        return RequestCloudContentArrayBuffer(fd, context);
    }
    BufferTranscodeRequestContent(fd, context);
}

ani_object MovingPhotoAni::Constructor(ani_env *env, [[maybe_unused]] ani_class clazz, ani_string photoUriAni)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    std::string photoUri;
    if (ANI_OK != MediaLibraryAniUtils::GetString(env, photoUriAni, photoUri)) {
        ANI_ERR_LOG("Failed to get photoUri");
        return nullptr;
    }
    unique_ptr<MovingPhotoAni> obj = make_unique<MovingPhotoAni>(photoUri);
    CHECK_COND_RET(obj != nullptr, nullptr, "obj is nullptr");
    static const char *className = PAH_ANI_CLASS_MOVING_PHOTO_HANDLE.c_str();
    ani_class cls;
    if (ANI_OK != env->FindClass(className, &cls)) {
        ANI_ERR_LOG("Failed to find class: %{public}s", className);
        return nullptr;
    }

    ani_method ctor;
    if (ANI_OK != env->Class_FindMethod(cls, "<ctor>", "l:", &ctor)) {
        ANI_ERR_LOG("Failed to find method: %{public}s", "ctor");
        return nullptr;
    }

    ani_object movingPhotoObject;
    if (ANI_OK != env->Object_New(cls, ctor, &movingPhotoObject, reinterpret_cast<ani_long>(obj.get()))) {
        ANI_ERR_LOG("New MovingPhoto Fail");
        return nullptr;
    }
    (void)obj.release();
    return movingPhotoObject;
}

MovingPhotoAni* MovingPhotoAni::Unwrap(ani_env *env, ani_object object)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    ani_long movingPhoto;
    if (ANI_OK != env->Object_GetFieldByName_Long(object, "nativeMovingPhoto", &movingPhoto)) {
        return nullptr;
    }
    return reinterpret_cast<MovingPhotoAni*>(movingPhoto);
}

std::string MovingPhotoAni::GetUriInner()
{
    return photoUri_;
}

SourceMode MovingPhotoAni::GetSourceMode()
{
    return sourceMode_;
}

void MovingPhotoAni::SetSourceMode(SourceMode sourceMode)
{
    sourceMode_ = sourceMode;
}

CompatibleMode MovingPhotoAni::GetCompatibleMode()
{
    return compatibleMode_;
}

void MovingPhotoAni::SetCompatibleMode(const CompatibleMode compatibleMode)
{
    compatibleMode_ = compatibleMode;
}

ani_ref MovingPhotoAni::GetProgressHandlerRef()
{
    return progressHandlerRef_;
}

void MovingPhotoAni::SetProgressHandlerRef(ani_ref &progressHandlerRef)
{
    progressHandlerRef_ = progressHandlerRef;
}

ani_vm *MovingPhotoAni::GetEtsVm() const
{
    return etsVm_;
}

void MovingPhotoAni::SetEtsVm(ani_vm *etsVm)
{
    etsVm_ = etsVm;
}

ThreadFunctionOnProgress MovingPhotoAni::GetThreadsafeFunction() const
{
    return threadsafeFunction_;
}

void MovingPhotoAni::SetThreadsafeFunction(ThreadFunctionOnProgress threadsafeFunction)
{
    threadsafeFunction_ = threadsafeFunction;
}

std::string MovingPhotoAni::GetRequestId()
{
    return requestId_;
}

void MovingPhotoAni::SetRequestId(const std::string &requestId)
{
    requestId_ = requestId;
}

int32_t MovingPhotoAni::GetFdFromUri(const std::string &uri)
{
    AppFileService::ModuleFileUri::FileUri destUri(uri);
    string destPath = destUri.GetRealPath();
    if (!MediaFileUtils::IsFileExists(destPath) && !MediaFileUtils::CreateFile(destPath)) {
        ANI_ERR_LOG("Create empty dest file in sandbox failed, path:%{private}s", destPath.c_str());
        return E_ERR;
    }
    return MediaFileUtils::OpenFile(destPath, MEDIA_FILEMODE_READWRITE);
}

static int32_t OpenReadOnlyVideo(const std::string& videoUri, bool isMediaLibUri, int32_t position)
{
    if (isMediaLibUri) {
        std::string openVideoUri = videoUri;
        if (position == POSITION_CLOUD) {
            MediaFileUtils::UriAppendKeyValue(openVideoUri, MEDIA_MOVING_PHOTO_OPRN_KEYWORD,
                OPEN_MOVING_PHOTO_VIDEO_CLOUD);
        } else {
            MediaFileUtils::UriAppendKeyValue(openVideoUri, MEDIA_MOVING_PHOTO_OPRN_KEYWORD,
                OPEN_MOVING_PHOTO_VIDEO);
        }
        Uri uri(openVideoUri);
        return UserFileClient::OpenFile(uri, MEDIA_FILEMODE_READONLY);
    }
    OHOS::AppFileService::ModuleFileUri::FileUri fileUri(videoUri);
    std::string realPath = fileUri.GetRealPath();
    int32_t fd = open(realPath.c_str(), O_RDONLY);
    if (fd < 0) {
        ANI_ERR_LOG("Failed to open read only video file, errno:%{public}d", errno);
        return -1;
    }
    return fd;
}

static int32_t OpenReadOnlyImage(const std::string& imageUri, bool isMediaLibUri, int32_t position)
{
    if (isMediaLibUri) {
        std::string openImageUri = imageUri;
        if (position == POSITION_CLOUD) {
            MediaFileUtils::UriAppendKeyValue(openImageUri, MEDIA_MOVING_PHOTO_OPRN_KEYWORD,
                OPEN_MOVING_PHOTO_VIDEO_CLOUD);
        }
        Uri uri(openImageUri);
        return UserFileClient::OpenFile(uri, MEDIA_FILEMODE_READONLY);
    }
    OHOS::AppFileService::ModuleFileUri::FileUri fileUri(imageUri);
    std::string realPath = fileUri.GetRealPath();
    int32_t fd = open(realPath.c_str(), O_RDONLY);
    if (fd < 0) {
        ANI_ERR_LOG("Failed to open read only image file, errno: %{public}d", errno);
        return -1;
    }
    return fd;
}

int32_t MovingPhotoAni::OpenReadOnlyFile(const std::string& uri, bool isReadImage, int32_t position)
{
    if (uri.empty()) {
        ANI_ERR_LOG("Failed to open read only file, uri is empty");
        return -1;
    }
    std::string curUri = uri;
    bool isMediaLibUri = MediaFileUtils::IsMediaLibraryUri(uri);
    if (!isMediaLibUri) {
        std::vector<std::string> uris;
        if (!MediaFileUtils::SplitMovingPhotoUri(uri, uris)) {
            ANI_ERR_LOG("Failed to open read only file, split moving photo failed");
            return -1;
        }
        curUri = uris[isReadImage ? MOVING_PHOTO_IMAGE_POS : MOVING_PHOTO_VIDEO_POS];
    }
    return isReadImage ? OpenReadOnlyImage(curUri, isMediaLibUri, position) :
        OpenReadOnlyVideo(curUri, isMediaLibUri, position);
}

int32_t MovingPhotoAni::OpenReadOnlyLivePhoto(const string& destLivePhotoUri, int32_t position)
{
    if (destLivePhotoUri.empty()) {
        ANI_ERR_LOG("Failed to open read only file, uri is empty");
        return E_ERR;
    }
    if (MediaFileUtils::IsMediaLibraryUri(destLivePhotoUri)) {
        string livePhotoUri = destLivePhotoUri;
        if (position == POSITION_CLOUD) {
            MediaFileUtils::UriAppendKeyValue(livePhotoUri, MEDIA_MOVING_PHOTO_OPRN_KEYWORD,
                OPEN_MOVING_PHOTO_VIDEO_CLOUD);
        } else {
            MediaFileUtils::UriAppendKeyValue(livePhotoUri, MEDIA_MOVING_PHOTO_OPRN_KEYWORD,
                OPEN_PRIVATE_LIVE_PHOTO);
        }
        Uri uri(livePhotoUri);
        return UserFileClient::OpenFile(uri, MEDIA_FILEMODE_READONLY);
    }
    return E_ERR;
}

int32_t MovingPhotoAni::OpenReadOnlyMetadata(const string& movingPhotoUri)
{
    if (movingPhotoUri.empty()) {
        ANI_ERR_LOG("Failed to open metadata of moving photo, uri is empty");
        return E_ERR;
    }

    if (!MediaFileUtils::IsMediaLibraryUri(movingPhotoUri)) {
        ANI_ERR_LOG("Failed to check uri of moving photo: %{private}s", movingPhotoUri.c_str());
        return E_ERR;
    }

    string movingPhotoMetadataUri = movingPhotoUri;
    MediaFileUtils::UriAppendKeyValue(
        movingPhotoMetadataUri, MEDIA_MOVING_PHOTO_OPRN_KEYWORD, OPEN_PRIVATE_MOVING_PHOTO_METADATA);
    Uri uri(movingPhotoMetadataUri);
    return UserFileClient::OpenFile(uri, MEDIA_FILEMODE_READONLY);
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
            ANI_ERR_LOG("Failed to copy file from srcFd=%{public}d to destFd=%{public}d, errno=%{public}d",
                srcFd, destFd, errno);
            return E_HAS_FS_ERROR;
        }
    }

    if (bytesRead < 0) {
        ANI_ERR_LOG("Failed to read from srcFd=%{public}d, errno=%{public}d", srcFd, errno);
        return E_HAS_FS_ERROR;
    }
    return E_OK;
}

static int32_t WriteToSandboxUri(int32_t srcFd, const string& sandboxUri,
    MovingPhotoResourceType type = MovingPhotoResourceType::DEFAULT)
{
    UniqueFd srcUniqueFd(srcFd);

    OHOS::AppFileService::ModuleFileUri::FileUri fileUri(sandboxUri);
    string destPath = fileUri.GetRealPath();
    if (!MediaFileUtils::IsFileExists(destPath) && !MediaFileUtils::CreateFile(destPath)) {
        ANI_ERR_LOG("Create empty dest file in sandbox failed, path:%{private}s", destPath.c_str());
        return E_HAS_FS_ERROR;
    }

    if (type == MovingPhotoResourceType::CLOUD_IMAGE) {
        return MovingPhotoFileUtils::ConvertToMovingPhoto(srcFd, destPath, "", "");
    } else if (type == MovingPhotoResourceType::CLOUD_VIDEO) {
        return MovingPhotoFileUtils::ConvertToMovingPhoto(srcFd, "", destPath, "");
    }

    int32_t destFd = MediaFileUtils::OpenFile(destPath, MEDIA_FILEMODE_READWRITE);
    if (destFd < 0) {
        ANI_ERR_LOG("Open dest file failed, error: %{public}d", errno);
        return E_HAS_FS_ERROR;
    }
    UniqueFd destUniqueFd(destFd);

    if (ftruncate(destUniqueFd.Get(), 0) == -1) {
        ANI_ERR_LOG("Truncate old file in sandbox failed, error:%{public}d", errno);
        return E_HAS_FS_ERROR;
    }
    return CopyFileFromMediaLibrary(srcUniqueFd.Get(), destUniqueFd.Get());
}

static bool HandleFd(int32_t& fd)
{
    if (fd == E_ERR) {
        fd = E_HAS_FS_ERROR;
        return false;
    } else if (fd < 0) {
        ANI_ERR_LOG("Open failed due to OpenFile failure, error: %{public}d", fd);
        return false;
    }
    return true;
}

static int32_t CallDoTranscoder(const std::shared_ptr<MovingPhotoProgressHandler> &mppHandler,
    std::atomic_bool &isTranscoder)
{
    if (MovingPhotoCallTranscoder::DoTranscode(mppHandler)) {
        return E_OK;
    }
    ANI_INFO_LOG("DoTranscode fail");
    isTranscoder.store(false);
    return E_ERR;
}

int32_t MovingPhotoAni::DoMovingPhotoTranscode(int32_t &videoFd, MovingPhotoAsyncContext* context)
{
    CHECK_COND_RET(context != nullptr, E_ERR, "context is null");
    if (videoFd == -1) {
        ANI_INFO_LOG("videoFd is null");
        return E_ERR;
    }
    int64_t offset = 0;
    UniqueFd uniqueVideoFd(videoFd);
    int64_t videoSize = 0;
    int64_t extraDataSize = 0;
    if (context->position == POSITION_CLOUD) {
        int32_t ret = MovingPhotoFileUtils::GetMovingPhotoDetailedSize(uniqueVideoFd.Get(), offset, videoSize,
            extraDataSize);
        CHECK_COND_RET(ret == E_OK, E_ERR, "get moving photo detailed size fail");
    } else {
        struct stat statSrc;
        if (fstat(uniqueVideoFd.Get(), &statSrc) == E_ERR) {
            ANI_DEBUG_LOG("File get stat failed, %{public}d", errno);
            return E_HAS_FS_ERROR;
        }
        videoSize = statSrc.st_size;
    }
    AppFileService::ModuleFileUri::FileUri fileUri(context->destVideoUri);
    string destPath = fileUri.GetRealPath();
    if (!MediaFileUtils::IsFileExists(destPath) && !MediaFileUtils::CreateFile(destPath)) {
        ANI_ERR_LOG("Create empty dest file in sandbox failed, path:%{private}s", destPath.c_str());
        return E_HAS_FS_ERROR;
    }
    int32_t destFd = MediaFileUtils::OpenFile(destPath, MEDIA_FILEMODE_READWRITE);
    if (destFd < 0) {
        close(destFd);
        ANI_ERR_LOG("Open dest file failed, error: %{public}d", errno);
        return E_HAS_FS_ERROR;
    }
    UniqueFd uniqueDestFd(destFd);
    context->isTranscoder.store(true);
    auto movingPhotoProgressHandler = std::make_shared<MovingPhotoProgressHandler>();
    CHECK_COND_RET(movingPhotoProgressHandler != nullptr, E_ERR, "movingPhotoProgressHandler is null");
    movingPhotoProgressHandler->srcFd = std::move(uniqueVideoFd);
    movingPhotoProgressHandler->destFd = std::move(uniqueDestFd);
    movingPhotoProgressHandler->progressHandlerRef = context->progressHandlerRef;
    movingPhotoProgressHandler->callbackFunc = MovingPhotoAni::AfterTranscoder;
    movingPhotoProgressHandler->etsVm = context->etsVm;
    movingPhotoProgressHandler->offset = offset;
    movingPhotoProgressHandler->size = videoSize;
    movingPhotoProgressHandler->contextData = context;
    movingPhotoProgressHandler->onProgressFunc = context->threadsafeFunction;
    return CallDoTranscoder(std::move(movingPhotoProgressHandler), context->isTranscoder);
}

static int32_t RequestContentToSandbox(MovingPhotoAsyncContext* context)
{
    CHECK_COND_RET(context != nullptr, E_ERR, "context is nullptr");
    string movingPhotoUri = context->movingPhotoUri;
    if (context->sourceMode == SourceMode::ORIGINAL_MODE) {
        MediaLibraryAniUtils::UriAppendKeyValue(movingPhotoUri, MEDIA_OPERN_KEYWORD, SOURCE_REQUEST);
    }
    if (!context->destImageUri.empty()) {
        int32_t imageFd = MovingPhotoAni::OpenReadOnlyFile(movingPhotoUri, true, context->position);
        CHECK_COND_RET(HandleFd(imageFd), imageFd, "Open source image file failed");
        int32_t ret = WriteToSandboxUri(imageFd, context->destImageUri,
            context->position == POSITION_CLOUD ? MovingPhotoResourceType::CLOUD_IMAGE
                                                : MovingPhotoResourceType::DEFAULT);
        CHECK_COND_RET(ret == E_OK, ret, "Write image to sandbox failed");
    }
    if (!context->destVideoUri.empty()) {
        int32_t videoFd = MovingPhotoAni::OpenReadOnlyFile(movingPhotoUri, false, context->position);
        CHECK_COND_RET(HandleFd(videoFd), videoFd, "Open source video file failed");
        if (context->compatibleMode == CompatibleMode::COMPATIBLE_FORMAT_MODE) {
            ANI_DEBUG_LOG("movingPhoto CompatibleMode  COMPATIBLE_FORMAT_MODE");
            int32_t ret = MovingPhotoAni::DoMovingPhotoTranscode(videoFd, context);
            CHECK_COND_RET(ret == E_OK, ret, "moving video transcode failed");
        } else {
            int32_t ret = WriteToSandboxUri(videoFd, context->destVideoUri,
                context->position == POSITION_CLOUD ? MovingPhotoResourceType::CLOUD_VIDEO
                                                    : MovingPhotoResourceType::DEFAULT);
            CHECK_COND_RET(ret == E_OK, ret, "Write video to sandbox failed");
        }
    }
    if (!context->destLivePhotoUri.empty()) {
        int32_t livePhotoFd = MovingPhotoAni::OpenReadOnlyLivePhoto(movingPhotoUri, context->position);
        CHECK_COND_RET(HandleFd(livePhotoFd), livePhotoFd, "Open source video file failed");
        int32_t ret = WriteToSandboxUri(livePhotoFd, context->destLivePhotoUri);
        CHECK_COND_RET(ret == E_OK, ret, "Write video to sandbox failed");
    }
    if (!context->destMetadataUri.empty()) {
        int32_t extraDataFd = MovingPhotoAni::OpenReadOnlyMetadata(movingPhotoUri);
        CHECK_COND_RET(HandleFd(extraDataFd), extraDataFd, "Open moving photo metadata failed");
        int32_t ret = WriteToSandboxUri(extraDataFd, context->destMetadataUri);
        CHECK_COND_RET(ret == E_OK, ret, "Write metadata to sandbox failed");
    }
    return E_OK;
}

static int32_t AcquireFdForArrayBuffer(MovingPhotoAsyncContext* context)
{
    CHECK_COND_RET(context != nullptr, E_ERR, "context is nullptr");
    int32_t fd = 0;
    string movingPhotoUri = context->movingPhotoUri;
    if (context->sourceMode == SourceMode::ORIGINAL_MODE) {
        MediaFileUtils::UriAppendKeyValue(movingPhotoUri, MEDIA_OPERN_KEYWORD, SOURCE_REQUEST);
    }
    switch (context->resourceType) {
        case ResourceType::IMAGE_RESOURCE: {
            fd = MovingPhotoAni::OpenReadOnlyFile(movingPhotoUri, true, context->position);
            CHECK_COND_RET(HandleFd(fd), fd, "Open source image file failed");
            return fd;
        }
        case ResourceType::VIDEO_RESOURCE:
            fd = MovingPhotoAni::OpenReadOnlyFile(movingPhotoUri, false, context->position);
            CHECK_COND_RET(HandleFd(fd), fd, "Open source video file failed");
            return fd;
        case ResourceType::PRIVATE_MOVING_PHOTO_RESOURCE:
            fd = MovingPhotoAni::OpenReadOnlyLivePhoto(movingPhotoUri, context->position);
            CHECK_COND_RET(HandleFd(fd), fd, "Open source video file failed");
            return fd;
        case ResourceType::PRIVATE_MOVING_PHOTO_METADATA:
            fd = MovingPhotoAni::OpenReadOnlyMetadata(movingPhotoUri);
            CHECK_COND_RET(HandleFd(fd), fd, "Open moving photo metadata failed");
            return fd;
        default:
            ANI_ERR_LOG("Invalid resource type: %{public}d", static_cast<int32_t>(context->resourceType));
            return -EINVAL;
    }
}

static int32_t ArrayBufferToTranscode(ani_env *env, MovingPhotoAsyncContext* context, int32_t fd)
{
    CHECK_COND_RET(context != nullptr, E_ERR, "context is null");
    UniqueFd uniqueFd(fd);
    int64_t offset = 0;
    int64_t videoSize = 0;
    int64_t extraDataSize = 0;
    if (context->position == POSITION_CLOUD) {
        int32_t ret = MovingPhotoFileUtils::GetMovingPhotoDetailedSize(uniqueFd.Get(), offset, videoSize,
            extraDataSize);
        if (ret != E_OK) {
            context->error = JS_INNER_FAIL;
            ANI_ERR_LOG("get moving photo detailed size fail");
            return E_ERR;
        }
    } else {
        struct stat statSrc;
        if (fstat(uniqueFd.Get(), &statSrc) == E_ERR) {
            ANI_DEBUG_LOG("File get stat failed, %{public}d", errno);
            return E_HAS_FS_ERROR;
        }
        videoSize = statSrc.st_size;
    }
    auto abilityContext = AbilityRuntime::Context::GetApplicationContext();
    CHECK_COND_RET(abilityContext != nullptr, E_ERR, "abilityContext is null");
    string cachePath = abilityContext->GetCacheDir();
    string destUri = cachePath + "/" +context->requestId + ".mp4";
    ANI_DEBUG_LOG("destUri:%{public}s", destUri.c_str());
    int destFd = MovingPhotoAni::GetFdFromUri(destUri);
    if (destFd < 0) {
        context->error = JS_INNER_FAIL;
        ANI_ERR_LOG("get destFd fail");
        return E_ERR;
    }
    UniqueFd uniqueDestFd(destFd);
    context->isTranscoder.store(true);
    auto movingPhotoProgressHandler = std::make_shared<MovingPhotoProgressHandler>();
    CHECK_COND_RET(movingPhotoProgressHandler != nullptr, E_ERR, "movingPhotoProgressHandler is null");
    movingPhotoProgressHandler->srcFd = std::move(uniqueFd);
    movingPhotoProgressHandler->destFd = std::move(uniqueDestFd);
    movingPhotoProgressHandler->progressHandlerRef = context->progressHandlerRef;
    movingPhotoProgressHandler->callbackFunc = MovingPhotoAni::AfterTranscoder;
    movingPhotoProgressHandler->etsVm = context->etsVm;
    movingPhotoProgressHandler->offset = offset;
    movingPhotoProgressHandler->size = videoSize;
    movingPhotoProgressHandler->contextData = context;
    movingPhotoProgressHandler->onProgressFunc = context->threadsafeFunction;
    return CallDoTranscoder(std::move(movingPhotoProgressHandler), context->isTranscoder);
}

static int32_t RequestContentToArrayBuffer(ani_env *env, MovingPhotoAsyncContext *context)
{
    CHECK_COND_RET(context != nullptr, E_ERR, "context is nullptr");
    int32_t fd = AcquireFdForArrayBuffer(context);
    if (fd < 0) {
        return fd;
    }
    if (context->resourceType == ResourceType::VIDEO_RESOURCE &&
        context->compatibleMode == CompatibleMode::COMPATIBLE_FORMAT_MODE) {
        return ArrayBufferToTranscode(env, context, fd);
    }
    MovingPhotoAni::SubRequestContent(fd, context);
    return E_OK;
}

static bool IsValidResourceType(int32_t resourceType)
{
    return (resourceType == static_cast<int>(ResourceType::IMAGE_RESOURCE) ||
        resourceType == static_cast<int>(ResourceType::VIDEO_RESOURCE) ||
        resourceType == static_cast<int>(ResourceType::PRIVATE_MOVING_PHOTO_RESOURCE));
}

static int32_t QueryPhotoPositionIPCExecute(const string &movingPhotoUri, int32_t userId, int32_t &position)
{
    RequestContentRespBody respBody;
    RequestContentReqBody reqBody;
    reqBody.mediaId = MediaFileUtils::GetIdFromUri(movingPhotoUri);
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_REQUEST_CONTENT);

    std::unordered_map<std::string, std::string> headerMap = {
        {MediaColumn::MEDIA_ID, reqBody.mediaId}, {URI_TYPE, TYPE_PHOTOS}
    };
    int32_t err =
        IPC::UserDefineIPCClient().SetUserId(userId).SetHeader(headerMap).Call(businessCode, reqBody, respBody);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, E_ERR, "get position fail. err:%{public}d", err);

    position = respBody.position;
    return E_OK;
}

static int32_t QueryPhotoPosition(const string &movingPhotoUri, bool hasReadPermission, int32_t &position)
{
    if (!MediaFileUtils::IsMediaLibraryUri(movingPhotoUri)) {
        position = static_cast<int32_t>(PhotoPositionType::LOCAL);
        return E_OK;
    }

    std::string str = movingPhotoUri;
    size_t pos = str.find(MULTI_USER_URI_FLAG);
    std::string userIdStr = "";
    if (pos != std::string::npos) {
        pos += MULTI_USER_URI_FLAG.length();
        size_t end = str.find_first_of("&?", pos);
        if (end == std::string::npos) {
            end = str.length();
        }
        userIdStr = str.substr(pos, end - pos);
        ANI_INFO_LOG("QueryPhotoPosition for other user is %{public}s", userIdStr.c_str());
    }
    int32_t userId = userIdStr != "" && MediaFileUtils::IsValidInteger(userIdStr) ? atoi(userIdStr.c_str()) : -1;

    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(MediaColumn::MEDIA_ID, MediaFileUtils::GetIdFromUri(movingPhotoUri));
    std::vector<std::string> fetchColumn { PhotoColumn::PHOTO_POSITION };
    string queryUri;
    if (hasReadPermission) {
        queryUri = PAH_QUERY_PHOTO;
    } else {
        queryUri = movingPhotoUri;
        MediaFileUri::RemoveAllFragment(queryUri);
    }
    Uri uri(queryUri);
    OperationObject object = OperationObject::UNKNOWN_OBJECT;
    auto mediaAssetRdbStore = MediaAssetRdbStore::GetInstance();
    CHECK_AND_RETURN_RET_LOG(mediaAssetRdbStore != nullptr, E_ERR, "mediaAssetRdbStore is null");
    if (!mediaAssetRdbStore->IsQueryAccessibleViaSandBox(uri, object, predicates) || userId != DEFAULT_USER_ID) {
        return QueryPhotoPositionIPCExecute(movingPhotoUri, userId, position);
    }

    int errCode = 0;
    auto resultSet = UserFileClient::Query(uri, predicates, fetchColumn, errCode, userId);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != E_OK) {
        ANI_ERR_LOG("query resultSet is nullptr");
        return E_ERR;
    }

    int index;
    int err = resultSet->GetColumnIndex(PhotoColumn::PHOTO_POSITION, index);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, E_ERR, "Failed to GetColumnIndex");
    resultSet->GetInt(index, position);
    return E_OK;
}

static bool HasReadPermission()
{
    static bool result = (Security::AccessToken::AccessTokenKit::VerifyAccessToken(IPCSkeleton::GetSelfTokenID(),
        PERM_READ_IMAGEVIDEO) == Security::AccessToken::PermissionState::PERMISSION_GRANTED);
    return result;
}

static void SetContextInfo(MovingPhotoAni* thisArg, std::unique_ptr<MovingPhotoAsyncContext>& context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    CHECK_NULL_PTR_RETURN_VOID(thisArg, "thisArg is nullptr");
    context->movingPhotoUri = thisArg->GetUriInner();
    context->sourceMode = thisArg->GetSourceMode();
    context->compatibleMode = thisArg->GetCompatibleMode();
    context->progressHandlerRef = thisArg->GetProgressHandlerRef();
    context->etsVm = thisArg->GetEtsVm();
    context->threadsafeFunction = thisArg->GetThreadsafeFunction();
}

static ani_status ParseArgsByImageFileAndVideoFile(ani_env *env, ani_string imageFileUri, ani_string videoFileUri,
    MovingPhotoAni* thisArg, std::unique_ptr<MovingPhotoAsyncContext>& context)
{
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");
    CHECK_COND_RET(thisArg != nullptr, ANI_ERROR, "thisArg is nullptr");
    SetContextInfo(thisArg, context);
    context->requestContentMode = MovingPhotoAsyncContext::WRITE_TO_SANDBOX;
    // write both image and video to sandbox
    CHECK_COND_WITH_RET_MESSAGE(env,
        MediaLibraryAniUtils::GetParamStringPathMax(env, imageFileUri, context->destImageUri) == ANI_OK,
        ANI_INVALID_ARGS, "Failed to get imageFileUri");
    CHECK_COND_WITH_RET_MESSAGE(env,
        MediaLibraryAniUtils::GetParamStringPathMax(env, videoFileUri, context->destVideoUri) == ANI_OK,
        ANI_INVALID_ARGS, "Failed to get videoFileUri");
    return ANI_OK;
}

static ani_status ParseArgsByResourceTypeAndFile(ani_env *env, ani_enum_item resourceTypeAni, ani_string videoFileUri,
    MovingPhotoAni* thisArg, std::unique_ptr<MovingPhotoAsyncContext>& context)
{
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");
    CHECK_COND_RET(thisArg != nullptr, ANI_ERROR, "thisArg is nullptr");
    SetContextInfo(thisArg, context);
    context->requestContentMode = MovingPhotoAsyncContext::WRITE_TO_SANDBOX;
    // write image or video to sandbox
    int32_t resourceType = static_cast<int32_t>(ResourceType::INVALID_RESOURCE);

    CHECK_COND_WITH_RET_MESSAGE(env,
        MediaLibraryEnumAni::EnumGetValueInt32(env, resourceTypeAni, resourceType) == ANI_OK, ANI_INVALID_ARGS,
        "Failed to get resourceType");

    CHECK_COND_WITH_RET_MESSAGE(env, IsValidResourceType(resourceType), ANI_INVALID_ARGS, "Invalid resource type");
    if (resourceType == static_cast<int>(ResourceType::IMAGE_RESOURCE)) {
        CHECK_COND_WITH_RET_MESSAGE(env, MediaLibraryAniUtils::GetParamStringPathMax(env, videoFileUri,
            context->destImageUri) == ANI_OK, ANI_INVALID_ARGS, "Failed to get destImageUri");
    } else if (resourceType == static_cast<int>(ResourceType::VIDEO_RESOURCE)) {
        CHECK_COND_WITH_RET_MESSAGE(env, MediaLibraryAniUtils::GetParamStringPathMax(env, videoFileUri,
            context->destVideoUri) == ANI_OK, ANI_INVALID_ARGS, "Failed to get destVideoUri");
    } else {
        CHECK_COND_WITH_RET_MESSAGE(env, MediaLibraryAniUtils::GetParamStringPathMax(env, videoFileUri,
            context->destLivePhotoUri) == ANI_OK, ANI_INVALID_ARGS, "Failed to get destLivePhotoUri");
    }
    context->resourceType = static_cast<ResourceType>(resourceType);
    return ANI_OK;
}

static ani_status ParseArgsByResourceType(ani_env *env, ani_enum_item resourceTypeAni,
    MovingPhotoAni* thisArg, std::unique_ptr<MovingPhotoAsyncContext>& context)
{
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");
    CHECK_COND_RET(thisArg != nullptr, ANI_ERROR, "thisArg is nullptr");
    SetContextInfo(thisArg, context);
    int32_t resourceType = static_cast<int32_t>(ResourceType::INVALID_RESOURCE);

    CHECK_COND_WITH_RET_MESSAGE(env,
        MediaLibraryEnumAni::EnumGetValueInt32(env, resourceTypeAni, resourceType) == ANI_OK, ANI_INVALID_ARGS,
        "Failed to get resourceType");

    CHECK_COND_WITH_RET_MESSAGE(env, IsValidResourceType(resourceType), ANI_INVALID_ARGS, "Invalid resource type");
    context->requestContentMode = MovingPhotoAsyncContext::WRITE_TO_ARRAY_BUFFER;
    context->resourceType = static_cast<ResourceType>(resourceType);
    return ANI_OK;
}

static void RequestContentExecute(ani_env *env, unique_ptr<MovingPhotoAsyncContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    int32_t ret = QueryPhotoPosition(context->movingPhotoUri, HasReadPermission(), context->position);
    if (ret != E_OK) {
        ANI_ERR_LOG("Failed to query position of moving photo, ret: %{public}d", ret);
        context->SaveError(ret);
        return;
    }
    switch (context->requestContentMode) {
        case MovingPhotoAsyncContext::WRITE_TO_SANDBOX:
            ret = RequestContentToSandbox(context.get());
            break;
        case MovingPhotoAsyncContext::WRITE_TO_ARRAY_BUFFER:
            ret = RequestContentToArrayBuffer(env, context.get());
            break;
        default:
            ANI_ERR_LOG("Invalid request content mode: %{public}d", static_cast<int32_t>(context->requestContentMode));
            context->error = OHOS_INVALID_PARAM_CODE;
            return;
    }
    if (ret != E_OK) {
        context->SaveError(ret);
        return;
    }
}

static ani_object RequestContentComplete(ani_env *env, unique_ptr<MovingPhotoAsyncContext> &context)
{
    ani_arraybuffer externalBuffer = {};
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");

    if (context->error == E_OK && context->requestContentMode == MovingPhotoAsyncContext::WRITE_TO_ARRAY_BUFFER) {
        void* buffer = nullptr;
        ani_status status = env->CreateArrayBuffer(context->arrayBufferLength, &buffer, &externalBuffer);
        if (context->arrayBufferData != nullptr) {
            if (buffer == nullptr) {
                ANI_ERR_LOG("Failed to malloc array buffer data, uri is %{public}s, resource type is %{public}d",
                    context->movingPhotoUri.c_str(), context->resourceType);
                context->error = JS_INNER_FAIL;
            } else {
                memcpy_s(buffer, context->arrayBufferLength, context->arrayBufferData, context->arrayBufferLength);
            }
        }
        free(context->arrayBufferData);
        if (status != ANI_OK) {
            ANI_ERR_LOG("Failed to create array buffer object, uri is %{public}s, resource type is %{public}d",
                context->movingPhotoUri.c_str(), context->resourceType);
            context->arrayBufferData = nullptr;
            context->error = JS_INNER_FAIL;
        }
    } else if (context->arrayBufferData != nullptr) {
        free(context->arrayBufferData);
        context->arrayBufferData = nullptr;
    }
    ani_object errorObj {};
    if (context->error != E_OK) {
        ANI_ERR_LOG("context error: %{public}d", context->error);
        context->HandleError(env, errorObj);
    }
    return externalBuffer;
}

static void WaitForTranscoder(const std::unique_ptr<MovingPhotoAsyncContext> &asyncContext)
{
    CHECK_NULL_PTR_RETURN_VOID(asyncContext, "asyncContext is null");
    static const int WAIT_TIMEOUT = 60000; // 60 seconds
    std::thread([&] {
        std::unique_lock<std::mutex> lock(asyncContext->mutex);
        asyncContext->cv.wait_for(lock, std::chrono::milliseconds(WAIT_TIMEOUT),
            [&] { return !asyncContext->isTranscoder.load(); });
        if (asyncContext->isTranscoder.load()) {
            ANI_ERR_LOG("Wait for transcoder timeout");
            asyncContext->error = JS_INNER_FAIL;
        }
    }).join();
}

void MovingPhotoAni::RequestContentByImageFileAndVideoFile(ani_env *env, ani_object object,
    ani_string imageFileUri, ani_string videoFileUri)
{
    unique_ptr<MovingPhotoAsyncContext> asyncContext = make_unique<MovingPhotoAsyncContext>();
    MovingPhotoAni* nativeObject = Unwrap(env, object);
    CHECK_NULL_PTR_RETURN_VOID(nativeObject, "nativeObject is nullptr");
    CHECK_ARGS_RET_VOID(env, ParseArgsByImageFileAndVideoFile(env, imageFileUri, videoFileUri, nativeObject,
        asyncContext), OHOS_INVALID_PARAM_CODE);
    RequestContentExecute(env, asyncContext);
    WaitForTranscoder(asyncContext);
    RequestContentComplete(env, asyncContext);
}

void MovingPhotoAni::RequestContentByResourceTypeAndFile(ani_env *env, ani_object object,
    ani_enum_item resourceTypeAni, ani_string fileUri)
{
    unique_ptr<MovingPhotoAsyncContext> asyncContext = make_unique<MovingPhotoAsyncContext>();
    MovingPhotoAni* nativeObject = Unwrap(env, object);
    CHECK_NULL_PTR_RETURN_VOID(nativeObject, "nativeObject is nullptr");
    CHECK_ARGS_RET_VOID(env, ParseArgsByResourceTypeAndFile(env, resourceTypeAni, fileUri, nativeObject,
        asyncContext), OHOS_INVALID_PARAM_CODE);
    RequestContentExecute(env, asyncContext);
    WaitForTranscoder(asyncContext);
    RequestContentComplete(env, asyncContext);
}

ani_object MovingPhotoAni::RequestContentByResourceType(ani_env *env, ani_object object, ani_enum_item resourceTypeAni)
{
    unique_ptr<MovingPhotoAsyncContext> asyncContext = make_unique<MovingPhotoAsyncContext>();
    MovingPhotoAni* nativeObject = Unwrap(env, object);

    CHECK_COND_WITH_RET_MESSAGE(env, ParseArgsByResourceType(env, resourceTypeAni, nativeObject,
        asyncContext) == ANI_OK, nullptr, "Failed to parse requestContent args");
    RequestContentExecute(env, asyncContext);
    WaitForTranscoder(asyncContext);
    return RequestContentComplete(env, asyncContext);
}

void MovingPhotoAni::AfterTranscoder(void *context, int32_t errCode)
{
    auto asyncContext = static_cast<MovingPhotoAsyncContext*>(context);
    CHECK_NULL_PTR_RETURN_VOID(asyncContext, "asyncContext is null");
    asyncContext->error = errCode;
    asyncContext->isTranscoder.store(false);
    asyncContext->cv.notify_all();
}

ani_string MovingPhotoAni::GetUri(ani_env *env, ani_object object)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    ani_string result = nullptr;
    auto movingPhotoAni = Unwrap(env, object);
    if (movingPhotoAni == nullptr) {
        ANI_ERR_LOG("movingPhotoAni is nullptr");
        return result;
    }

    const std::string& uri = movingPhotoAni->GetUriInner();
    const char *utf8String = uri.c_str();
    const ani_size stringLength = uri.length();
    env->String_NewUTF8(utf8String, stringLength, &result);
    return result;
}

void MovingPhotoAni::RequestCloudContentArrayBuffer(int32_t fd, MovingPhotoAsyncContext* context)
{
    if (context->position != POSITION_CLOUD) {
        ANI_ERR_LOG("Failed to check postion: %{public}d", context->position);
        context->arrayBufferData = nullptr;
        context->error = JS_INNER_FAIL;
        return;
    }

    int64_t imageSize = 0;
    int64_t videoSize = 0;
    int64_t extraDataSize = 0;
    int32_t err = MovingPhotoFileUtils::GetMovingPhotoDetailedSize(fd, imageSize, videoSize, extraDataSize);
    if (err != E_OK) {
        ANI_ERR_LOG("Failed to get detailed size of moving photo");
        context->arrayBufferData = nullptr;
        context->SaveError(E_HAS_FS_ERROR);
        return;
    }

    int32_t ret = E_FAIL;
    size_t fileSize = 0;
    switch (context->resourceType) {
        case ResourceType::IMAGE_RESOURCE:
            fileSize = static_cast<size_t>(imageSize);
            context->arrayBufferData = malloc(fileSize);
            ret = MovingPhotoFileUtils::ConvertToMovingPhoto(fd, context->arrayBufferData, nullptr, nullptr);
            break;
        case ResourceType::VIDEO_RESOURCE:
            fileSize = static_cast<size_t>(videoSize);
            context->arrayBufferData = malloc(fileSize);
            ret = MovingPhotoFileUtils::ConvertToMovingPhoto(fd, nullptr, context->arrayBufferData, nullptr);
            break;
        default:
            ANI_ERR_LOG("Invalid resource type: %{public}d", static_cast<int32_t>(context->resourceType));
            break;
    }

    if (!context->arrayBufferData || ret != E_OK) {
        ANI_ERR_LOG(
            "Failed to get arraybuffer, resource type is %{public}d", static_cast<int32_t>(context->resourceType));
        context->error = JS_INNER_FAIL;
        return;
    }
    context->arrayBufferLength = fileSize;
}
//ani -> napi
ani_ref MovingPhotoAni::TransferToDynamicMovingPhoto(ani_env *env, [[maybe_unused]] ani_class, ani_object input)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    ani_ref undefinedRef {};
    env->GetUndefined(&undefinedRef);
    napi_env jsEnv;

    arkts_napi_scope_open(env, &jsEnv);
    MovingPhotoAni* movingPhotoAni = Unwrap(env, input);
    if (movingPhotoAni == nullptr) {
        ANI_ERR_LOG("movingPhotoAni is null.");
        return undefinedRef;
    }

    TransferUtils::TransferMovingPhotoParam movingPhotoParam;
    std::string uri = movingPhotoAni->GetUriInner();
    int sourceMode = static_cast<int>(movingPhotoAni->GetSourceMode());
    movingPhotoParam.compatibleMode = static_cast<int>(movingPhotoAni->GetCompatibleMode());
    movingPhotoParam.requestId = movingPhotoAni->GetRequestId();
    movingPhotoParam.progressHandlerRef = reinterpret_cast<napi_ref>(movingPhotoAni->GetProgressHandlerRef());
    movingPhotoParam.threadsafeFunction = nullptr;
    CreateTransferMovingPhotoFn funcHandle = nullptr;
    if (!LibManager::GetSymbol("CreateTransferMovingPhotoNapi", funcHandle)) {
        ANI_ERR_LOG("%{public}s Get CreateTransferMovingPhotoNapi symbol failed", __func__);
        arkts_napi_scope_close_n(jsEnv, 0, nullptr, &undefinedRef);
        return undefinedRef;
    }
    napi_value napiMovingPhoto = funcHandle(jsEnv, uri, sourceMode, movingPhotoParam);
    if (napiMovingPhoto == nullptr) {
        ANI_ERR_LOG("napiMovingPhoto is null.");
        arkts_napi_scope_close_n(jsEnv, 0, nullptr, &undefinedRef);
        return undefinedRef;
    }
    ani_ref result {};
    arkts_napi_scope_close_n(jsEnv, 1, &napiMovingPhoto, &result);

    return result;
}
//napi -> ani
ani_object MovingPhotoAni::TransferToStaticMovingPhoto(ani_env *env, [[maybe_unused]] ani_class, ani_object input)
{
    MovingPhotoNapi *napiMovingPhoto = nullptr;

    arkts_esvalue_unwrap(env, input, (void **)&napiMovingPhoto);
    CHECK_COND_RET(napiMovingPhoto != nullptr, nullptr, "napiMovingPhoto is null");

    TransferUtils::TransferMovingPhotoParam movingPhotoParam;
    std::string uri = "";
    int sourceMode = static_cast<int>(SourceMode::EDITED_MODE);

    GetPropertyMovingPhotoNapiFn funcHandle = nullptr;
    if (!LibManager::GetSymbol("MovingPhotoGetProperty", funcHandle)) {
        ANI_ERR_LOG("%{public}s Get MovingPhotoGetProperty symbol failed", __func__);
        return nullptr;
    }
    CHECK_COND_RET(funcHandle != nullptr, nullptr, "funcHandle is null");
    bool ret = funcHandle(napiMovingPhoto, uri, sourceMode, movingPhotoParam);
    CHECK_COND_RET(ret != false, nullptr, "Failed to get property from napiMovingPhoto");

    MovingPhotoParam movingPhotoParamAni;
    movingPhotoParamAni.compatibleMode = static_cast<CompatibleMode>(movingPhotoParam.compatibleMode);
    movingPhotoParamAni.requestId = movingPhotoParam.requestId;
    movingPhotoParamAni.progressHandlerRef = reinterpret_cast<ani_ref>(movingPhotoParam.progressHandlerRef);
    movingPhotoParam.threadsafeFunction = nullptr;

    return MovingPhotoAni::NewMovingPhotoAni(env, uri, static_cast<SourceMode>(sourceMode), movingPhotoParamAni);
}

static void IsSandBoxMovingPhotoVideoReady(unique_ptr<MovingPhotoAsyncContext> &context, string &videoUri)
{
    AppFileService::ModuleFileUri::FileUri fileUri(videoUri);
    std::string videoPath = fileUri.GetRealPath();
    size_t fileSize = 0;
    context->isVideoReady = MediaFileUtils::GetFileSize(videoPath, fileSize) && (fileSize > 0);
    ANI_DEBUG_LOG("videoUri:%{public}s, video size:%zu", videoUri.c_str(), fileSize);
}

static void IsMovingPhotoVideoReady(unique_ptr<MovingPhotoAsyncContext> &context)
{
    DataShare::DataSharePredicates predicates;
    string queryId = MediaFileUtils::GetIdFromUri(context->movingPhotoUri);
    predicates.EqualTo(MediaColumn::MEDIA_ID, queryId);
    vector<string> columns;
    Uri uri(MEDIALIBRARY_DATA_URI + "/" + MEDIA_QUERY_OPRN_MOVING_PHOTO_VIDEO_READY + "/"
         + MEDIA_QUERY_OPRN_MOVING_PHOTO_VIDEO_READY);
    int errCode = 0;
    shared_ptr<DataShare::DataShareResultSet> resultSet = UserFileClient::Query(uri, predicates, columns, errCode);
    if (errCode == E_PERMISSION_DENIED) {
        context->error = OHOS_PERMISSION_DENIED_CODE;
        return;
    }
    if ((resultSet == nullptr) || (resultSet->GoToFirstRow() != NativeRdb::E_OK)) {
        ANI_ERR_LOG("Query movingphoto fail");
        context->error = JS_E_INNER_FAIL;
        return;
    }
    int32_t ready;
    if (resultSet->GetInt(0, ready) != NativeRdb::E_OK) {
        ANI_ERR_LOG("can not get movingphoto video ready");
        context->error = JS_E_INNER_FAIL;
        return;
    }
    ANI_INFO_LOG("movingphoto video ready:%d", ready);
    context->isVideoReady = (ready != 0);
}

static void IsVideoReadyExecute(ani_env *env, unique_ptr<MovingPhotoAsyncContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    if (MediaFileUtils::IsMediaLibraryUri(context->movingPhotoUri)) {
        IsMovingPhotoVideoReady(context);
        return;
    }

    std::vector<std::string> uris;
    if (MediaFileUtils::SplitMovingPhotoUri(context->movingPhotoUri, uris)) {
        IsSandBoxMovingPhotoVideoReady(context, uris[MOVING_PHOTO_VIDEO_POS]);
        return;
    }

    ANI_ERR_LOG("Failed to check uri of moving photo:%{public}s", context->movingPhotoUri.c_str());
    context->error = JS_E_INNER_FAIL;
}

static void IsVideoReadyComplete(ani_env *env, unique_ptr<MovingPhotoAsyncContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is nullptr");
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    ani_object errorObj {};
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, errorObj);
    }
    context.reset();
}

ani_boolean MovingPhotoAni::IsVideoReady(ani_env *env, ani_object object)
{
    ANI_DEBUG_LOG("IsVideoReady start");
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return false;
    }
    unique_ptr<MovingPhotoAsyncContext> asyncContext = make_unique<MovingPhotoAsyncContext>();
    MovingPhotoAni* nativeObject = Unwrap(env, object);
    if (nativeObject == nullptr) {
        ANI_ERR_LOG("nativeObject is nullptr");
        return false;
    }
    asyncContext->movingPhotoUri = nativeObject->GetUriInner();
    IsVideoReadyExecute(env, asyncContext);
    ani_boolean aniIsVideoReady = asyncContext->isVideoReady;
    IsVideoReadyComplete(env, asyncContext);
    return aniIsVideoReady;
}
} // namespace Media
} // namespace OHOS
