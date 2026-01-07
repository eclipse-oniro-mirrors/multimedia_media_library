/*
 * Copyright (C) 2024-2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_PHOTO_SOURCE_PATH_OPERATION_H
#define OHOS_MEDIA_PHOTO_SOURCE_PATH_OPERATION_H

#include <string>
#include <vector>
#include <sstream>

#include "medialibrary_rdbstore.h"

namespace OHOS::Media {
class PhotoSourcePathOperation {
private:
    class PhotoAssetInfo {
    public:
        int64_t albumId;
        std::string albumName;
        std::string lPath;
        int64_t fileId;
        std::string displayName;
        int32_t hidden;
        int64_t dateTrashed;
        std::string sourcePath;

    public:
        std::string ToString() const
        {
            std::stringstream ss;
            ss << "PhotoAssetInfo["
               << ", albumId: " << this->albumId << ", albumName: " << this->albumName << ", lPath: " << this->lPath
               << ", fileId: " << this->fileId << "displayName: " << this->displayName << ", hidden: " << this->hidden
               << ", dateTrashed: " << this->dateTrashed << ", sourcePath: " << this->sourcePath << "]";
            return ss.str();
        }
    };

public:
    void ResetPhotoSourcePath(std::shared_ptr<MediaLibraryRdbStore> mediaRdbStorePtr);

private:
    std::vector<PhotoAssetInfo> GetPhotoOfMissingSourcePath(
        std::shared_ptr<MediaLibraryRdbStore> mediaRdbStorePtr, const int32_t offset = 0, const int32_t limit = 200);

private:
    const std::string SQL_PHOTO_SOURCE_PATH_MISSING_QUERY = "\
        SELECT \
            album_id, \
            album_name, \
            lpath, \
            file_id, \
            display_name, \
            hidden, \
            date_trashed, \
            source_path \
        FROM Photos \
            LEFT JOIN PhotoAlbum \
            ON Photos.owner_album_id = PhotoAlbum.album_id \
        WHERE PhotoAlbum.album_id IS NOT NULL AND \
            COALESCE(PhotoAlbum.lpath, '') <> '' AND \
            COALESCE(source_path, '') = '' AND \
            (Photos.hidden <> 0 || date_trashed <> 0) \
        LIMIT ?, ? ;";
    const std::string SOURCE_PATH_PREFIX = "/storage/emulated/0";
    const std::string SQL_PHOTO_SOURCE_PATH_FIX_UPDATE = "\
        UPDATE Photos \
        SET source_path = ? \
        WHERE COALESCE(source_path, '') = '' AND \
            (Photos.hidden <> 0 || date_trashed <> 0) AND \
            file_id = ? ;";
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_PHOTO_SOURCE_PATH_OPERATION_H