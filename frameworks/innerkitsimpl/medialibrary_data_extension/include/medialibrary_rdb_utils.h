/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIALIBRARY_RDB_UTILS_H
#define OHOS_MEDIALIBRARY_RDB_UTILS_H

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <map>

#include "medialibrary_rdbstore.h"
#include "rdb_predicates.h"
#include "rdb_store.h"
#include "userfile_manager_types.h"
#include "datashare_values_bucket.h"
#include "shooting_mode_column.h"
namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))

enum NotifyAlbumType : uint16_t {
    NO_NOTIFY = 0x00,
    SYS_ALBUM = 0x01,
    SYS_ALBUM_HIDDEN = 0X02,
    USER_ALBUM = 0X04,
    SOURCE_ALBUM = 0X08,
    ANA_ALBUM = 0X10,
};

enum AlbumOperationType : int32_t {
    DEFAULT = 0,
    DELETE_PHOTO,
    RECOVER_PHOTO,
    HIDE_PHOTO,
    UNHIDE_PHOTO,
};

struct AlbumCounts {
    int count;
    int hiddenCount;
    int imageCount;
    int videoCount;
};

struct UpdateAlbumData {
    int32_t albumId;
    int32_t albumSubtype;
    int32_t hiddenCount;
    int32_t albumCount;
    int32_t albumImageCount;
    int32_t albumVideoCount;
    std::string hiddenCover;
    std::string albumCoverUri;
    uint8_t isCoverSatisfied;
    int32_t newTotalCount {0};
    bool shouldNotify {false};
    bool hasChanged {false};
    bool shouldUpdateDateModified {false};
    int32_t coverUriSource {0};
    int64_t coverDateTime;
    int64_t hiddenCoverDateTime;
    std::string albumName;
    std::string groupTag;
};

struct UpdateAllAlbumsData {
    NotifyAlbumType type = NotifyAlbumType::NO_NOTIFY;
    bool isBackUpAndRestore = false;
    AlbumOperationType albumOperationType = AlbumOperationType::DEFAULT;
    bool shouldUpdateDateModified = false;
};

class MediaLibraryRdbUtils {
public:
    // Update count, cover_uri of albums.
    // If album id / subtype is specified, only update the specified album. Otherwise update all albums of the type.
    EXPORT static void UpdateSystemAlbumInternal(const std::shared_ptr<MediaLibraryRdbStore> rdbStore,
        const std::vector<std::string> &subtypes = {}, bool shouldNotify = false);
    EXPORT static void UpdateUserAlbumInternal(std::shared_ptr<MediaLibraryRdbStore> rdbStore,
        const std::vector<std::string> &userAlbumIds = {}, bool shouldNotify = false,
        bool shouldUpdateDateModified = false);
    EXPORT static void UpdateSourceAlbumInternal(std::shared_ptr<MediaLibraryRdbStore> rdbStore,
        const std::vector<std::string> &sourceAlbumIds = {}, bool shouldNotify = false,
        bool shouldUpdateDateModified = false);
    EXPORT static void UpdateCommonAlbumInternal(const std::shared_ptr<MediaLibraryRdbStore> rdbStore,
        const std::vector<std::string> &albumIds = {}, bool shouldNotify = false,
        bool shouldUpdateDateModified = false);
    EXPORT static void UpdateAnalysisAlbumInternal(const std::shared_ptr<MediaLibraryRdbStore>& rdbStore,
        const std::vector<std::string> &userAlbumIds = {});

    // Update hidden_count, hidden_cover of albums.
    // If album id / subtype is specified, only update the specified album. Otherwise update all albums of the type.
    EXPORT static void UpdateSysAlbumHiddenState(const std::shared_ptr<MediaLibraryRdbStore> rdbStore,
        const std::vector<std::string> &subtypes = {});
    EXPORT static void UpdateUserAlbumHiddenState(const std::shared_ptr<MediaLibraryRdbStore> rdbStore,
        const std::vector<std::string> &userAlbumIds = {});
    EXPORT static void UpdateSourceAlbumHiddenState(const std::shared_ptr<MediaLibraryRdbStore> rdbStore,
        const std::vector<std::string> &sourceAlbumIds = {});
    EXPORT static void UpdateCommonAlbumHiddenState(const std::shared_ptr<MediaLibraryRdbStore> rdbStore,
        const std::vector<std::string> &albumIds = {});

