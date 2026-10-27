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

#define MLOG_TAG "LocalEnhancementManager"

#include "local_enhancement_manager.h"

#include "media_log.h"
#include "multistages_capture_dao.h"
#include "new_image_pipeline.h"
#include "yuv_pipeline.h"

namespace OHOS::Media {

LocalEnhancementManager& LocalEnhancementManager::GetInstance()
{
    static LocalEnhancementManager instance;
    return instance;
}

static std::shared_ptr<LocalEnhancementPipeline> ConcretePipeline(const CameraPipelineType& pipelineType)
{
    MEDIA_INFO_LOG("ConcretePipeline, pipelineType: %{public}d", static_cast<int32_t>(pipelineType));

    switch (pipelineType) {
        case CameraPipelineType::NEW_IMAGE:
            return std::make_shared<NewImagePipeline>(CameraBasePipelineType::LOCAL_ENHANCEMENT_PIPELINE);
        case CameraPipelineType::YUV:
            return std::make_shared<YuvPipeline>(CameraBasePipelineType::LOCAL_ENHANCEMENT_PIPELINE);
        default:
            break;
    }

    MEDIA_ERR_LOG("Invalid pipelineType: %{public}d.", static_cast<int32_t>(pipelineType));
    return nullptr;
}

size_t LocalEnhancementManager::InsertLocalEnhancementData(CameraPipelineType type,
    const CameraAssetInfo &inputAssetInfo)
{
    bool cond = inputAssetInfo.GetFileId() > 0 && !inputAssetInfo.GetPhotoId().empty();
    CHECK_AND_RETURN_RET_LOG(cond, localEnhancementMap_.size(), "assetInfo is invalid.");

    auto pipeline = ConcretePipeline(type);
    if (pipeline == nullptr) {
        MEDIA_WARN_LOG("failed to ConcretePipeline.");
        return localEnhancementMap_.size();
    }

    pipeline->Init(inputAssetInfo);

    std::lock_guard<std::mutex> lock(mutex_);
    if (fileId2PhotoId_.find(inputAssetInfo.GetFileId()) != fileId2PhotoId_.end() ||
        localEnhancementMap_.find(inputAssetInfo.GetPhotoId()) != localEnhancementMap_.end()) {
        MEDIA_ERR_LOG("Duplicate data is not allowed in the camera process.");
        return localEnhancementMap_.size();
    }
    fileId2PhotoId_.emplace(std::make_pair(inputAssetInfo.GetFileId(), inputAssetInfo.GetPhotoId()));
    localEnhancementMap_.emplace(std::make_pair(inputAssetInfo.GetPhotoId(), pipeline));
    return localEnhancementMap_.size();
}

std::shared_ptr<LocalEnhancementPipeline> LocalEnhancementManager::GetLocalEnhancementPipeline(
    int32_t fileId)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (fileId2PhotoId_.find(fileId) == fileId2PhotoId_.end()) {
        MEDIA_ERR_LOG("fileId is not in fileId2PhotoId: %{public}d.", fileId);
        return nullptr;
    }

    std::string photoId = fileId2PhotoId_.at(fileId);
    if (localEnhancementMap_.find(photoId) == localEnhancementMap_.end()) {
        MEDIA_ERR_LOG("photoId is not in pipeLinesMap: %{public}s.", photoId.c_str());
        return nullptr;
    }

    auto pipeline = localEnhancementMap_.at(photoId);
    CHECK_AND_RETURN_RET_LOG(pipeline != nullptr, nullptr, "pipeline is nullptr.");
    return pipeline;
}

bool LocalEnhancementManager::IsPhotoInProcessEnhancementByPhotoId(const std::string &photoId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    return localEnhancementMap_.find(photoId) != localEnhancementMap_.end();
}

std::shared_ptr<LocalEnhancementPipeline> LocalEnhancementManager::GetLocalEnhancementPipeline(
    const std::string &photoId, CameraPipelineType& type)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = localEnhancementMap_.find(photoId);
    if (it == localEnhancementMap_.end()) {
        MEDIA_WARN_LOG("photoId is not int localEnhancementMap: %{public}s", photoId.c_str());
        type = CameraPipelineType::UNDEFINED;
        return nullptr;
    }
    return it->second;
}

std::shared_ptr<LocalEnhancementPipeline> LocalEnhancementManager::ImprovedLocalEnhancementPipeline(
    const std::shared_ptr<LocalEnhancementPipeline>& pipelineInput, const CameraPipelineType& expectedType)
{
    MEDIA_INFO_LOG("ImprovedPipeline begin type: %{public}d.", static_cast<int32_t>(expectedType));
    CHECK_AND_RETURN_RET_LOG(pipelineInput != nullptr, nullptr, "pipelineInput is nullptr.");

    // 仅 NEW_IMAGE 允许修正
    CameraPipelineType type = pipelineInput->GetPipelineType();
    if (type != CameraPipelineType::NEW_IMAGE) {
        MEDIA_WARN_LOG("other type no need improve.");
        return pipelineInput;
    }
    CHECK_AND_RETURN_RET_LOG(expectedType == CameraPipelineType::YUV, pipelineInput, "not supported.");

    auto pipelineOutput = ConcretePipeline(expectedType);
    if (pipelineOutput == nullptr) {
        MEDIA_WARN_LOG("failed to ConcretePipeline.");
        return pipelineInput;
    }
    auto assetInfo = pipelineInput->GetAssetInfo();
    pipelineOutput->Init(assetInfo);

    std::string photoId = assetInfo.GetPhotoId();
    // 重新替换原有的数据, 仅替换pipeline
    std::lock_guard<std::mutex> lock(mutex_);
    auto iter = localEnhancementMap_.find(photoId);
    if (iter != localEnhancementMap_.end()) {
        localEnhancementMap_.erase(iter);
    }
    localEnhancementMap_.emplace(std::make_pair(photoId, pipelineOutput));
    return pipelineOutput;
}

