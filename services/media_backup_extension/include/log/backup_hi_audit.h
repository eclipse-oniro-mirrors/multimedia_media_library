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

#ifndef BACKUP_HI_AUDIT_H
#define BACKUP_HI_AUDIT_H

#include <cstdint>
#include <mutex>
#include <queue>
#include <string>
#include <sys/stat.h>

#include "nocopyable.h"

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))
struct BackupAuditLog {
    bool isUserBehavior{false};
    std::string cause;
    std::string operationType;
    std::string operationScenario;
    uint32_t operationCount{0};
    std::string operationStatus;
    std::string extend;
    std::string taskId;

    const virtual std::string TitleString() const
    {
        return "happenTime,packageName,isForeground,cause,isUserBehavior,operationType,operationScenario,"
            "operationStatus,operationCount,extend,taskId";
    }

    const virtual std::string ToString() const
    {
        return cause + "," + std::to_string(isUserBehavior) + "," + operationType + "," + operationScenario +
            "," + operationStatus + "," + std::to_string(operationCount) + "," + extend + "," + taskId;
    }
};

class BackupHiAudit : public NoCopyable {
public:
    EXPORT static BackupHiAudit& GetInstance();
    EXPORT void Write(const BackupAuditLog& auditLog);

private:
    BackupHiAudit();
    ~BackupHiAudit();

    void Init();
    void GetWriteFilePath();
    void WriteToFile(const std::string& log);
    uint64_t GetMilliseconds();
    std::string GetFormattedTimestamp(time_t timeStamp, const std::string& format);
    std::string GetFormattedTimestampEndWithMilli();
    void CleanOldAuditFile();
    void ZipAuditLog();

private:
    std::mutex mutex_;
    int writeFd_;
    std::atomic<uint32_t> writeLogSize_ = 0;
};
} // namespace OHOS::Media
#endif // BACKUP_HI_AUDIT_H