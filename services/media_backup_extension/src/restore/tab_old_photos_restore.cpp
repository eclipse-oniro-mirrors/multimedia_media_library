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
#define MLOG_TAG "TabOldPhotosRestore"

#include "tab_old_photos_restore.h"

#include <numeric>

#include "backup_database_utils.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_column.h"

namespace OHOS::Media {
int32_t TabOldPhotosRestore::Restore(
    std::shared_ptr<NativeRdb::RdbStore> rdbStorePtr, const std::vector<FileInfo> &fileInfos)
{
    CHECK_AND_RETURN_RET_LOG(rdbStorePtr != nullptr, NativeRdb::E_DB_NOT_EXIST,
        "rdbStorePtr is nullptr, Maybe init failed");

    TabOldPhotosRestoreHelper restoreHelper;
    
    int64_t startInsert = MediaFileUtils::UTCTimeMilliSeconds();
    int32_t ret = restoreHelper.BatchInsertWithRetry(rdbStorePtr, fileInfos);
    CHECK_AND_EXECUTE(ret == NativeRdb::E_OK, MEDIA_ERR_LOG("Restore failed, ret=%{public}d", ret));

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("End Restore insert cost %{public}" PRId64, end - startInsert);
    return ret;
}

std::vector<NativeRdb::ValuesBucket> TabOldPhotosRestoreHelper::GetInsertValues(const std::vector<FileInfo> &fileInfos)
{
    std::vector<NativeRdb::ValuesBucket> values;

    for (auto &fileInfo : fileInfos) {
        NativeRdb::ValuesBucket value;

        value.PutInt(PhotoColumn::TAB_OLD_PHOTOS_COLUMN_FILE_ID, fileInfo.fileIdNew);
        value.PutInt(PhotoColumn::TAB_OLD_PHOTOS_COLUMN_OLD_FILE_ID, fileInfo.localMediaId);
        value.PutString(PhotoColumn::TAB_OLD_PHOTOS_COLUMN_OLD_DATA, fileInfo.oldPath);
        value.PutString(PhotoColumn::TAB_OLD_PHOTOS_COLUMN_DATA, fileInfo.cloudPath);

        values.emplace_back(value);
    }

    return values;
}

int32_t TabOldPhotosRestoreHelper::BatchInsertWithRetry(std::shared_ptr<NativeRdb::RdbStore> rdbStorePtr,
    const std::vector<FileInfo> &fileInfos)
{
    int32_t errCode = E_ERR;
    TransactionOperations trans{ __func__ };
    trans.SetBackupRdbStore(rdbStorePtr);

    int64_t rowNum = 0;
    auto values = GetInsertValues(fileInfos);
    if (values.empty()) {
        MEDIA_ERR_LOG("TabOldPhotosRestoreHelper::BatchInsertWithRetry values are empty!");
        return E_ERR;
    }
    std::function<int(void)> func = [&]()->int {
        errCode = trans.BatchInsert(rowNum, PhotoColumn::TAB_OLD_PHOTOS_TABLE, values);
        CHECK_AND_PRINT_LOG(errCode == E_OK,
            "InsertSql failed, errCode: %{public}d, rowNum: %{public}ld.", errCode, (long)rowNum);
        return errCode;
    };
    errCode = trans.RetryTrans(func, true);
    CHECK_AND_PRINT_LOG(errCode == E_OK, "BatchInsertWithRetry: trans finish fail!, ret:%{public}d", errCode);
    return errCode;
}
} // namespace OHOS::Media