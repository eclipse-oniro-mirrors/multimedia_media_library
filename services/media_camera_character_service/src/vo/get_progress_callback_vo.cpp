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
#define MLOG_TAG "MediaGetProgressCallback"
 
#include "get_progress_callback_vo.h"
 
#include <sstream>
 
#include "media_log.h"

namespace OHOS::Media {
bool GetProgressCallbackReqBody::Unmarshalling(MessageParcel &parcel)
{
    return true;
}

bool GetProgressCallbackReqBody::Marshalling(MessageParcel &parcel) const
{
    return true;
}

bool GetProgressCallbackRespBody::Unmarshalling(MessageParcel &parcel)
{
    int32_t size = 0;
    parcel.ReadInt32(size);
    if (size <= 0) {
        return true;
    }
    for (int32_t i = 0; i < size; ++i) {
        std::string requestId;
        bool status = parcel.ReadString(requestId);
        CHECK_AND_RETURN_RET(status, status);

        double progress;
        status = parcel.ReadDouble(progress);
        CHECK_AND_RETURN_RET(status, status);
        this->progressMap[requestId] = progress;
    }
    return true;
}

bool GetProgressCallbackRespBody::Marshalling(MessageParcel &parcel) const
{
    parcel.WriteInt32(this->progressMap.size());
    for (auto [requestId, progress] : this->progressMap) {
        bool status = parcel.WriteString(requestId);
        CHECK_AND_RETURN_RET(status, status);

        status = parcel.WriteDouble(progress);
        CHECK_AND_RETURN_RET(status, status);
    }
    return true;
}

std::string GetProgressCallbackRespBody::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"progressMap\": [";
    
    auto it = progressMap.begin();
    auto endIt = progressMap.end();
    while (it != endIt) {
        ss << "{\"" << it->first << "\":" << std::to_string(it->second) << "}";
        if (std::next(it) != endIt) {
            ss << ", ";
        }
        ++it;
    }
    ss << "]"
       << "}";
    return ss.str();
}
}  // namespace OHOS::Media