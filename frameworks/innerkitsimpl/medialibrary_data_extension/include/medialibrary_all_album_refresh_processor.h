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

#ifndef OHOS_MEDIALIBRARY_ALL_ALBUM_REFRESH_PROCESSOR_H
#define OHOS_MEDIALIBRARY_ALL_ALBUM_REFRESH_PROCESSOR_H

#include <mutex>

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
enum class AlbumRefreshStatus : int32_t {
    NOT_START = 0,
    SYSTEM,
    USER,
    SOURCE,
    ANALYSIS,
    SHARE,
};

class MediaLibraryAllAlbumRefreshProcessor {
public:
    EXPORT static std::shared_ptr<MediaLibraryAllAlbumRefreshProcessor> GetInstance();
    void OnCurrentStatusChanged(bool currentStatus);
    EXPORT void OnCloudSyncStateChanged(bool isCloudSyncing);
    virtual ~MediaLibraryAllAlbumRefreshProcessor() = default;
private:
    MediaLibraryAllAlbumRefreshProcessor();
    bool CheckRefreshConditionLocked();
    void TryRefreshAllAlbums();
    void PostRefreshAllAlbumsTask();
    int64_t GetNowTimeUs();
    int32_t RefreshAlbums(AlbumRefreshStatus albumRefreshStatus,
        const std::vector<int32_t>& albumIds);

    /* singleton */
    static std::shared_ptr<MediaLibraryAllAlbumRefreshProcessor> instance_;
    static std::mutex instanceMutex_;

    std::mutex refreshAllAlbumsLock_;
    int64_t lastRefreshAllAlbumsTime_ {0};
    bool currentStatus_ = false;
    bool isCloudSyncing_ = false;
    AlbumRefreshStatus albumRefreshStatus_ = AlbumRefreshStatus::NOT_START;
    int32_t currentAlbumId_ = 0;
};
} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIALIBRARY_ALL_ALBUM_REFRESH_PROCESSOR_H
