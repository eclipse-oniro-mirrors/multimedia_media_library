/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_BACKGROUND_MEDIA_CLOUD_SYNC_BACKGROUND_TASK_H
#define OHOS_MEDIA_BACKGROUND_MEDIA_CLOUD_SYNC_BACKGROUND_TASK_H

#include <vector>

#include "i_media_background_task.h"
#include "media_hidden_and_recycle_task.h"

namespace OHOS::Media::Background {
class MediaCloudSyncBackgroundTask : public IMediaBackGroundTask {
public:
    MediaCloudSyncBackgroundTask();
    virtual ~MediaCloudSyncBackgroundTask() = default;

public:
    bool Accept() override;
    void Execute() override;

private:
    std::vector<std::shared_ptr<IMediaBackGroundTask>> tasks_;
};
}  // namespace OHOS::Media::Background
#endif  // OHOS_MEDIA_BACKGROUND_MEDIA_CLOUD_SYNC_BACKGROUND_TASK_H