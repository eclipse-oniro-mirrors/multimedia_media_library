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

#include "moving_photo_napi.h"

#include <fcntl.h>
#include <unistd.h>

#include "access_token.h"
#include "accesstoken_kit.h"
#include "directory_ex.h"
#include "file_uri.h"
#include "media_file_utils.h"
#include "media_file_uri.h"
#include "moving_photo_file_utils.h"
#include "media_library_napi.h"
#include "medialibrary_client_errno.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "medialibrary_napi_utils.h"
#include "userfile_client.h"
#include "userfile_manager_types.h"
#include "media_call_transcode.h"
#include "permission_utils.h"

using namespace std;
using namespace OHOS::Security::AccessToken;

namespace OHOS {
namespace Media {

static const string MOVING_PHOTO_NAPI_CLASS = "MovingPhoto";
thread_local napi_ref MovingPhotoNapi::constructor_ = nullptr;
static std::function<void(int, int, std::string)> callback_;
static SafeMap<std::string, bool> isMovingPhotoTranscoderMap;
static SafeMap<std::string, MovingPhotoAsyncContext *> requestContentCompleteResult;
static std::mutex isMovingPhotoTranscoderMapMutex;
static std::mutex requestContentCompleteResultMutex;

enum class MovingPhotoResourceType : int32_t {
    DEFAULT = 0,
    CLOUD_IMAGE,
    CLOUD_VIDEO,
    CLOUD_LIVE_PHOTO,
    CLOUD_METADATA,
};

napi_value MovingPhotoNapi::Init(napi_env env, napi_value exports)
{
    NapiClassInfo info = {
        .name = MOVING_PHOTO_NAPI_CLASS,
        .ref = &constructor_,
        .constructor = Constructor,
        .props = {
            DECLARE_NAPI_FUNCTION("requestContent", JSRequestContent),
            DECLARE_NAPI_FUNCTION("getUri", JSGetUri),
        }
    };
    MediaLibraryNapiUtils::NapiDefineClass(env, exports, info);
    return exports;
}

napi_value MovingPhotoNapi::Constructor(napi_env env, napi_callback_info info)
{
    napi_value newTarget = nullptr;
    CHECK_ARGS(env, napi_get_new_target(env, info, &newTarget), JS_INNER_FAIL);
    CHECK_COND_RET(newTarget != nullptr, nullptr, "Invalid call to constructor");

    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = { 0 };
    napi_value thisVar = nullptr;
    napi_valuetype valueType;
    CHECK_ARGS(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr), JS_INNER_FAIL);
    CHECK_COND_WITH_MESSAGE(env, argc == ARGS_ONE, "Number of args is invalid");
    CHECK_ARGS(env, napi_typeof(env, argv[PARAM0], &valueType), JS_INNER_FAIL);
    CHECK_COND_WITH_MESSAGE(env, valueType == napi_string, "Invalid argument type");
    size_t result;
    char photoUri[PATH_MAX];
    CHECK_ARGS(env, napi_get_value_string_utf8(env, argv[PARAM0], photoUri, PATH_MAX, &result), JS_INNER_FAIL);

