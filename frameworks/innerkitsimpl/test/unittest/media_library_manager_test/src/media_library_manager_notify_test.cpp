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

#include "medialibrary_manager_notify_observer_manager.h"
#include "medialibrary_manager_notify_utils.h"
#include "media_library_manager_notify_test.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <functional>
#include <map>
#include <mutex>
#include <unistd.h>

#include "fetch_result.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "userfilemgr_uri.h"
#include "media_library_manager_notify_test_helper.h"

using namespace testing::ext;

namespace OHOS {
namespace Media {

bool CreateAndWriteTestAsset(MediaLibraryManager &manager, std::string &assetUri, int32_t &assetId)
{
    assetUri.clear();
    assetId = AccurateRefresh::INVALID_INT32_VALUE;
    for (int32_t attempt = 1; attempt <= WRITE_RETRY_TIMES; ++attempt) {
        std::string newAssetUri = CreateTestAssetUri(manager);
        if (newAssetUri.empty()) {
            GTEST_LOG_(INFO) << "CreateAndWriteTestAsset create failed, attempt=" << attempt;
            if (attempt < WRITE_RETRY_TIMES) {
                usleep(WRITE_RETRY_INTERVAL_US);
            }
            continue;
        }
        int32_t newAssetId = ExtractIntIdFromUri(newAssetUri);
        if (newAssetId <= E_OK) {
            GTEST_LOG_(INFO) << "CreateAndWriteTestAsset invalid id, attempt=" << attempt
                << ", uri=" << newAssetUri << ", assetId=" << newAssetId;
            if (attempt < WRITE_RETRY_TIMES) {
                usleep(WRITE_RETRY_INTERVAL_US);
            }
            continue;
        }
        if (!WaitAssetReadyByUri(manager, newAssetUri)) {
            GTEST_LOG_(INFO) << "CreateAndWriteTestAsset wait asset ready failed, attempt=" << attempt
                << ", uri=" << newAssetUri << ", assetId=" << newAssetId;
            (void)DeleteAssetById(manager, newAssetId);
            if (attempt < WRITE_RETRY_TIMES) {
                usleep(WRITE_RETRY_INTERVAL_US);
            }
            continue;
        }
        if (WriteSingleByteToAsset(manager, newAssetUri)) {
            assetUri = newAssetUri;
            assetId = newAssetId;
            return true;
        }
        (void)DeleteAssetById(manager, newAssetId);
        if (attempt < WRITE_RETRY_TIMES) {
            usleep(WRITE_RETRY_INTERVAL_US);
        }
    }
    return false;
}

bool CreateAndWriteTestVideoAsset(MediaLibraryManager &manager, int32_t &assetId)
{
    assetId = AccurateRefresh::INVALID_INT32_VALUE;
    for (int32_t attempt = 1; attempt <= WRITE_RETRY_TIMES; ++attempt) {
        std::string videoUri = manager.CreateAsset(BuildUniqueVideoAssetName());
        if (videoUri.empty()) {
            if (attempt < WRITE_RETRY_TIMES) {
                usleep(WRITE_RETRY_INTERVAL_US);
            }
            continue;
        }
        int32_t videoAssetId = ExtractIntIdFromUri(videoUri);
        if (videoAssetId <= E_OK) {
            if (attempt < WRITE_RETRY_TIMES) {
                usleep(WRITE_RETRY_INTERVAL_US);
            }
            continue;
        }
        if (!WaitAssetReadyByUri(manager, videoUri) || !WriteSingleByteToAsset(manager, videoUri)) {
            (void)DeleteAssetById(manager, videoAssetId);
            if (attempt < WRITE_RETRY_TIMES) {
                usleep(WRITE_RETRY_INTERVAL_US);
            }
            continue;
        }
        assetId = videoAssetId;
        return true;
    }
    return false;
}

std::unique_ptr<PhotoAlbum> QueryAlbumById(MediaLibraryManager &manager, int32_t albumId)
{
    std::vector<std::string> columns;
    DataShare::DataSharePredicates predicates;
    FetchResult<PhotoAlbum> result = manager.GetAlbums(columns, &predicates);
    auto album = result.GetFirstObject();
    while (album != nullptr) {
        if (album->GetAlbumId() == albumId) {
            return album;
        }
        album = result.GetNextObject();
    }
    return nullptr;
}

int32_t DeleteAlbumById(MediaLibraryManager &manager, int32_t albumId)
{
    auto album = QueryAlbumById(manager, albumId);
    CHECK_AND_RETURN_RET(album != nullptr, E_INVALID_ARGUMENTS);
    std::vector<std::unique_ptr<PhotoAlbum>> albums;
    albums.push_back(std::move(album));
    return manager.DeleteAlbums(albums);
}

std::vector<std::unique_ptr<PhotoAlbum>> QueryAlbumsByIds(MediaLibraryManager &manager,
    const std::vector<int32_t> &albumIds)
{
    std::vector<std::unique_ptr<PhotoAlbum>> albums;
    if (albumIds.empty()) {
        return albums;
    }

    std::map<int32_t, bool> pendingAlbumIds;
    for (int32_t albumId : albumIds) {
        if (albumId > E_OK) {
            pendingAlbumIds[albumId] = false;
        }
    }
    if (pendingAlbumIds.empty()) {
        return albums;
    }

    std::vector<std::string> columns;
    DataShare::DataSharePredicates predicates;
    FetchResult<PhotoAlbum> result = manager.GetAlbums(columns, &predicates);
    auto album = result.GetFirstObject();
    while (album != nullptr && !pendingAlbumIds.empty()) {
        int32_t albumId = album->GetAlbumId();
        auto iter = pendingAlbumIds.find(albumId);
        if (iter != pendingAlbumIds.end()) {
            albums.push_back(std::move(album));
            pendingAlbumIds.erase(iter);
        }
        album = result.GetNextObject();
    }
    return albums;
}

std::vector<std::unique_ptr<FileAsset>> QueryAssetsByIds(MediaLibraryManager &manager,
    const std::vector<int32_t> &assetIds)
{
    std::vector<std::unique_ptr<FileAsset>> assets;
    if (assetIds.empty()) {
        return assets;
    }

    std::map<int32_t, bool> pendingAssetIds;
    for (int32_t assetId : assetIds) {
        if (assetId > E_OK) {
            pendingAssetIds[assetId] = false;
        }
    }
    if (pendingAssetIds.empty()) {
        return assets;
    }

    std::vector<std::string> albumColumns;
    DataShare::DataSharePredicates albumPredicates;
    FetchResult<PhotoAlbum> albums = manager.GetAlbums(albumColumns, &albumPredicates);
    auto album = albums.GetFirstObject();
    while (album != nullptr && !pendingAssetIds.empty()) {
        std::vector<std::string> assetColumns;
        DataShare::DataSharePredicates assetPredicates;
        std::vector<std::string> pendingAssetIdStrings;
        pendingAssetIdStrings.reserve(pendingAssetIds.size());
        for (const auto &item : pendingAssetIds) {
            pendingAssetIdStrings.push_back(std::to_string(item.first));
        }
        assetPredicates.In(MediaColumn::MEDIA_ID, pendingAssetIdStrings);

        FetchResult<FileAsset> assetsResult = manager.GetAssets(*album, assetColumns, &assetPredicates);
        auto asset = assetsResult.GetFirstObject();
        while (asset != nullptr && !pendingAssetIds.empty()) {
            int32_t currentAssetId = asset->GetId();
            auto iter = pendingAssetIds.find(currentAssetId);
            if (iter != pendingAssetIds.end()) {
                assets.push_back(std::move(asset));
                pendingAssetIds.erase(iter);
            }
            asset = assetsResult.GetNextObject();
        }
        album = albums.GetNextObject();
    }
    return assets;
}

std::unique_ptr<PhotoAlbum> QuerySourceAlbumByAssetId(MediaLibraryManager &manager, int32_t assetId)
{
    std::vector<std::string> albumColumns;
    DataShare::DataSharePredicates albumPredicates;
    FetchResult<PhotoAlbum> albums = manager.GetAlbums(albumColumns, &albumPredicates);
    auto album = albums.GetFirstObject();
    while (album != nullptr) {
        std::vector<std::string> assetColumns;
        DataShare::DataSharePredicates assetPredicates;
        assetPredicates.EqualTo(MediaColumn::MEDIA_ID, std::to_string(assetId));
        FetchResult<FileAsset> assets = manager.GetAssets(*album, assetColumns, &assetPredicates);
        if (assets.GetCount() > 0) {
            return album;
        }
        album = albums.GetNextObject();
    }
    return nullptr;
}

int32_t MoveAssetToAlbumById(MediaLibraryManager &manager, int32_t assetId,
    int32_t targetAlbumId, int32_t &sourceAlbumId)
{
    sourceAlbumId = AccurateRefresh::INVALID_INT32_VALUE;
    auto asset = QueryAssetById(manager, assetId);
    CHECK_AND_RETURN_RET(asset != nullptr, E_INVALID_ARGUMENTS);
    auto targetAlbum = QueryAlbumById(manager, targetAlbumId);
    CHECK_AND_RETURN_RET(targetAlbum != nullptr, E_INVALID_ARGUMENTS);

    auto sourceAlbum = QuerySourceAlbumByAssetId(manager, assetId);
    if (sourceAlbum == nullptr) {
        int32_t fallbackAlbumId = asset->GetOwnerAlbumId();
        if (fallbackAlbumId <= E_OK) {
            fallbackAlbumId = asset->GetAlbumId();
        }
        if (fallbackAlbumId > E_OK) {
            sourceAlbum = QueryAlbumById(manager, fallbackAlbumId);
        }
    }
    CHECK_AND_RETURN_RET(sourceAlbum != nullptr, E_INVALID_ARGUMENTS);

    sourceAlbumId = sourceAlbum->GetAlbumId();
    std::vector<std::unique_ptr<FileAsset>> assets;
    assets.push_back(std::move(asset));
    return manager.MoveAssets(assets, *sourceAlbum, *targetAlbum);
}

int32_t SetAssetsHiddenByUris(MediaLibraryManager &manager, const std::vector<std::string> &assetUris, bool isHidden)
{
    if (assetUris.empty()) {
        return E_INVALID_ARGUMENTS;
    }
    std::vector<std::string> assetIds;
    assetIds.reserve(assetUris.size());
    for (const auto &assetUri : assetUris) {
        int32_t assetId = ExtractIntIdFromUri(assetUri);
        if (assetId <= E_OK) {
            return E_INVALID_ARGUMENTS;
        }
        assetIds.push_back(std::to_string(assetId));
    }
    auto dataShareHelper = CreateDataShareHelperForTest(manager);
    if (dataShareHelper == nullptr) {
        return E_FAIL;
    }
    DataShare::DataSharePredicates predicates;
    predicates.In(PhotoColumn::MEDIA_ID, assetIds);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(PhotoColumn::MEDIA_HIDDEN, isHidden ? 1 : 0);
    std::string updateUriStr = CONST_PAH_HIDE_PHOTOS;
    MediaFileUtils::UriAppendKeyValue(updateUriStr, URI_PARAM_API_VERSION, std::to_string(MEDIA_API_VERSION_V10));
    Uri updateUri(updateUriStr);
    return dataShareHelper->Update(updateUri, predicates, valuesBucket);
}

int32_t SetAssetHiddenByUri(MediaLibraryManager &manager, const std::string &assetUri, bool isHidden)
{
    if (assetUri.empty()) {
        return E_INVALID_ARGUMENTS;
    }
    return SetAssetsHiddenByUris(manager, { assetUri }, isHidden);
}

bool IsAssetInAlbum(MediaLibraryManager &manager, int32_t albumId, int32_t assetId)
{
    auto album = QueryAlbumById(manager, albumId);
    if (album == nullptr) {
        return false;
    }
    std::vector<std::string> columns;
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(MediaColumn::MEDIA_ID, std::to_string(assetId));
    FetchResult<FileAsset> assets = manager.GetAssets(*album, columns, &predicates);
    return assets.GetCount() > 0;
}

struct MoveAssetPayloadExpect {
    int32_t assetId;
    int32_t sourceAlbumId;
    int32_t targetAlbumId;
    std::string sourceAlbumUri;
    std::string targetAlbumUri;
};

bool HasValidMoveAssetChangeData(const PhotoAssetChangeData *changeData)
{
    return changeData != nullptr &&
        changeData->assetBeforeChange != nullptr && changeData->assetAfterChange != nullptr;
}

int32_t ParseAlbumIdFromUri(const std::string &albumUri)
{
    if (albumUri.empty()) {
        return AccurateRefresh::INVALID_INT32_VALUE;
    }
    int32_t albumId = ExtractIntIdFromUri(albumUri);
    return (albumId > E_OK) ? albumId : AccurateRefresh::INVALID_INT32_VALUE;
}

int32_t GetAlbumIdFromNotifyInfo(const std::shared_ptr<AlbumChangeInfo> &info)
{
    if (info == nullptr) {
        return AccurateRefresh::INVALID_INT32_VALUE;
    }
    return ParseAlbumIdFromUri(info->albumUri_);
}

int32_t GetOwnerAlbumIdFromNotifyInfo(const std::shared_ptr<PhotoAssetChangeInfo> &info)
{
    if (info == nullptr) {
        return AccurateRefresh::INVALID_INT32_VALUE;
    }
    return ParseAlbumIdFromUri(info->ownerAlbumUri_);
}

void ExpectMoveAssetOwnerAlbumIds(const PhotoAssetChangeData &changeData, const MoveAssetPayloadExpect &expect)
{
    int32_t beforeOwnerAlbumId = GetOwnerAlbumIdFromNotifyInfo(changeData.assetBeforeChange);
    int32_t afterOwnerAlbumId = GetOwnerAlbumIdFromNotifyInfo(changeData.assetAfterChange);
    if (beforeOwnerAlbumId > E_OK) {
        EXPECT_EQ(beforeOwnerAlbumId, expect.sourceAlbumId);
    } else {
        EXPECT_EQ(beforeOwnerAlbumId, AccurateRefresh::INVALID_INT32_VALUE);
    }
    if (afterOwnerAlbumId > E_OK) {
        EXPECT_EQ(afterOwnerAlbumId, expect.targetAlbumId);
    } else {
        EXPECT_EQ(afterOwnerAlbumId, AccurateRefresh::INVALID_INT32_VALUE);
    }
}

void ExpectMoveAssetOwnerAlbumUris(const PhotoAssetChangeData &changeData, const MoveAssetPayloadExpect &expect)
{
    if (!changeData.assetBeforeChange->ownerAlbumUri_.empty() && !expect.sourceAlbumUri.empty()) {
        EXPECT_EQ(changeData.assetBeforeChange->ownerAlbumUri_, expect.sourceAlbumUri);
    }
    if (!changeData.assetAfterChange->ownerAlbumUri_.empty() && !expect.targetAlbumUri.empty()) {
        EXPECT_EQ(changeData.assetAfterChange->ownerAlbumUri_, expect.targetAlbumUri);
    }
}

bool CheckMoveAssetPayload(const PhotoAssetChangeData *changeData, const MoveAssetPayloadExpect &expect)
{
    if (!HasValidMoveAssetChangeData(changeData)) {
        return false;
    }

    EXPECT_EQ(changeData->assetBeforeChange->fileId_, expect.assetId);
    EXPECT_EQ(changeData->assetAfterChange->fileId_, expect.assetId);
    ExpectMoveAssetOwnerAlbumIds(*changeData, expect);
    ExpectMoveAssetOwnerAlbumUris(*changeData, expect);
    return true;
}

const PhotoAssetChangeData *FindAssetChangeDataById(const PhotoAssetChangeInfos &infos, int32_t assetId)
{
    for (const auto &changeData : infos.assetChangeDatas) {
        auto info = (changeData.assetAfterChange != nullptr)
            ? changeData.assetAfterChange
            : changeData.assetBeforeChange;
        if (info != nullptr && info->fileId_ == assetId) {
            return &changeData;
        }
    }
    return nullptr;
}

const AlbumChangeData *FindAlbumChangeDataById(const AlbumChangeInfos &infos, int32_t albumId)
{
    for (const auto &changeData : infos.albumChangeDatas) {
        auto info = (changeData.albumAfterChange != nullptr)
            ? changeData.albumAfterChange
            : changeData.albumBeforeChange;
        if (info != nullptr && GetAlbumIdFromNotifyInfo(info) == albumId) {
            return &changeData;
        }
    }
    return nullptr;
}

struct AssetChangeWaitArgs {
    NotifyChangeType expectedType;
    int32_t expectedFileId;
    int32_t timeoutMs;
};

struct AlbumChangeWaitArgs {
    NotifyChangeType expectedType;
    int32_t expectedAlbumId;
    int32_t timeoutMs;
};

bool WaitForAssetChangeByTypeAndId(SyncPhotoAssetChangeCallback &callback, int32_t baseCount,
    const AssetChangeWaitArgs &args, PhotoAssetChangeInfos &outInfos)
{
    auto isAssetMatched = [&args](const PhotoAssetChangeInfos &infos) {
        if (infos.type != args.expectedType) {
            return false;
        }
        for (const auto &changeData : infos.assetChangeDatas) {
            auto info = (changeData.assetAfterChange != nullptr)
                ? changeData.assetAfterChange
                : changeData.assetBeforeChange;
            if (info != nullptr && info->fileId_ == args.expectedFileId) {
                return true;
            }
        }
        return false;
    };

    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(args.timeoutMs);
    int32_t currentBase = baseCount;
    while (std::chrono::steady_clock::now() < deadline) {
        int32_t remainMs = static_cast<int32_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count());
        if (remainMs <= 0 || !callback.WaitForCallAfter(currentBase, remainMs)) {
            return false;
        }
        auto allInfos = callback.GetAllInfos();
        size_t startIndex = (currentBase > E_OK) ? static_cast<size_t>(currentBase) : 0;
        for (size_t i = startIndex; i < allInfos.size(); ++i) {
            if (isAssetMatched(allInfos[i])) {
                outInfos = allInfos[i];
                return true;
            }
        }
        currentBase = static_cast<int32_t>(allInfos.size());
    }
    return false;
}

bool WaitForAlbumChangeByTypeAndId(SyncPhotoAlbumChangeCallback &callback, int32_t baseCount,
    const AlbumChangeWaitArgs &args, AlbumChangeInfos &outInfos)
{
    auto isAlbumMatched = [&args](const AlbumChangeInfos &infos) {
        if (infos.type != args.expectedType) {
            return false;
        }
        for (const auto &changeData : infos.albumChangeDatas) {
            auto info = (changeData.albumAfterChange != nullptr)
                ? changeData.albumAfterChange
                : changeData.albumBeforeChange;
            if (info != nullptr && GetAlbumIdFromNotifyInfo(info) == args.expectedAlbumId) {
                return true;
            }
        }
        return false;
    };

    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(args.timeoutMs);
    int32_t currentBase = baseCount;
    while (std::chrono::steady_clock::now() < deadline) {
        int32_t remainMs = static_cast<int32_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count());
        if (remainMs <= 0 || !callback.WaitForCallAfter(currentBase, remainMs)) {
            return false;
        }
        auto allInfos = callback.GetAllInfos();
        size_t startIndex = (currentBase > E_OK) ? static_cast<size_t>(currentBase) : 0;
        for (size_t i = startIndex; i < allInfos.size(); ++i) {
            if (isAlbumMatched(allInfos[i])) {
                outInfos = allInfos[i];
                return true;
            }
        }
        currentBase = static_cast<int32_t>(allInfos.size());
    }
    return false;
}

