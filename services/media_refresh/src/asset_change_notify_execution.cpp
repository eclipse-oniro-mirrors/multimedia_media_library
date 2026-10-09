/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "AccurateRefresh::AssetChangeNotifyExecution"
#include "asset_change_notify_execution.h"

#include "trash_asset_helper.h"
#include "hidden_asset_helper.h"
#include "share_asset_helper.h"
#include "accurate_debug_log.h"
#include "medialibrary_notify_new.h"

namespace OHOS {
namespace Media::AccurateRefresh {
using namespace std;
using namespace OHOS::Media::Notification;

const std::vector<std::pair<std::function<bool(std::pair<NotifyAssetType, NotifyAssetType>)>,
    Notification::AssetRefreshOperation>> NORMAL_ASSET_OPERATION_CALS = {
    {
        [](std::pair<NotifyAssetType, NotifyAssetType> info) -> bool {
            return info.first != ASSET_NORMAL && info.second == ASSET_NORMAL;
        },
        Notification::ASSET_OPERATION_UPDATE_ADD_NORMAL
    },
    {
        [](std::pair<NotifyAssetType, NotifyAssetType> info) -> bool {
            return info.first == ASSET_NORMAL && info.second != ASSET_NORMAL;
        },
        Notification::ASSET_OPERATION_UPDATE_REMOVE_NORMAL
    },
    {
        [](std::pair<NotifyAssetType, NotifyAssetType> info) -> bool {
            return info.first == ASSET_NORMAL && info.second == ASSET_NORMAL;
        },
        Notification::ASSET_OPERATION_UPDATE_NORMAL
    },
};
const std::vector<std::pair<std::function<bool(std::pair<NotifyAssetType, NotifyAssetType>)>,
    Notification::AssetRefreshOperation>> HIDDEN_ASSET_OPERATION_CALS = {
    {
        [](std::pair<NotifyAssetType, NotifyAssetType> info) -> bool {
            return info.first != ASSET_HIDDEN && info.second == ASSET_HIDDEN;
        },
        Notification::ASSET_OPERATION_UPDATE_ADD_HIDDEN
    },
    {
        [](std::pair<NotifyAssetType, NotifyAssetType> info) -> bool {
            return info.first == ASSET_HIDDEN && info.second != ASSET_HIDDEN;
        },
        Notification::ASSET_OPERATION_UPDATE_REMOVE_HIDDEN
    },
    {
        [](std::pair<NotifyAssetType, NotifyAssetType> info) -> bool {
            return info.first == ASSET_HIDDEN && info.second == ASSET_HIDDEN;
        },
        Notification::ASSET_OPERATION_UPDATE_HIDDEN
    },
};
const std::vector<std::pair<std::function<bool(std::pair<NotifyAssetType, NotifyAssetType>)>,
    Notification::AssetRefreshOperation>> TRASH_ASSET_OPERATION_CALS = {
    {
        [](std::pair<NotifyAssetType, NotifyAssetType> info) -> bool {
            return (info.first & ASSET_TRASH) != ASSET_TRASH && (info.second & ASSET_TRASH) == ASSET_TRASH;
        },
        Notification::ASSET_OPERATION_UPDATE_ADD_TRASH
    },
    {
        [](std::pair<NotifyAssetType, NotifyAssetType> info) -> bool {
            return (info.first & ASSET_TRASH) == ASSET_TRASH && (info.second & ASSET_TRASH) != ASSET_TRASH;
        },
        Notification::ASSET_OPERATION_UPDATE_REMOVE_TRASH
    },
    {
        [](std::pair<NotifyAssetType, NotifyAssetType> info) -> bool {
            return (info.first & ASSET_TRASH) == ASSET_TRASH && (info.second & ASSET_TRASH) == ASSET_TRASH;
        },
        Notification::ASSET_OPERATION_UPDATE_TRASH
    },
};

static void PrintNotifyLog(Notification::AssetRefreshOperation operationType,
    size_t totalLogSize, bool& recordFirst, const NotifyInfoInner& notifyInfo, const PhotoAssetChangeData& changeData)
{
    size_t logTimesLimit = 50;
    if (totalLogSize <= logTimesLimit || !recordFirst) {
        ACCURATE_INFO("notify PHOTOS info: operationType(0x%{public}x), level(%{public}d), info(%{public}s)",
            static_cast<int32_t>(operationType),
            static_cast<int32_t>(notifyInfo.notifyLevel.priority), changeData.ToString().c_str());
        recordFirst = true;
        if (totalLogSize > logTimesLimit) {
            ACCURATE_INFO("%{public}zu total notify datas", totalLogSize);
        }
    }
}

void AssetChangeNotifyExecution::Notify(const vector<PhotoAssetChangeData> &changeDatas)
{
    // 轮询所有的changeDatas
    for (auto &changeData : changeDatas) {
        auto &infoBefore = changeData.infoBeforeChange_;
        auto &infoAfter = changeData.infoAfterChange_;
        auto rdbOperation = changeData.operation_;
        if (rdbOperation == RDB_OPERATION_ADD) {
            AssetRefreshOperation operationType = GetAddOperation(infoAfter);
            if (operationType != ASSET_OPERATION_UNDEFINED) {
                InsertNotifyInfo(operationType, changeData);
            } else {
                MEDIA_WARN_LOG("invalid after asset info:%{public}s", infoAfter.ToString().c_str());
            }
        } else if (rdbOperation == RDB_OPERATION_REMOVE) {
            AssetRefreshOperation operationType = GetRemoveOperation(infoBefore);
            if (operationType != ASSET_OPERATION_UNDEFINED) {
                InsertNotifyInfo(operationType, changeData);
            } else {
                MEDIA_WARN_LOG("invalid before asset info:%{public}s", infoBefore.ToString().c_str());
            }
        } else if (rdbOperation == RDB_OPERATION_UPDATE) {
            bool isShare = infoBefore.isShared_ != 0 || infoAfter.isShared_ != 0;
            if (isShare) {
                InsertShareAssetOperation(changeData);
            } else {
                InsertNormalAssetOperation(changeData);
                InsertTrashAssetOperation(changeData);
                InsertHiddenlAssetOperation(changeData);
            }
        }
    }

    for (auto &item : notifyInfos_) {
        NotifyInfoInner notifyInfo;
        notifyInfo.tableType = NotifyTableType::PHOTOS;
        notifyInfo.operationType = item.first;
        NotifyLevel level;
        notifyInfo.notifyLevel = level;
        auto &changeDatas = item.second;
        bool recordFirst = false;
        for (auto &changeData : changeDatas) {
            notifyInfo.infos.push_back(changeData);
            PrintNotifyLog(item.first, changeDatas.size(), recordFirst, notifyInfo, changeData);
        }
        // 调用发送通知接口
        Notification::MediaLibraryNotifyNew::AddItem(notifyInfo);
    }
}

void AssetChangeNotifyExecution::NotifyYuvReady(const PhotoAssetChangeData &changeData)
{
    InsertNotifyInfo(ASSET_OPERATION_YUV_READY, changeData);
    NotifyInfoInner notifyInfo;
    notifyInfo.tableType = NotifyTableType::PHOTOS;
    notifyInfo.operationType = ASSET_OPERATION_YUV_READY;
    NotifyLevel level;
    notifyInfo.notifyLevel = level;
    auto &changeDatas = notifyInfos_[ASSET_OPERATION_YUV_READY];
    bool recordFirst = false;
    for (auto &changeData : changeDatas) {
        notifyInfo.infos.push_back(changeData);
        PrintNotifyLog(ASSET_OPERATION_YUV_READY, changeDatas.size(), recordFirst, notifyInfo, changeData);
    }
    // 调用发送通知接口
    Notification::MediaLibraryNotifyNew::AddItem(notifyInfo);
}

void AssetChangeNotifyExecution::InsertNotifyInfo(AssetRefreshOperation operationType,
    const PhotoAssetChangeData &changeData)
{
    PhotoAssetChangeData modifiedChangeData = changeData;
    if ((operationType & ASSET_ADD) == ASSET_ADD) {
        modifiedChangeData.infoBeforeChange_.fileId_ = INVALID_INT32_VALUE;
    } else if ((operationType & ASSET_REMOVE) == ASSET_REMOVE) {
        modifiedChangeData.infoAfterChange_.fileId_ = INVALID_INT32_VALUE;
    }
    auto iter = notifyInfos_.find(operationType);
    if (iter == notifyInfos_.end()) {
        vector<PhotoAssetChangeData> infos;
        infos.push_back(modifiedChangeData);
        notifyInfos_.emplace(operationType, infos);
    } else {
        auto &infos = iter->second;
        infos.push_back(modifiedChangeData);
    }
}

AssetRefreshOperation AssetChangeNotifyExecution::GetAddOperation(const PhotoAssetChangeInfo &changeInfo)
{
    if (ShareAssetHelper::IsAsset(changeInfo)) {
        return ASSET_OPERATION_ADD_SHARE;
    }
    if (AlbumAssetHelper::IsCommonSystemAsset(changeInfo)) {
        return ASSET_OPERATION_ADD;
    }
    if (TrashAssetHelper::IsAsset(changeInfo)) {
        return ASSET_OPERATION_ADD_TRASH;
    }
    if (HiddenAssetHelper::IsAsset(changeInfo)) {
        return ASSET_OPERATION_ADD_HIDDEN;
    }
    return ASSET_OPERATION_UNDEFINED;
}

AssetRefreshOperation AssetChangeNotifyExecution::GetRemoveOperation(const PhotoAssetChangeInfo &changeInfo)
{
    if (ShareAssetHelper::IsAsset(changeInfo)) {
        return ASSET_OPERATION_REMOVE_SHARE;
    }
    if (AlbumAssetHelper::IsCommonSystemAsset(changeInfo)) {
        return ASSET_OPERATION_REMOVE;
    }
    if (TrashAssetHelper::IsAsset(changeInfo)) {
        return ASSET_OPERATION_REMOVE_TRASH;
    }
    if (HiddenAssetHelper::IsAsset(changeInfo)) {
        return ASSET_OPERATION_REMOVE_HIDDEN;
    }
    return ASSET_OPERATION_UNDEFINED;
}

void AssetChangeNotifyExecution::InsertNormalAssetOperation(const PhotoAssetChangeData &changeData)
{
    NotifyAssetType before =
        AlbumAssetHelper::IsCommonSystemAsset(changeData.infoBeforeChange_) ? ASSET_NORMAL : ASSET_INVALID;
    NotifyAssetType after =
        AlbumAssetHelper::IsCommonSystemAsset(changeData.infoAfterChange_) ? ASSET_NORMAL : ASSET_INVALID;
    MEDIA_DEBUG_LOG("before: 0x%{public}x, after: 0x%{public}x", static_cast<int32_t>(before),
        static_cast<int32_t>(after));
    for (auto &operationCal : NORMAL_ASSET_OPERATION_CALS) {
        if (operationCal.first({before, after})) {
            InsertNotifyInfo(operationCal.second, changeData);
            MEDIA_DEBUG_LOG("insert normal operation: %{public}x", operationCal.second);
            break;
        }
    }
}

void AssetChangeNotifyExecution::InsertTrashAssetOperation(const PhotoAssetChangeData &changeData)
{
    NotifyAssetType before = TrashAssetHelper::IsAsset(changeData.infoBeforeChange_) ? ASSET_TRASH : ASSET_INVALID;
    NotifyAssetType after = TrashAssetHelper::IsAsset(changeData.infoAfterChange_) ? ASSET_TRASH : ASSET_INVALID;
    MEDIA_DEBUG_LOG("before: 0x%{public}x, after: 0x%{public}x", static_cast<int32_t>(before),
        static_cast<int32_t>(after));
    for (auto &operationCal : TRASH_ASSET_OPERATION_CALS) {
        if (operationCal.first({before, after})) {
            InsertNotifyInfo(operationCal.second, changeData);
            MEDIA_DEBUG_LOG("insert trash operation: %{public}x", operationCal.second);
            break;
        }
    }
}

void AssetChangeNotifyExecution::InsertHiddenlAssetOperation(const PhotoAssetChangeData &changeData)
{
    NotifyAssetType before = HiddenAssetHelper::IsAsset(changeData.infoBeforeChange_) ? ASSET_HIDDEN : ASSET_INVALID;
    NotifyAssetType after = HiddenAssetHelper::IsAsset(changeData.infoAfterChange_) ? ASSET_HIDDEN : ASSET_INVALID;
    MEDIA_DEBUG_LOG("before: 0x%{public}x, after: 0x%{public}x", static_cast<int32_t>(before),
        static_cast<int32_t>(after));
    for (auto &operationCal : HIDDEN_ASSET_OPERATION_CALS) {
        if (operationCal.first({before, after})) {
            InsertNotifyInfo(operationCal.second, changeData);
            MEDIA_DEBUG_LOG("insert hidden operation: %{public}x", operationCal.second);
            break;
        }
    }
}

void AssetChangeNotifyExecution::InsertShareAssetOperation(const PhotoAssetChangeData &changeData)
{
    InsertNotifyInfo(ASSET_OPERATION_UPDATE_SHARE, changeData);
}

} // namespace Media
} // namespace OHOS