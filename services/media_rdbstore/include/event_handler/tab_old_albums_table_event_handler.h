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

#ifndef OHOS_MEDIA_TAB_OLD_ALBUM_EVENT_HANDLER_H
#define OHOS_MEDIA_TAB_OLD_ALBUM_EVENT_HANDLER_H

#include <string>
#include <vector>

#include "rdb_store.h"
#include "rdb_open_event.h"

namespace OHOS::Media {
class TabOldAlbumTableEventHandler : public IRdbOpenEvent {
public:
    int32_t OnCreate(NativeRdb::RdbStore &store) override;
    int32_t OnUpgrade(NativeRdb::RdbStore &store, int oldVersion, int newVersion) override;

private:
    int32_t CreateTable(NativeRdb::RdbStore &store);

private:
    const std::string TABLE_NAME = "tab_old_albums";
    const std::string CREATE_TABLE_SQL = "\
        CREATE TABLE IF NOT EXISTS tab_old_albums ( \
            id              INTEGER PRIMARY KEY AUTOINCREMENT, \
            old_album_id   INTEGER NOT NULL, \
            album_id       INTEGER NOT NULL, \
            album_type     INTEGER NOT NULL, \
            album_subtype  INTEGER NOT NULL, \
            clone_sequence  INTEGER NOT NULL \
        );";
};
} // namespace OHOS::Media
#endif // OHOS_MEDIA_TAB_OLD_ALBUM_EVENT_HANDLER_H