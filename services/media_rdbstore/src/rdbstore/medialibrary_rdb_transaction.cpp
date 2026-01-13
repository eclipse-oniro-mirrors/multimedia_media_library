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

#include "medialibrary_rdb_transaction.h"
#include "medialibrary_restore.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "photo_album_column.h"
#include "photo_map_column.h"
#include "medialibrary_rdbstore.h"
#include "cloud_sync_helper.h"
#include "values_buckets.h"

namespace OHOS::Media {
using namespace std;
using namespace OHOS::NativeRdb;
constexpr int32_t E_HAS_DB_ERROR = -222;
constexpr int32_t E_OK = 0;
constexpr int32_t RETRY_TRANS_MAX_TIMES = 2;
constexpr int32_t RETRY_TRANS_MAX_TIMES_FOR_BACKUP = 10;

TransactionOperations::TransactionOperations(std::string funcName) : funcName_(funcName), reporter_(funcName)
{}

TransactionOperations::~TransactionOperations()
{
    if (transaction_ == nullptr) {
        return;
    }
    Rollback();
}

void TransactionOperations::SetBackupRdbStore(std::shared_ptr<OHOS::NativeRdb::RdbStore> rdbStore)
{
    backupRdbStore_ = rdbStore;
}

int32_t TransactionOperations::Start(bool isBackup)
{
    if (isBackup) {
        rdbStore_ = backupRdbStore_;
        if (rdbStore_ == nullptr) {
            rdbStore_ = MediaLibraryRdbStore::GetRaw();
        }
    } else {
        rdbStore_ = MediaLibraryRdbStore::GetRaw();
    }
    if (rdbStore_ == nullptr) {
        reporter_.ReportError(DfxTransaction::AbnormalType::NULLPTR_ERROR, E_HAS_DB_ERROR);
        MEDIA_ERR_LOG("rdbStore_ is null, isBackup = %{public}d", isBackup);
        return E_HAS_DB_ERROR;
    }

    int currentTime = 0;
    int errCode = -1;
    while (currentTime <= MAX_TRY_TIMES) {
        auto [ret, transaction] = rdbStore_->CreateTransaction(OHOS::NativeRdb::Transaction::DEFERRED);
        errCode = ret;
        if (ret == NativeRdb::E_OK) {
            transaction_ = transaction;
            break;
        } else if (ret == NativeRdb::E_SQLITE_LOCKED || ret == NativeRdb::E_DATABASE_BUSY) {
            this_thread::sleep_for(chrono::milliseconds(TRANSACTION_WAIT_INTERVAL));
            currentTime++;
            MEDIA_ERR_LOG("CreateTransaction busy, ret:%{public}d, time:%{public}d", ret, currentTime);
        } else if (ret == NativeRdb::E_SQLITE_CORRUPT) {
            MediaLibraryRestore::GetInstance().CheckRestore(ret);
            MEDIA_ERR_LOG("CreateTransaction corrupt, ret = %{public}d", ret);
            return E_HAS_DB_ERROR;
        } else {
            MEDIA_ERR_LOG("CreateTransaction faile, ret = %{public}d", ret);
            break;
        }
    }
    if (errCode != NativeRdb::E_OK) {
        reporter_.ReportError(DfxTransaction::AbnormalType::CREATE_ERROR, errCode);
        errCode = E_HAS_DB_ERROR;
    }
    return errCode;
}

int32_t TransactionOperations::Commit()
{
    if (transaction_ == nullptr) {
        reporter_.ReportError(DfxTransaction::AbnormalType::NULLPTR_ERROR, E_HAS_DB_ERROR);
        MEDIA_ERR_LOG("transaction is null");
        return E_HAS_DB_ERROR;
    }
    MEDIA_INFO_LOG("Commit transaction, funcName is :%{public}s", funcName_.c_str());
    auto ret = transaction_->Commit();
    if (ret != NativeRdb::E_OK) {
        reporter_.ReportError(DfxTransaction::AbnormalType::COMMIT_ERROR, ret);
        MEDIA_ERR_LOG("transaction commit fail!, ret:%{public}d", ret);
    } else {
        reporter_.ReportIfTimeout();
    }
    return ret;
}

int32_t TransactionOperations::Finish()
{
    if (transaction_ == nullptr) {
        reporter_.ReportError(DfxTransaction::AbnormalType::NULLPTR_ERROR, E_HAS_DB_ERROR);
        MEDIA_ERR_LOG("transaction is null");
        return E_HAS_DB_ERROR;
    }
    auto ret = transaction_->Commit();
    transaction_ = nullptr;
    if (ret != NativeRdb::E_OK) {
        reporter_.ReportError(DfxTransaction::AbnormalType::COMMIT_ERROR, ret);
        MEDIA_ERR_LOG("transaction commit fail!, ret:%{public}d", ret);
    } else {
        reporter_.ReportIfTimeout();
    }
#ifdef CLOUD_SYNC_MANAGER
    if (isSkipCloudSync_) {
        MEDIA_INFO_LOG("recover cloud sync for commit");
        CloudSyncHelper::GetInstance()->StartSync();
        isSkipCloudSync_ = false;
    }
#endif
    return ret;
}

int32_t TransactionOperations::TryTrans(std::function<int(void)> &func, bool isBackup)
{
    int32_t err = NativeRdb::E_OK;
    err = Start(isBackup);
    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to begin transaction, err: %{public}d", err);
        return err;
    }
    err = func();
    if (err != E_OK) {
        MEDIA_ERR_LOG("TryTrans: trans function fail!, ret:%{public}d", err);
        Rollback();
        return err;
    }
    err = Finish();
    if (err != E_OK) {
        MEDIA_ERR_LOG("TryTrans: trans finish fail!, ret:%{public}d", err);
    }
    return err;
}

