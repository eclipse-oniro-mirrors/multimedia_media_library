/*
 Copyright (C) 2025 Huawei Device Co., Ltd.
 Licensed under the Apache License, Version 2.0 (the "License");
 You may not use this file except in compliance with the license.
 You may obtain a copy of the license at
 *
 * http: //www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the license is distributed on an "AS IS" BASIS,
 Without WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the Specific Language Governing permissions and
 * Limitations under the license.
 */

#ifndef OHOS_MEDIA_TAB_CLONED_OLD_PHOTOS_TABLE_EVENT_HANDLER_H
#define OHOS_MEDIA_TAB_CLONED_OLD_PHOTOS_TABLE_EVENT_HANDLER_H
 
#include <string>
#include <vector>
 
#include "rdb_store.h"
#include "rdb_open_event.h"
 
namespace OHOS::Media {
class TabClonedOldPhotosTableEventHandler : public IRdbOpenEvent {
public:
    int32_t OnCreate(NativeRdb::RdbStore& store) override;
    int32_t OnUpgrade(NativeRdb::RdbStore& store, int oldVersion, int newVersion) override;

private:
    int32_t CreateTable(NativeRdb::RdbStore& store);

private:
    const std::string TABLE_NAME = "tab_cloned_old_photos";
    const std::string CREATE_TABLE_SQL = "\
         CREATE TABLE IF NOT EXISTS tab_cloned_old_photos ( \
            file_id INTEGER PRIMARY KEY, \
            data TEXT, \
            old_file_id INTEGER, \
            old_data TEXT, \
            clone_sequence INTEGER \
         );";
};
} // namespace OHOS::Media
#endif // OHOS_MEDIA_TAB_CLONED_OLD_PHOTOS_TABLE_EVENT_HANDLER_H