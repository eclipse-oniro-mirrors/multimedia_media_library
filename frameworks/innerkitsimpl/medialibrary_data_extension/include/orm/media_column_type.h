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

#ifndef OHOS_MEDIA_ORM_MEDIA_COLUMN_TYPE_H
#define OHOS_MEDIA_ORM_MEDIA_COLUMN_TYPE_H

#include <string>
#include <map>

#include "media_column.h"
#include "photo_album_column.h"

namespace OHOS::Media::ORM {
namespace MediaColumnType {
enum class DataType : int32_t { INT, LONG, DOUBLE, STRING };

static const std::map<std::string, DataType> PHOTOS_COLUMNS = {
    {PhotoColumn::MEDIA_FILE_PATH, DataType::STRING},
    {PhotoColumn::MEDIA_SIZE, DataType::LONG},
    {PhotoColumn::MEDIA_TITLE, DataType::STRING},
    {PhotoColumn::MEDIA_NAME, DataType::STRING},
    {PhotoColumn::MEDIA_TYPE, DataType::INT},
    {PhotoColumn::MEDIA_MIME_TYPE, DataType::STRING},
    {PhotoColumn::MEDIA_OWNER_PACKAGE, DataType::STRING},
    {PhotoColumn::MEDIA_OWNER_APPID, DataType::STRING},
    {PhotoColumn::MEDIA_PACKAGE_NAME, DataType::STRING},
    {PhotoColumn::MEDIA_DEVICE_NAME, DataType::STRING},
    {PhotoColumn::MEDIA_DATE_ADDED, DataType::LONG},
    {PhotoColumn::MEDIA_DATE_MODIFIED, DataType::LONG},
    {PhotoColumn::MEDIA_DATE_TAKEN, DataType::LONG},
    {PhotoColumn::MEDIA_DURATION, DataType::INT},
    {PhotoColumn::MEDIA_TIME_PENDING, DataType::LONG},
    {PhotoColumn::MEDIA_IS_FAV, DataType::INT},
    {PhotoColumn::MEDIA_DATE_TRASHED, DataType::LONG},
    {PhotoColumn::MEDIA_DATE_DELETED, DataType::LONG},
    {PhotoColumn::MEDIA_HIDDEN, DataType::INT},
    {PhotoColumn::MEDIA_PARENT_ID, DataType::INT},
    {PhotoColumn::MEDIA_RELATIVE_PATH, DataType::STRING},
    {PhotoColumn::MEDIA_VIRTUAL_PATH, DataType::STRING},
    {PhotoColumn::PHOTO_DIRTY, DataType::INT},
    /* keep cloud_id at the last, DataType:: so RecordToValueBucket can skip it*/
    {PhotoColumn::PHOTO_CLOUD_ID, DataType::STRING},
    {PhotoColumn::PHOTO_META_DATE_MODIFIED, DataType::LONG},
    {PhotoColumn::PHOTO_SYNC_STATUS, DataType::INT},
    {PhotoColumn::PHOTO_CLOUD_VERSION, DataType::LONG},
    {PhotoColumn::PHOTO_ORIENTATION, DataType::INT},
    {PhotoColumn::PHOTO_LATITUDE, DataType::DOUBLE},
    {PhotoColumn::PHOTO_LONGITUDE, DataType::DOUBLE},
    {PhotoColumn::PHOTO_HEIGHT, DataType::INT},
    {PhotoColumn::PHOTO_WIDTH, DataType::INT},
    {PhotoColumn::PHOTO_EDIT_TIME, DataType::LONG},
    {PhotoColumn::PHOTO_LCD_VISIT_TIME, DataType::LONG},
    {PhotoColumn::PHOTO_POSITION, DataType::INT},
    {PhotoColumn::PHOTO_SUBTYPE, DataType::INT},
    {PhotoColumn::PHOTO_ORIGINAL_SUBTYPE, DataType::INT},
    {PhotoColumn::CAMERA_SHOT_KEY, DataType::STRING},
    {PhotoColumn::PHOTO_USER_COMMENT, DataType::STRING},
    {PhotoColumn::PHOTO_ALL_EXIF, DataType::STRING},
    {PhotoColumn::PHOTO_DATE_YEAR, DataType::STRING},
    {PhotoColumn::PHOTO_DATE_MONTH, DataType::STRING},
    {PhotoColumn::PHOTO_DATE_DAY, DataType::STRING},
    {PhotoColumn::PHOTO_SHOOTING_MODE, DataType::STRING},
    {PhotoColumn::PHOTO_SHOOTING_MODE_TAG, DataType::STRING},
    {PhotoColumn::PHOTO_LAST_VISIT_TIME, DataType::LONG},
    {PhotoColumn::PHOTO_HIDDEN_TIME, DataType::LONG},
    {PhotoColumn::PHOTO_THUMB_STATUS, DataType::INT},
    {PhotoColumn::PHOTO_CLEAN_FLAG, DataType::INT},
    {PhotoColumn::PHOTO_ID, DataType::STRING},
    {PhotoColumn::PHOTO_QUALITY, DataType::INT},
    {PhotoColumn::PHOTO_FIRST_VISIT_TIME, DataType::LONG},
    {PhotoColumn::PHOTO_DEFERRED_PROC_TYPE, DataType::INT},
    {PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE, DataType::INT},
    {PhotoColumn::PHOTO_HDR_MODE, DataType::INT},
    {PhotoColumn::MOVING_PHOTO_EFFECT_MODE, DataType::INT},
    {PhotoColumn::PHOTO_COVER_POSITION, DataType::LONG},
    {PhotoColumn::PHOTO_IS_RECTIFICATION_COVER, DataType::INT},
    {PhotoColumn::PHOTO_EXIF_ROTATE, DataType::INT},
    {PhotoColumn::PHOTO_THUMBNAIL_READY, DataType::LONG},
    {PhotoColumn::PHOTO_LCD_SIZE, DataType::STRING},
    {PhotoColumn::PHOTO_THUMB_SIZE, DataType::STRING},
    {PhotoColumn::PHOTO_FRONT_CAMERA, DataType::STRING},
    {PhotoColumn::PHOTO_IS_TEMP, DataType::INT},
    {PhotoColumn::PHOTO_BURST_COVER_LEVEL, DataType::INT},
    {PhotoColumn::PHOTO_BURST_KEY, DataType::STRING},
    {PhotoColumn::PHOTO_CE_AVAILABLE, DataType::INT},
    {PhotoColumn::PHOTO_CE_STATUS_CODE, DataType::INT},
    {PhotoColumn::PHOTO_STRONG_ASSOCIATION, DataType::INT},
    {PhotoColumn::PHOTO_ASSOCIATE_FILE_ID, DataType::INT},
    {PhotoColumn::PHOTO_HAS_CLOUD_WATERMARK, DataType::INT},
    {PhotoColumn::PHOTO_COMPOSITE_DISPLAY_STATUS, DataType::INT},
    {PhotoColumn::PHOTO_DETAIL_TIME, DataType::STRING},
    {PhotoColumn::PHOTO_OWNER_ALBUM_ID, DataType::INT},
    {PhotoColumn::PHOTO_ORIGINAL_ASSET_CLOUD_ID, DataType::STRING},
    {PhotoColumn::PHOTO_THUMBNAIL_VISIBLE, DataType::INT},
    {PhotoColumn::PHOTO_SOURCE_PATH, DataType::STRING},
    {PhotoColumn::SUPPORTED_WATERMARK_TYPE, DataType::INT},
    {PhotoColumn::PHOTO_METADATA_FLAGS, DataType::INT},
    {PhotoColumn::PHOTO_CHECK_FLAG, DataType::INT},
    {PhotoColumn::STAGE_VIDEO_TASK_STATUS, DataType::INT},
    {PhotoColumn::PHOTO_IS_AUTO, DataType::INT},
    {MediaColumn::MEDIA_ID, DataType::INT},
    {"album_cloud_id", DataType::STRING},
    {"lpath", DataType::STRING},
    {PhotoColumn::PHOTO_VIDEO_MODE, DataType::INT},
    {PhotoColumn::PHOTO_FILE_SOURCE_TYPE, DataType::INT},
    {PhotoColumn::PHOTO_STORAGE_PATH, DataType::STRING},
    {PhotoColumn::PHOTO_SOUTH_DEVICE_TYPE, DataType::INT},
};

static const std::map<std::string, DataType> PHOTO_ALBUM_COLUMNS = {
    {PhotoAlbumColumns::ALBUM_ID, DataType::INT},
    {PhotoAlbumColumns::ALBUM_TYPE, DataType::INT},
    {PhotoAlbumColumns::ALBUM_SUBTYPE, DataType::INT},
    {PhotoAlbumColumns::ALBUM_NAME, DataType::STRING},
    {PhotoAlbumColumns::ALBUM_COVER_URI, DataType::STRING},
    {PhotoAlbumColumns::ALBUM_COUNT, DataType::INT},
    {PhotoAlbumColumns::ALBUM_DATE_MODIFIED, DataType::LONG},
    {PhotoAlbumColumns::ALBUM_DIRTY, DataType::INT},
    {PhotoAlbumColumns::ALBUM_CLOUD_ID, DataType::STRING},
    {PhotoAlbumColumns::ALBUM_RELATIVE_PATH, DataType::STRING},
    {PhotoAlbumColumns::CONTAINS_HIDDEN, DataType::INT},
    {PhotoAlbumColumns::HIDDEN_COUNT, DataType::INT},
    {PhotoAlbumColumns::HIDDEN_COVER, DataType::STRING},
    {PhotoAlbumColumns::ALBUM_ORDER, DataType::INT},
    {PhotoAlbumColumns::ALBUM_IMAGE_COUNT, DataType::INT},
    {PhotoAlbumColumns::ALBUM_VIDEO_COUNT, DataType::INT},
    {PhotoAlbumColumns::ALBUM_BUNDLE_NAME, DataType::STRING},
    {PhotoAlbumColumns::ALBUM_LOCAL_LANGUAGE, DataType::STRING},
    {PhotoAlbumColumns::ALBUM_IS_LOCAL, DataType::INT},
    {PhotoAlbumColumns::ALBUM_DATE_ADDED, DataType::LONG},
    {PhotoAlbumColumns::ALBUM_LPATH, DataType::STRING},
    {PhotoAlbumColumns::ALBUM_PRIORITY, DataType::INT},
    {PhotoAlbumColumns::ALBUM_CHECK_FLAG, DataType::INT},
    {PhotoAlbumColumns::COVER_URI_SOURCE, DataType::INT},
    {PhotoAlbumColumns::COVER_CLOUD_ID, DataType::STRING},
    {PhotoAlbumColumns::UPLOAD_STATUS, DataType::INT},
};
}  // namespace MediaColumnType
}  // namespace OHOS::Media::ORM
#endif  // OHOS_MEDIA_ORM_MEDIA_COLUMN_TYPE_H
