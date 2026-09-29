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

#ifndef OHOS_MEDIA_DB_INTEGRITY_CHECKER_H
#define OHOS_MEDIA_DB_INTEGRITY_CHECKER_H

#include <memory>
#include <string>
#include <future>
#include <atomic>
#include "rdb_store.h"

namespace OHOS {
namespace Media {
class BackupDatabaseUtils;

class DbIntegrityChecker {
public:
    explicit DbIntegrityChecker(const std::string& backupRestorePath,
        const std::string& upgradePath);
    ~DbIntegrityChecker();

    DbIntegrityChecker(const DbIntegrityChecker&) = delete;
    DbIntegrityChecker& operator=(const DbIntegrityChecker&) = delete;
    DbIntegrityChecker(DbIntegrityChecker&&) = delete;
    DbIntegrityChecker& operator=(DbIntegrityChecker&&) = delete;

    void StartAsyncCheck();
    // max = 20min
    bool WaitForResult(int32_t timeoutMs = 1200000);
    bool IsCheckPassed() const;

private:
    bool PerformIntegrityCheck();
    bool OpenAndCheckDatabase(const std::string& dbPath);

    std::string backupRestorePath_;
    std::string upgradePath_;
    std::atomic<bool> checkResult_{false};
    std::atomic<bool> isCheckCompleted_{false};
    std::atomic<bool> isCancelled_{false};
    std::future<bool> checkFuture_;
};

} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIA_DB_INTEGRITY_CHECKER_H