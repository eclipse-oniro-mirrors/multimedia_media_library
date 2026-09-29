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

#ifndef SERVICES_MEDIA_THUMBNAIL_DOMAIN_BACKGROUND_GENERATE_MANAGER_H_
#define SERVICES_MEDIA_THUMBNAIL_DOMAIN_BACKGROUND_GENERATE_MANAGER_H_

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace OHOS {
namespace Media {
class MediaLibraryRdbStore;

// Background thumbnail generation orchestrator.
// Not a singleton: owned and initialized by ThumbnailService.
// On Start() spawns a non-resident producer thread that scans DB for missing
// basic thumbnails (one pass) and feeds them to the BACKGROUND worker pool 2 at
// a time at MID priority, waiting on a CV for each pair to finish (or for Stop).
// On interrupt (Stop) the producer stops feeding new pairs; in-flight MID tasks
// are left in the pool to finish naturally (MID is not cleared by InterruptBgworker).
class BackgroundGenerateManager {
public:
    BackgroundGenerateManager();
    ~BackgroundGenerateManager();

    void Init(std::shared_ptr<MediaLibraryRdbStore> rdbStore);

    // Idempotent. Returns E_OK. Spawns the producer thread if not already running.
    int32_t Start();

    // Idempotent. Signals the producer to stop feeding and joins it.
    // In-flight pool tasks continue; only the producer thread is stopped.
    void Stop();

private:
    void ProducerRun();

    std::shared_ptr<MediaLibraryRdbStore> rdbStore_;
    std::atomic<bool> running_{false};
    std::thread producerThread_;
    std::mutex startMtx_;
    // Shared with wrapped pool tasks so they can notify on completion even after
    // the manager/producer is gone; held by shared_ptr to avoid UAF.
    std::shared_ptr<std::mutex> waitMtx_ = std::make_shared<std::mutex>();
    std::shared_ptr<std::condition_variable> waitCv_ = std::make_shared<std::condition_variable>();
};
} // namespace Media
} // namespace OHOS

#endif  // SERVICES_MEDIA_THUMBNAIL_DOMAIN_BACKGROUND_GENERATE_MANAGER_H_
