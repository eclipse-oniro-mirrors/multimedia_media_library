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

#define MLOG_TAG "MediaLibraryAllAlbumRefreshProcessor"

#include "medialibrary_all_album_refresh_processor.h"

#include "albums_refresh_manager.h"
#include "cloud_sync_helper.h"
#include "ffrt.h"
#include "ffrt_inner.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_unistore_manager.h"
#include "photo_album_column.h"
#include "vision_album_column.h"
#include "vision_column.h"

namespace OHOS {
namespace Media {
using namespace std;
namespace {
const int64_t REFRESH_ALL_ALBUMS_INTERVAL = 86400000000;  // 24 hours
}

shared_ptr<MediaLibraryAllAlbumRefreshProcessor> MediaLibraryAllAlbumRefreshProcessor::instance_ = nullptr;
mutex MediaLibraryAllAlbumRefreshProcessor::instanceMutex_;

shared_ptr<MediaLibraryAllAlbumRefreshProcessor> MediaLibraryAllAlbumRefreshProcessor::GetInstance()
{
    if (instance_ == nullptr) {
        lock_guard<mutex> guard(instanceMutex_);
        if (instance_ != nullptr) {
            return instance_;
        }
        auto *mediaLibraryAllAlbumRefreshProcessor = new (nothrow)MediaLibraryAllAlbumRefreshProcessor();
        if (mediaLibraryAllAlbumRefreshProcessor == nullptr) {
            MEDIA_ERR_LOG("Failed to new MediaLibraryAllAlbumRefreshProcessor");
        }
        instance_ = shared_ptr<MediaLibraryAllAlbumRefreshProcessor>(mediaLibraryAllAlbumRefreshProcessor);
    }
    return instance_;
}

MediaLibraryAllAlbumRefreshProcessor::MediaLibraryAllAlbumRefreshProcessor()
{
}

int64_t MediaLibraryAllAlbumRefreshProcessor::GetNowTimeUs()
{
    struct timespec t;
    constexpr int64_t SEC_TO_USEC = 1e6;
    constexpr int64_t NSEC_TO_USEC = 1e3;
    clock_gettime(CLOCK_REALTIME, &t);
    return t.tv_sec * SEC_TO_USEC + t.tv_nsec / NSEC_TO_USEC;
}

void MediaLibraryAllAlbumRefreshProcessor::OnCurrentStatusChanged(bool currentStatus)
{
    std::lock_guard<std::mutex> lock(refreshAllAlbumsLock_);
    currentStatus_ = currentStatus;
    MEDIA_INFO_LOG("OnCurrentStatusChanged! %{public}d", currentStatus_);
    if (currentStatus_) {
        PostRefreshAllAlbumsTask();
    }
}

void MediaLibraryAllAlbumRefreshProcessor::OnCloudSyncStateChanged(bool isCloudSyncing)
{
    std::lock_guard<std::mutex> lock(refreshAllAlbumsLock_);
    isCloudSyncing_ = isCloudSyncing;
    MEDIA_INFO_LOG("OnCloudSyncStateChanged! %{public}d", isCloudSyncing_);
    if (!isCloudSyncing_) {
        PostRefreshAllAlbumsTask();
    }
}

bool MediaLibraryAllAlbumRefreshProcessor::CheckRefreshConditionLocked()
{
    return currentStatus_ && !isCloudSyncing_ &&
           (albumRefreshStatus_ != AlbumRefreshStatus::NOT_START || lastRefreshAllAlbumsTime_ == 0 ||
               GetNowTimeUs() - lastRefreshAllAlbumsTime_ > REFRESH_ALL_ALBUMS_INTERVAL);
}

AlbumRefreshStatus GetNextRefreshStatus(AlbumRefreshStatus albumRefreshStatus)
{
    switch (albumRefreshStatus) {
        case AlbumRefreshStatus::NOT_START:
            return AlbumRefreshStatus::SYSTEM;
        case AlbumRefreshStatus::SYSTEM:
            return AlbumRefreshStatus::USER;
        case AlbumRefreshStatus::USER:
            return AlbumRefreshStatus::SOURCE;
        case AlbumRefreshStatus::SOURCE:
            return AlbumRefreshStatus::ANALYSIS;
        case AlbumRefreshStatus::ANALYSIS:
            return AlbumRefreshStatus::NOT_START;
        default:
            return AlbumRefreshStatus::NOT_START;
    }
}

static int32_t GetAlbumId(const shared_ptr<NativeRdb::ResultSet>& resultSet)
{
    int32_t albumId = 0;
    CHECK_AND_RETURN_RET_LOG(resultSet->GetInt(0, albumId) == NativeRdb::E_OK,
        E_HAS_DB_ERROR, "Failed to get album_id");
    return albumId;
}

int32_t GetPhotoAlbumIds(PhotoAlbumSubType albumSubtype, int32_t currentAlbumId, vector<int32_t>& albumIds)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "rdbStore is null");

