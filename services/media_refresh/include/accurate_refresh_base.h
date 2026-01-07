/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIALIBRARY_ACCURATE_REFRESH_BASE_H
#define OHOS_MEDIALIBRARY_ACCURATE_REFRESH_BASE_H

#include <mutex>
#include <vector>

#include "accurate_common_data.h"
#include "abs_rdb_predicates.h"
#include "values_bucket.h"
#include "medialibrary_command.h"
#include "medialibrary_rdb_transaction.h"
#include "dfx_refresh_manager.h"
#include "media_log.h"
#include "media_file_utils.h"

namespace OHOS {
namespace Media::AccurateRefresh {
#define EXPORT __attribute__ ((visibility ("default")))

class EXPORT AccurateRefreshBase {
public:
    AccurateRefreshBase() {}
    AccurateRefreshBase(std::shared_ptr<TransactionOperations> trans) : trans_(trans) {}
    AccurateRefreshBase(const std::string &targetBusiness, std::shared_ptr<TransactionOperations> trans) : trans_(trans)
    {
        dfxRefreshManager_ = std::make_shared<AccurateRefresh::DfxRefreshManager>(targetBusiness);
        dfxRefreshManager_->SetStartTime();
    }
    virtual ~AccurateRefreshBase();
    // init的查询语句
    virtual int32_t Init() = 0;
    virtual int32_t Init(const NativeRdb::AbsRdbPredicates &predicates) = 0;
    virtual int32_t Init(const std::string &sql, const std::vector<NativeRdb::ValueObject> bindArgs) = 0;
    virtual int32_t Init(const std::vector<int32_t> &keys) = 0; // 删除/更新指定fileIds场景使用

    // database execution
    int32_t Insert(MediaLibraryCommand &cmd, int64_t &outRowId);
    int32_t Insert(int64_t &outRowId, const std::string &table, NativeRdb::ValuesBucket &value);

    int32_t BatchInsert(MediaLibraryCommand &cmd, int64_t& changedRows,
        std::vector<NativeRdb::ValuesBucket>& values);
    int32_t BatchInsert(int64_t &changedRows, const std::string &table,
        std::vector<NativeRdb::ValuesBucket> &values, int &rdbError,
        NativeRdb::ConflictResolution resolution = NativeRdb::ConflictResolution::ON_CONFLICT_NONE);

    int32_t Update(MediaLibraryCommand &cmd, int32_t &changedRows);
    int32_t Update(int32_t &changedRows, const std::string &table, const NativeRdb::ValuesBucket &value,
        const std::string &whereClause, const std::vector<std::string> &args);
    int32_t Update(int32_t &changedRows, const NativeRdb::ValuesBucket &value,
        const NativeRdb::AbsRdbPredicates &predicates, RdbOperation operation = RDB_OPERATION_UPDATE);
    int32_t UpdateWithNoDateTime(int32_t &changedRows, const NativeRdb::ValuesBucket &value,
        const NativeRdb::AbsRdbPredicates &predicates, RdbOperation operation = RDB_OPERATION_UPDATE);
    int32_t UpdateWithDateTime(NativeRdb::ValuesBucket &values, const NativeRdb::AbsRdbPredicates &predicates);

    virtual int32_t LogicalDeleteReplaceByUpdate(MediaLibraryCommand &cmd, int32_t &deletedRows);
    virtual int32_t LogicalDeleteReplaceByUpdate(const NativeRdb::AbsRdbPredicates &predicates, int32_t &deletedRows);
    int32_t Delete(int32_t &deletedRows, const std::string &table, const std::string &whereClause,
        const std::vector<std::string> &args);
    int32_t Delete(int32_t &deletedRows, const NativeRdb::AbsRdbPredicates &predicates);

    int32_t ExecuteSql(const std::string &sql, RdbOperation operation);

    // 返回值为rowId
    int32_t ExecuteForLastInsertedRowId(const std::string &sql,
        const std::vector<NativeRdb::ValueObject> &bindArgs, RdbOperation operation);
    int32_t ExecuteSql(const std::string &sql,
        const std::vector<NativeRdb::ValueObject> &bindArgs, RdbOperation operation);
    int32_t ExecuteForChangedRowCount(int64_t &outValue, const std::string &sql,
        const std::vector<NativeRdb::ValueObject> &bindArgs, RdbOperation operation);
    std::shared_ptr<DfxRefreshManager> GetDfxRefreshManager();
    void CloseDfxReport();

protected:
    virtual int32_t AddAlbumIdForMoveOperation(const NativeRdb::AbsRdbPredicates &predicates) = 0;
    // 数据库操作后，触发更新修改后的数据
    virtual int32_t UpdateModifiedDatasInner(const std::vector<int32_t> &keys, RdbOperation operation,
        PendingInfo pendingInfo = PendingInfo()) = 0;
    virtual std::string GetReturningKeyName() = 0;
    virtual bool IsValidTable(std::string tableName) = 0;
    std::shared_ptr<DfxRefreshManager> dfxRefreshManager_;
protected:
    std::shared_ptr<TransactionOperations> trans_;
private:
    std::vector<int32_t> GetReturningKeys(const std::pair<int32_t, NativeRdb::Results> &retWithResults);
    bool isReport_ = true;
};
} // namespace Media
} // namespace OHOS

#endif