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

#ifndef INTERFACES_INNERAPI_MEDIA_LIBRARY_HELPER_INCLUDE_SOURCE_ALBUM_H
#define INTERFACES_INNERAPI_MEDIA_LIBRARY_HELPER_INCLUDE_SOURCE_ALBUM_H

#include "media_column.h"
#include "photo_album_column.h"
#include "photo_map_column.h"

namespace OHOS {
namespace Media {
const std::string COVER_URI_VALUE_INSERT =
    " (SELECT '" + PhotoColumn::PHOTO_URI_PREFIX + "'||NEW." + MediaColumn::MEDIA_ID + "||" +
    "(SELECT SUBSTR(NEW." + MediaColumn::MEDIA_FILE_PATH +
    ", (SELECT LENGTH(NEW." + MediaColumn::MEDIA_FILE_PATH +
    ") - INSTR(reverseStr, '/') + 1) , (SELECT (SELECT LENGTH(NEW." +
    MediaColumn::MEDIA_FILE_PATH + ") - INSTR(reverseStr, '.')) - (SELECT LENGTH(NEW." +
    MediaColumn::MEDIA_FILE_PATH + ") - INSTR(reverseStr, '/')))) from (select " +
    " (WITH RECURSIVE reverse_string(str, revstr) AS ( SELECT NEW." +
    MediaColumn::MEDIA_FILE_PATH + ", '' UNION ALL SELECT SUBSTR(str, 1, LENGTH(str) - 1), " +
    "revstr || SUBSTR(str, LENGTH(str), 1) FROM reverse_string WHERE LENGTH(str) > 1 ) " +
    " SELECT revstr || str FROM reverse_string WHERE LENGTH(str) = 1) as reverseStr)) ||'/'||NEW." +
    MediaColumn::MEDIA_NAME + ")";

const std::string COVER_URI_VALUE_UPDATE =
    "( SELECT '" + PhotoColumn::PHOTO_URI_PREFIX + "' || fileId ||" +
    " ( SELECT SUBSTR( filePath, ( SELECT LENGTH( filePath ) - INSTR ( reverseStr, '/' ) + 1 )," +
    " ( SELECT ( SELECT LENGTH( filePath ) - INSTR ( reverseStr, '.' ) ) -" +
    " ( SELECT LENGTH( filePath ) - INSTR ( reverseStr, '/' ) ) ) ) ) || '/' || fileName" +
    " FROM ( SELECT fileId, filePath, fileName, ( WITH RECURSIVE reverse_string ( str, revstr ) AS" +
    " ( SELECT filePath, '' UNION ALL SELECT SUBSTR( str, 1, LENGTH( str ) - 1 ), revstr ||" +
    " SUBSTR( str, LENGTH( str ), 1 ) FROM reverse_string WHERE LENGTH( str ) > 1 ) SELECT revstr ||" +
    " str FROM reverse_string WHERE LENGTH( str ) = 1 ) AS reverseStr FROM ( SELECT " +
    MediaColumn::MEDIA_ID + " AS fileId, " +
    MediaColumn::MEDIA_FILE_PATH + " AS filePath, " +
    MediaColumn::MEDIA_NAME + " AS fileName FROM " +
    PhotoColumn::PHOTOS_TABLE +
    " WHERE " +
    MediaColumn::MEDIA_PACKAGE_NAME + " = OLD." + MediaColumn::MEDIA_PACKAGE_NAME + " AND " +
    MediaColumn::MEDIA_TIME_PENDING + " = 0 AND " +
    MediaColumn::MEDIA_DATE_TRASHED + " = 0 AND " +
    MediaColumn::MEDIA_HIDDEN + " = 0 ORDER BY " +
    MediaColumn::MEDIA_DATE_MODIFIED + " DESC LIMIT 1 ) ) )";

const std::string COUNT_VALUE_INSERT =
    " (SELECT COUNT(1) FROM " + PhotoColumn::PHOTOS_TABLE +
    " WHERE " +
    MediaColumn::MEDIA_PACKAGE_NAME + " = NEW." + MediaColumn::MEDIA_PACKAGE_NAME + " AND " +
    MediaColumn::MEDIA_TIME_PENDING + " = 0 AND " +
    MediaColumn::MEDIA_DATE_TRASHED + " = 0 AND " +
    MediaColumn::MEDIA_HIDDEN + " = 0 )";

const std::string COUNT_VALUE_UPDATE =
    " (SELECT COUNT(1) FROM " + PhotoColumn::PHOTOS_TABLE +
    " WHERE " +
    MediaColumn::MEDIA_PACKAGE_NAME + " = OLD." + MediaColumn::MEDIA_PACKAGE_NAME + " AND " +
    MediaColumn::MEDIA_TIME_PENDING + " = 0 AND " +
    MediaColumn::MEDIA_DATE_TRASHED + " = 0 AND " +
    MediaColumn::MEDIA_HIDDEN + " = 0 )";

const std::string INSERT_PHOTO_MAP =
    " INSERT INTO " + PhotoMap::TABLE +
    " (" + PhotoMap::ALBUM_ID + " , " + PhotoMap::ASSET_ID + " )" +
    " VALUES " +
    " ( ( SELECT " + PhotoAlbumColumns::ALBUM_ID + " FROM " + PhotoAlbumColumns::TABLE +
    " WHERE " + PhotoAlbumColumns::ALBUM_NAME + " = NEW." + MediaColumn::MEDIA_PACKAGE_NAME + " AND " +
    PhotoAlbumColumns::ALBUM_TYPE + " = "+ std::to_string(OHOS::Media::PhotoAlbumType::SYSTEM) + " AND " +
    PhotoAlbumColumns::ALBUM_SUBTYPE + " = "+ std::to_string(OHOS::Media::PhotoAlbumSubType::SOURCE) + ")," +
    " NEW." + MediaColumn::MEDIA_ID + " );";

const std::string SOURCE_ALBUM_WHERE =
    " WHERE " + PhotoAlbumColumns::ALBUM_NAME + " = NEW." + MediaColumn::MEDIA_PACKAGE_NAME +
    " AND " + PhotoAlbumColumns::ALBUM_TYPE + " = " + std::to_string(OHOS::Media::PhotoAlbumType::SYSTEM) +
    " AND " + PhotoAlbumColumns::ALBUM_SUBTYPE + " = " + std::to_string(OHOS::Media::PhotoAlbumSubType::SOURCE);

const std::string SOURCE_ALBUM_WHERE_UPDATE =
    " WHERE " + PhotoAlbumColumns::ALBUM_NAME + " = OLD." + MediaColumn::MEDIA_PACKAGE_NAME +
    " AND " + PhotoAlbumColumns::ALBUM_TYPE + " = " + std::to_string(OHOS::Media::PhotoAlbumType::SYSTEM) +
    " AND " + PhotoAlbumColumns::ALBUM_SUBTYPE + " = " + std::to_string(OHOS::Media::PhotoAlbumSubType::SOURCE);

const std::string WNEH_SOURCE_PHOTO_COUNT =
    " WHEN ( SELECT COUNT(1) FROM " + PhotoAlbumColumns::TABLE + SOURCE_ALBUM_WHERE + " )";

const std::string TRIGGER_CODE_UPDATE_AND_DELETE =
    " BEGIN UPDATE " + PhotoAlbumColumns::TABLE +
    " SET " + PhotoAlbumColumns::ALBUM_COUNT + " = " + COUNT_VALUE_UPDATE + SOURCE_ALBUM_WHERE_UPDATE + ";" +
    " UPDATE " + PhotoAlbumColumns::TABLE +
    " SET " + PhotoAlbumColumns::ALBUM_COVER_URI + " = '' " + SOURCE_ALBUM_WHERE_UPDATE +
    " AND " + PhotoAlbumColumns::ALBUM_COUNT + " = 0;" +
    " UPDATE " + PhotoAlbumColumns::TABLE +
    " SET " + PhotoAlbumColumns::ALBUM_COVER_URI + " = " + COVER_URI_VALUE_UPDATE + SOURCE_ALBUM_WHERE_UPDATE +
    " AND " + PhotoAlbumColumns::ALBUM_COUNT + " > 0;" + " END;";

const std::string DROP_INSERT_PHOTO_INSERT_SOURCE_ALBUM = "DROP TRIGGER IF EXISTS insert_photo_insert_source_album";

const std::string DROP_INSERT_PHOTO_UPDATE_SOURCE_ALBUM = "DROP TRIGGER IF EXISTS insert_photo_update_source_album";

const std::string DROP_UPDATE_PHOTO_UPDATE_SOURCE_ALBUM = "DROP TRIGGER IF EXISTS update_photo_update_source_album";

const std::string DROP_DELETE_PHOTO_UPDATE_SOURCE_ALBUM = "DROP TRIGGER IF EXISTS delete_photo_update_source_album";

const std::string INSERT_PHOTO_INSERT_SOURCE_ALBUM =
    "CREATE TRIGGER insert_photo_insert_source_album AFTER INSERT ON " + PhotoColumn::PHOTOS_TABLE +
    WNEH_SOURCE_PHOTO_COUNT + " = 0 " +
    " BEGIN INSERT INTO " + PhotoAlbumColumns::TABLE + "(" +
    PhotoAlbumColumns::ALBUM_TYPE + " , " +
    PhotoAlbumColumns::ALBUM_SUBTYPE + " , " +
    PhotoAlbumColumns::ALBUM_NAME + " , "
    + PhotoAlbumColumns::ALBUM_COVER_URI + " , " +
    PhotoAlbumColumns::ALBUM_COUNT +
    " ) VALUES ( " +
    std::to_string(OHOS::Media::PhotoAlbumType::SYSTEM) + " , " +
    std::to_string(OHOS::Media::PhotoAlbumSubType::SOURCE) +
    " , NEW." + MediaColumn::MEDIA_PACKAGE_NAME + " , " +
    COVER_URI_VALUE_INSERT + " , " +
    COUNT_VALUE_INSERT +
    ");" + INSERT_PHOTO_MAP + "END;";

const std::string INSERT_PHOTO_UPDATE_SOURCE_ALBUM =
    "CREATE TRIGGER insert_photo_update_source_album AFTER INSERT ON " + PhotoColumn::PHOTOS_TABLE +
    WNEH_SOURCE_PHOTO_COUNT + "> 0 " +
    " BEGIN UPDATE " + PhotoAlbumColumns::TABLE +
    " SET " + PhotoAlbumColumns::ALBUM_COVER_URI + " = " + COVER_URI_VALUE_INSERT + "," +
    PhotoAlbumColumns::ALBUM_COUNT + " = " + COUNT_VALUE_INSERT +
    SOURCE_ALBUM_WHERE + ";" +
    INSERT_PHOTO_MAP + " END;";

const std::string UPDATE_PHOTO_UPDATE_SOURCE_ALBUM =
    "CREATE TRIGGER update_photo_update_source_album AFTER UPDATE ON " +
    PhotoColumn::PHOTOS_TABLE + TRIGGER_CODE_UPDATE_AND_DELETE;

const std::string DELETE_PHOTO_UPDATE_SOURCE_ALBUM =
    "CREATE TRIGGER delete_photo_update_source_album AFTER DELETE ON " +
    PhotoColumn::PHOTOS_TABLE + TRIGGER_CODE_UPDATE_AND_DELETE;
} // namespace Media
} // namespace OHOS
#endif // INTERFACES_INNERAPI_MEDIA_LIBRARY_HELPER_INCLUDE_SOURCE_ALBUM_H