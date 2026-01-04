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

#ifndef MEDIA_FUSE_HDC_OPERATIONS_H
#define MEDIA_FUSE_HDC_OPERATIONS_H
#define FUSE_USE_VERSION FUSE_MAKE_VERSION(3, 17)
#include <string>
#include <vector>
#include <set>
#include <memory>
#include <sys/stat.h>
#include <fuse.h>
#include "result_set.h"

namespace OHOS {
namespace Media {

class MediaFuseHdcOperations {
public:
    static int32_t GetArgs(const std::string &path, std::vector<std::string> &parts);
    static int32_t GetPathFromDisplayname(const std::string &displayName, int albumId, std::string &filePath);
    static bool IsImageOrVideoFile(const std::string &fileName);
    static int32_t Parse(const std::string &path, int32_t &albumId, std::string &filePath, std::string &displayName);
    static int32_t HandleMovingPhoto(std::string &filePath, std::string &displayName, int32_t albumId);
    static int32_t HandleFstat(const struct fuse_file_info *fi, struct stat *stbuf);
    static int32_t HandleRootOrPhoto(const char *path, struct stat *stbuf);
    static int32_t HandleLstat(const std::string &localPath, struct stat *stbuf);
    static int32_t HandlePhotoPath(const std::string &inputPath,
        int32_t &albumId, std::string &localPath, struct stat *stbuf);
    static int32_t HandleFilePath(const std::vector<std::string> &args, int32_t &albumId, std::string &localPath);
    static int32_t ConvertToLocalPhotoPath(const std::string &inputPath, std::string &output);
    static int32_t CreateFd(const std::string &displayName, const int32_t &albumId, int32_t &fd);
    static int32_t ScanFileByPath(const std::string &path);
    static int32_t ReadPhotoRootDir(void *buf, fuse_fill_dir_t filler, off_t offset);
    static int32_t ReadAlbumDir(const std::string &inputPath, void* buf, fuse_fill_dir_t filler, off_t offset);
    static int32_t DeletePhotoByFilePath(const std::string &filePath);
    static std::string JpgToMp4(const std::string& displayName);

private:
    static time_t GetAlbumMTime(const std::shared_ptr<NativeRdb::ResultSet>& resultSet);
    static void FillDirStat(struct stat *stbuf, time_t mtime = 0, const std::string& uniqueKey = "");
    static int32_t GetAlbumIdFromAlbumName(const std::string &name, int32_t &albumId);
    static int32_t GetFileIdFromPath(const std::string &filePath, std::string &fileId);
    static int32_t ExtractFileNameAndExtension(const std::string &input, std::string &outName, std::string &outExt);
    static bool IsMovingPhoto(int32_t subtype, int32_t effectMode);
    static int32_t HandleDirStat(const int32_t &albumId, struct stat *stbuf);
    static int32_t UpdatePhotoRdb(const std::string &displayName, const std::string &filePath);
    static bool FillDirectoryEntry(void* buf, fuse_fill_dir_t filler,
        const std::string& name, const std::string& fullPath, off_t nextoff);
    static std::shared_ptr<NativeRdb::ResultSet> QueryAlbumPhotos(const int32_t &albumId);
};
} // namespace Media
} // namespace OHOS

#endif // MEDIA_FUSE_HDC_OPERATIONS_H