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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_SHARE_ALBUM_MEMBER_DAO_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_SHARE_ALBUM_MEMBER_DAO_H

#include <string>
#include <vector>
#include <unordered_map>

#include "media_column.h"
#include "rdb_store.h"
#include "result_set.h"
#include "medialibrary_db_const.h"
#include "cloud_media_sync_const.h"
#include "photo_album_dto.h"
#include "share_member_column.h"
#include "share_member_data_vo.h"
#include "medialibrary_rdbstore.h"
#include "cloud_media_common_dao.h"
#include "cloud_media_define.h"

namespace OHOS::Media::CloudSync {
using namespace OHOS::Media::ORM;

class EXPORT CloudMediaShareAlbumMemberDao {
public:
    CloudMediaShareAlbumMemberDao() = default;
    ~CloudMediaShareAlbumMemberDao() = default;

public:
    int32_t HandleAlbumMembers(int32_t albumId, const std::vector<ShareMemberDataDto> &members);
    int32_t DeleteAlbumMembers(int32_t albumId);
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_SHARE_ALBUM_MEMBER_DAO_H