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
#include "lake_check_scenario.h"

#include "global_scanner.h"
#include "lake_scan_rule_config.h"
#include "media_lake_check.h"
#include "media_log.h"

namespace OHOS::Media {
bool LakeCheckScenario::IsConditionSatisfied(const ConsistencyCheck::DeviceStatus &deviceStatus)
{
    const int32_t PROPER_DEVICE_BATTERY_CAPACITY = 50;
    const int32_t PROPER_DEVICE_TEMPERATURE_LEVEL_37 = 1;
    return deviceStatus.isScreenOff && deviceStatus.isCharging && deviceStatus.isBackgroundTaskAllowed &&
        deviceStatus.batteryCapacity >= PROPER_DEVICE_BATTERY_CAPACITY &&
        deviceStatus.temperature <= PROPER_DEVICE_TEMPERATURE_LEVEL_37 && MediaInLakeNeedCheck();
}

void LakeCheckScenario::Execute(std::atomic<bool> &isInterrupted)
{
    MEDIA_INFO_LOG("Start Execute");
    CHECK_AND_RETURN_LOG(!isInterrupted.load(), "Execute interrupted at entry");
    CheckDfxCollector dfxCollector(CheckScene::LAKE);
    dfxCollector.OnCheckStart();
    ConsistencyCheck::ScenarioProgress progress;
    ScenarioContext context = {isInterrupted, dfxCollector, progress};

    int32_t runningStatus = RunForward(context);
    if (runningStatus != RunningStatus::FINISHED) {
        MEDIA_ERR_LOG("RunForward not finished, end executing. RunningStatus: %{public}d", runningStatus);
        return;
    }

    runningStatus = RunBackward(context);
    if (runningStatus == RunningStatus::INTERRUPTED) {
        MEDIA_WARN_LOG("RunBackward interrupted");
        return;
    }

    SaveFinishedProgress();
    dfxCollector.OnCheckEnd();
    dfxCollector.Report();
}

int32_t LakeCheckScenario::RunForward(ScenarioContext &context)
{
    MEDIA_INFO_LOG("Start RunForward");
    CHECK_AND_RETURN_RET(!context.isInterrupted.load(), RunningStatus::INTERRUPTED);
    auto &scanner = GlobalScanner::GetInstance();
    if (scanner.GetScannerStatus() != ScannerStatus::IDLE) {
        return RunningStatus::NOT_STARTED;
    }

    scanner.RunLakeScan(std::string(LAKE_ROOT_PATH), context.dfxCollector, false);

    return context.isInterrupted.load() ? RunningStatus::INTERRUPTED : RunningStatus::FINISHED;
}

int32_t LakeCheckScenario::RunBackward(ScenarioContext &context)
{
    MEDIA_INFO_LOG("Start RunBackward");
    CHECK_AND_RETURN_RET(!context.isInterrupted.load(), RunningStatus::INTERRUPTED);
    int32_t deleteNum = 0;
    bool ret = CheckAndIfNeedDeletePhotoAlbum(
        deleteNum, [this, &context]() -> bool { return context.isInterrupted.load(); });
    context.dfxCollector.OnAlbumDelete(deleteNum);
    CHECK_AND_RETURN_RET(ret, RunningStatus::INTERRUPTED);
    ClearLakeAlbum();
    return RunningStatus::FINISHED;
}

void LakeCheckScenario::SaveFinishedProgress()
{
    MediaInLakeSetCheckFinish();
}
} // namespace OHOS::Media