    NativeRdb::RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(albumSubtype))
    ->And()
    ->GreaterThan(PhotoAlbumColumns::ALBUM_ID, currentAlbumId)
    ->OrderByAsc(PhotoAlbumColumns::ALBUM_ID);
    vector<string> columns = { PhotoAlbumColumns::ALBUM_ID };
    shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "Failed to query photo album");

    while (resultSet->GoToNextRow() == E_OK) {
        int32_t albumId = GetAlbumId(resultSet);
        if (albumId <= 0) {
            MEDIA_WARN_LOG("Failed to GetAlbumId: %{public}d", albumId);
            continue;
        }
        albumIds.push_back(albumId);
    }
    return E_OK;
}

int32_t GetSourceAlbumIds(int32_t currentAlbumId, vector<int32_t>& albumIds)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "rdbStore is null");

    NativeRdb::RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_TYPE, std::to_string(PhotoAlbumType::SOURCE))
    ->And()
    ->GreaterThan(PhotoAlbumColumns::ALBUM_ID, currentAlbumId)
    ->OrderByAsc(PhotoAlbumColumns::ALBUM_ID);
    vector<string> columns = { PhotoAlbumColumns::ALBUM_ID };
    shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "Failed to query photo album");

    while (resultSet->GoToNextRow() == E_OK) {
        int32_t albumId = GetAlbumId(resultSet);
        if (albumId <= 0) {
            MEDIA_WARN_LOG("Failed to GetAlbumId: %{public}d", albumId);
            continue;
        }
        albumIds.push_back(albumId);
    }
    return E_OK;
}

int32_t GetAnalysisAlbumIds(int32_t currentAlbumId, vector<int32_t>& albumIds)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "rdbStore is null");

    NativeRdb::RdbPredicates predicates(ANALYSIS_ALBUM_TABLE);
    predicates.GreaterThan(ALBUM_ID, currentAlbumId)->OrderByAsc(ALBUM_ID);
    vector<string> columns = { ALBUM_ID };
    shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "Failed to query analysis album");

    while (resultSet->GoToNextRow() == E_OK) {
        int32_t albumId = GetAlbumId(resultSet);
        if (albumId <= 0) {
            MEDIA_WARN_LOG("Failed to GetAlbumId: %{public}d", albumId);
            continue;
        }
        albumIds.push_back(albumId);
    }
    return E_OK;
}

int32_t GetAlbumIds(AlbumRefreshStatus albumRefreshStatus, int32_t currentAlbumId, vector<int32_t>& albumIds)
{
    switch (albumRefreshStatus) {
        case AlbumRefreshStatus::SYSTEM:
            // refresh all albums for system album, no need to fill albumIds
            return E_OK;
        case AlbumRefreshStatus::USER:
            return GetPhotoAlbumIds(PhotoAlbumSubType::USER_GENERIC, currentAlbumId, albumIds);
        case AlbumRefreshStatus::SOURCE:
            return GetSourceAlbumIds(currentAlbumId, albumIds);
        case AlbumRefreshStatus::ANALYSIS:
            return GetAnalysisAlbumIds(currentAlbumId, albumIds);
        default:
            MEDIA_ERR_LOG("Failed to check album refresh status: %{public}d", static_cast<int32_t>(albumRefreshStatus));
            return E_ERR;
    }
}

