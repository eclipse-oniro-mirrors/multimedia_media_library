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

#include "media_upgrade.h"
#include "media_column.h"
#include "base_column.h"
#include "userfile_manager_types.h"
#include "medialibrary_db_const.h"
#include "highlight_column.h"

namespace OHOS {
namespace Media {
using namespace std;

const std::string PhotoUpgrade::CREATE_PHOTO_TABLE = "CREATE TABLE IF NOT EXISTS " +
    PhotoColumn::PHOTOS_TABLE + " (" +
    MediaColumn::MEDIA_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    MediaColumn::MEDIA_FILE_PATH + " TEXT, " +
    MediaColumn::MEDIA_SIZE + " BIGINT, " +
    MediaColumn::MEDIA_TITLE + " TEXT, " +
    MediaColumn::MEDIA_NAME + " TEXT, " +
    MediaColumn::MEDIA_TYPE + " INT, " +
    MediaColumn::MEDIA_MIME_TYPE + " TEXT, " +
    MediaColumn::MEDIA_OWNER_PACKAGE + " TEXT, " +
    MediaColumn::MEDIA_OWNER_APPID + " TEXT, " +
    MediaColumn::MEDIA_PACKAGE_NAME + " TEXT, " +
    MediaColumn::MEDIA_DEVICE_NAME + " TEXT, " +
    MediaColumn::MEDIA_DATE_ADDED + " BIGINT, " +
    MediaColumn::MEDIA_DATE_MODIFIED + " BIGINT, " +
    MediaColumn::MEDIA_DATE_TAKEN + " BIGINT DEFAULT 0, " +
    MediaColumn::MEDIA_DURATION + " INT, " +
    MediaColumn::MEDIA_TIME_PENDING + " BIGINT DEFAULT 0, " +
    MediaColumn::MEDIA_IS_FAV + " INT DEFAULT 0, " +
    MediaColumn::MEDIA_DATE_TRASHED + " BIGINT DEFAULT 0, " +
    MediaColumn::MEDIA_DATE_DELETED + " BIGINT DEFAULT 0, " +
    MediaColumn::MEDIA_HIDDEN + " INT DEFAULT 0, " +
    MediaColumn::MEDIA_PARENT_ID + " INT DEFAULT 0, " +
    MediaColumn::MEDIA_RELATIVE_PATH + " TEXT, " +
    MediaColumn::MEDIA_VIRTURL_PATH + " TEXT UNIQUE, " +
    PhotoColumn::PHOTO_DIRTY + " INT DEFAULT 1, " +
    PhotoColumn::PHOTO_CLOUD_ID + " TEXT, " +
    PhotoColumn::PHOTO_META_DATE_MODIFIED + "  BIGINT DEFAULT 0, " +
    PhotoColumn::PHOTO_SYNC_STATUS + "  INT DEFAULT 0, " +
    PhotoColumn::PHOTO_CLOUD_VERSION + " BIGINT DEFAULT 0, " +
    PhotoColumn::PHOTO_ORIENTATION + " INT DEFAULT 0, " +
    PhotoColumn::PHOTO_EXIF_ROTATE + " INT NOT NULL DEFAULT 0, " +
    PhotoColumn::PHOTO_LATITUDE + " DOUBLE DEFAULT 0, " +
    PhotoColumn::PHOTO_LONGITUDE + " DOUBLE DEFAULT 0, " +
    PhotoColumn::PHOTO_HEIGHT + " INT, " +
    PhotoColumn::PHOTO_WIDTH + " INT, " +
    PhotoColumn::PHOTO_EDIT_TIME + " BIGINT DEFAULT 0, " +
    PhotoColumn::PHOTO_LCD_VISIT_TIME + " BIGINT DEFAULT 0, " +
    PhotoColumn::PHOTO_POSITION + " INT DEFAULT 1, " +
    PhotoColumn::PHOTO_SUBTYPE + " INT DEFAULT 0, " +
    PhotoColumn::PHOTO_ORIGINAL_SUBTYPE + " INT," +
    PhotoColumn::CAMERA_SHOT_KEY + " TEXT, " +
    PhotoColumn::PHOTO_USER_COMMENT + " TEXT, " +
    PhotoColumn::PHOTO_ALL_EXIF  + " TEXT, " +
    PhotoColumn::PHOTO_DATE_YEAR + " TEXT, " +
    PhotoColumn::PHOTO_DATE_MONTH + " TEXT, " +
    PhotoColumn::PHOTO_DATE_DAY + " TEXT, " +
    PhotoColumn::PHOTO_SHOOTING_MODE + " TEXT, " +
    PhotoColumn::PHOTO_SHOOTING_MODE_TAG + " TEXT, " +
    PhotoColumn::PHOTO_LAST_VISIT_TIME + " BIGINT DEFAULT 0, " +
    PhotoColumn::PHOTO_HIDDEN_TIME + " BIGINT DEFAULT 0, " +
    PhotoColumn::PHOTO_THUMB_STATUS + " INT DEFAULT 0, " +
    PhotoColumn::PHOTO_CLEAN_FLAG + " INT DEFAULT 0, " +
    PhotoColumn::PHOTO_ID + " TEXT, " +
    PhotoColumn::PHOTO_QUALITY + " INT, " +
    PhotoColumn::PHOTO_FIRST_VISIT_TIME + " BIGINT DEFAULT 0, " +
    PhotoColumn::PHOTO_DEFERRED_PROC_TYPE + " INT DEFAULT 0, " +
    PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE + " INT DEFAULT 0, " +
    PhotoColumn::MOVING_PHOTO_EFFECT_MODE + " INT DEFAULT 0, " +
    PhotoColumn::PHOTO_COVER_POSITION + " BIGINT DEFAULT 0, " +
    PhotoColumn::PHOTO_IS_RECTIFICATION_COVER + " INT NOT NULL DEFAULT 0, " +
    PhotoColumn::PHOTO_THUMBNAIL_READY + " BIGINT DEFAULT 0, " +
    PhotoColumn::PHOTO_LCD_SIZE + " TEXT, " +
    PhotoColumn::PHOTO_THUMB_SIZE + " TEXT," +
    PhotoColumn::PHOTO_FRONT_CAMERA + " TEXT, " +
    PhotoColumn::PHOTO_IS_TEMP + " INT DEFAULT 0," +
    PhotoColumn::PHOTO_BURST_COVER_LEVEL + " INT DEFAULT 1, " +
    PhotoColumn::PHOTO_BURST_KEY + " TEXT, " +
    PhotoColumn::PHOTO_DETAIL_TIME + " TEXT, " +
    PhotoColumn::PHOTO_OWNER_ALBUM_ID + " INT DEFAULT 0, " +
    PhotoColumn::PHOTO_ORIGINAL_ASSET_CLOUD_ID + " TEXT, " +
    PhotoColumn::PHOTO_SOURCE_PATH + " TEXT, " +
    PhotoColumn::PHOTO_CE_AVAILABLE + " INT DEFAULT 0, " +
    PhotoColumn::PHOTO_CE_STATUS_CODE + " INT, " +
    PhotoColumn::PHOTO_STRONG_ASSOCIATION + " INT DEFAULT 0, " +
    PhotoColumn::PHOTO_ASSOCIATE_FILE_ID + " INT DEFAULT 0, " +
    PhotoColumn::PHOTO_HAS_CLOUD_WATERMARK + " INT DEFAULT 0, " +
    PhotoColumn::PHOTO_THUMBNAIL_VISIBLE + " INT DEFAULT 0, " +
    PhotoColumn::SUPPORTED_WATERMARK_TYPE + " INT, " +
    PhotoColumn::PHOTO_METADATA_FLAGS + " INT DEFAULT 0, " +
    PhotoColumn::PHOTO_CHECK_FLAG + " INT DEFAULT 0, " +
    PhotoColumn::STAGE_VIDEO_TASK_STATUS + " INT NOT NULL DEFAULT 0, " +
    PhotoColumn::PHOTO_IS_AUTO + " INT NOT NULL DEFAULT 0, " +
    PhotoColumn::PHOTO_MEDIA_SUFFIX + " TEXT, " +
    PhotoColumn::PHOTO_IS_RECENT_SHOW + " INT NOT NULL DEFAULT 1, " +
    PhotoColumn::PHOTO_REAL_LCD_VISIT_TIME + " BIGINT NOT NULL DEFAULT 0, " +
    PhotoColumn::PHOTO_VISIT_COUNT + " INT NOT NULL DEFAULT 0, " +
    PhotoColumn::PHOTO_LCD_VISIT_COUNT + " INT NOT NULL DEFAULT 0," +
    PhotoColumn::PHOTO_HAS_APPLINK + " INT NOT NULL DEFAULT 0," +
    PhotoColumn::PHOTO_APPLINK + " TEXT, " +
    PhotoColumn::PHOTO_TRANSCODE_TIME + " BIGINT NOT NULL DEFAULT 0, " +
    PhotoColumn::PHOTO_TRANS_CODE_FILE_SIZE + " BIGINT NOT NULL DEFAULT 0, " +
    PhotoColumn::PHOTO_EXIST_COMPATIBLE_DUPLICATE + " INT NOT NULL DEFAULT 0, " +
    PhotoColumn::PHOTO_SOUTH_DEVICE_TYPE + " INT NOT NULL DEFAULT 0, " +
    PhotoColumn::PHOTO_COMPOSITE_DISPLAY_STATUS + " INT NOT NULL DEFAULT 0, " +
    PhotoColumn::PHOTO_FILE_INODE + " TEXT, " +
    PhotoColumn::PHOTO_STORAGE_PATH + " TEXT, " +
    PhotoColumn::PHOTO_FILE_SOURCE_TYPE + " INT NOT NULL DEFAULT 0, " +
    PhotoColumn::PHOTO_HDR_MODE + " INT NOT NULL DEFAULT 0, " +
    PhotoColumn::PHOTO_VIDEO_MODE + " INT NOT NULL DEFAULT -1, " +
    PhotoColumn::PHOTO_ASPECT_RATIO + " DOUBLE NOT NULL DEFAULT -2, " +
    PhotoColumn::PHOTO_CHANGE_TIME + " BIGINT NOT NULL DEFAULT 0, " +
    PhotoColumn::PHOTO_MOVINGPHOTO_ENHANCEMENT_TYPE + " INT NOT NULL DEFAULT 0, " +
    PhotoColumn::PHOTO_IS_CRITICAL + " INT NOT NULL DEFAULT 0," +
    PhotoColumn::PHOTO_RISK_STATUS + " INT NOT NULL DEFAULT 0" +
    ") ";

const std::string PhotoUpgrade::CREATE_CLOUD_ID_INDEX = BaseColumn::CreateIndex() +
    PhotoColumn::PHOTO_CLOUD_ID_INDEX + " ON " + PhotoColumn::PHOTOS_TABLE + " (" +
    PhotoColumn::PHOTO_CLOUD_ID + " DESC)";

const std::string PhotoUpgrade::CREATE_YEAR_INDEX = BaseColumn::CreateIndex() +
    PhotoColumn::PHOTO_DATE_YEAR_INDEX + " ON " + PhotoColumn::PHOTOS_TABLE + " (" +
    PhotoColumn::PHOTO_DATE_YEAR + " DESC)";

const std::string PhotoUpgrade::CREATE_MONTH_INDEX = BaseColumn::CreateIndex() +
    PhotoColumn::PHOTO_DATE_MONTH_INDEX + " ON " + PhotoColumn::PHOTOS_TABLE + " (" +
    PhotoColumn::PHOTO_DATE_MONTH + " DESC)";

const std::string PhotoUpgrade::CREATE_DAY_INDEX = BaseColumn::CreateIndex() +
    PhotoColumn::PHOTO_DATE_DAY_INDEX + " ON " + PhotoColumn::PHOTOS_TABLE + " (" +
    PhotoColumn::PHOTO_DATE_DAY + " DESC)";

const std::string PhotoUpgrade::CREATE_SCHPT_DAY_INDEX = BaseColumn::CreateIndex() +
    PhotoColumn::PHOTO_SCHPT_DAY_INDEX + " ON " +
    PhotoColumn::PHOTOS_TABLE + " (" + PhotoColumn::PHOTO_SYNC_STATUS + "," +
    PhotoColumn::PHOTO_CLEAN_FLAG + "," + PhotoColumn::MEDIA_DATE_TRASHED + "," +
    MediaColumn::MEDIA_TIME_PENDING + "," + MediaColumn::MEDIA_HIDDEN + "," +
    PhotoColumn::PHOTO_IS_TEMP + "," + PhotoColumn::PHOTO_BURST_COVER_LEVEL + "," +
    PhotoColumn::PHOTO_DATE_DAY + " DESC, " + PhotoColumn::PHOTO_THUMBNAIL_VISIBLE + ");";

const std::string PhotoUpgrade::DROP_SCHPT_DAY_INDEX = BaseColumn::DropIndex() + PhotoColumn::PHOTO_SCHPT_DAY_INDEX;

const std::string PhotoUpgrade::DROP_SCHPT_MEDIA_TYPE_INDEX = "DROP INDEX IF EXISTS " +
    PhotoColumn::PHOTO_SCHPT_MEDIA_TYPE_INDEX;

const std::string PhotoUpgrade::DROP_BURST_MODE_ALBUM_INDEX = BaseColumn::DropIndex() +
    PhotoColumn::PHOTO_BURST_MODE_ALBUM_INDEX;

const std::string PhotoUpgrade::CREATE_SCHPT_MEDIA_TYPE_INDEX = BaseColumn::CreateIndex() +
    PhotoColumn::PHOTO_SCHPT_MEDIA_TYPE_INDEX + " ON " + PhotoColumn::PHOTOS_TABLE +
    " (" + PhotoColumn::PHOTO_SYNC_STATUS + "," + PhotoColumn::PHOTO_CLEAN_FLAG + "," +
    MediaColumn::MEDIA_DATE_TRASHED + "," + MediaColumn::MEDIA_HIDDEN +
    "," + MediaColumn::MEDIA_TIME_PENDING + ", " + PhotoColumn::PHOTO_IS_TEMP + "," +
    PhotoColumn::PHOTO_BURST_COVER_LEVEL + "," + MediaColumn::MEDIA_TYPE + "," +
    MediaColumn::MEDIA_DATE_ADDED + " DESC, " + PhotoColumn::PHOTO_THUMBNAIL_VISIBLE + ");";

const std::string PhotoUpgrade::CREATE_SCHPT_YEAR_COUNT_READY_INDEX = BaseColumn::CreateIndex() +
    PhotoColumn::PHOTO_SCHPT_DATE_YEAR_COUNT_READY_INDEX + " ON " + PhotoColumn::PHOTOS_TABLE + " (" +
    PhotoColumn::PHOTO_SYNC_STATUS + "," + PhotoColumn::PHOTO_CLEAN_FLAG + "," + MediaColumn::MEDIA_DATE_TRASHED +
    "," + MediaColumn::MEDIA_TIME_PENDING + "," + MediaColumn::MEDIA_HIDDEN + "," + PhotoColumn::PHOTO_IS_TEMP +
    "," + PhotoColumn::PHOTO_BURST_COVER_LEVEL + "," + PhotoColumn::PHOTO_DATE_YEAR + " DESC, " +
    PhotoColumn::PHOTO_THUMBNAIL_VISIBLE + ");";

const std::string PhotoUpgrade::CREATE_SCHPT_MONTH_COUNT_READY_INDEX = BaseColumn::CreateIndex() +
    PhotoColumn::PHOTO_SCHPT_DATE_MONTH_COUNT_READY_INDEX + " ON " + PhotoColumn::PHOTOS_TABLE + " (" +
    PhotoColumn::PHOTO_SYNC_STATUS + "," + PhotoColumn::PHOTO_CLEAN_FLAG + "," + MediaColumn::MEDIA_DATE_TRASHED +
    "," + MediaColumn::MEDIA_TIME_PENDING + "," + MediaColumn::MEDIA_HIDDEN + "," + PhotoColumn::PHOTO_IS_TEMP +
    "," + PhotoColumn::PHOTO_BURST_COVER_LEVEL + "," + PhotoColumn::PHOTO_DATE_MONTH + " DESC, " +
    PhotoColumn::PHOTO_THUMBNAIL_VISIBLE + ");";

const std::string PhotoUpgrade::CREATE_SCHPT_MEDIA_TYPE_COUNT_READY_INDEX = BaseColumn::CreateIndex() +
    PhotoColumn::PHOTO_SCHPT_MEDIA_TYPE_COUNT_READY_INDEX + " ON " + PhotoColumn::PHOTOS_TABLE + " (" +
    PhotoColumn::PHOTO_SYNC_STATUS + "," + PhotoColumn::PHOTO_CLEAN_FLAG + "," + MediaColumn::MEDIA_DATE_TRASHED +
    "," + MediaColumn::MEDIA_TIME_PENDING + "," + MediaColumn::MEDIA_HIDDEN + ", " + PhotoColumn::PHOTO_IS_TEMP +
    "," + PhotoColumn::PHOTO_BURST_COVER_LEVEL + "," + MediaColumn::MEDIA_TYPE + "," +
    PhotoColumn::PHOTO_THUMBNAIL_VISIBLE + ");";

const std::string PhotoUpgrade::CREATE_SCHPT_CLOUD_ENHANCEMENT_ALBUM_INDEX =
    BaseColumn::CreateIndex() + PhotoColumn::PHOTO_SCHPT_CLOUD_ENHANCEMENT_ALBUM_INDEX + " ON " +
    PhotoColumn::PHOTOS_TABLE + " (" + PhotoColumn::PHOTO_SYNC_STATUS + "," + PhotoColumn::PHOTO_CLEAN_FLAG +
    "," + MediaColumn::MEDIA_HIDDEN + "," + MediaColumn::MEDIA_TIME_PENDING + "," +
    MediaColumn::MEDIA_DATE_TRASHED + "," + PhotoColumn::PHOTO_IS_TEMP + "," +
    PhotoColumn::PHOTO_STRONG_ASSOCIATION + "," + PhotoColumn::PHOTO_BURST_COVER_LEVEL +
    "," + MediaColumn::MEDIA_DATE_TAKEN + " DESC);";

const std::string PhotoUpgrade::DROP_SCHPT_YEAR_COUNT_READY_INDEX = "DROP INDEX IF EXISTS " +
    PhotoColumn::PHOTO_SCHPT_DATE_YEAR_COUNT_READY_INDEX;

const std::string PhotoUpgrade::DROP_SCHPT_MONTH_COUNT_READY_INDEX = "DROP INDEX IF EXISTS " +
    PhotoColumn::PHOTO_SCHPT_DATE_MONTH_COUNT_READY_INDEX;

const std::string PhotoUpgrade::DROP_SCHPT_MEDIA_TYPE_COUNT_READY_INDEX = "DROP INDEX IF EXISTS " +
    PhotoColumn::PHOTO_SCHPT_MEDIA_TYPE_COUNT_READY_INDEX;

const std::string PhotoUpgrade::CREATE_HIDDEN_TIME_INDEX = BaseColumn::CreateIndex() +
    PhotoColumn::PHOTO_HIDDEN_TIME_INDEX + " ON " + PhotoColumn::PHOTOS_TABLE + " (" +
    PhotoColumn::PHOTO_HIDDEN_TIME + " DESC)";

const std::string PhotoUpgrade::CREATE_SCHPT_HIDDEN_TIME_INDEX =
    BaseColumn::CreateIndex() + PhotoColumn::PHOTO_SCHPT_HIDDEN_TIME_INDEX + " ON " + PhotoColumn::PHOTOS_TABLE +
    " (" + PhotoColumn::PHOTO_SYNC_STATUS + "," + PhotoColumn::PHOTO_CLEAN_FLAG + "," +
    MediaColumn::MEDIA_HIDDEN + "," + MediaColumn::MEDIA_TIME_PENDING + "," + MediaColumn::MEDIA_DATE_TRASHED +
    "," + PhotoColumn::PHOTO_IS_TEMP + "," + PhotoColumn::PHOTO_BURST_COVER_LEVEL + "," +
    PhotoColumn::PHOTO_HIDDEN_TIME + " DESC, " + MediaColumn::MEDIA_TYPE + "," +
    PhotoColumn::PHOTO_OWNER_ALBUM_ID + ");";

const std::string PhotoUpgrade::DROP_SCHPT_HIDDEN_TIME_INDEX = BaseColumn::DropIndex() +
    PhotoColumn::PHOTO_SCHPT_HIDDEN_TIME_INDEX;

const std::string PhotoUpgrade::CREATE_PHOTO_FAVORITE_INDEX =
    BaseColumn::CreateIndex() + PhotoColumn::PHOTO_FAVORITE_INDEX + " ON " + PhotoColumn::PHOTOS_TABLE +
    " (" + PhotoColumn::PHOTO_SYNC_STATUS + "," + PhotoColumn::PHOTO_CLEAN_FLAG + "," +
    MediaColumn::MEDIA_HIDDEN + "," + MediaColumn::MEDIA_TIME_PENDING + "," + MediaColumn::MEDIA_DATE_TRASHED +
    "," + PhotoColumn::PHOTO_IS_TEMP + "," + MediaColumn::MEDIA_IS_FAV + "," +
    PhotoColumn::PHOTO_BURST_COVER_LEVEL + "," + MediaColumn::MEDIA_DATE_TAKEN + " DESC);";

const std::string PhotoUpgrade::DROP_PHOTO_FAVORITE_INDEX = BaseColumn::DropIndex() +
    PhotoColumn::PHOTO_FAVORITE_INDEX;

const std::string PhotoUpgrade::CREATE_PHOTO_DISPLAYNAME_INDEX = BaseColumn::CreateIndex() +
    PhotoColumn::PHOTO_DISPLAYNAME_INDEX + " ON " + PhotoColumn::PHOTOS_TABLE + " (" +
    MediaColumn::MEDIA_NAME + ")";

const std::string PhotoUpgrade::CREATE_PHOTO_BURSTKEY_INDEX = BaseColumn::CreateIndex() +
    PhotoColumn::PHOTO_BURSTKEY_INDEX + " ON " + PhotoColumn::PHOTOS_TABLE + " (" +
    PhotoColumn::PHOTO_BURST_KEY + "," + MediaColumn::MEDIA_TIME_PENDING  +
    "," + MediaColumn::MEDIA_NAME + " ASC);";

const std::string PhotoUpgrade::QUERY_MEDIA_VOLUME = "SELECT sum(" + MediaColumn::MEDIA_SIZE + ") AS " +
    MediaColumn::MEDIA_SIZE + "," +
    MediaColumn::MEDIA_TYPE + " FROM " +
    PhotoColumn::PHOTOS_TABLE + " WHERE " +
    "(" + MediaColumn::MEDIA_TYPE + " = " + std::to_string(MEDIA_TYPE_IMAGE) + " OR " +
    MediaColumn::MEDIA_TYPE + " = " + std::to_string(MEDIA_TYPE_VIDEO) + ") AND " +
    PhotoColumn::PHOTO_POSITION + " != 2" + " GROUP BY " +
    MediaColumn::MEDIA_TYPE;

// Create indexes
const std::string PhotoUpgrade::INDEX_SCTHP_ADDTIME =
    BaseColumn::CreateIndex() + PhotoColumn::PHOTO_SCHPT_ADDED_INDEX + " ON " + PhotoColumn::PHOTOS_TABLE +
    " (" + PhotoColumn::PHOTO_SYNC_STATUS + "," + PhotoColumn::PHOTO_CLEAN_FLAG + "," +
    MediaColumn::MEDIA_DATE_TRASHED + "," + MediaColumn::MEDIA_HIDDEN + "," +
    MediaColumn::MEDIA_TIME_PENDING + "," + PhotoColumn::PHOTO_IS_TEMP + "," +
    PhotoColumn::PHOTO_BURST_COVER_LEVEL + "," + MediaColumn::MEDIA_DATE_TAKEN + " DESC, " +
    MediaColumn::MEDIA_ID + " DESC);";

const std::string PhotoUpgrade::INDEX_SCHPT_ALBUM_GENERAL =
    BaseColumn::CreateIndex() + PhotoColumn::PHOTO_SCHPT_ALBUM_GENERAL_INDEX +
    " ON " + PhotoColumn::PHOTOS_TABLE + " (" + PhotoColumn::PHOTO_SYNC_STATUS + "," +
    PhotoColumn::PHOTO_CLEAN_FLAG + "," + MediaColumn::MEDIA_DATE_TRASHED + "," +
    MediaColumn::MEDIA_HIDDEN + "," + MediaColumn::MEDIA_TIME_PENDING + "," +
    PhotoColumn::PHOTO_IS_TEMP + "," + PhotoColumn::PHOTO_BURST_COVER_LEVEL +
    "," + PhotoColumn::PHOTO_OWNER_ALBUM_ID + ");";

const std::string PhotoUpgrade::INDEX_SCHPT_ALBUM =
    BaseColumn::CreateIndex() + PhotoColumn::PHOTO_SCHPT_ALBUM_INDEX + " ON " + PhotoColumn::PHOTOS_TABLE +
    " (" + PhotoColumn::PHOTO_SYNC_STATUS + "," + PhotoColumn::PHOTO_CLEAN_FLAG +
    "," + MediaColumn::MEDIA_DATE_TRASHED + "," + MediaColumn::MEDIA_HIDDEN + "," +
    MediaColumn::MEDIA_TIME_PENDING + "," + PhotoColumn::PHOTO_IS_TEMP + "," +
    PhotoColumn::PHOTO_BURST_COVER_LEVEL + "," + PhotoColumn::PHOTO_OWNER_ALBUM_ID + "," +
    MediaColumn::MEDIA_DATE_TAKEN + " DESC, " + MediaColumn::MEDIA_ID + " DESC);";

// Create dateadded index
const std::string PhotoUpgrade::INDEX_SCTHP_PHOTO_DATEADDED =
    BaseColumn::CreateIndex() + PhotoColumn::PHOTO_SCHPT_PHOTO_DATEADDED_INDEX + " ON " +
    PhotoColumn::PHOTOS_TABLE + " (" + PhotoColumn::PHOTO_SYNC_STATUS + "," +
    PhotoColumn::PHOTO_CLEAN_FLAG + "," + MediaColumn::MEDIA_DATE_TRASHED + "," +
    MediaColumn::MEDIA_HIDDEN + "," + MediaColumn::MEDIA_TIME_PENDING + "," +
    PhotoColumn::PHOTO_IS_TEMP + "," + PhotoColumn::PHOTO_BURST_COVER_LEVEL + "," +
    MediaColumn::MEDIA_DATE_ADDED + " DESC, " + PhotoColumn::PHOTO_THUMBNAIL_VISIBLE + ");";

const std::string PhotoUpgrade::INDEX_LATITUDE =
    BaseColumn::CreateIndex() + PhotoColumn::LATITUDE_INDEX + " ON " +
    PhotoColumn::PHOTOS_TABLE + " (" + PhotoColumn::PHOTO_LATITUDE + ");";

const std::string PhotoUpgrade::INDEX_LONGITUDE =
    BaseColumn::CreateIndex() + PhotoColumn::LONGITUDE_INDEX + " ON " +
    PhotoColumn::PHOTOS_TABLE + " (" + PhotoColumn::PHOTO_LONGITUDE + ");";

const std::string PhotoUpgrade::UPDATE_LATITUDE_AND_LONGITUDE_DEFAULT_NULL =
    " UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " + PhotoColumn::PHOTO_LATITUDE +
    " = NULL, " + PhotoColumn::PHOTO_LONGITUDE + " = NULL " + " WHERE " +
    PhotoColumn::PHOTO_LATITUDE + " = 0 AND " + PhotoColumn::PHOTO_LONGITUDE + " = 0;";

const std::string PhotoUpgrade::INDEX_QUERY_THUMBNAIL_WHITE_BLOCKS =
    BaseColumn::CreateIndex() + PhotoColumn::PHOTO_QUERY_THUMBNAIL_WHITE_BLOCKS_INDEX +
    " ON " + PhotoColumn::PHOTOS_TABLE + " (" + MediaColumn::MEDIA_TYPE + "," +
    PhotoColumn::PHOTO_SYNC_STATUS + "," + PhotoColumn::PHOTO_LCD_VISIT_TIME +
    "," + PhotoColumn::PHOTO_POSITION + "," + PhotoColumn::PHOTO_THUMB_STATUS + "," +
    PhotoColumn::PHOTO_CLEAN_FLAG + "," + PhotoColumn::PHOTO_THUMBNAIL_VISIBLE + " );";

const std::string PhotoUpgrade::UPDATE_PHOTO_QUALITY_OF_NULL_PHOTO_ID =
    " UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " + PhotoColumn::PHOTO_QUALITY + " = 0 WHERE " +
    PhotoColumn::PHOTO_QUALITY + " = 1 AND " + PhotoColumn::PHOTO_ID + " IS NULL;";

const std::string PhotoUpgrade::DROP_INDEX_SCTHP_ADDTIME = BaseColumn::DropIndex() +
    PhotoColumn::PHOTO_SCHPT_ADDED_INDEX;

const std::string PhotoUpgrade::DROP_INDEX_SCHPT_ADDTIME_ALBUM = BaseColumn::DropIndex() +
    PhotoColumn::PHOTO_SCHPT_ADDED_ALBUM_INDEX;

const std::string PhotoUpgrade::INDEX_CAMERA_SHOT_KEY =
    BaseColumn::CreateIndex() + "idx_camera_shot_key" + " ON " + PhotoColumn::PHOTOS_TABLE +
    " (" + PhotoColumn::CAMERA_SHOT_KEY + ");";

const std::string PhotoUpgrade::INDEX_SCHPT_READY = BaseColumn::CreateIndex() +
    PhotoColumn::PHOTO_SCHPT_READY_INDEX + " ON " + PhotoColumn::PHOTOS_TABLE +
    " (" + PhotoColumn::PHOTO_SYNC_STATUS + "," + PhotoColumn::PHOTO_CLEAN_FLAG +
    "," + MediaColumn::MEDIA_DATE_TRASHED + "," + MediaColumn::MEDIA_TIME_PENDING +
    ", " + MediaColumn::MEDIA_HIDDEN + "," + PhotoColumn::PHOTO_IS_TEMP + "," +
    PhotoColumn::PHOTO_BURST_COVER_LEVEL + "," + MediaColumn::MEDIA_DATE_TAKEN +
    " DESC, " + MediaColumn::MEDIA_ID + " DESC, " + PhotoColumn::PHOTO_THUMBNAIL_VISIBLE + ");";

const std::string PhotoUpgrade::DROP_INDEX_SCHPT_READY = BaseColumn::DropIndex() + PhotoColumn::PHOTO_SCHPT_READY_INDEX;

const std::string PhotoUpgrade::CREATE_PHOTO_SORT_MEDIA_TYPE_DATE_ADDED_INDEX = BaseColumn::CreateIndex() +
    PhotoColumn::PHOTO_SORT_MEDIA_TYPE_DATE_ADDED_INDEX + " ON " + PhotoColumn::PHOTOS_TABLE +
    " (" + PhotoColumn::PHOTO_SYNC_STATUS + "," + PhotoColumn::PHOTO_CLEAN_FLAG + "," +
    MediaColumn::MEDIA_DATE_TRASHED + "," + MediaColumn::MEDIA_HIDDEN +
    "," + MediaColumn::MEDIA_TIME_PENDING + "," + PhotoColumn::PHOTO_IS_TEMP +
    "," + PhotoColumn::PHOTO_BURST_COVER_LEVEL + "," + MediaColumn::MEDIA_TYPE +
    "," + MediaColumn::MEDIA_DATE_ADDED + " DESC," + MediaColumn::MEDIA_NAME +
    " DESC, " + PhotoColumn::PHOTO_STRONG_ASSOCIATION + ");";

const std::string PhotoUpgrade::CREATE_PHOTO_SORT_MEDIA_TYPE_DATE_TAKEN_INDEX = BaseColumn::CreateIndex() +
    PhotoColumn::PHOTO_SORT_MEDIA_TYPE_DATE_TAKEN_INDEX + " ON " + PhotoColumn::PHOTOS_TABLE +
    " (" + PhotoColumn::PHOTO_SYNC_STATUS + "," + PhotoColumn::PHOTO_CLEAN_FLAG +
    "," + MediaColumn::MEDIA_DATE_TRASHED + "," + MediaColumn::MEDIA_HIDDEN +
    "," + MediaColumn::MEDIA_TIME_PENDING + "," + PhotoColumn::PHOTO_IS_TEMP +
    "," + PhotoColumn::PHOTO_BURST_COVER_LEVEL + "," + MediaColumn::MEDIA_TYPE + "," +
    MediaColumn::MEDIA_DATE_TAKEN + " DESC," + MediaColumn::MEDIA_NAME +
    " DESC, " + PhotoColumn::PHOTO_STRONG_ASSOCIATION + ");";

const std::string PhotoUpgrade::CREATE_PHOTO_SORT_IN_ALBUM_DATE_ADDED_INDEX = BaseColumn::CreateIndex() +
    PhotoColumn::PHOTO_SORT_IN_ALBUM_DATE_ADDED_INDEX + " ON " + PhotoColumn::PHOTOS_TABLE +
    " (" + PhotoColumn::PHOTO_OWNER_ALBUM_ID + "," + MediaColumn::MEDIA_HIDDEN + "," +
    PhotoColumn::PHOTO_CLEAN_FLAG + "," +  PhotoColumn::PHOTO_SYNC_STATUS + "," +
    MediaColumn::MEDIA_DATE_TRASHED + "," + MediaColumn::MEDIA_TIME_PENDING + "," +
    PhotoColumn::PHOTO_IS_TEMP + "," + PhotoColumn::PHOTO_BURST_COVER_LEVEL + "," +
    MediaColumn::MEDIA_DATE_ADDED + " DESC," + MediaColumn::MEDIA_NAME + " DESC);";

const std::string PhotoUpgrade::CREATE_PHOTO_SORT_IN_ALBUM_DATE_TAKEN_INDEX = BaseColumn::CreateIndex() +
    PhotoColumn::PHOTO_SORT_IN_ALBUM_DATE_TAKEN_INDEX + " ON " + PhotoColumn::PHOTOS_TABLE +
    " (" + PhotoColumn::PHOTO_SYNC_STATUS + "," + PhotoColumn::PHOTO_CLEAN_FLAG + "," +
    MediaColumn::MEDIA_DATE_TRASHED + "," + MediaColumn::MEDIA_HIDDEN +
    "," + MediaColumn::MEDIA_TIME_PENDING + "," + PhotoColumn::PHOTO_IS_TEMP + "," +
    PhotoColumn::PHOTO_BURST_COVER_LEVEL + "," + PhotoColumn::PHOTO_OWNER_ALBUM_ID +
    "," + MediaColumn::MEDIA_DATE_TAKEN + " DESC," + MediaColumn::MEDIA_NAME + " DESC);";

const std::string PhotoUpgrade::CREATE_PHOTO_SORT_IN_ALBUM_SIZE_INDEX = BaseColumn::CreateIndex() +
    PhotoColumn::PHOTO_SORT_IN_ALBUM_SIZE_INDEX + " ON " + PhotoColumn::PHOTOS_TABLE +
    " (" + PhotoColumn::PHOTO_SYNC_STATUS + "," + PhotoColumn::PHOTO_CLEAN_FLAG + "," +
    MediaColumn::MEDIA_DATE_TRASHED + "," + MediaColumn::MEDIA_HIDDEN +
    "," + MediaColumn::MEDIA_TIME_PENDING + ", " + PhotoColumn::PHOTO_IS_TEMP + "," +
    PhotoColumn::PHOTO_BURST_COVER_LEVEL + "," + PhotoColumn::PHOTO_OWNER_ALBUM_ID +
    "," + MediaColumn::MEDIA_SIZE + " DESC," + MediaColumn::MEDIA_ID + " DESC);";

const std::string PhotoUpgrade::CREATE_PHOTO_SORT_MEDIA_TYPE_SIZE_INDEX = BaseColumn::CreateIndex() +
    PhotoColumn::PHOTO_SORT_MEDIA_TYPE_SIZE_INDEX + " ON " + PhotoColumn::PHOTOS_TABLE +
    " (" + PhotoColumn::PHOTO_SYNC_STATUS + "," + PhotoColumn::PHOTO_CLEAN_FLAG + "," +
    MediaColumn::MEDIA_DATE_TRASHED + "," + MediaColumn::MEDIA_HIDDEN +
    "," + MediaColumn::MEDIA_TIME_PENDING + ", " + PhotoColumn::PHOTO_IS_TEMP + "," +
    PhotoColumn::PHOTO_BURST_COVER_LEVEL + "," + MediaColumn::MEDIA_TYPE +
    "," + MediaColumn::MEDIA_SIZE + " DESC," + MediaColumn::MEDIA_ID + " DESC," +
    PhotoColumn::PHOTO_STRONG_ASSOCIATION + ");";

const std::string PhotoUpgrade::CREATE_PHOTO_SORT_IN_ALBUM_DISPLAY_NAME_INDEX = BaseColumn::CreateIndex() +
    PhotoColumn::PHOTO_SORT_IN_ALBUM_DISPLAY_NAME_INDEX + " ON " + PhotoColumn::PHOTOS_TABLE +
    " (" + PhotoColumn::PHOTO_SYNC_STATUS + "," + PhotoColumn::PHOTO_CLEAN_FLAG + "," +
    MediaColumn::MEDIA_DATE_TRASHED + "," + MediaColumn::MEDIA_HIDDEN +
    "," + MediaColumn::MEDIA_TIME_PENDING + ", " + PhotoColumn::PHOTO_IS_TEMP + "," +
    PhotoColumn::PHOTO_BURST_COVER_LEVEL + "," + PhotoColumn::PHOTO_OWNER_ALBUM_ID +
    "," + MediaColumn::MEDIA_NAME + " DESC);";

const std::string PhotoUpgrade::CREATE_PHOTO_SORT_MEDIA_TYPE_DISPLAY_NAME_INDEX = BaseColumn::CreateIndex() +
    PhotoColumn::PHOTO_SORT_MEDIA_TYPE_DISPLAY_NAME_INDEX + " ON " + PhotoColumn::PHOTOS_TABLE +
    " (" + PhotoColumn::PHOTO_SYNC_STATUS + "," + PhotoColumn::PHOTO_CLEAN_FLAG +
    "," + MediaColumn::MEDIA_DATE_TRASHED + "," + MediaColumn::MEDIA_HIDDEN +
    "," + MediaColumn::MEDIA_TIME_PENDING + ", " + PhotoColumn::PHOTO_IS_TEMP +
    "," + PhotoColumn::PHOTO_BURST_COVER_LEVEL + "," + MediaColumn::MEDIA_TYPE +
    "," + MediaColumn::MEDIA_NAME + " DESC," + PhotoColumn::PHOTO_STRONG_ASSOCIATION + ");";

const std::string PhotoUpgrade::CREATE_PHOTO_SHOOTING_MODE_ALBUM_GENERAL_INDEX = BaseColumn::CreateIndex() +
    PhotoColumn::PHOTO_SHOOTING_MODE_ALBUM_GENERAL_INDEX + " ON " + PhotoColumn::PHOTOS_TABLE +
    " (" + PhotoColumn::PHOTO_SYNC_STATUS + "," + PhotoColumn::PHOTO_CLEAN_FLAG +
    "," + MediaColumn::MEDIA_DATE_TRASHED + "," + MediaColumn::MEDIA_HIDDEN +
    "," + MediaColumn::MEDIA_TIME_PENDING + ", " + PhotoColumn::PHOTO_IS_TEMP +
    "," + PhotoColumn::PHOTO_BURST_COVER_LEVEL + "," + PhotoColumn::PHOTO_SHOOTING_MODE +
    "," + MediaColumn::MEDIA_DATE_TAKEN + " DESC," + MediaColumn::MEDIA_NAME + " DESC);";

const std::string PhotoUpgrade::CREATE_PHOTO_BURST_MODE_ALBUM_INDEX = BaseColumn::CreateIndex() +
    PhotoColumn::PHOTO_BURST_MODE_ALBUM_INDEX + " ON " + PhotoColumn::PHOTOS_TABLE +
    " (" + PhotoColumn::PHOTO_SYNC_STATUS + "," + PhotoColumn::PHOTO_CLEAN_FLAG +
    "," + MediaColumn::MEDIA_DATE_TRASHED + "," + MediaColumn::MEDIA_HIDDEN +
    "," + MediaColumn::MEDIA_TIME_PENDING + ", " + PhotoColumn::PHOTO_IS_TEMP +
    "," + PhotoColumn::PHOTO_BURST_COVER_LEVEL + "," + PhotoColumn::PHOTO_SUBTYPE +
    "," + MediaColumn::MEDIA_DATE_TAKEN + " DESC," + MediaColumn::MEDIA_NAME +
    " DESC," + PhotoColumn::PHOTO_BURST_KEY + ");";

const std::string PhotoUpgrade::CREATE_PHOTO_FRONT_CAMERA_ALBUM_INDEX = BaseColumn::CreateIndex() +
    PhotoColumn::PHOTO_FRONT_CAMERA_ALBUM_INDEX + " ON " + PhotoColumn::PHOTOS_TABLE +
    " (" + PhotoColumn::PHOTO_SYNC_STATUS + "," + PhotoColumn::PHOTO_CLEAN_FLAG +
    "," + MediaColumn::MEDIA_DATE_TRASHED + "," + MediaColumn::MEDIA_HIDDEN +
    "," + MediaColumn::MEDIA_TIME_PENDING + ", " + PhotoColumn::PHOTO_IS_TEMP +
    "," + PhotoColumn::PHOTO_BURST_COVER_LEVEL + "," + PhotoColumn::PHOTO_FRONT_CAMERA +
    "," + MediaColumn::MEDIA_DATE_TAKEN + " DESC," + MediaColumn::MEDIA_NAME + " DESC);";

const std::string PhotoUpgrade::CREATE_PHOTO_RAW_IMAGE_ALBUM_INDEX = BaseColumn::CreateIndex() +
    PhotoColumn::PHOTO_RAW_IMAGE_ALBUM_INDEX + " ON " + PhotoColumn::PHOTOS_TABLE +
    " (" + PhotoColumn::PHOTO_SYNC_STATUS + "," + PhotoColumn::PHOTO_CLEAN_FLAG +
    "," + MediaColumn::MEDIA_DATE_TRASHED + "," + MediaColumn::MEDIA_HIDDEN +
    "," + MediaColumn::MEDIA_TIME_PENDING + ", " + PhotoColumn::PHOTO_IS_TEMP +
    "," + PhotoColumn::PHOTO_BURST_COVER_LEVEL + "," + MediaColumn::MEDIA_MIME_TYPE +
    "," + MediaColumn::MEDIA_DATE_TAKEN + " DESC," + MediaColumn::MEDIA_NAME + " DESC);";

const std::string PhotoUpgrade::CREATE_PHOTO_MOVING_PHOTO_ALBUM_INDEX = BaseColumn::CreateIndex() +
    PhotoColumn::PHOTO_MOVING_PHOTO_ALBUM_INDEX + " ON " + PhotoColumn::PHOTOS_TABLE +
    " (" + PhotoColumn::PHOTO_SYNC_STATUS + "," + PhotoColumn::PHOTO_CLEAN_FLAG +
    "," + MediaColumn::MEDIA_DATE_TRASHED + "," + MediaColumn::MEDIA_HIDDEN +
    "," + MediaColumn::MEDIA_TIME_PENDING + ", " + PhotoColumn::PHOTO_IS_TEMP +
    "," + PhotoColumn::PHOTO_BURST_COVER_LEVEL +
    "," + MediaColumn::MEDIA_DATE_TAKEN + " DESC," + MediaColumn::MEDIA_NAME + " DESC) "
    "WHERE (subtype = " + to_string(static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)) +
        " OR (moving_photo_effect_mode = " + to_string(static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY)) +
            " AND subtype = " + to_string(static_cast<int32_t>(PhotoSubType::DEFAULT)) + "));";

