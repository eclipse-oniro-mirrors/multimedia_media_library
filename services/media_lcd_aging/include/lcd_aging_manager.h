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

#ifndef OHOS_MEDIA_LCD_AGING_MANAGER_H
#define OHOS_MEDIA_LCD_AGING_MANAGER_H

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>

#include "deep_optimize_space_callback.h"
#include "iremote_object.h"
#include "lcd_aging_dao.h"
#include "lcd_aging_worker.h"
#include "thumbnail_data.h"

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))

class EXPORT LcdAgingManager {
public:
    static LcdAgingManager& GetInstance();

    int32_t AnalysisRemoveCloudLcd(const std::vector<int64_t> &fileIds);
    void DelayLcdAgingTime();
    std::mutex& GetLcdOperationMutex();
    int32_t GenerateLcdWithLocal(const LcdAgingFileInfo &agingFileInfo);
    int32_t DeleteLocalFile(const std::string &localPath);

    int32_t StartDeepOptimizeSpace(const sptr<IRemoteObject> &clientRemote, const sptr<IRemoteObject> &callbackRemote);
    int32_t StopDeepOptimizeSpace();
    int32_t BatchAgingLcdFileTask(const std::atomic<bool> &shouldStop);
    bool GetIsActiveLcdAging();
    int32_t SetIsActiveLcdAging(bool isActive);

private:
    LcdAgingManager() {}
    ~LcdAgingManager() {}
    LcdAgingManager(const LcdAgingManager &manager) = delete;
    const LcdAgingManager &operator=(const LcdAgingManager &manager) = delete;

    using BatchAgingLcdFileFunc = int32_t (LcdAgingManager::*)(const int32_t size,
        const std::atomic<bool> &shouldStop);

    int32_t InitAgingTask(int64_t &taskSize);
    int32_t ExecuteAgingLoop(int64_t taskSize, const std::atomic<bool> &shouldStop);
    int32_t ExecuteSingleBatch(const int32_t batchSize, bool &hasTrashedData, const std::atomic<bool> &shouldStop);
    
    int32_t BatchAgingLcdFileTrashed(const int32_t size, const std::atomic<bool> &shouldStop);
    int32_t BatchAgingLcdFileNotTrashed(const int32_t size, const std::atomic<bool> &shouldStop);
    int32_t DoBatchAgingLcdFile(const std::vector<LcdAgingFileInfo> &lcdAgingFileInfoList,
        const std::atomic<bool> &shouldStop);
    int32_t DoBatchAgingLcdFileInternal(const std::vector<LcdAgingFileInfo> &lcdAgingFileInfoList,
        int64_t &agingSuccessSize, std::vector<std::string> &failFileIds);
    int32_t ReadyAgingLcd();
    void UpdateLastLcdAgingEndTime(const int64_t lastLcdAgingEndTime);
    std::vector<std::string> GetFileIdFromAgingFiles(const std::vector<LcdAgingFileInfo> &agingFileInfos);
    int32_t GetNeedAgingLcdSize(int64_t &taskSize);
    int32_t FinishAgingTask();
    ThumbnailData ConvertLcdAgingFileInfoToThumbnailData(const LcdAgingFileInfo &agingFileInfo);
    std::vector<std::string> GetFailedFileIds(const std::vector<LcdAgingFileInfo> &agingFileInfos,
        const std::vector<std::string> &failCloudIds);
    struct DeleteLcdFilesTask {
        std::vector<LcdAgingFileInfo> agingFileInfos;
        std::vector<std::string> failFileIds;
    };

    void AsyncDeleteLcdFiles(const std::vector<LcdAgingFileInfo> &lcdAgingFileInfoList,
        const std::vector<std::string> &failFileIds);
    void ProcessDeleteLcdFilesTasks();
    void ExecuteDeleteLcdFiles(const DeleteLcdFilesTask &task);

    int32_t CheckLcdAgingStatus(const std::atomic<bool> &shouldStop);
    void HandleAgingProgress();
    void HandleAfterAgingProgress(const int32_t errorCode);
    bool LoadIsActiveLcdAgingFromPrefs();
    void SaveIsActiveLcdAgingToPrefs(bool isActive);

private:
    LcdAgingDao lcdAgingDao_;
    std::mutex lcdOperationMutex_;
    std::atomic<bool> isCompletePull_ {true};
    std::vector<std::string> notAgingFileIds_;
    std::atomic<int64_t> hasAgingLcdNumber_ {0};
    std::atomic<int64_t> totalAgingLcdNumber_ {0};
    std::atomic<uint32_t> lastAgingProgress_ {0};
    std::atomic<int32_t> isActiveLcdAging_{-1};

    std::queue<DeleteLcdFilesTask> deleteLcdFilesTaskQueue_;
    std::mutex deleteLcdFilesTaskMutex_;
    std::condition_variable deleteLcdFilesTaskCv_;
    std::atomic<bool> isDeleteLcdFilesWorkerRunning_ {false};
    int64_t freeSizeOld_{0};
    int64_t startTime_{0};
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_LCD_AGING_MANAGER_H