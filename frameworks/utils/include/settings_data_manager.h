/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEIDA_LIBRARY_SETTINGS_DATA_MANAGER_H
#define OHOS_MEIDA_LIBRARY_SETTINGS_DATA_MANAGER_H

#include <string>

#include "data_ability_observer_stub.h"
#include "safe_map.h"

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))

enum SwitchStatus {
    NONE = -1,
    CLOSE = 0,
    CLOUD,
    HDC,
};

const std::unordered_map<std::string, SwitchStatus> STRING_SWITCH_STATUS_MAP = {
    {std::to_string(static_cast<int>(SwitchStatus::HDC)),
        SwitchStatus::HDC},
    {std::to_string(static_cast<int>(SwitchStatus::CLOUD)),
        SwitchStatus::CLOUD},
    {std::to_string(static_cast<int>(SwitchStatus::CLOSE)),
        SwitchStatus::CLOSE},
    {std::to_string(static_cast<int>(SwitchStatus::NONE)),
        SwitchStatus::NONE},
};

class SettingsDataManager {
public:
    static SwitchStatus GetPhotosSyncSwitchStatus();
    EXPORT static bool GetHdcDeviceId(std::string& deviceId);

private:
    static int32_t QueryParamInSettingData(const std::string &key, std::string &value);
};

} // OHOS

#endif // OHOS_MEIDA_LIBRARY_SETTINGS_DATA_MANAGER_H
