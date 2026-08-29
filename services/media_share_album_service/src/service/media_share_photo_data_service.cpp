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

#define MLOG_TAG "Media_Service"

#include "media_share_photo_data_service.h"

#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"

#include <vector>

namespace OHOS::Media::ShareAlbum {

constexpr int32_t PHOTOS_HIDDEN = 1;
constexpr int32_t PHOTOS_NOT_TRASHED = 0;
constexpr int32_t PHOTOS_IS_SHARED = 1;
int32_t MediaSharePhotoDataService::GetShareAlbumOwnerId(const std::string &data, std::string &shareAlbumOwner)
{
    constexpr int32_t ERR_NOT_FOUND = -1;
    constexpr int32_t ERR_RESULT_NOT_SHARED = -3;
    std::vector<PhotosPo> photosPos;

    int32_t ret = this->photoDataDao_.GetShareAlbumOwnerId(data, photosPos);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to GetShareAlbumOwnerId, ret = %{public}d", ret);
    if (photosPos.empty()) {
        MEDIA_INFO_LOG("Empty result, data:%{public}s", MediaFileUtils::DesensitizePath(data).c_str());
        return ERR_NOT_FOUND;
    }
    if (photosPos.size() > 1) {
        MEDIA_INFO_LOG("More than one result found, data:%{public}s", MediaFileUtils::DesensitizePath(data).c_str());
    }

    const PhotosPo &photosInfo = photosPos.front();
    int32_t hidden = photosInfo.hidden.value_or(0);
    int64_t dateTrashed = photosInfo.dateTrashed.value_or(0);
    if (hidden == PHOTOS_HIDDEN || dateTrashed != PHOTOS_NOT_TRASHED) {
        MEDIA_INFO_LOG("File is hidden or trashed, hidden:%{public}d, dateTrashed:%{public}lld, data:%{public}s",
            hidden, (long long)dateTrashed, MediaFileUtils::DesensitizePath(data).c_str());
        return ERR_NOT_FOUND;
    }

    int32_t photoIsShared = photosInfo.isShared.value_or(0);
    if (photoIsShared != PHOTOS_IS_SHARED) {
        MEDIA_INFO_LOG("Photo is not shared album photo, is_shared:%{public}d, data:%{public}s",
            photoIsShared, MediaFileUtils::DesensitizePath(data).c_str());
        return ERR_RESULT_NOT_SHARED;
    }

    shareAlbumOwner = photosInfo.shareAlbumOwner.value_or("");
    MEDIA_INFO_LOG("Data:%{public}s, shareAlbumOwner:%{public}s",
        MediaFileUtils::DesensitizePath(data).c_str(), shareAlbumOwner.c_str());
    return E_OK;
}

}  // namespace OHOS::Media::ShareAlbum
