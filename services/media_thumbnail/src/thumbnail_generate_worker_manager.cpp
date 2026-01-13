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

#include "thumbnail_generate_worker_manager.h"

#include "medialibrary_errno.h"
#include "media_log.h"

namespace OHOS {
namespace Media {
// LCOV_EXCL_START
ThumbnailGenerateWorkerManager::~ThumbnailGenerateWorkerManager()
{
    thumbnailWorkerMap_.Clear();
}

ThumbnailWorkerPtr ThumbnailGenerateWorkerManager::GetThumbnailWorker(const ThumbnailTaskType &taskType)
{
    ThumbnailWorkerPtr ptr;
    if (thumbnailWorkerMap_.Find(taskType, ptr)) {
        return ptr;
    }

    int status = InitThumbnailWorker(taskType);
    if (status != E_OK) {
        MEDIA_ERR_LOG("get thumbnail worker failed, status: %{public}d", status);
        return nullptr;
    }
    thumbnailWorkerMap_.Find(taskType, ptr);
    return ptr;
}

int32_t ThumbnailGenerateWorkerManager::InitThumbnailWorker(const ThumbnailTaskType &taskType)
{
    std::lock_guard<std::mutex> lock(mutex_);
    ThumbnailWorkerPtr ptr;
    if (thumbnailWorkerMap_.Find(taskType, ptr)) {
        return E_OK;
    }

    ptr = std::make_shared<ThumbnailGenerateWorker>();
    int32_t status = ptr->Init(taskType);
    if (status != E_OK) {
        MEDIA_ERR_LOG("init thumbnail worker failed, status: %{public}d", status);
        return status;
    }
    thumbnailWorkerMap_.Insert(taskType, ptr);
    return E_OK;
}

void ThumbnailGenerateWorkerManager::ClearAllTask()
{
    if (thumbnailWorkerMap_.IsEmpty()) {
        MEDIA_INFO_LOG("thumbnail worker empty, no need to clear");
        return;
    }

    MEDIA_INFO_LOG("ClearAllTask in thumbnail thread pool");
    thumbnailWorkerMap_.Iterate([](ThumbnailTaskType taskType, ThumbnailWorkerPtr &ptr) {
        if (ptr != nullptr) {
            ptr->ReleaseTaskQueue(ThumbnailTaskPriority::HIGH);
            ptr->ReleaseTaskQueue(ThumbnailTaskPriority::MID);
            ptr->ReleaseTaskQueue(ThumbnailTaskPriority::LOW);
        }
    });
}

void ThumbnailGenerateWorkerManager::TryCloseThumbnailWorkerTimer()
{
    if (thumbnailWorkerMap_.IsEmpty()) {
        MEDIA_INFO_LOG("thumbnail worker empty, no need to close timer");
        return;
    }

    thumbnailWorkerMap_.Iterate([](ThumbnailTaskType taskType, ThumbnailWorkerPtr &ptr) {
        if (ptr != nullptr) {
            ptr->TryCloseTimer();
        }
    });
}
// LCOV_EXCL_STOP
} // namespace Media
} // namespace OHOS