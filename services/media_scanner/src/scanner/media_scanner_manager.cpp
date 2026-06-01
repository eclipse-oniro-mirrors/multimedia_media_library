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

#define MLOG_TAG "MediaScannerManager"

#include "media_scanner_manager.h"

#include "directory_ex.h"

#include "media_log.h"
#include "medialibrary_errno.h"
#include "media_scanner_db.h"
#include "scan_config_builder.h"

namespace OHOS {
namespace Media {
std::shared_ptr<MediaScannerManager> MediaScannerManager::instance_ = nullptr;
std::mutex MediaScannerManager::instanceMutex_;

MediaScannerManager::MediaScannerManager()
{
    enhancedExecutor_ = std::make_shared<EnhancedScanExecutor>();
    MEDIA_INFO_LOG("EnhancedScanExecutor created in constructor");
}

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
    MEDIA_DEBUG_LOG("scan file %{public}s, api%{public}d",
        MediaFileUtils::DesensitizePath(path).c_str(), static_cast<int>(api));

    string realPath;
    if (!PathToRealPath(path, realPath)) {
        MEDIA_ERR_LOG("failed to get real path %{public}s, errno %{public}d",
            MediaFileUtils::DesensitizePath(path).c_str(), errno);
        return E_INVALID_PATH;
    }

    if (!ScannerUtils::IsRegularFile(realPath)) {
        MEDIA_ERR_LOG("the path %{public}s is not a regular file",
            MediaFileUtils::DesensitizePath(realPath).c_str());
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
    MEDIA_DEBUG_LOG("scan file %{public}s, api%{public}d",
        MediaFileUtils::DesensitizePath(path).c_str(), static_cast<int>(api));

    string realPath;
    if (!PathToRealPath(path, realPath)) {
        MEDIA_ERR_LOG("failed to get real path %{public}s, errno %{public}d",
            MediaFileUtils::DesensitizePath(path).c_str(), errno);
        return E_INVALID_PATH;
    }

    if (!ScannerUtils::IsRegularFile(realPath)) {
        MEDIA_ERR_LOG("the path %{public}s is not a regular file",
            MediaFileUtils::DesensitizePath(realPath).c_str());
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
    MEDIA_DEBUG_LOG("scan dir %{public}s", MediaFileUtils::DesensitizePath(path).c_str());

    string realPath;
    if (!PathToRealPath(path, realPath)) {
        MEDIA_ERR_LOG("failed to get real path %{public}s, errno %{public}d",
            MediaFileUtils::DesensitizePath(path).c_str(), errno);
        return E_INVALID_PATH;
    }

    if (!ScannerUtils::IsDirectory(realPath)) {
        MEDIA_ERR_LOG("the path %{public}s is not a directory",
            MediaFileUtils::DesensitizePath(realPath).c_str());
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
    MEDIA_DEBUG_LOG("scan dir %{public}s", MediaFileUtils::DesensitizePath(path).c_str());

    string realPath;
    if (!PathToRealPath(path, realPath)) {
        MEDIA_ERR_LOG("failed to get real path %{public}s, errno %{public}d",
            MediaFileUtils::DesensitizePath(path).c_str(), errno);
        return E_INVALID_PATH;
    }

    if (!ScannerUtils::IsDirectory(realPath)) {
        MEDIA_ERR_LOG("the path %{public}s is not a directory",
            MediaFileUtils::DesensitizePath(realPath).c_str());
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

    if (enhancedExecutor_ != nullptr) {
        enhancedExecutor_->Stop();
    }

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

int32_t MediaScannerManager::ScanSync(const ScanConfig &config)
{
    MEDIA_INFO_LOG("scan file sync, path %{public}s, fileId %{public}d",
        MediaFileUtils::DesensitizePath(config.GetFilePath()).c_str(), config.GetFileId());

    auto context = PrepareValidatedContext(config, ScanExecutionMode::SYNC);
    if (context == nullptr) {
        MEDIA_ERR_LOG("prepare context failed");
        return E_INVALID_PATH;
    }

    if (enhancedExecutor_ == nullptr) {
        MEDIA_ERR_LOG("enhancedExecutor is null");
        return E_ERR;
    }

    ScanSubmitResult submitResult = enhancedExecutor_->Submit(context);
    if (submitResult == ScanSubmitResult::REJECTED) {
        MEDIA_ERR_LOG("task rejected");
        return E_ERR;
    }

    if (submitResult == ScanSubmitResult::WAITING) {
        enhancedExecutor_->WaitForSyncScanCompletion(context->config.GetFileId());
        MEDIA_INFO_LOG("waiting completed (fileId %{public}d)", context->config.GetFileId());
        return E_OK;
    }

    enhancedExecutor_->StartSync(context);
    MEDIA_INFO_LOG("completed (fileId %{public}d)", context->config.GetFileId());
    return E_OK;
}

int32_t MediaScannerManager::ScanAsync(const ScanConfig &config)
{
    MEDIA_INFO_LOG("scan file async, path %{public}s, fileId %{public}d",
        MediaFileUtils::DesensitizePath(config.GetFilePath()).c_str(), config.GetFileId());
    
    auto context = PrepareValidatedContext(config, ScanExecutionMode::ASYNC);
    if (context == nullptr) {
        MEDIA_ERR_LOG("prepare context failed");
        return E_INVALID_PATH;
    }

    if (enhancedExecutor_ == nullptr) {
        MEDIA_ERR_LOG("enhancedExecutor is null");
        return E_ERR;
    }

    ScanSubmitResult submitResult = enhancedExecutor_->Submit(context);
    if (submitResult == ScanSubmitResult::REJECTED) {
        MEDIA_ERR_LOG("task rejected");
        return E_ERR;
    }

    if (submitResult == ScanSubmitResult::WAITING) {
        MEDIA_INFO_LOG("merged to existing task (fileId %{public}d)",
            context->config.GetFileId());
        return E_OK;
    }

    enhancedExecutor_->StartAsync();
    MEDIA_INFO_LOG("submitted (fileId %{public}d, result %{public}d)",
        context->config.GetFileId(), static_cast<int32_t>(submitResult));
    return E_OK;
}

std::shared_ptr<ScanTaskContext> MediaScannerManager::PrepareValidatedContext(const ScanConfig &config,
    ScanExecutionMode executionMode)
{
    std::string realPath;
    if (!config.Validate(realPath)) {
        return nullptr;
    }

    auto finalConfig = ScanConfigBuilder(config)
        .SetFilePath(realPath)
        .SetExecutionMode(executionMode)
        .Build();

    return std::make_shared<ScanTaskContext>(finalConfig);
}

} // namespace Media
} // namespace OHOS