/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MovingPhotoFileUtils"

#include "moving_photo_file_utils.h"

#include <fcntl.h>
#include <regex>
#include <sys/sendfile.h>
#include <sys/stat.h>

#include "avmetadatahelper.h"
#include "directory_ex.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_tracer.h"
#include "medialibrary_type_const.h"

using namespace std;

namespace OHOS::Media {
const std::string MEDIA_EXTRA_DATA_DIR = MEDIA_EDIT_DATA_DIR;

const std::string LIVE_PHOTO_CINEMAGRAPH_INFO = "CinemagraphInfo";
const std::string LIVE_PHOTO_VIDEO_INFO_METADATA = "VideoInfoMetadata";
const std::string LIVE_PHOTO_SIGHT_TREMBLE_META_DATA = "SightTrembleMetadata";
const std::string LIVE_PHOTO_VERSION_AND_FRAME_NUM = "VersionAndFrameNum";
constexpr int32_t HEX_BASE = 16;
constexpr int64_t AUTO_PLAY_DURATION_MS = 600;

// LCOV_EXCL_START
static string GetVersionPositionTag(uint32_t frame, bool hasExtraData,
    const string& data = "", bool isCameraShotMovingPhoto = false)
{
    string buffer;
    bool hasCinemagraph{false};
    if (data.size() != 0) {
        uint32_t version{0};
        uint32_t frameIndex{0};
        if (MovingPhotoFileUtils::GetVersionAndFrameNum(data, version, frameIndex, hasCinemagraph) != E_OK) {
            return buffer;
        }
        buffer = "v" + to_string(version) + "_f";
    } else if (hasExtraData) {
        return buffer;
    } else {
        buffer += isCameraShotMovingPhoto ? "v6_f" : "v3_f";
    }
    buffer += to_string(frame);
    if (hasCinemagraph) {
        buffer += "_c";
    }
    uint32_t left = LIVE_TAG_LEN - buffer.length();
    for (uint32_t i = 0; i < left; ++i) {
        buffer += ' ';
    }
    return buffer;
}

static string GetDurationTag(int64_t coverPosition, const string& data = "")
{
    int64_t frame = coverPosition / 1000;
    if (coverPosition < 0) {
        frame = 0;
        MEDIA_WARN_LOG("coverPosition data err %{public}" PRId64, coverPosition);
    }
    string buffer;
    if (data.size() != 0 && !MediaFileUtils::StartsWith(data, "0:0")) {
        buffer += data;
    } else {
        if (frame < AUTO_PLAY_DURATION_MS) {
            buffer += "0:" + to_string(frame);
        } else {
            buffer += to_string(frame - AUTO_PLAY_DURATION_MS) + ":" + to_string(frame);
        }
    }
    uint16_t left = PLAY_INFO_LEN - buffer.length();
    for (uint16_t i = 0; i < left; ++i) {
        buffer += ' ';
    }
    return buffer;
}

static string GetVideoInfoTag(off_t fileSize)
{
    string buffer = "LIVE_" + to_string(fileSize);
    uint16_t left = VERSION_TAG_LEN - buffer.length();
    for (uint16_t i = 0; i < left; ++i) {
        buffer += ' ';
    }
    return buffer;
}

static off_t GetFileSize(const int32_t fd)
{
    if (fd < 0) {
        MEDIA_ERR_LOG("file is error");
        return E_ERR;
    }
    struct stat st;
    if (fstat(fd, &st) != E_OK) {
        MEDIA_ERR_LOG("failed to get file size, errno: %{public}d", errno);
        return E_ERR;
    }
    return st.st_size;
}

static off_t GetFileSize(const string& path)
{
    struct stat st;
    if (stat(path.c_str(), &st) != E_OK) {
        MEDIA_ERR_LOG("failed to get file size, errno: %{public}d", errno);
        return E_ERR;
    }
    return st.st_size;
}

static int32_t WriteContentTofile(const UniqueFd& destFd, const UniqueFd& srcFd)
{
    const uint32_t BUFFER_LENGTH = 16 * 1024; // 16KB
    if (lseek(srcFd.Get(), 0, SEEK_SET) == E_ERR) {
        MEDIA_ERR_LOG("failed to lseek file, errno: %{public}d", errno);
        return E_ERR;
    }
    char buffer[BUFFER_LENGTH];
    ssize_t bytesRead = 0;
    ssize_t bytesWritten = 0;
    while ((bytesRead = read(srcFd.Get(), buffer, BUFFER_LENGTH)) > 0) {
        bytesWritten = write(destFd.Get(), buffer, bytesRead);
        if (bytesWritten != bytesRead) {
            MEDIA_ERR_LOG("failed to write file, errno: %{public}d", errno);
            return E_ERR;
        }
    }
    if (bytesRead < 0) {
        MEDIA_ERR_LOG("failed to read from srcFd:%{public}d, errno:%{public}d", srcFd.Get(), errno);
        return E_ERR;
    }
    return E_OK;
}

static int32_t AddStringToFile(const UniqueFd& destFd, const string& temp)
{
    ssize_t ret = write(destFd.Get(), temp.c_str(), temp.size());
    if (ret < 0 || static_cast<size_t>(ret) != temp.size()) {
        MEDIA_ERR_LOG("failed to write file, errno: %{public}d, ret: %{public}" PRId64, errno,
            static_cast<int64_t>(ret));
        return E_ERR;
    }
    return E_OK;
}

static string GetExtraData(const UniqueFd& fd, off_t fileSize, off_t offset, off_t needSize)
{
    bool cond = (fileSize < 0 || offset < 0 || needSize < 0);
    CHECK_AND_RETURN_RET_LOG(!cond, "", "failed to check fileSize: %{public}" PRId64
        ", offset: %{public}" PRId64 ", needSize: %{public}" PRId64, fileSize, offset, needSize);

    off_t readPosition = fileSize >= offset ? fileSize - offset : 0;
    if (lseek(fd.Get(), readPosition, SEEK_SET) == E_ERR) {
        MEDIA_ERR_LOG("failed to lseek extra file errno: %{public}d", errno);
        return "";
    }
    char* buffer = new (std::nothrow) char[needSize + 1];
    if (buffer == nullptr) {
        MEDIA_ERR_LOG("failed to allocate buffer");
        return "";
    }
    memset_s(buffer, needSize + 1, 0, needSize + 1);
    ssize_t bytesRead;
    if ((bytesRead = read(fd.Get(), buffer, needSize)) < 0) {
        MEDIA_ERR_LOG("failed to read extra file errno: %{public}d", errno);
        delete[] buffer;
        buffer = nullptr;
        return "";
    }
    string content(buffer, bytesRead);
    delete[] buffer;
    buffer = nullptr;
    return content;
}

static int32_t ReadExtraFile(const std::string& extraPath, map<string, string>& extraData)
{
    string absExtraPath;
    if (!PathToRealPath(extraPath, absExtraPath)) {
        MEDIA_ERR_LOG("file is not real path: %{private}s, errno: %{public}d", extraPath.c_str(), errno);
        return E_HAS_FS_ERROR;
    }
    UniqueFd fd(open(absExtraPath.c_str(), O_RDONLY));
    if (fd.Get() == E_ERR) {
        MEDIA_ERR_LOG("failed to open extra file, errno: %{public}d", errno);
        return E_ERR;
    }
    uint32_t version{0};
    uint32_t frameIndex{0};
    bool hasCinemagraphInfo{false};
    bool hasVersion = MovingPhotoFileUtils::GetVersionAndFrameNum(
        fd.Get(), version, frameIndex, hasCinemagraphInfo) == E_OK;
    off_t fileSize = GetFileSize(fd.Get());
    extraData[LIVE_PHOTO_VIDEO_INFO_METADATA] = GetExtraData(fd, fileSize, LIVE_TAG_LEN, LIVE_TAG_LEN);
    extraData[LIVE_PHOTO_SIGHT_TREMBLE_META_DATA] = GetExtraData(fd, fileSize, LIVE_TAG_LEN + PLAY_INFO_LEN,
        PLAY_INFO_LEN);
    if (hasVersion) {
        extraData[LIVE_PHOTO_VERSION_AND_FRAME_NUM] = GetExtraData(fd, fileSize, MIN_STANDARD_SIZE, VERSION_TAG_LEN);
        if (hasCinemagraphInfo) {
            extraData[LIVE_PHOTO_CINEMAGRAPH_INFO] = GetExtraData(fd, fileSize, fileSize, fileSize - MIN_STANDARD_SIZE);
        }
    } else if (fileSize > LIVE_TAG_LEN + PLAY_INFO_LEN) {
        extraData[LIVE_PHOTO_CINEMAGRAPH_INFO] = GetExtraData(fd, fileSize, fileSize,
            fileSize - LIVE_TAG_LEN - PLAY_INFO_LEN);
    }
    return E_OK;
}

static int32_t WriteExtraData(const string& extraPath, const UniqueFd& livePhotoFd, const UniqueFd& videoFd,
    int64_t coverPosition)
{
    map<string, string> extraData;
    bool hasExtraData{false};
    if (MediaFileUtils::IsFileValid(extraPath)) {
        hasExtraData = true;
        if (ReadExtraFile(extraPath, extraData) == E_ERR) {
            MEDIA_ERR_LOG("read extra file err");
            return E_ERR;
        }
        if (AddStringToFile(livePhotoFd, extraData[LIVE_PHOTO_CINEMAGRAPH_INFO]) == E_ERR) {
            MEDIA_ERR_LOG("write cinemagraph info err");
            return E_ERR;
        }
    }
    string versonAndFrameNum = GetVersionPositionTag(MovingPhotoFileUtils::GetFrameIndex(coverPosition, videoFd.Get()),
        hasExtraData, extraData[LIVE_PHOTO_VERSION_AND_FRAME_NUM]);
    if (AddStringToFile(livePhotoFd, versonAndFrameNum) == E_ERR) {
        MEDIA_ERR_LOG("write version position tag err");
        return E_ERR;
    }
    if (AddStringToFile(livePhotoFd,
        GetDurationTag(coverPosition, extraData[LIVE_PHOTO_SIGHT_TREMBLE_META_DATA])) == E_ERR) {
        MEDIA_ERR_LOG("write duration tag err");
        return E_ERR;
    }
    off_t fileSize = GetFileSize(videoFd.Get());
    if (fileSize <= 0) {
        MEDIA_ERR_LOG("failed to check fileSize: %{public}" PRId64, fileSize);
        return E_ERR;
    }
    if (AddStringToFile(livePhotoFd, GetVideoInfoTag(static_cast<size_t>(fileSize) +
        versonAndFrameNum.size() + extraData[LIVE_PHOTO_CINEMAGRAPH_INFO].size())) == E_ERR) {
        MEDIA_ERR_LOG("write video info tag err");
        return E_ERR;
    }
    return E_OK;
}

int32_t MovingPhotoFileUtils::GetExtraDataLen(const string& imagePath, const string& videoPath,
    uint32_t frameIndex, int64_t coverPosition, off_t &fileSize, bool isCameraShotMovingPhoto)
{
    string absImagePath;
    if (!PathToRealPath(imagePath, absImagePath)) {
        MEDIA_ERR_LOG("file is not real path: %{private}s, errno: %{public}d", imagePath.c_str(), errno);
        return E_HAS_FS_ERROR;
    }
    string extraDir = MovingPhotoFileUtils::GetMovingPhotoExtraDataDir(absImagePath);
    string extraPath = MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(absImagePath);
    if (MediaFileUtils::IsFileValid(extraPath)) {
        fileSize = GetFileSize(extraPath);
        return E_OK;
    }
    CHECK_AND_RETURN_RET_LOG(
        MediaFileUtils::CreateDirectory(extraDir), E_ERR, "Cannot create dir %{private}s, errno:%{public}d",
        extraDir.c_str(), errno);
    if (!MediaFileUtils::IsFileExists(extraPath) && MediaFileUtils::CreateAsset(extraPath) != E_OK) {
        MEDIA_ERR_LOG("Failed to create file, path:%{private}s, errno:%{public}d", extraPath.c_str(), errno);
        return E_HAS_FS_ERROR;
    }
    UniqueFd extraDataFd(open(extraPath.c_str(), O_WRONLY | O_TRUNC));
    if (extraDataFd.Get() == E_ERR) {
        MEDIA_ERR_LOG("failed to open extra data, errno:%{public}d", errno);
        return E_ERR;
    }
    if (AddStringToFile(extraDataFd, GetVersionPositionTag(frameIndex, false, "", isCameraShotMovingPhoto)) == E_ERR) {
        MEDIA_ERR_LOG("write version position tag err");
        return E_ERR;
    }
    if (AddStringToFile(extraDataFd, GetDurationTag(coverPosition)) == E_ERR) {
        MEDIA_ERR_LOG("write duration tag err");
        return E_ERR;
    }
    if (AddStringToFile(extraDataFd, GetVideoInfoTag(GetFileSize(videoPath) + VERSION_TAG_LEN)) == E_ERR) {
        MEDIA_ERR_LOG("write video info tag err");
        return E_ERR;
    }
    fileSize = MIN_STANDARD_SIZE;
    return E_OK;
}

static int32_t MergeFile(const UniqueFd& imageFd, const UniqueFd& videoFd, const UniqueFd& livePhotoFd,
    const string& extraPath, int64_t coverPosition)
{
    if (WriteContentTofile(livePhotoFd, imageFd) == E_ERR) {
        MEDIA_ERR_LOG("failed to sendfile from image file");
        return E_ERR;
    }
    if (WriteContentTofile(livePhotoFd, videoFd) == E_ERR) {
        MEDIA_ERR_LOG("failed to sendfile from video file");
        return E_ERR;
    }
    if (WriteExtraData(extraPath, livePhotoFd, videoFd, coverPosition) == E_ERR) {
        MEDIA_ERR_LOG("write cinemagraph info err");
        return E_ERR;
    }
    return E_OK;
}
// LCOV_EXCL_STOP

uint32_t MovingPhotoFileUtils::GetFrameIndex(int64_t time, const int32_t fd)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetFrameIndex");
    uint32_t index{0};
    if (time == 0) {
        return index;
    }
    if (fd < 0) {
        MEDIA_ERR_LOG("file is error");
        return index;
    }
    std::shared_ptr<AVMetadataHelper> avMetadataHelper = AVMetadataHelperFactory::CreateAVMetadataHelper();
    if (avMetadataHelper == nullptr) {
        MEDIA_ERR_LOG("AV metadata helper is null");
        return index;
    }
    if (avMetadataHelper->SetSource(fd, 0, static_cast<int64_t>(GetFileSize(fd)),
        AV_META_USAGE_FRAME_INDEX_CONVERT) != E_OK) {
        MEDIA_ERR_LOG("failed to set source");
        return index;
    }
    if (avMetadataHelper->GetFrameIndexByTime(time, index) != E_OK) {
        MEDIA_ERR_LOG("failed to get frame index");
        return index;
    }
    tracer.Finish();
    return index;
}

