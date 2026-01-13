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
#ifndef FRAMEWORKS_MEDIATOOLS_UTILS_DB_CONST_H_
#define FRAMEWORKS_MEDIATOOLS_UTILS_DB_CONST_H_
#include <string>
#include <unordered_map>

#include "fetch_result.h"
#include "media_column.h"
#include "userfile_manager_types.h"
#include "media_audio_column.h"

namespace OHOS {
namespace Media {
namespace MediaTool {
const std::unordered_map<std::string, ResultSetDataType> RESULT_TYPE_MAP = {
    { MEDIA_DATA_DB_URI, ResultSetDataType::TYPE_STRING},
    { MediaColumn::MEDIA_ID, ResultSetDataType::TYPE_INT32 },
    { MediaColumn::MEDIA_FILE_PATH, ResultSetDataType::TYPE_STRING },
    { MediaColumn::MEDIA_SIZE, ResultSetDataType::TYPE_INT64 },
    { MediaColumn::MEDIA_TITLE, ResultSetDataType::TYPE_STRING },
    { MediaColumn::MEDIA_NAME, ResultSetDataType::TYPE_STRING },
    { MediaColumn::MEDIA_TYPE, ResultSetDataType::TYPE_INT32 },
    { MediaColumn::MEDIA_MIME_TYPE, ResultSetDataType::TYPE_STRING },
    { MediaColumn::MEDIA_OWNER_PACKAGE, ResultSetDataType::TYPE_STRING },
    { MediaColumn::MEDIA_OWNER_APPID, ResultSetDataType::TYPE_STRING },
    { MediaColumn::MEDIA_PACKAGE_NAME, ResultSetDataType::TYPE_STRING },
    { MediaColumn::MEDIA_DEVICE_NAME, ResultSetDataType::TYPE_STRING },
    { MediaColumn::MEDIA_DATE_ADDED, ResultSetDataType::TYPE_INT64 },
    { MediaColumn::MEDIA_DATE_MODIFIED, ResultSetDataType::TYPE_INT64 },
    { MediaColumn::MEDIA_DATE_TAKEN, ResultSetDataType::TYPE_INT64 },
    { MediaColumn::MEDIA_DURATION, ResultSetDataType::TYPE_INT32 },
    { MediaColumn::MEDIA_TIME_PENDING, ResultSetDataType::TYPE_INT64 },
    { MediaColumn::MEDIA_IS_FAV, ResultSetDataType::TYPE_INT32 },
    { MediaColumn::MEDIA_DATE_TRASHED, ResultSetDataType::TYPE_INT64 },
    { MediaColumn::MEDIA_DATE_DELETED, ResultSetDataType::TYPE_INT64 },
    { MediaColumn::MEDIA_HIDDEN, ResultSetDataType::TYPE_INT32 },
    { MediaColumn::MEDIA_PARENT_ID, ResultSetDataType::TYPE_INT32 },
    { MediaColumn::MEDIA_RELATIVE_PATH, ResultSetDataType::TYPE_STRING },
    { MediaColumn::MEDIA_VIRTURL_PATH, ResultSetDataType::TYPE_STRING },
    { AudioColumn::AUDIO_ARTIST, ResultSetDataType::TYPE_STRING },
    { AudioColumn::AUDIO_ALBUM, ResultSetDataType::TYPE_STRING },
    { PhotoColumn::PHOTO_DIRTY, ResultSetDataType::TYPE_INT32 },
    { PhotoColumn::PHOTO_CLOUD_ID, ResultSetDataType::TYPE_STRING },
    { PhotoColumn::PHOTO_META_DATE_MODIFIED, ResultSetDataType::TYPE_INT64 },
    { PhotoColumn::PHOTO_SYNC_STATUS, ResultSetDataType::TYPE_INT32 },
    { PhotoColumn::PHOTO_CLOUD_VERSION, ResultSetDataType::TYPE_INT64 },
    { PhotoColumn::PHOTO_ORIENTATION, ResultSetDataType::TYPE_INT32 },
    { PhotoColumn::PHOTO_LATITUDE, ResultSetDataType::TYPE_DOUBLE },
    { PhotoColumn::PHOTO_LONGITUDE, ResultSetDataType::TYPE_DOUBLE },
    { PhotoColumn::PHOTO_HEIGHT, ResultSetDataType::TYPE_INT32 },
    { PhotoColumn::PHOTO_WIDTH, ResultSetDataType::TYPE_INT32 },
    { PhotoColumn::PHOTO_LCD_VISIT_TIME, ResultSetDataType::TYPE_INT64 },
    { PhotoColumn::PHOTO_EDIT_TIME, ResultSetDataType::TYPE_INT64 },
    { PhotoColumn::PHOTO_POSITION, ResultSetDataType::TYPE_INT32 },
    { PhotoColumn::PHOTO_CLEAN_FLAG, ResultSetDataType::TYPE_INT32 },
    { PhotoColumn::PHOTO_THUMB_STATUS, ResultSetDataType::TYPE_INT32 },
    { PhotoColumn::PHOTO_SUBTYPE, ResultSetDataType::TYPE_INT32 },
    { PhotoColumn::PHOTO_ORIGINAL_SUBTYPE, ResultSetDataType::TYPE_INT32 },
    { PhotoColumn::CAMERA_SHOT_KEY, ResultSetDataType::TYPE_STRING },
    { PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE, TYPE_INT32},
    { PhotoColumn::MOVING_PHOTO_EFFECT_MODE, TYPE_INT32},
    { PhotoColumn::PHOTO_BURST_COVER_LEVEL, ResultSetDataType::TYPE_INT32 },
    { PhotoColumn::PHOTO_BURST_KEY, ResultSetDataType::TYPE_STRING },
    { PhotoColumn::PHOTO_CE_AVAILABLE, ResultSetDataType::TYPE_INT32 },
    { PhotoColumn::PHOTO_COVER_POSITION, ResultSetDataType::TYPE_INT64 },
};
} // namespace MediaTool
} // namespace Media
} // namespace OHOS
#endif // FRAMEWORKS_MEDIATOOLS_UTILS_DB_CONST_H_
