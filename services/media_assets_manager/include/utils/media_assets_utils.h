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

#ifndef OHOS_MEDIA_MEDIA_ASSETS_UTILS_H
#define OHOS_MEDIA_MEDIA_ASSETS_UTILS_H

#include <vector>

#include "media_assets_dao.h"

namespace OHOS::Media {
class MediaAssetsUtils {
public:
    static std::string GetFileId(const std::string &arg);
};

class TimeLogger {
public:
    TimeLogger() = default;
    void Start(const std::string &eventName, const std::string &extraInfo = "")
    {
        this->beginTime_ = MediaFileUtils::UTCTimeMilliSeconds();
        this->eventName_ = eventName;
        this->extraInfo_ = extraInfo;
        MEDIA_INFO_LOG("event: %{public}s, begin: %{public}s, extraInfo: %{public}s",
            this->eventName_.c_str(),
            std::to_string(this->beginTime_).c_str(),
            this->extraInfo_.c_str());
    }
    ~TimeLogger()
    {
        this->endTime_ = MediaFileUtils::UTCTimeMilliSeconds();
        MEDIA_INFO_LOG(
            "event: %{public}s, duration: %{public}s, begin: %{public}s, end: %{public}s, extraInfo: %{public}s",
            this->eventName_.c_str(),
            std::to_string(this->endTime_ - this->beginTime_).c_str(),
            std::to_string(this->beginTime_).c_str(),
            std::to_string(this->endTime_).c_str(),
            this->extraInfo_.c_str());
    }

private:
    std::string eventName_;
    std::string extraInfo_;
    int64_t beginTime_;
    int64_t endTime_;
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_MEDIA_ASSETS_UTILS_H