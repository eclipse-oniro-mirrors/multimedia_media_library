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

#define MLOG_TAG "LakeSizeRepair"

#include "media_lake_size_repair_task.h"

#include <cinttypes>
#include <vector>

#include "global_scanner.h"
#include "lake_file_scanner.h"
#include "media_file_notify_info.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_subscriber.h"
#include "preferences.h"
#include "preferences_helper.h"

namespace OHOS::Media::Background {
namespace {
constexpr char LAKE_SIZE_REPAIR_PREFERENCES[] = "/data/storage/el2/base/preferences/task_progress.xml";
constexpr int32_t LAKE_SIZE_REPAIR_VERSION = 1;
constexpr int32_t BATCH_SIZE = 100;
const std::string KEY_TASK_VERSION = "lake_size_repair_version";
const std::string KEY_REPAIR_CURSOR = "lake_size_repair_cursor";

std::shared_ptr<NativePreferences::Preferences> GetRepairPrefs()
{
    int32_t errCode = E_OK;
    auto prefs = NativePreferences::PreferencesHelper::GetPreferences(LAKE_SIZE_REPAIR_PREFERENCES, errCode);
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, nullptr, "Get preferences failed, errCode: %{public}d", errCode);
    return prefs;
}

bool SaveIntPref(const std::string &key, int32_t value)
{
    auto prefs = GetRepairPrefs();
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, false, "prefs is nullptr, key=%{public}s, value=%{public}d",
        key.c_str(), value);
    prefs->PutInt(key, value);
    prefs->FlushSync();
    return true;
}
}  // namespace

bool MediaLakeSizeRepairTask::Accept()
{
    return MedialibrarySubscriber::IsCurrentStatusOn();
}

void MediaLakeSizeRepairTask::Execute()
{
    PrepareProgress();
    RepairCorruptedPhotos();
}

void MediaLakeSizeRepairTask::PrepareProgress()
{
    CHECK_AND_RETURN(GetSavedTaskVersion() != LAKE_SIZE_REPAIR_VERSION);
    SaveCursor(KEY_REPAIR_CURSOR, 0);
    SaveTaskVersion(LAKE_SIZE_REPAIR_VERSION);
    MEDIA_INFO_LOG("Lake size repair version advanced to %{public}d, cursor reset", LAKE_SIZE_REPAIR_VERSION);
}

void MediaLakeSizeRepairTask::RepairCorruptedPhotos()
{
    if (GlobalScanner::GetInstance().GetScannerStatus() != ScannerStatus::IDLE) {
        MEDIA_INFO_LOG("GlobalScanner not IDLE, skip lake size repair this round");
        return;
    }
    int32_t lastFileId = LoadCursor(KEY_REPAIR_CURSOR);
    const int32_t startCursor = lastFileId;
    int64_t processedCount = 0;
    while (Accept()) {
        auto photos = repairDao_.QueryCorruptedLakePhotos(lastFileId, BATCH_SIZE);
        CHECK_AND_BREAK(!photos.empty());

        const int32_t batchLastId = photos.back().fileId;
        std::vector<MediaNotifyInfo> notifyInfos;
        notifyInfos.reserve(photos.size());
        for (const auto &photo : photos) {
            CHECK_AND_CONTINUE_INFO_LOG(!photo.storagePath.empty(),
                "Skip empty storagePath, fileId=%{public}d", photo.fileId);
            MediaNotifyInfo info;
            info.afterPath = photo.storagePath;
            info.objType = FileNotifyObjectType::FILE;
            info.optType = FileNotifyOperationType::MOD;
            notifyInfos.emplace_back(info);
        }
        // A deferred (conflicting) Run no-ops silently; advancing the cursor then would skip records
        // permanently this version. Re-check IDLE immediately before scanning and bail without
        // advancing if it flipped. Residual flip-during-Run is bounded by the version gate.
        if (GlobalScanner::GetInstance().GetScannerStatus() != ScannerStatus::IDLE) {
            MEDIA_INFO_LOG("GlobalScanner became busy, keep cursor=%{public}d", lastFileId);
            break;
        }
        LakeFileScanner scanner;
        scanner.Run(notifyInfos);
        processedCount += static_cast<int64_t>(notifyInfos.size());

        LogBatchResult("lake_size_repair", lastFileId, batchLastId, photos.size(), processedCount);
        lastFileId = batchLastId;
        SaveCursor(KEY_REPAIR_CURSOR, lastFileId);
    }
    MEDIA_INFO_LOG("Lake size repair done, start=%{public}d, end=%{public}d, processed=%{public}" PRId64,
        startCursor, lastFileId, processedCount);
}

void MediaLakeSizeRepairTask::LogBatchResult(const char *stage, int32_t startCursor, int32_t endCursor,
    size_t scannedCount, int64_t processedCount) const
{
    MEDIA_INFO_LOG("stage=%{public}s, cursor=%{public}d -> %{public}d, scanned=%{public}zu, processed=%{public}" PRId64,
        stage, startCursor, endCursor, scannedCount, processedCount);
}

int32_t MediaLakeSizeRepairTask::GetSavedTaskVersion()
{
    auto prefs = GetRepairPrefs();
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, 0, "prefs is nullptr");
    return prefs->GetInt(KEY_TASK_VERSION, 0);
}

void MediaLakeSizeRepairTask::SaveTaskVersion(int32_t version)
{
    SaveIntPref(KEY_TASK_VERSION, version);
}

int32_t MediaLakeSizeRepairTask::LoadCursor(const std::string &key)
{
    auto prefs = GetRepairPrefs();
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, 0, "prefs is nullptr");
    return prefs->GetInt(key, 0);
}

void MediaLakeSizeRepairTask::SaveCursor(const std::string &key, int32_t value)
{
    SaveIntPref(key, value);
}
}  // namespace OHOS::Media::Background
