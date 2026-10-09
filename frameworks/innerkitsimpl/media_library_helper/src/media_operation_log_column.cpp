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

#include "media_operation_log_column.h"

#include <string>
#include <unordered_map>
#include "media_log.h"
#include "medialibrary_type_const.h"

namespace OHOS::Media {
using namespace std;
using namespace NativeRdb;

// TabOperationLogColumn table
const string TabOperationLogColumn::FILE_ID = "id";
const string TabOperationLogColumn::TIMESTAMP = "time";
const string TabOperationLogColumn::EVENT_TYPE = "type";
const string TabOperationLogColumn::FILE_UIID = "unique_id";

const std::string TabOperationLogColumn::TABLE = "tab_operation_log";

const std::string TabOperationLogColumn::CREATE_TABLE = "\
    CREATE TABLE IF NOT EXISTS tab_operation_log ( \
        id              INTEGER PRIMARY KEY AUTOINCREMENT, \
        time            BIGINT NOT NULL DEFAULT (strftime('%s','now')), \
        type            TEXT, \
        unique_id       TEXT  \
    );";
} // namespace OHOS::MEDIA