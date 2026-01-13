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

#ifndef BACKUP_LOG_UTILS_H
#define BACKUP_LOG_UTILS_H

#include <string>

#include "backup_const.h"
#include "media_backup_report_data_type.h"

namespace OHOS::Media {
class BackupLogUtils {
public:
    static std::string FileInfoToString(int32_t sceneCode, const FileInfo &info,
        const std::vector<std::string> &extendList = {});
    static std::string ErrorInfoToString(const ErrorInfo &info);
    static std::string RestoreErrorToString(int32_t error);
    static std::string Format(const std::string &infoString);
    static std::string FileDbCheckInfoToString(const FileDbCheckInfo &info);
};
} // namespace OHOS::Media

#endif // BACKUP_LOG_UTILS_H