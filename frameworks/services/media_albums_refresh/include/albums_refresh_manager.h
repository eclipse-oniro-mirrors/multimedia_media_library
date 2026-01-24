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

#ifndef FRAMEWORKS_SERVICES_MEDIA_ALBUMS_REFRESH_INCLUDE_ALBUMS_REFRESH_MANAGER_H
#define FRAMEWORKS_SERVICES_MEDIA_ALBUMS_REFRESH_INCLUDE_ALBUMS_REFRESH_MANAGER_H

#include <string>
#include <unordered_map>
#include <mutex>


#include "medialibrary_type_const.h"
#include "medialibrary_command.h"
#include "result_set.h"
#include "rdb_predicates.h"
#include "rdb_store.h"
#include "rdb_utils.h"
#include "medialibrary_rdbstore.h"
#include "albums_refresh_worker.h"
#include "cloud_sync_observer.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
struct RefreshAlbumData {
    int32_t albumId;
    int32_t albumSubtype;
};

class AlbumsRefreshManager {
public:
    EXPORT static AlbumsRefreshManager& GetInstance();
    EXPORT void RefreshPhotoAlbums(SyncNotifyInfo& info);
    EXPORT void AddAlbumRefreshTask(SyncNotifyInfo& info);
    EXPORT void NotifyPhotoAlbums(SyncNotifyInfo& info);
    EXPORT SyncNotifyInfo GetSyncNotifyInfo(CloudSyncNotifyInfo &notifyInfo, uint8_t uriType);
    EXPORT void TryDeleteAlbum(SyncNotifyInfo &info, std::vector<std::string>& albumIds);
    EXPORT void GetSystemAlbumIds(SyncNotifyInfo& info, std::vector<std::string>& albumIds);
    EXPORT std::shared_ptr<NativeRdb::ResultSet>  CovertCloudId2AlbumId(
        const std::shared_ptr<MediaLibraryRdbStore> rdbStore, std::vector<std::string>& cloudIds);
    EXPORT std::shared_ptr<NativeRdb::ResultSet>  CovertCloudId2FileId(
        const std::shared_ptr<MediaLibraryRdbStore> rdbStore, std::vector<std::string>& cloudIds);
    EXPORT bool HasRefreshingSystemAlbums();
    EXPORT void RefreshPhotoAlbumsBySyncNotifyInfo(const std::shared_ptr<MediaLibraryRdbStore> rdbStore,
        SyncNotifyInfo &info);
    EXPORT int32_t SendNotifyInfoOfAssetAndAlbum(const NotifyType type, const std::set<std::string> &cloudIds,
        const std::set<std::string> &albumIds);

private:
    AlbumsRefreshManager();
    ~AlbumsRefreshManager();
    AlbumsRefreshManager(const AlbumsRefreshManager &manager) = delete;
    const AlbumsRefreshManager &operator=(const AlbumsRefreshManager &manager) = delete;
    void AddSystemAlbumIds(std::set<std::string> &albumIds);

    std::shared_ptr<AlbumsRefreshWorker> refreshWorker_;
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_ALBUMS_REFRESH_INCLUDE_ALBUMS_REFRESH_MANAGER_H
