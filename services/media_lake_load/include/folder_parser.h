/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_FOLDER_PARSER_H
#define OHOS_FOLDER_PARSER_H

#include <string>
#include "medialibrary_unistore_manager.h"
#include "lake_const.h"

namespace OHOS {
namespace Media {

using namespace std;

struct AlbumPluginInfo {
    bool isValid = false;
    std::string lpath;
    std::string albumName;
    std::string albumNameEn;
    std::string bundleName;
};

struct LakeAlbumInfo {
    bool isValid = false;
    int32_t albumId {-1};
    std::string lpath;
    std::string albumName;
    std::string bundleName;
    std::string displayName;
    int64_t dateModified {0};
    int64_t dateAdded {0};
};

enum class FolderOperationType {
    SKIP = 0,
    UPDATE = 1,
    INSERT = 2,
    CONTINUE
};

class FolderParser {
public:
    FolderParser(const std::string &storagePath, LakeScanMode scanType = LakeScanMode::INCREMENT);
    ~FolderParser() = default;
    
    FolderOperationType PreProcessFolder();
    int32_t InsertAlbumInfo();
    int32_t UpdatePhotoAlbum();
    LakeAlbumInfo GetAlbumInfo();
    int32_t StartFolderParser();

private:
    bool isLpathValid = false;
    std::shared_ptr<MediaLibraryRdbStore> mediaLibraryRdb_;
    std::string storagePath_;
    LakeAlbumInfo lakeAlbumInfo_;
    AlbumPluginInfo albumPluginInfo_;
    bool hasInsert = false;
    bool hasConverted = false;
    const LakeScanMode scanMode_;

private:
    int32_t GetAlbumPhotoInfo(LakeAlbumInfo &photoAlbumInfo);
    int32_t GetAlbumPluginInfo(AlbumPluginInfo &albumPluginInfo);
    int32_t PreProcessUpdate();
    int32_t GetConvertedLpath(const std::string &data, std::string &lpath);
    int32_t GetDisplayName(const std::string &path, std::string &displayName);
    int32_t GetAlbumName(LakeAlbumInfo &lakeAlbumInfo);
    bool CheckAlbumNameUnique(const std::string &albumName);
    std::shared_ptr<MediaLibraryRdbStore> GetMediaLibraryRdb();
    std::string ToString(const std::vector<NativeRdb::ValueObject> &bindArgs);

    bool IsFolderSkip();

    int32_t InsertAlbum(LakeAlbumInfo &lakeAlbumInfo);
    int32_t UpdateAlbum(const LakeAlbumInfo &lakeAlbumInfo, const NativeRdb::AbsRdbPredicates &predicates);
    int32_t DeleteAlbum(const NativeRdb::AbsRdbPredicates &predicates);
    long long GetTimestampMs();

private:
    const int32_t MAX_ALBUM_NAME_SEQUENCE = 1000;
    const std::string SQL_PHOTO_ALBUM_SELECT_BY_LPATH = "\
        SELECT album_id, \
        album_name, \
        bundle_name, \
        lpath, \
        date_modified, \
        date_added  \
        FROM PhotoAlbum \
        WHERE LOWER(lpath) = LOWER(?) AND dirty != 4 \
        ORDER BY album_id DESC \
        LIMIT 1;";

    const std::string SQL_QUERY_ALBUM_NAME_FROM_ALBUM_PLUGIN_ONLY = "\
        SELECT album_name, \
        lpath, \
        album_name_en, \
        bundle_name \
        FROM album_plugin \
        WHERE LOWER(lpath) = LOWER(?) \
        LIMIT 1;";

    // The albumName of PhotoAlbum, which is not in album_plugin, should be unique.
    const std::string SQL_PHOTO_ALBUM_CHECK_ALBUM_NAME_UNIQUE = "\
        SELECT COUNT(1) AS count \
        FROM PhotoAlbum \
        WHERE LOWER(PhotoAlbum.album_name) = LOWER(?) AND \
            PhotoAlbum.dirty != 4;";
};
} // namespace Media
} // namespace OHOS

#endif  // OHOS_FOLDER_PARSER_H