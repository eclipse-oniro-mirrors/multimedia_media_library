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
#define MLOG_TAG "Media_IPC"

#include "user_define_ipc_client.h"

#include "iservice_registry.h"

namespace OHOS::Media::IPC {
static const std::u16string DESCRIPTOR = u"OHOS.DataShare.IDataShare";
static const int STORAGE_MANAGER_MANAGER_ID = 5003;
UserDefineIPCClient &UserDefineIPCClient::SetTraceId(const std::string &traceId)
{
    this->traceId_ = traceId;
    return *this;
}

std::string UserDefineIPCClient::GetTraceId() const
{
    return this->traceId_;
}

UserDefineIPCClient &UserDefineIPCClient::SetUserId(const int32_t &userId)
{
    this->userId_ = userId;
    return *this;
}

int32_t UserDefineIPCClient::GetUserId() const
{
    return this->userId_;
}

std::unordered_map<std::string, std::string> UserDefineIPCClient::GetHeader() const
{
    return this->header_;
}

UserDefineIPCClient &UserDefineIPCClient::SetHeader(const std::unordered_map<std::string, std::string> &header)
{
    this->header_ = header;
    return *this;
}

int32_t UserDefineIPCClient::InitClient(const int32_t &userId)
{
    userId_ = userId;
    bool errConn = UserFileClient::IsValid(userId);
    CHECK_AND_RETURN_RET(!errConn, E_OK);
    MEDIA_ERR_LOG("Call IPC UserFileClient::IsInValid");
    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    errConn = saManager == nullptr;
    CHECK_AND_RETURN_RET_LOG(!errConn, E_ERR, "Get system ability mgr failed.");
    auto remoteObj = saManager->GetSystemAbility(STORAGE_MANAGER_MANAGER_ID);
    errConn = remoteObj == nullptr;
    CHECK_AND_RETURN_RET_LOG(!errConn, E_ERR, "GetSystemAbility Service Failed.");
    UserFileClient::Init(remoteObj, true, userId);
    return E_OK;
}
int32_t UserDefineIPCClient::HeaderMarshalling(MessageParcel &data)
{
    bool errConn = !data.WriteInterfaceToken(DESCRIPTOR);
    CHECK_AND_RETURN_RET_LOG(!errConn, E_FAIL, "WriteInterfaceToken failed");
    return E_OK;
}
}  // namespace OHOS::Media::IPC