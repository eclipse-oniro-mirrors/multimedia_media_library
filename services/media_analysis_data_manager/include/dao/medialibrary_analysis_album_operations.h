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

#ifndef OHOS_MEDIALIBRARY_ANALYSIS_ALBUM_OPERATIONS_H
#define OHOS_MEDIALIBRARY_ANALYSIS_ALBUM_OPERATIONS_H

#include <memory>
#include <mutex>
#include <securec.h>
#include <string>

#include "abs_shared_result_set.h"
#include "datashare_predicates.h"
#include "datashare_values_bucket.h"
#include "medialibrary_album_operations.h"
#include "medialibrary_command.h"
#include "medialibrary_async_worker.h"
#include "native_album_asset.h"
#include "rdb_predicates.h"
#include "rdb_result_set_bridge.h"
#include "vision_column.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
struct GroupPhotoAlbumInfo {
    int32_t albumId;
    std::string tagId;
    std::string coverUri;
    int32_t isCoverSatisfied;
    int32_t count;
    int32_t fileId;
    std::string candidateUri;
    int32_t isMe;
    std::string albumName;
    int32_t isRemoved;
    int32_t renameOperation;
};
class MediaLibraryAnalysisAlbumOperations {
public:
    EXPORT static int32_t UpdateMergeGroupAlbumsInfo(const vector<MergeAlbumInfo> &mergeAlbumInfo);
    EXPORT static int32_t HandleGroupPhotoAlbum(const OperationType &opType, const NativeRdb::ValuesBucket &values,
        const DataShare::DataSharePredicates &predicates);
    EXPORT static std::shared_ptr<NativeRdb::ResultSet> QueryGroupPhotoAlbum(MediaLibraryCommand &cmd,
        const std::vector<std::string> &columns);
    EXPORT static void UpdateGroupPhotoAlbumById(int32_t albumId);
    EXPORT static void UpdatePortraitAlbumCoverSatisfied(int32_t fileId);
    EXPORT static int32_t SetAnalysisAlbumOrderPosition(MediaLibraryCommand &cmd);
    EXPORT static int32_t SetAnalysisAlbumPortraitsOrder(MediaLibraryCommand &cmd);
    EXPORT static int32_t SetGroupAlbumName(const NativeRdb::ValuesBucket &values,
        const DataShare::DataSharePredicates &predicates);
    EXPORT static int32_t SetGroupCoverUri(const NativeRdb::ValuesBucket &values,
        const DataShare::DataSharePredicates &predicates);
    EXPORT static int32_t DismissGroupPhotoAlbum(const NativeRdb::ValuesBucket &values,
        const DataShare::DataSharePredicates &predicates);
    EXPORT static int32_t SetHighlightAttribute(const int32_t &albumId,
        const int32_t &highlightAlbumChangeAttribute, const std::string &value);
};
} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIALIBRARY_ANALYSIS_ALBUM_OPERATIONS_H
