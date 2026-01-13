/*
 * Copyright (C) 2024-2025 Huawei Device Co., Ltd.
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

#ifndef MEDIALIBRARY_NAPI_ENUM_COMM_H
#define MEDIALIBRARY_NAPI_ENUM_COMM_H

#include <map>
#include <memory>
#include <set>
#include <vector>

#include "data_query.h"
#include "location_column.h"
#include "media_column.h"
#include "medialibrary_db_const.h"
#include "media_audio_column.h"

namespace OHOS {
namespace Media {
const std::vector<std::string> deliveryModeEnum {
    "FAST_MODE", "HIGH_QUALITY_MODE", "BALANCE_MODE"
};

const std::vector<std::string> sourceModeEnum {
    "ORIGINAL_MODE", "EDITED_MODE"
};

const std::vector<std::string> privateAlbumTypeNameEnum {
    "TYPE_FAVORITE", "TYPE_TRASH", "TYPE_HIDE", "TYPE_SMART", "TYPE_SEARCH"
};

const std::vector<std::string> AuthorizationModeEnum {
    "SHORT_TIME_AUTHORIZATION"
};

const std::vector<std::string> compatibleModeEnum {
    "ORIGINAL_FORMAT_MODE", "COMPATIBLE_FORMAT_MODE"
};

const std::vector<std::string> HIDDEN_PHOTOS_DISPLAY_MODE_ENUM {
    "ASSETS_MODE", "ALBUMS_MODE"
};

const std::vector<std::string> mediaTypesEnum {
    "FILE", "IMAGE", "VIDEO", "AUDIO", "MEDIA", "ALBUM_LIST", "ALBUM_LIST_INFO"
};

const std::vector<std::string> mediaTypesUserFileEnum {
    "IMAGE", "VIDEO", "AUDIO"
};

const std::vector<std::string> keyFrameThumbnailTypeEnum {
    "LCD", "THM", "THM_ASTC"
};

const std::vector<std::string> directoryEnum {
    "DIR_CAMERA", "DIR_VIDEO", "DIR_IMAGE", "DIR_AUDIO", "DIR_DOCUMENTS", "DIR_DOWNLOAD"
};

const std::vector<std::string> virtualAlbumTypeEnum {
    "TYPE_FAVORITE", "TYPE_TRASH"
};

const std::vector<std::string> directoryEnumValues {
    "Camera/", "Videos/", "Pictures/", "Audios/", "Documents/", "Download/"
};

const std::vector<std::string> systemAlbumSubType {
    "FAVORITE", "VIDEO", "HIDDEN", "TRASH", "SCREENSHOT", "CAMERA", "IMAGE", "CLOUD_ENHANCEMENT"
};

const std::vector<std::string> analysisAlbumSubType {
    "GEOGRAPHY_LOCATION", "GEOGRAPHY_CITY", "SHOOTING_MODE", "PORTRAIT", "GROUP_PHOTO",
    "HIGHLIGHT", "HIGHLIGHT_SUGGESTIONS"
};

const std::vector<std::string> positionTypeEnum {
    "LOCAL", "CLOUD", "LOCAL_AND_CLOUD"
};

const std::vector<std::string> photoSubTypeEnum {
    "DEFAULT", "SCREENSHOT", "CAMERA", "MOVING_PHOTO", "BURST", "CINEMATIC_VIDEO", "SPATIAL_3DGS"
};

const std::vector<std::string> photoPermissionTypeEnum {
    "TEMPORARY_READ_IMAGEVIDEO", "PERSISTENT_READ_IMAGEVIDEO"
};

const std::vector<std::string> hideSensitiveTypeEnum {
    "HIDE_LOCATION_AND_SHOOTING_PARAM", "HIDE_LOCATION_ONLY", "HIDE_SHOOTING_PARAM_ONLY",
    "NO_HIDE_SENSITIVE_TYPE", "DEFAULT"
};

const std::vector<std::string> notifyTypeEnum {
    "NOTIFY_ADD", "NOTIFY_UPDATE", "NOTIFY_REMOVE", "NOTIFY_ALBUM_ADD_ASSET", "NOTIFY_ALBUM_REMOVE_ASSET"
};

const std::vector<std::string> requestPhotoTypeEnum {
    "REQUEST_ALL_THUMBNAILS", "REQUEST_FAST_THUMBNAIL", "REQUEST_QUALITY_THUMBNAIL"
};

const std::vector<std::string> resourceTypeEnum {
    "IMAGE_RESOURCE", "VIDEO_RESOURCE", "PHOTO_PROXY", "PRIVATE_MOVING_PHOTO_RESOURCE",
    "PRIVATE_MOVING_PHOTO_METADATA"
};

const std::vector<std::string> dynamicRangeTypeEnum {
    "SDR", "HDR"
};

const std::vector<std::string> hdrModeEnum {
    "DEFAULT", "HDR_ISO_SINGLE", "HDR_ISO_DUAL", "HDR_CUVA", "HDR_VIVID_SINGLE", "HDR_VIVID_DUAL"
};

const std::vector<std::string> videoModeEnum{
    "DEFAULT", "NOT_LOG_VIDEO", "LOG_VIDEO"
};

const std::vector<std::string> movingPhotoEffectModeEnum {
    "DEFAULT", "BOUNCE_PLAY", "LOOP_PLAY", "LONG_EXPOSURE", "MULTI_EXPOSURE", "CINEMA_GRAPH"
};

const std::vector<std::string> imageFileTypeEnum {
    "JPEG", "HEIF"
};

const std::vector<std::string> cloudEnhancementTaskStageEnum {
    "TASK_STAGE_EXCEPTION", "TASK_STAGE_PREPARING", "TASK_STAGE_UPLOADING",
    "TASK_STAGE_EXECUTING", "TASK_STAGE_DOWNLOADING", "TASK_STAGE_FAILED", "TASK_STAGE_COMPLETED"
};

const std::vector<std::string> cloudEnhancementStateEnum {
    "UNAVAILABLE", "AVAILABLE", "EXECUTING", "COMPLETED"
};

const std::vector<std::string> watermarkTypeEnum {
    "DEFAULT", "BRAND_COMMON", "COMMON", "BRAND"
};

const std::vector<std::string> videoEnhancementTypeEnum {
    "QUALITY_ENHANCEMENT_LOCAL", "QUALITY_ENHANCEMENT_CLOUD", "QUALITY_ENHANCEMENT_LOCAL_AND_CLOUD"
};

const std::vector<std::string> compositeDisplayModeEnum {
    "DEFAULT", "CLOUD_ENHANCEMENT"
};

const std::vector<std::string> cloudMediaDownloadTypeEnum {
    "DOWNLOAD_FORCE", "DOWNLOAD_GENTLE"
};

const std::vector<std::string> cloudMediaRetainTypeEnum {
    "RETAIN_FORCE", // RETAIN_FORCE is an external interface and cannot be modified, corresponds to RETAIN_FORCE
    "HDC_RETAIN_FORCE"
};

const std::vector<std::string> cloudMediaAssetTaskStatusEnum {
    "DOWNLOADING", "PAUSED", "IDLE"
};

const std::vector<std::string> cloudMediaTaskPauseCauseEnum {
    "NO_PAUSE", "TEMPERATURE_LIMIT", "ROM_LIMIT", "NETWORK_FLOW_LIMIT", "WIFI_UNAVAILABLE",
    "POWER_LIMIT", "BACKGROUND_TASK_UNAVAILABLE", "FREQUENT_USER_REQUESTS", "CLOUD_ERROR", "USER_PAUSED",
};

const std::vector<std::string> notifyChangeTypeEnum {
    "NOTIFY_CHANGE_ADD", "NOTIFY_CHANGE_UPDATE", "NOTIFY_CHANGE_REMOVE", "NOTIFY_CHANGE_YUV_READY"
};
 
const std::vector<std::string> thumbnailChangeStatusEnum {
    "THUMBNAIL_NOT_EXISTS", "THUMBNAIL_ADD", "THUMBNAIL_UPDATE", "THUMBNAIL_NOT_CHANGE"
};
 
const std::vector<std::string> strongAssociationTypeEnum {
    "NORMAL", "CLOUD_ENHANCEMENT"
};

const std::vector<std::string> downloadCloudAssetCodeEnum {
    "ADD_DOWNLOAD_TASK_SUCCESS", "DOWNLOAD_ASSET_NOT_EXIST"
};

const std::vector<std::string> downloadAssetsNotifyTypeEnum {
    "DOWNLOAD_PROGRESS", "DOWNLOAD_FINISHED", "DOWNLOAD_FAILED", "DOWNLOAD_ASSET_DELETED", "DOWNLOAD_AUTO_PAUSED",
    "DOWNLOAD_AUTO_RESUMED", "DOWNLOAD_REFRESHED"
};

const std::vector<std::string> photoRiskStatusEnum {
    "UNIDENTIFIED", "APPROVED", "SUSPICIOUS", "REJECTED",
};

const std::vector<std::pair<std::string, int32_t>> PHOTO_SUB_TYPE_ENUM_PROPERTIES = {
    std::make_pair("DEFAULT",           static_cast<int32_t>(PhotoSubType::DEFAULT)),
    std::make_pair("SCREENSHOT",        static_cast<int32_t>(PhotoSubType::SCREENSHOT)),
    std::make_pair("CAMERA",            static_cast<int32_t>(PhotoSubType::CAMERA)),
    std::make_pair("MOVING_PHOTO",      static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)),
    std::make_pair("BURST",             static_cast<int32_t>(PhotoSubType::BURST)),
    std::make_pair("SLOW_MOTION_VIDEO", static_cast<int32_t>(PhotoSubType::SLOW_MOTION_VIDEO)),
    std::make_pair("SPATIAL_3DGS",      static_cast<int32_t>(PhotoSubType::SPATIAL_3DGS))
};

const std::vector<std::pair<std::string, std::string>> FILE_KEY_ENUM_PROPERTIES = {
    std::make_pair("ID",                        MEDIA_DATA_DB_ID),
    std::make_pair("RELATIVE_PATH",             MEDIA_DATA_DB_RELATIVE_PATH),
    std::make_pair("DISPLAY_NAME",              MEDIA_DATA_DB_NAME),
    std::make_pair("PARENT",                    MEDIA_DATA_DB_PARENT_ID),
    std::make_pair("MIME_TYPE",                 MEDIA_DATA_DB_MIME_TYPE),
    std::make_pair("MEDIA_TYPE",                MEDIA_DATA_DB_MEDIA_TYPE),
    std::make_pair("SIZE",                      MEDIA_DATA_DB_SIZE),
    std::make_pair("DATE_ADDED",                MEDIA_DATA_DB_DATE_ADDED),
    std::make_pair("DATE_MODIFIED",             MEDIA_DATA_DB_DATE_MODIFIED),
    std::make_pair("DATE_TAKEN",                MEDIA_DATA_DB_DATE_TAKEN),
    std::make_pair("TITLE",                     MEDIA_DATA_DB_TITLE),
    std::make_pair("ARTIST",                    MEDIA_DATA_DB_ARTIST),
    std::make_pair("AUDIOALBUM",                MEDIA_DATA_DB_AUDIO_ALBUM),
    std::make_pair("DURATION",                  MEDIA_DATA_DB_DURATION),
    std::make_pair("WIDTH",                     MEDIA_DATA_DB_WIDTH),
    std::make_pair("HEIGHT",                    MEDIA_DATA_DB_HEIGHT),
    std::make_pair("ORIENTATION",               MEDIA_DATA_DB_ORIENTATION),
    std::make_pair("ALBUM_ID",                  MEDIA_DATA_DB_BUCKET_ID),
    std::make_pair("ALBUM_NAME",                MEDIA_DATA_DB_BUCKET_NAME)
};

const std::vector<std::pair<std::string, std::string>> USERFILEMGR_FILEKEY_ENUM_PROPERTIES = {
    std::make_pair("URI",                       MEDIA_DATA_DB_URI),
    std::make_pair("RELATIVE_PATH",             MEDIA_DATA_DB_RELATIVE_PATH),
    std::make_pair("DISPLAY_NAME",              MEDIA_DATA_DB_NAME),
    std::make_pair("DATE_ADDED",                MEDIA_DATA_DB_DATE_ADDED),
    std::make_pair("DATE_MODIFIED",             MEDIA_DATA_DB_DATE_MODIFIED),
    std::make_pair("TITLE",                     MEDIA_DATA_DB_TITLE)
};

const std::vector<std::pair<std::string, std::string>> AUDIOKEY_ENUM_PROPERTIES = {
    std::make_pair("URI",                       MEDIA_DATA_DB_URI),
    std::make_pair("DISPLAY_NAME",              MEDIA_DATA_DB_NAME),
    std::make_pair("DATE_ADDED",                MEDIA_DATA_DB_DATE_ADDED),
    std::make_pair("DATE_MODIFIED",             MEDIA_DATA_DB_DATE_MODIFIED),
    std::make_pair("TITLE",                     MEDIA_DATA_DB_TITLE),
    std::make_pair("ARTIST",                    MEDIA_DATA_DB_ARTIST),
    std::make_pair("AUDIOALBUM",                MEDIA_DATA_DB_AUDIO_ALBUM),
    std::make_pair("DURATION",                  MEDIA_DATA_DB_DURATION),
    std::make_pair("FAVORITE",                  MEDIA_DATA_DB_IS_FAV),
    std::make_pair("SIZE",                      MediaColumn::MEDIA_SIZE),
    std::make_pair("PACKAGE_NAME",              MediaColumn::MEDIA_PACKAGE_NAME)
};

const std::vector<std::pair<std::string, std::string>> IMAGEVIDEOKEY_ENUM_PROPERTIES = {
    std::make_pair("URI",                       MEDIA_DATA_DB_URI),
    std::make_pair("DISPLAY_NAME",              MediaColumn::MEDIA_NAME),
    std::make_pair("DATE_ADDED",                MediaColumn::MEDIA_DATE_ADDED),
    std::make_pair("FILE_TYPE",                 MediaColumn::MEDIA_TYPE),
    std::make_pair("PHOTO_TYPE",                MediaColumn::MEDIA_TYPE),
    std::make_pair("DATE_MODIFIED",             MediaColumn::MEDIA_DATE_MODIFIED),
    std::make_pair("TITLE",                     MediaColumn::MEDIA_TITLE),
    std::make_pair("DURATION",                  MediaColumn::MEDIA_DURATION),
    std::make_pair("WIDTH",                     PhotoColumn::PHOTO_WIDTH),
    std::make_pair("HEIGHT",                    PhotoColumn::PHOTO_HEIGHT),
    std::make_pair("DATE_TAKEN",                MediaColumn::MEDIA_DATE_TAKEN),
    std::make_pair("DATE_TAKEN_MS",             MEDIA_DATA_DB_DATE_TAKEN_MS),
    std::make_pair("DETAIL_TIME",               PhotoColumn::PHOTO_DETAIL_TIME),
    std::make_pair("ORIENTATION",               PhotoColumn::PHOTO_ORIENTATION),
    std::make_pair("EXIF_ROTATE",               PhotoColumn::PHOTO_EXIF_ROTATE),
    std::make_pair("FAVORITE",                  MediaColumn::MEDIA_IS_FAV),
    std::make_pair("MEDIA_TYPE",                MediaColumn::MEDIA_TYPE),
    std::make_pair("DATE_TRASHED",              MediaColumn::MEDIA_DATE_TRASHED),
    std::make_pair("POSITION",                  PhotoColumn::PHOTO_POSITION),
    std::make_pair("HIDDEN",                    MediaColumn::MEDIA_HIDDEN),
    std::make_pair("SIZE",                      MediaColumn::MEDIA_SIZE),
    std::make_pair("PACKAGE_NAME",              MediaColumn::MEDIA_PACKAGE_NAME),
    std::make_pair("CAMERA_SHOT_KEY",           PhotoColumn::CAMERA_SHOT_KEY),
    std::make_pair("USER_COMMENT",              PhotoColumn::PHOTO_USER_COMMENT),
    std::make_pair("DATE_YEAR",                 PhotoColumn::PHOTO_DATE_YEAR),
    std::make_pair("DATE_MONTH",                PhotoColumn::PHOTO_DATE_MONTH),
    std::make_pair("DATE_DAY",                  PhotoColumn::PHOTO_DATE_DAY),
    std::make_pair("PENDING",                   PENDING_STATUS),
    std::make_pair("DATE_ADDED_MS",             MEDIA_DATA_DB_DATE_ADDED_MS),
    std::make_pair("DATE_MODIFIED_MS",          MEDIA_DATA_DB_DATE_MODIFIED_MS),
    std::make_pair("DATE_TRASHED_MS",           MEDIA_DATA_DB_DATE_TRASHED_MS),
    std::make_pair("PHOTO_SUBTYPE",             PhotoColumn::PHOTO_SUBTYPE),
    std::make_pair("DYNAMIC_RANGE_TYPE",        PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE),
    std::make_pair("LCD_SIZE",                  PhotoColumn::PHOTO_LCD_SIZE),
    std::make_pair("THM_SIZE",                  PhotoColumn::PHOTO_THUMB_SIZE),
    std::make_pair("MOVING_PHOTO_EFFECT_MODE",  PhotoColumn::MOVING_PHOTO_EFFECT_MODE),
    std::make_pair("COVER_POSITION",            PhotoColumn::PHOTO_COVER_POSITION),
    std::make_pair("BURST_COVER_LEVEL",         PhotoColumn::PHOTO_BURST_COVER_LEVEL),
    std::make_pair("BURST_KEY",                 PhotoColumn::PHOTO_BURST_KEY),
    std::make_pair("THUMBNAIL_READY",           PhotoColumn::PHOTO_THUMBNAIL_READY),
    std::make_pair("CE_AVAILABLE",              PhotoColumn::PHOTO_CE_AVAILABLE),
    std::make_pair("OWNER_ALBUM_ID",            PhotoColumn::PHOTO_OWNER_ALBUM_ID),
    std::make_pair("THUMBNAIL_VISIBLE",         PhotoColumn::PHOTO_THUMBNAIL_VISIBLE),
    std::make_pair("SUPPORTED_WATERMARK_TYPE",  PhotoColumn::SUPPORTED_WATERMARK_TYPE),
    std::make_pair("PHOTO_HAS_APPLINK",         PhotoColumn::PHOTO_HAS_APPLINK),
    std::make_pair("PHOTO_APPLINK",             PhotoColumn::PHOTO_APPLINK),
    std::make_pair("LATITUDE",                  PhotoColumn::PHOTO_LATITUDE),
    std::make_pair("LONGITUDE",                 PhotoColumn::PHOTO_LONGITUDE),
    std::make_pair("IS_CE_AUTO",                PhotoColumn::PHOTO_IS_AUTO),
    std::make_pair("MEDIA_SUFFIX",              PhotoColumn::PHOTO_MEDIA_SUFFIX),
    std::make_pair("IS_RECENT_SHOW",            PhotoColumn::PHOTO_IS_RECENT_SHOW),
    std::make_pair("COMPOSITE_DISPLAY_STATUS",  PhotoColumn::PHOTO_COMPOSITE_DISPLAY_STATUS),
    std::make_pair("SUM_SIZE",                  MEDIA_SUM_SIZE),
    std::make_pair("HAS_APPLINK",               PhotoColumn::PHOTO_HAS_APPLINK),
    std::make_pair("APPLINK",                   PhotoColumn::PHOTO_APPLINK),
    std::make_pair("CHANGE_TIME",               PhotoColumn::PHOTO_CHANGE_TIME),
    std::make_pair("EXIST_COMPATIBLE_DUPLICATE", PhotoColumn::PHOTO_EXIST_COMPATIBLE_DUPLICATE),
    std::make_pair("HDR_MODE",                  PhotoColumn::PHOTO_HDR_MODE),
    std::make_pair("CLOUD_ID",                  PhotoColumn::PHOTO_CLOUD_ID),
    std::make_pair("VIDEO_MODE",                PhotoColumn::PHOTO_VIDEO_MODE),
    std::make_pair("FUSION_ASSET_STORAGE_PATH", PhotoColumn::PHOTO_STORAGE_PATH),
    std::make_pair("ASSET_SOURCE_TYPE",         PhotoColumn::PHOTO_FILE_SOURCE_TYPE),
    std::make_pair("ASPECT_RATIO",              PhotoColumn::PHOTO_ASPECT_RATIO),
    std::make_pair("EDIT_DATA_EXIST",           PhotoColumn::PHOTO_EDIT_DATA_EXIST),
    std::make_pair("IS_CRITICAL",               PhotoColumn::PHOTO_IS_CRITICAL),
    std::make_pair("PHOTO_RISK_STATUS",         PhotoColumn::PHOTO_RISK_STATUS),
};

const std::vector<std::pair<std::string, std::string>> ALBUMKEY_ENUM_PROPERTIES = {
    std::make_pair("URI",                       MEDIA_DATA_DB_URI),
    std::make_pair("ALBUM_NAME",                PhotoAlbumColumns::ALBUM_NAME),
    std::make_pair("ALBUM_LPATH",               PhotoAlbumColumns::ALBUM_LPATH),
    std::make_pair("FILE_TYPE",                 MEDIA_DATA_DB_MEDIA_TYPE),
    std::make_pair("DATE_ADDED",                MEDIA_DATA_DB_DATE_ADDED),
    std::make_pair("DATE_MODIFIED",             MEDIA_DATA_DB_DATE_MODIFIED),
    std::make_pair("BUNDLE_NAME",               PhotoAlbumColumns::ALBUM_BUNDLE_NAME),
    std::make_pair("COVER_URI_SOURCE",          PhotoAlbumColumns::COVER_URI_SOURCE),
    std::make_pair("CHANGE_TIME",               PhotoAlbumColumns::CHANGE_TIME),
    std::make_pair("UPLOAD_STATUS",             PhotoAlbumColumns::UPLOAD_STATUS),
    std::make_pair("HIDDEN",                    PhotoAlbumColumns::ALBUM_HIDDEN),
};

const std::vector<std::pair<std::string, std::string>> DEFAULT_URI_ENUM_PROPERTIES = {
    std::make_pair("DEFAULT_PHOTO_URI",         PhotoColumn::DEFAULT_PHOTO_URI),
    std::make_pair("DEFAULT_ALBUM_URI",         PhotoAlbumColumns::DEFAULT_PHOTO_ALBUM_URI),
    std::make_pair("DEFAULT_AUDIO_URI",         AudioColumn::DEFAULT_AUDIO_URI),
    std::make_pair("DEFAULT_HIDDEN_ALBUM_URI",  PhotoAlbumColumns::DEFAULT_HIDDEN_ALBUM_URI),
};

const std::map<std::string, std::pair<std::string, int32_t>> LOCATION_PARAM_MAP = {
    { START_LATITUDE, { LATITUDE, DataShare::GREATER_THAN_OR_EQUAL_TO } },
    { END_LATITUDE, { LATITUDE, DataShare::LESS_THAN } },
    { START_LONGITUDE, { LONGITUDE, DataShare::GREATER_THAN_OR_EQUAL_TO } },
    { END_LONGITUDE, { LONGITUDE, DataShare::LESS_THAN } },
    { DIAMETER, { DIAMETER, DataShare::EQUAL_TO } },
    { LANGUAGE, { LANGUAGE, DataShare::EQUAL_TO } },
};

const std::vector<std::string> ALBUM_COLUMN = {
    PhotoAlbumColumns::ALBUM_ID,
    PhotoAlbumColumns::ALBUM_TYPE,
    PhotoAlbumColumns::ALBUM_SUBTYPE,
    PhotoAlbumColumns::ALBUM_NAME,
    PhotoAlbumColumns::ALBUM_COVER_URI,
    PhotoAlbumColumns::ALBUM_COUNT,
    PhotoAlbumColumns::ALBUM_IMAGE_COUNT,
    PhotoAlbumColumns::ALBUM_VIDEO_COUNT,
};

const std::vector<std::string> PHOTO_COLUMN = {
    MEDIA_DATA_DB_ID,
    MEDIA_DATA_DB_FILE_PATH,
    MEDIA_DATA_DB_MEDIA_TYPE,
    MEDIA_DATA_DB_NAME,
    MEDIA_DATA_DB_SIZE,
    MEDIA_DATA_DB_DATE_ADDED,
    MEDIA_DATA_DB_DATE_MODIFIED,
    MEDIA_DATA_DB_DURATION,
    MEDIA_DATA_DB_WIDTH,
    MEDIA_DATA_DB_HEIGHT,
    MEDIA_DATA_DB_DATE_TAKEN,
    MEDIA_DATA_DB_ORIENTATION,
    MEDIA_DATA_DB_IS_FAV,
    MEDIA_DATA_DB_TITLE,
    MEDIA_DATA_DB_POSITION,
    MEDIA_DATA_DB_DATE_TRASHED,
    MediaColumn::MEDIA_HIDDEN,
    PhotoColumn::PHOTO_USER_COMMENT,
    PhotoColumn::CAMERA_SHOT_KEY,
    PhotoColumn::PHOTO_DATE_YEAR,
    PhotoColumn::PHOTO_DATE_MONTH,
    PhotoColumn::PHOTO_DATE_DAY,
    MEDIA_DATA_DB_TIME_PENDING,
    PhotoColumn::PHOTO_SUBTYPE,
    PhotoColumn::MOVING_PHOTO_EFFECT_MODE,
    PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE,
    PhotoColumn::PHOTO_THUMBNAIL_READY,
    PhotoColumn::PHOTO_LCD_SIZE,
    PhotoColumn::PHOTO_THUMB_SIZE,
};

const std::set<std::string> TIME_COLUMN = {
    MEDIA_DATA_DB_DATE_ADDED,
    MEDIA_DATA_DB_DATE_MODIFIED,
    MEDIA_DATA_DB_DATE_TRASHED,
};

const std::vector<std::pair<std::string, std::string>> SUPPORTED_IMAGE_FORMAT_ENUM_PROPERTIES = {
    std::make_pair("AVFILE_FORMAT_JPG", "jpg"),
};

} // Media
} // OHOS
#endif // MEDIALIBRARY_NAPI_ENUM_COMM_H
