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

#ifdef MEDIALIBRARY_SECURE_ALBUM_ENABLE

#include "critical_label_task_queue.h"
#include "medialibrary_subscriber.h"
#include "media_critical_label_task.h"
#include "medialibrary_related_system_state_manager.h"
#include "media_column.h"
#include "result_set_utils.h"
#include "medialibrary_unistore_manager.h"

namespace OHOS {
namespace Media {

class Element {
public:
    std::string truncated_path_;
    std::string original_path_;
    int id_;
    int priority_;
    int64_t insertion_time_;
    std::string uri_;
    int type_;
    bool is_send_;

    Element(std::string truncated_path, int id, int p, const std::string& element_uri,
        int element_type, bool is_send, std::string original_path)
    {
        truncated_path_ = truncated_path;
        id_ = id;
        priority_ = p;
        insertion_time_ = std::chrono::duration_cast<std::chrono::nanoseconds>(
                              std::chrono::steady_clock::now().time_since_epoch()
                        ).count();
        uri_ = element_uri;
        type_ = element_type;
        is_send_ = is_send;
        original_path_ = original_path;
    }

    bool IsExpired() const
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
                .count() - insertion_time_ > TTL_SECONDS *  1'000'000'000LL;
    }

    bool operator<(const Element& other) const
    {
        if (priority_ == other.priority_) {
            return insertion_time_ > other.insertion_time_;
        }
        return priority_ < other.priority_;
    }

    std::string GenerateSignature() const
    {
        return truncated_path_;
    }

    void MarkAsSent()
    {
        is_send_ = true;
    }

    bool IsSent() const
    {
        return is_send_;
    }

    int GetPriority() const
    {
        return priority_;
    }

    int64_t GetInsertionTime() const
    {
        return insertion_time_;
    }
};

bool ElementPtrCompare::operator()(const std::shared_ptr<Element>& a,
    const std::shared_ptr<Element>& b) const
{
    if (a->GetPriority() == b->GetPriority()) {
        return a->GetInsertionTime() > b->GetInsertionTime();
    }
    return a->GetPriority() < b->GetPriority();
}
TTLPriorityQueue::TTLPriorityQueue() : slotUsed(MAX_SIZE, false)
{
    LoadPreferenceCriticalAssets();
    t = std::thread(&TTLPriorityQueue::CleanupExpiredItemsPeriodically, this);
}

TTLPriorityQueue::~TTLPriorityQueue()
{
    stop_cleanup_thread.store(true, std::memory_order_release);
    cv_.notify_one();
    if (t.joinable()) {
        t.join();
    }
}

    TTLPriorityQueue* TTLPriorityQueue::GetInstance()
    {
        if (instance_ == nullptr) {
            std::lock_guard<std::mutex> lock(instance_mutex_);
            if (instance_ == nullptr) {
                instance_ = std::make_unique<TTLPriorityQueue>();
            }
        }
        return instance_.get();
    }

    int32_t TTLPriorityQueue::GetRemainingQueueSize() const
    {
        std::lock_guard<std::mutex> lock(mtx);
        return MAX_SIZE - pq.size();
    }

    std::vector<std::string> TTLPriorityQueue::GetElementsTruncatedPaths() const
    {
        std::lock_guard<std::mutex> lock(mtx);
        return originalPathsInQueue;
    }

    std::shared_ptr<Element> TTLPriorityQueue::GetBack(std::priority_queue<std::shared_ptr<Element>,
        std::vector<std::shared_ptr<Element>>, ElementPtrCompare> queue)
    {
        while (queue.size() > 1) {
            queue.pop();
        }
        return queue.top();
    }

