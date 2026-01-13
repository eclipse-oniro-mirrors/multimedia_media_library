/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "hi_audit.h"

#include <chrono>
#include <ctime>
#include <dirent.h>
#include <fcntl.h>
#include <iomanip>
#include <sstream>
#include <sys/time.h>
#include <unistd.h>

#include "media_log.h"
#include "zip_util.h"

namespace OHOS::Media {
struct HiAuditConfig {
    std::string logPath;
    std::string logName;
    uint32_t logSize;
    uint32_t fileSize;
    uint32_t fileCount;
};

const HiAuditConfig HIAUDIT_CONFIG = { "/data/storage/el2/log/audit/", "media_library", 2 * 1024, 3 * 1204 * 1024, 10 };
constexpr int8_t MILLISECONDS_LENGTH = 3;
constexpr int64_t SEC_TO_MILLISEC = 1000;
constexpr int MAX_TIME_BUFF = 64;
const std::string HIAUDIT_LOG_NAME = HIAUDIT_CONFIG.logPath + HIAUDIT_CONFIG.logName + "_audit.csv";

HiAudit::HiAudit()
{
    Init();
}

HiAudit::~HiAudit()
{
    if (writeFd_ >= 0) {
        close(writeFd_);
    }
}

HiAudit& HiAudit::GetInstance()
{
    static HiAudit hiAudit;
    return hiAudit;
}

void HiAudit::Init()
{
    if (access(HIAUDIT_CONFIG.logPath.c_str(), F_OK) != 0) {
        int ret = mkdir(HIAUDIT_CONFIG.logPath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
        if (ret != 0) {
            MEDIA_ERR_LOG("Failed to create directory %{public}s.", HIAUDIT_CONFIG.logPath.c_str());
        }
    }
    std::lock_guard<std::mutex> lock(mutex_);
    writeFd_ = open(HIAUDIT_LOG_NAME.c_str(), O_CREAT | O_APPEND | O_RDWR,
        S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (writeFd_ < 0) {
        MEDIA_ERR_LOG("writeFd_ open error errno: %{public}d", errno);
    }
    struct stat st;
    writeLogSize_ = stat(HIAUDIT_LOG_NAME.c_str(), &st) ? 0 : static_cast<uint64_t>(st.st_size);
    MEDIA_INFO_LOG("writeLogSize: %{public}u", writeLogSize_.load());
}

uint64_t HiAudit::GetMilliseconds()
{
    auto now = std::chrono::system_clock::now();
    auto millisecs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return millisecs.count();
}

std::string HiAudit::GetFormattedTimestamp(time_t timeStamp, const std::string& format)
{
    auto seconds = timeStamp / SEC_TO_MILLISEC;
    char date[MAX_TIME_BUFF] = {0};
    struct tm result {};
    if (localtime_r(&seconds, &result) != nullptr) {
        strftime(date, MAX_TIME_BUFF, format.c_str(), &result);
    }
    return std::string(date);
}

std::string HiAudit::GetFormattedTimestampEndWithMilli()
{
    uint64_t milliSeconds = GetMilliseconds();
    std::string formattedTimeStamp = GetFormattedTimestamp(milliSeconds, "%Y%m%d%H%M%S");
    std::stringstream ss;
    ss << formattedTimeStamp;
    milliSeconds = milliSeconds % SEC_TO_MILLISEC;
    ss << std::setfill('0') << std::setw(MILLISECONDS_LENGTH) << milliSeconds;
    return ss.str();
}

void HiAudit::Write(const AuditLog& auditLog)
{
    MEDIA_INFO_LOG("write");
    std::lock_guard<std::mutex> lock(mutex_);
    if (writeLogSize_ == 0) {
        WriteToFile(auditLog.TitleString() + "\n");
    }
    std::string writeLog = GetFormattedTimestampEndWithMilli() + ", " +
        HIAUDIT_CONFIG.logName + ", NO, " + auditLog.ToString();
    MEDIA_INFO_LOG("write %{public}s.", writeLog.c_str());
    if (writeLog.length() > HIAUDIT_CONFIG.logSize) {
        writeLog = writeLog.substr(0, HIAUDIT_CONFIG.logSize);
    }
    writeLog = writeLog + "\n";
    WriteToFile(writeLog);
}

void HiAudit::GetWriteFilePath()
{
    if (writeLogSize_ < HIAUDIT_CONFIG.fileSize) {
        return;
    }

    close(writeFd_);
    ZipAuditLog();
    CleanOldAuditFile();
    
    writeFd_ = open(HIAUDIT_LOG_NAME.c_str(), O_CREAT | O_TRUNC | O_RDWR,
        S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (writeFd_ < 0) {
        MEDIA_ERR_LOG("fd open error errno: %{public}d", errno);
    }
    
    writeLogSize_ = 0;
}

void HiAudit::CleanOldAuditFile()
{
    uint32_t zipFileSize = 0;
    std::string oldestAuditFile;
    DIR* dir = opendir(HIAUDIT_CONFIG.logPath.c_str());
    while (true) {
        struct dirent* ptr = readdir(dir);
        if (ptr == nullptr) {
            break;
        }
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

void HiAudit::WriteToFile(const std::string& content)
{
    GetWriteFilePath();
    if (writeFd_ < 0) {
        MEDIA_ERR_LOG("fd invalid.");
        return;
    }
    write(writeFd_, content.c_str(), content.length());
    writeLogSize_ = writeLogSize_ + content.length();
}

void HiAudit::ZipAuditLog()
{
    std::string zipFileName = HIAUDIT_CONFIG.logPath + HIAUDIT_CONFIG.logName + "_audit_" +
        GetFormattedTimestamp(GetMilliseconds(), "%Y%m%d%H%M%S");
    std::rename(HIAUDIT_LOG_NAME.c_str(), (zipFileName + ".csv").c_str());
    zipFile compressZip = Media::ZipUtil::CreateZipFile(zipFileName + ".zip");
    if (compressZip == nullptr) {
        MEDIA_WARN_LOG("open zip file failed.");
        return;
    }
    if (Media::ZipUtil::AddFileInZip(compressZip, zipFileName + ".csv", Media::KEEP_NONE_PARENT_PATH) == 0) {
        remove((zipFileName + ".csv").c_str());
    }
    Media::ZipUtil::CloseZipFile(compressZip);
}

void HiAudit::WriteForCloudDownload(const std::string& bundleName, const int32_t& downloadType,
    const std::string& status)
{
    MEDIA_ERR_LOG("cloud download, bundleName: %{public}s, downloadType: %{public}d, status: %{public}s",
        bundleName.c_str(), downloadType, status.c_str());
    AuditLog auditLog = {true, "DFX", "CLOUD_DOWNLOAD", "0", 1};
    auditLog.id = bundleName;
    auditLog.type = downloadType;
    auditLog.operationStatus = status;
    Write(auditLog);
}
}