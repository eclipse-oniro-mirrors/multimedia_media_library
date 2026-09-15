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

#include "media_result_type_map_utils.h"
#include "medialibrary_db_const.h"
#include "custom_records_column.h"
#include "media_column.h"

namespace OHOS::Media {
const std::unordered_map<std::string, ResultSetDataType> &MediaResultTypeMapUtils::GetResultTypeMap()
{
    static const ResultTypeMap RESULT_TYPE_MAP = {
        {CONST_MEDIA_DATA_DB_ID, TYPE_INT32},
        {CONST_MEDIA_DATA_DB_NAME, TYPE_STRING},
        {CONST_MEDIA_DATA_DB_RELATIVE_PATH, TYPE_STRING},
        {CONST_MEDIA_DATA_DB_MEDIA_TYPE, TYPE_INT32},
        {CONST_MEDIA_DATA_DB_PARENT_ID, TYPE_INT32},
        {CONST_MEDIA_DATA_DB_SIZE, TYPE_INT64},
        {CONST_MEDIA_DATA_DB_DATE_ADDED, TYPE_INT64},
        {CONST_MEDIA_DATA_DB_DATE_MODIFIED, TYPE_INT64},
        {CONST_MEDIA_DATA_DB_DATE_TAKEN, TYPE_INT64},
        {CONST_MEDIA_DATA_DB_FILE_PATH, TYPE_STRING},
        {CONST_MEDIA_DATA_DB_MIME_TYPE, TYPE_STRING},
        {CONST_MEDIA_DATA_DB_TITLE, TYPE_STRING},
        {CONST_MEDIA_DATA_DB_ARTIST, TYPE_STRING},
        {CONST_MEDIA_DATA_DB_ALBUM, TYPE_STRING},
        {CONST_MEDIA_DATA_DB_WIDTH, TYPE_INT32},
        {CONST_MEDIA_DATA_DB_HEIGHT, TYPE_INT32},
        {CONST_MEDIA_DATA_DB_DURATION, TYPE_INT32},
        {CONST_MEDIA_DATA_DB_ORIENTATION, TYPE_INT32},
        {CONST_MEDIA_DATA_DB_BUCKET_ID, TYPE_INT32},
        {CONST_MEDIA_DATA_DB_BUCKET_NAME, TYPE_STRING},
        {CONST_MEDIA_DATA_DB_TIME_PENDING, TYPE_INT64},
        {CONST_MEDIA_DATA_DB_IS_FAV, TYPE_INT32},
        {CONST_MEDIA_DATA_DB_DATE_TRASHED, TYPE_INT64},
        {CONST_MEDIA_DATA_DB_SELF_ID, TYPE_STRING},
        {CONST_MEDIA_DATA_DB_RECYCLE_PATH, TYPE_STRING},
        {CONST_MEDIA_DATA_DB_IS_TRASH, TYPE_INT32},
        {CONST_MEDIA_DATA_DB_AUDIO_ALBUM, TYPE_STRING},
        {CONST_MEDIA_DATA_DB_OWNER_PACKAGE, TYPE_STRING},
        {CONST_MEDIA_DATA_DB_OWNER_APPID, TYPE_STRING},
        {MediaColumn::MEDIA_PACKAGE_NAME, TYPE_STRING},
        {CONST_MEDIA_DATA_DB_POSITION, TYPE_INT32},
        {MediaColumn::MEDIA_HIDDEN, TYPE_INT32},
        {MediaColumn::MEDIA_VIRTUAL_PATH, TYPE_STRING},
        {PhotoColumn::PHOTO_SUBTYPE, TYPE_INT32},
        {CONST_MEDIA_COLUMN_COUNT, TYPE_INT32},
        {PhotoColumn::CAMERA_SHOT_KEY, TYPE_STRING},
        {PhotoColumn::PHOTO_ALL_EXIF, TYPE_STRING},
        {PhotoColumn::PHOTO_USER_COMMENT, TYPE_STRING},
        {CONST_PHOTO_INDEX, TYPE_INT32},
        {CONST_MEDIA_DATA_DB_COUNT, TYPE_INT32},
        {PhotoColumn::PHOTO_DATE_YEAR, TYPE_STRING},
        {PhotoColumn::PHOTO_DATE_MONTH, TYPE_STRING},
        {PhotoColumn::PHOTO_DATE_DAY, TYPE_STRING},
        {PhotoColumn::PHOTO_SHOOTING_MODE, TYPE_STRING},
        {PhotoColumn::PHOTO_SHOOTING_MODE_TAG, TYPE_STRING},
        {PhotoColumn::PHOTO_LAST_VISIT_TIME, TYPE_INT64},
        {PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE, TYPE_INT32},
        {PhotoColumn::PHOTO_HDR_MODE, TYPE_INT32},
        {PhotoColumn::PHOTO_LCD_SIZE, TYPE_STRING},
        {PhotoColumn::PHOTO_THUMB_SIZE, TYPE_STRING},
        {PhotoColumn::MOVING_PHOTO_EFFECT_MODE, TYPE_INT32},
        {PhotoColumn::PHOTO_COVER_POSITION, TYPE_INT64},
        {PhotoColumn::PHOTO_FRONT_CAMERA, TYPE_STRING},
        {PhotoColumn::PHOTO_ORIGINAL_SUBTYPE, TYPE_INT32},
        {PhotoColumn::PHOTO_BURST_COVER_LEVEL, TYPE_INT32},
        {PhotoColumn::PHOTO_BURST_KEY, TYPE_STRING},
        {PhotoColumn::PHOTO_THUMBNAIL_READY, TYPE_INT64},
        {PhotoColumn::PHOTO_CE_AVAILABLE, TYPE_INT32},
        {PhotoColumn::PHOTO_DETAIL_TIME, TYPE_STRING},
        {PhotoColumn::PHOTO_OWNER_ALBUM_ID, TYPE_INT32},
        {PhotoColumn::PHOTO_THUMBNAIL_VISIBLE, TYPE_INT32},
        {PhotoColumn::SUPPORTED_WATERMARK_TYPE, TYPE_INT32},
        {PhotoColumn::PHOTO_QUALITY, TYPE_INT32},
        {PhotoColumn::PHOTO_CLOUD_ID, TYPE_STRING},
        {PhotoColumn::PHOTO_IS_AUTO, TYPE_INT32},
        {PhotoColumn::PHOTO_MEDIA_SUFFIX, TYPE_STRING},
        {PhotoColumn::PHOTO_IS_RECENT_SHOW, TYPE_INT32},
        {PhotoColumn::PHOTO_COMPOSITE_DISPLAY_STATUS, TYPE_INT32},
        {PhotoColumn::IS_STYLE_PHOTO, TYPE_INT32},
        {PhotoColumn::PHOTO_HAS_APPLINK, TYPE_INT32},
        {PhotoColumn::PHOTO_APPLINK, TYPE_STRING},
        {CONST_MEDIA_SUM_SIZE, TYPE_INT64},
        {CustomRecordsColumns::FILE_ID, TYPE_INT32},
        {CustomRecordsColumns::BUNDLE_NAME, TYPE_STRING},
        {CustomRecordsColumns::SHARE_COUNT, TYPE_INT32},
        {CustomRecordsColumns::LCD_JUMP_COUNT, TYPE_INT32},
        {PhotoColumn::PHOTO_EXIST_COMPATIBLE_DUPLICATE, TYPE_INT32},
        {PhotoColumn::PHOTO_XT_STYLE_TEMPLATE_NAME, TYPE_STRING},
        {PhotoColumn::PHOTO_EXIF_ROTATE, TYPE_INT32},
        {PhotoColumn::SUPPORTED_DEFERRED_EFFECTS, TYPE_INT32},
        {PhotoColumn::DEFERRED_EFFECT_STATUS, TYPE_INT32},
        {PhotoColumn::PHOTO_STORAGE_PATH, TYPE_STRING},
        {PhotoColumn::PHOTO_FILE_SOURCE_TYPE, TYPE_INT32},
        {PhotoColumn::PHOTO_VIDEO_MODE, TYPE_INT32},
        {PhotoColumn::PHOTO_EDIT_DATA_EXIST, TYPE_INT32},
        {PhotoColumn::PHOTO_DIRTY, TYPE_INT32},
        {PhotoColumn::PHOTO_ASPECT_RATIO, TYPE_DOUBLE},
        {PhotoColumn::PHOTO_CHANGE_TIME, TYPE_INT64},
        {PhotoColumn::PHOTO_IS_CRITICAL, TYPE_INT32},
        {PhotoColumn::PHOTO_RISK_STATUS, TYPE_INT32},
        {PhotoColumn::PHOTO_DATE_ADDED_DAY, TYPE_STRING},
        {PhotoColumn::PHOTO_DATE_ADDED_MONTH, TYPE_STRING},
        {PhotoColumn::PHOTO_DATE_ADDED_YEAR, TYPE_STRING},
        {PhotoColumn::PHOTO_HIDDEN_TIME, TYPE_INT64},
        {PhotoColumn::LOCAL_ASSET_SIZE, TYPE_INT64},
        {PhotoColumn::PHOTO_TRANS_CODE_FILE_SIZE, TYPE_INT64},
        {PhotoColumn::PHOTO_TRANSCODE_TIME, TYPE_INT64},
        {PhotoColumn::ATTACHMENT_SIZE, TYPE_INT64},
        {PhotoColumn::MUSIC_MASTER_MODE, TYPE_INT32},
        {PhotoColumn::PHOTO_THUMB_STATUS, TYPE_INT32},
        {PhotoColumn::PHOTO_LCD_FILE_SIZE, TYPE_INT32},
        {PhotoColumn::COMPRESSION_QUALITY, TYPE_INT32},
        {PhotoColumn::PHOTO_IS_SHARED, TYPE_INT32},
    };
    return RESULT_TYPE_MAP;
}
}  // namespace OHOS::Media