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

#define MLOG_TAG "LakeSizeRepairDao"

#include "media_lake_size_repair_dao.h"

#include "media_column.h"
#include "media_log.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"
#include "result_set_utils.h"
#include "userfile_manager_types.h"

using namespace OHOS::NativeRdb;

namespace OHOS::Media::Background {
namespace {
constexpr int32_t MEDIA_HO_LAKE_SOURCE_TYPE = static_cast<int32_t>(FileSourceType::MEDIA_HO_LAKE);
constexpr int32_t POSITION_LOCAL = static_cast<int32_t>(PhotoPositionType::LOCAL);
constexpr int32_t POSITION_LOCAL_AND_CLOUD = static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
}  // namespace

std::vector<LakeSizeRepairRecord> MediaLakeSizeRepairDao::QueryCorruptedLakePhotos(int32_t lastFileId, int32_t limit)
{
    const std::string sql =
        "SELECT file_id, storage_path FROM Photos WHERE file_id > ? AND file_source_type = ? "
        "AND position IN (?, ?) AND sync_status = 0 AND clean_flag = 0 AND time_pending = 0 AND is_temp = 0 "
        "AND size IN (0, -1) ORDER BY file_id LIMIT ?";
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, {}, "rdbStore is nullptr");
    auto resultSet = rdbStore->QuerySql(sql,
        {lastFileId, MEDIA_HO_LAKE_SOURCE_TYPE, POSITION_LOCAL, POSITION_LOCAL_AND_CLOUD, limit});
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, {}, "QuerySql failed");
    std::vector<LakeSizeRepairRecord> records;
    while (resultSet->GoToNextRow() == E_OK) {
        LakeSizeRepairRecord record;
        record.fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        record.storagePath = GetStringVal(PhotoColumn::PHOTO_STORAGE_PATH, resultSet);
        records.emplace_back(record);
    }
    resultSet->Close();
    return records;
}
}  // namespace OHOS::Media::Background
