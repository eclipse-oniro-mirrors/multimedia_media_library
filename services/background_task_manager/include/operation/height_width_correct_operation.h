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

#ifndef OHOS_MEDIA_HEIGHT_WIDTH_CORRECT_OPERATION_H
#define OHOS_MEDIA_HEIGHT_WIDTH_CORRECT_OPERATION_H

#include <string>
#include <vector>
#include <unordered_set>

#include "medialibrary_rdbstore.h"

namespace OHOS::Media {

struct CheckPhotoInfo {
    int32_t exifRotate;
    int32_t fileId;
    int32_t height;
    std::string lcdSize;
    int32_t mediaType;
    int32_t movingPhotoEffectMode;
    int32_t orientation;
    int32_t originalSubtype;
    std::string path;
    int32_t position;
    int32_t subtype;
    int32_t width;
};

class HeightWidthCorrectOperation {
public:
    static void UpdateHeightAndWidth();
    static void Stop();
    static int32_t QueryNoCheckPhotoCount(int32_t startFileId);
    static std::vector<CheckPhotoInfo> QueryNoCheckPhotoInfo(int32_t startFileId);
    static std::vector<CheckPhotoInfo> QueryCheckFailPhotoInfo(std::vector<int32_t> &failIds);
    static void HandlePhotoInfos(const std::vector<CheckPhotoInfo> &photoInfos, int32_t &curFileId,
        std::unordered_set<int32_t> &failedIds, int32_t &count);
    static bool UpdatePhotoHeightWidth(const CheckPhotoInfo &photoInfo);
    static void RemoveInvalidFromFailIds(std::unordered_set<int32_t> &failIds);

private:
    static std::atomic<bool> isContinue_;
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_HEIGHT_WIDTH_CORRECT_OPERATION_H