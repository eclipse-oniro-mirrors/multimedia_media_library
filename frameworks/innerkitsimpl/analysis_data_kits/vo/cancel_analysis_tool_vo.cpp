/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "cancel_analysis_tool_vo.h"
#include <sstream>
#include "media_log.h"
namespace OHOS::Media {
bool CancelAnalysisToolReqBody::Unmarshalling(MessageParcel &parcel)
{
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(taskId), false,
        "Failed to read cancel analysis tool taskId");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(param), false,
        "Failed to read cancel analysis tool param");
    return true;
}

bool CancelAnalysisToolReqBody::Marshalling(MessageParcel &parcel) const
{
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(taskId), false,
        "Failed to write cancel analysis tool taskId");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(param), false,
        "Failed to write cancel analysis tool param, size: %{public}zu", param.size());
    return true;
}

std::string CancelAnalysisToolReqBody::ToString() const
{
    std::stringstream ss;
    ss << "taskId=" << taskId << ", paramSize=" << param.size();
    return ss.str();
}

bool CancelAnalysisToolRespBody::Unmarshalling(MessageParcel &parcel)
{
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(result), false,
        "Failed to read cancel analysis tool result");
    return true;
}

bool CancelAnalysisToolRespBody::Marshalling(MessageParcel &parcel) const
{
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(result), false,
        "Failed to write cancel analysis tool result: %{public}d", result);
    return true;
}
std::string CancelAnalysisToolRespBody::ToString() const
{
    std::stringstream ss;
    ss << "result=" << result;
    return ss.str();
}
} // namespace OHOS::Media
