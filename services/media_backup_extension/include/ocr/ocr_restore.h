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

#ifndef OCR_RESTORE_H
#define OCR_RESTORE_H

#include <mutex>
#include <sstream>
#include <string>

#include "backup_const.h"
#include "nlohmann/json.hpp"
#include "rdb_store.h"

namespace OHOS::Media {

struct GalleryOCRInfo {
    PhotoInfo photoInfo;
    int fileIdOld;
    string hash;
    string ocrText;
    int ocrVersion;
    int width;
    int height;
};

class OCRRestore {
public:
    void Init(int32_t sceneCode, const std::string& taskId, std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
        std::shared_ptr<NativeRdb::RdbStore> galleryRdb);
    void RestoreOCR(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap, bool isCloudRestoreSatisfied);

private:
    void UpdateOcrInsertValues(std::vector<NativeRdb::ValuesBucket> &values, const GalleryOCRInfo &ocrInfo);
    void RestoreOCRInfos(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap, bool isCloudRestoreSatisfied);
    void RestoreOCRTotal(const vector<int32_t> &fileIds);

    int32_t BatchInsertWithRetry(
        const std::string &tableName, std::vector<NativeRdb::ValuesBucket> &values, int64_t &rowNum);

    int32_t sceneCode_{-1};
    std::string taskId_;
    std::shared_ptr<NativeRdb::RdbStore> galleryRdb_;
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb_;
};
}  // namespace OHOS::Media

#endif  // OCR_RESTORE_H