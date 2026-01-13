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
#define MLOG_TAG "Media_Upgrade"

#include "table_event_handler.h"

#include <string>
#include <vector>

#include "media_log.h"
#include "media_file_utils.h"
#include "photo_map_table_event_handler.h"
#include "download_resources_table_event_handler.h"

namespace OHOS::Media {
TableEventHandler::TableEventHandler()
{
    this->handlers_ = {
        std::make_shared<PhotoMapTableEventHandler>(),
        std::make_shared<DownloadResourcesTableEventHandler>(),
    };
}

int32_t TableEventHandler::OnCreate(std::shared_ptr<MediaLibraryRdbStore> store)
{
    MEDIA_INFO_LOG("TableEventHandler OnCreate, in");
    CHECK_AND_RETURN_RET_LOG(store != nullptr, E_OK, "store is null");
    int32_t ret = E_OK;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    for (auto handler : this->handlers_) {
        CHECK_AND_CONTINUE_ERR_LOG(handler != nullptr, "handler is null");
        ret = handler->OnCreate(store);
        CHECK_AND_PRINT_LOG(ret == 0, "handler OnCreate failed");
    }
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    MEDIA_INFO_LOG("OnCreate, cost %{public}" PRId64 "ms.", costTime);
    return E_OK;
}

int32_t TableEventHandler::OnUpgrade(
    std::shared_ptr<MediaLibraryRdbStore> store, int32_t oldVersion, int32_t newVersion)
{
    CHECK_AND_RETURN_RET_LOG(store != nullptr, E_OK, "store is null");
    int32_t ret = E_OK;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    for (auto handler : this->handlers_) {
        CHECK_AND_CONTINUE_ERR_LOG(handler != nullptr, "handler is null");
        ret = handler->OnUpgrade(store, oldVersion, newVersion);
        CHECK_AND_PRINT_LOG(ret == 0, "handler OnUpgrade failed");
    }
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    MEDIA_INFO_LOG("OnUpgrade, cost %{public}" PRId64 "ms.", costTime);
    return E_OK;
}
}  // namespace OHOS::Media