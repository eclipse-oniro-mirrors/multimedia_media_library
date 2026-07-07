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
#ifndef OHOS_MEDIA_ANALYSIS_TOOL_MANAGER_H
#define OHOS_MEDIA_ANALYSIS_TOOL_MANAGER_H

#include <memory>

#include "invoke_analysis_tool_dto.h"
#include "cancel_analysis_tool_dto.h"

namespace OHOS::Media::AnalysisData {
class AnalysisToolRemoteInvoker {
public:
    virtual ~AnalysisToolRemoteInvoker() = default;
    virtual sptr<IRemoteObject> GetSaRemote() const = 0;
    virtual int32_t InvokeAnalysisTool(const InvokeAnalysisToolDto &dto) const = 0;
    virtual int32_t CancelAnalysisTool(const CancelAnalysisToolDto &dto) const = 0;
};

class AnalysisToolManager {
private:
    AnalysisToolManager(std::shared_ptr<AnalysisToolRemoteInvoker> invoker);
public:
    static AnalysisToolManager &GetInstance();

    AnalysisToolManager(const AnalysisToolManager &) = delete;
    AnalysisToolManager &operator=(const AnalysisToolManager &) = delete;
    void SetInvoker(std::shared_ptr<AnalysisToolRemoteInvoker> invoker);

    int32_t SubmitTask(const InvokeAnalysisToolDto &dto, int32_t &resultCode,
        sptr<IRemoteObject> &saRemote, std::string &taskid);
    int32_t CancelTask(const CancelAnalysisToolDto &dto, int32_t &resultCode);
private:
    static bool IsSupportedType(int32_t type);
    std::shared_ptr<AnalysisToolRemoteInvoker> invoker_;
};
} // namespace OHOS::Media::AnalysisData
#endif // OHOS MEDIA ANALYSIS TOOL MANAGER H