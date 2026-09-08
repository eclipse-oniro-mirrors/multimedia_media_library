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
    std::string updateVideoModeSql = "SELECT * FROM Photos WHERE video_mode = ";
    updateVideoModeSql += std::to_string(static_cast<int32_t>(VideoMode::DEFAULT));
    updateVideoModeSql +=
        " AND file_id BETWEEN " + std::to_string(startFileId) + " AND " + std::to_string(startFileId + batchSize);
    updateVideoModeSql += " AND position != " + std::to_string(static_cast<int32_t>(PhotoPositionType::CLOUD));
    MEDIA_INFO_LOG("HandleMediaFileManagerVideoMode sql=%{public}s", updateVideoModeSql.c_str());
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
    std::vector<std::string> noLogFileIds;
    std::vector<std::string> LogFileIds;
    for (size_t i = 0; i < videoModeInfo.fileIds.size(); ++i) {
        const int32_t fileId = videoModeInfo.fileIds[i];
        const std::string filePath = videoModeInfo.filePaths[i];
        MEDIA_INFO_LOG("UpdateVideoMode fileId = %{public}d, filePath = %{public}s", fileId, filePath.c_str());
        unique_ptr<Metadata> videoModeData = make_unique<Metadata>();
        videoModeData->SetFilePath(filePath);
        int32_t err = MetadataExtractor::ExtractAVMetadata(videoModeData);
        if (err != E_OK) {
            MEDIA_ERR_LOG("Failed to extract metadata: %{public}s", DfxUtils::GetSafePath(filePath).c_str());
            continue;
        }
        int32_t videoMode = videoModeData->GetVideoMode();
        MEDIA_INFO_LOG("HandleMediaFileManagerVideoMode videoMode=%{public}d", videoMode);
        if (videoMode == static_cast<int32_t>(VideoMode::NOT_LOG_VIDEO)) {
            noLogFileIds.push_back(std::to_string(fileId));
        } else {
            LogFileIds.push_back(std::to_string(fileId));
        }
    }
    auto ret = PhotoVideoModeOperation::BatchUpdatePhotosVideoMode(rdbStore, noLogFileIds, LogFileIds);
    CHECK_AND_RETURN_LOG(ret == E_OK, "Failed to UpdatePhotosVideoMod, ret: %{public}d", ret);
}

void MediaVideoModeTask::HandleMediaFileManagerVideoMode()
{
    static const int32_t batchSize = 100;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "rdbStore is nullptr");
    int32_t maxFileId = PhotoVideoModeOperation::GetMaxFileId(rdbStore);
    CHECK_AND_EXECUTE(MediaFileUtils::IsFileExists(FILE_MANAGER_VIDEO_MODE_EVENT),
        SetBatchStatus(defaultValueZero));
    int32_t currStartFileId = GetBatchStatus();
    CHECK_AND_RETURN_LOG(currStartFileId != prefsNullErrCode, "prefs is nullptr");
    int32_t startFileId = currStartFileId == defaultValueZero ? 1 : currStartFileId;
    while (startFileId <= maxFileId) {
        MEDIA_INFO_LOG("kaishi houtai up");
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