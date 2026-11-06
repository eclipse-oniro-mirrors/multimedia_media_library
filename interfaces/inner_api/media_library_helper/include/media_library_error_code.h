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

#ifndef INTERFACES_INNER_API_MEDIA_LIBRARY_HELPER_INCLUDE_MEDIA_LIBRARY_ERROR_CODE_H
#define INTERFACES_INNER_API_MEDIA_LIBRARY_HELPER_INCLUDE_MEDIA_LIBRARY_ERROR_CODE_H

#include <cstdint>

namespace OHOS::Media {
constexpr int32_t MEDIA_LIBRARY_INVALID_PARAMETER_ERROR = 23800151;
constexpr int32_t MEDIA_LIBRARY_FILE_FORMAT_ERROR = 23800109;

constexpr int32_t MEDIA_LIBRARY_ACTIVE_ANALYSIS_HIGH_TEMPERATURE_ERROR = 23800203;
constexpr int32_t MEDIA_LIBRARY_ACTIVE_ANALYSIS_LOW_BATTERY_ERROR = 23800204;
constexpr int32_t MEDIA_LIBRARY_ACTIVE_ANALYSIS_LOW_STORAGE_ERROR = 23800205;
constexpr int32_t MEDIA_LIBRARY_ACTIVE_ANALYSIS_POWER_SAVE_MODE_ERROR = 23800206;
constexpr int32_t MEDIA_LIBRARY_ACTIVE_ANALYSIS_ALREADY_RUNNING_ERROR = 23800207;
constexpr int32_t MEDIA_LIBRARY_ACTIVE_ANALYSIS_SWITCH_DISABLED_ERROR = 23800208;
constexpr int32_t MEDIA_LIBRARY_ACTIVE_ANALYSIS_OTHER_ERROR = 23800209;

constexpr int32_t MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR = 23800301;
} // namespace OHOS::Media

#endif // INTERFACES_INNER_API_MEDIA_LIBRARY_HELPER_INCLUDE_MEDIA_LIBRARY_ERROR_CODE_H