    unique_ptr<MovingPhotoNapi> obj = make_unique<MovingPhotoNapi>(string(photoUri));
    CHECK_ARGS(env,
        napi_wrap(env, thisVar, reinterpret_cast<void*>(obj.get()), MovingPhotoNapi::Destructor, nullptr,
            nullptr),
        JS_INNER_FAIL);
    obj.release();
    return thisVar;
}

void MovingPhotoNapi::Destructor(napi_env env, void* nativeObject, void* finalizeHint)
{
    auto* movingPhotoNapi = reinterpret_cast<MovingPhotoNapi*>(nativeObject);
    if (movingPhotoNapi == nullptr) {
        return;
    }

    delete movingPhotoNapi;
    movingPhotoNapi = nullptr;
}

string MovingPhotoNapi::GetUri()
{
    return photoUri_;
}

SourceMode MovingPhotoNapi::GetSourceMode()
{
    return sourceMode_;
}

void MovingPhotoNapi::SetSourceMode(SourceMode sourceMode)
{
    sourceMode_ = sourceMode;
}

CompatibleMode MovingPhotoNapi::GetCompatibleMode()
{
    return compatibleMode_;
}

void MovingPhotoNapi::SetCompatibleMode(const CompatibleMode compatibleMode)
{
    compatibleMode_ = compatibleMode;
}

void MovingPhotoNapi::SetMovingPhotoCallback(const std::function<void(int, int, std::string)> callback)
{
    callback_ = callback;
}

std::function<void(int, int, std::string)> MovingPhotoNapi::GetMovingPhotoCallback()
{
    return callback_;
}

std::string MovingPhotoNapi::GetRequestId()
{
    return requestId_;
}

void MovingPhotoNapi::SetRequestId(const std::string requestId)
{
    requestId_ = requestId;
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
    AppFileService::ModuleFileUri::FileUri fileUri(videoUri);
    std::string realPath = fileUri.GetRealPath();
    int32_t fd = open(realPath.c_str(), O_RDONLY);
    if (fd < 0) {
        NAPI_ERR_LOG("Failed to open read only video file, errno:%{public}d", errno);
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
    AppFileService::ModuleFileUri::FileUri fileUri(imageUri);
    std::string realPath = fileUri.GetRealPath();
    int32_t fd = open(realPath.c_str(), O_RDONLY);
    if (fd < 0) {
        NAPI_ERR_LOG("Failed to open read only image file, errno: %{public}d", errno);
        return -1;
    }
    return fd;
}

int32_t MovingPhotoNapi::OpenReadOnlyFile(const std::string& uri, bool isReadImage, int32_t position)
{
    if (uri.empty()) {
        NAPI_ERR_LOG("Failed to open read only file, uri is empty");
        return -1;
    }
    std::string curUri = uri;
    bool isMediaLibUri = MediaFileUtils::IsMediaLibraryUri(uri);
    if (!isMediaLibUri) {
        std::vector<std::string> uris;
        if (!MediaFileUtils::SplitMovingPhotoUri(uri, uris)) {
            NAPI_ERR_LOG("Failed to open read only file, split moving photo failed");
            return -1;
        }
        curUri = uris[isReadImage ? MOVING_PHOTO_IMAGE_POS : MOVING_PHOTO_VIDEO_POS];
    }
    return isReadImage ? OpenReadOnlyImage(curUri, isMediaLibUri, position) :
        OpenReadOnlyVideo(curUri, isMediaLibUri, position);
}

int32_t MovingPhotoNapi::OpenReadOnlyLivePhoto(const string& destLivePhotoUri, int32_t position)
{
    if (destLivePhotoUri.empty()) {
        NAPI_ERR_LOG("Failed to open read only file, uri is empty");
        return E_ERR;
    }
    if (MediaFileUtils::IsMediaLibraryUri(destLivePhotoUri)) {
        std::string str = destLivePhotoUri;
        std::string MULTI_USER_URI_FLAG = "user=";
        size_t pos = str.find(MULTI_USER_URI_FLAG);
        std::string userId = "";
        if (pos != std::string::npos) {
            pos += MULTI_USER_URI_FLAG.length();
            size_t end = str.find_first_of("&?", pos);
            if (end == std::string::npos) {
                end = str.length();
            }
            userId = str.substr(pos, end - pos);
            NAPI_ERR_LOG("ReadMovingPhotoVideo for other user is %{public}s", userId.c_str());
        }
        string livePhotoUri = destLivePhotoUri;
        if (position == POSITION_CLOUD) {
            MediaFileUtils::UriAppendKeyValue(livePhotoUri, MEDIA_MOVING_PHOTO_OPRN_KEYWORD,
                OPEN_MOVING_PHOTO_VIDEO_CLOUD);
        } else {
            MediaFileUtils::UriAppendKeyValue(livePhotoUri, MEDIA_MOVING_PHOTO_OPRN_KEYWORD,
                OPEN_PRIVATE_LIVE_PHOTO);
        }
        Uri uri(livePhotoUri);
        return UserFileClient::OpenFile(uri, MEDIA_FILEMODE_READONLY, userId !="" ? stoi(userId) : -1);
    }
    return E_ERR;
}

int32_t MovingPhotoNapi::OpenReadOnlyMetadata(const string& movingPhotoUri)
{
    if (movingPhotoUri.empty()) {
        NAPI_ERR_LOG("Failed to open metadata of moving photo, uri is empty");
        return E_ERR;
    }

    if (!MediaFileUtils::IsMediaLibraryUri(movingPhotoUri)) {
        NAPI_ERR_LOG("Failed to check uri of moving photo: %{private}s", movingPhotoUri.c_str());
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
            NAPI_ERR_LOG("Failed to copy file from srcFd=%{public}d to destFd=%{public}d, errno=%{public}d",
                srcFd, destFd, errno);
            return E_HAS_FS_ERROR;
        }
    }

    if (bytesRead < 0) {
        NAPI_ERR_LOG("Failed to read from srcFd=%{public}d, errno=%{public}d", srcFd, errno);
        return E_HAS_FS_ERROR;
    }
    return E_OK;
}

static int32_t WriteToSandboxUri(int32_t srcFd, const string& sandboxUri,
    MovingPhotoResourceType type = MovingPhotoResourceType::DEFAULT)
{
    UniqueFd srcUniqueFd(srcFd);

    AppFileService::ModuleFileUri::FileUri fileUri(sandboxUri);
    string destPath = fileUri.GetRealPath();
    if (!MediaFileUtils::IsFileExists(destPath) && !MediaFileUtils::CreateFile(destPath)) {
        NAPI_ERR_LOG("Create empty dest file in sandbox failed, path:%{private}s", destPath.c_str());
        return E_HAS_FS_ERROR;
    }

    if (type == MovingPhotoResourceType::CLOUD_IMAGE) {
        return MovingPhotoFileUtils::ConvertToMovingPhoto(srcFd, destPath, "", "");
    } else if (type == MovingPhotoResourceType::CLOUD_VIDEO) {
        return MovingPhotoFileUtils::ConvertToMovingPhoto(srcFd, "", destPath, "");
    }

    int32_t destFd = MediaFileUtils::OpenFile(destPath, MEDIA_FILEMODE_READWRITE);
    if (destFd < 0) {
        NAPI_ERR_LOG("Open dest file failed, error: %{public}d", errno);
        return E_HAS_FS_ERROR;
    }
    UniqueFd destUniqueFd(destFd);

    if (ftruncate(destUniqueFd.Get(), 0) == -1) {
        NAPI_ERR_LOG("Truncate old file in sandbox failed, error:%{public}d", errno);
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
        NAPI_ERR_LOG("Open failed due to OpenFile failure, error: %{public}d", fd);
        return false;
    }
    return true;
}

static int32_t RequestContentToSandbox(napi_env env, MovingPhotoAsyncContext* context)
{
    string movingPhotoUri = context->movingPhotoUri;
    if (context->sourceMode == SourceMode::ORIGINAL_MODE) {
        MediaFileUtils::UriAppendKeyValue(movingPhotoUri, MEDIA_OPERN_KEYWORD, SOURCE_REQUEST);
    }
    if (!context->destImageUri.empty()) {
        int32_t imageFd = MovingPhotoNapi::OpenReadOnlyFile(movingPhotoUri, true, context->position);
        CHECK_COND_RET(HandleFd(imageFd), imageFd, "Open source image file failed");
        int32_t ret = WriteToSandboxUri(imageFd, context->destImageUri,
            context->position == POSITION_CLOUD ? MovingPhotoResourceType::CLOUD_IMAGE
                                                : MovingPhotoResourceType::DEFAULT);
        CHECK_COND_RET(ret == E_OK, ret, "Write image to sandbox failed");
    }
    if (!context->destVideoUri.empty()) {
        int32_t videoFd = MovingPhotoNapi::OpenReadOnlyFile(movingPhotoUri, false, context->position);
        CHECK_COND_RET(HandleFd(videoFd), videoFd, "Open source video file failed");
        if (context->compatibleMode == CompatibleMode::COMPATIBLE_FORMAT_MODE) {
            NAPI_DEBUG_LOG("movingPhoto CompatibleMode  COMPATIBLE_FORMAT_MODE");
            int32_t ret = MovingPhotoNapi::DoMovingPhotoTranscode(env, videoFd, context);
            CHECK_COND_RET(ret == E_OK, ret, "moving video transcode failed");
        } else {
            int32_t ret = WriteToSandboxUri(videoFd, context->destVideoUri,
                context->position == POSITION_CLOUD ? MovingPhotoResourceType::CLOUD_VIDEO
                                                    : MovingPhotoResourceType::DEFAULT);
            CHECK_COND_RET(ret == E_OK, ret, "Write video to sandbox failed");
        }
    }
    if (!context->destLivePhotoUri.empty()) {
        int32_t livePhotoFd = MovingPhotoNapi::OpenReadOnlyLivePhoto(movingPhotoUri, context->position);
        CHECK_COND_RET(HandleFd(livePhotoFd), livePhotoFd, "Open source video file failed");
        int32_t ret = WriteToSandboxUri(livePhotoFd, context->destLivePhotoUri);
        CHECK_COND_RET(ret == E_OK, ret, "Write video to sandbox failed");
    }
    if (!context->destMetadataUri.empty()) {
        int32_t extraDataFd = MovingPhotoNapi::OpenReadOnlyMetadata(movingPhotoUri);
        CHECK_COND_RET(HandleFd(extraDataFd), extraDataFd, "Open moving photo metadata failed");
        int32_t ret = WriteToSandboxUri(extraDataFd, context->destMetadataUri);
        CHECK_COND_RET(ret == E_OK, ret, "Write metadata to sandbox failed");
    }
    return E_OK;
}

static int32_t AcquireFdForArrayBuffer(MovingPhotoAsyncContext* context)
{
    int32_t fd = 0;
    string movingPhotoUri = context->movingPhotoUri;
    if (context->sourceMode == SourceMode::ORIGINAL_MODE) {
        MediaFileUtils::UriAppendKeyValue(movingPhotoUri, MEDIA_OPERN_KEYWORD, SOURCE_REQUEST);
    }
    switch (context->resourceType) {
        case ResourceType::IMAGE_RESOURCE: {
            fd = MovingPhotoNapi::OpenReadOnlyFile(movingPhotoUri, true, context->position);
            CHECK_COND_RET(HandleFd(fd), fd, "Open source image file failed");
            return fd;
        }
        case ResourceType::VIDEO_RESOURCE:
            fd = MovingPhotoNapi::OpenReadOnlyFile(movingPhotoUri, false, context->position);
            CHECK_COND_RET(HandleFd(fd), fd, "Open source video file failed");
            return fd;
        case ResourceType::PRIVATE_MOVING_PHOTO_RESOURCE:
            fd = MovingPhotoNapi::OpenReadOnlyLivePhoto(movingPhotoUri, context->position);
            CHECK_COND_RET(HandleFd(fd), fd, "Open live photo failed");
            return fd;
        case ResourceType::PRIVATE_MOVING_PHOTO_METADATA:
            fd = MovingPhotoNapi::OpenReadOnlyMetadata(movingPhotoUri);
            CHECK_COND_RET(HandleFd(fd), fd, "Open moving photo metadata failed");
            return fd;
        default:
            NAPI_ERR_LOG("Invalid resource type: %{public}d", static_cast<int32_t>(context->resourceType));
            return -EINVAL;
    }
}

int32_t MovingPhotoNapi::GetFdFromUri(const std::string &uri)
{
    AppFileService::ModuleFileUri::FileUri destUri(uri);
    string destPath = destUri.GetRealPath();
    if (!MediaFileUtils::IsFileExists(destPath) && !MediaFileUtils::CreateFile(destPath)) {
        NAPI_ERR_LOG("Create empty dest file in sandbox failed, path:%{private}s", destPath.c_str());
        return E_ERR;
    }
    return MediaFileUtils::OpenFile(destPath, MEDIA_FILEMODE_READWRITE);
}

static int32_t ArrayBufferToTranscode(napi_env env, MovingPhotoAsyncContext* context, int32_t fd)
{
    CHECK_COND_RET(context != nullptr, E_ERR, "context is null");
    {
        std::lock_guard<std::mutex> lockMutex(requestContentCompleteResultMutex);
        requestContentCompleteResult.Insert(context->requestId, context);
    }
    auto abilityContext = AbilityRuntime::Context::GetApplicationContext();
    CHECK_COND_RET(abilityContext != nullptr, E_ERR, "abilityContext is null");
    string cachePath = abilityContext->GetCacheDir();
    string destUri = cachePath + "/" +context->requestId + ".mp4";
    NAPI_DEBUG_LOG("destUri:%{public}s", destUri.c_str());
    int destFd = MovingPhotoNapi::GetFdFromUri(destUri);
    if (destFd < 0) {
        context->error = JS_INNER_FAIL;
        NAPI_ERR_LOG("get destFd fail");
        return E_ERR;
    }
    UniqueFd uniqueFd(fd);
    UniqueFd uniqueDestFd(destFd);
    int64_t offset = 0;
    int64_t videoSize = 0;
    int64_t extraDataSize = 0;
    if (context->position == POSITION_CLOUD) {
        int32_t ret = MovingPhotoFileUtils::GetMovingPhotoDetailedSize(uniqueFd.Get(), offset, videoSize,
            extraDataSize);
        if (ret != E_OK) {
            context->error = JS_INNER_FAIL;
            NAPI_ERR_LOG("get moving photo detailed size fail");
            return E_ERR;
        }
    } else {
        struct stat statSrc;
        if (fstat(uniqueFd.Get(), &statSrc) == E_ERR) {
            NAPI_DEBUG_LOG("File get stat failed, %{public}d", errno);
            return E_HAS_FS_ERROR;
        }
        videoSize = statSrc.st_size;
    }
    MediaCallTranscode::RegisterCallback(context->callback);
    if (!MediaCallTranscode::DoTranscode(uniqueFd.Get(), uniqueDestFd.Get(), videoSize, context->requestId, offset)) {
        NAPI_INFO_LOG("DoTranscode fail");
        return E_GET_PRAMS_FAIL;
    }
    {
        std::lock_guard<std::mutex> lock(isMovingPhotoTranscoderMapMutex);
        isMovingPhotoTranscoderMap.Insert(context->requestId, true);
    }
    return E_OK;
}

static int32_t RequestContentToArrayBuffer(napi_env env, MovingPhotoAsyncContext* context)
{
    if (context == nullptr) {
        NAPI_INFO_LOG("context is null");
        return E_ERR;
    }
    int32_t fd = AcquireFdForArrayBuffer(context);
    if (fd < 0) {
        return fd;
    }
    if (context->resourceType == ResourceType::VIDEO_RESOURCE &&
        context->compatibleMode == CompatibleMode::COMPATIBLE_FORMAT_MODE) {
        {
            std::lock_guard<std::mutex> lockMutex(requestContentCompleteResultMutex);
            requestContentCompleteResult.Insert(context->requestId, context);
        }
        return ArrayBufferToTranscode(env, context, fd);
    }
    MovingPhotoNapi::SubRequestContent(fd, context);
    return E_OK;
}

static bool IsValidResourceType(int32_t resourceType)
{
    return resourceType == static_cast<int32_t>(ResourceType::IMAGE_RESOURCE) ||
           resourceType == static_cast<int32_t>(ResourceType::VIDEO_RESOURCE) ||
           (resourceType == static_cast<int32_t>(ResourceType::PRIVATE_MOVING_PHOTO_RESOURCE) &&
               MediaLibraryNapiUtils::IsSystemApp()) ||
           (resourceType == static_cast<int32_t>(ResourceType::PRIVATE_MOVING_PHOTO_METADATA) &&
               MediaLibraryNapiUtils::IsSystemApp());
}

static int32_t QueryPhotoPosition(const string &movingPhotoUri, bool hasReadPermission, int32_t &position)
{
    if (!MediaFileUtils::IsMediaLibraryUri(movingPhotoUri)) {
        position = static_cast<int32_t>(PhotoPositionType::LOCAL);
        return E_OK;
    }

    std::string MULTI_USER_URI_FLAG = "user=";
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
        NAPI_INFO_LOG("QueryPhotoPosition for other user is %{public}s", userIdStr.c_str());
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
    int errCode = 0;
    auto resultSet = UserFileClient::Query(uri, predicates, fetchColumn, errCode, userId);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != E_OK) {
        NAPI_ERR_LOG("query resultSet is nullptr");
        return E_ERR;
    }

