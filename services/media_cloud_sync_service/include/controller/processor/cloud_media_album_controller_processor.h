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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_ALBUM_CONTROLLER_PROCESSOR_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_ALBUM_CONTROLLER_PROCESSOR_H

#include <string>
#include <vector>

#include "photo_album_po.h"
#include "cloud_mdkrecord_photo_album_vo.h"
#include "on_create_records_album_vo.h"
#include "on_mdirty_records_album_vo.h"
#include "photo_album_dto.h"
#include "report_failure_vo.h"
#include "report_failure_dto.h"
#include "cloud_media_define.h"

namespace OHOS::Media::CloudSync {
class EXPORT CloudMediaAlbumControllerProcessor {
public:
    CloudMdkRecordPhotoAlbumVo ConvertRecordPoToVo(PhotoAlbumPo record);
    PhotoAlbumDto ConvertToPhotoAlbumDto(const OnCreateRecordsAlbumReqBodyAlbumData &recordVo);
    PhotoAlbumDto ConvertToPhotoAlbumDto(const OnMdirtyAlbumRecord &recordVo);
    void ConvertShareMemberDataFromVoToDto(const std::vector<ShareMemberDataVo> &shareMemberDataVoList,
        std::vector<ShareMemberDataDto> &shareMemberDataDtoList);
    void ConvertShareAlbumDetailFromVoToDto(const std::optional<ShareAlbumDetailVo> &shareAlbumDetailVoOp,
        std::optional<ShareAlbumDetailDto> &shareAlbumDetailDtoOp);
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_ALBUM_CONTROLLER_PROCESSOR_H
