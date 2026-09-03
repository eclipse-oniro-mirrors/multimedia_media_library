/*
 * Copyright (C) 2025-2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_SLOW_MOTION_TRANSCODE_OPERATION_H
#define OHOS_MEDIA_SLOW_MOTION_TRANSCODE_OPERATION_H

#include <cstdint>
#include <string>
#include "result_set_utils.h"

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))
class EXPORT SlowMotionTranscodeOperation {
public:
    SlowMotionTranscodeOperation() = default;
    ~SlowMotionTranscodeOperation() = default;

    static int32_t ProcessSlowMotionTranscode(int32_t fileId, const std::string &requestId, int32_t &editTime);
private:
    static uint64_t GetInnerStorageFreeSize();
    static int32_t CheckSlowMotionDup(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
        int32_t fileId, int32_t &editTime, std::string &path, int32_t &duration);
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_SLOW_MOTION_TRANSCODE_OPERATION_H