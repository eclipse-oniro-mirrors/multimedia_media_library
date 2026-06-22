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

#ifndef OHOS_MEDIA_IMAGE_PIPELINE_H
#define OHOS_MEDIA_IMAGE_PIPELINE_H

#include <stdint.h>
#include <string>

#include "camera_asset_pipeline.h"

namespace OHOS::Media {
class ImagePipeline : public CameraAssetPipeline {
public:
    EXPORT ImagePipeline();
    virtual ~ImagePipeline() = default;

private:
    int32_t AddFiltersToPhoto(const std::string& sourcePath, const std::string& tempFilterPath,
        const std::string& editData);
    int32_t AddFiltersExecute(const std::string& filePath, bool executeForLowImage, std::string& tempSourcePath,
        std::string& tempFiltersPath);

    // 一阶段上报
    virtual bool CloseCameraFileFdWithMutex(const std::string& realPath, const std::string& tempPath,
        const CameraPathType& pathType) override;
    virtual void OnDelivery(std::shared_ptr<Media::Picture> picture) override;

    // 一阶段落盘
    virtual bool UpdateExtValuesForStageInternal(const SaveCameraPhotoDto &dto, NativeRdb::ValuesBucket &values,
        CameraAssetInfo& modifyAssetInfo) override;
    virtual void SaveImageForStageInternal(const SaveCameraPhotoDto& dto) override;
    virtual void ScanFileForStageInternal() override;

    void ExecuteImageWithoutMutex(const std::string& path, int32_t errcode, const std::string& tempSourcePath,
        const std::string& tempFiltersPath);
    void ExecuteLowImageWithMutex(const std::string& path, int32_t& errcode, std::string& tempSourcePath,
        std::string& tempFiltersPath);

    // 二阶段落盘
    virtual bool InitForOnProcessInternal(const OnProcessImageWrapper &wrapper) override;
    virtual bool CheckCanSaveDirectlyInternal(const std::shared_ptr<FileAsset> &fileAsset) override;
    virtual int32_t ProcessMultistagesPhotoInternal(const std::shared_ptr<FileAsset> &fileAsset) override;
    virtual void ScanFileForOnProcessInternal() override;

    int32_t SaveOneImageForOnProcess(const ImageFileMapper& sourceImage, const std::string& path);
    int32_t AddFiltersForRecheckHighImage(const std::string& path, const std::string& tempHighPath);
    int32_t SaveTwoImageForOnProcess(const ImageFileMapper& sourceImage, const std::string& path);

    std::mutex fileMutex_;

    // 二阶段数据
    OnProcessParamForImage image_;
};
} // namespace OHOS::Media
#endif // OHOS_MEDIA_IMAGE_PIPELINE_H