static bool IsValidLivePhotoCache(const string &movingPhotoImagepath, const string &cachePath, int32_t userId)
{
    if (!MediaFileUtils::IsFileExists(cachePath)) {
        return false;
    }
    int64_t movingPhotoSize =
        static_cast<int64_t>(MovingPhotoFileUtils::GetMovingPhotoSize(movingPhotoImagepath, userId));
    size_t livePhotoSize = 0;
    if (!MediaFileUtils::GetFileSize(cachePath, livePhotoSize) ||
        static_cast<int64_t>(livePhotoSize) != movingPhotoSize) {
        MEDIA_ERR_LOG("refresh live photo cache, size from %{public}ld to %{public}ld",
            static_cast<long>(livePhotoSize), static_cast<long>(movingPhotoSize));
        CHECK_AND_PRINT_LOG(MediaFileUtils::DeleteFile(cachePath),
            "Failed to delete live photo cache, errno: %{public}d", errno);
        return false;
    }
    return true;
}

// LCOV_EXCL_START
int32_t MovingPhotoFileUtils::ConvertToLivePhoto(const string& movingPhotoImagepath, int64_t coverPosition,
    std::string &livePhotoPath, int32_t userId)
{
    string imagePath = AppendUserId(movingPhotoImagepath, userId);
    string videoPath = GetMovingPhotoVideoPath(movingPhotoImagepath, userId);
    string cacheDir = GetLivePhotoCacheDir(movingPhotoImagepath, userId);
    string extraPath = GetMovingPhotoExtraDataPath(movingPhotoImagepath, userId);
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateDirectory(cacheDir),
        E_HAS_FS_ERROR, "Cannot create dir %{private}s, errno %{public}d", cacheDir.c_str(), errno);
    string cachePath = GetLivePhotoCachePath(movingPhotoImagepath, userId);
    if (IsValidLivePhotoCache(movingPhotoImagepath, cachePath, userId)) {
        livePhotoPath = cachePath;
        return E_OK;
    }
    string absImagePath;
    CHECK_AND_RETURN_RET_LOG(PathToRealPath(imagePath, absImagePath),
        E_HAS_FS_ERROR, "file is not real path: %{private}s, errno: %{public}d", imagePath.c_str(), errno);
    UniqueFd imageFd(open(absImagePath.c_str(), O_RDONLY));
    if (imageFd.Get() == E_ERR) {
        MEDIA_ERR_LOG("failed to open image file, errno: %{public}d", errno);
        return E_ERR;
    }
    string absVideoPath;
    CHECK_AND_RETURN_RET_LOG(PathToRealPath(videoPath, absVideoPath),
        E_HAS_FS_ERROR, "file is not real path: %{private}s, errno: %{public}d", videoPath.c_str(), errno);
    UniqueFd videoFd(open(absVideoPath.c_str(), O_RDONLY));
    if (videoFd.Get() == E_ERR) {
        MEDIA_ERR_LOG("failed to open video file, errno: %{public}d", errno);
        return E_ERR;
    }
    if (MediaFileUtils::CreateAsset(cachePath) != E_OK) {
        MEDIA_ERR_LOG("Failed to create file, path:%{private}s", cachePath.c_str());
        return E_ERR;
    }
    string absCachePath;
    CHECK_AND_RETURN_RET_LOG(PathToRealPath(cachePath, absCachePath),
        E_HAS_FS_ERROR, "file is not real path: %{private}s, errno: %{public}d", cachePath.c_str(), errno);
    UniqueFd livePhotoFd(open(absCachePath.c_str(), O_WRONLY | O_TRUNC));
    if (livePhotoFd.Get() == E_ERR) {
        MEDIA_ERR_LOG("failed to open live photo file, errno: %{public}d", errno);
        return E_ERR;
    }
    if (MergeFile(imageFd, videoFd, livePhotoFd, extraPath, coverPosition) == E_ERR) {
        MEDIA_ERR_LOG("failed to MergeFile file");
        if (!MediaFileUtils::DeleteFile(absCachePath)) {
            MEDIA_ERR_LOG("failed to delete cache file, errno: %{public}d", errno);
        }
        return E_ERR;
    }
    livePhotoPath = absCachePath;
    return E_OK;
}