const std::string PhotoUpgrade::CREATE_PHOTOS_DELETE_TRIGGER =
                        "CREATE TRIGGER IF NOT EXISTS photos_delete_trigger AFTER UPDATE ON " +
                        PhotoColumn::PHOTOS_TABLE + " FOR EACH ROW WHEN new." + PhotoColumn::PHOTO_DIRTY +
                        " = " + std::to_string(static_cast<int32_t>(DirtyTypes::TYPE_DELETED)) +
                        " AND OLD." + PhotoColumn::PHOTO_POSITION + " = 1 AND is_caller_self_func() = 'true'" +
                        " AND OLD." + PhotoColumn::PHOTO_FILE_SOURCE_TYPE + " <> " +
                        to_string(static_cast<int32_t>(FileSourceTypes::TEMP_FILE_MANAGER)) +
                        " BEGIN DELETE FROM " + PhotoColumn::PHOTOS_TABLE +
                        " WHERE " + PhotoColumn::MEDIA_ID + " = old." + PhotoColumn::MEDIA_ID + ";" +
                        " END;";

const std::string PhotoUpgrade::CREATE_PHOTOS_FDIRTY_TRIGGER =
                        "CREATE TRIGGER IF NOT EXISTS photos_fdirty_trigger AFTER UPDATE ON " +
                        PhotoColumn::PHOTOS_TABLE + " FOR EACH ROW WHEN OLD.position <> 1 AND" +
                        " new.date_modified <> old.date_modified " +
                        " AND new.dirty = old.dirty AND is_caller_self_func() = 'true'" +
                        " AND OLD." + PhotoColumn::PHOTO_FILE_SOURCE_TYPE + " <> " +
                        to_string(static_cast<int32_t>(FileSourceTypes::TEMP_FILE_MANAGER)) +
                        " BEGIN " +
                        " UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET dirty = " +
                        std::to_string(static_cast<int32_t>(DirtyTypes::TYPE_FDIRTY)) +
                        " WHERE file_id = old.file_id;" +
                        " SELECT cloud_sync_func(); " +
                        " END;";

