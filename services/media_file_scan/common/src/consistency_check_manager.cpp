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
#define MLOG_TAG "ConsistencyCheckManager"

#include "consistency_check_manager.h"

#include "cpu_utils.h"
#include "global_scanner.h"
#include "media_log.h"
#include "media_thread.h"
#ifdef MEDIALIBRARY_FILE_MGR_SUPPORT
#include "file_manager_check_scenario.h"
#endif
#ifdef MEDIALIBRARY_LAKE_SUPPORT
#include "lake_check_scenario.h"
#endif

namespace OHOS::Media {
const std::vector<CheckScene> SUPPORTED_SCENES = {CheckScene::LAKE, CheckScene::FILE_MANAGER};

ConsistencyCheckManager &ConsistencyCheckManager::GetInstance()
{
    static ConsistencyCheckManager instance;
    return instance;
}

ConsistencyCheckManager::ConsistencyCheckManager() = default;

ConsistencyCheckManager::~ConsistencyCheckManager()
{
    StopAll();
}

void ConsistencyCheckManager::OnDeviceStatusChanged(const ConsistencyCheck::DeviceStatus &deviceStatus)
{
    MEDIA_DEBUG_LOG("OnDeviceStatusChanged, %{public}s", deviceStatus.ToString().c_str());
    StopAction action = HandleDeviceStatusAndGetStopAction(deviceStatus);
    if (action == StopAction::STOP_ALL) {
        StopAll();
        return;
    }
    // Note: only stop current scene, other scenes can still be added
    if (action == StopAction::STOP_RUNNING_SCENE) {
        StopRunningScene();
    }
    for (auto scene : SUPPORTED_SCENES) {
        if (!IsSceneConditionSatisfied(scene, deviceStatus)) {
            continue;
        }
        RequestRun(scene);
    }
}

void ConsistencyCheckManager::RequestRun(CheckScene scene)
{
    std::lock_guard<std::mutex> lock(mutex_);

    CHECK_AND_RETURN(checkEnabled_);
    CHECK_AND_RETURN(scene != CheckScene::IDLE);
    CHECK_AND_RETURN(scene != runningScene_ && !IsScenePending(scene));

    pendingScenes_.push_back(scene);
    MEDIA_INFO_LOG("Add %{public}d, pendingScenes_ size: %{public}zu", static_cast<int32_t>(scene),
        pendingScenes_.size());
    TryStartWorkerLocked();
}

void ConsistencyCheckManager::DisableCheck()
{
    MEDIA_INFO_LOG("Start DisableCheck");
    StopScenesInternal(true, true);
    MEDIA_INFO_LOG("End DisableCheck");
}

void ConsistencyCheckManager::EnableCheck()
{
    std::lock_guard<std::mutex> lock(mutex_);
    checkEnabled_ = true;
    MEDIA_INFO_LOG("checkEnabled_: %{public}d", checkEnabled_);
}

bool ConsistencyCheckManager::IsCheckAllowed() const
{
    return checkEnabled_;
}

ConsistencyCheckManager::StopAction ConsistencyCheckManager::HandleDeviceStatusAndGetStopAction(
    const ConsistencyCheck::DeviceStatus &deviceStatus)
{
    std::lock_guard<std::mutex> lock(mutex_);
    deviceStatus_ = deviceStatus;

    if (!IsCheckAllowed()) {
        return ConsistencyCheckManager::StopAction::STOP_ALL;
    }
    if (runningScene_ != CheckScene::IDLE && !IsSceneConditionSatisfied(runningScene_, deviceStatus)) {
        return ConsistencyCheckManager::StopAction::STOP_RUNNING_SCENE;
    }
    return ConsistencyCheckManager::StopAction::NONE;
}

void ConsistencyCheckManager::StopScenesInternal(bool disableCheck, bool clearQueue)
{
    bool shouldInterruptScanner = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (disableCheck) {
            checkEnabled_ = false;
            MEDIA_INFO_LOG("DisableCheck: checkEnabled_: %{public}d", checkEnabled_);
        }
        if (clearQueue) {
            MEDIA_INFO_LOG("Clear pendingScenes_, size: %{public}zu", pendingScenes_.size());
            pendingScenes_.clear();
        }
        isInterrupted_.store(true);
        if (runningScene_ != CheckScene::IDLE) {
            shouldInterruptScanner = true;
            MEDIA_INFO_LOG("runningScene[%{public}d] is active, disableCheck[%{public}d], clearQueue[%{public}d]",
                static_cast<int32_t>(runningScene_), disableCheck, clearQueue);
        }
    }
    if (shouldInterruptScanner) {
        GlobalScanner::GetInstance().InterruptScanner();
    }
}

