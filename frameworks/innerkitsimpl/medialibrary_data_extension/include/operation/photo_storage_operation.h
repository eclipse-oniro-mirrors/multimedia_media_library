/*
 * Copyright (C) 2025-2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_PHOTO_STORAGE_OPERATION_H
#define OHOS_MEDIA_PHOTO_STORAGE_OPERATION_H

#include <string>

#include "medialibrary_rdbstore.h"

namespace OHOS::Media {

struct TotalThumbnailSizeResult {
    int64_t totalThumbnailSize;
    int32_t thumbnailCount;
};

struct TotalEditdataSizeResult {
    int64_t totalEditdataSize;
    int32_t editdataCount;
};

struct LocalPhotoSizeResult {
    int64_t localImageSize;
    int64_t localVideoSize;
};

class PhotoStorageOperation {
public:
    std::shared_ptr<NativeRdb::ResultSet> FindStorage(std::shared_ptr<MediaLibraryRdbStore> mediaRdbStorePtr);
    std::shared_ptr<NativeRdb::ResultSet> QueryHighlightDirectorySize(std::shared_ptr<MediaLibraryRdbStore> rdbStore);
    int64_t GetCacheSize();
    int64_t GetBackUpSize();
    int64_t GetAudioSize();
    int64_t GetCameraSize();
    int64_t GetPictureSize();
    int64_t GetMediaVideoSize();
    int64_t GetCustomSize();
    int64_t GetMetaSize();
    int64_t GetDataSize();
    int64_t GetHighlightSizeFromPreferences();
    void GetTotalThumbnailSize(std::shared_ptr<MediaLibraryRdbStore> rdbStore,
        TotalThumbnailSizeResult &totalThumbnailSizeResult);

    void GetTotalEditdataSize(std::shared_ptr<MediaLibraryRdbStore> rdbStore,
        TotalEditdataSizeResult &totalEditdataSizeResult);
    void GetLocalPhotoSize(std::shared_ptr<MediaLibraryRdbStore> rdbStore, LocalPhotoSizeResult &localPhotoSizeResult);

private:
    int64_t GetHighlightSize();
    void SaveHighlightSizeToPreferences(int64_t size);

private:
    // media_type : 1-photo, 2-video, -1-thumbnail & cache
    const std::string SQL_DB_STORAGE_QUERY = "\
        SELECT \
            media_type, \
            SUM(size) AS size \
        FROM Photos \
        WHERE \
            media_type IN (1, 2) AND \
            position != 2 AND \
            file_source_type != 3 \
        GROUP BY media_type \
        ;";
 
    const std::string SQL_DB_STORAGE_INFO_QUERY = "\
        SELECT \
            -1 AS media_type, \
            ? AS size \
        UNION \
        SELECT \
            1 AS media_type, \
            ? AS size \
        UNION \
        SELECT \
            2 AS media_type, \
            ? AS size \
        ;";
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_PHOTO_STORAGE_OPERATION_H