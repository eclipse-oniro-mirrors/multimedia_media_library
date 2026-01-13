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

#ifndef OHOS_MEDIA_PHOTO_MAP_TABLE_EVENT_HANDLER_H
#define OHOS_MEDIA_PHOTO_MAP_TABLE_EVENT_HANDLER_H

#include <string>

#include "rdb_store.h"
#include "i_media_rdb_open_event.h"
#include "database_utils.h"
#include "photo_map_column.h"

namespace OHOS::Media {
static const std::string SQL_DROP_NEW_TRIGGER = "DROP TRIGGER IF EXISTS album_map_insert_cloud_sync_trigger;";
static const std::string SQL_DROP_DELETE_TRIGGER = "DROP TRIGGER IF EXISTS album_map_delete_trigger;";
static const std::string SQL_DROP_INSERT_SEARCH_TRIGGER = "DROP TRIGGER IF EXISTS album_map_insert_search_trigger;";
static const std::string SQL_DROP_DELETE_SEARCH_TRIGGER = "DROP TRIGGER IF EXISTS album_map_delete_search_trigger;";

class PhotoMapTableEventHandler : public IMediaRdbOpenEvent {
public:
    virtual ~PhotoMapTableEventHandler() = default;

public:
    int32_t OnCreate(std::shared_ptr<MediaLibraryRdbStore> store) override;
    int32_t OnUpgrade(std::shared_ptr<MediaLibraryRdbStore> store, int32_t oldVersion, int32_t newVersion) override;

private:
    using UpgradeHandle = int32_t (PhotoMapTableEventHandler::*)(
        std::shared_ptr<MediaLibraryRdbStore>, int32_t, int32_t);
    int32_t DropAllTriggers(std::shared_ptr<MediaLibraryRdbStore> store, int32_t oldVersion, int32_t newVersion);

private:
    const std::vector<UpgradeHandle> upgradeHandles_ = {
        &PhotoMapTableEventHandler::DropAllTriggers,
    };

private:
    DatabaseUtils dbUpgradeUtils_;
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_PHOTO_MAP_TABLE_EVENT_HANDLER_H