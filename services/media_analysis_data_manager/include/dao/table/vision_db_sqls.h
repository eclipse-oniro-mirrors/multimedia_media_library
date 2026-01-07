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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_VISION_DB_SQLS_H
#define FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_VISION_DB_SQLS_H

#include "media_analysis_progress_column.h"
#include "search_column.h"
#include "vision_aesthetics_score_column.h"
#include "vision_video_aesthetics_score_column.h"
#include "vision_album_column.h"
#include "vision_column_comm.h"
#include "vision_column.h"
#include "vision_composition_column.h"
#include "vision_face_tag_column.h"
#include "vision_head_column.h"
#include "vision_image_face_column.h"
#include "vision_video_face_column.h"
#include "vision_label_column.h"
#include "vision_object_column.h"
#include "vision_ocr_column.h"
#include "vision_affective_column.h"
#include "vision_pet_face_column.h"
#include "vision_photo_map_column.h"
#include "vision_pose_column.h"
#include "vision_recommendation_column.h"
#include "vision_saliency_detect_column.h"
#include "vision_segmentation_column.h"
#include "vision_total_column.h"
#include "vision_video_label_column.h"
#include "vision_multi_crop_column.h"

namespace OHOS {
namespace Media {
const std::string CREATE_TAB_ANALYSIS_OCR = "CREATE TABLE IF NOT EXISTS " + VISION_OCR_TABLE + " (" +
    ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    FILE_ID + " INT UNIQUE, " +
    OCR_TEXT + " TEXT, " +
    OCR_VERSION + " TEXT, " +
    OCR_TEXT_MSG + " TEXT, " +
    OCR_WIDTH + " INT, " +
    OCR_HEIGHT + " INT, " +
    OCR_CARD_TEXT + " TEXT, " +
    OCR_CARD_TEXT_MSG + " TEXT, " +
    ANALYSIS_VERSION + " TEXT)";

const std::string CREATE_TAB_ANALYSIS_LABEL = "CREATE TABLE IF NOT EXISTS " + VISION_LABEL_TABLE + " (" +
    ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    FILE_ID + " INT UNIQUE, " +
    CATEGORY_ID + " INT, " +
    SUB_LABEL + " TEXT, " +
    PROB + " REAL, " +
    FEATURE + " TEXT, " +
    SIM_RESULT + " TEXT, " +
    LABEL_VERSION + " TEXT, " +
    SALIENCY_SUB_PROB + " TEXT, " +
    ANALYSIS_VERSION + " TEXT, " +
    SEARCH_TAG_TYPE + " TEXT, " +
    SEARCH_TAG_VECTOR + " BLOB)";

const std::string CREATE_TAB_ANALYSIS_VIDEO_LABEL = "CREATE TABLE IF NOT EXISTS " + VISION_VIDEO_LABEL_TABLE + " (" +
    ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    FILE_ID + " INT, " +
    CATEGORY_ID + " TEXT, " +
    CONFIDENCE_PROBABILITY + " REAL, " +
    SUB_CATEGORY + " TEXT, " +
    SUB_CONFIDENCE_PROB + " REAL, " +
    SUB_LABEL + " TEXT, " +
    SUB_LABEL_PROB + " REAL, " +
    SUB_LABEL_TYPE + " TEXT, " +
    TRACKS + " TEXT, " +
    VIDEO_PART_FEATURE + " BLOB, " +
    FILTER_TAG + " TEXT, " +
    ALGO_VERSION + " TEXT, " +
    ANALYSIS_VERSION + " TEXT, " +
    TRIGGER_GENERATE_THUMBNAIL + " INT DEFAULT 0)";

const std::string CREATE_TAB_ANALYSIS_AESTHETICS = "CREATE TABLE IF NOT EXISTS " + VISION_AESTHETICS_TABLE + " (" +
    ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    FILE_ID + " INT UNIQUE, " +
    AESTHETICS_SCORE + " INT, " +
    AESTHETICS_VERSION + " TEXT, " +
    PROB + " REAL, " +
    ANALYSIS_VERSION + " TEXT, " +
    AESTHETICS_ALL_VERSION + " TEXT, " +
    AESTHETICS_SCORE_ALL + " INT NOT NULL DEFAULT 0, " +
    IS_FILTERED_HARD + " BOOLEAN NOT NULL DEFAULT 0, " +
    CLARITY_SCORE_ALL + " DOUBLE NOT NULL DEFAULT 0, " +
    SATURATION_SCORE_ALL + " DOUBLE NOT NULL DEFAULT 0, " +
    LUMINANCE_SCORE_ALL + " DOUBLE NOT NULL DEFAULT 0, " +
    SEMANTICS_SCORE + " DOUBLE NOT NULL DEFAULT 0, " +
    IS_BLACK_WHITE_STRIPE + " BOOLEAN NOT NULL DEFAULT 0, " +
    IS_BLURRY + " BOOLEAN NOT NULL DEFAULT 0, " +
    IS_MOSAIC + " BOOLEAN NOT NULL DEFAULT 0)";

const std::string CREATE_TAB_VIDEO_ANALYSIS_AESTHETICS = "CREATE TABLE IF NOT EXISTS " +
    VISION_VIDEO_AESTHETICS_TABLE + " (" +
    ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    FILE_ID + " INT UNIQUE, " +
    VIDEO_AESTHETICS_SCORE + " INT, " +
    VIDEO_AESTHETICS_VERSION + " TEXT, " +
    PROB + " REAL)";

const std::string CREATE_TAB_ANALYSIS_SALIENCY_DETECT = "CREATE TABLE IF NOT EXISTS " + VISION_SALIENCY_TABLE + " (" +
    ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    FILE_ID + " INT UNIQUE, " +
    SALIENCY_X + " REAL, " +
    SALIENCY_Y + " REAL, " +
    SALIENCY_VERSION + " TEXT, " +
    ANALYSIS_VERSION + " TEXT)";

const std::string CREATE_TAB_ANALYSIS_OBJECT = "CREATE TABLE IF NOT EXISTS " + VISION_OBJECT_TABLE + " (" +
    ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    FILE_ID + " INT, " +
    OBJECT_ID + " INT, " +
    OBJECT_LABEL + " INT, " +
    OBJECT_SCALE_X + " INT, " +
    OBJECT_SCALE_Y + " INT, " +
    OBJECT_SCALE_WIDTH + " INT, " +
    OBJECT_SCALE_HEIGHT + " INT, " +
    PROB + " REAL, " +
    OBJECT_VERSION + " TEXT, " +
    SCALE_X + " REAL DEFAULT 0, " +
    SCALE_Y + " REAL DEFAULT 0, " +
    SCALE_WIDTH + " REAL DEFAULT 0, " +
    SCALE_HEIGHT + " REAL DEFAULT 0, " +
    ANALYSIS_VERSION + " TEXT)";

const std::string CREATE_TAB_ANALYSIS_AFFECTIVE = "CREATE TABLE IF NOT EXISTS " + VISION_AFFECTIVE_TABLE +
    " (" +
    ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    FILE_ID + " INT, " +
    EMOTION_CATEGORY + " TEXT, " +
    VALENCE + " INT, " +
    AROUSAL + " INT, " +
    MODEL_VERSION + " TEXT, " +
    MODEL_NAME + " TEXT, " +
    EXTRA + " TEXT, " +
    CREATE_TIMESTAMP + " BIGINT, " +
    ANALYSIS_VERSION + " TEXT)";

const std::string CREATE_TAB_ANALYSIS_RECOMMENDATION = "CREATE TABLE IF NOT EXISTS " + VISION_RECOMMENDATION_TABLE +
    " (" +
    ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    FILE_ID + " INT, " +
    RECOMMENDATION_ID + " INT, " +
    RECOMMENDATION_RESOLUTION + " TEXT, " +
    RECOMMENDATION_SCALE_X + " INT, " +
    RECOMMENDATION_SCALE_Y + " INT, " +
    RECOMMENDATION_SCALE_WIDTH + " INT, " +
    RECOMMENDATION_SCALE_HEIGHT + " INT, " +
    RECOMMENDATION_VERSION + " TEXT, " +
    SCALE_X + " REAL DEFAULT 0, " +
    SCALE_Y + " REAL DEFAULT 0, " +
    SCALE_WIDTH + " REAL DEFAULT 0, " +
    SCALE_HEIGHT + " REAL DEFAULT 0, " +
    MOVEMENT_CROP + " TEXT, " +
    MOVEMENT_VERSION + " TEXT, " +
    ANALYSIS_VERSION + " TEXT)";

const std::string CREATE_TAB_ANALYSIS_SEGMENTATION = "CREATE TABLE IF NOT EXISTS " + VISION_SEGMENTATION_TABLE + " (" +
    ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    FILE_ID + " INT UNIQUE, " +
    SEGMENTATION_AREA + " TEXT, " +
    SEGMENTATION_NAME + " INT, " +
    PROB + " REAL, " +
    SEGMENTATION_VERSION + " TEXT, " +
    ANALYSIS_VERSION + " TEXT)";

const std::string CREATE_TAB_ANALYSIS_COMPOSITION = "CREATE TABLE IF NOT EXISTS " + VISION_COMPOSITION_TABLE + " (" +
    ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    FILE_ID + " INT, " +
    COMPOSITION_ID + " INT, " +
    COMPOSITION_RESOLUTION + " TEXT, " +
    CLOCK_STYLE + " INT, " +
    CLOCK_LOCATION_X + " INT, " +
    CLOCK_LOCATION_Y + " INT, " +
    CLOCK_COLOUR + " TEXT, " +
    COMPOSITION_SCALE_X + " INT, " +
    COMPOSITION_SCALE_Y + " INT, " +
    COMPOSITION_SCALE_WIDTH + " INT, " +
    COMPOSITION_SCALE_HEIGHT + " INT, " +
    COMPOSITION_VERSION + " TEXT, " +
    SCALE_X + " REAL DEFAULT 0, " +
    SCALE_Y + " REAL DEFAULT 0, " +
    SCALE_WIDTH + " REAL DEFAULT 0, " +
    SCALE_HEIGHT + " REAL DEFAULT 0, " +
    ANALYSIS_VERSION + " TEXT)";

const std::string CREATE_TAB_ANALYSIS_HEAD = "CREATE TABLE IF NOT EXISTS " + VISION_HEAD_TABLE + " (" +
    ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    FILE_ID + " INT, " +
    HEAD_ID + " INT, " +
    HEAD_LABEL + " INT, " +
    HEAD_SCALE_X + " INT, " +
    HEAD_SCALE_Y + " INT, " +
    HEAD_SCALE_WIDTH + " INT, " +
    HEAD_SCALE_HEIGHT + " INT, " +
    PROB + " REAL, " +
    HEAD_VERSION + " TEXT, " +
    SCALE_X + " REAL DEFAULT 0, " +
    SCALE_Y + " REAL DEFAULT 0, " +
    SCALE_WIDTH + " REAL DEFAULT 0, " +
    SCALE_HEIGHT + " REAL DEFAULT 0, " +
    ANALYSIS_VERSION + " TEXT)";

const std::string CREATE_TAB_ANALYSIS_POSE = "CREATE TABLE IF NOT EXISTS " + VISION_POSE_TABLE + " (" +
    ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    FILE_ID + " INT, " +
    POSE_ID + " INT, " +
    POSE_LANDMARKS + " BLOB, " +
    POSE_SCALE_X + " INT, " +
    POSE_SCALE_Y + " INT, " +
    POSE_SCALE_WIDTH + " INT, " +
    POSE_SCALE_HEIGHT + " INT, " +
    PROB + " REAL, " +
    POSE_VERSION + " TEXT, " +
    POSE_TYPE + " INT, " +
    SCALE_X + " REAL DEFAULT 0, " +
    SCALE_Y + " REAL DEFAULT 0, " +
    SCALE_WIDTH + " REAL DEFAULT 0, " +
    SCALE_HEIGHT + " REAL DEFAULT 0, " +
    ANALYSIS_VERSION + " TEXT)";

const std::string CREATE_TAB_ANALYSIS_TOTAL = "CREATE TABLE IF NOT EXISTS " + VISION_TOTAL_TABLE + " (" +
    ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    FILE_ID + " INT UNIQUE, " +
    STATUS + " INT, " +
    OCR + " INT, " +
    LABEL + " INT, " +
    AESTHETICS_SCORE + " INT) ";

const std::string CREATE_TAB_ANALYSIS_TOTAL_FOR_ONCREATE = "CREATE TABLE IF NOT EXISTS " + VISION_TOTAL_TABLE + " (" +
    ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    FILE_ID + " INT UNIQUE, " +
    STATUS + " INT, " +
    OCR + " INT, " +
    LABEL + " INT, " +
    AESTHETICS_SCORE + " INT, " +
    FACE + " INT, " +
    OBJECT + " INT, " +
    RECOMMENDATION + " INT, " +
    SEGMENTATION + " INT, " +
    COMPOSITION + " INT, " +
    SALIENCY + " INT, " +
    HEAD + " INT, " +
    POSE + " INT, " +
    GEO + " INT DEFAULT 0, " +
    AESTHETICS_SCORE_ALL_STATUS + " INT NOT NULL DEFAULT 0, " +
    PRIORITY + " INT NOT NULL DEFAULT 1, " +
    AFFECTIVE + " INT NOT NULL DEFAULT 0, " +
    PET_STATUS + " INT NOT NULL DEFAULT 0) ";

const std::string CREATE_TAB_ANALYSIS_VIDEO_TOTAL = "CREATE TABLE IF NOT EXISTS " +
    VISION_VIDEO_TOTAL_TABLE + " (" +
    FILE_ID + " INTEGER PRIMARY KEY, " +
    STATUS + " INT, " +
    LABEL + " INT, " +
    FACE + " INT) ";

const std::string CREATE_TAB_IMAGE_FACE = "CREATE TABLE IF NOT EXISTS " + VISION_IMAGE_FACE_TABLE + " (" +
    ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    FILE_ID + " INTEGER, " +
    FACE_ID + " TEXT, " +
    TAG_ID +  " TEXT, " +
    SCALE_X + " REAL, " +
    SCALE_Y + " REAL, " +
    SCALE_WIDTH + " REAL, " +
    SCALE_HEIGHT + " REAL, " +
    LANDMARKS + " BLOB, " +
    PITCH + " REAL, " +
    YAW + " REAL, " +
    ROLL + " REAL, " +
    PROB + " REAL, " +
    TOTAL_FACES + " INTEGER, " +
    IMAGE_FACE_VERSION + " TEXT, " +
    IMAGE_FEATURES_VERSION + " TEXT, " +
    FEATURES + " BLOB, " +
    FACE_OCCLUSION + " INT, " +
    ANALYSIS_VERSION + " TEXT, " +
    BEAUTY_BOUNDER_X + " REAL, " +
    BEAUTY_BOUNDER_Y + " REAL, " +
    BEAUTY_BOUNDER_WIDTH + " REAL, " +
    BEAUTY_BOUNDER_HEIGHT + " REAL, " +
    FACE_AESTHETICS_SCORE + " REAL, " +
    BEAUTY_BOUNDER_VERSION + " TEXT DEFAULT '', " +
    IS_EXCLUDED + " INT DEFAULT 0, " +
    IS_PRIMARY_FACE + " REAL, " +
    JOINT_BEAUTY_BOUNDER_X + " REAL, " +
    JOINT_BEAUTY_BOUNDER_Y + " REAL, " +
    JOINT_BEAUTY_BOUNDER_WIDTH + " REAL, " +
    JOINT_BEAUTY_BOUNDER_HEIGHT + " REAL, " +
    GROUP_VERSION + " TEXT, " +
    FACE_EYE_CLOSE + " REAL, " +
    FACE_DETAIL_VERSION + " TEXT, " +
    AGE + " DOUBLE, " +
    GENDER + " INTEGER )";

const std::string CREATE_TAB_VIDEO_FACE = "CREATE TABLE IF NOT EXISTS " + VISION_VIDEO_FACE_TABLE + " (" +
    ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    FILE_ID + " INTEGER, " +
    FACE_ID + " TEXT, " +
    TAG_ID +  " TEXT, " +
    SCALE_X + " BLOB, " +
    SCALE_Y + " BLOB, " +
    SCALE_WIDTH + " BLOB, " +
    SCALE_HEIGHT + " BLOB, " +
    LANDMARKS + " BLOB, " +
    PITCH + " BLOB, " +
    YAW + " BLOB, " +
    ROLL + " BLOB, " +
    PROB + " BLOB, " +
    TOTAL_FACES + " INTEGER, " +
    FRAME_ID + " BLOB, " +
    FRAME_TIMESTAMP + " BLOB, " +
    TRACKS + " TEXT, " +
    ALGO_VERSION + " TEXT, " +
    FEATURES + " BLOB, " +
    ANALYSIS_VERSION + " TEXT) ";

const std::string CREATE_TAB_FACE_TAG = "CREATE TABLE IF NOT EXISTS " + VISION_FACE_TAG_TABLE + " (" +
    ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    TAG_ID +  " TEXT UNIQUE, " +
    TAG_NAME +  " TEXT, " +
    USER_OPERATION + " INTEGER, " +
    GROUP_TAG +  " TEXT, " +
    RENAME_OPERATION + " INTEGER, " +
    CENTER_FEATURES + " BLOB, " +
    TAG_VERSION + " TEXT, " +
    USER_DISPLAY_LEVEL + " INTEGER, " +
    TAG_ORDER + " INTEGER, " +
    IS_ME + " INTEGER, " +
    COVER_URI +  " TEXT, " +
    COUNT + " INTEGER, " +
    PORTRAIT_DATE_MODIFY + " BIGINT, " +
    ALBUM_TYPE + " INTEGER, " +
    IS_REMOVED + " INTEGER, " +
    ANALYSIS_VERSION + " TEXT, " +
    AGE + " DOUBLE, " +
    GENDER + " INTEGER)";

const std::string CREATE_ANALYSIS_ALBUM = "CREATE TABLE IF NOT EXISTS " + ANALYSIS_ALBUM_TABLE + " (" +
    ALBUM_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    ALBUM_TYPE + " INT, " +
    ALBUM_SUBTYPE + " INT, " +
    ALBUM_NAME + " TEXT, " +
    COVER_URI + " TEXT, " +
    COUNT + " INT DEFAULT 0, " +
    DATE_MODIFIED + " BIGINT, " +
    RANK + " INT) ";

const std::string CREATE_ANALYSIS_ALBUM_MAP = "CREATE TABLE IF NOT EXISTS " + ANALYSIS_PHOTO_MAP_TABLE + " (" +
    MAP_ALBUM + " INT, " +
    MAP_ASSET + " INT, " +
    ORDER_POSITION + " INT, " +
    "PRIMARY KEY (" + MAP_ALBUM + "," + MAP_ASSET + ")) ";

const std::string CREATE_TAB_ANALYSIS_PET_FACE = "CREATE TABLE IF NOT EXISTS " + VISION_PET_FACE_TABLE + " (" +
    ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    FILE_ID + " INT, " +
    PET_ID + " INT, " +
    PET_TAG_ID + " TEXT, " +
    PET_TOTAL_FACES + " INT, " +
    PET_LABEL + " INT, " +
    FEATURES + " BLOB, " +
    PROB + " REAL, " +
    SCALE_X + " REAL DEFAULT 0, " +
    SCALE_Y + " REAL DEFAULT 0, " +
    SCALE_WIDTH + " REAL DEFAULT 0, " +
    SCALE_HEIGHT + " REAL DEFAULT 0, " +
    HEAD_VERSION + " TEXT, " +
    PET_FEATURE_VERSION + " TEXT, " +
    TAG_VERSION + " TEXT, " +
    ANALYSIS_VERSION + " TEXT, " +
    BEAUTY_BOUNDER_X + " REAL, " +
    BEAUTY_BOUNDER_Y + " REAL, " +
    BEAUTY_BOUNDER_WIDTH + " REAL, " +
    BEAUTY_BOUNDER_HEIGHT + " REAL, " +
    DATE_MODIFIED + " BIGINT) ";

const std::string CREATE_TAB_ANALYSIS_PET_TAG = "CREATE TABLE IF NOT EXISTS " + VISION_PET_TAG_TABLE + " (" +
    ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    TAG_ID + " TEXT UNIQUE, " +
    CENTER_FEATURES + " BLOB, " +
    PET_LABEL + " INTEGER, " +
    COUNT + " INTEGER, " +
    TAG_VERSION + " TEXT, " +
    ANALYSIS_VERSION + " TEXT, " +
    DATE_MODIFIED + " BIGINT)";

const std::string CREATE_TAB_ANALYSIS_ALBUM_TOTAL = "CREATE TABLE IF NOT EXISTS " +
    VISION_ANALYSIS_ALBUM_TOTAL_TABLE + " (" +
    ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    FILE_ID + " INT, " +
    STATUS + " INT DEFAULT 0, " +
    GEO + " INT DEFAULT 0, " +
    LABEL + " INT DEFAULT 0, " +
    FACE + " INT DEFAULT 0) ";

const std::string INIT_TAB_ANALYSIS_ALBUM_TOTAL = "INSERT INTO " + VISION_ANALYSIS_ALBUM_TOTAL_TABLE + " (" +
    FILE_ID + ", " +
    STATUS + ", " +
    GEO + ", " +
    LABEL + ", " +
    FACE + ") " +
    "SELECT " + FILE_ID +
    ", 0, 0, 0, 0" +
    " FROM " + VISION_TOTAL_TABLE;


const std::string INIT_TAB_ANALYSIS_TOTAL = "INSERT INTO " + VISION_TOTAL_TABLE + " (" +
    FILE_ID + ", " +
    STATUS + ", " +
    OCR + ", " +
    AESTHETICS_SCORE + ", " +
    LABEL + ") " +
    "SELECT " + FILE_ID +
    ", CASE WHEN date_trashed > 0 THEN 2 ELSE 0 END," +
    " 0," +
    " CASE WHEN subtype = 1 THEN -1 ELSE 0 END," +
    " CASE WHEN subtype = 1 THEN -1 ELSE 0 END," +
    " FROM " + PhotoColumn::PHOTOS_TABLE + " WHERE MEDIA_TYPE = 1";

// trigger
const std::string CREATE_VISION_DELETE_TRIGGER = "CREATE TRIGGER IF NOT EXISTS delete_vision_trigger AFTER DELETE ON " +
    PhotoColumn::PHOTOS_TABLE + " FOR EACH ROW " +
    " BEGIN " +
    " UPDATE " + VISION_TOTAL_TABLE +
    " SET " + STATUS + " = -1 " +
    " WHERE " + FILE_ID +
    " = OLD.file_id;" +
    " UPDATE " + VISION_VIDEO_TOTAL_TABLE +
    " SET " + STATUS + " = -1 " +
    " WHERE " + FILE_ID +
    " = OLD.file_id AND OLD.media_type = 2;" +
    " END;";

const std::string CREATE_TOTAL_INSERT_TRIGGER_FOR_ADD_ANALYSIS_ALBUM_TOTAL =
    "CREATE TRIGGER IF NOT EXISTS insert_vision_total_trigger AFTER INSERT ON " +
    VISION_TOTAL_TABLE + " FOR EACH ROW " +
    " BEGIN " +
   " INSERT INTO " + VISION_ANALYSIS_ALBUM_TOTAL_TABLE +" (" +
    FILE_ID + ", " + STATUS + ", " + GEO + ", " +
    LABEL + ", " +
    FACE + ") " +
    " VALUES (" +
    " NEW.file_id, 0, 0, 0, 0);" +
    " END;";

const std::string CREATE_VISION_UPDATE_TRIGGER_FOR_UPDATE_ANALYSIS_ALBUM_TOTAL_STATUS =
    "CREATE TRIGGER IF NOT EXISTS update_total_vision_trigger AFTER UPDATE OF status ON " +
    VISION_TOTAL_TABLE + " FOR EACH ROW " +
    " WHEN NEW.status = -1" +
    " BEGIN " +
    " UPDATE " + VISION_ANALYSIS_ALBUM_TOTAL_TABLE +
    " SET " + STATUS + " = -1" +
    " WHERE file_id = OLD.file_id;" +
    " END;";

const std::string CREATE_VISION_UPDATE_TRIGGER = "CREATE TRIGGER IF NOT EXISTS update_vision_trigger AFTER UPDATE ON " +
    PhotoColumn::PHOTOS_TABLE + " FOR EACH ROW " +
    " WHEN ((NEW.date_trashed > 0 AND OLD.date_trashed = 0)" +
    " OR (NEW.date_trashed = 0 AND OLD.date_trashed > 0))" +
    " AND (NEW.MEDIA_TYPE = 1 OR NEW.MEDIA_TYPE = 2)" +
    " BEGIN " +
    " UPDATE " + VISION_TOTAL_TABLE +
    " SET " + STATUS + " = " +
    " (CASE WHEN NEW.date_trashed > 0 THEN 2 ELSE 0 END)" +
    " WHERE file_id = OLD.file_id;" +
    " UPDATE " + VISION_VIDEO_TOTAL_TABLE +
    " SET " + STATUS + " = " +
    " (CASE WHEN NEW.date_trashed > 0 THEN 2 ELSE 0 END)" +
    " WHERE file_id = OLD.file_id AND OLD.media_type = 2;" +
    " END;";

const std::string CREATE_VISION_UPDATE_TRIGGER_FOR_ADD_VIDEO_LABEL =
    "CREATE TRIGGER IF NOT EXISTS update_vision_trigger AFTER UPDATE ON " +
    PhotoColumn::PHOTOS_TABLE + " FOR EACH ROW " +
    " WHEN ((NEW.date_trashed > 0 AND OLD.date_trashed = 0)" +
    " OR (NEW.date_trashed = 0 AND OLD.date_trashed > 0))" +
    " AND (NEW.MEDIA_TYPE = 1 OR NEW.MEDIA_TYPE = 2)" +
    " BEGIN " +
    " UPDATE " + VISION_TOTAL_TABLE +
    " SET " + STATUS + " = " +
    " (CASE WHEN NEW.date_trashed > 0 THEN 2 ELSE 0 END)" +
    " WHERE file_id = OLD.file_id;" +
    " END;";

const std::string CREATE_VISION_INSERT_TRIGGER = "CREATE TRIGGER IF NOT EXISTS insert_vision_trigger AFTER INSERT ON " +
    PhotoColumn::PHOTOS_TABLE + " FOR EACH ROW " +
    " WHEN NEW.MEDIA_TYPE = 1" +
    " BEGIN " +
    " INSERT INTO " + VISION_TOTAL_TABLE +" (" +
    FILE_ID + ", " + STATUS + ", " + OCR + ", " +
    AESTHETICS_SCORE + ", " +
    LABEL + ") " +
    " VALUES (" +
    " NEW.file_id, 0, 0," +
    " (CASE WHEN NEW.subtype = 1 THEN -1 ELSE 0 END)," +
    " (CASE WHEN NEW.subtype = 1 THEN -1 ELSE 0 END));" +
    " END;";

const std::string CREATE_VISION_INSERT_TRIGGER_FOR_ONCREATE =
    "CREATE TRIGGER IF NOT EXISTS insert_vision_trigger AFTER INSERT ON " +
    PhotoColumn::PHOTOS_TABLE + " FOR EACH ROW " +
    " WHEN (NEW.MEDIA_TYPE = 1 OR NEW.MEDIA_TYPE = 2)" +
    " AND NEW." + PhotoColumn::PHOTO_FILE_SOURCE_TYPE + " <> " +
    std::to_string(static_cast<int32_t>(FileSourceTypes::TEMP_FILE_MANAGER)) +
    " BEGIN " +
    " INSERT INTO " + VISION_TOTAL_TABLE +" (" + FILE_ID + ", " + STATUS + ", " + OCR + ", " + AESTHETICS_SCORE + ", " +
    LABEL + ", " + FACE + ", " + OBJECT + ", " + RECOMMENDATION + ", " + SEGMENTATION + ", " + COMPOSITION + "," +
    SALIENCY + ", " + HEAD + ", " + POSE + ") " + " VALUES ( NEW.file_id, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 );" +
    " INSERT INTO " + VISION_VIDEO_TOTAL_TABLE + " (" + FILE_ID + ", " + STATUS + ", " + LABEL + ", " + FACE +
    ") SELECT NEW.file_id, 0, 0, 0 WHERE NEW.MEDIA_TYPE = 2;" +
    " END;";

const std::string DROP_INSERT_VISION_TRIGGER = "DROP TRIGGER IF EXISTS insert_vision_trigger";
const std::string DROP_UPDATE_VISION_TRIGGER = "DROP TRIGGER IF EXISTS update_vision_trigger";
const std::string DROP_DELETE_VISION_TRIGGER = "DROP TRIGGER IF EXISTS delete_vision_trigger";

const std::string CREATE_VISION_INSERT_TRIGGER_FOR_ADD_VIDEO_LABEL =
    "CREATE TRIGGER IF NOT EXISTS insert_vision_trigger AFTER INSERT ON " +
    PhotoColumn::PHOTOS_TABLE + " FOR EACH ROW " +
    " WHEN (NEW.MEDIA_TYPE = 1 OR NEW.MEDIA_TYPE = 2)" +
    " BEGIN " +
    " INSERT INTO " + VISION_TOTAL_TABLE +" (" + FILE_ID + ", " + STATUS + ", " + OCR + ", " + AESTHETICS_SCORE + ", " +
    LABEL + ", " + FACE + ", " + OBJECT + ", " + RECOMMENDATION + ", " + SEGMENTATION + ", " + COMPOSITION + "," +
    SALIENCY + ", " + HEAD + ", " + POSE + ") " +
    " VALUES (" + " NEW.file_id, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 );" + " END;";

const std::string CREATE_INSERT_VISION_TRIGGER_FOR_ADD_FACE =
    "CREATE TRIGGER IF NOT EXISTS insert_vision_trigger AFTER INSERT ON " +
    PhotoColumn::PHOTOS_TABLE + " FOR EACH ROW " +
    " WHEN NEW.MEDIA_TYPE = 1" +
    " BEGIN " +
    " INSERT INTO " + VISION_TOTAL_TABLE +" (" +
    FILE_ID + ", " + STATUS + ", " + OCR + ", " +
    AESTHETICS_SCORE + ", " +
    LABEL + ", " +
    FACE + ") " +
    " VALUES (" +
    " NEW.file_id, 0, 0," +
    " (CASE WHEN NEW.subtype = 1 THEN -1 ELSE 0 END)," +
    " (CASE WHEN NEW.subtype = 1 THEN -1 ELSE 0 END)," +
    " (CASE WHEN NEW.subtype = 1 THEN -1 ELSE 0 END));" +
    " END;";

const std::string CREATE_VISION_INSERT_TRIGGER_FOR_ADD_AC =
    "CREATE TRIGGER IF NOT EXISTS insert_vision_trigger AFTER INSERT ON " +
    PhotoColumn::PHOTOS_TABLE + " FOR EACH ROW " +
    " WHEN NEW.MEDIA_TYPE = 1" +
    " BEGIN " +
    " INSERT INTO " + VISION_TOTAL_TABLE +" (" +
    FILE_ID + ", " + STATUS + ", " + OCR + ", " +
    AESTHETICS_SCORE + ", " +
    LABEL + ", " +
    FACE + ", " +
    OBJECT + ", " +
    RECOMMENDATION + ", " +
    SEGMENTATION + ", " +
    COMPOSITION + ")" +
    " VALUES (" +
    " NEW.file_id, 0, 0," +
    " (CASE WHEN NEW.subtype = 1 THEN -1 ELSE 0 END)," +
    " (CASE WHEN NEW.subtype = 1 THEN -1 ELSE 0 END)," +
    " (CASE WHEN NEW.subtype = 1 THEN -1 ELSE 0 END)," +
    " (CASE WHEN NEW.subtype = 1 THEN -1 ELSE 0 END)," +
    " (CASE WHEN NEW.subtype = 1 THEN -1 ELSE 0 END)," +
    " (CASE WHEN NEW.subtype = 1 THEN -1 ELSE 0 END)," +
    " (CASE WHEN NEW.subtype = 1 THEN -1 ELSE 0 END));" +
    " END;";

const std::string CREATE_VISION_INSERT_TRIGGER_FOR_ADD_SALIENCY =
    "CREATE TRIGGER IF NOT EXISTS insert_vision_trigger AFTER INSERT ON " +
    PhotoColumn::PHOTOS_TABLE + " FOR EACH ROW " +
    " WHEN NEW.MEDIA_TYPE = 1" +
    " BEGIN " +
    " INSERT INTO " + VISION_TOTAL_TABLE +" (" +
    FILE_ID + ", " + STATUS + ", " + OCR + ", " +
    AESTHETICS_SCORE + ", " +
    LABEL + ", " +
    FACE + ", " +
    OBJECT + ", " +
    RECOMMENDATION + ", " +
    SEGMENTATION + ", " +
    COMPOSITION + "," +
    SALIENCY + ") " +
    " VALUES (" +
    " NEW.file_id, 0, 0," +
    " (CASE WHEN NEW.subtype = 1 THEN -1 ELSE 0 END)," +
    " (CASE WHEN NEW.subtype = 1 THEN -1 ELSE 0 END)," +
    " (CASE WHEN NEW.subtype = 1 THEN -1 ELSE 0 END)," +
    " (CASE WHEN NEW.subtype = 1 THEN -1 ELSE 0 END)," +
    " (CASE WHEN NEW.subtype = 1 THEN -1 ELSE 0 END)," +
    " (CASE WHEN NEW.subtype = 1 THEN -1 ELSE 0 END)," +
    " (CASE WHEN NEW.subtype = 1 THEN -1 ELSE 0 END)," +
    " 0 );" +
    " END;";

const std::string CREATE_VISION_INSERT_TRIGGER_FOR_UPDATE_SPEC =
    "CREATE TRIGGER IF NOT EXISTS insert_vision_trigger AFTER INSERT ON " +
    PhotoColumn::PHOTOS_TABLE + " FOR EACH ROW " +
    " WHEN NEW.MEDIA_TYPE = 1" +
    " BEGIN " +
    " INSERT INTO " + VISION_TOTAL_TABLE +" (" + FILE_ID + ", " + STATUS + ", " + OCR + ", " + AESTHETICS_SCORE + ", " +
    LABEL + ", " + FACE + ", " + OBJECT + ", " + RECOMMENDATION + ", " + SEGMENTATION + ", " + COMPOSITION + "," +
    SALIENCY + ") " +
    " VALUES (" + " NEW.file_id, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 );" + " END;";

const std::string CREATE_VISION_INSERT_TRIGGER_FOR_ADD_HEAD_AND_POSE =
    "CREATE TRIGGER IF NOT EXISTS insert_vision_trigger AFTER INSERT ON " +
    PhotoColumn::PHOTOS_TABLE + " FOR EACH ROW " +
    " WHEN NEW.MEDIA_TYPE = 1" +
    " BEGIN " +
    " INSERT INTO " + VISION_TOTAL_TABLE +" (" + FILE_ID + ", " + STATUS + ", " + OCR + ", " + AESTHETICS_SCORE + ", " +
    LABEL + ", " + FACE + ", " + OBJECT + ", " + RECOMMENDATION + ", " + SEGMENTATION + ", " + COMPOSITION + "," +
    SALIENCY + ", " + HEAD + ", " + POSE + ") " +
    " VALUES (" + " NEW.file_id, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 );" + " END;";

const std::string ADD_FACE_STATUS_COLUMN = "ALTER TABLE " + VISION_TOTAL_TABLE + " ADD COLUMN " + FACE + " INT";
const std::string UPDATE_TOTAL_VALUE = "UPDATE " + VISION_TOTAL_TABLE + " SET " + STATUS + " = 0, " + FACE +
    " = 0 WHERE " + FILE_ID + " IN (SELECT " + FILE_ID + " FROM " + PhotoColumn::PHOTOS_TABLE + " WHERE subtype != 1)";
const std::string UPDATE_NOT_SUPPORT_VALUE = "UPDATE " + VISION_TOTAL_TABLE + " SET " + FACE +
    " = -1 WHERE " + FACE + " IS NULL";
const std::string IMAGE_FACE_INDEX = "image_face_index";
const std::string CREATE_IMAGE_FACE_INDEX = "CREATE UNIQUE INDEX IF NOT EXISTS " + IMAGE_FACE_INDEX + " ON " +
    VISION_IMAGE_FACE_TABLE + " (" + FILE_ID + "," + FACE_ID + ")";

const std::string IMAGE_FACE_TAG_ID_INDEX = "tag_id_index";
const std::string CREATE_IMAGE_FACE_TAG_ID_INDEX = "CREATE INDEX IF NOT EXISTS " + IMAGE_FACE_TAG_ID_INDEX +
    " ON " + VISION_IMAGE_FACE_TABLE + " (" + TAG_ID + ")" + " WHERE " + TAG_ID + " LIKE " + "'ser%'";

const std::string VIDEO_FACE_INDEX = "video_face_index";
const std::string CREATE_VIDEO_FACE_INDEX = "CREATE UNIQUE INDEX IF NOT EXISTS " + VIDEO_FACE_INDEX + " ON " +
    VISION_VIDEO_FACE_TABLE + " (" + FILE_ID + "," + FACE_ID + ")";

const std::string VIDEO_FACE_TAG_ID_INDEX = "video_face_tag_id_index";
const std::string CREATE_VIDEO_FACE_TAG_ID_INDEX = "CREATE INDEX IF NOT EXISTS " + VIDEO_FACE_TAG_ID_INDEX +
    " ON " + VISION_VIDEO_FACE_TABLE + " (" + TAG_ID + ")" + " WHERE " + TAG_ID + " LIKE " + "'ser%'";

const std::string ADD_SALIENCY_STATUS_COLUMN = "ALTER TABLE " + VISION_TOTAL_TABLE + " ADD COLUMN " + SALIENCY + " INT";
const std::string UPDATE_SALIENCY_TOTAL_VALUE = "UPDATE " + VISION_TOTAL_TABLE +
    " SET " + STATUS + " = 0, " + SALIENCY + " = 0 WHERE " +
    FILE_ID + " IN (SELECT " + FILE_ID + " FROM " + PhotoColumn::PHOTOS_TABLE + " WHERE media_type = 1)";
const std::string UPDATE_SALIENCY_NOT_SUPPORT_VALUE = "UPDATE " + VISION_TOTAL_TABLE + " SET " + SALIENCY +
    " = -1 WHERE " + SALIENCY + " IS NULL";

const std::string AC_ADD_OBJECT_COLUMN_FOR_TOTAL = "ALTER TABLE " + VISION_TOTAL_TABLE + " ADD COLUMN " +
    OBJECT + " INT";
const std::string AC_UPDATE_OBJECT_TOTAL_VALUE = "UPDATE " + VISION_TOTAL_TABLE + " SET " + STATUS + " = 0, " + OBJECT +
    " = 0 WHERE " + FILE_ID + " IN (SELECT " + FILE_ID + " FROM " + PhotoColumn::PHOTOS_TABLE + " WHERE subtype != 1)";
const std::string AC_UPDATE_OBJECT_TOTAL_NOT_SUPPORT_VALUE = "UPDATE " + VISION_TOTAL_TABLE + " SET " + OBJECT +
    " = -1 WHERE " + OBJECT + " IS NULL";
const std::string OBJECT_INDEX = "object_index";
const std::string CREATE_OBJECT_INDEX = "CREATE UNIQUE INDEX IF NOT EXISTS " + OBJECT_INDEX + " ON " +
    VISION_OBJECT_TABLE + " (" + FILE_ID + "," + OBJECT_ID + ")";

const std::string AC_ADD_RECOMMENDATION_COLUMN_FOR_TOTAL = "ALTER TABLE " + VISION_TOTAL_TABLE + " ADD COLUMN " +
    RECOMMENDATION + " INT";
const std::string AC_UPDATE_RECOMMENDATION_TOTAL_VALUE = "UPDATE " + VISION_TOTAL_TABLE + " SET " + STATUS + " = 0, " +
    RECOMMENDATION + " = 0 WHERE " + FILE_ID + " IN (SELECT " + FILE_ID + " FROM " + PhotoColumn::PHOTOS_TABLE +
    " WHERE subtype != 1)";
const std::string AC_UPDATE_RECOMMENDATION_TOTAL_NOT_SUPPORT_VALUE = "UPDATE " + VISION_TOTAL_TABLE + " SET " +
    RECOMMENDATION + " = -1 WHERE " + RECOMMENDATION + " IS NULL";
const std::string RECOMMENDATION_INDEX = "recommendation_index";
const std::string CREATE_RECOMMENDATION_INDEX = "CREATE UNIQUE INDEX IF NOT EXISTS " + RECOMMENDATION_INDEX + " ON " +
    VISION_RECOMMENDATION_TABLE + " (" + FILE_ID + "," + RECOMMENDATION_ID + ")";

const std::string AC_ADD_SEGMENTATION_COLUMN_FOR_TOTAL = "ALTER TABLE " + VISION_TOTAL_TABLE + " ADD COLUMN " +
    SEGMENTATION + " INT";
const std::string AC_UPDATE_SEGMENTATION_TOTAL_VALUE = "UPDATE " + VISION_TOTAL_TABLE + " SET " + STATUS + " = 0, " +
    SEGMENTATION + " = 0 WHERE " + FILE_ID + " IN (SELECT " + FILE_ID + " FROM " + PhotoColumn::PHOTOS_TABLE +
    " WHERE subtype != 1)";
const std::string AC_UPDATE_SEGMENTATION_TOTAL_NOT_SUPPORT_VALUE = "UPDATE " + VISION_TOTAL_TABLE + " SET " +
    SEGMENTATION + " = -1 WHERE " + SEGMENTATION + " IS NULL";

const std::string AC_ADD_COMPOSITION_COLUMN_FOR_TOTAL = "ALTER TABLE " + VISION_TOTAL_TABLE + " ADD COLUMN " +
    COMPOSITION + " INT";
const std::string AC_UPDATE_COMPOSITION_TOTAL_VALUE = "UPDATE " + VISION_TOTAL_TABLE + " SET " + STATUS + " = 0, " +
    COMPOSITION + " = 0 WHERE " + FILE_ID + " IN (SELECT " + FILE_ID + " FROM " + PhotoColumn::PHOTOS_TABLE +
    " WHERE subtype != 1)";
const std::string AC_UPDATE_COMPOSITION_TOTAL_NOT_SUPPORT_VALUE = "UPDATE " + VISION_TOTAL_TABLE + " SET " +
    COMPOSITION + " = -1 WHERE " + COMPOSITION + " IS NULL";
const std::string COMPOSITION_INDEX = "composition_index";
const std::string CREATE_COMPOSITION_INDEX = "CREATE UNIQUE INDEX IF NOT EXISTS " + COMPOSITION_INDEX + " ON " +
    VISION_COMPOSITION_TABLE + " (" + FILE_ID + "," + COMPOSITION_ID + ")";

const std::string ADD_HEAD_STATUS_COLUMN = "ALTER TABLE " + VISION_TOTAL_TABLE + " ADD COLUMN " + HEAD + " INT";
const std::string UPDATE_HEAD_TOTAL_VALUE = "UPDATE " + VISION_TOTAL_TABLE + " SET " + STATUS + " = 0, " + HEAD +
    " = 0 WHERE " + FILE_ID + " IN (SELECT " + FILE_ID + " FROM " + PhotoColumn::PHOTOS_TABLE +
    " WHERE media_type = 1)";
const std::string UPDATE_HEAD_NOT_SUPPORT_VALUE = "UPDATE " + VISION_TOTAL_TABLE + " SET " + HEAD + " = -1 WHERE " +
    HEAD + " IS NULL";
const std::string HEAD_INDEX = "head_index";
const std::string CREATE_HEAD_INDEX = "CREATE UNIQUE INDEX IF NOT EXISTS " + HEAD_INDEX + " ON " + VISION_HEAD_TABLE +
    " (" + FILE_ID + "," + HEAD_ID + ")";

const std::string UPDATE_VIDEO_LABEL_TOTAL_VALUE = "UPDATE " + VISION_TOTAL_TABLE + " SET " + STATUS + " = 0, " +
    LABEL + " = 0 WHERE " + FILE_ID + " IN (SELECT " + FILE_ID + " FROM " + PhotoColumn::PHOTOS_TABLE +
    " WHERE media_type = 2)";
const std::string UPDATE_SEARCH_INDEX_FOR_VIDEO = "UPDATE " + SEARCH_TOTAL_TABLE + " SET " + TBL_SEARCH_PHOTO_STATUS +
    " = 2, " + TBL_SEARCH_CV_STATUS + " = 0 WHERE " + FILE_ID + " IN (SELECT " + FILE_ID + " FROM " +
    PhotoColumn::PHOTOS_TABLE + " WHERE media_type = 2)";
const std::string MAP_ASSET_INDEX = "map_asset_index";
const std::string CREATE_ANALYSIS_PHOTO_MAP_MAP_ASSET_INDEX = "CREATE INDEX IF NOT EXISTS " + MAP_ASSET_INDEX + " ON " +
    ANALYSIS_PHOTO_MAP_TABLE + " (" + MAP_ASSET + ")";
const std::string CREATE_TAB_ANALYSIS_PROGRESS = "CREATE TABLE IF NOT EXISTS " + TAB_ANALYSIS_PROGRESS_TABLE + " (" +
    SEARCH_FINISH_CNT + " INT NOT NULL DEFAULT 0, " +
    LOCATION_FINISH_CNT + " INT NOT NULL DEFAULT 0, " +
    FACE_FINISH_CNT + " INT NOT NULL DEFAULT 0, " +
    OBJECT_FINISH_CNT + " INT NOT NULL DEFAULT 0, " +
    AESTHETIC_FINISH_CNT + " INT NOT NULL DEFAULT 0, " +
    OCR_FINISH_CNT + " INT NOT NULL DEFAULT 0, " +
    POSE_FINISH_CNT + " INT NOT NULL DEFAULT 0, " +
    SALIENCY_FINISH_CNT + " INT NOT NULL DEFAULT 0, " +
    RECOMMENDATION_FINISH_CNT + " INT NOT NULL DEFAULT 0, " +
    SEGMENTATION_FINISH_CNT + " INT NOT NULL DEFAULT 0, " +
    BEAUTY_AESTHETIC_FINISH_CNT + " INT NOT NULL DEFAULT 0, " +
    HEAD_DETECT_FINISH_CNT + " INT NOT NULL DEFAULT 0, " +
    LABEL_DETECT_FINISH_CNT + " INT NOT NULL DEFAULT 0, " +
    TOTAL_IMAGE_CNT + " INT NOT NULL DEFAULT 0, " +
    FULLY_ANALYZED_IMAGE_CNT + " INT NOT NULL DEFAULT 0, " +
    TOTAL_PROGRESS + " INT NOT NULL DEFAULT 0, " +
    CREATED_TIME + " BIGINT NOT NULL DEFAULT 0, " +
    UPDATED_TIME + " BIGINT NOT NULL DEFAULT 0, " +
    EXTRA_QUOTA_INDEX_BUILD_CNT + " INT NOT NULL DEFAULT 0, " +
    EXTRA_QUOTA_INDEX_UPDATE_CNT + " INT NOT NULL DEFAULT 0, " +
    EXTRA_QUOTA_INDEX_DELETE_CNT + " INT NOT NULL DEFAULT 0, " +
    EXTRA_QUOTA_OCR_CNT + " INT NOT NULL DEFAULT 0, " +
    EXTRA_QUOTA_SHARED_BACKBONE_CNT + " INT NOT NULL DEFAULT 0, " +
    EXTRA_QUOTA_MODIFY_TIME + " BIGINT NOT NULL DEFAULT 0, " +
    BASE_QUOTA_INDEX_BUILD_CNT + " INT NOT NULL DEFAULT 0, " +
    BASE_QUOTA_INDEX_UPDATE_CNT + " INT NOT NULL DEFAULT 0, " +
    BASE_QUOTA_INDEX_DELETE_CNT + " INT NOT NULL DEFAULT 0, " +
    BASE_QUOTA_OCR_CNT + " INT NOT NULL DEFAULT 0, " +
    BASE_QUOTA_SHARED_BACKBONE_CNT + " INT NOT NULL DEFAULT 0, " +
    BASE_QUOTA_LABEL_CNT + " INT NOT NULL DEFAULT 0, " +
    BASE_QUOTA_MODIFY_TIME + " BIGINT NOT NULL DEFAULT 0, " +
    CHECK_SPACE_FLAG + " INT NOT NULL DEFAULT 1)";

const std::string ADD_EXTRA_QUOTA_INDEX_BUILD_CNT_COLUMN = "ALTER TABLE " + TAB_ANALYSIS_PROGRESS_TABLE +
    " ADD COLUMN " + EXTRA_QUOTA_INDEX_BUILD_CNT + " INT NOT NULL DEFAULT 0";
const std::string ADD_EXTRA_QUOTA_INDEX_UPDATE_CNT_COLUMN = "ALTER TABLE " + TAB_ANALYSIS_PROGRESS_TABLE +
    " ADD COLUMN " + EXTRA_QUOTA_INDEX_UPDATE_CNT + " INT NOT NULL DEFAULT 0";
const std::string ADD_EXTRA_QUOTA_INDEX_DELETE_CNT_COLUMN = "ALTER TABLE " + TAB_ANALYSIS_PROGRESS_TABLE +
    " ADD COLUMN " + EXTRA_QUOTA_INDEX_DELETE_CNT + " INT NOT NULL DEFAULT 0";
const std::string ADD_EXTRA_QUOTA_OCR_CNT_COLUMN = "ALTER TABLE " + TAB_ANALYSIS_PROGRESS_TABLE +
    " ADD COLUMN " + EXTRA_QUOTA_OCR_CNT + " INT NOT NULL DEFAULT 0";
const std::string ADD_EXTRA_QUOTA_SHARED_BACKBONE_CNT_COLUMN = "ALTER TABLE " + TAB_ANALYSIS_PROGRESS_TABLE +
    " ADD COLUMN " + EXTRA_QUOTA_SHARED_BACKBONE_CNT + " INT NOT NULL DEFAULT 0";
const std::string ADD_EXTRA_QUOTA_MODIFY_TIME_COLUMN = "ALTER TABLE " + TAB_ANALYSIS_PROGRESS_TABLE +
    " ADD COLUMN " + EXTRA_QUOTA_MODIFY_TIME + " BIGINT NOT NULL DEFAULT 0";

const std::string ADD_BASE_QUOTA_INDEX_BUILD_CNT_COLUMN = "ALTER TABLE " + TAB_ANALYSIS_PROGRESS_TABLE +
    " ADD COLUMN " + BASE_QUOTA_INDEX_BUILD_CNT + " INT NOT NULL DEFAULT 0";
const std::string ADD_BASE_QUOTA_INDEX_UPDATE_CNT_COLUMN = "ALTER TABLE " + TAB_ANALYSIS_PROGRESS_TABLE +
    " ADD COLUMN " + BASE_QUOTA_INDEX_UPDATE_CNT + " INT NOT NULL DEFAULT 0";
const std::string ADD_BASE_QUOTA_INDEX_DELETE_CNT_COLUMN = "ALTER TABLE " + TAB_ANALYSIS_PROGRESS_TABLE +
    " ADD COLUMN " + BASE_QUOTA_INDEX_DELETE_CNT + " INT NOT NULL DEFAULT 0";
const std::string ADD_BASE_QUOTA_OCR_CNT_COLUMN = "ALTER TABLE " + TAB_ANALYSIS_PROGRESS_TABLE +
    " ADD COLUMN " + BASE_QUOTA_OCR_CNT + " INT NOT NULL DEFAULT 0";
const std::string ADD_BASE_QUOTA_SHARED_BACKBONE_CNT_COLUMN = "ALTER TABLE " + TAB_ANALYSIS_PROGRESS_TABLE +
    " ADD COLUMN " + BASE_QUOTA_SHARED_BACKBONE_CNT + " INT NOT NULL DEFAULT 0";
const std::string ADD_BASE_QUOTA_LABEL_CNT_COLUMN = "ALTER TABLE " + TAB_ANALYSIS_PROGRESS_TABLE +
    " ADD COLUMN " + BASE_QUOTA_LABEL_CNT + " INT NOT NULL DEFAULT 0";
const std::string ADD_BASE_QUOTA_MODIFY_TIME_COLUMN = "ALTER TABLE " + TAB_ANALYSIS_PROGRESS_TABLE +
    " ADD COLUMN " + BASE_QUOTA_MODIFY_TIME + " BIGINT NOT NULL DEFAULT 0";
const std::string ADD_CHECK_SPACE_FLAG_COLUMN = "ALTER TABLE " + TAB_ANALYSIS_PROGRESS_TABLE +
    " ADD COLUMN " + CHECK_SPACE_FLAG + " INT NOT NULL DEFAULT 1";

const std::string ADD_PET_STATUS_COLUMN = "ALTER TABLE " + VISION_TOTAL_TABLE + " ADD COLUMN " +
    PET_STATUS + " INT NOT NULL DEFAULT 0";
const std::string PET_INDEX = "pet_index";
const std::string CREATE_PET_INDEX = "CREATE UNIQUE INDEX " + PET_INDEX + " ON " + VISION_PET_FACE_TABLE + " (" +
    FILE_ID + "," + PET_ID + ")";

const std::string PET_TAG_ID_INDEX = "pet_tag_id_index";
const std::string CREATE_PET_TAG_ID_INDEX = "CREATE INDEX IF NOT EXISTS " + PET_TAG_ID_INDEX + " ON " +
    VISION_PET_FACE_TABLE + " (" + PET_TAG_ID + ")" + " WHERE " + PET_TAG_ID + " LIKE " + "'ser%'";
const std::string UPDATE_PET_TOTAL_VALUE = "UPDATE " + VISION_TOTAL_TABLE + " SET " + STATUS + " = 0, " + PET_STATUS +
    " = 0 WHERE " + FILE_ID + " IN (SELECT " + FILE_ID + " FROM " + PhotoColumn::PHOTOS_TABLE +
    " WHERE media_type = 1)";

} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_VISION_DB_SQLS_H