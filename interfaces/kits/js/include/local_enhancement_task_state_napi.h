/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_LOCAL_ENHANCEMENT_TASK_STATE_NAPI_H
#define INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_LOCAL_ENHANCEMENT_TASK_STATE_NAPI_H

#include "cloud_enhancement_napi.h"

namespace OHOS {
namespace Media {

class LocalEnhancementTaskStateNapi {
public:
    LocalEnhancementTaskStateNapi() = default;
    ~LocalEnhancementTaskStateNapi() = default;
    static napi_value NewLocalEnhancementTaskStateNapi(napi_env env, CloudEnhancementAsyncContext* context);
    EXPORT static napi_value Init(napi_env env, napi_value exports);

private:
    EXPORT static napi_value Constructor(napi_env env, napi_callback_info info);
    EXPORT static void Destructor(napi_env env, void* nativeObject, void* finalizeHint);
    EXPORT static napi_value JSGetProgress(napi_env env, napi_callback_info info);

    int32_t GetProgress() const;
    void SetProgress(int32_t progress);

    static thread_local napi_ref constructor_;
    static const int32_t UNDEFINED = -1;
    int32_t progress_ {UNDEFINED};
};

} // namespace Media
} // namespace OHOS

#endif // INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_LOCAL_ENHANCEMENT_TASK_STATE_NAPI_H