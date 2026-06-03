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

#ifndef FRAMEWORKS_SERVICES_MEDIA_OPERATION_LOG_COLUMNS_H
#define FRAMEWORKS_SERVICES_MEDIA_OPERATION_LOG_COLUMNS_H

#include <set>
#include <string>
#include <unordered_map>

#include "base_column.h"
#include "rdb_predicates.h"
#include "userfile_manager_types.h"

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))
class TabOperationLogColumn : BaseColumn {
public:
    // TabOperationLogColumn name
    static const std::string TABLE EXPORT;
    static const std::string CREATE_TABLE EXPORT;

    static const std::string FILE_ID EXPORT;
    static const std::string TIMESTAMP EXPORT;
    static const std::string EVENT_TYPE EXPORT;
    static const std::string FILE_UIID EXPORT;
};
} // namespace OHOS::Media
#endif // FRAMEWORKS_SERVICES_MEDIA_OPERATION_LOG_COLUMNS_H