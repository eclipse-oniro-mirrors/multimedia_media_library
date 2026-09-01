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

#define MLOG_TAG "Media_Background"

#include "media_fix_duplicate_unique_id_task.h"

#include "abs_rdb_predicates.h"
#include "media_column.h"
#include "rdb_predicates.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_subscriber.h"
#include "medialibrary_unistore_manager.h"
#include "result_set_utils.h"

using namespace OHOS::NativeRdb;

namespace OHOS::Media::Background {

bool MediaFixDuplicateUniqueIdTask::Accept()
{
    return MedialibrarySubscriber::IsCurrentStatusOn();
}

void MediaFixDuplicateUniqueIdTask::Execute()
{
    HandleFixDuplicateUniqueId();
}

void MediaFixDuplicateUniqueIdTask::HandleFixDuplicateUniqueId()
{
    MEDIA_INFO_LOG("FixDuplicateUniqueId start");
    std::vector<std::string> duplicateIds = FindDuplicateUniqueIds();
    MEDIA_INFO_LOG("FixDuplicateUniqueId found %{public}zu duplicate uniqueIds", duplicateIds.size());

    for (const auto &uniqueId : duplicateIds) {
        if (!Accept()) {
            MEDIA_INFO_LOG("FixDuplicateUniqueId Accept check failed, return");
            return;
        }

        int32_t ret = ProcessDuplicateGroup(uniqueId);
        if (ret != E_OK) {
            MEDIA_WARN_LOG("FixDuplicateUniqueId ProcessDuplicateGroup failed, uniqueId: %{public}s, ret: %{public}d",
                uniqueId.c_str(), ret);
        }
    }
    MEDIA_INFO_LOG("FixDuplicateUniqueId done");
}

std::vector<std::string> MediaFixDuplicateUniqueIdTask::FindDuplicateUniqueIds()
{
    std::vector<std::string> result;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, result, "FixDuplicateUniqueId rdbStore is nullptr");

    std::string sql = "SELECT " + PhotoColumn::UNIQUE_ID + " FROM " + PhotoColumn::PHOTOS_TABLE +
        " WHERE " + PhotoColumn::UNIQUE_ID + " IS NOT NULL AND " +
        PhotoColumn::UNIQUE_ID + " != '' AND " +
        PhotoColumn::UNIQUE_ID + " != '-1'" +
        " GROUP BY " + PhotoColumn::UNIQUE_ID +
        " HAVING COUNT(*) > 1";

    auto resultSet = rdbStore->QuerySql(sql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, result, "FixDuplicateUniqueId QuerySql is null");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string uniqueId = GetStringVal(PhotoColumn::UNIQUE_ID, resultSet);
        if (!uniqueId.empty()) {
            result.push_back(uniqueId);
        }
    }
    resultSet->Close();
    return result;
}

int32_t MediaFixDuplicateUniqueIdTask::ProcessDuplicateGroup(const std::string &uniqueId)
{
    std::vector<DuplicateRecordInfo> records = QueryDuplicateRecords(uniqueId);
    if (records.size() <= 1) {
        MEDIA_INFO_LOG("FixDuplicateUniqueId group no longer duplicate, uniqueId: %{public}s", uniqueId.c_str());
        return E_OK;
    }

    MEDIA_INFO_LOG("FixDuplicateUniqueId processing group, uniqueId: %{public}s, count: %{public}zu",
        uniqueId.c_str(), records.size());

    for (size_t i = 1; i < records.size(); i++) {
        std::string newUniqueId = MediaFileUtils::GenerateUUID();

        int32_t ret = UpdateRecordUniqueId(records[i].fileId, newUniqueId);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("FixDuplicateUniqueId UpdateRecordUniqueId failed, fileId: %{public}d, ret: %{public}d",
                records[i].fileId, ret);
            continue;
        }

        MEDIA_INFO_LOG("FixDuplicateUniqueId fixed record, fileId: %{public}d, oldUniqueId: %{public}s, "
            "newUniqueId: %{public}s", records[i].fileId, uniqueId.c_str(), newUniqueId.c_str());
    }

    return E_OK;
}

std::vector<DuplicateRecordInfo> MediaFixDuplicateUniqueIdTask::QueryDuplicateRecords(const std::string &uniqueId)
{
    std::vector<DuplicateRecordInfo> records;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, records, "FixDuplicateUniqueId rdbStore is nullptr");

    std::vector<std::string> columns = { PhotoColumn::MEDIA_ID, MediaColumn::MEDIA_DATE_MODIFIED };
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::UNIQUE_ID, uniqueId)
        ->OrderByAsc(MediaColumn::MEDIA_DATE_MODIFIED)
        ->OrderByAsc(PhotoColumn::MEDIA_ID);

    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, records, "FixDuplicateUniqueId QueryWithFilter is null");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        DuplicateRecordInfo info;
        info.fileId = GetInt32Val(PhotoColumn::MEDIA_ID, resultSet);
        info.dateModified = GetInt64Val(MediaColumn::MEDIA_DATE_MODIFIED, resultSet);
        records.push_back(info);
    }
    resultSet->Close();
    return records;
}

int32_t MediaFixDuplicateUniqueIdTask::UpdateRecordUniqueId(int32_t fileId, const std::string &newUniqueId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "FixDuplicateUniqueId rdbStore is nullptr");

    ValuesBucket values;
    values.PutString(PhotoColumn::UNIQUE_ID, newUniqueId);

    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, std::to_string(fileId));

    int32_t changedRows = 0;
    int32_t ret = rdbStore->Update(changedRows, values, predicates);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK && changedRows > 0, E_ERR,
        "FixDuplicateUniqueId Update failed, fileId: %{public}d, ret: %{public}d, changedRows: %{public}d",
        fileId, ret, changedRows);
    return E_OK;
}

}  // namespace OHOS::Media::Background
