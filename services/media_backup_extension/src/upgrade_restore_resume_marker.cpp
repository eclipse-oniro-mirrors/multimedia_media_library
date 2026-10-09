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

#include "upgrade_restore_resume_marker.h"
#include "media_log.h"
#include "media_file_utils.h"
#include "preferences_helper.h"
#include "preferences_errno.h"

namespace OHOS {
namespace Media {

const std::string UpgradeRestoreResumeMarker::XML_PATH =
    "/data/storage/el2/base/preferences/upgrade_restore_resume.xml";
const std::string UpgradeRestoreResumeMarker::KEY_SCENE_CODE = "scene_code";
const std::string UpgradeRestoreResumeMarker::KEY_TIMESTAMP = "timestamp";
const std::string UpgradeRestoreResumeMarker::KEY_BIZ_FLAGS = "biz_flags";
const std::string UpgradeRestoreResumeMarker::KEY_GALLERY_LOCAL_IDX = "gallery_local_idx";
const std::string UpgradeRestoreResumeMarker::KEY_GALLERY_CLOUD_IDX = "gallery_cloud_idx";
const std::string UpgradeRestoreResumeMarker::KEY_EXT_CAM_OFFSET = "external_cam_offset";
const std::string UpgradeRestoreResumeMarker::KEY_EXT_OTH_OFFSET = "external_oth_offset";
const std::string UpgradeRestoreResumeMarker::KEY_CONTINUE_INFO = "continue_info";
std::mutex UpgradeRestoreResumeMarker::markerMutex_;

bool UpgradeRestoreResumeMarker::Exists()
{
    return MediaFileUtils::IsFileExists(XML_PATH);
}

bool UpgradeRestoreResumeMarker::Create(int32_t sceneCode)
{
    std::lock_guard<std::mutex> lock(markerMutex_);
    MEDIA_INFO_LOG("UpgradeRestoreResumeMarker: Create marker, sceneCode=%{public}d", sceneCode);

    int errCode = 0;
    NativePreferences::PreferencesHelper::RemovePreferencesFromCache(XML_PATH);
    auto preferences = OHOS::NativePreferences::PreferencesHelper::GetPreferences(XML_PATH, errCode);
    if (preferences == nullptr || errCode != 0) {
        MEDIA_ERR_LOG("UpgradeRestoreResumeMarker: Failed to get preferences, errCode=%{public}d", errCode);
        return false;
    }

    int64_t currentTime = MediaFileUtils::UTCTimeMilliSeconds();
    int32_t ret = preferences->PutInt(KEY_SCENE_CODE, sceneCode);
    CHECK_AND_RETURN_RET_LOG(ret == NativePreferences::E_OK, false,
        "UpgradeRestoreResumeMarker: Failed to put sceneCode, ret=%{public}d", ret);
    ret = preferences->PutLong(KEY_TIMESTAMP, currentTime);
    CHECK_AND_RETURN_RET_LOG(ret == NativePreferences::E_OK, false,
        "UpgradeRestoreResumeMarker: Failed to put timestamp, ret=%{public}d", ret);
    ret = preferences->PutInt(KEY_BIZ_FLAGS, 0);
    CHECK_AND_RETURN_RET_LOG(ret == NativePreferences::E_OK, false,
        "UpgradeRestoreResumeMarker: Failed to put biz_flags, ret=%{public}d", ret);
    ret = preferences->PutInt(KEY_GALLERY_LOCAL_IDX, 0);
    CHECK_AND_RETURN_RET_LOG(ret == NativePreferences::E_OK, false,
        "UpgradeRestoreResumeMarker: Failed to put gallery_local_idx, ret=%{public}d", ret);
    ret = preferences->PutInt(KEY_GALLERY_CLOUD_IDX, 0);
    CHECK_AND_RETURN_RET_LOG(ret == NativePreferences::E_OK, false,
        "UpgradeRestoreResumeMarker: Failed to put gallery_cloud_idx, ret=%{public}d", ret);
    ret = preferences->PutInt(KEY_EXT_CAM_OFFSET, 0);
    CHECK_AND_RETURN_RET_LOG(ret == NativePreferences::E_OK, false,
        "UpgradeRestoreResumeMarker: Failed to put external_cam_offset, ret=%{public}d", ret);
    ret = preferences->PutInt(KEY_EXT_OTH_OFFSET, 0);
    CHECK_AND_RETURN_RET_LOG(ret == NativePreferences::E_OK, false,
        "UpgradeRestoreResumeMarker: Failed to put external_oth_offset, ret=%{public}d", ret);
    ret = preferences->PutInt(KEY_CONTINUE_INFO, 0);
    CHECK_AND_RETURN_RET_LOG(ret == NativePreferences::E_OK, false,
        "UpgradeRestoreResumeMarker: Failed to put continue_info, ret=%{public}d", ret);
    ret = preferences->FlushSync();
    CHECK_AND_RETURN_RET_LOG(ret == NativePreferences::E_OK, false,
        "UpgradeRestoreResumeMarker: Failed to flush preferences, ret=%{public}d", ret);

    MEDIA_INFO_LOG("UpgradeRestoreResumeMarker: Marker created");
    return true;
}

bool UpgradeRestoreResumeMarker::Delete()
{
    std::lock_guard<std::mutex> lock(markerMutex_);
    MEDIA_INFO_LOG("UpgradeRestoreResumeMarker: Delete marker");

    if (!MediaFileUtils::IsFileExists(XML_PATH)) {
        NativePreferences::PreferencesHelper::RemovePreferencesFromCache(XML_PATH);
        return true;
    }

    int32_t errCode = NativePreferences::PreferencesHelper::DeletePreferences(XML_PATH);
    CHECK_AND_RETURN_RET_LOG(errCode == NativePreferences::E_OK, false,
        "UpgradeRestoreResumeMarker: Failed to delete preferences, errCode=%{public}d", errCode);

    MEDIA_INFO_LOG("UpgradeRestoreResumeMarker: Marker deleted");
    return true;
}

int32_t UpgradeRestoreResumeMarker::GetInt(const std::string &key, int32_t defaultValue)
{
    if (!MediaFileUtils::IsFileExists(XML_PATH)) {
        return defaultValue;
    }
    int errCode = 0;
    NativePreferences::PreferencesHelper::RemovePreferencesFromCache(XML_PATH);
    auto preferences = OHOS::NativePreferences::PreferencesHelper::GetPreferences(XML_PATH, errCode);
    if (preferences == nullptr || errCode != 0) {
        MEDIA_ERR_LOG("UpgradeRestoreResumeMarker: Failed to get preferences, errCode=%{public}d", errCode);
        return defaultValue;
    }
    return preferences->GetInt(key, defaultValue);
}

bool UpgradeRestoreResumeMarker::PutInt(const std::string &key, int32_t value)
{
    int errCode = 0;
    NativePreferences::PreferencesHelper::RemovePreferencesFromCache(XML_PATH);
    auto preferences = OHOS::NativePreferences::PreferencesHelper::GetPreferences(XML_PATH, errCode);
    if (preferences == nullptr || errCode != 0) {
        MEDIA_ERR_LOG("UpgradeRestoreResumeMarker: Failed to get preferences, errCode=%{public}d", errCode);
        return false;
    }
    int32_t ret = preferences->PutInt(key, value);
    CHECK_AND_RETURN_RET_LOG(ret == NativePreferences::E_OK, false,
        "UpgradeRestoreResumeMarker: Failed to put %{public}s, ret=%{public}d", key.c_str(), ret);
    ret = preferences->FlushSync();
    CHECK_AND_RETURN_RET_LOG(ret == NativePreferences::E_OK, false,
        "UpgradeRestoreResumeMarker: Failed to flush, ret=%{public}d", ret);
    return true;
}

int32_t UpgradeRestoreResumeMarker::GetBizFlags()
{
    std::lock_guard<std::mutex> lock(markerMutex_);
    return GetInt(KEY_BIZ_FLAGS, 0);
}

bool UpgradeRestoreResumeMarker::SetBizFlags(int32_t flags)
{
    std::lock_guard<std::mutex> lock(markerMutex_);
    return PutInt(KEY_BIZ_FLAGS, flags);
}

bool UpgradeRestoreResumeMarker::IsBusinessDone(ResumeBusinessFlag flag)
{
    int32_t flags = GetBizFlags();
    int32_t bit = 1 << static_cast<int32_t>(flag);
    return (flags & bit) != 0;
}

bool UpgradeRestoreResumeMarker::SetBusinessDone(ResumeBusinessFlag flag)
{
    std::lock_guard<std::mutex> lock(markerMutex_);
    int32_t flags = GetInt(KEY_BIZ_FLAGS, 0);
    int32_t bit = 1 << static_cast<int32_t>(flag);
    flags |= bit;
    bool ret = PutInt(KEY_BIZ_FLAGS, flags);
    MEDIA_INFO_LOG("UpgradeRestoreResumeMarker: SetBusinessDone flag=%{public}d, ret=%{public}d",
        static_cast<int32_t>(flag), ret);
    return ret;
}

int32_t UpgradeRestoreResumeMarker::GetSceneCode()
{
    std::lock_guard<std::mutex> lock(markerMutex_);
    return GetInt(KEY_SCENE_CODE, -1);
}

int32_t UpgradeRestoreResumeMarker::GetGalleryLocalMinIdIndex()
{
    std::lock_guard<std::mutex> lock(markerMutex_);
    return GetInt(KEY_GALLERY_LOCAL_IDX, 0);
}

bool UpgradeRestoreResumeMarker::SetGalleryLocalMinIdIndex(int32_t index)
{
    std::lock_guard<std::mutex> lock(markerMutex_);
    return PutInt(KEY_GALLERY_LOCAL_IDX, index);
}

int32_t UpgradeRestoreResumeMarker::GetGalleryCloudMinIdIndex()
{
    std::lock_guard<std::mutex> lock(markerMutex_);
    return GetInt(KEY_GALLERY_CLOUD_IDX, 0);
}

bool UpgradeRestoreResumeMarker::SetGalleryCloudMinIdIndex(int32_t index)
{
    std::lock_guard<std::mutex> lock(markerMutex_);
    return PutInt(KEY_GALLERY_CLOUD_IDX, index);
}

int32_t UpgradeRestoreResumeMarker::GetExternalCameraOffset()
{
    std::lock_guard<std::mutex> lock(markerMutex_);
    return GetInt(KEY_EXT_CAM_OFFSET, 0);
}

bool UpgradeRestoreResumeMarker::SetExternalCameraOffset(int32_t offset)
{
    std::lock_guard<std::mutex> lock(markerMutex_);
    return PutInt(KEY_EXT_CAM_OFFSET, offset);
}

int32_t UpgradeRestoreResumeMarker::GetExternalOthersOffset()
{
    std::lock_guard<std::mutex> lock(markerMutex_);
    return GetInt(KEY_EXT_OTH_OFFSET, 0);
}

bool UpgradeRestoreResumeMarker::SetExternalOthersOffset(int32_t offset)
{
    std::lock_guard<std::mutex> lock(markerMutex_);
    return PutInt(KEY_EXT_OTH_OFFSET, offset);
}

int32_t UpgradeRestoreResumeMarker::GetContinueInfo()
{
    std::lock_guard<std::mutex> lock(markerMutex_);
    return GetInt(KEY_CONTINUE_INFO, 0);
}

bool UpgradeRestoreResumeMarker::SetContinueInfo(int32_t info)
{
    std::lock_guard<std::mutex> lock(markerMutex_);
    return PutInt(KEY_CONTINUE_INFO, info);
}

} // namespace Media
} // namespace OHOS
