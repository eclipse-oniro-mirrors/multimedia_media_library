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

#ifndef FRAMEWORKS_INNERKITSIMPL_MEDIA_LIBRARY_INCLUDE_PHOTO_FILE_UTILS_H
#define FRAMEWORKS_INNERKITSIMPL_MEDIA_LIBRARY_INCLUDE_PHOTO_FILE_UTILS_H

#include <string>
#include <tuple>

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))

const int64_t MIN_MILSEC_TIMESTAMP = 1'000;
// The minimum time that can be set on a mobile phone system is 2008-01-01 00:00:00.
// Convert it to a timestamp and multiply it by 10 to get 2349-12-30 00:00:00(11'991'456'000'000).
const int64_t MAX_MILSEC_TIMESTAMP = 11'991'456'000'000;
const int64_t MAX_TIMESTAMP_DIFF = 93'600'000;  // 26 hours(UTC+14,UTC-12)
const int64_t MIN_TIMESTAMP_DIFF = 1'000;       // 1 second

class PhotoFileUtils {
public:
    EXPORT static std::string GetEditDataDir(const std::string &photoPath, int32_t userId = -1);
    EXPORT static std::string GetEditDataPath(const std::string &photoPath, int32_t userId = -1);
    EXPORT static std::string GetEditDataCameraPath(const std::string &photoPath, int32_t userId = -1);
    EXPORT static std::string GetTransCodePath(const std::string &photoPath, int32_t userId = -1);
    EXPORT static std::string GetEditDataSourcePath(const std::string &photoPath, int32_t userId = -1);
    EXPORT static std::string GetEditDataSourceBackPath(const std::string &photoPath, int32_t userId = -1);
    EXPORT static std::string GetEditDataTempPath(const std::string &photoPath, int32_t userId = -1);
    EXPORT static std::string GetEditDataSourceTempPath(const std::string &photoPath, int32_t userId = -1);
    EXPORT static bool IsEditDataSourceBackExists(const std::string &photoPath, int32_t userId = -1);
    EXPORT static bool HasEditData(int64_t editTime);
    EXPORT static bool HasSource(bool hasEditDataCamera, int64_t editTime, int32_t effectMode, int32_t subtype = 0);

    EXPORT static int32_t GetMetaPathFromOrignalPath(const std::string &srcPath, std::string &metaPath);
    EXPORT static std::string GetMetaDataRealPath(const std::string &photoPath, int32_t userId = -1);
    EXPORT static bool IsThumbnailExists(const std::string &photoPath);
    EXPORT static bool IsThumbnailLatest(const std::string &photoPath);
    EXPORT static std::tuple<int64_t, std::string> ExtractTimeInfo(
        const std::string &timeStr, const std::string &format, bool isUTC = false);
    EXPORT static std::tuple<std::string, std::string, std::string> ExtractYearMonthDay(const std::string &detailTime);
    EXPORT static int64_t NormalizeTimestamp(int64_t timestamp, int64_t fallbackValue);
    EXPORT static std::string GetAbsoluteLakeDir(int32_t userId);
    EXPORT static std::string GetAbsoluteLakePath(const std::string &storagePath, int32_t userId);

protected:
    EXPORT static std::string AppendUserId(const std::string &path, int32_t userId = -1);
    EXPORT static std::string GetThumbDir(const std::string &photoPath, int32_t userId = -1);
    EXPORT static std::string GetLCDPath(const std::string &photoPath, int32_t userId = -1);
    EXPORT static std::string GetTHMPath(const std::string &photoPath, int32_t userId = -1);
};
} // namespace OHOS::Media

#endif // FRAMEWORKS_INNERKITSIMPL_MEDIA_LIBRARY_INCLUDE_PHOTO_FILE_UTILS_H
