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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_PHOTOS_RENAME_SERVICE_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_PHOTOS_RENAME_SERVICE_H

#include <string>

#include "cloud_media_define.h"
#include "photo_displayname_operation.h"
#include "photos_dto.h"
#include "photos_po.h"
#include "asset_accurate_refresh.h"

namespace OHOS::Media::CloudSync {
using namespace OHOS::Media::ORM;
class EXPORT CloudMediaPhotosRenameService {
public:
    int32_t HandleSameNameRename(
        const PhotosDto &photo, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);

private:
    int32_t FindNextDisplayName(const PhotosPo &photoInfo, std::string &nextDisplayName);
    int32_t FindNextStoragePath(
        const PhotosPo &photoInfo, const std::string &nextDisplayName, std::string &nextStoragePath);
    int32_t RenameAsset(const PhotosPo &photoInfo, const std::string &nextDisplayName,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t FindTitleAndSuffix(const std::string &displayName, std::string &title, std::string &suffix);

private:
    PhotoDisplayNameOperation displayNameOperation_;
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_PHOTOS_RENAME_SERVICE_H