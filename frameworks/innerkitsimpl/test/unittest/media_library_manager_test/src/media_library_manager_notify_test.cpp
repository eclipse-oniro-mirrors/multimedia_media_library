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

using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace {
constexpr int32_t URI_ID_DECIMAL_BASE = 10;
constexpr int32_t WRITE_RETRY_TIMES = 3;
constexpr int32_t WRITE_RETRY_INTERVAL_US = 100000;
constexpr int32_t ASSET_READY_RETRY_TIMES = 20;

class ScopeExit {
public:
    explicit ScopeExit(std::function<void()> cleanup) : cleanup_(std::move(cleanup)) {}
    ~ScopeExit()
    {
        if (cleanup_ != nullptr) {
            cleanup_();
        }
    }

private:
    std::function<void()> cleanup_;
};

std::string BuildUniqueAssetName()
{
    static std::atomic<int32_t> seq { 0 };
    return "notify_asset_" + std::to_string(++seq) + ".jpg";
}

std::string BuildUniqueRenamedAssetName()
{
    static std::atomic<int32_t> seq { 0 };
    return "notify_asset_renamed_" + std::to_string(++seq) + ".jpg";
}

std::string BuildUniqueAlbumName()
{
    static std::atomic<int32_t> seq { 0 };
    return "notify_album_" + std::to_string(++seq);
}

std::string CreateTestAssetUri(MediaLibraryManager &manager)
{
    return manager.CreateAsset(BuildUniqueAssetName());
}

int32_t CreateTestAlbum(MediaLibraryManager &manager)
{
    return manager.CreateAlbum(BuildUniqueAlbumName());
}

std::string QueryFirstAlbumUri(MediaLibraryManager &manager)
{
    std::vector<std::string> columns;
    DataShare::DataSharePredicates predicates;
    FetchResult<PhotoAlbum> result = manager.GetAlbums(columns, &predicates);
    auto album = result.GetFirstObject();
    if (album == nullptr) {
        return "";
    }
    return album->GetAlbumUri();
}

std::string QueryAlbumUriById(MediaLibraryManager &manager, int32_t albumId)
{
    std::vector<std::string> columns;
    DataShare::DataSharePredicates predicates;
    FetchResult<PhotoAlbum> result = manager.GetAlbums(columns, &predicates);
    auto album = result.GetFirstObject();
    while (album != nullptr) {
        if (album->GetAlbumId() == albumId) {
            return album->GetAlbumUri();
        }
        album = result.GetNextObject();
    }
    return "";
}

std::string EnsureAlbumUri(MediaLibraryManager &manager)
{
    std::string albumUri = QueryFirstAlbumUri(manager);
    if (!albumUri.empty()) {
        return albumUri;
    }
    if (CreateTestAlbum(manager) <= E_OK) {
        return "";
    }
    return QueryFirstAlbumUri(manager);
}

int32_t ExtractIntIdFromUri(const std::string &uri)
{
    std::string id = MediaFileUtils::GetIdFromUri(uri);
    CHECK_AND_RETURN_RET(!id.empty(), AccurateRefresh::INVALID_INT32_VALUE);
    return static_cast<int32_t>(std::strtol(id.c_str(), nullptr, URI_ID_DECIMAL_BASE));
}

void ResetNotifyObservers(MediaLibraryManager &manager)
{
    (void)manager.UnregisterSinglePhotoChange();
    (void)manager.UnregisterSinglePhotoAlbumChange();
    (void)manager.UnregisterPhotoChange();
    (void)manager.UnregisterPhotoAlbumCallback();
    (void)manager.UnregisterHiddenPhotoChange();
    (void)manager.UnregisterHiddenAlbumChange();
    (void)manager.UnregisterTrashedPhotoChange();
    (void)manager.UnregisterTrashedAlbumChange();
}

template <typename CallbackT>
bool WaitForCallbackIdle(CallbackT &callback, int32_t quietMs, int32_t maxWaitMs)
{
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(maxWaitMs);
    int32_t baseCount = callback.GetCallTimes();
    while (std::chrono::steady_clock::now() < deadline) {
        if (!callback.WaitForCallAfter(baseCount, quietMs)) {
            return true;
        }
        baseCount = callback.GetCallTimes();
    }
    return false;
}

bool IsHiddenRegisterPermissionDenied(int32_t ret)
{
    return ret == E_PERMISSION_DENIED || ret == -E_CHECK_SYSTEMAPP_FAIL;
}

std::shared_ptr<Notification::MediaChangeInfo> CreateMediaChangeInfo(
    Notification::AccurateNotifyType notifyType,
    Notification::NotifyUriType notifyUri = Notification::NotifyUriType::PHOTO_URI,
    bool isForRecheck = false)
{
    auto changeInfo = std::make_shared<Notification::MediaChangeInfo>();
    changeInfo->notifyType = notifyType;
    changeInfo->notifyUri = notifyUri;
    changeInfo->isForRecheck = isForRecheck;
    changeInfo->isSystem = false;
    return changeInfo;
}

AccurateRefresh::PhotoAssetChangeData CreatePhotoAssetChangeData(int32_t beforeId, int32_t afterId)
{
    AccurateRefresh::PhotoAssetChangeData changeData;
    changeData.infoBeforeChange_.fileId_ = beforeId;
    changeData.infoAfterChange_.fileId_ = afterId;
    return changeData;
}

AccurateRefresh::AlbumChangeData CreateAlbumChangeData(int32_t beforeId, int32_t afterId)
{
    AccurateRefresh::AlbumChangeData changeData;
    changeData.infoBeforeChange_.albumId_ = beforeId;
    changeData.infoAfterChange_.albumId_ = afterId;
    return changeData;
}

class SyncPhotoAssetChangeCallback final : public PhotoAssetChangeCallback {
public:
    void OnChange(const PhotoAssetChangeInfos &changeInfos) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        lastInfos_ = changeInfos;
        allInfos_.push_back(changeInfos);
        callTimes_++;
        cv_.notify_all();
    }

    int32_t GetCallTimes()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return callTimes_;
    }

    PhotoAssetChangeInfos GetLastInfos()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return lastInfos_;
    }

    std::vector<PhotoAssetChangeInfos> GetAllInfos()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return allInfos_;
    }

    bool WaitForCallAfter(int32_t baseCount, int32_t timeoutMs)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return cv_.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this, baseCount]() {
            return callTimes_ > baseCount;
        });
    }

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    int32_t callTimes_ = 0;
    PhotoAssetChangeInfos lastInfos_;
    std::vector<PhotoAssetChangeInfos> allInfos_;
};

