/*
 * Copyright (C) 2021-2023 Huawei Device Co., Ltd.
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
#include <memory>
#include <mutex>
#include <securec.h>
#include <unistd.h>
#include <unordered_map>

#include "dir_asset.h"
#include "file_asset.h"
#include "medialibrary_db_const.h"
#include "medialibrary_command.h"
#include "native_album_asset.h"
#include "rdb_store.h"
#include "values_bucket.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
constexpr int32_t DEFAULT_ALBUMID = -1;
constexpr int32_t DEFAULT_ASSETID = -1;
class MediaLibrarySmartAlbumMapOperations {
public:
    EXPORT static int32_t HandleSmartAlbumMapOperation(MediaLibraryCommand &cmd);
    EXPORT static int32_t HandleAddAssetOperation(const int32_t albumId, const int32_t childFileAssetId,
        const int32_t childAlbumId, MediaLibraryCommand &cmd);
    EXPORT static int32_t HandleRemoveAssetOperation(const int32_t albumId, const int32_t childFileAssetId,
        MediaLibraryCommand &cmd);
    EXPORT static int32_t HandleAgingOperation(std::shared_ptr<int> countPtr = nullptr);
    static void SetInterrupt(bool interrupt);

private:
    static bool GetInterrupt();

    static std::atomic<bool> isInterrupt_;
    static std::mutex g_opMutex;
};
} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIALIBRARY_SMARTALBUM_MAP_OPERATIONS_H
