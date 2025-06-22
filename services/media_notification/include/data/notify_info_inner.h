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

#ifndef OHOS_MEDIA_NOTIFY_INFO_INNER_H
#define OHOS_MEDIA_NOTIFY_INFO_INNER_H

#include <string>
#include <vector>
#include <variant>

#include "photo_asset_change_info.h"
#include "album_change_info.h"

namespace OHOS {
namespace Media {
namespace Notification {
enum NotifyTableType {
    PHOTOS,
    PHOTO_ALBUM,
    ANALYSIS_ALBUM
};
const int32_t ASSET_ADD = 0x100;
const int32_t ASSET_REMOVE = 0x200;
const int32_t ASSET_OPERATION_ADD_BASE = 0x10000;
const int32_t ASSET_OPERATION_REMOVE_BASE = 0x20000;
const int32_t ASSET_OPERATION_UPDATE_BASE = 0x40000;
const int32_t ASSET_OPERATION_UPDATE_HIDDEN_BASE = 0x80000;
const int32_t ASSET_OPERATION_UPDATE_TRASH_BASE = 0x100000;
const int32_t ASSET_OPERATION_OTHER_BASE = 0x200000;
enum AssetRefreshOperation {
    ASSET_OPERATION_UNDEFINED,
    // 普通资产
    ASSET_OPERATION_ADD = ASSET_OPERATION_ADD_BASE | ASSET_ADD | 0X1,
    ASSET_OPERATION_ADD_HIDDEN = ASSET_OPERATION_ADD_BASE | ASSET_ADD | 0x2, // 云同步
    ASSET_OPERATION_ADD_TRASH = ASSET_OPERATION_ADD_BASE | ASSET_ADD | 0x4, // 回收站
 
    ASSET_OPERATION_REMOVE = ASSET_OPERATION_REMOVE_BASE | ASSET_REMOVE | 0x1,
    ASSET_OPERATION_REMOVE_HIDDEN = ASSET_OPERATION_REMOVE_BASE | ASSET_REMOVE | 0x2,
    ASSET_OPERATION_REMOVE_TRASH = ASSET_OPERATION_REMOVE_BASE | ASSET_REMOVE | 0x4,
 
    ASSET_OPERATION_UPDATE_NORMAL = ASSET_OPERATION_UPDATE_BASE | 0x1,
    ASSET_OPERATION_UPDATE_ADD_NORMAL  = ASSET_OPERATION_UPDATE_BASE | ASSET_ADD | 0x2,
    ASSET_OPERATION_UPDATE_REMOVE_NORMAL = ASSET_OPERATION_UPDATE_BASE | ASSET_REMOVE | 0x4,
 
    ASSET_OPERATION_UPDATE_HIDDEN = ASSET_OPERATION_UPDATE_HIDDEN_BASE | 0x1,
    ASSET_OPERATION_UPDATE_ADD_HIDDEN = ASSET_OPERATION_UPDATE_HIDDEN_BASE | ASSET_ADD | 0x2,
    ASSET_OPERATION_UPDATE_REMOVE_HIDDEN = ASSET_OPERATION_UPDATE_HIDDEN_BASE | ASSET_REMOVE | 0x4,
    
    ASSET_OPERATION_UPDATE_TRASH = ASSET_OPERATION_UPDATE_TRASH_BASE | 0x1,
    ASSET_OPERATION_UPDATE_ADD_TRASH = ASSET_OPERATION_UPDATE_TRASH_BASE | ASSET_ADD | 0x2,
    ASSET_OPERATION_UPDATE_REMOVE_TRASH = ASSET_OPERATION_UPDATE_TRASH_BASE | ASSET_REMOVE | 0x4,
	
    ASSET_OPERATION_TRASH = ASSET_OPERATION_OTHER_BASE,
    ASSET_OPERATION_UNTRASH,
    ASSET_OPERATION_HIDDEN,
    ASSET_OPERATION_UNHIDDEN,

    ASSET_OPERATION_RECHECK,
};
 
enum AlbumRefreshOperation {
    ALBUM_OPERATION_UNDEFINED,
    ALBUM_OPERATION_ADD,
    ALBUM_OPERATION_REMOVE,
    ALBUM_OPERATION_UPDATE,
    ALBUM_OPERATION_UPDATE_HIDDEN,
    ALBUM_OPERATION_UPDATE_TRASH,
    ALBUM_OPERATION_RECHECK,
};

enum Priority {
    NORMAL,
    EMERGENCY,
};

struct NotifyLevel {
    Priority priority = Priority::NORMAL;
    int32_t waitLoopCnt = 1;
};

struct NotifyInfoInner {
    NotifyTableType tableType;
    std::vector<std::variant<AccurateRefresh::PhotoAssetChangeData, AccurateRefresh::AlbumChangeData>> infos;
    std::variant<AssetRefreshOperation, AlbumRefreshOperation> operationType;
    NotifyLevel notifyLevel;
};
} // namespace Notification
} // namespace Media
} // namespace OHOS

#endif  // OHOS_MEDIA_NOTIFY_INFO_INNER_H