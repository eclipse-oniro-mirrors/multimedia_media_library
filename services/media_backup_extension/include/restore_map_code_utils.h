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

#ifndef RESTORE_MAP_CODE_UTILS_H
#define RESTORE_MAP_CODE_UTILS_H

#include "media_column.h"
#include "base_restore.h"
#include "backup_const.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <sys/stat.h>

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

class RestoreMapCodeUtils {
public:
    EXPORT static int32_t GetNotReadyPhotoCount(const std::shared_ptr<NativeRdb::RdbStore> store);
    EXPORT static int32_t ReverseFileInfosToMapCode(const std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
        const vector<FileInfo> &fileInfos);
    EXPORT static int32_t FileInfosToMapCode(const std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
        const vector<FileInfo> &fileInfos);
    EXPORT static int32_t FileInfoToMapCode(const FileInfo &fileInfo,
        const std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb);
   
    EXPORT static int64_t DeleteMapCodesByFileIds(const vector<string> &fileIds);
};
} // namespace Media
} // namespace OHOS

#endif // RESTORE_MAP_CODE_UTILS_H
