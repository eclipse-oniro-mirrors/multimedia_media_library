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

#define MLOG_TAG "Scanner"

#include "media_scanner_manager.h"

#include "directory_ex.h"

#include "media_log.h"
#include "medialibrary_errno.h"
#include "media_scanner_db.h"

namespace OHOS {
namespace Media {
std::shared_ptr<MediaScannerManager> MediaScannerManager::instance_ = nullptr;
std::mutex MediaScannerManager::instanceMutex_;

std::shared_ptr<MediaScannerManager> MediaScannerManager::GetInstance()
{
    if (instance_ == nullptr) {
        std::lock_guard<std::mutex> guard(instanceMutex_);

        if (instance_ != nullptr) {
            return instance_;
        }

        instance_ = std::shared_ptr<MediaScannerManager>(new (std::nothrow) MediaScannerManager());
    }

    return instance_;
}

int32_t MediaScannerManager::ScanFile(const std::string &path, const std::shared_ptr<IMediaScannerCallback> &callback,
    MediaLibraryApi api, bool isCameraShotMovingPhoto)
{
    MEDIA_DEBUG_LOG("scan file %{private}s, api%{public}d", path.c_str(), static_cast<int>(api));

    string realPath;
    if (!PathToRealPath(path, realPath)) {
        MEDIA_ERR_LOG("failed to get real path %{private}s, errno %{public}d", path.c_str(), errno);
        return E_INVALID_PATH;
    }

    if (!ScannerUtils::IsRegularFile(realPath)) {
        MEDIA_ERR_LOG("the path %{private}s is not a regular file", realPath.c_str());
        return E_INVALID_PATH;
    }

    MediaScannerObj::ScanType scanType =
        isCameraShotMovingPhoto ? MediaScannerObj::CAMERA_SHOT_MOVING_PHOTO : MediaScannerObj::FILE;
    std::unique_ptr<MediaScannerObj> scanner =
        std::make_unique<MediaScannerObj>(realPath, callback, scanType, api);
    executor_.Commit(move(scanner));

    return E_OK;
}

int32_t MediaScannerManager::ScanFileSync(const std::string &path,
    const std::shared_ptr<IMediaScannerCallback> &callback, MediaLibraryApi api, bool isForceScan, int32_t fileId)
{
    MEDIA_DEBUG_LOG("scan file %{private}s, api%{public}d", path.c_str(), static_cast<int>(api));

    string realPath;
    if (!PathToRealPath(path, realPath)) {
        MEDIA_ERR_LOG("failed to get real path %{private}s, errno %{public}d", path.c_str(), errno);
        return E_INVALID_PATH;
    }

    if (!ScannerUtils::IsRegularFile(realPath)) {
        MEDIA_ERR_LOG("the path %{private}s is not a regular file", realPath.c_str());
        return E_INVALID_PATH;
    }

    MediaScannerObj scanner = MediaScannerObj(realPath, callback, MediaScannerObj::FILE, api);
    if (isForceScan) {
        scanner.SetForceScan(true);
    }
    scanner.SetFileId(fileId);
    scanner.Scan();

    return E_OK;
}

int32_t MediaScannerManager::ScanFileSyncWithoutAlbumUpdate(const std::string &path,
    const std::shared_ptr<IMediaScannerCallback> &callback, MediaLibraryApi api, bool isForceScan, int32_t fileId)
{
    MEDIA_DEBUG_LOG("scan file %{private}s, api%{public}d", path.c_str(), static_cast<int>(api));

    string realPath;
    if (!PathToRealPath(path, realPath)) {
        MEDIA_ERR_LOG("failed to get real path %{private}s, errno %{public}d", path.c_str(), errno);
        return E_INVALID_PATH;
    }

    if (!ScannerUtils::IsRegularFile(realPath)) {
        MEDIA_ERR_LOG("the path %{private}s is not a regular file", realPath.c_str());
        return E_INVALID_PATH;
    }

    MediaScannerObj scanner = MediaScannerObj(realPath, callback, MediaScannerObj::FILE, api);
    if (isForceScan) {
        scanner.SetForceScan(true);
    }
    scanner.SetFileId(fileId);
    scanner.SetIsSkipAlbumUpdate(true);
    scanner.Scan();

    return E_OK;
}

int32_t MediaScannerManager::ScanDir(const std::string &path, const std::shared_ptr<IMediaScannerCallback> &callback)
{
    MEDIA_DEBUG_LOG("scan dir %{private}s", path.c_str());

    string realPath;
    if (!PathToRealPath(path, realPath)) {
        MEDIA_ERR_LOG("failed to get real path %{private}s, errno %{public}d", path.c_str(), errno);
        return E_INVALID_PATH;
    }

    if (!ScannerUtils::IsDirectory(realPath)) {
        MEDIA_ERR_LOG("the path %{private}s is not a directory", realPath.c_str());
        return E_INVALID_PATH;
    }

    std::unique_ptr<MediaScannerObj> scanner = std::make_unique<MediaScannerObj>(realPath, callback,
        MediaScannerObj::DIRECTORY);
    executor_.Commit(move(scanner));

    return E_OK;
}

int32_t MediaScannerManager::ScanDirSync(const std::string &path,
    const std::shared_ptr<IMediaScannerCallback> &callback)
{
    MEDIA_DEBUG_LOG("scan dir %{private}s", path.c_str());

    string realPath;
    if (!PathToRealPath(path, realPath)) {
        MEDIA_ERR_LOG("failed to get real path %{private}s, errno %{public}d", path.c_str(), errno);
        return E_INVALID_PATH;
    }

    if (!ScannerUtils::IsDirectory(realPath)) {
        MEDIA_ERR_LOG("the path %{private}s is not a directory", realPath.c_str());
        return E_INVALID_PATH;
    }

    MediaScannerObj scanner = MediaScannerObj(realPath, callback, MediaScannerObj::DIRECTORY);
    scanner.Scan();

    return E_OK;
}

void MediaScannerManager::Start()
{
    executor_.Start();

    std::unique_ptr<MediaScannerObj> scanner = std::make_unique<MediaScannerObj>(MediaScannerObj::START);
    executor_.Commit(move(scanner));
}

void MediaScannerManager::Stop()
{
    /* stop all working threads */
    this->executor_.Stop();

    MediaScannerDb::GetDatabaseInstance()->DeleteError(ROOT_MEDIA_DIR);
}

void MediaScannerManager::ScanError()
{
    std::unique_ptr<MediaScannerObj> scanner = std::make_unique<MediaScannerObj>(MediaScannerObj::ERROR);
    executor_.Commit(move(scanner));
}

void MediaScannerManager::ErrorRecord(const std::string &path)
{
    std::unique_ptr<MediaScannerObj> scanner = std::make_unique<MediaScannerObj>(MediaScannerObj::SET_ERROR);
    scanner->SetErrorPath(path);
    executor_.Commit(move(scanner));
}
} // namespace Media
} // namespace OHOS