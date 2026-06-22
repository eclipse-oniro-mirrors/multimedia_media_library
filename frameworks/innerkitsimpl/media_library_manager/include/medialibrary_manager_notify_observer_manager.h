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

#ifndef OHOS_MEDIALIBRARY_MANAGER_NOTIFY_OBSERVER_MANAGER_H
#define OHOS_MEDIALIBRARY_MANAGER_NOTIFY_OBSERVER_MANAGER_H

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "datashare_helper.h"
#include "media_change_info.h"
#include "media_library_notify_callback.h"

namespace OHOS {
namespace Media {
class MediaLibraryManagerNotifyObserver;

class MediaLibraryManagerNotifyObserverManager {
public:
    static MediaLibraryManagerNotifyObserverManager &GetInstance();

    int32_t RegisterAssetCallback(std::shared_ptr<DataShare::DataShareHelper> dataShareHelper,
        Notification::NotifyUriType uriType, const std::shared_ptr<PhotoAssetChangeCallback> &callback);
    int32_t UnregisterAssetCallback(std::shared_ptr<DataShare::DataShareHelper> dataShareHelper,
        Notification::NotifyUriType uriType, const std::shared_ptr<PhotoAssetChangeCallback> &callback = nullptr);
    int32_t RegisterAlbumCallback(std::shared_ptr<DataShare::DataShareHelper> dataShareHelper,
        Notification::NotifyUriType uriType, const std::shared_ptr<PhotoAlbumChangeCallback> &callback);
    int32_t UnregisterAlbumCallback(std::shared_ptr<DataShare::DataShareHelper> dataShareHelper,
        Notification::NotifyUriType uriType, const std::shared_ptr<PhotoAlbumChangeCallback> &callback = nullptr);
    int32_t RegisterSingleAssetCallback(std::shared_ptr<DataShare::DataShareHelper> dataShareHelper,
        Notification::NotifyUriType uriType, const std::string &assetUri,
        const std::shared_ptr<PhotoAssetChangeCallback> &callback);
    int32_t UnregisterSingleAssetCallback(std::shared_ptr<DataShare::DataShareHelper> dataShareHelper,
        Notification::NotifyUriType uriType, const std::string &assetUri = "",
        const std::shared_ptr<PhotoAssetChangeCallback> &callback = nullptr);
    int32_t RegisterSingleAlbumCallback(std::shared_ptr<DataShare::DataShareHelper> dataShareHelper,
        Notification::NotifyUriType uriType, const std::string &albumUri,
        const std::shared_ptr<PhotoAlbumChangeCallback> &callback);
    int32_t UnregisterSingleAlbumCallback(std::shared_ptr<DataShare::DataShareHelper> dataShareHelper,
        Notification::NotifyUriType uriType, const std::string &albumUri = "",
        const std::shared_ptr<PhotoAlbumChangeCallback> &callback = nullptr);

    void NotifyChange(const std::shared_ptr<Notification::MediaChangeInfo> &changeInfo);

private:
    struct NotifyObserverRecord {
        Notification::NotifyUriType uriType = Notification::NotifyUriType::INVALID;
        std::string registerUri;
        std::shared_ptr<MediaLibraryManagerNotifyObserver> observer;
        std::vector<std::shared_ptr<PhotoAssetChangeCallback>> assetCallbacks;
        std::vector<std::shared_ptr<PhotoAlbumChangeCallback>> albumCallbacks;
        std::map<std::string, std::vector<std::shared_ptr<PhotoAssetChangeCallback>>> singleAssetCallbacks;
        std::map<std::string, std::vector<std::shared_ptr<PhotoAlbumChangeCallback>>> singleAlbumCallbacks;
    };

    MediaLibraryManagerNotifyObserverManager() = default;
    ~MediaLibraryManagerNotifyObserverManager() = default;

    // "WithLockHeld" 表示调用方已持有 mutex_，这些函数内部不再重复加锁。
    int32_t RegisterBaseObserverWithLockHeld(std::shared_ptr<DataShare::DataShareHelper> dataShareHelper,
        Notification::NotifyUriType uriType, const std::string &registerUri, NotifyObserverRecord &record);
    int32_t UnregisterBaseObserverWithLockHeld(std::shared_ptr<DataShare::DataShareHelper> dataShareHelper,
        NotifyObserverRecord &record);
    int32_t RegisterSingleIdWithLockHeld(std::shared_ptr<DataShare::DataShareHelper> dataShareHelper,
        const std::string &registerUri, const std::string &singleId,
        const std::shared_ptr<DataShare::DataShareObserver> &observer);
    int32_t UnregisterSingleIdWithLockHeld(std::shared_ptr<DataShare::DataShareHelper> dataShareHelper,
        const std::string &registerUri, const std::string &singleId,
        const std::shared_ptr<DataShare::DataShareObserver> &observer);
    int32_t CheckSinglePhotoPermission(std::shared_ptr<DataShare::DataShareHelper> dataShareHelper,
        Notification::NotifyUriType uriType, const std::string &singleId);

    bool IsRecordEmpty(const NotifyObserverRecord &record) const;
    void DispatchAssetChange(const std::shared_ptr<Notification::MediaChangeInfo> &changeInfo,
        Notification::NotifyUriType uriType);
    void DispatchAlbumChange(const std::shared_ptr<Notification::MediaChangeInfo> &changeInfo,
        Notification::NotifyUriType uriType);
    void DispatchSingleAssetChange(const std::shared_ptr<Notification::MediaChangeInfo> &changeInfo);
    void DispatchSingleAlbumChange(const std::shared_ptr<Notification::MediaChangeInfo> &changeInfo);

    std::mutex mutex_;
    std::map<Notification::NotifyUriType, NotifyObserverRecord> observerRecords_;
};
} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIALIBRARY_MANAGER_NOTIFY_OBSERVER_MANAGER_H
