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

#include "custom_records_column.h"

namespace OHOS::Media {
const std::string CustomRecordsColumns::FILE_ID = "file_id";
const std::string CustomRecordsColumns::BUNDLE_NAME = "bundle_name";
const std::string CustomRecordsColumns::SHARE_COUNT = "share_count";
const std::string CustomRecordsColumns::LCD_JUMP_COUNT = "lcd_jump_count";

const std::string CustomRecordsColumns::TABLE = "tab_custom_records";

const std::string CustomRecordsColumns::CREATE_TABLE = CreateTable() +
    TABLE + " (" +
    FILE_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    BUNDLE_NAME + " TEXT, " +
    SHARE_COUNT + " INT DEFAULT 0, " +
    LCD_JUMP_COUNT + " INT DEFAULT 0)";

const std::string CustomRecordsColumns::CUSTOM_RECORDS_URI_PREFIX = "file://media/custom_records";
} // namespace OHOS::MEDIA
