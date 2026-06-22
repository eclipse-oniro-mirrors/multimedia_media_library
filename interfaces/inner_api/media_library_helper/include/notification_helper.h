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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_NOTIFICATION_HELPER_H
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_NOTIFICATION_HELPER_H

#include <memory>
#include <vector>
#include "userfile_manager_types.h"
#include "album_change_info.h"
#include "datashare_helper.h"

namespace OHOS {
namespace Media {
namespace NotificationHelper {
#define EXPORT __attribute__ ((visibility ("default")))

/** 0: Registration / unregistration successful (Media Library Notification Management FDS). */
constexpr int32_t NOTIFY_OK = 0;
/** Register: -1 permission verification. unRegister: -1 same callback cannot be canceled repeatedly (HQ FDS). */
constexpr int32_t NOTIFY_ERR_PERMISSION = -1;
/** Register (HQ FDS): -1 permission; unRegister (HQ FDS): -1 cannot cancel repeatedly — same numeric value. */
constexpr int32_t NOTIFY_ERR_UNREGISTER_REPEAT = -1;
/** Register (HQ FDS): -2 IPC timeout. */
constexpr int32_t NOTIFY_ERR_IPC_TIMEOUT = -2;
/** Register (HQ FDS): -3 database exception. */
constexpr int32_t NOTIFY_ERR_DATABASE = -3;
/** Register (HQ FDS): -4 file system exception. */
constexpr int32_t NOTIFY_ERR_FS_EXCEPTION = -4;

using namespace Media::AccurateRefresh;

struct AlbumChangeData {
    std::shared_ptr<AlbumChangeInfo> albumBeforeChange;
    std::shared_ptr<AlbumChangeInfo> albumAfterChange;
    int64_t version = 0;
};

struct AlbumChangeInfos {
    NotifyChangeType type = NotifyChangeType::NOTIFY_CHANGE_INVALID;
    std::vector<AlbumChangeData> albumChangeDatas;
    bool isForRecheck = false;
};

class EXPORT PhotoAlbumChangeCallback {
public:
    PhotoAlbumChangeCallback() = default;
    EXPORT virtual ~PhotoAlbumChangeCallback() = default;
    EXPORT virtual int32_t OnChange(AlbumChangeInfos info) = 0;
};

/**
 * Client-side notification helper. Registers a DataShare observer for PHOTO_ALBUM_URI
 * so that album change notifications arrive cross-process via IPC from the media library
 * server and are dispatched to registered PhotoAlbumChangeCallback instances.
 */
class EXPORT NotificationHelper {
public:
    /**
     * @brief Registers a listener for an ordinary photo album.
     * @param callback The callback to receive album change notifications.
     * @return Error code (Album Accurate Notification inner API — HQ Functional Design Specification):
     *         0: Registration successful.
     *         -1: Permission verification.
     *         -2: IPC timeout.
     *         -3: Database exception.
     *         -4: File system exception.
     * @note Required permission: ohos.permission.READ_IMAGEVIDEO.
     */
    EXPORT static int32_t RegisterPhotoAlbumCallback(std::shared_ptr<PhotoAlbumChangeCallback> callback);

    /**
     * @brief Unregisters the listener for a regular photo album.
     * @param callback The callback to unregister.
     * @return Error code (HQ Functional Design Specification):
     *         0: Unregistration successful.
     *         -1: The same callback cannot be canceled repeatedly (e.g. not registered or already removed).
     * @note Required permission: ohos.permission.READ_IMAGEVIDEO.
     */
    EXPORT static int32_t unRegisterPhotoAlbumCallback(std::shared_ptr<PhotoAlbumChangeCallback> callback);

private:
    friend class InternalAlbumObserver;

    NotificationHelper() = default;
    ~NotificationHelper() = default;

    static void NotifyAllCallbacks(const AlbumChangeInfos& changeInfos);
    static bool StartObserverIfNeeded();
    static void StopObserverIfNeeded();

    static std::mutex callbackMutex_;
    static std::vector<std::weak_ptr<PhotoAlbumChangeCallback>> callbacks_;
    static std::shared_ptr<DataShare::DataShareHelper> dataShareHelper_;
    static std::shared_ptr<DataShare::DataShareObserver> internalObserver_;
};

} // namespace NotificationHelper
} // namespace Media
} // namespace OHOS

#endif // INTERFACES_INNERKITS_NATIVE_INCLUDE_NOTIFICATION_HELPER_H