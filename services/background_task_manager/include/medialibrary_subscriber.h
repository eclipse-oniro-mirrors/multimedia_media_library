/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#ifndef MEDIALIBRARY_SUBSCRIBER_H
#define MEDIALIBRARY_SUBSCRIBER_H

#include <atomic>
#include <future>
#include <thread>

#include "common_event_manager.h"
#include "common_event_subscribe_info.h"
#include "common_event_subscriber.h"
#include "common_event_support.h"
#include "matching_skills.h"
#include "medialibrary_async_worker.h"
#include "media_background_task_factory.h"
#include "datashare_helper.h"
#include "datashare_observer.h"
#include "cloud_sync_utils.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
enum class StatusEventType {
    CHARGING,
    DISCHARGING,
    POWER_CONNECTED,
    POWER_DISCONNECTED,
    SCREEN_OFF,
    SCREEN_ON,
    BATTERY_CHANGED,
    THERMAL_LEVEL_CHANGED,
    TIME_TICK
};

static const std::string CLOUD_DATASHARE_URI = "datashareproxy://com.huawei.hmos.clouddrive";
static const std::string CLOUD_URI = CLOUD_DATASHARE_URI + "/cloud_sp?key=useMobileNetworkData";

class MedialibrarySubscriber;

#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
class CloudMediaAssetUnlimitObserver : public DataShare::DataShareObserver {
public:
    CloudMediaAssetUnlimitObserver(std::weak_ptr<MedialibrarySubscriber> subscriber) : subscriber_(subscriber) {}
    ~CloudMediaAssetUnlimitObserver() {}
    void OnChange(const ChangeInfo &changeInfo) override;

private:
    std::weak_ptr<MedialibrarySubscriber> subscriber_;
};
#endif

class EXPORT MedialibrarySubscriber : public EventFwk::CommonEventSubscriber {
public:
    class DelayTask {
    public:
        DelayTask(std::string taskName) : taskName(std::move(taskName)) {}
        ~DelayTask() { EndBackgroundOperationThread();}
        bool IsDelayTaskTimeOut();
        void EndBackgroundOperationThread();
        void SetOperationThread(std::function<void()> operationTask);
    private:
        std::string taskName;
        int64_t delayTime;
        std::mutex lock;
        std::condition_variable cv;
        bool isTaskWaiting{false};
        std::thread operationThread;
    };
    EXPORT MedialibrarySubscriber() = default;
    EXPORT explicit MedialibrarySubscriber(const EventFwk::CommonEventSubscribeInfo &subscriberInfo);
    EXPORT static void SubscribeAsync(void);
    EXPORT static bool UnSubscribe();
    EXPORT virtual ~MedialibrarySubscriber();

    EXPORT virtual void OnReceiveEvent(const EventFwk::CommonEventData &eventData) override;
    EXPORT static bool IsCellularNetConnected();
    EXPORT static bool IsWifiConnected();
    EXPORT static bool IsCurrentStatusOn();
    EXPORT static void RefreshCellularNetStatus();
private:
    std::shared_ptr<DataShare::DataShareHelper> cloudHelper_;
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    std::shared_ptr<CloudMediaAssetUnlimitObserver> CloudMediaAssetUnlimitObserver_;
#endif
    static const std::vector<std::string> events_;
    bool isScreenOff_ {false};
    bool isCharging_ {false};
    bool isDeviceTemperatureProper_{false};
    static bool isWifiConnected_;
    static bool currentStatus_;
    bool thumbnailBgGenerationStatus_{false};
    bool checkInLakeStatus_{false};
    bool timerStatus_{false};
    static bool isCellularNetConnected_;
    std::mutex mutex_;
    int32_t agingCount_ {0};
    int32_t deviceTemperatureLevel_ {0};
    int32_t newTemperatureLevel_ {0};
    int32_t batteryCapacity_ {0};
    int64_t lockTime_ {0};
    Background::MediaBackgroundTaskFactory backgroundTaskFactory_;
    static std::shared_ptr<MedialibrarySubscriber> subscriber_;
    static std::future<bool> subscribeAsyncTask_;
    static std::mutex subscribeLock_;
    static std::mutex subscribeAsyncTaskLock_;

    DelayTask backgroundDelayTask_{"backgroundTask"};
    DelayTask thumbnailBgDelayTask_{"thumbnailBgTask"};
    EXPORT static bool Subscribe(void);
    EXPORT void ClearDirtyData();
    EXPORT void DoBackgroundOperation();
    EXPORT void DoBackgroundOperationStepTwo();
    EXPORT void DoThumbnailBgOperation();
    EXPORT void StopBackgroundOperation();
    EXPORT void StopThumbnailBgOperation();
    EXPORT void StartAnalysisService();
    EXPORT void AgingTmpCompatibleDuplicates(bool isAge);
    EXPORT static void ClearContinueCloneData(AsyncTaskData *data);
    EXPORT int32_t DoClearContinueCloneData();
    EXPORT bool IsClearContinueCloneData(const std::string &path);
    EXPORT bool TryClearContinueCloneData();
    EXPORT std::string GetDataCloneDescriptionJsonPath();
    EXPORT bool GetCloneTimestamp(const std::string &path, int64_t &cloneTimestamp);
    EXPORT void WalCheckPointAsync();
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    EXPORT void HandleBatchDownloadWhenNetChange();
#endif

#ifdef MEDIALIBRARY_MTP_ENABLE
    void DoStartMtpService();
#endif
    void RevertPendingByPackage(const std::string &bundleName);
    int64_t GetNowTime();
    void Init();
    bool GetPowerConnected();
    void UpdateBackgroundOperationStatus(const AAFwk::Want &want, const StatusEventType statusEventType);
    void UpdateCloudMediaAssetDownloadStatus(const AAFwk::Want &want, const StatusEventType statusEventType);
    void UpdateCurrentStatus();
    void UpdateThumbnailBgGenerationStatus();
    void UpdateMediaInLakeCheckStatus();
    void CheckHalfDayMissions();
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    void UpdateBackgroundTimer();
#endif
    void DoAgingOperation();
    void DealWithEventsAfterUpdateStatus(const StatusEventType statusEventType);
    void UploadDB();
    void ResetCloneFlagAfterOneDay();
};
}  // namespace Media
}  // namespace OHOS

#endif // MEDIALIBRARY_SUBSCRIBER_H
