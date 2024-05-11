/*
 * Copyright (C) 2021-2023 Huawei Device Co., Ltd.
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

#ifndef FRAMEWORKS_INNERKITSIMPL_MEDIA_LIBRARY_INCLUDE_MEDIA_FILE_UTILS_H_
#define FRAMEWORKS_INNERKITSIMPL_MEDIA_LIBRARY_INCLUDE_MEDIA_FILE_UTILS_H_

#include <memory>
#include <string>
#include <unordered_set>

#include "unique_fd.h"
#include "userfile_manager_types.h"

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))
EXPORT const std::string MOVING_PHOTO_URI_SPLIT = ";";
EXPORT const uint8_t MOVING_PHOTO_IMAGE_POS = 0;
EXPORT const uint8_t MOVING_PHOTO_VIDEO_POS = 1;
EXPORT const std::string MEDIA_FILEMODE_READONLY = "r";
EXPORT const std::string MEDIA_FILEMODE_WRITEONLY = "w";
EXPORT const std::string MEDIA_FILEMODE_READWRITE = "rw";
EXPORT const std::string MEDIA_FILEMODE_WRITETRUNCATE = "wt";
EXPORT const std::string MEDIA_FILEMODE_WRITEAPPEND = "wa";
EXPORT const std::string MEDIA_FILEMODE_READWRITETRUNCATE = "rwt";
EXPORT const std::string MEDIA_FILEMODE_READWRITEAPPEND = "rwa";
EXPORT const std::unordered_set<std::string> MEDIA_OPEN_MODES = {
    MEDIA_FILEMODE_READONLY,
    MEDIA_FILEMODE_WRITEONLY,
    MEDIA_FILEMODE_READWRITE,
    MEDIA_FILEMODE_WRITETRUNCATE,
    MEDIA_FILEMODE_WRITEAPPEND,
    MEDIA_FILEMODE_READWRITETRUNCATE,
    MEDIA_FILEMODE_READWRITEAPPEND
};

constexpr int VIRTUAL_ID_DIVIDER = 5;
constexpr int PHOTO_VIRTUAL_IDENTIFIER = 4;
constexpr int AUDIO_VIRTUAL_IDENTIFIER = 3;
constexpr int FILE_VIRTUAL_IDENTIFIER = 2;
constexpr int CAMERA_SHOT_KEY_SIZE = 30;

constexpr int64_t MSEC_TO_SEC = 1e3;
constexpr int64_t MSEC_TO_NSEC = 1e6;

enum EXPORT TrashType {
    NOT_TRASHED = 0,
    TRASHED_ASSET,
    TRASHED_DIR,
    TRASHED_DIR_CHILD
};

enum EXPORT PortraitPages : int32_t {
    UNFAVORITE_PAGE = 0,
    FIRST_PAGE,
    SECOND_PAGE,
    FAVORITE_PAGE
};
EXPORT const std::unordered_set<int32_t> PORTRAIT_PAGE_MODE = {
    PortraitPages::FIRST_PAGE,
    PortraitPages::SECOND_PAGE,
    PortraitPages::FAVORITE_PAGE,
    PortraitPages::UNFAVORITE_PAGE
};

/**
 * @brief Utility class for file operations
 *
 * @since 1.0
 * @version 1.0
 */
