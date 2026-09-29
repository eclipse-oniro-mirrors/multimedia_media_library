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

#include "reverse_clone_reliability_marker.h"
#include "media_log.h"
#include "media_file_utils.h"
#include "preferences_helper.h"
#include "preferences_errno.h"

namespace OHOS {
namespace Media {

const std::string ReverseCloneReliabilityMarker::MARKER_XML =
    "/data/storage/el2/base/preferences/reverse_restore_marker.xml";
const std::string ReverseCloneReliabilityMarker::KEY_STAGE = "stage";
const std::string ReverseCloneReliabilityMarker::KEY_TIMESTAMP = "timestamp";

bool ReverseCloneReliabilityMarker::Delete()
{
    MEDIA_INFO_LOG("ReverseCloneReliabilityMarker: Delete marker");

    if (!MediaFileUtils::IsFileExists(MARKER_XML)) {
        NativePreferences::PreferencesHelper::RemovePreferencesFromCache(MARKER_XML);
        return true;
    }

    int32_t errCode = NativePreferences::PreferencesHelper::DeletePreferences(MARKER_XML);
    CHECK_AND_RETURN_RET_LOG(errCode == NativePreferences::E_OK, false,
        "ReverseCloneReliabilityMarker: Failed to delete preferences, errCode=%{public}d", errCode);

    MEDIA_INFO_LOG("ReverseCloneReliabilityMarker: Marker deleted");
    return true;
}

bool ReverseCloneReliabilityMarker::Exists()
{
    return MediaFileUtils::IsFileExists(MARKER_XML);
}

bool ReverseCloneReliabilityMarker::SetStage(ReverseCloneRestoreStage stage)
{
    MEDIA_INFO_LOG("ReverseCloneReliabilityMarker: Set stage to %{public}d", static_cast<int>(stage));

    int errCode = 0;
    NativePreferences::PreferencesHelper::RemovePreferencesFromCache(MARKER_XML);
    auto preferences = OHOS::NativePreferences::PreferencesHelper::GetPreferences(MARKER_XML, errCode);
    if (preferences == nullptr || errCode != 0) {
        MEDIA_ERR_LOG("ReverseCloneReliabilityMarker: Failed to get preferences, errCode=%{public}d", errCode);
        return false;
    }

    int64_t currentTime = MediaFileUtils::UTCTimeMilliSeconds();
    int32_t ret = preferences->PutInt(KEY_STAGE, static_cast<int>(stage));
    CHECK_AND_RETURN_RET_LOG(ret == NativePreferences::E_OK, false,
        "ReverseCloneReliabilityMarker: Failed to put stage, ret=%{public}d", ret);
    ret = preferences->PutLong(KEY_TIMESTAMP, currentTime);
    CHECK_AND_RETURN_RET_LOG(ret == NativePreferences::E_OK, false,
        "ReverseCloneReliabilityMarker: Failed to put timestamp, ret=%{public}d", ret);
    ret = preferences->FlushSync();
    CHECK_AND_RETURN_RET_LOG(ret == NativePreferences::E_OK, false,
        "ReverseCloneReliabilityMarker: Failed to flush preferences, ret=%{public}d", ret);

    MEDIA_INFO_LOG("ReverseCloneReliabilityMarker: Stage updated to %{public}d", static_cast<int>(stage));
    return true;
}

bool ReverseCloneReliabilityMarker::GetStage(ReverseCloneRestoreStage &stage)
{
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsFileExists(MARKER_XML), false,
        "ReverseCloneReliabilityMarker: Marker does not exist");
    int errCode = 0;
    NativePreferences::PreferencesHelper::RemovePreferencesFromCache(MARKER_XML);
    auto preferences = OHOS::NativePreferences::PreferencesHelper::GetPreferences(MARKER_XML, errCode);
    if (preferences == nullptr || errCode != 0) {
        MEDIA_ERR_LOG("ReverseCloneReliabilityMarker: Failed to get preferences, errCode=%{public}d", errCode);
        return false;
    }

    int stageValue = preferences->GetInt(KEY_STAGE, -1);
    if (stageValue < 0 || stageValue > static_cast<int>(ReverseCloneRestoreStage::COMPLETED)) {
        MEDIA_ERR_LOG("ReverseCloneReliabilityMarker: Invalid stage value: %{public}d", stageValue);
        return false;
    }

    stage = static_cast<ReverseCloneRestoreStage>(stageValue);
    MEDIA_INFO_LOG("ReverseCloneReliabilityMarker: Got stage=%{public}d", static_cast<int>(stage));
    return true;
}

} // namespace Media
} // namespace OHOS