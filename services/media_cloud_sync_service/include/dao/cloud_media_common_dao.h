/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_COMMON_DAO_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_COMMON_DAO_H

#include <string>
#include <vector>
#include <map>

#include "photos_po.h"
#include "photo_album_po.h"

namespace OHOS::Media::CloudSync {
using namespace OHOS::Media::ORM;
class CloudMediaCommonDao {
public:
    CloudMediaCommonDao() = default;
    ~CloudMediaCommonDao() = default;

public:
    int32_t QueryLocalByCloudId(const std::vector<std::string> &cloudIds, const std::vector<std::string> &columns,
        std::vector<PhotosPo> &result);
    int32_t QueryLocalMap(const int32_t &fileId, std::map<int32_t, int32_t> &localMapIds);
    int32_t QueryPhotoByCloudId(const std::string &cloudId, std::optional<PhotosPo> &photoInfoOp) const;
    int32_t QueryPhotoByFilePath(const std::string &filePath, std::optional<PhotosPo> &photoInfoOp) const;
    int32_t QueryPhotoAlbumByAlbumId(const int32_t albumId, std::optional<PhotoAlbumPo> &photoAlbumInfoOp) const;
    int32_t QueryPhotoAlbumByCloudId(const std::string cloudId, std::optional<PhotoAlbumPo> &photoAlbumInfoOp) const;
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_COMMON_DAO_H