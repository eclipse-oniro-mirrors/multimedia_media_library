/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIALIBRARY_RDB_TRANSACTION_H
#define OHOS_MEDIALIBRARY_RDB_TRANSACTION_H

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>

#include "dfx_transaction.h"
#include "medialibrary_async_worker.h"
#include "medialibrary_unistore.h"
#include "timer.h"
#include "value_object.h"
#include "transaction.h"

namespace OHOS::Media {
constexpr int32_t MAX_TRY_TIMES = 30;
constexpr int32_t MAX_BUSY_TRY_TIMES = 2;
constexpr int32_t TRANSACTION_WAIT_INTERVAL = 50; // in milliseconds.

#define EXPORT __attribute__ ((visibility ("default")))
/**
 * This class is used for database transaction creation, commit, and rollback
 * The usage of class is as follows:
 *   1. initialize TransactionOperations object with a rdb instance
 *          (for example: TranscationOperations opt(rdb))
 *   2. After init opt, you need call Start() function to start transaction
 *          int32_t err = opt.Start();
 *          if err != E_OK, transaction init failed
 *   3. If you need to commit transaction, then use
 *          int32_t err = opt.Finish();
 *          if err != E_OK, transaction commit failed and auto rollback
 *   4. If TransactionOperations is destructed without successfully finish, it will be auto rollback
 */
class TransactionOperations {
public:
    EXPORT TransactionOperations(std::string funcName);
    EXPORT ~TransactionOperations();
    EXPORT int32_t Start(bool isBackup = false);
    EXPORT int32_t Commit();
    EXPORT int32_t Finish();
    EXPORT int32_t TryTrans(std::function<int(void)> &func, bool isBackup);
    EXPORT int32_t RetryTrans(std::function<int(void)> &func, bool isBackup = false);
    EXPORT int32_t Rollback();
    EXPORT void SetBackupRdbStore(std::shared_ptr<OHOS::NativeRdb::RdbStore> rdbStore);

    EXPORT int32_t ExecuteSql(const std::string &sql, const std::vector<NativeRdb::ValueObject> &args = {});
    EXPORT int32_t Execute(const std::string &sql, const std::vector<NativeRdb::ValueObject> &args = {});
    EXPORT int32_t ExecuteForLastInsertedRowId(const std::string &sql,
        const std::vector<NativeRdb::ValueObject> &bindArgs);
    EXPORT int32_t Insert(MediaLibraryCommand &cmd, int64_t &rowId);
    EXPORT int32_t Insert(int64_t &rowId, const std::string tableName, const NativeRdb::ValuesBucket &values);
    EXPORT int32_t BatchInsert(int64_t &outRowId, const std::string &table,
        const std::vector<NativeRdb::ValuesBucket> &values);
    EXPORT int32_t BatchInsert(int64_t &changeRows, const std::string &table,
        const std::vector<NativeRdb::ValuesBucket> &values, NativeRdb::ConflictResolution resolution);
    EXPORT int32_t BatchInsert(MediaLibraryCommand &cmd, int64_t &outInsertNum,
        const std::vector<NativeRdb::ValuesBucket> &values);
    EXPORT int32_t Update(NativeRdb::ValuesBucket &values, const NativeRdb::AbsRdbPredicates &predicates);
    EXPORT int32_t Update(int32_t &changedRows, NativeRdb::ValuesBucket &values,
        const NativeRdb::AbsRdbPredicates &predicates);
    EXPORT int32_t Update(MediaLibraryCommand &cmd, int32_t &changedRows);
    EXPORT int32_t Delete(MediaLibraryCommand &cmd, int32_t &deletedRows);

    EXPORT std::pair<int32_t, NativeRdb::Results> BatchInsert(const std::string &table,
        const std::vector<NativeRdb::ValuesBucket> &values, const std::string &returningField,
        NativeRdb::ConflictResolution resolution = NativeRdb::ConflictResolution::ON_CONFLICT_NONE);
    EXPORT std::pair<int32_t, NativeRdb::Results> Update(const NativeRdb::ValuesBucket &values,
        const NativeRdb::AbsRdbPredicates &predicates, const std::string &returningField);
    EXPORT std::pair<int32_t, NativeRdb::Results> Delete(const NativeRdb::AbsRdbPredicates &predicates,
        const std::string &returningField);
    EXPORT std::pair<int32_t, NativeRdb::Results> Execute(const std::string &sql,
        const std::vector<NativeRdb::ValueObject> &args, const std::string &returningField);
    EXPORT std::shared_ptr<NativeRdb::ResultSet> QueryByStep(const NativeRdb::AbsRdbPredicates &predicates,
        const std::vector<std::string> &columns);
    EXPORT std::shared_ptr<NativeRdb::ResultSet> QueryByStep(const std::string &sql,
        const std::vector<NativeRdb::ValueObject> &args = {});
    EXPORT bool GetIsOperate();

private:
    void SetIsOperate(const std::pair<int32_t, NativeRdb::Results> &result);

private:
    std::shared_ptr<OHOS::NativeRdb::Transaction> transaction_ = nullptr;
    std::shared_ptr<OHOS::NativeRdb::RdbStore> rdbStore_;
    std::shared_ptr<OHOS::NativeRdb::RdbStore> backupRdbStore_;
    std::string funcName_ = "";
    bool isSkipCloudSync_ = false;
    DfxTransaction reporter_;
    // 接入精准刷新框架时，查询修改前的数据不用事务，防止事务升级
    bool isOperate_ = false;
};
} // namespace OHOS::Media

#endif // OHOS_MEDIALIBRARY_RDB_TRANSACTION_H
