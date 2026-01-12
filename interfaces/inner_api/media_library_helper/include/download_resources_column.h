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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_DOWNLOAD_RESOURCE_COLUMN_H_
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_DOWNLOAD_RESOURCE_COLUMN_H_

#include <set>
#include <string>
#include <unordered_map>

#include "base_column.h"
#include "rdb_predicates.h"
#include "userfile_manager_types.h"

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))
class DownloadResourcesColumn : BaseColumn {
public:
    // DownloadResourcesTask column name
    static const std::string MEDIA_ID EXPORT;
    static const std::string MEDIA_NAME EXPORT;
    static const std::string MEDIA_SIZE EXPORT;
    static const std::string MEDIA_URI EXPORT;
    static const std::string MEDIA_DATE_ADDED EXPORT;
    static const std::string MEDIA_DATE_FINISH EXPORT;
    static const std::string MEDIA_DOWNLOAD_STATUS EXPORT;
    static const std::string MEDIA_PERCENT EXPORT;
    static const std::string MEDIA_AUTO_PAUSE_REASON EXPORT;
    static const std::string MEDIA_COVER_LEVEL EXPORT;
    static const std::string TABLE EXPORT;
    static const std::string MEDIA_TASK_SEQ EXPORT;
    static const std::string MEDIA_NETWORK_POLICY EXPORT;
    static const std::string CREATE_TABLE EXPORT;

    // index in DownloadResources
    static const std::string IDSTATUS_INDEX EXPORT;

    static const std::string INDEX_DRTR_ID_STATUS EXPORT;
};
} // namespace OHOS::Media
#endif // INTERFACES_INNERKITS_NATIVE_INCLUDE_DOWNLOAD_RESOURCE_COLUMN_H_
