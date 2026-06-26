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
#include "asset_accurate_refresh.h"
#include "datashare_predicates.h"
#include "photo_album_column.h"

namespace OHOS::Media::CloudSync {
int32_t CloudMediaPhotosAlbumHiddenService::UpdateEmptyAlbumHidden()
{
    auto albumRefresh = std::make_shared<AccurateRefresh::AlbumAccurateRefresh>();
    CHECK_AND_RETURN_RET_LOG(albumRefresh != nullptr, E_RDB_STORE_NULL,
        "UpdateEmptyAlbumHidden get store failed.");
    NativeRdb::ValuesBucket value;
    value.PutInt(PhotoAlbumColumns::ALBUM_HIDDEN, 1);
    NativeRdb::RdbPredicates rdbPredicates(PhotoAlbumColumns::TABLE);
    rdbPredicates.EqualTo(PhotoAlbumColumns::ALBUM_COUNT, "0");
    rdbPredicates.NotEqualTo(PhotoAlbumColumns::HIDDEN_COUNT, "0");
    rdbPredicates.NotEqualTo(PhotoAlbumColumns::ALBUM_HIDDEN, "1");
    int32_t changedRows = 0;
    int32_t err = albumRefresh->Update(changedRows, value, rdbPredicates);
    CHECK_AND_RETURN_RET_LOG(err == NativeRdb::E_OK, err,
        "UpdateEmptyAlbumHidden failed, err: %{public}d", err);
    albumRefresh->Notify();
    MEDIA_INFO_LOG("UpdateEmptyAlbumHidden success, changedRows: %{public}d", changedRows);
    return err;
}
} // namespace OHOS::Media::CloudSync