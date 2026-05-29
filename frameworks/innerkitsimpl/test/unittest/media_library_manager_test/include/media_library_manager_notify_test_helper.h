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

#ifndef MEDIA_LIBRARY_MANAGER_NOTIFY_TEST_HELPER_H
#define MEDIA_LIBRARY_MANAGER_NOTIFY_TEST_HELPER_H

namespace OHOS {
namespace Media {

constexpr int32_t URI_ID_DECIMAL_BASE = 10;
constexpr int32_t WRITE_RETRY_TIMES = 3;
constexpr int32_t WRITE_RETRY_INTERVAL_US = 100000;
constexpr int32_t ASSET_READY_RETRY_TIMES = 20;
constexpr int32_t BATCH_NOTIFY_TIMEOUT_MS = 30000;
constexpr int32_t BATCH_IDLE_QUIET_MS = 300;

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

std::string BuildUniqueVideoAssetName()
{
    static std::atomic<int32_t> seq { 0 };
    return "notify_video_" + std::to_string(++seq) + ".mp4";
}

std::string BuildUniqueRenamedVideoAssetName()
{
    static std::atomic<int32_t> seq { 0 };
    return "notify_video_renamed_" + std::to_string(++seq) + ".mp4";
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
    std::vector<std::string> albumColumns;
    DataShare::DataSharePredicates albumPredicates;
    FetchResult<PhotoAlbum> albums = manager.GetAlbums(albumColumns, &albumPredicates);
    auto album = albums.GetFirstObject();
    while (album != nullptr) {
        std::vector<std::string> assetColumns;
        DataShare::DataSharePredicates assetPredicates;
        assetPredicates.EqualTo(MediaColumn::MEDIA_ID, std::to_string(assetId));
        FetchResult<FileAsset> assets = manager.GetAssets(*album, assetColumns, &assetPredicates);
        auto asset = assets.GetFirstObject();
        if (asset != nullptr) {
            return asset;
        }
        album = albums.GetNextObject();
    }
    return nullptr;
}

bool WaitAssetReadyById(MediaLibraryManager &manager, int32_t assetId)
{
    if (assetId <= E_OK) {
        return false;
    }
    std::vector<std::string> columns = { PhotoColumn::MEDIA_ID };
    for (int32_t attempt = 1; attempt <= ASSET_READY_RETRY_TIMES; ++attempt) {
        if (QueryAssetById(manager, assetId) != nullptr) {
            return true;
        }
        auto resultSet = MediaLibraryManager::GetResultSetFromDb(
            PhotoColumn::MEDIA_ID, std::to_string(assetId), columns);
        if (resultSet != nullptr && manager.CheckResultSet(resultSet) == E_OK) {
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

int32_t UpdateAssetFavoriteByDataShare(MediaLibraryManager &manager, int32_t assetId, bool isFavorite)
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
}
} // namespace

#endif // MEDIA_LIBRARY_MANAGER_NOTIFY_TEST_HELPER_H
