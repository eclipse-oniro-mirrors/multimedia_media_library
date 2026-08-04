/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef MEDIA_CALL_TRANSCODE_H
#define MEDIA_CALL_TRANSCODE_H

#include "napi/native_api.h"
#include <string>

namespace OHOS {
namespace Media {
class MediaCallTranscode {
public:
    MediaCallTranscode() = default;
    ~MediaCallTranscode() = default;
    static void CallTranscodeHandle(napi_env env, int srcFd, int destFd,
        napi_value &result, off_t &size, std::string requestId);
    static bool DoTranscode(int srcFd, int destFd, int64_t &size, std::string requestId, int64_t offset);
    static void CallTranscodeRelease(const std::string &requestId);
    using CallbackType = std::function<void(int, int, std::string)>;
    static void RegisterCallback(const CallbackType &cb);
};

} // namespace Media
} // namespace OHOS
#endif // MEDIA_CALL_TRANSCODE_H