void ConsistencyCheckManager::StopAll()
{
    MEDIA_INFO_LOG("Start StopAll");
    StopScenesInternal(false, true);
    MEDIA_INFO_LOG("End StopAll");
}

void ConsistencyCheckManager::StopRunningScene()
{
    MEDIA_INFO_LOG("Start StopRunningScene");
    StopScenesInternal(false, false);
    MEDIA_INFO_LOG("End StopRunningScene");
}

void ConsistencyCheckManager::TryStartWorkerLocked()
{
    if (workerRunning_ || pendingScenes_.empty()) {
        MEDIA_INFO_LOG("workerRunning_: %{public}d, empty pendingScenes_: %{public}d", workerRunning_,
            pendingScenes_.empty());
        return;
    }

    workerRunning_ = true;
    auto func = [this]() { WorkerMain(); };
    Media::thread thread("ConsistencyCheck", func);
    if (thread.is_invalid()) {
        MEDIA_ERR_LOG("Start consistency check thread failed.");
    } else {
        thread.detach();
    }
}

void ConsistencyCheckManager::WorkerMain()
{
    while (true) {
        CheckScene scene = CheckScene::IDLE;
        {
            std::lock_guard<std::mutex> lock(mutex_);

            if (pendingScenes_.empty()) {
                runningScene_ = CheckScene::IDLE;
                workerRunning_ = false;
                MEDIA_INFO_LOG("pendingScenes_ is empty, reset runningScene_: %{public}d, isInterrupted_: %{public}d",
                    static_cast<int32_t>(runningScene_), isInterrupted_.load());
                return;
            }
            scene = pendingScenes_.front();
            pendingScenes_.pop_front();
            runningScene_ = scene;
            isInterrupted_.store(false);
            MEDIA_INFO_LOG("Set runningScene_: %{public}d, pendingScenes_ size: %{public}zu, isInterrupted_: "
                "%{public}d", static_cast<int32_t>(runningScene_), pendingScenes_.size(), isInterrupted_.load());
        }
        ExecuteScene(scene);
    }
}

void ConsistencyCheckManager::ExecuteScene(CheckScene scene)
{
    ConsistencyCheck::DeviceStatus deviceStatus;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        deviceStatus = deviceStatus_;
    }
    std::unique_ptr<ICheckScenario> checkScenario = CreateCheckScenario(scene);
    CHECK_AND_RETURN_LOG(IsSceneConditionSatisfied(checkScenario, deviceStatus),
        "Condition of %{public}d no longer statisfied, %{public}s",
        static_cast<int32_t>(scene), deviceStatus.ToString().c_str());

    MEDIA_INFO_LOG("Execute scene: %{public}d", static_cast<int32_t>(scene));
    CpuUtils::SetSelfThreadAffinity(CpuAffinityType::CPU_IDX_9);
    checkScenario->Execute(isInterrupted_);
}

std::unique_ptr<ICheckScenario> ConsistencyCheckManager::CreateCheckScenario(CheckScene scene) const
{
    switch (scene) {
        case CheckScene::LAKE:
#ifdef MEDIALIBRARY_LAKE_SUPPORT
            return std::make_unique<LakeCheckScenario>();
#else
            return nullptr;
#endif
        case CheckScene::FILE_MANAGER:
#ifdef MEDIALIBRARY_FILE_MGR_SUPPORT
            return std::make_unique<FileManagerCheckScenario>();
#else
            return nullptr;
#endif
        default:
            MEDIA_ERR_LOG("Unknown scene: %{public}d", static_cast<int32_t>(scene));
            return nullptr;
    }
}

bool ConsistencyCheckManager::IsSceneConditionSatisfied(CheckScene scene,
    const ConsistencyCheck::DeviceStatus &deviceStatus) const
{
    std::unique_ptr<ICheckScenario> checkScenario = CreateCheckScenario(scene);
    return IsSceneConditionSatisfied(checkScenario, deviceStatus);
}

bool ConsistencyCheckManager::IsSceneConditionSatisfied(std::unique_ptr<ICheckScenario> &checkScenario,
    const ConsistencyCheck::DeviceStatus &deviceStatus) const
{
    return checkScenario != nullptr && checkScenario->IsConditionSatisfied(deviceStatus);
}

bool ConsistencyCheckManager::IsScenePending(CheckScene scene)
{
    return std::find(pendingScenes_.begin(), pendingScenes_.end(), scene) != pendingScenes_.end();
}
}  // namespace OHOS::Media
