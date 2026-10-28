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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_ASSET_MANAGER_ADAPTER_H
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_ASSET_MANAGER_ADAPTER_H

#include <cstdint>

#include "datashare_helper.h"
#include "media_asset_manager_adapter_const.h"
#include "query_local_enhancement_status_vo.h"

namespace OHOS {
namespace Media {
struct IpcCallContext {
    int32_t fileId{0};
    DeliveryMode mode = DeliveryMode::FAST;
    std::string photoUri;
    bool hasReadPermission{false};
    bool needsExtraInfo{true};

    std::shared_ptr<DataShare::DataShareHelper> datashareHelper = nullptr;
    int32_t userId{-1};
};

class MediaAssetManagerAdapter {
public:
    MediaAssetManagerAdapter() = default;
    ~MediaAssetManagerAdapter() = default;

    static MultiStagesCapturePhotoStatus QueryPhotoStatusWithDfx(const IpcCallContext& param, std::string& photoId);

    // 端侧AI增强
    static int32_t QueryLocalEnhancementStatus(const IpcCallContext& param,
        const QueryLocalEnhancementStatusReqBody &reqBody, std::string& photoId);
};
} // Media
} // OHOS
#endif // INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_ASSET_MANAGER_ADAPTER_H