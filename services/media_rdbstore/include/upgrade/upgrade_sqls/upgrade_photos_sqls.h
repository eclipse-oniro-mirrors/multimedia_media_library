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

#ifndef UPGRADE_PHOTOS_SQLS_H
#define UPGRADE_PHOTOS_SQLS_H
// table name need to be added here
#define TABLE_PHOTOS "Photos"
// column name should be added here
#define COLUMN_PHOTO_RISK_STATUS "photo_risk_status"
#define COLUMN_CRITICAL_TYPE "critical_type"
#define COLUMN_PHOTO_NEED_THUMBNAIL "need_thumbnail"
#define COLUMN_ATTACHMENT_SIZE "attachment_size"
// index name shoule be added here
#define INDEX_PHOTO_SORT_IN_ALBUM_DATE_ADDED_INDEX "idx_photo_sort_in_album_date_added"
#define INDEX_PHOTO_SORT_IN_ALBUM_DATE_TAKEN_INDEX "idx_photo_sort_in_album_date_taken"
#define INDEX_PHOTO_SORT_IN_ALBUM_DISPLAY_NAME_INDEX "idx_photo_sort_in_album_display_name"
#define INDEX_PHOTO_SORT_IN_ALBUM_SIZE_INDEX "idx_photo_sort_in_album_size"
#define INDEX_PHOTO_SORT_MEDIA_TYPE_DATE_ADDED_INDEX "idx_photo_sort_media_type_date_added"
#define INDEX_PHOTO_SORT_MEDIA_TYPE_DATE_TAKEN_INDEX "idx_photo_sort_media_type_date_taken"
#define INDEX_PHOTO_SORT_MEDIA_TYPE_DISPLAY_NAME_INDEX "idx_photo_sort_media_type_display_name"
#define INDEX_PHOTO_SORT_MEDIA_TYPE_SIZE_INDEX "idx_photo_sort_media_type_size"
#define INDEX_PHOTO_FAVORITE_INDEX "idx_photo_is_favorite"
#define INDEX_PHOTO_SHOOTING_MODE_ALBUM_GENERAL_INDEX "idx_shooting_mode_album_general"
#define INDEX_PHOTO_MOVING_PHOTO_ALBUM_INDEX "idx_moving_photo_album"
#define INDEX_PHOTO_RAW_IMAGE_ALBUM_INDEX "idx_raw_image_album"
#define INDEX_PHOTO_SCHPT_PHOTO_DATEADDED_INDEX "idx_schpt_date_added_new"
#define INDEX_PHOTO_SCHPT_HIDDEN_TIME_INDEX "idx_schpt_hidden_time"
// sqls only execute in upgrade progress should be added here
#define SQL_UPGRADE_CREATE_PHOTO_SORT_IN_ALBUM_DATE_ADDED_INDEX \
    "CREATE INDEX IF NOT EXISTS idx_photo_sort_in_album_date_added ON Photos (" \
    "owner_album_id,hidden,clean_flag,sync_status,date_trashed,time_pending," \
    "is_temp,burst_cover_level,date_added DESC,display_name DESC, file_id);"

#define SQL_UPGRADE_CREATE_PHOTO_SORT_IN_ALBUM_DATE_TAKEN_INDEX \
    "CREATE INDEX IF NOT EXISTS idx_photo_sort_in_album_date_taken ON Photos (" \
    "sync_status,clean_flag,date_trashed,hidden,time_pending,is_temp," \
    "burst_cover_level,owner_album_id,date_taken DESC,display_name DESC, file_id);"

#define SQL_UPGRADE_CREATE_PHOTO_SORT_IN_ALBUM_DISPLAY_NAME_INDEX \
    "CREATE INDEX IF NOT EXISTS idx_photo_sort_in_album_display_name ON Photos (" \
    "sync_status,clean_flag,date_trashed,hidden,time_pending, is_temp," \
    "burst_cover_level,owner_album_id,display_name DESC, date_taken DESC, file_id DESC);"

#define SQL_UPGRADE_CREATE_PHOTO_SORT_IN_ALBUM_SIZE_INDEX \
    "CREATE INDEX IF NOT EXISTS idx_photo_sort_in_album_size ON Photos (" \
    "sync_status,clean_flag,date_trashed,hidden,time_pending, " \
    "is_temp,burst_cover_level,owner_album_id,size DESC,file_id DESC, date_taken DESC);"