    // Update count, cover_uri, hidden_count, hidden_cover of albums.
    // Only update albums related with specified uri assets.
    // The specified uri assets must exist at the time of calling these functions.
    // If uris is empty, all albums of the type will be updated.
    EXPORT static void UpdateSystemAlbumsByUris(const std::shared_ptr<MediaLibraryRdbStore> rdbStore,
        AlbumOperationType albumOperationType, const std::vector<std::string> &uris = {},
        NotifyAlbumType type = NotifyAlbumType::NO_NOTIFY);
    EXPORT static void UpdateUserAlbumByUri(const std::shared_ptr<MediaLibraryRdbStore> rdbStore,
        const std::vector<std::string> &uris, bool shouldNotify = false, bool shouldUpdateDateModified = false);
    EXPORT static void UpdateSourceAlbumByUri(const std::shared_ptr<MediaLibraryRdbStore> rdbStore,
        const std::vector<std::string> &uris, bool shouldNotify = false, bool shouldUpdateDateModified = false);
    static void UpdateCommonAlbumByUri(const std::shared_ptr<MediaLibraryRdbStore> rdbStore,
        const std::vector<std::string> &uris, bool shouldNotify = false, bool shouldUpdateDateModified = false);
    EXPORT static void UpdateAnalysisAlbumByUri(const std::shared_ptr<MediaLibraryRdbStore> rdbStore,
        const std::vector<std::string> &uris);
    EXPORT static void UpdateAllAlbums(std::shared_ptr<MediaLibraryRdbStore> rdbStore,
        const std::vector<std::string> &uris = {},
        const UpdateAllAlbumsData &updateAlbumsData = UpdateAllAlbumsData());

    EXPORT static int32_t QueryAnalysisAlbumIdOfAssets(const std::vector<std::string>& assetIds,
        std::set<std::string>& albumIds);
    EXPORT static std::shared_ptr<NativeRdb::ResultSet> QueryPortraitAlbumCover(
        const std::shared_ptr<MediaLibraryRdbStore>& rdbStore, const std::string &albumId,
        const PhotoAlbumSubType &subType);
    EXPORT static int32_t QueryAnalysisAlbumMapByAssets(const std::vector<std::string>& assetIds,
        std::unordered_map<int32_t, std::set<int32_t>>& fileToAlbums, std::set<int32_t>& allAlbumIds);
    EXPORT static int32_t QueryAnalysisAlbumsForAccurateRefresh(
        const std::vector<int32_t> &affectedAlbumIds,
        std::vector<UpdateAlbumData> &albumDatas,
        std::unordered_map<std::string, std::vector<int32_t>> &portraitGroupMap);
    EXPORT static void GetAlbumCountAndCoverPredicates(const UpdateAlbumData& albumInfo,
        NativeRdb::RdbPredicates &predicates, const bool hiddenState, const bool isUpdateAlbum = false);
    EXPORT static void DetermineQueryOrder(NativeRdb::RdbPredicates& predicates, const UpdateAlbumData& data,
        bool hiddenState, std::vector<std::string>& columns);
    EXPORT static std::string GetCover(const std::shared_ptr<NativeRdb::ResultSet> &resultSet);

    static void AddQueryFilter(NativeRdb::AbsRdbPredicates &predicates);

    EXPORT static void UpdateAnalysisAlbumByFile(const std::shared_ptr<MediaLibraryRdbStore> rdbStore,
        const std::vector<std::string> &fileIds, const std::vector<int> &albumTypes);

    EXPORT static int32_t RefreshAllAlbums(const std::shared_ptr<MediaLibraryRdbStore> rdbStore,
        std::function<void(PhotoAlbumType, PhotoAlbumSubType, int)> refreshProcessHandler,
        std::function<void()> refreshCallback);
    EXPORT static int32_t IsNeedRefreshByCheckTable(const std::shared_ptr<MediaLibraryRdbStore> rdbStore,
        bool &signal);

