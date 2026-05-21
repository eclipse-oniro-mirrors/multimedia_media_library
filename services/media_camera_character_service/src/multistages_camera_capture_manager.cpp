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

#define MLOG_TAG "MultistagesCameraCaptureManager"

#include "multistages_camera_capture_manager.h"

#include <string>

#include "camera_character_types.h"
#include "camera_path_utils.h"
#include "image_pipeline.h"
#include "media_string_utils.h"
#include "multistages_capture_dao.h"
#include "new_image_pipeline.h"
#include "yuv_pipeline.h"

namespace OHOS {
namespace Media {
MultistagesCameraCaptureManager& MultistagesCameraCaptureManager::GetInstance()
{
    static MultistagesCameraCaptureManager instance;
    return instance;
}

static std::shared_ptr<CameraAssetPipeline> ConcretePipeline(const CameraPipelineType& pipelineType)
{
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} ConcretePipeline, pipelineType: %{public}d",
        MLOG_TAG, __FUNCTION__, __LINE__, static_cast<int32_t>(pipelineType));

    switch (pipelineType) {
        case CameraPipelineType::NEW_IMAGE:
            return std::make_shared<NewImagePipeline>();
        case CameraPipelineType::IMAGE:
            return std::make_shared<ImagePipeline>();
        case CameraPipelineType::YUV:
            return std::make_shared<YuvPipeline>();
        case CameraPipelineType::VIDEO:
            MEDIA_INFO_LOG("not support, pipelineType: %{public}d", static_cast<int32_t>(pipelineType));
            return nullptr;
        default:
            break;
    }

    MEDIA_ERR_LOG("Invalid pipelineType: %{public}d.", static_cast<int32_t>(pipelineType));
    return nullptr;
}

size_t MultistagesCameraCaptureManager::InsertCaptureData(MediaLibraryCommand &cmd, const FileAsset& fileAsset,
    const std::string& editData)
{
    std::string pipelineType = cmd.GetQuerySetParam(CAMERA_PIPELINE_TYPE);
    int32_t type = 0;
    if (!MediaStringUtils::ConvertToInt(pipelineType, type)) {
        MEDIA_ERR_LOG("input is not a number.");
        return pipeLinesMap_.size();
    }

    auto pipeline = ConcretePipeline(static_cast<CameraPipelineType>(type));
    if (pipeline == nullptr) {
        MEDIA_WARN_LOG("failed to ConcretePipeline.");
        return pipeLinesMap_.size();
    }

    CameraAssetInfo assetInfo(fileAsset);
    pipeline->Init(assetInfo);

    // 保存editdata
    if (type == static_cast<int32_t>(CameraPipelineType::NEW_IMAGE)) {
        pipeline->SaveEditDataCamera(cmd, fileAsset.GetOwnerPackage(), editData);
    }

    // 插入数据
    return InsertCaptureData(fileAsset.GetId(), fileAsset.GetPhotoId(), pipeline);
}

size_t MultistagesCameraCaptureManager::InsertCaptureData(const int32_t &fileId, const std::string &photoId,
    const std::shared_ptr<CameraAssetPipeline> &pipeline)
{
    std::lock_guard<std::mutex> lock(mapMutex_);
    CHECK_AND_RETURN_RET_LOG(fileId > 0, pipeLinesMap_.size(), "invalid fileId: %{public}d.", fileId);

    std::string photoIdInput = photoId;
    if (photoId.empty()) {
        MEDIA_WARN_LOG("photoId is empty, using fileId instead.");
        photoIdInput = std::to_string(fileId);
    }

    if (fileId2PhotoId_.find(fileId) != fileId2PhotoId_.end() || pipeLinesMap_.find(photoId) != pipeLinesMap_.end()) {
        MEDIA_ERR_LOG("Duplicate data is not allowed in the camera process.");
        return pipeLinesMap_.size();
    }
    fileId2PhotoId_.emplace(std::make_pair(fileId, photoIdInput));
    pipeLinesMap_.emplace(std::make_pair(photoIdInput, pipeline));
    return pipeLinesMap_.size();
}