class CollectPhotoAssetChangeCallback final : public PhotoAssetChangeCallback {
public:
    void OnChange(const PhotoAssetChangeInfos &changeInfos) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        allInfos_.push_back(changeInfos);
        callTimes_++;
        cv_.notify_all();
    }

    int32_t GetCallTimes()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return callTimes_;
    }

    bool WaitForCallAfter(int32_t baseCount, int32_t timeoutMs)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return cv_.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this, baseCount]() {
            return callTimes_ > baseCount;
        });
    }

    std::vector<PhotoAssetChangeInfos> GetAllInfos()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return allInfos_;
    }

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    int32_t callTimes_ = 0;
    std::vector<PhotoAssetChangeInfos> allInfos_;
};

class SyncPhotoAlbumChangeCallback final : public PhotoAlbumChangeCallback {
public:
    void OnChange(const AlbumChangeInfos &changeInfos) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        lastInfos_ = changeInfos;
        allInfos_.push_back(changeInfos);
        callTimes_++;
        cv_.notify_all();
    }

    int32_t GetCallTimes()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return callTimes_;
    }

    AlbumChangeInfos GetLastInfos()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return lastInfos_;
    }

    std::vector<AlbumChangeInfos> GetAllInfos()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return allInfos_;
    }

    bool WaitForCallAfter(int32_t baseCount, int32_t timeoutMs)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return cv_.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this, baseCount]() {
            return callTimes_ > baseCount;
        });
    }

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    int32_t callTimes_ = 0;
    AlbumChangeInfos lastInfos_;
    std::vector<AlbumChangeInfos> allInfos_;
};

class CollectPhotoAlbumChangeCallback final : public PhotoAlbumChangeCallback {
public:
    void OnChange(const AlbumChangeInfos &changeInfos) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        allInfos_.push_back(changeInfos);
        callTimes_++;
        cv_.notify_all();
    }

    int32_t GetCallTimes()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return callTimes_;
    }

    bool WaitForCallAfter(int32_t baseCount, int32_t timeoutMs)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return cv_.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this, baseCount]() {
            return callTimes_ > baseCount;
        });
    }

    std::vector<AlbumChangeInfos> GetAllInfos()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return allInfos_;
    }

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    int32_t callTimes_ = 0;
    std::vector<AlbumChangeInfos> allInfos_;
};

bool WaitAssetReadyByUri(MediaLibraryManager &manager, const std::string &assetUri)
{
    std::vector<std::string> columns = { PhotoColumn::MEDIA_ID };
    for (int32_t i = 1; i <= ASSET_READY_RETRY_TIMES; ++i) {
        auto resultSet = MediaLibraryManager::GetResultSetFromDb(CONST_MEDIA_DATA_DB_URI, assetUri, columns);
        if (resultSet != nullptr && manager.CheckResultSet(resultSet) == E_OK) {
            return true;
        }
        if (i < ASSET_READY_RETRY_TIMES) {
            usleep(WRITE_RETRY_INTERVAL_US);
        }
    }
    return false;
}

std::shared_ptr<DataShare::DataShareHelper> CreateDataShareHelperForTest(MediaLibraryManager &manager)
{
    auto token = manager.InitToken();
    if (token == nullptr) {
        return nullptr;
    }
    return DataShare::DataShareHelper::Creator(token, MEDIALIBRARY_DATA_URI);
}

int32_t CloseAssetByPhotoAccess(MediaLibraryManager &manager, const std::string &assetUri)
{
    auto dataShareHelper = CreateDataShareHelperForTest(manager);
    if (dataShareHelper == nullptr) {
        return E_FAIL;
    }
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(CONST_MEDIA_DATA_DB_URI, assetUri);
    std::string closeUri = CONST_PAH_CLOSE_PHOTO;
    MediaFileUtils::UriAppendKeyValue(closeUri, URI_PARAM_API_VERSION, std::to_string(MEDIA_API_VERSION_V10));
    MediaFileUtils::UriAppendKeyValue(closeUri, MediaColumn::MEDIA_TIME_PENDING, "0");
    Uri closeAssetUri(closeUri);
    return dataShareHelper->Insert(closeAssetUri, valuesBucket);
}

int32_t CloseAssetWithFallback(MediaLibraryManager &manager, const std::string &assetUri, int32_t fd)
{
    int32_t closeRet = manager.CloseAsset(assetUri, fd);
    if (closeRet == E_OK || closeRet == E_SUCCESS || closeRet != E_INVALID_FILEID) {
        return closeRet;
    }
    int32_t pahCloseRet = CloseAssetByPhotoAccess(manager, assetUri);
    if (pahCloseRet == E_OK || pahCloseRet == E_SUCCESS) {
        GTEST_LOG_(INFO) << "CloseAsset fallback to PAH close success, uri=" << assetUri;
        return E_OK;
    }
    GTEST_LOG_(INFO) << "CloseAsset fallback to PAH close failed, uri=" << assetUri
        << ", closeRet=" << closeRet << ", pahCloseRet=" << pahCloseRet;
    return closeRet;
}

bool WriteSingleByteToAsset(MediaLibraryManager &manager, const std::string &assetUri)
{
    std::string fileId = MediaFileUtils::GetIdFromUri(assetUri);

    for (int32_t attempt = 1; attempt <= WRITE_RETRY_TIMES; ++attempt) {
        std::string openUri = assetUri;
        int32_t fd = manager.OpenAsset(openUri, MEDIA_FILEMODE_READWRITE);
        if (fd < 0) {
            GTEST_LOG_(INFO) << "WriteSingleByteToAsset open failed, attempt=" << attempt
                << ", uri=" << assetUri << ", fd=" << fd;
            if (attempt < WRITE_RETRY_TIMES) {
                usleep(WRITE_RETRY_INTERVAL_US);
                continue;
            }
            return false;
        }

        ssize_t writeRet = write(fd, "a", 1);
        int32_t writeErrno = (writeRet > 0) ? E_OK : errno;
        bool writeOk = writeRet > 0;

        if (writeOk && !WaitAssetReadyByUri(manager, openUri)) {
            GTEST_LOG_(INFO) << "WriteSingleByteToAsset uri-ready wait timeout, attempt=" << attempt
                << ", uri=" << openUri;
        }

        // Close with the same uri form used for open to avoid provider-side uri mismatch.
        std::string closeUri = openUri;
        int32_t closeRet = CloseAssetWithFallback(manager, closeUri, fd);
        bool closeOk = (closeRet == E_SUCCESS || closeRet == E_OK);
        if (writeOk && closeOk) {
            return true;
        }
        GTEST_LOG_(INFO) << "WriteSingleByteToAsset failed, attempt=" << attempt
            << ", openUri=" << openUri << ", closeUri=" << closeUri
            << ", fileId=" << fileId << ", fd=" << fd
            << ", writeRet=" << writeRet << ", writeErrno=" << writeErrno
            << ", closeRet=" << closeRet;
        if (attempt < WRITE_RETRY_TIMES) {
            usleep(WRITE_RETRY_INTERVAL_US);
        }
    }
    return false;
}

