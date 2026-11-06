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

#ifndef COMMON_UTILS_MEDIA_PATH_UTILS_H_
#define COMMON_UTILS_MEDIA_PATH_UTILS_H_

#include <cstdint>
#include <string>
#include <sys/stat.h>
#include <functional>

#include "userfile_manager_types.h"

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))

#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT
using GetForegroundAccountIdHandler = std::function<bool(int32_t&)>;
#endif

struct PathEntry {
    std::string rootDir;   // e.g. "/storage/cloud/"
    std::string midfix;    // e.g. "files/"，accountId 插在 midfix 与 bucket 之间
    std::string bucket;    // e.g. "Photo"、".thumbs/Photo"
};

enum class MediaPathType : int32_t {
    // cloud
    CLOUD_ROOT = 100000,               // "/storage/cloud/"
    CLOUD_FILES,                       // "/storage/cloud/files/<accountId>/"

    CLOUD_PHOTO = 110000,              // "/storage/cloud/files/<accountId>/Photo/"
    CLOUD_EDIT_DATA,                   // "/storage/cloud/files/<accountId>/.editData/"
    CLOUD_THUMB,                       // "/storage/cloud/files/<accountId>/.thumbs/"
    CLOUD_CACHE,                       // "/storage/cloud/files/<accountId>/.cache/"
    CLOUD_CUSTOM_RESTORE,              // "/storage/cloud/files/<accountId>/custom_restore/"
    CLOUD_AUDIO,                       // "/storage/cloud/files/<accountId>/Audio/"
    CLOUD_AUDIOS,                      // "/storage/cloud/files/<accountId>/Audios/"
    CLOUD_DOCS,                        // "/storage/cloud/files/<accountId>/Docs/"
    CLOUD_CAMERA,                      // "/storage/cloud/files/<accountId>/Camera/"
    CLOUD_VIDEOS,                      // "/storage/cloud/files/<accountId>/Videos/"
    CLOUD_PICTURES,                    // "/storage/cloud/files/<accountId>/Pictures/"
    CLOUD_BACK_UP,                     // "/storage/cloud/files/<accountId>/.backup/"

    CLOUD_EDIT_DATA_PHOTO = 120000,    // "/storage/cloud/files/<accountId>/.editData/Photo/"
    CLOUD_THUMB_PHOTO,                 // "/storage/cloud/files/<accountId>/.thumbs/Photo/"

    // local
    LOCAL_STORAGE_ROOT = 200000,       // "/storage/media/" (parent of LOCAL_ROOT, for userId injection)
    LOCAL_ROOT,                        // "/storage/media/local/"
    LOCAL_FILES,                       // "/storage/media/local/files/<accountId>/"

    LOCAL_PHOTO = 210000,              // "/storage/media/local/files/<accountId>/Photo/"
    LOCAL_EDIT_DATA,                   // "/storage/media/local/files/<accountId>/.editData/"
    LOCAL_THUMB,                       // "/storage/media/local/files/<accountId>/.thumbs/"
    LOCAL_CACHE,                       // "/storage/media/local/files/<accountId>/.cache/"
    LOCAL_CUSTOM_RESTORE,              // "/storage/media/local/files/<accountId>/custom_restore/"
    LOCAL_AUDIO,                       // "/storage/media/local/files/<accountId>/Audio/"
    LOCAL_AUDIOS,                      // "/storage/media/local/files/<accountId>/Audios/"
    LOCAL_DOCS,                        // "/storage/media/local/files/<accountId>/Docs/"
    LOCAL_CAMERA,                      // "/storage/media/local/files/<accountId>/Camera/"
    LOCAL_VIDEOS,                      // "/storage/media/local/files/<accountId>/Videos/"
    LOCAL_PICTURES,                    // "/storage/media/local/files/<accountId>/Pictures/"
    LOCAL_BACK_UP,                     // "/storage/media/local/files/<accountId>/.backup/"

    LOCAL_EDIT_DATA_PHOTO = 220000,    // "/storage/media/local/files/<accountId>/.editData/Photo/"
    LOCAL_THUMB_PHOTO,                 // "/storage/media/local/files/<accountId>/.thumbs/Photo/"
};

enum class AdaptAccount : int32_t {
    NO = 0,
    YES,
};

enum class AppendSlash : int32_t {
    NO = 0,
    YES,
};

enum class FileBucketType : int32_t {
    PHOTO = 0,
    EDIT_DATA,
    THUMB,
    CACHE,
    CUSTOM_RESTORE,
    AUDIO,
    AUDIOS,
    DOCS,
    CAMERA,
    VIDEOS,
    PICTURES,
    BACK_UP,
};

enum class ThumbnailFileType : int32_t {
    LCD = 0,
    LCD_EX = 1,
};

/**
 * media path utils which the ability process path
 */