string MovingPhotoFileUtils::GetSourceBackMovingPhotoImagePath(const string& imagePath, int32_t userId)
{
    return GetEditDataSourceBackPath(imagePath, userId);
}

string MovingPhotoFileUtils::GetSourceBackMovingPhotoVideoPath(const string& imagePath, int32_t userId)
{
    return GetMovingPhotoVideoPath(GetSourceBackMovingPhotoImagePath(imagePath, userId));
}

int32_t MovingPhotoFileUtils::ConvertToSourceLivePhoto(const string& movingPhotoImagePath,
    string& sourceLivePhotoPath, int32_t userId)
{
    string sourceImagePath = GetSourceMovingPhotoImagePath(movingPhotoImagePath, userId);
    string sourceVideoPath = GetSourceMovingPhotoVideoPath(movingPhotoImagePath, userId);
    if (!MediaFileUtils::IsFileExists(sourceVideoPath)) {
        sourceVideoPath = GetMovingPhotoVideoPath(movingPhotoImagePath, userId);
    }
    string extraDataPath = GetMovingPhotoExtraDataPath(movingPhotoImagePath, userId);
    string cacheDir = GetLivePhotoCacheDir(movingPhotoImagePath, userId);
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateDirectory(cacheDir), E_HAS_FS_ERROR,
        "Cannot create dir %{private}s, errno %{public}d", cacheDir.c_str(), errno);
    string sourceCachePath = GetSourceLivePhotoCachePath(movingPhotoImagePath, userId);
    if (MediaFileUtils::IsFileExists(sourceCachePath)) {
        sourceLivePhotoPath = sourceCachePath;
        MEDIA_INFO_LOG("source live photo exists: %{private}s", sourceCachePath.c_str());
        return E_OK;
    }
    string absSourceImagePath;
    CHECK_AND_RETURN_RET_LOG(PathToRealPath(sourceImagePath, absSourceImagePath),
        E_HAS_FS_ERROR, "file is not real path: %{private}s, errno: %{public}d", sourceImagePath.c_str(), errno);
    UniqueFd imageFd(open(absSourceImagePath.c_str(), O_RDONLY));
    CHECK_AND_RETURN_RET_LOG(imageFd.Get() >= 0, E_HAS_FS_ERROR,
        "Failed to open source image:%{private}s, errno:%{public}d", sourceImagePath.c_str(), errno);
    string absSourceVideoPath;
    CHECK_AND_RETURN_RET_LOG(PathToRealPath(sourceVideoPath, absSourceVideoPath),
        E_HAS_FS_ERROR, "file is not real path: %{private}s, errno: %{public}d", sourceVideoPath.c_str(), errno);
    UniqueFd videoFd(open(absSourceVideoPath.c_str(), O_RDONLY));
    CHECK_AND_RETURN_RET_LOG(videoFd.Get() >= 0, E_HAS_FS_ERROR,
        "Failed to open source video:%{private}s, errno:%{public}d", sourceVideoPath.c_str(), errno);
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateAsset(sourceCachePath) == E_OK, E_HAS_FS_ERROR,
        "Failed to create source live photo:%{private}s, errno:%{public}d", sourceCachePath.c_str(), errno);
    string absSourceCachePath;
    CHECK_AND_RETURN_RET_LOG(PathToRealPath(sourceCachePath, absSourceCachePath),
        E_HAS_FS_ERROR, "file is not real path: %{private}s, errno: %{public}d", sourceCachePath.c_str(), errno);
    UniqueFd livePhotoFd(open(absSourceCachePath.c_str(), O_WRONLY | O_TRUNC));
    CHECK_AND_RETURN_RET_LOG(livePhotoFd.Get() >= 0, E_HAS_FS_ERROR,
        "Failed to open source live photo:%{private}s, errno:%{public}d", absSourceCachePath.c_str(), errno);

    if (MergeFile(imageFd, videoFd, livePhotoFd, extraDataPath, 0) != E_OK) {
        MEDIA_ERR_LOG("Failed to merge file of sourve live photo");
        return E_ERR;
    }
    sourceLivePhotoPath = absSourceCachePath;
    return E_OK;
}