size_t MultistagesCameraCaptureManager::RecoverForSessionSync(const FileAsset& fileAsset, bool recoverForOnError)
{
    MEDIA_INFO_LOG("RecoverForSessionSync begin.");
    auto pipeline = std::make_shared<NewImagePipeline>();
    CHECK_AND_RETURN_RET_LOG(pipeline != nullptr, pipeLinesMap_.size(), "failed to create pipeline.");

    CameraAssetInfo assetInfo(fileAsset);
    pipeline->Init(assetInfo);

    if (recoverForOnError) {
        pipeline->SetActiveType(CameraInfoActiveType::RecoverForSessionSync);
    }

    return InsertCaptureData(fileAsset.GetId(), fileAsset.GetPhotoId(), pipeline);
}

std::shared_ptr<CameraAssetPipeline> MultistagesCameraCaptureManager::ImprovedPipeline(
    const std::shared_ptr<CameraAssetPipeline>& pipelineInput, const CameraPipelineType& expectedType)
{
    MEDIA_INFO_LOG("ImprovedPipeline begin type: %{public}d.", static_cast<int32_t>(expectedType));
    CHECK_AND_RETURN_RET_LOG(pipelineInput != nullptr, nullptr, "pipelineInput is nullptr.");

    // 仅 NEW_IMAGE 允许修正
    CameraPipelineType type = pipelineInput->GetPipelineType();
    if (type != CameraPipelineType::NEW_IMAGE) {
        MEDIA_WARN_LOG("other type no need improve.");
        return pipelineInput;
    }
    bool cond = (expectedType == CameraPipelineType::IMAGE || expectedType == CameraPipelineType::YUV);
    CHECK_AND_RETURN_RET_LOG(cond, pipelineInput, "not supported.");

    auto pipelineOutput = ConcretePipeline(expectedType);
    if (pipelineOutput == nullptr) {
        MEDIA_WARN_LOG("failed to ConcretePipeline.");
        return pipelineInput;
    }
    auto assetInfo = pipelineInput->GetAssetInfo();
    pipelineOutput->Init(assetInfo);

    std::string photoId = assetInfo.GetPhotoId();
    // 重新替换原有的数据, 仅替换pipeline
    std::lock_guard<std::mutex> lock(mapMutex_);
    auto iter = pipeLinesMap_.find(photoId);
    if (iter != pipeLinesMap_.end()) {
        pipeLinesMap_.erase(iter);
    }
    pipeLinesMap_.emplace(std::make_pair(photoId, pipelineOutput));
    return pipelineOutput;
}

std::shared_ptr<CameraAssetPipeline> MultistagesCameraCaptureManager::GetPipelineByFileIdInternal(int32_t fileId,
    CameraPipelineType& type)
{
    std::lock_guard<std::mutex> lock(mapMutex_);

    if (fileId2PhotoId_.find(fileId) == fileId2PhotoId_.end()) {
        MEDIA_ERR_LOG("fileId is not in fileId2PhotoId: %{public}d.", fileId);
        type = CameraPipelineType::UNDEFINED;
        return nullptr;
    }

    std::string photoId = fileId2PhotoId_.at(fileId);
    if (pipeLinesMap_.find(photoId) == pipeLinesMap_.end()) {
        MEDIA_ERR_LOG("photoId is not in pipeLinesMap: %{public}s.", photoId.c_str());
        type = CameraPipelineType::UNDEFINED;
        return nullptr;
    }

    auto pipeline = pipeLinesMap_.at(photoId);
    CHECK_AND_RETURN_RET_LOG(pipeline != nullptr, nullptr, "pipeline is nullptr.");
    type = pipeline->GetPipelineType();
    return pipeline;
}