class MediaPathUtils {
public:
    EXPORT MediaPathUtils();
    EXPORT ~MediaPathUtils();

    /**
     * @brief Get the directory path for the specified path type
     * @param pathType The path type enum combining dir type and bucket type
     * @param adaptAccount Whether to append the foreground account id
     * @param appendSlash Whether to append trailing slash
     * @return The full directory path, or empty string if pathType is invalid
     */
    EXPORT static std::string GetDir(MediaPathType pathType,
        AdaptAccount adaptAccount = AdaptAccount::NO,
        AppendSlash appendSlash = AppendSlash::YES);
    /**
     * @brief Check if the path starts with the files directory for the specified path type
     */
    EXPORT static bool CheckFilesPath(MediaPathType pathType, const std::string& path,
        AdaptAccount adaptAccount = AdaptAccount::NO);
    /**
     * @brief Insert userId before accountId in the cloud path
     */
    EXPORT static std::string AppendUserId(const std::string& path, int32_t userId);

    /**
     * @brief Get the local thumbnail file path for the specified photo path
     */
    EXPORT static std::string GetLocalThumbnailFilePath(const std::string &photoPath,
        ThumbnailFileType thumbnailFileType);

    /**
     * @brief Get the bucket suffix string for the specified bucket type
     */
    EXPORT static std::string GetBucketFix(FileBucketType bucketType);
    /**
     * @brief Get the thumbnail file suffix for the specified thumbnail type
     */
    EXPORT static std::string GetThumbnailFileFix(ThumbnailFileType thumbnailFileType);

    /**
     * @brief Desensitize a cloud files path by replacing the cloud files directory prefix with "*"
     */
    EXPORT static std::string DesensitizePath(const std::string &path);
    /**
     * @brief Print stat information (uid, gid, mode) of the specified path
     */
    EXPORT static void PrintStatInformation(const std::string& path);
    /**
     * @brief Get the parent directory path of the given path
     */
    EXPORT static std::string GetParentPath(const std::string &path);
    EXPORT static std::string GetFileName(const std::string &filePath);
    EXPORT static std::string GetExtension(const std::string &path);
    EXPORT static bool CheckPathTraversal(const std::string& path);
    EXPORT static bool CheckIsCloudFile(const std::string &sandboxPath);
    /**
     * @brief Convert a cloud path to a local path by replacing the root prefix
     */
    EXPORT static std::string ConvertCloudPathToLocalPath(const std::string &cloudPath);
    /**
     * @brief Check if the photo path is a valid cloud file path
     */
    EXPORT static bool CheckPhotoPath(const std::string& photoPath);

    /**
     * @brief Convert a local path to a cloud path by replacing the root prefix
     */
    EXPORT static std::string ConvertLocalPathToCloudPath(const std::string &localPath);

    EXPORT static std::string AdaptPreferencesPathForAllDevice(const std::string& xmlPath, bool addAccount = false);

    /**
     * @brief Get the root preferences directory "/data/storage/el2/base/preferences/"
     */
    EXPORT static std::string GetPreferencesDir();

#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT
    /**
     * @brief Register a handler for retrieving the foreground account id
     */
    EXPORT static void RegisterGetAccountId(GetForegroundAccountIdHandler handler);
    /**
     * @brief Get the foreground account id via the registered handler
     */
    EXPORT static bool GetForegroundAccountId(int32_t& accountId);
    /**
     * @brief Remove the accountId segment from a media path
     */
    EXPORT static std::string RemoveAccountId(const std::string& path);
    /**
     * @brief Insert accountId before the "files/" segment in a media path
     */
    EXPORT static std::string AppendAccountId(const std::string& path, int32_t accountId = -1);
    /**
     * @brief Check if the accountId in the path matches the specified account
     */
    EXPORT static bool CheckAccountId(const std::string& path, int32_t accountId = -1);
    /**
     * @brief Parse accountId from a media path (segment after /files/).
     *        Returns INVALID_ACCOUNT_ID (-1) if not found or invalid.
     */
    EXPORT static int32_t ParseAccountIdFromPath(const std::string& path);
#endif

    /**
     * @brief Desensitize a file name by masking characters in the middle
     */
    EXPORT static std::string DesensitizeName(const std::string &name);
    /**
     * @brief Copy a directory recursively
     */
    EXPORT static int32_t CopyDirectory(const std::string &srcDir, const std::string &dstDir);
    /**
     * @brief Check if the file is a transcode file
     */
    EXPORT static bool IsTranscodeFile(const std::string &filePath);

private:
    /**
     * @brief Check if the preference path starts with "/data/storage/el2/base/preferences/"
     */
    EXPORT static bool CheckPreferencePath(const std::string& xmlPath);

#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT
    static GetForegroundAccountIdHandler handler_;
#endif
};
} // namespace OHOS::Media

#endif // COMMON_UTILS_MEDIA_PATH_UTILS_H_