#define SQL_UPGRADE_CREATE_PHOTO_SORT_MEDIA_TYPE_DATE_ADDED_INDEX \
    "CREATE INDEX IF NOT EXISTS idx_photo_sort_media_type_date_added ON Photos (" \
    "sync_status,clean_flag,date_trashed,hidden,time_pending,is_temp," \
    "burst_cover_level,media_type,date_added DESC,display_name DESC, strong_association,file_id DESC);"

#define SQL_UPGRADE_CREATE_PHOTO_SORT_MEDIA_TYPE_DATE_TAKEN_INDEX \
    "CREATE INDEX IF NOT EXISTS idx_photo_sort_media_type_date_taken ON Photos (" \
    "sync_status,clean_flag,date_trashed,hidden,time_pending,is_temp," \
    "burst_cover_level,media_type,date_taken DESC,display_name DESC, strong_association,file_id DESC);"

#define SQL_UPGRADE_CREATE_PHOTO_SORT_MEDIA_TYPE_DISPLAY_NAME_INDEX \
    "CREATE INDEX IF NOT EXISTS idx_photo_sort_media_type_display_name ON Photos (" \
    "sync_status,clean_flag,date_trashed,hidden,time_pending, is_temp," \
    "burst_cover_level,media_type,display_name DESC,strong_association,date_taken DESC, file_id DESC);"

#define SQL_UPGRADE_CREATE_PHOTO_SORT_MEDIA_TYPE_SIZE_INDEX \
    "CREATE INDEX IF NOT EXISTS idx_photo_sort_media_type_size ON Photos (" \
    "sync_status,clean_flag,date_trashed,hidden,time_pending, is_temp,burst_cover_level," \
    "media_type,size DESC,file_id DESC,strong_association,display_name DESC);"

#define SQL_UPGRADE_CREATE_PHOTO_FAVORITE_INDEX \
    "CREATE INDEX IF NOT EXISTS idx_photo_is_favorite ON Photos (" \
    "sync_status,clean_flag,hidden,time_pending,date_trashed,is_temp,is_favorite," \
    "burst_cover_level,date_taken DESC, display_name DESC, file_id DESC);"

#define SQL_UPGRADE_CREATE_PHOTO_SHOOTING_MODE_ALBUM_GENERAL_INDEX \
    "CREATE INDEX IF NOT EXISTS idx_shooting_mode_album_general ON Photos (" \
    "sync_status,clean_flag,date_trashed,hidden,time_pending, is_temp,burst_cover_level," \
    "shooting_mode,date_taken DESC,display_name DESC, file_id);"

#define SQL_UPGRADE_CREATE_PHOTO_MOVING_PHOTO_ALBUM_INDEX \
    "CREATE INDEX IF NOT EXISTS idx_moving_photo_album ON Photos (" \
    "sync_status,clean_flag,date_trashed,hidden,time_pending, is_temp,burst_cover_level," \
    "date_taken DESC,display_name DESC, file_id) " \
    "WHERE (subtype = 6 OR (moving_photo_effect_mode = 1 AND subtype = 0));"

#define SQL_UPGRADE_CREATE_PHOTO_RAW_IMAGE_ALBUM_INDEX \
    "CREATE INDEX IF NOT EXISTS idx_raw_image_album ON Photos (" \
    "sync_status,clean_flag,date_trashed,hidden,time_pending, is_temp,burst_cover_level," \
    "mime_type,date_taken DESC,display_name DESC, file_id DESC);"

#define SQL_UPGRADE_INDEX_SCTHP_PHOTO_DATEADDED \
    "CREATE INDEX IF NOT EXISTS idx_schpt_date_added_new ON Photos (" \
    "sync_status,clean_flag,date_trashed,hidden,time_pending,is_temp,burst_cover_level," \
    "date_added DESC, thumbnail_visible,display_name DESC, file_id DESC);"

#define SQL_UPGRADE_CREATE_SCHPT_HIDDEN_TIME_INDEX \
    "CREATE INDEX IF NOT EXISTS idx_schpt_hidden_time ON Photos (" \
    "sync_status,clean_flag,hidden,time_pending,date_trashed,is_temp,burst_cover_level," \
    "hidden_time DESC, media_type,owner_album_id,display_name DESC, file_id DESC);"

#define SQL_UPGRADE_CREATE_LPATH_INDEX \
    "CREATE INDEX IF NOT EXISTS lpath_index ON tab_cover_record (" \
    "album_type,album_subtype,lpath COLLATE NOCASE)"

#endif // UPGRADE_PHOTOS_SQLS_H