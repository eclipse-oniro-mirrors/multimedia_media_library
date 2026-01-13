/*
 * Copyright (C) 2024-2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_FUSE_MANAGER_H
#define OHOS_MEDIA_FUSE_MANAGER_H

#include <string>
#include <sys/stat.h>
#define FUSE_USE_VERSION FUSE_MAKE_VERSION(3, 17)
#include <fuse.h>
#include <atomic>
#include "media_fuse_low_daemon.h"

namespace OHOS {
namespace Media {
class MediaFuseHighDaemon;
class MediaFuseManager {
public:
    static MediaFuseManager &GetInstance();
    void Start();
    void Stop();
    int32_t DoGetAttr(const char *path, struct stat *stbuf);
    int32_t DoHdcGetAttr(const char *path, struct stat *stbuf, struct fuse_file_info *fi);
    int32_t DoOpen(const char *path, int flags, int &fd);
    int32_t DoHdcOpen(const char *path, int flags, int &fd);
    int32_t DoHdcCreate(const char *path, mode_t mode, struct fuse_file_info *fi);
    int32_t DoRelease(const char *path, const int &fd);
    int32_t DoHdcRelease(const char *path, const int32_t &fd);
    int32_t DoHdcUnlink(const char *path);
    int32_t DoHdcReadDir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
        enum fuse_readdir_flags flags);
    void SetUid(uid_t uid);
    uid_t GetUid();

private:
    MediaFuseManager() = default;
    ~MediaFuseManager() = default;

    int32_t MountFuse(std::string &mountpoint);
    int32_t UMountFuse();
    bool CheckDeviceInLinux();
    int32_t DoMedialibraryReadPermission(const std::string &fileId, const std::string &target, uid_t uid);

private:
    std::shared_ptr<MediaFuseLowDaemon> fuseLowDaemon_;
    std::shared_ptr<MediaFuseHighDaemon> fuseHighDaemon_;
    std::atomic<uid_t> uid_;
    bool isInLinux_;
};

class MediafusePermCheckInfo {
public:
    MediafusePermCheckInfo(const std::string &filePath, const std::string &mode, const std::string &fileId,
        const std::string &appId, const int32_t &uid);
    ~MediafusePermCheckInfo() = default;
    int32_t CheckPermission(uint32_t &tokenCaller);
private:
    std::string filePath_;
    std::string mode_;
    std::string fileId_;
    std::string appId_;
    int32_t uid_;
};
} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIA_FUSE_MANAGER_H