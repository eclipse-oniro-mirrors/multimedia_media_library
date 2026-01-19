/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_RESTORE_PHOTOS_ALBUM_HIDDEN_H
#define OHOS_MEDIA_RESTORE_PHOTOS_ALBUM_HIDDEN_H

#include <string>
#include <vector>

#include "rdb_store.h"

namespace OHOS::Media {
class RestorePhotosAlbumHidden {
public:
    int32_t UpdateEmptyAlbumHidden(std::shared_ptr<NativeRdb::RdbStore> &rdbStore);

private:
const std::string SQL_UPDATE_EMPTY_PHOTO_ALBUM_HIDDEN = "\
        UPDATE PhotoAlbum \
        SET hidden = 1 \
        WHERE count = 0 AND \
            hidden_count <> 0 AND \
            hidden <> 1;";
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_RESTORE_PHOTOS_ALBUM_HIDDEN_H