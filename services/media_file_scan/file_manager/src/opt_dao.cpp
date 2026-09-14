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
#define MLOG_TAG "OptDao"

#include "opt_dao.h"

#include "media_fileinterwork_column.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdbstore.h"
#include "rdb_predicates.h"
#include "values_bucket.h"

using namespace OHOS::NativeRdb;
namespace OHOS::Media {
OptDao::OptDao(const std::shared_ptr<MediaLibraryRdbStore> &rdbStore) : rdbStore_(rdbStore) {}

OptDao::~OptDao() = default;

int32_t OptDao::DeleteAllOpt()
{
    CHECK_AND_RETURN_RET_LOG(rdbStore_ != nullptr, E_HAS_DB_ERROR, "rdbStore_ is null.");
    RdbPredicates predicates(MediaFileInterworkColumn::OPT_TABLE_NAME);
    int32_t deletedRows = 0;
    int32_t ret = rdbStore_->Delete(deletedRows, predicates);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_HAS_DB_ERROR,
        "DeleteAllOpt failed, ret = %{public}d", ret);
    MEDIA_INFO_LOG("Cleaned tab_file_opt table successfully");
    return E_OK;
}

std::shared_ptr<ResultSet> OptDao::QueryOptByStatus(int32_t status, int32_t limit)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore_ != nullptr, nullptr, "rdbStore_ is null.");
    RdbPredicates predicates(MediaFileInterworkColumn::OPT_TABLE_NAME);
    predicates.EqualTo(MediaFileInterworkColumn::OPT_STATUS_COLUMN, status);
    predicates.Limit(limit);
    return rdbStore_->Query(predicates, {MediaFileInterworkColumn::AFTER_PATH_COLUMN});
}

int32_t OptDao::UpdateOptStatusByPaths(const std::vector<std::string> &fileBatch, int32_t status)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore_ != nullptr, E_HAS_DB_ERROR, "rdbStore_ is null.");
    RdbPredicates predicates(MediaFileInterworkColumn::OPT_TABLE_NAME);
    predicates.In(MediaFileInterworkColumn::AFTER_PATH_COLUMN, fileBatch);
    ValuesBucket values;
    values.PutInt(MediaFileInterworkColumn::OPT_STATUS_COLUMN, status);
    int32_t changeRow = 0;
    return rdbStore_->Update(changeRow, values, predicates);
}

int32_t OptDao::QueryOptExisting(const std::string &filePath, bool &found)
{
    found = false;
    CHECK_AND_RETURN_RET_LOG(rdbStore_ != nullptr, E_ERR, "rdbStore_ is null.");
    RdbPredicates predicates(MediaFileInterworkColumn::OPT_TABLE_NAME);
    predicates.EqualTo(MediaFileInterworkColumn::AFTER_PATH_COLUMN, filePath);
    predicates.EqualTo(MediaFileInterworkColumn::OPT_STATUS_COLUMN, 0);
    auto resultSet = rdbStore_->Query(predicates, {});
    if (resultSet == nullptr) {
        return E_ERR;
    }
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        found = true;
    }
    resultSet->Close();
    return E_OK;
}

int32_t OptDao::InsertOpt(const std::string &filePath)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore_ != nullptr, E_ERR, "rdbStore_ is null.");
    ValuesBucket values;
    values.PutString(MediaFileInterworkColumn::AFTER_PATH_COLUMN, filePath);
    values.PutInt(MediaFileInterworkColumn::OPT_COLUMN, 0);
    values.PutInt(MediaFileInterworkColumn::OPT_STATUS_COLUMN, 0);
    int64_t rowId = 0;
    return rdbStore_->Insert(rowId, MediaFileInterworkColumn::OPT_TABLE_NAME, values);
}
} // namespace OHOS::Media
