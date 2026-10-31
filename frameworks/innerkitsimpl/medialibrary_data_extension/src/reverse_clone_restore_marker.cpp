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
#include "media_pure_file_utils.h"
#include "preferences.h"
#include "preferences_helper.h"

#include <cinttypes>
#include <string>

namespace OHOS {
namespace Media {
namespace {
const std::string REVERSE_CLONE_RESTORE_RDB_MARKER_XML =
    "/data/storage/el2/database/rdb/media_library_reverse_restore_marker.xml";
const std::string REVERSE_CLONE_RESTORE_KVDB_MARKER_XML =
    "/data/storage/el2/database/kvdb/kvdb_reverse_restore_marker.xml";
const std::string REVERSE_CLONE_RESTORE_MARKER_TIME = "reverse_restore_marker_time";
constexpr int64_t MILLISECONDS_PER_SECOND = 1000;
constexpr int64_t REVERSE_CLONE_RESTORE_MARKER_TTL_MILLISECONDS = 48 * 60 * 60 * MILLISECONDS_PER_SECOND;
} // namespace

std::string ReverseCloneRestoreMarker::GetMarkerPath(ReverseCloneMarkerType type)
{
    return type == ReverseCloneMarkerType::RDB ? REVERSE_CLONE_RESTORE_RDB_MARKER_XML :
        REVERSE_CLONE_RESTORE_KVDB_MARKER_XML;
}

bool ReverseCloneRestoreMarker::GetRecordTime(ReverseCloneMarkerType type, int64_t &recordTimeMilliseconds)
{
    std::string markerPath = GetMarkerPath(type);
    CHECK_AND_RETURN_RET_INFO_LOG(MediaPureFileUtils::IsFileExists(markerPath), false,
        "reverse clone restore marker does not exist, type=%{public}d", static_cast<int>(type));
    int32_t errCode = 0;
    auto prefs = NativePreferences::PreferencesHelper::GetPreferences(markerPath, errCode);
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, false,
        "get reverse clone marker prefs failed, type=%{public}d, err=%{public}d", static_cast<int>(type), errCode);

    recordTimeMilliseconds = prefs->GetLong(REVERSE_CLONE_RESTORE_MARKER_TIME, 0);
    return recordTimeMilliseconds > 0;
}

bool ReverseCloneRestoreMarker::Exists(ReverseCloneMarkerType type)
{
    return MediaPureFileUtils::IsFileExists(GetMarkerPath(type));
}

bool ReverseCloneRestoreMarker::Delete(ReverseCloneMarkerType type)
{
    std::string markerPath = GetMarkerPath(type);
    CHECK_AND_RETURN_RET_INFO_LOG(MediaPureFileUtils::IsFileExists(markerPath), true,
        "reverse clone restore marker does not exist, type=%{public}d", static_cast<int>(type));
    int32_t errCode = NativePreferences::PreferencesHelper::DeletePreferences(markerPath);
    CHECK_AND_RETURN_RET_LOG(errCode == 0, false,
        "delete reverse clone restore marker failed, type=%{public}d, err=%{public}d",
        static_cast<int>(type), errCode);
    MEDIA_INFO_LOG("reverse clone restore marker deleted, type=%{public}d", static_cast<int>(type));
    return true;
}

bool ReverseCloneRestoreMarker::Recreate(ReverseCloneMarkerType type)
{
    CHECK_AND_RETURN_RET(Delete(type), false);

    std::string markerPath = GetMarkerPath(type);
    int32_t errCode = 0;
    auto prefs = NativePreferences::PreferencesHelper::GetPreferences(markerPath, errCode);
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, false,
        "get reverse clone marker prefs failed, type=%{public}d, err=%{public}d",
        static_cast<int>(type), errCode);

    int64_t currentTime = MediaFileUtils::UTCTimeMilliSeconds();
    int32_t ret = prefs->PutLong(REVERSE_CLONE_RESTORE_MARKER_TIME, currentTime);
    CHECK_AND_RETURN_RET_LOG(ret == NativePreferences::E_OK, false,
        "put reverse clone restore marker failed, type=%{public}d, ret=%{public}d",
        static_cast<int>(type), ret);
    ret = prefs->FlushSync();
    CHECK_AND_RETURN_RET_LOG(ret == NativePreferences::E_OK, false,
        "flush reverse clone restore marker failed, type=%{public}d, ret=%{public}d",
        static_cast<int>(type), ret);
    MEDIA_INFO_LOG("reverse clone restore marker recreated, type=%{public}d, timeMs=%{public}" PRId64,
        static_cast<int>(type), currentTime);
    return true;
}

bool ReverseCloneRestoreMarker::IsExpired(ReverseCloneMarkerType type, int64_t currentTimeMilliseconds)
{
    int64_t recordTime = 0;
    CHECK_AND_RETURN_RET(GetRecordTime(type, recordTime), false);
    return currentTimeMilliseconds - recordTime >= REVERSE_CLONE_RESTORE_MARKER_TTL_MILLISECONDS;
}

bool ReverseCloneRestoreMarker::DeleteIfExpired(ReverseCloneMarkerType type, int64_t currentTimeMilliseconds)
{
    std::string markerPath = GetMarkerPath(type);
    int64_t recordTime = 0;
    CHECK_AND_RETURN_RET_INFO_LOG(MediaPureFileUtils::IsFileExists(markerPath), false,
        "reverse clone restore marker does not exist, type=%{public}d", static_cast<int>(type));
    if (!GetRecordTime(type, recordTime)) {
        MEDIA_WARN_LOG("reverse clone restore marker is invalid, delete it, type=%{public}d", static_cast<int>(type));
        return Delete(type);
    }
    CHECK_AND_RETURN_RET(currentTimeMilliseconds - recordTime >= REVERSE_CLONE_RESTORE_MARKER_TTL_MILLISECONDS, false);

    int64_t latestRecordTime = 0;
    if (!GetRecordTime(type, latestRecordTime)) {
        MEDIA_WARN_LOG("reverse clone restore marker is invalid before delete, delete it, type=%{public}d",
            static_cast<int>(type));
        return Delete(type);
    }
    CHECK_AND_RETURN_RET_LOG(latestRecordTime == recordTime, false,
        "reverse clone restore marker changed, type=%{public}d, oldTimeMs=%{public}" PRId64
        ", latestTimeMs=%{public}" PRId64,
        static_cast<int>(type), recordTime, latestRecordTime);
    return Delete(type);
}
} // namespace Media
} // namespace OHOS
// LCOV_EXCL_STOP
