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

#ifndef OHOS_PHOTO_MAP_OPERATIONS_H
#define OHOS_PHOTO_MAP_OPERATIONS_H

#include "datashare_values_bucket.h"
#include "rdb_predicates.h"
#include "file_asset.h"
#include "userfile_manager_types.h"
#include "medialibrary_command.h"
#include "result_set.h"

struct MergedAlbumInfo {
    std::vector<std::string> sourceAlbumIds;
    std::vector<std::string> targetAlbumIds;
    std::vector<std::string> commonAssets;
    std::vector<std::string> unCommonAssets;
};

namespace OHOS::Media {
class PhotoMapOperations {
public:
    static int32_t AddPhotoAssets(const std::vector<DataShare::DataShareValuesBucket> &values);
    static int32_t RemovePhotoAssets(NativeRdb::RdbPredicates &predicates);
    static std::shared_ptr<NativeRdb::ResultSet> QueryPhotoAssets(const NativeRdb::RdbPredicates &rdbPredicate,
        const std::vector<std::string> &columns);
    static int32_t AddAnaLysisPhotoAssets(const std::vector<DataShare::DataShareValuesBucket> &values);
    static int32_t DismissAssets(NativeRdb::RdbPredicates &predicates);
    static int32_t SmartMoveAssets(const std::string &strAlbumId, const std::string &targetAlbumId,
        const std::vector<std::string> &assetsArray);
    static int32_t AddHighlightPhotoAssets(const std::vector<DataShare::DataShareValuesBucket> &values);
};
} // namespace OHOS::Media
#endif // OHOS_PHOTO_MAP_OPERATIONS_H
