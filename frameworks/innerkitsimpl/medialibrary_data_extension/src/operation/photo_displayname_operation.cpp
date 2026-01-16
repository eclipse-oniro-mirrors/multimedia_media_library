/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#define MLOG_TAG "Media_Operation"

#include "photo_displayname_operation.h"

#include "display_name_info.h"

namespace OHOS::Media {
PhotoDisplayNameOperation &PhotoDisplayNameOperation::SetTargetPhotoInfo(const PhotoAssetInfo &photoAssetInfo)
{
    this->photoAssetInfo_ = photoAssetInfo;
    return *this;
}

std::string PhotoDisplayNameOperation::FindDisplayName(const std::shared_ptr<MediaLibraryRdbStore> &rdbStore)
{
    return this->FindDislayName(rdbStore, this->photoAssetInfo_);
}

std::string PhotoDisplayNameOperation::FindDislayName(
    const std::shared_ptr<MediaLibraryRdbStore> &rdbStore, const PhotoAssetInfo &photoAssetInfo)
{
    if (photoAssetInfo.displayName.empty() || rdbStore == nullptr) {
        MEDIA_ERR_LOG("displayName is empty. Object: %{public}s", photoAssetInfo.ToString().c_str());
        return photoAssetInfo.displayName;
    }
    DisplayNameInfo displayNameInfo(photoAssetInfo);
    std::string displayName = displayNameInfo.ToString();
    int32_t retryCount = 0;
    const int32_t MAX_RETRY_COUNT = 100;
    while (IsDisplayNameExists(rdbStore, photoAssetInfo.ownerAlbumId, displayName)) {
        displayName = displayNameInfo.Next();
        if (retryCount++ > MAX_RETRY_COUNT) {
            MEDIA_ERR_LOG("can not find unique display after retry %{public}d", MAX_RETRY_COUNT);
            break;
        }
    }
    if (photoAssetInfo.displayName != displayName) {
        MEDIA_INFO_LOG("displayName changed from %{public}s to %{public}s",
            photoAssetInfo.ToString().c_str(),
            displayName.c_str());
    }
    return displayName;
}

bool PhotoDisplayNameOperation::IsDisplayNameExists(
    const std::shared_ptr<MediaLibraryRdbStore> rdbStore, const int32_t ownerAlbumId, const std::string &displayName)
{
    if (ownerAlbumId <= 0 || displayName.empty() || rdbStore == nullptr) {
        return false;
    }
    std::string querySql = this->SQL_PHOTOS_TABLE_QUERY_DISPLAY_NAME;
    const std::vector<NativeRdb::ValueObject> bindArgs = {displayName};
    auto resultSet = rdbStore->QuerySql(querySql, bindArgs);
    if (resultSet == nullptr) {
        return false;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t albumIdInDb = GetInt32Val(PhotoColumn::PHOTO_OWNER_ALBUM_ID, resultSet);
        if (albumIdInDb == ownerAlbumId) {
            MEDIA_DEBUG_LOG("the same displayName: %{public}s, ownerAlbumId: %{public}d",
                displayName.c_str(), ownerAlbumId);
            resultSet->Close();
            return true;
        }
    }
    resultSet->Close();
    return false;
}
}  // namespace OHOS::Media