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
#define MLOG_TAG "Media_Db_Integrity_Checker"

#include "db_integrity_checker.h"
#include "media_log.h"
#include "backup_database_utils.h"
#include "application_context.h"
#include <sys/stat.h>

namespace OHOS {
namespace Media {

static constexpr const char* MEDIA_DB_PATH = "/data/storage/el2/database/rdb/media_library.db";
static constexpr const char* BACKUP_DIR_NAME = ".backup";
static constexpr int32_t INTEGRITY_CHECK_TIMEOUT_MS = 5000;

DbIntegrityChecker::DbIntegrityChecker(const std::string& backupRestorePath,
    const std::string& upgradePath)
    : backupRestorePath_(backupRestorePath), upgradePath_(upgradePath)
{
}

DbIntegrityChecker::~DbIntegrityChecker()
{
    if (checkFuture_.valid()) {
        checkFuture_.wait();
    }
}

void DbIntegrityChecker::StartAsyncCheck()
{
    isCancelled_.store(false);
    checkFuture_ = std::async(std::launch::async, [this]() {
        return PerformIntegrityCheck();
    });
}

bool DbIntegrityChecker::WaitForResult(int32_t timeoutMs)
{
    if (!checkFuture_.valid()) {
        MEDIA_ERR_LOG("DbIntegrityChecker: check task not started");
        return false;
    }

    auto futureStatus = checkFuture_.wait_for(std::chrono::milliseconds(timeoutMs));
    if (futureStatus == std::future_status::timeout) {
        MEDIA_ERR_LOG("DbIntegrityChecker: integrity check timeout, cancelling task");
        isCancelled_.store(true);
        return false;
    }

    checkResult_ = checkFuture_.get();
    isCheckCompleted_.store(true);
    return checkResult_;
}

bool DbIntegrityChecker::IsCheckPassed() const
{
    return checkResult_.load();
}

bool DbIntegrityChecker::PerformIntegrityCheck()
{
    MEDIA_INFO_LOG("DbIntegrityChecker: start integrity check");

    std::string backupDbPath = backupRestorePath_ + MEDIA_DB_PATH;
    struct stat statBuf;
    if (stat(backupDbPath.c_str(), &statBuf) != 0) {
        MEDIA_WARN_LOG("DbIntegrityChecker: backup db not exist: %{public}s", backupDbPath.c_str());
        return true;
    }

    if (isCancelled_.load()) {
        MEDIA_WARN_LOG("DbIntegrityChecker: task cancelled before database check");
        return false;
    }

    if (!OpenAndCheckDatabase(backupDbPath)) {
        MEDIA_ERR_LOG("DbIntegrityChecker: integrity check failed for backup db");
        return false;
    }

    MEDIA_INFO_LOG("DbIntegrityChecker: integrity check passed");
    return true;
}

bool DbIntegrityChecker::OpenAndCheckDatabase(const std::string& dbPath)
{
    auto context = AbilityRuntime::Context::GetApplicationContext();
    if (context == nullptr) {
        MEDIA_ERR_LOG("DbIntegrityChecker: get application context failed");
        return false;
    }

    std::shared_ptr<NativeRdb::RdbStore> rdbStore;
    int32_t err = BackupDatabaseUtils::InitDb(rdbStore, CONST_MEDIA_DATA_ABILITY_DB_NAME,
        dbPath, CONST_BUNDLE_NAME, true, context->GetArea());
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("DbIntegrityChecker: open db failed, err=%{public}d", err);
        return false;
    }

    std::string integrityCheckSql = "PRAGMA integrity_check";
    auto resultSet = BackupDatabaseUtils::QuerySql(rdbStore, integrityCheckSql, {});
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("DbIntegrityChecker: integrity_check query failed");
        return false;
    }

    int32_t errCode = resultSet->GoToFirstRow();
    if (errCode != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("DbIntegrityChecker: go to first row failed, err=%{public}d", errCode);
        resultSet->Close();
        return false;
    }

    std::string result;
    errCode = resultSet->GetString(0, result);
    resultSet->Close();

    if (errCode != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("DbIntegrityChecker: get result string failed, err=%{public}d", errCode);
        return false;
    }

    bool isOk = (result == "ok");
    MEDIA_INFO_LOG("DbIntegrityChecker: integrity_check result: %{public}s", result.c_str());
    return isOk;
}

} // namespace Media
} // namespace OHOS