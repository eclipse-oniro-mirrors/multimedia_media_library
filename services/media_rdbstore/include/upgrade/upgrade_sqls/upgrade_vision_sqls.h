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

#ifndef UPGRADE_VISION_SQLS_H
#define UPGRADE_VISION_SQLS_H
// table name need to be added here
#define TABLE_TAB_ANALYSIS_LABEL "tab_analysis_label"
#define TABLE_TAB_ANALYSIS_TOTAL "tab_analysis_total"
#define TABLE_TAB_ANALYSIS_OCR "tab_analysis_ocr"
// column name should be added here
#define COLUMN_ANALYSIS_CAPTION "caption"
#define COLUMN_OCR_TIME "ocr_time"
#define COLUMN_OCR_SUBTYPE "ocr_subtype"
// trigger name should be added here
#define TRIGGER_ANALYSIS_UPDATE_SEARCH_TRIGGER "analysis_update_search_trigger"

// sqls only execute in upgrade progress should be added here
#define SQL_CREATE_TAB_ANALYSIS_OCR \
    "CREATE TABLE IF NOT EXISTS tab_analysis_ocr (" \
    "id INTEGER PRIMARY KEY AUTOINCREMENT, " \
    "file_id INT UNIQUE, " \
    "ocr_text TEXT, " \
    "ocr_version TEXT, " \
    "ocr_text_msg TEXT, " \
    "width INT, " \
    "height INT, " \
    "analysis_version TEXT)"

#define SQL_CREATE_TAB_ANALYSIS_LABEL \
    "CREATE TABLE IF NOT EXISTS tab_analysis_label (" \
    "id INTEGER PRIMARY KEY AUTOINCREMENT, " \
    "file_id INT UNIQUE, " \
    "category_id INT, " \
    "sub_label TEXT, " \
    "prob REAL, " \
    "feature TEXT, " \
    "sim_result TEXT, " \
    "label_version TEXT, " \
    "saliency_sub_prob TEXT, " \
    "analysis_version TEXT, " \
    "caption_result TEXT, " \
    "caption_version TEXT, " \
    "search_tag_type TEXT DEFAULT '', " \
    "search_tag_vector BLOB, " \
    "significance_score INT, " \
    "significance_score_version TEXT)"

#define SQL_CREATE_TAB_ANALYSIS_AESTHETICS \
    "CREATE TABLE IF NOT EXISTS tab_analysis_aesthetics (" \
    "id INTEGER PRIMARY KEY AUTOINCREMENT, " \
    "file_id INT UNIQUE, " \
    "aesthetics_score INT, " \
    "aesthetics_version TEXT, " \
    "prob REAL, " \
    "analysis_version TEXT, " \
    "selected_flag INT, " \
    "selected_algo_version TEXT, " \
    "selected_status INT, " \
    "negative_flag INT, " \
    "negative_algo_version TEXT, " \
    "aesthetics_all_version TEXT, " \
    "aesthetics_score_all INT NOT NULL DEFAULT 0, " \
    "is_filtered_hard BOOLEAN NOT NULL DEFAULT 0, " \
    "clarity_score_all DOUBLE NOT NULL DEFAULT 0, " \
    "saturation_score_all DOUBLE NOT NULL DEFAULT 0, " \
    "luminance_score_all DOUBLE NOT NULL DEFAULT 0, " \
    "semantics_score DOUBLE NOT NULL DEFAULT 0, " \
    "is_black_white_stripe BOOLEAN NOT NULL DEFAULT 0, " \
    "is_blurry BOOLEAN NOT NULL DEFAULT 0, " \
    "is_mosaic BOOLEAN NOT NULL DEFAULT 0)"

#define SQL_CREATE_TAB_ANALYSIS_TOTAL \
    "CREATE TABLE IF NOT EXISTS tab_analysis_total (" \
    "id INTEGER PRIMARY KEY AUTOINCREMENT, " \
    "file_id INT UNIQUE, " \
    "status INT, " \
    "ocr INT, " \
    "label INT, " \
    "aesthetics_score INT)"

