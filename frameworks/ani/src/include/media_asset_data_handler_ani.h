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

#ifndef FRAMEWORKS_ANI_SRC_INCLUDE_MEDIA_ASSETS_DATA_HANDLER_ANI_H
#define FRAMEWORKS_ANI_SRC_INCLUDE_MEDIA_ASSETS_DATA_HANDLER_ANI_H

#include <ani.h>
#include <string>
#include <mutex>
#include "progress_handler.h"

namespace OHOS {
namespace Media {
enum class ReturnDataType {
    TYPE_IMAGE_SOURCE = 0,
    TYPE_ARRAY_BUFFER,
    TYPE_MOVING_PHOTO,
    TYPE_TARGET_PATH,
    TYPE_PICTURE,
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

enum class CompatibleMode {
    ORIGINAL_FORMAT_MODE = 0,
    COMPATIBLE_FORMAT_MODE = 1,
};

constexpr const char* ON_DATA_PREPARED_FUNC = "onDataPrepared";
constexpr const char* ON_PROGRESS_FUNC = "onProgress";
constexpr const char* ON_MEDIA_ASSET_DATA_PREPARED_FUNC = "onMediaAssetDataPrepared";
constexpr const char* ON_QUICK_IMAGE_DATA_PREPARED_FUNC = "onQuickImageDataPrepared";
constexpr const char* ON_MEDIA_ASSET_PROGRESS_FUNC = "onMediaAssetProgress";

class AniMediaAssetDataHandler {
public:
    AniMediaAssetDataHandler(ani_env *env, ani_ref dataHandler, ReturnDataType dataType, const std::string &uri,
        const std::string &destUri, SourceMode sourceMode);
    ~AniMediaAssetDataHandler();
    void DeleteAniReference(ani_env *env);
    ReturnDataType GetReturnDataType();
    std::string GetRequestUri();
    std::string GetDestUri();
    SourceMode GetSourceMode();
    bool IsCompositeAuxiliary();
    void SetCompositeAuxiliary(bool isCompositeAuxiliary);
    int GetCompositeFd();
    void SetCompositeFd(int compositeFd);
    void SetNotifyMode(NotifyMode trigger);
    NotifyMode GetNotifyMode();
    void EtsOnDataPrepared(ani_env *env, ani_object exports, ani_object extraInfo);
    void EtsOnDataPrepared(ani_env *env, ani_object pictures, ani_object exports, ani_object extraInfo);

    CompatibleMode GetCompatibleMode();
    void SetCompatibleMode(const CompatibleMode &compatibleMode);
    std::string GetRequestId();
    void SetRequestId(std::string requestId);
    ani_ref GetProgressHandlerRef();
    void SetProgressHandlerRef(ani_ref &progressHandlerRef);
    ThreadFunctionOnProgress GetThreadsafeFunction();
    void SetThreadsafeFunction(ThreadFunctionOnProgress &threadsafeFunction);
private:
    ani_env *env_ = nullptr;
    ani_ref dataHandlerRef_ = nullptr;
    ReturnDataType dataType_;
    std::string requestUri_;
    std::string destUri_;
    SourceMode sourceMode_;
    NotifyMode notifyMode_ = NotifyMode::FAST_NOTIFY;
    CompatibleMode compatibleMode_;
    ani_ref progressHandlerRef_ = nullptr;
    ThreadFunctionOnProgress threadsafeFunction_ = nullptr;
    std::string requestId_;
    bool isCompositeAuxiliary_ = false;
    int compositeFd_ = -1;
    static std::mutex dataHandlerRefMutex_;
};
} // namespace Media
} // namespace OHOS
#endif // FRAMEWORKS_ANI_SRC_INCLUDE_MEDIA_ASSETS_DATA_HANDLER_ANI_H
