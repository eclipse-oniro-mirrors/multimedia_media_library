/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_TAB_OLD_PHOTOS_EVENT_HANDLER_H
#define OHOS_MEDIA_TAB_OLD_PHOTOS_EVENT_HANDLER_H

#include <string>
#include <vector>

#include "rdb_store.h"
#include "rdb_open_event.h"

namespace OHOS::Media {
class TabOldPhotosTableEventHandler : public IRdbOpenEvent {
public:
    int32_t OnCreate(NativeRdb::RdbStore &store) override;
    int32_t OnUpgrade(NativeRdb::RdbStore &store, int oldVersion, int newVersion) override;

private:
    int32_t CreateTable(NativeRdb::RdbStore &store);
    int32_t CreateIndex(NativeRdb::RdbStore &store);
    int32_t CreateTrigger(NativeRdb::RdbStore &store);

private:
    const std::string TABLE_NAME = "tab_old_photos";
    const std::string CREATE_TABLE_SQL = "\
        CREATE TABLE IF NOT EXISTS tab_old_photos ( \
            file_id     INTEGER PRIMARY KEY, \
            data        TEXT, \
            old_file_id INTEGER, \
            old_data    TEXT, \
            clone_sequence INTEGER \
        );";
    const std::string TRIGGER_DELETE_CLEAR_TAB_OLD_PHOTOS = "\
        CREATE TRIGGER IF NOT EXISTS delete_clear_tab_photos \
            AFTER DELETE \
                ON Photos \
        BEGIN \
            DELETE FROM tab_old_photos \
                WHERE file_id = OLD.file_id; \
        END;";
    const std::string INDEX_OLD_FILE_ID = "\
        CREATE INDEX IF NOT EXISTS idx_old_file_id_on_tab_old_photos ON tab_old_photos ( \
            old_file_id \
        );";
    const std::string INDEX_OLD_DATA = "\
        CREATE INDEX IF NOT EXISTS idx_old_data_on_tab_old_photos ON tab_old_photos ( \
            old_data \
        );";
    const std::string INDEX_FILE_ID = "\
        CREATE INDEX IF NOT EXISTS idx_file_id_on_tab_old_photos ON tab_old_photos ( \
            file_id \
        );";
    const std::vector<std::string> CREATE_INDEX_SQLS = {INDEX_OLD_FILE_ID, INDEX_OLD_DATA, INDEX_FILE_ID};
};
} // namespace OHOS::Media
#endif // OHOS_MEDIA_TAB_OLD_PHOTOS_EVENT_HANDLER_H