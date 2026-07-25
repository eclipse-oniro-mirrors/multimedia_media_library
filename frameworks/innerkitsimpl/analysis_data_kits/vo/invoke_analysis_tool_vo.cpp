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

#include "invoke_analysis_tool_vo.h"
#include <sstream>
#include "media_log.h"
namespace OHOS::Media {
bool InvokeAnalysisToolReqBody::Unmarshalling(MessageParcel &parcel)
{
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(type), false,
        "Failed to read invoke analysis tool type");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(param), false,
        "Failed to read invoke analysis tool param");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(taskId), false,
        "Failed to read invoke analysis tool taskId");
    callbackRemote = parcel.ReadRemoteObject();
    CHECK_AND_RETURN_RET_LOG(callbackRemote != nullptr, false,
        "Failed to read invoke analysis tool callback remote");
    return true;
}

bool InvokeAnalysisToolReqBody::Marshalling(MessageParcel &parcel) const
{
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(type), false,
        "Failed to write invoke analysis tool type: %{public}d", type);
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(param), false,
        "Failed to write invoke analysis tool param, size: %{public}zu", param.size());
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(taskId), false,
        "Failed to write invoke analysis tool taskId");
    CHECK_AND_RETURN_RET_LOG(callbackRemote != nullptr, false,
        "Failed to marshalling invoke analysis tool, callbackRemote is null");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteRemoteObject(callbackRemote), false,
        "Failed to write invoke analysis tool callback remote");
    return true;
}

std::string InvokeAnalysisToolReqBody::ToString() const
{
    std::stringstream ss;
    ss << "type=" << type << ", paramSize=" << param.size() << ", taskId=" << taskId;
    return ss.str();
}

bool InvokeAnalysisToolRespBody::Unmarshalling(MessageParcel &parcel)
{
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(result), false,
        "Failed to read invoke analysis tool result");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(taskId), false,
        "Failed to read invoke analysis tool taskId");
    saRemote = parcel.ReadRemoteObject();
    CHECK_AND_RETURN_RET_LOG(saRemote != nullptr, false,
        "Failed to read invoke analysis tool saRemote");
    return true;
}

bool InvokeAnalysisToolRespBody::Marshalling(MessageParcel &parcel) const
{
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(result), false,
        "Failed to write invoke analysis tool result: %{public}d", result);
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(taskId), false,
        "Failed to write invoke analysis tool taskId");
    CHECK_AND_RETURN_RET_LOG(saRemote != nullptr, false,
        "Failed to marshalling invoke analysis tool resp, saRemote is null");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteRemoteObject(saRemote), false,
        "Failed to write invoke analysis tool saRemote");
    return true;
}

std::string InvokeAnalysisToolRespBody::ToString() const
{
    std::stringstream ss;
    ss << "result=" << result << ", taskId=" << taskId << ", saRemoteValid=" << (saRemote != nullptr);
    return ss.str();
}
} // namespace OHOS::Media