const std::string PhotoUpgrade::CREATE_PHOTOS_MDIRTY_TRIGGER =
                        "CREATE TRIGGER IF NOT EXISTS photos_mdirty_trigger AFTER UPDATE ON " +
                        PhotoColumn::PHOTOS_TABLE + " FOR EACH ROW WHEN OLD.position <> 1" +
                        " AND new.date_modified = old.date_modified AND ( old.dirty = " +
                        std::to_string(static_cast<int32_t>(DirtyTypes::TYPE_SYNCED)) + " OR old.dirty =" +
                        std::to_string(static_cast<int32_t>(DirtyTypes::TYPE_SDIRTY)) + " OR old.dirty =" +
                        std::to_string(static_cast<int32_t>(DirtyTypes::TYPE_TDIRTY)) +
                        ") AND new.dirty = old.dirty AND is_caller_self_func() = 'true'" +
                        " AND " + PhotoColumn::CheckUploadPhotoColumns() +
                        " AND OLD." + PhotoColumn::PHOTO_FILE_SOURCE_TYPE + " <> " +
                        to_string(static_cast<int32_t>(FileSourceTypes::TEMP_FILE_MANAGER)) +
                        " BEGIN " +
                        " UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET dirty = " +
                        std::to_string(static_cast<int32_t>(DirtyTypes::TYPE_MDIRTY)) +
                        " WHERE file_id = old.file_id;" +
                        " SELECT cloud_sync_func(); " +
                        " END;";

