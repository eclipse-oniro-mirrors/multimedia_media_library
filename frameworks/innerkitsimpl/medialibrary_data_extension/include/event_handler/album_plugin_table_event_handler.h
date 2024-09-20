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

#ifndef OHOS_MEDIA_ALBUM_PLUGIN_TABLE_EVENT_HANDLER_H
#define OHOS_MEDIA_ALBUM_PLUGIN_TABLE_EVENT_HANDLER_H

#include <string>

#include "rdb_store.h"
#include "rdb_open_event.h"

namespace OHOS::Media {

class AlbumPluginTableEventHandler : public IRdbOpenEvent {
public:
    int32_t OnCreate(NativeRdb::RdbStore &store) override;
    int32_t OnUpgrade(NativeRdb::RdbStore &store, int oldVersion, int newVersion) override;

private:
    int32_t InitiateData(NativeRdb::RdbStore &store);
    bool IsTableCreated(NativeRdb::RdbStore &store, const std::string &tableName);

private:
    const std::string TABLE_NAME = "album_plugin";
    const std::string CREATE_TABLE_SQL = "\
        CREATE TABLE IF NOT EXISTS album_plugin \
        ( \
            lpath TEXT, \
            album_name TEXT, \
            album_name_en TEXT, \
            bundle_name TEXT, \
            cloud_id TEXT, \
            dual_album_name TEXT, \
            priority INT \
        );";
    const std::string DROP_TABLE_SQL = "DROP TABLE IF EXISTS album_plugin;";
    const std::string INSERT_DATA_SQL = "\
        INSERT INTO album_plugin( \
                lpath, \
                album_name, \
                album_name_en, \
                bundle_name, \
                cloud_id, \
                dual_album_name, \
                priority \
        ) VALUES (?, ?, ?, ?, ?, ?, ?);";
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_ALBUM_PLUGIN_TABLE_EVENT_HANDLER_H