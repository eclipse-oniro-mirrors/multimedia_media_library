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

#ifndef OHOS_MEDIA_BACKGROUND_MEDIA_FIX_DUPLICATE_UNIQUE_ID_TASK_H
#define OHOS_MEDIA_BACKGROUND_MEDIA_FIX_DUPLICATE_UNIQUE_ID_TASK_H

#include <string>
#include <vector>

#include "i_media_background_task.h"

namespace OHOS::Media::Background {
#define EXPORT __attribute__ ((visibility ("default")))

struct DuplicateRecordInfo {
    int32_t fileId = -1;
    int64_t dateModified = 0;
};

class EXPORT MediaFixDuplicateUniqueIdTask : public IMediaBackGroundTask {
public:
    virtual ~MediaFixDuplicateUniqueIdTask() = default;

public:
    bool Accept() override;
    void Execute() override;

private:
    void HandleFixDuplicateUniqueId();
    std::vector<std::string> FindDuplicateUniqueIds();
    int32_t ProcessDuplicateGroup(const std::string &uniqueId);
    std::vector<DuplicateRecordInfo> QueryDuplicateRecords(const std::string &uniqueId);
    int32_t UpdateRecordUniqueId(int32_t fileId, const std::string &newUniqueId);
};

}  // namespace OHOS::Media::Background
#endif  // OHOS_MEDIA_BACKGROUND_MEDIA_FIX_DUPLICATE_UNIQUE_ID_TASK_H