std::shared_ptr<CameraAssetPipeline> MultistagesCameraCaptureManager::GetPipelineByFileId(int32_t fileId,
    CameraPipelineType& type)
{
    auto pipeline = GetPipelineByFileIdInternal(fileId, type);
    if (pipeline != nullptr) {
        return pipeline;
    }

    MEDIA_WARN_LOG("Not found in the cache, trying to retrieve from the database.");
    auto fileAsset = MultiStagesCaptureDao::RecoverPipelineByFileId(fileId);
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, nullptr, "fileAsset is nullptr");

    RecoverForSessionSync(*fileAsset, false);
    return GetPipelineByFileIdInternal(fileId, type);
}

std::shared_ptr<CameraAssetPipeline> MultistagesCameraCaptureManager::GetPipelineByPhotoIdInternal(
    const std::string &photoId, CameraPipelineType& type)
{
    std::lock_guard<std::mutex> lock(mapMutex_);

    if (pipeLinesMap_.find(photoId) == pipeLinesMap_.end()) {
        MEDIA_ERR_LOG("photoId is not in pipeLinesMap: %{public}s.", photoId.c_str());
        type = CameraPipelineType::UNDEFINED;
        return nullptr;
    }

    auto pipeline = pipeLinesMap_.at(photoId);
    CHECK_AND_RETURN_RET_LOG(pipeline != nullptr, nullptr, "pipeline is nullptr.");
    type = pipeline->GetPipelineType();
    return pipeline;
}

std::shared_ptr<CameraAssetPipeline> MultistagesCameraCaptureManager::GetPipelineByPhotoId(const std::string &photoId,
    CameraPipelineType& type)
{
    auto pipeline = GetPipelineByPhotoIdInternal(photoId, type);
    if (pipeline != nullptr) {
        return pipeline;
    }

    MEDIA_WARN_LOG("Not found in the cache, trying to retrieve from the database.");
    auto fileAsset = MultiStagesCaptureDao::RecoverPipelineByPhotoId(photoId);
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, nullptr, "fileAsset is nullptr");

    RecoverForSessionSync(*fileAsset, false);
    return GetPipelineByPhotoIdInternal(photoId, type);
}

std::shared_ptr<CameraAssetPipeline> MultistagesCameraCaptureManager::GetPipelineByFileIdWithExpected(
    int32_t fileId, const CameraPipelineType& expectedType)
{
    CHECK_AND_RETURN_RET_LOG(expectedType != CameraPipelineType::UNDEFINED, nullptr, "expectedType not supported.");

    MEDIA_DEBUG_LOG("GetPipelineByFileIdWithExpected begin, fileId: %{public}d, type: %{public}d.",
        fileId, static_cast<int32_t>(expectedType));
    CameraPipelineType type = CameraPipelineType::UNDEFINED;
    auto pipeline = GetPipelineByFileId(fileId, type);
    if (type != expectedType) {
        // 重新修正 pipeline
        pipeline = ImprovedPipeline(pipeline, expectedType);
    }
    MEDIA_INFO_LOG("GetPipelineByFileIdWithExpected end, fileId: %{public}d.", fileId);
    return pipeline;
}

std::shared_ptr<CameraAssetPipeline> MultistagesCameraCaptureManager::GetPipelineByPhotoIdWithExpected(
    const std::string& photoId, const CameraPipelineType& expectedType)
{
    CHECK_AND_RETURN_RET_LOG(expectedType != CameraPipelineType::UNDEFINED, nullptr, "expectedType not supported.");

    CameraPipelineType type = CameraPipelineType::UNDEFINED;
    auto pipeline = GetPipelineByPhotoId(photoId, type);
    if (type != expectedType) {
        // 重新修正 pipeline
        pipeline = ImprovedPipeline(pipeline, expectedType);
    }
    return pipeline;
}

static bool CanDeletePipeline(const std::shared_ptr<CameraAssetPipeline>& pipeline)
{
    if (pipeline == nullptr) {
        MEDIA_WARN_LOG("pipeline is nullptr, delete by default.");
        return true;
    }

    return pipeline->IsLifeFinished();
}

