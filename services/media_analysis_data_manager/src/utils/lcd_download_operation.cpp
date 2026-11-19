/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "LcdDownloadOperation"

#include "lcd_download_operation.h"

#include "media_log.h"
#include "medialibrary_errno.h"
#include "net_conn_client.h"
#include "cloud_sync_manager.h"
#include "analysis_lcd_aging_dao.h"
#include "analysis_lcd_download_callback.h"
#include "media_file_uri.h"

using namespace OHOS::NetManagerStandard;
using namespace OHOS::FileManagement::CloudSync;

namespace OHOS::Media {

std::shared_ptr<LcdDownloadOperation> LcdDownloadOperation::instance_ = nullptr;
std::mutex LcdDownloadOperation::instanceMutex_;

std::shared_ptr<LcdDownloadOperation> LcdDownloadOperation::GetInstance()
{
    if (instance_ == nullptr) {
        std::lock_guard<std::mutex> lock(instanceMutex_);
        if (instance_ == nullptr) {
            instance_ = std::make_shared<LcdDownloadOperation>();
            MEDIA_INFO_LOG("create cloud media asset task.");
        }
    }
    return instance_;
}

LcdDownloadOperation::LcdDownloadOperation()
{
    MEDIA_INFO_LOG("LcdDownloadOperation constructed");
}

LcdDownloadStatus LcdDownloadOperation::GetLcdDownloadStatus()
{
    return downloadStatus_;
}

int32_t LcdDownloadOperation::StartDownload(const std::vector<int64_t> &fileIds, uint32_t netBearerBitmap)
{
    MEDIA_INFO_LOG("StartDownload called, fileIds.size()=%{public}zu, netBearerBitmap=%{public}u",
        fileIds.size(), netBearerBitmap);

    if (downloadStatus_ == LcdDownloadStatus::DOWNLOADING) {
        MEDIA_WARN_LOG("StartDownload: already downloading");
        return E_ERR;
    }

    fileIds_ = fileIds;
    downloadResults_.clear();
    pathToFileIdMap_.clear();

    requiredNetBearerBitmap_.store(netBearerBitmap);

    downloadCallback_ = std::make_shared<AnalysisLcdDownloadCallback>(instance_);
    CHECK_AND_RETURN_RET_LOG(downloadCallback_ != nullptr, E_ERR, "downloadCallback_ is null.");

    int32_t ret = RegisterNetObserver();
    if (ret != E_OK) {
        MEDIA_ERR_LOG("StartDownload: failed to register net observer, ret=%{public}d", ret);
        return E_ERR;
    }
    downloadStatus_ = LcdDownloadStatus::DOWNLOADING;
    SubmitDownloadToCloudSync();
    return E_OK;
}

int32_t LcdDownloadOperation::PauseDownload()
{
    MEDIA_INFO_LOG("PauseDownload called");

    std::lock_guard<std::mutex> lock(mutex_);

    if (downloadStatus_ != LcdDownloadStatus::DOWNLOADING) {
        MEDIA_WARN_LOG("PauseDownload: not downloading");
        return E_ERR;
    }

    PauseDownloadTask();
    return E_OK;
}

int32_t LcdDownloadOperation::ResumeDownload()
{
    MEDIA_INFO_LOG("ResumeDownload called");

    std::lock_guard<std::mutex> lock(mutex_);

    if (downloadStatus_ != LcdDownloadStatus::PAUSED) {
        MEDIA_WARN_LOG("ResumeDownload: not paused");
        return E_ERR;
    }

    ResumeDownloadTask();
    return E_OK;
}

int32_t LcdDownloadOperation::CancelDownload()
{
    MEDIA_INFO_LOG("CancelDownload called");

    std::lock_guard<std::mutex> lock(mutex_);

    if (downloadStatus_ == LcdDownloadStatus::IDLE) {
        MEDIA_WARN_LOG("CancelDownload: already idle");
        return E_ERR;
    }

    PauseDownloadTask();
    UnregisterNetObserver();
    fileIds_.clear();
    downloadResults_.clear();
    downloadCallback_ = nullptr;
    downloadStatus_ = LcdDownloadStatus::IDLE;
    requiredNetBearerBitmap_.store(0);
    pathToFileIdMap_.clear();
    downloadIds_.Clear();

    return E_OK;
}

int32_t LcdDownloadOperation::RegisterNetObserver()
{
    OHOS::sptr<AnalysisNetConnectObserver> observer;
    {
        std::lock_guard<std::mutex> lock(netObserverMutex_);
        if (netObserver_ != nullptr) {
            MEDIA_INFO_LOG("RegisterNetObserver: already registered");
            return E_OK;
        }
        netObserver_ = new (std::nothrow) AnalysisNetConnectObserver();
        CHECK_AND_RETURN_RET_LOG(netObserver_ != nullptr, E_ERR, "Failed to create netObserver_");
        observer = netObserver_;
    }
    observer->SetRequiredNetBearerBitmap(requiredNetBearerBitmap_.load());

    int32_t ret = NetConnClient::GetInstance().RegisterNetConnCallback(observer);
    CHECK_AND_RETURN_RET_LOG(ret == NETMANAGER_SUCCESS, E_ERR,
        "RegisterNetObserver failed, ret=%{public}d", ret);
    
    MEDIA_INFO_LOG("RegisterNetObserver: success, requiredNetBearerBitmap_=%{public}u",
        requiredNetBearerBitmap_.load());
    return E_OK;
}

void LcdDownloadOperation::UnregisterNetObserver()
{
    OHOS::sptr<AnalysisNetConnectObserver> observer;
    {
        std::lock_guard<std::mutex> lock(netObserverMutex_);
        if (netObserver_ == nullptr) {
            return;
        }
        observer = netObserver_;
        netObserver_ = nullptr;
    }
    NetConnClient::GetInstance().UnregisterNetConnCallback(observer);
}

void LcdDownloadOperation::PauseDownloadTask()
{
    MEDIA_INFO_LOG("PauseDownloadTask");

    downloadStatus_ = LcdDownloadStatus::PAUSED;

    // 调用 CloudSyncManager::StopFileCache 暂停所有批次的下载
    auto downloadIds = downloadIds_.ToVector();
    for (auto downloadId : downloadIds) {
        if (downloadId != -1) {
            CloudSyncManager::GetInstance().StopFileCache(downloadId, false);
            MEDIA_INFO_LOG("PauseDownloadTask: stop file cache, downloadId=%{public}" PRId64, downloadId);
        }
    }
    downloadIds_.Clear();
}

void LcdDownloadOperation::ResumeDownloadTask()
{
    MEDIA_INFO_LOG("ResumeDownloadTask");

    downloadStatus_ = LcdDownloadStatus::DOWNLOADING;
    SubmitDownloadToCloudSync();
}

void LcdDownloadOperation::SubmitDownloadToCloudSync()
{
    MEDIA_INFO_LOG("SubmitDownloadToCloudSync: fileIds.size()=%{public}zu", fileIds_.size());

    std::vector<std::string> uriVec;
    if (!PrepareDownloadUris(uriVec)) {
        return;
    }

    SubmitBatchesAndWait(uriVec);
}

bool LcdDownloadOperation::PrepareDownloadUris(std::vector<std::string> &uriVec)
{
    std::vector<AnalysisData::DownloadLcdFileInfo> downloadInfos;
    int32_t ret = AnalysisData::AnalysisLcdAgingDao().QueryDownloadLcdInfo(fileIds_, downloadInfos);
    if (ret != E_OK || downloadInfos.empty()) {
        MEDIA_ERR_LOG("PrepareDownloadUris: query download info failed, ret=%{public}d", ret);
        for (auto fileId : fileIds_) {
            OnDownloadComplete(fileId, false);
        }
        return false;
    }

    for (const auto &info : downloadInfos) {
        std::string fileUri = MediaFileUri::GetPhotoUri(std::to_string(info.fileId),
            info.filePath, info.fileName);
        if (!fileUri.empty()) {
            uriVec.push_back(fileUri);
            pathToFileIdMap_[fileUri] = info.fileId;
        }
    }

    if (uriVec.empty()) {
        MEDIA_ERR_LOG("PrepareDownloadUris: no valid URIs");
        for (auto fileId : fileIds_) {
            OnDownloadComplete(fileId, false);
        }
        return false;
    }

    return true;
}

void LcdDownloadOperation::SubmitBatchesAndWait(const std::vector<std::string> &uriVec)
{
    downloadIds_.Clear();
    const int32_t BATCH_SIZE = 100;
    int32_t totalBatches = (uriVec.size() + BATCH_SIZE - 1) / BATCH_SIZE;
    MEDIA_INFO_LOG("SubmitBatchesAndWait: total URIs=%{public}zu, batches=%{public}d",
        uriVec.size(), totalBatches);
    bool allSuccess = true;
    for (int32_t i = 0; i < totalBatches; ++i) {
        auto startIt = uriVec.begin() + i * BATCH_SIZE;
        auto endIt = (i == totalBatches - 1) ? uriVec.end() : uriVec.begin() + (i + 1) * BATCH_SIZE;
        std::vector<std::string> batchUriVec(startIt, endIt);
        int64_t downloadId = -1;
        int32_t ret = CloudSyncManager::GetInstance().StartFileCache(batchUriVec, downloadId,
            FieldKey::FIELDKEY_LCD, downloadCallback_);
        if (ret != E_OK || downloadId == -1) {
            MEDIA_ERR_LOG("SubmitBatchesAndWait: batch %{public}d StartFileCache failed, ret=%{public}d",
                i, ret);
            allSuccess = false;
            HandleBatchFailure(batchUriVec);
        } else {
            downloadIds_.PushBack(downloadId);
            MEDIA_INFO_LOG("SubmitBatchesAndWait: batch %{public}d started, downloadId=%{public}" PRId64
                ", size=%{public}zu", i, downloadId, batchUriVec.size());
        }
    }
    if (!allSuccess || downloadIds_.Empty()) {
        MEDIA_ERR_LOG("SubmitBatchesAndWait: some batches failed or no valid downloadIds");
        return;
    }
    std::unique_lock<std::mutex> lock(mutex_);
    bool notified = cv_.wait_for(lock, std::chrono::minutes(3), [this]() {
        return downloadResults_.size() >= fileIds_.size() || downloadStatus_ != LcdDownloadStatus::DOWNLOADING;
    });
    if (!notified && downloadStatus_ == LcdDownloadStatus::DOWNLOADING) {
        MEDIA_WARN_LOG("SubmitBatchesAndWait: timeout, marking unfinished downloads as failed");
        HandleTimeout();
    }
    
    MEDIA_INFO_LOG("SubmitBatchesAndWait: all callbacks completed, results.size()=%{public}zu",
        downloadResults_.size());
}

void LcdDownloadOperation::HandleBatchFailure(const std::vector<std::string> &batchUriVec)
{
    for (const auto &uri : batchUriVec) {
        auto it = pathToFileIdMap_.find(uri);
        if (it != pathToFileIdMap_.end()) {
            OnDownloadComplete(it->second, false);
        }
    }
}

void LcdDownloadOperation::HandleTimeout()
{
    for (auto fileId : fileIds_) {
        if (downloadResults_.find(fileId) == downloadResults_.end()) {
            OnDownloadComplete(fileId, false);
        }
    }
}

void LcdDownloadOperation::OnDownloadComplete(int64_t fileId, bool success)
{
    downloadResults_[fileId] = success;

    // 如果所有文件都处理完成，通知回调
    if (downloadResults_.size() == fileIds_.size()) {
        downloadStatus_ = LcdDownloadStatus::IDLE;
        UnregisterNetObserver();
        pathToFileIdMap_.clear();
    }
}

std::map<int64_t, bool> LcdDownloadOperation::GetDownloadResults() const
{
    return downloadResults_;
}

uint32_t LcdDownloadOperation::GetCurrentNetBearerBitmap() const
{
    return currentNetBearerBitmap_.load();
}

void LcdDownloadOperation::HandleCallback(const std::string &uri, bool success)
{
    MEDIA_INFO_LOG("HandleCallback: uri=%{public}s, success=%{public}d", uri.c_str(), success);

    std::lock_guard<std::mutex> lock(mutex_);

    // 根据 URI 查找对应的 fileId
    auto it = pathToFileIdMap_.find(uri);
    if (it == pathToFileIdMap_.end()) {
        MEDIA_WARN_LOG("HandleCallback: uri not found in pathToFileIdMap");
        return;
    }

    int64_t fileId = it->second;
    downloadResults_[fileId] = success;

    MEDIA_INFO_LOG("HandleCallback: fileId=%{public}" PRId64 ", success=%{public}d, completed=%{public}zu/%{public}zu",
        fileId, success, downloadResults_.size(), fileIds_.size());

    if (downloadResults_.size() >= fileIds_.size()) {
        downloadStatus_ = LcdDownloadStatus::IDLE;
        cv_.notify_all();
        MEDIA_INFO_LOG("HandleCallback: all downloads completed, notified");
    }
}

void LcdDownloadOperation::HandleSuccessCallback(const DownloadProgressObj &progress)
{
    HandleCallback(progress.path, true);
}

void LcdDownloadOperation::HandleFailedCallback(const DownloadProgressObj &progress)
{
    HandleCallback(progress.path, false);
}

void LcdDownloadOperation::HandleStoppedCallback(const DownloadProgressObj &progress)
{
    HandleCallback(progress.path, false);
}
}  // namespace OHOS::Media