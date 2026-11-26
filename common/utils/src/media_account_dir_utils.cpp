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

#define MLOG_TAG "MediaAccountDirUtils"

#include "media_account_dir_utils.h"

#include <unistd.h>

#include <cerrno>
#include <ftw.h>
#include <memory>
#include <sstream>

#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT

#include "media_log.h"
#include "media_path_utils.h"
#include "media_pure_file_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_type_const.h"

namespace OHOS {
namespace Media {
const std::vector<std::string> PRESET_SUB_DIRS = {
    CAMERA_DIR_VALUES, VIDEO_DIR_VALUES, PIC_DIR_VALUES, AUDIO_DIR_VALUES, AUDIO_BUCKET + SLASH_STR,
    PHOTO_BUCKET + SLASH_STR, BACKUP_DATA_DIR_VALUE,
    EDIT_DATA_DIR_VALUE + SLASH_STR, BACKUP_SINGLE_DATA_DIR_VALUE, CACHE_DIR_VALUE,
    CUSTOM_RESTORE_VALUES, CAMERA_CACHE_DIR_VALUES,
    CAMERA_CACHE_DIR_VALUES + CAMERA_CACHE_ENHANCE_DIR_VALUES + SLASH_STR,
    CAMERA_CACHE_DIR_VALUES + CAMERA_CACHE_TEMP_DIR_VALUES + SLASH_STR,
    std::string(".meta/") + THUMB_DIR_VALUE,
};

static std::vector<std::string> GetMediaLibraryRootPaths()
{
    std::vector<std::string> paths;

    std::string cloudBase = MediaPathUtils::GetDir(MediaPathType::CLOUD_FILES, AdaptAccount::YES);
    paths.emplace_back(cloudBase);

    std::string localBase = MediaPathUtils::GetDir(MediaPathType::LOCAL_FILES, AdaptAccount::YES);
    paths.emplace_back(localBase);
    return paths;
}

bool MediaAccountDirUtils::CheckRootDirsReady(int32_t accountId)
{
    MEDIA_INFO_LOG("enter, accountId=%{public}d", accountId);
    const auto& paths = GetMediaLibraryRootPaths();

    // Check and create root dirs
    for (const auto& rootDir : paths) {
        if (rootDir.empty()) {
            MEDIA_ERR_LOG("empty rootDir for accountId=%{public}d", accountId);
            return false;
        }
        if (access(rootDir.c_str(), F_OK) != 0) {
            // Root dir not exist, create it
            if (!MediaPureFileUtils::CreateDirectory(rootDir)) {
                MEDIA_ERR_LOG("failed to create root dir, path=%{public}s", rootDir.c_str());
                return false;
            }
        }

        // Check and create preset sub-dirs
        for (const auto& subDir : PRESET_SUB_DIRS) {
            std::string fullDir = rootDir + subDir;
            if (access(fullDir.c_str(), F_OK) != 0) {
                if (!MediaPureFileUtils::CreateDirectory(fullDir)) {
                    MEDIA_ERR_LOG("failed to create subDir, subDir=%{public}s", subDir.c_str());
                    return false;
                }
            }
        }
    }

    MEDIA_INFO_LOG("all dirs ready for accountId=%{public}d", accountId);
    return true;
}
}  // namespace Media
}  // namespace OHOS

#endif  // MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT
