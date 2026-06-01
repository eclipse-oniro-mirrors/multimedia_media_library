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

#include "deep_optimize_space_vo.h"

#include <sstream>

#include "media_log.h"

namespace OHOS::Media {
bool StartDeepOptimizeSpaceReqBody::Unmarshalling(MessageParcel &parcel)
{
    clientRemote = parcel.ReadRemoteObject();
    CHECK_AND_RETURN_RET_LOG(clientRemote != nullptr, false,
        "Failed to read deep optimize space client remote");
    
    bool hasCallbackRemote = false;
    CHECK_AND_RETURN_RET_LOG(parcel.ReadBool(hasCallbackRemote), false,
        "Failed to read deep optimize space hasCallbackRemote flag");
    
    if (hasCallbackRemote) {
        callbackRemote = parcel.ReadRemoteObject();
        CHECK_AND_RETURN_RET_LOG(callbackRemote != nullptr, false,
            "Failed to read deep optimize space callback remote");
    } else {
        callbackRemote = nullptr;
    }
    return true;
}

bool StartDeepOptimizeSpaceReqBody::Marshalling(MessageParcel &parcel) const
{
    CHECK_AND_RETURN_RET_LOG(parcel.WriteRemoteObject(clientRemote), false,
        "Failed to write deep optimize space client remote");
    
    bool hasCallbackRemote = (callbackRemote != nullptr);
    CHECK_AND_RETURN_RET_LOG(parcel.WriteBool(hasCallbackRemote), false,
        "Failed to write deep optimize space hasCallbackRemote flag");
    
    if (hasCallbackRemote) {
        CHECK_AND_RETURN_RET_LOG(parcel.WriteRemoteObject(callbackRemote), false,
            "Failed to write deep optimize space callback remote");
    }
    return true;
}

std::string StartDeepOptimizeSpaceReqBody::ToString() const
{
    std::stringstream ss;
    ss << "hasClientRemote=" << (clientRemote != nullptr)
       << ", hasCallbackRemote=" << (callbackRemote != nullptr);
    return ss.str();
}
} // namespace OHOS::Media