    int index;
    int err = resultSet->GetColumnIndex(PhotoColumn::PHOTO_POSITION, index);
    if (err != E_OK) {
        NAPI_ERR_LOG("Failed to GetColumnIndex");
        return E_ERR;
    }
    resultSet->GetInt(index, position);
    return E_OK;
}

static bool HasReadPermission()
{
    static bool result = (AccessTokenKit::VerifyAccessToken(IPCSkeleton::GetSelfTokenID(), PERM_READ_IMAGEVIDEO)
        == PermissionState::PERMISSION_GRANTED);
    return result;
}

static napi_value ParseArgsForRequestContent(napi_env env, size_t argc, const napi_value argv[],
    MovingPhotoNapi* thisArg, unique_ptr<MovingPhotoAsyncContext>& context)
{
    CHECK_COND_WITH_MESSAGE(env, (argc == ARGS_ONE || argc == ARGS_TWO), "Invalid number of arguments");
    CHECK_COND(env, thisArg != nullptr, JS_INNER_FAIL);
    context->movingPhotoUri = thisArg->GetUri();
    context->sourceMode = thisArg->GetSourceMode();
    context->compatibleMode = thisArg->GetCompatibleMode();
    context->callback = thisArg->GetMovingPhotoCallback();
    context->requestId = thisArg->GetRequestId();

    int32_t resourceType = 0;
    if (argc == ARGS_ONE) {
        // return by array buffer
        CHECK_ARGS(env, napi_get_value_int32(env, argv[ARGS_ZERO], &resourceType), JS_INNER_FAIL);
        CHECK_COND_WITH_MESSAGE(env, IsValidResourceType(resourceType), "Invalid resource type");
        context->requestContentMode = MovingPhotoAsyncContext::WRITE_TO_ARRAY_BUFFER;
        context->resourceType = static_cast<ResourceType>(resourceType);
    } else if (argc == ARGS_TWO) {
        context->requestContentMode = MovingPhotoAsyncContext::WRITE_TO_SANDBOX;
        napi_valuetype valueTypeFront;
        napi_valuetype valueTypeBack;
        CHECK_ARGS(env, napi_typeof(env, argv[ARGS_ZERO], &valueTypeFront), JS_INNER_FAIL);
        CHECK_ARGS(env, napi_typeof(env, argv[ARGS_ONE], &valueTypeBack), JS_INNER_FAIL);
        if (valueTypeFront == napi_string && valueTypeBack == napi_string) {
            // write both image and video to sandbox
            CHECK_ARGS(env, MediaLibraryNapiUtils::GetParamStringPathMax(env, argv[ARGS_ZERO], context->destImageUri),
                JS_INNER_FAIL);
            CHECK_ARGS(env, MediaLibraryNapiUtils::GetParamStringPathMax(env, argv[ARGS_ONE], context->destVideoUri),
                JS_INNER_FAIL);
        } else if (valueTypeFront == napi_number && valueTypeBack == napi_string) {
            // write specific resource to sandbox
            CHECK_ARGS(env, napi_get_value_int32(env, argv[ARGS_ZERO], &resourceType), JS_INNER_FAIL);
            CHECK_COND_WITH_MESSAGE(env, IsValidResourceType(resourceType), "Invalid resource type");
            if (resourceType == static_cast<int32_t>(ResourceType::IMAGE_RESOURCE)) {
                CHECK_ARGS(env, MediaLibraryNapiUtils::GetParamStringPathMax(env, argv[ARGS_ONE],
                    context->destImageUri), JS_INNER_FAIL);
            } else if (resourceType == static_cast<int32_t>(ResourceType::VIDEO_RESOURCE)) {
                CHECK_ARGS(env, MediaLibraryNapiUtils::GetParamStringPathMax(env, argv[ARGS_ONE],
                    context->destVideoUri), JS_INNER_FAIL);
            } else if (resourceType == static_cast<int32_t>(ResourceType::PRIVATE_MOVING_PHOTO_RESOURCE)) {
                CHECK_ARGS(env, MediaLibraryNapiUtils::GetParamStringPathMax(env, argv[ARGS_ONE],
                    context->destLivePhotoUri), JS_INNER_FAIL);
            } else {
                CHECK_ARGS(env, MediaLibraryNapiUtils::GetParamStringPathMax(env, argv[ARGS_ONE],
                    context->destMetadataUri), JS_INNER_FAIL);
            }
            context->resourceType = static_cast<ResourceType>(resourceType);
        } else {
            NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, __FUNCTION__, __LINE__, "Invalid type of arguments");
            return nullptr;
        }
    }
    RETURN_NAPI_TRUE(env);
}