int32_t TransactionOperations::RetryTrans(std::function<int(void)> &func, bool isBackup)
{
    int32_t currentTime = 0;
    int maxTryTimes = isBackup ? RETRY_TRANS_MAX_TIMES_FOR_BACKUP : RETRY_TRANS_MAX_TIMES;
    int32_t err = NativeRdb::E_OK;
    while (currentTime < maxTryTimes) {
        err = TryTrans(func, isBackup);
        if (err == E_OK) {
            return err;
        }
        if (err == NativeRdb::E_SQLITE_BUSY && !isSkipCloudSync_) {
            MEDIA_ERR_LOG("TryTrans busy, err:%{public}d", err);
#ifdef CLOUD_SYNC_MANAGER
            MEDIA_INFO_LOG("Stop cloud sync");
            FileManagement::CloudSync::CloudSyncManager::GetInstance().StopSync(
                "com.ohos.medialibrary.medialibrarydata");
            isSkipCloudSync_ = true;
#endif
        }
        currentTime++;
        reporter_.Restart();
    }
    MEDIA_INFO_LOG("RetryTrans result is :%{public}d", err);
    return err;
}

int32_t TransactionOperations::Rollback()
{
    if (transaction_ == nullptr) {
        MEDIA_ERR_LOG("transaction_ is null");
        return NativeRdb::E_OK;
    }
    auto ret = transaction_->Rollback();
    transaction_ = nullptr;
    if (ret != NativeRdb::E_OK) {
        reporter_.ReportError(DfxTransaction::AbnormalType::ROLLBACK_ERROR, ret);
        MEDIA_ERR_LOG("Rollback fail:%{public}d", ret);
    }
#ifdef CLOUD_SYNC_MANAGER
    if (isSkipCloudSync_) {
        MEDIA_INFO_LOG("recover cloud sync for rollback");
        CloudSyncHelper::GetInstance()->StartSync();
        isSkipCloudSync_ = false;
    }
#endif
    return ret;
}

int32_t TransactionOperations::ExecuteSql(const std::string &sql, const std::vector<NativeRdb::ValueObject> &args)
{
    if (transaction_ == nullptr) {
        MEDIA_ERR_LOG("transaction is null");
        return E_HAS_DB_ERROR;
    }
    auto [ret, value] = transaction_->Execute(sql, args);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("rdbStore_->ExecuteSql failed, ret = %{public}d", ret);
        MediaLibraryRestore::GetInstance().CheckRestore(ret);
        return E_HAS_DB_ERROR;
    }
    return ret;
}

int32_t TransactionOperations::Execute(const std::string &sql, const std::vector<NativeRdb::ValueObject> &args)
{
    CHECK_AND_RETURN_RET_LOG(transaction_ != nullptr, E_HAS_DB_ERROR, "transaction is null");
    auto [ret, value] = transaction_->Execute(sql, args);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("rdbStore_->Execute failed, ret = %{public}d", ret);
        MediaLibraryRestore::GetInstance().CheckRestore(ret);
        return E_HAS_DB_ERROR;
    }
    return ret;
}

