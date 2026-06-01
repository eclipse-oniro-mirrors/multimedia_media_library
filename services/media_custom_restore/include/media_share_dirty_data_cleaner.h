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
#ifndef OHOS_MEDIALIBRARY_MEDIA_SHARE_DIRTY_DATA_CLEANER_H
#define OHOS_MEDIALIBRARY_MEDIA_SHARE_DIRTY_DATA_CLEANER_H

#include <string>
#include <unordered_map>
#include <vector>
#include <atomic>

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))

static const std::string CLEAN_FLAG = "clean_flag";
static const std::string LAST_SHARE_TIME = "last_share_time";
static const std::string SHARE_PACKAGE_NAME = "com.huawei.hmos.instantshare";
class EXPORT MediaShareDirtyDataCleaner {
public:
    MediaShareDirtyDataCleaner() = default;
    ~MediaShareDirtyDataCleaner() = default;

    static void CheckDirtyData();
    static bool UpdateCleanFlag(bool isNeedClean);
    static bool UpdateShareTime(bool isStartShare);
    static bool GetSharingState();
    static void SetSharingState(bool isSharing);

private:
    static std::atomic<bool> isSharing_;
    static bool IsNeedClean(int64_t &lastShareTime);
    static void CleanDirtyData(int64_t lastShareTime);
    static std::unordered_map<int32_t, std::string> GetDirtyData(int64_t lastShareTime);
    static bool DeleteDb(const std::vector<std::string> &fileIds);
    static void DeleteFiles(const std::string &path);
    static void DeleteFileWithCheck(const std::string &path);
};

} // namespace OHOS::Media
#endif // OHOS_MEDIALIBRARY_MEDIA_SHARE_DIRTY_DATA_CLEANER_H