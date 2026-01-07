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

#include "backup_zip_util.h"

#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <securec.h>

#include "media_log.h"

namespace OHOS::Media {
namespace {
constexpr int READ_MORE_LENGTH = 100 * 1024;
constexpr int ERROR_MEMSET_STRUCT = 1001;
constexpr int ERROR_GET_HANDLE = 1002;
};

zipFile BackupZipUtil::CreateZipFile(const std::string& zipPath, int32_t zipMode)
{
    return zipOpen(zipPath.c_str(), zipMode);
}

void BackupZipUtil::CloseZipFile(zipFile& zipfile)
{
    zipClose(zipfile, nullptr);
}

int BackupZipUtil::AddFileInZip(
    zipFile& zipfile, const std::string& srcFile, int keepParentPathStatus, const std::string& destFileName)
{
    zip_fileinfo zipInfo;
    errno_t result = memset_s(&zipInfo, sizeof(zipInfo), 0, sizeof(zipInfo));
    CHECK_AND_RETURN_RET_LOG(result == EOK, ERROR_MEMSET_STRUCT,
        "AddFileInZip memset_s error, file:%{public}s.", srcFile.c_str());
    FILE *srcFp = GetFileHandle(srcFile);
    CHECK_AND_RETURN_RET_LOG(srcFp != nullptr, ERROR_GET_HANDLE,
        "get file handle failed:%{public}s, errno: %{public}d.", srcFile.c_str(), errno);
    std::string srcFileName = GetDestFilePath(srcFile, destFileName, keepParentPathStatus);
    zipOpenNewFileInZip(
        zipfile, srcFileName.c_str(), &zipInfo, nullptr, 0, nullptr, 0, nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION);

    int errcode = 0;
    char buf[READ_MORE_LENGTH] = {0};
    while (!feof(srcFp)) {
        size_t numBytes = fread(buf, 1, sizeof(buf), srcFp);
        if (numBytes == 0) {
            break;
        }
        zipWriteInFileInZip(zipfile, buf, static_cast<unsigned int>(numBytes));
        if (ferror(srcFp)) {
            MEDIA_ERR_LOG("zip file failed:%{public}s, errno: %{public}d.", srcFile.c_str(), errno);
            errcode = errno;
            break;
        }
    }
    (void)fclose(srcFp);
    zipCloseFileInZip(zipfile);
    return errcode;
}

FILE* BackupZipUtil::GetFileHandle(const std::string& file)
{
    char realPath[PATH_MAX] = {0};
    CHECK_AND_RETURN_RET(realpath(file.c_str(), realPath) != nullptr, nullptr);
    return fopen(realPath, "rb");
}

std::string BackupZipUtil::GetDestFilePath(
    const std::string& srcFile, const std::string& destFilePath, int keepParentPathStatus)
{
    CHECK_AND_RETURN_RET(destFilePath.empty(), destFilePath);
    std::string file = srcFile;
    std::string result = file;
    std::string parentPathName;
    auto pos = file.rfind("/");
    if (pos != std::string::npos && pos != file.length() - 1) {
        result = file.substr(pos + 1);
        std::string parent = file.substr(0, pos);
        pos = parent.rfind("/");
        if (pos != std::string::npos && pos != parent.length() - 1) {
            parentPathName = parent.substr(pos + 1);
        } else {
            parentPathName = parent;
        }
    }
    parentPathName.append("/");
    if (keepParentPathStatus == KEEP_ONE_PARENT_PATH) {
        // srcFileName with relative directory path
        result.insert(0, parentPathName);
    }
    return result;
}
} // namespace OHOS::Media