const std::string PhotoUpgrade::INSERT_GENERATE_HIGHLIGHT_THUMBNAIL =
                        "CREATE TRIGGER IF NOT EXISTS insert_generate_highlight_thumbnail_trigger AFTER INSERT ON " +
                        PhotoColumn::HIGHLIGHT_TABLE + " BEGIN SELECT begin_generate_highlight_thumbnail " +
                        "(NEW." + MEDIA_DATA_DB_ID + ", NEW." + MEDIA_DATA_DB_VIDEO_TRACKS +
                        ", NEW." + PhotoColumn::MEDIA_DATA_DB_HIGHLIGHT_TRIGGER + ", '" +
                        MEDIA_DATA_DB_INSERT_TYPE + "'); END;";

const std::string PhotoUpgrade::UPDATE_GENERATE_HIGHLIGHT_THUMBNAIL =
                        "CREATE TRIGGER IF NOT EXISTS update_generate_highlight_thumbnail_trigger AFTER UPDATE ON " +
                        PhotoColumn::HIGHLIGHT_TABLE + " FOR EACH ROW " + " WHEN OLD." +
                        PhotoColumn::MEDIA_DATA_DB_HIGHLIGHT_TRIGGER + "= 1 " + "AND NEW." +
                        PhotoColumn::MEDIA_DATA_DB_HIGHLIGHT_TRIGGER +
                        "= 0 BEGIN SELECT begin_generate_highlight_thumbnail " +
                        "(NEW." + MEDIA_DATA_DB_ID + ", NEW." + MEDIA_DATA_DB_VIDEO_TRACKS +
                        ", NEW." + PhotoColumn::MEDIA_DATA_DB_HIGHLIGHT_TRIGGER + ", '" +
                        MEDIA_DATA_DB_UPDATE_TYPE + "'); END;";

