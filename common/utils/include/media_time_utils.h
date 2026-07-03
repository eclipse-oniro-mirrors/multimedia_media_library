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

#ifndef COMMON_UTILS_MEDIA_TIME_UTILS_H_
#define COMMON_UTILS_MEDIA_TIME_UTILS_H_

#include <string>

#include <ctime>

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))

/**
 * media path utils which the ability process path
 */
class MediaTimeUtils {
public:
    EXPORT MediaTimeUtils();
    EXPORT ~MediaTimeUtils();
    EXPORT static std::string StrCreateTime(const std::string &format, int64_t time);
    EXPORT static std::string StrCreateTimeByMilliseconds(const std::string &format, int64_t time);
    EXPORT static int64_t Timespec2Millisecond(const struct timespec &time);
    EXPORT static int64_t UTCTimeSeconds();
    EXPORT static int64_t UTCTimeMilliSeconds();
    EXPORT static int64_t UTCTimeNanoSeconds();
    EXPORT static bool TimeStampToUtcDate(int64_t timestamp, int& year, int& month, int& day);
    // 检查year, month, day代表的日历时间是否能够合理代表timestamp在某个时区的日期。
    // 如果year, month, day在timestamp转化成UTC时间日期后在[-12h, +14h]的范围内，则返回true，否则返回false。
    EXPORT static bool IsPlausibleDateTime(int year, int month, int day, int64_t timestamp);
};
} // namespace OHOS::Media

#endif // COMMON_UTILS_MEDIA_TIME_UTILS_H_
