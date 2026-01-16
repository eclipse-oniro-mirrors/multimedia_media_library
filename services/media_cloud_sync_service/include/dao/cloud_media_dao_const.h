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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_DAO_CONST_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_DAO_CONST_H

#include <string>
#include <vector>
#include <unordered_map>

#include "media_column.h"
#include "userfile_manager_types.h"

namespace OHOS::Media::CloudSync {
using namespace OHOS::Media::ORM;
const std::string ALBUM_LOCAL_PATH_PREFIX = "/OH/";
const std::string WRITE_LIST_TABLE_NAME = "album_plugin";
const std::string HIDDEN_ALBUM = ".hiddenAlbum";
const std::string HIDDEN_ALBUM_CLOUD_ID = "default-album-4";
const std::string SCREENSHOT_ALBUM_CLOUD_ID = "default-album-2";
const std::string PYHSICAL_ALBUM_CLOUD_ID_PREFIX = "default-album-";
const std::string ANALYSIS_PHOTO_MAP_TABLE = "AnalysisPhotoMap";
const std::vector<std::string> ALL_SYSTEM_PHOTO_ALBUM = {
    std::to_string(PhotoAlbumSubType::FAVORITE),
    std::to_string(PhotoAlbumSubType::VIDEO),
    std::to_string(PhotoAlbumSubType::HIDDEN),
    std::to_string(PhotoAlbumSubType::TRASH),
    std::to_string(PhotoAlbumSubType::SCREENSHOT),
    std::to_string(PhotoAlbumSubType::CAMERA),
    std::to_string(PhotoAlbumSubType::IMAGE),
    std::to_string(PhotoAlbumSubType::CLOUD_ENHANCEMENT),
};
const std::vector<std::string> PULL_QUERY_COLUMNS = {
    PhotoColumn::MEDIA_FILE_PATH,
    PhotoColumn::MEDIA_SIZE,
    PhotoColumn::MEDIA_DATE_MODIFIED,
    PhotoColumn::PHOTO_DIRTY,
    PhotoColumn::MEDIA_DATE_TRASHED,
    PhotoColumn::PHOTO_POSITION,
    PhotoColumn::PHOTO_CLOUD_ID,
    PhotoColumn::PHOTO_CLOUD_VERSION,
    MediaColumn::MEDIA_ID,
    PhotoColumn::MEDIA_RELATIVE_PATH,
    PhotoColumn::MEDIA_DATE_ADDED,
    PhotoColumn::MEDIA_DATE_TAKEN,
    PhotoColumn::PHOTO_OWNER_ALBUM_ID,
    PhotoColumn::PHOTO_META_DATE_MODIFIED,
    PhotoColumn::PHOTO_SYNC_STATUS,
    PhotoColumn::PHOTO_THUMB_STATUS,
    PhotoColumn::MEDIA_NAME,
    PhotoColumn::PHOTO_ORIENTATION,
    PhotoColumn::PHOTO_SUBTYPE,
    PhotoColumn::MOVING_PHOTO_EFFECT_MODE,
    PhotoColumn::PHOTO_ORIGINAL_SUBTYPE,
    PhotoColumn::PHOTO_SOURCE_PATH,
    MediaColumn::MEDIA_MIME_TYPE,
    MediaColumn::MEDIA_TYPE,
    PhotoColumn::PHOTO_ORIGINAL_ASSET_CLOUD_ID,
    PhotoColumn::PHOTO_EXIF_ROTATE,
    PhotoColumn::PHOTO_FILE_SOURCE_TYPE,
    PhotoColumn::PHOTO_STORAGE_PATH,
};

struct KeyData {
    std::string displayName;
    std::string filePath;
    int64_t isize;
    int64_t createTime;
    int64_t modifyTime;
    int32_t exifRotateValue;
    int32_t mediaType;
    std::string sourceAlbum;
    std::string lPath;
    int64_t dateTrashed;
};

//联调临时代码
enum class PrintfFieldType {
    PFIELD_TYPE_NULL = 0,
    PFIELD_TYPE_INT32,      // int32_t
    PFIELD_TYPE_INT64,      // int64_t
    PFIELD_TYPE_DOUBLE,     // double
    PFIELD_TYPE_STRING,     // std::string
    PFIELD_TYPE_BOOL,       // bool
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_DAO_CONST_H