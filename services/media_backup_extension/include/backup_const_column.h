/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef BACKUP_CONST_COLUMN_H
#define BACKUP_CONST_COLUMN_H

#include "backup_const.h"
#include "media_smart_album_column.h"

namespace OHOS {
namespace Media {
constexpr int32_t IS_LOCAL_TRUE = 1;
constexpr int32_t RENAME_OPERATION_RENAMED = 1;
const std::string DEFAULT_BACKUP_VERSION = "backup1.0";
const std::string GALLERY_TABLE_MERGE_FACE = "merge_face";
const std::string GALLERY_TABLE_MERGE_TAG = "merge_tag";
const std::string GALLERY_TABLE_MEDIA = "gallery_media";
const std::string GALLERY_TABLE_FACE = "face";
const std::string GALLERY_TAG_ID = "tag_id";
const std::string GALLERY_FACE_ID = "face_id";
const std::string GALLERY_GROUP_TAG = "merge_tag.group_tag";
const std::string GALLERY_TAG_NAME = "merge_tag.tag_name";
const std::string GALLERY_USER_OPERATION = "merge_tag.user_operation";
const std::string GALLERY_RENAME_OPERATION = "merge_tag.rename_operation";
const std::string GALLERY_USER_DISPLAY_LEVEL = "merge_tag.user_display_level";
const std::string GALLERY_SCALE_X = "merge_face.scale_x";
const std::string GALLERY_SCALE_Y = "merge_face.scale_y";
const std::string GALLERY_SCALE_WIDTH = "merge_face.scale_width";
const std::string GALLERY_SCALE_HEIGHT = "merge_face.scale_height";
const std::string GALLERY_PITCH = "merge_face.pitch";
const std::string GALLERY_YAW = "merge_face.yaw";
const std::string GALLERY_ROLL = "merge_face.roll";
const std::string GALLERY_PROB = "merge_face.prob";
const std::string GALLERY_TOTAL_FACE = "merge_face.total_face";
const std::string GALLERY_MERGE_FACE_HASH = GALLERY_TABLE_MERGE_FACE + "." + GALLERY_HASH;
const std::string GALLERY_MERGE_FACE_TAG_ID = GALLERY_TABLE_MERGE_FACE + "." + GALLERY_TAG_ID;
const std::string GALLERY_MERGE_TAG_TAG_ID = GALLERY_TABLE_MERGE_TAG + "." + GALLERY_TAG_ID;
const std::string GALLERY_RELATIONSHIP = "merge_tag.relationship";
const std::string GALLERY_MEDIA_ID = GALLERY_TABLE_MEDIA + "." + GALLERY_ID;
const std::string GALLERY_MEDIA_HASH = GALLERY_TABLE_MEDIA + "." + GALLERY_HASH;
const std::string GALLERY_FACE_HASH = GALLERY_TABLE_FACE + "." + GALLERY_HASH;
const std::string GALLERY_MERGE_FACE_FACE_ID = GALLERY_TABLE_MERGE_FACE + "." + GALLERY_FACE_ID;
const std::string GALLERY_FACE_FACE_ID = GALLERY_TABLE_FACE + "." + GALLERY_FACE_ID;
const std::string E_VERSION = "-1";
const std::string TAG_ID_PREFIX = "ser_";
const std::string TAG_ID_UNPROCESSED = "-1";

/* AnalysisSearchIndex TBL COL_NAME */
const std::string ANALYSIS_SEARCH_INDEX_TABLE = "tab_analysis_search_index";
const std::string SEARCH_IDX_COL_ID = "id";                     // INTEGER
const std::string SEARCH_IDX_COL_FILE_ID = "file_id";           // INT
const std::string SEARCH_IDX_COL_DATA = "data";                 // TEXT
const std::string SEARCH_IDX_COL_DISPLAY_NAME = "display_name"; // TEXT
const std::string SEARCH_IDX_COL_LATITUDE = "latitude";         // DOUBLE
const std::string SEARCH_IDX_COL_LONGITUDE = "longitude";       // DOUBLE
const std::string SEARCH_IDX_COL_DATE_MODIFIED = "date_modified"; // BIGINT
const std::string SEARCH_IDX_COL_PHOTO_STATUS = "photo_status"; // INT
const std::string SEARCH_IDX_COL_CV_STATUS = "cv_status";       // INT
const std::string SEARCH_IDX_COL_GEO_STATUS = "geo_status";     // INT
const std::string SEARCH_IDX_COL_VERSION = "version";           // INT
const std::string SEARCH_IDX_COL_SYSTEM_LANGUAGE = "system_language"; // TEXT

/* AnalysisAlbum TBL COL_NAME */
const std::string ANALYSIS_COL_ALBUM_ID = "album_id";
const std::string ANALYSIS_COL_ALBUM_TYPE = "album_type";
const std::string ANALYSIS_COL_ALBUM_SUBTYPE = "album_subtype";
const std::string ANALYSIS_COL_ALBUM_NAME = "album_name";
const std::string ANALYSIS_COL_COVER_URI = "cover_uri";
const std::string ANALYSIS_COL_COUNT = "count";
const std::string ANALYSIS_COL_DATE_MODIFIED = "date_modified";
const std::string ANALYSIS_COL_RANK = "rank";
const std::string ANALYSIS_COL_TAG_ID = "tag_id";
const std::string ANALYSIS_COL_USER_OPERATION = "user_operation";
const std::string ANALYSIS_COL_GROUP_TAG = "group_tag";
const std::string ANALYSIS_COL_USER_DISPLAY_LEVEL = "user_display_level";
const std::string ANALYSIS_COL_IS_ME = "is_me";
const std::string ANALYSIS_COL_IS_REMOVED = "is_removed";
const std::string ANALYSIS_COL_RENAME_OPERATION = "rename_operation";
const std::string ANALYSIS_COL_IS_LOCAL = "is_local";
const std::string ANALYSIS_COL_IS_COVER_SATISFIED = "is_cover_satisfied";
const std::string ANALYSIS_COL_RELATIONSHIP = "relationship";
const std::string ANALYSIS_COL_EDIT_OPERATION = "edit_operation";

/* FaceTag TBL COL_NAME */
const std::string FACE_TAG_COL_ID = "id";
const std::string FACE_TAG_COL_TAG_ID = "tag_id";
const std::string FACE_TAG_COL_TAG_NAME = "tag_name";
const std::string FACE_TAG_COL_USER_OPERATION = "user_operation";
const std::string FACE_TAG_COL_GROUP_TAG = "group_tag";
const std::string FACE_TAG_COL_RENAME_OPERATION = "rename_operation";
const std::string FACE_TAG_COL_CENTER_FEATURES = "center_features";
const std::string FACE_TAG_COL_TAG_VERSION = "tag_version";
const std::string FACE_TAG_COL_USER_DISPLAY_LEVEL = "user_display_level";
const std::string FACE_TAG_COL_TAG_ORDER = "tag_order";
const std::string FACE_TAG_COL_IS_ME = "is_me";
const std::string FACE_TAG_COL_COVER_URI = "cover_uri";
const std::string FACE_TAG_COL_COUNT = "count";
const std::string FACE_TAG_COL_DATE_MODIFY = "date_modify";
const std::string FACE_TAG_COL_ALBUM_TYPE = "album_type";
const std::string FACE_TAG_COL_IS_REMOVED = "is_removed";
const std::string FACE_TAG_COL_ANALYSIS_VERSION = "analysis_version";
const std::string FACE_TAG_COL_AGE = "age";
const std::string FACE_TAG_COL_GENDER = "gender";

// image_face_tbl COL_NAME
const std::string IMAGE_FACE_COL_ID = "id";
const std::string IMAGE_FACE_COL_FILE_ID = "file_id";
const std::string IMAGE_FACE_COL_FACE_ID = "face_id";
const std::string IMAGE_FACE_COL_TAG_ID = "tag_id";
const std::string IMAGE_FACE_COL_SCALE_X = "scale_x";
const std::string IMAGE_FACE_COL_SCALE_Y = "scale_y";
const std::string IMAGE_FACE_COL_SCALE_WIDTH = "scale_width";
const std::string IMAGE_FACE_COL_SCALE_HEIGHT = "scale_height";
const std::string IMAGE_FACE_COL_LANDMARKS = "landmarks";
const std::string IMAGE_FACE_COL_PITCH = "pitch";
const std::string IMAGE_FACE_COL_YAW = "yaw";
const std::string IMAGE_FACE_COL_ROLL = "roll";
const std::string IMAGE_FACE_COL_PROB = "prob";
const std::string IMAGE_FACE_COL_TOTAL_FACES = "total_faces";
const std::string IMAGE_FACE_COL_FACE_VERSION = "face_version";
const std::string IMAGE_FACE_COL_FEATURES_VERSION = "features_version";
const std::string IMAGE_FACE_COL_FEATURES = "features";
const std::string IMAGE_FACE_COL_FACE_OCCLUSION = "face_occlusion";
const std::string IMAGE_FACE_COL_ANALYSIS_VERSION = "analysis_version";
const std::string IMAGE_FACE_COL_BEAUTY_BOUNDER_X = "beauty_bounder_x";
const std::string IMAGE_FACE_COL_BEAUTY_BOUNDER_Y = "beauty_bounder_y";
const std::string IMAGE_FACE_COL_BEAUTY_BOUNDER_WIDTH = "beauty_bounder_width";
const std::string IMAGE_FACE_COL_BEAUTY_BOUNDER_HEIGHT = "beauty_bounder_height";
const std::string IMAGE_FACE_COL_AESTHETICS_SCORE = "aesthetics_score";
const std::string IMAGE_FACE_COL_BEAUTY_BOUNDER_VERSION = "beauty_bounder_version";
const std::string IMAGE_FACE_COL_IS_EXCLUDED = "is_excluded";
const std::string ANALYSIS_ALBUM_SUBTYPE = "album_subtype";
const std::string IMAGE_FACE_COL_FACE_CLARITY = "face_clarity";
const std::string IMAGE_FACE_COL_FACE_LUMINANCE = "face_luminance";
const std::string IMAGE_FACE_COL_FACE_SATURATION = "face_saturation";
const std::string IMAGE_FACE_COL_FACE_EYE_CLOSE = "face_eye_close";
const std::string IMAGE_FACE_COL_FACE_DETAIL_VERSION = "face_detail_version";
const std::string IMAGE_FACE_COL_FACE_EXPRESSION = "face_expression";
const std::string IMAGE_FACE_COL_AGE = "age";
const std::string IMAGE_FACE_COL_GENDER = "gender";
const std::string IMAGE_FACE_COL_PREFERRED_GRADE = "preferred_grade";
const std::string IMAGE_FACE_COL_JOINT_BEAUTY_BOUNDER_X = "joint_beauty_bounder_x";
const std::string IMAGE_FACE_COL_JOINT_BEAUTY_BOUNDER_Y = "joint_beauty_bounder_y";
const std::string IMAGE_FACE_COL_JOINT_BEAUTY_BOUNDER_WIDTH = "joint_beauty_bounder_width";
const std::string IMAGE_FACE_COL_JOINT_BEAUTY_BOUNDER_HEIGHT = "joint_beauty_bounder_height";
const std::string IMAGE_FACE_COL_GROUP_VERSION = "group_version";


// video_face_tbl COL_NAME
const std::string ANALYSIS_VIDEO_FACE_TABLE = "tab_analysis_video_face";
const std::string VIDEO_FACE_COL_ID = "id";
const std::string VIDEO_FACE_COL_FILE_ID = "file_id";
const std::string VIDEO_FACE_COL_FACE_ID = "face_id";
const std::string VIDEO_FACE_COL_TAG_ID = "tag_id";
const std::string VIDEO_FACE_COL_SCALE_X = "scale_x";
const std::string VIDEO_FACE_COL_SCALE_Y = "scale_y";
const std::string VIDEO_FACE_COL_SCALE_WIDTH = "scale_width";
const std::string VIDEO_FACE_COL_SCALE_HEIGHT = "scale_height";
const std::string VIDEO_FACE_COL_LANDMARKS = "landmarks";
const std::string VIDEO_FACE_COL_PITCH = "pitch";
const std::string VIDEO_FACE_COL_YAW = "yaw";
const std::string VIDEO_FACE_COL_ROLL = "roll";
const std::string VIDEO_FACE_COL_PROB = "prob";
const std::string VIDEO_FACE_COL_TOTAL_FACES = "total_faces";
const std::string VIDEO_FACE_COL_FRAME_ID = "frame_id";
const std::string VIDEO_FACE_COL_FRAME_TIMESTAMP = "frame_timestamp";
const std::string VIDEO_FACE_COL_TRACKS = "tracks";
const std::string VIDEO_FACE_COL_ALGO_VERSION = "algo_version";
const std::string VIDEO_FACE_COL_FEATURES = "features";
const std::string VIDEO_FACE_COL_ANALYSIS_VERSION = "analysis_version";

// beauty_score_tbl COL_NAME
const std::string ANALYSIS_BEAUTY_SCORE_TABLE = "tab_analysis_aesthetics_score";
const std::string BEAUTY_SCORE_COL_ID = "id";
const std::string BEAUTY_SCORE_COL_FILE_ID = "file_id";
const std::string BEAUTY_SCORE_COL_AESTHETICS_SCORE = "aesthetics_score";
const std::string BEAUTY_SCORE_COL_AESTHETICS_VERSION = "aesthetics_version";
const std::string BEAUTY_SCORE_COL_PROB = "prob";
const std::string BEAUTY_SCORE_COL_ANALYSIS_VERSION = "analysis_version";
const std::string BEAUTY_SCORE_COL_SELECTED_FLAG = "selected_flag";
const std::string BEAUTY_SCORE_COL_SELECTED_ALGO_VERSION = "selected_algo_version";
const std::string BEAUTY_SCORE_COL_SELECTED_STATUS = "selected_status";
const std::string BEAUTY_SCORE_COL_NEGATIVE_FLAG = "negative_flag";
const std::string BEAUTY_SCORE_COL_NEGATIVE_ALGO_VERSION = "negative_algo_version";
const std::string BEAUTY_SCORE_COL_AESTHETICS_ALL_VERSION = "aesthetics_all_version";
const std::string BEAUTY_SCORE_COL_AESTHETICS_SCORE_ALL = "aesthetics_score_all";
const std::string BEAUTY_SCORE_COL_IS_FILTERED_HARD = "is_filtered_hard";
const std::string BEAUTY_SCORE_COL_CLARITY_SCORE_ALL = "clarity_score_all";
const std::string BEAUTY_SCORE_COL_SATURATION_SCORE_ALL = "saturation_score_all";
const std::string BEAUTY_SCORE_COL_LUMINANCE_SCORE_ALL = "luminance_score_all";
const std::string BEAUTY_SCORE_COL_SEMANTICS_SCORE = "semantics_score";
const std::string BEAUTY_SCORE_COL_IS_BLACK_WHITE_STRIPE = "is_black_white_stripe";
const std::string BEAUTY_SCORE_COL_IS_BLURRY = "is_blurry";
const std::string BEAUTY_SCORE_COL_IS_MOSAIC = "is_mosaic";

// Asset Map TBL COL_NAME
const std::string TAB_OLD_PHOTOS = "tab_old_photos";
const std::string ASSET_MAP_COL_FILE_ID = "file_id";
const std::string ASSET_MAP_COL_DATA = "data";
const std::string ASSET_MAP_COL_OLD_FILE_ID = "old_file_id";
const std::string ASSET_MAP_COL_OLD_DATA = "old_data";
const std::string ASSET_MAP_COL_CLONE_SEQUENCE = "clone_sequence";

//  analysis_pet_tag TBL COL_NAME
const std::string ANALYSIS_PET_TAG_TABLE = "tab_analysis_pet_tag";
const std::string PET_TAG_COL_ID = "id";
const std::string PET_TAG_COL_TAG_ID = "tag_id";
const std::string PET_TAG_COL_PET_LABEL = "pet_label";
const std::string PET_TAG_COL_CENTER_FEATURES = "center_features";
const std::string PET_TAG_COL_TAG_VERSION = "tag_version";
const std::string PET_TAG_COL_COUNT = "count";
const std::string PET_TAG_COL_DATE_MODIFIED = "date_modified";
const std::string PET_TAG_COL_ANALYSIS_VERSION = "analysis_version";

//  analysis_pet_face TBL COL_NAME
const std::string ANALYSIS_PET_FACE_TABLE = "tab_analysis_pet_face";
const std::string PET_FACE_COL_ID = "id";
const std::string PET_FACE_COL_FILE_ID = "file_id";
const std::string PET_FACE_COL_PET_ID = "pet_id";
const std::string PET_FACE_COL_PROB = "prob";
const std::string PET_FACE_COL_PET_LABEL = "pet_label";
const std::string PET_FACE_COL_PET_TOTAL_FACES = "pet_total_faces";
const std::string PET_FACE_COL_FEATURES = "features";
const std::string PET_FACE_COL_PET_TAG_ID = "pet_tag_id";
const std::string PET_FACE_COL_SCALE_X = "scale_x";
const std::string PET_FACE_COL_SCALE_Y = "scale_y";
const std::string PET_FACE_COL_SCALE_WIDTH = "scale_width";
const std::string PET_FACE_COL_SCALE_HEIGHT = "scale_height";
const std::string PET_FACE_COL_HEAD_VERSION = "head_version";
const std::string PET_FACE_COL_PET_FEATURE_VERSION = "feature_version";
const std::string PET_FACE_COL_TAG_VERSION = "tag_version";
const std::string PET_FACE_COL_ANALYSIS_VERSION = "analysis_version";
const std::string PET_FACE_COL_BEAUTY_BOUNDER_X = "beauty_bounder_x";
const std::string PET_FACE_COL_BEAUTY_BOUNDER_Y = "beauty_bounder_y";
const std::string PET_FACE_COL_BEAUTY_BOUNDER_WIDTH = "beauty_bounder_width";
const std::string PET_FACE_COL_BEAUTY_BOUNDER_HEIGHT = "beauty_bounder_height";
const std::string PET_FACE_COL_DATE_MODIFIED = "date_modified";

// Relationship
enum RelationshipIndex {
    INDEX_ME = 0,
    INDEX_WIFE = 2,
    INDEX_HUSBAND = 3,
    INDEX_DAD = 4,
    INDEX_MUM = 5,
    INDEX_WORKMATE = 6,
    INDEX_FRIEND = 7,
    INDEX_CLASSMATE = 8,
    INDEX_BESTIE = 9,
    INDEX_BOYFRIEND = 10,
    INDEX_GIRLFRIEND = 11,
    INDEX_FAMILY = 12,
    INDEX_GRANDFATHER = 13,
    INDEX_GRANDMOTHER = 14,
    INDEX_PATERNALGRANDFATHER = 15,
    INDEX_PATERNALGRANDMOTHER = 16,
    INDEX_BROTHER = 17,
    INDEX_SISTER = 18,
    INDEX_LITTLEBROTHER = 19,
    INDEX_LITTLESISTER = 20,
    INDEX_RELATIVE = 21,
    INDEX_OTHER = 22,
    INDEX_SON = 23,
    INDEX_DAUGHTER = 24
};

const std::unordered_map<std::string, std::string> RELATIONSHIP_MAP = {
    {std::to_string(INDEX_ME), "me"},
    {std::to_string(INDEX_SON), "son"},
    {std::to_string(INDEX_DAUGHTER), "daughter"},
    {std::to_string(INDEX_WIFE), "wife"},
    {std::to_string(INDEX_HUSBAND), "husband"},
    {std::to_string(INDEX_DAD), "father"},
    {std::to_string(INDEX_MUM), "mother"},
    {std::to_string(INDEX_WORKMATE), "colleague"},
    {std::to_string(INDEX_FRIEND), "friend"},
    {std::to_string(INDEX_CLASSMATE), "classmate"},
    {std::to_string(INDEX_BESTIE), "best_friend_female"},
    {std::to_string(INDEX_BOYFRIEND), "boyfriend"},
    {std::to_string(INDEX_GIRLFRIEND), "girlfriend"},
    {std::to_string(INDEX_FAMILY), "family"},
    {std::to_string(INDEX_GRANDFATHER), "maternal_grandfather"},
    {std::to_string(INDEX_GRANDMOTHER), "maternal_grandmother"},
    {std::to_string(INDEX_PATERNALGRANDFATHER), "paternal_grandfather"},
    {std::to_string(INDEX_PATERNALGRANDMOTHER), "paternal_grandmother"},
    {std::to_string(INDEX_BROTHER), "older_brother"},
    {std::to_string(INDEX_SISTER), "older_sister"},
    {std::to_string(INDEX_LITTLEBROTHER), "younger_brother"},
    {std::to_string(INDEX_LITTLESISTER), "younger_sister"},
    {std::to_string(INDEX_RELATIVE), "relative"},
    {std::to_string(INDEX_OTHER), "other"}
};

// Tab Old Albums TBL COL_NAME
const std::string TAB_OLD_ALBUMS = "tab_old_albums";
const std::string OLD_ALBUM_ID_COL = "old_album_id";
const std::string ALBUM_ID_COL = "album_id";
const std::string ALBUM_TYPE_COL = "album_type";
const std::string ALBUM_SUBTYPE_COL = "album_subtype";
const std::string ALBUM_CLONE_SEQUENCE_COL = "clone_sequence";

const std::string QUERY_FACE_TAG_COUNT = "SELECT count(1) AS count FROM " + VISION_FACE_TAG_TABLE;
const std::string QUERY_IMAGE_FACE_COUNT = "SELECT count(1) AS count FROM " + VISION_IMAGE_FACE_TABLE;
const std::string QUERY_VIDEO_FACE_COUNT = "SELECT count(1) AS count FROM " + VISION_VIDEO_FACE_TABLE;
const std::string QUERY_BEAUTY_SCORE_COUNT = "SELECT count(1) AS count FROM " + VISION_AESTHETICS_TABLE;
const std::string QUERY_PET_TAG_COUNT = "SELECT count(1) AS count FROM " + ANALYSIS_PET_TAG_TABLE;
const std::string QUERY_PET_FACE_COUNT = "SELECT count(1) AS count FROM " + ANALYSIS_PET_FACE_TABLE;

const std::string CREATE_FACE_TAG_INDEX =
    "CREATE INDEX IF NOT EXISTS face_clone_tag_index ON tab_analysis_face_tag (tag_id)";
const std::string CREATE_PET_TAG_INDEX =
    "CREATE INDEX IF NOT EXISTS pet_clone_tag_index ON tab_analysis_pet_tag (tag_id)";

const std::string GALLERY_TAG_NAME_NOT_NULL_OR_EMPTY = GALLERY_TAG_NAME + " IS NOT NULL AND " + GALLERY_TAG_NAME +
    " != ''";
const std::string GALLERY_TAG_WITH_PHOTOS = "EXISTS (SELECT 1 FROM " + GALLERY_TABLE_MERGE_FACE + " INNER JOIN \
    gallery_media ON " + GALLERY_MERGE_FACE_HASH + " = gallery_media.hash WHERE " + LOCAL_PHOTOS_WHERE_CLAUSE +
    " AND " + GALLERY_MERGE_FACE_TAG_ID + " = " + GALLERY_MERGE_TAG_TAG_ID + ")";

const std::string GALLERY_TAG_WITH_CLOUD_PHOTOS = "EXISTS (SELECT 1 FROM " + GALLERY_TABLE_MERGE_FACE + " INNER JOIN \
    gallery_media ON " + GALLERY_MERGE_FACE_HASH + " = gallery_media.hash WHERE " + ALL_PHOTOS_WHERE_CLAUSE + " AND " +
    GALLERY_MERGE_FACE_TAG_ID + " = " + GALLERY_MERGE_TAG_TAG_ID + ")";

const std::string GALLERY_PORTRAIT_ALBUM_TABLE = GALLERY_TABLE_MERGE_TAG + " WHERE " +
    GALLERY_TAG_WITH_PHOTOS;

const std::string GALLERY_PORTRAIT_ALBUM_TABLE_WITH_CLOUD = GALLERY_TABLE_MERGE_TAG + " WHERE " +
    GALLERY_TAG_WITH_CLOUD_PHOTOS;

const std::string QUERY_GALLERY_PORTRAIT_ALBUM_COUNT = "SELECT count(1) as count FROM " + GALLERY_PORTRAIT_ALBUM_TABLE;

const std::string QUERY_GALLERY_PORTRAIT_ALBUM_WITH_CLOUD_COUNT = "SELECT count(1) as count FROM " +
    GALLERY_PORTRAIT_ALBUM_TABLE_WITH_CLOUD;

// Path related
const std::string INTERNAL_PREFIX = "/storage/emulated";
constexpr int32_t INTERNAL_PREFIX_LEVEL = 4;
constexpr int32_t SD_PREFIX_LEVEL = 3;
const std::string APP_TWIN_DATA_PREFIX = "/AppTwinData";
const std::string CLONE_STAT_EDIT_DATA_DIR = "/storage/media/local/files/.editData/";
const std::string CLONE_STAT_RDB_DIR = "/data/storage/el2/database/rdb/";
const std::string CLONE_STAT_KVDB_DIR = "/data/storage/el2/database/kvdb/";
const std::string CLONE_STAT_HIGHLIGHT_DIR = "/storage/media/local/files/highlight/";
const std::string DEFAULT_PATH_PREFIX = "/storage";
} // namespace Media
} // namespace OHOS

#endif  // BACKUP_CONST_COLUMN_H