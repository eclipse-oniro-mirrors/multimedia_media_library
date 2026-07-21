/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
 
#include "media_video_mode_task.h"
 
#include "rdb_predicates.h"
#include "preferences.h"
#include "preferences_helper.h"
#include "dfx_utils.h"
#include "media_file_utils.h"
#include "medialibrary_subscriber.h"
#include "media_log.h"
#include "photo_video_mode_operation.h"
#include "result_set_utils.h"
#include "metadata.h"
#include "metadata_extractor.h"
#ifdef MEDIALIBRARY_LAKE_SUPPORT
#include "file_scan_utils.h"
#endif
#include "directory_ex.h"
#if defined(MEDIALIBRARY_FILE_MGR_SUPPORT) || defined(MEDIALIBRARY_LAKE_SUPPORT)
#include "media_file_access_utils.h"
#endif

using namespace OHOS::NativeRdb;
 
namespace OHOS::Media::Background {
 
static const int32_t batchSize = 100;
static const int32_t defaultValueZero = 0;
static const int32_t prefsNullErrCode = -1;
 
bool MediaVideoModeTask::Accept()
{
    return MedialibrarySubscriber::IsCurrentStatusOn();
}
 
void MediaVideoModeTask::Execute()
{
    this->HandleMediaFileManagerVideoMode();
    return;
}
 
void MediaVideoModeTask::SetBatchStatus(int32_t startFileId)
{
    MEDIA_INFO_LOG("MediaVideoModeTask::SetBatchStatus start");
    int32_t errCode = 0;
    std::shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(FILE_MANAGER_VIDEO_MODE_EVENT, errCode);
    MEDIA_INFO_LOG("file_manager_video_mode_events prefs errCode: %{public}d", errCode);
    CHECK_AND_RETURN_LOG(prefs != nullptr, "prefs is nullptr");
    prefs->PutInt("startFileId", startFileId);
    prefs->FlushSync();
    MEDIA_INFO_LOG("startFileId set to: %{public}d", startFileId);
}
 
int32_t MediaVideoModeTask::GetBatchStatus()
{
    MEDIA_INFO_LOG("MediaVideoModeTask::GetBatchStatus start");
    int32_t errCode = 0;
    std::shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(FILE_MANAGER_VIDEO_MODE_EVENT, errCode);
    MEDIA_INFO_LOG("file_manager_temp_file_aging_events prefs errCode: %{public}d", errCode);
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, prefsNullErrCode, "prefs is nullptr");
    int32_t defaultVal = 0;
    int32_t currStartFileId = prefs->GetInt("startFileId", defaultVal);
    MEDIA_INFO_LOG("currStartFileId is %{public}d", currStartFileId);
    return currStartFileId;
}
 
VideoModeInfo MediaVideoModeTask::QueryFiles(std::shared_ptr<MediaLibraryRdbStore> &rdbStore, int32_t startFileId)
{
    VideoModeInfo videoModeInfo;
    std::string updateVideoModeSql = "SELECT file_id, data FROM Photos WHERE";
    updateVideoModeSql += " sync_status = 0 AND clean_flag = 0 AND time_pending = 0 AND is_temp = 0"
                          " AND media_type = 2 AND file_id BETWEEN " +
                          std::to_string(startFileId) + " AND " + std::to_string(startFileId + batchSize);
    updateVideoModeSql += " AND position != " + std::to_string(static_cast<int32_t>(PhotoPositionType::CLOUD));
    MEDIA_DEBUG_LOG("HandleMediaFileManagerVideoMode sql = %{public}s", updateVideoModeSql.c_str());
    std::shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(updateVideoModeSql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, {}, "Failed to query batch selected files!");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        videoModeInfo.fileIds.emplace_back(fileId);
        std::string filePath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
        videoModeInfo.filePaths.emplace_back(filePath);
    }
    resultSet->Close();
    return videoModeInfo;
}
 
void MediaVideoModeTask::UpdateVideoMode(std::shared_ptr<MediaLibraryRdbStore> &rdbStore,
                                         const VideoModeInfo &videoModeInfo)
{
    std::vector<std::string> logFileIds;
    for (size_t i = 0; i < videoModeInfo.fileIds.size(); ++i) {
        const int32_t fileId = videoModeInfo.fileIds[i];          // 第 i 个 fileId
        const std::string filePath = videoModeInfo.filePaths[i];  // 对应第 i 个 path
        MEDIA_DEBUG_LOG("UpdateVideoMode fileId = %{public}d, filePath = %{public}s",
            fileId,
            MediaFileUtils::DesensitizePath(filePath).c_str());
        unique_ptr<Metadata> videoModeData = make_unique<Metadata>();
#if defined(MEDIALIBRARY_FILE_MGR_SUPPORT) || defined(MEDIALIBRARY_LAKE_SUPPORT)
        string realPath = MediaFileAccessUtils::GetAssetRealPath(filePath);
        string absVideoPath;
        if (!PathToRealPath(realPath, absVideoPath)) {
            MEDIA_ERR_LOG(
                "file is not real path, file path: %{public}s", MediaFileUtils::DesensitizePath(realPath).c_str());
            continue;
        }
        videoModeData->SetFilePath(realPath);
#else
        videoModeData->SetFilePath(filePath);
#endif
        int32_t err = MetadataExtractor::ExtractAVMetadata(videoModeData);
        if (err != E_OK) {
            MEDIA_ERR_LOG("Failed to extract metadata: %{public}s", DfxUtils::GetSafePath(filePath).c_str());
            continue;
        }
        int32_t videoMode = videoModeData->GetVideoMode();
        MEDIA_DEBUG_LOG("HandleMediaFileManagerVideoMode videoMode=%{public}d", videoMode);
        if (videoMode == static_cast<int32_t>(VideoMode::LOG_VIDEO)) {
            logFileIds.push_back(std::to_string(fileId));
        }
    }
    auto ret = PhotoVideoModeOperation::BatchUpdatePhotosVideoMode(rdbStore, logFileIds);
    CHECK_AND_RETURN_LOG(ret == NativeRdb::E_OK,
        "Failed to UpdatePhotosVideoMode, ret: %{public}d", ret);
}
 
void MediaVideoModeTask::HandleMediaFileManagerVideoMode()
{
    static const int32_t batchSize = 100;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "rdbstore is nullptr");
    int32_t maxFileId = PhotoVideoModeOperation::GetMaxFileId(rdbStore);
    MEDIA_INFO_LOG("HandleMediaFileManagerVideoMode maxFileId = %{public}d", maxFileId);
    CHECK_AND_EXECUTE(MediaFileUtils::IsFileExists(FILE_MANAGER_VIDEO_MODE_EVENT),
        SetBatchStatus(defaultValueZero));
    int32_t currStartFileId = GetBatchStatus();
    CHECK_AND_RETURN_LOG(currStartFileId != prefsNullErrCode, "prefs is nullptr");
    int32_t startFileId = currStartFileId == defaultValueZero ? 1 : currStartFileId;
    while (startFileId <= maxFileId) {
        MEDIA_DEBUG_LOG("kaishi houtai up");
        if (!this->Accept()) {
            MEDIA_ERR_LOG("FileManagerVideoMode check condition failed End");
            SetBatchStatus(startFileId);
            return;
        }
        VideoModeInfo videoModeInfo = QueryFiles(rdbStore, startFileId);
        MEDIA_INFO_LOG("videoModeInfo size = %{public}d", static_cast<int>(videoModeInfo.fileIds.size()));
        UpdateVideoMode(rdbStore, videoModeInfo);
        startFileId += batchSize;
    }
    SetBatchStatus(maxFileId + 1);
}
}