std::unique_ptr<FileAsset> QueryAssetById(MediaLibraryManager &manager, int32_t assetId)
{
    std::vector<std::string> columns;
    DataShare::DataSharePredicates predicates;
    FetchResult<FileAsset> result = manager.GetAssets(columns, &predicates);
    auto asset = result.GetFirstObject();
    while (asset != nullptr) {
        if (asset->GetId() == assetId) {
            return asset;
        }
        asset = result.GetNextObject();
    }
    return nullptr;
}

bool WaitAssetReadyById(MediaLibraryManager &manager, int32_t assetId)
{
    if (assetId <= E_OK) {
        return false;
    }
    for (int32_t attempt = 1; attempt <= ASSET_READY_RETRY_TIMES; ++attempt) {
        if (QueryAssetById(manager, assetId) != nullptr) {
            return true;
        }
        if (attempt < ASSET_READY_RETRY_TIMES) {
            usleep(WRITE_RETRY_INTERVAL_US);
        }
    }
    return false;
}

int32_t DeleteAssetById(MediaLibraryManager &manager, int32_t assetId)
{
    auto asset = QueryAssetById(manager, assetId);
    CHECK_AND_RETURN_RET(asset != nullptr, E_INVALID_ARGUMENTS);
    std::vector<std::unique_ptr<FileAsset>> assets;
    assets.push_back(std::move(asset));
    return manager.DeleteAssets(assets);
}

int32_t SetAssetFavoriteById(MediaLibraryManager &manager, int32_t assetId, bool isFavorite)
{
    if (assetId <= E_OK) {
        return E_INVALID_ARGUMENTS;
    }
    auto dataShareHelper = CreateDataShareHelperForTest(manager);
    if (dataShareHelper == nullptr) {
        return E_FAIL;
    }
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, std::to_string(assetId));
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(PhotoColumn::MEDIA_IS_FAV, isFavorite ? 1 : 0);
    std::string updateUriStr = CONST_PAH_BATCH_UPDATE_FAVORITE;
    MediaFileUtils::UriAppendKeyValue(updateUriStr, URI_PARAM_API_VERSION, std::to_string(MEDIA_API_VERSION_V10));
    Uri updateUri(updateUriStr);
    return dataShareHelper->Update(updateUri, predicates, valuesBucket);
}

