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

#include "backup_hi_audit.h"

#include <chrono>
#include <ctime>
#include <dirent.h>
#include <fcntl.h>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <sys/time.h>
#include <unistd.h>

#include "backup_zip_util.h"
#include "media_log.h"
#include "media_file_utils.h"

namespace OHOS::Media {
struct BackupHiAuditConfig {
    std::string logPath;
    std::string logName;
    uint32_t logSize;
    uint32_t fileSize;
    uint32_t fileCount;
};

const BackupHiAuditConfig HIAUDIT_CONFIG = { "/data/storage/el2/log/audit/", "media_library_backup", 2 * 1024,
    3 * 1024 * 1024, 10 };
constexpr int8_t MILLISECONDS_LENGTH = 3;
constexpr int64_t SEC_TO_MILLISEC = 1000;
constexpr int MAX_TIME_BUFF = 64;
const std::string HIAUDIT_LOG_NAME = HIAUDIT_CONFIG.logPath + HIAUDIT_CONFIG.logName + "_audit.csv";
// LCOV_EXCL_START
BackupHiAudit::BackupHiAudit()
{
    Init();
}

BackupHiAudit::~BackupHiAudit()
{
    if (writeFd_ >= 0) {
        close(writeFd_);
    }
}

BackupHiAudit& BackupHiAudit::GetInstance()
{
    static BackupHiAudit hiAudit;
    return hiAudit;
}

void BackupHiAudit::Init()
{
    if (!std::filesystem::exists(HIAUDIT_CONFIG.logPath)) {
        if (!MediaFileUtils::CreateDirectory(HIAUDIT_CONFIG.logPath)) {
            MEDIA_ERR_LOG("Create hiaudit log dir  %{public}s failed", HIAUDIT_CONFIG.logPath.c_str());
            return ;
        }
        std::filesystem::permissions(HIAUDIT_CONFIG.logPath,
            std::filesystem::perms::owner_all | std::filesystem::perms::group_all | std::filesystem::perms::others_all,
            std::filesystem::perm_options::replace);
    }

    std::lock_guard<std::mutex> lock(mutex_);
    writeFd_ = open(HIAUDIT_LOG_NAME.c_str(), O_CREAT | O_APPEND | O_RDWR,
        S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (writeFd_ < 0) {
        MEDIA_ERR_LOG("writeFd_ open error errno: %{public}d", errno);
    }
    struct stat st;
    writeLogSize_ = stat(HIAUDIT_LOG_NAME.c_str(), &st) ? 0 : static_cast<uint64_t>(st.st_size);
    MEDIA_INFO_LOG("writeLogSize: %{public}u", writeLogSize_.load());
}

uint64_t BackupHiAudit::GetMilliseconds()
{
    auto now = std::chrono::system_clock::now();
    auto millisecs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return millisecs.count();
}

std::string BackupHiAudit::GetFormattedTimestamp(time_t timeStamp, const std::string& format)
{
    auto seconds = timeStamp / SEC_TO_MILLISEC;
    char date[MAX_TIME_BUFF] = {0};
    struct tm result {};
    if (localtime_r(&seconds, &result) != nullptr) {
        strftime(date, MAX_TIME_BUFF, format.c_str(), &result);
    }
    return std::string(date);
}

std::string BackupHiAudit::GetFormattedTimestampEndWithMilli()
{
    uint64_t milliSeconds = GetMilliseconds();
    std::string formattedTimeStamp = GetFormattedTimestamp(milliSeconds, "%Y%m%d%H%M%S");
    std::stringstream ss;
    ss << formattedTimeStamp;
    milliSeconds = milliSeconds % SEC_TO_MILLISEC;
    ss << std::setfill('0') << std::setw(MILLISECONDS_LENGTH) << milliSeconds;
    return ss.str();
}

void BackupHiAudit::Write(const BackupAuditLog& auditLog)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (writeLogSize_ == 0) {
        WriteToFile(auditLog.TitleString() + "\n");
    }
    std::string writeLog = GetFormattedTimestampEndWithMilli() + "," +
        HIAUDIT_CONFIG.logName + ",NO," + auditLog.ToString();
    if (writeLog.length() > HIAUDIT_CONFIG.logSize) {
        MEDIA_INFO_LOG("write exceeds %{public}u: %{public}s.", HIAUDIT_CONFIG.logSize, writeLog.c_str());
        writeLog = writeLog.substr(0, HIAUDIT_CONFIG.logSize);
    }
    writeLog = writeLog + "\n";
    WriteToFile(writeLog);
}

void BackupHiAudit::GetWriteFilePath()
{
    if (writeLogSize_ < HIAUDIT_CONFIG.fileSize) {
        return;
    }

    close(writeFd_);
    ZipAuditLog();
    CleanOldAuditFile();

    writeFd_ = open(HIAUDIT_LOG_NAME.c_str(), O_CREAT | O_TRUNC | O_RDWR,
        S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    CHECK_AND_PRINT_LOG(writeFd_ >= 0, "fd open error errno: %{public}d", errno);

    writeLogSize_ = 0;
}

void BackupHiAudit::CleanOldAuditFile()
{
    DIR* dir = opendir(HIAUDIT_CONFIG.logPath.c_str());
    CHECK_AND_RETURN_LOG(dir != nullptr, "failed open dir, errno: %{public}d.", errno);

    uint32_t zipFileSize = 0;
    std::string oldestAuditFile;
    struct dirent *ptr = nullptr;
    while ((ptr = readdir(dir)) != nullptr) {
        if (std::string(ptr->d_name).find(HIAUDIT_CONFIG.logName) != std::string::npos &&
            std::string(ptr->d_name).find("zip") != std::string::npos) {
            zipFileSize = zipFileSize + 1;
            if (oldestAuditFile.empty()) {
                oldestAuditFile = HIAUDIT_CONFIG.logPath + std::string(ptr->d_name);
                continue;
            }
            struct stat st;
            stat((HIAUDIT_CONFIG.logPath + std::string(ptr->d_name)).c_str(), &st);
            struct stat oldestSt;
            stat(oldestAuditFile.c_str(), &oldestSt);
            if (st.st_mtime < oldestSt.st_mtime) {
                oldestAuditFile = HIAUDIT_CONFIG.logPath + std::string(ptr->d_name);
            }
        }
    }
    closedir(dir);
    if (zipFileSize > HIAUDIT_CONFIG.fileCount) {
        remove(oldestAuditFile.c_str());
    }
}

void BackupHiAudit::WriteToFile(const std::string& content)
{
    GetWriteFilePath();
    if (writeFd_ < 0) {
        MEDIA_ERR_LOG("fd invalid.");
        return;
    }
    write(writeFd_, content.c_str(), content.length());
    writeLogSize_ = writeLogSize_ + content.length();
}

void BackupHiAudit::ZipAuditLog()
{
    std::string zipFileName = HIAUDIT_CONFIG.logPath + HIAUDIT_CONFIG.logName + "_audit_" +
        GetFormattedTimestamp(GetMilliseconds(), "%Y%m%d%H%M%S");
    std::rename(HIAUDIT_LOG_NAME.c_str(), (zipFileName + ".csv").c_str());
    zipFile compressZip = BackupZipUtil::CreateZipFile(zipFileName + ".zip");
    if (compressZip == nullptr) {
        MEDIA_WARN_LOG("open zip file failed.");
        return;
    }
    if (BackupZipUtil::AddFileInZip(compressZip, zipFileName + ".csv", KEEP_NONE_PARENT_PATH) == 0) {
        remove((zipFileName + ".csv").c_str());
    }
    BackupZipUtil::CloseZipFile(compressZip);
}
// LCOV_EXCL_STOP
} // namespace OHOS::Media