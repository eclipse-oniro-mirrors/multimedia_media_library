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
 
#include "media_fileinterwork_column.h"
 
namespace OHOS::Media {
using namespace std;
 
// MediaFileInterworkColumn table
const std::string MediaFileInterworkColumn::OPT_TABLE_NAME = "tab_file_opt";
const std::string MediaFileInterworkColumn::ID_COLUMN = "id";
const std::string MediaFileInterworkColumn::OPT_COLUMN = "opt";
const std::string MediaFileInterworkColumn::BEFORE_PATH_COLUMN = "before_path";
const std::string MediaFileInterworkColumn::AFTER_PATH_COLUMN = "after_path";
const std::string MediaFileInterworkColumn::OPT_STATUS_COLUMN = "status";
 
// Create tables
const string MediaFileInterworkColumn::CREATE_FILE_OPT_TABLE = "CREATE TABLE IF NOT EXISTS tab_file_opt (" +
    ID_COLUMN + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    OPT_COLUMN + " INTEGER, " +
    BEFORE_PATH_COLUMN + " TEXT, " +
    AFTER_PATH_COLUMN + " TEXT, " +
    OPT_STATUS_COLUMN + " INTEGER)";
 
const std::string MediaFileInterworkColumn::FILE_ROOT_DIR = "/storage/media/local/files/Docs";
const std::string MediaFileInterworkColumn::HO_DATA_DIR = "/HO_DATA_EXT_MISC";
const std::string MediaFileInterworkColumn::THUMBS_DIR = "/.thumbs";
const std::string MediaFileInterworkColumn::RECENT_DIR = "/.Recent";
const std::string MediaFileInterworkColumn::BACKUP_DIR = "/.backup";
const std::string MediaFileInterworkColumn::TRASH_DIR_DIR = "/.Trash";
const std::string MediaFileInterworkColumn::VM_DOCS_DIR = "/.VMDocs";
const std::string MediaFileInterworkColumn::OHPM_DIR = "/.ohpm";
const std::string MediaFileInterworkColumn::PCE_ENGINE_DIR = "/PCEngine";
const std::string MediaFileInterworkColumn::APPDATA_DIR = "/appdata";
} // namespace OHOS::Media