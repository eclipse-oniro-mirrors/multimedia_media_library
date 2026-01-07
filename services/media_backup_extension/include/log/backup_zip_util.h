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

#ifndef BACKUP_ZIP_UTIL_H
#define BACKUP_ZIP_UTIL_H

#include <string>
#include <contrib/minizip/zip.h>

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))
enum {
    KEEP_NONE_PARENT_PATH,
    KEEP_ONE_PARENT_PATH,
};

class BackupZipUtil {
public:
    EXPORT static zipFile CreateZipFile(const std::string& zipPath, int32_t zipMode = APPEND_STATUS_CREATE);
    EXPORT static void CloseZipFile(zipFile& zipfile);
    EXPORT static int AddFileInZip(
        zipFile& zipfile, const std::string& srcFile, int keepParentPathStatus, const std::string& dstFileName = "");
    EXPORT static std::string GetDestFilePath(
        const std::string& srcFile, const std::string& destFilePath, int keepParentPathStatus);

private:
    static FILE* GetFileHandle(const std::string& file);
};
} // namespace OHOS::Media
#endif // BACKUP_ZIP_UTIL_H