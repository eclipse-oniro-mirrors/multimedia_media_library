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
#ifndef OHOS_MEDIA_LAKE_CHECK_SCENARIO_H
#define OHOS_MEDIA_LAKE_CHECK_SCENARIO_H

#include "i_check_scenario.h"

namespace OHOS::Media {
class LakeCheckScenario final : public ICheckScenario {
public:
    bool IsConditionSatisfied(const ConsistencyCheck::DeviceStatus &deviceStatus) override;
    void Execute(std::atomic<bool> &isInterrupted) override;

private:
    int32_t RunForward(ScenarioContext &context);
    int32_t RunBackward(ScenarioContext &context);

    void SaveFinishedStatus();
};
} // namespace OHOS::Media

#endif // OHOS_MEDIA_LAKE_CHECK_SCENARIO_H