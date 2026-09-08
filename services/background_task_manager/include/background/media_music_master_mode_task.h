/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_BACKGROUND_MEDIA_MUSIC_MASTER_MODE_TASK_H
#define OHOS_MEDIA_BACKGROUND_MEDIA_MUSIC_MASTER_MODE_TASK_H

#include <string>
#include <vector>

#include "i_media_background_task.h"

namespace OHOS::Media::Background {

struct MusicMasterAssetInfo {
    int32_t fileId = 0;
    int32_t fileSourceType = 0;
    std::string path;
    std::string storagePath;
};

class MediaMusicMasterModeTask : public IMediaBackGroundTask {
public:
    virtual ~MediaMusicMasterModeTask() = default;

public:
    bool Accept() override;
    void Execute() override;

private:
    void HandleMusicMasterMode();
    void QueryMusicMasterAssets(int32_t startFileId, int32_t maxFileId,
        std::vector<MusicMasterAssetInfo> &assetInfos);
    void HandleMusicMasterAssets(const std::vector<MusicMasterAssetInfo> &assetInfos);
    int32_t GetBatchStatus();
    void SetBatchStatus(int32_t startFileId);
};

}  // namespace OHOS::Media::Background
#endif  // OHOS_MEDIA_BACKGROUND_MEDIA_MUSIC_MASTER_MODE_TASK_H
