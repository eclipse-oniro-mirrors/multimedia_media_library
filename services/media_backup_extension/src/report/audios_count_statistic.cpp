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
#include "audios_count_statistic.h"

namespace OHOS::Media {
std::vector<AlbumMediaStatisticInfo> AudiosCountStatistic::Load()
{
    return {this->GetAudioCountStatInfo()};
}

AlbumMediaStatisticInfo AudiosCountStatistic::GetAudioCountStatInfo()
{
    AlbumMediaStatisticInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    info.albumName = "All_Restore_Audio";
    info.totalCount = this->audioCount_;
    return info;
}

}  // namespace OHOS::Media