int32_t RenameAssetById(MediaLibraryManager &manager, int32_t assetId, const std::string &displayName)
{
    if (assetId <= E_OK || displayName.empty()) {
        return E_INVALID_ARGUMENTS;
    }
    auto dataShareHelper = CreateDataShareHelperForTest(manager);
    if (dataShareHelper == nullptr) {
        return E_FAIL;
    }
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, std::to_string(assetId));
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(MediaColumn::MEDIA_NAME, displayName);
    std::string updateUriStr = CONST_PAH_UPDATE_PHOTO;
    MediaFileUtils::UriAppendKeyValue(updateUriStr, URI_PARAM_API_VERSION, std::to_string(MEDIA_API_VERSION_V10));
    Uri updateUri(updateUriStr);
    return dataShareHelper->Update(updateUri, predicates, valuesBucket);
}

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
        if (!WaitAssetReadyById(manager, newAssetId)) {
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
            if (info != nullptr && info->albumId_ == args.expectedAlbumId) {
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
            notifiedAlbums[info->albumId_] = { info->albumType_, info->albumSubType_ };
            GTEST_LOG_(INFO) << "  change[" << j << "] notifyType=" << static_cast<int32_t>(infos.type)
                << " albumId=" << info->albumId_
                << " albumType=" << info->albumType_
                << " albumSubType=" << info->albumSubType_;
        }
    }
    for (const auto &item : notifiedAlbums) {
        GTEST_LOG_(INFO) << "Album notified summary: albumId=" << item.first
            << " albumType=" << item.second.first
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
        << ", subType=" << info->subType_
        << ", ownerAlbumId=" << info->ownerAlbumId_
        << ", ownerAlbumUri=" << info->ownerAlbumUri_
        << ", size=" << info->size_
        << ", dateAddedMs=" << info->dateAddedMs_
        << ", dateModifiedMs=" << info->dateModifiedMs_
        << ", isFavorite=" << info->isFavorite_
        << ", isHidden=" << info->isHidden_
        << ", dirty=" << info->dirty_
        << ", path=" << info->path_;
}
} // namespace

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
 * @tc.number: MediaLibraryManager_notify_test_007
 * @tc.name: PhotoAssetChangeInfos default value check
 * @tc.desc: Verify default values of public payload type.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_007, TestSize.Level1)
{
    PhotoAssetChangeInfos infos;
    EXPECT_EQ(infos.type, NotifyChangeType::NOTIFY_CHANGE_INVALID);
    EXPECT_TRUE(infos.assetChangeDatas.empty());
    EXPECT_FALSE(infos.isForRecheck);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_008
 * @tc.name: AlbumChangeInfo default value check
 * @tc.desc: Verify default values of album payload type.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_008, TestSize.Level1)
{
    AlbumChangeInfo info;
    EXPECT_EQ(info.albumId_, MEDIA_LIBRARY_NOTIFY_INVALID_INT32);
    EXPECT_EQ(info.imageCount_, MEDIA_LIBRARY_NOTIFY_INVALID_INT32);
    EXPECT_EQ(info.videoCount_, MEDIA_LIBRARY_NOTIFY_INVALID_INT32);
    EXPECT_EQ(info.coverDateTime_, MEDIA_LIBRARY_NOTIFY_INVALID_INT64);
    EXPECT_EQ(info.hidden_, 0);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_009
 * @tc.name: Register/Unregister photo callback
 * @tc.desc: Verify success path, duplicate register and repeated unregister.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_009, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    auto callback = std::make_shared<MockPhotoAssetChangeCallback>();
    EXPECT_EQ(manager.RegisterPhotoChange(callback), E_OK);
    EXPECT_EQ(manager.RegisterPhotoChange(callback), E_INVALID_ARGUMENTS);
    EXPECT_EQ(manager.UnregisterPhotoChange(callback), E_OK);
    EXPECT_EQ(manager.UnregisterPhotoChange(callback), E_INVALID_ARGUMENTS);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_010
 * @tc.name: UnregisterPhotoChange with optional callback
 * @tc.desc: callback optional path removes all callbacks of current notify type.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_010, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    auto callback1 = std::make_shared<MockPhotoAssetChangeCallback>();
    auto callback2 = std::make_shared<MockPhotoAssetChangeCallback>();
    EXPECT_EQ(manager.RegisterPhotoChange(callback1), E_OK);
    EXPECT_EQ(manager.RegisterPhotoChange(callback2), E_OK);
    EXPECT_EQ(manager.UnregisterPhotoChange(), E_OK);
    EXPECT_EQ(manager.UnregisterPhotoChange(), E_INVALID_ARGUMENTS);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_011
 * @tc.name: Register/Unregister album callback
 * @tc.desc: Verify success path, duplicate register and repeated unregister.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_011, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    auto callback = std::make_shared<MockPhotoAlbumChangeCallback>();
    EXPECT_EQ(manager.RegisterPhotoAlbumCallback(callback), E_OK);
    EXPECT_EQ(manager.RegisterPhotoAlbumCallback(callback), E_INVALID_ARGUMENTS);
    EXPECT_EQ(manager.UnregisterPhotoAlbumCallback(callback), E_OK);
    EXPECT_EQ(manager.UnregisterPhotoAlbumCallback(callback), E_INVALID_ARGUMENTS);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_012
 * @tc.name: UnregisterPhotoAlbumCallback with optional callback
 * @tc.desc: callback optional path removes all album callbacks of current notify type.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_012, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    auto callback1 = std::make_shared<MockPhotoAlbumChangeCallback>();
    auto callback2 = std::make_shared<MockPhotoAlbumChangeCallback>();
    EXPECT_EQ(manager.RegisterPhotoAlbumCallback(callback1), E_OK);
    EXPECT_EQ(manager.RegisterPhotoAlbumCallback(callback2), E_OK);
    EXPECT_EQ(manager.UnregisterPhotoAlbumCallback(), E_OK);
    EXPECT_EQ(manager.UnregisterPhotoAlbumCallback(), E_INVALID_ARGUMENTS);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_013
 * @tc.name: Register/Unregister hidden photo callback
 * @tc.desc: Verify hidden photo register/unregister path.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_013, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    auto callback = std::make_shared<MockPhotoAssetChangeCallback>();
    int32_t registerRet = manager.RegisterHiddenPhotoChange(callback);
    if (registerRet == E_OK) {
        EXPECT_EQ(manager.UnregisterHiddenPhotoChange(callback), E_OK);
        EXPECT_EQ(manager.UnregisterHiddenPhotoChange(callback), E_INVALID_ARGUMENTS);
        return;
    }
    EXPECT_TRUE(IsHiddenRegisterPermissionDenied(registerRet));
    EXPECT_EQ(manager.UnregisterHiddenPhotoChange(callback), E_INVALID_ARGUMENTS);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_014
 * @tc.name: Register/Unregister hidden album callback
 * @tc.desc: Verify hidden album register/unregister path.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_014, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    auto callback = std::make_shared<MockPhotoAlbumChangeCallback>();
    int32_t registerRet = manager.RegisterHiddenAlbumChange(callback);
    if (registerRet == E_OK) {
        EXPECT_EQ(manager.UnregisterHiddenAlbumChange(callback), E_OK);
        EXPECT_EQ(manager.UnregisterHiddenAlbumChange(callback), E_INVALID_ARGUMENTS);
        return;
    }
    EXPECT_TRUE(IsHiddenRegisterPermissionDenied(registerRet));
    EXPECT_EQ(manager.UnregisterHiddenAlbumChange(callback), E_INVALID_ARGUMENTS);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_015
 * @tc.name: Register/Unregister trashed photo callback
 * @tc.desc: Verify trashed photo register/unregister path.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_015, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    auto callback = std::make_shared<MockPhotoAssetChangeCallback>();
    EXPECT_EQ(manager.RegisterTrashedPhotoChange(callback), E_OK);
    EXPECT_EQ(manager.UnregisterTrashedPhotoChange(callback), E_OK);
    EXPECT_EQ(manager.UnregisterTrashedPhotoChange(callback), E_INVALID_ARGUMENTS);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_016
 * @tc.name: Register/Unregister trashed album callback
 * @tc.desc: Verify trashed album register/unregister path.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_016, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    auto callback = std::make_shared<MockPhotoAlbumChangeCallback>();
    EXPECT_EQ(manager.RegisterTrashedAlbumChange(callback), E_OK);
    EXPECT_EQ(manager.UnregisterTrashedAlbumChange(callback), E_OK);
    EXPECT_EQ(manager.UnregisterTrashedAlbumChange(callback), E_INVALID_ARGUMENTS);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_017
 * @tc.name: Register/Unregister single photo callback
 * @tc.desc: Verify single photo URI + callback unregister path.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_017, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    std::string assetUri;
    int32_t assetId = AccurateRefresh::INVALID_INT32_VALUE;
    auto cleanup = ScopeExit([&]() {
        if (assetId > E_OK) {
            (void)DeleteAssetById(manager, assetId);
        }
    });
    ASSERT_TRUE(CreateAndWriteTestAsset(manager, assetUri, assetId));
    auto callback = std::make_shared<MockPhotoAssetChangeCallback>();
    EXPECT_EQ(manager.RegisterSinglePhotoChange(assetUri, callback), E_OK);
    EXPECT_EQ(manager.UnregisterSinglePhotoChange(assetUri, callback), E_OK);
    EXPECT_EQ(manager.UnregisterSinglePhotoChange(assetUri, callback), E_INVALID_ARGUMENTS);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_018
 * @tc.name: UnregisterSinglePhotoChange uri-only path
 * @tc.desc: Verify uri-only unregister removes callbacks under current singleId.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_018, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    std::string assetUri;
    int32_t assetId = AccurateRefresh::INVALID_INT32_VALUE;
    auto cleanup = ScopeExit([&]() {
        if (assetId > E_OK) {
            (void)DeleteAssetById(manager, assetId);
        }
    });
    ASSERT_TRUE(CreateAndWriteTestAsset(manager, assetUri, assetId));
    auto callback = std::make_shared<MockPhotoAssetChangeCallback>();
    EXPECT_EQ(manager.RegisterSinglePhotoChange(assetUri, callback), E_OK);
    EXPECT_EQ(manager.UnregisterSinglePhotoChange(assetUri), E_OK);
    EXPECT_EQ(manager.UnregisterSinglePhotoChange(assetUri), E_INVALID_ARGUMENTS);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_019
 * @tc.name: UnregisterSinglePhotoChange no-arg path
 * @tc.desc: Verify no-arg unregister removes all single photo callbacks.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_019, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    std::string assetUri;
    int32_t assetId = AccurateRefresh::INVALID_INT32_VALUE;
    auto cleanup = ScopeExit([&]() {
        if (assetId > E_OK) {
            (void)DeleteAssetById(manager, assetId);
        }
    });
    ASSERT_TRUE(CreateAndWriteTestAsset(manager, assetUri, assetId));
    auto callback = std::make_shared<MockPhotoAssetChangeCallback>();
    EXPECT_EQ(manager.RegisterSinglePhotoChange(assetUri, callback), E_OK);
    EXPECT_EQ(manager.UnregisterSinglePhotoChange(), E_OK);
    EXPECT_EQ(manager.UnregisterSinglePhotoChange(), E_INVALID_ARGUMENTS);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_020
 * @tc.name: Register/Unregister single album callback
 * @tc.desc: Verify single album URI + callback unregister path.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_020, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    std::string albumUri = QueryFirstAlbumUri(manager);
    ASSERT_FALSE(albumUri.empty());
    auto callback = std::make_shared<MockPhotoAlbumChangeCallback>();
    EXPECT_EQ(manager.RegisterSinglePhotoAlbumChange(albumUri, callback), E_OK);
    EXPECT_EQ(manager.UnregisterSinglePhotoAlbumChange(albumUri, callback), E_OK);
    EXPECT_EQ(manager.UnregisterSinglePhotoAlbumChange(albumUri, callback), E_INVALID_ARGUMENTS);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_021
 * @tc.name: UnregisterSinglePhotoAlbumChange uri-only path
 * @tc.desc: Verify uri-only unregister removes callbacks under current albumId.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_021, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    std::string albumUri = QueryFirstAlbumUri(manager);
    ASSERT_FALSE(albumUri.empty());
    auto callback = std::make_shared<MockPhotoAlbumChangeCallback>();
    EXPECT_EQ(manager.RegisterSinglePhotoAlbumChange(albumUri, callback), E_OK);
    EXPECT_EQ(manager.UnregisterSinglePhotoAlbumChange(albumUri), E_OK);
    EXPECT_EQ(manager.UnregisterSinglePhotoAlbumChange(albumUri), E_INVALID_ARGUMENTS);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_022
 * @tc.name: UnregisterSinglePhotoAlbumChange no-arg path
 * @tc.desc: Verify no-arg unregister removes all single album callbacks.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_022, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    std::string albumUri = QueryFirstAlbumUri(manager);
    ASSERT_FALSE(albumUri.empty());
    auto callback = std::make_shared<MockPhotoAlbumChangeCallback>();
    EXPECT_EQ(manager.RegisterSinglePhotoAlbumChange(albumUri, callback), E_OK);
    EXPECT_EQ(manager.UnregisterSinglePhotoAlbumChange(), E_OK);
    EXPECT_EQ(manager.UnregisterSinglePhotoAlbumChange(), E_INVALID_ARGUMENTS);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_023
 * @tc.name: Optional unregister when not registered
 * @tc.desc: Verify optional unregister returns invalid-arguments when observer is absent.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_023, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    EXPECT_EQ(manager.UnregisterPhotoChange(), E_INVALID_ARGUMENTS);
    EXPECT_EQ(manager.UnregisterPhotoAlbumCallback(), E_INVALID_ARGUMENTS);
    EXPECT_EQ(manager.UnregisterHiddenPhotoChange(), E_INVALID_ARGUMENTS);
    EXPECT_EQ(manager.UnregisterHiddenAlbumChange(), E_INVALID_ARGUMENTS);
    EXPECT_EQ(manager.UnregisterTrashedPhotoChange(), E_INVALID_ARGUMENTS);
    EXPECT_EQ(manager.UnregisterTrashedAlbumChange(), E_INVALID_ARGUMENTS);
    EXPECT_EQ(manager.UnregisterSinglePhotoChange(), E_INVALID_ARGUMENTS);
    EXPECT_EQ(manager.UnregisterSinglePhotoAlbumChange(), E_INVALID_ARGUMENTS);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_024
 * @tc.name: Single unregister invalid URI after registration
 * @tc.desc: Verify invalid single URI branch when observer already exists.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_024, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    std::string assetUri;
    int32_t assetId = AccurateRefresh::INVALID_INT32_VALUE;
    auto cleanup = ScopeExit([&]() {
        if (assetId > E_OK) {
            (void)DeleteAssetById(manager, assetId);
        }
    });
    ASSERT_TRUE(CreateAndWriteTestAsset(manager, assetUri, assetId));
    std::string albumUri = QueryFirstAlbumUri(manager);
    ASSERT_FALSE(albumUri.empty());

    auto assetCallback = std::make_shared<MockPhotoAssetChangeCallback>();
    auto albumCallback = std::make_shared<MockPhotoAlbumChangeCallback>();
    EXPECT_EQ(manager.RegisterSinglePhotoChange(assetUri, assetCallback), E_OK);
    EXPECT_EQ(manager.RegisterSinglePhotoAlbumChange(albumUri, albumCallback), E_OK);

    EXPECT_EQ(manager.UnregisterSinglePhotoChange("invalid_uri"), E_INVALID_ARGUMENTS);
    EXPECT_EQ(manager.UnregisterSinglePhotoAlbumChange("invalid_uri"), E_INVALID_ARGUMENTS);

    EXPECT_EQ(manager.UnregisterSinglePhotoChange(), E_OK);
    EXPECT_EQ(manager.UnregisterSinglePhotoAlbumChange(), E_OK);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_025
 * @tc.name: Callback object records OnChange payload
 * @tc.desc: Verify callback base interface is usable by public payload.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_025, TestSize.Level1)
{
    MockPhotoAssetChangeCallback assetCallback;
    MockPhotoAlbumChangeCallback albumCallback;

    PhotoAssetChangeInfos assetInfos;
    assetInfos.type = NotifyChangeType::NOTIFY_CHANGE_UPDATE;
    assetInfos.isForRecheck = true;

    AlbumChangeInfos albumInfos;
    albumInfos.type = NotifyChangeType::NOTIFY_CHANGE_ADD;
    albumInfos.isForRecheck = false;

    assetCallback.OnChange(assetInfos);
    albumCallback.OnChange(albumInfos);

    EXPECT_EQ(assetCallback.GetCallTimes(), 1);
    EXPECT_EQ(assetCallback.GetChangeInfos().type, NotifyChangeType::NOTIFY_CHANGE_UPDATE);
    EXPECT_TRUE(assetCallback.GetChangeInfos().isForRecheck);
    EXPECT_EQ(albumCallback.GetCallTimes(), 1);
    EXPECT_EQ(albumCallback.GetChangeInfos().type, NotifyChangeType::NOTIFY_CHANGE_ADD);
    EXPECT_FALSE(albumCallback.GetChangeInfos().isForRecheck);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_026
 * @tc.name: RegisterPhotoChange passive receive callback
 * @tc.desc: Verify callback receives changeInfos passively after data change.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_026, TestSize.Level1)
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
    int32_t baseCount = callback->GetCallTimes();

    std::string assetUri;
    ASSERT_TRUE(CreateAndWriteTestAsset(manager, assetUri, assetId));

    bool received = callback->WaitForCallAfter(baseCount, 5000);

    EXPECT_TRUE(received);
    PhotoAssetChangeInfos infos = callback->GetLastInfos();
    EXPECT_EQ(infos.type, NotifyChangeType::NOTIFY_CHANGE_ADD);
    GTEST_LOG_(INFO) << "Received asset change callback with type: " << static_cast<int32_t>(infos.type);

    EXPECT_EQ(manager.UnregisterPhotoChange(callback), E_OK);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_027
 * @tc.name: RegisterPhotoAlbumCallback passive receive callback
 * @tc.desc: Verify album callback receives changeInfos passively after data change.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_027, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    auto callback = std::make_shared<SyncPhotoAlbumChangeCallback>();
    ASSERT_EQ(manager.RegisterPhotoAlbumCallback(callback), E_OK);

    ASSERT_TRUE(WaitForCallbackIdle(*callback, 100, 1000));
    int32_t baseCount = callback->GetCallTimes();

    int32_t albumId = CreateTestAlbum(manager);
    ASSERT_GT(albumId, 0);

    EXPECT_TRUE(callback->WaitForCallAfter(baseCount, 3000));
    AlbumChangeInfos infos = callback->GetLastInfos();
    EXPECT_EQ(infos.type, NotifyChangeType::NOTIFY_CHANGE_ADD);
    GTEST_LOG_(INFO) << "Received album change callback with type: " << static_cast<int32_t>(infos.type);

    EXPECT_EQ(manager.UnregisterPhotoAlbumCallback(callback), E_OK);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_028
 * @tc.name: ConvertProviderError maps provider errors to public errors
 * @tc.desc: Verify each mapping branch in ConvertProviderError.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_028, TestSize.Level1)
{
    struct ErrorMapCase {
        int32_t input;
        int32_t expected;
    };
    const std::vector<ErrorMapCase> cases = {
        { E_OK, E_OK },
        { E_SUCCESS, E_OK },
        { E_PERMISSION_DENIED, E_PERMISSION_DENIED },
        { E_MAX_ON_SINGLE_NUM, E_MAX_ON_SINGLE_NUM },
        { E_INVALID_ARGUMENTS, E_INVALID_ARGUMENTS },
        { E_URI_IS_INVALID, E_INVALID_ARGUMENTS },
        { E_URI_NOT_EXIST, E_INVALID_ARGUMENTS },
        { E_DATAOBSERVER_IS_NULL, E_INVALID_ARGUMENTS },
        { E_DATAOBSERVER_IS_REPEATED, E_INVALID_ARGUMENTS },
        { E_CHECK_SYSTEMAPP_FAIL, -E_CHECK_SYSTEMAPP_FAIL },
        { -E_CHECK_SYSTEMAPP_FAIL, -E_CHECK_SYSTEMAPP_FAIL },
        { -12345, -12345 },
        { 12345, E_FAIL },
    };

    for (const auto &item : cases) {
        EXPECT_EQ(MediaLibraryManagerNotifyUtils::ConvertProviderError(item.input), item.expected)
            << "input=" << item.input;
    }
}

