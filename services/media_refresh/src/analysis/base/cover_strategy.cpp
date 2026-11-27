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
#include "cover_strategy.h"
#include "media_string_utils.h"

namespace OHOS {
namespace Media {
namespace AccurateRefresh {

void CoverStrategyBase::RecordPotentialCoverChange(const PhotoAssetChangeData &data,
    AnalysisAlbumRefreshInfo &info, int32_t assetDelta)
{
    if (assetDelta < 0) { // 删除封面候选
        UpdateDeleteFileId(data, info);
        return;
    }

    // 新增候选，可能成为封面
    if (assetDelta > 0 && NeedUpdateAddCover(data, info)) {
        UpdateAddCover(data, info);
        return;
    }
}

bool CoverStrategyBase::NeedCoverRefresh(const UpdateAlbumData &baseInfo, AnalysisAlbumRefreshInfo &info)
{
    CHECK_AND_RETURN_RET_INFO_LOG(ShouldRefreshCover(baseInfo, info), false,
        "No need to change cover, albumId: %{public}d", baseInfo.albumId);
    info.needRefreshCover_ = true;
    return info.needRefreshCover_;
}

void CoverStrategyBase::UpdateDeleteFileId(const PhotoAssetChangeData &data, AnalysisAlbumRefreshInfo &info)
{
    info.removeFileIds_.insert(data.infoBeforeChange_.fileId_);
}

bool CoverStrategyBase::NeedUpdateAddCover(const PhotoAssetChangeData &data, AnalysisAlbumRefreshInfo &info)
{
    CHECK_AND_RETURN_RET(info.deltaAddCover_.fileId_ != INVALID_INT32_VALUE, true);

    return std::make_tuple(data.infoAfterChange_.dateTakenMs_, data.infoAfterChange_.fileId_) >
           std::make_tuple(info.deltaAddCover_.dateTakenMs_, info.deltaAddCover_.fileId_);
}

void CoverStrategyBase::UpdateAddCover(const PhotoAssetChangeData &data, AnalysisAlbumRefreshInfo &info)
{
    info.deltaAddCover_ = data.infoAfterChange_;
}

std::string CoverStrategyBase::GetPhotoId(const std::string &uri)
{
    if (uri.compare(0, PhotoColumn::PHOTO_URI_PREFIX.size(),
        PhotoColumn::PHOTO_URI_PREFIX) != 0) {
        return "";
    }
    std::string tmp = uri.substr(PhotoColumn::PHOTO_URI_PREFIX.size());
    auto position = tmp.find_first_of('/');
    CHECK_AND_RETURN_RET(position != std::string::npos, "");
    return tmp.substr(0, position);
}

bool CoverStrategyBase::isCurrentCoverDeleted(const UpdateAlbumData &oldAlbum, const AnalysisAlbumRefreshInfo &info)
{
    CHECK_AND_RETURN_RET_LOG(oldAlbum.albumCoverUri != "", false,
        "Empty cover before change, nothing to be deleted, id: %{public}d", oldAlbum.albumId);

    std::string oldCoverFileIdStr = GetPhotoId(oldAlbum.albumCoverUri);

    CHECK_AND_RETURN_RET_LOG(MediaStringUtils::IsValidInteger(oldCoverFileIdStr), true,
        "Invalid cover input, need refresh cover, id: %{public}d", oldAlbum.albumId);

    int32_t oldCoverFileId = std::stoi(oldCoverFileIdStr);

    CHECK_AND_RETURN_RET_LOG(info.removeFileIds_.count(oldCoverFileId) <= 0, true,
        "Cover changed due to delete, id: %{public}d", oldAlbum.albumId);

    return false;
}

bool CoverStrategyBase::ShouldRefreshCover(const UpdateAlbumData &oldAlbum, const AnalysisAlbumRefreshInfo &info)
{
    CHECK_AND_RETURN_RET_LOG(!info.isForceRefresh_, true,
        "Force refresh analysis album, id: %{public}d", oldAlbum.albumId);

    CHECK_AND_RETURN_RET_LOG(!isCurrentCoverDeleted(oldAlbum, info), true,
        "Invalid cover due to delete, id: %{public}d", oldAlbum.albumId);

    CHECK_AND_RETURN_RET_LOG(info.deltaAddCover_.fileId_ == INVALID_INT32_VALUE, true,
        "Potential new cover added, id: %{public}d", oldAlbum.albumId);

    return false;
}

} // namespace AccurateRefresh
} // namespace Media
} // namespace OHOS
