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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_COLUMN_H_
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_COLUMN_H_

#include <set>
#include <string>

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))
enum class DirtyTypes : int32_t {
    TYPE_SYNCED,
    TYPE_NEW,
    TYPE_MDIRTY,
    TYPE_FDIRTY,
    TYPE_DELETED,
    TYPE_RETRY,
    TYPE_SDIRTY
};

class MediaColumn {
public:
    // Asset Base Parameter
    static const std::string MEDIA_ID EXPORT;
    static const std::string MEDIA_FILE_PATH EXPORT;
    static const std::string MEDIA_SIZE EXPORT;
    static const std::string MEDIA_TITLE EXPORT;
    static const std::string MEDIA_NAME EXPORT;
    static const std::string MEDIA_TYPE EXPORT;
    static const std::string MEDIA_MIME_TYPE EXPORT;
    static const std::string MEDIA_OWNER_PACKAGE EXPORT;
    static const std::string MEDIA_OWNER_APPID EXPORT;
    static const std::string MEDIA_PACKAGE_NAME EXPORT;
    static const std::string MEDIA_DEVICE_NAME EXPORT;

    // As set Parameter about time
    static const std::string MEDIA_DATE_MODIFIED EXPORT;
    static const std::string MEDIA_DATE_ADDED EXPORT;
    static const std::string MEDIA_DATE_TAKEN EXPORT;
    static const std::string MEDIA_DURATION EXPORT;
    static const std::string MEDIA_TIME_PENDING EXPORT;
    static const std::string MEDIA_IS_FAV EXPORT;
    static const std::string MEDIA_DATE_TRASHED EXPORT;
    static const std::string MEDIA_DATE_DELETED EXPORT;
    static const std::string MEDIA_HIDDEN EXPORT;

    // Asset Parameter deperated
    static const std::string MEDIA_PARENT_ID EXPORT;
    static const std::string MEDIA_RELATIVE_PATH EXPORT;
    static const std::string MEDIA_VIRTURL_PATH EXPORT;

    // All Columns
    static const std::set<std::string> MEDIA_COLUMNS EXPORT;
    // Default fetch columns
    static const std::set<std::string> DEFAULT_FETCH_COLUMNS EXPORT;

    // Util consts
    static const std::string ASSETS_QUERY_FILTER EXPORT;
};

class PhotoColumn : public MediaColumn {
public:
    // column only in PhotoTable
    static const std::string PHOTO_ORIENTATION EXPORT;
    static const std::string PHOTO_LATITUDE EXPORT;
    static const std::string PHOTO_LONGITUDE EXPORT;
    static const std::string PHOTO_HEIGHT EXPORT;
    static const std::string PHOTO_WIDTH EXPORT;
    static const std::string PHOTO_LCD_VISIT_TIME EXPORT;
    static const std::string PHOTO_EDIT_TIME EXPORT;
    static const std::string PHOTO_POSITION EXPORT;
    static const std::string PHOTO_DIRTY EXPORT;
    static const std::string PHOTO_CLOUD_ID EXPORT;
    static const std::string PHOTO_SUBTYPE EXPORT;
    static const std::string PHOTO_META_DATE_MODIFIED EXPORT;
    static const std::string PHOTO_SYNC_STATUS EXPORT;
    static const std::string PHOTO_CLOUD_VERSION EXPORT;
    static const std::string CAMERA_SHOT_KEY EXPORT;
    static const std::string PHOTO_USER_COMMENT EXPORT;
    static const std::string PHOTO_ALL_EXIF EXPORT;
    static const std::string PHOTO_CLEAN_FLAG EXPORT;
    static const std::string PHOTO_HAS_ASTC EXPORT;

    static const std::string PHOTO_SYNCING EXPORT;
    static const std::string PHOTO_DATE_YEAR EXPORT;
    static const std::string PHOTO_DATE_MONTH EXPORT;
    static const std::string PHOTO_DATE_DAY EXPORT;
    static const std::string PHOTO_SHOOTING_MODE EXPORT;
    static const std::string PHOTO_SHOOTING_MODE_TAG EXPORT;
    static const std::string PHOTO_LAST_VISIT_TIME EXPORT;
    static const std::string PHOTO_HIDDEN_TIME EXPORT;
    static const std::string PHOTO_THUMB_STATUS EXPORT;
    static const std::string PHOTO_ID EXPORT;
    static const std::string PHOTO_QUALITY EXPORT;
    static const std::string PHOTO_FIRST_VISIT_TIME EXPORT;
    static const std::string PHOTO_DEFERRED_PROC_TYPE EXPORT;

    // Photo-only default fetch columns
    static const std::set<std::string> DEFAULT_FETCH_COLUMNS EXPORT;

