/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIALIBRARY_ALBUM_FUSION_UTILS_H
#define OHOS_MEDIALIBRARY_ALBUM_FUSION_UTILS_H

#include <vector>
#include <mutex>

#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_album_compatibility_fusion_sql.h"
#include "medialibrary_errno.h"
#include "photo_map_column.h"
#include "source_album.h"
#include "medialibrary_rdb_transaction.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore.h"
#include "medialibrary_unistore_manager.h"

namespace OHOS {
namespace Media {

enum class AlbumFusionState {
    START = 1,
    SUCCESS = 2,
    FAILED = 3
};
class MediaLibraryAlbumFusionUtils {
public:
    EXPORT static int32_t RemoveMisAddedHiddenData(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore);
    EXPORT static int32_t HandleMatchedDataFusion(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore);
    EXPORT static int32_t HandleNotMatchedDataFusion(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore);
    EXPORT static int32_t HandleNotMatchedDataMigration(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore,
        std::multimap<int32_t, std::vector<int32_t>> &notMatchedMap);
    EXPORT static int32_t HandleSingleFileCopy(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore,
        const int32_t &assetId, const int32_t &ownerAlbumId, int64_t &newAssetId);
    EXPORT static int32_t RebuildAlbumAndFillCloudValue(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore);
    EXPORT static int32_t HandleChangeNameAlbum(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore);
    EXPORT static int32_t CompensateLpathForLocalAlbum(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore);
    EXPORT static int32_t MergeClashSourceAlbum(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore,
        std::shared_ptr<NativeRdb::ResultSet> &resultSet, const int32_t &sourceAlbumId, const int64_t &targetAlbumId);
    EXPORT static void SetParameterToStopSync();
    EXPORT static void SetParameterToStartSync();
    EXPORT static int32_t CleanInvalidCloudAlbumAndData();
    EXPORT static int32_t CloneSingleAsset(const int64_t &assetId, const std::string title);
    EXPORT static int32_t CopyLocalSingleFile(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore,
        const int32_t &ownerAlbumId, std::shared_ptr<NativeRdb::ResultSet> &resultSet, int64_t &newAssetId,
        std::string displayName = "");
    EXPORT static int32_t CopyCloudSingleFile(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore,
        const int32_t &assetId, const int32_t &ownerAlbumId, std::shared_ptr<NativeRdb::ResultSet> &resultSet,
        int64_t &newAssetId);
    EXPORT static int32_t DeleteAlbumAndUpdateRelationship(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore,
        const int32_t &oldAlbumId, const int64_t &newAlbumId, bool isCloudAblum);
    EXPORT static bool IsCloudAlbum(std::shared_ptr<NativeRdb::ResultSet> resultSet);
    EXPORT static void BuildAlbumInsertValuesSetName(const std::shared_ptr<MediaLibraryRdbStore>& upgradeStore,
        NativeRdb::ValuesBucket &values, std::shared_ptr<NativeRdb::ResultSet> &resultSet,
        const std::string &newAlbumName);
    EXPORT static int32_t RefreshAllAlbums();
    EXPORT static int32_t GetAlbumFuseUpgradeStatus();
    EXPORT static int32_t SetAlbumFuseUpgradeStatus(int32_t upgradeStatus);
    EXPORT static void ReportAlbumFusionData(int64_t albumFusionTag, AlbumFusionState albumFusionState,
        const std::shared_ptr<MediaLibraryRdbStore> rdbStore);

private:
    EXPORT static void SetRefreshAlbum(bool needRefresh);
    EXPORT static int32_t HandleRestData(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore,
        const int32_t &assetId, const std::vector<int32_t> &restOwnerAlbumIds, int32_t &handledCount);
    EXPORT static int32_t HandleNoOwnerData(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore);
    EXPORT static int32_t HandleExpiredAlbumData(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore);
    EXPORT static int32_t QueryNoMatchedMap(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore,
        std::multimap<int32_t, std::vector<int32_t>> &notMatchedMap, bool isUpgrade);
    EXPORT static int32_t HandleNewCloudDirtyData(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore,
        std::multimap<int32_t, std::vector<int32_t>> &notMatchedMap);
    EXPORT static int32_t HandleDuplicateAlbum(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore);
    EXPORT static int32_t HandleMisMatchScreenRecord(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore);

private:
    static std::mutex cloudAlbumAndDataMutex_;
    static std::atomic<bool> isNeedRefreshAlbum;
};
}
}
#endif // OHOS_MEDIALIBRARY_ALBUM_FUSION_UTILS_H