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
#define MLOG_TAG "CheckStatusHelper"

#include "check_status_helper.h"

#include <cinttypes>

#include "media_file_utils.h"
#include "media_log.h"

namespace OHOS::Media {
const std::string KEY_LAST_FILE_ID = "last_file_id";
const std::string KEY_LAST_ALBUM_ID = "last_album_id";
const std::string KEY_LAST_PHOTO_ADD_COUNT = "last_photo_add_count";
const std::string KEY_LAST_PHOTO_UPDATE_COUNT = "last_photo_update_count";
const std::string KEY_LAST_PHOTO_DELETE_COUNT = "last_photo_delete_count";
const std::string KEY_LAST_ALBUM_ADD_COUNT = "last_album_add_count";
const std::string KEY_LAST_ALBUM_UPDATE_COUNT = "last_album_update_count";
const std::string KEY_LAST_ALBUM_DELETE_COUNT = "last_album_delete_count";
const std::string KEY_LAST_START_TIME_IN_MS = "last_start_time_in_ms";
const std::string KEY_LAST_END_TIME_IN_MS = "last_end_time_in_ms";

int32_t CheckStatusHelper::GetInt32ValueByKey(const std::string &key, int32_t defaultValue)
{
    auto prefs = GetPreferences();
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, defaultValue, "GetPreferences failed");
    return prefs->GetInt(key, defaultValue);
}

void CheckStatusHelper::SetInt32ValueByKey(const std::string &key, int32_t value)
{
    auto prefs = GetPreferences();
    CHECK_AND_RETURN_LOG(prefs != nullptr, "GetPreferences failed");
    prefs->PutInt(key, value);
    prefs->FlushSync();
    MEDIA_INFO_LOG("%{public}s: %{public}d", key.c_str(), value);
}

int64_t CheckStatusHelper::GetInt64ValueByKey(const std::string &key, int64_t defaultValue)
{
    auto prefs = GetPreferences();
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, defaultValue, "GetPreferences failed");
    return prefs->GetLong(key, defaultValue);
}

void CheckStatusHelper::SetInt64ValueByKey(const std::string &key, int64_t value)
{
    auto prefs = GetPreferences();
    CHECK_AND_RETURN_LOG(prefs != nullptr, "GetPreferences failed");
    prefs->PutLong(key, value);
    prefs->FlushSync();
    MEDIA_INFO_LOG("%{public}s: %{public}" PRId64, key.c_str(), value);
}

int64_t CheckStatusHelper::GetLastEndTimeInMs(int64_t defaultValue)
{
    return GetInt64ValueByKey(KEY_LAST_END_TIME_IN_MS, defaultValue);
}

void CheckStatusHelper::LoadStatus(ConsistencyCheck::ScenarioProgress &progress, ConsistencyCheck::DfxStats &dfxStats)
{
    auto prefs = GetPreferences();
    CHECK_AND_RETURN_LOG(prefs != nullptr, "GetPreferences failed");

    progress.lastFileId = prefs->GetInt(KEY_LAST_FILE_ID, 0);
    progress.lastAlbumId = prefs->GetInt(KEY_LAST_ALBUM_ID, 0);
    dfxStats.photoAddCount = prefs->GetInt(KEY_LAST_PHOTO_ADD_COUNT, 0);
    dfxStats.photoUpdateCount = prefs->GetInt(KEY_LAST_PHOTO_UPDATE_COUNT, 0);
    dfxStats.photoDeleteCount = prefs->GetInt(KEY_LAST_PHOTO_DELETE_COUNT, 0);
    dfxStats.albumAddCount = prefs->GetInt(KEY_LAST_ALBUM_ADD_COUNT, 0);
    dfxStats.albumUpdateCount = prefs->GetInt(KEY_LAST_ALBUM_UPDATE_COUNT, 0);
    dfxStats.albumDeleteCount = prefs->GetInt(KEY_LAST_ALBUM_DELETE_COUNT, 0);
    dfxStats.startTimeInMs = static_cast<uint64_t>(prefs->GetLong(KEY_LAST_START_TIME_IN_MS, 0));
    dfxStats.endTimeInMs = static_cast<uint64_t>(prefs->GetLong(KEY_LAST_END_TIME_IN_MS, 0));

    MEDIA_INFO_LOG("Get %{public}s, %{public}s", progress.ToString().c_str(), dfxStats.ToString().c_str());
}

