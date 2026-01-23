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

#ifndef OHOS_MEDIA_IPC_MEDIA_REQ_VO_H
#define OHOS_MEDIA_IPC_MEDIA_REQ_VO_H

#include <string>
#include <sstream>
#include <unordered_map>

#include "i_media_parcelable.h"
#include "media_log.h"
#include "media_itypes_utils.h"
#include "media_time_utils.h"

namespace OHOS::Media::IPC {
template <class T>
class MediaReqVo : public IPC::IMediaParcelable {
private:
    uint32_t code_;
    std::string traceId_;
    int32_t userId_;
    std::unordered_map<std::string, std::string> header_;
    T body;

public:  // constructors & destructors
    virtual ~MediaReqVo() = default;

public:  // gettter and setter
    const std::string GetTraceId() const
    {
        return this->traceId_;
    }
    void SetTraceId(const std::string &traceId)
    {
        this->traceId_ = traceId;
        if (traceId.empty()) {
            this->traceId_ = "media_trace_" + std::to_string(MediaTimeUtils::UTCTimeSeconds());
            MEDIA_INFO_LOG("traceId is empty, auto generate traceId: %{public}s", this->traceId_.c_str());
        }
        return;
    }
    T GetBody() const
    {
        return this->body;
    }
    void SetBody(T body)
    {
        this->body = body;
        return;
    }
    void SetCode(uint32_t code)
    {
        this->code_ = code;
    }
    uint32_t GetCode() const
    {
        return this->code_;
    }
    void SetUserId(const int32_t &userId)
    {
        this->userId_ = userId;
    }
    int32_t GetUserId() const
    {
        return this->userId_;
    }

    std::unordered_map<std::string, std::string> GetHeader() const
    {
        return this->header_;
    }

    void SetHeader(const std::unordered_map<std::string, std::string> &header)
    {
        this->header_ = header;
    }

public:  // functions of Parcelable.
    bool Unmarshalling(MessageParcel &parcel) override
    {
        parcel.ReadUint32(this->code_);
        parcel.ReadString(this->traceId_);
        parcel.ReadInt32(this->userId_);
        ITypeMediaUtil::Unmarshalling(this->header_, parcel);
        this->body.Unmarshalling(parcel);
        return true;
    }

    bool Marshalling(MessageParcel &parcel) const override
    {
        parcel.WriteUint32(this->code_);
        parcel.WriteString(this->traceId_);
        parcel.WriteInt32(this->userId_);
        ITypeMediaUtil::Marshalling(this->header_, parcel);
        this->body.Marshalling(parcel);
        return true;
    }

public:  // basic functions
    std::string ToString() const
    {
        std::stringstream ss;
        ss << "{"
           << "\"userId:\": \"" << this->userId_ << "\","
           << "\"traceId\": \"" << this->traceId_ << "\", ";
        ss << "\"header\": {";
        for (auto &item : this->header_) {
            ss << "\"" << item.first << "\": \"" << item.second << "\", ";
        }
        ss << "}";  // header
        ss << "}";  // end of MediaReqVo
        return ss.str();
    }
};
}  // namespace OHOS::Media::IPC
#endif  // OHOS_MEDIA_IPC_MEDIA_REQ_VO_H