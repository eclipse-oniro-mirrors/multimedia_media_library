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

#include "cloud_media_share_album_member_service.h"

#include "cloud_media_context.h"
#include "media_log.h"
#include "medialibrary_errno.h"

namespace OHOS::Media::CloudSync {
int32_t CloudMediaShareAlbumMemberService::HandleShareAlbumMembers(PhotoAlbumDto &record)
{
    this->commonDao_.QueryPhotoAlbumByCloudId(record.cloudId, record.localAlbumInfo);
    return this->HandleShareAlbumMembersInner(record);
}

int32_t CloudMediaShareAlbumMemberService::HandleShareAlbumMembersInner(const PhotoAlbumDto &record)
{
    bool isValid = CloudMediaContext::GetInstance().GetSceneType() == static_cast<int32_t>(SceneType::SHARE);
    CHECK_AND_RETURN_RET(isValid, E_OK);

    isValid = record.shareAlbumDetailDtoOp.has_value();
    CHECK_AND_RETURN_RET_LOG(isValid, E_OK, "no shareAlbumDetail. cloudId: %{public}s", record.cloudId.c_str());

    ShareAlbumDetailDto shareAlbumDetailDto = record.shareAlbumDetailDtoOp.value();
    isValid = !shareAlbumDetailDto.shareMemberDataList.empty();
    CHECK_AND_RETURN_RET_LOG(isValid, E_OK, "no shareAlbumMember. cloudId: %{public}s", record.cloudId.c_str());

    isValid = record.localAlbumInfo.has_value();
    CHECK_AND_RETURN_RET_LOG(isValid, E_OK, "can not find localAlbuminfo. cloudId: %{public}s", record.cloudId.c_str());

    const PhotoAlbumPo &photoAlbumInfo = record.localAlbumInfo.value();
    int32_t albumid = photoAlbumInfo.albumId.value_or(0);

    return this->shareAlbumDao_.HandleAlbumMembers(albumid,
        shareAlbumDetailDto.shareMemberDataList);
}

int32_t CloudMediaShareAlbumMemberService::HandleDeleteMembers(const PhotoAlbumDto &record)
{
    bool isValid = CloudMediaContext::GetInstance().GetSceneType() == static_cast<int32_t>(SceneType::SHARE);
    CHECK_AND_RETURN_RET(isValid, E_OK);

    isValid = record.localAlbumInfo.has_value();
    CHECK_AND_RETURN_RET_LOG(isValid, E_OK, "can not find localAlbuminfo. cloudId: %{public}s", record.cloudId.c_str());

    const PhotoAlbumPo &photoAlbumInfo = record.localAlbumInfo.value();
    return this->shareAlbumDao_.DeleteAlbumMembers(photoAlbumInfo.albumId.value_or(0));
}
}  // namespace OHOS::Media::CloudSync
