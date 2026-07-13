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

#ifndef OHOS_MEDIALIBRARY_ALBUM_COMPATIBILITY_FUSION_DATA_SQL_H
#define OHOS_MEDIALIBRARY_ALBUM_COMPATIBILITY_FUSION_DATA_SQL_H

#include "media_column.h"
#include "photo_map_column.h"
#include "source_album.h"

namespace OHOS {
namespace Media {
const std::string DROP_PHOTO_ALBUM_CLEAR_MAP_SQL =
    "DROP TRIGGER IF EXISTS photo_album_clear_map";
const std::string DROP_INSERT_PHOTO_INSERT_SOURCE_ALBUM_SQL =
    "DROP TRIGGER IF EXISTS insert_photo_insert_source_album";
const std::string DROP_INSERT_PHOTO_UPDATE_SOURCE_ALBUM_SQL =
    "DROP TRIGGER IF EXISTS insert_photo_update_source_album";

const std::string DROP_INSERT_SOURCE_PHOTO_CREATE_SOURCE_ALBUM_TRIGGER =
    "DROP TRIGGER IF EXISTS insert_source_photo_create_source_album_trigger";
const std::string DROP_INSERT_SOURCE_PHOTO_UPDATE_ALBUM_ID_TRIGGER =
    "DROP TRIGGER IF EXISTS insert_source_photo_update_album_id_trigger";

const std::string DROP_INDEX_SOURCE_ALBUM_INDEX =
    "DROP INDEX IF EXISTS " + SOURCE_ALBUM_INDEX;

const std::string QUERY_NOT_MATCHED_DATA_IN_PHOTOMAP_BY_PAGE =
    "SELECT " + PhotoMap::ASSET_ID + ", " + PhotoMap::ALBUM_ID + " FROM " + PhotoMap::TABLE +
    " WHERE dirty <>4 and dirty<>6 LIMIT 0, 200";

const std::string CREATE_DEFALUT_ALBUM_FOR_NO_RELATIONSHIP_ASSET =
    "INSERT INTO " + PhotoAlbumColumns::TABLE +
        "(album_type, album_subtype, album_name,bundle_name, dirty, is_local, date_modified, " +
        "date_added, lpath, priority) Values ('2048', '2049', '其它', 'com.other.album', '1', '1', " +
        "strftime('%s000', 'now'), strftime('%s000', 'now'), '/Pictures/其它', '1')";

const std::string CREATE_HIDDEN_ALBUM_FOR_DUAL_ASSET =
    "INSERT INTO " + PhotoAlbumColumns::TABLE +
        "(album_type, album_subtype, album_name, bundle_name, dirty, is_local, date_added, lpath, priority)"
        " Values ('2048', '2049', '.hiddenAlbum', 'com.hidden.album', '1', "
        "'1', strftime('%s000', 'now'), '/Pictures/hiddenAlbum', '1')";
} // namespace Media
} // namespace OHOS
#endif  // OHOS_MEDIALIBRARY_ALBUM_COMPATIBILITY_FUSION_DATA_SQL_H