    EXPORT static void UpdateSystemAlbumCountInternal(const std::shared_ptr<MediaLibraryRdbStore> rdbStore,
        const std::vector<std::string> &subtypes = {});
    EXPORT static void UpdateUserAlbumCountInternal(const std::shared_ptr<MediaLibraryRdbStore> rdbStore,
        const std::vector<std::string> &userAlbumIds = {});
    EXPORT static void UpdateAnalysisAlbumCountInternal(const std::shared_ptr<MediaLibraryRdbStore> rdbStore,
        const std::vector<std::string> &subtypes = {});
    EXPORT static void UpdateAllAlbumsForCloud(const std::shared_ptr<MediaLibraryRdbStore> rdbStore);
    EXPORT static void UpdateAllAlbumsCountForCloud(const std::shared_ptr<MediaLibraryRdbStore> rdbStore);

    EXPORT static bool IsNeedRefreshAlbum();
    EXPORT static void SetNeedRefreshAlbum(bool isNeedRefresh);
    EXPORT static bool IsInRefreshTask();
    EXPORT static int32_t GetAlbumIdsForPortrait(const std::shared_ptr<MediaLibraryRdbStore>& rdbStore,
        std::vector<std::string> &portraitAlbumIds);
    EXPORT static int32_t GetAlbumSubtypeArgument(const NativeRdb::RdbPredicates &predicates);
    EXPORT static void AddVirtualColumnsOfDateType(std::vector<std::string>& columns);
    EXPORT static void AddQueryIndex(NativeRdb::AbsPredicates& predicates, const std::vector<std::string>& columns);
    EXPORT static bool HasDataToAnalysis(const std::shared_ptr<MediaLibraryRdbStore> rdbStore);
    EXPORT static int32_t UpdateTrashedAssetOnAlbum(const std::shared_ptr<MediaLibraryRdbStore> rdbStore,
        NativeRdb::RdbPredicates &predicates);
    EXPORT static int32_t UpdateOwnerAlbumId(const std::shared_ptr<MediaLibraryRdbStore> rdbStore,
        const std::vector<DataShare::DataShareValuesBucket> &values, std::vector<int32_t> &updateIds, bool &hidden);
    EXPORT static int32_t UpdateRemovedAssetToTrash(const std::shared_ptr<MediaLibraryRdbStore> rdbStore,
        const std::vector<std::string> &whereIdArgs);
    EXPORT static int32_t UpdateThumbnailRelatedDataToDefault(const std::shared_ptr<MediaLibraryRdbStore> rdbStore,
        const int64_t fileId);
    EXPORT static void TransformAppId2TokenId(const std::shared_ptr<MediaLibraryRdbStore> &store);
    EXPORT static int32_t FillOneAlbumCountAndCoverUri(const std::shared_ptr<MediaLibraryRdbStore> rdbStore,
        int32_t albumId, PhotoAlbumSubType subtype, std::string &sql);
    EXPORT static void UpdateSystemAlbumExcludeSource(bool shouldNotify = false);
    EXPORT static int32_t UpdateHighlightPlayInfo(const std::shared_ptr<MediaLibraryRdbStore> rdbStore,
        const std::string &albumId);
    EXPORT static bool AnalyzePhotosData();
    EXPORT static int32_t GetUpdateValues(const std::shared_ptr<MediaLibraryRdbStore> rdbStore,
        UpdateAlbumData &data, NativeRdb::ValuesBucket &values, const bool hiddenState);
    EXPORT static bool QueryShootingModeAlbumIdByType(ShootingModeAlbumType type, int32_t& albumId);
    EXPORT static bool QueryAllShootingModeAlbumIds(std::vector<int32_t>& albumIds);
    EXPORT static void TransformOwnerAppIdToTokenId(const std::shared_ptr<MediaLibraryRdbStore> &rdbStore);
    EXPORT static void CleanAmbiguousColumn(std::vector<std::string> &columns,
        DataShare::DataSharePredicates &predicates, const std::string tableName);
    EXPORT static int32_t GetAlbumIdBySubType(PhotoAlbumSubType subtype);
    EXPORT static bool ExecuteDatabaseQuickCheck(const std::shared_ptr<MediaLibraryRdbStore> &rdbStore);
    EXPORT static void UpdateAnalysisAlbumByCoverUri(const std::shared_ptr<MediaLibraryRdbStore>& rdbStore,
        const std::string& fileId);

private:
    static std::atomic<bool> isNeedRefreshAlbum;
    static std::atomic<bool> isInRefreshTask;
    static std::mutex sRefreshAlbumMutex_;
    static std::map<PhotoAlbumSubType, int32_t> subType2AlbumIdMap;
};
} // namespace OHOS::Media
#endif // OHOS_MEDIALIBRARY_RDB_UTILS_H
