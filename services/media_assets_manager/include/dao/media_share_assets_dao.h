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

#ifndef OHOS_MEDIA_SHARE_ASSETS_DAO_H
#define OHOS_MEDIA_SHARE_ASSETS_DAO_H

#include <stdint.h>
#include <string>
#include <vector>

#include "medialibrary_errno.h"
#include "photos_po.h"

// LCOV_EXCL_START
namespace OHOS::Media {
using namespace OHOS::Media::ORM;

class MediaShareAssetsDao {
public:
    MediaShareAssetsDao() = default;
    ~MediaShareAssetsDao() = default;

    // 删除共享场景下的空相册
    int32_t DeleteShareAlbums();
    // 查询待更新标记的共享资产, 结果通过 updateFileIds 返回
    bool HasShareAssetToMarkDeleted(std::vector<std::string> &updateFileIds, const std::string &lastFileId);
    // 批量标记共享资产为待删除
    int32_t MarkDeletedAndClearCloudInfo(const std::vector<std::string> &updateFileIds);
    // 批量删除已清理的共享资产数据库记录
    int32_t DeleteShareAssets(const std::vector<std::string> &fileIds);
    int32_t GetShareAssetToRemove(std::vector<PhotosPo> &photoInfoList);

private:
    const std::string SQL_SHARE_ALBUM_QUERY_ALL = "SELECT * FROM PhotoAlbum WHERE album_type = 8192;";
    const std::string SQL_SHARE_ALBUM_DELETE_ALL = "DELETE FROM PhotoAlbum WHERE album_type = 8192;";
};
} // namespace OHOS::Media
// LCOV_EXCL_STOP
#endif // OHOS_MEDIA_SHARE_ASSETS_DAO_H