static void RequestContentExecute(napi_env env, void *data)
{
    auto* context = static_cast<MovingPhotoAsyncContext*>(data);
    int32_t ret = QueryPhotoPosition(context->movingPhotoUri, HasReadPermission(), context->position);
    if (ret != E_OK) {
        NAPI_ERR_LOG("Failed to query position of moving photo, ret: %{public}d", ret);
        context->SaveError(ret);
        return;
    }
    switch (context->requestContentMode) {
        case MovingPhotoAsyncContext::WRITE_TO_SANDBOX:
            ret = RequestContentToSandbox(env, context);
            break;
        case MovingPhotoAsyncContext::WRITE_TO_ARRAY_BUFFER:
            ret = RequestContentToArrayBuffer(env, context);
            break;
        default:
            NAPI_ERR_LOG("Invalid request content mode: %{public}d", static_cast<int32_t>(context->requestContentMode));
            context->error = OHOS_INVALID_PARAM_CODE;
            return;
    }
    if (ret != E_OK) {
        context->SaveError(ret);
        return;
    }
}

static void RequestContentComplete(napi_env env, napi_status status, void *data)
{
    MovingPhotoAsyncContext *context = static_cast<MovingPhotoAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    bool isTranscoder = false;
    {
        std::lock_guard<std::mutex> lock(isMovingPhotoTranscoderMapMutex);
        isMovingPhotoTranscoderMap.Find(context->requestId, isTranscoder);
    }
    if (isTranscoder) {
        std::lock_guard<std::mutex> lockMutex(requestContentCompleteResultMutex);
        requestContentCompleteResult.EnsureInsert(context->requestId, context);
        return;
    }
    napi_value outBuffer = nullptr;
    if (context->error == E_OK && context->requestContentMode == MovingPhotoAsyncContext::WRITE_TO_ARRAY_BUFFER) {
        napi_status status = napi_create_external_arraybuffer(
            env, context->arrayBufferData, context->arrayBufferLength,
            [](napi_env env, void* data, void* hint) { free(data); }, nullptr, &outBuffer);
        if (status != napi_ok) {
            NAPI_ERR_LOG("Failed to create array buffer object, uri is %{public}s, resource type is %{public}d",
                context->movingPhotoUri.c_str(), context->resourceType);
            free(context->arrayBufferData);
            context->arrayBufferData = nullptr;
            context->error = JS_INNER_FAIL;
        }
    } else if (context->arrayBufferData != nullptr) {
        free(context->arrayBufferData);
        context->arrayBufferData = nullptr;
    }

    unique_ptr<JSAsyncContextOutput> outContext = make_unique<JSAsyncContextOutput>();
    outContext->status = false;
    napi_get_undefined(env, &outContext->data);

    if (context->error != E_OK) {
        context->HandleError(env, outContext->error);
    } else {
        outContext->status = true;
        if (context->requestContentMode == MovingPhotoAsyncContext::WRITE_TO_ARRAY_BUFFER) {
            outContext->data = outBuffer;
        }
    }
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, nullptr,
                                                   context->work, *outContext);
    } else {
        NAPI_ERR_LOG("Async work is nullptr");
    }
    delete context;
}

