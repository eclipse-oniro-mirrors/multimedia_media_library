/*
* Copyright (C) 2026 Huawei Device Co., Ltd.
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef OHOS_MEDIA_DRIVE_KIT_SCADETAIL_H
#define OHOS_MEDIA_DRIVE_KIT_SCADETAIL_H
#include <map>
#include <string>

namespace OHOS::Media::CloudSync {
struct MDKScadetail {
    std::string usage; // 风控对应的类型
    int scaState; // 内容审核状态 0：待自动审核(默认值)1：审核完成 2：自动审核完成 3：待人工审核 4：投诉审核
    int scaRank; // 风控分级 0：无风险 1：低风险 2：中风险 3：高风险
    int64_t scaVersion; // 风控版本号
    std::map<std::string, std::string> scaAttributes;
    int riskResult;
};
} // namespace OHOS::Media::CloudSync

#endif