const std::string PhotoUpgrade::INDEX_HIGHLIGHT_FILEID =
                        BaseColumn::CreateIndex() + MEDIA_DATA_DB_HIGHLIGHT_INDEX + " ON " +
                        PhotoColumn::HIGHLIGHT_TABLE + " (" + MEDIA_DATA_DB_ID + ");";

const std::string  PhotoUpgrade::CREATE_PHOTOS_INSERT_CLOUD_SYNC =
                        " CREATE TRIGGER IF NOT EXISTS photo_insert_cloud_sync_trigger AFTER INSERT ON " +
                        PhotoColumn::PHOTOS_TABLE +
                        " WHEN new." + PhotoColumn::PHOTO_FILE_SOURCE_TYPE + " <> " +
                        to_string(static_cast<int32_t>(FileSourceTypes::TEMP_FILE_MANAGER)) +
                        " BEGIN SELECT cloud_sync_func(); END;";

const std::string PhotoUpgrade::CREATE_PHOTOS_UPDATE_CLOUD_SYNC =
                        " CREATE TRIGGER IF NOT EXISTS photo_update_cloud_sync_trigger AFTER UPDATE ON " +
                        PhotoColumn::PHOTOS_TABLE +
                        " FOR EACH ROW WHEN OLD.dirty IN (1,2,3,5) AND new.dirty != " +
                        std::to_string(static_cast<int32_t>(DirtyTypes::TYPE_SYNCED)) +
                        " AND OLD." + PhotoColumn::PHOTO_FILE_SOURCE_TYPE + " <> " +
                        to_string(static_cast<int32_t>(FileSourceTypes::TEMP_FILE_MANAGER)) +
                        " BEGIN SELECT cloud_sync_func(); END;";

