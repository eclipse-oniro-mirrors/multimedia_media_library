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

#ifndef OHOS_MEDIA_LOCAL_ENHANCEMENT_MANAGER_H
#define OHOS_MEDIA_LOCAL_ENHANCEMENT_MANAGER_H

#include <map>
#include <mutex>
#include <string>

#include "local_enhancement_pipeline.h"

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))

class LocalEnhancementManager {
public:
    EXPORT static LocalEnhancementManager& GetInstance();

    // 该接口不会失败, 仅返回当前内存中的pipeline的个数
    EXPORT size_t InsertLocalEnhancementData(CameraPipelineType type, const CameraAssetInfo &inputAssetInfo);

    // 获取pipeline
    EXPORT std::shared_ptr<LocalEnhancementPipeline> GetLocalEnhancementPipeline(int32_t fileId);
    EXPORT std::shared_ptr<LocalEnhancementPipeline> GetLocalEnhancementPipeline(const std::string &photoId,
        CameraPipelineType& type);
    EXPORT std::shared_ptr<LocalEnhancementPipeline> GetLocalEnhancementPipelineWithExpected(const std::string &photoId,
        const CameraPipelineType& expectedType);
    EXPORT bool IsPhotoInProcessEnhancementByPhotoId(const std::string &photoId);

    // 清理pipeline
    EXPORT size_t DeleteLocalEnhancementPipeline(const std::string &photoId);

    // 进度管理
    EXPORT void SetEnhancementProgress(int32_t fileId, int32_t progress);
    EXPORT bool GetEnhancementProgress(int32_t fileId, int32_t &progress);
    EXPORT void RemoveEnhancementProgress(int32_t fileId);
    EXPORT void RemoveEnhancementProgress(const std::string &photoId);

private:
    LocalEnhancementManager() = default;
    ~LocalEnhancementManager() = default;
    LocalEnhancementManager(const LocalEnhancementManager&) = delete;
    const LocalEnhancementManager& operator=(const LocalEnhancementManager&) = delete;

    // 异常场景: 基于base重新创建pipeline
    std::shared_ptr<LocalEnhancementPipeline> ImprovedLocalEnhancementPipeline(
        const std::shared_ptr<LocalEnhancementPipeline>& pipelineInput, const CameraPipelineType& expectedType);

    std::mutex mutex_;
    // <fileId, photoId>
    std::map<int32_t, std::string> fileId2PhotoId_;
    // <photoId, std::shared_ptr<LocalEnhancementPipeline>(包含photoId)>
    std::map<std::string, std::shared_ptr<LocalEnhancementPipeline>> localEnhancementMap_;

    // 进度缓存
    std::mutex progressMutex_;
    // <fileId, progress(0-100)>
    std::map<int32_t, int32_t> progressMap_;
};

} // namespace OHOS::Media

#endif // OHOS_MEDIA_LOCAL_ENHANCEMENT_MANAGER_H
