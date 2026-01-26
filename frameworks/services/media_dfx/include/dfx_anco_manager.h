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

#ifndef OHOS_MEDIA_ANCO_MANAGER_H
#define OHOS_MEDIA_ANCO_MANAGER_H

#include <mutex>
#include <timer.h>

#include "dfx_const.h"
#include "dfx_reporter.h"

namespace OHOS {
namespace Media {

class AncoDfxManager {
public:
    ~AncoDfxManager();
    static AncoDfxManager& GetInstance();
    void ShutDownTimer();
    void RunDfx();
    void ReportFirstLoadInfo(uint64_t startTime, uint64_t endTime);
    void ReportAncoCheckInfo(const AncoCheckInfo& reportData);
    void ReportAncoOperationChangeInfo(const AncoOperationChangeInfo& reportData);
    void NotifyOperationChange(const int32_t objType, const int32_t optType);

private:
    AncoDfxManager();
    void StartTimer();
    void SetStartTime();
    void ResetStartTime();
    void InnerReportAncoCountFormatInfo(uint64_t loadStartTime = 0, uint64_t loadEndTime = 0,
        bool firstLoad = false);
    void InnerReportAndResetOptChangeInfo();
    void StartFormatCountTimer();
    void StartOptChangeInfoTimer();
    void RegisterFormatCountTimer();
    void RegisterOptChangeInfoTimer();
    std::mutex timerMutex_;

    Utils::Timer timer_{ "AncoDfxTimer" };
    uint32_t timerId_{ 0 };
    AncoOperationChangeInfo ancoOptChangeInfo_{
        0, 0, 0, 0, 0, 0, 0 };
    std::mutex ancoOptChangeInfoMutex_;
    int32_t NOTIFY_FILE_TYPE = 0;
    int32_t NOTIFY_DIR_TYPE = 1;
    int32_t NOTIFY_ADD_OPT_TYPE = 0;
    int32_t NOTIFY_MOD_OPT_TYPE = 1;
    int32_t NOTIFY_DEL_OPT_TYPE = 2;
    int32_t REPORT_OPT_CHANGE_INFO_INTERVAL = SIX_HOUR;
    int32_t REPORT_FORMAT_COUNT_INFO_INTERVAL = ONE_WEEK;
    std::string ANCO_DFX_START_TIME = "anco_dfx_start_time";
    std::string ANCO_OPT_CHANGE_LAST_REPORT_TIME = "anco_opt_change_info_last_report_time";
    std::string ANCO_FORMAT_COUNT_LAST_REPORT_TIME = "anco_foramt_count_info_last_report_time";
    std::string ANCO_FIRST_LOAD_FLAG = "anco_first_load_flag";
    bool alreadyRunDfx_ = false;
    bool alreadyRegisterOptChangeInfoTimer_ = false;
    bool alreadyRegisterFormatCountTimer_ = false;
};

} // Media
} // OHOS

#endif // OHOS_MEDIA_ANCO_MANAGER_H