static bool CheckPipelineType(const CameraPipelineType& expectedType)
{
    return expectedType == CameraPipelineType::NEW_IMAGE ||
           expectedType == CameraPipelineType::YUV;
}

std::shared_ptr<LocalEnhancementPipeline> LocalEnhancementManager::GetLocalEnhancementPipelineWithExpected(
    const std::string &photoId, const CameraPipelineType& expectedType)
{
    CHECK_AND_RETURN_RET_LOG(CheckPipelineType(expectedType), nullptr, "expectedType not supported.");

    CameraPipelineType type = CameraPipelineType::UNDEFINED;
    auto pipeline = GetLocalEnhancementPipeline(photoId, type);
    if (type != expectedType) {
        // 重新修正 pipeline
        pipeline = ImprovedLocalEnhancementPipeline(pipeline, expectedType);
    }
    return pipeline;
}

size_t LocalEnhancementManager::DeleteLocalEnhancementPipeline(const std::string &photoId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_DEBUG_LOG("DeleteLocalEnhancementPipeline start, photoId: %{public}s.", photoId.c_str());

    auto photoIdIter = localEnhancementMap_.find(photoId);
    if (photoIdIter == localEnhancementMap_.end()) {
        MEDIA_WARN_LOG("photoId: %{public}s is not in localEnhancementMap_, no need Delete.", photoId.c_str());
        return localEnhancementMap_.size();
    }

    auto pipeline = localEnhancementMap_.at(photoId);
    if (pipeline == nullptr) {
        MEDIA_WARN_LOG("pipeline is nullptr, clear invalid data, photoId: %{public}s.", photoId.c_str());
        localEnhancementMap_.erase(photoIdIter);
        return localEnhancementMap_.size();
    }

    auto assetInfo = pipeline->GetAssetInfo();
    auto fileIdIter = fileId2PhotoId_.find(assetInfo.GetFileId());
    if (fileIdIter == fileId2PhotoId_.end()) {
        MEDIA_WARN_LOG("fileId: %{public}d is not in fileId2PhotoId_, clear invalid data.", assetInfo.GetFileId());
        localEnhancementMap_.erase(photoIdIter);
        return localEnhancementMap_.size();
    }

    // 清理
    int32_t fileId = assetInfo.GetFileId();
    fileId2PhotoId_.erase(fileIdIter);
    localEnhancementMap_.erase(photoIdIter);
    MEDIA_INFO_LOG("DeleteLocalEnhancementPipeline success, photoId: %{public}s.", photoId.c_str());
    // progressMap_ 的清理交给 RemoveEnhancementProgress, 避免直接操作 progressMap_ 造成数据竞争
    RemoveEnhancementProgress(fileId);
    return localEnhancementMap_.size();
}

void LocalEnhancementManager::SetEnhancementProgress(int32_t fileId, int32_t progress)
{
    std::lock_guard<std::mutex> lock(progressMutex_);
    progressMap_[fileId] = progress;
    MEDIA_INFO_LOG("SetEnhancementProgress: fileId=%{public}d, progress=%{public}d", fileId, progress);
}

bool LocalEnhancementManager::GetEnhancementProgress(int32_t fileId, int32_t &progress)
{
    std::lock_guard<std::mutex> lock(progressMutex_);
    auto iter = progressMap_.find(fileId);
    if (iter == progressMap_.end()) {
        MEDIA_INFO_LOG("GetEnhancementProgress: fileId=%{public}d not found", fileId);
        return false;
    }
    progress = iter->second;
    MEDIA_INFO_LOG("GetEnhancementProgress: fileId=%{public}d, progress=%{public}d", fileId, progress);
    return true;
}

void LocalEnhancementManager::RemoveEnhancementProgress(int32_t fileId)
{
    std::lock_guard<std::mutex> lock(progressMutex_);
    auto iter = progressMap_.find(fileId);
    if (iter == progressMap_.end()) {
        MEDIA_INFO_LOG("RemoveEnhancementProgress: fileId=%{public}d not found", fileId);
        return;
    }
    progressMap_.erase(iter);
    MEDIA_INFO_LOG("RemoveEnhancementProgress: fileId=%{public}d removed", fileId);
}

void LocalEnhancementManager::RemoveEnhancementProgress(const std::string &photoId)
{
    int32_t fileId = -1;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto iter = localEnhancementMap_.find(photoId);
        if (iter == localEnhancementMap_.end() || iter->second == nullptr) {
            MEDIA_WARN_LOG("RemoveEnhancementProgress: photoId=%{public}s not found or pipeline is null",
                photoId.c_str());
            return;
        }
        fileId = iter->second->GetAssetInfo().GetFileId();
    }
    if (fileId < 0) {
        MEDIA_WARN_LOG("RemoveEnhancementProgress: invalid fileId=%{public}d, photoId=%{public}s",
            fileId, photoId.c_str());
        return;
    }
    RemoveEnhancementProgress(fileId);
}

} // namespace OHOS::Media
