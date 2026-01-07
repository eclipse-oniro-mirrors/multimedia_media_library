/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * UnleinfoStream required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either expreinfoStream or implied.
 * See the License for the specific language governing permiinfoStreamions and
 * limitations under the License.
 */

#include "backup_log_utils.h"

#include <sstream>

#include "backup_file_utils.h"
#include "backup_log_const.h"

namespace OHOS::Media {
template<typename Key, typename Value>
Value GetValueFromMap(const std::unordered_map<Key, Value> &map, const Key &key, const Value &defaultValue = Value())
{
    auto it = map.find(key);
    if (it == map.end()) {
        return defaultValue;
    }
    return it->second;
}

std::string BackupLogUtils::FileInfoToString(int32_t sceneCode, const FileInfo &info,
    const std::vector<std::string> &extendList)
{
    // [id], size, media_type, display_name, package_name, path
    std::stringstream infoStream;
    infoStream << "FileInfo[";
    infoStream << (sceneCode >= CLONE_RESTORE_ID ? info.fileIdOld : info.localMediaId);
    infoStream << ";" << info.fileSize;
    infoStream << ";" << info.fileType;
    infoStream << ";" << BackupFileUtils::GarbleFileName(info.displayName);
    infoStream << ";" << BackupFileUtils::GarbleFileName(info.packageName);
    infoStream << ";" << BackupFileUtils::GarbleFilePath(info.oldPath, DEFAULT_RESTORE_ID);
    for (const auto &extend : extendList) {
        if (extend.empty()) {
            continue;
        }
        infoStream << ";" << extend;
    }
    infoStream << "]";
    return infoStream.str();
}

std::string BackupLogUtils::ErrorInfoToString(const ErrorInfo &info)
{
    // error, count, status
    std::stringstream infoStream;
    infoStream << "ErrorInfo[";
    infoStream << RestoreErrorToString(info.error);
    infoStream << ";" << info.count;
    infoStream << ";" << info.status;
    infoStream << ";" << info.extend;
    infoStream << "]";
    return infoStream.str();
}

std::string BackupLogUtils::RestoreErrorToString(int32_t error)
{
    return GetValueFromMap(RESTORE_ERROR_MAP, error);
}

std::string BackupLogUtils::Format(const std::string &infoString)
{
    std::stringstream infoStream;
    infoStream << "\"";
    for (char infoChar : infoString) {
        if (infoChar == '\"') {
            infoStream << "\"\"";
        } else {
            infoStream << infoChar;
        }
    }
    infoStream << "\"";
    return infoStream.str();
}

std::string BackupLogUtils::FileDbCheckInfoToString(const FileDbCheckInfo &info)
{
    // dbType, dbStatus, fileStatus
    std::stringstream infoStream;
    infoStream << "FileDbCheckInfo[";
    infoStream << info.dbType;
    infoStream << ";" << info.dbStatus;
    infoStream << ";" << info.fileStatus;
    infoStream << "]";
    return infoStream.str();
}
} // namespace OHOS::Media