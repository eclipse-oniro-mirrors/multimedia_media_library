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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_SHARE_ALBUM_MEMBER_SERVICE_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_SHARE_ALBUM_MEMBER_SERVICE_H

#include <map>
#include <vector>

#include "media_column.h"
#include "rdb_store.h"
#include "photos_dto.h"
#include "photo_album_po.h"
#include "photo_album_dto.h"
#include "on_fetch_records_album_vo.h"
#include "dataobs_mgr_changeinfo.h"
#include "cloud_media_share_album_member_dao.h"
#include "cloud_media_common_dao.h"
#include "cloud_media_define.h"
#include "media_operate_result.h"

namespace OHOS::Media::CloudSync {
using ChangeType = OHOS::AAFwk::ChangeInfo::ChangeType;
class EXPORT CloudMediaShareAlbumMemberService {
public:
    int32_t HandleShareAlbumMembers(PhotoAlbumDto &record);
    int32_t HandleDeleteMembers(const PhotoAlbumDto &record);

private:
    int32_t HandleShareAlbumMembersInner(const PhotoAlbumDto &record);

private:
    CloudMediaShareAlbumMemberDao shareAlbumDao_;
    CloudMediaCommonDao commonDao_;
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_SHARE_ALBUM_MEMBER_SERVICE_H