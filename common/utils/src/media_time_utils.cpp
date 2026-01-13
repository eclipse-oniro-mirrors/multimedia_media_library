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

#include "media_time_utils.h"

#include "media_log.h"

namespace OHOS::Media {
constexpr int64_t MSEC_TO_SEC = 1e3;
constexpr int64_t SEC_TO_MSEC = 1e3;
constexpr int64_t MSEC_TO_NSEC = 1e6;
constexpr size_t DEFAULT_TIME_SIZE = 32;

int64_t MediaTimeUtils::Timespec2Millisecond(const struct timespec &time)
{
    return time.tv_sec * MSEC_TO_SEC + time.tv_nsec / MSEC_TO_NSEC;
}

int64_t MediaTimeUtils::UTCTimeSeconds()
{
    struct timespec t{};
    t.tv_sec = 0;
    t.tv_nsec = 0;
    clock_gettime(CLOCK_REALTIME, &t);
    return (int64_t)(t.tv_sec);
}

int64_t MediaTimeUtils::UTCTimeMilliSeconds()
{
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return t.tv_sec * SEC_TO_MSEC + t.tv_nsec / MSEC_TO_NSEC;
}

int64_t MediaTimeUtils::UTCTimeNanoSeconds()
{
    struct timespec t {};
    constexpr int64_t SEC_TO_NSEC = 1e9;
    clock_gettime(CLOCK_REALTIME, &t);
    return t.tv_sec * SEC_TO_NSEC + t.tv_nsec;
}

std::string MediaTimeUtils::StrCreateTime(const std::string &format, int64_t time)
{
    char strTime[DEFAULT_TIME_SIZE] = "";
    CHECK_AND_RETURN_RET_LOG(time >= 0, strTime, "invalid time: %{public}lld", static_cast<long long>(time));
    struct tm localTm;
    CHECK_AND_RETURN_RET_LOG(localtime_noenv_r(&time, &localTm) != nullptr, strTime,
        "localtime_noenv_r error: %{public}d", errno);
    CHECK_AND_PRINT_LOG(strftime(strTime, sizeof(strTime), format.c_str(), &localTm) != 0,
        "strftime error: %{public}d", errno);
    return strTime;
}

std::string MediaTimeUtils::StrCreateTimeByMilliseconds(const std::string &format, int64_t time)
{
    char strTime[DEFAULT_TIME_SIZE] = "";
    int64_t times = time / MSEC_TO_SEC;
    struct tm localTm;

    if (localtime_noenv_r(&times, &localTm) == nullptr) {
        MEDIA_ERR_LOG("localtime_noenv_r error: %{public}d", errno);
        CHECK_AND_PRINT_LOG(time >= 0, "Time value is negative: %{public}lld", static_cast<long long>(time));
        return strTime;
    }

    if (strftime(strTime, sizeof(strTime), format.c_str(), &localTm) == 0) {
        MEDIA_ERR_LOG("strftime error: %{public}d", errno);
    }

    if (time < 0) {
        MEDIA_ERR_LOG("Time value is negative: %{public}lld", static_cast<long long>(time));
    }
    return strTime;
}
} // namespace OHOS::Media