bool MovingPhotoFileUtils::IsLivePhoto(const string& path)
{
    string absPath;
    if (!PathToRealPath(path, absPath)) {
        MEDIA_ERR_LOG("file is not real path: %{private}s, errno: %{public}d", path.c_str(), errno);
        return false;
    }
    UniqueFd livePhotoFd(open(absPath.c_str(), O_RDONLY));
    if (GetFileSize(livePhotoFd.Get()) < LIVE_TAG_LEN) {
        MEDIA_ERR_LOG("failed to get file size errno: %{public}d", errno);
        return false;
    }
    off_t offset = lseek(livePhotoFd.Get(), -LIVE_TAG_LEN, SEEK_END);
    if (offset == E_ERR) {
        MEDIA_ERR_LOG("failed to lseek file errno: %{public}d", errno);
        return false;
    }
    char buffer[LIVE_TAG_LEN + 1];
    ssize_t bytesRead = read(livePhotoFd.Get(), buffer, LIVE_TAG_LEN);
    if (bytesRead == E_ERR) {
        MEDIA_ERR_LOG("failed to read file errno: %{public}d", errno);
        return false;
    }
    buffer[bytesRead] = '\0';
    for (uint16_t i = 0; i < LIVE_TAG.size(); i++) {
        if (LIVE_TAG[i] != buffer[i]) {
            return false;
        }
    }
    return true;
}

int32_t MovingPhotoFileUtils::GetLivePhotoSize(int32_t fd, int64_t &liveSize)
{
    if (fd < 0) {
        MEDIA_ERR_LOG("invalid live photo fd");
        return E_ERR;
    }
    if (lseek(fd, -LIVE_TAG_LEN, SEEK_END) == E_ERR) {
        MEDIA_ERR_LOG("failed to lseek file, errno: %{public}d", errno);
        return E_ERR;
    }
    char buffer[LIVE_TAG_LEN + 1];
    ssize_t bytesRead = read(fd, buffer, LIVE_TAG_LEN);
    if (bytesRead == E_ERR) {
        MEDIA_ERR_LOG("failed to read file, errno: %{public}d", errno);
        return E_ERR;
    }
    buffer[bytesRead] = '\0';
    for (size_t i = 0; i < LIVE_TAG.size(); i++) {
        if (LIVE_TAG[i] != buffer[i]) {
            return E_ERR;
        }
    }
    liveSize = atoi(buffer + LIVE_TAG.length());
    return E_OK;
}

