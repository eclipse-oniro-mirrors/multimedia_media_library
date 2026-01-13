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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_VISION_TOTAL_COLUMN_H
#define FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_VISION_TOTAL_COLUMN_H

#include "vision_column_comm.h"

namespace OHOS {
namespace Media {
const std::string STATUS = "status";
const std::string OCR = "ocr";
const std::string LABEL = "label";
const std::string VIDEO_LABEL = "video_label";
const std::string SALIENCY = "saliency";
const std::string OBJECT = "object";
const std::string RECOMMENDATION = "recommendation";
const std::string SEGMENTATION = "segmentation";
const std::string COMPOSITION = "composition";
const std::string FACE = "face";
const std::string FACE_TAG = "face_tag";
const std::string HEAD = "head";
const std::string POSE = "pose";
const std::string DETAIL_ADDRESS = "detail_address";
const std::string GEO = "geo";
const std::string SELECTED = "selected";
const std::string NEGATIVE = "negative";
const std::string ABSTRACT_NODE_ANALYSIS = "abstract_node_analysis";
const std::string AESTHETICS_SCORE_ALL_STATUS = "aesthetics_score_all";
const std::string PRIORITY = "priority";
const std::string AFFECTIVE = "affective";
const std::string PET_STATUS = "pet";
const std::string PET_TAG = "pet_tag";
const std::string PET_FACE = "pet_face";
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_VISION_TOTAL_COLUMN_H