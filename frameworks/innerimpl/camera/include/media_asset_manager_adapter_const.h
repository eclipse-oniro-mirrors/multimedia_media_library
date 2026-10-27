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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_ASSET_MANAGER_ADAPTER_CONST_H
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_ASSET_MANAGER_ADAPTER_CONST_H

namespace OHOS {
namespace Media {
enum class DeliveryMode : int32_t {
    FAST = 0,
    HIGH_QUALITY,
    BALANCED_MODE,
};

enum class MultiStagesCapturePhotoStatus : int32_t {
    QUERY_INNER_FAIL = 0,
    HIGH_QUALITY_STATUS,
    LOW_QUALITY_STATUS,
};

enum class LocalEnhanceStatus : int32_t {
    NOT_SUPPORT_ENHANCE = 1,
    PROCESSING_ENHANCE,
    SUCCESS_ENHANCE,
};
} // Media
} // OHOS
#endif // INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_ASSET_MANAGER_ADAPTER_CONST_H