/*
 * Copyright (C) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIALIBRARY_ALBUM_OPERATIONS_H
#define OHOS_MEDIALIBRARY_ALBUM_OPERATIONS_H

#include <memory>
#include <securec.h>
#include <string>

#include "abs_shared_result_set.h"
#include "datashare_predicates.h"
#include "datashare_values_bucket.h"
#include "medialibrary_command.h"
#include "native_album_asset.h"
#include "rdb_predicates.h"
#include "rdb_result_set_bridge.h"
#include "vision_column.h"
#include "medialibrary_async_worker.h"
#include "medialibrary_rdb_transaction.h"
#include "album_accurate_refresh.h"

namespace OHOS {
namespace Media {
constexpr int32_t NULL_REFERENCE_ALBUM_ID = -1;
struct MergeAlbumInfo {
    int albumId;
    int albumType;
    int albumSubtype;
    std::string groupTag;
    int count;
    int isMe;
    std::string coverUri;
    int userDisplayLevel;
    int userOperation;
    int rank;
    int renameOperation;
    std::string albumName;
    uint8_t isCoverSatisfied;
    std::string tagId;
    std::vector<string> repeatedAlbumIds;
    std::string relationship;
};

struct SetCoverUriAlbumInfo {
    int32_t albumId;
    int32_t dirty;
};

class MediaLibraryAlbumOperations {
public:
    static int32_t CreateAlbumOperation(MediaLibraryCommand &cmd);
    static int32_t ModifyAlbumOperation(MediaLibraryCommand &cmd);
    static shared_ptr<NativeRdb::ResultSet> QueryAlbumOperation(MediaLibraryCommand &cmd,
        const std::vector<std::string> &columns);

    static int32_t HandlePhotoAlbumOperations(MediaLibraryCommand &cmd);
    static std::shared_ptr<NativeRdb::ResultSet> QueryPhotoAlbum(MediaLibraryCommand &cmd,
        const std::vector<std::string> &columns);
    static int32_t DeletePhotoAlbum(NativeRdb::RdbPredicates &predicates);
    static int32_t DeletePhotoAssetsCompleted(const DataShare::DataSharePredicates &predicates, const bool isAging);
    static int32_t DeleteHighlightAlbums(NativeRdb::RdbPredicates &predicates);
    static int32_t AddPhotoAssets(const vector<DataShare::DataShareValuesBucket> &values);
    static int32_t HandlePhotoAlbum(const OperationType &opType, const NativeRdb::ValuesBucket &values,
        const DataShare::DataSharePredicates &predicates, std::shared_ptr<int> countPtr = nullptr);
    static int32_t HandleAnalysisPhotoAlbum(const OperationType &opType, const NativeRdb::ValuesBucket &values,
        const DataShare::DataSharePredicates &predicates, std::shared_ptr<int> countPtr = nullptr);
    static std::shared_ptr<NativeRdb::ResultSet> QueryPortraitAlbum(MediaLibraryCommand &cmd,
        const std::vector<std::string> &columns);
    static void DealwithNoAlbumAssets(const std::vector<std::string> &whereArgs);
    static void RecoverAlbum(const string &assetId, const string &lPath, bool &isUserAlbum, int64_t &newAlbumId);
    static int32_t GetLPathFromSourcePath(const string &sourcePath, string &lPath, int32_t mediaType);
    static int32_t RenewDeletedPhotoAlbum(int32_t id, const NativeRdb::ValuesBucket &albumValues,
        std::shared_ptr<TransactionOperations> trans, AccurateRefresh::AlbumAccurateRefresh *albumRefresh = nullptr);
    static int32_t SetAlbumName(const NativeRdb::ValuesBucket &values,
        const DataShare::DataSharePredicates &predicates);
    static int32_t SetHighlightAlbumName(const NativeRdb::ValuesBucket &values,
        const DataShare::DataSharePredicates &predicates);
    static int32_t HandleSetAlbumNameRequest(const NativeRdb::ValuesBucket &values,
        const DataShare::DataSharePredicates &predicates);
    static int32_t SetCoverUri(const NativeRdb::ValuesBucket &values,
        const DataShare::DataSharePredicates &predicates);
    static int32_t SetHighlightCoverUri(const NativeRdb::ValuesBucket &values,
        const DataShare::DataSharePredicates &predicates);
    static int32_t UpdatePhotoAlbum(const NativeRdb::ValuesBucket &values,
        const DataShare::DataSharePredicates &predicates);
    static int32_t SetDisplayLevel(const NativeRdb::ValuesBucket &values,
        const DataShare::DataSharePredicates &predicates);
    static int32_t SetHighlightSubtitle(const NativeRdb::ValuesBucket &values,
        const DataShare::DataSharePredicates &predicates);
    static int32_t SetPortraitAlbumRelationship(const NativeRdb::ValuesBucket &values,
        const DataShare::DataSharePredicates &predicates, const int32_t isMeAlbum);
    static int32_t SetIsMe(const NativeRdb::ValuesBucket &values,
        const DataShare::DataSharePredicates &predicates);
    static int32_t RecoverPhotoAssets(const DataShare::DataSharePredicates &predicates);
    static int32_t DeletePhotoAssets(const DataShare::DataSharePredicates &predicates,
        const bool isAging, const bool compatible);
    static bool IsOnlyPortraitOrPetAlbumMerge(const vector<MergeAlbumInfo>& mergeAlbumInfo);
    static int32_t MergeAlbums(const NativeRdb::ValuesBucket &values);
    static int32_t OrderSingleAlbum(const NativeRdb::ValuesBucket &values);
    static int32_t UpdateAlbumCoverUri(const NativeRdb::ValuesBucket &values,
        const DataShare::DataSharePredicates &predicates, bool isSystemAlbum);
    static int32_t ResetCoverUri(const NativeRdb::ValuesBucket &values,
        const DataShare::DataSharePredicates &predicates);
    static bool IsCoverInAlbum(const string &fileId, int32_t albumSubtype, int32_t albumId);
    static bool IsCoverInSystemAlbum(NativeRdb::RdbPredicates &predicates, int32_t albumSubtype);
    static bool IsManunalCloudCover(const std::string &fileId, std::string &coverCloudId);
    static int32_t UpdateCoverUriExecute(const SetCoverUriAlbumInfo& albumInfo, const std::string &coverUri,
        const std::string &fileId, int64_t coverDateTime, AccurateRefresh::AlbumAccurateRefresh& albumRefresh);
    static int32_t UpdatePhotoAlbumOrder(const vector<NativeRdb::ValuesBucket> &valuesBuckets,
        const vector<NativeRdb::RdbPredicates> &predicatesArray);
    static int32_t CreatePortraitAlbum(const string &albumName);

    // Get column informations of PhotoAlbum table except primary key column
    static const std::unordered_map<std::string, ColumnSchema>& GetPhotoAlbumTableSchema();
};
} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIALIBRARY_ALBUM_OPERATIONS_H
