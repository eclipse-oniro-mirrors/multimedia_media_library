/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_UPGRADE_H_
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_UPGRADE_H_

#include <string>

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))

class MediaUpgrade {
public:
    // Util consts
    static const std::string ASSETS_QUERY_FILTER EXPORT;
};

class PhotoUpgrade {
public:
    // create PhotoTable sql
    static const std::string CREATE_PHOTO_TABLE EXPORT;
    static const std::string CREATE_CLOUD_ID_INDEX EXPORT;
    static const std::string CREATE_YEAR_INDEX EXPORT;
    static const std::string CREATE_MONTH_INDEX EXPORT;
    static const std::string CREATE_DAY_INDEX EXPORT;
    static const std::string DROP_SCHPT_MEDIA_TYPE_INDEX EXPORT;
    static const std::string DROP_BURST_MODE_ALBUM_INDEX EXPORT;
    static const std::string CREATE_SCHPT_MEDIA_TYPE_INDEX EXPORT;
    static const std::string CREATE_SCHPT_DAY_INDEX EXPORT;
    static const std::string DROP_SCHPT_DAY_INDEX EXPORT;
    static const std::string CREATE_HIDDEN_TIME_INDEX EXPORT;
    static const std::string CREATE_SCHPT_HIDDEN_TIME_INDEX EXPORT;
    static const std::string DROP_SCHPT_HIDDEN_TIME_INDEX EXPORT;
    static const std::string CREATE_PHOTO_FAVORITE_INDEX EXPORT;
    static const std::string DROP_PHOTO_FAVORITE_INDEX EXPORT;
    static const std::string CREATE_PHOTO_DISPLAYNAME_INDEX EXPORT;
    static const std::string CREATE_PHOTO_BURSTKEY_INDEX EXPORT;
    static const std::string UPDATE_READY_ON_THUMBNAIL_UPGRADE EXPORT;
    static const std::string UPDATA_PHOTOS_DATA_UNIQUE EXPORT;
    static const std::string UPDATE_LCD_STATUS_NOT_UPLOADED EXPORT;
    static const std::string UPDATE_LATITUDE_AND_LONGITUDE_DEFAULT_NULL EXPORT;
    static const std::string UPDATE_PHOTO_QUALITY_OF_NULL_PHOTO_ID EXPORT;

    // create indexes for Photo
    static const std::string INDEX_SCTHP_ADDTIME EXPORT;
    static const std::string DROP_INDEX_SCTHP_ADDTIME EXPORT;
    static const std::string DROP_INDEX_SCHPT_ADDTIME_ALBUM EXPORT;
    static const std::string INDEX_CAMERA_SHOT_KEY EXPORT;
    static const std::string INDEX_SCHPT_READY EXPORT;
    static const std::string DROP_INDEX_SCHPT_READY EXPORT;
    static const std::string CREATE_SCHPT_YEAR_COUNT_READY_INDEX;
    static const std::string CREATE_SCHPT_MONTH_COUNT_READY_INDEX;
    static const std::string CREATE_SCHPT_MEDIA_TYPE_COUNT_READY_INDEX;
    static const std::string DROP_SCHPT_YEAR_COUNT_READY_INDEX;
    static const std::string DROP_SCHPT_MONTH_COUNT_READY_INDEX;
    static const std::string DROP_SCHPT_MEDIA_TYPE_COUNT_READY_INDEX;
    static const std::string CREATE_SCHPT_CLOUD_ENHANCEMENT_ALBUM_INDEX;
    static const std::string INDEX_SCHPT_ALBUM_GENERAL;
    static const std::string INDEX_SCHPT_ALBUM;
    static const std::string INDEX_SCTHP_PHOTO_DATEADDED;
    static const std::string INDEX_LATITUDE;
    static const std::string INDEX_LONGITUDE;
    static const std::string CREATE_PHOTO_SORT_MEDIA_TYPE_DATE_ADDED_INDEX;
    static const std::string CREATE_PHOTO_SORT_MEDIA_TYPE_DATE_TAKEN_INDEX;
    static const std::string CREATE_PHOTO_SORT_IN_ALBUM_DATE_ADDED_INDEX;
    static const std::string CREATE_PHOTO_SORT_IN_ALBUM_DATE_TAKEN_INDEX;
    static const std::string CREATE_PHOTO_SORT_IN_ALBUM_SIZE_INDEX;
    static const std::string CREATE_PHOTO_SORT_MEDIA_TYPE_SIZE_INDEX;
    static const std::string CREATE_PHOTO_SORT_IN_ALBUM_DISPLAY_NAME_INDEX;
    static const std::string CREATE_PHOTO_SORT_MEDIA_TYPE_DISPLAY_NAME_INDEX;
    static const std::string CREATE_PHOTO_SHOOTING_MODE_ALBUM_GENERAL_INDEX;
    static const std::string CREATE_PHOTO_BURST_MODE_ALBUM_INDEX;
    static const std::string CREATE_PHOTO_FRONT_CAMERA_ALBUM_INDEX;
    static const std::string CREATE_PHOTO_RAW_IMAGE_ALBUM_INDEX;
    static const std::string CREATE_PHOTO_MOVING_PHOTO_ALBUM_INDEX;
    static const std::string INDEX_QUERY_THUMBNAIL_WHITE_BLOCKS;

    // create Photo cloud sync trigger
    static const std::string CREATE_PHOTOS_DELETE_TRIGGER EXPORT;
    static const std::string CREATE_PHOTOS_FDIRTY_TRIGGER EXPORT;
    static const std::string CREATE_PHOTOS_MDIRTY_TRIGGER EXPORT;
    static const std::string CREATE_PHOTOS_INSERT_CLOUD_SYNC EXPORT;
    static const std::string CREATE_PHOTOS_UPDATE_CLOUD_SYNC EXPORT;
    static const std::string CREATE_PHOTOS_METADATA_DIRTY_TRIGGER EXPORT;

    // highlight trigger
    static const std::string INSERT_GENERATE_HIGHLIGHT_THUMBNAIL EXPORT;
    static const std::string UPDATE_GENERATE_HIGHLIGHT_THUMBNAIL EXPORT;
    static const std::string INDEX_HIGHLIGHT_FILEID EXPORT;

    static const std::string QUERY_MEDIA_VOLUME EXPORT;
    static const std::string PHOTOS_QUERY_FILTER EXPORT;
};

class PhotoExtUpgrade {
public:
    // create table sql
    static const std::string CREATE_PHOTO_EXT_TABLE EXPORT;
};
} // namespace OHOS::Media
#endif  // INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_UPGRADE_H_