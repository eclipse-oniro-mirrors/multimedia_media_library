/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_BACKGROUND_MEDIA_FILE_MANAGER_OFFLINE_CLEANUP_TASK_H
#define OHOS_MEDIA_BACKGROUND_MEDIA_FILE_MANAGER_OFFLINE_CLEANUP_TASK_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>

#include "album_accurate_refresh.h"
#include "asset_accurate_refresh.h"
#include "i_media_background_task.h"
#include "media_file_manager_offline_cleanup_dao.h"

namespace OHOS::Media::Background {

class MediaFileManagerOfflineCleanupTask : public IMediaBackGroundTask {
public:
    virtual ~MediaFileManagerOfflineCleanupTask() = default;

    bool Accept() override;
    void Execute() override;

private:
    struct CleanupResult {
        std::string tag;
        int32_t startId{0};
        int32_t endId{0};
        int64_t count{0};

        std::string ToString() const
        {
            return tag + "[" + std::to_string(startId) + ", " + std::to_string(endId) + ", " +
                std::to_string(count) + "]";
        }
    };

    struct CleanupStatistics {
        CleanupResult markedForDeletion{"mark"};
        CleanupResult deletedPhotos{"delete"};
        CleanupResult burstConverted{"burst"};
        CleanupResult localCloudConverted{"localCloud"};
        CleanupResult cloudOnlyConverted{"cloudOnly"};
        CleanupResult albumRelationsMigrated{"migrateAlbum"};
        CleanupResult legacyAlbumsDeleted{"deleteAlbum"};
    };

    void ResetRunState();
    void PrepareProgress();

    void ProcessLocalPhotosToDelete();
    void CleanupPendingDeletedPhotos();

    void ConvertBurstCoverPhotos();
    void ConvertLocalCloudPhotos();
    void ConvertCloudOnlyPhotos();

    void MigratePhotoAlbumRelations();
    void CleanupLegacyAlbums();
    void ReportCleanupResult();
    void WriteDeleteAuditLog(const OfflineCleanupPhotoRecord &photo, int32_t totalCount);
    void WriteAlbumDeleteAuditLog(const OfflineCleanupAlbumRecord &album, int32_t totalCount);
    void LogBatchResult(const char *stage, int32_t startCursor, int32_t endCursor,
        size_t scannedCount, int64_t processedCount) const;

    bool ShouldMarkForDeletion(const OfflineCleanupPhotoRecord &photo);
    bool ShouldConvertToMediaBurstCover(const OfflineCleanupPhotoRecord &photo);
    bool ConvertBurstCoverPhoto(const OfflineCleanupPhotoRecord &photo);
    bool ConvertLocalCloudPhoto(const OfflineCleanupPhotoRecord &photo);
    bool CreateOriginDentry(const OfflineCleanupPhotoRecord &photo);

    int32_t EnsureTargetAlbum(const OfflineCleanupAlbumRecord &sourceAlbum);
    std::string ResolveTargetAlbumName(const OfflineCleanupAlbumRecord &sourceAlbum);

    int32_t GetSavedTaskVersion();
    void SaveTaskVersion(int32_t version);
    int32_t LoadCursor(const std::string &key);
    void SaveCursor(const std::string &key, int32_t value);
    void ResetAllCursors();

    void RefreshAssets();

    std::unordered_map<std::string, int32_t> targetAlbumIdCache_;
    CleanupStatistics statistics_;

    MediaFileManagerOfflineCleanupDao cleanupDao_;
    AccurateRefresh::AssetAccurateRefresh assetRefresh_;
    AccurateRefresh::AlbumAccurateRefresh albumRefresh_;
};

}  // namespace OHOS::Media::Background

#endif  // OHOS_MEDIA_BACKGROUND_MEDIA_FILE_MANAGER_OFFLINE_CLEANUP_TASK_H
