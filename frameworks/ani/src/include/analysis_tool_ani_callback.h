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

#ifndef FRAMEWORKS_ANI_SRC_INCLUDE_ANALYSIS_TOOL_ANI_CALLBACK_H
#define FRAMEWORKS_ANI_SRC_INCLUDE_ANALYSIS_TOOL_ANI_CALLBACK_H

#include <cstdint>
#include <memory>

#include "active_analysis/active_analysis_callback.h"
#include "analysis_tool_ani_callback_holder.h"
#include "iremote_object.h"

namespace OHOS::Media {
class AnalysisToolAniCallbackStub final : public ActiveAnalysisCallbackStub {
public:
    explicit AnalysisToolAniCallbackStub(std::shared_ptr<AnalysisToolAniCallbackHolder> holder);

    int32_t OnToolFinished(const AnalysisToolCallbackResult &result) override;

private:
    std::shared_ptr<AnalysisToolAniCallbackHolder> holder_;
};

class AnalysisToolAniCallbackRegistry final {
public:
    static uint64_t Register(const std::shared_ptr<AnalysisToolAniCallbackHolder> &holder,
        const sptr<AnalysisToolAniCallbackStub> &callbackStub, const sptr<IRemoteObject> &callbackRemote);
    static void Unregister(uint64_t registryId);
};
} // namespace OHOS::Media

#endif // FRAMEWORKS_ANI_SRC_INCLUDE_ANALYSIS_TOOL_ANI_CALLBACK_H
