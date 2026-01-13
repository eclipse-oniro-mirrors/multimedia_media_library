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

#define MLOG_TAG "NotifyTaskWorker"

#include "notify_task_worker.h"

#include "media_log.h"
#include "media_notification_utils.h"
#include "medialibrary_rdbstore.h"
#include "result_set_utils.h"
#include "userfilemgr_uri.h"
#include "medialibrary_tracer.h"
#include "notification_classification.h"
#include "notify_info.h"
#include "media_change_info.h"
#include "notification_distribution.h"
#include "notification_merging.h"

#include <map>
#include <unordered_set>

using namespace std;

namespace OHOS {
namespace Media {
namespace Notification {
shared_ptr<NotifyTaskWorker> NotifyTaskWorker::notifyTaskWorker_{nullptr};
std::unordered_map<int32_t, NotifyTaskInfo> NotifyTaskWorker::taskInfos_;
mutex NotifyTaskWorker::instanceMtx_;
std::mutex NotifyTaskWorker::mapMutex_;

std::vector<UserDefineNotifyInfo> NotifyTaskWorker::userDefineTaskInfos_;
std::mutex NotifyTaskWorker::userDefineVecMutex_;

static const int32_t MAX_WAIT_TIME = 50;
static const int32_t WAIT_FOR_MS = 100;
shared_ptr<NotifyTaskWorker> NotifyTaskWorker::GetInstance()
{
    if (notifyTaskWorker_ == nullptr) {
        lock_guard<mutex> lockGuard(instanceMtx_);
        if (notifyTaskWorker_ == nullptr) {
            notifyTaskWorker_ = make_shared<NotifyTaskWorker>();
        }
    }
    return notifyTaskWorker_;
}

NotifyTaskWorker::NotifyTaskWorker() {}

NotifyTaskWorker::~NotifyTaskWorker() {}

void NotifyTaskWorker::StartWorker()
{
    MEDIA_INFO_LOG("StartWorker");
    isThreadRunning_ = true;
    std::thread([this]() { this->HandleNotifyTaskPeriod(); }).detach();
}

void NotifyTaskWorker::AddTaskInfo(NotifyInfoInner &notifyInfoInner)
{
    lock_guard<mutex> lock(mapMutex_);
    int32_t waitLoopCnt = notifyInfoInner.notifyLevel.waitLoopCnt;
    if (taskInfos_.count(waitLoopCnt) == 0) {
        MEDIA_INFO_LOG("taskInfos_ is zero");
        NotifyTaskInfo notifyTaskInfo;
        notifyTaskInfo.loopedCnt_ = 0;
        notifyTaskInfo.notifyInfos =  {notifyInfoInner};
        taskInfos_[waitLoopCnt] = notifyTaskInfo;
    } else {
        NotifyTaskInfo notifyTaskInfo = taskInfos_[waitLoopCnt];
        notifyTaskInfo.notifyInfos.push_back(notifyInfoInner);
        taskInfos_[waitLoopCnt] = notifyTaskInfo;
        MEDIA_INFO_LOG("notifyInfos size: %{public}d, waitLoopCnt: %{public}d",
            (int32_t)notifyTaskInfo.notifyInfos.size(), waitLoopCnt);
    }
}

void NotifyTaskWorker::AddUserDefineTaskInfo(const UserDefineNotifyInfo &notifyInfoInner)
{
    lock_guard<mutex> lock(userDefineVecMutex_);
    userDefineTaskInfos_.push_back(notifyInfoInner);
}

bool NotifyTaskWorker::IsTaskInfosEmpty()
{
    return taskInfos_.empty() && userDefineTaskInfos_.empty();
}

bool NotifyTaskWorker::IsRunning()
{
    return isThreadRunning_;
}

std::vector<NotifyTaskInfo> NotifyTaskWorker::GetCurrentNotifyMap()
{
    lock_guard<mutex> lock(mapMutex_);
    MEDIA_INFO_LOG("taskInfos_: %{public}d", (int32_t)taskInfos_.size());
    std::vector<NotifyTaskInfo> notifyTaskInfos;
    for (auto it = taskInfos_.begin(); it != taskInfos_.end();) {
        NotifyTaskInfo notifyTaskInfo = it->second;
        int32_t count = (int32_t)it->first;
        notifyTaskInfo.loopedCnt_++;
        if (count == notifyTaskInfo.loopedCnt_) {
            MEDIA_INFO_LOG("same");
            notifyTaskInfos.push_back(notifyTaskInfo);
            it = taskInfos_.erase(it);
        } else {
            taskInfos_[count] = notifyTaskInfo;
            it++;
        }
    }
    
    MEDIA_INFO_LOG("notifyTaskInfos size: %{public}d", (int32_t)notifyTaskInfos.size());
    return notifyTaskInfos;
}

std::vector<UserDefineNotifyInfo> NotifyTaskWorker::GetCurrentUserDefineNotifyVec()
{
    lock_guard<mutex> lock(userDefineVecMutex_);
    MEDIA_INFO_LOG("UserDefineTaskInfos_: %{public}d", (int32_t)userDefineTaskInfos_.size());
    std::vector<UserDefineNotifyInfo> notifyTaskInfos = userDefineTaskInfos_;
    userDefineTaskInfos_.clear();
    
    MEDIA_INFO_LOG("NotifyTaskInfos size: %{public}d, userDefineTaskInfos_: %{public}d",
        (int32_t)notifyTaskInfos.size(), (int32_t)userDefineTaskInfos_.size());
    return notifyTaskInfos;
}

void NotifyTaskWorker::WaitForTask()
{
    std::unique_lock<std::mutex> lock(workLock_);
    if (IsTaskInfosEmpty()) {
        noTaskTims_++;
        if (noTaskTims_ == MAX_WAIT_TIME) {
            MEDIA_INFO_LOG("end worker");
            isThreadRunning_ = false;
            noTaskTims_ = 0;
            return;
        }
    }
    workCv_.wait_for(lock, std::chrono::milliseconds(WAIT_FOR_MS), [this]() { return !isThreadRunning_; });
}

void NotifyTaskWorker::HandleNotifyTask()
{
    MediaLibraryTracer tracer;
    tracer.Start("HandleNotifyTask");
    std::vector<NotifyTaskInfo> notifyTaskInfos = GetCurrentNotifyMap();
    auto changeInfos = ClassifyNotifyInfo(notifyTaskInfos);
    if (changeInfos.empty()) {
        MEDIA_INFO_LOG("changeInfos is empty");
    }
    auto notifyInfos = MergeNotifyInfo(changeInfos);
    if (notifyInfos.empty()) {
        MEDIA_INFO_LOG("notifyInfos is empty");
    }
    DistributeNotifyInfo(notifyInfos);
}

void NotifyTaskWorker::HandleUserDefineNotifyTask()
{
    std::vector<UserDefineNotifyInfo> notifyTaskInfos = GetCurrentUserDefineNotifyVec();
    NotificationDistribution::DistributeUserDefineNotifyInfo(notifyTaskInfos);
}

std::vector<MediaChangeInfo> NotifyTaskWorker::ClassifyNotifyInfo(std::vector<NotifyTaskInfo> &notifyTaskInfos)
{
    MEDIA_INFO_LOG("ClassifyNotifyInfo");
    MEDIA_INFO_LOG("notifyInfos size: %{public}d", (int32_t)notifyTaskInfos.size());
    std::vector<MediaChangeInfo> mediaChangeInfos;
    for (NotifyTaskInfo notifyTaskInfo: notifyTaskInfos) {
        int32_t size = notifyTaskInfo.notifyInfos.size();
        MEDIA_INFO_LOG("notifyInfo size: %{public}d", size);
        NotificationClassification::ConvertNotification(notifyTaskInfo.notifyInfos, mediaChangeInfos);
    }
    return mediaChangeInfos;
}

void NotifyTaskWorker::HandleNotifyTaskPeriod()
{
    MEDIA_INFO_LOG("start notify worker");
    string name("NewNotifyThread");
    pthread_setname_np(pthread_self(), name.c_str());
    while (isThreadRunning_) {
        WaitForTask();
        if (!isThreadRunning_) {
            break;
        }
        if (IsTaskInfosEmpty()) {
            continue;
        }
        HandleNotifyTask();
        HandleUserDefineNotifyTask();
        noTaskTims_ = 0;
    }
    NotificationUtils::UpdateNotificationProp();
    MEDIA_INFO_LOG("end notify worker");
}

std::vector<NotifyInfo> NotifyTaskWorker::MergeNotifyInfo(std::vector<MediaChangeInfo> changeInfos)
{
    if (changeInfos.empty()) {
        MEDIA_INFO_LOG("No change information provided - returning empty result");
        return {};
    }
    return NotificationMerging::MergeNotifyInfo(changeInfos);
}

void NotifyTaskWorker::DistributeNotifyInfo(std::vector<NotifyInfo> notifyInfos)
{
    MediaLibraryTracer tracer;
    tracer.Start("DistributeNotifyInfo");
    if (notifyInfos.empty()) {
        MEDIA_INFO_LOG("No notifications to distribute - returning empty list");
    }
    NotificationDistribution::DistributeNotifyInfo(notifyInfos);
}
} // Notification
} // Media
} // OHOS