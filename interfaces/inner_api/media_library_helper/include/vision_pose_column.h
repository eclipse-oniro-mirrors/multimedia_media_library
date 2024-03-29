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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_VISION_POSE_COLUMN_H
#define FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_VISION_POSE_COLUMN_H

#include "vision_column_comm.h"

namespace OHOS {
namespace Media {
const std::string POSE_ID = "pose_id";
const std::string POSE_LANDMARKS = "pose_landmarks";
const std::string POSE_SCALE_X = "pose_scale_x";
const std::string POSE_SCALE_Y = "pose_scale_y";
const std::string POSE_SCALE_WIDTH = "pose_scale_width";
const std::string POSE_SCALE_HEIGHT = "pose_scale_height";
const std::string POSE_VERSION = "pose_version";
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_VISION_POSE_COLUMN_H