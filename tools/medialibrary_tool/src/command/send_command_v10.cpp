/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#define MLOG_TAG "MediatoolSend"
#include "command/send_command_v10.h"

#include <algorithm>
#include <chrono>
#include <dirent.h>
#include <fcntl.h>
#include <map>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

#include "constant.h"
#include "directory_ex.h"
#include "exec_env.h"
#include "image_source.h"
#include "media_file_uri.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "medialibrary_napi_utils.h"
#include "mediatool_uri.h"
#include "moving_photo_file_utils.h"
#include "unique_fd.h"
#include "userfile_client_ex.h"
#include "userfilemgr_uri.h"

namespace OHOS {
namespace Media {
namespace MediaTool {
static constexpr int32_t SLEEP_NOTIFY_TIME = 2000;

struct FileInfo {
    Media::MediaType mediaType {Media::MediaType::MEDIA_TYPE_DEFAULT};
    std::string displayName;
    std::string path;
    std::string uri;
    int32_t id {-1};
    bool toBeRemove {false};
    int32_t result {-1};
    bool isContentValid {true};
    bool isMovingPhoto {false};
    bool hasSourceStat {false};
    std::string mimeType;
    std::string videoPath;
    int64_t durationMs {0};
    int32_t movingPhotoVersion {0};
    struct stat sourceStat {};
    struct stat videoSourceStat {};
    [[nodiscard]] std::string ToStr() const
    {
        std::string str;
        str.append("mediaType:");
        str.append(std::to_string(static_cast<uint32_t>(mediaType)));
        str.append(", displayName:");
        str.append(displayName);
        str.append(", path:");
        str.append(path);
        str.append(", uri:");
        str.append(uri);
        str.append(", id:");
        str.append(std::to_string(id));
        return str;
    }
};

static inline int32_t GetFileInfo(const ExecEnv &env, const std::string &path, std::vector<FileInfo> &fileInfos)
{
    FileInfo fileInfo;
    fileInfo.displayName = ExtractFileName(path);
    fileInfo.mediaType = MediaFileUtils::GetMediaType(fileInfo.displayName);
    fileInfo.path = path;
    fileInfo.result = Media::E_ERR;
    auto mediaTypes = UserFileClientEx::GetSupportTypes();
    for (auto mediaType : mediaTypes) {
        if (mediaType != fileInfo.mediaType) {
            continue;
        }
        fileInfos.push_back(fileInfo);
        return Media::E_OK;
    }
    return Media::E_ERR;
}

static int32_t GetDirInfo(const ExecEnv &env, const std::string &path, std::vector<FileInfo> &fileInfos)
{
    std::vector<std::string> files;
    GetDirFiles(path, files);
    if (!files.size() && errno) {
        printf("%s can not get dir files. errno:%d\n", STR_FAIL.c_str(), errno);
    }
    for (auto &file : files) {
        auto ret = GetFileInfo(env, file, fileInfos);
        if (ret != Media::E_OK) {
            printf("%s get dir information failed. ret:%d, file:%s\n", STR_FAIL.c_str(), ret, file.c_str());
            return ret;
        }
    }
    return Media::E_OK;
}

static bool IsSameSource(const struct stat &expected, const struct stat &actual)
{
    return S_ISREG(actual.st_mode) && expected.st_dev == actual.st_dev && expected.st_ino == actual.st_ino &&
        expected.st_size == actual.st_size && expected.st_mtim.tv_sec == actual.st_mtim.tv_sec &&
        expected.st_mtim.tv_nsec == actual.st_mtim.tv_nsec;
}

static bool IsSourceUnchanged(const std::string &path, const struct stat &expected)
{
    struct stat actual {};
    return lstat(path.c_str(), &actual) == 0 && IsSameSource(expected, actual);
}

static int32_t ReadMediaMetadata(FileInfo &fileInfo)
{
    std::string realPath;
    if (!fileInfo.hasSourceStat && !PathToRealPath(fileInfo.path, realPath)) {
        MEDIA_ERR_LOG("Get real path failed, name:%{private}s", fileInfo.displayName.c_str());
        return Media::E_ERR;
    }
    UniqueFd sourceFd(open(realPath.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW));
    if (sourceFd.Get() < 0 || fstat(sourceFd.Get(), &fileInfo.sourceStat) != 0 ||
        !S_ISREG(fileInfo.sourceStat.st_mode) || fileInfo.sourceStat.st_size <= 0) {
        MEDIA_ERR_LOG("Candidate source invalid, fd:%{public}d, errno:%{public}d, name:%{private}s",
            sourceFd.Get(), errno, fileInfo.displayName.c_str());
        if (fileInfo.mediaType == MediaType::MEDIA_TYPE_IMAGE) {
            fileInfo.isContentValid = false;
        }
        return Media::E_ERR;
    }
    fileInfo.hasSourceStat = true;
    fileInfo.mimeType = MediaFileUtils::GetMimeTypeFromDisplayName(fileInfo.displayName);
    if (fileInfo.mediaType == MediaType::MEDIA_TYPE_IMAGE) {
        SourceOptions options;
        uint32_t imageError = Media::E_OK;
        auto imageSource = ImageSource::CreateImageSource(sourceFd.Get(), options, imageError);
        if (imageSource == nullptr || imageError != Media::E_OK) {
            MEDIA_ERR_LOG("Create candidate image source failed, err:%{public}u, name:%{private}s",
                imageError, fileInfo.displayName.c_str());
            fileInfo.isContentValid = false;
            return Media::E_ERR;
        }
        ImageInfo imageInfo;
        imageError = imageSource->GetImageInfo(0, imageInfo);
        if (imageError != Media::E_OK || imageInfo.encodedFormat.empty()) {
            MEDIA_ERR_LOG("Candidate image format invalid, err:%{public}u, name:%{private}s",
                imageError, fileInfo.displayName.c_str());
            fileInfo.isContentValid = false;
            return Media::E_ERR;
        }
    } else if (fileInfo.mediaType == MediaType::MEDIA_TYPE_VIDEO) {
        fileInfo.durationMs = MovingPhotoFileUtils::GetMovingPhotoVideoDuration(sourceFd);
        if (fileInfo.durationMs < 0) {
            MEDIA_ERR_LOG("Read candidate video duration failed, ret:%{public}d, name:%{private}s",
                static_cast<int32_t>(fileInfo.durationMs), fileInfo.displayName.c_str());
            return Media::E_ERR;
        }
    }
    const int32_t sourceResult = IsSourceUnchanged(fileInfo.path, fileInfo.sourceStat) ? Media::E_OK : Media::E_ERR;
    if (sourceResult != Media::E_OK) {
        MEDIA_ERR_LOG("Candidate source changed during metadata read, name:%{private}s", fileInfo.displayName.c_str());
    }
    return sourceResult;
}

static std::string GetFileStem(const std::string &name)
{
    const auto position = name.rfind('.');
    return position == std::string::npos ? name : name.substr(0, position);
}

static size_t FindMovingPhotoImage(const std::vector<size_t> &candidates, std::vector<FileInfo> &fileInfos)
{
    for (const auto index : candidates) {
        auto &fileInfo = fileInfos[index];
        if (!fileInfo.isContentValid || !MediaFileUtils::CheckMovingPhotoImage(fileInfo.path) ||
            ReadMediaMetadata(fileInfo) != Media::E_OK) {
            continue;
        }
        return index;
    }
    return fileInfos.size();
}

static size_t FindMovingPhotoVideo(const std::vector<size_t> &candidates, std::vector<FileInfo> &fileInfos)
{
    for (const auto index : candidates) {
        auto &fileInfo = fileInfos[index];
        if (!fileInfo.isContentValid || !MediaFileUtils::CheckMovingPhotoVideoExtension(
            MediaFileUtils::GetExtensionFromPath(fileInfo.path)) || ReadMediaMetadata(fileInfo) != Media::E_OK) {
            continue;
        }
        if (MovingPhotoFileUtils::CheckMovingPhotoVideoDuration(static_cast<int32_t>(fileInfo.durationMs))) {
            return index;
        }
    }
    return fileInfos.size();
}

static void PairMovingPhotoFiles(std::vector<FileInfo> &fileInfos)
{
    std::map<std::string, std::vector<size_t>> images;
    std::map<std::string, std::vector<size_t>> videos;
    for (size_t index = 0; index < fileInfos.size(); ++index) {
        const auto &fileInfo = fileInfos[index];
        if (fileInfo.mediaType == MediaType::MEDIA_TYPE_IMAGE) {
            images[GetFileStem(fileInfo.displayName)].push_back(index);
        } else if (fileInfo.mediaType == MediaType::MEDIA_TYPE_VIDEO) {
            videos[GetFileStem(fileInfo.displayName)].push_back(index);
        }
    }
    std::vector<bool> pairedVideos(fileInfos.size(), false);
    for (const auto &image : images) {
        const auto video = videos.find(image.first);
        if (video == videos.end()) {
            continue;
        }
        const size_t imageIndex = FindMovingPhotoImage(image.second, fileInfos);
        if (imageIndex == fileInfos.size()) {
            continue;
        }
        const size_t videoIndex = FindMovingPhotoVideo(video->second, fileInfos);
        if (videoIndex == fileInfos.size()) {
            continue;
        }
        auto &imageInfo = fileInfos[imageIndex];
        const auto &videoInfo = fileInfos[videoIndex];
        imageInfo.isMovingPhoto = true;
        imageInfo.videoPath = videoInfo.path;
        imageInfo.videoSourceStat = videoInfo.sourceStat;
        imageInfo.movingPhotoVersion = imageInfo.mimeType == "image/jpeg" ?
            TOOL_MOVING_PHOTO_JPEG_VERSION : TOOL_MOVING_PHOTO_HEIF_VERSION;
        pairedVideos[videoIndex] = true;
    }
    std::vector<FileInfo> imports;
    imports.reserve(fileInfos.size());
    for (size_t index = 0; index < fileInfos.size(); ++index) {
        if (pairedVideos[index]) {
            continue;
        }
        if (!fileInfos[index].isMovingPhoto) {
            fileInfos[index].hasSourceStat = false;
        }
        imports.push_back(std::move(fileInfos[index]));
    }
    fileInfos.swap(imports);
}

static int32_t GetRecognizedDirInfo(const ExecEnv &env, std::vector<FileInfo> &fileInfos)
{
    DIR *directory = opendir(env.sendParam.sendPath.c_str());
    if (directory == nullptr) {
        MEDIA_ERR_LOG("Mediatool opendir failed, errno:%{public}d", errno);
        return Media::E_ERR;
    }
    for (errno = 0; const auto entry = readdir(directory); errno = 0) {
        const std::string name = entry->d_name;
        if (name == "." || name == "..") {
            continue;
        }
        const std::string path = env.sendParam.sendPath + name;
        if (GetFileInfo(env, path, fileInfos) != Media::E_OK) {
            continue;
        }
        auto &fileInfo = fileInfos.back();
        struct stat source {};
        if (lstat(path.c_str(), &source) != 0) {
            MEDIA_ERR_LOG("Candidate stat failed, errno:%{public}d, name:%{private}s", errno, name.c_str());
            fileInfo.isContentValid = false;
        } else if (!S_ISREG(source.st_mode)) {
            fileInfos.pop_back();
            continue;
        }
        if (!fileInfo.isContentValid) {
            printf("%s source changed or unavailable. path:%s\n", STR_FAIL.c_str(), path.c_str());
        }
    }
    const int32_t result = errno == 0 ? Media::E_OK : Media::E_ERR;
    const int32_t readErrno = errno;
    if (closedir(directory) != 0) {
        MEDIA_ERR_LOG("Mediatool closedir failed, errno:%{public}d", errno);
        return Media::E_ERR;
    }
    if (result != Media::E_OK) {
        MEDIA_ERR_LOG("Mediatool readdir failed, errno:%{public}d", readErrno);
        return result;
    }
    std::sort(fileInfos.begin(), fileInfos.end(), [](const FileInfo &left, const FileInfo &right) {
        return left.displayName < right.displayName;
    });
    const size_t scannedCount = fileInfos.size();
    PairMovingPhotoFiles(fileInfos);
    MEDIA_INFO_LOG("Moving photo scan finished, files:%{public}zu, imports:%{public}zu, movingPhotos:%{public}zu",
        scannedCount, fileInfos.size(), scannedCount - fileInfos.size());
    return Media::E_OK;
}

static int32_t RemoveFiles(const ExecEnv &env, std::vector<FileInfo> &fileInfos)
{
    int32_t result = Media::E_OK;
    for (auto &fileInfo : fileInfos) {
        if (fileInfo.result != Media::E_OK) {
            continue;
        }
        if (!fileInfo.toBeRemove) {
            continue;
        }
        if (fileInfo.hasSourceStat && (!IsSourceUnchanged(fileInfo.path, fileInfo.sourceStat) ||
            (fileInfo.isMovingPhoto && !IsSourceUnchanged(fileInfo.videoPath, fileInfo.videoSourceStat)))) {
            MEDIA_ERR_LOG("Source changed before removal, retain sources, fileId:%{public}d", fileInfo.id);
            printf("%s source changed before removal; retain sources. path:%s\n", STR_FAIL.c_str(),
                fileInfo.path.c_str());
            result = Media::E_FAIL;
            continue;
        }
        RemoveFile(fileInfo.path);
        if (fileInfo.isMovingPhoto) {
            RemoveFile(fileInfo.videoPath);
        }
    }
    fileInfos.clear();
    return result;
}

static std::string EncodeDisplayName(const std::string &displayName)
{
    std::set<char> CHAR_FILTERS = {
        '\\', '/', ':',
        '*', '?', '"',
        '<', '>', '|',
    };
    std::string encodedStr = "";
    for (char c : displayName) {
        if (CHAR_FILTERS.find(c) != CHAR_FILTERS.end()) {
            encodedStr += MediaFileUtils::Encode(string(1, c));
        } else {
            encodedStr += c;
        }
    }
    return encodedStr;
}

static int32_t CreateRecord(const ExecEnv &env, FileInfo &fileInfo, bool isRestart)
{
    const MediaType mediaType = fileInfo.mediaType;
    std::string tableName = UserFileClientEx::GetTableNameByMediaType(mediaType);
    const std::string displayName = EncodeDisplayName(fileInfo.displayName);
    string uri;
    auto fileId = UserFileClientEx::InsertExt(tableName, displayName, uri, isRestart, fileInfo.isMovingPhoto);
    if (fileId <= 0) {
        printf("%s create record failed. fileId:%d, fileInfo:%s\n",
            STR_FAIL.c_str(), fileId, fileInfo.ToStr().c_str());
        return Media::E_ERR;
    }
    if (!uri.empty()) {
        fileInfo.uri = uri;
    } else {
        printf("%s create record failed. uri is empty. fileInfo:%s\n",
            STR_FAIL.c_str(), fileInfo.ToStr().c_str());
        return Media::E_FAIL;
    }
    fileInfo.id = fileId;
    return Media::E_OK;
}

static int32_t OpenSendSource(FileInfo &fileInfo)
{
    string realPath;
    if (!fileInfo.hasSourceStat && !PathToRealPath(fileInfo.path, realPath)) {
        printf("%s path issue. errno:%d, path:%s.\n", STR_FAIL.c_str(), errno, fileInfo.path.c_str());
        return Media::E_ERR;
    }
    const int32_t flags = O_RDONLY | O_CLOEXEC | (fileInfo.hasSourceStat ? O_NOFOLLOW : 0);
    int32_t sourceFd = open(realPath.c_str(), flags);
    if (sourceFd < 0) {
        printf("%s open file failed. rfd:%d, path:%s\n", STR_FAIL.c_str(), sourceFd, realPath.c_str());
        return Media::E_ERR;
    }
    if (fileInfo.hasSourceStat) {
        struct stat actual {};
        if (fstat(sourceFd, &actual) != 0 || !IsSameSource(fileInfo.sourceStat, actual)) {
            close(sourceFd);
            return Media::E_ERR;
        }
    }
    return sourceFd;
}

static int32_t WriteFile(const ExecEnv &env, FileInfo &fileInfo, bool isRestart)
{
    int32_t rfd = OpenSendSource(fileInfo);
    if (rfd < 0) {
        return Media::E_ERR;
    }
    int32_t wfd = UserFileClientEx::Open(fileInfo.uri, Media::MEDIA_FILEMODE_WRITETRUNCATE, isRestart);
    if (wfd <= 0) {
        printf("%s open failed. wfd:%d, uri:%s\n", STR_FAIL.c_str(), wfd, fileInfo.uri.c_str());
        close(rfd);
        return Media::E_ERR;
    }
    int32_t ret = MediaFileUtils::CopyFile(rfd, wfd);
    if (!ret) {
        printf("%s send data failed. rfd:%d, wfd:%d\n", STR_FAIL.c_str(), rfd, wfd);
        close(rfd);
        close(wfd);
        return Media::E_ERR;
    }
    if (env.sendParam.isCreateThumbSyncInSend) {
        ret = UserFileClientEx::Close(fileInfo.uri, wfd, Media::MEDIA_FILEMODE_WRITETRUNCATE, true, isRestart);
    } else {
        ret = UserFileClientEx::Close(fileInfo.uri, wfd, Media::MEDIA_FILEMODE_WRITETRUNCATE, false, isRestart);
    }
    if (ret != E_OK) {
        printf("close file has err [%d]\n", ret);
    }

    close(rfd);
    return ret;
}

static int32_t CopyMovingPhotoPart(const std::string &sourcePath, const struct stat &expected,
    const std::string &targetUri, bool isRestart)
{
    std::string realPath;
    if (!PathToRealPath(sourcePath, realPath)) {
        MEDIA_ERR_LOG("Get real path failed, source:%{private}s", sourcePath.c_str());
        return Media::E_ERR;
    }
    int32_t sourceFd = open(realPath.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (sourceFd < 0) {
        MEDIA_ERR_LOG("Open moving photo source failed, errno:%{public}d, source:%{private}s",
            errno, realPath.c_str());
        return Media::E_ERR;
    }
    struct stat actual {};
    if (fstat(sourceFd, &actual) != 0 || !IsSameSource(expected, actual)) {
        MEDIA_ERR_LOG("Moving photo source invalid or changed, source:%{private}s", realPath.c_str());
        close(sourceFd);
        return Media::E_ERR;
    }
    int32_t targetFd = UserFileClientEx::Open(targetUri, Media::MEDIA_FILEMODE_WRITETRUNCATE, isRestart, true);
    if (targetFd < 0) {
        MEDIA_ERR_LOG("Open moving photo target failed, ret:%{public}d, uri:%{private}s", targetFd, targetUri.c_str());
        close(sourceFd);
        return Media::E_ERR;
    }
    const bool copied = MediaFileUtils::CopyFile(sourceFd, targetFd);
    const bool unchanged = fstat(sourceFd, &actual) == 0 && IsSameSource(expected, actual) &&
        IsSourceUnchanged(sourcePath, expected);
    const int32_t targetClose = close(targetFd);
    const int32_t sourceClose = close(sourceFd);
    if (!copied || !unchanged || targetClose != 0 || sourceClose != 0) {
        MEDIA_ERR_LOG("Moving photo copy failed, copied:%{public}d, unchanged:%{public}d, "
            "targetClose:%{public}d, sourceClose:%{public}d", copied, unchanged, targetClose, sourceClose);
    }
    return copied && unchanged && targetClose == 0 && sourceClose == 0 ? Media::E_OK : Media::E_ERR;
}

static int32_t WriteMovingPhoto(const ExecEnv &env, FileInfo &fileInfo, bool isRestart)
{
    MEDIA_INFO_LOG("Begin moving photo write, fileId:%{public}d, version:%{public}d, sync:%{public}d",
        fileInfo.id, fileInfo.movingPhotoVersion, env.sendParam.isCreateThumbSyncInSend);
    int32_t result = CopyMovingPhotoPart(fileInfo.path, fileInfo.sourceStat, fileInfo.uri, isRestart);
    if (result != Media::E_OK) {
        MEDIA_ERR_LOG("Moving photo image copy failed, fileId:%{public}d, ret:%{public}d", fileInfo.id, result);
        return result;
    }
    std::string videoUri = fileInfo.uri;
    MediaFileUtils::UriAppendKeyValue(videoUri, CONST_MEDIA_MOVING_PHOTO_OPRN_KEYWORD,
        CONST_OPEN_MOVING_PHOTO_VIDEO);
    result = CopyMovingPhotoPart(fileInfo.videoPath, fileInfo.videoSourceStat, videoUri, isRestart);
    if (result != Media::E_OK || !IsSourceUnchanged(fileInfo.path, fileInfo.sourceStat) ||
        !IsSourceUnchanged(fileInfo.videoPath, fileInfo.videoSourceStat)) {
        MEDIA_ERR_LOG("Moving photo video copy or source check failed, fileId:%{public}d, ret:%{public}d",
            fileInfo.id, result);
        return Media::E_ERR;
    }
    result = UserFileClientEx::CompleteMovingPhoto(fileInfo.uri, fileInfo.movingPhotoVersion,
        env.sendParam.isCreateThumbSyncInSend, isRestart);
    if (result != Media::E_OK) {
        MEDIA_ERR_LOG("Moving photo completion request failed, fileId:%{public}d, ret:%{public}d", fileInfo.id, result);
    }
    return result;
}

static constexpr int MAX_RESTART_TIME = 5;
static void RestartMediaLibraryServer(int32_t restartTime)
{
    printf("Run operations failed, restart Time %d\n", restartTime);
    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_NOTIFY_TIME));
}

static int32_t SendFile(const ExecEnv &env, FileInfo &fileInfo)
{
    int32_t restartTime = 0;

    while (restartTime < MAX_RESTART_TIME) {
        fileInfo.result = CreateRecord(env, fileInfo, (restartTime > 0) ? true : false);
        if (fileInfo.result != Media::E_OK) {
            restartTime++;
            RestartMediaLibraryServer(restartTime);
            continue;
        } else {
            break;
        }
    }
    if (fileInfo.result != Media::E_OK) {
        return Media::E_ERR;
    }

    std::string tableName = UserFileClientEx::GetTableNameByUri(fileInfo.uri);
    if (tableName.empty()) {
        printf("%s uri issue. uri:%s\n", STR_FAIL.c_str(), fileInfo.uri.c_str());
        return Media::E_ERR;
    }

    while (restartTime < MAX_RESTART_TIME) {
        fileInfo.result = fileInfo.isMovingPhoto ? WriteMovingPhoto(env, fileInfo, restartTime > 0) :
            WriteFile(env, fileInfo, restartTime > 0);
        if (fileInfo.isMovingPhoto && fileInfo.result == Media::E_OK && fileInfo.hasSourceStat &&
            (!IsSourceUnchanged(fileInfo.path, fileInfo.sourceStat) ||
            !IsSourceUnchanged(fileInfo.videoPath, fileInfo.videoSourceStat))) {
            MEDIA_ERR_LOG("Moving photo source changed after completion, fileId:%{public}d", fileInfo.id);
            fileInfo.result = Media::E_ERR;
        }
        if (fileInfo.result != Media::E_OK) {
            restartTime++;
            RestartMediaLibraryServer(restartTime);
            continue;
        } else {
            break;
        }
    }
    if (fileInfo.result != Media::E_OK) {
        return Media::E_ERR;
    }
    fileInfo.result = Media::E_OK;
    return Media::E_OK;
}

static int32_t SendFiles(const ExecEnv &env, std::vector<FileInfo> &fileInfos)
{
    int count = 0;
    int correctCount = 0;
    for (auto &fileInfo : fileInfos) {
        ++count;
        const bool sourceValid = (fileInfo.mediaType != MediaType::MEDIA_TYPE_IMAGE || fileInfo.isContentValid) &&
            (!fileInfo.isMovingPhoto || (fileInfo.isContentValid &&
            (!fileInfo.hasSourceStat || IsSourceUnchanged(fileInfo.path, fileInfo.sourceStat)) &&
            IsSourceUnchanged(fileInfo.videoPath, fileInfo.videoSourceStat)));
        if (!sourceValid) {
            MEDIA_ERR_LOG("Send source invalid before asset creation, name:%{private}s", fileInfo.displayName.c_str());
        }
        int32_t ret = sourceValid ? SendFile(env, fileInfo) : Media::E_ERR;
        if (ret != Media::E_OK) {
            const bool hasUri = !fileInfo.uri.empty();
            printf("%s send %s [%s] failed.\n", STR_FAIL.c_str(), hasUri ? "uri" : "source",
                hasUri ? fileInfo.uri.c_str() : fileInfo.path.c_str());
        } else {
            fileInfo.toBeRemove = true;
            printf("%s\n", fileInfo.uri.c_str());
            ++correctCount;
        }
    }

    if (count > ARGS_ONE) {
        if (count == correctCount) {
            printf("%s send %d and %d is successful.\n", STR_SUCCESS.c_str(), count, correctCount);
            return Media::E_OK;
        } else {
            printf("%s send %d and %d is successful.\n", STR_FAIL.c_str(), count, correctCount);
            return Media::E_FAIL;
        }
    } else {
        if (correctCount == count) {
            return Media::E_OK;
        } else {
            return Media::E_FAIL;
        }
    }
}

int32_t SendCommandV10::Start(const ExecEnv &env)
{
    std::vector<FileInfo> fileInfos;
    int32_t ret = Media::E_ERR;
    if (env.sendParam.isRecognizeMovingPhotoInSend) {
        if (env.sendParam.isFile) {
            return Media::E_ERR;
        }
        ret = GetRecognizedDirInfo(env, fileInfos);
    } else {
        ret = env.sendParam.isFile ? GetFileInfo(env, env.sendParam.sendPath, fileInfos) :
            GetDirInfo(env, env.sendParam.sendPath, fileInfos);
    }
    if (ret != Media::E_OK) {
        printf("%s get file information failed. ret:%d\n", STR_FAIL.c_str(), ret);
        return ret;
    }
    ret = SendFiles(env, fileInfos);
    if (env.sendParam.isRemoveOriginFileInSend) {
        const int32_t removalResult = RemoveFiles(env, fileInfos);
        if (ret == Media::E_OK && removalResult != Media::E_OK) {
            ret = removalResult;
        }
    }
    return ret;
}
} // namespace MediaTool
} // namespace Media
} // namespace OHOS
