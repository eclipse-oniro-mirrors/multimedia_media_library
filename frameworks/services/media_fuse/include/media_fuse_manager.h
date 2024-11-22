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

#ifndef OHOS_MEDIA_FUSE_MANAGER_H
#define OHOS_MEDIA_FUSE_MANAGER_H

#include <string>
#include <sys/stat.h>

namespace OHOS {
namespace Media {
class MediaFuseDaemon;

class MediaFuseManager {
public:
    static MediaFuseManager &GetInstance();
    void Start();
    void Stop();
    int32_t DoGetAttr(const char *path, struct stat *stbuf);
    int32_t DoOpen(const char *path, int flags, int &fd);
    int32_t DoRelease(const char *path, const int &fd);
private:
    MediaFuseManager() = default;
    ~MediaFuseManager() = default;

    int32_t MountFuse(std::string &mountpoint);
    int32_t UMountFuse();

private:
    std::shared_ptr<MediaFuseDaemon> fuseDaemon_;
};
} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIA_FUSE_MANAGER_H

