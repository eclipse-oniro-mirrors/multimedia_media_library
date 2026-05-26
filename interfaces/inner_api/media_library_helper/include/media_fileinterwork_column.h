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
 
#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_INTERWORK_COLUMN_H
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_INTERWORK_COLUMN_H
 
#include <string>
 
#include "base_column.h"
 
namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))
class MediaFileInterworkColumn {
public:
    // table name
    static const std::string OPT_TABLE_NAME EXPORT;
    // create table sql
    static const std::string CREATE_FILE_OPT_TABLE EXPORT;
 
    // column name
    static const std::string ID_COLUMN EXPORT;
    static const std::string OPT_COLUMN EXPORT;
    static const std::string BEFORE_PATH_COLUMN EXPORT;
    static const std::string AFTER_PATH_COLUMN EXPORT;
    static const std::string OPT_STATUS_COLUMN EXPORT;
 
    // systemparam
    static const std::string FILE_SYNC_STATUS_PARAM EXPORT;
 
    static const std::string FILE_ROOT_DIR EXPORT;
 
    static const std::string HO_DATA_DIR;
    static const std::string THUMBS_DIR;
    static const std::string RECENT_DIR;
    static const std::string BACKUP_DIR;
    static const std::string TRASH_DIR_DIR;
    static const std::string VM_DOCS_DIR;
    static const std::string OHPM_DIR;
    static const std::string PCE_ENGINE_DIR;
    static const std::string APPDATA_DIR;
};
} // namespace OHOS::Media
#endif // INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_INTERWORK_COLUMN_H
