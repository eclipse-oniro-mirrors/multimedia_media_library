/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "Media_Cloud_Utils"

#include "cloud_media_file_utils.h"
#include "media_path_utils.h"

#include <sys/ioctl.h>
#include <sys/xattr.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

#include "hmdfs.h"
#include "media_log.h"
#include "media_file_utils.h"

namespace OHOS::Media::CloudSync {
constexpr unsigned WRITEOPEN_CMD = 0x02;
#define HMDFS_IOC_GET_WRITEOPEN_CNT _IOR(HMDFS_IOC, WRITEOPEN_CMD, uint32_t)
#define O_RDONLY 00

bool CloudMediaFileUtils::GetParentPathAndFilename(
    const std::string &path, std::string &parentPath, std::string &filename)
{
    size_t slashIndex = path.rfind("/");
    size_t pointIndex = path.rfind(".");
    if (slashIndex == std::string::npos || pointIndex == std::string::npos || slashIndex >= pointIndex) {
        MEDIA_ERR_LOG("Invalid path: %{public}s.", MediaPathUtils::DesensitizePath(path).c_str());
        return false;
    }
    parentPath = path.substr(0, slashIndex + 1);
    filename = path.substr(slashIndex + 1);
    return true;
}

bool CloudMediaFileUtils::GetFileSizeV2(const std::string &filePath, size_t &size)
{
    MEDIA_DEBUG_LOG("GetFileSize enter: %{public}s", filePath.c_str());
    struct stat statbuf;
    if (stat(filePath.c_str(), &statbuf) == -1) {
        MEDIA_WARN_LOG("GetFileSize Failed, errno: %{public}d, path: %{public}s", errno, filePath.c_str());
        size = 0;
        return false;
    }
    if (statbuf.st_size < 0) {
        MEDIA_WARN_LOG("GetFileSize negative, path: %{public}s", filePath.c_str());
        size = 0;
        return false;
    }
    size = static_cast<size_t>(statbuf.st_size);
    MEDIA_DEBUG_LOG("GetFileSize end: %{public}s", filePath.c_str());
    return true;
}

bool CloudMediaFileUtils::LocalWriteOpen(const std::string &dfsPath)
{
    std::unique_ptr<char[]> absPath = std::make_unique<char[]>(PATH_MAX + 1);
    if (realpath(dfsPath.c_str(), absPath.get()) == nullptr) {
        return false;
    }
    std::string realPath = absPath.get();
    char resolvedPath[PATH_MAX] = {'\0'};
    char *realPaths = realpath(realPath.c_str(), resolvedPath);
    if (realPaths == NULL) {
        MEDIA_ERR_LOG("realpath failed");
        return false;
    }
    int fd = open(realPaths, O_RDONLY);
    if (fd < 0) {
        MEDIA_ERR_LOG("open failed, errno:%{public}d", errno);
        return false;
    }
    uint32_t writeOpenCnt = 0;
    int ret = ioctl(fd, HMDFS_IOC_GET_WRITEOPEN_CNT, &writeOpenCnt);
    close(fd);
    if (ret < 0) {
        MEDIA_ERR_LOG("ioctl failed, errno:%{public}d", errno);
        return false;
    }

    return writeOpenCnt != 0;
}
}  // namespace OHOS::Media::CloudSync