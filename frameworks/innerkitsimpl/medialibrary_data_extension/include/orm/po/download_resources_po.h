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

#ifndef OHOS_MEDIA_ORM_DOWNLOAD_RESOURCES_TASK_PO_H
#define OHOS_MEDIA_ORM_DOWNLOAD_RESOURCES_TASK_PO_H

#include <string>
#include <map>
#include <sstream>

namespace OHOS::Media::ORM {
class DownloadResourcesTaskPo {
public:
    std::optional<int32_t> fileId;       //  MediaColumn::MEDIA_ID;
    std::optional<std::string> fileName;
    std::optional<int64_t> fileSize;
    std::optional<std::string> fileUri;
    std::optional<int64_t> dateAdded;
    std::optional<int64_t> dateFinish;
    std::optional<int32_t> downloadStatus;
    std::optional<int32_t> percent;
    std::optional<int32_t> autoPauseReason;
    std::optional<int32_t> coverLevel;
    std::optional<int32_t> taskSeq;
    std::optional<int32_t> networkPolicy;

public:
    std::string ToString()
    {
        std::stringstream ss;
        ss << "{"
           << "\"fileId\": " << fileId.value_or(0) << ", "
           << "\"display_name\": " << fileName.value_or("") << ","
           << "\"size\": " << fileSize.value_or(-1) << ","
           << "\"added_time\": " << dateAdded.value_or(-1) << ","
           << "\"finish_time\": " << dateFinish.value_or(-1) << ","
           << "\"download_status\": " << downloadStatus.value_or(-1) << ","
           << "\"auto_pause_reason\": " << autoPauseReason.value_or(0) << ","
           << "\"coverLevel\": " << coverLevel.value_or(1) << ","
           << "\"percent\": " << percent.value_or(0) << ","
           << "\"taskSeq\": " << taskSeq.value_or(0) << ","
           << "\"networkPolicy\": " << networkPolicy.value_or(0) << "}";
        return ss.str();
    }
};
}  // namespace OHOS::Media::ORM
#endif  // OHOS_MEDIA_ORM_DOWNLOAD_RESOURCES_TASK_PO_H
