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

#ifndef NOTIFYCHANGE_FUZZER_H
#define NOTIFYCHANGE_FUZZER_H

#define FUZZ_PROJECT_NAME "medialibraryrefresh_fuzzer"

#include "notify_info_inner.h"
#include "media_datashare_stub_impl.h"
#include "media_log.h"
#include "i_observer_manager_interface.h"
#include "media_observer_manager.h"
#include "notify_register_permission.h"
#include "observer_callback_recipient.h"
#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {

const int32_t ASSET_FILE_ID = 0;
const string ASSET_URI = "uri";
const string ASSET_DATE_DAY = "20250525";
const int32_t ASSET_MEDIA_TYPE_IMAGE = static_cast<int32_t>(MEDIA_TYPE_IMAGE);
const int32_t ASSET_STRONG_ASSOCIATION_NORMAL = static_cast<int32_t>(StrongAssociationType::NORMAL);
const int32_t ASSET_THUMBNAIL_VISIBLE = 1;
const int64_t ASSET_DATE_ADDED = 123456;
const int64_t ASSET_DATE_TAKEN = 123456;
const int32_t ASSET_SUBTYPE_DEFAULT = static_cast<int32_t>(PhotoSubType::DEFAULT);
const int32_t ASSET_SYNC_STATUS_VISIBLE = static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE);
const int32_t ASSET_CLEAN_FLAG_NO = static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN);
const int32_t ASSET_BURST_COVER_LEVEL = static_cast<int32_t>(BurstCoverLevelType::COVER);
const string ASSET_DISPLAY_NAME = "asset_display_name";
const string ASSET_PATH = "asset_path";

const std::string CREATE_PHOTO_ALBUM_TABLE = "CREATE TABLE IF NOT EXISTS " +
    PhotoAlbumColumns::TABLE + " (" +
    PhotoAlbumColumns::ALBUM_ID + " INT, " +
    PhotoAlbumColumns::ALBUM_TYPE + " INT, " +
    PhotoAlbumColumns::ALBUM_SUBTYPE + " INT, " +
    PhotoAlbumColumns::ALBUM_NAME + " TEXT COLLATE NOCASE, " +
    PhotoAlbumColumns::ALBUM_COVER_URI + " TEXT, " +
    PhotoAlbumColumns::ALBUM_COUNT + " INT DEFAULT 0, " +
    PhotoAlbumColumns::ALBUM_DATE_MODIFIED + " BIGINT DEFAULT 0, " +
    PhotoAlbumColumns::ALBUM_DIRTY + " INT DEFAULT " +
        std::to_string(static_cast<int32_t>(DirtyTypes::TYPE_NEW)) + ", " +
    PhotoAlbumColumns::ALBUM_CLOUD_ID + " TEXT, " +
    PhotoAlbumColumns::ALBUM_RELATIVE_PATH + " TEXT, " +
    PhotoAlbumColumns::CONTAINS_HIDDEN + " INT DEFAULT 0, " +
    PhotoAlbumColumns::HIDDEN_COUNT + " INT DEFAULT 0, " +
    PhotoAlbumColumns::HIDDEN_COVER + " TEXT DEFAULT '', " +
    PhotoAlbumColumns::ALBUM_ORDER + " INT," +
    PhotoAlbumColumns::ALBUM_IMAGE_COUNT + " INT DEFAULT 0, " +
    PhotoAlbumColumns::ALBUM_VIDEO_COUNT + " INT DEFAULT 0, " +
    PhotoAlbumColumns::ALBUM_BUNDLE_NAME + " TEXT, " +
    PhotoAlbumColumns::ALBUM_LOCAL_LANGUAGE + " TEXT, " +
    PhotoAlbumColumns::ALBUM_IS_LOCAL + " INT, " +
    PhotoAlbumColumns::ALBUM_DATE_ADDED + " BIGINT DEFAULT 0, " +
    PhotoAlbumColumns::ALBUM_LPATH + " TEXT, " +
    PhotoAlbumColumns::ALBUM_PRIORITY + " INT, " +
    PhotoAlbumColumns::ALBUM_CHECK_FLAG + " INT DEFAULT 0, " +
    PhotoAlbumColumns::COVER_DATE_TIME + " BIGINT DEFAULT 0, " +
    PhotoAlbumColumns::COVER_URI_SOURCE + " INT DEFAULT 0, " +
    PhotoAlbumColumns::HIDDEN_COVER_DATE_TIME + " BIGINT DEFAULT 0, " +
    PhotoAlbumColumns::ALBUMS_ORDER + " INT DEFAULT -1, " +
    PhotoAlbumColumns::ORDER_SECTION + " INT DEFAULT -1)";

