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

#ifndef OHOS_MEDIA_CAMERA_ASSET_PIPELINE_H
#define OHOS_MEDIA_CAMERA_ASSET_PIPELINE_H

#include <mutex>
#include <stdint.h>
#include <string>

#include "asset_accurate_refresh.h"
#include "camera_asset_info.h"
#include "camera_character_types.h"
#include "camera_mapper.h"
#include "create_camera_file_fd_dto.h"
#include "file_asset.h"
#include "get_deferred_picture_info_vo.h"
#include "medialibrary_command.h"
#include "save_camera_photo_dto.h"
#include "scan_camera_file_dto.h"

namespace OHOS::Media {
class EXPORT CameraAssetPipeline {
public:
    EXPORT CameraAssetPipeline() {}
    EXPORT virtual ~CameraAssetPipeline() = default;

    // 水印(Image、Yuv)
    EXPORT virtual void SaveEditDataCamera(MediaLibraryCommand &cmd, const std::string& bundleName,
        const std::string& editData = "");
    // 初始化
    EXPORT void Init(const CameraAssetInfo& assetInfo);
    // 一阶段上报
    EXPORT int32_t CreateCameraFileFd(const CreateCameraFileFdDto &dto);
    EXPORT int32_t CloseCameraFileFd(const ScanCameraFileDto &dto);
    EXPORT virtual void OnDelivery(std::shared_ptr<Media::Picture> picture) = 0;
    // 一阶段落盘
    EXPORT int32_t SaveCameraPhoto(const SaveCameraPhotoDto &dto);
    // 二阶段落盘
    EXPORT virtual int32_t OnProcessImageDone(const OnProcessImageWrapper &wrapper);
    // 二阶段失败
    EXPORT bool OnErrorImage(const MediaDpsErrorCode error, bool& isMovingPhoto);

    // 对外仅允许返回状态
    EXPORT bool IsValid();
    EXPORT const CameraAssetInfo GetAssetInfo() const;
    EXPORT CameraPipelineType GetPipelineType() const;
    EXPORT void SetActiveType(const CameraInfoActiveType& activeType);
    EXPORT bool IsLifeFinished();
    EXPORT void SaveCameraPhotoFinished();
    EXPORT void OnProcessFinished();
    EXPORT int32_t GetCompressionQuality();

    // 相机定制化inner接口
    EXPORT void GetDeferredPictureInfo(GetDeferredPictureInfoRespBody& respbody);

protected:
    MediaDpsMetadata GetMediaDpsMetadata() const;
    void SetMediaDpsMetadata(const MediaDpsMetadata& metadata);
    void SetPipelineType(const CameraPipelineType& pipelineType);
    void SetTakeEffectStatus(const TakeEffectStatus& takeEffectStatus);
    void SetEffectiveFileSaved(bool effectiveFileSaved);
    void SetSourceFileSaved(bool sourceFileSaved);

    std::string CreateUri();

    // 一阶段上报
    virtual bool CloseCameraFileFdWithMutex(const std::string& realPath, const std::string& tempPath,
        const CameraPathType& pathType) = 0;

    // 一阶段流程
    virtual bool UpdateExtValuesForStageInternal(const SaveCameraPhotoDto &dto, NativeRdb::ValuesBucket &values,
        CameraAssetInfo& modifyAssetInfo) = 0;
    virtual void SaveImageForStageInternal(const SaveCameraPhotoDto& dto) = 0;
    virtual void ScanFileForStageInternal() = 0;
    virtual int32_t CheckSaveImageForYuv();

    // 二阶段落盘
    virtual bool InitForOnProcessInternal(const OnProcessImageWrapper &wrapper) = 0;
    virtual bool CheckCanSaveDirectlyInternal(const std::shared_ptr<FileAsset> &fileAsset) = 0;
    virtual int32_t ProcessMultistagesPhotoInternal(const std::shared_ptr<FileAsset> &fileAsset) = 0;
    virtual void ScanFileForOnProcessInternal() = 0;

    // 通知
    void NotifyImageIfTempFile(bool isError);

private:
    // 初始化
    void InitCachePath();

    // 一阶段流程
    void RecheckEffectStatus();
    int32_t HandleSaveCameraPhoto(const SaveCameraPhotoDto &dto);
    int32_t UpdateIsTempAndDirty(const SaveCameraPhotoDto &dto);
    int32_t DoAccurateRefresh(const SaveCameraPhotoDto &dto, AccurateRefresh::AssetAccurateRefresh &assetRefresh,
        NativeRdb::ValuesBucket &values, NativeRdb::RdbPredicates &predicates);

    // 二阶段落盘
    void ProcessMultistagesPhoto(const std::shared_ptr<FileAsset> &fileAsset);
    // 二阶段失败
    void HandleIrrecoverableErrImage(const CameraAssetInfo& assetInfo);
    void HandleHighTemperatureImage(const CameraAssetInfo& assetInfo);
    void HandleRecoverableErrImage(const CameraAssetInfo& assetInfo);

private:
    std::mutex dbMutex_;
    // 不允许添加其他 info 数据
    CameraAssetInfo assetInfo_;
    CameraPipelineType pipelineType_{CameraPipelineType::UNDEFINED};
    // 二阶段入参
    MediaDpsMetadata metadata_;
};
} // namespace OHOS::Media
#endif // OHOS_MEDIA_CAMERA_ASSET_PIPELINE_H