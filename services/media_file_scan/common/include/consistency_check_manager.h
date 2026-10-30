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
#ifndef OHOS_MEDIA_CONSISTENCY_CHECK_MANAGER_H
#define OHOS_MEDIA_CONSISTENCY_CHECK_MANAGER_H

#include <atomic>
#include <deque>
#include <mutex>
#include <string>

#include "check_scene.h"
#include "consistency_check_data_types.h"
#include "i_check_scenario.h"

namespace OHOS::Media {
class ICheckScenario;

class ConsistencyCheckManager {
public:
    static ConsistencyCheckManager &GetInstance();

    void OnDeviceStatusChanged(const ConsistencyCheck::DeviceStatus &deviceStatus);
    void RequestRun(CheckScene scene);

    void DisableCheck();
    void EnableCheck();

private:
    enum class StopAction {
        NONE = 0,
        STOP_RUNNING_SCENE,
        STOP_ALL,
    };

    ConsistencyCheckManager();
    ~ConsistencyCheckManager();
    ConsistencyCheckManager(const ConsistencyCheckManager&) = delete;
    const ConsistencyCheckManager &operator=(const ConsistencyCheckManager&) = delete;

    bool IsCheckAllowed() const;
    StopAction HandleDeviceStatusAndGetStopAction(const ConsistencyCheck::DeviceStatus &deviceStatus);
    void StopScenesInternal(bool disableCheck, bool clearQueue);
    void StopAll();
    void StopRunningScene();

    void TryStartWorkerLocked();
    void WorkerMain();
    void ExecuteScene(CheckScene scene);

    std::unique_ptr<ICheckScenario> CreateCheckScenario(CheckScene scene) const;
    bool IsSceneConditionSatisfied(CheckScene scene, const ConsistencyCheck::DeviceStatus &deviceStatus) const;
    bool IsSceneConditionSatisfied(std::unique_ptr<ICheckScenario> &checkScenario,
        const ConsistencyCheck::DeviceStatus &deviceStatus) const;
    bool IsScenePending(CheckScene scene);
    
    std::mutex mutex_;
    std::atomic<bool> isInterrupted_ {false};
    ConsistencyCheck::DeviceStatus deviceStatus_;
    std::deque<CheckScene> pendingScenes_;
    CheckScene runningScene_ {CheckScene::IDLE};
    bool workerRunning_ {false};
    bool checkEnabled_ {true};
};
} // namespace OHOS::Media

#endif // OHOS_MEDIA_CONSISTENCY_CHECK_MANAGER_H