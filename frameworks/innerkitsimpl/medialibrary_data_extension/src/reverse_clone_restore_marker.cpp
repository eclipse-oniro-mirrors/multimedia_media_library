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

#define MLOG_TAG "Media_Reverse_Clone_Marker"

#include "reverse_clone_restore_marker.h"

#include "media_file_utils.h"
#include "media_log.h"
#include "preferences.h"
#include "preferences_helper.h"

#include <cinttypes>
#include <string>

namespace OHOS {
namespace Media {
namespace {
const std::string REVERSE_CLONE_RESTORE_MARKER_XML =
    "/data/storage/el2/database/rdb/media_library_reverse_restore_marker.xml";
const std::string REVERSE_CLONE_RESTORE_MARKER_TIME = "reverse_restore_marker_time";
constexpr int64_t MILLISECONDS_PER_SECOND = 1000;
constexpr int64_t REVERSE_CLONE_RESTORE_MARKER_TTL_MILLISECONDS = 48 * 60 * 60 * MILLISECONDS_PER_SECOND;
} // namespace

bool ReverseCloneRestoreMarker::GetRecordTime(int64_t &recordTimeMilliseconds)
{
    CHECK_AND_RETURN_RET_INFO_LOG(MediaFileUtils::IsFileExists(REVERSE_CLONE_RESTORE_MARKER_XML), false,
        "reverse clone restore marker does not exist");
    int32_t errCode = 0;
    auto prefs = NativePreferences::PreferencesHelper::GetPreferences(REVERSE_CLONE_RESTORE_MARKER_XML, errCode);
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, false, "get reverse clone marker prefs failed, err=%{public}d",
        errCode);

    recordTimeMilliseconds = prefs->GetLong(REVERSE_CLONE_RESTORE_MARKER_TIME, 0);
    return recordTimeMilliseconds > 0;
}

bool ReverseCloneRestoreMarker::Delete()
{
    CHECK_AND_RETURN_RET_INFO_LOG(MediaFileUtils::IsFileExists(REVERSE_CLONE_RESTORE_MARKER_XML), true,
        "reverse clone restore marker does not exist");
    int32_t errCode = NativePreferences::PreferencesHelper::DeletePreferences(REVERSE_CLONE_RESTORE_MARKER_XML);
    CHECK_AND_RETURN_RET_LOG(errCode == 0, false, "delete reverse clone restore marker failed, err=%{public}d",
        errCode);
    MEDIA_INFO_LOG("reverse clone restore marker deleted");
    return true;
}

bool ReverseCloneRestoreMarker::Recreate()
{
    CHECK_AND_RETURN_RET(Delete(), false);

    int32_t errCode = 0;
    auto prefs = NativePreferences::PreferencesHelper::GetPreferences(REVERSE_CLONE_RESTORE_MARKER_XML, errCode);
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, false, "get reverse clone marker prefs failed, err=%{public}d",
        errCode);

    int64_t currentTime = MediaFileUtils::UTCTimeMilliSeconds();
    int32_t ret = prefs->PutLong(REVERSE_CLONE_RESTORE_MARKER_TIME, currentTime);
    CHECK_AND_RETURN_RET_LOG(ret == NativePreferences::E_OK, false,
        "put reverse clone restore marker failed, ret=%{public}d", ret);
    ret = prefs->FlushSync();
    CHECK_AND_RETURN_RET_LOG(ret == NativePreferences::E_OK, false,
        "flush reverse clone restore marker failed, ret=%{public}d", ret);
    MEDIA_INFO_LOG("reverse clone restore marker recreated, timeMs=%{public}" PRId64, currentTime);
    return true;
}

bool ReverseCloneRestoreMarker::IsExpired(int64_t currentTimeMilliseconds)
{
    int64_t recordTime = 0;
    CHECK_AND_RETURN_RET(GetRecordTime(recordTime), false);
    return currentTimeMilliseconds - recordTime >= REVERSE_CLONE_RESTORE_MARKER_TTL_MILLISECONDS;
}

bool ReverseCloneRestoreMarker::DeleteIfExpired(int64_t currentTimeMilliseconds)
{
    int64_t recordTime = 0;
    CHECK_AND_RETURN_RET_INFO_LOG(MediaFileUtils::IsFileExists(REVERSE_CLONE_RESTORE_MARKER_XML), false,
        "reverse clone restore marker does not exist");
    if (!GetRecordTime(recordTime)) {
        MEDIA_WARN_LOG("reverse clone restore marker is invalid, delete it");
        return Delete();
    }
    CHECK_AND_RETURN_RET(currentTimeMilliseconds - recordTime >= REVERSE_CLONE_RESTORE_MARKER_TTL_MILLISECONDS, false);

    int64_t latestRecordTime = 0;
    if (!GetRecordTime(latestRecordTime)) {
        MEDIA_WARN_LOG("reverse clone restore marker is invalid before delete, delete it");
        return Delete();
    }
    CHECK_AND_RETURN_RET_LOG(latestRecordTime == recordTime, false,
        "reverse clone restore marker changed, oldTimeMs=%{public}" PRId64 ", latestTimeMs=%{public}" PRId64,
        recordTime, latestRecordTime);
    return Delete();
}
} // namespace Media
} // namespace OHOS
