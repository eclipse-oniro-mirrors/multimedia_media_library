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

#ifndef REPORT_EVENT_H_
#define REPORT_EVENT_H_

#include <string>
#include <climits>
#include <map>
#include "app_event.h"
#include "app_event_processor_mgr.h"

namespace OHOS {
namespace Media {

enum ProcessorIdStatus : int64_t {
    DEFAULT_PROCESSOR_ID = -1,
    NON_APPLICATION_PROCESSOR_ID = -200
};

const int64_t TEN_MINUTE_MS = 10 * 60 * 1000;
class ReportEvent {
public:
    int32_t callTimes_ = 0;                       // 调用次数
    int32_t successTimes_ = 0;                    // 调用成功次数
    int64_t maxCostTime_ = 0;                     // 最大时延
    int64_t minCostTime_ = INT64_MAX;             // 最小时延
    int64_t totalCostTime_ = 0;                   // 累计时延
    std::vector<std::string> errorCodeTypes_;     // 错误码类型分布
    std::vector<int> errorCodeNum_;               // 错误码数量分布
    std::string apiName_;                         // 调用接口名
    std::map<std::string, int64_t> errCodes_;     // 统计错误码

    void WriteCallStatusEvent();
    void CountTimeAndNum(int64_t startTime, int64_t endTime, std::string errCode);

    static int64_t beginTime_;                    // 首次调用开始时间
    static bool isReport_;                        // 是否上报打点
    static int64_t processorId_;
    static void AddEventProcessor();
private:
};

} // namespace Media
} // namespace OHOS

#endif  // REPORT_EVENT_H_