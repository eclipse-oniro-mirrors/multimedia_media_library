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

#ifndef OHOS_MEDIA_DFX_MANAGER_H
#define OHOS_MEDIA_DFX_MANAGER_H

#include <mutex>
#include <string>

#include "dfx_collector.h"
#include "dfx_analyzer.h"
#include "dfx_reporter.h"

#include "ipc_skeleton.h"
#include "dfx_worker.h"
#include "dfx_cloud_const.h"

namespace OHOS {
namespace Media {

struct DeleteBehaviorData {
    std::map<std::string, std::string> displayNames;
    std::map<std::string, std::string> albumNames;
    std::map<std::string, std::string> ownerAlbumIds;
};

enum class MultiStagesCaptureVideoStatus {
    HIGH_QUALITY = 0,
    LOW_QUALITY,
};

class DeleteBehaviorTask : public DfxData {
public:
    DeleteBehaviorTask(std::string id, int32_t type, int32_t size, std::vector<std::string> &uris,
        std::shared_ptr<DfxReporter> dfxReporter,
        const DeleteBehaviorData &deleteBehaviorData = {}) : id_(id), type_(type),  size_(size), uris_(uris),
        dfxReporter_(dfxReporter), deleteBehaviorData_(deleteBehaviorData) {}
    virtual ~DeleteBehaviorTask() override = default;
    std::string id_;
    int32_t type_;
    int32_t size_;
    std::vector<std::string> uris_;
    std::shared_ptr<DfxReporter> dfxReporter_;
    DeleteBehaviorData deleteBehaviorData_;
};

class StatisticData : public DfxData {
public:
    StatisticData(std::shared_ptr<DfxReporter> dfxReporter) : dfxReporter_(dfxReporter) {}
    virtual ~StatisticData() override = default;
    std::shared_ptr<DfxReporter> dfxReporter_;
};

class DfxManager {
public:
    DfxManager();
    ~DfxManager();

    EXPORT static std::shared_ptr<DfxManager> GetInstance();
    void HandleControllerServiceError(uint32_t operationCode, int32_t errorCode);
    void HandleTimeOutOperation(std::string &bundleName, int32_t type, int32_t object, int32_t time);
    int32_t HandleHighMemoryThumbnail(std::string &path, int32_t mediaType, int32_t width, int32_t height);
    void HandleThumbnailError(const std::string &path, int32_t method, int32_t errCode);
    void HandleThumbnailGeneration(const ThumbnailData::GenerateStats &stats);
    void HandleFiveMinuteTask();
    int64_t HandleMiddleReport();
    int64_t HandleOneDayReport();
    void HandleCommonBehavior(std::string bundleName, int32_t type);
    void HandleDeleteBehavior(int32_t type, int32_t size, std::vector<std::string> &uris, std::string bundleName = "",
        const DeleteBehaviorData &deleteBehaviorData = {});
    void HandleDeleteBehaviors();
    void HandleNoPermmison(int32_t type, int32_t object, int32_t error);
    void HandleHalfDayMissions();
    void HandleTwoDayMissions();
    void HandleAdaptationToMovingPhoto(const std::string &appName, bool adapted);
    void IsDirectoryExist(const std::string &dirName);
    void CheckStatus();
    void HandleSyncStart(const std::string& taskId, const int32_t syncReason = 1);
    void HandleUpdateMetaStat(uint32_t index, uint64_t diff, uint32_t syncType = 0);
    void HandleUpdateAttachmentStat(uint32_t index, uint64_t diff);
    void HandleUpdateAlbumStat(uint32_t index, uint64_t diff);
    void HandleUpdateUploadMetaStat(uint32_t index, uint64_t diff);
    void HandleUpdateUploadDetailError(int32_t error);
    void HandleSyncEnd(const int32_t stopReason = 0);
    void HandleReportSyncFault(const std::string& position, const SyncFaultEvent& event);
    void HandleOneWeekMissions();
    EXPORT void HandleTranscodeAccessTime(const TranscodeAccessType type);
    EXPORT void HandleTranscodeFailed(const TranscodeErrorType type);
    EXPORT void HandleTranscodeCostTime(const int32_t costTime);
    void HandleAccurateRefreshTimeOut(const AccurateRefreshDfxDataPoint& reportData);
    void HandleCinematicVideoAccessTimes(bool isByUri, bool isHighQualityRequest, const std::string &fileId = "");
    void HandleCinematicVideoAddStartTime(const CinematicWaitType waitType, const std::string &videoId);
    void HandleCinematicVideoAddEndTime(const CinematicWaitType waitType, const std::string &videoId);
    void HandleCinematicVideoMultistageResult(bool multistageResult);
    void HandleUpgradeFault(const UpgradeExceptionInfo& reportData);
    int32_t HandleThmInodeCleanInfo(const ThmInodeCleanInfo &info);

private:
    void Init();
    void HandleAlbumInfoBySubtype(int32_t albumSubType);
    void ResetStatistic();
    void CopyStatistic(CloudSyncStat& stat);

private:
    static std::mutex instanceLock_;
    static std::shared_ptr<DfxManager> dfxManagerInstance_;
    std::atomic<bool> isInitSuccess_;
    std::shared_ptr<DfxCollector> dfxCollector_;
    std::shared_ptr<DfxAnalyzer> dfxAnalyzer_;
    std::shared_ptr<DfxReporter> dfxReporter_;
    std::shared_ptr<DfxWorker> dfxWorker_;

    std::vector<std::atomic<uint64_t>> downloadMeta_;
    std::vector<std::atomic<uint64_t>> uploadMeta_;
    std::vector<std::atomic<uint64_t>> downloadThumb_;
    std::vector<std::atomic<uint64_t>> downloadLcd_;
    std::vector<std::atomic<uint64_t>> uploadAlbum_;
    std::vector<std::atomic<uint64_t>> downloadAlbum_;
    std::vector<std::atomic<uint64_t>> updateDetails_;
    std::vector<std::atomic<uint64_t>> uploadMetaErr_;
    CloudSyncInfo syncInfo_;
    std::string taskId_;
};
} // namespace Media
} // namespace OHOS

#endif  // OHOS_MEDIA_DFX_MANAGER_H