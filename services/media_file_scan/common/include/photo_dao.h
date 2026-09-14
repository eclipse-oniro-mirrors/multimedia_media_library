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
#ifndef PHOTO_DAO_H
#define PHOTO_DAO_H

#include <memory>
#include <string>
#include <vector>

#include "file_const.h"
#include "medialibrary_db_const.h"
#include "rdb_store.h"

namespace OHOS::Media {
class MediaLibraryRdbStore;

class PhotoDao {
public:
    PhotoDao();
    ~PhotoDao();

    struct PhotosRowData {
        int32_t fileId {0};
        int32_t mediaType {0};
        int32_t fileSourceType {0};
        int32_t ownerAlbumId {0};
        int32_t syncStatus {0};
        int64_t size {0};
        int64_t dateModified {0};
        int64_t dateTaken {0};
        int64_t editTime {0};
        std::string data;
        std::string inode;
        std::string mimeType;
        std::string storagePath;
        std::string ownerPackage;
        std::string packageName;
        std::string detailTime;
        std::string dateYear;
        std::string dateMonth;
        std::string dateDay;
        std::string cloudId;
        std::string displayName;
        bool IsExist();
        std::string ToString() const;
        int32_t subtype {0};
        int32_t position {1};
    };

    int32_t QueryThumbnailInfos(const std::vector<std::string> &inodes,
        std::vector<ThumbnailInfo> &infos, std::vector<int32_t> &thumbnailVisibleList);
    int32_t IsExistSameFileForCloneRestore(const std::vector<NativeRdb::ValueObject> &params);
    int32_t QuerySubtypeAndEffectMode(int32_t fileId, int32_t &subtype, int32_t &effectMode, bool &found);
    int32_t QuerySubtype(int32_t fileId, int32_t &subtype, bool &found);
    std::vector<std::string> QueryPhotoPathsByStoragePaths(const std::vector<std::string> &files);

    PhotosRowData FindSameFileByStoragePath(const std::string &storagePath, FileSourceType sourceType);

private:
    PhotosRowData FindSameFileInDatabase(const std::string &querySql,
        const std::vector<NativeRdb::ValueObject> &params);
    std::shared_ptr<MediaLibraryRdbStore> rdbStore_;
};
} // namespace OHOS::Media
#endif // PHOTO_DAO_H