#define SQL_CREATE_VISION_UPDATE_TRIGGER \
    "CREATE TRIGGER IF NOT EXISTS update_vision_trigger AFTER UPDATE ON Photos " \
    "FOR EACH ROW " \
    "WHEN ((NEW.date_trashed > 0 AND OLD.date_trashed = 0) " \
    "OR (NEW.date_trashed = 0 AND OLD.date_trashed > 0)) " \
    "AND (NEW.MEDIA_TYPE = 1 OR NEW.MEDIA_TYPE = 2) " \
    "BEGIN " \
    "UPDATE tab_analysis_total " \
    "SET status = " \
    "(CASE WHEN NEW.date_trashed > 0 THEN 2 ELSE 0 END) " \
    "WHERE file_id = OLD.file_id; " \
    "UPDATE tab_analysis_video_total " \
    "SET status = " \
    "(CASE WHEN NEW.date_trashed > 0 THEN 2 ELSE 0 END) " \
    "WHERE file_id = OLD.file_id AND OLD.media_type = 2; " \
    "END"

#define SQL_CREATE_VISION_DELETE_TRIGGER \
    "CREATE TRIGGER IF NOT EXISTS delete_vision_trigger AFTER DELETE ON Photos " \
    "FOR EACH ROW " \
    "BEGIN " \
    "UPDATE tab_analysis_total " \
    "SET status = -1 " \
    "WHERE file_id = OLD.file_id; " \
    "UPDATE tab_analysis_video_total " \
    "SET status = -1 " \
    "WHERE file_id = OLD.file_id AND OLD.media_type = 2;" \
    "END"

#define SQL_CREATE_VISION_INSERT_TRIGGER \
    "CREATE TRIGGER IF NOT EXISTS insert_vision_trigger AFTER INSERT ON Photos " \
    "FOR EACH ROW " \
    "WHEN NEW.MEDIA_TYPE = 1 " \
    "BEGIN " \
    "INSERT INTO tab_analysis_total (" \
    "file_id, status, ocr, " \
    "aesthetics_score, " \
    "label) " \
    "VALUES (" \
    "NEW.file_id, 0, 0," \
    "(CASE WHEN NEW.subtype = 1 THEN -1 ELSE 0 END)," \
    "(CASE WHEN NEW.subtype = 1 THEN -1 ELSE 0 END)); " \
    "END"

#define SQL_INIT_TAB_ANALYSIS_TOTAL \
    "INSERT INTO tab_analysis_total (" \
    "file_id, " \
    "status, " \
    "ocr, " \
    "aesthetics_score, " \
    "label) " \
    "SELECT file_id," \
    "CASE WHEN date_trashed > 0 THEN 2 ELSE 0 END," \
    "0," \
    "CASE WHEN subtype = 1 THEN -1 ELSE 0 END," \
    "CASE WHEN subtype = 1 THEN -1 ELSE 0 END" \
    "FROM Photos WHERE MEDIA_TYPE = 1"

#define SQL_UPGRADE_CREATE_TAB_ANALYSIS_CAPTION \
    "CREATE TABLE IF NOT EXISTS tab_analysis_caption (" \
    "file_id INTEGER PRIMARY KEY, " \
    "caption TEXT, " \
    "caption_version TEXT, " \
    "caption_vector TEXT, " \
    "analysis_version TEXT)"

#define SQL_UPGRADE_CREATE_ANALYSIS_UPDATE_SEARCH_TRIGGER \
    "CREATE TRIGGER IF NOT EXISTS analysis_update_search_trigger AFTER UPDATE " \
    " OF status, ocr, label, face, caption" \
    " ON tab_analysis_total FOR EACH ROW " \
    " WHEN (NEW.status = 1" \
    " OR NEW.ocr = 1 OR NEW.label = 1 OR NEW.face > 2 OR NEW.caption = 1)" \
    " BEGIN " \
    " UPDATE tab_analysis_search_index" \
    " SET cv_status = " \
    " CASE WHEN (NEW.status = 1) THEN 0" \
    " ELSE 2 END" \
    " WHERE  (file_id = old.file_id " \
    " AND cv_status = 1" \
    " AND EXISTS (SELECT 1 FROM Photos WHERE file_id = old.file_id AND media_type != 2));" \
    " END;"

#endif // UPGRADE_VISION_SQLS_H