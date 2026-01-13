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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_VISION_VIDEO_LABEL_COLUMN_H
#define FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_VISION_VIDEO_LABEL_COLUMN_H

#include "vision_column_comm.h"
#include "vision_label_column.h"

namespace OHOS {
namespace Media {
const std::string CONFIDENCE_PROBABILITY = "confidence_probability";
const std::string SUB_CATEGORY = "sub_category";
const std::string SUB_CONFIDENCE_PROB = "sub_confidence_prob";
const std::string SUB_LABEL_PROB = "sub_label_prob";
const std::string SUB_LABEL_TYPE = "sub_label_type";
const std::string TRACKS = "tracks";
const std::string VIDEO_PART_FEATURE = "video_part_feature";
const std::string FILTER_TAG = "filter_tag";
const std::string ALGO_VERSION = "algo_version";
const std::string TRIGGER_GENERATE_THUMBNAIL = "trigger_generate_thumbnail";
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_VISION_VIDEO_LABEL_COLUMN_H