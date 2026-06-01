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

#ifndef OHOS_MEDIA_ENHANCE_MEDIA_ENHANCE_HANDLES_H
#define OHOS_MEDIA_ENHANCE_MEDIA_ENHANCE_HANDLES_H

#include <stdint.h>
#include "media_enhance_constants_c_api.h"

#ifdef __cplusplus
extern "C" {
#endif

namespace OHOS {
namespace MediaEnhance {
/**
 * 声明所提供的各实例的handle
 */
typedef struct MediaEnhanceClientHandle MediaEnhanceClientHandle;
typedef struct MediaEnhanceBundleHandle MediaEnhanceBundleHandle;

typedef void (*OnSuccess)(const char* taskId, MediaEnhanceBundleHandle* bundleWrapper);
typedef void (*OnFailed)(const char* taskId, MediaEnhanceBundleHandle* bundleWrapper);
typedef void (*OnGuard)(MediaEnhance_GUARD_TYPE guardType);
typedef void (*OnSAReconnected)();
typedef void (*OnSADestroy)();

typedef struct MediaEnhance_Callbacks {
    OnSuccess onSuccessFunc;
    OnFailed onFailedFunc;
    OnGuard onGuardFunc;
    OnSAReconnected onSAReconnectedFunc;
    OnSADestroy onSADestroyFunc;
} MediaEnhance_Callbacks;

typedef struct Raw_Data {
    uint8_t* buffer;
    uint32_t size;
} Raw_Data;

typedef struct Pending_Task {
    char* taskId;
} Pending_Task;
}  // end of namespace MediaEnhance
}  // end of namespace OHOS

#ifdef __cplusplus
}
#endif
#endif  //OHOS_MEDIA_ENHANCE_MEDIA_ENHANCE_HANDLES_H