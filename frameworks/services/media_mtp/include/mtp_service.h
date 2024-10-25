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
#ifndef FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_SERVICE_H_
#define FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_SERVICE_H_
#include <mutex>
#include "mtp_monitor.h"
#define MTP_API_EXPORT __attribute__ ((visibility ("default")))
namespace OHOS {
namespace Media {
class MtpService {
public:
    ~MtpService() = default;
    MTP_API_EXPORT static std::shared_ptr<MtpService> GetInstance();
    MTP_API_EXPORT void StartService();
    MTP_API_EXPORT void StopService();

private:
    MtpService();
    void Init();

    static std::shared_ptr<MtpService> mtpServiceInstance_;
    static std::mutex instanceLock_;
    std::shared_ptr<MtpMonitor> monitorPtr_;
    bool isMonitorRun_;
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_SERVICE_H_