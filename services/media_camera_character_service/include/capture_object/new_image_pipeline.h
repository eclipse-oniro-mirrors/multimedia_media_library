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

#ifndef OHOS_MEDIA_NEW_IMAGE_PIPELINE_H
#define OHOS_MEDIA_NEW_IMAGE_PIPELINE_H

#include <stdint.h>
#include <string>

#include "camera_asset_pipeline.h"

namespace OHOS::Media {
class NewImagePipeline : public CameraAssetPipeline {
public:
    EXPORT NewImagePipeline();
    virtual ~NewImagePipeline() = default;

private:
    // 保存水印
    virtual void SaveEditDataCamera(MediaLibraryCommand &cmd, const std::string& editData,
        const std::string& bundleName) override;

    // 一阶段上报
    virtual bool CloseCameraFileFdWithMutex(const std::string& realPath, const std::string& tempPath,
        const CameraPathType& pathType) override;
    virtual void OnDelivery(std::shared_ptr<Media::Picture> picture) override;

    // 一阶段落盘
    virtual bool UpdateExtValuesForStageInternal(const SaveCameraPhotoDto &dto, NativeRdb::ValuesBucket &values,
        CameraAssetInfo& modifyAssetInfo) override;
    virtual void SaveImageForStageInternal(const SaveCameraPhotoDto& dto) override;
    virtual void ScanFileForStageInternal() override;

    // 二阶段落盘
    virtual bool InitForOnProcessInternal(const OnProcessImageWrapper &wrapper) override;
    virtual bool CheckCanSaveDirectlyInternal(const std::shared_ptr<FileAsset> &fileAsset) override;
    virtual int32_t ProcessMultistagesPhotoInternal(const std::shared_ptr<FileAsset> &fileAsset) override;
    virtual void ScanFileForOnProcessInternal() override;

    int32_t ProcessSaveOneImage(const CameraAssetInfo& assetInfo,
        const std::shared_ptr<FileAsset> &fileAsset, const std::map<std::string, ImageFileMapper> &files);
    int32_t ProcessSaveTwoImage(const CameraAssetInfo& assetInfo,
    const std::shared_ptr<FileAsset> &fileAsset, const std::map<std::string, ImageFileMapper> &files);

    std::mutex fileMutex_;

    // 一阶段临时数据(一阶段结束时, 需要清理)
    std::shared_ptr<Media::Picture> resultPictureForFirstStage_;

    // 二阶段数据
    OnProcessParamForNewImage newImage_;
};
} // namespace OHOS::Media
#endif // OHOS_MEDIA_NEW_IMAGE_PIPELINE_H