/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "Media_Cloud_Service"

#include "cloud_media_photos_album_hidden_service.h"

#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_unistore_manager.h"

namespace OHOS::Media::CloudSync {
int32_t CloudMediaPhotosAlbumHiddenService::UpdateEmptyAlbumHidden()
{
    MEDIA_INFO_LOG("enter UpdateEmptyAlbumHidden");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "UpdateEmptyAlbumHidden get store failed.");
    int32_t ret = rdbStore->ExecuteSql(SQL_UPDATE_EMPTY_PHOTO_ALBUM_HIDDEN);
    MEDIA_INFO_LOG("UpdateEmptyAlbumHidden: Update albums, ret: %{public}d", ret);
    return ret;
}
} // namespace OHOS::Media::CloudSync