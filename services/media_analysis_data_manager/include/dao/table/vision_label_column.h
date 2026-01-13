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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_VISION_LABEL_COLUMN_H
#define FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_VISION_LABEL_COLUMN_H

#include "vision_column_comm.h"

namespace OHOS {
namespace Media {
const std::string CATEGORY_ID = "category_id";
const std::string SUB_LABEL = "sub_label";
const std::string LABEL_VERSION = "label_version";
const std::string FEATURE = "feature";
const std::string SIM_RESULT = "sim_result";
const std::string SALIENCY_SUB_PROB = "saliency_sub_prob";
const std::string SEARCH_TAG_TYPE = "search_tag_type";
const std::string SEARCH_TAG_VECTOR = "search_tag_vector";
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_VISION_LABEL_COLUMN_H