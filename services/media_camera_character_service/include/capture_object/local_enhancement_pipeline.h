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

#ifndef OHOS_MEDIA_LOCAL_ENHANCEMENT_PIPELINE_H
#define OHOS_MEDIA_LOCAL_ENHANCEMENT_PIPELINE_H

#include <map>
#include <string>

#include "camera_asset_info.h"
#include "camera_mapper.h"
#include "i_camera_pipeline.h"

namespace OHOS::Media {
class LocalEnhancementPipeline : public virtual ICameraPipeline {
public:
    EXPORT LocalEnhancementPipeline() {}
    EXPORT virtual ~LocalEnhancementPipeline() = default;

    EXPORT void SetLocalEnhancementType(const LocalEnhancementType& localEnhancementType);

    // 端侧AI增强落盘
    int32_t OnProcessImageEnhancementDone(const OnProcessImageWrapper &wrapper);

    bool OnEnhancementImageError(const MediaDpsErrorCode error);

protected:
    // 端侧AI增强落盘
    virtual bool InitForOnProcessEnhancementInternal(const OnProcessImageWrapper &wrapper) = 0;
    virtual int32_t ProcessEnhancementPhotoInternal(const std::shared_ptr<FileAsset> &fileAsset, bool isManual) = 0;

    static bool GetEnhancementExifValue(LocalEnhancementType type, std::map<std::string, std::string> &values);

private:
    void ProcessEnhancementPhoto(const std::shared_ptr<FileAsset> &fileAsset);

    // 三阶段失败
    void OnEnhancementIrrecoverableErrImage(const CameraAssetInfo& assetInfo, const MediaDpsErrorCode error);
    void OnEnhancementRecoverableErrImage(const CameraAssetInfo& assetInfo, const MediaDpsErrorCode error);
};

} // namespace OHOS::Media

#endif // OHOS_MEDIA_LOCAL_ENHANCEMENT_PIPELINE_H