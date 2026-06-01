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

#include "media_share_dirty_data_cleaner.h"

#include <sstream>

#include "preferences_helper.h"
#include "rdb_predicates.h"

#include "media_log.h"
#include "custom_restore_const.h"
#include "media_time_utils.h"
#include "medialibrary_unistore_manager.h"
#include "media_column.h"
#include "media_pure_file_utils.h"
#include "moving_photo_file_utils.h"
#include "media_edit_utils.h"
#include "medialibrary_errno.h"
#include "dfx_utils.h"

using namespace std;

namespace OHOS::Media {
static const int32_t MEDIA_ID_INDEX = 0;
static const int32_t MEDIA_FILE_PATH_INDEX = 1;

std::atomic<bool> MediaShareDirtyDataCleaner::isSharing_ = false;

void MediaShareDirtyDataCleaner::CheckDirtyData()
{
    int64_t lastShareTime = MediaTimeUtils::UTCTimeMilliSeconds();
    bool isNeedClean = IsNeedClean(lastShareTime) && !isSharing_;
    if (isNeedClean) {
        MEDIA_INFO_LOG("start clean share dirty data");
        CleanDirtyData(lastShareTime);
        UpdateCleanFlag(false);
        UpdateShareTime(false);
        MEDIA_INFO_LOG("end clean share dirty data");
    } else {
        MEDIA_INFO_LOG("no need clean, isSharing_: %{public}d", static_cast<int32_t> (isSharing_));
    }
}

bool MediaShareDirtyDataCleaner::UpdateCleanFlag(bool isNeedClean)
{
    int32_t errCode;
    std::shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(SHARE_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get shared preferences error: %{public}d", errCode);
        return false;
    }
    if (isNeedClean) {
        prefs->PutBool(CLEAN_FLAG, true);
    } else {
        prefs->Delete(CLEAN_FLAG);
    }
    return prefs->FlushSync();
}

bool MediaShareDirtyDataCleaner::UpdateShareTime(bool isStartShare)
{
    int32_t errCode;
    std::shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(SHARE_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get shared preferences error: %{public}d", errCode);
        return false;
    }
    if (isStartShare) {
        prefs->PutLong(LAST_SHARE_TIME, MediaTimeUtils::UTCTimeMilliSeconds());
    } else {
        prefs->Delete(LAST_SHARE_TIME);
    }
    return prefs->FlushSync();
}

bool MediaShareDirtyDataCleaner::IsNeedClean(int64_t &lastShareTime)
{
    int32_t errCode;
    std::shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(SHARE_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get shared preferences error: %{public}d", errCode);
        return false;
    }
    bool cleanFlag = prefs->GetBool(CLEAN_FLAG, false);
    if (cleanFlag) {
        lastShareTime = prefs->GetLong(LAST_SHARE_TIME, MediaTimeUtils::UTCTimeMilliSeconds());
    }
    return cleanFlag;
}

void MediaShareDirtyDataCleaner::CleanDirtyData(int64_t lastShareTime)
{
    std::unordered_map<int32_t, std::string> dirtyDataMap = GetDirtyData(lastShareTime);
    MEDIA_INFO_LOG("%{public}d dirty data need clean", (int32_t)dirtyDataMap.size());
    std::vector<std::string> deletedFileIds;
    for (auto &data : dirtyDataMap) {
        DeleteFiles(data.second);
        deletedFileIds.push_back(to_string(data.first));
    }
    if (deletedFileIds.empty()) {
        MEDIA_INFO_LOG("no share dirty data need clean");
        return;
    }
    DeleteDb(deletedFileIds);
}

void MediaShareDirtyDataCleaner::SetSharingState(bool isSharing)
{
    isSharing_ = isSharing;
}

bool MediaShareDirtyDataCleaner::GetSharingState()
{
    return isSharing_;
}

std::unordered_map<int32_t, std::string> MediaShareDirtyDataCleaner::GetDirtyData(int64_t lastShareTime)
{
    std::unordered_map<int32_t, std::string> result;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, result, "fail to get rdbstore");
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_OWNER_PACKAGE, SHARE_PACKAGE_NAME);
    predicates.EqualTo(MediaColumn::MEDIA_TIME_PENDING, -1);
    predicates.GreaterThan(MediaColumn::MEDIA_DATE_ADDED, lastShareTime);
    vector<string> columns = { MediaColumn::MEDIA_ID, MediaColumn::MEDIA_FILE_PATH };
    auto resultSet = rdbStore->Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, result, "GetDirtyData resultSet is null");
    int32_t err;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string path;
        int32_t fileId;
        CHECK_AND_CONTINUE_ERR_LOG(resultSet->GetInt(MEDIA_ID_INDEX, fileId) == NativeRdb::E_OK,
            "Can not get column %{public}s value", MediaColumn::MEDIA_ID.c_str());
        CHECK_AND_CONTINUE_ERR_LOG(resultSet->GetString(MEDIA_FILE_PATH_INDEX, path) == NativeRdb::E_OK,
            "Can not get column %{public}s value", MediaColumn::MEDIA_FILE_PATH.c_str());

        result[fileId] = path;
    }
    return result;
}

bool MediaShareDirtyDataCleaner::DeleteDb(const std::vector<std::string> &fileIds)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "fail to get rdbstore");
    NativeRdb::RdbPredicates photosPredicates(PhotoColumn::PHOTOS_TABLE);
    photosPredicates.In(MediaColumn::MEDIA_ID, fileIds);
    int32_t deletePhotosRows = 0;
    int32_t photosErr = rdbStore->Delete(deletePhotosRows, photosPredicates);
    NativeRdb::RdbPredicates photosExtPredicates(PhotoExtColumn::PHOTOS_EXT_TABLE);
    photosExtPredicates.In(PhotoExtColumn::PHOTO_ID, fileIds);
    int32_t deletePhotosExtRows = 0;
    int32_t photosExtErr = rdbStore->Delete(deletePhotosExtRows, photosExtPredicates);
    MEDIA_INFO_LOG("delete %{public}d photo, result: %{public}d, %{public}d photoExt, result: %{public}d",
        deletePhotosRows, photosErr, deletePhotosExtRows, photosExtErr);
    return photosErr == NativeRdb::E_OK && photosExtErr == NativeRdb::E_OK;
}

void MediaShareDirtyDataCleaner::DeleteFiles(const std::string &path)
{
    DeleteFileWithCheck(path);
    DeleteFileWithCheck(MovingPhotoFileUtils::GetMovingPhotoVideoPath(path));
    DeleteFileWithCheck(MediaEditUtils::GetEditDataPath(path));
    DeleteFileWithCheck(MediaEditUtils::GetEditDataCameraPath(path));
    DeleteFileWithCheck(MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(path));
    DeleteFileWithCheck(MediaEditUtils::GetEditDataSourcePath(path));
    DeleteFileWithCheck(MediaEditUtils::GetEditDataSourceBackPath(path));
    DeleteFileWithCheck(MovingPhotoFileUtils::GetSourceMovingPhotoVideoPath(path));
    DeleteFileWithCheck(MovingPhotoFileUtils::GetSourceBackMovingPhotoVideoPath(path));
}

void MediaShareDirtyDataCleaner::DeleteFileWithCheck(const std::string &path)
{
    if (MediaPureFileUtils::IsFileExists(path)) {
        MediaPureFileUtils::DeleteFile(path);
        MEDIA_INFO_LOG("delete file %{public}s result: %{public}d", DfxUtils::GetSafePath(path).c_str(), errno);
    }
}
} // namespace OHOS::Media
