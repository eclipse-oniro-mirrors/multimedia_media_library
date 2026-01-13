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

#define MLOG_TAG "AccurateRefresh::AlbumChangeNotifyExecution"

#include "album_change_notify_execution.h"
#include "accurate_debug_log.h"
#include "medialibrary_notify_new.h"

using namespace std;
using namespace OHOS::Media::Notification;
namespace OHOS {
namespace Media::AccurateRefresh {

bool IsNotifyUpdateForHiddenAlbum(const AlbumChangeInfo& beforeChangeInfo, const AlbumChangeInfo& afterChangeInfo)
{
    const std::string EMPTY_STR = "";
    return (afterChangeInfo.hiddenCount_ > 0 && afterChangeInfo.hiddenCoverUri_ != EMPTY_STR &&
        beforeChangeInfo.hiddenCount_ == INVALID_INT32_VALUE && beforeChangeInfo.hiddenCoverUri_ == EMPTY_STR) ?
        true : false;
}

void AlbumChangeNotifyExecution::AddInfosToNewNotify()
{
    for (auto &item : notifyInfos_) {
        NotifyInfoInner notifyInfo;
        notifyInfo.tableType = NotifyTableType::PHOTO_ALBUM;
        notifyInfo.operationType = item.first;
        NotifyLevel level;
        notifyInfo.notifyLevel = level;
        auto &changeDatas = item.second;
        for (auto &changeData : changeDatas) {
            notifyInfo.infos.push_back(changeData);
            ACCURATE_INFO("notify PHOTO_ALBUM info: operationType(0x%{public}x), level(%{public}d), info(%{public}s)",
                static_cast<int32_t>(item.first),
                static_cast<int32_t>(notifyInfo.notifyLevel.priority), changeData.ToString().c_str());
        }
        // 调用发送通知接口
        Notification::MediaLibraryNotifyNew::AddItem(notifyInfo);
    }
}

void AlbumChangeNotifyExecution::InsertNotifyInfoForAdd(const AlbumChangeData& changeData)
{
    InsertNotifyInfo(ALBUM_OPERATION_ADD, changeData);
    if (IsNotifyUpdateForHiddenAlbum(changeData.infoBeforeChange_, changeData.infoAfterChange_)) {
        InsertNotifyInfo(ALBUM_OPERATION_UPDATE_HIDDEN, changeData);
    }
}

void AlbumChangeNotifyExecution::Notify(vector<AlbumChangeData> changeDatas)
{
    for (auto &changeData : changeDatas) {
        if (changeData.operation_ == RDB_OPERATION_ADD) {
            InsertNotifyInfoForAdd(changeData);
        } else if (changeData.operation_ == RDB_OPERATION_REMOVE) {
            InsertNotifyInfo(ALBUM_OPERATION_REMOVE, changeData);
        } else if (changeData.operation_ == RDB_OPERATION_UPDATE) {
            auto subType = changeData.infoAfterChange_.albumSubType_;
            // 隐藏相册
            if (subType == static_cast<int32_t>(PhotoAlbumSubType::HIDDEN)) {
                InsertNotifyInfo(ALBUM_OPERATION_UPDATE_HIDDEN, changeData);
                continue;
            }
            // 回收站相册
            if (subType == static_cast<int32_t>(PhotoAlbumSubType::TRASH)) {
                InsertNotifyInfo(ALBUM_OPERATION_UPDATE_TRASH, changeData);
                continue;
            }

            // 相册删除逻辑
            bool isDelete = changeData.infoAfterChange_.dirty_ == static_cast<int32_t>(DirtyType::TYPE_DELETED) &&
                changeData.infoBeforeChange_.dirty_ != static_cast<int32_t>(DirtyType::TYPE_DELETED);
            if (isDelete) {
                changeData.isDelete_ = true;
                InsertNotifyInfo(ALBUM_OPERATION_REMOVE, changeData);
                continue;
            }

            // 相册新增逻辑
            bool isCreate = changeData.infoBeforeChange_.dirty_ == static_cast<int32_t>(DirtyType::TYPE_DELETED) &&
                changeData.infoAfterChange_.dirty_ != static_cast<int32_t>(DirtyType::TYPE_DELETED);
            if (isCreate) {
                InsertNotifyInfoForAdd(changeData);
                continue;
            }

            if (changeData.IsAlbumHiddenInfoChange()) {
                InsertNotifyInfo(ALBUM_OPERATION_UPDATE_HIDDEN, changeData);
            }
            if (changeData.IsAlbumInfoChange()) {
                InsertNotifyInfo(ALBUM_OPERATION_UPDATE, changeData);
            }
        }
    }
    AddInfosToNewNotify();
}

void AlbumChangeNotifyExecution::InsertNotifyInfo(AlbumRefreshOperation operation, const AlbumChangeData &changeData)
{
    AlbumChangeData albumChangeData = changeData;
    if (AccurateRefresh::accurateDebugLevel >= AccurateRefresh::accurateDebugLevelHigh) {
        HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} origin data: %{public}s",
            MLOG_TAG, __FUNCTION__, __LINE__, changeData.ToString(true).c_str());
    }
    // screenshot do not need to send notify
    bool isSrceenshot =
        albumChangeData.infoBeforeChange_.albumSubType_ == static_cast<int32_t>(PhotoAlbumSubType::SCREENSHOT) ||
        albumChangeData.infoAfterChange_.albumSubType_ == static_cast<int32_t>(PhotoAlbumSubType::SCREENSHOT);
    CHECK_AND_RETURN_LOG(!isSrceenshot, "album subtype is screenshot(1029)");
    // end
    if (operation == ALBUM_OPERATION_ADD) {
        albumChangeData.infoBeforeChange_.albumId_ = INVALID_INT32_VALUE;
        ACCURATE_DEBUG("modify remove data: %{public}s", albumChangeData.ToString(true).c_str());
    } else if (operation == ALBUM_OPERATION_REMOVE) {
        albumChangeData.infoAfterChange_.albumId_ = INVALID_INT32_VALUE;
        ACCURATE_DEBUG("modify remove data: %{public}s", albumChangeData.ToString(true).c_str());
    }
    auto iter = notifyInfos_.find(operation);
    if (iter == notifyInfos_.end()) {
        vector<AlbumChangeData> infos;
        infos.push_back(albumChangeData);
        notifyInfos_.emplace(operation, infos);
        return;
    } else {
        auto &infos = iter->second;
        infos.push_back(albumChangeData);
    }
}

} // namespace Media
} // namespace OHOS