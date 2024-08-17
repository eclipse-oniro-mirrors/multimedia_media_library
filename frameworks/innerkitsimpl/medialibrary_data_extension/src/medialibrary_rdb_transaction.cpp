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

#include "cloud_sync_helper.h"
#include "medialibrary_rdb_transaction.h"
#include "medialibrary_restore.h"
#include "media_log.h"

namespace OHOS::Media {
using namespace std;

constexpr int32_t E_HAS_DB_ERROR = -222;
constexpr int32_t E_OK = 0;

constexpr int RDB_TRANSACTION_WAIT_MS = 1000;
std::mutex TransactionOperations::transactionMutex_;
std::condition_variable TransactionOperations::transactionCV_;
std::atomic<bool> TransactionOperations::isInTransaction_(false);
constexpr int32_t MAX_TRY_TIMES = 30;
constexpr int32_t MAX_TRY_TIMES_FOR_UPGRADE = 500;
constexpr int32_t TRANSACTION_WAIT_INTERVAL = 50; // in milliseconds.

TransactionOperations::TransactionOperations(
    const shared_ptr<OHOS::NativeRdb::RdbStore> &rdbStore) : rdbStore_(rdbStore) {}

TransactionOperations::~TransactionOperations()
{
    if (isStart && !isFinish) {
        TransactionRollback();
    }
}

int32_t TransactionOperations::Start(bool isUpgrade)
{
    if (isStart || isFinish) {
        return 0;
    }
    int32_t errCode = BeginTransaction(isUpgrade);
    if (errCode == 0) {
        isStart = true;
    }
    return errCode;
}

void TransactionOperations::Finish()
{
    if (!isStart) {
        return;
    }
    if (!isFinish) {
        int32_t ret = TransactionCommit();
        if (ret == 0) {
            isFinish = true;
        }
    }
}

int32_t TransactionOperations::BeginTransaction(bool isUpgrade)
{
    if (rdbStore_ == nullptr) {
        MEDIA_ERR_LOG("Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
        return E_HAS_DB_ERROR;
    }
    MEDIA_DEBUG_LOG("Start transaction");

    unique_lock<mutex> cvLock(transactionMutex_);
    if (isInTransaction_.load()) {
        transactionCV_.wait_for(cvLock, chrono::milliseconds(RDB_TRANSACTION_WAIT_MS),
            [this] () { return !(isInTransaction_.load()); });
    }

    int curTryTime = 0;
    int maxTryTimes = isUpgrade ? MAX_TRY_TIMES_FOR_UPGRADE : MAX_TRY_TIMES;
    while (curTryTime < maxTryTimes) {
        if (rdbStore_->IsInTransaction()) {
            if (!isInTransaction_.load()) {
                MEDIA_INFO_LOG("Stop cloud sync");
                FileManagement::CloudSync::CloudSyncManager::GetInstance()
                    .StopSync("com.ohos.medialibrary.medialibrarydata");
                isSkipCloudSync = true;
            }
            this_thread::sleep_for(chrono::milliseconds(TRANSACTION_WAIT_INTERVAL));
            if (isInTransaction_.load() || rdbStore_->IsInTransaction()) {
                curTryTime++;
                MEDIA_INFO_LOG("RdbStore is in transaction, try %{public}d times...", curTryTime);
                continue;
            }
        }

        int32_t errCode = rdbStore_->BeginTransaction();
        if (errCode == NativeRdb::E_SQLITE_LOCKED || errCode == NativeRdb::E_DATABASE_BUSY ||
            errCode == NativeRdb::E_SQLITE_BUSY) {
            curTryTime++;
            MEDIA_ERR_LOG("Sqlite database file is locked! try %{public}d times...", curTryTime);
            continue;
        } else if (errCode != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Start Transaction failed, errCode=%{public}d", errCode);
            isInTransaction_.store(false);
            transactionCV_.notify_one();
            MediaLibraryRestore::GetInstance().CheckRestore(errCode);
            return E_HAS_DB_ERROR;
        } else {
            isInTransaction_.store(true);
            return E_OK;
        }
    }

    MEDIA_ERR_LOG("RdbStore is still in transaction after try %{public}d times, abort.", maxTryTimes);
    return E_HAS_DB_ERROR;
}

int32_t TransactionOperations::TransactionCommit()
{
    if (rdbStore_ == nullptr) {
        return E_HAS_DB_ERROR;
    }
    MEDIA_DEBUG_LOG("Try commit transaction");

    if (!(isInTransaction_.load()) || !(rdbStore_->IsInTransaction())) {
        MEDIA_ERR_LOG("no transaction now");
        return E_HAS_DB_ERROR;
    }

    int32_t errCode = rdbStore_->Commit();
    isInTransaction_.store(false);
    transactionCV_.notify_all();
    if (errCode != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("commit failed, errCode=%{public}d", errCode);
        return E_HAS_DB_ERROR;
    }

    if (isSkipCloudSync) {
        MEDIA_INFO_LOG("recover cloud sync for commit");
        CloudSyncHelper::GetInstance()->StartSync();
        isSkipCloudSync = false;
    }

    return E_OK;
}

int32_t TransactionOperations::TransactionRollback()
{
    if (rdbStore_ == nullptr) {
        return E_HAS_DB_ERROR;
    }
    MEDIA_INFO_LOG("Try rollback transaction");

    if (!(isInTransaction_.load()) || !(rdbStore_->IsInTransaction())) {
        MEDIA_ERR_LOG("no transaction now");
        return E_HAS_DB_ERROR;
    }

    int32_t errCode = rdbStore_->RollBack();
    isInTransaction_.store(false);
    transactionCV_.notify_all();
    if (errCode != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("rollback failed, errCode=%{public}d", errCode);
        return E_HAS_DB_ERROR;
    }

    if (isSkipCloudSync) {
        MEDIA_INFO_LOG("recover cloud sync for rollback");
        CloudSyncHelper::GetInstance()->StartSync();
        isSkipCloudSync = false;
    }

    return E_OK;
}
} // namespace OHOS::Media
