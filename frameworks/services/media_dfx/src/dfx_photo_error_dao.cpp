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

#define MLOG_TAG "DfxPhotoError"

#include "dfx_photo_error_dao.h"

#include "media_column.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "result_set_utils.h"
#include "rdb_predicates.h"
#include "values_bucket.h"

namespace OHOS {
namespace Media {

std::string PhotoErrorRow::ToString() const
{
    return "fileId=" + std::to_string(fileId) + " fst=" + std::to_string(fileSourceType) +
        " mt=" + std::to_string(mediaType) + " sd=" + std::to_string(southDeviceType) +
        " pos=" + std::to_string(position) + " size=" + std::to_string(size);
}

std::vector<PhotoErrorRow> DfxPhotoErrorDao::QueryBatch(int32_t lastFileId, int32_t limit)
{
    std::vector<PhotoErrorRow> rows;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, rows, "rdbStore is nullptr");

    const std::string sql = "SELECT file_id, data, storage_path, file_source_type, media_type, "
        "south_device_type, position, size, subtype, moving_photo_effect_mode, original_subtype "
        "FROM Photos WHERE file_id > ? AND sync_status = 0 AND clean_flag = 0 AND "
        "time_pending = 0 AND is_temp = 0 AND position IN (1, 3) ORDER BY file_id LIMIT ?";
    std::vector<NativeRdb::ValueObject> bindArgs = {lastFileId, limit};
    auto resultSet = rdbStore->QuerySql(sql, bindArgs);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, rows, "QuerySql failed");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        PhotoErrorRow row;
        row.fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        row.data = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
        row.storagePath = GetStringVal(PhotoColumn::PHOTO_STORAGE_PATH, resultSet);
        row.fileSourceType = GetInt32Val(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, resultSet);
        row.mediaType = GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet);
        row.southDeviceType = GetInt32Val(PhotoColumn::PHOTO_SOUTH_DEVICE_TYPE, resultSet);
        row.position = GetInt32Val(PhotoColumn::PHOTO_POSITION, resultSet);
        row.size = GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet);
        row.subtype = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
        row.movingPhotoEffectMode = GetInt32Val(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, resultSet);
        row.originalSubtype = GetInt32Val(PhotoColumn::PHOTO_ORIGINAL_SUBTYPE, resultSet);
        rows.push_back(std::move(row));
    }
    resultSet->Close();
    return rows;
}
} // namespace Media
} // namespace OHOS