    bool TTLPriorityQueue::AddElementInner(const AssetParams& dataParams)
    {
        if (signatures.find(dataParams.truncated_path) != signatures.end()) {
            MEDIA_DEBUG_LOG("Element with display_name %{public}s already exists. Skipping insertion.",
                dataParams.truncated_path.c_str());
            return false;
        }

        auto newElement = std::make_shared<Element>(dataParams.truncated_path, dataParams.id,
                    dataParams.priority, dataParams.uri, dataParams.type, dataParams.is_sent, dataParams.original_path);

        int realTimePriority = 2;
        int nonRealTimePriority = 1;
        if (pq.size() >= MAX_SIZE) {
            auto backElement = GetBack(pq);
            if (newElement->priority_ == realTimePriority && backElement->priority_== nonRealTimePriority) {
                RemoveByNameInner(backElement->truncated_path_);
            } else {
            MEDIA_DEBUG_LOG("Queue is full, size: %{public}zu. Element display_name %{public}s skip insert",
                MAX_SIZE, dataParams.truncated_path.c_str());
            return false;
            }
        }
        
        UpdatePreferenceCriticalAssets(dataParams);

        originalPathsInQueue.push_back(dataParams.original_path);
        pq.push(newElement);
        signatures.insert(dataParams.truncated_path);
        MEDIA_INFO_LOG("Element added with display_name %{public}s.", dataParams.truncated_path.c_str());
        return true;
    }

    bool TTLPriorityQueue::AddElement(const AssetParams& dataParams)
    {
        bool result = false;
        CleanupExpiredItems();
        {
            std::lock_guard<std::mutex> lock(mtx);

            if (loading) {
                pendingAdds.push_back(dataParams);
                return true;
            }
            result = AddElementInner(dataParams);
        }
        if (result) {
            cv_.notify_one();
        }
        return result;
    }

    bool TTLPriorityQueue::RemoveByNameInner(const std::string& name)
    {
        if (signatures.find(name) == signatures.end()) {
            MEDIA_INFO_LOG("Element with display_name %{public}s not found.", name.c_str());
            return false;
        }

        std::priority_queue<std::shared_ptr<Element>,
            std::vector<std::shared_ptr<Element>>, ElementPtrCompare> newQueue;
        while (!pq.empty()) {
            auto topElement = pq.top();
            pq.pop();

            if (topElement->GenerateSignature() != name) {
                newQueue.push(topElement);
            } else {
                signatures.erase(name);
                std::string original_path = topElement->original_path_;
                auto it = find(originalPathsInQueue.begin(),
                    originalPathsInQueue.end(), original_path);
                if (it != originalPathsInQueue.end()) {
                    originalPathsInQueue.erase(it);
                }
                MEDIA_INFO_LOG("Element removed with display_name %{public}s.", name.c_str());
            }
        }

        RemovePreferenceCriticalAssets(name);
        pq = newQueue;
        return true;
    }

    bool TTLPriorityQueue::RemoveByName(const std::string& name)
    {
        bool result = false;
        bool needTrigger = false;
        uint32_t nonRealTimeTriggerSize = 10;
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (loading) {
                pendingRemoves.push_back(name);
                return true;
            }
            result = RemoveByNameInner(name);
            needTrigger = result && pq.size() <= nonRealTimeTriggerSize;
        }

        if (needTrigger) {
            MEDIA_INFO_LOG("Size is less than 10 try to add element from non realtime");
            Background::MediaCriticalLabelTask::HandleCriticalLabelProcessing();
        }