int32_t TransactionOperations::ExecuteForLastInsertedRowId(
    const std::string &sql, const std::vector<NativeRdb::ValueObject> &bindArgs)
{
    CHECK_AND_RETURN_RET_LOG(transaction_ != nullptr, E_HAS_DB_ERROR, "transaction_ is null");
    int64_t lastInsertRowId = 0;
    auto [err, valueObject] = transaction_->Execute(sql, bindArgs);
    (void)valueObject.GetLong(lastInsertRowId);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to execute insert, err: %{public}d", err);
        MediaLibraryRestore::GetInstance().CheckRestore(err);
        return E_HAS_DB_ERROR;
    }
    return lastInsertRowId;
}

int32_t TransactionOperations::Insert(MediaLibraryCommand &cmd, int64_t &rowId)
{
    if (transaction_ == nullptr) {
        MEDIA_ERR_LOG("transaction is null");
        return E_HAS_DB_ERROR;
    }

    auto [ret, rows] = transaction_->Insert(cmd.GetTableName(), cmd.GetValueBucket());
    rowId = rows;
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("rdbStore_->Insert failed, ret = %{public}d", ret);
        MediaLibraryRestore::GetInstance().CheckRestore(ret);
        return E_HAS_DB_ERROR;
    }
    isOperate_ = true;
    MEDIA_DEBUG_LOG("rdbStore_->Insert end, rowId = %d, ret = %{public}d", (int)rowId, ret);
    return ret;
}

int32_t TransactionOperations::Update(NativeRdb::ValuesBucket &values, const NativeRdb::AbsRdbPredicates &predicates)
{
    if (transaction_ == nullptr) {
        MEDIA_ERR_LOG("transaction is null");
        return E_HAS_DB_ERROR;
    }
    if (predicates.GetTableName() == PhotoColumn::PHOTOS_TABLE) {
        values.PutLong(PhotoColumn::PHOTO_META_DATE_MODIFIED, MediaFileUtils::UTCTimeMilliSeconds());
        values.PutLong(PhotoColumn::PHOTO_LAST_VISIT_TIME, MediaFileUtils::UTCTimeMilliSeconds());
    }
    auto [err, changedRows] = transaction_->Update(values, predicates);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to execute update, err: %{public}d", err);
        MediaLibraryRestore::GetInstance().CheckRestore(err);
        return E_HAS_DB_ERROR;
    }
    return changedRows;
}

int32_t TransactionOperations::Update(
    int32_t &changedRows, NativeRdb::ValuesBucket &values, const NativeRdb::AbsRdbPredicates &predicates)
{
    if (transaction_ == nullptr) {
        MEDIA_ERR_LOG("transaction is null");
        return E_HAS_DB_ERROR;
    }
    auto [err, rows] = transaction_->Update(values, predicates);
    changedRows = rows;
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to execute update, err: %{public}d", err);
        MediaLibraryRestore::GetInstance().CheckRestore(err);
        return err;
    }
    return err;
}

int32_t TransactionOperations::Update(MediaLibraryCommand &cmd, int32_t &changedRows)
{
    CHECK_AND_RETURN_RET_LOG(transaction_ != nullptr, E_HAS_DB_ERROR, "transaction_ is null");
    if (cmd.GetTableName() == PhotoColumn::PHOTOS_TABLE) {
        cmd.GetValueBucket().PutLong(PhotoColumn::PHOTO_META_DATE_MODIFIED, MediaFileUtils::UTCTimeMilliSeconds());
        cmd.GetValueBucket().PutLong(PhotoColumn::PHOTO_LAST_VISIT_TIME, MediaFileUtils::UTCTimeMilliSeconds());
    }

    int32_t ret = E_HAS_DB_ERROR;
    auto res = transaction_->Update(cmd.GetTableName(),
        cmd.GetValueBucket(),
        cmd.GetAbsRdbPredicates()->GetWhereClause(),
        cmd.GetAbsRdbPredicates()->GetBindArgs());
    ret = res.first;
    changedRows = res.second;

    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("rdbStore_->Update failed, ret = %{public}d", ret);
        MediaLibraryRestore::GetInstance().CheckRestore(ret);
        return E_HAS_DB_ERROR;
    }
    return ret;
}