void CheckStatusHelper::SaveCurrentStatus(const ConsistencyCheck::ScenarioProgress &progress,
    const ConsistencyCheck::DfxStats &dfxStats)
{
    auto prefs = GetPreferences();
    CHECK_AND_RETURN_LOG(prefs != nullptr, "GetPreferences failed");

    prefs->PutInt(KEY_LAST_FILE_ID, progress.lastFileId);
    prefs->PutInt(KEY_LAST_ALBUM_ID, progress.lastAlbumId);
    prefs->PutInt(KEY_LAST_PHOTO_ADD_COUNT, dfxStats.photoAddCount);
    prefs->PutInt(KEY_LAST_PHOTO_UPDATE_COUNT, dfxStats.photoUpdateCount);
    prefs->PutInt(KEY_LAST_PHOTO_DELETE_COUNT, dfxStats.photoDeleteCount);
    prefs->PutInt(KEY_LAST_ALBUM_ADD_COUNT, dfxStats.albumAddCount);
    prefs->PutInt(KEY_LAST_ALBUM_UPDATE_COUNT, dfxStats.albumUpdateCount);
    prefs->PutInt(KEY_LAST_ALBUM_DELETE_COUNT, dfxStats.albumDeleteCount);
    prefs->PutLong(KEY_LAST_START_TIME_IN_MS, dfxStats.startTimeInMs);
    // Note: only set endTimeInMs when it is finished
    prefs->FlushSync();

    MEDIA_INFO_LOG("Set %{public}s, %{public}s", progress.ToString().c_str(), dfxStats.ToString().c_str());
}

void CheckStatusHelper::SaveFinishedStatus(int64_t endTimeInMs)
{
    auto prefs = GetPreferences();
    CHECK_AND_RETURN_LOG(prefs != nullptr, "GetPreferences failed");

    prefs->PutInt(KEY_LAST_FILE_ID, 0);
    prefs->PutInt(KEY_LAST_ALBUM_ID, 0);
    prefs->PutInt(KEY_LAST_PHOTO_ADD_COUNT, 0);
    prefs->PutInt(KEY_LAST_PHOTO_UPDATE_COUNT, 0);
    prefs->PutInt(KEY_LAST_PHOTO_DELETE_COUNT, 0);
    prefs->PutInt(KEY_LAST_ALBUM_ADD_COUNT, 0);
    prefs->PutInt(KEY_LAST_ALBUM_UPDATE_COUNT, 0);
    prefs->PutInt(KEY_LAST_ALBUM_DELETE_COUNT, 0);
    prefs->PutLong(KEY_LAST_START_TIME_IN_MS, 0);
    prefs->PutLong(KEY_LAST_END_TIME_IN_MS, endTimeInMs);
    prefs->FlushSync();

    MEDIA_INFO_LOG("Set endTimeInMs %{public}" PRId64, endTimeInMs);
}

std::shared_ptr<NativePreferences::Preferences> CheckStatusHelper::GetPreferences()
{
    if (prefs_ != nullptr) {
        return prefs_;
    }
    std::string xmlPath = "/data/storage/el2/base/preferences/consistency_check_" +
        std::to_string(static_cast<int32_t>(scene_)) + ".xml";
    int32_t errCode;
    prefs_ = NativePreferences::PreferencesHelper::GetPreferences(xmlPath, errCode);
    CHECK_AND_PRINT_LOG(prefs_ != nullptr, "Get preferences failed, err: %{public}d, scene: %{public}d",
        errCode, static_cast<int32_t>(scene_));
    return prefs_;
}
}  // namespace OHOS::Media