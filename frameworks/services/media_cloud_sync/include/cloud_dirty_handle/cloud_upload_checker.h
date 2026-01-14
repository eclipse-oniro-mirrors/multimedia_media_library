/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_CLOUD_UPLOAD_CHECKER_H
#define OHOS_CLOUD_UPLOAD_CHECKER_H

#include <mutex>
#include <stdint.h>
#include <string>
#include <vector>

#include "rdb_predicates.h"

namespace OHOS {
namespace Media {
struct CheckedPhotoInfo {
    int32_t fileId;
    std::string path;
    int64_t size;
    int32_t subtype;
    int32_t movingPhotoEffectMode;
    int32_t fileSourceType;
};

class CloudUploadChecker {
public:
    static void RepairNoOriginPhoto();

private:
    static void HandleNoOriginPhoto();
    static std::vector<CheckedPhotoInfo> QueryPhotoInfo(int32_t startFiled);
    static void HandlePhotoInfos(const std::vector<CheckedPhotoInfo> &photoInfos, int32_t &curFileId);
    static void UpdateDirty(const std::vector<std::string> &idList, int32_t dirtyType);
    static int32_t QueryLcdPhotoCount(int32_t startFileId);
    static void UpdateFileSize(const CheckedPhotoInfo &photoInfo, bool isMovingPhoto);
    static void HandleMissingFile(
        const CheckedPhotoInfo &photoInfo, bool isMovingPhoto, std::vector<std::string> &noLcdList);
    static void FillDbFileSize(const CheckedPhotoInfo &photoInfo, NativeRdb::ValuesBucket &values);
    static void FillDbHeightAndWidth(const CheckedPhotoInfo &photoInfo, NativeRdb::ValuesBucket &values);
    static bool RepairPhotoByLcdUpdateDbColumn(const CheckedPhotoInfo &photoInfo);

private:
    static std::mutex mutex_;
};
}  // namespace Media
}  // namespace OHOS
#endif  // OHOS_CLOUD_UPLOAD_CHECKER_H
