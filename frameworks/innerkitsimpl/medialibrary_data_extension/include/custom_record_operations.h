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

#ifndef OHOS_MEDIALIBRARY_CUSTOM_RECORD_OPERATIONS_H
#define OHOS_MEDIALIBRARY_CUSTOM_RECORD_OPERATIONS_H

#include "cloud_sync_manager.h"
#include "directory_ex.h"
#include "file_asset.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_command.h"
#include "rdb_utils.h"
#include "medialibrary_unistore_manager.h"

namespace OHOS::Media {

class CustomRecordOperations {
public:
    EXPORT static int32_t InsertCustomRecord(std::shared_ptr<MediaLibraryRdbStore> &rdbStore,
        MediaLibraryCommand &cmd);
    EXPORT static int32_t BatchAddCustomRecords(MediaLibraryCommand &cmd,
        const std::vector<DataShare::DataShareValuesBucket> &values);
};
} // namespace OHOS::Media

#endif // OHOS_MEDIALIBRARY_CUSTOM_RECORD_OPERATIONS_H