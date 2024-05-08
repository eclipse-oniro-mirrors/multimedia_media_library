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

#include "vision_aesthetics_score_column.h"
#include "vision_album_column.h"
#include "vision_column_comm.h"
#include "vision_column.h"
#include "vision_composition_column.h"
#include "vision_face_tag_column.h"
#include "vision_head_column.h"
#include "vision_image_face_column.h"
#include "vision_label_column.h"
#include "vision_object_column.h"
#include "vision_ocr_column.h"
#include "vision_photo_map_column.h"
#include "vision_pose_column.h"
#include "vision_recommendation_column.h"
#include "vision_saliency_detect_column.h"
#include "vision_segmentation_column.h"
#include "vision_total_column.h"
#include "vision_video_label_column.h"

namespace OHOS {
namespace Media {
const std::string CREATE_TAB_ANALYSIS_OCR = "CREATE TABLE IF NOT EXISTS " + VISION_OCR_TABLE + " (" +
    ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    FILE_ID + " INT UNIQUE, " +
    OCR_TEXT + " TEXT, " +
    OCR_VERSION + " TEXT, " +
    OCR_TEXT_MSG + " TEXT, " +
    OCR_WIDTH + " INT, " +
    OCR_HEIGHT + " INT)";

const std::string CREATE_TAB_ANALYSIS_LABEL = "CREATE TABLE IF NOT EXISTS " + VISION_LABEL_TABLE + " (" +
    ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    FILE_ID + " INT UNIQUE, " +
    CATEGORY_ID + " INT, " +
    SUB_LABEL + " TEXT, " +
    PROB + " REAL, " +
    FEATURE + " TEXT, " +
    SIM_RESULT + " TEXT, " +
    LABEL_VERSION + " TEXT, " +
    SALIENCY_SUB_PROB + " TEXT) ";

const std::string CREATE_TAB_ANALYSIS_VIDEO_LABEL = "CREATE TABLE IF NOT EXISTS " + VISION_VIDEO_LABEL_TABLE + " (" +
    ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    FILE_ID + " INT, " +
    CATEGORY_ID + " TEXT, " +
    CONFIDENCE_PROBABILITY + " REAL, " +
    SUB_CATEGORY + " TEXT, " +
    SUB_CONFIDENCE_PROB + " REAL, " +
    SUB_LABEL + " TEXT, " +
    SUB_LABEL_PROB + " REAL, " +
    SUB_LABEL_TYPE + " INT, " +
    TRACKS + " TEXT, " +
    VIDEO_PART_FEATURE + " BLOB, " +
    FILTER_TAG + " TEXT, " +
    ALGO_VERSION + " TEXT) ";

const std::string CREATE_TAB_ANALYSIS_AESTHETICS = "CREATE TABLE IF NOT EXISTS " + VISION_AESTHETICS_TABLE + " (" +
    ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    FILE_ID + " INT UNIQUE, " +
    AESTHETICS_SCORE + " INT, " +
    AESTHETICS_VERSION + " TEXT, " +
    PROB + " REAL) ";

const std::string CREATE_TAB_ANALYSIS_SALIENCY_DETECT = "CREATE TABLE IF NOT EXISTS " + VISION_SALIENCY_TABLE + " (" +
    ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    FILE_ID + " INT UNIQUE, " +
    SALIENCY_X + " REAL, " +
    SALIENCY_Y + " REAL, " +
    SALIENCY_VERSION + " TEXT) ";

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
    OBJECT_VERSION + " TEXT) ";

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
    RECOMMENDATION_VERSION + " TEXT) ";

const std::string CREATE_TAB_ANALYSIS_SEGMENTATION = "CREATE TABLE IF NOT EXISTS " + VISION_SEGMENTATION_TABLE + " (" +
    ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    FILE_ID + " INT UNIQUE, " +
    SEGMENTATION_AREA + " TEXT, " +
    SEGMENTATION_NAME + " INT, " +
    PROB + " REAL, " +
    SEGMENTATION_VERSION + " TEXT) ";

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
    COMPOSITION_VERSION + " TEXT) ";

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
    HEAD_VERSION + " TEXT) ";

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
    POSE_VERSION + " TEXT) ";

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
    POSE + " INT) ";

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
    FEATURES + " BLOB) ";

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
    IS_REMOVED + " INTEGER) ";

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
    "PRIMARY KEY (" + MAP_ALBUM + "," + MAP_ASSET + ")) ";


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
    " END;";

const std::string CREATE_VISION_UPDATE_TRIGGER = "CREATE TRIGGER IF NOT EXISTS update_vision_trigger AFTER UPDATE ON " +
    PhotoColumn::PHOTOS_TABLE + " FOR EACH ROW " +
    " WHEN ((NEW.date_trashed > 0 AND OLD.date_trashed = 0)" +
    " OR (NEW.date_trashed = 0 AND OLD.date_trashed > 0))" +
    " AND NEW.MEDIA_TYPE = 1" +
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
    " BEGIN " +
    " INSERT INTO " + VISION_TOTAL_TABLE +" (" + FILE_ID + ", " + STATUS + ", " + OCR + ", " + AESTHETICS_SCORE + ", " +
    LABEL + ", " + FACE + ", " + OBJECT + ", " + RECOMMENDATION + ", " + SEGMENTATION + ", " + COMPOSITION + "," +
    SALIENCY + ", " + HEAD + ", " + POSE + ") " +
    " VALUES (" + " NEW.file_id, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 );" + " END;";

const std::string DROP_INSERT_VISION_TRIGGER = "DROP TRIGGER IF EXISTS insert_vision_trigger";

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
const std::string CREATE_OBJECT_INDEX = "CREATE UNIQUE INDEX " + OBJECT_INDEX + " ON " +
    VISION_OBJECT_TABLE + " (" + FILE_ID + "," + OBJECT_ID + ")";

const std::string AC_ADD_RECOMMENDATION_COLUMN_FOR_TOTAL = "ALTER TABLE " + VISION_TOTAL_TABLE + " ADD COLUMN " +
    RECOMMENDATION + " INT";
const std::string AC_UPDATE_RECOMMENDATION_TOTAL_VALUE = "UPDATE " + VISION_TOTAL_TABLE + " SET " + STATUS + " = 0, " +
    RECOMMENDATION + " = 0 WHERE " + FILE_ID + " IN (SELECT " + FILE_ID + " FROM " + PhotoColumn::PHOTOS_TABLE +
    " WHERE subtype != 1)";
const std::string AC_UPDATE_RECOMMENDATION_TOTAL_NOT_SUPPORT_VALUE = "UPDATE " + VISION_TOTAL_TABLE + " SET " +
    RECOMMENDATION + " = -1 WHERE " + RECOMMENDATION + " IS NULL";
const std::string RECOMMENDATION_INDEX = "recommendation_index";
const std::string CREATE_RECOMMENDATION_INDEX = "CREATE UNIQUE INDEX " + RECOMMENDATION_INDEX + " ON " +
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
const std::string CREATE_COMPOSITION_INDEX = "CREATE UNIQUE INDEX " + COMPOSITION_INDEX + " ON " +
    VISION_COMPOSITION_TABLE + " (" + FILE_ID + "," + COMPOSITION_ID + ")";

const std::string ADD_HEAD_STATUS_COLUMN = "ALTER TABLE " + VISION_TOTAL_TABLE + " ADD COLUMN " + HEAD + " INT";
const std::string UPDATE_HEAD_TOTAL_VALUE = "UPDATE " + VISION_TOTAL_TABLE + " SET " + STATUS + " = 0, " + HEAD +
    " = 0 WHERE " + FILE_ID + " IN (SELECT " + FILE_ID + " FROM " + PhotoColumn::PHOTOS_TABLE +
    " WHERE media_type = 1)";
const std::string UPDATE_HEAD_NOT_SUPPORT_VALUE = "UPDATE " + VISION_TOTAL_TABLE + " SET " + HEAD + " = -1 WHERE " +
    HEAD + " IS NULL";
const std::string HEAD_INDEX = "head_index";
const std::string CREATE_HEAD_INDEX = "CREATE UNIQUE INDEX " + HEAD_INDEX + " ON " + VISION_HEAD_TABLE + " (" +
    FILE_ID + "," + HEAD_ID + ")";

const std::string ADD_POSE_STATUS_COLUMN = "ALTER TABLE " + VISION_TOTAL_TABLE + " ADD COLUMN " + POSE + " INT";
const std::string UPDATE_POSE_TOTAL_VALUE = "UPDATE " + VISION_TOTAL_TABLE + " SET " + STATUS + " = 0, " + POSE +
    " = 0 WHERE " + FILE_ID + " IN (SELECT " + FILE_ID + " FROM " + PhotoColumn::PHOTOS_TABLE +
    " WHERE media_type = 1)";
const std::string UPDATE_POSE_NOT_SUPPORT_VALUE = "UPDATE " + VISION_TOTAL_TABLE + " SET " + POSE + " = -1 WHERE " +
    POSE + " IS NULL";
const std::string POSE_INDEX = "pose_index";
const std::string CREATE_POSE_INDEX = "CREATE UNIQUE INDEX " + POSE_INDEX + " ON " + VISION_POSE_TABLE + " (" +
    FILE_ID + "," + POSE_ID + ")";

const std::string ALTER_WIDTH_COLUMN = "ALTER TABLE tab_analysis_ocr ADD COLUMN width INT;";
const std::string ALTER_HEIGHT_COLUMN = "ALTER TABLE tab_analysis_ocr ADD COLUMN height INT;";
const std::string DROP_TABLE_ANALYSISALBUM = "DROP TABLE AnalysisAlbum;";
const std::string DROP_TABLE_ANALYSISPHOTOMAP = "DROP TABLE AnalysisPhotoMap;";

const std::string ADD_TAG_ID_COLUMN_FOR_ALBUM = "ALTER TABLE " + ANALYSIS_ALBUM_TABLE + " ADD COLUMN " +
    TAG_ID + " TEXT";
const std::string ADD_USER_OPERATION_COLUMN_FOR_ALBUM = "ALTER TABLE " + ANALYSIS_ALBUM_TABLE + " ADD COLUMN " +
    USER_OPERATION + " INT";
const std::string ADD_GROUP_TAG_COLUMN_FOR_ALBUM = "ALTER TABLE " + ANALYSIS_ALBUM_TABLE + " ADD COLUMN " +
    GROUP_TAG + " TEXT";
const std::string ADD_USER_DISPLAY_LEVEL_COLUMN_FOR_ALBUM = "ALTER TABLE " + ANALYSIS_ALBUM_TABLE + " ADD COLUMN " +
    USER_DISPLAY_LEVEL + " INT";
const std::string ADD_IS_ME_COLUMN_FOR_ALBUM = "ALTER TABLE " + ANALYSIS_ALBUM_TABLE + " ADD COLUMN " +
    IS_ME + " INT";
const std::string ADD_IS_REMOVED_COLUMN_FOR_ALBUM = "ALTER TABLE " + ANALYSIS_ALBUM_TABLE + " ADD COLUMN " +
    IS_REMOVED + " INT";
const std::string ADD_RENAME_OPERATION_COLUMN_FOR_ALBUM = "ALTER TABLE " + ANALYSIS_ALBUM_TABLE + " ADD COLUMN " +
    RENAME_OPERATION + " INT";
const std::string ADD_IS_LOCAL_COLUMN_FOR_ALBUM = "ALTER TABLE " + ANALYSIS_ALBUM_TABLE + " ADD COLUMN " +
    IS_LOCAL + " INT";
const std::string ADD_IS_COVER_SATISFIED_FOR_ALBUM = "ALTER TABLE " + ANALYSIS_ALBUM_TABLE + " ADD COLUMN " +
    IS_COVER_SATISFIED + " INT";
const std::string CREATE_ANALYSIS_ALBUM_FOR_ONCREATE = "CREATE TABLE IF NOT EXISTS " + ANALYSIS_ALBUM_TABLE + " (" +
    ALBUM_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    ALBUM_TYPE + " INT, " +
    ALBUM_SUBTYPE + " INT, " +
    ALBUM_NAME + " TEXT, " +
    COVER_URI + " TEXT, " +
    COUNT + " INT, " +
    DATE_MODIFIED + " BIGINT, " +
    RANK + " INT, " +
    TAG_ID + " TEXT, " +
    USER_OPERATION + " INT, " +
    GROUP_TAG + " TEXT, " +
    USER_DISPLAY_LEVEL + " INT, " +
    IS_ME + " INT, " +
    IS_REMOVED + " INT, " +
    RENAME_OPERATION + " INT, " +
    IS_LOCAL + " INT, " +
    IS_COVER_SATISFIED + " INT) ";
const std::string ANALYSIS_ALBUM_UPDATE_SEARCH_TRIGGER = "analysis_album_update_search_trigger";
const std::string CREATE_ANALYSIS_ALBUM_UPDATE_SEARCH_TRIGGER = "CREATE TRIGGER " +
    ANALYSIS_ALBUM_UPDATE_SEARCH_TRIGGER +
    " AFTER UPDATE OF " + ALBUM_NAME +
    " ON " + ANALYSIS_ALBUM_TABLE +
    " FOR EACH ROW WHEN (NEW." + ALBUM_SUBTYPE + " = " + std::to_string(PORTRAIT) + ")" +
    " BEGIN " +
    " UPDATE " + "tab_analysis_search_index" +
    " SET " + "cv_status" + " = 0 " +
    " WHERE (" + FILE_ID + " IN( " +
    " SELECT " + MAP_ASSET +
    " FROM " + ANALYSIS_PHOTO_MAP_TABLE +
    " WHERE (old." + ALBUM_ID + " = " + ANALYSIS_PHOTO_MAP_TABLE + "." + MAP_ALBUM + ")" +
    ")); END;";
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_VISION_DB_SQLS_H