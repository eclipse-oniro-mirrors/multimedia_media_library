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
 
#ifndef OHOS_MEDIA_NOTIFICATION_CLASSIFICATION_H
#define OHOS_MEDIA_NOTIFICATION_CLASSIFICATION_H
 
#include <vector>
#include <unordered_map>
#include <functional>
#include <variant>
 
#include "notify_info_inner.h"
#include "media_change_info.h"
 
namespace OHOS {
namespace Media {
namespace Notification {
#define EXPORT __attribute__ ((visibility ("default")))
class NotificationClassification {
public:
    EXPORT NotificationClassification();
    EXPORT ~NotificationClassification();
    EXPORT static void ConvertNotification(std::vector<NotifyInfoInner> &notifyInfos,
        std::vector<MediaChangeInfo> &mediaChangeInfos);
    EXPORT static void AddAlbum(const std::string &albumId);
private:
    static MediaChangeInfo BuildMediaChangeInfo(NotifyInfoInner &notifyInfoInner,
        bool isForRecheck, AccurateNotifyType notifyType, NotifyUriType notifyUriType);
    static std::vector<MediaChangeInfo> HandleAssetAdd(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAssetAddHidden(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAssetAddTrash(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAssetRemove(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAssetRemoveHidden(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAssetRemoveTrash(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAssetUpdateNormal(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAssetUpdateRemoveNormal(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAssetUpdateAddNormal(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAssetUpdateHidden(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAssetUpdateRemoveHidden(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAssetUpdateAddHidden(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAssetUpdateTrash(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAssetUpdateAddTrash(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAssetUpdateRemoveTrash(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAssetTrash(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAssetUntrash(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAssetHidden(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAssetUnhidden(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAssetRecheck(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleYuvReady(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAssetAddWithAnalysis(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAssetRemoveWithAnalysis(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAssetUpdateWithAnalysis(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAnalysisAssetAdd(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAnalysisAssetRemove(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAnalysisAssetRecheck(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAssetAddShare(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAssetRemoveShare(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAssetUpdateShare(NotifyInfoInner &notifyInfoInner);
 
    static std::vector<MediaChangeInfo> HandleAlbumAdd(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAlbumRemove(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAlbumUpdate(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAlbumUpdateHidden(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAlbumUpdateTrash(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAlbumRecheck(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAlbumAddAndUpdate(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAnalysisAlbumAdd(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAnalysisAlbumRemove(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAnalysisAlbumUpdate(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAnalysisAlbumRecheck(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAlbumAddShare(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAlbumRemoveShare(NotifyInfoInner &notifyInfoInner);
    static std::vector<MediaChangeInfo> HandleAlbumUpdateShare(NotifyInfoInner &notifyInfoInner);
private:
    static std::unordered_map<std::variant<AssetRefreshOperation, AlbumRefreshOperation>,
        std::function<std::vector<MediaChangeInfo>(NotifyInfoInner&)>> classificationMap;
    // 存储使用触发器创建的albumId
    static std::unordered_set<int32_t> addAlbumIdSet_;
    static std::mutex addAlbumIdMutex_;
};
} // namespace Notification
} // namespace Media
} // namespace OHOS
 
#endif  // OHOS_MEDIA_NOTIFICATION_CLASSIFICATION_H