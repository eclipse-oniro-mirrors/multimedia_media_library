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

#ifndef OHOS_MEDIA_BACKGROUND_MEDIA_FILE_MANAGER_OFFLINE_CLEANUP_DAO_H
#define OHOS_MEDIA_BACKGROUND_MEDIA_FILE_MANAGER_OFFLINE_CLEANUP_DAO_H

#include <string>
#include <vector>

#include "album_accurate_refresh.h"
#include "asset_accurate_refresh.h"

namespace OHOS::Media::Background {

struct OfflineCleanupPhotoRecord {
    int32_t fileId{0};
    int32_t mediaType{0};
    int32_t fileSourceType{0};
    int32_t position{0};
    int32_t hidden{0};
    int32_t subtype{0};
    int32_t burstCoverLevel{0};
    int32_t ownerAlbumId{0};
    int32_t albumSubtype{0};
    int32_t effectMode{0};
    int32_t dirty{0};
    int64_t dateTrashed{0};
    int64_t dateTaken{0};
    int64_t dateModified{0};
    int64_t size{0};
    std::string data;
    std::string sourcePath;
    std::string storagePath;
    std::string displayName;
    std::string burstKey;
    std::string cloudId;
    std::string albumName;
    std::string albumLpath;
    std::string ToString() const;
};

struct OfflineCleanupAlbumRecord {
    int32_t albumId{0};
    int32_t albumType{0};
    int32_t albumSubtype{0};
    int32_t dirty{0};
    std::string albumName;
    std::string lpath;
    std::string ToString() const;
};

class MediaFileManagerOfflineCleanupDao {
public:
    std::vector<OfflineCleanupPhotoRecord> QueryLocalDeleteCandidates(int32_t lastFileId, int32_t limit);
    std::vector<OfflineCleanupPhotoRecord> QueryPendingDeletedPhotos(int32_t lastFileId, int32_t limit);
    std::vector<OfflineCleanupPhotoRecord> QueryBurstCoverPhotos(int32_t lastFileId, int32_t limit);
    std::vector<OfflineCleanupPhotoRecord> QueryLocalCloudPhotos(int32_t lastFileId, int32_t limit);
    std::vector<OfflineCleanupPhotoRecord> QueryCloudOnlyPhotos(int32_t lastFileId, int32_t limit);
    std::vector<OfflineCleanupPhotoRecord> QueryLegacyAlbumPhotos(int32_t lastFileId, int32_t limit);
    std::vector<OfflineCleanupAlbumRecord> QueryEmptyLegacyAlbums(int32_t lastAlbumId, int32_t limit);

    bool MarkPhotosForOfflineCleanup(const std::vector<int32_t> &fileIds,
        AccurateRefresh::AssetAccurateRefresh &assetRefresh, int32_t &changedRows);
    bool DeleteOfflineCleanupPhotos(const std::vector<std::string> &fileIds,
        AccurateRefresh::AssetAccurateRefresh &assetRefresh, int32_t &deletedRows);

    bool UpdateBurstCoverPhoto(const OfflineCleanupPhotoRecord &photo,
        AccurateRefresh::AssetAccurateRefresh &assetRefresh);
    bool UpdateLocalCloudPhoto(const OfflineCleanupPhotoRecord &photo,
        AccurateRefresh::AssetAccurateRefresh &assetRefresh);
    bool UpdateCloudOnlyPhotos(const std::vector<int32_t> &fileIds,
        AccurateRefresh::AssetAccurateRefresh &assetRefresh);

    bool ExistMediaBurstMember(const OfflineCleanupPhotoRecord &photo);
    bool QueryAlbumByLpath(const std::string &lpath, OfflineCleanupAlbumRecord &album);
    bool IsAlbumNameOccupied(const std::string &albumName);
    bool RenewDeletedAlbum(int32_t albumId, AccurateRefresh::AlbumAccurateRefresh &albumRefresh);

    bool UpdatePhotoAlbumRelation(int32_t fileId, int32_t oldAlbumId, int32_t targetAlbumId,
        const std::string &targetSourcePath, AccurateRefresh::AssetAccurateRefresh &assetRefresh);
    bool LogicalDeleteEmptyLegacyAlbums(const std::vector<int32_t> &albumIds,
        AccurateRefresh::AlbumAccurateRefresh &albumRefresh, int32_t &deletedCount);

    int64_t CountLegacyPhotos();
    int64_t CountPendingDeletedPhotos();
    int64_t CountLegacyAlbums();
};

}  // namespace OHOS::Media::Background

#endif  // OHOS_MEDIA_BACKGROUND_MEDIA_FILE_MANAGER_OFFLINE_CLEANUP_DAO_H
