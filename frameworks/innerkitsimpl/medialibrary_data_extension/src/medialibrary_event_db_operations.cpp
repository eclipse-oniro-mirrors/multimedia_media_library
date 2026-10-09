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

#include "medialibrary_event_db_operations.h"
#include <uuid.h>
#include "media_operation_log_column.h"

#include <string>

#include "rdb_store.h"
#include "rdb_errno.h"
#include "media_log.h"
#include "medialibrary_unistore_manager.h"

namespace OHOS::Media {
std::vector<MedialibraryEventDbOperations::OperationRecord> MedialibraryEventDbOperations::GetEvents(
    const EventQuery& query)
{
    return {};
}

std::string MedialibraryEventDbOperations::EventTypeToString(EventType type)
{
    switch (type) {
        case EventType::ASSET_ADD:           return "ASSET_ADD";
        case EventType::ASSET_UPDATE_META:   return "ASSET_UPDATE_META";
        case EventType::ASSET_UPDATE_FILE:   return "ASSET_UPDATE_FILE";
        case EventType::ASSET_HIDDEN:        return "ASSET_HIDDEN";
        case EventType::ASSET_DELETE:        return "ASSET_DELETE";
        case EventType::ASSET_MOVE:          return "ASSET_MOVE";
        case EventType::ALBUM_ADD:           return "ALBUM_ADD";
        case EventType::ALBUM_UPDATE:        return "ALBUM_UPDATE";
        case EventType::ALBUM_DELETE:        return "ALBUM_DELETE";
        default:                             return "UNKNOWN";
    }
}

void MedialibraryEventDbOperations::CreateEvent(EventType type, const std::string& unique_id)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "Medialibrary rdbStore is nullptr!");

    const std::string typeStr = EventTypeToString(type);

    std::string sql = "INSERT INTO " + TabOperationLogColumn::TABLE + " (" +
        TabOperationLogColumn::EVENT_TYPE + ", " + TabOperationLogColumn::FILE_UIID + ") VALUES ('" +
        typeStr + "', '" + unique_id + "')";
    int32_t ret = rdbStore->ExecuteSql(sql);
    CHECK_AND_RETURN_LOG(ret == NativeRdb::E_OK,
        "Failed to execute sql, photoId is %{public}s, error code is %{public}d", unique_id.c_str(), ret);
}

void MedialibraryEventDbOperations::UpdateEvent(EventType type, const std::string& unique_id)
{
}

void MedialibraryEventDbOperations::DeleteEvent(EventType type, const std::string& unique_id)
{
}

std::string MedialibraryEventDbOperations::GenerateUuid()
{
    uuid_t uuid;
    uuid_generate(uuid);
    char str[UUID_STR_LENGTH] = {};
    uuid_unparse(uuid, str);
    return str;
}
} // namespace OHOS::Media
