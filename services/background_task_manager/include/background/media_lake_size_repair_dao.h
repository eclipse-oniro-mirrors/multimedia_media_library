/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_BACKGROUND_MEDIA_LAKE_SIZE_REPAIR_DAO_H
#define OHOS_MEDIA_BACKGROUND_MEDIA_LAKE_SIZE_REPAIR_DAO_H

#include <string>
#include <vector>

namespace OHOS::Media::Background {

struct LakeSizeRepairRecord {
    int32_t fileId{0};
    std::string storagePath;
};

class MediaLakeSizeRepairDao {
public:
    std::vector<LakeSizeRepairRecord> QueryCorruptedLakePhotos(int32_t lastFileId, int32_t limit);
};

}  // namespace OHOS::Media::Background

#endif  // OHOS_MEDIA_BACKGROUND_MEDIA_LAKE_SIZE_REPAIR_DAO_H
