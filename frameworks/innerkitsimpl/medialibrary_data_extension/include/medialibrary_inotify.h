/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#ifndef OHOS_MEDIALIBRARY_INOTIFY_H
#define OHOS_MEDIALIBRARY_INOTIFY_H

#include <sys/inotify.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
struct WatchInfo {
    WatchInfo(const std::string &path, const std::string &uri, const std::string &bundleName,
        MediaLibraryApi api, const int64_t &currentTime): path_(path), uri_(uri),
        bundleName_(bundleName), api_(api), meetEvent_(0), currentTime_(currentTime){};
    std::string path_;
    std::string uri_;
    std::string bundleName_;
    MediaLibraryApi api_;
    uint32_t meetEvent_;
    int64_t currentTime_;
};

struct WatchBundleInfo {
    WatchBundleInfo(const int32_t count, const int64_t firstEntryTime,
        const std::string &firstUri, const std::string &bundleName): count(count),
        firstEntryTime(firstEntryTime), firstUri(firstUri), bundleName(bundleName){};
    int32_t count;
    int64_t firstEntryTime;
    std::string firstUri;
    std::string bundleName;
    std::string Dump() const
    {
        return "count:" + std::to_string(count) + ", firstEntryTime:" + std::to_string(firstEntryTime) +
            ", firstUri:" + firstUri + ", bundleName:" + bundleName;
    }
};

class MediaLibraryInotify {
public:
    EXPORT static std::shared_ptr<MediaLibraryInotify> GetInstance();
    EXPORT int32_t AddWatchList(const std::string &path, const std::string &uri,
        MediaLibraryApi api = MediaLibraryApi::API_OLD);
    EXPORT MediaLibraryInotify() = default;
    EXPORT ~MediaLibraryInotify() = default;
    EXPORT int32_t RemoveByFileUri(const std::string &uri, MediaLibraryApi api = MediaLibraryApi::API_OLD);
    EXPORT void DoAging();
    void DoStop();
    EXPORT const std::string BuildDfxInfo();

private:
    int32_t Remove(int wd);
    void WatchCallBack();
    int32_t Init();
    void Restart();
    int32_t GetBundleCount(const std::string &bundleName);

private:
    static std::shared_ptr<MediaLibraryInotify> instance_;
    static std::mutex mutex_;
    static std::once_flag onceFlag_;
    static inline std::unordered_map<int, struct WatchInfo> watchList_;
    static inline std::atomic<int> inotifyFd_ = 0;
    static inline std::atomic<bool> isWatching_ = false;
};
} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIALIBRARY_INOTIFY_H