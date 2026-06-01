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

#ifndef OHOS_MEDIA_MULTISTAGES_CAMERA_CAPTURE_MANAGER_H
#define OHOS_MEDIA_MULTISTAGES_CAMERA_CAPTURE_MANAGER_H

#include <mutex>
#include <string>
#include <map>

#include "camera_asset_pipeline.h"

namespace OHOS::Media {
class MultistagesCameraCaptureManager {
public:
    EXPORT static MultistagesCameraCaptureManager& GetInstance();

    // 该接口不会失败, 仅返回当前内存中的pipeline的个数
    EXPORT size_t InsertCaptureData(MediaLibraryCommand &cmd, const FileAsset& fileAsset,
        const std::string& editData = "");
    EXPORT size_t InsertCaptureData(const int32_t &fileId, const std::string &photoId,
        const std::shared_ptr<CameraAssetPipeline> &pipeline);

    // 异常场景恢复数据
    EXPORT size_t RecoverForSessionSync(const FileAsset& fileAsset, bool recoverForOnError);

    // 获取pipeline
    EXPORT std::shared_ptr<CameraAssetPipeline> GetPipelineByFileId(int32_t fileId, CameraPipelineType &type);
    EXPORT std::shared_ptr<CameraAssetPipeline> GetPipelineByPhotoId(
        const std::string &photoId, CameraPipelineType &type);
    EXPORT std::shared_ptr<CameraAssetPipeline> GetPipelineByFileIdWithExpected(
        int32_t fileId, const CameraPipelineType& expectedType);
    EXPORT std::shared_ptr<CameraAssetPipeline> GetPipelineByPhotoIdWithExpected(
        const std::string& photoId, const CameraPipelineType& expectedType);

    // 清理pipeline
    EXPORT size_t DeletePipelineWithFileId(const int32_t fileId, bool isDiscard);
    EXPORT size_t DeletePipelineWithPhotoId(const std::string& photoId, bool isDiscard);

    // 设置saveCameraPhoto的清理数据
    EXPORT void SetLastSavePhotoId(const std::string& photoId);
    EXPORT std::string GetLastSavePhotoId();

private:
    MultistagesCameraCaptureManager() {}
    ~MultistagesCameraCaptureManager() {}
    MultistagesCameraCaptureManager(const MultistagesCameraCaptureManager&) = delete;
    MultistagesCameraCaptureManager& operator=(const MultistagesCameraCaptureManager&) = delete;

    // 异常场景: 基于base重新创建pipeline
    std::shared_ptr<CameraAssetPipeline> ImprovedPipeline(
        const std::shared_ptr<CameraAssetPipeline>& pipelineInput, const CameraPipelineType& expectedType);
    std::shared_ptr<CameraAssetPipeline> GetPipelineByFileIdInternal(int32_t fileId,
        CameraPipelineType& type);
    std::shared_ptr<CameraAssetPipeline> GetPipelineByPhotoIdInternal(const std::string &photoId,
        CameraPipelineType& type);

private:
    std::mutex mapMutex_;

    // <fileId, photoId>
    std::map<int32_t, std::string> fileId2PhotoId_;

    // <photoId, std::shared_ptr<CameraAssetPipeline>(包含photoId)>
    std::map<std::string, std::shared_ptr<CameraAssetPipeline>> pipeLinesMap_;

    // 一阶段拖尾
    std::string lastSavePhotoId_;
};
} // namespace OHOS::Media
#endif // OHOS_MEDIA_MULTISTAGES_CAMERA_CAPTURE_MANAGER_H