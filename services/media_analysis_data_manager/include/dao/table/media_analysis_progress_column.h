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

#ifndef MEDIA_ANALYSIS_PROGRESS_COLUMN_H
#define MEDIA_ANALYSIS_PROGRESS_COLUMN_H

#include "vision_column_comm.h"

namespace OHOS {
namespace Media {
// progress table name
const std::string TAB_ANALYSIS_PROGRESS_TABLE = "tab_analysis_progress";

const std::string SEARCH_FINISH_CNT = "search_finish_cnt";
const std::string LOCATION_FINISH_CNT = "location_finish_cnt";
const std::string FACE_FINISH_CNT = "face_finish_cnt";
const std::string OBJECT_FINISH_CNT = "object_finish_cnt";
const std::string AESTHETIC_FINISH_CNT = "aesthetic_finish_cnt";
const std::string OCR_FINISH_CNT = "ocr_finish_cnt";
const std::string POSE_FINISH_CNT = "pose_finish_cnt";
const std::string SALIENCY_FINISH_CNT = "saliency_finish_cnt";
const std::string RECOMMENDATION_FINISH_CNT = "recommendation_finish_cnt";
const std::string SEGMENTATION_FINISH_CNT = "segmentation_finish_cnt";
const std::string BEAUTY_AESTHETIC_FINISH_CNT = "beauty_aesthetic_finish_cnt";
const std::string HEAD_DETECT_FINISH_CNT = "head_detect_finish_cnt";
const std::string LABEL_DETECT_FINISH_CNT = "label_detect_finish_cnt";
const std::string TOTAL_IMAGE_CNT = "total_image_cnt";
const std::string FULLY_ANALYZED_IMAGE_CNT = "fully_analyzed_image_cnt";
const std::string TOTAL_PROGRESS = "total_progress";
const std::string CREATED_TIME = "created_time";
const std::string UPDATED_TIME = "updated_time";

const std::string EXTRA_QUOTA_INDEX_BUILD_CNT = "extra_quota_index_build_cnt";
const std::string EXTRA_QUOTA_INDEX_UPDATE_CNT = "extra_quota_index_update_cnt";
const std::string EXTRA_QUOTA_INDEX_DELETE_CNT = "extra_quota_index_delete_cnt";
const std::string EXTRA_QUOTA_OCR_CNT = "extra_quota_ocr_cnt";
const std::string EXTRA_QUOTA_SHARED_BACKBONE_CNT = "extra_quota_shared_backbone_cnt";
const std::string EXTRA_QUOTA_MODIFY_TIME = "extra_quota_modify_time";

const std::string BASE_QUOTA_INDEX_BUILD_CNT = "base_quota_index_build_cnt";
const std::string BASE_QUOTA_INDEX_UPDATE_CNT = "base_quota_index_update_cnt";
const std::string BASE_QUOTA_INDEX_DELETE_CNT = "base_quota_index_delete_cnt";
const std::string BASE_QUOTA_OCR_CNT = "base_quota_ocr_cnt";
const std::string BASE_QUOTA_SHARED_BACKBONE_CNT = "base_quota_shared_backbone_cnt";
const std::string BASE_QUOTA_LABEL_CNT = "base_quota_label_cnt";
const std::string BASE_QUOTA_MODIFY_TIME = "base_quota_modify_time";
const std::string CHECK_SPACE_FLAG = "check_space_flag";

const std::string URI_PROGRESS_TABLE = "datashare:///media/" + TAB_ANALYSIS_PROGRESS_TABLE;
} // namespace Media
} // namespace OHOS
#endif // MEDIA_ANALYSIS_PROGRESS_COLUMN_H