napi_value MovingPhotoNapi::JSRequestContent(napi_env env, napi_callback_info info)
{
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;
    CHECK_ARGS(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr), JS_INNER_FAIL);

    unique_ptr<MovingPhotoAsyncContext> asyncContext = make_unique<MovingPhotoAsyncContext>();
    MovingPhotoNapi* nativeObject;
    CHECK_ARGS(env, napi_unwrap(env, thisVar, reinterpret_cast<void **>(&nativeObject)), JS_INNER_FAIL);
    CHECK_NULLPTR_RET(ParseArgsForRequestContent(env, argc, argv, nativeObject, asyncContext));

    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSRequestContent",
        RequestContentExecute, RequestContentComplete);
}

napi_value MovingPhotoNapi::NewMovingPhotoNapi(napi_env env, const string& photoUri, SourceMode sourceMode,
    MovingPhotoParam movingPhotoParam, const std::function<void(int, int, std::string)> callbacks)
{
    napi_value constructor = nullptr;
    napi_value instance = nullptr;
    napi_value napiStringUri = nullptr;
    napi_status status = napi_create_string_utf8(env, photoUri.c_str(), NAPI_AUTO_LENGTH, &napiStringUri);
    CHECK_COND_RET(status == napi_ok, nullptr, "Failed to create napi string, napi status: %{public}d",
        static_cast<int>(status));
    status = napi_get_reference_value(env, constructor_, &constructor);
    CHECK_COND_RET(status == napi_ok, nullptr, "Failed to get reference of constructor, napi status: %{public}d",
        static_cast<int>(status));
    status = napi_new_instance(env, constructor, 1, &napiStringUri, &instance);
    CHECK_COND_RET(status == napi_ok, nullptr, "Failed to get new instance of moving photo, napi status: %{public}d",
        static_cast<int>(status));
    CHECK_COND_RET(instance != nullptr, nullptr, "Instance is nullptr");

    MovingPhotoNapi* movingPhotoNapi = nullptr;
    status = napi_unwrap(env, instance, reinterpret_cast<void**>(&movingPhotoNapi));
    CHECK_COND_RET(status == napi_ok, nullptr, "Failed to unwarp instance of MovingPhotoNapi");
    CHECK_COND_RET(movingPhotoNapi != nullptr, nullptr, "movingPhotoNapi is nullptr");
    movingPhotoNapi->SetSourceMode(sourceMode);
    movingPhotoNapi->SetRequestId(movingPhotoParam.requestId);
    movingPhotoNapi->SetCompatibleMode(movingPhotoParam.compatibleMode);
    movingPhotoNapi->SetMovingPhotoCallback(callbacks);
    return instance;
}

