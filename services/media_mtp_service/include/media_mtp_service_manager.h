/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#ifndef OHOS_MEDIA_MTP_SERVICE_MANAGER_H
#define OHOS_MEDIA_MTP_SERVICE_MANAGER_H

#include <mutex>
#include <atomic>

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

enum class MtpMode {
    NONE_MODE,
    MTP_MODE,
    PTP_MODE
};

class MediaMtpServiceManager {
public:
    EXPORT virtual ~MediaMtpServiceManager() = default;
    EXPORT explicit MediaMtpServiceManager(void* handler);
    EXPORT static void StopMtpService();
    EXPORT static void StartMtpService(const MtpMode mode);
private:
    EXPORT static void NotifyRefreshStopWaitTime();
    EXPORT static void CloseDlopenByStop();
    EXPORT static void CloseLibrary();
private:
    static inline void* handler_ = nullptr;
    static inline std::atomic_bool isThreadRunning_ = false;
    static inline std::atomic_bool isTimerRefresh_ = false;
    static inline std::atomic_bool isNeedClose_ = false;
    static inline std::mutex mutex_;
    static inline std::condition_variable cv_;
};
} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIA_MTP_SERVICE_MANAGER_H