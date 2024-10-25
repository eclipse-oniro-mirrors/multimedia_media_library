
/*
 * Copyright (C) 2024-2024 Huawei Device Co., Ltd.
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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_USER_PHOTOGRAPHY_INFO_COLUMN_H
#define FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_USER_PHOTOGRAPHY_INFO_COLUMN_H

#include <string>

namespace OHOS {
namespace Media {
// user photography info table name
const std::string USER_PHOTOGRAPHY_INFO_TABLE = "tab_user_photography_info";

// create user photography info table
const std::string AVERAGE_AESTHETICS_SCORE = "average_aesthetics_score";
const std::string CAPTURE_AESTHETICS_SCORE = "capture_aesthetics_score";
const std::string AVERAGE_AESTHETICS_COUNT = "average_aesthetics_count";
const std::string CAPTURE_AESTHETICS_COUNT = "capture_aesthetics_count";
const std::string CALCULATE_TIME_START = "calculate_time_start";
const std::string CALCULATE_TIME_END = "calculate_time_end";

const std::string URI_USER_PHOTOGRAPHY_INFO = MEDIALIBRARY_DATA_URI + "/" + USER_PHOTOGRAPHY_INFO_TABLE;
} // namespace Media
} // namespace OHOS
#endif // FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_USER_PHOTOGRAPHY_INFO_COLUMN_H