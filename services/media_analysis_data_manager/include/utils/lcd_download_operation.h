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

#ifndef OHOS_LCD_DOWNLOAD_OPERATION_H
#define OHOS_LCD_DOWNLOAD_OPERATION_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "safe_vector.h"
#include "cloud_sync_common.h"
#include "analysis_lcd_download_callback.h"
#include "analysis_net_connect_observer.h"

namespace OHOS::Media {

enum class LcdDownloadStatus : int32_t {
    DOWNLOADING = 0,
    PAUSED,
    IDLE,
};

class LcdDownloadOperation {
public:
    LcdDownloadOperation();
    virtual ~LcdDownloadOperation() = default;
    EXPORT static std::shared_ptr<LcdDownloadOperation> GetInstance();
    int32_t StartDownload(const std::vector<int64_t> &fileIds, uint32_t netBearerBitmap);
    int32_t PauseDownload();
    int32_t ResumeDownload();
    int32_t CancelDownload();
    void HandleSuccessCallback(const DownloadProgressObj &progress);
    void HandleFailedCallback(const DownloadProgressObj &progress);
    void HandleStoppedCallback(const DownloadProgressObj &progress);
    LcdDownloadStatus GetLcdDownloadStatus();

    // 获取下载结果
    std::map<int64_t, bool> GetDownloadResults() const;
    // 获取当前网络类型 bitmap
    uint32_t GetCurrentNetBearerBitmap() const;

private:
    int32_t RegisterNetObserver();
    void UnregisterNetObserver();
    void PauseDownloadTask();
    void ResumeDownloadTask();
    void SubmitDownloadToCloudSync();
    void OnDownloadComplete(int64_t fileId, bool success);
    void HandleCallback(const std::string &uri, bool success);
    bool PrepareDownloadUris(std::vector<std::string> &uriVec);
    void SubmitBatchesAndWait(const std::vector<std::string> &uriVec);
    void HandleBatchFailure(const std::vector<std::string> &batchUriVec);
    void HandleTimeout();

public:
    static std::shared_ptr<LcdDownloadOperation> instance_;
    static std::mutex instanceMutex_;

private:
    LcdDownloadStatus downloadStatus_ = LcdDownloadStatus::IDLE;
    std::atomic<uint32_t> requiredNetBearerBitmap_{0};
    std::atomic<uint32_t> currentNetBearerBitmap_{0};

    std::vector<int64_t> fileIds_;
    std::map<int64_t, bool> downloadResults_;
    std::shared_ptr<AnalysisLcdDownloadCallback> downloadCallback_;

    OHOS::sptr<AnalysisNetConnectObserver> netObserver_;
    std::mutex netObserverMutex_;
    std::mutex mutex_;
    std::condition_variable cv_;

    SafeVector<int64_t> downloadIds_;
    std::map<std::string, int64_t> pathToFileIdMap_;  // URI -> fileId 映射
};
}  // namespace OHOS::Media

#endif  // OHOS_LCD_DOWNLOAD_OPERATION_H