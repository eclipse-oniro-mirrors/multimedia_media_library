/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_MEDIALIBRARY_BG_TASK_MANAGER_H
#define OHOS_MEDIA_MEDIALIBRARY_BG_TASK_MANAGER_H

#include "medialibrary_base_bg_processor.h"

#include <mutex>
#include <string>

namespace OHOS {
namespace Media {
class MediaLibraryBgTaskManager {
public:
    using BgTaskFunc =
        int32_t (MediaLibraryBgTaskManager::*)(const std::string &taskName, const std::string &taskExtra);

    static MediaLibraryBgTaskManager& GetInstance();
    int32_t CommitTaskOps(const std::string &operation, const std::string &taskName,
        const std::string &taskExtra);
    int32_t Start(const std::string &taskName, const std::string &taskExtra);
    int32_t Stop(const std::string &taskName, const std::string &taskExtra);

private:
    MediaLibraryBgTaskManager() {}
    ~MediaLibraryBgTaskManager() {}

    void OnRemoveTaskNameComplete(const std::string &taskName);

    static std::mutex mapMutex_;
    std::unordered_map<std::string, std::shared_ptr<MediaLibraryBaseBgProcessor>> processorMap_;
};
} // namespace Media
} // namespace OHOS
#endif  // OHOS_MEDIA_MEDIALIBRARY_BG_TASK_MANAGER_H