        return result;
    }

    std::shared_ptr<Element> TTLPriorityQueue::Pop_non_send()
    {
        std::lock_guard<std::mutex> lock(mtx);

        std::priority_queue<std::shared_ptr<Element>> tempQueue;
        std::shared_ptr<Element> poppedElement = std::make_shared<Element>("", -1, -1, "", -1, false, "");

        while (!pq.empty()) {
            auto currentElement = pq.top();
            pq.pop();
            tempQueue.push(currentElement);
            if (!currentElement->IsSent()) {
                poppedElement = currentElement;
                break;
            }
        }

        while (!tempQueue.empty()) {
            pq.push(tempQueue.top());
            tempQueue.pop();
        }

        return poppedElement;
    }

    void TTLPriorityQueue::UpdateFromXML(std::vector<AssetParams>& criticalAssets,
        std::shared_ptr<NativePreferences::Preferences> pref)
    {
        for (uint32_t i = 0; i < MAX_SIZE; i++) {
            std::string assetName = pref->GetString("CA_" + std::to_string(i) + "_NAME", "");
        if (assetName.empty()) {
            continue;
        }
            AssetParams params;
            params.id = pref->GetInt("CA_" + std::to_string(i) + "_ID", -1);
            params.uri = pref->GetString("CA_" + std::to_string(i) + "_URI", "");
            params.priority = pref->GetInt("CA_" + std::to_string(i) + "_PRIORITY", 0);
            params.original_path = pref->GetString("CA_" + std::to_string(i) + "_PATH", "");
            params.truncated_path = pref->GetString("CA_" + std::to_string(i) + "_NAME", "");
            params.type = pref->GetInt("CA_" + std::to_string(i) + "_TYPE", 0);
            params.added_time = pref->GetLong("CA_" + std::to_string(i) + "_ADDEDTIME", 0);
            params.is_sent = pref->GetBool("CA_" + std::to_string(i) + "_ISSENT", false);
            reverseCriticalAssetsMap.emplace(params.truncated_path, i);

            criticalAssets.push_back(params);
        }
    }

    void TTLPriorityQueue::LoadPreferenceCriticalAssets()
    {
        std::lock_guard<std::mutex> lock(mtx);
        MEDIA_INFO_LOG("loadPreferenceCriticalAssets start");
        std::vector<AssetParams> criticalAssets;
        std::shared_ptr<NativePreferences::Preferences> pref = nullptr;
        pq = std::priority_queue<std::shared_ptr<Element>,
        std::vector<std::shared_ptr<Element>>, ElementPtrCompare>();
        signatures.clear();

        int32_t errcode = ERR_OK;
        pref = NativePreferences::PreferencesHelper::GetPreferences(CA_CONFIG_PATH, errcode);
        CHECK_AND_RETURN_LOG(pref != nullptr, "pref is nullptr, errCode: %{public}d", errcode);
        int count = pref->GetInt("CA_COUNT", 0);
        if (count <= 0) {
            loading = false;
            MEDIA_INFO_LOG("xml is empty");
            return;
        }
        UpdateFromXML(criticalAssets, pref);
        MEDIA_INFO_LOG("LoadPreferenceCriticalAssets criticalassetssize %{public}zu", criticalAssets.size());
        std::sort(criticalAssets.begin(), criticalAssets.end(),
            [](const AssetParams& a, const AssetParams& b) {
                return a.added_time < b.added_time;
            });

        for (const auto& asset : criticalAssets) {
            auto elementFromXml = std::make_shared<Element>(asset.truncated_path, asset.id,
                asset.priority, asset.uri, asset.type, asset.is_sent, asset.original_path);
            pq.push(elementFromXml);
            signatures.insert(asset.truncated_path);
        }

        for (const auto& pendingAssetRemove : pendingRemoves) {
            RemoveByNameInner(pendingAssetRemove);
        }
        for (const auto& pendingAssetAdd : pendingAdds) {
            AddElementInner(pendingAssetAdd);
        }
        pendingRemoves.clear();
        pendingAdds.clear();
        loading = false;
    }

    int32_t TTLPriorityQueue::FindFreeSlot()
    {
        for (uint32_t i = 0; i < slotUsed.size(); i++) {
            if (!slotUsed[i]) {
                return i;
            }
        }
        return -1;
    }

    void TTLPriorityQueue::UpdatePreferenceCriticalAssets(AssetParams assetParam)
    {
        std::lock_guard<std::mutex> lock(xmlUpdateMutex);
        MEDIA_INFO_LOG("updatePreferenceCriticalAssets start");
        std::shared_ptr<NativePreferences::Preferences> pref = nullptr;
        int32_t errcode = ERR_OK;
        pref = NativePreferences::PreferencesHelper::GetPreferences(CA_CONFIG_PATH, errcode);

        CHECK_AND_RETURN_LOG(pref != nullptr, "pref is nullptr, errCode: %{public}d", errcode);

        int count = pref->GetInt("CA_COUNT", 0);
        if (count < 0) {
            count = 0;
        }

        // check duplicate
        auto it = reverseCriticalAssetsMap.find(assetParam.truncated_path);
        if (it != reverseCriticalAssetsMap.end()) {
            return;
        }

        // check empty slot
        int32_t slotIdx = FindFreeSlot();
        CHECK_AND_RETURN_LOG(slotIdx != -1, "size is full");

        int slot = slotIdx;
        // assets in xml always evaluated as non real-time
        auto preferencePriority = 1;
        pref->PutInt("CA_" + std::to_string(slot) + "_ID", assetParam.id);
        pref->PutString("CA_" + std::to_string(slot) + "_URI", assetParam.uri);
        pref->PutString("CA_" + std::to_string(slot) + "_PATH", assetParam.original_path);
        pref->PutInt("CA_" + std::to_string(slot) + "_PRIORITY", preferencePriority);
        pref->PutString("CA_" + std::to_string(slot) + "_NAME", assetParam.truncated_path);
        pref->PutInt("CA_" + std::to_string(slot) + "_TYPE", assetParam.type);
        pref->PutLong("CA_" + std::to_string(slot) + "_ADDEDTIME", assetParam.added_time);
        pref->PutBool("CA_" + std::to_string(slot) + "_ISSENT", assetParam.is_sent);

        pref->PutInt("CA_COUNT", count + 1);
        pref->FlushSync();
        reverseCriticalAssetsMap.emplace(assetParam.truncated_path, slot);
        slotUsed[slot] = true;
    }

    void TTLPriorityQueue::ClearSlot(const std::shared_ptr<NativePreferences::Preferences> &pref, int slot)
    {
        pref->Delete("CA_" + std::to_string(slot) + "_ID");
        pref->Delete("CA_" + std::to_string(slot) + "_URI");
        pref->Delete("CA_" + std::to_string(slot) + "_PATH");
        pref->Delete("CA_" + std::to_string(slot) + "_PRIORITY");
        pref->Delete("CA_" + std::to_string(slot) + "_NAME");
        pref->Delete("CA_" + std::to_string(slot) + "_TYPE");
        pref->Delete("CA_" + std::to_string(slot) + "_ADDEDTIME");
        pref->Delete("CA_" + std::to_string(slot) + "_ISSENT");
    }

    void TTLPriorityQueue::RemovePreferenceCriticalAssets(const std::string &dpName)
    {
        std::lock_guard<std::mutex> lock(xmlUpdateMutex);
        MEDIA_INFO_LOG("removePreferenceCriticalAssets start");
        std::shared_ptr<NativePreferences::Preferences> pref = nullptr;
        int32_t errcode = ERR_OK;
        pref = NativePreferences::PreferencesHelper::GetPreferences(CA_CONFIG_PATH, errcode);
        CHECK_AND_RETURN_LOG(pref != nullptr, "pref is nullptr, errCode: %{public}d", errcode);

        int count = pref->GetInt("CA_COUNT", 0);
        CHECK_AND_RETURN_LOG(count > 0, "There is no critical asset to remove in xml");

        int idx = -1;
        auto it = reverseCriticalAssetsMap.find(dpName);
        if (it != reverseCriticalAssetsMap.end()) {
            idx = it->second;
        }

        CHECK_AND_RETURN_LOG(idx != -1, "asset is not found");
        
        ClearSlot(pref, idx);

        pref->PutInt("CA_COUNT", count - 1);
        pref->FlushSync();

        reverseCriticalAssetsMap.erase(dpName);
        slotUsed[idx] = false;
    }

    void TTLPriorityQueue::CleanupExpiredItems()
    {
        std::lock_guard<std::mutex> lock(mtx);
        while (!pq.empty() && pq.top()->IsExpired()) {
            MEDIA_DEBUG_LOG("Expired item removed from queue");
            const std::shared_ptr<Element> expired = pq.top();
            std::string signature = expired->GenerateSignature();
            std::string original_path = expired->original_path_;
            RemovePreferenceCriticalAssets(signature);
            signatures.erase(signature);
            auto it = find(originalPathsInQueue.begin(),
                originalPathsInQueue.end(), original_path);
            if (it != originalPathsInQueue.end()) {
                originalPathsInQueue.erase(it);
            }
            pq.pop();
        }
    }

    void TTLPriorityQueue::UpdateIsSentInXML(const std::string& displayName, bool isSent)
    {
        std::lock_guard<std::mutex> lock(xmlUpdateMutex);

        MEDIA_INFO_LOG("updateIsSentInXML start for display_name %{public}s, isSent: %{public}d",
            displayName.c_str(), isSent);

        std::shared_ptr<NativePreferences::Preferences> pref = nullptr;
        int32_t errcode = ERR_OK;
        pref = NativePreferences::PreferencesHelper::GetPreferences(CA_CONFIG_PATH, errcode);

        CHECK_AND_RETURN_LOG(pref != nullptr, "pref is nullptr, errCode: %{public}d", errcode);

        auto it = reverseCriticalAssetsMap.find(displayName);
        if (it == reverseCriticalAssetsMap.end()) {
            MEDIA_INFO_LOG("Element with display_name %{public}s not found in XML.", displayName.c_str());
            return;
        }

        int slot = it->second;
        pref->PutBool("CA_" + std::to_string(slot) + "_ISSENT", isSent);
        pref->FlushSync();

        MEDIA_INFO_LOG("is_sent updated for display_name %{public}s to %{public}d",
            displayName.c_str(), isSent);
    }

    void TTLPriorityQueue::PopInsertBack(const std::shared_ptr<Element>& element)
    {
        AssetParams params;
        params.truncated_path = element->truncated_path_;
        params.original_path = element->original_path_;
        params.id = element->id_;
        params.priority = element->priority_;
        params.type = element->type_;
        params.uri = element->uri_;
        RemoveByName(element->truncated_path_);
        AddElement(params);
    }

    bool TTLPriorityQueue::UpdatePhotoRiskStatus(const std::string& displayName, const int32_t risk_status)
    {
        auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
        CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "rdbStore is nullptr");

        std::string sql = "UPDATE " + PhotoColumn::PHOTOS_TABLE +
            " SET " + PhotoColumn::PHOTO_RISK_STATUS + " = " +
            std::to_string(risk_status) +
            " ," + PhotoColumn::PHOTO_IS_CRITICAL + " = 1" +
            " WHERE " + MediaColumn::MEDIA_FILE_PATH + " LIKE '%" + displayName + "'";

        auto ret = rdbStore->ExecuteSql(sql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("UpdatePhotoRiskStatus failed, ret = %{public}d", ret);
            return false;
        }
        return true;
    }

    bool TTLPriorityQueue::SendAsset(const std::shared_ptr<Element>& element)
    {
        auto dataManagerInstance = MediaLibraryDataManager::GetInstance();
        CHECK_AND_RETURN_RET_LOG(dataManagerInstance != nullptr, false, "dataManagerInstance is nullptr");
        auto cloudAuditInstance = dataManagerInstance->GetCloudAuditInstance();
        CHECK_AND_RETURN_RET_LOG(cloudAuditInstance != nullptr, false, "cloudAuditInstance is nullptr");
        MEDIA_DEBUG_LOG("UploadInfoToAudit Start: Asset sending: %{public}s at: %{public}" PRId64,
                element->truncated_path_.c_str(), element->insertion_time_);
        int32_t maxRetryCount = 3;
        int32_t retryCount = 0;
        int32_t res = -1;
        while (res != 0 && retryCount < maxRetryCount) {
            auto instance = MedialibraryRelatedSystemStateManager::GetInstance();
            CHECK_AND_RETURN_RET_LOG(instance != nullptr, false,
                "MedialibraryRelatedSystemStateManager instance is nullptr");
            bool isWifiConnected = instance->IsNetAvailableInOnlyWifiCondition();
            bool networkAvaliable = isWifiConnected
                    || (instance->IsNetValidatedAtRealTime() && instance->IsCellularNetConnected());
            CHECK_AND_RETURN_RET_LOG(networkAvaliable != false, false,
                "MedialibraryRelatedSystemStateManager network is not connected");
            retryCount++;
            res = cloudAuditInstance->UploadInfoToAudit(element->type_, element->uri_, element->truncated_path_);
            if (res != 0) {
                MEDIA_DEBUG_LOG("UploadInfoToAudit Failed display_name %{public}s, retryCount %{public}d",
                    element->truncated_path_.c_str(), retryCount);
            }
        }
        // If 3 time upload failled
        if (res != 0 && maxRetryCount == retryCount) {
            MEDIA_DEBUG_LOG("UploadInfoToAudit Failed to upload to audit display_name %{public}s",
                element->truncated_path_.c_str());
            UpdatePhotoRiskStatus(element->truncated_path_,
                static_cast<int32_t>(PhotoRiskStatus::SUSPICIOUS));
            RemoveByName(element->truncated_path_);
        }
        element->MarkAsSent();
        UpdateIsSentInXML(element->truncated_path_, true);
        return true;
    }

    void TTLPriorityQueue::CheckConditions(bool &doPass, const int &priority)
    {
        if (priority == 1) {
            doPass = MedialibrarySubscriber::IsCriticalTypeStatusOn();
        } else {
            auto instance = MedialibraryRelatedSystemStateManager::GetInstance();
            CHECK_AND_RETURN_LOG(instance != nullptr, "MedialibraryRelatedSystemStateManager instance is nullptr");
            bool isWifiConnected = instance->IsNetAvailableInOnlyWifiCondition();
            bool isRealtimeNetworkSufficient = isWifiConnected
                || (instance->IsNetValidatedAtRealTime() && instance->IsCellularNetConnected());
            doPass = (isRealtimeNetworkSufficient && (WatchSystemHandler::GetAllowNetworkSwitch() || isWifiConnected));
        }
    }

    bool TTLPriorityQueue::QueueHasTaskToDo()
    {
        if (pq.empty()) {
            return false;
        }
        auto instance = MedialibraryRelatedSystemStateManager::GetInstance();
        CHECK_AND_RETURN_RET_LOG(instance != nullptr, false,
            "MedialibraryRelatedSystemStateManager instance is nullptr");
        bool isWifiConnected = instance->IsNetAvailableInOnlyWifiCondition();
        bool networkAvaliable = isWifiConnected
                || (instance->IsNetValidatedAtRealTime() && instance->IsCellularNetConnected());

        CHECK_AND_RETURN_RET_LOG(networkAvaliable != false, false,
            "MedialibraryRelatedSystemStateManager network is not connected");
        bool suitableRealtime = false;
        bool suitableNonRealtime = false;
        std::priority_queue<std::shared_ptr<Element>> tempQueue;
        bool queueHasAssetToUpload = false;
        const int32_t realtimePriority = 2;
        while (!pq.empty()) {
            auto currentElement = pq.top();
            pq.pop();
            tempQueue.push(currentElement);
            if (!currentElement->IsSent()) {
                queueHasAssetToUpload = true;
                if (currentElement->priority_ == realtimePriority) {
                    suitableRealtime = true;
                } else {
                    suitableNonRealtime = true;
                }
                break;
            }
        }

        while (!tempQueue.empty()) {
            pq.push(tempQueue.top());
            tempQueue.pop();
        }

        CHECK_AND_RETURN_RET_LOG(queueHasAssetToUpload != false, false,
            "MedialibraryRelatedSystemStateManager queue has no asset that not yet send.");
        
        if (suitableRealtime && (WatchSystemHandler::GetAllowNetworkSwitch() || isWifiConnected)) {
            return true;
        }
        return suitableNonRealtime && MedialibrarySubscriber::IsCriticalTypeStatusOn();
    }

    void TTLPriorityQueue::NotifyThread()
    {
        cv_.notify_one();
    }

    void TTLPriorityQueue::CleanupExpiredItemsPeriodically()
    {
        MEDIA_DEBUG_LOG("cleanupExpiredItemsPeriodically start");
        std::chrono::steady_clock::time_point next_cleanup_time =
            std::chrono::steady_clock::now() + std::chrono::milliseconds(TTL_PERIOD);
        bool do_pass = false;
        std::string cache = "";
        uint32_t count = 0;
        uint32_t maxTry = 3;

        while (!stop_cleanup_thread.load(std::memory_order_acquire)) {
            std::unique_lock<std::mutex> lck(mtx);
            cv_.wait(lck, [&]() {
                return stop_cleanup_thread.load(std::memory_order_acquire) || QueueHasTaskToDo();
            });
            if (stop_cleanup_thread.load(std::memory_order_acquire)) {
                break;
            }
            lck.unlock();

            std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
            if (now < next_cleanup_time) {
                std::this_thread::sleep_until(next_cleanup_time);
                now = std::chrono::steady_clock::now();
            }
            next_cleanup_time = now + std::chrono::milliseconds(TTL_PERIOD);
            CleanupExpiredItems();
            auto element = Pop_non_send();
            CHECK_AND_CONTINUE(element->id_ != -1);
            do_pass = false;
            CheckConditions(do_pass, element->priority_);
            if (!do_pass) {
                if (cache == "") {
                    cache = element->truncated_path_;
                }
                if (element->truncated_path_ == cache) {
                    count++;
                }
                if (count == maxTry) {
                    PopInsertBack(element);
                    count = 0;
                    cache = "";
                    continue;
                }
            } else {
                if (!SendAsset(element)) {
                    continue;
                }
                count = 0;
                cache = "";
            }
        }
    }
} // namespace Media
} // namespace OHOS

#endif