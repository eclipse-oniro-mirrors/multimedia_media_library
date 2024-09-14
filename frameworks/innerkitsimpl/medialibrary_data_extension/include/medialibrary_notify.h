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
#ifndef OHOS_MEDIALIBRARY_NOTIFY_H
#define OHOS_MEDIALIBRARY_NOTIFY_H

#include <cstddef>
#include <string>
#include <unordered_map>

#include "dataobs_mgr_client.h"
#include "file_asset.h"
#include "medialibrary_async_worker.h"
#include "parcel.h"
#include "rdb_predicates.h"
#include "timer.h"
#include "uri.h"
#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class NotifyTaskData : public AsyncTaskData {
public:
    NotifyTaskData(const std::string &uri, const NotifyType &notifyType, const int albumId, const bool hiddenOnly)
        : uri_(std::move(uri)), notifyType_(notifyType), albumId_(albumId), hiddenOnly_(hiddenOnly) {}
    virtual ~NotifyTaskData() override = default;
    std::string uri_;
    NotifyType notifyType_;
    int albumId_;
    bool hiddenOnly_ = false;
};
constexpr size_t MAX_NOTIFY_LIST_SIZE = 32;
constexpr size_t MNOTIFY_TIME_INTERVAL = 100;
class MediaLibraryNotify {
public:
    EXPORT static std::shared_ptr<MediaLibraryNotify> GetInstance();
    EXPORT virtual ~MediaLibraryNotify();
    EXPORT int32_t Notify(
        const std::string &uri, const NotifyType notifyType, const int albumId = 0, bool hiddenOnly = false);
    EXPORT int32_t Notify(const std::shared_ptr<FileAsset> &closeAsset);
    EXPORT int32_t GetAlbumIdBySubType(const PhotoAlbumSubType subType);
    static void GetNotifyUris(const NativeRdb::AbsRdbPredicates &predicates, std::vector<std::string> &notifyUris);

    static Utils::Timer timer_;
    static uint32_t timerId_;
    static std::mutex mutex_;
    static std::unordered_map<std::string, std::unordered_map<NotifyType, std::list<Uri>>> nfListMap_;
private:
    MediaLibraryNotify();
    int32_t Init();
    int32_t GetDefaultAlbums(std::unordered_map<PhotoAlbumSubType, int> &outAlbums);
    static std::shared_ptr<MediaLibraryNotify> instance_;
    std::unordered_map<PhotoAlbumSubType, int> defaultAlbums_;
};
} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIALIBRARY_NOTIFY_H