/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIALIBRARY_SMARTALBUM_MAP_OPERATIONS_H
#define OHOS_MEDIALIBRARY_SMARTALBUM_MAP_OPERATIONS_H

#include <string>
#include <variant>
#include <grp.h>
#include <mutex>
#include <securec.h>
#include <unistd.h>
#include <unordered_map>

#include "dir_asset.h"
#include "file_asset.h"
#include "medialibrary_db_const.h"
#include "medialibrary_data_manager_utils.h"
#include "medialibrary_command.h"
#include "native_album_asset.h"
#include "rdb_store.h"
#include "values_bucket.h"

namespace OHOS {
namespace Media {
enum TrashType {
    NOT_TRASHED = 0,
    TRASHED_ASSET,
    TRASHED_DIR,
    TRASHED_DIR_CHILD
};
constexpr int32_t DEFAULT_ALBUMID = -1;
constexpr int32_t DEFAULT_ASSETID = -1;
class MediaLibrarySmartAlbumMapOperations {
public:
    static int32_t HandleSmartAlbumMapOperation(MediaLibraryCommand &cmd);
    static int32_t HandleAddAssetOperation(const int32_t albumId, const int32_t childFileAssetId,
        const int32_t childAlbumId, MediaLibraryCommand &cmd);
    static int32_t HandleRemoveAssetOperation(const int32_t albumId, const int32_t childFileAssetId,
        MediaLibraryCommand &cmd);
    static int32_t HandleAgingOperation();
    static void SetInterrupt(bool interrupt);
    static bool GetInterrupt();

private:
    static std::atomic<bool> isInterrupt_;

    static std::mutex g_opMutex;
};
} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIALIBRARY_SMARTALBUM_MAP_OPERATIONS_H
