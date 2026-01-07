/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef MEDIALIBRARY_VISION_COLUMN_H
#define MEDIALIBRARY_VISION_COLUMN_H

#include <vector>

#include "media_column.h"
#include "userfile_manager_types.h"
#include "userfilemgr_uri.h"
#include "story_play_info_column.h"
#include "story_cover_info_column.h"
#include "story_album_column.h"

namespace OHOS {
namespace Media {
// table name
const std::string PHOTOS_TABLE = "Photos";
const std::string VISION_OCR_TABLE = "tab_analysis_ocr";
const std::string VISION_LABEL_TABLE = "tab_analysis_label";
const std::string VISION_VIDEO_LABEL_TABLE = "tab_analysis_video_label";
const std::string VISION_AESTHETICS_TABLE = "tab_analysis_aesthetics_score";
const std::string VISION_VIDEO_AESTHETICS_TABLE = "tab_analysis_video_aesthetics_score";
const std::string VISION_OBJECT_TABLE = "tab_analysis_object";
const std::string VISION_RECOMMENDATION_TABLE = "tab_analysis_recommendation";
const std::string VISION_SEGMENTATION_TABLE = "tab_analysis_segmentation";
const std::string VISION_COMPOSITION_TABLE = "tab_analysis_composition";
const std::string VISION_SALIENCY_TABLE = "tab_analysis_saliency_detect";
const std::string VISION_HEAD_TABLE = "tab_analysis_head";
const std::string VISION_POSE_TABLE = "tab_analysis_pose";
const std::string VISION_TOTAL_TABLE = "tab_analysis_total";
const std::string VISION_VIDEO_TOTAL_TABLE = "tab_analysis_video_total";
const std::string VISION_IMAGE_FACE_TABLE = "tab_analysis_image_face";
const std::string VISION_VIDEO_FACE_TABLE = "tab_analysis_video_face";
const std::string VISION_ANALYSIS_ALBUM_TOTAL_TABLE = "tab_analysis_album_total";
const std::string VISION_FACE_TAG_TABLE = "tab_analysis_face_tag";
const std::string VISION_AFFECTIVE_TABLE = "tab_analysis_affective";
const std::string SEARCH_TOTAL_TABLE = "tab_analysis_search_index";
const std::string ANALYSIS_ALBUM_TABLE = "AnalysisAlbum";
const std::string ANALYSIS_PHOTO_MAP_TABLE = "AnalysisPhotoMap";
const std::string ANALYSIS_ASSET_SD_MAP_TABLE = "tab_analysis_asset_sd_map";
const std::string ANALYSIS_ALBUM_ASSET_MAP_TABLE = "tab_analysis_album_asset_map";
const std::vector<std::string> VISION_HIGHLIGHT_TABLES = {
    VISION_SALIENCY_TABLE,
    VISION_OBJECT_TABLE,
    VISION_RECOMMENDATION_TABLE,
    VISION_SEGMENTATION_TABLE,
    VISION_COMPOSITION_TABLE,
    VISION_POSE_TABLE,
    VISION_OCR_TABLE,
    VISION_LABEL_TABLE,
    SEARCH_TOTAL_TABLE,
    VISION_AESTHETICS_TABLE,
    HIGHLIGHT_PLAY_INFO_TABLE,
    HIGHLIGHT_COVER_INFO_TABLE,
    HIGHLIGHT_ALBUM_TABLE,
    VISION_IMAGE_FACE_TABLE,
    VISION_FACE_TAG_TABLE,
    VISION_HEAD_TABLE,
    VISION_TOTAL_TABLE,
};
// fake column for merge album
const std::string TARGET_ALBUM_ID = "target_album_id";
const std::string VISION_PET_FACE_TABLE = "tab_analysis_pet_face";
const std::string VISION_PET_TAG_TABLE = "tab_analysis_pet_tag";

const std::string URI_OCR = MEDIALIBRARY_DATA_URI + "/phaccess_ana_ocr_operation";
const std::string URI_LABEL = MEDIALIBRARY_DATA_URI + "/phaccess_ana_label_operation";
const std::string URI_VIDEO_LABEL = MEDIALIBRARY_DATA_URI + "/phaccess_ana_video_label_operation";
const std::string URI_AESTHETICS = MEDIALIBRARY_DATA_URI + "/phaccess_ana_atts_operation";
const std::string URI_VIDEO_AESTHETICS = MEDIALIBRARY_DATA_URI + "/phaccess_ana_video_atts_operation";
const std::string URI_OBJECT = MEDIALIBRARY_DATA_URI + "/phaccess_ana_object_operation";
const std::string URI_RECOMMENDATION = MEDIALIBRARY_DATA_URI + "/phaccess_ana_recommendation_operation";
const std::string URI_SEGMENTATION = MEDIALIBRARY_DATA_URI + "/phaccess_ana_segmentation_operation";
const std::string URI_COMPOSITION = MEDIALIBRARY_DATA_URI + "/phaccess_ana_composition_operation";
const std::string URI_TOTAL = MEDIALIBRARY_DATA_URI + "/phaccess_ana_total_operation";
const std::string URI_SALIENCY = MEDIALIBRARY_DATA_URI + "/" + VISION_SALIENCY_TABLE;
const std::string URI_IMAGE_FACE = MEDIALIBRARY_DATA_URI + "/" + VISION_IMAGE_FACE_TABLE;
const std::string URI_VIDEO_FACE = MEDIALIBRARY_DATA_URI + "/" + VISION_VIDEO_FACE_TABLE;
const std::string URI_FACE_TAG = MEDIALIBRARY_DATA_URI + "/" + VISION_FACE_TAG_TABLE;
const std::string URI_HEAD = MEDIALIBRARY_DATA_URI + "/phaccess_ana_head_operation";
const std::string URI_PET_TAG = MEDIALIBRARY_DATA_URI + "/" + VISION_PET_TAG_TABLE;
const std::string URI_PET_FACE = MEDIALIBRARY_DATA_URI + "/" + VISION_PET_FACE_TABLE;
const std::string URI_POSE = MEDIALIBRARY_DATA_URI + "/phaccess_ana_pose_operation";
const std::string URI_MULTI_CROP = MEDIALIBRARY_DATA_URI + "/phaccess_ana_multi_crop_operation";
const std::string URI_ANALYSIS_ALBUM_TOTAL = MEDIALIBRARY_DATA_URI + "/" + VISION_ANALYSIS_ALBUM_TOTAL_TABLE;
const std::string URI_ASSET_SD_MAP = MEDIALIBRARY_DATA_URI + "/" + ANALYSIS_ASSET_SD_MAP_TABLE;
const std::string URI_ALBUM_ASSET_MAP = MEDIALIBRARY_DATA_URI + "/" + ANALYSIS_ALBUM_ASSET_MAP_TABLE;

constexpr int32_t ANALYSIS_ALBUM_OFFSET = 100000000;
} // namespace Media
} // namespace OHOS
#endif // MEDIALIBRARY_VISION_COLUMN_H