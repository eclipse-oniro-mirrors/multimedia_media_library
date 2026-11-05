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

#include <ftw.h>
#include <memory>
#include <sstream>
#include <sys/stat.h>

#include "media_log.h"
#include "medialibrary_errno.h"
#include "media_path_utils.h"

namespace OHOS::Media {
const int32_t OPEN_FDS = 64;
static const mode_t CHOWN_RWX_USR_GRP = 02771;
bool MediaPureFileUtils::IsFileExists(const std::string &fileName)
{
    struct stat statInfo {};
    return ((stat(fileName.c_str(), &statInfo)) == E_SUCCESS);
}

bool MediaPureFileUtils::DeleteDirectory(const std::string &dirName)
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

        subStr.append('/' + segment);
        if (!IsDirectory(subStr, errCodePtr)) {
            CHECK_AND_RETURN_RET(Mkdir(subStr, errCodePtr), false);
        }
    }

    return true;
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
            MediaPathUtils::PrintStatInformation(MediaPathUtils::GetParentPath(subStr));
        }
        umask(mask);
        return (err == EEXIST) ? true : false;
    }
    umask(mask);
    return true;
}
} // namespace OHOS::Media