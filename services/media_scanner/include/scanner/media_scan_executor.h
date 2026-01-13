/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#ifndef MEDIA_SCAN_EXECUTOR_H
#define MEDIA_SCAN_EXECUTOR_H

#include <string>
#include <iostream>
#include <queue>
#include <memory>

#include "media_scanner.h"

namespace OHOS {
namespace Media {
class MediaScanExecutor {
public:
    MediaScanExecutor() = default;
    virtual ~MediaScanExecutor() = default;

    int32_t Commit(std::unique_ptr<MediaScannerObj> scanner);

    void Start();
    void Stop();

private:
    void HandleScanExecution();

    const size_t MAX_THREAD = 4;
    size_t activeThread_ = 0;

    std::queue<std::unique_ptr<MediaScannerObj>> queue_;
    std::mutex queueMutex_;

    std::shared_ptr<bool> stopFlag_ = make_shared<bool>(false);
    int32_t sleepTime_ = 200;
};
} // namespace Media
} // namespace OHOS

#endif /* MEDIA_SCAN_EXECUTOR_H */