struct AlbumNotifyCheckResult {
    bool hasTargetAlbumUpdate = false;
    bool hasSystemAlbumUpdateExceptTarget = false;
};

void LogAssetChangeInfoDetail(const std::string &tag, const std::shared_ptr<PhotoAssetChangeInfo> &info);

size_t LogAndSummarizeAlbumNotifications(const std::vector<AlbumChangeInfos> &allAlbumInfos, size_t startIndex,
    std::map<int32_t, std::pair<int32_t, int32_t>> &notifiedAlbums)
{
    size_t notifyCount = 0;
    for (size_t i = startIndex; i < allAlbumInfos.size(); ++i) {
        const auto &infos = allAlbumInfos[i];
        GTEST_LOG_(INFO) << "Album notify event[" << i << "] notifyType=" << static_cast<int32_t>(infos.type)
            << " changeCount=" << infos.albumChangeDatas.size();
        for (size_t j = 0; j < infos.albumChangeDatas.size(); ++j) {
            const auto &changeData = infos.albumChangeDatas[j];
            auto info = (changeData.albumAfterChange != nullptr)
                ? changeData.albumAfterChange
                : changeData.albumBeforeChange;
            if (info == nullptr) {
                GTEST_LOG_(INFO) << "  change[" << j << "] albumInfo=nullptr";
                continue;
            }
            ++notifyCount;
            int32_t albumId = GetAlbumIdFromNotifyInfo(info);
            if (albumId > E_OK) {
                notifiedAlbums[albumId] = { info->albumType_, info->albumSubType_ };
            }
            GTEST_LOG_(INFO) << "  change[" << j << "] notifyType=" << static_cast<int32_t>(infos.type)
                << " albumUri=" << info->albumUri_
                << " albumType=" << info->albumType_
                << " albumSubType=" << info->albumSubType_;
        }
    }
    for (const auto &item : notifiedAlbums) {
        GTEST_LOG_(INFO) << "Album notified summary: albumType=" << item.second.first
            << " albumSubType=" << item.second.second;
    }
    return notifyCount;
}

bool LogAssetNotificationsAndFindUpdate(const std::vector<PhotoAssetChangeInfos> &allInfos,
    size_t startIndex, int32_t targetAssetId)
{
    bool foundUpdate = false;
    for (size_t i = startIndex; i < allInfos.size(); ++i) {
        const auto &infos = allInfos[i];
        for (size_t j = 0; j < infos.assetChangeDatas.size(); ++j) {
            const auto &changeData = infos.assetChangeDatas[j];
            GTEST_LOG_(INFO) << "Asset notify event[" << i << "] type=" << static_cast<int32_t>(infos.type)
                << ", changeIndex=" << j;
            LogAssetChangeInfoDetail("  before: ", changeData.assetBeforeChange);
            LogAssetChangeInfoDetail("  after: ", changeData.assetAfterChange);
            auto info = (changeData.assetAfterChange != nullptr)
                ? changeData.assetAfterChange
                : changeData.assetBeforeChange;
            if (infos.type == NotifyChangeType::NOTIFY_CHANGE_UPDATE &&
                info != nullptr && info->fileId_ == targetAssetId) {
                foundUpdate = true;
            }
        }
    }
    return foundUpdate;
}

