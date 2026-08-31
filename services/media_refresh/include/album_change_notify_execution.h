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

#ifndef OHOS_MEDIALIBRARY_ALBUM_CHANGE_NOTIFY_EXECUTION_H
#define OHOS_MEDIALIBRARY_ALBUM_CHANGE_NOTIFY_EXECUTION_H

#include <map>
#include <vector>

#include "album_change_info.h"
#include "notify_info_inner.h"

namespace OHOS {
namespace Media::AccurateRefresh {

class AlbumChangeNotifyExecution {
public:
    void Notify(std::vector<AlbumChangeData> changeDatas);

private:
    void InsertNotifyInfo(Notification::AlbumRefreshOperation operation, const AlbumChangeData &changeData);
    void InsertNotifyInfoForAdd(const AlbumChangeData& changeData);
    void AddInfosToNewNotify();
    bool IsShareAlbumChangeInfo(const AlbumChangeData& changeData);
    void HandleInsertShareNotifyInfo(AlbumChangeData& changeData);

private:
    std::map<Notification::AlbumRefreshOperation, std::vector<AlbumChangeData>> notifyInfos_;
};

} // namespace Media
} // namespace OHOS

#endif