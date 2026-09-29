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

#define MLOG_TAG "Media_Notify"

#include "asset_batch_notify.h"

#include "analysis_album_accurate_refresh.h"
#include "asset_accurate_refresh.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_notify.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_unistore_manager.h"
#include "userfile_manager_types.h"

namespace OHOS::Media {

int32_t AssetBatchNotify::TryNotifyAssetsChange(const std::vector<std::string> &fileIds)
{
    this->fileIds_.insert(this->fileIds_.end(), fileIds.begin(), fileIds.end());
    this->totalFileCount_ += static_cast<int32_t>(fileIds.size());
    if (static_cast<int32_t>(this->fileIds_.size()) >= BATCH_NOTIFY_CLOUD_FILE) {
        this->NotifyAssetsChange(this->fileIds_);
        this->fileIds_.clear();
    }
    return E_OK;
}

int32_t AssetBatchNotify::FinalNotifyAssetsChange()
{
    CHECK_AND_RETURN_RET(!this->fileIds_.empty(), E_OK);

    this->NotifyAssetsChange(this->fileIds_);
    this->fileIds_.clear();

    this->TryUpdateAllAlbums();
    return E_OK;
}

int32_t AssetBatchNotify::TryUpdateAllAlbums()
{
    CHECK_AND_RETURN_RET(this->totalFileCount_ > 0, E_OK);

    MEDIA_INFO_LOG("TryUpdateAllAlbums. totalFileCount_: %{public}d", this->totalFileCount_);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_OK, "UpdateAllAlbums failed. rdbStore is null.");
    MediaLibraryRdbUtils::UpdateAllAlbums(rdbStore);

    return E_OK;
}

void AssetBatchNotify::NotifyAssetsChange(const std::vector<std::string> &notifyFileIds)
{
    AccurateRefresh::AssetAccurateRefresh::NotifyForReCheck();
    AccurateRefresh::AnalysisAlbumAccurateRefresh::NotifyForAnalysisAssetReCheck();
    CHECK_AND_RETURN_LOG(!notifyFileIds.empty(), "notifyFileIds is null.");
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_LOG(watch != nullptr, "watch is null.");
    for (size_t i = 0; i < notifyFileIds.size(); i++) {
        watch->Notify(MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX, notifyFileIds[i]),
            NotifyType::NOTIFY_REMOVE);
    }
}
} // namespace OHOS::Media
