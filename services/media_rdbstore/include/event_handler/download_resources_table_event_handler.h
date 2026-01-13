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

#ifndef OHOS_MEDIA_DOWNLOAD_RESOURCES_TABLE_EVENT_HANDLER_H
#define OHOS_MEDIA_DOWNLOAD_RESOURCES_TABLE_EVENT_HANDLER_H

#include <string>

#include "rdb_store.h"
#include "i_media_rdb_open_event.h"
#include "download_resources_column.h"

namespace OHOS::Media {

class DownloadResourcesTableEventHandler : public IMediaRdbOpenEvent {
public:
    virtual ~DownloadResourcesTableEventHandler() = default;
    int32_t OnCreate(std::shared_ptr<MediaLibraryRdbStore> store) override;
    int32_t OnUpgrade(std::shared_ptr<MediaLibraryRdbStore> store, int32_t oldVersion, int32_t newVersion) override;
private:
    int32_t CreateTable(std::shared_ptr<MediaLibraryRdbStore> &store);
 
private:

    const std::string CREATE_TABLE_SQL = "\
        CREATE TABLE IF NOT EXISTS download_resources_task_records ( \
            file_id         INTEGER  PRIMARY KEY NOT NULL, \
            display_name       TEXT     NOT NULL DEFAULT \"\", \
            size       BIGINT NOT NULL DEFAULT -1, \
            uri        TEXT, \
            add_time      BIGINT NOT NULL DEFAULT -1, \
            finish_time     BIGINT NOT NULL DEFAULT -1, \
            download_status INT NOT NULL DEFAULT -1, \
            percent         INT NOT NULL DEFAULT -1, \
            auto_pause_reason INT NOT NULL DEFAULT 0, \
            cover_level INT NOT NULL DEFAULT 1 \
        );";
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_DOWNLOAD_RESOURCES_TABLE_EVENT_HANDLER_H