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

#define MLOG_TAG "Media_Cloud_Dao"

#include "cloud_media_file_manager_dao.h"

#include "abs_rdb_predicates.h"
#include "media_log.h"
#include "media_path_utils.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "values_bucket.h"
#include "media_file_utils.h"
#include "media_column.h"

namespace OHOS::Media::CloudSync {
// LCOV_EXCL_START
int32_t CloudMediaFileManagerDao::UpdateStoragePath(const int32_t fileId, const std::string &storagePath)
{
    const bool isValid = fileId > 0;
    CHECK_AND_RETURN_RET_LOG(isValid,
                             NativeRdb::E_INVALID_ARGS,
                             "Invalid arguments, fileId: %{public}d, storagePath: %{public}s",
                             fileId,
                             MediaPathUtils::DesensitizePath(storagePath).c_str());

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Failed to get rdbStore.");

    const std::string displayName = MediaFileUtils::GetFileName(storagePath);
    const std::string title = MediaFileUtils::GetTitleFromDisplayName(displayName);

    NativeRdb::ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_TITLE, title);
    values.PutString(MediaColumn::MEDIA_NAME, displayName);
    values.PutString(PhotoColumn::PHOTO_STORAGE_PATH, storagePath);
    values.PutLong(PhotoColumn::PHOTO_META_DATE_MODIFIED, MediaFileUtils::UTCTimeMilliSeconds());

    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);

    int32_t changedRows = 0;
    int32_t ret = rdbStore->Update(changedRows, values, predicates);
    MEDIA_INFO_LOG("UpdateStoragePath completed, "
                   "ret: %{public}d, changedRows: %{public}d, fileId: %{public}d, newStoragePath: %{public}s",
                   ret,
                   changedRows,
                   fileId,
                   MediaPathUtils::DesensitizePath(storagePath).c_str());
    return ret;
}
// LCOV_EXCL_STOP
}  // namespace OHOS::Media::CloudSync