size_t MultistagesCameraCaptureManager::DeletePipelineWithFileId(const int32_t fileId, bool isDiscard)
{
    std::lock_guard<std::mutex> lock(mapMutex_);
    MEDIA_DEBUG_LOG("DeletePipelineWithFileId start, fileId: %{public}d.", fileId);

    auto fileIdIter = fileId2PhotoId_.find(fileId);
    if (fileIdIter == fileId2PhotoId_.end()) {
        MEDIA_WARN_LOG("fileId: %{public}d is not in fileId2PhotoId_, no need Delete.", fileId);
        return pipeLinesMap_.size();
    }

    std::string photoId = fileId2PhotoId_.at(fileId);
    auto photoIdIter = pipeLinesMap_.find(photoId);
    if (photoIdIter == pipeLinesMap_.end()) {
        MEDIA_WARN_LOG("photoId: %{public}s is not in pipeLinesMap_, clear invalid data.", photoId.c_str());
        // 无效数据可以清理
        fileId2PhotoId_.erase(fileIdIter);
        return pipeLinesMap_.size();
    }
    auto pipeline = pipeLinesMap_.at(photoId);
    if (pipeline == nullptr) {
        MEDIA_WARN_LOG("pipeline is nullptr, clear invalid data, fileId: %{public}d.", fileId);
        // 无效数据可以清理
        fileId2PhotoId_.erase(fileIdIter);
        pipeLinesMap_.erase(photoIdIter);
        return pipeLinesMap_.size();
    }
    // 校验 ActiveType
    if (!CanDeletePipeline(pipeline) && !isDiscard) {
        MEDIA_WARN_LOG("The pipeline cannot be cleared yet.");
        return pipeLinesMap_.size();
    }

    // 清理
    fileId2PhotoId_.erase(fileIdIter);
    pipeLinesMap_.erase(photoIdIter);
    MEDIA_INFO_LOG("DeletePipelineWithFileId success, fileId: %{public}d.", fileId);
    return pipeLinesMap_.size();
}

size_t MultistagesCameraCaptureManager::DeletePipelineWithPhotoId(const std::string& photoId, bool isDiscard)
{
    std::lock_guard<std::mutex> lock(mapMutex_);
    MEDIA_DEBUG_LOG("DeletePipelineWithPhotoId start, photoId: %{public}s.", photoId.c_str());

    auto photoIdIter = pipeLinesMap_.find(photoId);
    if (photoIdIter == pipeLinesMap_.end()) {
        MEDIA_WARN_LOG("photoId: %{public}s is not in pipeLinesMap_, no need Delete.", photoId.c_str());
        return pipeLinesMap_.size();
    }

    auto pipeline = pipeLinesMap_.at(photoId);
    if (pipeline == nullptr) {
        MEDIA_WARN_LOG("pipeline is nullptr, clear invalid data, photoId: %{public}s.", photoId.c_str());
        pipeLinesMap_.erase(photoIdIter);
        return pipeLinesMap_.size();
    }

    auto assetInfo = pipeline->GetAssetInfo();
    auto fileIdIter = fileId2PhotoId_.find(assetInfo.GetFileId());
    if (fileIdIter == fileId2PhotoId_.end()) {
        MEDIA_WARN_LOG("fileId: %{public}d is not in fileId2PhotoId_, clear invalid data.", assetInfo.GetFileId());
        pipeLinesMap_.erase(photoIdIter);
        return pipeLinesMap_.size();
    }

    // 校验 ActiveType
    if (!CanDeletePipeline(pipeline) && !isDiscard) {
        MEDIA_WARN_LOG("The pipeline cannot be cleared yet.");
        return pipeLinesMap_.size();
    }

    // 清理
    fileId2PhotoId_.erase(fileIdIter);
    pipeLinesMap_.erase(photoIdIter);
    MEDIA_INFO_LOG("DeletePipelineWithPhotoId success, photoId: %{public}s.", photoId.c_str());
    return pipeLinesMap_.size();
}

void MultistagesCameraCaptureManager::SetLastSavePhotoId(const std::string& photoId)
{
    lastSavePhotoId_ = photoId;
}

std::string MultistagesCameraCaptureManager::GetLastSavePhotoId()
{
    return lastSavePhotoId_;
}
} // Media
} // OHOS