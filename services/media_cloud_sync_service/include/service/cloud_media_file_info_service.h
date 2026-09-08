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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_FILE_INFO_SERVICE_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_FILE_INFO_SERVICE_H

#include <string>

#include "cloud_media_define.h"
#include "cloud_media_pull_data_dto.h"
#include "photos_po.h"
#include "cloud_media_common_dao.h"

namespace OHOS::Media::CloudSync {
using namespace OHOS::Media::ORM;
class EXPORT CloudMediaFileInfoService {
public:
    void FixFileInfo(CloudMediaPullDataDto &pullData);

private:
    void FixFileInfoWithLocal(CloudMediaPullDataDto &pullData, const PhotosPo &photoInfo);
    void FixFileInfoWithCloudOnly(CloudMediaPullDataDto &pullData);
    void AdjustFileInfoWithLocal(CloudMediaPullDataDto &pullData, PhotosPo &photoInfo);
    bool IsNameNotChanged(const CloudMediaPullDataDto &pullData, const PhotosPo &photoInfo) const;
    bool IsHiddenNotChanged(const CloudMediaPullDataDto &pullData, const PhotosPo &photoInfo) const;
    bool IsTrashedNotChanged(const CloudMediaPullDataDto &pullData, const PhotosPo &photoInfo) const;
    bool IsAlbumOrSourcePathNotChanged(CloudMediaPullDataDto &pullData, PhotosPo &photoInfo) const;
    bool IsPhotoAlbumNotChanged(const CloudMediaPullDataDto &pullData, PhotosPo &photoInfo) const;

private:
    CloudMediaCommonDao commonDao_;
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_FILE_INFO_SERVICE_H