static int32_t SendLivePhoto(const int32_t &livePhotoFd, const string &destPath, int64_t sizeToSend, off_t &offset)
{
    struct stat64 statSrc {};
    CHECK_AND_RETURN_RET_LOG(livePhotoFd >= 0, livePhotoFd, "Failed to check src fd of live photo");
    CHECK_AND_RETURN_RET_LOG(fstat64(livePhotoFd, &statSrc) == 0, E_HAS_FS_ERROR,
        "Failed to get file state of live photo, errno = %{public}d", errno);
    off_t totalSize = statSrc.st_size;
    CHECK_AND_RETURN_RET_LOG(sizeToSend <= totalSize - offset, E_INVALID_LIVE_PHOTO, "Failed to check sizeToSend");

    if (!MediaFileUtils::IsFileExists(destPath) && MediaFileUtils::CreateAsset(destPath) != E_OK) {
        MEDIA_ERR_LOG("Failed to create file, path:%{private}s", destPath.c_str());
        return E_HAS_FS_ERROR;
    }
    string absDestPath;
    if (!PathToRealPath(destPath, absDestPath)) {
        MEDIA_ERR_LOG("file is not real path: %{private}s, errno: %{public}d", destPath.c_str(), errno);
        return E_HAS_FS_ERROR;
    }
    UniqueFd destFd(open(absDestPath.c_str(), O_WRONLY));
    if (destFd.Get() < 0) {
        MEDIA_ERR_LOG("Failed to open dest path:%{private}s, errno:%{public}d", absDestPath.c_str(), errno);
        return destFd.Get();
    }

    while (sizeToSend > 0) {
        ssize_t sent = sendfile(destFd.Get(), livePhotoFd, &offset, sizeToSend);
        if (sent < 0) {
            MEDIA_ERR_LOG("Failed to sendfile with errno=%{public}d", errno);
            return sent;
        }
        sizeToSend -= sent;
    }
    return E_OK;
}

static bool IsValidHexInteger(const string &hexStr)
{
    constexpr int32_t HEX_INT_LENGTH = 8;
    if (hexStr.length() > HEX_INT_LENGTH) {
        MEDIA_ERR_LOG("hexStr length > HEX_INT_LENGTH");
        return false;
    }
    char* end = nullptr;
    errno = 0;
    uint64_t num = std::strtoull(hexStr.c_str(), &end, HEX_BASE);
    if (errno == ERANGE || end == nullptr || end == hexStr.c_str() || *end != '\0') {
        MEDIA_ERR_LOG("strtoull exec error");
        return false;
    }
    int32_t maxInteger = numeric_limits<int32_t>::max();
    if (num > static_cast<uint64_t>(maxInteger)) {
        MEDIA_ERR_LOG("num > int32_t.max, num: %{public}llu", static_cast<unsigned long long>(num));
        return false;
    }
    return true;
}

static int32_t ReadLivePhoto(const int32_t &fd, void *buffer, off_t needSize, off_t offset)
{
    if (buffer == nullptr) {
        return E_OK;
    }

    if (lseek(fd, offset, SEEK_SET) == E_ERR) {
        MEDIA_ERR_LOG("failed to lseek extra file errno: %{public}d", errno);
        return E_ERR;
    }
    memset_s(buffer, needSize + 1, 0, needSize + 1);
    while (needSize > 0) {
        ssize_t bytesRead = read(fd, buffer, needSize);
        if (bytesRead < 0) {
            MEDIA_ERR_LOG("Failed to read from live photo, errno=%{public}d", errno);
            return bytesRead;
        }
        needSize -= bytesRead;
    }
    return E_OK;
}

static int32_t GetExtraDataSize(int32_t livePhotoFd, int64_t &extraDataSize, int64_t maxFileSize)
{
    struct stat64 st;
    CHECK_AND_RETURN_RET_LOG(fstat64(livePhotoFd, &st) == 0, E_HAS_FS_ERROR,
        "Failed to get file state of live photo, errno:%{public}d", errno);
    int64_t totalSize = st.st_size;
    CHECK_AND_RETURN_RET_LOG(totalSize > MIN_STANDARD_SIZE, E_INVALID_LIVE_PHOTO,
        "Failed to check live photo, total size is %{public}" PRId64, totalSize);

    char versionTag[VERSION_TAG_LEN + 1] = {0};
    CHECK_AND_RETURN_RET_LOG(lseek(livePhotoFd, -MIN_STANDARD_SIZE, SEEK_END) != -1, E_HAS_FS_ERROR,
        "Failed to lseek version tag, errno:%{public}d", errno);
    CHECK_AND_RETURN_RET_LOG(read(livePhotoFd, versionTag, VERSION_TAG_LEN) != -1, E_HAS_FS_ERROR,
        "Failed to read version tag, errno:%{public}d", errno);

    uint32_t version = 0;
    uint32_t frameIndex = 0;
    bool hasCinemagraph = false;
    int32_t ret = MovingPhotoFileUtils::GetVersionAndFrameNum(versionTag, version, frameIndex, hasCinemagraph);
    if (ret != E_OK) { // not standard version tag
        extraDataSize = LIVE_TAG_LEN + PLAY_INFO_LEN;
        return E_OK;
    }

    if (!hasCinemagraph) { // extra data without cinemagraph
        extraDataSize = MIN_STANDARD_SIZE;
        return E_OK;
    }

    // extra data with cinemagraph
    CHECK_AND_RETURN_RET_LOG(totalSize > MIN_STANDARD_SIZE + CINEMAGRAPH_INFO_SIZE_LEN, E_INVALID_LIVE_PHOTO,
        "Failed to check live photo with cinemagraph, total size is %{public}" PRId64, totalSize);
    char cinemagraphSize[CINEMAGRAPH_INFO_SIZE_LEN] = {0};
    CHECK_AND_RETURN_RET_LOG(lseek(livePhotoFd, -(MIN_STANDARD_SIZE + CINEMAGRAPH_INFO_SIZE_LEN), SEEK_END) != -1,
        E_HAS_FS_ERROR, "Failed to lseek cinemagraph size, errno:%{public}d", errno);
    CHECK_AND_RETURN_RET_LOG(read(livePhotoFd, cinemagraphSize, CINEMAGRAPH_INFO_SIZE_LEN) != -1, E_HAS_FS_ERROR,
        "Failed to read cinemagraph size, errno:%{public}d", errno);
    stringstream cinemagraphSizeStream;
    for (int32_t i = 0; i < CINEMAGRAPH_INFO_SIZE_LEN; i++) {
        cinemagraphSizeStream << hex << static_cast<int32_t>(cinemagraphSize[i]);
    }
    if (!IsValidHexInteger(cinemagraphSizeStream.str())) {
        extraDataSize = MIN_STANDARD_SIZE;
        MEDIA_WARN_LOG("hex string over int max %{public}s", cinemagraphSizeStream.str().c_str());
        return E_OK;
    }
    extraDataSize = MIN_STANDARD_SIZE + std::stoi(cinemagraphSizeStream.str(), 0, HEX_BASE);
    if (extraDataSize >= maxFileSize) {
        extraDataSize = MIN_STANDARD_SIZE;
        MEDIA_WARN_LOG("extra data size over total file size %{public}" PRId64, extraDataSize);
    }
    return E_OK;
}

