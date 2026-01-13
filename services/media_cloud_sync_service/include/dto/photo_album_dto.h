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

#ifndef OHOS_MEDIA_CLOUD_SYNC_PHOTO_ALBUM_DTO_H
#define OHOS_MEDIA_CLOUD_SYNC_PHOTO_ALBUM_DTO_H

#include <string>
#include <vector>
#include "cloud_media_define.h"
#include "cloud_media_sync_const.h"
#include "cloud_error_detail_vo.h"
#include "medialibrary_errno.h"
#include "photo_album_po.h"

namespace OHOS::Media::CloudSync {
using namespace OHOS::Media::ORM;
class EXPORT PhotoAlbumDto {
public:
    int32_t albumId;
    int32_t albumType;
    int32_t albumSubType;
    std::string albumName;
    std::string lPath;
    std::string bundleName;
    int32_t priority;
    std::string cloudId;
    std::string newCloudId;
    std::string localLanguage;
    int64_t albumDateCreated;
    int64_t albumDateAdded;
    int64_t albumDateModified;
    bool isDelete;
    bool isSuccess;
    int32_t coverUriSource;
    std::string coverCloudId;
    int32_t serverErrorCode;
    ErrorType errorType;
    std::vector<CloudErrorDetail> errorDetails;
    std::optional<PhotoAlbumPo> localAlbumInfo;

public:
    std::string ToString() const;
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_PHOTO_ALBUM_DTO_H