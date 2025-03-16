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
class PhotoStorageOperation {
public:
    std::shared_ptr<NativeRdb::ResultSet> FindStorage(std::shared_ptr<MediaLibraryRdbStore> mediaRdbStorePtr);

private:
    int64_t GetCacheSize();

private:
    // media_type : 1-photo, 2-video, -1-thumbnail & cache
    const std::string SQL_DB_STORAGE_QUERY = "\
        SELECT \
            media_type, \
            SUM(size) AS size \
        FROM Photos \
        WHERE \
            media_type IN (1, 2) AND \
            position != 2 \
        GROUP BY media_type \
        UNION \
        SELECT \
            -1 AS media_type, \
            SUM(thumbnail_size) + ? AS size \
        FROM tab_photos_ext \
        ;";
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_PHOTO_STORAGE_OPERATION_H