const std::string PhotoUpgrade::UPDATE_READY_ON_THUMBNAIL_UPGRADE =
                        " UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " + PhotoColumn::PHOTO_THUMBNAIL_READY +
                        " = 6 " + " WHERE " + PhotoColumn::PHOTO_THUMBNAIL_READY + " != 0; END;";

const std::string PhotoUpgrade::CREATE_PHOTOS_METADATA_DIRTY_TRIGGER =
                        "CREATE TRIGGER IF NOT EXISTS photos_metadata_dirty_trigger AFTER UPDATE ON " +
                        PhotoColumn::PHOTOS_TABLE + " FOR EACH ROW WHEN old." +
                        PhotoColumn::PHOTO_POSITION + " != 2" +
                        " AND old.metadata_flags = " +
                        std::to_string(static_cast<int32_t>(MetadataFlags::TYPE_UPTODATE)) +
                        " AND new.metadata_flags = old.metadata_flags" +
                        " AND " + PhotoColumn::CheckMetaRecoveryPhotoColumns() +
                        " AND OLD." + PhotoColumn::PHOTO_FILE_SOURCE_TYPE + " <> " +
                        to_string(static_cast<int32_t>(FileSourceTypes::TEMP_FILE_MANAGER)) +
                        " BEGIN " +
                        " UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET metadata_flags = " +
                        std::to_string(static_cast<int32_t>(MetadataFlags::TYPE_DIRTY)) +
                        " WHERE file_id = old.file_id;" +
                        " END;";