int32_t MediaLibraryAllAlbumRefreshProcessor::RefreshAlbums(AlbumRefreshStatus albumRefreshStatus,
    const vector<int32_t>& albumIds)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "rdbStore is null");
    if (albumRefreshStatus == AlbumRefreshStatus::SYSTEM) {
        MediaLibraryRdbUtils::UpdateSystemAlbumsByUris(rdbStore, AlbumOperationType::DEFAULT);
        return E_OK;
    }

    int32_t currentAlbumId = currentAlbumId_;
    for (auto iter = albumIds.begin(); iter != albumIds.end(); iter++) {
        int32_t albumId = *iter;
        if (albumRefreshStatus == AlbumRefreshStatus::USER) {
            MediaLibraryRdbUtils::UpdateUserAlbumInternal(rdbStore, { to_string(albumId) }, false, false);
            MediaLibraryRdbUtils::UpdateUserAlbumHiddenState(rdbStore, { to_string(albumId) });
        } else if (albumRefreshStatus == AlbumRefreshStatus::SOURCE) {
            MediaLibraryRdbUtils::UpdateSourceAlbumInternal(rdbStore, { to_string(albumId) }, false, false);
            MediaLibraryRdbUtils::UpdateSourceAlbumHiddenState(rdbStore, { to_string(albumId) });
        } else if (albumRefreshStatus == AlbumRefreshStatus::ANALYSIS) {
            MediaLibraryRdbUtils::UpdateAnalysisAlbumInternal(rdbStore, { to_string(albumId) });
        } else {
            MEDIA_WARN_LOG("Ignore to refresh album %{public}d id: %{public}d",
                static_cast<int32_t>(albumRefreshStatus), albumId);
        }

        currentAlbumId = albumId;
        if (!currentStatus_ || isCloudSyncing_) {
            MEDIA_INFO_LOG("Condition changes, currentStatus_: %{public}d, isCloudSyncing_: %{public}d",
                currentStatus_, isCloudSyncing_);
            break;
        }
    }

    if (albumIds.empty() || currentAlbumId == albumIds.back()) {
        MEDIA_INFO_LOG("Finish refreshing album type: %{public}d", albumRefreshStatus);
        return E_OK;
    }
    MEDIA_INFO_LOG(
        "Refresh album type: %{public}d, album id: %{public}d, currentStatus_: %{public}d, isCloudSyncing_: %{public}d",
        albumRefreshStatus, currentAlbumId, currentStatus_, isCloudSyncing_);
    return currentAlbumId;
}

void MediaLibraryAllAlbumRefreshProcessor::TryRefreshAllAlbums()
{
    {
        std::lock_guard<std::mutex> lock(refreshAllAlbumsLock_);
        if (!CheckRefreshConditionLocked()) {
            MEDIA_DEBUG_LOG("Not meet the condition of refreshing albums");
            return;
        }
        MEDIA_INFO_LOG("RefreshAllAlbums! now: %{public}" PRId64 ", last : %{public}" PRId64,
            GetNowTimeUs(), lastRefreshAllAlbumsTime_);
        lastRefreshAllAlbumsTime_ = GetNowTimeUs();
    }

    if (albumRefreshStatus_ == AlbumRefreshStatus::NOT_START) {
        albumRefreshStatus_ = GetNextRefreshStatus(AlbumRefreshStatus::NOT_START);
    }

    while (albumRefreshStatus_ != AlbumRefreshStatus::NOT_START && currentStatus_ && !isCloudSyncing_) {
        vector<int32_t> albumIds = {};
        int32_t ret = GetAlbumIds(albumRefreshStatus_, currentAlbumId_, albumIds);
        if (ret < E_OK) {
            albumRefreshStatus_ = GetNextRefreshStatus(albumRefreshStatus_);
            currentAlbumId_ = 0;
            continue;
        }

        ret = RefreshAlbums(albumRefreshStatus_, albumIds);
        if (ret > 0) {
            currentAlbumId_ = ret;
            break;
        }
        albumRefreshStatus_ = GetNextRefreshStatus(albumRefreshStatus_);
        currentAlbumId_ = 0;
    }
}

void MediaLibraryAllAlbumRefreshProcessor::PostRefreshAllAlbumsTask()
{
    ffrt::submit([this]() { TryRefreshAllAlbums(); });
}
} // namespace Media
} // namespace OHOS
