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
#define MLOG_TAG "Media_Dao"

#include "media_custom_restore_dao.h"

#include "media_column.h"
#include "medialibrary_unistore_manager.h"
#include "rdb_predicates.h"

namespace OHOS::Media::Restore {
// LCOV_EXCL_START
int32_t MediaCustomRestoreDao::UpdatePhotos(const std::string &filePath, const int32_t livePhoto4dStatus)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "rdbStore is null");

    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_FILE_PATH, filePath);

    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_STATUS, livePhoto4dStatus);

    int32_t changedRows = 0;
    int32_t ret = rdbStore->Update(changedRows, values, predicates);
    MEDIA_INFO_LOG("rdbStore completed, ret: %{public}d, changedRows: %{public}d", ret, changedRows);
    return ret;
}
// LCOV_EXCL_STOP
}  // namespace OHOS::Media::Restore
