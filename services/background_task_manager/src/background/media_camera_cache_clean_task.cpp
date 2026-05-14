/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "media_camera_cache_clean_task.h"

#include "media_column.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_type_const.h"
#include "medialibrary_subscriber.h"
#include "media_file_utils.h"
#include <sys/stat.h>

using namespace std;

namespace OHOS::Media::Background {
bool MediaCameraCacheCleanTask::Accept()
{
    return MedialibrarySubscriber::IsCurrentStatusOn();
}

void MediaCameraCacheCleanTask::Execute()
{
    MEDIA_DEBUG_LOG("Begin to MediaCameraCacheCleanTask");
    {
        std::unique_lock<std::mutex> taskLock(taskRunningMutex_, std::defer_lock);
        CHECK_AND_RETURN_WARN_LOG(taskLock.try_lock(), "HandleCameraCacheClean is running");
    }
    std::thread([this] {
        std::unique_lock<std::mutex> taskLock(taskRunningMutex_, std::defer_lock);
        CHECK_AND_RETURN_WARN_LOG(taskLock.try_lock(), "HandleCameraCacheClean thread is running");
        this->HandleCameraCacheClean();
    }).detach();
}

void MediaCameraCacheCleanTask::DelEnhanceFolderDirtyFile(const std::string &realPathToEnhanceDir,
    const std::string &fileName)
{
    time_t now = time(nullptr);
    constexpr double thresholdSeconds = 24 * 60 * 60; // 24 hours
    struct stat statInfo {};
    std::string file = realPathToEnhanceDir + SLASH_STR + fileName;
    if (stat(file.c_str(), &statInfo) != 0) {
        MEDIA_WARN_LOG("skip %{private}s, stat errno: %{public}d", file.c_str(), errno);
        return;
    }
    time_t timeModified = statInfo.st_mtime;
    double duration = difftime(now, timeModified); // diff in seconds
    MEDIA_INFO_LOG("DelEnhanceFolderDirtyFile fileName: %{public}s, duration: %{public}f", fileName.c_str(), duration);
    CHECK_AND_RETURN(duration >= thresholdSeconds);
    if (!MediaFileUtils::DeleteFile(file)) {
        MEDIA_ERR_LOG("Delete DirtyFile Failed %{private}s, errno: %{public}d", file.c_str(), errno);
    }
}

void MediaCameraCacheCleanTask::HandleCameraCacheClean()
{
    std::string enhanceDirCloudView = ROOT_MEDIA_CAMERA_CACHE_DIR + SLASH_STR + CAMERA_CACHE_ENHANCE_DIR_VALUES +
        SLASH_STR;
    std::vector<std::string> fileNameVec;
    MediaFileUtils::GetAllFileNameListUnderPath(enhanceDirCloudView, fileNameVec);
    for (const auto& fileName : fileNameVec) {
        DelEnhanceFolderDirtyFile(enhanceDirCloudView, fileName);
    }
    MEDIA_INFO_LOG("HandleCameraCacheClean End");
}
} // namespace OHOS::Media::Background