/**
 * @tc.number: MediaLibraryManager_notify_test_029
 * @tc.name: IsAssetNotifyType truth table
 * @tc.desc: Verify asset uri types return true and non-asset uri types return false.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_029, TestSize.Level1)
{
    EXPECT_TRUE(MediaLibraryManagerNotifyUtils::IsAssetNotifyType(Notification::NotifyUriType::PHOTO_URI));
    EXPECT_TRUE(MediaLibraryManagerNotifyUtils::IsAssetNotifyType(Notification::NotifyUriType::HIDDEN_PHOTO_URI));
    EXPECT_TRUE(MediaLibraryManagerNotifyUtils::IsAssetNotifyType(Notification::NotifyUriType::TRASH_PHOTO_URI));
    EXPECT_TRUE(MediaLibraryManagerNotifyUtils::IsAssetNotifyType(Notification::NotifyUriType::SINGLE_PHOTO_URI));
    EXPECT_FALSE(MediaLibraryManagerNotifyUtils::IsAssetNotifyType(Notification::NotifyUriType::PHOTO_ALBUM_URI));
    EXPECT_FALSE(MediaLibraryManagerNotifyUtils::IsAssetNotifyType(Notification::NotifyUriType::INVALID));
}

/**
 * @tc.number: MediaLibraryManager_notify_test_030
 * @tc.name: BuildPhotoAssetChangeInfos handles null input
 * @tc.desc: Verify null MediaChangeInfo returns default value without crash.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_030, TestSize.Level1)
{
    PhotoAssetChangeInfos infos = MediaLibraryManagerNotifyUtils::BuildPhotoAssetChangeInfos(nullptr);
    EXPECT_EQ(infos.type, NotifyChangeType::NOTIFY_CHANGE_INVALID);
    EXPECT_TRUE(infos.assetChangeDatas.empty());
    EXPECT_FALSE(infos.isForRecheck);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_031
 * @tc.name: BuildPhotoAssetChangeInfos filters non-asset items
 * @tc.desc: Verify AlbumChangeData item in variant is skipped.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_031, TestSize.Level1)
{
    auto changeInfo = CreateMediaChangeInfo(Notification::AccurateNotifyType::NOTIFY_ASSET_UPDATE);
    AccurateRefresh::AlbumChangeData albumData;
    changeInfo->changeInfos.push_back(albumData);

    PhotoAssetChangeInfos infos = MediaLibraryManagerNotifyUtils::BuildPhotoAssetChangeInfos(changeInfo);
    EXPECT_EQ(infos.type, NotifyChangeType::NOTIFY_CHANGE_UPDATE);
    EXPECT_FALSE(infos.isForRecheck);
    EXPECT_TRUE(infos.assetChangeDatas.empty());
}

/**
 * @tc.number: MediaLibraryManager_notify_test_032
 * @tc.name: BuildPhotoAssetChangeInfos maps invalid fileId to nullptr
 * @tc.desc: Verify invalid fileId triggers early-return branch for before-change info.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_032, TestSize.Level1)
{
    auto changeInfo = CreateMediaChangeInfo(Notification::AccurateNotifyType::NOTIFY_ASSET_ADD);
    auto changeData = CreatePhotoAssetChangeData(AccurateRefresh::INVALID_INT32_VALUE, 101);
    changeInfo->changeInfos.push_back(changeData);

    PhotoAssetChangeInfos infos = MediaLibraryManagerNotifyUtils::BuildPhotoAssetChangeInfos(changeInfo);
    ASSERT_EQ(infos.assetChangeDatas.size(), 1);
    EXPECT_EQ(infos.assetChangeDatas[0].assetBeforeChange, nullptr);
    ASSERT_NE(infos.assetChangeDatas[0].assetAfterChange, nullptr);
    EXPECT_EQ(infos.assetChangeDatas[0].assetAfterChange->fileId_, 101);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_033
 * @tc.name: BuildPhotoAssetChangeInfos handles hidden uri and null album pointer
 * @tc.desc: Verify hiddenTime assignment and nullptr album item forwarding branch.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_033, TestSize.Level1)
{
    auto changeInfo = CreateMediaChangeInfo(Notification::AccurateNotifyType::NOTIFY_ASSET_UPDATE);
    auto changeData = CreatePhotoAssetChangeData(201, 202);
    changeData.infoAfterChange_.hiddenTime_ = 9527;
    changeData.infoAfterChange_.albumChangeInfos_.push_back(nullptr);

    auto validAlbumInfo = std::make_shared<AccurateRefresh::AlbumChangeInfo>();
    validAlbumInfo->albumId_ = 3001;
    validAlbumInfo->albumType_ = 8;
    validAlbumInfo->albumName_ = "album_from_change_info";
    changeData.infoAfterChange_.albumChangeInfos_.push_back(validAlbumInfo);
    changeInfo->changeInfos.push_back(changeData);

    PhotoAssetChangeInfos infos = MediaLibraryManagerNotifyUtils::BuildPhotoAssetChangeInfos(
        changeInfo, Notification::NotifyUriType::HIDDEN_PHOTO_URI);
    ASSERT_EQ(infos.assetChangeDatas.size(), 1);
    ASSERT_NE(infos.assetChangeDatas[0].assetAfterChange, nullptr);
    EXPECT_EQ(infos.assetChangeDatas[0].assetAfterChange->hiddenTime_, 9527);
    ASSERT_EQ(infos.assetChangeDatas[0].assetAfterChange->albumChangeInfos_.size(), 2);
    EXPECT_EQ(infos.assetChangeDatas[0].assetAfterChange->albumChangeInfos_[0], nullptr);
    ASSERT_NE(infos.assetChangeDatas[0].assetAfterChange->albumChangeInfos_[1], nullptr);
    EXPECT_EQ(infos.assetChangeDatas[0].assetAfterChange->albumChangeInfos_[1]->albumType_, 8);
    EXPECT_EQ(infos.assetChangeDatas[0].assetAfterChange->albumChangeInfos_[1]->albumName_,
        "album_from_change_info");
}

/**
 * @tc.number: MediaLibraryManager_notify_test_034
 * @tc.name: BuildPhotoAssetChangeInfos returns nullptr album when albumId invalid
 * @tc.desc: Verify non-null album pointer with invalid albumId becomes nullptr after conversion.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_034, TestSize.Level1)
{
    auto changeInfo = CreateMediaChangeInfo(Notification::AccurateNotifyType::NOTIFY_ASSET_UPDATE);
    auto changeData = CreatePhotoAssetChangeData(401, 402);

    auto invalidAlbumInfo = std::make_shared<AccurateRefresh::AlbumChangeInfo>();
    changeData.infoAfterChange_.albumChangeInfos_.push_back(invalidAlbumInfo);
    changeInfo->changeInfos.push_back(changeData);

    PhotoAssetChangeInfos infos = MediaLibraryManagerNotifyUtils::BuildPhotoAssetChangeInfos(changeInfo);
    ASSERT_EQ(infos.assetChangeDatas.size(), 1);
    ASSERT_NE(infos.assetChangeDatas[0].assetAfterChange, nullptr);
    ASSERT_EQ(infos.assetChangeDatas[0].assetAfterChange->albumChangeInfos_.size(), 1);
    EXPECT_EQ(infos.assetChangeDatas[0].assetAfterChange->albumChangeInfos_[0], nullptr);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_036
 * @tc.name: NotifyChange default branch for unsupported notify uri
 * @tc.desc: Verify unsupported notify uri does not trigger registered callbacks.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_036, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    auto callback = std::make_shared<MockPhotoAssetChangeCallback>();
    ASSERT_EQ(manager.RegisterPhotoChange(callback), E_OK);

    auto changeInfo = CreateMediaChangeInfo(
        Notification::AccurateNotifyType::NOTIFY_ASSET_UPDATE,
        Notification::NotifyUriType::INVALID);
    changeInfo->changeInfos.push_back(CreatePhotoAssetChangeData(1001, 1002));

    MediaLibraryManagerNotifyObserverManager::GetInstance().NotifyChange(changeInfo);
    EXPECT_EQ(callback->GetCallTimes(), 0);

    EXPECT_EQ(manager.UnregisterPhotoChange(callback), E_OK);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_037
 * @tc.name: Single photo recheck broadcast path
 * @tc.desc: Verify SINGLE_PHOTO_URI recheck dispatches to all single-photo callbacks.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_037, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    std::string assetUri;
    int32_t assetId = AccurateRefresh::INVALID_INT32_VALUE;
    auto cleanup = ScopeExit([&]() {
        if (assetId > E_OK) {
            (void)DeleteAssetById(manager, assetId);
        }
    });
    ASSERT_TRUE(CreateAndWriteTestAsset(manager, assetUri, assetId));

    auto callback1 = std::make_shared<MockPhotoAssetChangeCallback>();
    auto callback2 = std::make_shared<MockPhotoAssetChangeCallback>();
    ASSERT_EQ(manager.RegisterSinglePhotoChange(assetUri, callback1), E_OK);
    ASSERT_EQ(manager.RegisterSinglePhotoChange(assetUri, callback2), E_OK);

    auto changeInfo = CreateMediaChangeInfo(
        Notification::AccurateNotifyType::NOTIFY_ASSET_UPDATE,
        Notification::NotifyUriType::SINGLE_PHOTO_URI,
        true);
    MediaLibraryManagerNotifyObserverManager::GetInstance().NotifyChange(changeInfo);

    EXPECT_EQ(callback1->GetCallTimes(), 1);
    EXPECT_EQ(callback2->GetCallTimes(), 1);
    EXPECT_TRUE(callback1->GetChangeInfos().isForRecheck);
    EXPECT_EQ(callback1->GetChangeInfos().type, NotifyChangeType::NOTIFY_CHANGE_UPDATE);

    EXPECT_EQ(manager.UnregisterSinglePhotoChange(), E_OK);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_038
 * @tc.name: Single photo fallback from beforeId to afterId
 * @tc.desc: Verify callback lookup fallback branch hits afterId when beforeId misses.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_038, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    std::string assetUri;
    int32_t assetId = AccurateRefresh::INVALID_INT32_VALUE;
    auto cleanup = ScopeExit([&]() {
        if (assetId > E_OK) {
            (void)DeleteAssetById(manager, assetId);
        }
    });
    ASSERT_TRUE(CreateAndWriteTestAsset(manager, assetUri, assetId));

    auto callback = std::make_shared<MockPhotoAssetChangeCallback>();
    ASSERT_EQ(manager.RegisterSinglePhotoChange(assetUri, callback), E_OK);

    auto changeInfo = CreateMediaChangeInfo(
        Notification::AccurateNotifyType::NOTIFY_ASSET_UPDATE,
        Notification::NotifyUriType::SINGLE_PHOTO_URI);
    changeInfo->changeInfos.push_back(CreatePhotoAssetChangeData(AccurateRefresh::INVALID_INT32_VALUE, assetId));

    MediaLibraryManagerNotifyObserverManager::GetInstance().NotifyChange(changeInfo);
    EXPECT_EQ(callback->GetCallTimes(), 1);
    ASSERT_EQ(callback->GetChangeInfos().assetChangeDatas.size(), 1);
    ASSERT_NE(callback->GetChangeInfos().assetChangeDatas[0].assetAfterChange, nullptr);
    EXPECT_EQ(callback->GetChangeInfos().assetChangeDatas[0].assetAfterChange->fileId_, assetId);

    EXPECT_EQ(manager.UnregisterSinglePhotoChange(), E_OK);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_039
 * @tc.name: Single photo skips non-asset variant item
 * @tc.desc: Verify album variant item in SINGLE_PHOTO_URI notify is ignored.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_039, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    std::string assetUri;
    int32_t assetId = AccurateRefresh::INVALID_INT32_VALUE;
    auto cleanup = ScopeExit([&]() {
        if (assetId > E_OK) {
            (void)DeleteAssetById(manager, assetId);
        }
    });
    ASSERT_TRUE(CreateAndWriteTestAsset(manager, assetUri, assetId));

    auto callback = std::make_shared<MockPhotoAssetChangeCallback>();
    ASSERT_EQ(manager.RegisterSinglePhotoChange(assetUri, callback), E_OK);

    auto changeInfo = CreateMediaChangeInfo(
        Notification::AccurateNotifyType::NOTIFY_ASSET_UPDATE,
        Notification::NotifyUriType::SINGLE_PHOTO_URI);
    AccurateRefresh::AlbumChangeData albumData;
    changeInfo->changeInfos.push_back(albumData);

    MediaLibraryManagerNotifyObserverManager::GetInstance().NotifyChange(changeInfo);
    EXPECT_EQ(callback->GetCallTimes(), 0);

    EXPECT_EQ(manager.UnregisterSinglePhotoChange(), E_OK);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_040
 * @tc.name: Single album recheck broadcast path
 * @tc.desc: Verify SINGLE_PHOTO_ALBUM_URI recheck dispatches to all single-album callbacks.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_040, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    std::string albumUri = EnsureAlbumUri(manager);
    ASSERT_FALSE(albumUri.empty());

    auto callback1 = std::make_shared<MockPhotoAlbumChangeCallback>();
    auto callback2 = std::make_shared<MockPhotoAlbumChangeCallback>();
    ASSERT_EQ(manager.RegisterSinglePhotoAlbumChange(albumUri, callback1), E_OK);
    ASSERT_EQ(manager.RegisterSinglePhotoAlbumChange(albumUri, callback2), E_OK);

    auto changeInfo = CreateMediaChangeInfo(
        Notification::AccurateNotifyType::NOTIFY_ALBUM_UPDATE,
        Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI,
        true);
    MediaLibraryManagerNotifyObserverManager::GetInstance().NotifyChange(changeInfo);

    EXPECT_EQ(callback1->GetCallTimes(), 1);
    EXPECT_EQ(callback2->GetCallTimes(), 1);
    EXPECT_TRUE(callback1->GetChangeInfos().isForRecheck);
    EXPECT_EQ(callback1->GetChangeInfos().type, NotifyChangeType::NOTIFY_CHANGE_UPDATE);

    EXPECT_EQ(manager.UnregisterSinglePhotoAlbumChange(), E_OK);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_041
 * @tc.name: Single album fallback from beforeId to afterId
 * @tc.desc: Verify callback lookup fallback branch hits afterId when beforeId misses.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_041, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    std::string albumUri = EnsureAlbumUri(manager);
    ASSERT_FALSE(albumUri.empty());
    int32_t albumId = ExtractIntIdFromUri(albumUri);
    ASSERT_GT(albumId, E_OK);

    auto callback = std::make_shared<MockPhotoAlbumChangeCallback>();
    ASSERT_EQ(manager.RegisterSinglePhotoAlbumChange(albumUri, callback), E_OK);

    auto changeInfo = CreateMediaChangeInfo(
        Notification::AccurateNotifyType::NOTIFY_ALBUM_UPDATE,
        Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI);
    changeInfo->changeInfos.push_back(CreateAlbumChangeData(AccurateRefresh::INVALID_INT32_VALUE, albumId));

    MediaLibraryManagerNotifyObserverManager::GetInstance().NotifyChange(changeInfo);
    EXPECT_EQ(callback->GetCallTimes(), 1);
    ASSERT_EQ(callback->GetChangeInfos().albumChangeDatas.size(), 1);
    EXPECT_EQ(callback->GetChangeInfos().albumChangeDatas[0].albumBeforeChange, nullptr);
    ASSERT_NE(callback->GetChangeInfos().albumChangeDatas[0].albumAfterChange, nullptr);
    EXPECT_EQ(callback->GetChangeInfos().type, NotifyChangeType::NOTIFY_CHANGE_UPDATE);

    EXPECT_EQ(manager.UnregisterSinglePhotoAlbumChange(), E_OK);
}

/**
 * @tc.number: MediaLibraryManager_notify_test_042
 * @tc.name: Single album skips non-album variant item
 * @tc.desc: Verify photo-asset variant item in SINGLE_PHOTO_ALBUM_URI notify is ignored.
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_notify_test_042, TestSize.Level1)
{
    ResetNotifyObservers(manager);
    std::string albumUri = EnsureAlbumUri(manager);
    ASSERT_FALSE(albumUri.empty());

    auto callback = std::make_shared<MockPhotoAlbumChangeCallback>();
    ASSERT_EQ(manager.RegisterSinglePhotoAlbumChange(albumUri, callback), E_OK);

    auto changeInfo = CreateMediaChangeInfo(
        Notification::AccurateNotifyType::NOTIFY_ALBUM_UPDATE,
        Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI);
    changeInfo->changeInfos.push_back(CreatePhotoAssetChangeData(2001, 2002));

    MediaLibraryManagerNotifyObserverManager::GetInstance().NotifyChange(changeInfo);
    EXPECT_EQ(callback->GetCallTimes(), 0);

    EXPECT_EQ(manager.UnregisterSinglePhotoAlbumChange(), E_OK);
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
    EXPECT_EQ(infos.albumChangeDatas[0].albumAfterChange->albumId_, albumId);
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
                << ", albumId=" << info->albumId_
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
            (void)SetAssetFavoriteById(manager, assetId, false);
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
    ASSERT_GE(SetAssetFavoriteById(manager, assetId, true), E_OK);
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
} // namespace Media
} // namespace OHOS
