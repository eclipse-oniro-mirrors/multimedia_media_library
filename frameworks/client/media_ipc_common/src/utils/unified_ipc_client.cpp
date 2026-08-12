/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "unified_ipc_client.h"

#include "base_data_uri.h"
#include "media_common_client.h"

namespace OHOS::Media::IPC {
static const std::u16string DESCRIPTOR = u"OHOS.DataShare.IDataShare";

UnifiedIPCClient &UnifiedIPCClient::SetTraceId(const std::string &traceId)
{
    this->traceId_ = traceId;
    return *this;
}

std::string UnifiedIPCClient::GetTraceId() const
{
    return this->traceId_;
}

UnifiedIPCClient &UnifiedIPCClient::SetUserId(const int32_t &userId)
{
    this->userId_ = userId;
    return *this;
}

int32_t UnifiedIPCClient::GetUserId() const
{
    return this->userId_;
}

std::unordered_map<std::string, std::string> UnifiedIPCClient::GetHeader() const
{
    return this->header_;
}

UnifiedIPCClient &UnifiedIPCClient::SetHeader(const std::unordered_map<std::string, std::string> &header)
{
    this->header_ = header;
    return *this;
}

UnifiedIPCClient &UnifiedIPCClient::SetDataShareHelper(
    std::shared_ptr<DataShare::DataShareHelper> dataShareHelper)
{
    this->dataShareHelper_ = dataShareHelper;
    return *this;
}

int32_t UnifiedIPCClient::HeaderMarshalling(MessageParcel &data)
{
    bool errConn = !data.WriteInterfaceToken(DESCRIPTOR);
    CHECK_AND_RETURN_RET_LOG(!errConn, E_FAIL, "WriteInterfaceToken failed");
    return E_OK;
}

int32_t UnifiedIPCClient::InitClient(const int32_t &userId)
{
    // Injection mode: use helper provided by caller via SetDataShareHelper().
    // If injected, do NOT fall back to global creation — caller takes priority.
    if (dataShareHelper_ != nullptr) {
        return E_OK;
    }
    // Global mode: get or create helper from MediaDataShareHelper (cached in SafeMap).
    dataShareHelper_ = MediaCommonClient::GetInstance().GetOrCreateDataShareHelper(userId);
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr, E_ERR, "Failed to get/create DataShareHelper");
    return E_OK;
}

int32_t UnifiedIPCClient::UserDefineFunc(MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    // Injection mode: use injected helper directly
    if (dataShareHelper_ != nullptr) {
        return dataShareHelper_->UserDefineFunc(data, reply, option);
    }
    // Helper not injected and global creation failed
    MEDIA_ERR_LOG("UserDefineFunc: dataShareHelper_ is nullptr, SetDataShareHelper() or InitClient() failed");
    return E_ERR;
}
}  // namespace OHOS::Media::IPC