int32_t MovingPhotoFileUtils::ConvertToMovingPhoto(const std::string &livePhotoPath, const string &movingPhotoImagePath,
    const string &movingPhotoVideoPath, const string &extraDataPath)
{
    string absLivePhotoPath;
    if (!PathToRealPath(livePhotoPath, absLivePhotoPath)) {
        MEDIA_ERR_LOG("file is not real path: %{private}s, errno: %{public}d", livePhotoPath.c_str(), errno);
        return E_HAS_FS_ERROR;
    }
    CHECK_AND_RETURN_RET_LOG(livePhotoPath.compare(movingPhotoVideoPath) != 0 &&
        livePhotoPath.compare(extraDataPath) != 0, E_INVALID_VALUES, "Failed to check dest path");
    UniqueFd livePhotoFd(open(absLivePhotoPath.c_str(), O_RDONLY));
    CHECK_AND_RETURN_RET_LOG(livePhotoFd.Get() >= 0, E_HAS_FS_ERROR,
        "Failed to open live photo:%{private}s, errno:%{public}d", absLivePhotoPath.c_str(), errno);

    struct stat64 st;
    CHECK_AND_RETURN_RET_LOG(fstat64(livePhotoFd.Get(), &st) == 0, E_HAS_FS_ERROR,
        "Failed to get file state of live photo, errno:%{public}d", errno);
    int64_t totalSize = st.st_size;
    CHECK_AND_RETURN_RET_LOG(totalSize > MIN_STANDARD_SIZE, E_INVALID_LIVE_PHOTO,
        "Failed to check live photo, total size is %{public}" PRId64, totalSize);
    char liveTag[LIVE_TAG_LEN + 1] = {0};
    CHECK_AND_RETURN_RET_LOG(lseek(livePhotoFd.Get(), -LIVE_TAG_LEN, SEEK_END) != -1, E_HAS_FS_ERROR,
        "Failed to lseek live tag, errno:%{public}d", errno);
    CHECK_AND_RETURN_RET_LOG(read(livePhotoFd.Get(), liveTag, LIVE_TAG_LEN) != -1, E_HAS_FS_ERROR,
        "Failed to read live tag, errno:%{public}d", errno);
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::StartsWith(liveTag, LIVE_TAG), E_INVALID_VALUES, "Invalid live photo");

    int64_t liveSize = atoi(liveTag + LIVE_TAG.length());
    int64_t imageSize = totalSize - liveSize - LIVE_TAG_LEN - PLAY_INFO_LEN;
    int64_t extraDataSize = 0;
    int32_t err = GetExtraDataSize(livePhotoFd.Get(), extraDataSize, totalSize - imageSize);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, E_INVALID_LIVE_PHOTO,
        "Failed to get size of extra data, err:%{public}" PRId64, extraDataSize);
    int64_t videoSize = totalSize - imageSize - extraDataSize;
    CHECK_AND_RETURN_RET_LOG(imageSize > 0 && videoSize > 0, E_INVALID_LIVE_PHOTO,
        "Failed to check live photo, image size:%{public}" PRId64 "video size:%{public}" PRId64, imageSize, videoSize);
    off_t offset = 0;
    bool isSameImagePath = livePhotoPath.compare(movingPhotoImagePath) == 0;
    string tempImagePath = isSameImagePath ? movingPhotoImagePath + ".temp" : movingPhotoImagePath;
    CHECK_AND_RETURN_RET_LOG((err = SendLivePhoto(livePhotoFd.Get(), tempImagePath, imageSize, offset)) == E_OK, err,
        "Failed to copy image of live photo");
    CHECK_AND_RETURN_RET_LOG((err = SendLivePhoto(livePhotoFd.Get(), movingPhotoVideoPath, videoSize, offset)) == E_OK,
        err, "Failed to copy video of live photo");
    if (!extraDataPath.empty()) {
        CHECK_AND_RETURN_RET_LOG((err = SendLivePhoto(livePhotoFd.Get(), extraDataPath, extraDataSize, offset)) == E_OK,
            err, "Failed to copy extra data of live photo");
    }
    if (isSameImagePath) {
        err = rename(tempImagePath.c_str(), movingPhotoImagePath.c_str());
        CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "Failed to rename image:%{public}d, errno:%{public}d", err, errno);
    }
    return E_OK;
}

int32_t MovingPhotoFileUtils::GetMovingPhotoDetailedSize(const int32_t fd,
    int64_t &imageSize, int64_t &videoSize, int64_t &extraDataSize)
{
    CHECK_AND_RETURN_RET_LOG(fd >= 0, E_HAS_FS_ERROR, "fd is invalid");
    struct stat64 st;
    CHECK_AND_RETURN_RET_LOG(fstat64(fd, &st) == 0, E_HAS_FS_ERROR,
        "Failed to get file state of live photo, errno:%{public}d", errno);
    int64_t totalSize = st.st_size;
    CHECK_AND_RETURN_RET_LOG(totalSize > MIN_STANDARD_SIZE, E_INVALID_LIVE_PHOTO,
        "Failed to check live photo, total size is %{public}" PRId64, totalSize);
    char liveTag[LIVE_TAG_LEN + 1] = {0};
    CHECK_AND_RETURN_RET_LOG(lseek(fd, -LIVE_TAG_LEN, SEEK_END) != -1, E_HAS_FS_ERROR,
        "Failed to lseek live tag, errno:%{public}d", errno);
    CHECK_AND_RETURN_RET_LOG(read(fd, liveTag, LIVE_TAG_LEN) != -1, E_HAS_FS_ERROR,
        "Failed to read live tag, errno:%{public}d", errno);
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::StartsWith(liveTag, LIVE_TAG), E_INVALID_VALUES, "Invalid live photo");

    int64_t liveSize = atoi(liveTag + LIVE_TAG.length());
    imageSize = totalSize - liveSize - LIVE_TAG_LEN - PLAY_INFO_LEN;
    int32_t err = GetExtraDataSize(fd, extraDataSize, totalSize - imageSize);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, E_INVALID_LIVE_PHOTO,
        "Failed to get size of extra data, err:%{public}" PRId64, extraDataSize);
    videoSize = totalSize - imageSize - extraDataSize;
    CHECK_AND_RETURN_RET_LOG(imageSize > 0 && videoSize > 0, E_INVALID_LIVE_PHOTO,
        "Failed to check live photo, image size:%{public}" PRId64 "video size:%{public}" PRId64, imageSize, videoSize);
    return E_OK;
}