    // index in PhotoTable
    static const std::string PHOTO_CLOUD_ID_INDEX EXPORT;
    static const std::string PHOTO_DATE_YEAR_INDEX EXPORT;
    static const std::string PHOTO_DATE_MONTH_INDEX EXPORT;
    static const std::string PHOTO_DATE_DAY_INDEX EXPORT;
    static const std::string PHOTO_SCHPT_ADDED_INDEX EXPORT;
    static const std::string PHOTO_SCHPT_MEDIA_TYPE_INDEX EXPORT;
    static const std::string PHOTO_SCHPT_DAY_INDEX EXPORT;
    static const std::string PHOTO_HIDDEN_TIME_INDEX EXPORT;
    static const std::string PHOTO_SCHPT_HIDDEN_TIME_INDEX EXPORT;
    static const std::string PHOTO_FAVORITE_INDEX EXPORT;
    // format in PhotoTable year month day
    static const std::string PHOTO_DATE_YEAR_FORMAT EXPORT;
    static const std::string PHOTO_DATE_MONTH_FORMAT EXPORT;
    static const std::string PHOTO_DATE_DAY_FORMAT EXPORT;
    // table name
    static const std::string PHOTOS_TABLE EXPORT;

    // create PhotoTable sql
    static const std::string CREATE_PHOTO_TABLE EXPORT;
    static const std::string CREATE_CLOUD_ID_INDEX EXPORT;
    static const std::string CREATE_YEAR_INDEX EXPORT;
    static const std::string CREATE_MONTH_INDEX EXPORT;
    static const std::string CREATE_DAY_INDEX EXPORT;
    static const std::string DROP_SCHPT_MEDIA_TYPE_INDEX EXPORT;
    static const std::string CREATE_SCHPT_MEDIA_TYPE_INDEX EXPORT;
    static const std::string CREATE_SCHPT_DAY_INDEX EXPORT;
    static const std::string CREATE_HIDDEN_TIME_INDEX EXPORT;
    static const std::string CREATE_SCHPT_HIDDEN_TIME_INDEX EXPORT;
    static const std::string CREATE_PHOTO_FAVORITE_INDEX EXPORT;

    // create indexes for Photo
    static const std::string INDEX_SCTHP_ADDTIME EXPORT;
    static const std::string INDEX_CAMERA_SHOT_KEY EXPORT;

    // create Photo cloud sync trigger
    static const std::string CREATE_PHOTOS_DELETE_TRIGGER EXPORT;
    static const std::string CREATE_PHOTOS_FDIRTY_TRIGGER EXPORT;
    static const std::string CREATE_PHOTOS_MDIRTY_TRIGGER EXPORT;
    static const std::string CREATE_PHOTOS_INSERT_CLOUD_SYNC EXPORT;
    static const std::string CREATE_PHOTOS_UPDATE_CLOUD_SYNC EXPORT;

    // photo uri
    static const std::string PHOTO_URI_PREFIX EXPORT;
    static const std::string PHOTO_TYPE_URI EXPORT;
    static const std::string DEFAULT_PHOTO_URI EXPORT;
    static const std::string PHOTO_CACHE_URI_PREFIX EXPORT;

    // all columns
    static const std::set<std::string> PHOTO_COLUMNS EXPORT;

    static const std::string QUERY_MEDIA_VOLUME EXPORT;

    static const std::string HIGHTLIGHT_COVER_URI EXPORT;

    EXPORT static bool IsPhotoColumn(const std::string &columnName);
    EXPORT static std::string CheckUploadPhotoColumns();
};

class AudioColumn : public MediaColumn {
public:
    // column only in AudioTable
    static const std::string AUDIO_ALBUM EXPORT;
    static const std::string AUDIO_ARTIST EXPORT;

    // table name
    static const std::string AUDIOS_TABLE EXPORT;

    // create AudioTable sql
    static const std::string CREATE_AUDIO_TABLE EXPORT;

    // audio uri
    static const std::string AUDIO_URI_PREFIX EXPORT;
    static const std::string AUDIO_TYPE_URI EXPORT;
    static const std::string DEFAULT_AUDIO_URI EXPORT;

    // all columns
    static const std::set<std::string> AUDIO_COLUMNS EXPORT;

    static const std::string QUERY_MEDIA_VOLUME EXPORT;

    static bool IsAudioColumn(const std::string &columnName) EXPORT;
};

class PhotoExtColumn {
public:
    // table name
    static const std::string PHOTOS_EXT_TABLE EXPORT;

    // column name
    static const std::string PHOTO_ID EXPORT;
    static const std::string THUMBNAIL_SIZE EXPORT;

    // create table sql
    static const std::string CREATE_PHOTO_EXT_TABLE EXPORT;
};

} // namespace OHOS::Media
#endif // INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_COLUMN_H_
