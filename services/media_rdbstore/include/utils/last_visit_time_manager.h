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
 
#ifndef OHOS_MEDIA_LAST_VISIT_TIME_MANAGER_H
#define OHOS_MEDIA_LAST_VISIT_TIME_MANAGER_H
 
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>
 
namespace OHOS {
namespace Media {
/*
 * Batch writer for photo last visit time. Each file open only marks the id as
 * visited (in-memory, microseconds); a single background thread flushes all
 * pending ids with one parameterized UPDATE ... WHERE media_id IN (...) per
 * batch at a debounce interval. This avoids per-open UPDATE transactions
 * competing for the shared rdb connection and blocking read queries.
 */
class LastVisitTimeManager {
public:
    static LastVisitTimeManager &GetInstance();
 
    /* Mark one asset id as visited. Cheap, never blocks, safe from any thread. */
    void MarkVisited(const std::string &id);
 
    /* Flush pending ids synchronously. Also used by tests and on shutdown. */
    void Flush();
 
private:
    LastVisitTimeManager() = default;
    ~LastVisitTimeManager();
    LastVisitTimeManager(const LastVisitTimeManager &) = delete;
    LastVisitTimeManager &operator=(const LastVisitTimeManager &) = delete;
 
    void EnsureStarted();
    void Stop();
    void FlusherLoop();
    void FlushPendingIds(const std::vector<std::string> &ids);
 
    static constexpr int64_t FLUSH_DEBOUNCE_MS = 2000;  /* batching window */
    static constexpr size_t MAX_PENDING_SIZE = 5000;    /* drop new ids beyond cap */
    static constexpr size_t MAX_BATCH_SIZE = 500;       /* ids per UPDATE statement */
 
    std::mutex mutex_;
    std::condition_variable cv_;
    bool running_ = false;
    std::set<std::string> pending_;
    std::thread worker_;
};
} // namespace Media
} // namespace OHOS
 
#endif // OHOS_MEDIA_LAST_VISIT_TIME_MANAGER_H