int32_t TransactionOperations::BatchInsert(
    int64_t &outRowId, const std::string &table, const std::vector<NativeRdb::ValuesBucket> &values)
{
    if (transaction_ == nullptr) {
        MEDIA_ERR_LOG("transaction_ is null");
        return E_HAS_DB_ERROR;
    }

    auto [ret, rows] = transaction_->BatchInsert(table, values);
    outRowId = rows;
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("transaction_->BatchInsert failed, ret = %{public}d", ret);
        MediaLibraryRestore::GetInstance().CheckRestore(ret);
        return E_HAS_DB_ERROR;
    }
    MEDIA_DEBUG_LOG("transaction_->BatchInsert end, rowId = %d, ret = %{public}d", (int)outRowId, ret);
    return ret;
}

int32_t TransactionOperations::BatchInsert(int64_t &changeRows, const std::string &table,
    const std::vector<NativeRdb::ValuesBucket> &values, NativeRdb::ConflictResolution resolution)
{
    if (transaction_ == nullptr) {
        MEDIA_ERR_LOG("transaction_ is null");
        return E_HAS_DB_ERROR;
    }
 
    auto [ret, rows] = transaction_->BatchInsert(table, values, resolution);
    changeRows = rows;
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("transaction_->BatchInsert failed, ret = %{public}d", ret);
        MediaLibraryRestore::GetInstance().CheckRestore(ret);
        return E_HAS_DB_ERROR;
    }
    MEDIA_DEBUG_LOG("transaction_->BatchInsert end, changeRows = %{public}" PRId64 ", ret = %{public}d",
        changeRows, ret);
    return ret;
}

int32_t TransactionOperations::BatchInsert(
    MediaLibraryCommand &cmd, int64_t &outInsertNum, const std::vector<ValuesBucket> &values)
{
    if (transaction_ == nullptr) {
        MEDIA_ERR_LOG("transaction_ is null");
        return E_HAS_DB_ERROR;
    }
    auto [ret, rows] = transaction_->BatchInsert(cmd.GetTableName(), values);
    outInsertNum = rows;
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("transaction_->BatchInsert failed, ret = %{public}d", ret);
        MediaLibraryRestore::GetInstance().CheckRestore(ret);
        return E_HAS_DB_ERROR;
    }
    MEDIA_DEBUG_LOG("rdbStore_->BatchInsert end, rowId = %d, ret = %{public}d", (int)outInsertNum, ret);
    return ret;
}

int32_t TransactionOperations::Insert(
    int64_t &rowId, const std::string tableName, const NativeRdb::ValuesBucket &values)
{
    CHECK_AND_RETURN_RET_LOG(transaction_ != nullptr, E_HAS_DB_ERROR, "transaction_ is null");
    auto [ret, rows] = transaction_->Insert(tableName, values);
    rowId = rows;
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("transaction_->Insert failed, ret = %{public}d", ret);
        MediaLibraryRestore::GetInstance().CheckRestore(ret);
        return E_HAS_DB_ERROR;
    }
    isOperate_ = true;
    MEDIA_DEBUG_LOG("transaction_->Insert end, rowId = %d, ret = %{public}d", (int)rowId, ret);
    return ret;
}

static int32_t DoDeleteFromPredicates(
    const AbsRdbPredicates &predicates, int32_t &deletedRows, std::shared_ptr<OHOS::NativeRdb::Transaction> transaction)
{
    int32_t ret = NativeRdb::E_ERROR;
    string tableName = predicates.GetTableName();
    ValuesBucket valuesBucket;
    if (tableName == MEDIALIBRARY_TABLE || tableName == PhotoColumn::PHOTOS_TABLE) {
        valuesBucket.PutInt(MEDIA_DATA_DB_DIRTY, static_cast<int32_t>(DirtyType::TYPE_DELETED));
        valuesBucket.PutInt(MEDIA_DATA_DB_SYNC_STATUS, static_cast<int32_t>(SyncStatusType::TYPE_UPLOAD));
        valuesBucket.PutLong(PhotoColumn::PHOTO_META_DATE_MODIFIED, MediaFileUtils::UTCTimeMilliSeconds());
        auto res = transaction->Update(valuesBucket, predicates);
        ret = res.first;
        deletedRows = res.second;
        MEDIA_INFO_LOG("delete photos permanently, ret: %{public}d", ret);
    } else if (tableName == PhotoAlbumColumns::TABLE) {
        valuesBucket.PutInt(PhotoAlbumColumns::ALBUM_DIRTY, static_cast<int32_t>(DirtyType::TYPE_DELETED));
        auto res = transaction->Update(valuesBucket, predicates);
        ret = res.first;
        deletedRows = res.second;
    } else if (tableName == PhotoMap::TABLE) {
        valuesBucket.PutInt(PhotoMap::DIRTY, static_cast<int32_t>(DirtyType::TYPE_DELETED));
        auto res = transaction->Update(valuesBucket, predicates);
        ret = res.first;
        deletedRows = res.second;
    } else {
        auto res = transaction->Delete(predicates);
        ret = res.first;
        deletedRows = res.second;
    }
    return ret;
}

