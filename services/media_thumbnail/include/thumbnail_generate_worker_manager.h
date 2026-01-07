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

#ifndef FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_GENERATE_WORKER_MANAGER_H
#define FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_GENERATE_WORKER_MANAGER_H

#include <safe_map.h>
#include <shared_mutex>

#include "thumbnail_generate_worker.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
using ThumbnailWorkerPtr = std::shared_ptr<ThumbnailGenerateWorker>;

class ThumbnailGenerateWorkerManager {
public:
    EXPORT static ThumbnailGenerateWorkerManager &GetInstance()
    {
        static ThumbnailGenerateWorkerManager instance;
        return instance;
    }

    EXPORT ThumbnailWorkerPtr GetThumbnailWorker(const ThumbnailTaskType &taskType);

    EXPORT void ClearAllTask();

    EXPORT void TryCloseThumbnailWorkerTimer();

private:
    ThumbnailGenerateWorkerManager() = default;
    EXPORT ~ThumbnailGenerateWorkerManager();

    EXPORT int32_t InitThumbnailWorker(const ThumbnailTaskType &taskType);

    SafeMap<ThumbnailTaskType, ThumbnailWorkerPtr> thumbnailWorkerMap_;

    std::mutex mutex_;
};
} // namespace Media
} // namespace OHOS

#endif //FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_GENERATE_WORKER_MANAGER_H