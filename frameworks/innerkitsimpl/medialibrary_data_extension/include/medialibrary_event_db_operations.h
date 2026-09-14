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

#ifndef MEDIALIBRARY_EVENT_DB_OPERATIONS_H
#define MEDIALIBRARY_EVENT_DB_OPERATIONS_H
 
#include "parameter.h"
#include "parameters.h"

namespace OHOS {
namespace Media {
  
class MedialibraryEventDbOperations {
public:
    enum class EventType {
        ASSET_ADD = 1,
        ASSET_UPDATE_META = 2,
        ASSET_UPDATE_FILE = 3,
        ASSET_HIDDEN = 4,
        ASSET_DELETE = 5,
        ASSET_MOVE = 6,
        ALBUM_ADD = 11,
        ALBUM_UPDATE = 12,
        ALBUM_DELETE = 13,
    };
    struct OperationRecord {
        int id;
        long timestamp;
        EventType type;
        std::string unique_id;
    };
    struct EventQuery {
        std::optional<int> id;
        std::optional<std::string> unique_id;
        std::vector<EventType> types;
    };
    std::vector<OperationRecord> GetEvents(const EventQuery& query);
    static void CreateEvent(EventType type, const std::string& unique_id);
    void UpdateEvent(EventType type, const std::string& unique_id);
    void DeleteEvent(EventType type, const std::string& unique_id);
    static std::string GenerateUuid();
private:
    enum { UUID_STR_LENGTH = 37 };
    static std::string EventTypeToString(EventType type);
};
} // namespace Media
} // namespace OHOS
#endif // MEDIALIBRARY_EVENT_DB_OPERATIONS_H