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

#ifndef OHOS_MEDIALIBRARY_MANAGER_NOTIFY_UTILS_H
#define OHOS_MEDIALIBRARY_MANAGER_NOTIFY_UTILS_H

#include <memory>
#include <string>

#include "media_change_info.h"
#include "media_library_notify_types.h"

namespace OHOS {
namespace Media {
class MediaLibraryManagerNotifyUtils {
public:
    static int32_t GetNotifyTypeAndUri(Notification::NotifyUriType type,
        Notification::NotifyUriType &registerUriType, std::string &registerUri);
    static int32_t GetSingleNotifyTypeAndUri(Notification::NotifyUriType type,
        Notification::NotifyUriType &registerUriType, std::string &registerUri);
    static int32_t ExtractSingleId(const std::string &uri, std::string &singleId);
    static int32_t ConvertProviderError(int32_t errCode);
    static NotifyChangeType ConvertNotifyChangeType(Notification::AccurateNotifyType notifyType);
    static bool IsAssetNotifyType(Notification::NotifyUriType uriType);
    static bool IsAlbumNotifyType(Notification::NotifyUriType uriType);

    static PhotoAssetChangeInfos BuildPhotoAssetChangeInfos(
        const std::shared_ptr<Notification::MediaChangeInfo> &changeInfo,
        Notification::NotifyUriType uriType = Notification::NotifyUriType::ANY);
    static AlbumChangeInfos BuildAlbumChangeInfos(
        const std::shared_ptr<Notification::MediaChangeInfo> &changeInfo);
    static PhotoAssetChangeInfos BuildSinglePhotoAssetChangeInfos(
        const AccurateRefresh::PhotoAssetChangeData &changeData,
        const std::shared_ptr<Notification::MediaChangeInfo> &changeInfo);
    static AlbumChangeInfos BuildSingleAlbumChangeInfos(
        const AccurateRefresh::AlbumChangeData &changeData,
        const std::shared_ptr<Notification::MediaChangeInfo> &changeInfo);
    static PhotoAssetChangeInfos BuildPhotoAssetRecheckChangeInfos();
    static AlbumChangeInfos BuildAlbumRecheckChangeInfos();

private:
    static std::shared_ptr<PhotoAssetChangeInfo> BuildPhotoAssetChangeInfo(
        const AccurateRefresh::PhotoAssetChangeInfo &changeInfo,
        Notification::NotifyUriType uriType = Notification::NotifyUriType::ANY);
    static std::shared_ptr<AlbumChangeInfo> BuildAlbumChangeInfo(const AccurateRefresh::AlbumChangeInfo &changeInfo);
    static PhotoAssetChangeData BuildPhotoAssetChangeData(const AccurateRefresh::PhotoAssetChangeData &changeData,
        Notification::NotifyUriType uriType = Notification::NotifyUriType::ANY);
    static AlbumChangeData BuildAlbumChangeData(const AccurateRefresh::AlbumChangeData &changeData);
};
} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIALIBRARY_MANAGER_NOTIFY_UTILS_H