int32_t TransactionOperations::Delete(MediaLibraryCommand &cmd, int32_t &deletedRows)
{
    CHECK_AND_RETURN_RET_LOG(transaction_ != nullptr, E_HAS_DB_ERROR, "transaction_ is null");
    /* local delete */
    int32_t ret = DoDeleteFromPredicates(*(cmd.GetAbsRdbPredicates()), deletedRows, transaction_);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("rdbStore_->Delete failed, ret = %{public}d", ret);
        MediaLibraryRestore::GetInstance().CheckRestore(ret);
        return E_HAS_DB_ERROR;
    }
    return ret;
}

pair<int32_t, NativeRdb::Results> TransactionOperations::BatchInsert(
    const string &table, const vector<ValuesBucket> &values, const string &returningField,
    NativeRdb::ConflictResolution resolution)
{
    if (transaction_ == nullptr) {
        MEDIA_ERR_LOG("transaction_ is null");
        return {E_HAS_DB_ERROR, -1};
    }
    ValuesBuckets refRows;
    for (auto &value : values) {
        refRows.Put(value);
    }

    pair<int32_t, NativeRdb::Results> result = transaction_->BatchInsert(table, refRows, { returningField },
        resolution);
    SetIsOperate(result);
    return result;
}

pair<int32_t, NativeRdb::Results> TransactionOperations::Update(
    const ValuesBucket &values, const AbsRdbPredicates &predicates, const string &returningField)
{
    if (transaction_ == nullptr) {
        MEDIA_ERR_LOG("transaction_ is null");
        return {E_HAS_DB_ERROR, -1};
    }

    pair<int32_t, NativeRdb::Results> result = transaction_->Update(values, predicates, { returningField });
    SetIsOperate(result);
    return result;
}

pair<int32_t, NativeRdb::Results> TransactionOperations::Delete(const AbsRdbPredicates &predicates,
    const string &returningField)
{
    if (transaction_ == nullptr) {
        MEDIA_ERR_LOG("transaction_ is null");
        return {E_HAS_DB_ERROR, -1};
    }

    pair<int32_t, NativeRdb::Results> result = transaction_->Delete(predicates, { returningField });
    SetIsOperate(result);
    return result;
}

pair<int32_t, NativeRdb::Results> TransactionOperations::Execute(const string &sql,
    const vector<ValueObject> &args, const string &returningField)
{
    if (transaction_ == nullptr) {
        MEDIA_ERR_LOG("transaction is null");
        return {E_HAS_DB_ERROR, -1};
    }
    string execSql = sql;
    execSql.append(" returning ").append(returningField);
    MEDIA_INFO_LOG("AccurateRefresh, sql:%{public}s", execSql.c_str());
    pair<int32_t, NativeRdb::Results> result = transaction_->ExecuteExt(execSql, args);
    SetIsOperate(result);
    return result;
}

shared_ptr<ResultSet> TransactionOperations::QueryByStep(const AbsRdbPredicates &predicates,
    const vector<string> &columns)
{
    if (transaction_ == nullptr) {
        MEDIA_ERR_LOG("transaction is null");
        return nullptr;
    }
    auto resultSet = transaction_->QueryByStep(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, nullptr, "resultSet is null.");
    return resultSet;
}

shared_ptr<ResultSet> TransactionOperations::QueryByStep(const string &sql, const vector<ValueObject> &args)
{
    if (transaction_ == nullptr) {
        MEDIA_ERR_LOG("transaction is null");
        return nullptr;
    }
    auto resultSet = transaction_->QueryByStep(sql, args);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, nullptr, "resultSet is null.");
    return resultSet;
}

bool TransactionOperations::GetIsOperate()
{
    return isOperate_;
}

void TransactionOperations::SetIsOperate(const pair<int32_t, NativeRdb::Results> &result)
{
    if (result.first == NativeRdb::E_OK) {
        isOperate_ = true;
    }
}
} // namespace OHOS::Media
