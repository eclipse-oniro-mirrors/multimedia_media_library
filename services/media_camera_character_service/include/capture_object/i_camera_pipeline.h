/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_I_CAMERA_PIPELINE_H
#define OHOS_MEDIA_I_CAMERA_PIPELINE_H

#include "camera_asset_info.h"
#include "camera_character_types.h"
#include "camera_mapper.h"
#include "scan_config.h"

namespace OHOS::Media {

class EXPORT ICameraPipeline {
public:
    EXPORT ICameraPipeline() = default;
    EXPORT virtual ~ICameraPipeline() = default;

    EXPORT void Init(const CameraAssetInfo& assetInfo);
    EXPORT CameraBasePipelineType GetBasePipelineType() const;
    EXPORT CameraPipelineType GetPipelineType() const;
    EXPORT CameraAssetInfo GetAssetInfo() const;

    EXPORT int32_t GetLocalEnhancementAbility() const;
    EXPORT void SetLocalEnhancementAbility(int32_t localEnhancementAbility);

protected:
    EXPORT void SetBasePipelineType(CameraBasePipelineType basePipelineType);
    EXPORT void SetPipelineType(const CameraPipelineType& pipelineType);

    EXPORT MediaDpsMetadata GetMediaDpsMetadata() const;
    EXPORT void SetMediaDpsMetadata(const MediaDpsMetadata& metadata);

    EXPORT bool IsValid() const;

    // 二阶段、三阶段：异步扫描
    virtual void AsyncScanFileForCamera(ScanQuality scanQuality) = 0;

protected:
    CameraAssetInfo assetInfo_;
    MediaDpsMetadata metadata_;

private:
    CameraBasePipelineType basePipelineType_{CameraBasePipelineType::UNDEFINED};
    CameraPipelineType pipelineType_{CameraPipelineType::UNDEFINED};
};

} // namespace OHOS::Media
#endif // OHOS_MEDIA_I_CAMERA_PIPELINE_H