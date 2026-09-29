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
#define MLOG_TAG "PhotoAssetCopyOperation"

#include "photo_asset_copy_operation.h"

#include "photo_displayname_operation.h"
#include "photo_burst_operation.h"
#include "media_log.h"
#include "media_column.h"
#include "result_set_utils.h"
#include "media_file_utils.h"
#include "photo_asset_info.h"
#include "scanner_utils.h"

namespace OHOS::Media {
PhotoAssetCopyOperation &PhotoAssetCopyOperation::SetTargetPhotoInfo(
    const std::shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Media_Operation: resultSet is null.");
        return *this;
    }
    this->photoAssetInfo_.displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
    this->photoAssetInfo_.subtype = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
    return *this;
}

/**
 * @brief Set the target album id.
 *
 * @param targetAlbumId The target album id. Only positive integers are accepted.
 */
PhotoAssetCopyOperation &PhotoAssetCopyOperation::SetTargetAlbumId(const int32_t targetAlbumId)
{
    if (targetAlbumId <= 0) {
        return *this;
    }
    this->photoAssetInfo_.ownerAlbumId = targetAlbumId;
    return *this;
}

/**
 * @brief Set the display name.
 *
 * @param displayName The display name. Only non-empty strings are accepted.
 */
PhotoAssetCopyOperation &PhotoAssetCopyOperation::SetDisplayName(const std::string &displayName)
{
    if (displayName.empty()) {
        return *this;
    }
    this->photoAssetInfo_.displayName = displayName;
    return *this;
}

/**
 * @brief Set the PhotoAssetInfo of the target photo to ValuesBucket
 *
 * @param rdbStore The rdb store.
 * @param values The values bucket.
 */
void PhotoAssetCopyOperation::CopyPhotoAsset(
    const std::shared_ptr<MediaLibraryRdbStore> &rdbStore, NativeRdb::ValuesBucket &values)
{
    PhotoAssetInfo photoAssetInfo(this->photoAssetInfo_);
    // Find the unique display name for the photo.
    std::string displayName = PhotoDisplayNameOperation().SetTargetPhotoInfo(photoAssetInfo).FindDisplayName(rdbStore);
    if (!displayName.empty()) {
        values.PutString(MediaColumn::MEDIA_NAME, displayName);
        values.PutString(MediaColumn::MEDIA_TITLE, MediaFileUtils::GetTitleFromDisplayName(displayName));
        values.PutString(PhotoColumn::PHOTO_MEDIA_SUFFIX, ScannerUtils::GetFileExtension(displayName));
    } else {
        MEDIA_ERR_LOG("Failed to get display name. Object: %{public}s", photoAssetInfo.ToString().c_str());
        return;
    }
    // Find the burst key by the unique display name for the burst photos.
    photoAssetInfo.displayName = displayName;
    this->photoAssetInfo_.newDisplayName = displayName;
    std::string burstKey = PhotoBurstOperation().SetTargetPhotoInfo(photoAssetInfo).FindBurstKey(rdbStore);
    if (!burstKey.empty()) {
        values.PutString(PhotoColumn::PHOTO_BURST_KEY, burstKey);
    }
}

/**
 * @brief Check whether the display name has been renamed.
 *
 * @param isRename bool value that will be set to true if the display name has been renamed, false otherwise.
 */
bool PhotoAssetCopyOperation::CheckDisplayNameRenamed()
{
    return (this->photoAssetInfo_.displayName != this->photoAssetInfo_.newDisplayName);
}
}  // namespace OHOS::Media