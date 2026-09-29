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
#define MLOG_TAG "AnalysisToolManager"
#include "analysis_tool_manager.h"
#include <algorithm>
#include <thread>
#include "media_analysis_proxy.h"
#include "media_library_error_code.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "message_option.h"
#include "message_parcel.h"

namespace OHOS::Media::AnalysisData {
namespace {
constexpr int32_t MEDIA_ANALYSIS_SERVICE_SAID = 101120;
constexpr int32_t SUPPORTED_TOOL_TYPE_BEGIN = 0; //ANALYSIS_BASE_TOOL_TYPE
constexpr int32_t SUPPORTED_TOOL_TYPE_END = 14; //AI_SEARCH_TOOL_TYPE
constexpr size_t MAX_TOOL_PARAM_LENGTH = 16 * 1024; //16KB

bool IsValidParam(const std::string &param)
{
    return param.empty() || param.size() <= MAX_TOOL_PARAM_LENGTH;
}
struct InvokeAnalysisToolRequestView {
    const int32_t &type;
    const std::string &taskId;
    const std::string &param;
};
struct CancelAnalysisToolRequestView {
    const std::string &taskId;
    const std::string &param;
};
bool WriteInvokeAnalysisToolRequest(MessageParcel &data, MediaAnalysisProxy &proxy,
    const InvokeAnalysisToolRequestView &request, const char *operation)
{
    CHECK_AND_RETURN_RET_LOG(data.WriteInterfaceToken(proxy.GetDescriptor()), false,
        "Failed to write %{public}s analysis tool interface token", operation);
    CHECK_AND_RETURN_RET_LOG(data.WriteInt32(request.type), false,
        "Failed to write %{public}s analysis tool type%{public}d", operation, request.type);
    CHECK_AND_RETURN_RET_LOG(data.WriteString(request.param), false,
        "Failed to write %{public}s analysis tool param, size:%{public}zu", operation, request.param.size());
    CHECK_AND_RETURN_RET_LOG(data.WriteString(request.taskId), false,
        "Failed to write %{public}s analysis tool taskId", operation);
    return true;
}

bool WriteCancelAnalysisToolRequest(MessageParcel&data, MediaAnalysisProxy &proxy,
    const CancelAnalysisToolRequestView &request, const char *operation)
{
    CHECK_AND_RETURN_RET_LOG(data.WriteInterfaceToken(proxy.GetDescriptor()), false,
        "Failed to write %{public}s analysis tool cancel interface token", operation);
    CHECK_AND_RETURN_RET_LOG(data.WriteString(request.taskId), false,
        "Failed to write %{public}s analysis tool cancel taskId", operation);
    CHECK_AND_RETURN_RET_LOG(data.WriteString(request.param), false,
        "Failed to write %{public}s analysis tool cancel param, size: %{public}zu", operation, request.param.size());
    return true;
}

int32_t SendCancelAnalysisToolSync(const std::string &taskId, const std::string &param)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);
    MediaAnalysisProxy proxy(nullptr);
    const CancelAnalysisToolRequestView request {taskId, param};
    if (!WriteCancelAnalysisToolRequest(data, proxy, request, "cancel")) {
        MEDIA_ERR_LOG("Failed to write cancel analysis tool request");
        return E_ERR;
    }
    return MediaAnalysisProxy::SendToolTransactCmd(
        static_cast<int32_t>(IMediaAnalysisService::CANCEL_ANALYSIS_TOOL),
        data, reply, option);
}
} // namespace


class AnalysisToolMediaAnalysisRemoteInvoker final :public AnalysisToolRemoteInvoker {
public:
    sptr<IRemoteObject> GetSaRemote() const override
    {
        auto remote = MediaAnalysisProxy::GetRemoteObject(
            static_cast<int32_t>(IMediaAnalysisService::INVOKE_ANALYSIS_TOOL));
        CHECK_AND_RETURN_RET_LOG(remote != nullptr, nullptr,
            "Failed to get media analysis service remote, said: %{public}d", MEDIA_ANALYSIS_SERVICE_SAID);
        return remote;
    }

