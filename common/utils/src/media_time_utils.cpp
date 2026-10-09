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

#include <mutex>

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

bool MediaTimeUtils::TimeStampToUtcDate(int64_t timestamp, int& year, int& month, int& day)
{
    std::time_t time = static_cast<std::time_t>(timestamp);
    std::tm tmTime{};
    {
        static std::mutex gmtimeMutex;
        std::lock_guard<std::mutex> lock(gmtimeMutex);

        const std::tm* tmPointer = std::gmtime(&time);
        if (tmPointer == nullptr) {
            MEDIA_ERR_LOG("gmtime error: %{public}d", errno);
            return false;
        }
        tmTime = *tmPointer;
    }
    constexpr int utcYearBase = 1900;
    constexpr int utcMonthBase = 1;
    year = tmTime.tm_year + utcYearBase;
    month = tmTime.tm_mon + utcMonthBase;
    day = tmTime.tm_mday;
    return true;
}

bool MediaTimeUtils::IsPlausibleDateTime(int year, int month, int day, int64_t timestamp)
{
    const int64_t timeStampSeconds = timestamp / MSEC_TO_SEC;
    constexpr int64_t lowerBoundOffset = -12 * 3600; // -12h in seconds
    constexpr int64_t upperBoundOffset = 14 * 3600;  // +14h in seconds

    int yearLowerBound{};
    int monthLowerBound{};
    int dayLowerBound{};
    int yearUpperBound{};
    int monthUpperBound{};
    int dayUpperBound{};
    if (!TimeStampToUtcDate(timeStampSeconds + lowerBoundOffset, yearLowerBound, monthLowerBound, dayLowerBound)) {
        return false;
    }
    if (!TimeStampToUtcDate(timeStampSeconds + upperBoundOffset, yearUpperBound, monthUpperBound, dayUpperBound)) {
        return false;
    }

    auto timeLowerOrEqualTo =
        [](int lowerYear, int lowerMonth, int lowerDay, int upperYear, int upperMonth, int upperDay) {
            if (lowerYear != upperYear) {
                return lowerYear < upperYear;
            }
            if (lowerMonth != upperMonth) {
                return lowerMonth < upperMonth;
            }
            return lowerDay <= upperDay;
        };
    return timeLowerOrEqualTo(yearLowerBound, monthLowerBound, dayLowerBound, year, month, day) &&
        timeLowerOrEqualTo(year, month, day, yearUpperBound, monthUpperBound, dayUpperBound);
}
} // namespace OHOS::Media