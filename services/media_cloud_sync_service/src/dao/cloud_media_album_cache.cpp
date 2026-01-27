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

#define MLOG_TAG "Media_Cloud_Dao"

#include "cloud_media_album_cache.h"

#include <string>
#include <utime.h>
#include <vector>

#include "medialibrary_errno.h"
#include "media_log.h"
#include "cloud_media_sync_utils.h"

namespace OHOS::Media::CloudSync {
// LCOV_EXCL_START
bool CloudMediaAlbumCache::IsEmpty()
{
    return this->albumInfoList_.Empty();
}

int32_t CloudMediaAlbumCache::SetAlbumCache(const std::vector<PhotoAlbumPo> &albumInfoList)
{
    MEDIA_INFO_LOG("SetAlbumCache, before size: %{public}zu, after size: %{public}zu",
        this->albumInfoList_.Size(),
        albumInfoList.size());
    this->albumInfoList_.Clear();
    for (const auto &item : albumInfoList) {
        this->albumInfoList_.PushBack(item);
    }
    return E_OK;
}

int32_t CloudMediaAlbumCache::ClearAlbumCache()
{
    MEDIA_INFO_LOG("ClearAlbumCache, size: %{public}zu", this->albumInfoList_.Size());
    this->albumInfoList_.Clear();
    return E_OK;
}

std::string CloudMediaAlbumCache::ToLower(const std::string &str)
{
    std::string lowerStr;
    std::transform(
        str.begin(), str.end(), std::back_inserter(lowerStr), [](unsigned char c) { return std::tolower(c); });
    return lowerStr;
}

int32_t CloudMediaAlbumCache::QueryAlbumByCloudId(const std::string &cloudId, std::optional<PhotoAlbumPo> &albumInfo)
{
    CHECK_AND_RETURN_RET(!cloudId.empty(), E_INVALID_VALUES);
    CHECK_AND_RETURN_RET_LOG(!this->albumInfoList_.Empty(), E_QUERY_CONTENT_IS_EMPTY, "albumInfoList_ is empty.");
    std::vector<PhotoAlbumPo> albumInfos = this->albumInfoList_.ToVector();
    auto it = std::find_if(albumInfos.begin(), albumInfos.end(), [&](const PhotoAlbumPo &info) {
        return info.cloudId.value_or("") == cloudId;
    });
    bool isFound = it != albumInfos.end();
    if (isFound) {
        albumInfo = *it;
    }
    MEDIA_INFO_LOG("QueryAlbumByCloudId, cloudId: %{public}s, isFound: %{public}d", cloudId.c_str(), isFound);
    return E_OK;
}

int32_t CloudMediaAlbumCache::QueryAlbumBylPath(const std::string &lPath, std::optional<PhotoAlbumPo> &albumInfo)
{
    CHECK_AND_RETURN_RET(!lPath.empty(), E_INVALID_VALUES);
    CHECK_AND_RETURN_RET_LOG(!this->albumInfoList_.Empty(), E_QUERY_CONTENT_IS_EMPTY, "albumInfoList_ is empty.");
    std::vector<PhotoAlbumPo> albumInfos = this->albumInfoList_.ToVector();
    std::string lowerlPath = this->ToLower(lPath);
    auto it = std::find_if(albumInfos.begin(), albumInfos.end(), [&](const PhotoAlbumPo &info) {
        return this->ToLower(info.lpath.value_or("")) == lowerlPath;
    });
    bool isFound = it != albumInfos.end();
    if (isFound) {
        albumInfo = *it;
    }
    MEDIA_INFO_LOG("QueryAlbumBylPath, lPath: %{public}s, isFound: %{public}d", lPath.c_str(), isFound);
    return E_OK;
}

int32_t CloudMediaAlbumCache::QueryAlbumBySourcePath(
    const std::string &sourcePath, std::optional<PhotoAlbumPo> &albumInfo)
{
    CHECK_AND_RETURN_RET(!sourcePath.empty(), E_INVALID_VALUES);
    CHECK_AND_RETURN_RET_LOG(!this->albumInfoList_.Empty(), E_QUERY_CONTENT_IS_EMPTY, "albumInfoList_ is empty.");
    std::vector<PhotoAlbumPo> albumInfos = this->albumInfoList_.ToVector();
    std::string lPath = CloudMediaSyncUtils::GetLpathFromSourcePath(sourcePath);
    return this->QueryAlbumBylPath(lPath, albumInfo);
}
// LCOV_EXCL_STOP
}  // namespace OHOS::Media::CloudSync