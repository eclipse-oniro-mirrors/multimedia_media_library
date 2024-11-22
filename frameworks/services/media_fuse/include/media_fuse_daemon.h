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

#ifndef OHOS_MEDIA_FUSE_DAEMON_H
#define OHOS_MEDIA_FUSE_DAEMON_H

#include <string>

namespace OHOS {
namespace Media {
class MediaFuseDaemon {
public:
    explicit MediaFuseDaemon(const std::string &mountpoint)
        : mountpoint_(mountpoint) {}
    ~MediaFuseDaemon() = default;

    int32_t StartFuse();

private:
    void DaemonThread();

private:
    std::atomic<bool> isRunning_{false};
    std::string mountpoint_;
};
} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIA_FUSE_DAEMON_H