napi_value MovingPhotoNapi::JSGetUri(napi_env env, napi_callback_info info)
{
    MovingPhotoNapi *obj = nullptr;
    napi_value thisVar = nullptr;
    CHECK_ARGS(env, napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr), JS_INNER_FAIL);
    if (thisVar == nullptr) {
        NapiError::ThrowError(env, JS_INNER_FAIL);
        return nullptr;
    }

    CHECK_ARGS(env, napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj)), JS_INNER_FAIL);
    if (obj == nullptr) {
        NapiError::ThrowError(env, JS_INNER_FAIL);
        return nullptr;
    }

    napi_value jsResult = nullptr;
    CHECK_ARGS(env, napi_create_string_utf8(env, obj->GetUri().c_str(), NAPI_AUTO_LENGTH, &jsResult), JS_INNER_FAIL);
    return jsResult;
}

int32_t MovingPhotoNapi::DoMovingPhotoTranscode(napi_env env, int32_t &videoFd, MovingPhotoAsyncContext* context)
{
    CHECK_COND_RET(context != nullptr, E_ERR, "context is null");
    if (videoFd == -1) {
        NAPI_INFO_LOG("videoFd is null");
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
            NAPI_DEBUG_LOG("File get stat failed, %{public}d", errno);
            return E_HAS_FS_ERROR;
        }
        videoSize = statSrc.st_size;
    }
    AppFileService::ModuleFileUri::FileUri fileUri(context->destVideoUri);
    string destPath = fileUri.GetRealPath();
    if (!MediaFileUtils::IsFileExists(destPath) && !MediaFileUtils::CreateFile(destPath)) {
        NAPI_ERR_LOG("Create empty dest file in sandbox failed, path:%{private}s", destPath.c_str());
        return E_HAS_FS_ERROR;
    }
    int32_t destFd = MediaFileUtils::OpenFile(destPath, MEDIA_FILEMODE_READWRITE);
    if (destFd < 0) {
        close(destFd);
        NAPI_ERR_LOG("Open dest file failed, error: %{public}d", errno);
        return E_HAS_FS_ERROR;
    }
    UniqueFd uniqueDestFd(destFd);
    MediaCallTranscode::RegisterCallback(context->callback);
    bool result = MediaCallTranscode::DoTranscode(uniqueVideoFd.Get(), uniqueDestFd.Get(), videoSize,
        context->requestId, offset);
    if (!result) {
        NAPI_ERR_LOG("DoTranscode fail");
        return E_GET_PRAMS_FAIL;
    }
    {
        std::lock_guard<std::mutex> lock(isMovingPhotoTranscoderMapMutex);
        isMovingPhotoTranscoderMap.Insert(context->requestId, true);
    }
    return E_OK;
}

