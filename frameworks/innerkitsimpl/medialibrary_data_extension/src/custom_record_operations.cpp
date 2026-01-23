/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#define MLOG_TAG "CustomRecordOperations"
#include "custom_record_operations.h"
#include "medialibrary_errno.h"
#include "media_log.h"
#include "custom_records_column.h"

namespace OHOS::Media {

int32_t CustomRecordOperations::InsertCustomRecord(std::shared_ptr<MediaLibraryRdbStore> &rdbStore,
    MediaLibraryCommand &cmd)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "custom record insert rdbStore is nullptr");
    int64_t outRowId = -1;
    int32_t errCode = rdbStore->Insert(cmd, outRowId);
    if (errCode != NativeRdb::E_OK || outRowId < 0) {
        MEDIA_ERR_LOG("custom record Insert into db failed, errCode = %{public}d", errCode);
        return E_HAS_DB_ERROR;
    }
    return static_cast<int32_t>(outRowId);
}

int32_t CustomRecordOperations::BatchAddCustomRecords(MediaLibraryCommand &cmd,
    const std::vector<DataShare::DataShareValuesBucket> &values)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "custom record insert rdbStore is nullptr");
    int32_t errCode = -1;
    for (auto &value : values) {
        NativeRdb::ValuesBucket rdbValue = RdbDataShareAdapter::RdbUtils::ToValuesBucket(value);
        cmd.SetValueBucket(rdbValue);
        errCode = InsertCustomRecord(rdbStore, cmd);
        CHECK_AND_RETURN_RET_LOG(errCode >= 0, E_HAS_DB_ERROR, "custom record insert fail");
    }
    return errCode;
}
}