int32_t MovingPhotoFileUtils::ConvertToMovingPhoto(const int32_t fd, const string &movingPhotoImagePath,
    const string &movingPhotoVideoPath, const string &extraDataPath)
{
    CHECK_AND_RETURN_RET_LOG(fd >= 0, E_HAS_FS_ERROR, "fd is invalid");
    int64_t imageSize = 0;
    int64_t videoSize = 0;
    int64_t extraDataSize = 0;
    int32_t err = GetMovingPhotoDetailedSize(fd, imageSize, videoSize, extraDataSize);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "Failed to get detailed size of moving photo");
    off_t offset = 0;
    if (!movingPhotoImagePath.empty()) {
        CHECK_AND_RETURN_RET_LOG((err = SendLivePhoto(fd, movingPhotoImagePath, imageSize, offset)) == E_OK, err,
            "Failed to copy image of live photo");
    }
    if (!movingPhotoVideoPath.empty()) {
        offset = imageSize;
        CHECK_AND_RETURN_RET_LOG((err = SendLivePhoto(fd, movingPhotoVideoPath, videoSize, offset)) == E_OK, err,
            "Failed to copy video of live photo");
    }
    if (!extraDataPath.empty()) {
        offset = imageSize + videoSize;
        CHECK_AND_RETURN_RET_LOG((err = SendLivePhoto(fd, extraDataPath, extraDataSize, offset)) == E_OK, err,
            "Failed to copy extra data of live photo");
    }
    return E_OK;
}

int32_t MovingPhotoFileUtils::ConvertToMovingPhoto(const int32_t fd, void *imageArrayBuffer,
    void *videoArrayBuffer, void *extraDataArrayBuffer)
{
    CHECK_AND_RETURN_RET_LOG(fd >= 0, E_HAS_FS_ERROR, "fd is invalid");
    int64_t imageSize = 0;
    int64_t videoSize = 0;
    int64_t extraDataSize = 0;
    int32_t err = GetMovingPhotoDetailedSize(fd, imageSize, videoSize, extraDataSize);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "Failed to get detailed size of moving photo");
    CHECK_AND_RETURN_RET_LOG((err = ReadLivePhoto(fd, imageArrayBuffer, imageSize, 0)) == E_OK, err,
        "Failed to copy image of live photo");
    CHECK_AND_RETURN_RET_LOG((err = ReadLivePhoto(fd, videoArrayBuffer, videoSize, imageSize)) == E_OK, err,
        "Failed to copy video of live photo");
    CHECK_AND_RETURN_RET_LOG(
        (err = ReadLivePhoto(fd, extraDataArrayBuffer, extraDataSize, imageSize + videoSize)) == E_OK, err,
        "Failed to copy extra data of live photo");
    return E_OK;
}

static int32_t GetMovingPhotoCoverPosition(const UniqueFd &uniqueFd, const int64_t size,
    const uint32_t frameIndex, uint64_t &coverPosition, int32_t scene)
{
    MediaLibraryTracer tracer;
    tracer.Start("AVMetadataHelper");
    shared_ptr<AVMetadataHelper> helper = AVMetadataHelperFactory::CreateAVMetadataHelper();
    if (helper == nullptr) {
        MEDIA_ERR_LOG("AV metadata helper is null");
        return E_AVMETADATA;
    }

    // notify media_service clone event.
    if (scene == Scene::AV_META_SCENE_CLONE) {
        helper->SetScene(static_cast<Scene>(scene));
    }
    int32_t err = helper->SetSource(uniqueFd.Get(), 0, size, AV_META_USAGE_FRAME_INDEX_CONVERT);
    tracer.Finish();
    if (err != 0) {
        MEDIA_ERR_LOG("SetSource failed for the given fd, err = %{public}d", err);
        return E_AVMETADATA;
    }

    tracer.Start("AVMetadataHelper->GetTimeByFrameIndex");
    err = helper->GetTimeByFrameIndex(frameIndex, coverPosition);
    tracer.Finish();
    if (err != 0) {
        MEDIA_ERR_LOG("Failed to GetTimeByFrameIndex, err = %{public}d", err);
        return E_AVMETADATA;
    }
    return E_OK;
}

int32_t MovingPhotoFileUtils::GetCoverPosition(const std::string &videoPath, const uint32_t frameIndex,
    uint64_t &coverPosition, int32_t scene)
{
    string absVideoPath;
    if (!PathToRealPath(videoPath, absVideoPath)) {
        MEDIA_ERR_LOG("Failed to get real path: %{private}s, errno: %{public}d", videoPath.c_str(), errno);
        return E_HAS_FS_ERROR;
    }

    UniqueFd uniqueFd(open(absVideoPath.c_str(), O_RDONLY));
    if (uniqueFd.Get() < 0) {
        MEDIA_ERR_LOG("Failed to open %{private}s, errno: %{public}d", absVideoPath.c_str(), errno);
        return E_HAS_FS_ERROR;
    }
    struct stat64 st;
    if (fstat64(uniqueFd.Get(), &st) != 0) {
        MEDIA_ERR_LOG("Failed to get file state, errno: %{public}d", errno);
        return E_HAS_FS_ERROR;
    }
    return GetMovingPhotoCoverPosition(uniqueFd, st.st_size, frameIndex, coverPosition, scene);
}

bool EndsWith(const string &str, const string &endStr)
{
    if (str.length() < endStr.length()) {
        return false;
    }
    return str.rfind(endStr) == str.length() - endStr.length();
}