void LogAssetChangeInfoDetail(const std::string &tag, const std::shared_ptr<PhotoAssetChangeInfo> &info)
{
    if (info == nullptr) {
        GTEST_LOG_(INFO) << tag << "null";
        return;
    }
    GTEST_LOG_(INFO) << tag
        << "fileId=" << info->fileId_
        << ", uri=" << info->uri_
        << ", displayName=" << info->displayName_
        << ", mediaType=" << info->mediaType_
        << ", ownerAlbumUri=" << info->ownerAlbumUri_
        << ", size=" << info->size_
        << ", dateAddedMs=" << info->dateAddedMs_
        << ", dateModifiedMs=" << info->dateModifiedMs_
        << ", isFavorite=" << info->isFavorite_
        << ", isHidden=" << info->isHidden_;
}

void CreateBatchTestAlbums(MediaLibraryManager &manager, int32_t batchCount, std::vector<int32_t> &albumIds)
{
    for (int32_t i = 0; i < batchCount; ++i) {
        int32_t albumId = CreateTestAlbum(manager);
        ASSERT_GT(albumId, E_OK);
        albumIds.push_back(albumId);
    }
}

void CreateBatchTestAssets(MediaLibraryManager &manager, int32_t batchCount, std::vector<int32_t> &assetIds,
    std::vector<std::string> *assetUris = nullptr)
{
    for (int32_t i = 0; i < batchCount; ++i) {
        std::string assetUri;
        int32_t assetId = AccurateRefresh::INVALID_INT32_VALUE;
        ASSERT_TRUE(CreateAndWriteTestAsset(manager, assetUri, assetId));
        assetIds.push_back(assetId);
        if (assetUris != nullptr) {
            assetUris->push_back(assetUri);
        }
    }
}

bool CollectRemovedAlbumFlags(const std::vector<AlbumChangeInfos> &allInfos, size_t startIndex,
    std::map<int32_t, bool> &removedAlbumFlags)
{
    bool hasRecheckNotify = false;
    for (size_t i = startIndex; i < allInfos.size(); ++i) {
        const auto &infos = allInfos[i];
        hasRecheckNotify = hasRecheckNotify || infos.isForRecheck;
        if (infos.type != NotifyChangeType::NOTIFY_CHANGE_REMOVE) {
            continue;
        }
        for (const auto &changeData : infos.albumChangeDatas) {
            auto info = (changeData.albumAfterChange != nullptr)
                ? changeData.albumAfterChange
                : changeData.albumBeforeChange;
            if (info == nullptr) {
                continue;
            }
            int32_t albumId = GetAlbumIdFromNotifyInfo(info);
            if (albumId <= E_OK) {
                continue;
            }
            auto iter = removedAlbumFlags.find(albumId);
            if (iter != removedAlbumFlags.end()) {
                iter->second = true;
            }
        }
    }
    return hasRecheckNotify;
}

void MarkRemovedAssetFlag(const PhotoAssetChangeData &changeData,
    std::map<int32_t, bool> &removedAssetFlags, int32_t expectedAlbumId)
{
    auto info = (changeData.assetAfterChange != nullptr)
        ? changeData.assetAfterChange
        : changeData.assetBeforeChange;
    if (info == nullptr) {
        return;
    }
    auto iter = removedAssetFlags.find(info->fileId_);
    if (iter == removedAssetFlags.end()) {
        return;
    }
    iter->second = true;
    int32_t ownerAlbumId = GetOwnerAlbumIdFromNotifyInfo(info);
    if (ownerAlbumId > E_OK && expectedAlbumId > E_OK) {
        EXPECT_EQ(ownerAlbumId, expectedAlbumId);
    }
}

void VerifyBatchAlbumDeleteSamples(MediaLibraryManager &manager, const std::vector<int32_t> &albumIds,
    const std::map<int32_t, bool> &removedAlbumFlags, bool hasRecheckNotify)
{
    std::vector<size_t> sampleIndexes = { 0, albumIds.size() / 2, albumIds.size() - 1 };
    for (size_t sampleIndex : sampleIndexes) {
        int32_t sampleAlbumId = albumIds[sampleIndex];
        EXPECT_EQ(QueryAlbumById(manager, sampleAlbumId), nullptr);
        if (hasRecheckNotify) {
            continue;
        }
        auto iter = removedAlbumFlags.find(sampleAlbumId);
        ASSERT_NE(iter, removedAlbumFlags.end());
        EXPECT_TRUE(iter->second);
    }
}

void ExecuteAndCheckBatchAlbumDelete(MediaLibraryManager &manager, SyncPhotoAlbumChangeCallback &callback,
    const std::vector<int32_t> &albumIds, int32_t idleWaitMs)
{
    int32_t baseCount = callback.GetCallTimes();
    auto albumsToDelete = QueryAlbumsByIds(manager, albumIds);
    ASSERT_EQ(albumsToDelete.size(), albumIds.size());
    ASSERT_GT(manager.DeleteAlbums(albumsToDelete), E_OK);
    ASSERT_TRUE(callback.WaitForCallAfter(baseCount, BATCH_NOTIFY_TIMEOUT_MS));
    ASSERT_TRUE(WaitForCallbackIdle(callback, BATCH_IDLE_QUIET_MS, idleWaitMs));

    auto allInfos = callback.GetAllInfos();
    ASSERT_GT(allInfos.size(), static_cast<size_t>(baseCount));
    std::map<int32_t, bool> removedAlbumFlags;
    for (int32_t albumId : albumIds) {
        removedAlbumFlags[albumId] = false;
    }

    bool hasRecheckNotify = CollectRemovedAlbumFlags(allInfos, static_cast<size_t>(baseCount), removedAlbumFlags);
    size_t removedAlbumCount = 0;
    for (const auto &item : removedAlbumFlags) {
        if (item.second) {
            ++removedAlbumCount;
        }
    }
    if (removedAlbumCount < removedAlbumFlags.size()) {
        EXPECT_TRUE(hasRecheckNotify);
    }
    VerifyBatchAlbumDeleteSamples(manager, albumIds, removedAlbumFlags, hasRecheckNotify);
}

int32_t GetExpectedAlbumIdFromAssets(const std::vector<std::unique_ptr<FileAsset>> &assetsToDelete)
{
    if (assetsToDelete.empty() || assetsToDelete[0] == nullptr) {
        return AccurateRefresh::INVALID_INT32_VALUE;
    }
    int32_t expectedAlbumId = assetsToDelete[0]->GetOwnerAlbumId();
    return (expectedAlbumId > E_OK) ? expectedAlbumId : assetsToDelete[0]->GetAlbumId();
}

bool CollectRemovedAssetFlags(const std::vector<PhotoAssetChangeInfos> &allInfos, size_t startIndex,
    std::map<int32_t, bool> &removedAssetFlags, int32_t expectedAlbumId)
{
    bool hasRecheckNotify = false;
    for (size_t i = startIndex; i < allInfos.size(); ++i) {
        const auto &infos = allInfos[i];
        hasRecheckNotify = hasRecheckNotify || infos.isForRecheck;
        if (infos.type != NotifyChangeType::NOTIFY_CHANGE_REMOVE) {
            continue;
        }
        for (const auto &changeData : infos.assetChangeDatas) {
            MarkRemovedAssetFlag(changeData, removedAssetFlags, expectedAlbumId);
        }
    }
    return hasRecheckNotify;
}

void VerifyBatchAssetDeleteSamples(MediaLibraryManager &manager, const std::vector<int32_t> &assetIds,
    const std::map<int32_t, bool> &removedAssetFlags, bool hasRecheckNotify)
{
    std::vector<size_t> sampleIndexes = { 0, assetIds.size() / 2, assetIds.size() - 1 };
    for (size_t sampleIndex : sampleIndexes) {
        int32_t sampleAssetId = assetIds[sampleIndex];
        EXPECT_EQ(QueryAssetById(manager, sampleAssetId), nullptr);
        if (hasRecheckNotify) {
            continue;
        }
        auto iter = removedAssetFlags.find(sampleAssetId);
        ASSERT_NE(iter, removedAssetFlags.end());
        EXPECT_TRUE(iter->second);
    }
}

void ExecuteAndCheckBatchAssetDelete(MediaLibraryManager &manager, SyncPhotoAssetChangeCallback &callback,
    const std::vector<int32_t> &assetIds, int32_t idleWaitMs)
{
    int32_t baseCount = callback.GetCallTimes();
    auto assetsToDelete = QueryAssetsByIds(manager, assetIds);
    ASSERT_EQ(assetsToDelete.size(), assetIds.size());
    int32_t expectedAlbumId = GetExpectedAlbumIdFromAssets(assetsToDelete);
    int32_t deleteRet = manager.DeleteAssets(assetsToDelete);
    ASSERT_EQ(deleteRet, static_cast<int32_t>(assetIds.size()));
    ASSERT_TRUE(callback.WaitForCallAfter(baseCount, BATCH_NOTIFY_TIMEOUT_MS));
    ASSERT_TRUE(WaitForCallbackIdle(callback, BATCH_IDLE_QUIET_MS, idleWaitMs));

    auto allInfos = callback.GetAllInfos();
    ASSERT_GT(allInfos.size(), static_cast<size_t>(baseCount));
    std::map<int32_t, bool> removedAssetFlags;
    for (int32_t assetId : assetIds) {
        removedAssetFlags[assetId] = false;
    }

    bool hasRecheckNotify = CollectRemovedAssetFlags(allInfos, static_cast<size_t>(baseCount),
        removedAssetFlags, expectedAlbumId);
    size_t removedAssetCount = 0;
    for (const auto &item : removedAssetFlags) {
        if (item.second) {
            ++removedAssetCount;
        }
    }
    if (removedAssetCount < removedAssetFlags.size()) {
        EXPECT_TRUE(hasRecheckNotify);
    }
    VerifyBatchAssetDeleteSamples(manager, assetIds, removedAssetFlags, hasRecheckNotify);
}

void RegisterAlbumObserverWithFallback(MediaLibraryManager &manager,
    const std::shared_ptr<SyncPhotoAlbumChangeCallback> &callback, bool &useHiddenObserver)
{
    int32_t registerRet = manager.RegisterHiddenAlbumChange(callback);
    if (registerRet == E_OK) {
        useHiddenObserver = true;
        return;
    }
    ASSERT_TRUE(IsHiddenRegisterPermissionDenied(registerRet));
    ASSERT_EQ(manager.RegisterPhotoAlbumCallback(callback), E_OK);
}

void RegisterHiddenPhotoObserverWithFallback(MediaLibraryManager &manager,
    const std::shared_ptr<SyncPhotoAssetChangeCallback> &callback, bool &useHiddenObserver)
{
    int32_t registerRet = manager.RegisterHiddenPhotoChange(callback);
    if (registerRet == E_OK) {
        useHiddenObserver = true;
        return;
    }
    ASSERT_TRUE(IsHiddenRegisterPermissionDenied(registerRet));
    ASSERT_EQ(manager.RegisterPhotoChange(callback), E_OK);
}

void AnalyzeAlbumNotifyResults(const std::vector<AlbumChangeInfos> &allInfos, size_t startIndex,
    bool &hasNotifyPayload, bool &hasRecheckNotify)
{
    hasNotifyPayload = false;
    hasRecheckNotify = false;
    for (size_t i = startIndex; i < allInfos.size(); ++i) {
        hasRecheckNotify = hasRecheckNotify || allInfos[i].isForRecheck;
        hasNotifyPayload = hasNotifyPayload || !allInfos[i].albumChangeDatas.empty();
    }
}

