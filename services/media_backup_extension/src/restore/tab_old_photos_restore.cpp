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

namespace OHOS::Media {
int32_t TabOldPhotosRestore::Restore(
    std::shared_ptr<NativeRdb::RdbStore> rdbStorePtr, const std::vector<FileInfo> &fileInfos)
{
    CHECK_AND_RETURN_RET_LOG(rdbStorePtr != nullptr, NativeRdb::E_DB_NOT_EXIST,
        "rdbStorePtr is nullptr, Maybe init failed");

    int64_t startSet = MediaFileUtils::UTCTimeMilliSeconds();
    TabOldPhotosRestoreHelper restoreHelper;
    restoreHelper.SetPlaceHoldersAndBindArgs(fileInfos);
    CHECK_AND_RETURN_RET(!restoreHelper.IsEmpty(), NativeRdb::E_OK);

    int64_t startInsert = MediaFileUtils::UTCTimeMilliSeconds();
    int32_t ret = restoreHelper.InsertIntoTable(rdbStorePtr);
    CHECK_AND_EXECUTE(ret == NativeRdb::E_OK,
        MEDIA_ERR_LOG("Restore failed, ret=%{public}d, executeSql=%{public}s, bindArgs: %{public}s",
            ret, restoreHelper.GetInsertSql().c_str(), ToString(restoreHelper.GetBindArgs()).c_str()));

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("Set %{public}zu cost %{public}" PRId64 ", insert cost %{public}" PRId64,
        restoreHelper.GetInsertSize(), startInsert - startSet, end - startInsert);
    return ret;
}

std::string TabOldPhotosRestore::ToString(const std::vector<NativeRdb::ValueObject> &values)
{
    std::vector<std::string> result;
    for (auto &value : values) {
        std::string str;
        value.GetString(str);
        result.emplace_back(str + ", ");
    }
    return std::accumulate(result.begin(), result.end(), std::string());
}

void TabOldPhotosRestoreHelper::SetPlaceHoldersAndBindArgs(const std::vector<FileInfo> &fileInfos)
{
    for (const auto &fileInfo : fileInfos) {
        if (!fileInfo.needMove) {
            continue;
        }
        AddPlaceHolders();
        AddBindArgs(fileInfo);
    }
}

bool TabOldPhotosRestoreHelper::IsEmpty()
{
    return placeHolders_.empty() || bindArgs_.empty();
}

int32_t TabOldPhotosRestoreHelper::InsertIntoTable(std::shared_ptr<NativeRdb::RdbStore> rdbStorePtr)
{
    CHECK_AND_RETURN_RET_LOG(rdbStorePtr != nullptr, NativeRdb::E_DB_NOT_EXIST, "rdbStorePtr is nullptr");

    std::string insertSql = GetInsertSql();
    std::vector<NativeRdb::ValueObject> bindArgs = GetBindArgs();
    return rdbStorePtr->ExecuteSql(insertSql, bindArgs);
}

std::string TabOldPhotosRestoreHelper::GetInsertSql()
{
    return GetInputTableClause() + SQL_TAB_OLD_PHOTOS_INSERT;
}

std::vector<NativeRdb::ValueObject> TabOldPhotosRestoreHelper::GetBindArgs()
{
    return bindArgs_;
}

size_t TabOldPhotosRestoreHelper::GetInsertSize()
{
    return placeHolders_.size();
}

void TabOldPhotosRestoreHelper::AddPlaceHolders()
{
    placeHolders_.emplace_back(SQL_PLACEHOLDERS);
}

void TabOldPhotosRestoreHelper::AddBindArgs(const FileInfo &fileInfo)
{
    bindArgs_.emplace_back(fileInfo.localMediaId);
    bindArgs_.emplace_back(fileInfo.oldPath);
    bindArgs_.emplace_back(fileInfo.cloudPath);
}

std::string TabOldPhotosRestoreHelper::GetInputTableClause()
{
    return "WITH INPUT (old_file_id, old_data, data) AS (VALUES " +
        BackupDatabaseUtils::JoinValues(placeHolders_, ",") + " ) ";
}
} // namespace OHOS::Media