int32_t MovingPhotoFileUtils::GetVersionAndFrameNum(const string &tag,
    uint32_t &version, uint32_t &frameIndex, bool &hasCinemagraphInfo)
{
    static const string VERSION_TAG_REGEX = "^[vV](\\d+)_[fF](\\d+).*";
    std::regex pattern(VERSION_TAG_REGEX);
    std::smatch result;
    if (!std::regex_search(tag, result, pattern)) {
        MEDIA_WARN_LOG("tag is not standard version tag: %{public}s", tag.c_str());
        return E_INVALID_VALUES;
    }

    constexpr int32_t VERSION_POSITION = 1;
    constexpr int32_t FRAME_INDEX_POSITION = 2;
    version = static_cast<uint32_t>(atoi(result[VERSION_POSITION].str().c_str()));
    frameIndex = static_cast<uint32_t>(atoi(result[FRAME_INDEX_POSITION].str().c_str()));
    size_t blankIndex = tag.find_first_of(' ');
    string tagTrimmed = tag;
    if (blankIndex != string::npos) {
        tagTrimmed = tagTrimmed.substr(0, blankIndex);
    }
    hasCinemagraphInfo = EndsWith(tagTrimmed, "_c") || EndsWith(tagTrimmed, "_C");
    return E_OK;
}

int32_t MovingPhotoFileUtils::GetVersionAndFrameNum(int32_t fd,
    uint32_t &version, uint32_t &frameIndex, bool &hasCinemagraphInfo)
{
    CHECK_AND_RETURN_RET_LOG(fd >= 0, E_HAS_FS_ERROR, "Failed to check fd, errno:%{public}d", errno);
    struct stat64 st;
    CHECK_AND_RETURN_RET_LOG(fstat64(fd, &st) == 0, E_HAS_FS_ERROR,
        "Failed to get file state, errno:%{public}d", errno);
    int64_t totalSize = st.st_size;
    CHECK_AND_RETURN_RET_LOG(totalSize >= MIN_STANDARD_SIZE, E_INVALID_LIVE_PHOTO,
        "Failed to fetch version tag, total size is %{public}" PRId64, totalSize);

    char versionTag[VERSION_TAG_LEN + 1] = {0};
    CHECK_AND_RETURN_RET_LOG(lseek(fd, -MIN_STANDARD_SIZE, SEEK_END) != -1, E_HAS_FS_ERROR,
        "Failed to lseek version tag, errno:%{public}d", errno);
    CHECK_AND_RETURN_RET_LOG(read(fd, versionTag, VERSION_TAG_LEN) != -1, E_HAS_FS_ERROR,
        "Failed to read version tag, errno:%{public}d", errno);
    return MovingPhotoFileUtils::GetVersionAndFrameNum(versionTag, version, frameIndex, hasCinemagraphInfo);
}

string MovingPhotoFileUtils::GetMovingPhotoVideoPath(const string &imagePath, int32_t userId)
{
    return MediaFileUtils::GetMovingPhotoVideoPath(AppendUserId(imagePath, userId));
}

string MovingPhotoFileUtils::GetMovingPhotoExtraDataDir(const string &imagePath, int32_t userId)
{
    if (imagePath.length() < ROOT_MEDIA_DIR.length() || !MediaFileUtils::StartsWith(imagePath, ROOT_MEDIA_DIR)) {
        return "";
    }
    return AppendUserId(MEDIA_EXTRA_DATA_DIR, userId) + imagePath.substr(ROOT_MEDIA_DIR.length());
}

string MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(const string &imagePath, int32_t userId)
{
    string parentPath = GetMovingPhotoExtraDataDir(imagePath, userId);
    if (parentPath.empty()) {
        return "";
    }
    return parentPath + "/extraData";
}

string MovingPhotoFileUtils::GetSourceMovingPhotoImagePath(const string& imagePath, int32_t userId)
{
    return GetEditDataSourcePath(imagePath, userId);
}

string MovingPhotoFileUtils::GetSourceMovingPhotoVideoPath(const string& imagePath, int32_t userId)
{
    return GetMovingPhotoVideoPath(GetSourceMovingPhotoImagePath(imagePath, userId));
}

string MovingPhotoFileUtils::GetLivePhotoCacheDir(const string &imagePath, int32_t userId)
{
    if (imagePath.length() < ROOT_MEDIA_DIR.length() || !MediaFileUtils::StartsWith(imagePath, ROOT_MEDIA_DIR)) {
        return "";
    }
    return AppendUserId(MEDIA_CACHE_DIR, userId) + imagePath.substr(ROOT_MEDIA_DIR.length());
}

string MovingPhotoFileUtils::GetLivePhotoCachePath(const string &imagePath, int32_t userId)
{
    string parentPath = GetLivePhotoCacheDir(imagePath, userId);
    if (parentPath.empty()) {
        return "";
    }
    return parentPath + "/livePhoto." + MediaFileUtils::GetExtensionFromPath(imagePath);
}

string MovingPhotoFileUtils::GetSourceLivePhotoCachePath(const string& imagePath, int32_t userId)
{
    string parentPath = GetLivePhotoCacheDir(imagePath, userId);
    if (parentPath.empty()) {
        return "";
    }
    return parentPath + "/sourceLivePhoto." + MediaFileUtils::GetExtensionFromPath(imagePath);
}

bool MovingPhotoFileUtils::IsMovingPhoto(int32_t subtype, int32_t effectMode, int32_t originalSubtype)
{
    return subtype == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO) ||
           effectMode == static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY) ||
           IsGraffiti(subtype, originalSubtype);
}

bool MovingPhotoFileUtils::IsGraffiti(int32_t subtype, int32_t originalSubtype)
{
    return subtype == static_cast<int32_t>(PhotoSubType::DEFAULT) &&
           originalSubtype == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO);
}

size_t MovingPhotoFileUtils::GetMovingPhotoSize(const std::string &imagePath, int32_t userId)
{
    string movingPhotoImagePath = AppendUserId(imagePath, userId);
    string movingPhotoVideoPath = GetMovingPhotoVideoPath(imagePath, userId);
    string movingPhotoExtraDataPath = GetMovingPhotoExtraDataPath(imagePath, userId);
    size_t imageSize = 0;
    size_t videoSize = 0;
    size_t extraDataSize = 0;
    (void)MediaFileUtils::GetFileSize(movingPhotoImagePath, imageSize);
    (void)MediaFileUtils::GetFileSize(movingPhotoVideoPath, videoSize);
    (void)MediaFileUtils::GetFileSize(movingPhotoExtraDataPath, extraDataSize);
    return imageSize + videoSize + extraDataSize;
}
// LCOV_EXCL_STOP
} // namespace OHOS::Media