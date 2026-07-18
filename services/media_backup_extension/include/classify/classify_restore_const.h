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

#ifndef CLASSIFY_RESTORE_CONST_H
#define CLASSIFY_RESTORE_CONST_H
#include <string>
#include <optional>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include "rdb_store.h"

struct ClassifyCloneInfo {
    std::optional<int32_t> id;
    std::optional<int32_t> fileIdOld;
    std::optional<int32_t> fileIdNew;
    std::optional<int32_t> categoryId;
    std::optional<std::string> subLabel;
    std::optional<double> prob;
    std::optional<std::vector<uint8_t>> feature;
    std::optional<std::string> simResult;
    std::optional<std::string> labelVersion;
    std::optional<std::string> saliencySubProb;
    std::optional<std::string> analysisVersion;
    std::optional<std::string> captionResult;
    std::optional<std::string> captionVersion;
    std::optional<int32_t> significanceScore;
    std::optional<std::string> significanceScoreVersion;
};

struct ClassifyVideoCloneInfo {
    std::optional<int32_t> id;
    std::optional<int32_t> fileIdOld;
    std::optional<int32_t> fileIdNew;
    std::optional<std::string> categoryId;
    std::optional<std::string> confidenceProbability;
    std::optional<std::string> subCategory;
    std::optional<std::string> subConfidenceProb;
    std::optional<std::string> subLabel;
    std::optional<std::string> subLabelProb;
    std::optional<std::string> subLabelType;
    std::optional<std::string> tracks;
    std::optional<std::vector<uint8_t>> videoPartFeature;
    std::optional<std::string> filterTag;
    std::optional<std::string> algoVersion;
    std::optional<std::string> analysisVersion;
    std::optional<int32_t> triggerGenerateThumbnail;
};

struct ClassifyAlbumInfo {
    std::optional<int32_t> albumId;
    std::optional<std::string> albumName;
    std::optional<std::string> albumType;
    std::optional<std::string> albumSubType;
    std::optional<int32_t> albumIdOld;
    std::optional<int32_t> albumIdNew;
};

struct ClassifyMapInfo {
    std::optional<int32_t> mapAlbum;
    std::optional<int32_t> mapAsset;
    std::optional<int32_t> orderPosition;
};

struct ClassifyCloneRestoreConfig {
    int32_t sceneCode;
    std::string taskId;
    std::shared_ptr<OHOS::NativeRdb::RdbStore> mediaLibraryRdb {nullptr};
    std::shared_ptr<OHOS::NativeRdb::RdbStore> mediaRdb {nullptr};
    std::unordered_map<int32_t, uint32_t>* scoreMaskMap {nullptr};
    std::unordered_map<int32_t, int32_t>* duplicateMap {nullptr};
};

const int32_t PAGE_SIZE = 200;
constexpr int32_t QUERY_COUNT = 200;

const uint32_t BIT1 = 1u << 1;  // 图像意义分
const uint32_t BIT20 = 1u << 20;  // 刷新状态标记

const std::string ID = "id";
const std::string FILE_ID = "file_id";
const std::string CATEGORY_ID = "category_id";
const std::string SUB_LABEL = "sub_label";
const std::string PROB = "prob";
const std::string FEATURE = "feature";
const std::string SIM_RESULT = "sim_result";
const std::string LABEL_VERSION = "label_version";
const std::string SALIENCY_SUB_PROB = "saliency_sub_prob";
const std::string ANALYSIS_VERSION = "analysis_version";
const std::string CAPTION_RESULT = "caption_result";
const std::string CAPTION_VERSION = "caption_version";
const std::string SIGNIFICANCE_SCORE = "significance_score";
const std::string SIGNIFICANCE_SCORE_VERSION = "significance_score_version";

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

const std::string ANALYSIS_LABEL_TABLE = "tab_analysis_label";
const std::string ANALYSIS_VIDEO_TABLE = "tab_analysis_video_label";
const std::string ANALYSIS_ALBUM_TABLE = "AnalysisAlbum";
const std::string ANALYSIS_PHOTO_MAP_TABLE = "AnalysisPhotoMap";
const std::string FIELD_TYPE_INT = "INT";

const int32_t CLASSIFY_STATUS_SUCCESS = 1;
const int32_t CLASSIFY_TYPE = 4097;
const int32_t FRONT_CAMERA = 1;

#endif // CLASSIFY_RESTORE_CONST_H