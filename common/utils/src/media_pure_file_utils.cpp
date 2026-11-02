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

#include "media_pure_file_utils.h"

#include <climits>
#include <fcntl.h>
#include <ftw.h>
#include <sstream>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unistd.h>

#include "directory_ex.h"
#include "media_log.h"
#include "media_path_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_type_const.h"

namespace OHOS::Media {
const int32_t OPEN_FDS = 64;

namespace {
const mode_t CHOWN_RWX_USR_GRP = 02771;
const mode_t CHOWN_RO_USR_GRP = 0644;
const size_t MAX_SENDFILE_SIZE = 0x7FFFF000;

/**
 * 文件描述符的 RAII 包装，等价于 media_file_utils 原来使用的 UniqueFd（在不引入 init 依赖的前提下保持一致的关闭语义）
 */
class ScopedFd {
public:
    explicit ScopedFd(int32_t fd) : fd_(fd) {}
    ~ScopedFd()
    {
        if (fd_ != E_ERR) {
            close(fd_);
        }
    }
    ScopedFd(const ScopedFd &) = delete;
    ScopedFd &operator=(const ScopedFd &) = delete;

    int32_t Get() const
    {
        return fd_;
    }
private:
    int32_t fd_ = E_ERR;
};

static bool SendFileWithRetry(int32_t dest, int32_t source, off_t totalSize, off_t &copied)
{
    while (copied < totalSize) {
        off_t leftSize = totalSize - copied;
        const off_t maxSendfileSize = static_cast<off_t>(MAX_SENDFILE_SIZE);
        size_t sendSize = static_cast<size_t>(leftSize > maxSendfileSize ? maxSendfileSize : leftSize);
        ssize_t ret = sendfile(dest, source, &copied, sendSize);
        if (ret == E_ERR) {
            CHECK_AND_CONTINUE(errno != EINTR);
            MEDIA_ERR_LOG("Failed to sendfile, errno: %{public}d", errno);
            return false;
        }
        CHECK_AND_RETURN_RET_LOG(ret != 0, false,
            "sendfile returned 0 before copy finished, copied: %{public}lld, total: %{public}lld",
            static_cast<long long>(copied), static_cast<long long>(totalSize));
    }
    return true;
}
} // namespace

bool MediaPureFileUtils::IsFileExists(const std::string &fileName)
{
    struct stat statInfo {};

    return ((stat(fileName.c_str(), &statInfo)) == E_SUCCESS);
}

bool MediaPureFileUtils::DeleteDir(const std::string &dirName)
{
    bool errRet = false;

    if (IsDirectory(dirName)) {
        errRet = (RemoveDirectory(dirName) == E_SUCCESS);
    }

    return errRet;
}


bool MediaPureFileUtils::IsDirectory(const std::string &dirName, std::shared_ptr<int> errCodePtr)
{
    struct stat statInfo {};

    int32_t ret = stat(dirName.c_str(), &statInfo);
    if (ret == E_SUCCESS) {
        if (statInfo.st_mode & S_IFDIR) {
            return true;
        }
    } else if (errCodePtr != nullptr) {
        *errCodePtr = errno;
        MEDIA_ERR_LOG("ret: %{public}d, errno: %{public}d", ret, errno);
        return false;
    }
    MEDIA_ERR_LOG("ret: %{public}d, errno: %{public}d", ret, errno);
    return false;
}

int32_t UnlinkCb(const char *fpath, const struct stat *sb, int32_t typeflag, struct FTW *ftwbuf)
{
    CHECK_AND_RETURN_RET_LOG(fpath != nullptr, E_FAIL, "fpath == nullptr");
    int32_t errRet = remove(fpath);
    CHECK_AND_PRINT_LOG(!errRet, "Failed to remove errno: %{public}d, path: %{private}s", errno, fpath);
    return 0; // if delete fail, we need continue
}

int32_t MediaPureFileUtils::RemoveDirectory(const std::string &path)
{
    return nftw(path.c_str(), UnlinkCb, OPEN_FDS, FTW_DEPTH | FTW_PHYS);
}

bool MediaPureFileUtils::DeleteFile(const std::string &fileName)
{
    return (remove(fileName.c_str()) == E_SUCCESS);
}

void MediaPureFileUtils::PrintStatInformation(const std::string &path)
{
    struct stat statInfo {};
    if ((stat(path.c_str(), &statInfo)) == E_SUCCESS) {
        MEDIA_INFO_LOG("path:%{public}s uid:%{public}d, gid:%{public}d, mode:%{public}d",
            MediaPathUtils::DesensitizePath(path).c_str(), statInfo.st_uid, statInfo.st_gid, statInfo.st_mode);
    } else {
        MEDIA_INFO_LOG("path:%{public}s is not exist", MediaPathUtils::DesensitizePath(path).c_str());
    }
}

bool MediaPureFileUtils::Mkdir(const std::string &subStr, std::shared_ptr<int> errCodePtr)
{
    mode_t mask = umask(0);
    if (mkdir(subStr.c_str(), CHOWN_RWX_USR_GRP) == -1) {
        if (errCodePtr != nullptr) {
            *errCodePtr = errno;
        }
        int err = errno;
        MEDIA_ERR_LOG("Failed to create directory %{public}d, path:%{public}s", err,
            MediaPathUtils::DesensitizePath(subStr).c_str());
        if (err == EACCES) {
            PrintStatInformation(MediaPathUtils::GetParentPath(subStr));
        }
        umask(mask);
        return (err == EEXIST) ? true : false;
    }
    umask(mask);
    return true;
}

bool MediaPureFileUtils::CreateDirectory(const std::string &dirPath, std::shared_ptr<int> errCodePtr)
{
    std::string subStr;
    std::string segment;

    /*  Create directory and its sub directories if does not exist
     *  take each string after '/' create directory if does not exist.
     *  Created directory will be the base path for the next sub directory.
     */

    std::stringstream folderStream(dirPath);
    while (getline(folderStream, segment, '/')) {
        if (segment.empty()) {    // skip the first "/" in case of "/storage/cloud/files"
            continue;
        }

        subStr.append(SLASH_CHAR + segment);
        if (!IsDirectory(subStr, errCodePtr)) {
            CHECK_AND_RETURN_RET(Mkdir(subStr, errCodePtr), false);
        }
    }

    return true;
}

bool MediaPureFileUtils::CopyFileUtil(const std::string &filePath, const std::string &newPath, bool skipCheckPermit)
{
    struct stat fst{};
    bool errCode = false;
    if (filePath.size() >= PATH_MAX) {
        MEDIA_ERR_LOG("File path too long %{public}d", static_cast<int>(filePath.size()));
        return errCode;
    }
    MEDIA_DEBUG_LOG("File path is %{public}s", MediaPathUtils::DesensitizePath(filePath).c_str());
    std::string absFilePath;
    if (!PathToRealPath(filePath, absFilePath)) {
        MEDIA_ERR_LOG("file is not real path, file path: %{public}s",
            MediaPathUtils::DesensitizePath(filePath).c_str());
        return errCode;
    }
    if (absFilePath.empty()) {
        MEDIA_ERR_LOG("Failed to obtain the canonical path for source path:%{public}s %{public}d",
            MediaPathUtils::DesensitizePath(filePath).c_str(), errno);
        return errCode;
    }

    int32_t source = open(absFilePath.c_str(), O_RDONLY);
    ScopedFd srcFd(source);
    CHECK_AND_RETURN_RET_LOG(srcFd.Get() != E_ERR, errCode, "Open failed for source file, errno: %{public}d", errno);

    CHECK_AND_RETURN_RET_LOG(!newPath.empty(), errCode, "dest path is empty, safety check");
    int32_t dest = open(newPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, CHOWN_RO_USR_GRP);
    ScopedFd destFd(dest);
    CHECK_AND_RETURN_RET_LOG(destFd.Get() != E_ERR, errCode, "Open failed for destination file %{public}d", errno);

    CHECK_AND_RETURN_RET_LOG(fstat(srcFd.Get(), &fst) == E_SUCCESS, errCode,
        "Failed to fstat source file, errno: %{public}d", errno);

    off_t total_size = fst.st_size;
    off_t copied = 0;
    CHECK_AND_RETURN_RET_LOG(SendFileWithRetry(destFd.Get(), srcFd.Get(), total_size, copied), errCode,
        "Failed to copy content by sendfile");

    if (copied == total_size) {
        errCode = true;
        if (skipCheckPermit) {
            MEDIA_ERR_LOG("Open failed for destination file %{public}d", errno);
            errCode = true;
        }
    }

    return errCode;
}
} // namespace OHOS::Media
