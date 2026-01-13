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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_VISION_AESTHETICS_SCORE_COLUMN_H
#define FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_VISION_AESTHETICS_SCORE_COLUMN_H

#include "vision_column_comm.h"

namespace OHOS {
namespace Media {
const std::string AESTHETICS_SCORE = "aesthetics_score";
const std::string AESTHETICS_VERSION = "aesthetics_version";
const std::string AESTHETICS_ALL_VERSION = "aesthetics_all_version";
const std::string AESTHETICS_SCORE_ALL = "aesthetics_score_all";
const std::string IS_FILTERED_HARD = "is_filtered_hard";
const std::string CLARITY_SCORE_ALL = "clarity_score_all";
const std::string SATURATION_SCORE_ALL = "saturation_score_all";
const std::string LUMINANCE_SCORE_ALL = "luminance_score_all";
const std::string SEMANTICS_SCORE = "semantics_score";
const std::string IS_BLACK_WHITE_STRIPE = "is_black_white_stripe";
const std::string IS_BLURRY = "is_blurry";
const std::string IS_MOSAIC = "is_mosaic";
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_VISION_AESTHETICS_SCORE_COLUMN_H