class MediaFileUtils {
public:
    EXPORT static bool IsFileExists(const std::string &fileName);
    EXPORT static bool IsDirEmpty(const std::string &path);
    EXPORT static bool CreateFile(const std::string &filePath);
    EXPORT static bool DeleteFile(const std::string &fileName);
    EXPORT static bool DeleteDir(const std::string &dirName);
    EXPORT static std::string GetFileName(const std::string &filePath);
    EXPORT static std::string GetParentPath(const std::string &path);
    EXPORT static std::string GetTitleFromDisplayName(const std::string &displayName);
    EXPORT static bool IsDirectory(const std::string &dirName, std::shared_ptr<int> errCodePtr = nullptr);
    EXPORT static bool MoveFile(const std::string &oldPath, const std::string &newPath);
    EXPORT static bool CopyFileUtil(const std::string &filePath, const std::string &newPath);
    EXPORT static bool WriteStrToFile(const std::string &filePath, const std::string &str);
    EXPORT static bool CopyFile(int32_t rfd, int32_t wfd);
    EXPORT static bool RenameDir(const std::string &oldPath, const std::string &newPath);
    EXPORT static bool CreateDirectory(const std::string &dirPath, std::shared_ptr<int> errCodePtr = nullptr);
    EXPORT static int32_t CheckAlbumName(const std::string &albumName);
    EXPORT static int32_t CheckDentryName(const std::string &dentryName);
    EXPORT static int32_t CheckDisplayName(const std::string &displayName);
    EXPORT static int32_t CheckFileDisplayName(const std::string &displayName);
    EXPORT static int32_t CheckRelativePath(const std::string &relativePath);
    EXPORT static void FormatRelativePath(std::string &relativePath);
    EXPORT static void GetRootDirFromRelativePath(const std::string &relativePath, std::string &rootDir);
    EXPORT static int64_t GetAlbumDateModified(const std::string &albumPath);
    EXPORT static int64_t UTCTimeSeconds();
    EXPORT static int64_t UTCTimeMilliSeconds();
    EXPORT static int64_t UTCTimeNanoSeconds();
    EXPORT static std::string GetIdFromUri(const std::string &uri);
    EXPORT static std::string GetNetworkIdFromUri(const std::string &uri);
    EXPORT static std::string UpdatePath(const std::string &path, const std::string &uri);
    EXPORT static MediaType GetMediaType(const std::string &filePath);
    EXPORT static std::string SplitByChar(const std::string &str, const char split);
    EXPORT static std::string GetExtensionFromPath(const std::string &path);
    EXPORT static int32_t OpenFile(const std::string &path, const std::string &mode,
        const std::string &clientbundleName = "");
    EXPORT static int32_t CreateAsset(const std::string &filePath);
    EXPORT static int32_t ModifyAsset(const std::string &oldPath, const std::string &newPath);
    EXPORT static int32_t OpenAsset(const std::string &filePath, const std::string &mode);
    EXPORT static int32_t CloseAsset(int32_t fd);
    EXPORT static std::string GetMediaTypeUri(MediaType mediaType);
    EXPORT static std::string GetMediaTypeUriV10(MediaType mediaType);
    EXPORT static bool CheckMode(const std::string &mode);
    EXPORT static size_t FindIgnoreCase(const std::string &str, const std::string &key);
    EXPORT static int64_t GetVirtualIdByType(int32_t id, MediaType type);
    EXPORT static double GetRealIdByTable(int32_t virtualId, const std::string &tableName);
    EXPORT static std::string GetVirtualUriFromRealUri(const std::string &uri, const std::string &extrUri = "");
    EXPORT static std::string GetRealUriFromVirtualUri(const std::string &uri);
    EXPORT static bool StartsWith(const std::string &str, const std::string &prefix);
    EXPORT static void UriAppendKeyValue(std::string &uri, const std::string &key, std::string value = "10");
    EXPORT static std::string GetExtraUri(const std::string &displayName, const std::string &path,
        const bool isNeedEncode = true);
    EXPORT static std::string GetUriByExtrConditions(const std::string &prefix, const std::string &fileId,
        const std::string &suffix = "");
    EXPORT static std::string Encode(const std::string &uri);
    EXPORT static bool CheckDisplayLevel(const int32_t &displayLevel);
    EXPORT static std::string GetHighlightPath(const std::string &uri);
#ifdef MEDIALIBRARY_COMPATIBILITY
    EXPORT static std::string GetTableFromVirtualUri(const std::string &uri);
#endif
    EXPORT static bool IsUriV10(const std::string &mediaType);
    EXPORT static bool IsFileTablePath(const std::string &path);
    EXPORT static bool IsPhotoTablePath(const std::string &path);
    EXPORT static std::string StrCreateTime(const std::string &format, int64_t time);
    EXPORT static std::string StrCreateTimeByMilliseconds(const std::string &format, int64_t time);
    EXPORT static std::string AddDocsToRelativePath(const std::string &relativePath);
    EXPORT static std::string RemoveDocsFromRelativePath(const std::string &relativePath);
    EXPORT static int64_t Timespec2Millisecond(const struct timespec &time);
    EXPORT static std::string GetMovingPhotoVideoPath(const std::string &imagePath);
    EXPORT static bool CheckMovingPhotoExtension(const std::string &extension);
    EXPORT static bool CheckMovingPhotoVideoExtension(const std::string &extension);
    EXPORT static bool CheckMovingPhotoImage(const std::string &path);
    EXPORT static bool CheckMovingPhotoVideo(const std::string &path);
    EXPORT static bool CheckMovingPhotoVideo(const UniqueFd &uniqueFd);
    EXPORT static bool CheckMovingPhotoVideoDuration(int32_t duration);
    EXPORT static bool GetFileSize(const std::string &filePath, size_t& size);
    EXPORT static bool SplitMovingPhotoUri(const std::string& uri, std::vector<std::string>& ret);
    EXPORT static bool IsMediaLibraryUri(const std::string& uri);

private:
    static bool Mkdir(const std::string &subStr, std::shared_ptr<int> errCodePtr);
    static int32_t RemoveDirectory(const std::string &path);
    static int32_t CheckStringSize(const std::string &str, const size_t max);
};
} // namespace OHOS::Media

#endif // FRAMEWORKS_INNERKITSIMPL_MEDIA_LIBRARY_INCLUDE_MEDIA_FILE_UTILS_H_