static void DeleteProcessHandlerSafe(ProgressHandler *handler, napi_env env)
{
    if (handler == nullptr) {
        return;
    }
    if (handler->progressRef != nullptr && env != nullptr) {
        napi_delete_reference(env, handler->progressRef);
        handler->progressRef = nullptr;
    }
    delete handler;
    handler = nullptr;
}

void CallMovingProgressCallback(napi_env env, ProgressHandler &progressHandler, int32_t process)
{
    napi_value result;
    napi_status status = napi_create_int32(env, process, &result);
    if (status != napi_ok) {
        NAPI_ERR_LOG("OnProgress napi_create_int32 fail");
    }
    napi_value callback;
    status = napi_get_reference_value(env, progressHandler.progressRef, &callback);
    if (status != napi_ok) {
        NAPI_ERR_LOG("OnProgress napi_get_reference_value fail, napi status: %{public}d",
            static_cast<int>(status));
        DeleteProcessHandlerSafe(&progressHandler, env);
        return;
    }
    napi_value jsOnProgress;
    status = napi_get_named_property(env, callback, ON_PROGRESS_FUNC, &jsOnProgress);
    if (status != napi_ok) {
        NAPI_ERR_LOG("jsOnProgress napi_get_named_property fail, napi status: %{public}d",
            static_cast<int>(status));
        DeleteProcessHandlerSafe(&progressHandler, env);
        return;
    }
    napi_value argv[1];
    size_t argc = ARGS_ONE;
    argv[PARAM0] = result;
    napi_value promise;
    status = napi_call_function(env, nullptr, jsOnProgress, argc, argv, &promise);
    if (status != napi_ok) {
        NAPI_ERR_LOG("call js function failed %{public}d", static_cast<int32_t>(status));
        NapiError::ThrowError(env, JS_INNER_FAIL, "calling onDataPrepared failed");
    }
    NAPI_DEBUG_LOG("CallProgressCallback process %{public}d", process);
}

void MovingPhotoNapi::RequestCloudContentArrayBuffer(int32_t fd, MovingPhotoAsyncContext* context)
{
    if (context->position != POSITION_CLOUD) {
        NAPI_ERR_LOG("Failed to check postion: %{public}d", context->position);
        context->arrayBufferData = nullptr;
        context->error = JS_INNER_FAIL;
        return;
    }

    int64_t imageSize = 0;
    int64_t videoSize = 0;
    int64_t extraDataSize = 0;
    int32_t err = MovingPhotoFileUtils::GetMovingPhotoDetailedSize(fd, imageSize, videoSize, extraDataSize);
    if (err != E_OK) {
        NAPI_ERR_LOG("Failed to get detailed size of moving photo");
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
            NAPI_ERR_LOG("Invalid resource type: %{public}d", static_cast<int32_t>(context->resourceType));
            break;
    }

    if (!context->arrayBufferData || ret != E_OK) {
        NAPI_ERR_LOG(
            "Failed to get arraybuffer, resource type is %{public}d", static_cast<int32_t>(context->resourceType));
        context->error = JS_INNER_FAIL;
        return;
    }
    context->arrayBufferLength = fileSize;
}

