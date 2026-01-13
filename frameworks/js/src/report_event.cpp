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

#include "report_event.h"

namespace OHOS {
namespace Media {

int64_t ReportEvent::beginTime_ = 0;
bool ReportEvent::isReport_ = false;
int64_t ReportEvent::processorId_ = DEFAULT_PROCESSOR_ID;

void ReportEvent::AddEventProcessor()
{
    HiviewDFX::HiAppEvent::ReportConfig config;
    config.name = "ha_app_event";  // 系统预制so，实现上报功能，由HA提供
    config.configName = "SDK_OCG"; // 固定内容，此配置内容由HA确认规格
    processorId_ = HiviewDFX::HiAppEvent::AppEventProcessorMgr::AddProcessor(config);
}

void ReportEvent::WriteCallStatusEvent()
{
    if (!isReport_) {
        return;
    }
    // 更新错误码列表
    for (auto it = errCodes_.begin(); it != errCodes_.end(); it++) {
        errorCodeTypes_.push_back(it->first);
        errorCodeNum_.push_back(it->second);
    }

    HiviewDFX::HiAppEvent::Event event("api_diagnostic", "api_called_stat", OHOS::HiviewDFX::HiAppEvent::BEHAVIOR);
    event.AddParam("api_name", apiName_);
    event.AddParam("sdk_name", std::string("MediaLibraryKit"));
    event.AddParam("begin_time", beginTime_);
    event.AddParam("call_times", callTimes_);
    event.AddParam("success_times", successTimes_);
    event.AddParam("max_cost_time", maxCostTime_);
    event.AddParam("min_cost_time", minCostTime_);
    event.AddParam("total_cost_time", totalCostTime_);
    event.AddParam("error_code_types", errorCodeTypes_);
    event.AddParam("error_code_num", errorCodeNum_);
    Write(event);
    ReportEvent::isReport_ = false;

    // 重置数据
    errorCodeTypes_.clear();
    errorCodeNum_.clear();
}

void ReportEvent::CountTimeAndNum(int64_t startTime, int64_t endTime, std::string errCode)
{
    int64_t runTime = endTime - startTime;
    if (runTime > maxCostTime_) {
        maxCostTime_ = runTime;
    }
    if (runTime < minCostTime_) {
        minCostTime_ = runTime;
    }
    totalCostTime_ += runTime;
    callTimes_++;
    if (errCode == "0") {  // 接口调用成功
        successTimes_++;
    } else {
        errCodes_[errCode]++;
    }
}
} // Media
} // OHOS