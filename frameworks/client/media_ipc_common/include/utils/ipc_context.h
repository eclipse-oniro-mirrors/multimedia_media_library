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

#ifndef OHOS_MEDIA_IPC_CONTEXT_H
#define OHOS_MEDIA_IPC_CONTEXT_H

#include <string>
#include <unordered_map>

#include "message_parcel.h"
#include "media_log.h"
#include "medialibrary_errno.h"

namespace OHOS::Media::IPC {
class IPCContext {
public:
    IPCContext(const MessageOption &option, int32_t byPassCode = 0) : option_(option), byPassCode_(byPassCode)
    {}

    const MessageOption &GetOption() const
    {
        return option_;
    }

    void SetByPassCode(int32_t byPassCode)
    {
        byPassCode_ = byPassCode;
    }

    int32_t GetByPassCode() const
    {
        return byPassCode_;
    }

    void SetHeader(const std::unordered_map<std::string, std::string> &header)
    {
        this->header_ = header;
    }

    std::unordered_map<std::string, std::string> GetHeader() const
    {
        return this->header_;
    }

private:
    MessageOption option_;
    int32_t byPassCode_;
    std::unordered_map<std::string, std::string> header_;
};
}  // namespace OHOS::Media::IPC
#endif  // OHOS_MEDIA_IPC_CONTEXT_H