void BufferTranscodeRequestContent(int32_t fd, MovingPhotoAsyncContext* context)
{
    UniqueFd uniqueFd(fd);
    off_t fileLen = lseek(uniqueFd.Get(), 0, SEEK_END);
    if (fileLen < 0) {
        NAPI_ERR_LOG("Failed to get file length, error: %{public}d", errno);
        context->SaveError(E_HAS_FS_ERROR);
        return;
    }

    off_t ret = lseek(uniqueFd.Get(), 0, SEEK_SET);
    if (ret < 0) {
        NAPI_ERR_LOG("Failed to reset file offset, error: %{public}d", errno);
        context->SaveError(E_HAS_FS_ERROR);
        return;
    }

    size_t fileSize = static_cast<size_t>(fileLen);
    context->arrayBufferData = malloc(fileSize);
    if (!context->arrayBufferData) {
        NAPI_ERR_LOG("Failed to malloc array buffer data, moving photo uri is %{public}s, resource type is %{public}d",
            context->movingPhotoUri.c_str(), static_cast<int32_t>(context->resourceType));
            context->error = JS_INNER_FAIL;
        return;
    }
    size_t readBytes = static_cast<size_t>(read(uniqueFd.Get(), context->arrayBufferData, fileSize));
    if (readBytes != fileSize) {
        NAPI_ERR_LOG("read file failed, read bytes is %{public}zu, actual length is %{public}zu, "
            "error: %{public}d", readBytes, fileSize, errno);
        free(context->arrayBufferData);
        context->arrayBufferData = nullptr;
        context->SaveError(E_HAS_FS_ERROR);
        return;
    }
    context->arrayBufferLength = fileSize;
    return;
}

void MovingPhotoNapi::SubRequestContent(int32_t fd, MovingPhotoAsyncContext* context)
{
    if (context->position == POSITION_CLOUD) {
        return RequestCloudContentArrayBuffer(fd, context);
    }
    BufferTranscodeRequestContent(fd, context);
}

void CallArrayBufferRequestContentComplete(napi_env env, MovingPhotoAsyncContext* context)
{
    auto abilityContext = AbilityRuntime::Context::GetApplicationContext();
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    CHECK_NULL_PTR_RETURN_VOID(abilityContext, "Async abilityContext is null");
    string cachePath = abilityContext->GetCacheDir();
    string destUri = cachePath + "/" +context->requestId + ".mp4";
    NAPI_INFO_LOG("CallArrayBufferRequestContentComplete start destUri:%{public}s", destUri.c_str());
    int fd = MovingPhotoNapi::GetFdFromUri(destUri);
    if (fd < 0) {
        NAPI_ERR_LOG("get fd fail");
        context->error = JS_INNER_FAIL;
        return;
    }
    UniqueFd uniqueFd(fd);
    BufferTranscodeRequestContent(uniqueFd.Get(), context);
    if (!MediaFileUtils::DeleteFile(destUri)) {
        NAPI_WARN_LOG("remove fail, errno:%{public}d", errno);
    }
    return;
}

void MovingPhotoNapi::OnProgress(napi_env env, napi_value cb, void *context, void *data)
{
    ProgressHandler *progressHandler = reinterpret_cast<ProgressHandler *>(data);
    if (progressHandler == nullptr) {
        NAPI_ERR_LOG("progressHandler handler is nullptr");
        DeleteProcessHandlerSafe(progressHandler, env);
        return;
    }
    int32_t process = progressHandler->retProgressValue.progress;
    int32_t type = progressHandler->retProgressValue.type;
    if ((type == INFO_TYPE_TRANSCODER_COMPLETED) || type == INFO_TYPE_ERROR) {
        MediaCallTranscode::CallTranscodeRelease(progressHandler->requestId);
        bool isTranscoder;
        {
            std::lock_guard<std::mutex> lock(isMovingPhotoTranscoderMapMutex);
            if (isMovingPhotoTranscoderMap.Find(progressHandler->requestId, isTranscoder)) {
                isMovingPhotoTranscoderMap.Erase(progressHandler->requestId);
            }
        }
        MovingPhotoAsyncContext* context;
        {
            std::lock_guard<std::mutex> lockMutex(requestContentCompleteResultMutex);
            if (!requestContentCompleteResult.Find(progressHandler->requestId, context)) {
                NAPI_ERR_LOG("find context fail requestId:%{public}s", progressHandler->requestId.c_str());
                return;
            }
            requestContentCompleteResult.Erase(progressHandler->requestId);
        }
        CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
        if (type == INFO_TYPE_ERROR) {
            context->error = JS_INNER_FAIL;
        }
        CallRequestContentCallBack(env, context);
        NAPI_INFO_LOG("OnProgress INFO_TYPE_TRANSCODER_COMPLETED type:%{public}d, process:%{public}d", type, process);
        DeleteProcessHandlerSafe(progressHandler, env);
        return;
    }
    CHECK_NULL_PTR_RETURN_VOID(progressHandler->progressRef, "Onprogress callback is null");
    CallMovingProgressCallback(env, *progressHandler, process);
}

void MovingPhotoNapi::CallRequestContentCallBack(napi_env env, MovingPhotoAsyncContext* context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    napi_status status = napi_ok;
    switch (context->requestContentMode) {
        case MovingPhotoAsyncContext::WRITE_TO_SANDBOX:
            RequestContentComplete(env, status, context);
            return;
        case MovingPhotoAsyncContext::WRITE_TO_ARRAY_BUFFER:
            CallArrayBufferRequestContentComplete(env, context);
            RequestContentComplete(env, status, context);
            return;
        default:
            NAPI_ERR_LOG("Request content mode: %{public}d", static_cast<int32_t>(context->requestContentMode));
            context->error = OHOS_INVALID_PARAM_CODE;
            return;
    }
}
} // namespace Media
} // namespace OHOS