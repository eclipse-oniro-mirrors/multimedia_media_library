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

#ifndef FRAMEWORKS_ANI_SRC_INCLUDE_MOVING_PHOTO_ANI_H
#define FRAMEWORKS_ANI_SRC_INCLUDE_MOVING_PHOTO_ANI_H

#include <string>
#include "ani_error.h"
#include "media_library_enum_ani.h"
#include "progress_handler.h"

namespace OHOS {
namespace Media {
struct MovingPhotoParam {
    std::string requestId;
    CompatibleMode compatibleMode;
    ani_ref progressHandlerRef;
    ThreadFunctionOnProgress threadsafeFunction;
    MovingPhotoParam() : requestId(""), compatibleMode(CompatibleMode::ORIGINAL_FORMAT_MODE),
        progressHandlerRef(nullptr), threadsafeFunction(nullptr) {}
};
struct MovingPhotoAsyncContext;

class MovingPhotoAni {
public:
    explicit MovingPhotoAni(const std::string& photoUri) : photoUri_(photoUri) {}
    ~MovingPhotoAni() = default;
    static ani_status Init(ani_env *env);
    static MovingPhotoAni* Unwrap(ani_env *env, ani_object object);
    static int32_t OpenReadOnlyFile(const std::string& uri, bool isReadImage, int32_t position);
    static int32_t OpenReadOnlyLivePhoto(const std::string& destLivePhotoUri, int32_t position);
    static int32_t OpenReadOnlyMetadata(const std::string& movingPhotoUri);
    static ani_object NewMovingPhotoAni(ani_env *env, const std::string& photoUri, SourceMode sourceMode,
        MovingPhotoParam &movingPhotoParam);
    static void SubRequestContent(int32_t fd, MovingPhotoAsyncContext* context);
    static void RequestCloudContentArrayBuffer(int32_t fd, MovingPhotoAsyncContext* context);
    std::string GetUriInner();
    SourceMode GetSourceMode();
    static int32_t GetFdFromUri(const std::string &uri);
    void SetSourceMode(SourceMode sourceMode);
    std::string GetRequestId();
    void SetRequestId(const std::string &requestId);
    CompatibleMode GetCompatibleMode();
    void SetCompatibleMode(const CompatibleMode compatibleMode);
    ani_ref GetProgressHandlerRef();
    void SetProgressHandlerRef(ani_ref &progressHandlerRef);
    ani_vm *GetEtsVm() const;
    void SetEtsVm(ani_vm *etsVm);
    ThreadFunctionOnProgress GetThreadsafeFunction() const;
    void SetThreadsafeFunction(ThreadFunctionOnProgress threadsafeFunction);
    static int32_t DoMovingPhotoTranscode(int32_t &videoFd, MovingPhotoAsyncContext* context);
    static void AfterTranscoder(void *context, int32_t errCode);
    static ani_ref TransferToDynamicMovingPhoto(ani_env *env, [[maybe_unused]] ani_class, ani_object input);
    static ani_object TransferToStaticMovingPhoto(ani_env *env, [[maybe_unused]] ani_class, ani_object input);

private:
    static ani_object Constructor(ani_env *env, [[maybe_unused]] ani_class clazz, ani_string photoUriAni);

    static void RequestContentByImageFileAndVideoFile(ani_env *env, ani_object object,
        ani_string imageFileUri, ani_string videoFileUri);
    static void RequestContentByResourceTypeAndFile(ani_env *env, ani_object object,
        ani_enum_item resourceTypeAni, ani_string fileUri);
    static ani_object RequestContentByResourceType(ani_env *env, ani_object object,
        ani_enum_item resourceTypeAni);
    static ani_string GetUri(ani_env *env, ani_object object);
    static ani_boolean IsVideoReady(ani_env *env, ani_object object);

    std::string photoUri_;
    SourceMode sourceMode_ = SourceMode::EDITED_MODE;
    CompatibleMode compatibleMode_ = CompatibleMode::COMPATIBLE_FORMAT_MODE;
    ani_ref progressHandlerRef_ = nullptr;
    ani_vm *etsVm_ = nullptr;
    ThreadFunctionOnProgress threadsafeFunction_ = nullptr;
    std::string requestId_;
};

struct MovingPhotoAsyncContext : public AniError {
    enum RequestContentMode {
        WRITE_TO_SANDBOX,
        WRITE_TO_ARRAY_BUFFER,
        UNDEFINED,
    };

    std::string movingPhotoUri;
    SourceMode sourceMode;
    CompatibleMode compatibleMode;
    std::function<void(int, int, std::string)> callback;
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
    // for transcode
    ani_ref progressHandlerRef = nullptr;
    ani_vm *etsVm = nullptr;
    ThreadFunctionOnProgress threadsafeFunction = nullptr;
    std::condition_variable cv;
    std::mutex mutex;
    std::atomic_bool isTranscoder {false};
};
} // namespace Media
} // namespace OHOS
#endif // FRAMEWORKS_ANI_SRC_INCLUDE_MOVING_PHOTO_ANI_H