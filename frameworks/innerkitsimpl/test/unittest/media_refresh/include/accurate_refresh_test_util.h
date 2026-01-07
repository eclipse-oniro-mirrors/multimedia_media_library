/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MEDIALIBRARY_ACCURATE_REFRESH_TEST_UTIL_H
#define MEDIALIBRARY_ACCURATE_REFRESH_TEST_UTIL_H

#include <string>

#include "values_bucket.h"
#include "medialibrary_type_const.h"
#include "userfile_manager_types.h"
#include "album_change_info.h"
#include "photo_asset_change_info.h"
#include "medialibrary_rdbstore.h"
#include "photo_album_column.h"
#include "media_column.h"

namespace OHOS {
namespace Media::AccurateRefresh {

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
    PhotoColumn::MEDIA_VIRTURL_PATH + " TEXT UNIQUE, " +
    PhotoColumn::PHOTO_DIRTY + " INT DEFAULT 1, " +
    PhotoColumn::PHOTO_CLOUD_ID + " TEXT, " +
    PhotoColumn::PHOTO_META_DATE_MODIFIED + "  BIGINT DEFAULT 0, " +
    PhotoColumn::PHOTO_SYNC_STATUS + "  INT DEFAULT 0, " +
    PhotoColumn::PHOTO_CLOUD_VERSION + " BIGINT DEFAULT 0, " +
    PhotoColumn::PHOTO_ORIENTATION + " INT DEFAULT 0, " +
    PhotoColumn::PHOTO_EXIF_ROTATE + " INT DEFAULT 0, " +
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
    PhotoColumn::PHOTO_COMPOSITE_DISPLAY_STATUS + " INT NOT NULL DEFAULT 0, " +
    PhotoColumn::PHOTO_FILE_SOURCE_TYPE + " INT NOT NULL DEFAULT 0)";

const int32_t FAVORITE_ALBUM_ID = 1;
const int32_t FAVORITE_ALBUM_ID_TOW = 101;
const std::string FAVORITE_ALBUM_LPATH = "favorite_lpath";
const int32_t FAVORITE_ALBUM_IMAGE_COUNT = 1111;
const int32_t FAVORITE_ALBUM_VIDEO_COUNT = 2222;
const std::string FAVORITE_ALBUM_NAME = "favorite_albumName";
const std::string FAVORITE_ALBUM_URI = PhotoAlbumColumns::ALBUM_URI_PREFIX + std::to_string(FAVORITE_ALBUM_ID);
const int32_t FAVORITE_ALBUM_COUNT = 3333;
const std::string FAVORITE_ALBUM_COVER_URI = "file://media/Photo/123/vio/asdf.jpg";
const int32_t FAVORITE_ALBUM_HIDDEN_COUNT = 4444;
const std::string FAVORITE_ALBUM_HIDDEN_COVER_URI = "file://media/Photo/234/vio/asdf.jpg";
const int64_t FAVORITE_ALBUM_COVER_DATE_TIME = 111111;
const int64_t FAVORITE_ALBUM_HIDDEN_COVER_DATE_TIME = 111111;
const int64_t FAVORITE_ALBUMS_ORDER = -1;
const int64_t FAVORITE_ORDER_SECTION = -1;
const int32_t FAVORITE_ALBUM_HIDDEN = 0;
const AlbumChangeInfo FAVORITE_ALBUM_INFO = {
    FAVORITE_ALBUM_ID, FAVORITE_ALBUM_LPATH, FAVORITE_ALBUM_IMAGE_COUNT, FAVORITE_ALBUM_VIDEO_COUNT,
    PhotoAlbumType::SYSTEM, PhotoAlbumSubType::FAVORITE, FAVORITE_ALBUM_NAME, FAVORITE_ALBUM_URI,
    FAVORITE_ALBUM_COUNT, FAVORITE_ALBUM_COVER_URI, FAVORITE_ALBUM_HIDDEN_COUNT, FAVORITE_ALBUM_HIDDEN_COVER_URI,
    false, false, FAVORITE_ALBUM_COVER_DATE_TIME, FAVORITE_ALBUM_HIDDEN_COVER_DATE_TIME,
    static_cast<int32_t>(DirtyType::TYPE_NEW), FAVORITE_ALBUMS_ORDER, FAVORITE_ORDER_SECTION, FAVORITE_ALBUM_HIDDEN
};

const AlbumChangeInfo FAVORITE_ALBUM_INFO_TOW = {
    FAVORITE_ALBUM_ID_TOW, FAVORITE_ALBUM_LPATH, FAVORITE_ALBUM_IMAGE_COUNT, FAVORITE_ALBUM_VIDEO_COUNT,
    PhotoAlbumType::SYSTEM, PhotoAlbumSubType::FAVORITE, FAVORITE_ALBUM_NAME, FAVORITE_ALBUM_URI,
    FAVORITE_ALBUM_COUNT, FAVORITE_ALBUM_COVER_URI, FAVORITE_ALBUM_HIDDEN_COUNT, FAVORITE_ALBUM_HIDDEN_COVER_URI,
    false, false, FAVORITE_ALBUM_COVER_DATE_TIME, FAVORITE_ALBUM_HIDDEN_COVER_DATE_TIME,
    static_cast<int32_t>(DirtyType::TYPE_NEW), FAVORITE_ALBUMS_ORDER, FAVORITE_ORDER_SECTION, FAVORITE_ALBUM_HIDDEN
};

const int32_t TRASH_ALBUM_ID = 2;
const int32_t TRASH_ALBUM_ID_TOW = 201;
const std::string TRASH_ALBUM_LPATH = "TRASH_lpath";
const int32_t TRASH_ALBUM_IMAGE_COUNT = 1;
const int32_t TRASH_ALBUM_VIDEO_COUNT = 2;
const std::string TRASH_ALBUM_NAME = "TRASH_albumName";
const std::string TRASH_ALBUM_URI = PhotoAlbumColumns::ALBUM_URI_PREFIX + std::to_string(TRASH_ALBUM_ID);
const int32_t TRASH_ALBUM_COUNT = 3;
const std::string TRASH_ALBUM_COVER_URI = "file://media/Photo/1113/vio/asdf.jpg";
const int32_t TRASH_ALBUM_HIDDEN_COUNT = 4;
const std::string TRASH_ALBUM_HIDDEN_COVER_URI = "file://media/Photo/123333/vio/asdf.jpg";
const int64_t TRASH_ALBUM_COVER_DATE_TIME = 222222;
const int64_t TRASH_ALBUM_HIDDEN_COVER_DATE_TIME = 222222;
const int64_t TRASH_ALBUMS_ORDER = -1;
const int64_t TRASH_ORDER_SECTION = -1;
const int32_t TRASH_ALBUM_HIDDEN = 0;
const AlbumChangeInfo TRASH_ALBUM_INFO = {
    TRASH_ALBUM_ID, TRASH_ALBUM_LPATH, TRASH_ALBUM_IMAGE_COUNT, TRASH_ALBUM_VIDEO_COUNT,
    PhotoAlbumType::SYSTEM, PhotoAlbumSubType::TRASH, TRASH_ALBUM_NAME, TRASH_ALBUM_URI,
    TRASH_ALBUM_COUNT, TRASH_ALBUM_COVER_URI, TRASH_ALBUM_HIDDEN_COUNT, TRASH_ALBUM_HIDDEN_COVER_URI,
    false, false, TRASH_ALBUM_COVER_DATE_TIME, TRASH_ALBUM_HIDDEN_COVER_DATE_TIME,
    static_cast<int32_t>(DirtyType::TYPE_NEW), TRASH_ALBUMS_ORDER, TRASH_ORDER_SECTION, TRASH_ALBUM_HIDDEN
};
const AlbumChangeInfo TRASH_ALBUM_INFO_TOW = {
    TRASH_ALBUM_ID_TOW, TRASH_ALBUM_LPATH, TRASH_ALBUM_IMAGE_COUNT, TRASH_ALBUM_VIDEO_COUNT,
    PhotoAlbumType::SYSTEM, PhotoAlbumSubType::TRASH, TRASH_ALBUM_NAME, TRASH_ALBUM_URI,
    TRASH_ALBUM_COUNT, TRASH_ALBUM_COVER_URI, TRASH_ALBUM_HIDDEN_COUNT, TRASH_ALBUM_HIDDEN_COVER_URI,
    false, false, TRASH_ALBUM_COVER_DATE_TIME, TRASH_ALBUM_HIDDEN_COVER_DATE_TIME,
    static_cast<int32_t>(DirtyType::TYPE_NEW), TRASH_ALBUMS_ORDER, TRASH_ORDER_SECTION, TRASH_ALBUM_HIDDEN
};

const int32_t HIDDEN_ALBUM_ID = 3;
const int32_t HIDDEN_ALBUM_ID_TOW = 301;
const std::string HIDDEN_ALBUM_LPATH = "HIDDEN_lpath";
const int32_t HIDDEN_ALBUM_IMAGE_COUNT = 11;
const int32_t HIDDEN_ALBUM_VIDEO_COUNT = 22;
const std::string HIDDEN_ALBUM_NAME = "HIDDEN_albumName";
const std::string HIDDEN_ALBUM_URI = PhotoAlbumColumns::ALBUM_URI_PREFIX + std::to_string(HIDDEN_ALBUM_ID);
const int32_t HIDDEN_ALBUM_COUNT = 33;
const std::string HIDDEN_ALBUM_COVER_URI = "file://media/Photo/121233/vio/asdf.jpg";
const int32_t HIDDEN_ALBUM_HIDDEN_COUNT = 44;
const std::string HIDDEN_ALBUM_HIDDEN_COVER_URI = HIDDEN_ALBUM_COVER_URI;
const int64_t HIDDEN_ALBUM_COVER_DATE_TIME = 333333;
const int64_t HIDDEN_ALBUM_HIDDEN_COVER_DATE_TIME = 333333;
const int64_t HIDDEN_ALBUMS_ORDER = -1;
const int64_t HIDDEN_ORDER_SECTION = -1;
const int32_t HIDDEN_ALBUM_HIDDEN = 0;
const AlbumChangeInfo HIDDEN_ALBUM_INFO = {
    HIDDEN_ALBUM_ID, HIDDEN_ALBUM_LPATH, HIDDEN_ALBUM_IMAGE_COUNT, HIDDEN_ALBUM_VIDEO_COUNT,
    PhotoAlbumType::SYSTEM, PhotoAlbumSubType::HIDDEN, HIDDEN_ALBUM_NAME, HIDDEN_ALBUM_URI,
    HIDDEN_ALBUM_COUNT, HIDDEN_ALBUM_COVER_URI, HIDDEN_ALBUM_HIDDEN_COUNT, HIDDEN_ALBUM_HIDDEN_COVER_URI,
    false, false, HIDDEN_ALBUM_COVER_DATE_TIME, HIDDEN_ALBUM_HIDDEN_COVER_DATE_TIME,
    static_cast<int32_t>(DirtyType::TYPE_NEW), HIDDEN_ALBUMS_ORDER, HIDDEN_ORDER_SECTION, HIDDEN_ALBUM_HIDDEN
};
const AlbumChangeInfo HIDDEN_ALBUM_INFO_TOW = {
    HIDDEN_ALBUM_ID_TOW, HIDDEN_ALBUM_LPATH, HIDDEN_ALBUM_IMAGE_COUNT, HIDDEN_ALBUM_VIDEO_COUNT,
    PhotoAlbumType::SYSTEM, PhotoAlbumSubType::HIDDEN, HIDDEN_ALBUM_NAME, HIDDEN_ALBUM_URI,
    HIDDEN_ALBUM_COUNT, HIDDEN_ALBUM_COVER_URI, HIDDEN_ALBUM_HIDDEN_COUNT, HIDDEN_ALBUM_HIDDEN_COVER_URI,
    false, false, HIDDEN_ALBUM_COVER_DATE_TIME, HIDDEN_ALBUM_HIDDEN_COVER_DATE_TIME,
    static_cast<int32_t>(DirtyType::TYPE_NEW), HIDDEN_ALBUMS_ORDER, HIDDEN_ORDER_SECTION, HIDDEN_ALBUM_HIDDEN
};

const int32_t VIDEO_ALBUM_ID = 4;
const int32_t VIDEO_ALBUM_ID_TOW = 401;
const std::string VIDEO_ALBUM_LPATH = "VIDEO_lpath";
const int32_t VIDEO_ALBUM_IMAGE_COUNT = 1;
const int32_t VIDEO_ALBUM_VIDEO_COUNT = 2;
const std::string VIDEO_ALBUM_NAME = "VIDEO_albumName";
const std::string VIDEO_ALBUM_URI = PhotoAlbumColumns::ALBUM_URI_PREFIX + std::to_string(VIDEO_ALBUM_ID);
const int32_t VIDEO_ALBUM_COUNT = 3;
const std::string VIDEO_ALBUM_COVER_URI = "file://media/Photo/125673/vio/asdf.jpg";
const int32_t VIDEO_ALBUM_HIDDEN_COUNT = 4;
const std::string VIDEO_ALBUM_HIDDEN_COVER_URI = "file://media/Photo/1239876/vio/asdf.jpg";
const int64_t VIDEO_ALBUM_COVER_DATE_TIME = 444444;
const int64_t VIDEO_ALBUM_HIDDEN_COVER_DATE_TIME = 444444;
const int64_t VIDEO_ALBUMS_ORDER = -1;
const int64_t VIDEO_ORDER_SECTION = -1;
const int32_t VIDEO_ALBUM_HIDDEN = 0;
const AlbumChangeInfo VIDEO_ALBUM_INFO = {
    VIDEO_ALBUM_ID, VIDEO_ALBUM_LPATH, VIDEO_ALBUM_IMAGE_COUNT, VIDEO_ALBUM_VIDEO_COUNT,
    PhotoAlbumType::SYSTEM, PhotoAlbumSubType::VIDEO, VIDEO_ALBUM_NAME, VIDEO_ALBUM_URI,
    VIDEO_ALBUM_COUNT, VIDEO_ALBUM_COVER_URI, VIDEO_ALBUM_HIDDEN_COUNT, VIDEO_ALBUM_HIDDEN_COVER_URI,
    false, false, VIDEO_ALBUM_COVER_DATE_TIME, VIDEO_ALBUM_HIDDEN_COVER_DATE_TIME,
    static_cast<int32_t>(DirtyType::TYPE_NEW), VIDEO_ALBUMS_ORDER, VIDEO_ORDER_SECTION, VIDEO_ALBUM_HIDDEN
};
const AlbumChangeInfo VIDEO_ALBUM_INFO_TOW = {
    VIDEO_ALBUM_ID_TOW, VIDEO_ALBUM_LPATH, VIDEO_ALBUM_IMAGE_COUNT, VIDEO_ALBUM_VIDEO_COUNT,
    PhotoAlbumType::SYSTEM, PhotoAlbumSubType::VIDEO, VIDEO_ALBUM_NAME, VIDEO_ALBUM_URI,
    VIDEO_ALBUM_COUNT, VIDEO_ALBUM_COVER_URI, VIDEO_ALBUM_HIDDEN_COUNT, VIDEO_ALBUM_HIDDEN_COVER_URI,
    false, false, VIDEO_ALBUM_COVER_DATE_TIME, VIDEO_ALBUM_HIDDEN_COVER_DATE_TIME,
    static_cast<int32_t>(DirtyType::TYPE_NEW), VIDEO_ALBUMS_ORDER, VIDEO_ORDER_SECTION, VIDEO_ALBUM_HIDDEN
};

const int32_t IMAGE_ALBUM_ID = 5;
const int32_t IMAGE_ALBUM_ID_TOW = 501;
const std::string IMAGE_ALBUM_LPATH = "IMAGE_lpath";
const int32_t IMAGE_ALBUM_IMAGE_COUNT = 1;
const int32_t IMAGE_ALBUM_VIDEO_COUNT = 2;
const std::string IMAGE_ALBUM_NAME = "IMAGE_albumName";
const std::string IMAGE_ALBUM_URI = PhotoAlbumColumns::ALBUM_URI_PREFIX + std::to_string(IMAGE_ALBUM_ID);
const int32_t IMAGE_ALBUM_COUNT = 3;
const std::string IMAGE_ALBUM_COVER_URI = "file://media/Photo/123858/vio/asdf.jpg";
const int32_t IMAGE_ALBUM_HIDDEN_COUNT = 4;
const std::string IMAGE_ALBUM_HIDDEN_COVER_URI = "file://media/Photo/148564/vio/asdf.jpg";
const int64_t IMAGE_ALBUM_COVER_DATE_TIME = 555555;
const int64_t IMAGE_ALBUM_HIDDEN_COVER_DATE_TIME = 555555;
const int64_t IMAGE_ALBUMS_ORDER = -1;
const int64_t IMAGE_ORDER_SECTION = -1;
const int32_t IMAGE_ALBUM_HIDDEN = 0;
const AlbumChangeInfo IMAGE_ALBUM_INFO = {
    IMAGE_ALBUM_ID, IMAGE_ALBUM_LPATH, IMAGE_ALBUM_IMAGE_COUNT, IMAGE_ALBUM_VIDEO_COUNT,
    PhotoAlbumType::SYSTEM, PhotoAlbumSubType::IMAGE, IMAGE_ALBUM_NAME, IMAGE_ALBUM_URI,
    IMAGE_ALBUM_COUNT, IMAGE_ALBUM_COVER_URI, IMAGE_ALBUM_HIDDEN_COUNT, IMAGE_ALBUM_HIDDEN_COVER_URI,
    false, false, IMAGE_ALBUM_COVER_DATE_TIME, IMAGE_ALBUM_HIDDEN_COVER_DATE_TIME,
    static_cast<int32_t>(DirtyType::TYPE_NEW), IMAGE_ALBUMS_ORDER, IMAGE_ORDER_SECTION, IMAGE_ALBUM_HIDDEN
};
const AlbumChangeInfo IMAGE_ALBUM_INFO_TOW = {
    IMAGE_ALBUM_ID_TOW, IMAGE_ALBUM_LPATH, IMAGE_ALBUM_IMAGE_COUNT, IMAGE_ALBUM_VIDEO_COUNT,
    PhotoAlbumType::SYSTEM, PhotoAlbumSubType::IMAGE, IMAGE_ALBUM_NAME, IMAGE_ALBUM_URI,
    IMAGE_ALBUM_COUNT, IMAGE_ALBUM_COVER_URI, IMAGE_ALBUM_HIDDEN_COUNT, IMAGE_ALBUM_HIDDEN_COVER_URI,
    false, false, IMAGE_ALBUM_COVER_DATE_TIME, IMAGE_ALBUM_HIDDEN_COVER_DATE_TIME,
    static_cast<int32_t>(DirtyType::TYPE_NEW), IMAGE_ALBUMS_ORDER, IMAGE_ORDER_SECTION, IMAGE_ALBUM_HIDDEN
};

const int32_t CLOUD_ENHANCEMENT_ALBUM_ID = 6;
const int32_t CLOUD_ENHANCEMENT_ALBUM_ID_TOW = 601;
const std::string CLOUD_ENHANCEMENT_ALBUM_LPATH = "CLOUD_ENHANCEMENT_lpath";
const int32_t CLOUD_ENHANCEMENT_ALBUM_IMAGE_COUNT = 1;
const int32_t CLOUD_ENHANCEMENT_ALBUM_VIDEO_COUNT = 2;
const std::string CLOUD_ENHANCEMENT_ALBUM_NAME = "CLOUD_ENHANCEMENT_albumName";
const std::string CLOUD_ENHANCEMENT_ALBUM_URI =
    PhotoAlbumColumns::ALBUM_URI_PREFIX + std::to_string(CLOUD_ENHANCEMENT_ALBUM_ID);
const int32_t CLOUD_ENHANCEMENT_ALBUM_COUNT = 3;
const std::string CLOUD_ENHANCEMENT_ALBUM_COVER_URI = "file://media/Photo/1287533/vio/asdf.jpg";
const int32_t CLOUD_ENHANCEMENT_ALBUM_HIDDEN_COUNT = 4;
const std::string CLOUD_ENHANCEMENT_ALBUM_HIDDEN_COVER_URI = "file://media/Photo/120986543/vio/asdf.jpg";
const int64_t CLOUD_ENHANCEMENT_ALBUM_COVER_DATE_TIME = 666666;
const int64_t CLOUD_ENHANCEMENT_ALBUM_HIDDEN_COVER_DATE_TIME = 666666;
const int64_t CLOUD_ENHANCEMENT_ALBUMS_ORDER = -1;
const int64_t CLOUD_ENHANCEMENT_ORDER_SECTION = -1;
const int32_t CLOUD_ENHANCEMENT_ALBUM_HIDDEN = 0;
const AlbumChangeInfo CLOUD_ENHANCEMENT_ALBUM_INFO = {
    CLOUD_ENHANCEMENT_ALBUM_ID, CLOUD_ENHANCEMENT_ALBUM_LPATH, CLOUD_ENHANCEMENT_ALBUM_IMAGE_COUNT,
    CLOUD_ENHANCEMENT_ALBUM_VIDEO_COUNT, PhotoAlbumType::SYSTEM, PhotoAlbumSubType::CLOUD_ENHANCEMENT,
    CLOUD_ENHANCEMENT_ALBUM_NAME, CLOUD_ENHANCEMENT_ALBUM_URI, CLOUD_ENHANCEMENT_ALBUM_COUNT,
    CLOUD_ENHANCEMENT_ALBUM_COVER_URI, CLOUD_ENHANCEMENT_ALBUM_HIDDEN_COUNT, CLOUD_ENHANCEMENT_ALBUM_HIDDEN_COVER_URI,
    false, false, CLOUD_ENHANCEMENT_ALBUM_COVER_DATE_TIME, CLOUD_ENHANCEMENT_ALBUM_HIDDEN_COVER_DATE_TIME,
    static_cast<int32_t>(DirtyType::TYPE_NEW), CLOUD_ENHANCEMENT_ALBUMS_ORDER, CLOUD_ENHANCEMENT_ORDER_SECTION,
    CLOUD_ENHANCEMENT_ALBUM_HIDDEN
};
const AlbumChangeInfo CLOUD_ENHANCEMENT_ALBUM_INFO_TOW = {
    CLOUD_ENHANCEMENT_ALBUM_ID_TOW, CLOUD_ENHANCEMENT_ALBUM_LPATH, CLOUD_ENHANCEMENT_ALBUM_IMAGE_COUNT,
    CLOUD_ENHANCEMENT_ALBUM_VIDEO_COUNT, PhotoAlbumType::SYSTEM, PhotoAlbumSubType::CLOUD_ENHANCEMENT,
    CLOUD_ENHANCEMENT_ALBUM_NAME, CLOUD_ENHANCEMENT_ALBUM_URI, CLOUD_ENHANCEMENT_ALBUM_COUNT,
    CLOUD_ENHANCEMENT_ALBUM_COVER_URI, CLOUD_ENHANCEMENT_ALBUM_HIDDEN_COUNT, CLOUD_ENHANCEMENT_ALBUM_HIDDEN_COVER_URI,
    false, false, CLOUD_ENHANCEMENT_ALBUM_COVER_DATE_TIME, CLOUD_ENHANCEMENT_ALBUM_HIDDEN_COVER_DATE_TIME,
    static_cast<int32_t>(DirtyType::TYPE_NEW), CLOUD_ENHANCEMENT_ALBUMS_ORDER, CLOUD_ENHANCEMENT_ORDER_SECTION,
    CLOUD_ENHANCEMENT_ALBUM_HIDDEN
};

const int32_t USER_ALBUM_ID = 100;
const int32_t USER_ALBUM_ID_TOW = 1001;
const std::string USER_ALBUM_LPATH = "USER_lpath";
const int32_t USER_ALBUM_IMAGE_COUNT = 1;
const int32_t USER_ALBUM_VIDEO_COUNT = 2;
const std::string USER_ALBUM_NAME = "USER_albumName";
const std::string USER_ALBUM_URI = PhotoAlbumColumns::ALBUM_URI_PREFIX + std::to_string(USER_ALBUM_ID);
const int32_t USER_ALBUM_COUNT = 3;
const std::string USER_ALBUM_COVER_URI = "file://media/Photo/1333323/vio/asdf.jpg";
const int32_t USER_ALBUM_HIDDEN_COUNT = 4;
const std::string USER_ALBUM_HIDDEN_COVER_URI = "file://media/Photo/1777723/vio/asdf.jpg";
const int64_t USER_ALBUM_COVER_DATE_TIME = 777777;
const int64_t USER_ALBUM_HIDDEN_COVER_DATE_TIME = 777777;
const int64_t USER_ALBUM_ALBUMS_ORDER = -1;
const int64_t USER_ALBUM_ORDER_SECTION = -1;
const int32_t USER_ALBUM_HIDDEN = 0;
const AlbumChangeInfo USER_ALBUM_INFO = {
    USER_ALBUM_ID, USER_ALBUM_LPATH, USER_ALBUM_IMAGE_COUNT, USER_ALBUM_VIDEO_COUNT,
    PhotoAlbumType::USER, PhotoAlbumSubType::USER_GENERIC, USER_ALBUM_NAME, USER_ALBUM_URI,
    USER_ALBUM_COUNT, USER_ALBUM_COVER_URI, USER_ALBUM_HIDDEN_COUNT, USER_ALBUM_HIDDEN_COVER_URI,
    false, false, USER_ALBUM_COVER_DATE_TIME, USER_ALBUM_HIDDEN_COVER_DATE_TIME,
    static_cast<int32_t>(DirtyType::TYPE_NEW), USER_ALBUM_ALBUMS_ORDER, USER_ALBUM_ORDER_SECTION,
    USER_ALBUM_HIDDEN
};
const AlbumChangeInfo USER_ALBUM_INFO_TOW = {
    USER_ALBUM_ID_TOW, USER_ALBUM_LPATH, USER_ALBUM_IMAGE_COUNT, USER_ALBUM_VIDEO_COUNT,
    PhotoAlbumType::USER, PhotoAlbumSubType::USER_GENERIC, USER_ALBUM_NAME, USER_ALBUM_URI,
    USER_ALBUM_COUNT, USER_ALBUM_COVER_URI, USER_ALBUM_HIDDEN_COUNT, USER_ALBUM_HIDDEN_COVER_URI,
    false, false, USER_ALBUM_COVER_DATE_TIME, USER_ALBUM_HIDDEN_COVER_DATE_TIME,
    static_cast<int32_t>(DirtyType::TYPE_NEW), USER_ALBUM_ALBUMS_ORDER, USER_ALBUM_ORDER_SECTION,
    USER_ALBUM_HIDDEN
};

const int32_t SOURCE_ALBUM_ID = 1000;
const int32_t SOURCE_ALBUM_ID_TOW = 10001;
const std::string SOURCE_ALBUM_LPATH = "SOURCE_lpath";
const int32_t SOURCE_ALBUM_IMAGE_COUNT = 1;
const int32_t SOURCE_ALBUM_VIDEO_COUNT = 2;
const std::string SOURCE_ALBUM_NAME = "SOURCE_albumName";
const std::string SOURCE_ALBUM_URI = PhotoAlbumColumns::ALBUM_URI_PREFIX + std::to_string(SOURCE_ALBUM_ID);
const int32_t SOURCE_ALBUM_COUNT = 3;
const std::string SOURCE_ALBUM_COVER_URI = "file://media/Photo/14333323/vio/asdf.jpg";
const int32_t SOURCE_ALBUM_HIDDEN_COUNT = 4;
const std::string SOURCE_ALBUM_HIDDEN_COVER_URI = "file://media/Photo/128889993/vio/asdf.jpg";
const int64_t SOURCE_ALBUM_COVER_DATE_TIME = 888888;
const int64_t SOURCE_ALBUM_HIDDEN_COVER_DATE_TIME = 888888;
const int64_t SOURCE_ALBUM_ALBUMS_ORDER = -1;
const int64_t SOURCE_ALBUM_ORDER_SECTION = -1;
const int32_t SOURCE_ALBUM_HIDDEN = 0;
const AlbumChangeInfo SOURCE_ALBUM_INFO = {
    SOURCE_ALBUM_ID, SOURCE_ALBUM_LPATH, SOURCE_ALBUM_IMAGE_COUNT, SOURCE_ALBUM_VIDEO_COUNT,
    PhotoAlbumType::SOURCE, PhotoAlbumSubType::ANY, SOURCE_ALBUM_NAME, SOURCE_ALBUM_URI,
    SOURCE_ALBUM_COUNT, SOURCE_ALBUM_COVER_URI, SOURCE_ALBUM_HIDDEN_COUNT, SOURCE_ALBUM_HIDDEN_COVER_URI,
    false, false, SOURCE_ALBUM_COVER_DATE_TIME, SOURCE_ALBUM_HIDDEN_COVER_DATE_TIME,
    static_cast<int32_t>(DirtyType::TYPE_NEW), SOURCE_ALBUM_ALBUMS_ORDER, SOURCE_ALBUM_ORDER_SECTION,
    SOURCE_ALBUM_HIDDEN
};
const AlbumChangeInfo SOURCE_ALBUM_INFO_TOW = {
    SOURCE_ALBUM_ID_TOW, SOURCE_ALBUM_LPATH, SOURCE_ALBUM_IMAGE_COUNT, SOURCE_ALBUM_VIDEO_COUNT,
    PhotoAlbumType::SOURCE, PhotoAlbumSubType::ANY, SOURCE_ALBUM_NAME, SOURCE_ALBUM_URI,
    SOURCE_ALBUM_COUNT, SOURCE_ALBUM_COVER_URI, SOURCE_ALBUM_HIDDEN_COUNT, SOURCE_ALBUM_HIDDEN_COVER_URI,
    false, false, SOURCE_ALBUM_COVER_DATE_TIME, SOURCE_ALBUM_HIDDEN_COVER_DATE_TIME,
    static_cast<int32_t>(DirtyType::TYPE_NEW), SOURCE_ALBUM_ALBUMS_ORDER, SOURCE_ALBUM_ORDER_SECTION,
    SOURCE_ALBUM_HIDDEN
};

NativeRdb::ValuesBucket GetPhotoAlbumInsertValue(const AlbumChangeInfo &albumInfo);
NativeRdb::ValuesBucket GetFavoriteInsertAlbum();
NativeRdb::ValuesBucket GetHiddenInsertAlbum();
bool IsEqualAlbumInfo(const AlbumChangeInfo &albumInfo1, const AlbumChangeInfo &albumInfo2);
NativeRdb::ValuesBucket GetTrashInsertAlbum();
NativeRdb::ValuesBucket GetVideoInsertAlbum();
NativeRdb::ValuesBucket GetImageInsertAlbum();
NativeRdb::ValuesBucket GetCloudEnhancementInsertAlbum();

AlbumChangeInfo GetUserInsertInfo(int32_t albumId);
AlbumChangeInfo GetSourceInsertInfo(int32_t albumId);
NativeRdb::ValuesBucket GetUserInsertAlbum(int32_t albumId);
NativeRdb::ValuesBucket GetSourceInsertAlbum(int32_t albumId);
int32_t GetAlbumCount(PhotoAlbumSubType subType, std::shared_ptr<MediaLibraryRdbStore> rdbStore);
int32_t GetAlbumDirtyType(PhotoAlbumSubType subType, std::shared_ptr<MediaLibraryRdbStore> rdbStore);
AlbumChangeInfo GetAlbumInfo(PhotoAlbumSubType subType, std::shared_ptr<MediaLibraryRdbStore> rdbStore);
AlbumChangeInfo GetAlbumInfo(int32_t albumId, std::shared_ptr<MediaLibraryRdbStore> rdbStore);
bool CheckAlbumChangeData(const AlbumChangeData &changeData, RdbOperation operation, const AlbumChangeInfo &infoBefore,
    const AlbumChangeInfo &infoAfter, bool isDelete = false);

} // namespace Media
} // namespace OHOS

#endif // MEDIALIBRARY_ACCURATE_REFRESH_TEST_UTIL_H