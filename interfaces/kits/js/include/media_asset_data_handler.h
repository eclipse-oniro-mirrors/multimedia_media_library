/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_MEDIA_ASSET_DATA_HANDLER_NAPI_H
#define INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_MEDIA_ASSET_DATA_HANDLER_NAPI_H

#include "medialibrary_napi_utils.h"

namespace OHOS {
namespace Media {
enum class ReturnDataType {
    TYPE_IMAGE_SOURCE = 0,
    TYPE_ARRAY_BUFFER,
    TYPE_MOVING_PHOTO,
    TYPE_TARGET_PATH,
    TYPE_PICTURE,
    TYPE_READY,
};

enum class DeliveryMode {
    FAST = 0,
    HIGH_QUALITY,
    BALANCED_MODE,
};

enum class SourceMode {
    ORIGINAL_MODE = 0,
    EDITED_MODE,
};

enum class NotifyMode : int32_t {
    FAST_NOTIFY = 0,
    WAIT_FOR_HIGH_QUALITY,
};

enum class ProgressMode : int32_t {
    ONLY_FOR_CAMERA = 0,
    ONLY_FOR_TRANSCODING,
    CAMERA_AND_TRANSCODING,
    UNDEFINED = 10,
};

enum class CameraProgressMode : int32_t {
    IMMEDIATELY_MODE = 0,   // 立即返回
    WAIT_FOR_POLLOT,
    UNDEFINED = 10,
};

enum class CompatibleMode {
    ORIGINAL_FORMAT_MODE = 0,
    COMPATIBLE_FORMAT_MODE = 1,
};

constexpr const char* ON_DATA_PREPARED_FUNC = "onDataPrepared";
constexpr const char* ON_PROGRESS_FUNC = "onProgress";

class NapiMediaAssetDataHandler {
public:
    NapiMediaAssetDataHandler(napi_env env, napi_ref dataHandler, ReturnDataType dataType, const std::string &uri,
        const std::string &destUri, SourceMode sourceMode);
    void DeleteNapiReference(napi_env env);
    ReturnDataType GetReturnDataType();
    std::string GetRequestUri();
    std::string GetDestUri();
    SourceMode GetSourceMode();
    void SetNotifyMode(NotifyMode trigger);
    NotifyMode GetNotifyMode();
    void JsOnDataPrepared(napi_env env, napi_value exports, napi_value extraInfo);
    void JsOnDataPrepared(napi_env env, napi_value pictures, napi_value exports, napi_value extraInfo);

    CompatibleMode GetCompatibleMode();
    void SetCompatibleMode(const CompatibleMode &compatibleMode);
    std::string GetRequestId();
    void SetRequestId(std::string requestId);
    napi_ref GetProgressHandlerRef();
    void SetProgressHandlerRef(napi_ref &progressHandlerRef);
    bool GetCinematicResult();
    void SetCinematicResult(bool cinematicResult);
    napi_threadsafe_function GetThreadsafeFunction()
    {
        return threadsafeFunction_;
    }
    void SetThreadsafeFunction(napi_threadsafe_function &threadsafeFunction)
    {
        threadsafeFunction_ = threadsafeFunction;
    }

private:
    napi_env env_ = nullptr;
    napi_ref dataHandlerRef_ = nullptr;
    ReturnDataType dataType_;
    std::string requestUri_;
    std::string destUri_;
    SourceMode sourceMode_;
    NotifyMode notifyMode_ = NotifyMode::FAST_NOTIFY;
    CompatibleMode compatibleMode_ {0};
    napi_ref progressHandlerRef_ = nullptr;
    napi_threadsafe_function threadsafeFunction_ = nullptr;
    std::string requestId_;
    static std::mutex dataHandlerRefMutex_;
    bool cinematicResult_ = false; // for Cinematic Video
};
} // Media
} // OHOS
#endif // INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_MEDIA_ASSET_DATA_HANDLER_NAPI_H
