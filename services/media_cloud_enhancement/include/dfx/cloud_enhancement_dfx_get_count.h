/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef FRAMEWORKS_INNERKITSIMPL_MEDIA_CLOUD_ENHANCEMENT_INCLUDE_CLOUD_ENHANCEMENT_DFX_GET_COUNT_H
#define FRAMEWORKS_INNERKITSIMPL_MEDIA_CLOUD_ENHANCEMENT_INCLUDE_CLOUD_ENHANCEMENT_DFX_GET_COUNT_H

#include <mutex>
#include <string>
#include <thread>

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

enum class CloudEnhancementTaskCompletedType : int32_t {
    SUCCESS,
    FAILED,
    EDIT_CANCELLATION,
    DELETE_CANCELLATION,
    SYNC_CANCELLATION,
};

class CloudEnhancementGetCount {
public:
    EXPORT static CloudEnhancementGetCount& GetInstance();
    EXPORT void AddStartTime(const std::string &photoId);
    EXPORT void RemoveStartTime(const std::string &photoId);
    EXPORT void Report(const std::string &completedType, const std::string &photoId,
        const int32_t finishType, const int32_t fileType);
    EXPORT std::unordered_map<std::string, int64_t> GetStartTimes();

private:
    CloudEnhancementGetCount();
    ~CloudEnhancementGetCount();

    std::unordered_map<std::string, int64_t> startTimes_;
};

}  // namespace Media
}  // namespace OHOS
#endif // FRAMEWORKS_INNERKITSIMPL_MEDIA_CLOUD_ENHANCEMENT_INCLUDE_CLOUD_ENHANCEMENT_DFX_GET_COUNT_H