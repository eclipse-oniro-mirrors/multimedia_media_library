/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_MEDIA_LIBRARY_NAPI_DEF_H_
#define INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_MEDIA_LIBRARY_NAPI_DEF_H_

#include <string>
#include <set>
#include "datashare_predicates_def.h"
#include "media_column.h"

namespace OHOS {
namespace Media {
static const unordered_set<DataShare::OperationType> API23_PLUS_OPERATIONS = {
    DataShare::NOT_EQUAL_TO,
    DataShare::LIKE,
    DataShare::BETWEEN,
    DataShare::NOTBETWEEN,
    DataShare::GREATER_THAN,
    DataShare::LESS_THAN,
    DataShare::GREATER_THAN_OR_EQUAL_TO,
    DataShare::LESS_THAN_OR_EQUAL_TO,
    DataShare::NOT_IN,
};

static const unordered_set<std::string> PUBLIC_PHOTO_KEYS = {
    CONST_MEDIA_DATA_DB_URI,
    MediaColumn::MEDIA_NAME,
    MediaColumn::MEDIA_DATE_ADDED,
    MediaColumn::MEDIA_TYPE,
    MediaColumn::MEDIA_DATE_MODIFIED,
    MediaColumn::MEDIA_TITLE,
    MediaColumn::MEDIA_DURATION,
    PhotoColumn::PHOTO_WIDTH,
    PhotoColumn::PHOTO_HEIGHT,
    MediaColumn::MEDIA_DATE_TAKEN,
    CONST_MEDIA_DATA_DB_DATE_TAKEN_MS,
    PhotoColumn::PHOTO_DETAIL_TIME,
    PhotoColumn::PHOTO_ORIENTATION,
    MediaColumn::MEDIA_IS_FAV,
    PhotoColumn::PHOTO_POSITION,
    MediaColumn::MEDIA_SIZE,
    CONST_MEDIA_DATA_DB_DATE_ADDED_MS,
    CONST_MEDIA_DATA_DB_DATE_MODIFIED_MS,
    PhotoColumn::PHOTO_SUBTYPE,
    PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE,
    PhotoColumn::PHOTO_LCD_SIZE,
    PhotoColumn::PHOTO_THUMB_SIZE,
    PhotoColumn::PHOTO_COVER_POSITION,
    PhotoColumn::PHOTO_BURST_COVER_LEVEL,
    PhotoColumn::PHOTO_BURST_KEY,
    PhotoColumn::PHOTO_OWNER_ALBUM_ID,
    PhotoColumn::PHOTO_LATITUDE,
    PhotoColumn::PHOTO_LONGITUDE,
    PhotoColumn::PHOTO_MEDIA_SUFFIX,
    PhotoColumn::PHOTO_CHANGE_TIME,
    PhotoColumn::PHOTO_ASPECT_RATIO,
    PhotoColumn::PHOTO_IS_CRITICAL
};
static const unordered_set<std::string> PUBLIC_ALBUM_KEYS = {
    CONST_MEDIA_DATA_DB_URI,
    PhotoAlbumColumns::ALBUM_NAME,
    PhotoAlbumColumns::ALBUM_LPATH,
    CONST_MEDIA_DATA_DB_MEDIA_TYPE,
    CONST_MEDIA_DATA_DB_DATE_ADDED,
    PhotoAlbumColumns::CHANGE_TIME
};

} // namespace Media
} // namespace OHOS
#endif // INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_MEDIA_LIBRARY_NAPI_DEF_H_