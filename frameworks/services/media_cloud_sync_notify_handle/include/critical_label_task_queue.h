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

#ifndef CRITICAL_LABEL_TASK_QUEUE_H
#define CRITICAL_LABEL_TASK_QUEUE_H

#ifdef MEDIALIBRARY_SECURE_ALBUM_ENABLE

#include <iostream>
#include <queue>
#include <vector>
#include <chrono>
#include <ctime>
#include <thread>
#include <mutex>
#include <unordered_set>
#include <sstream>
#include <functional>
#include <condition_variable>
#include "medialibrary_data_manager.h"
#include "watch_lite/cloud_audit_impl.h"
#include "watch_system_handler.h"
#include "media_log.h"
#include "preferences.h"
#include "preferences_helper.h"

namespace OHOS {
namespace Media {

#define EXPORT __attribute__ ((visibility ("default")))
#define TTL_SECONDS 3600
#define TTL_PERIOD 10000   // 10 second

class Element;
struct ElementPtrCompare {
    bool operator()(const std::shared_ptr<Element>& a, const std::shared_ptr<Element>& b) const;
};
class TTLPriorityQueue {
    public:
    struct AssetParams {
        std::string truncated_path;
        std::string original_path;
        int id {-1};
        int priority {0};
        int64_t added_time {0};
        std::string uri;
        int type {0};
        bool is_sent {false};
    };

private:
    static inline std::unique_ptr<TTLPriorityQueue> instance_;
    static inline std::mutex instance_mutex_;
    std::priority_queue<std::shared_ptr<Element>,
            std::vector<std::shared_ptr<Element>>, ElementPtrCompare> pq;
    std::unordered_set<std::string> signatures;
    std::thread t;
    mutable std::mutex mtx;
    mutable std::mutex xmlUpdateMutex;
    std::atomic<bool> stop_cleanup_thread{false};
    std::atomic<bool> loading{true};
    std::condition_variable cv_;
    const size_t MAX_SIZE = 50;
    const std::string CA_CONFIG_PATH = "/data/storage/el2/base/preferences/CAConfig.xml";
    std::unordered_map<std::string, int32_t> reverseCriticalAssetsMap;
    std::vector<bool> slotUsed;
    std::vector<AssetParams> pendingAdds;
    std::vector<std::string> pendingRemoves;
    std::vector<std::string> originalPathsInQueue;

    void LoadPreferenceCriticalAssets();
    void UpdatePreferenceCriticalAssets(AssetParams assetParam);
    void ClearSlot(const std::shared_ptr<NativePreferences::Preferences> &pref, int slot);
    void RemovePreferenceCriticalAssets(const std::string &dpName);
    void CleanupExpiredItems();
    void CleanupExpiredItemsPeriodically();
    void UpdateFromXML(std::vector<AssetParams>& criticalAssets, std::shared_ptr<NativePreferences::Preferences> pref);
    bool AddElementInner(const AssetParams& dataParams);
    bool RemoveByNameInner(const std::string& name);
    void UpdateIsSentInXML(const std::string& displayName, bool isSent);
    void PopInsertBack(const std::shared_ptr<Element>& element);
    bool SendAsset(const std::shared_ptr<Element>& element);
    std::shared_ptr<Element> GetBack(std::priority_queue<std::shared_ptr<Element>,
            std::vector<std::shared_ptr<Element>>, ElementPtrCompare> queue);
    void CheckConditions(bool &doPass, const int &priority);
    bool UpdatePhotoRiskStatus(const std::string& displayName, const int32_t risk_status);
    bool QueueHasTaskToDo();
    int32_t FindFreeSlot();

public:
    TTLPriorityQueue();
    ~TTLPriorityQueue();

    static TTLPriorityQueue* GetInstance();
    bool AddElement(const AssetParams& dataParams);
    bool RemoveByName(const std::string& name);
    std::shared_ptr<Element> Pop_non_send();
    int32_t GetRemainingQueueSize() const;
    std::vector<std::string> GetElementsTruncatedPaths() const;
    void NotifyThread();
};
} // namespace Media
} // namespace OHOS

#endif
#endif // CRITICAL_LABEL_TASK_QUEUE_H