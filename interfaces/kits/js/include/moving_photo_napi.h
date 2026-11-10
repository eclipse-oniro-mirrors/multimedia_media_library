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

#ifndef INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_MOVING_PHOTO_NAPI_H
#define INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_MOVING_PHOTO_NAPI_H

#include <memory>

#include "media_asset_data_handler.h"
#include "media_library_napi.h"

namespace OHOS {
namespace Media {
class MovingPhotoNapi;

struct MovingPhotoAsyncContext : public NapiError {
    enum RequestContentMode {
        WRITE_TO_SANDBOX,
        WRITE_TO_ARRAY_BUFFER,
        UNDEFINED,
    };

    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef;
    MovingPhotoNapi *objectInfo = nullptr;

    std::string movingPhotoUri;
    SourceMode sourceMode;
    bool isFromSingleFile = false;
    CompatibleMode compatibleMode;
    napi_ref progressHandlerRef;
    napi_env mediaAssetEnv;
    bool isTranscoder = false;
    napi_threadsafe_function threadsafeFunction;
    std::string requestId;
    ResourceType resourceType;
    std::string destImageUri;
    std::string destVideoUri;
    std::string destLivePhotoUri;
    std::string destMetadataUri;
    RequestContentMode requestContentMode = UNDEFINED;
    void* arrayBufferData = nullptr;
    size_t arrayBufferLength = 0;
    int32_t position = 0;
    bool isVideoReady = false;
    size_t argc;
    napi_value argv[NAPI_ARGC_MAX];
};

struct MovingPhotoParam {
    std::string requestId;
    CompatibleMode compatibleMode;
    napi_ref progressHandlerRef;
    napi_threadsafe_function threadsafeFunction;
    MovingPhotoParam() : requestId(""), compatibleMode(CompatibleMode::ORIGINAL_FORMAT_MODE),
        progressHandlerRef(nullptr), threadsafeFunction(nullptr) {};
};

class MovingPhotoNapi {
public:
    MovingPhotoNapi(const std::string& photoUri) : photoUri_(photoUri) {};
    ~MovingPhotoNapi() = default;
    EXPORT static napi_value Init(napi_env env, napi_value exports);
    static int32_t OpenReadOnlyFile(const string& uri, bool isReadImage, int32_t position);
    static int32_t OpenReadOnlyLivePhoto(const string& destLivePhotoUri, int32_t position);
    static int32_t OpenReadOnlyMetadata(const string& movingPhotoUri);
    static napi_value NewMovingPhotoNapi(napi_env env, const string& photoUri, SourceMode sourceMode,
        MovingPhotoParam &movingPhotoParam);
    static napi_value NewSingleFileMovingPhotoNapi(napi_env env, const string& photoUri);
    std::string GetUri();
    SourceMode GetSourceMode();
    void SetSourceMode(SourceMode sourceMode);
    bool IsFromSingleFile() const;
    void SetIsFromSingleFile(bool isFromSingleFile);
    std::string GetRequestId();
    void SetRequestId(const std::string requestId);
    CompatibleMode GetCompatibleMode();
    void SetCompatibleMode(const CompatibleMode compatibleMode);
    napi_ref GetProgressHandlerRef();
    void SetProgressHandlerRef(napi_ref &progressHandlerRef);
    napi_env GetMediaAssetEnv();
    void SetMediaAssetEnv(napi_env mediaAssetEnv);
    napi_threadsafe_function GetThreadsafeFunction()
    {
        return threadsafeFunction_;
    }
    void SetThreadsafeFunction(napi_threadsafe_function threadsafeFunction)
    {
        threadsafeFunction_ = threadsafeFunction;
    }
    static int32_t DoMovingPhotoTranscode(napi_env env, int32_t &videoFd, MovingPhotoAsyncContext* context);
    static void OnProgress(napi_env env, napi_value cb, void *context, void *data);
    static int32_t GetFdFromUri(const std::string &sandBoxUri);
    static void SubRequestContent(int32_t fd, MovingPhotoAsyncContext* context);
    static void RequestLivePhotoContentArrayBuffer(int32_t fd, MovingPhotoAsyncContext* context);
    static void CallRequestContentCallBack(napi_env env, void* context, int32_t errorCode);
private:
    EXPORT static napi_value Constructor(napi_env env, napi_callback_info info);
    EXPORT static void Destructor(napi_env env, void* nativeObject, void* finalizeHint);

    EXPORT static napi_value JSRequestContent(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetUri(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSIsVideoReady(napi_env env, napi_callback_info info);

    static thread_local napi_ref constructor_;
    std::string photoUri_;
    SourceMode sourceMode_ = SourceMode::EDITED_MODE;
    CompatibleMode compatibleMode_ = CompatibleMode::COMPATIBLE_FORMAT_MODE;
    std::string requestId_;
    bool isFromSingleFile_ = false;
    napi_ref progressHandlerRef_ = nullptr;
    napi_env media_asset_env_ = nullptr;
    napi_threadsafe_function threadsafeFunction_ = nullptr;
};

} // Media
} // OHOS
#endif // INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_MOVING_PHOTO_NAPI_H