const std::string PhotoUpgrade::UPDATA_PHOTOS_DATA_UNIQUE = "CREATE UNIQUE INDEX IF NOT EXISTS photo_data_index ON " +
    PhotoColumn::PHOTOS_TABLE + " (" + MediaColumn::MEDIA_FILE_PATH + ");";

const std::string PhotoUpgrade::UPDATE_LCD_STATUS_NOT_UPLOADED =
                        " UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " + PhotoColumn::PHOTO_LCD_VISIT_TIME +
                        " = 0 " + " WHERE " + PhotoColumn::PHOTO_DIRTY + " = 1; END;";

const std::string MediaUpgrade::ASSETS_QUERY_FILTER =
    PhotoColumn::PHOTO_SYNC_STATUS + " = 0" + " AND " +
    MediaColumn::MEDIA_DATE_TRASHED + " = 0" + " AND " +
    MediaColumn::MEDIA_HIDDEN + " = 0" + " AND " +
    MediaColumn::MEDIA_TIME_PENDING + " = 0 ";

const std::string PhotoExtUpgrade::CREATE_PHOTO_EXT_TABLE =
    "CREATE TABLE IF NOT EXISTS " +
    PhotoExtColumn::PHOTOS_EXT_TABLE + " (" +
    PhotoExtColumn::PHOTO_ID + " INTEGER PRIMARY KEY, " +
    PhotoExtColumn::THUMBNAIL_SIZE + " BIGINT DEFAULT 0)";

// For Photos table query filter
const std::string PhotoUpgrade::PHOTOS_QUERY_FILTER =
    MediaColumn::MEDIA_DATE_TRASHED + " = 0" + " AND " +
    MediaColumn::MEDIA_HIDDEN + " = 0" + " AND " +
    MediaColumn::MEDIA_TIME_PENDING + " = 0" + " AND " +
    PhotoColumn::PHOTO_IS_TEMP + " = 0" + " AND " +
    PhotoColumn::PHOTO_BURST_COVER_LEVEL + " = 1 ";
}  // namespace Media
}  // namespace OHOS