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

#ifndef OHOS_MEDIA_FUNC_TIMER_H
#define OHOS_MEDIA_FUNC_TIMER_H

#include <cstdarg>
#include <cstdio>
#include <securec.h>
#include <string>

#include "hitrace_meter.h"
#include "media_file_utils.h"
#include "media_log.h"

namespace OHOS::Media {
static constexpr int32_t FUNC_TIMER_BUF_SIZE = 254;

class MediaFuncTimer {
public:
    explicit MediaFuncTimer(const std::string &desc) : desc_(desc)
    {
        startTime_ = MediaFileUtils::UTCTimeMilliSeconds();
        StartTrace(HITRACE_TAG_ZMEDIA, desc_);
    }

    explicit MediaFuncTimer(const char *fmt, ...)
    {
        if (fmt == nullptr) {
            desc_ = "MediaFuncTimer Param invalid";
        } else {
            char buf[FUNC_TIMER_BUF_SIZE] = { 0 };
            va_list args;
            va_start(args, fmt);
            int32_t ret = vsnprintf_s(buf, sizeof(buf), sizeof(buf) - 1, fmt, args);
            va_end(args);
            desc_ = (ret >= 0) ? std::string(buf) : "MediaFuncTimer Format Error";
        }
        startTime_ = MediaFileUtils::UTCTimeMilliSeconds();
        StartTrace(HITRACE_TAG_ZMEDIA, desc_);
    }

    ~MediaFuncTimer()
    {
        FinishTrace(HITRACE_TAG_ZMEDIA);
        int64_t interval = MediaFileUtils::UTCTimeMilliSeconds() - startTime_;
        MEDIA_INFO_LOG("%{public}s cost %{public}lld ms", desc_.c_str(), static_cast<long long>(interval));
    }

private:
    std::string desc_;
    int64_t startTime_ {0};
};
} // namespace OHOS::Media
#endif // OHOS_MEDIA_FUNC_TIMER_H
