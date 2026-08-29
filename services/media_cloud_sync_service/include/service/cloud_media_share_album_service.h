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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_SHARE_ALBUM_SERVICE_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_SHARE_ALBUM_SERVICE_H

#include <string>
#include <vector>

#include "dataobs_mgr_changeinfo.h"
#include "photo_album_dto.h"
#include "cloud_media_album_dao.h"
#include "cloud_media_common_dao.h"

namespace OHOS::Media::CloudSync {
using ChangeType = OHOS::AAFwk::ChangeInfo::ChangeType;
class EXPORT CloudMediaShareAlbumService {
public:
    int32_t OnFetchRecords(std::vector<PhotoAlbumDto> &albumDtoList,
        std::vector<int32_t> &stats, std::vector<std::string> &failedRecords);

private:
    int32_t FindAlbumInfo(PhotoAlbumDto &record);
    int32_t HandleRecord(PhotoAlbumDto &record, ChangeType &changeType,
        std::vector<int32_t> &stats, std::vector<std::string> &failedRecords);
    int32_t PullInsert(const PhotoAlbumDto &record, ChangeType &changeType,
        std::vector<int32_t> &stats, std::vector<std::string> &failedRecords);
    int32_t PullUpdate(const PhotoAlbumDto &record, ChangeType &changeType,
        std::vector<int32_t> &stats, std::vector<std::string> &failedRecords);
    int32_t PullDelete(const PhotoAlbumDto &record, ChangeType &changeType,
        std::vector<int32_t> &stats, std::vector<std::string> &failedRecords);

private:
    CloudMediaCommonDao commonDao_;
    CloudMediaAlbumDao albumDao_;
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_SHARE_ALBUM_SERVICE_H
