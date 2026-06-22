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

#ifndef OHOS_MEDIA_BACKGROUND_MEDIA_CAMERA_CACHE_CLEAN_TASK_H
#define OHOS_MEDIA_BACKGROUND_MEDIA_CAMERA_CACHE_CLEAN_TASK_H

#include <cstdint>
#include <vector>
#include <string>
#include <mutex>
#include "i_media_background_task.h"

namespace OHOS::Media::Background {
#define EXPORT __attribute__ ((visibility ("default")))

class EXPORT MediaCameraCacheCleanTask : public IMediaBackGroundTask {
public:
    MediaCameraCacheCleanTask() = default;
    virtual ~MediaCameraCacheCleanTask() = default;
    bool Accept() override;
    void Execute() override;

private:
    void DelEnhanceFolderDirtyFile(const std::string &realPathToEnhanceDir, const std::string &fileName);
    void HandleCameraCacheClean();

    std::mutex taskRunningMutex_;
};
} // namespace OHOS::Media::Background
#endif // OHOS_MEDIA_BACKGROUND_MEDIA_CAMERA_CACHE_CLEAN_TASK_H
