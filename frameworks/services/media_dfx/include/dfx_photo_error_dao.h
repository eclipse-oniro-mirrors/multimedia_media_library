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

#ifndef OHOS_MEDIA_DFX_PHOTO_ERROR_DAO_H
#define OHOS_MEDIA_DFX_PHOTO_ERROR_DAO_H

#include <cstdint>
#include <string>
#include <vector>

namespace OHOS {
namespace Media {

struct PhotoErrorRow {
    int32_t fileId{0};
    std::string data;                    // MEDIA_FILE_PATH
    std::string storagePath;             // PHOTO_STORAGE_PATH
    int32_t fileSourceType{0};
    int32_t mediaType{0};
    int32_t southDeviceType{0};
    int32_t position{0};
    int64_t size{0};                     // MEDIA_SIZE（DB 侧比对值）
    int32_t subtype{0};
    int32_t movingPhotoEffectMode{0};
    int32_t originalSubtype{0};
    std::string ToString() const;
};

class DfxPhotoErrorDao {
public:
    DfxPhotoErrorDao() = default;
    ~DfxPhotoErrorDao() = default;
    std::vector<PhotoErrorRow> QueryBatch(int32_t lastFileId, int32_t limit);
};
} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIA_DFX_PHOTO_ERROR_DAO_H
