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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_PHOTO_ALBUM_COLUMN_H
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_PHOTO_ALBUM_COLUMN_H

#include <set>
#include <string>
#include <unordered_map>

#include "album_order.h"
#include "base_column.h"
#include "rdb_predicates.h"
#include "userfile_manager_types.h"

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))
class PhotoAlbumColumns : BaseColumn {
public:
    // columns only in PhotoAlbumTable
    static const std::string ALBUM_ID;
    static const std::string ALBUM_TYPE;
    static const std::string ALBUM_SUBTYPE;
    static const std::string ALBUM_NAME;
    static const std::string ALBUM_COVER_URI;
    static const std::string ALBUM_COUNT;
    static const std::string ALBUM_DATE_MODIFIED;
    static const std::string ALBUM_DIRTY;
    static const std::string ALBUM_CLOUD_ID;
    static const std::string ALBUM_IMAGE_COUNT;
    static const std::string ALBUM_VIDEO_COUNT;
    static const std::string ALBUM_LATITUDE;
    static const std::string ALBUM_LONGITUDE;
    static const std::string ALBUM_BUNDLE_NAME;
    static const std::string ALBUM_LOCAL_LANGUAGE;
    static const std::string ALBUM_IS_LOCAL;
    static const std::string ALBUM_DATE_ADDED;
    static const std::string ALBUM_PRIORITY;
    static const std::string ALBUM_LPATH;
    static const std::string ALBUM_CHECK_FLAG;
    static const std::string COVER_URI_SOURCE;
    static const std::string COVER_CLOUD_ID;
    static const std::string UPLOAD_STATUS;
    static const std::string ALBUM_HIDDEN;
    static const std::string UNIQUE_ID;
    static const std::string ALBUM_FILE_HIDDEN;
    static const std::string ALBUM_SCENE_ID;
    static const std::string ALBUM_SHARE_TYPE;
    static const std::string COVER_ORDER_KEY;
    static const std::string COVER_ORDER_SUBKEY;
    static const std::string COVER_ORDER_TYPE;
    static const std::string HIDDEN_COVER_ORDER_KEY;
    static const std::string HIDDEN_COVER_ORDER_SUBKEY;
    static const std::string HIDDEN_COVER_ORDER_TYPE;

    // share album columns
    static const std::string SHARE_RISK_STATUS;
    static const std::string SHARE_RISK_TYPE;
    static const std::string SHARE_ALBUM_OWNER;

    // For api9 compatibility
    static const std::string ALBUM_RELATIVE_PATH;

    static const std::string CONTAINS_HIDDEN;
    static const std::string HIDDEN_COUNT;
    static const std::string HIDDEN_COVER;

    // For sorting albums
    static const std::string ALBUM_ORDER;
    static const std::string REFERENCE_ALBUM_ID;

    // For accurate refresh
    static const std::string COVER_DATE_TIME;
    static const std::string HIDDEN_COVER_DATE_TIME;

    // default fetch columns
    static const std::set<std::string> DEFAULT_FETCH_COLUMNS;
    static const std::vector<std::string> LOCATION_DEFAULT_FETCH_COLUMNS;
    static const std::vector<std::string> CITY_DEFAULT_FETCH_COLUMNS;

    // For api19 sorting
    static const std::string ALBUMS_ORDER;
    static const std::string ORDER_SECTION;
    static const std::string ORDER_TYPE;
    static const std::string ORDER_STATUS;
    static const std::string STYLE2_ALBUMS_ORDER;
    static const std::string STYLE2_ORDER_SECTION;
    static const std::string STYLE2_ORDER_TYPE;
    static const std::string STYLE2_ORDER_STATUS;
    // for incremental query
    static const std::string CHANGE_TIME;

    static const std::vector<std::string> ALBUM_ORDER_COLUMNS;
    static const std::vector<std::string> ALBUM_ORDER_SECTION_COLUMNS;
    static const std::vector<std::string> ALBUM_ORDER_TYPE_COLUMNS;
    static const std::vector<std::string> ALBUM_ORDER_STATUS_COLUMNS;

    static const std::unordered_map<AlbumOrderParam, std::vector<std::string>> ORDER_COLUMN_STYLE_MAP;
    static const std::set<std::string> DEFAULT_FETCH_ORDER_COLUMNS_STYLE1;
    static const std::set<std::string> DEFAULT_FETCH_ORDER_COLUMNS_STYLE2;

    // table name
    static const std::string TABLE;
    // create PhotoAlbumTable sql
    static const std::string CREATE_TABLE;

    // create indexes for PhotoAlbum
    static const std::string INDEX_ALBUM_TYPES;

    // create triggers
    static const std::string CREATE_ALBUM_INSERT_TRIGGER;
    static const std::string CREATE_ALBUM_MDIRTY_TRIGGER;
    static const std::string CREATE_ALBUM_DELETE_TRIGGER;
    static const std::string ALBUM_DELETE_ORDER_TRIGGER;
    static const std::string ALBUM_INSERT_ORDER_TRIGGER;

    // util constants
    static const std::string ALBUM_URI_PREFIX;
    static const std::string DEFAULT_PHOTO_ALBUM_URI;
    static const std::string HIDDEN_ALBUM_URI_PREFIX;
    static const std::string DEFAULT_HIDDEN_ALBUM_URI;
    static const std::string ANALYSIS_ALBUM_URI_PREFIX;
    static const std::string TRASHED_ALBUM_URI_PREFIX;

    // cloud sync uri
    static const std::string ALBUM_CLOUD_URI_PREFIX;
    static const std::string ALBUM_GALLERY_CLOUD_URI_PREFIX;
    static const std::string PHOTO_GALLERY_CLOUD_SYNC_INFO_URI_PREFIX;
    static const std::string PHOTO_GALLERY_DOWNLOAD_URI_PREFIX;

    // specified album lpath
    static const std::string LPATH_CAMERA;
    static const std::string LPATH_SCREENSHOT;
    static const std::string LPATH_SCREENRECORD;

    static bool IsPhotoAlbumColumn(const std::string &columnName);

    static void GetUserAlbumPredicates(const int32_t albumId, NativeRdb::RdbPredicates &predicates,
        const bool hiddenState);
    static bool GetSystemAlbumPredicates(const PhotoAlbumSubType subType, NativeRdb::RdbPredicates &predicates,
        const bool hiddenState);
    static void GetAnalysisPhotoMapPredicates(const int32_t albumId, NativeRdb::RdbPredicates &predicates,
        const bool hiddenState);
    static void GetPortraitAlbumPredicates(const int32_t albumId, NativeRdb::RdbPredicates &predicates);
    static void GetSourceAlbumPredicates(const int32_t albumId, NativeRdb::RdbPredicates &predicates,
        const bool hiddenState);
    // 共享相册全量刷新 predicates
    static void GetShareAlbumPredicates(const int32_t albumId, NativeRdb::RdbPredicates &predicates,
        const bool hiddenState);
    static std::string CheckUploadPhotoAlbumColumns();
};
} // namespace OHOS::Media
#endif // INTERFACES_INNERKITS_NATIVE_INCLUDE_PHOTO_ALBUM_COLUMN_H
