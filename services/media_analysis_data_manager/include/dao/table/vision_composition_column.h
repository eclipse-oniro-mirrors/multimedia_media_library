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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_VISION_COMPOSITION_COLUMN_H
#define FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_VISION_COMPOSITION_COLUMN_H

#include "vision_column_comm.h"

namespace OHOS {
namespace Media {
const std::string COMPOSITION_ID = "composition_id";
const std::string COMPOSITION_RESOLUTION = "composition_resolution";
const std::string CLOCK_STYLE = "clock_style";
const std::string CLOCK_LOCATION_X = "clock_location_x";
const std::string CLOCK_LOCATION_Y = "clock_location_y";
const std::string CLOCK_COLOUR = "clock_colour";
const std::string COMPOSITION_SCALE_X = "composition_scale_x";
const std::string COMPOSITION_SCALE_Y = "composition_scale_y";
const std::string COMPOSITION_SCALE_WIDTH = "composition_scale_width";
const std::string COMPOSITION_SCALE_HEIGHT = "composition_scale_height";
const std::string COMPOSITION_VERSION = "composition_version";
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_VISION_COMPOSITION_COLUMN_H