    int32_t InvokeAnalysisTool(const InvokeAnalysisToolDto &dto) const override
    {
        MEDIA_INFO_LOG("[tool] taskid:%{public}s, type:%{public}d", dto.taskId.c_str(), dto.type);
        MessageParcel data;
        MessageParcel reply;
        MessageOption option(MessageOption::TF_ASYNC);
        MediaAnalysisProxy proxy(nullptr);

        const InvokeAnalysisToolRequestView request {dto.type, dto.taskId, dto.param};
        CHECK_AND_RETURN_RET_LOG(
            WriteInvokeAnalysisToolRequest(data, proxy, request, "invoke"),
            MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR, "Failed to build invoke analysis tool request");
        CHECK_AND_RETURN_RET_LOG(data.WriteRemoteObject(dto.callbackRemote), MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR,
            "Failed to write invoke analysis tool callback remote");
        bool sendRet = MediaAnalysisProxy::SendTransactCmd(
            static_cast<int32_t>(IMediaAnalysisService::INVOKE_ANALYSIS_TOOL),
            data, reply, option);
        CHECK_AND_RETURN_RET_LOG(sendRet, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR,
            "Failed to send invoke analysis tool transact");
        return E_OK;
    }
    int32_t CancelAnalysisTool(const CancelAnalysisToolDto &dto) const override
    {
        MEDIA_INFO_LOG("[tool] taskid:%{public}s", dto.taskId.c_str());
        return SendCancelAnalysisToolSync(dto.taskId, dto.param);
    }
};

AnalysisToolManager &AnalysisToolManager::GetInstance()
{
    static AnalysisToolManager manager(std::make_shared<AnalysisToolMediaAnalysisRemoteInvoker>());
    return manager;
}

AnalysisToolManager::AnalysisToolManager(std::shared_ptr<AnalysisToolRemoteInvoker> invoker)
    : invoker_(std::move(invoker))
{
}

void AnalysisToolManager::SetInvoker(std::shared_ptr<AnalysisToolRemoteInvoker> invoker)
{
    invoker_ = std::move(invoker);
}

int32_t AnalysisToolManager::SubmitTask(
    const InvokeAnalysisToolDto &dto, int32_t &resultCode, sptr<IRemoteObject> &saRemote, std::string &taskId)
{
    saRemote = nullptr;
    taskId = dto.taskId;
    CHECK_AND_RETURN_RET_LOG(IsSupportedType(dto.type), resultCode = MEDIA_LIBRARY_INVALID_PARAMETER_ERROR,
        "Unsupported analysis tool type: %{public}d", dto.type);
    CHECK_AND_RETURN_RET_LOG(IsValidParam(dto.param), resultCode =MEDIA_LIBRARY_INVALID_PARAMETER_ERROR,
        "Invalid analysis tool param, size: %{public}zu", dto.param.size());
    CHECK_AND_RETURN_RET_LOG(dto.callbackRemote != nullptr, resultCode = MEDIA_LIBRARY_INVALID_PARAMETER_ERROR,
        "Analysis tool callback remote is nullptr");
    CHECK_AND_RETURN_RET_LOG(!dto.taskId.empty(), resultCode = MEDIA_LIBRARY_INVALID_PARAMETER_ERROR,
        "Analysis tool taskId is empty");
    CHECK_AND_RETURN_RET_LOG(invoker_ != nullptr, resultCode = MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR,
        "Analysis tool invoke invoker_ is nullptr");

    resultCode = invoker_->InvokeAnalysisTool(dto);
    CHECK_AND_RETURN_RET_LOG(resultCode == E_OK, resultCode,
        "Failed to invoke analysis tool, resultCode: %{public}d", resultCode);
    saRemote = invoker_->GetSaRemote();
    CHECK_AND_RETURN_RET_LOG(saRemote != nullptr, resultCode = MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR,
        "Failed to get analysis tool sa remote after invoke");
    return resultCode;
}

int32_t AnalysisToolManager::CancelTask(const CancelAnalysisToolDto &dto, int32_t &resultCode)
{
    CHECK_AND_RETURN_RET_LOG(!dto.taskId.empty(), resultCode = MEDIA_LIBRARY_INVALID_PARAMETER_ERROR,
        "Analysis tool cancel taskId is empty");
    CHECK_AND_RETURN_RET_LOG(IsValidParam(dto.param), resultCode = MEDIA_LIBRARY_INVALID_PARAMETER_ERROR,
        "Invalid analysis tool cancel param, size: %{public}zu", dto.param.size());
    CHECK_AND_RETURN_RET_LOG(invoker_ != nullptr, resultCode = MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR,
        "Analysis tool cancel invoker_ is nullptr");
    resultCode = invoker_->CancelAnalysisTool(dto);
    CHECK_AND_RETURN_RET_LOG(resultCode == E_OK, resultCode,
        "Failed to cancel analysis tool, resultCode: %{public}d", resultCode);
    return E_OK;
}
bool AnalysisToolManager::IsSupportedType(int32_t type)
{
    return type >= SUPPORTED_TOOL_TYPE_BEGIN && type <= SUPPORTED_TOOL_TYPE_END;
}
} // namespace OHOS::Media::AnalysisData