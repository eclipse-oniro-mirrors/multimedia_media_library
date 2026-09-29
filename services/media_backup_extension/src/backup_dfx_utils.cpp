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

#define MLOG_TAG "MediaLibraryBackupUtils"

#include "backup_dfx_utils.h"

#include "hisysevent.h"
#include "media_log.h"

namespace OHOS {
namespace Media {
static constexpr char MEDIA_LIBRARY[] = "MEDIALIBRARY";
const std::string KEY_ALBUM_COUNT = "ALBUM_COUNT";
const std::string KEY_PHOTO_COUNT = "PHOTO_COUNT";
const std::string KEY_FACE_COUNT = "FACE_COUNT";
const std::string KEY_TOTAL_TIME_COST = "TOTAL_TIME_COST";

void BackupDfxUtils::PostPortraitStat(uint32_t albumCount, uint64_t photoCount, uint64_t faceCount,
    uint64_t totalTimeCost)
{
    int32_t ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_BACKUP_PORTRAIT_STAT",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        KEY_ALBUM_COUNT, albumCount,
        KEY_PHOTO_COUNT, photoCount,
        KEY_FACE_COUNT, faceCount,
        KEY_TOTAL_TIME_COST, totalTimeCost);
    CHECK_AND_PRINT_LOG(ret==0, "PostPortraitStat error:%{public}d", ret);
}
} // namespace Media
} // namespace OHOS