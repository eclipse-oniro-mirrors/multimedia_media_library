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
#ifndef OHOS_MEDIA_CHECK_STATUS_HELPER_H
#define OHOS_MEDIA_CHECK_STATUS_HELPER_H

#include <string>

#include "check_scene.h"
#include "consistency_check_data_types.h"
#include "preferences_helper.h"

namespace OHOS::Media {
class CheckStatusHelper {
public:
    CheckStatusHelper(CheckScene scene) : scene_(scene) {}

    int32_t GetInt32ValueByKey(const std::string &key, int32_t defaultValue = 0);
    void SetInt32ValueByKey(const std::string &key, int32_t value);
    int64_t GetInt64ValueByKey(const std::string &key, int64_t defaultValue = 0);
    void SetInt64ValueByKey(const std::string &key, int64_t value);

    int64_t GetLastEndTimeInMs(int64_t defaultValue = 0);
    void LoadStatus(ConsistencyCheck::ScenarioProgress &progress, ConsistencyCheck::DfxStats &dfxStats);
    void SaveCurrentStatus(const ConsistencyCheck::ScenarioProgress &progress,
        const ConsistencyCheck::DfxStats &dfxStats);
    void SaveFinishedStatus(int64_t timeInMs);

private:
    std::shared_ptr<NativePreferences::Preferences> GetPreferences();

    CheckScene scene_ {CheckScene::IDLE};
    std::shared_ptr<NativePreferences::Preferences> prefs_;
};
} // namespace OHOS::Media

#endif // OHOS_MEDIA_CHECK_STATUS_HELPER_H
