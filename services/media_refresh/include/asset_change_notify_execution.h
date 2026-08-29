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

#ifndef OHOS_MEDIALIBRARY_ASSET_CHANGE_NOTIFY_EXECUTION_H
#define OHOS_MEDIALIBRARY_ASSET_CHANGE_NOTIFY_EXECUTION_H

#include <functional>
#include <map>
#include <vector>

#include "notify_info_inner.h"
#include "photo_asset_change_info.h"

namespace OHOS {
namespace Media::AccurateRefresh {

const int32_t OPERATION_HIDDEN_FLAG = 0x1;
const int32_t OPERATION_TRASH_FLAG = 0x2;
const int32_t OPERATION_SHARE_FLAG = 0x4;
const int32_t OPERATION_NONE_FLAG = 0x100;
enum NotifyAssetType {
    ASSET_NORMAL = 0,
    ASSET_HIDDEN = OPERATION_HIDDEN_FLAG,
    ASSET_TRASH = OPERATION_TRASH_FLAG,
    ASSET_SHARE = OPERATION_SHARE_FLAG,
    ASSET_TRASH_HIDDEN = OPERATION_TRASH_FLAG + OPERATION_HIDDEN_FLAG,
    ASSET_INVALID = OPERATION_NONE_FLAG,
};

class AssetChangeNotifyExecution {
public:
    void Notify(const std::vector<PhotoAssetChangeData> &changeDatas);
    void NotifyYuvReady(const PhotoAssetChangeData &changeData);
private:
    void InsertNotifyInfo(Notification::AssetRefreshOperation operationType, const PhotoAssetChangeData &changeData);
    Notification::AssetRefreshOperation GetAddOperation(const PhotoAssetChangeInfo &changeInfo);
    Notification::AssetRefreshOperation GetRemoveOperation(const PhotoAssetChangeInfo &changeInfo);
    void InsertNormalAssetOperation(const PhotoAssetChangeData &changeData);
    void InsertTrashAssetOperation(const PhotoAssetChangeData &changeData);
    void InsertHiddenlAssetOperation(const PhotoAssetChangeData &changeData);
    void InsertShareAssetOperation(const PhotoAssetChangeData &changeData);

private:
    std::map<Notification::AssetRefreshOperation, std::vector<PhotoAssetChangeData>> notifyInfos_;
};

} // namespace Media
} // namespace OHOS

#endif