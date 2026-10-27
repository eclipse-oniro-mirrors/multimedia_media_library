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

#define MLOG_TAG "ICameraPipeline"

#include "camera_path_utils.h"
#include "i_camera_pipeline.h"
#include "media_file_utils.h"
#include "media_log.h"

namespace OHOS {
namespace Media {

CameraPipelineType ICameraPipeline::GetPipelineType() const
{
    return pipelineType_;
}

void ICameraPipeline::SetPipelineType(const CameraPipelineType& pipelineType)
{
    pipelineType_ = pipelineType;
}

MediaDpsMetadata ICameraPipeline::GetMediaDpsMetadata() const
{
    return metadata_;
}

void ICameraPipeline::SetMediaDpsMetadata(const MediaDpsMetadata& metadata)
{
    metadata_ = metadata;
}

CameraAssetInfo ICameraPipeline::GetAssetInfo() const
{
    return assetInfo_;
}

void ICameraPipeline::SetBasePipelineType(CameraBasePipelineType basePipelineType)
{
    basePipelineType_ = basePipelineType;
}

CameraBasePipelineType ICameraPipeline::GetBasePipelineType() const
{
    return basePipelineType_;
}

static void InitCachePath(const CameraAssetInfo& assetInfo)
{
    std::string path = assetInfo.GetPath();
    CHECK_AND_RETURN_LOG(!path.empty(), "Failed to get asset path");

    std::string cacheDirPath = CameraPathUtils::GetCacheDir(path);
    CHECK_AND_RETURN_LOG(!cacheDirPath.empty(), "Can not get cache dir path");
    CHECK_AND_RETURN_LOG(MediaFileUtils::CreateDirectory(cacheDirPath),
        "Can not create dir %{private}s", cacheDirPath.c_str());
    MEDIA_INFO_LOG("InitCachePath success, fileId: %{public}d", assetInfo.GetFileId());
}

void ICameraPipeline::Init(const CameraAssetInfo& assetInfo)
{
    assetInfo_ = assetInfo;
    InitCachePath(assetInfo);
}

int32_t ICameraPipeline::GetLocalEnhancementAbility() const
{
    return assetInfo_.GetLocalEnhancementAbility();
}

void ICameraPipeline::SetLocalEnhancementAbility(int32_t localEnhancementAbility)
{
    assetInfo_.SetLocalEnhancementAbility(localEnhancementAbility);
}

bool ICameraPipeline::IsValid() const
{
    return assetInfo_.GetFileId() > 0;
}

} // namespace Media
} // namespace OHOS