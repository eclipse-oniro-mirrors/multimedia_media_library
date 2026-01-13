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

#ifndef MULTISTAGES_CAPTURE_NOTIFY_H
#define MULTISTAGES_CAPTURE_NOTIFY_H

#include <memory>
#include <string>

#include "camera_character_types.h"
#include "file_asset.h"

namespace OHOS {
namespace Media::Notification {
class MultistagesCaptureNotify {
#define EXPORT __attribute__ ((visibility ("default")))
public:
    EXPORT static int32_t NotifyOnProcess(
        const std::shared_ptr<FileAsset> &fileAsset, const MultistagesCaptureNotifyType &notifyType);
    static int32_t NotifyLowQualityMemoryCount();
};
} // namespace Media::Notification
} // namespace OHOS
#endif // MULTISTAGES_CAPTURE_NOTIFY_H