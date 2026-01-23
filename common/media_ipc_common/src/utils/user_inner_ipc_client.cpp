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

#include "user_inner_ipc_client.h"

namespace OHOS::Media::IPC {
static const std::u16string DESCRIPTOR = u"OHOS.DataShare.IDataShare";
UserInnerIPCClient &UserInnerIPCClient::SetTraceId(const std::string &traceId)
{
    this->traceId_ = traceId;
    return *this;
}

std::string UserInnerIPCClient::GetTraceId() const
{
    return this->traceId_;
}

UserInnerIPCClient &UserInnerIPCClient::SetUserId(const int32_t &userId)
{
    this->userId_ = userId;
    return *this;
}

int32_t UserInnerIPCClient::GetUserId() const
{
    return this->userId_;
}

std::unordered_map<std::string, std::string> UserInnerIPCClient::GetHeader() const
{
    return this->header_;
}

UserInnerIPCClient &UserInnerIPCClient::SetHeader(const std::unordered_map<std::string, std::string> &header)
{
    this->header_ = header;
    return *this;
}

UserInnerIPCClient &UserInnerIPCClient::SetDataShareHelper(std::shared_ptr<DataShare::DataShareHelper> dataShareHelper)
{
    this->dataShareHelper_ = dataShareHelper;
    return *this;
}

int32_t UserInnerIPCClient::HeaderMarshalling(MessageParcel &data)
{
    bool errConn = !data.WriteInterfaceToken(DESCRIPTOR);
    CHECK_AND_RETURN_RET_LOG(!errConn, E_FAIL, "WriteInterfaceToken failed");
    return E_OK;
}

int32_t UserInnerIPCClient::UserDefineFunc(MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    CHECK_AND_RETURN_RET_LOG(this->dataShareHelper_ != nullptr, E_ERR, "dataShareHelper_ is nullptr");
    return this->dataShareHelper_->UserDefineFunc(data, reply, option);
}
}  // namespace OHOS::Media::IPC