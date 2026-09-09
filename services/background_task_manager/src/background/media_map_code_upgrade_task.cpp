/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "Media_Background"

#include "media_map_code_upgrade_task.h"

#include "preferences.h"
#include "preferences_helper.h"

#include "medialibrary_unistore_manager.h"
#include "medialibrary_subscriber.h"
#include "medialibrary_restore.h"
#include "photo_map_code_operation.h"
#include "media_log.h"

using namespace OHOS::NativeRdb;

namespace OHOS::Media::Background {
const std::string MAP_CODE_AGING_XML = "/data/storage/el2/base/preferences/map_code_aging.xml";

bool MediaMapCodeUpgradeTask::Accept()
{
    return MedialibrarySubscriber::IsCurrentStatusOn();
}

void MediaMapCodeUpgradeTask::HandleMapCodeUpgrade()
{
    auto store = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(store != nullptr, "rdbStore is nullptr");

    PhotoMapCodeOperation::SetMapCodeReadyStatus(MAP_CODE_IS_NOT_READY);

    constexpr int32_t BATCH_SIZE = 500;
    constexpr int32_t MAX_PROCESS_COUNT = 1000;
    int32_t processedCount = 0;

    while (processedCount < MAX_PROCESS_COUNT) {
        if (!MedialibrarySubscriber::IsCurrentStatusOn()) {
            MEDIA_DEBUG_LOG("Map code task screen on, exit");
            break;
        }

        int32_t ret = PhotoMapCodeOperation::UpgradePendingPhotoMapCodes(store, BATCH_SIZE);
        if (ret < 0) {
            MEDIA_ERR_LOG("UpgradePendingPhotoMapCodes failed, ret: %{public}d", ret);
            break;
        }

        if (ret == 0) {
            MEDIA_INFO_LOG("Map code task completed, no more pending data");
            PhotoMapCodeOperation::SetMapCodeReadyStatus(MAP_CODE_IS_READY);
            break;
        }

        processedCount++;
        MEDIA_INFO_LOG("Map code processed batch: %{public}d, processedCount: %{public}d", ret, processedCount);
    }

    MEDIA_INFO_LOG("Map code task end, total batches: %{public}d", processedCount);
}

void MediaMapCodeUpgradeTask::Execute()
{
    MEDIA_INFO_LOG("MediaMapCodeUpgradeTask begin");
    CHECK_AND_RETURN_LOG(MedialibrarySubscriber::IsCurrentStatusOn(), "screen off, exit");

    HandleMapCodeUpgrade();
    MEDIA_INFO_LOG("MediaMapCodeUpgradeTask end");
}
} // namespace OHOS::Media::Background
