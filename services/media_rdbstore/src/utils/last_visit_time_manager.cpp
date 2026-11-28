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
 
#define MLOG_TAG "LastVisitTimeManager"
 
#include "last_visit_time_manager.h"
 
#include <pthread.h>
 
#include <algorithm>
 
#include "media_column.h"
#include "media_log.h"
#include "media_time_utils.h"
#include "medialibrary_restore.h"
#include "medialibrary_tracer.h"
#include "medialibrary_unistore_manager.h"
#include "rdb_predicates.h"
#include "values_bucket.h"
 
using namespace OHOS::NativeRdb;
 
namespace OHOS {
namespace Media {
namespace {
constexpr int FLUSH_THREAD_NAME_MAX_LEN = 15;
}
 
LastVisitTimeManager &LastVisitTimeManager::GetInstance()
{
    static LastVisitTimeManager instance;
    return instance;
}
 
LastVisitTimeManager::~LastVisitTimeManager()
{
    Stop();
    Flush();
}
 
void LastVisitTimeManager::MarkVisited(const std::string &id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    EnsureStarted();
    if (pending_.size() >= MAX_PENDING_SIZE && pending_.find(id) == pending_.end()) {
        MEDIA_ERR_LOG("pending ids reach limit %{public}zu, drop id", MAX_PENDING_SIZE);
        return;
    }
    pending_.insert(id);
    cv_.notify_one();
}
 
void LastVisitTimeManager::EnsureStarted()
{
    // caller holds mutex_
    if (running_) {
        return;
    }
    running_ = true;
    worker_ = std::thread(&LastVisitTimeManager::FlusherLoop, this);
}
 
void LastVisitTimeManager::FlusherLoop()
{
    std::string name("MedLastVisTime");
    name.resize(FLUSH_THREAD_NAME_MAX_LEN);
    pthread_setname_np(pthread_self(), name.c_str());
    while (true) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !pending_.empty() || !running_; });
        if (!running_ && pending_.empty()) {
            return;
        }
        // debounce window: batch more ids into a single flush
        cv_.wait_for(lock, std::chrono::milliseconds(FLUSH_DEBOUNCE_MS), [this] { return !running_; });
        if (!running_ && pending_.empty()) {
            return;
        }
        std::vector<std::string> ids(pending_.begin(), pending_.end());
        pending_.clear();
        lock.unlock();
        FlushPendingIds(ids);
    }
}
 
void LastVisitTimeManager::Flush()
{
    std::vector<std::string> ids;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ids.assign(pending_.begin(), pending_.end());
        pending_.clear();
    }
    if (!ids.empty()) {
        FlushPendingIds(ids);
    }
}
 
void LastVisitTimeManager::Stop()
{
    std::thread worker;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_ && !worker_.joinable()) {
            return;
        }
        running_ = false;
        cv_.notify_all();
        worker = std::move(worker_);
    }
    if (worker.joinable()) {
        worker.join();
    }
}
 
void LastVisitTimeManager::FlushPendingIds(const std::vector<std::string> &ids)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdbStore is nullptr, drop %{public}zu pending ids", ids.size());
        return;
    }
    MediaLibraryTracer tracer;
    tracer.Start("FlushPendingIds");
    int64_t now = MediaTimeUtils::UTCTimeMilliSeconds();
    for (size_t begin = 0; begin < ids.size(); begin += MAX_BATCH_SIZE) {
        size_t end = std::min(begin + MAX_BATCH_SIZE, ids.size());
        std::vector<std::string> chunk(ids.begin() + begin, ids.begin() + end);
        ValuesBucket values;
        values.Put(PhotoColumn::PHOTO_LAST_VISIT_TIME, now);
        RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
        predicates.In(MediaColumn::MEDIA_ID, chunk);
        int32_t changedRows = 0;
        int32_t ret = rdbStore->Update(changedRows, values, predicates);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("batch update lastVisitTime failed, ret = %{public}d, chunk = %{public}zu",
                ret, chunk.size());
            MediaLibraryRestore::GetInstance().CheckRestore(ret);
        } else if (changedRows < static_cast<int32_t>(chunk.size())) {
            MEDIA_WARN_LOG("batch update lastVisitTime changedRows = %{public}d, chunk = %{public}zu",
                changedRows, chunk.size());
        }
    }
}
} // namespace Media
} // namespace OHOS