const std::string CREATE_PHOTO_TABLE = "CREATE TABLE IF NOT EXISTS " +
    PhotoColumn::PHOTOS_TABLE + " (" +
    PhotoColumn::MEDIA_ID + " INT, " +
    PhotoColumn::MEDIA_FILE_PATH + " TEXT, " +
    PhotoColumn::MEDIA_SIZE + " BIGINT, " +
    PhotoColumn::MEDIA_TITLE + " TEXT, " +
    PhotoColumn::MEDIA_NAME + " TEXT, " +
    PhotoColumn::MEDIA_TYPE + " INT, " +
    PhotoColumn::MEDIA_MIME_TYPE + " TEXT, " +
    PhotoColumn::MEDIA_OWNER_PACKAGE + " TEXT, " +
    PhotoColumn::MEDIA_OWNER_APPID + " TEXT, " +
    PhotoColumn::MEDIA_PACKAGE_NAME + " TEXT, " +
    PhotoColumn::MEDIA_DEVICE_NAME + " TEXT, " +
    PhotoColumn::MEDIA_DATE_ADDED + " BIGINT, " +
    PhotoColumn::MEDIA_DATE_MODIFIED + " BIGINT, " +
    PhotoColumn::MEDIA_DATE_TAKEN + " BIGINT DEFAULT 0, " +
    PhotoColumn::MEDIA_DURATION + " INT, " +
    PhotoColumn::MEDIA_TIME_PENDING + " BIGINT DEFAULT 0, " +
    PhotoColumn::MEDIA_IS_FAV + " INT DEFAULT 0, " +
    PhotoColumn::MEDIA_DATE_TRASHED + " BIGINT DEFAULT 0, " +
    PhotoColumn::MEDIA_DATE_DELETED + " BIGINT DEFAULT 0, " +
    PhotoColumn::MEDIA_HIDDEN + " INT DEFAULT 0, " +
    PhotoColumn::MEDIA_PARENT_ID + " INT DEFAULT 0, " +
    PhotoColumn::MEDIA_RELATIVE_PATH + " TEXT, " +
    PhotoColumn::MEDIA_VIRTUAL_PATH + " TEXT UNIQUE, " +
    PhotoColumn::PHOTO_DIRTY + " INT DEFAULT 1, " +
    PhotoColumn::PHOTO_CLOUD_ID + " TEXT, " +
    PhotoColumn::PHOTO_META_DATE_MODIFIED + "  BIGINT DEFAULT 0, " +
    PhotoColumn::PHOTO_SYNC_STATUS + "  INT DEFAULT 0, " +
    PhotoColumn::PHOTO_CLOUD_VERSION + " BIGINT DEFAULT 0, " +
    PhotoColumn::PHOTO_ORIENTATION + " INT DEFAULT 0, " +
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
    PhotoColumn::PHOTO_THUMBNAIL_READY + " BIGINT DEFAULT 0, " +
    PhotoColumn::PHOTO_LCD_SIZE + " TEXT, " +
    PhotoColumn::PHOTO_THUMB_SIZE + " TEXT," +
    PhotoColumn::PHOTO_FRONT_CAMERA + " TEXT, " +
    PhotoColumn::PHOTO_IS_TEMP + " INT DEFAULT 0," +
    PhotoColumn::PHOTO_BURST_COVER_LEVEL + " INT DEFAULT 1, " +
    PhotoColumn::PHOTO_BURST_KEY + " TEXT, " +
    PhotoColumn::PHOTO_CE_AVAILABLE + " INT DEFAULT 0, " +
    PhotoColumn::PHOTO_CE_STATUS_CODE + " INT, " +
    PhotoColumn::PHOTO_STRONG_ASSOCIATION + " INT DEFAULT 0, " +
    PhotoColumn::PHOTO_ASSOCIATE_FILE_ID + " INT DEFAULT 0, " +
    PhotoColumn::PHOTO_HAS_CLOUD_WATERMARK + " INT DEFAULT 0, " +
    PhotoColumn::PHOTO_DETAIL_TIME + " TEXT, " +
    PhotoColumn::PHOTO_OWNER_ALBUM_ID + " INT DEFAULT 0, " +
    PhotoColumn::PHOTO_ORIGINAL_ASSET_CLOUD_ID + " TEXT, " +
    PhotoColumn::PHOTO_THUMBNAIL_VISIBLE + " INT DEFAULT 0, " +
    PhotoColumn::PHOTO_SOURCE_PATH + " TEXT, " +
    PhotoColumn::SUPPORTED_WATERMARK_TYPE + " INT, " +
    PhotoColumn::PHOTO_METADATA_FLAGS + " INT DEFAULT 0, " +
    PhotoColumn::PHOTO_CHECK_FLAG + " INT DEFAULT 0, " +
    PhotoColumn::STAGE_VIDEO_TASK_STATUS + " INT NOT NULL DEFAULT 0, " +
    PhotoColumn::PHOTO_IS_AUTO + " INT NOT NULL DEFAULT 0, " +
    PhotoColumn::PHOTO_MEDIA_SUFFIX + " TEXT, " +
    PhotoColumn::PHOTO_IS_RECENT_SHOW + " INT NOT NULL DEFAULT 1, " +
    PhotoColumn::PHOTO_FILE_SOURCE_TYPE + " INT NOT NULL DEFAULT 0)";

const AccurateRefresh::PhotoAssetChangeInfo NORMAL_ASSET = { ASSET_FILE_ID, ASSET_URI, ASSET_DATE_DAY,
    "uri", // owner album uri
    false, // isFavorite
    ASSET_MEDIA_TYPE_IMAGE, // default image
    false, // isHidden
    0,  // dateTrash
    ASSET_STRONG_ASSOCIATION_NORMAL,
    ASSET_THUMBNAIL_VISIBLE, ASSET_DATE_ADDED, ASSET_DATE_TAKEN, ASSET_SUBTYPE_DEFAULT, ASSET_SYNC_STATUS_VISIBLE,
    ASSET_CLEAN_FLAG_NO,
    0, // timePending
    false, // isTemp
    ASSET_BURST_COVER_LEVEL,
    0, // owner album id
    0, // hidden time
    0,
    ASSET_DISPLAY_NAME,
    ASSET_PATH,
    1, //local
    666, //size
    0 //fileSourceType
};

const std::vector<PhotoAlbumSubType> PHOTO_ALBUM_SUB_TYPE = {
    PhotoAlbumSubType::FAVORITE,
    PhotoAlbumSubType::TRASH,
    PhotoAlbumSubType::HIDDEN,
};
} // namespace Media
} // namespace OHOS
#endif