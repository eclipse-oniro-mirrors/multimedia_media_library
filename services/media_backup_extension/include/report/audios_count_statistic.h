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
#ifndef OHOS_MEDIA_BACKUP_AUDIOS_COUNT_STATISITIC_H
#define OHOS_MEDIA_BACKUP_AUDIOS_COUNT_STATISITIC_H

#include <string>
#include <vector>

#include "media_backup_report_data_type.h"

namespace OHOS::Media {
class AudiosCountStatistic {
public:
    AudiosCountStatistic &SetSceneCode(int32_t sceneCode)
    {
        this->sceneCode_ = sceneCode;
        return *this;
    }
    AudiosCountStatistic &SetTaskId(const std::string &taskId)
    {
        this->taskId_ = taskId;
        return *this;
    }
    AudiosCountStatistic &SetAudioCount(const uint64_t audioCount)
    {
        this->audioCount_ = static_cast<int32_t>(audioCount);
        return *this;
    }
    std::vector<AlbumMediaStatisticInfo> Load();

private:
    AlbumMediaStatisticInfo GetAudioCountStatInfo();
private:
    int32_t sceneCode_;
    std::string taskId_;
    int32_t audioCount_ = 0;
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_BACKUP_AUDIOS_COUNT_STATISITIC_H