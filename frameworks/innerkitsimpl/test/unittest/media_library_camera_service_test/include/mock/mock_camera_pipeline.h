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

#ifndef OHOS_MEDIA_MOCK_CAMERA_PIPELINE_H
#define OHOS_MEDIA_MOCK_CAMERA_PIPELINE_H

#include <gtest/gtest.h>

#include "camera_asset_pipeline.h"

namespace OHOS {
namespace Media {
class MockCameraPipeline : public CameraAssetPipeline {
public:
    MockCameraPipeline() {}
    virtual ~MockCameraPipeline() = default;

    int32_t OnProcessImageDone(const OnProcessImageWrapper &wrapper) override
    {
        return DoOnProcessImageDone_ ? E_OK : E_ERR;
    }

    // 保存水印
    void SaveEditDataCamera(MediaLibraryCommand &cmd, const std::string& editData,
        const std::string& bundleName) override
    {
    }

    // 一阶段上报
    bool CloseCameraFileFdWithMutex(const std::string& realPath, const std::string& tempPath,
        const CameraPathType& pathType) override
    {
        return true;
    }

    virtual void OnDelivery(std::shared_ptr<Media::Picture> picture) override
    {
    }

    // 一阶段落盘
    bool UpdateExtValuesForStageInternal(const SaveCameraPhotoDto &dto, NativeRdb::ValuesBucket &values,
        CameraAssetInfo& modifyAssetInfo) override
    {
        return false;
    }

    void SaveImageForStageInternal(const SaveCameraPhotoDto& dto) override
    {
    }

    void ScanFileForStageInternal() override
    {
    }

    // 二阶段落盘
    bool InitForOnProcessInternal(const OnProcessImageWrapper &wrapper) override
    {
        return true;
    }

    bool CheckCanSaveDirectlyInternal(const std::shared_ptr<FileAsset> &fileAsset) override
    {
        return true;
    }

    int32_t ProcessMultistagesPhotoInternal(const std::shared_ptr<FileAsset> &fileAsset) override
    {
        return 0;
    }

    void ScanFileForOnProcessInternal() override
    {
    }

    // 缩略图业务

    // 打点相关

public:
    bool DoOnProcessImageDone_{false};
};
} // namespace Media
} // namespace OHOS
#endif  // OHOS_MEDIA_MOCK_CAMERA_PIPELINE_H