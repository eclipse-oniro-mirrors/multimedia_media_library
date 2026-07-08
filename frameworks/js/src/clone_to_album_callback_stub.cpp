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

#include "clone_to_album_callback_stub.h"

#include "itypes_util.h"
#include "media_log.h"
#include "media_itypes_utils.h"
#include "medialibrary_client_errno.h"

namespace OHOS {
namespace Media {

int32_t CloneToAlbumCallbackStub::OnRemoteRequest(uint32_t code, MessageParcel &data,
    MessageParcel &reply, MessageOption &option)
{
    CHECK_AND_RETURN_RET(data.ReadInterfaceToken() == GetDescriptor(), JS_E_INNER_FAIL);
    auto it = HANDLERS.find(code);
    if (it == HANDLERS.end()) {
        MEDIA_INFO_LOG("OnRemoteRequest not find code");
        return -1;
    }
    return (this->*(it->second))(data, reply);
}

int32_t CloneToAlbumCallbackStub::OnProgress(MessageParcel &data, MessageParcel &reply)
{
    uint64_t processedSize = data.ReadUint64();
    uint64_t totalSize = data.ReadUint64();
    uint32_t processedCount = data.ReadUint32();
    uint32_t totalCount = data.ReadUint32();
    return OnProgress(processedSize, totalSize, processedCount, totalCount);
}

int32_t CloneToAlbumCallbackStub::OnComplete(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter OnComplete");
    int32_t errorCode = data.ReadInt32();
    MEDIA_INFO_LOG("enter OnComplete errorCode:%{public}d", errorCode);
    std::vector<std::string> successUris;
    data.ReadStringVector(&successUris);
    auto resultSet = DataShare::DataShareResultSet::Unmarshal(data);
    return OnComplete(errorCode, successUris, resultSet);
}
} // namespace Media
} // namespace OHOS
