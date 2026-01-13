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

#include "medialibrary_upgrade_utils.h"

#include <unordered_map>
#include "medialibrary_db_const.h"
#include "media_log.h"
#include "dfx_reporter.h"
#include "dfx_manager.h"
#include "media_file_utils.h"

using namespace std;

namespace OHOS {
namespace Media {
static unordered_map<int32_t, string> UPGRADE_VALUE_MAP = {
    { VERSION_FIX_DB_UPGRADE_TO_API20, "VERSION_FIX_DB_UPGRADE_TO_API20" },
    { VERSION_UPDATE_PHOTO_ALBUM_DATEMODIFIED_TIGGER, "VERSION_UPDATE_PHOTO_ALBUM_DATEMODIFIED_TIGGER" },
    { VERSION_ADD_RELATIONSHIP_AND_UPDATE_TRIGGER, "VERSION_ADD_RELATIONSHIP_AND_UPDATE_TRIGGER" },
    { VERSION_ADD_APPLINK_VERSION, "VERSION_ADD_APPLINK_VERSION" },
    { VERSION_CREATE_TMP_COMPATIBLE_DUP, "VERSION_CREATE_TMP_COMPATIBLE_DUP" },
    { VERSION_ADD_MEDIA_BACKUP_INFO, "VERSION_ADD_MEDIA_BACKUP_INFO" },
    { VERSION_ADD_HIGHLIGHT_VIEWED_NOTIFICATION, "VERSION_ADD_HIGHLIGHT_VIEWED_NOTIFICATION" },
    { VERSION_ADD_SOUTH_DEVICE_TYPE, "VERSION_ADD_SOUTH_DEVICE_TYPE"},
    { VERSION_PHOTO_ALBUM_ADD_UPLOAD_STATUS, "VERSION_PHOTO_ALBUM_ADD_UPLOAD_STATUS" },
    { VERSION_ADD_TAB_ANALYSIS_PROGRESS, "VERSION_ADD_TAB_ANALYSIS_PROGRESS" },
    { VERSION_ADD_COMPOSITE_DISPLAY_STATUS_COLUMNS, "VERSION_ADD_COMPOSITE_DISPLAY_STATUS_COLUMNS" },
    { VERSION_ADD_INDEX_FOR_PHOTO_SORT_IN_ALBUM, "VERSION_ADD_INDEX_FOR_PHOTO_SORT_IN_ALBUM" },
    { VERSION_ADD_TAB_OLD_PHOTOS_CLONE_SEQUENCE, "VERSION_ADD_TAB_OLD_PHOTOS_CLONE_SEQUENCE" },
    { VERSION_ADD_FILE_SOURCE_TYPE, "VERSION_ADD_FILE_SOURCE_TYPE" },
    { VERSION_ADD_INDEX_FOR_CLOUD_AND_PITAYA, "VERSION_ADD_INDEX_FOR_CLOUD_AND_PITAYA" },
    { VERSION_ADD_HDR_MODE, "VERSION_ADD_HDR_MODE" },
    { VERSION_ADD_ANALYSIS_STATUS, "VERSION_ADD_ANALYSIS_STATUS" },
    { VERSION_ADD_AFFECTIVE_TABLE, "VERSION_ADD_AFFECTIVE_TABLE"},
    { VERSION_ADD_3DGS_MODE, "VERSION_ADD_3DGS_MODE"},
    { VERSION_UPGRADE_IDX_SCHPT_HIDDEN_TIME, "VERSION_UPGRADE_IDX_SCHPT_HIDDEN_TIME"},
    { VERSION_CREATE_TAB_OLD_ALBUM, "VERSION_CREATE_TAB_OLD_ALBUM" },
    { VERSION_UPGRADE_IDX_SCHPT_DATE_ADDED, "VERSION_UPGRADE_IDX_SCHPT_DATE_ADDED"},
    { VERSION_ADD_IMAGE_FACE_DETAIL, "VERSION_ADD_IMAGE_FACE_DETAIL" },
    { VERSION_ADD_TAB_ANALYSIS_PROGRESS_COLUMNS, "VERSION_ADD_TAB_ANALYSIS_PROGRESS_COLUMNS" },
    { VERSION_ADD_BATCH_DOWNLOAD, "VERSION_ADD_BATCH_DOWNLOAD" },
    { VERSION_VIDEO_MODE, "VERSION_VIDEO_MODE" },
    { VERSION_CREATE_VIDEO_FACE_TAG_ID_INDEX, "VERSION_CREATE_VIDEO_FACE_TAG_ID_INDEX" },
    { VERSION_CREATE_TAB_CLONED_OLD_PHOTOS, "VERSION_CREATE_TAB_CLONED_OLD_PHOTOS"},
    { VERSION_UPDATE_VIDEO_LABLE_FACE, "VERSION_UPDATE_VIDEO_LABLE_FACE" },
    { VERSION_UPDATE_VIDEO_FACE_TAGID, "VERSION_UPDATE_VIDEO_FACE_TAGID" },
    { VERSION_UPDATE_MDIRTY_TRIGGER_FOR_STRONG_ASSOCIATION, "VERSION_UPDATE_MDIRTY_TRIGGER_FOR_STRONG_ASSOCIATION"},
    { VERSION_ADD_IMAGE_FACE_AND_FACE_TAG_AGE_GENDER, "VERSION_ADD_IMAGE_FACE_AND_FACE_TAG_AGE_GENDER"},
    { VERSION_ADD_QUICK_CAPTURE_AND_TIME_LAPSE, "VERSION_ADD_QUICK_CAPTURE_AND_TIME_LAPSE" },
    { VERSION_ADD_LAKE_COLUMN, "VERSION_ADD_LAKE_COLUMN"},
    { VERSION_FILTER_TAB_ASSET_ALBUM_OPERATION, "VERSION_FILTER_TAB_ASSET_ALBUM_OPERATION"},
    { VERSION_ADD_TAB_ANALYSIS_PROGRESS_CHECK_SPACE_COLUMN, "VERSION_ADD_TAB_ANALYSIS_PROGRESS_CHECK_SPACE_COLUMN"},
    { VERSION_ADD_PET_TABLES, "VERSION_ADD_PET_TABLES"},
    { VERSION_ADD_ASPECT_RATIO, "VERSION_ADD_ASPECT_RATIO"},
    { VERSION_SOURCE_ALBUM_BUNDLE_UPDATE_TRIGGER_USE_LPATH, "VERSION_SOURCE_ALBUM_BUNDLE_UPDATE_TRIGGER_USE_LPATH"},
    { VERSION_ADD_PHOTO_ALBUM_HIDDEN, "VERSION_ADD_PHOTO_ALBUM_HIDDEN" },
    { VERSION_ADD_CHANGE_TIME, "VERSION_ADD_CHANGE_TIME" },
    { VERSION_ADD_EDIT_OPERATION, "VERSION_ADD_EDIT_OPERATION" },
    { VERSION_UPDATE_MDIRTY_TRIGGER_FOR_HDR_MODE, "VERSION_UPDATE_MDIRTY_TRIGGER_FOR_HDR_MODE" },
    { VERSION_ADD_AUDIO_IS_TEMP, "VERSION_ADD_AUDIO_IS_TEMP" },
    { VERSION_ADD_ANALYSIS_UPDATE_SEARCH_TRIGGER, "VERSION_ADD_ANALYSIS_UPDATE_SEARCH_TRIGGER" },
    { VERSION_ADD_ALBUM_ORDER_BACK_VERSION, "VERSION_ADD_ALBUM_ORDER_BACK_VERSION" },
    { VERSION_DROP_PHOTO_STATUS_FOR_SEARCH_INDEX, "VERSION_DROP_PHOTO_STATUS_FOR_SEARCH_INDEX" },
    { VERSION_ADD_PHOTO_MOVINGPHOTO_ENHANCEMENT_TYPE, "VERSION_ADD_PHOTO_MOVINGPHOTO_ENHANCEMENT_TYPE" },
    { VERSION_ADD_CRITICAL_TYPE_COLUMN_ON_PHOTOS, "VERSION_ADD_CRITICAL_TYPE_COLUMN_ON_PHOTOS"},
    { VERSION_ADD_PHOTO_RISK_STATUS, "VERSION_ADD_PHOTO_RISK_STATUS" },
    { VERSION_FIX_MINECRAFT_PE, "VERSION_FIX_MINECRAFT_PE" },
};
static vector<string> UPGRADE_DFX_MESSAGES;

bool RdbUpgradeUtils::HasUpgraded(int32_t version, bool isSync)
{
    if (UPGRADE_VALUE_MAP.find(version) == UPGRADE_VALUE_MAP.end()) {
        return false;
    }
    int32_t errCode = 0;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(RDB_UPGRADE_EVENT, errCode);
    MEDIA_INFO_LOG("rdb_upgrade_events prefs errCode: %{public}d", errCode);
    CHECK_AND_RETURN_RET_WARN_LOG(prefs != nullptr, false, "prefs is nullptr");

    string versionKey = UPGRADE_VALUE_MAP.at(version);
    int32_t upgradeStatus = prefs->GetInt(versionKey, UPGRADE_STATUS::UNKNOWN);
    MEDIA_INFO_LOG("HasUpgraded current version:%{public}s, current upgradeStatus: %{public}d",
        versionKey.c_str(), upgradeStatus);
    if (upgradeStatus == UPGRADE_STATUS::UNKNOWN) {
        return false;
    }

    return isSync ? (upgradeStatus == UPGRADE_STATUS::SYNC || upgradeStatus == UPGRADE_STATUS::ALL) :
        (upgradeStatus == UPGRADE_STATUS::ASYNC || upgradeStatus == UPGRADE_STATUS::ALL);
}

void RdbUpgradeUtils::SetUpgradeStatus(int32_t version, bool isSync)
{
    if (UPGRADE_VALUE_MAP.find(version) == UPGRADE_VALUE_MAP.end()) {
        MEDIA_INFO_LOG("upgrade map value not exist, version: %{public}d", version);
        return;
    }
    int32_t errCode = 0;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(RDB_UPGRADE_EVENT, errCode);
    MEDIA_INFO_LOG("rdb_upgrade_events prefs errCode: %{public}d", errCode);
    CHECK_AND_RETURN_LOG(prefs != nullptr, "prefs is nullptr");

    string versionKey = UPGRADE_VALUE_MAP.at(version);
    int32_t currentStatus = prefs->GetInt(versionKey, UPGRADE_STATUS::UNKNOWN);
    MEDIA_INFO_LOG("SetUpgradeStatus current version:%{public}s, current upgradeStatus: %{public}d",
        versionKey.c_str(), currentStatus);

    int32_t nextStatus = UPGRADE_STATUS::UNKNOWN;
    switch (currentStatus) {
        case UPGRADE_STATUS::UNKNOWN:
            nextStatus = isSync ? UPGRADE_STATUS::SYNC : UPGRADE_STATUS::ASYNC;
            break;
        case UPGRADE_STATUS::SYNC:
            nextStatus = isSync ? UPGRADE_STATUS::SYNC : UPGRADE_STATUS::ALL;
            break;
        case UPGRADE_STATUS::ASYNC:
            nextStatus = isSync ? UPGRADE_STATUS::ALL : UPGRADE_STATUS::ASYNC;
            break;
        case UPGRADE_STATUS::ALL:
            nextStatus = isSync ? UPGRADE_STATUS::ALL : UPGRADE_STATUS::ALL;
            break;
        default:
            break;
    }

    prefs->PutInt(versionKey, nextStatus);
    prefs->FlushSync();
    MEDIA_INFO_LOG("version %{public}s set to: %{public}d", versionKey.c_str(), nextStatus);
}

void RdbUpgradeUtils::AddMapValueToPreference()
{
    int32_t errCode = 0;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(RDB_UPGRADE_EVENT, errCode);
    MEDIA_INFO_LOG("rdb_upgrade_events prefs errCode: %{public}d", errCode);
    CHECK_AND_RETURN_LOG(prefs != nullptr, "prefs is nullptr");
    for (auto& pair : UPGRADE_VALUE_MAP) {
        prefs->PutInt(pair.second, UPGRADE_STATUS::ALL);
    }
    prefs->FlushSync();
}

void RdbUpgradeUtils::AddUpgradeDfxMessages(int32_t version, int32_t index, int32_t error)
{
    if (error == NativeRdb::E_OK) {
        return;
    }
    MEDIA_INFO_LOG("[add dfx messages] version: %{public}d, index: %{public}d, error: %{public}d",
        version, index, error);
    string message = to_string(version) + "_" + to_string(index) + "_" + to_string(error);
    UPGRADE_DFX_MESSAGES.emplace_back(message);
}

void RdbUpgradeUtils::ReportUpgradeDfxMessages(int64_t startTime, int32_t srcVersion,
    int32_t dstVersion, bool isSync)
{
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    UpgradeExceptionInfo reportData;
    reportData.srcVersion = srcVersion;
    reportData.dstVersion = dstVersion;
    reportData.duration = endTime - startTime;
    reportData.isSync = isSync;
    string exceptionVersions = std::accumulate(UPGRADE_DFX_MESSAGES.begin(),
        UPGRADE_DFX_MESSAGES.end(),
        std::string(),
        [](std::string a, const std::string &b) {
            if (!a.empty()) {
                a += ",";
            }
            a += b;
            return a;
        });
    reportData.exceptionVersions = exceptionVersions.substr(0, UPGRADE_EXCEPTION_VERSIONS_STR_LIMIT);
    DfxManager::GetInstance()->HandleUpgradeFault(reportData);
    UPGRADE_DFX_MESSAGES.clear();
}
} // namespace Media
} // namespace OHOS
