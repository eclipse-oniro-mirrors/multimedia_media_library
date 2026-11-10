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

#ifndef OHOS_MEDIA_NOTIFY_TASK_WORKER_H
#define OHOS_MEDIA_NOTIFY_TASK_WORKER_H

#include "notify_info_inner.h"
#include "media_change_info.h"
#include "notify_info.h"
#include "media_change_info.h"
#include "user_define_notify_info.h"

#include <atomic>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <condition_variable>

namespace OHOS {
namespace Media {
namespace Notification {
#define EXPORT __attribute__ ((visibility ("default")))
struct NotifyTaskInfo {
    int32_t loopedCnt_{0};
    std::vector<NotifyInfoInner> notifyInfos;
};

class NotifyTaskWorker {
public:
    EXPORT NotifyTaskWorker();
    EXPORT ~NotifyTaskWorker();
    EXPORT static std::shared_ptr<NotifyTaskWorker> GetInstance();
    EXPORT void StartWorker();
    EXPORT void AddTaskInfo(NotifyInfoInner &notifyInfoInner);
    EXPORT void AddUserDefineTaskInfo(const UserDefineNotifyInfo &notifyInfoInner);
    EXPORT void AddDbAvailabilityTaskInfo(const std::string& status, const std::string& reason);
    EXPORT bool IsRunning();

private:
    EXPORT void HandleNotifyTaskPeriod();
    EXPORT void HandleNotifyTask();
    std::vector<MediaChangeInfo> ClassifyNotifyInfo(std::vector<NotifyTaskInfo> &notifyTaskInfos);
    EXPORT void WaitForTask();
    EXPORT bool IsTaskInfosEmpty();
    EXPORT std::vector<NotifyTaskInfo> GetCurrentNotifyMap();
    EXPORT std::vector<NotifyInfo> MergeNotifyInfo(std::vector<MediaChangeInfo> changeInfos);
    EXPORT void DistributeNotifyInfo(std::vector<NotifyInfo> notifyInfos);

    EXPORT std::vector<UserDefineNotifyInfo> GetCurrentUserDefineNotifyVec();
    EXPORT std::vector<std::pair<std::string, std::string>> GetCurrentDbAvailabilityNotifyVec();
    EXPORT void HandleUserDefineNotifyTask();
    EXPORT void HandleDbAvailabilityTask();

private:
    EXPORT static std::unordered_map<int32_t, NotifyTaskInfo> taskInfos_;
    EXPORT static std::shared_ptr<NotifyTaskWorker> notifyTaskWorker_;
    EXPORT static std::mutex instanceMtx_;
    EXPORT static std::once_flag onceFlag_;
    EXPORT std::atomic<bool> isThreadRunning_{false};
    EXPORT std::mutex workLock_;
    EXPORT std::atomic<int32_t> noTaskTims_{0};
    EXPORT std::condition_variable workCv_;
    EXPORT static std::mutex mapMutex_;

    EXPORT static std::vector<UserDefineNotifyInfo> userDefineTaskInfos_;
    EXPORT static std::mutex userDefineVecMutex_;

    EXPORT static std::vector<std::pair<std::string, std::string>> dbAvailabilityTaskInfos_;
    EXPORT static std::mutex dbAvailabilityVecMutex_;
};
} // namespace Notification
} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIA_NOTIFY_H