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
 
#ifndef BATCH_RESTORE_UTILS_H
#define BATCH_RESTORE_UTILS_H
 
#include <string>
#include <unordered_map>
#include <vector>
 
#include "batch_restore_types.h"
#include "metadata.h"
#include "scan_config.h"
#include "values_bucket.h"
 
namespace OHOS {
namespace Media {
 
class BatchRestoreUtils {
public:
    static std::vector<RestoreFileInfo> GetFileInfos(const std::vector<std::string> &filePathVector,
        UniqueNumber &uniqueNumber);
    static bool IsDuplication(const BatchScanInfo &config,
        const std::unordered_set<std::string> &photoCache, RestoreFileInfo &fileInfo);
    static int32_t FillMetadata(const std::unordered_map<std::string, TimeInfo> &timeInfoMap,
        const RestoreFileInfo &fileInfo, std::unique_ptr<Metadata> &data);
    static int32_t GetFileMetadata(std::unique_ptr<Metadata> &data);
    static void SetTimeInfo(const std::unique_ptr<Metadata> &data, RestoreFileInfo &info,
        NativeRdb::ValuesBucket &value);
};
 
} // namespace Media
} // namespace OHOS
 
#endif // BATCH_RESTORE_UTILS_H