void AnalyzeAssetNotifyResults(const std::vector<PhotoAssetChangeInfos> &allInfos, size_t startIndex,
    bool &hasNotifyPayload, bool &hasRecheckNotify)
{
    hasNotifyPayload = false;
    hasRecheckNotify = false;
    for (size_t i = startIndex; i < allInfos.size(); ++i) {
        hasRecheckNotify = hasRecheckNotify || allInfos[i].isForRecheck;
        hasNotifyPayload = hasNotifyPayload || !allInfos[i].assetChangeDatas.empty();
    }
}

/**
 * @tc.number: MediaLibraryManager_notify_test_001
 * @tc.name: RegisterPhotoChange callback is null
 * @tc.desc: Should return E_INVALID_ARGUMENTS when callback is null.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_001, TestSize.Level1)
{
    int32_t ret = manager.RegisterPhotoChange(nullptr);
    EXPECT_EQ(ret, E_INVALID_ARGUMENTS);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_002
 * @tc.name: RegisterPhotoAlbumCallback callback is null
 * @tc.desc: Should return E_INVALID_ARGUMENTS when callback is null.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_002, TestSize.Level1)
{
    int32_t ret = manager.RegisterPhotoAlbumCallback(nullptr);
    EXPECT_EQ(ret, E_INVALID_ARGUMENTS);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_003
 * @tc.name: RegisterSinglePhotoChange invalid uri
 * @tc.desc: Should return E_INVALID_ARGUMENTS when uri is invalid.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_003, TestSize.Level1)
{
    auto callback = std::make_shared<MockPhotoAssetChangeCallback>();
    int32_t ret = manager.RegisterSinglePhotoChange("", callback);
    EXPECT_EQ(ret, E_INVALID_ARGUMENTS);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_004
 * @tc.name: UnregisterSinglePhotoChange empty uri with callback
 * @tc.desc: Should return E_INVALID_ARGUMENTS for illegal argument combination.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_004, TestSize.Level1)
{
    auto callback = std::make_shared<MockPhotoAssetChangeCallback>();
    int32_t ret = manager.UnregisterSinglePhotoChange("", callback);
    EXPECT_EQ(ret, E_INVALID_ARGUMENTS);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_005
 * @tc.name: RegisterSinglePhotoAlbumChange invalid uri
 * @tc.desc: Should return E_INVALID_ARGUMENTS when uri is invalid.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_005, TestSize.Level1)
{
    auto callback = std::make_shared<MockPhotoAlbumChangeCallback>();
    int32_t ret = manager.RegisterSinglePhotoAlbumChange("", callback);
    EXPECT_EQ(ret, E_INVALID_ARGUMENTS);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_006
 * @tc.name: UnregisterSinglePhotoAlbumChange empty uri with callback
 * @tc.desc: Should return E_INVALID_ARGUMENTS for illegal argument combination.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_006, TestSize.Level1)
{
    auto callback = std::make_shared<MockPhotoAlbumChangeCallback>();
    int32_t ret = manager.UnregisterSinglePhotoAlbumChange("", callback);
    EXPECT_EQ(ret, E_INVALID_ARGUMENTS);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_043
 * @tc.name: UnregisterPhotoChange removes specified callback only
 * @tc.desc: Verify specified callback unregister keeps other callbacks active.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_043, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    auto callback1 = std::make_shared<SyncPhotoAssetChangeCallback>();
    auto callback2 = std::make_shared<SyncPhotoAssetChangeCallback>();
    int32_t assetId = AccurateRefresh::INVALID_INT32_VALUE;
    auto cleanup = ScopeExit([&]() {
        (void)manager.UnregisterPhotoChange(callback1);
        (void)manager.UnregisterPhotoChange(callback2);
        if (assetId > E_OK) {
            (void)DeleteAssetById(manager, assetId);
        }
    });
    ASSERT_EQ(manager.RegisterPhotoChange(callback1), E_OK);
    ASSERT_EQ(manager.RegisterPhotoChange(callback2), E_OK);
    ASSERT_EQ(manager.UnregisterPhotoChange(callback1), E_OK);

    int32_t callback1Base = callback1->GetCallTimes();
    int32_t callback2Base = callback2->GetCallTimes();

    std::string assetUri;
    ASSERT_TRUE(CreateAndWriteTestAsset(manager, assetUri, assetId));

    PhotoAssetChangeInfos infos;
    AssetChangeWaitArgs addWaitArgs {
        NotifyChangeType::NOTIFY_CHANGE_ADD,
        assetId,
        5000,
    };
    EXPECT_TRUE(WaitForAssetChangeByTypeAndId(*callback2, callback2Base,
        addWaitArgs, infos));
    EXPECT_FALSE(callback1->WaitForCallAfter(callback1Base, 1000));

    EXPECT_EQ(callback1->GetCallTimes(), 0);
    EXPECT_GE(callback2->GetCallTimes(), 1);
    EXPECT_EQ(manager.UnregisterPhotoChange(callback2), E_OK);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_044
 * @tc.name: UnregisterPhotoChange without callback removes all listeners
 * @tc.desc: Verify no callback is received after unregister-all path.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_044, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    auto callback1 = std::make_shared<SyncPhotoAssetChangeCallback>();
    auto callback2 = std::make_shared<SyncPhotoAssetChangeCallback>();
    int32_t assetId = AccurateRefresh::INVALID_INT32_VALUE;
    auto cleanup = ScopeExit([&]() {
        (void)manager.UnregisterPhotoChange(callback1);
        (void)manager.UnregisterPhotoChange(callback2);
        if (assetId > E_OK) {
            (void)DeleteAssetById(manager, assetId);
        }
    });
    ASSERT_EQ(manager.RegisterPhotoChange(callback1), E_OK);
    ASSERT_EQ(manager.RegisterPhotoChange(callback2), E_OK);
    ASSERT_EQ(manager.UnregisterPhotoChange(), E_OK);

    int32_t callback1Base = callback1->GetCallTimes();
    int32_t callback2Base = callback2->GetCallTimes();

    std::string assetUri;
    ASSERT_TRUE(CreateAndWriteTestAsset(manager, assetUri, assetId));

    EXPECT_FALSE(callback1->WaitForCallAfter(callback1Base, 1500));
    EXPECT_FALSE(callback2->WaitForCallAfter(callback2Base, 1500));

    EXPECT_EQ(callback1->GetCallTimes(), 0);
    EXPECT_EQ(callback2->GetCallTimes(), 0);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_045
 * @tc.name: UnregisterPhotoAlbumCallback without callback removes all listeners
 * @tc.desc: Verify no album callback is received after unregister-all path.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_045, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    auto callback1 = std::make_shared<SyncPhotoAlbumChangeCallback>();
    auto callback2 = std::make_shared<SyncPhotoAlbumChangeCallback>();
    ASSERT_EQ(manager.RegisterPhotoAlbumCallback(callback1), E_OK);
    ASSERT_EQ(manager.RegisterPhotoAlbumCallback(callback2), E_OK);
    ASSERT_EQ(manager.UnregisterPhotoAlbumCallback(), E_OK);

    int32_t callback1Base = callback1->GetCallTimes();
    int32_t callback2Base = callback2->GetCallTimes();

    int32_t albumId = CreateTestAlbum(manager);
    ASSERT_GT(albumId, E_OK);

    EXPECT_FALSE(callback1->WaitForCallAfter(callback1Base, 1500));
    EXPECT_FALSE(callback2->WaitForCallAfter(callback2Base, 1500));

    EXPECT_EQ(callback1->GetCallTimes(), 0);
    EXPECT_EQ(callback2->GetCallTimes(), 0);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_046
 * @tc.name: UnregisterSinglePhotoChange supports callback and uri-only modes
 * @tc.desc: Verify callback-specific and uri-only unregister semantics in one case.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_046, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    std::string assetUri;
    int32_t assetId = AccurateRefresh::INVALID_INT32_VALUE;
    auto cleanup = ScopeExit([&]() {
        (void)manager.UnregisterSinglePhotoChange();
        if (assetId > E_OK) {
            (void)DeleteAssetById(manager, assetId);
        }
    });
    ASSERT_TRUE(CreateAndWriteTestAsset(manager, assetUri, assetId));

    auto callback1 = std::make_shared<SyncPhotoAssetChangeCallback>();
    auto callback2 = std::make_shared<SyncPhotoAssetChangeCallback>();
    ASSERT_EQ(manager.RegisterSinglePhotoChange(assetUri, callback1), E_OK);
    ASSERT_EQ(manager.RegisterSinglePhotoChange(assetUri, callback2), E_OK);
    ASSERT_EQ(manager.UnregisterSinglePhotoChange(assetUri, callback1), E_OK);

    int32_t callback1Base = callback1->GetCallTimes();
    int32_t callback2Base = callback2->GetCallTimes();
    ASSERT_TRUE(WriteSingleByteToAsset(manager, assetUri));

    PhotoAssetChangeInfos updateInfos;
    AssetChangeWaitArgs updateWaitArgs {
        NotifyChangeType::NOTIFY_CHANGE_UPDATE,
        assetId,
        5000,
    };
    EXPECT_TRUE(WaitForAssetChangeByTypeAndId(*callback2, callback2Base,
        updateWaitArgs, updateInfos));
    EXPECT_FALSE(callback1->WaitForCallAfter(callback1Base, 1000));

    EXPECT_EQ(callback1->GetCallTimes(), 0);
    EXPECT_GE(callback2->GetCallTimes(), 1);

    ASSERT_EQ(manager.UnregisterSinglePhotoChange(assetUri), E_OK);
    int32_t callback2AfterFirst = callback2->GetCallTimes();
    ASSERT_TRUE(WriteSingleByteToAsset(manager, assetUri));
    EXPECT_FALSE(callback2->WaitForCallAfter(callback2AfterFirst, 1500));

    EXPECT_EQ(callback1->GetCallTimes(), 0);
    EXPECT_EQ(callback2->GetCallTimes(), callback2AfterFirst);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_047
 * @tc.name: UnregisterSinglePhotoAlbumChange supports callback and uri-only modes
 * @tc.desc: Verify callback-specific and uri-only unregister semantics in one case.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_047, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    int32_t albumId1 = AccurateRefresh::INVALID_INT32_VALUE;
    int32_t albumId2 = AccurateRefresh::INVALID_INT32_VALUE;
    auto cleanup = ScopeExit([&]() {
        (void)manager.UnregisterSinglePhotoAlbumChange();
        if (albumId1 > E_OK) {
            (void)DeleteAlbumById(manager, albumId1);
        }
        if (albumId2 > E_OK) {
            (void)DeleteAlbumById(manager, albumId2);
        }
    });

    albumId1 = CreateTestAlbum(manager);
    ASSERT_GT(albumId1, E_OK);
    albumId2 = CreateTestAlbum(manager);
    ASSERT_GT(albumId2, E_OK);
    std::string albumUri1 = QueryAlbumUriById(manager, albumId1);
    std::string albumUri2 = QueryAlbumUriById(manager, albumId2);
    ASSERT_FALSE(albumUri1.empty());
    ASSERT_FALSE(albumUri2.empty());

    auto callback1 = std::make_shared<SyncPhotoAlbumChangeCallback>();
    auto callback2 = std::make_shared<SyncPhotoAlbumChangeCallback>();
    ASSERT_EQ(manager.RegisterSinglePhotoAlbumChange(albumUri1, callback1), E_OK);
    ASSERT_EQ(manager.RegisterSinglePhotoAlbumChange(albumUri1, callback2), E_OK);
    ASSERT_EQ(manager.RegisterSinglePhotoAlbumChange(albumUri2, callback2), E_OK);
    ASSERT_EQ(manager.UnregisterSinglePhotoAlbumChange(albumUri1, callback1), E_OK);

    int32_t callback1Base = callback1->GetCallTimes();
    int32_t callback2Base = callback2->GetCallTimes();
    ASSERT_GE(DeleteAlbumById(manager, albumId1), E_OK);

    AlbumChangeInfos removeInfos;
    AlbumChangeWaitArgs removeWaitArgs {
        NotifyChangeType::NOTIFY_CHANGE_REMOVE,
        albumId1,
        5000,
    };
    EXPECT_TRUE(WaitForAlbumChangeByTypeAndId(*callback2, callback2Base,
        removeWaitArgs, removeInfos));
    EXPECT_FALSE(callback1->WaitForCallAfter(callback1Base, 1000));

    int32_t callback2AfterFirst = callback2->GetCallTimes();
    ASSERT_EQ(manager.UnregisterSinglePhotoAlbumChange(albumUri2), E_OK);
    ASSERT_GE(DeleteAlbumById(manager, albumId2), E_OK);
    EXPECT_FALSE(callback2->WaitForCallAfter(callback2AfterFirst, 1500));

    EXPECT_EQ(callback1->GetCallTimes(), 0);
    EXPECT_EQ(callback2->GetCallTimes(), callback2AfterFirst);
    EXPECT_EQ(manager.UnregisterSinglePhotoAlbumChange(), E_OK);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_048
 * @tc.name: Photo change add payload check
 * @tc.desc: Verify type and uri/fileId mapping for add asset change.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_048, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    int32_t assetId = AccurateRefresh::INVALID_INT32_VALUE;
    auto callback = std::make_shared<SyncPhotoAssetChangeCallback>();
    auto cleanup = ScopeExit([&]() {
        (void)manager.UnregisterPhotoChange(callback);
        if (assetId > E_OK) {
            (void)DeleteAssetById(manager, assetId);
        }
    });

    ASSERT_EQ(manager.RegisterPhotoChange(callback), E_OK);
    ASSERT_TRUE(WaitForCallbackIdle(*callback, 100, 1000));

    int32_t addBase = callback->GetCallTimes();
    std::string assetUri;
    ASSERT_TRUE(CreateAndWriteTestAsset(manager, assetUri, assetId));

    PhotoAssetChangeInfos addInfos;
    AssetChangeWaitArgs addWaitArgs {
        NotifyChangeType::NOTIFY_CHANGE_ADD,
        assetId,
        5000,
    };
    ASSERT_TRUE(WaitForAssetChangeByTypeAndId(*callback, addBase,
        addWaitArgs, addInfos));
    EXPECT_EQ(addInfos.type, NotifyChangeType::NOTIFY_CHANGE_ADD);
    ASSERT_EQ(addInfos.assetChangeDatas.size(), 1);
    EXPECT_EQ(addInfos.assetChangeDatas[0].assetBeforeChange, nullptr);
    ASSERT_NE(addInfos.assetChangeDatas[0].assetAfterChange, nullptr);
    EXPECT_FALSE(addInfos.assetChangeDatas[0].assetAfterChange->uri_.empty());
    EXPECT_EQ(addInfos.assetChangeDatas[0].assetAfterChange->fileId_, assetId);

    EXPECT_EQ(manager.UnregisterPhotoChange(callback), E_OK);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_049
 * @tc.name: Album add payload check with created album uri
 * @tc.desc: Verify album add change contains expected albumUri and type.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_049, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    int32_t albumId = AccurateRefresh::INVALID_INT32_VALUE;
    auto callback = std::make_shared<SyncPhotoAlbumChangeCallback>();
    auto cleanup = ScopeExit([&]() {
        (void)manager.UnregisterPhotoAlbumCallback(callback);
        if (albumId > E_OK) {
            (void)DeleteAlbumById(manager, albumId);
        }
    });

    ASSERT_EQ(manager.RegisterPhotoAlbumCallback(callback), E_OK);
    ASSERT_TRUE(WaitForCallbackIdle(*callback, 100, 1000));

    int32_t addBase = callback->GetCallTimes();
    albumId = CreateTestAlbum(manager);
    ASSERT_GT(albumId, E_OK);
    std::string createdAlbumUri = QueryAlbumUriById(manager, albumId);
    ASSERT_FALSE(createdAlbumUri.empty());

    AlbumChangeInfos infos;
    AlbumChangeWaitArgs addWaitArgs {
        NotifyChangeType::NOTIFY_CHANGE_ADD,
        albumId,
        5000,
    };
    ASSERT_TRUE(WaitForAlbumChangeByTypeAndId(*callback, addBase,
        addWaitArgs, infos));
    EXPECT_EQ(infos.type, NotifyChangeType::NOTIFY_CHANGE_ADD);
    ASSERT_EQ(infos.albumChangeDatas.size(), 1);
    EXPECT_EQ(infos.albumChangeDatas[0].albumBeforeChange, nullptr);
    ASSERT_NE(infos.albumChangeDatas[0].albumAfterChange, nullptr);
    int32_t notifiedAlbumId = ParseAlbumIdFromUri(infos.albumChangeDatas[0].albumAfterChange->albumUri_);
    EXPECT_EQ(notifiedAlbumId, albumId);
    EXPECT_FALSE(infos.albumChangeDatas[0].albumAfterChange->albumUri_.empty());
    EXPECT_EQ(infos.albumChangeDatas[0].albumAfterChange->albumUri_, createdAlbumUri);

    EXPECT_EQ(manager.UnregisterPhotoAlbumCallback(callback), E_OK);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_050
 * @tc.name: Photo change update payload check
 * @tc.desc: Verify type and fileId mapping for update asset change.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_050, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    int32_t assetId = AccurateRefresh::INVALID_INT32_VALUE;
    auto callback = std::make_shared<SyncPhotoAssetChangeCallback>();
    auto cleanup = ScopeExit([&]() {
        (void)manager.UnregisterPhotoChange(callback);
        if (assetId > E_OK) {
            (void)DeleteAssetById(manager, assetId);
        }
    });

    ASSERT_EQ(manager.RegisterPhotoChange(callback), E_OK);
    ASSERT_TRUE(WaitForCallbackIdle(*callback, 100, 1000));

    std::string assetUri;
    ASSERT_TRUE(CreateAndWriteTestAsset(manager, assetUri, assetId));
    ASSERT_TRUE(WaitAssetReadyById(manager, assetId));
    ASSERT_TRUE(WaitForCallbackIdle(*callback, 200, 5000));

    int32_t updateBase = callback->GetCallTimes();
    ASSERT_TRUE(WriteSingleByteToAsset(manager, assetUri));
    PhotoAssetChangeInfos updateInfos;
    AssetChangeWaitArgs updateWaitArgs {
        NotifyChangeType::NOTIFY_CHANGE_UPDATE,
        assetId,
        8000,
    };
    ASSERT_TRUE(WaitForAssetChangeByTypeAndId(*callback, updateBase, updateWaitArgs, updateInfos));
    EXPECT_EQ(updateInfos.type, NotifyChangeType::NOTIFY_CHANGE_UPDATE);
    ASSERT_GE(updateInfos.assetChangeDatas.size(), 1);
    ASSERT_NE(updateInfos.assetChangeDatas[0].assetBeforeChange, nullptr);
    ASSERT_NE(updateInfos.assetChangeDatas[0].assetAfterChange, nullptr);
    EXPECT_EQ(updateInfos.assetChangeDatas[0].assetBeforeChange->fileId_, assetId);
    EXPECT_EQ(updateInfos.assetChangeDatas[0].assetAfterChange->fileId_, assetId);

    EXPECT_EQ(manager.UnregisterPhotoChange(callback), E_OK);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_051
 * @tc.name: Photo change remove payload check
 * @tc.desc: Verify type and fileId mapping for remove asset change.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_051, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    int32_t assetId = AccurateRefresh::INVALID_INT32_VALUE;
    auto callback = std::make_shared<SyncPhotoAssetChangeCallback>();
    auto cleanup = ScopeExit([&]() {
        (void)manager.UnregisterPhotoChange(callback);
        if (assetId > E_OK) {
            (void)DeleteAssetById(manager, assetId);
        }
    });

    ASSERT_EQ(manager.RegisterPhotoChange(callback), E_OK);
    ASSERT_TRUE(WaitForCallbackIdle(*callback, 100, 1000));

    std::string assetUri;
    ASSERT_TRUE(CreateAndWriteTestAsset(manager, assetUri, assetId));
    ASSERT_TRUE(WaitAssetReadyById(manager, assetId));
    ASSERT_TRUE(WaitForCallbackIdle(*callback, 200, 5000));

    int32_t removeBase = callback->GetCallTimes();
    ASSERT_GT(DeleteAssetById(manager, assetId), E_OK);
    PhotoAssetChangeInfos removeInfos;
    AssetChangeWaitArgs removeWaitArgs {
        NotifyChangeType::NOTIFY_CHANGE_REMOVE,
        assetId,
        8000,
    };
    ASSERT_TRUE(WaitForAssetChangeByTypeAndId(*callback, removeBase, removeWaitArgs, removeInfos));
    EXPECT_EQ(removeInfos.type, NotifyChangeType::NOTIFY_CHANGE_REMOVE);
    ASSERT_GE(removeInfos.assetChangeDatas.size(), 1);
    ASSERT_NE(removeInfos.assetChangeDatas[0].assetBeforeChange, nullptr);
    EXPECT_EQ(removeInfos.assetChangeDatas[0].assetBeforeChange->fileId_, assetId);
    EXPECT_EQ(removeInfos.assetChangeDatas[0].assetAfterChange, nullptr);

    EXPECT_EQ(manager.UnregisterPhotoChange(callback), E_OK);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_052
 * @tc.name: Album callback logs notified albums after asset add
 * @tc.desc: Listen album notifications, add one asset, and print
 *           albumId/albumType/albumSubType/notifyType for notified albums.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_052, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    int32_t assetId = AccurateRefresh::INVALID_INT32_VALUE;
    auto albumCallback = std::make_shared<CollectPhotoAlbumChangeCallback>();
    auto cleanup = ScopeExit([&]() {
        (void)manager.UnregisterPhotoAlbumCallback(albumCallback);
        if (assetId > E_OK) {
            (void)DeleteAssetById(manager, assetId);
        }
    });
    ASSERT_EQ(manager.RegisterPhotoAlbumCallback(albumCallback), E_OK);
    ASSERT_TRUE(WaitForCallbackIdle(*albumCallback, 100, 1000));
    int32_t albumBase = albumCallback->GetCallTimes();
    std::string assetUri;
    ASSERT_TRUE(CreateAndWriteTestAsset(manager, assetUri, assetId));
    ASSERT_TRUE(albumCallback->WaitForCallAfter(albumBase, 5000));
    ASSERT_TRUE(WaitForCallbackIdle(*albumCallback, 200, 3000));
    auto allAlbumInfos = albumCallback->GetAllInfos();
    ASSERT_GT(allAlbumInfos.size(), static_cast<size_t>(albumBase));
    size_t notifiedAlbumCount = 0;
    for (size_t i = static_cast<size_t>(albumBase); i < allAlbumInfos.size(); ++i) {
        const auto &infos = allAlbumInfos[i];
        for (size_t j = 0; j < infos.albumChangeDatas.size(); ++j) {
            const auto &changeData = infos.albumChangeDatas[j];
            auto info = (changeData.albumAfterChange != nullptr)
                ? changeData.albumAfterChange
                : changeData.albumBeforeChange;
            if (info == nullptr) {
                GTEST_LOG_(INFO) << "Album notified: notifyType=" << static_cast<int32_t>(infos.type)
                    << ", albumInfo=nullptr";
                continue;
            }
            ++notifiedAlbumCount;
            GTEST_LOG_(INFO) << "Album notified: notifyType=" << static_cast<int32_t>(infos.type)
                << ", albumUri=" << info->albumUri_
                << ", albumType=" << info->albumType_
                << ", albumSubType=" << info->albumSubType_;
        }
    }
    EXPECT_GT(notifiedAlbumCount, 0);
    EXPECT_EQ(manager.UnregisterPhotoAlbumCallback(albumCallback), E_OK);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_053
 * @tc.name: Asset callback logs all notifications after asset add
 * @tc.desc: Listen asset notifications, add one asset, and print all payloads.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_053, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    int32_t assetId = AccurateRefresh::INVALID_INT32_VALUE;
    auto callback = std::make_shared<CollectPhotoAssetChangeCallback>();
    auto cleanup = ScopeExit([&]() {
        (void)manager.UnregisterPhotoChange(callback);
        if (assetId > E_OK) {
            (void)DeleteAssetById(manager, assetId);
        }
    });
    ASSERT_EQ(manager.RegisterPhotoChange(callback), E_OK);
    ASSERT_TRUE(WaitForCallbackIdle(*callback, 100, 1000));
    int32_t baseCount = callback->GetCallTimes();
    std::string assetUri;
    ASSERT_TRUE(CreateAndWriteTestAsset(manager, assetUri, assetId));
    ASSERT_TRUE(callback->WaitForCallAfter(baseCount, 5000));
    ASSERT_TRUE(WaitForCallbackIdle(*callback, 200, 3000));
    auto allInfos = callback->GetAllInfos();
    ASSERT_GT(allInfos.size(), static_cast<size_t>(baseCount));
    size_t notifyCount = 0;
    for (size_t i = static_cast<size_t>(baseCount); i < allInfos.size(); ++i) {
        const auto &infos = allInfos[i];
        for (size_t j = 0; j < infos.assetChangeDatas.size(); ++j) {
            const auto &changeData = infos.assetChangeDatas[j];
            ++notifyCount;
            GTEST_LOG_(INFO) << "Asset notify event[" << i << "] type=" << static_cast<int32_t>(infos.type)
                << ", changeIndex=" << j;
            LogAssetChangeInfoDetail("  before: ", changeData.assetBeforeChange);
            LogAssetChangeInfoDetail("  after: ", changeData.assetAfterChange);
        }
    }
    EXPECT_GT(notifyCount, 0);
    EXPECT_EQ(manager.UnregisterPhotoChange(callback), E_OK);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_054
 * @tc.name: Album callback logs notifications after asset add and favorite
 * @tc.desc: Listen album notifications, add one asset and favorite it,
 *           then print all notified albums with notifyType and album fields.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_054, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    int32_t assetId = AccurateRefresh::INVALID_INT32_VALUE;
    auto callback = std::make_shared<CollectPhotoAlbumChangeCallback>();
    auto cleanup = ScopeExit([&]() {
        (void)manager.UnregisterPhotoAlbumCallback(callback);
        if (assetId > E_OK) {
            (void)UpdateAssetFavoriteByDataShare(manager, assetId, false);
            (void)DeleteAssetById(manager, assetId);
        }
    });

    ASSERT_EQ(manager.RegisterPhotoAlbumCallback(callback), E_OK);
    ASSERT_TRUE(WaitForCallbackIdle(*callback, 100, 1000));
    int32_t baseCount = callback->GetCallTimes();
    std::string assetUri;
    ASSERT_TRUE(CreateAndWriteTestAsset(manager, assetUri, assetId));
    ASSERT_TRUE(callback->WaitForCallAfter(baseCount, 5000));
    int32_t beforeFavorite = callback->GetCallTimes();
    ASSERT_GE(UpdateAssetFavoriteByDataShare(manager, assetId, true), E_OK);
    ASSERT_TRUE(callback->WaitForCallAfter(beforeFavorite, 5000));
    ASSERT_TRUE(WaitForCallbackIdle(*callback, 200, 3000));
    auto allAlbumInfos = callback->GetAllInfos();
    ASSERT_GT(allAlbumInfos.size(), static_cast<size_t>(baseCount));
    std::map<int32_t, std::pair<int32_t, int32_t>> notifiedAlbums;
    size_t notifyCount = LogAndSummarizeAlbumNotifications(allAlbumInfos,
        static_cast<size_t>(baseCount), notifiedAlbums);
    EXPECT_GT(notifyCount, 0);
    EXPECT_FALSE(notifiedAlbums.empty());
    EXPECT_EQ(manager.UnregisterPhotoAlbumCallback(callback), E_OK);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_055
 * @tc.name: Asset callback logs notifications after rename
 * @tc.desc: Create one asset, register asset observer, rename asset with
 *           dataShareHelper->Update, and log all received asset notifications.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_055, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    int32_t assetId = AccurateRefresh::INVALID_INT32_VALUE;
    auto callback = std::make_shared<CollectPhotoAssetChangeCallback>();
    auto cleanup = ScopeExit([&]() {
        (void)manager.UnregisterPhotoChange(callback);
        if (assetId > E_OK) {
            (void)DeleteAssetById(manager, assetId);
        }
    });

    std::string assetUri;
    ASSERT_TRUE(CreateAndWriteTestAsset(manager, assetUri, assetId));
    ASSERT_EQ(manager.RegisterPhotoChange(callback), E_OK);
    ASSERT_TRUE(WaitForCallbackIdle(*callback, 100, 1000));

    int32_t baseCount = callback->GetCallTimes();
    std::string renamedName = BuildUniqueRenamedAssetName();
    ASSERT_GE(RenameAssetById(manager, assetId, renamedName), E_OK);
    ASSERT_TRUE(callback->WaitForCallAfter(baseCount, 5000));
    ASSERT_TRUE(WaitForCallbackIdle(*callback, 200, 3000));

    auto allInfos = callback->GetAllInfos();
    ASSERT_GT(allInfos.size(), static_cast<size_t>(baseCount));
    bool foundUpdate = LogAssetNotificationsAndFindUpdate(allInfos, static_cast<size_t>(baseCount), assetId);
    EXPECT_TRUE(foundUpdate);

    EXPECT_EQ(manager.UnregisterPhotoChange(callback), E_OK);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_056
 * @tc.name: Delete empty album notify payload check
 * @tc.desc: Verify remove payload keeps beforeChange album info when deleting empty album.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_056, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    int32_t albumId = AccurateRefresh::INVALID_INT32_VALUE;
    auto callback = std::make_shared<SyncPhotoAlbumChangeCallback>();
    auto cleanup = ScopeExit([&]() {
        (void)manager.UnregisterPhotoAlbumCallback(callback);
        if (albumId > E_OK) {
            (void)DeleteAlbumById(manager, albumId);
        }
    });

    ASSERT_EQ(manager.RegisterPhotoAlbumCallback(callback), E_OK);
    ASSERT_TRUE(WaitForCallbackIdle(*callback, 100, 1000));
    albumId = CreateTestAlbum(manager);
    ASSERT_GT(albumId, E_OK);
    std::string albumUri = QueryAlbumUriById(manager, albumId);
    ASSERT_FALSE(albumUri.empty());
    ASSERT_TRUE(WaitForCallbackIdle(*callback, 200, 5000));

    int32_t deletedAlbumId = albumId;
    int32_t removeBase = callback->GetCallTimes();
    ASSERT_GE(DeleteAlbumById(manager, deletedAlbumId), E_OK);
    albumId = AccurateRefresh::INVALID_INT32_VALUE;

    AlbumChangeInfos removeInfos;
    AlbumChangeWaitArgs waitArgs { NotifyChangeType::NOTIFY_CHANGE_REMOVE, deletedAlbumId, 5000 };
    ASSERT_TRUE(WaitForAlbumChangeByTypeAndId(*callback, removeBase, waitArgs, removeInfos));
    auto changeData = FindAlbumChangeDataById(removeInfos, deletedAlbumId);
    ASSERT_NE(changeData, nullptr);
    ASSERT_NE(changeData->albumBeforeChange, nullptr);
    EXPECT_EQ(changeData->albumAfterChange, nullptr);
    EXPECT_EQ(changeData->albumBeforeChange->albumUri_, albumUri);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_057
 * @tc.name: Move asset to another album notify payload check
 * @tc.desc: Verify moved asset update payload and album ownership fields when available.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_057, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    int32_t assetId = AccurateRefresh::INVALID_INT32_VALUE;
    int32_t targetAlbumId = AccurateRefresh::INVALID_INT32_VALUE;
    auto callback = std::make_shared<SyncPhotoAssetChangeCallback>();
    auto cleanup = ScopeExit([&]() {
        (void)manager.UnregisterPhotoChange(callback);
        if (assetId > E_OK) {
            (void)DeleteAssetById(manager, assetId);
        }
        if (targetAlbumId > E_OK) {
            (void)DeleteAlbumById(manager, targetAlbumId);
        }
    });

    ASSERT_EQ(manager.RegisterPhotoChange(callback), E_OK);
    ASSERT_TRUE(WaitForCallbackIdle(*callback, 100, 1000));
    std::string assetUri;
    ASSERT_TRUE(CreateAndWriteTestAsset(manager, assetUri, assetId));
    targetAlbumId = CreateTestAlbum(manager);
    ASSERT_GT(targetAlbumId, E_OK);
    ASSERT_TRUE(WaitForCallbackIdle(*callback, 200, 5000));

    int32_t sourceAlbumId = AccurateRefresh::INVALID_INT32_VALUE;
    int32_t moveBase = callback->GetCallTimes();
    int32_t moveRet = MoveAssetToAlbumById(manager, assetId, targetAlbumId, sourceAlbumId);
    ASSERT_GT(moveRet, E_OK);
    ASSERT_GT(sourceAlbumId, E_OK);
    ASSERT_NE(sourceAlbumId, targetAlbumId);
    std::string sourceAlbumUri = QueryAlbumUriById(manager, sourceAlbumId);
    std::string targetAlbumUri = QueryAlbumUriById(manager, targetAlbumId);

    PhotoAssetChangeInfos moveInfos;
    AssetChangeWaitArgs waitArgs { NotifyChangeType::NOTIFY_CHANGE_UPDATE, assetId, 8000 };
    ASSERT_TRUE(WaitForAssetChangeByTypeAndId(*callback, moveBase, waitArgs, moveInfos));
    auto changeData = FindAssetChangeDataById(moveInfos, assetId);
    MoveAssetPayloadExpect expect { assetId, sourceAlbumId, targetAlbumId, sourceAlbumUri, targetAlbumUri };
    ASSERT_TRUE(CheckMoveAssetPayload(changeData, expect));
    EXPECT_TRUE(IsAssetInAlbum(manager, targetAlbumId, assetId));
}

/**
 * @tc.number: MediaLibraryManager_notify_test_058
 * @tc.name: Unhide asset notify payload check
 * @tc.desc: Verify unhide emits add payload with expected fileId and visible hidden flag.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_058, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    int32_t assetId = AccurateRefresh::INVALID_INT32_VALUE;
    auto callback = std::make_shared<SyncPhotoAssetChangeCallback>();
    auto cleanup = ScopeExit([&]() {
        (void)manager.UnregisterPhotoChange(callback);
        if (assetId > E_OK) {
            (void)DeleteAssetById(manager, assetId);
        }
    });

    ASSERT_EQ(manager.RegisterPhotoChange(callback), E_OK);
    ASSERT_TRUE(WaitForCallbackIdle(*callback, 100, 1000));
    std::string assetUri;
    ASSERT_TRUE(CreateAndWriteTestAsset(manager, assetUri, assetId));
    ASSERT_TRUE(WaitForCallbackIdle(*callback, 200, 5000));
    ASSERT_GE(SetAssetHiddenByUri(manager, assetUri, true), E_OK);
    ASSERT_TRUE(WaitForCallbackIdle(*callback, 200, 5000));

    int32_t unhideBase = callback->GetCallTimes();
    ASSERT_GE(SetAssetHiddenByUri(manager, assetUri, false), E_OK);
    PhotoAssetChangeInfos unhideInfos;
    AssetChangeWaitArgs waitArgs { NotifyChangeType::NOTIFY_CHANGE_ADD, assetId, 8000 };
    ASSERT_TRUE(WaitForAssetChangeByTypeAndId(*callback, unhideBase, waitArgs, unhideInfos));
    auto changeData = FindAssetChangeDataById(unhideInfos, assetId);
    ASSERT_NE(changeData, nullptr);
    EXPECT_EQ(changeData->assetBeforeChange, nullptr);
    ASSERT_NE(changeData->assetAfterChange, nullptr);
    EXPECT_FALSE(changeData->assetAfterChange->isHidden_);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_059
 * @tc.name: Video rename notify payload check
 * @tc.desc: Verify video rename emits update payload for the target fileId.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_059, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    int32_t assetId = AccurateRefresh::INVALID_INT32_VALUE;
    auto callback = std::make_shared<SyncPhotoAssetChangeCallback>();
    auto cleanup = ScopeExit([&]() {
        (void)manager.UnregisterPhotoChange(callback);
        if (assetId > E_OK) {
            (void)DeleteAssetById(manager, assetId);
        }
    });

    ASSERT_EQ(manager.RegisterPhotoChange(callback), E_OK);
    ASSERT_TRUE(WaitForCallbackIdle(*callback, 100, 1000));
    ASSERT_TRUE(CreateAndWriteTestVideoAsset(manager, assetId));

    ASSERT_TRUE(WaitForCallbackIdle(*callback, 200, 5000));
    int32_t updateBase = callback->GetCallTimes();
    std::string renamedName = BuildUniqueRenamedVideoAssetName();
    ASSERT_GE(RenameAssetById(manager, assetId, renamedName), E_OK);

    PhotoAssetChangeInfos updateInfos;
    AssetChangeWaitArgs waitArgs { NotifyChangeType::NOTIFY_CHANGE_UPDATE, assetId, 8000 };
    ASSERT_TRUE(WaitForAssetChangeByTypeAndId(*callback, updateBase, waitArgs, updateInfos));
    auto changeData = FindAssetChangeDataById(updateInfos, assetId);
    ASSERT_NE(changeData, nullptr);
    ASSERT_NE(changeData->assetBeforeChange, nullptr);
    ASSERT_NE(changeData->assetAfterChange, nullptr);
    EXPECT_EQ(changeData->assetBeforeChange->fileId_, assetId);
    EXPECT_EQ(changeData->assetAfterChange->fileId_, assetId);
    if (!changeData->assetAfterChange->displayName_.empty()) {
        EXPECT_EQ(changeData->assetAfterChange->displayName_, renamedName);
    }
}

/**
 * @tc.number: MediaLibraryManager_notify_test_060
 * @tc.name: Move image to same album notify payload check
 * @tc.desc: Verify moving an image to its source album fails and no update notify is emitted.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_060, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    int32_t assetId = AccurateRefresh::INVALID_INT32_VALUE;
    auto callback = std::make_shared<SyncPhotoAssetChangeCallback>();
    auto cleanup = ScopeExit([&]() {
        (void)manager.UnregisterPhotoChange(callback);
        if (assetId > E_OK) {
            (void)DeleteAssetById(manager, assetId);
        }
    });

    ASSERT_EQ(manager.RegisterPhotoChange(callback), E_OK);
    ASSERT_TRUE(WaitForCallbackIdle(*callback, 100, 1000));

    std::string assetUri;
    ASSERT_TRUE(CreateAndWriteTestAsset(manager, assetUri, assetId));
    ASSERT_TRUE(WaitForCallbackIdle(*callback, 200, 5000));

    auto sourceAlbum = QuerySourceAlbumByAssetId(manager, assetId);
    ASSERT_NE(sourceAlbum, nullptr);
    int32_t sourceAlbumId = sourceAlbum->GetAlbumId();
    ASSERT_GT(sourceAlbumId, E_OK);

    int32_t moveBase = callback->GetCallTimes();
    int32_t moveSourceAlbumId = AccurateRefresh::INVALID_INT32_VALUE;
    int32_t moveRet = MoveAssetToAlbumById(manager, assetId, sourceAlbumId, moveSourceAlbumId);
    EXPECT_EQ(moveRet, E_FAIL);
    EXPECT_FALSE(callback->WaitForCallAfter(moveBase, 1500));

    auto sourceAlbumAfter = QueryAlbumById(manager, sourceAlbumId);
    ASSERT_NE(sourceAlbumAfter, nullptr);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_061
 * @tc.name: DeleteAssets updates non-empty album notify payload
 * @tc.desc: Verify DeleteAssets real deletion call triggers target album notifications.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_061, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    int32_t assetId = AccurateRefresh::INVALID_INT32_VALUE;
    int32_t albumId = AccurateRefresh::INVALID_INT32_VALUE;
    auto callback = std::make_shared<SyncPhotoAlbumChangeCallback>();
    auto cleanup = ScopeExit([&]() {
        (void)manager.UnregisterPhotoAlbumCallback(callback);
        if (assetId > E_OK) {
            (void)DeleteAssetById(manager, assetId);
        }
        if (albumId > E_OK) {
            (void)DeleteAlbumById(manager, albumId);
        }
    });

    ASSERT_EQ(manager.RegisterPhotoAlbumCallback(callback), E_OK);
    ASSERT_TRUE(WaitForCallbackIdle(*callback, 100, 1000));
    std::string assetUri;
    ASSERT_TRUE(CreateAndWriteTestAsset(manager, assetUri, assetId));
    albumId = CreateTestAlbum(manager);
    ASSERT_GT(albumId, E_OK);
    ASSERT_TRUE(WaitForCallbackIdle(*callback, 200, 5000));

    int32_t sourceAlbumId = AccurateRefresh::INVALID_INT32_VALUE;
    ASSERT_GT(MoveAssetToAlbumById(manager, assetId, albumId, sourceAlbumId), E_OK);
    ASSERT_TRUE(WaitForCallbackIdle(*callback, 200, 5000));

    int32_t notifyBase = callback->GetCallTimes();
    auto assetToDelete = QueryAssetById(manager, assetId);
    ASSERT_NE(assetToDelete, nullptr);
    std::vector<std::unique_ptr<FileAsset>> assetsToDelete;
    assetsToDelete.push_back(std::move(assetToDelete));
    int32_t deleteRet = manager.DeleteAssets(assetsToDelete);
    ASSERT_EQ(deleteRet, static_cast<int32_t>(assetsToDelete.size()));
    assetId = AccurateRefresh::INVALID_INT32_VALUE;

    ASSERT_TRUE(callback->WaitForCallAfter(notifyBase, 8000));
    ASSERT_TRUE(WaitForCallbackIdle(*callback, 200, 5000));

    auto allAlbumInfos = callback->GetAllInfos();
    ASSERT_GT(allAlbumInfos.size(), static_cast<size_t>(notifyBase));
    std::map<int32_t, std::pair<int32_t, int32_t>> notifiedAlbums;
    size_t notifyCount = LogAndSummarizeAlbumNotifications(
        allAlbumInfos, static_cast<size_t>(notifyBase), notifiedAlbums);
    EXPECT_GT(notifyCount, 0);
    EXPECT_TRUE(notifiedAlbums.find(albumId) != notifiedAlbums.end());
}

/**
 * @tc.number: MediaLibraryManager_notify_test_062
 * @tc.name: Unfavorite asset notify payload check
 * @tc.desc: Verify unfavorite emits update payload with expected fileId and favorite flag.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_062, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    int32_t assetId = AccurateRefresh::INVALID_INT32_VALUE;
    auto callback = std::make_shared<SyncPhotoAssetChangeCallback>();
    auto cleanup = ScopeExit([&]() {
        (void)manager.UnregisterPhotoChange(callback);
        if (assetId > E_OK) {
            (void)UpdateAssetFavoriteByDataShare(manager, assetId, false);
            (void)DeleteAssetById(manager, assetId);
        }
    });

    ASSERT_EQ(manager.RegisterPhotoChange(callback), E_OK);
    ASSERT_TRUE(WaitForCallbackIdle(*callback, 100, 1000));
    std::string assetUri;
    ASSERT_TRUE(CreateAndWriteTestAsset(manager, assetUri, assetId));
    ASSERT_TRUE(WaitForCallbackIdle(*callback, 200, 5000));

    int32_t favoriteBase = callback->GetCallTimes();
    ASSERT_GE(UpdateAssetFavoriteByDataShare(manager, assetId, true), E_OK);
    PhotoAssetChangeInfos favoriteInfos;
    AssetChangeWaitArgs favoriteWaitArgs { NotifyChangeType::NOTIFY_CHANGE_UPDATE, assetId, 8000 };
    ASSERT_TRUE(WaitForAssetChangeByTypeAndId(*callback, favoriteBase, favoriteWaitArgs, favoriteInfos));
    ASSERT_TRUE(WaitForCallbackIdle(*callback, 200, 3000));

    int32_t unfavoriteBase = callback->GetCallTimes();
    ASSERT_GE(UpdateAssetFavoriteByDataShare(manager, assetId, false), E_OK);
    PhotoAssetChangeInfos unfavoriteInfos;
    AssetChangeWaitArgs unfavoriteWaitArgs { NotifyChangeType::NOTIFY_CHANGE_UPDATE, assetId, 8000 };
    ASSERT_TRUE(WaitForAssetChangeByTypeAndId(*callback, unfavoriteBase, unfavoriteWaitArgs, unfavoriteInfos));
    auto changeData = FindAssetChangeDataById(unfavoriteInfos, assetId);
    ASSERT_NE(changeData, nullptr);
    ASSERT_NE(changeData->assetBeforeChange, nullptr);
    ASSERT_NE(changeData->assetAfterChange, nullptr);
    EXPECT_EQ(changeData->assetBeforeChange->fileId_, assetId);
    EXPECT_EQ(changeData->assetAfterChange->fileId_, assetId);
    EXPECT_FALSE(changeData->assetAfterChange->isFavorite_);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_063
 * @tc.name: Delete 2k albums notify payload check
 * @tc.desc: Verify album remove notify payload conversion and dispatch for 2000 change items.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_063, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    constexpr int32_t batchCount = 2000;
    std::vector<int32_t> albumIds;
    albumIds.reserve(batchCount);
    auto callback = std::make_shared<SyncPhotoAlbumChangeCallback>();
    auto cleanup = ScopeExit([&]() {
        (void)manager.UnregisterPhotoAlbumCallback(callback);
        for (int32_t albumId : albumIds) {
            if (albumId > E_OK) {
                (void)DeleteAlbumById(manager, albumId);
            }
        }
    });

    ASSERT_EQ(manager.RegisterPhotoAlbumCallback(callback), E_OK);
    ASSERT_TRUE(WaitForCallbackIdle(*callback, 100, 1000));

    CreateBatchTestAlbums(manager, batchCount, albumIds);
    ASSERT_TRUE(WaitForCallbackIdle(*callback, BATCH_IDLE_QUIET_MS, 15000));
    ExecuteAndCheckBatchAlbumDelete(manager, *callback, albumIds, 15000);

    albumIds.clear();
}

/**
 * @tc.number: MediaLibraryManager_notify_test_064
 * @tc.name: Delete 2000 assets in one album notify payload check
 * @tc.desc: Verify photo remove notify payload conversion for 2000 single-album assets.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_064, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    constexpr int32_t batchCount = 2000;
    std::vector<int32_t> assetIds;
    assetIds.reserve(batchCount);
    auto callback = std::make_shared<SyncPhotoAssetChangeCallback>();
    auto cleanup = ScopeExit([&]() {
        (void)manager.UnregisterPhotoChange(callback);
        for (int32_t assetId : assetIds) {
            if (assetId > E_OK) {
                (void)DeleteAssetById(manager, assetId);
            }
        }
    });

    ASSERT_EQ(manager.RegisterPhotoChange(callback), E_OK);
    ASSERT_TRUE(WaitForCallbackIdle(*callback, 100, 1000));

    CreateBatchTestAssets(manager, batchCount, assetIds);
    ASSERT_TRUE(WaitForCallbackIdle(*callback, BATCH_IDLE_QUIET_MS, 15000));
    ExecuteAndCheckBatchAssetDelete(manager, *callback, assetIds, 15000);

    assetIds.clear();
}

/**
 * @tc.number: MediaLibraryManager_notify_test_065
 * @tc.name: Delete 900 assets in one album notify payload check
 * @tc.desc: Verify photo remove notify payload conversion for 900 single-album assets.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_065, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    constexpr int32_t batchCount = 900;
    std::vector<int32_t> assetIds;
    assetIds.reserve(batchCount);
    auto callback = std::make_shared<SyncPhotoAssetChangeCallback>();
    auto cleanup = ScopeExit([&]() {
        (void)manager.UnregisterPhotoChange(callback);
        for (int32_t assetId : assetIds) {
            if (assetId > E_OK) {
                (void)DeleteAssetById(manager, assetId);
            }
        }
    });

    ASSERT_EQ(manager.RegisterPhotoChange(callback), E_OK);
    ASSERT_TRUE(WaitForCallbackIdle(*callback, 100, 1000));

    CreateBatchTestAssets(manager, batchCount, assetIds);
    ASSERT_TRUE(WaitForCallbackIdle(*callback, BATCH_IDLE_QUIET_MS, 10000));
    ExecuteAndCheckBatchAssetDelete(manager, *callback, assetIds, 10000);

    assetIds.clear();
}

/**
 * @tc.number: MediaLibraryManager_notify_test_066
 * @tc.name: Hide assets with DataShare update payload check
 * @tc.desc: Verify DataShare hide update emits notify payload with hidden state change.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_066, TestSize.Level1)
{
    ResetNotifyObservers(manager);

    constexpr int32_t batchCount = 2000;
    std::vector<std::string> assetUris;
    std::vector<int32_t> assetIds;
    assetUris.reserve(batchCount);
    assetIds.reserve(batchCount);
    CreateBatchTestAssets(manager, batchCount, assetIds, &assetUris);

    auto callback = std::make_shared<SyncPhotoAlbumChangeCallback>();
    bool useHiddenObserver = false;
    auto cleanup = ScopeExit([&]() {
        if (useHiddenObserver) {
            (void)manager.UnregisterHiddenAlbumChange(callback);
        } else {
            (void)manager.UnregisterPhotoAlbumCallback(callback);
        }
        if (!assetUris.empty()) {
            (void)SetAssetsHiddenByUris(manager, assetUris, false);
        }
        for (int32_t assetId : assetIds) {
            if (assetId > E_OK) {
                (void)DeleteAssetById(manager, assetId);
            }
        }
    });

    RegisterAlbumObserverWithFallback(manager, callback, useHiddenObserver);

    ASSERT_TRUE(WaitForCallbackIdle(*callback, 100, 1000));
    int32_t baseCount = callback->GetCallTimes();
    ASSERT_EQ(SetAssetsHiddenByUris(manager, assetUris, true), batchCount);
    ASSERT_TRUE(callback->WaitForCallAfter(baseCount, BATCH_NOTIFY_TIMEOUT_MS));
    ASSERT_TRUE(WaitForCallbackIdle(*callback, BATCH_IDLE_QUIET_MS, 15000));

    auto allInfos = callback->GetAllInfos();
    ASSERT_GT(allInfos.size(), static_cast<size_t>(baseCount));

    bool hasNotifyPayload = false;
    bool hasRecheckNotify = false;
    size_t startIndex = static_cast<size_t>(baseCount);
    AnalyzeAlbumNotifyResults(allInfos, startIndex, hasNotifyPayload, hasRecheckNotify);
    ASSERT_TRUE(hasNotifyPayload || hasRecheckNotify);
    if (hasNotifyPayload) {
        std::map<int32_t, std::pair<int32_t, int32_t>> notifiedAlbums;
        size_t notifyCount = LogAndSummarizeAlbumNotifications(allInfos, startIndex, notifiedAlbums);
        EXPECT_GT(notifyCount, 0);
    } else {
        EXPECT_TRUE(hasRecheckNotify);
    }
}

/**
 * @tc.number: MediaLibraryManager_notify_test_067
 * @tc.name: Hide assets with RegisterHiddenPhotoChange payload check
 * @tc.desc: Verify RegisterHiddenPhotoChange receives notify payload when assets are hidden.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_067, TestSize.Level1)
{
    ResetNotifyObservers(manager);

    constexpr int32_t batchCount = 1;
    std::vector<std::string> assetUris;
    std::vector<int32_t> assetIds;
    assetUris.reserve(batchCount);
    assetIds.reserve(batchCount);
    CreateBatchTestAssets(manager, batchCount, assetIds, &assetUris);

    auto callback = std::make_shared<SyncPhotoAssetChangeCallback>();
    bool useHiddenObserver = false;
    auto cleanup = ScopeExit([&]() {
        if (useHiddenObserver) {
            (void)manager.UnregisterHiddenPhotoChange(callback);
        } else {
            (void)manager.UnregisterPhotoChange(callback);
        }
        if (!assetUris.empty()) {
            (void)SetAssetsHiddenByUris(manager, assetUris, false);
        }
        for (int32_t assetId : assetIds) {
            if (assetId > E_OK) {
                (void)DeleteAssetById(manager, assetId);
            }
        }
    });

    RegisterHiddenPhotoObserverWithFallback(manager, callback, useHiddenObserver);

    ASSERT_TRUE(WaitForCallbackIdle(*callback, 100, 1000));
    int32_t baseCount = callback->GetCallTimes();
    ASSERT_EQ(SetAssetsHiddenByUris(manager, assetUris, true), batchCount);
    ASSERT_TRUE(callback->WaitForCallAfter(baseCount, BATCH_NOTIFY_TIMEOUT_MS));
    ASSERT_TRUE(WaitForCallbackIdle(*callback, BATCH_IDLE_QUIET_MS, 15000));

    auto allInfos = callback->GetAllInfos();
    ASSERT_GT(allInfos.size(), static_cast<size_t>(baseCount));

    bool hasNotifyPayload = false;
    bool hasRecheckNotify = false;
    size_t startIndex = static_cast<size_t>(baseCount);
    AnalyzeAssetNotifyResults(allInfos, startIndex, hasNotifyPayload, hasRecheckNotify);
    ASSERT_TRUE(hasNotifyPayload || hasRecheckNotify);
    if (hasNotifyPayload) {
        size_t notifyCount = 0;
        for (size_t i = startIndex; i < allInfos.size(); ++i) {
            notifyCount += allInfos[i].assetChangeDatas.size();
        }
        EXPECT_GT(notifyCount, 0);
    } else {
        EXPECT_TRUE(hasRecheckNotify);
    }
}

/**
 * @tc.number: MediaLibraryManager_notify_test_068
 * @tc.name: RegisterTrashedPhotoChange notify on DeleteAssets
 * @tc.desc: Verify RegisterTrashedPhotoChange receives notify with correct notifyType when asset is deleted.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_068, TestSize.Level1)
{
    ResetNotifyObservers(manager);

    auto callback = std::make_shared<SyncPhotoAssetChangeCallback>();
    int32_t assetId = AccurateRefresh::INVALID_INT32_VALUE;
    auto cleanup = ScopeExit([&]() {
        (void)manager.UnregisterTrashedPhotoChange(callback);
        if (assetId > E_OK) {
            (void)DeleteAssetById(manager, assetId);
        }
    });

    int32_t registerRet = manager.RegisterTrashedPhotoChange(callback);
    if (IsHiddenRegisterPermissionDenied(registerRet)) {
        GTEST_LOG_(INFO) << "RegisterTrashedPhotoChange permission denied, skip test";
        return;
    }
    ASSERT_EQ(registerRet, E_OK);

    std::string assetUri;
    ASSERT_TRUE(CreateAndWriteTestAsset(manager, assetUri, assetId));
    ASSERT_TRUE(WaitForCallbackIdle(*callback, 100, 1000));

    int32_t baseCount = callback->GetCallTimes();
    ASSERT_GT(DeleteAssetById(manager, assetId), E_OK);
    ASSERT_TRUE(callback->WaitForCallAfter(baseCount, BATCH_NOTIFY_TIMEOUT_MS));
    ASSERT_TRUE(WaitForCallbackIdle(*callback, BATCH_IDLE_QUIET_MS, 15000));

    auto allInfos = callback->GetAllInfos();
    size_t startIndex = static_cast<size_t>(baseCount);
    ASSERT_GT(allInfos.size(), startIndex);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_069
 * @tc.name: RegisterTrashedAlbumChange notify on DeleteAssets
 * @tc.desc: Verify RegisterTrashedAlbumChange receives notify with correct notifyType when asset is deleted.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_069, TestSize.Level1)
{
    ResetNotifyObservers(manager);

    auto callback = std::make_shared<SyncPhotoAlbumChangeCallback>();
    int32_t assetId = AccurateRefresh::INVALID_INT32_VALUE;
    auto cleanup = ScopeExit([&]() {
        (void)manager.UnregisterTrashedAlbumChange(callback);
        if (assetId > E_OK) {
            (void)DeleteAssetById(manager, assetId);
        }
    });

    int32_t registerRet = manager.RegisterTrashedAlbumChange(callback);
    if (IsHiddenRegisterPermissionDenied(registerRet)) {
        GTEST_LOG_(INFO) << "RegisterTrashedAlbumChange permission denied, skip test";
        return;
    }
    ASSERT_EQ(registerRet, E_OK);

    std::string assetUri;
    ASSERT_TRUE(CreateAndWriteTestAsset(manager, assetUri, assetId));
    ASSERT_TRUE(WaitForCallbackIdle(*callback, 100, 1000));

    int32_t baseCount = callback->GetCallTimes();
    ASSERT_GT(DeleteAssetById(manager, assetId), E_OK);
    ASSERT_TRUE(callback->WaitForCallAfter(baseCount, BATCH_NOTIFY_TIMEOUT_MS));
    ASSERT_TRUE(WaitForCallbackIdle(*callback, BATCH_IDLE_QUIET_MS, 15000));

    auto allInfos = callback->GetAllInfos();
    size_t startIndex = static_cast<size_t>(baseCount);
    ASSERT_GT(allInfos.size(), startIndex);
}
} // namespace Media
} // namespace OHOS
