/*
 * Copyright (C) 2021-2025 Huawei Device Co., Ltd.
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
#define MLOG_TAG "FileUtils"

#include "media_file_utils.h"

#include <stack>
#include <dirent.h>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <ftw.h>
#include <regex>
#include <sys/sendfile.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
#include <utime.h>

#include "avmetadatahelper.h"
#include "directory_ex.h"
#include "hmdfs.h"
#include "ipc_skeleton.h"
#include "media_file_uri.h"
#include "media_log.h"
#include "media_map_const_utils.h"
#include "medialibrary_errno.h"
#include "mimetype_utils.h"
#include "medialibrary_tracer.h"
#include "ptp_medialibrary_manager_uri.h"
#include "string_ex.h"
#include "userfilemgr_uri.h"
#include "data_secondary_directory_uri.h"
#include "image_source.h"
#include "image_packer.h"

using namespace std;

namespace OHOS::Media {
static const mode_t CHOWN_RWX_USR_GRP = 02771;
static const mode_t CHOWN_RW_USR_GRP = 0660;
static const mode_t CHOWN_RO_USR_GRP = 0644;
constexpr size_t DISPLAYNAME_MAX = 255;
constexpr int32_t HAS_APPLINK_MIN = 0;
constexpr int32_t HAS_APPLINK_MAX = 2;
constexpr size_t APPLINK_MAX = 512;
const int32_t OPEN_FDS = 64;
const std::string PATH_PARA = "path=";
constexpr unsigned short MAX_RECURSION_DEPTH = 4;
constexpr size_t DEFAULT_TIME_SIZE = 32;
constexpr int32_t CROSS_POLICY_ERR = 18;
const int32_t HMFS_MONITOR_FL = 2;
const int32_t INTEGER_MAX_LENGTH = 10;
const std::string LISTENING_BASE_PATH = "/storage/media/local/files/";
const std::string PHOTO_DIR = "Photo";
const std::string AUDIO_DIR = "Audio";
const std::string THUMBS_DIR = ".thumbs";
const std::string EDIT_DATA_DIR = ".editData";
const std::string THUMBS_PHOTO_DIR = ".thumbs/Photo";
const std::string EDIT_DATA_PHOTO_DIR = ".editData/Photo";
const std::string CLOUD_FILE_PATH = "/storage/cloud/files";
const std::string TMP_SUFFIX = "tmp";
const std::vector<std::string> SET_LISTEN_DIR = {
    PHOTO_DIR, AUDIO_DIR, THUMBS_DIR, EDIT_DATA_DIR, THUMBS_PHOTO_DIR, EDIT_DATA_PHOTO_DIR
};
const std::string KVSTORE_FILE_ID_TEMPLATE = "0000000000";
const std::string KVSTORE_DATE_KEY_TEMPLATE = "0000000000000";
const std::string MAX_INTEGER = "2147483648";
const std::string DEFAULT_IMAGE_NAME = "IMG_";
const std::string DEFAULT_VIDEO_NAME = "VID_";
const std::string DEFAULT_AUDIO_NAME = "AUD_";
const int64_t UNIT = 1000;
const int64_t STD_UNIT = 1024;
const int64_t THRESHOLD = 512;
const int64_t VIRTUAL_ID_MAX_SIZE = 18;
const std::string DATA_PATH = "/data/storage/el2/base";
#define HMFS_IOCTL_HW_GET_FLAGS _IOR(0XF5, 70, unsigned int)
#define HMFS_IOCTL_HW_SET_FLAGS _IOR(0XF5, 71, unsigned int)
const std::string MEDIA_DATA_DEVICE_PATH = "local";
const int32_t ASPECT_RATIO_UNSUPPORT = -1;
const int32_t ASPECT_RATIO_MAX = 1000;
const double ASPECT_RATIO_MIN = 0.001;
const double ASPECT_RATIO_PRECISION = 1000.0;

static const std::unordered_map<std::string, std::vector<std::string>> MEDIA_EXTRA_MIME_TYPE_MAP = {
    { "audio/3gpp", { "3gpp" } },
    { "audio/midi", { "mid", "midi", "kar" } },
    { "audio/basic", { "au" } },
    { "audio/x-pn-realaudio", { "rm", "ram" } },
    { "audio/aac-adts", { "aac" } },
    { "audio/mpeg", { "m4a", "mpga", "mpega" } },
    { "audio/x-mpegurl", { "m3u", "m3u8" } },
    { "audio/ffmpeg", { "ape" } },
    { "audio/mp4", { "isma" } },
    { "audio/ac4", { "ac4" } },
    { "audio/amr-wb", { "awb" } },
    { "audio/annodex", { "axa" } },
    { "audio/csound", { "csd", "orc", "sco" } },
    { "audio/ogg", { "oga", "ogg", "opus", "spx" } },
    { "audio/prs.sid", { "sid" } },
    { "audio/x-aiff", { "aif", "aiff", "aifc" } },
    { "audio/x-gsm", { "gsm"} },
    { "audio/x-mpegurl", { "m3u" } },
    { "audio/x-ms-wma", { "wma" } },
    { "audio/x-ms-wax", { "wax" } },
    { "audio/x-realaudio", { "ra" } },
    { "audio/x-scpls", { "pls" } },
    { "audio/x-sd2", { "sd2" } },
    { "audio/x-wav", { "wav" } }
};

vector<string> MediaFileUtils::GetAllTypes(const int32_t extension)
{
    vector<std::string> allTypesVec;
    if (extension != MEDIA_TYPE_IMAGE && extension != MEDIA_TYPE_VIDEO) {
        return allTypesVec;
    }
    for (auto &item : MediaMapConstUtils::GetMimeTypeMap()) {
        if (extension == MimeTypeUtils::GetMediaTypeFromMimeType(item.first)) {
            for (auto &ext : item.second) {
                allTypesVec.push_back(ext);
            }
        }
    }
    return allTypesVec;
}

int32_t UnlinkCb(const char *fpath, const struct stat *sb, int32_t typeflag, struct FTW *ftwbuf)
{
    CHECK_AND_RETURN_RET_LOG(fpath != nullptr, E_FAIL, "fpath == nullptr");
    int32_t errRet = remove(fpath);
    CHECK_AND_PRINT_LOG(!errRet, "Failed to remove errno: %{public}d, path: %{private}s", errno, fpath);
    return errRet;
}

int32_t MediaFileUtils::RemoveDirectory(const string &path)
{
    return nftw(path.c_str(), UnlinkCb, OPEN_FDS, FTW_DEPTH | FTW_PHYS);
}

std::string MediaFileUtils::DesensitizePath(const std::string &path)
{
    string result = path;
    CHECK_AND_RETURN_RET(result.length() > CLOUD_FILE_PATH.length(), result);
    return result.replace(0, CLOUD_FILE_PATH.length(), "*");
}

void MediaFileUtils::PrintStatInformation(const std::string& path)
{
    struct stat statInfo {};
    if ((stat(path.c_str(), &statInfo)) == E_SUCCESS) {
        MEDIA_INFO_LOG("path:%{public}s uid:%{public}d, gid:%{public}d, mode:%{public}d",
            DesensitizePath(path).c_str(), statInfo.st_uid, statInfo.st_gid, statInfo.st_mode);
    } else {
        MEDIA_INFO_LOG("path:%{public}s is not exist", DesensitizePath(path).c_str());
    }
}

bool MediaFileUtils::Mkdir(const string &subStr, shared_ptr<int> errCodePtr)
{
    mode_t mask = umask(0);
    if (mkdir(subStr.c_str(), CHOWN_RWX_USR_GRP) == -1) {
        if (errCodePtr != nullptr) {
            *errCodePtr = errno;
        }
        int err = errno;
        MEDIA_ERR_LOG("Failed to create directory %{public}d, path:%{public}s", err, DesensitizePath(subStr).c_str());
        if (err == EACCES) {
            PrintStatInformation(GetParentPath(subStr));
        }
        umask(mask);
        return (err == EEXIST) ? true : false;
    }
    umask(mask);
    return true;
}

bool MediaFileUtils::CreateDirectory(const string &dirPath, shared_ptr<int> errCodePtr)
{
    string subStr;
    string segment;

    /*  Create directory and its sub directories if does not exist
     *  take each string after '/' create directory if does not exist.
     *  Created directory will be the base path for the next sub directory.
     */

    stringstream folderStream(dirPath);
    while (getline(folderStream, segment, '/')) {
        if (segment.empty()) {    // skip the first "/" in case of "/storage/cloud/files"
            continue;
        }

        subStr.append(SLASH_CHAR + segment);
        if (!IsDirectory(subStr, errCodePtr)) {
            CHECK_AND_RETURN_RET(Mkdir(subStr, errCodePtr), false);
        }
    }

    return true;
}

bool MediaFileUtils::IsFileExists(const string &fileName)
{
    struct stat statInfo {};

    return ((stat(fileName.c_str(), &statInfo)) == E_SUCCESS);
}

bool MediaFileUtils::IsFileValid(const string &fileName)
{
    struct stat statInfo {};
    if (!fileName.empty()) {
        if (stat(fileName.c_str(), &statInfo) == E_SUCCESS) {
            // if the given path is a directory path, return
            CHECK_AND_RETURN_RET_LOG(!(statInfo.st_mode & S_IFDIR), false, "file is a directory");

            // if the file is empty
            CHECK_AND_WARN_LOG(statInfo.st_size != 0, "file is empty");
            return true;
        }
    }
    return false;
}

bool MediaFileUtils::IsDirEmpty(const string &path)
{
    DIR *dir = opendir(path.c_str());
    if (dir == nullptr) {
        MEDIA_ERR_LOG("Failed to open dir:%{private}s, errno: %{public}d. Just return dir NOT empty.",
            path.c_str(), errno);
        return false;
    }
    bool ret = true;
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            ret = false;
            break;
        }
    }
    if (closedir(dir) < 0) {
        MEDIA_ERR_LOG("Fail to closedir: %{private}s, errno: %{public}d.", path.c_str(), errno);
    }
    return ret;
}

string MediaFileUtils::GetFileName(const string &filePath)
{
    string fileName;

    if (!(filePath.empty())) {
        size_t lastSlash = filePath.rfind('/');
        if (lastSlash != string::npos) {
            if (filePath.size() > (lastSlash + 1)) {
                fileName = filePath.substr(lastSlash + 1);
            }
        }
    }

    return fileName;
}

int32_t MediaFileUtils::CreateAssetRealName(int32_t fileId, int32_t mediaType,
    const string &extension, string &name)
{
    const int32_t maxComplementAssetId = 999;
    string fileNumStr = to_string(fileId);
    if (fileId <= maxComplementAssetId) {
        size_t fileIdLen = fileNumStr.length();
        fileNumStr = ("00" + fileNumStr).substr(fileIdLen - 1);
    }

    string mediaTypeStr;
    switch (mediaType) {
        case MediaType::MEDIA_TYPE_IMAGE:
            mediaTypeStr = DEFAULT_IMAGE_NAME;
            break;
        case MediaType::MEDIA_TYPE_VIDEO:
            mediaTypeStr = DEFAULT_VIDEO_NAME;
            break;
        case MediaType::MEDIA_TYPE_AUDIO:
            mediaTypeStr = DEFAULT_AUDIO_NAME;
            break;
        default:
            MEDIA_ERR_LOG("This mediatype %{public}d can not get real name", mediaType);
            return E_INVALID_VALUES;
    }

    static const int32_t CONFLICT_TIME = 100;
    if (extension.length() == 0) {
        name = mediaTypeStr + to_string(MediaFileUtils::UTCTimeSeconds() + CONFLICT_TIME) + "_" + fileNumStr;
    } else {
        name = mediaTypeStr + to_string(MediaFileUtils::UTCTimeSeconds() + CONFLICT_TIME) + "_" +
           fileNumStr + "." + extension;
    }
    return E_OK;
}

bool MediaFileUtils::IsDirectory(const string &dirName, shared_ptr<int> errCodePtr)
{
    struct stat statInfo {};

    int32_t ret = stat(dirName.c_str(), &statInfo);
    if (ret == E_SUCCESS) {
        if (statInfo.st_mode & S_IFDIR) {
            return true;
        }
    } else if (errCodePtr != nullptr) {
        *errCodePtr = errno;
        MEDIA_ERR_LOG("ret: %{public}d, errno: %{public}d", ret, errno);
        return false;
    }
    MEDIA_ERR_LOG("ret: %{public}d, errno: %{public}d", ret, errno);
    return false;
}

bool MediaFileUtils::CreateFile(const string &filePath)
{
    bool state = false;

    if (filePath.empty()) {
        MEDIA_ERR_LOG("Invalid file path");
        return state;
    }
    if (IsFileExists(filePath)) {
        MEDIA_ERR_LOG("file already exists");
        return state;
    }

    auto normalizedDstPath = std::filesystem::absolute(filePath).lexically_normal();
    ofstream file(normalizedDstPath);
    if (!file) {
        MEDIA_ERR_LOG("Output file path could not be created");
        return state;
    }

    if (chmod(normalizedDstPath.c_str(), CHOWN_RW_USR_GRP) == E_SUCCESS) {
        state = true;
    } else {
        MEDIA_ERR_LOG("Failed to change permissions, error: %{public}d", errno);
    }

    file.close();

    return state;
}

bool MediaFileUtils::DeleteFile(const string &fileName)
{
    return (remove(fileName.c_str()) == E_SUCCESS);
}

bool MediaFileUtils::DeleteFileWithRetry(const string &fileName)
{
    int32_t maxRetryCount = 3;
    int32_t retryCount = 0;
    bool isRemoved = false;
    bool isExists = false;
    bool isDeleted = false;
    while (!isDeleted && retryCount < maxRetryCount) {
        retryCount++;
        isRemoved = DeleteFile(fileName);
        isExists = IsFileExists(fileName);
        isDeleted = isRemoved && !isExists;
        CHECK_AND_PRINT_LOG(isDeleted,
            "DeleteFileWithRetry fail, retryCount: %{public}d, "
            "isRemoved: %{public}d, isExists: %{public}d, fileName: %{public}s",
            retryCount,
            isRemoved,
            isExists,
            fileName.c_str());
    }
    return isDeleted;
}

bool MediaFileUtils::DeleteDir(const string &dirName)
{
    bool errRet = false;

    if (IsDirectory(dirName)) {
        errRet = (RemoveDirectory(dirName) == E_SUCCESS);
    }

    return errRet;
}

bool MediaFileUtils::CopyFileAndDelSrc(const std::string &srcFile, const std::string &destFile)
{
    bool fileExist = IsFileExists(destFile);
    if (fileExist) {
        MEDIA_INFO_LOG("destFile:%{private}s already exists", destFile.c_str());
        CHECK_AND_RETURN_RET_LOG(CopyFileUtil(destFile, destFile + TMP_SUFFIX), false,
            "copy destfile:%{private}s failed", destFile.c_str());
        CHECK_AND_RETURN_RET_LOG(DeleteFile(destFile), false, "delete destFile:%{private}s error", destFile.c_str());
    }

    if (CopyFileUtil(srcFile, destFile)) {
        CHECK_AND_PRINT_LOG(DeleteFile(srcFile), "delete srcFile:%{private}s failed", srcFile.c_str());
        CHECK_AND_PRINT_LOG(DeleteFile(destFile + TMP_SUFFIX), "delete tmpFile:%{private}s failed", srcFile.c_str());
        return true;
    }

    if (fileExist) {
        CHECK_AND_PRINT_LOG(CopyFileUtil(destFile + TMP_SUFFIX, destFile),
            "recover destFile:%{private}s error", destFile.c_str());
        CHECK_AND_PRINT_LOG(DeleteFile(destFile + TMP_SUFFIX), "delete tmpFile:%{private}s failed", srcFile.c_str());
    } else {
        bool delDestFileRet = DeleteFile(destFile);
        MEDIA_ERR_LOG("copy srcFile:%{private}s failed,delDestFileRet:%{public}d", srcFile.c_str(), delDestFileRet);
    }
    return false;
}

/**
 * @brief Copy the contents of srcPath to destPath, delete the successfully copied files and directories.
 *
 * @param srcPath must be a directory.
 * @param destPath must be a directory.
 * @param curRecursionDepth current recursion depth. The maximum value is {@code MAX_RECURSION_DEPTH}.
 * @return true: all contents of {@code srcPath} are successfully copied to {@code destPath}.
 *         false: as long as there is one item of {@code srcPath} is not successfully copied to {@code destPath}.
 */
bool MediaFileUtils::CopyDirAndDelSrc(const std::string &srcPath, const std::string &destPath,
    unsigned short curRecursionDepth)
{
    CHECK_AND_RETURN_RET_LOG(curRecursionDepth <= MAX_RECURSION_DEPTH, false,
        "curRecursionDepth:%{public}d>MAX_RECURSION_DEPTH", curRecursionDepth);
    ++curRecursionDepth;
    bool ret = true;
    DIR* srcDir = opendir(srcPath.c_str());
    CHECK_AND_RETURN_RET_LOG(srcDir != nullptr, false,
        "open srcDir:%{private}s failed,errno:%{public}d", srcPath.c_str(), errno);

    if (!IsFileExists(destPath)) {
        if (!CreateDirectory(destPath)) {
            MEDIA_ERR_LOG("create destPath:%{private}s failed", srcPath.c_str());
            closedir(srcDir);
            return false;
        }
    }
    struct dirent* entry;
    while ((entry = readdir(srcDir))!= nullptr) {
        bool cond = (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0);
        CHECK_AND_CONTINUE(!cond);
        string srcSubPath = srcPath + SLASH_STR + (entry->d_name);
        string destSubPath = destPath + SLASH_STR + (entry->d_name);
        if (entry->d_type == DT_DIR) {
            ret = CopyDirAndDelSrc(srcSubPath, destSubPath, curRecursionDepth) && ret;
            continue;
        }
        if (entry->d_type == DT_REG) {
            ret = CopyFileAndDelSrc(srcSubPath, destSubPath) && ret;
        } else {
            MEDIA_ERR_LOG("unknown file type,srcSubPath:%{private}s", srcSubPath.c_str());
            ret = false;
        }
    }

    closedir(srcDir);
    MEDIA_INFO_LOG("srcPath:%{private}s,destPath:%{private}s,coypPathAndDelSrcRet:%{public}d",
        srcPath.c_str(), destPath.c_str(), ret);
    return ret;
}

void MediaFileUtils::BackupPhotoDir()
{
    string dirPath = ROOT_MEDIA_DIR + PHOTO_BUCKET;
    // check whether dir empty
    if (!IsDirEmpty(dirPath)) {
        MEDIA_INFO_LOG("backup for: %{private}s", dirPath.c_str());
        string suffixName = dirPath.substr(ROOT_MEDIA_DIR.length());
        CreateDirectory(ROOT_MEDIA_DIR + MEDIALIBRARY_TEMP_DIR);
        CopyDirAndDelSrc(dirPath, ROOT_MEDIA_DIR + MEDIALIBRARY_TEMP_DIR + SLASH_STR + suffixName);
    }
}

void MediaFileUtils::RecoverMediaTempDir()
{
    string recoverPath = ROOT_MEDIA_DIR + MEDIALIBRARY_TEMP_DIR + SLASH_STR + PHOTO_BUCKET;
    if (!IsDirEmpty(recoverPath)) {
        DIR *dir = opendir((recoverPath).c_str());
        if (dir == nullptr) {
            MEDIA_ERR_LOG("Error opening temp directory, errno: %{public}d", errno);
            return;
        }
        
        struct dirent *entry;
        while ((entry = readdir(dir)) != nullptr) {
            // filter . && .. dir
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            std::string fullPath = recoverPath + SLASH_STR + entry->d_name;
            struct stat fileStat;
            if (stat(fullPath.c_str(), &fileStat) == -1) {
                closedir(dir);
                return;
            }
            string suffixName = fullPath.substr((recoverPath).length());
            CopyDirAndDelSrc(fullPath, ROOT_MEDIA_DIR + PHOTO_BUCKET + suffixName);
        }
        DeleteDir(ROOT_MEDIA_DIR + MEDIALIBRARY_TEMP_DIR);
        closedir(dir);
    }
}

bool MediaFileUtils::MoveFile(const string &oldPath, const string &newPath, bool isSupportCrossPolicy)
{
    bool errRet = false;
    if (!IsFileExists(oldPath) || IsFileExists(newPath)) {
        return errRet;
    }

    errRet = (rename(oldPath.c_str(), newPath.c_str()) == E_SUCCESS);
    if (!errRet && isSupportCrossPolicy && errno == CROSS_POLICY_ERR) {
        errRet = CopyFileAndDelSrc(oldPath, newPath);
    }
    return errRet;
}

bool MediaFileUtils::CopyFileUtil(const string &filePath, const string &newPath)
{
    struct stat fst{};
    bool errCode = false;
    if (filePath.size() >= PATH_MAX) {
        MEDIA_ERR_LOG("File path too long %{public}d", static_cast<int>(filePath.size()));
        return errCode;
    }
    MEDIA_DEBUG_LOG("File path is %{private}s", filePath.c_str());
    string absFilePath;
    if (!PathToRealPath(filePath, absFilePath)) {
        MEDIA_ERR_LOG("file is not real path, file path: %{private}s", filePath.c_str());
        return errCode;
    }
    if (absFilePath.empty()) {
        MEDIA_ERR_LOG("Failed to obtain the canonical path for source path:%{public}s %{public}d",
                      filePath.c_str(), errno);
        return errCode;
    }

    int32_t source = open(absFilePath.c_str(), O_RDONLY);
    if (source == -1) {
        MEDIA_ERR_LOG("Open failed for source file, errno: %{public}d", errno);
        return errCode;
    }

    int32_t dest = open(newPath.c_str(), O_WRONLY | O_CREAT, CHOWN_RO_USR_GRP);
    if (dest == -1) {
        MEDIA_ERR_LOG("Open failed for destination file %{public}d", errno);
        close(source);
        return errCode;
    }

    if (fstat(source, &fst) == E_SUCCESS) {
        // Copy file content
        if (sendfile(dest, source, nullptr, fst.st_size) != E_ERR) {
            // Copy ownership and mode of source file
            if (fchown(dest, fst.st_uid, fst.st_gid) == E_SUCCESS &&
                fchmod(dest, fst.st_mode) == E_SUCCESS) {
                errCode = true;
            }
        }
    }

    close(source);
    close(dest);

    return errCode;
}

static std::unique_ptr<Picture> DecodeAsset(int32_t fd)
{
    if (fd < 0) {
        MEDIA_ERR_LOG("fd = %{public}d is invalid", fd);
        return nullptr;
    }
    MediaLibraryTracer tracer;
    tracer.Start("DecodeAsset");

    SourceOptions opts;
    uint32_t err = E_OK;
    unique_ptr<ImageSource> imageSource = ImageSource::CreateImageSource(fd, opts, err);
    CHECK_AND_RETURN_RET_LOG(imageSource != nullptr, nullptr, "CreateImageSource failed, err: %{public}u", err);

    DecodingOptionsForPicture decodeOptions;
    std::unique_ptr<Picture> picturePtr = imageSource->CreatePicture(decodeOptions, err);
    CHECK_AND_RETURN_RET_LOG(picturePtr != nullptr, nullptr, "CreatePicture failed, err: %{public}u", err);
    return picturePtr;
}

static bool EncodeSaveAsset(std::unique_ptr<Picture> picturePtr, const std::string &mimeType, int32_t dstFd)
{
    if (picturePtr == nullptr) {
        MEDIA_ERR_LOG("picturePtr is nullptr");
        return false;
    }
    MediaLibraryTracer tracer;
    tracer.Start("EncodeSaveAsset");

    Media::ImagePacker imagePacker;
    Media::PackOption packOption;
    packOption.format = mimeType;
    packOption.desiredDynamicRange = EncodeDynamicRange::AUTO;
    packOption.needsPackProperties = true;

    int32_t ret = imagePacker.StartPacking(dstFd, packOption);
    CHECK_AND_RETURN_RET_LOG(ret == E_SUCCESS, false, "StartPacking failed, ret: %{public}d", ret);
    ret = imagePacker.AddPicture(*(picturePtr));
    CHECK_AND_RETURN_RET_LOG(ret == E_SUCCESS, false, "AddPicture failed, ret: %{public}d", ret);
    ret = imagePacker.FinalizePacking();
    CHECK_AND_RETURN_RET_LOG(ret == E_SUCCESS, false, "FinalizePacking failed, ret: %{public}d", ret);
    return true;
}

static bool DecodeEncodeSaveAsset(int32_t srcFd, int32_t dstFd, const std::string &extension)
{
    std::unique_ptr<Picture> picturePtr = DecodeAsset(srcFd);
    if (picturePtr == nullptr) {
        MEDIA_ERR_LOG("DecodeAsset failed");
        return false;
    }

    std::string mimeType = MimeTypeUtils::GetMimeTypeFromExtension(extension, MediaMapConstUtils::GetMimeTypeMap());
    if (!EncodeSaveAsset(std::move(picturePtr), mimeType, dstFd)) {
        MEDIA_ERR_LOG("EncodeAsset mimeType: %{public}s failed", mimeType.c_str());
        return false;
    }

    return true;
}

bool MediaFileUtils::ConvertFormatCopy(const std::string &srcFile, const std::string &dstFile,
    const std::string &extension)
{
    MEDIA_DEBUG_LOG("ConvertFormatCopy srcFile: %{public}s, dstFile: %{public}s, extension: %{public}s",
        srcFile.c_str(), dstFile.c_str(), extension.c_str());
    if (srcFile.size() >= PATH_MAX) {
        MEDIA_ERR_LOG("File path too long %{public}d", static_cast<int>(srcFile.size()));
        return false;
    }
    string absFilePath;
    if (!PathToRealPath(srcFile, absFilePath)) {
        MEDIA_ERR_LOG("file is not real path, file path: %{private}s", srcFile.c_str());
        return false;
    }
    if (absFilePath.empty()) {
        MEDIA_ERR_LOG("Failed to obtain the canonical path for source path:%{public}s %{public}d",
                      srcFile.c_str(), errno);
        return false;
    }

    auto normalizedDstPath = std::filesystem::absolute(dstFile).lexically_normal();
    if (normalizedDstPath.empty()) {
        MEDIA_ERR_LOG("Failed to obtain the canonical path for destination path:%{public}s", srcFile.c_str());
        return false;
    }
    UniqueFd srcFd(open(absFilePath.c_str(), O_RDONLY));
    if (srcFd.Get() == E_ERR) {
        MEDIA_ERR_LOG("Open failed for source file, errno: %{public}d", errno);
        return false;
    }
    UniqueFd dstFd(open(normalizedDstPath.c_str(), O_WRONLY | O_CREAT, CHOWN_RO_USR_GRP));
    if (dstFd.Get() == E_ERR) {
        MEDIA_ERR_LOG("Open failed for destination file %{public}d", errno);
        return false;
    }

    if (!DecodeEncodeSaveAsset(srcFd.Get(), dstFd.Get(), extension)) {
        MEDIA_ERR_LOG("DecodeEncodeSaveAsset failed");
        DeleteFile(normalizedDstPath);
        return false;
    }

    bool errCode = false;
    struct stat fst{};
    if (fstat(srcFd.Get(), &fst) == E_SUCCESS) {
        // Copy ownership and mode of source file
        if (fchown(dstFd.Get(), fst.st_uid, fst.st_gid) == E_SUCCESS && fchmod(dstFd.Get(), fst.st_mode) == E_SUCCESS) {
            errCode = true;
        }
    }

    return errCode;
}

static bool IsTranscodeFile(const std::string &filePath)
{
    std::string displayName = filePath;
    size_t pos = filePath.find_last_of("/");
    if (pos != std::string::npos) {
        displayName = filePath.substr(pos + 1);
    }
    if (displayName == "transcode.jpg" && filePath.find(".editData") != std::string::npos) {
        MEDIA_INFO_LOG("displayname %{private}s, success", displayName.c_str());
        return true;
    }
    return false;
}

bool MediaFileUtils::ConvertFormatExtraDataDirectory(const std::string &srcDir, const std::string &dstDir,
    const std::string &extension)
{
    if (srcDir.empty() || dstDir.empty()) {
        MEDIA_ERR_LOG("srcDir or dstDir is empty");
        return E_MODIFY_DATA_FAIL;
    }
    if (!IsFileExists(srcDir)) {
        MEDIA_ERR_LOG("SrcDir: %{public}s is not exist", srcDir.c_str());
        return E_NO_SUCH_FILE;
    }
    if (IsFileExists(dstDir)) {
        MEDIA_ERR_LOG("DstDir: %{public}s exists", dstDir.c_str());
        return E_FILE_EXIST;
    }
    if (!IsDirectory(srcDir) || !CreateDirectory(dstDir)) {
        MEDIA_ERR_LOG("srcDir is not directory or Create dstDir failed");
        return E_FAIL;
    }

    std::string sourceFilePath = srcDir + "/source." + GetExtensionFromPath(srcDir);
    for (const auto& entry : std::filesystem::recursive_directory_iterator(srcDir)) {
        std::string srcFilePath = entry.path();
        std::string tmpFilePath = srcFilePath;
        std::string dstFilePath = tmpFilePath.replace(0, srcDir.length(), dstDir);
        if (entry.is_directory()) {
            if (!CreateDirectory(dstFilePath)) {
                MEDIA_ERR_LOG("Create dir:%{public}s failed", DesensitizePath(dstFilePath).c_str());
                return E_FAIL;
            }
        } else if (entry.is_regular_file()) {
            CHECK_AND_CONTINUE(!IsTranscodeFile(srcFilePath));
            error_code ec;
            bool ret = false;
            bool isEqual = std::filesystem::equivalent(srcFilePath, sourceFilePath, ec);
            if (isEqual) {
                dstFilePath = dstDir + "/source." + extension;
                ret = ConvertFormatCopy(srcFilePath, dstFilePath, extension);
            } else {
                ret = CopyFileUtil(srcFilePath, dstFilePath);
            }
            if (!ret) {
                MEDIA_ERR_LOG("copyFile failed, isSrcFile: %{public}d", isEqual);
                return E_FAIL;
            }
        } else {
            MEDIA_ERR_LOG("Unhandled path type, path:%{public}s", DesensitizePath(srcFilePath).c_str());
            return E_FAIL;
        }
    }
    return E_OK;
}

bool MediaFileUtils::CopyFileSafe(const string &filePath, const string &newPath)
{
    if (newPath.empty()) {
        MEDIA_ERR_LOG("newPath is empty");
        return false;
    }
    string newTempPath = newPath + "_" + std::to_string(MediaFileUtils::UTCTimeMilliSeconds());
    if (!CopyFileUtil(filePath, newTempPath)) {
        MEDIA_ERR_LOG("copy file errno: %{public}d", errno);
        if (!DeleteFile(newTempPath)) {
            MEDIA_ERR_LOG("del temp file errno: %{public}d", errno);
        }
        return false;
    }
    return rename(newTempPath.c_str(), newPath.c_str()) == E_OK;
}

void MediaFileUtils::SetDeletionRecord(int fd, const string &fileName)
{
    unsigned int flags = 0;
    int ret = -1;
    ret = ioctl(fd, HMFS_IOCTL_HW_GET_FLAGS, &flags);
    if (ret < 0) {
        MEDIA_ERR_LOG("File %{public}s Failed to get flags, errno is %{public}d", fileName.c_str(), errno);
        return;
    }

    if (flags & HMFS_MONITOR_FL) {
        return;
    }
    flags |= HMFS_MONITOR_FL;
    ret = ioctl(fd, HMFS_IOCTL_HW_SET_FLAGS, &flags);
    if (ret < 0) {
        MEDIA_ERR_LOG("File %{public}s Failed to set flags, errno is %{public}d", fileName.c_str(), errno);
        return;
    }
    MEDIA_INFO_LOG("Flie %{public}s Set Delete control flags is success", fileName.c_str());
}

void MediaFileUtils::MediaFileDeletionRecord()
{
    int fd = -1;
    int bucketFd = -1;
    string path;
    struct dirent* dirEntry;
    DIR* fileDir;
    MEDIA_INFO_LOG("Set DeletionRecord for directory");
    for (auto &dir : SET_LISTEN_DIR) {
        path = LISTENING_BASE_PATH + dir;
        fd = open(path.c_str(), O_RDONLY);
        if (fd < 0) {
            MEDIA_ERR_LOG("Failed to open the Dir, errno is %{public}d, %{public}s", errno, dir.c_str());
            continue;
        }
        SetDeletionRecord(fd, dir);
        close(fd);
        if ((strcmp(dir.c_str(), ".thumbs") == 0) || (strcmp(dir.c_str(), ".editData") == 0)) {
            continue;
        }
        if ((fileDir = opendir(path.c_str())) == nullptr) {
            MEDIA_ERR_LOG("dir not exist: %{private}s, error: %{public}d", path.c_str(), errno);
            continue;
        }
        while ((dirEntry = readdir(fileDir)) != nullptr) {
            if ((strcmp(dirEntry->d_name, ".") == 0) || (strcmp(dirEntry->d_name, "..") == 0)) {
                continue;
            }
            std::string fileName = path + "/" + dirEntry->d_name;
            bucketFd = open(fileName.c_str(), O_RDONLY);
            if (bucketFd < 0) {
                MEDIA_ERR_LOG("Failed to open the bucketFd Dir error: %{public}d", errno);
                continue;
            }
            SetDeletionRecord(bucketFd, dirEntry->d_name);
            close(bucketFd);
        }
        closedir(fileDir);
    }
}

bool MediaFileUtils::WriteStrToFile(const string &filePath, const string &str)
{
    if (filePath.empty()) {
        MEDIA_ERR_LOG("FilePath is empty");
        return false;
    }
    if (str.empty()) {
        MEDIA_ERR_LOG("Write str is empty");
        return false;
    }

    if (!IsFileExists(filePath)) {
        MEDIA_ERR_LOG("Can not get FilePath %{private}s", filePath.c_str());
        return false;
    }

    string absFilePath;
    if (!PathToRealPath(filePath, absFilePath)) {
        MEDIA_ERR_LOG("file is not real path, file path: %{private}s", filePath.c_str());
        return false;
    }
    ofstream file(absFilePath);
    if (!file.is_open()) {
        MEDIA_ERR_LOG("Can not open FilePath %{private}s", absFilePath.c_str());
        return false;
    }

    file << str;
    file.close();
    if (!file.good()) {
        MEDIA_ERR_LOG("Can not write FilePath %{private}s", absFilePath.c_str());
        return false;
    }
    return true;
}

bool MediaFileUtils::ReadStrFromFile(const std::string &filePath, std::string &fileContent)
{
    if (filePath.empty()) {
        MEDIA_ERR_LOG("FilePath is empty");
        return false;
    }
    if (!IsFileExists(filePath)) {
        MEDIA_ERR_LOG("Can not get FilePath %{private}s", filePath.c_str());
        return false;
    }

    string absFilePath;
    if (!PathToRealPath(filePath, absFilePath)) {
        MEDIA_ERR_LOG("Failed to open a nullptr path %{private}s, errno=%{public}d", filePath.c_str(), errno);
        return false;
    }

    ifstream file(absFilePath);
    if (!file.is_open()) {
        MEDIA_ERR_LOG("Can not open FilePath %{private}s", absFilePath.c_str());
        return false;
    }
    char ch;
    while (file.get(ch)) {
        fileContent += ch;
    }
    file.close();
    return true;
}

bool MediaFileUtils::CopyFile(int32_t rfd, int32_t wfd)
{
    static const off_t sendSize1G = 1LL * 1024 * 1024 * 1024;
    static const off_t maxSendSize2G = 2LL * 1024 * 1024 * 1024;
    struct stat fst = {0};
    if (fstat(rfd, &fst) != 0) {
        MEDIA_INFO_LOG("fstat failed, errno=%{public}d", errno);
        return false;
    }
    off_t fileSize = fst.st_size;

    if (fileSize >= maxSendSize2G) {
        off_t offset = 0;
        while (offset < fileSize) {
            off_t sendSize = fileSize - offset;
            if (sendSize > sendSize1G) {
                sendSize = sendSize1G;
            }
            if (sendfile(wfd, rfd, &offset, sendSize) != sendSize) {
                MEDIA_INFO_LOG("send failed, errno=%{public}d", errno);
                return false;
            }
        }
    } else {
        if (sendfile(wfd, rfd, nullptr, fst.st_size) != fileSize) {
            MEDIA_INFO_LOG("send failed, errno=%{public}d", errno);
            return false;
        }
    }
    return true;
}

bool MediaFileUtils::RenameDir(const string &oldPath, const string &newPath)
{
    bool errRet = false;

    if (IsDirectory(oldPath)) {
        errRet = (rename(oldPath.c_str(), newPath.c_str()) == E_SUCCESS);
        if (!errRet) {
            MEDIA_ERR_LOG("Failed RenameDir errno %{public}d", errno);
        }
    }

    return errRet;
}

int32_t MediaFileUtils::CheckStringSize(const string &str, const size_t max)
{
    size_t size = str.length();
    if (size == 0) {
        return -EINVAL;
    }
    if (size > max) {
        return -ENAMETOOLONG;
    }
    return E_OK;
}

static inline bool RegexCheck(const string &str, const string &regexStr)
{
    const regex express(regexStr);
    return regex_search(str, express);
}

int32_t MediaFileUtils::CheckTitle(const string &title)
{
    if (title.empty()) {
        MEDIA_ERR_LOG("Title is empty.");
        return -EINVAL;
    }
    
    static const string titleRegexCheck = R"([\\/:*?"<>|])";
    if (RegexCheck(title, titleRegexCheck)) {
        MEDIA_ERR_LOG("Failed to check title regex: %{private}s", title.c_str());
        return -EINVAL;
    }
    return E_OK;
}

int32_t MediaFileUtils::CheckTitleCompatible(const string& title)
{
    CHECK_AND_RETURN_RET_LOG(!title.empty(), -EINVAL, "Title is empty.");
    static const string titleRegexCheck = R"([\.\\/:*?"'`<>|{}\[\]])";
    CHECK_AND_RETURN_RET_LOG(!RegexCheck(title, titleRegexCheck), -EINVAL,
        "Failed to check title regex: %{private}s", title.c_str());
    return E_OK;
}

std::string MediaFileUtils::GetFileAssetUri(const std::string &fileAssetData, const std::string &displayName,
    const int32_t &fileId)
{
    std::string filePath = fileAssetData;
    std::string baseUri = "file://media";
    size_t lastSlashInData = filePath.rfind('/');
    std::string fileNameInData =
        (lastSlashInData != std::string::npos) ? filePath.substr(lastSlashInData + 1) : filePath;
    size_t dotPos = fileNameInData.rfind('.');
    if (dotPos != std::string::npos) {
        fileNameInData = fileNameInData.substr(0, dotPos);
    }
    return baseUri + "/Photo/" + std::to_string(fileId) + "/" + fileNameInData + "/" + displayName;
}

int32_t MediaFileUtils::CheckDisplayName(const string &displayName, const bool compatibleCheckTitle)
{
    int err = CheckStringSize(displayName, DISPLAYNAME_MAX);
    if (err < 0) {
        return err;
    }
    if (displayName.at(0) == '.') {
        return -EINVAL;
    }
    string title = GetTitleFromDisplayName(displayName);
    if (title.empty()) {
        return -EINVAL;
    }

    if (compatibleCheckTitle) {
        // 为了向前兼容，此处进行老版本的title校验
        return CheckTitleCompatible(title);
    }
    return CheckTitle(title);
}

int32_t MediaFileUtils::CheckFileDisplayName(const string &displayName)
{
    int err = CheckStringSize(displayName, DISPLAYNAME_MAX);
    if (err < 0) {
        return err;
    }
    if (displayName.at(0) == '.') {
        return -EINVAL;
    }
    static const string TITLE_REGEX_CHECK = R"([\\/:*?"'`<>|{}\[\]])";
    if (RegexCheck(displayName, TITLE_REGEX_CHECK)) {
        MEDIA_ERR_LOG("Failed to check displayName regex: %{private}s", displayName.c_str());
        return -EINVAL;
    }
    return E_OK;
}

int32_t MediaFileUtils::CheckRelativePath(const std::string &relativePath)
{
    if (relativePath.empty()) {
        return -EINVAL;
    }

    size_t firstPoint = (relativePath.front() == '/') ? 1 : 0;
    size_t lastPoint = 0;
    while (true) {
        lastPoint = relativePath.find_first_of('/', firstPoint);
        if (lastPoint == string::npos) {
            lastPoint = relativePath.length();
        }
        size_t len = lastPoint - firstPoint;
        if (len == 0) {
            MEDIA_ERR_LOG("relativePath %{private}s is invalid", relativePath.c_str());
            return -EINVAL;
        }
        string checkedDirName = relativePath.substr(firstPoint, len);
        if (CheckDentryName(checkedDirName) != E_OK) {
            MEDIA_ERR_LOG("Dir Name %{private}s is invalid in path %{private}s",
                checkedDirName.c_str(), relativePath.c_str());
            return -EINVAL;
        }
        if (lastPoint == relativePath.length()) {
            break;
        }
        firstPoint = lastPoint + 1;
        if (firstPoint == relativePath.length()) {
            break;
        }
    }
    return E_OK;
}

bool MediaFileUtils::CheckDisplayLevel(const int32_t &displayLevel)
{
    if (PORTRAIT_PAGE_MODE.find(displayLevel) == PORTRAIT_PAGE_MODE.end()) {
        MEDIA_ERR_LOG("display level %{private}d is invalid", displayLevel);
        return false;
    }
    return true;
}

string MediaFileUtils::GetHighlightPath(const string &uri)
{
    int prefixLen = 0;
    string uriPrefix = "datashare:///media";
    if (uri.find(uriPrefix) != string::npos) {
        prefixLen = static_cast<int>(uriPrefix.length());
    } else if (uri.find(ML_FILE_URI_PREFIX) != string::npos) {
        prefixLen = static_cast<int>(ML_FILE_URI_PREFIX.length());
    } else {
        return "";
    }

    string path;
    if (IsFileExists(ROOT_MEDIA_DIR + HIGHLIGHT_INFO_OLD)) {
        path = "/storage/cloud/files/.thumbs" + uri.substr(prefixLen);
    } else {
        path = "/storage/cloud/files" + uri.substr(prefixLen);
    }
    
    return path;
}

string MediaFileUtils::GetHighlightVideoPath(const string &uri)
{
    int prefixLen = 0;
    string uriPrefix = "datashare:///media";
    if (uri.find(uriPrefix) != string::npos) {
        prefixLen = static_cast<int>(uriPrefix.length());
    } else if (uri.find(ML_FILE_URI_PREFIX) != string::npos) {
        prefixLen = static_cast<int>(ML_FILE_URI_PREFIX.length());
    } else {
        return "";
    }
    string path = "/storage/cloud/files" + uri.substr(prefixLen);
    return path;
}

void MediaFileUtils::FormatRelativePath(string &relativePath)
{
    if (relativePath.empty()) {
        return;
    }
    string FormatRelativePath = relativePath;
    if (relativePath.back() != '/') {
        relativePath += '/';
    }
    if (relativePath.front() == '/') {
        relativePath = relativePath.substr(1);
    }
}

void MediaFileUtils::GetRootDirFromRelativePath(const string &relativePath, string &rootDir)
{
    rootDir = relativePath;
    if (relativePath.empty()) {
        return;
    }
    if (relativePath.back() != '/') {
        rootDir += '/';
    }
    if (rootDir[0] == '/') {
        size_t dirIndex = rootDir.find_first_of('/', 1);
        if (dirIndex == string::npos) {
            return;
        }
        rootDir = rootDir.substr(1, dirIndex);
    } else {
        size_t dirIndex = rootDir.find_first_of('/');
        if (dirIndex == string::npos) {
            return;
        }
        rootDir = rootDir.substr(0, dirIndex + 1);
    }
}

int32_t MediaFileUtils::CheckAlbumName(const string &albumName)
{
    int err = CheckStringSize(albumName, DISPLAYNAME_MAX);
    if (err < 0) {
        MEDIA_ERR_LOG("Album name string size check failed: %{public}d, size is %{public}zu", err, albumName.length());
        return err;
    }

    static const string ALBUM_NAME_REGEX = R"([\.\\/:*?"'`<>|{}\[\]])";
    if (RegexCheck(albumName, ALBUM_NAME_REGEX)) {
        MEDIA_ERR_LOG("Failed to check album name regex: %{private}s", albumName.c_str());
        return -EINVAL;
    }
    return E_OK;
}

int32_t MediaFileUtils::CheckAppLink(const string &link)
{
    int err = CheckStringSize(link, APPLINK_MAX);
    if (err < 0) {
        MEDIA_ERR_LOG("link string size check failed: %{public}d, size is %{public}zu", err, link.length());
        return err;
    }
    return E_OK;
}
 
bool MediaFileUtils::CheckHasAppLink(int32_t hasLink)
{
    return hasLink >= HAS_APPLINK_MIN && hasLink <= HAS_APPLINK_MAX;
}

int32_t MediaFileUtils::CheckHighlightSubtitle(const string &highlightSubtitle)
{
    size_t size = highlightSubtitle.length();
    CHECK_AND_RETURN_RET_LOG(size <= DISPLAYNAME_MAX, -ENAMETOOLONG,
        "Highlight subtitle string size check failed: size is %{public}zu", highlightSubtitle.length());

    static const string ALBUM_NAME_REGEX = R"([\.\\/:*?"'`<>|{}\[\]])";
    CHECK_AND_RETURN_RET_LOG(!RegexCheck(highlightSubtitle, ALBUM_NAME_REGEX), -EINVAL,
        "Failed to check album name regex: %{private}s", highlightSubtitle.c_str());
    return E_OK;
}

int32_t MediaFileUtils::CheckDentryName(const string &dentryName)
{
    int err = CheckStringSize(dentryName, DISPLAYNAME_MAX);
    if (err < 0) {
        return err;
    }

    static const string DENTRY_REGEX_CHECK = R"([\\/:*?"'`<>|{}\[\]])";
    if (RegexCheck(dentryName, DENTRY_REGEX_CHECK)) {
        MEDIA_ERR_LOG("Failed to check dentry regex: %{private}s", dentryName.c_str());
        return -EINVAL;
    }
    return E_OK;
}

string MediaFileUtils::GetParentPath(const string &path)
{
    string name;
    size_t slashIndex = path.rfind("/");
    if (slashIndex != string::npos) {
        name = path.substr(0, slashIndex);
    }

    return name;
}

string MediaFileUtils::GetTitleFromDisplayName(const string &displayName)
{
    string title;
    if (!displayName.empty()) {
        string::size_type pos = displayName.find_last_of('.');
        if (pos == string::npos) {
            return "";
        }
        title = displayName.substr(0, pos);
    }
    return title;
}

int64_t MediaFileUtils::GetAlbumDateModified(const string &albumPath)
{
    struct stat statInfo {};
    if (!albumPath.empty() && stat(albumPath.c_str(), &statInfo) == 0) {
        return (statInfo.st_mtime);
    }
    return 0;
}

int64_t MediaFileUtils::UTCTimeSeconds()
{
    struct timespec t{};
    t.tv_sec = 0;
    t.tv_nsec = 0;
    clock_gettime(CLOCK_REALTIME, &t);
    return (int64_t)(t.tv_sec);
}

int64_t MediaFileUtils::UTCTimeMilliSeconds()
{
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return t.tv_sec * SEC_TO_MSEC + t.tv_nsec / MSEC_TO_NSEC;
}

int64_t MediaFileUtils::UTCTimeNanoSeconds()
{
    struct timespec t {};
    constexpr int64_t SEC_TO_NSEC = 1e9;
    clock_gettime(CLOCK_REALTIME, &t);
    return t.tv_sec * SEC_TO_NSEC + t.tv_nsec;
}

string MediaFileUtils::StrCreateTime(const string &format, int64_t time)
{
    char strTime[DEFAULT_TIME_SIZE] = "";
    CHECK_AND_RETURN_RET_LOG(time > 0, strTime, "invalid time: %{public}" PRId64, time);
    struct tm localTm;
    CHECK_AND_RETURN_RET_LOG(localtime_noenv_r(&time, &localTm) != nullptr, strTime,
        "localtime_noenv_r error: %{public}d", errno);
    CHECK_AND_PRINT_LOG(strftime(strTime, sizeof(strTime), format.c_str(), &localTm) != 0,
        "strftime error: %{public}d", errno);
    return strTime;
}

std::string MediaFileUtils::StrCreateTimeByMilliseconds(const string &format, int64_t time)
{
    char strTime[DEFAULT_TIME_SIZE] = "";
    int64_t times = time / MSEC_TO_SEC;
    struct tm localTm;

    if (localtime_noenv_r(&times, &localTm) == nullptr) {
        MEDIA_ERR_LOG("localtime_noenv_r error: %{public}d", errno);
        CHECK_AND_PRINT_LOG(time >= 0, "Time value is negative: %{public}lld", static_cast<long long>(time));
        return strTime;
    }

    if (strftime(strTime, sizeof(strTime), format.c_str(), &localTm) == 0) {
        MEDIA_ERR_LOG("strftime error: %{public}d", errno);
    }

    if (time < 0) {
        MEDIA_ERR_LOG("Time value is negative: %{public}lld", static_cast<long long>(time));
    }
    return strTime;
}

string MediaFileUtils::GetIdFromUri(const string &uri)
{
    return MediaFileUri(uri).GetFileId();
}

string MediaFileUtils::GetNetworkIdFromUri(const string &uri)
{
    return MediaFileUri(uri).GetNetworkId();
}

string MediaFileUtils::UpdatePath(const string &path, const string &uri)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaFileUtils::UpdatePath");

    string retStr = path;
    MEDIA_DEBUG_LOG("MediaFileUtils::UpdatePath path = %{private}s, uri = %{private}s", path.c_str(), uri.c_str());
    if (path.empty() || uri.empty()) {
        return retStr;
    }

    string networkId = GetNetworkIdFromUri(uri);
    if (networkId.empty()) {
        MEDIA_DEBUG_LOG("MediaFileUtils::UpdatePath retStr = %{private}s", retStr.c_str());
        return retStr;
    }

    size_t pos = path.find(MEDIA_DATA_DEVICE_PATH);
    if (pos == string::npos) {
        return retStr;
    }

    string beginStr = path.substr(0, pos);
    if (beginStr.empty()) {
        return retStr;
    }

    string endStr = path.substr(pos + MEDIA_DATA_DEVICE_PATH.length());
    CHECK_AND_RETURN_RET(!endStr.empty(), retStr);

    retStr = beginStr + networkId + endStr;
    MEDIA_DEBUG_LOG("MediaFileUtils::UpdatePath retStr = %{private}s", retStr.c_str());
    return retStr;
}

MediaType MediaFileUtils::GetMediaType(const string &filePath)
{
    if (filePath.empty()) {
        return MEDIA_TYPE_ALL;
    }

    string extention = GetExtensionFromPath(filePath);
    string mimeType = MimeTypeUtils::GetMimeTypeFromExtension(extention, MediaMapConstUtils::GetMimeTypeMap());
    return MimeTypeUtils::GetMediaTypeFromMimeType(mimeType);
}

MediaType MediaFileUtils::GetMediaTypeNotSupported(const string &filePath)
{
    if (filePath.empty()) {
        return MEDIA_TYPE_ALL;
    }

    string extention = GetExtensionFromPath(filePath);
    string mimeType = MimeTypeUtils::GetMimeTypeFromExtension(extention, MEDIA_EXTRA_MIME_TYPE_MAP);
    return MimeTypeUtils::GetMediaTypeFromMimeType(mimeType);
}

string MediaFileUtils::SplitByChar(const string &str, const char split)
{
    size_t splitIndex = str.find_last_of(split);
    return (splitIndex == string::npos) ? ("") : (str.substr(splitIndex + 1));
}

string MediaFileUtils::GetExtensionFromPath(const string &path)
{
    string extention = SplitByChar(path, '.');
    if (!extention.empty()) {
        transform(extention.begin(), extention.end(), extention.begin(), ::tolower);
    }
    return extention;
}

static void SendHmdfsCallerInfoToIoctl(const int32_t fd, const string &clientBundleName)
{
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    HmdfsCallerInfo callerInfo;
    callerInfo.tokenId = tokenId;

    if (strcpy_s(callerInfo.bundleName, sizeof(callerInfo.bundleName), clientBundleName.c_str()) != 0) {
        MEDIA_ERR_LOG("Failed to copy clientBundleName: %{public}s", clientBundleName.c_str());
    } else {
        MEDIA_DEBUG_LOG("clientBundleName = %{public}s", clientBundleName.c_str());
        int32_t ret = ioctl(fd, HMDFS_IOC_GET_CALLER_INFO, callerInfo);
        if (ret < 0) {
            MEDIA_DEBUG_LOG("Failed to set callerInfo to fd: %{public}d, error: %{public}d", fd, errno);
        }
    }
}

int32_t MediaFileUtils::OpenFile(const string &filePath, const string &mode, const string &clientbundleName)
{
    int32_t errCode = E_ERR;

    if (filePath.empty() || mode.empty()) {
        MEDIA_ERR_LOG("Invalid open argument! mode: %{private}s, path: %{private}s", mode.c_str(), filePath.c_str());
        return errCode;
    }

    static const unordered_map<string, int32_t> MEDIA_OPEN_MODE_MAP = {
        { MEDIA_FILEMODE_READONLY, O_RDONLY },
        { MEDIA_FILEMODE_WRITEONLY, O_WRONLY },
        { MEDIA_FILEMODE_READWRITE, O_RDWR },
        { MEDIA_FILEMODE_WRITETRUNCATE, O_WRONLY | O_TRUNC },
        { MEDIA_FILEMODE_WRITEAPPEND, O_WRONLY | O_APPEND },
        { MEDIA_FILEMODE_READWRITETRUNCATE, O_RDWR | O_TRUNC },
        { MEDIA_FILEMODE_READWRITEAPPEND, O_RDWR | O_APPEND },
    };
    if (MEDIA_OPEN_MODE_MAP.find(mode) == MEDIA_OPEN_MODE_MAP.end()) {
        return E_ERR;
    }

    if (filePath.size() >= PATH_MAX) {
        MEDIA_ERR_LOG("File path too long %{public}d", (int)filePath.size());
        return errCode;
    }
    string absFilePath;
    if (!PathToRealPath(filePath, absFilePath)) {
        MEDIA_ERR_LOG("file is not real path, file path: %{private}s", filePath.c_str());
        return errCode;
    }
    if (absFilePath.empty()) {
        MEDIA_ERR_LOG("Failed to obtain the canonical path for source path %{public}d %{private}s",
                      errno, filePath.c_str());
        return errCode;
    }
    MEDIA_DEBUG_LOG("File absFilePath is %{private}s", absFilePath.c_str());
    int32_t fd = open(absFilePath.c_str(), MEDIA_OPEN_MODE_MAP.at(mode));
    if (clientbundleName.empty()) {
        MEDIA_DEBUG_LOG("ClientbundleName is empty,failed to to set caller_info to fd");
    } else {
        SendHmdfsCallerInfoToIoctl(fd, clientbundleName);
    }
    return fd;
}

int32_t MediaFileUtils::CreateAsset(const string &filePath)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaFileUtils::CreateAsset");

    int32_t errCode = E_ERR;

    if (filePath.empty()) {
        MEDIA_ERR_LOG("Filepath is empty");
        return E_VIOLATION_PARAMETERS;
    }

    if (IsFileExists(filePath)) {
        MEDIA_ERR_LOG("the file exists path: %{public}s", filePath.c_str());
        return E_FILE_EXIST;
    }

    size_t slashIndex = filePath.rfind('/');
    if (slashIndex != string::npos) {
        string fileName = filePath.substr(slashIndex + 1);
        if (!fileName.empty() && fileName.at(0) != '.') {
            size_t dotIndex = filePath.rfind('.');
            if ((dotIndex == string::npos) && (GetMediaType(filePath) != MEDIA_TYPE_FILE)) {
                return errCode;
            }
        }
    }

    auto normalizedDstPath = std::filesystem::absolute(filePath).lexically_normal();
    ofstream file(normalizedDstPath);
    if (!file) {
        MEDIA_ERR_LOG("Output file path could not be created errno %{public}d", errno);
        return errCode;
    }

    file.close();

    return E_SUCCESS;
}

int32_t MediaFileUtils::ModifyAsset(const string &oldPath, const string &newPath)
{
    int32_t err = E_MODIFY_DATA_FAIL;

    if (oldPath.empty() || newPath.empty()) {
        MEDIA_ERR_LOG("Failed to modify asset, oldPath: %{private}s or newPath: %{private}s is empty!",
            oldPath.c_str(), newPath.c_str());
        return err;
    }
    if (!IsFileExists(oldPath)) {
        MEDIA_ERR_LOG("Failed to modify asset, oldPath: %{private}s does not exist!", oldPath.c_str());
        return E_NO_SUCH_FILE;
    }
    if (IsFileExists(newPath)) {
        MEDIA_ERR_LOG("Failed to modify asset, newPath: %{private}s is already exist!", newPath.c_str());
        return E_FILE_EXIST;
    }
    err = rename(oldPath.c_str(), newPath.c_str());
    if (err < 0) {
        int errorno = errno;
        MEDIA_ERR_LOG("Failed rename, errno: %{public}d, old path: %{public}s exists: %{public}d, "
            "new path: %{public}s exists: %{public}d", errorno, DesensitizePath(oldPath).c_str(),
            IsFileExists(oldPath), DesensitizePath(newPath).c_str(), IsFileExists(newPath));
        if (errorno == EACCES) {
            PrintStatInformation(GetParentPath(oldPath));
        }
        return E_FILE_OPER_FAIL;
    }

    return E_SUCCESS;
}

int32_t MediaFileUtils::OpenAsset(const string &filePath, const string &mode)
{
    if (filePath.empty()) {
        return E_INVALID_PATH;
    }
    if (mode.empty()) {
        return E_INVALID_MODE;
    }

    int32_t flags = O_RDWR;
    if (mode == MEDIA_FILEMODE_READONLY) {
        flags = O_RDONLY;
    } else if (mode == MEDIA_FILEMODE_WRITEONLY) {
        flags = O_WRONLY;
    } else if (mode == MEDIA_FILEMODE_WRITETRUNCATE) {
        flags = O_WRONLY | O_TRUNC;
    } else if (mode == MEDIA_FILEMODE_WRITEAPPEND) {
        flags = O_WRONLY | O_APPEND;
    } else if (mode == MEDIA_FILEMODE_READWRITETRUNCATE) {
        flags = O_RDWR | O_TRUNC;
    }

    if (filePath.size() >= PATH_MAX) {
        MEDIA_ERR_LOG("File path too long %{public}d", (int)filePath.size());
        return E_INVALID_PATH;
    }
    MEDIA_DEBUG_LOG("File path is %{private}s", filePath.c_str());
    std::string absFilePath;
    if (!PathToRealPath(filePath, absFilePath)) {
        MEDIA_ERR_LOG("file is not real path, file path: %{private}s", filePath.c_str());
        return E_INVALID_PATH;
    }
    CHECK_AND_RETURN_RET_LOG(!absFilePath.empty(), E_INVALID_PATH,
        "Failed to obtain the canonical path for source path %{private}s %{public}d", filePath.c_str(), errno);

    MEDIA_DEBUG_LOG("File absFilePath is %{private}s", absFilePath.c_str());
    return open(absFilePath.c_str(), flags);
}

int32_t MediaFileUtils::CloseAsset(int32_t fd)
{
    return close(fd);
}

std::string MediaFileUtils::GetMediaTypeUri(MediaType mediaType)
{
    switch (mediaType) {
        case MEDIA_TYPE_AUDIO:
            return MEDIALIBRARY_AUDIO_URI;
        case MEDIA_TYPE_VIDEO:
            return MEDIALIBRARY_VIDEO_URI;
        case MEDIA_TYPE_IMAGE:
            return MEDIALIBRARY_IMAGE_URI;
        case MEDIA_TYPE_SMARTALBUM:
            return MEDIALIBRARY_SMARTALBUM_CHANGE_URI;
        case MEDIA_TYPE_DEVICE:
            return MEDIALIBRARY_DEVICE_URI;
        case MEDIA_TYPE_FILE:
        default:
            return MEDIALIBRARY_FILE_URI;
    }
}

std::string MediaFileUtils::GetMediaTypeUriV10(MediaType mediaType)
{
    switch (mediaType) {
        case MEDIA_TYPE_AUDIO:
            return AudioColumn::DEFAULT_AUDIO_URI;
        case MEDIA_TYPE_VIDEO:
        case MEDIA_TYPE_IMAGE:
            return PhotoColumn::DEFAULT_PHOTO_URI;
        case MEDIA_TYPE_SMARTALBUM:
            return MEDIALIBRARY_SMARTALBUM_CHANGE_URI;
        case MEDIA_TYPE_DEVICE:
            return MEDIALIBRARY_DEVICE_URI;
        case MEDIA_TYPE_FILE:
        default:
            return MEDIALIBRARY_FILE_URI;
    }
}

bool MediaFileUtils::CheckMode(const string &mode)
{
    if (mode.empty()) {
        return false;
    }
    if (MEDIA_OPEN_MODES.find(mode) != MEDIA_OPEN_MODES.end()) {
        return true;
    } else {
        MEDIA_ERR_LOG("Input Mode %{private}s is invalid", mode.c_str());
        return false;
    }
}

size_t MediaFileUtils::FindIgnoreCase(const std::string &str, const std::string &key)
{
    auto it = search(str.begin(), str.end(), key.begin(), key.end(), [](const char a, const char b) {
        return ::tolower(a) == ::tolower(b);
    });
    if (it == str.end()) {
        return string::npos;
    }
    size_t pos = static_cast<size_t>(it - str.begin());
    return (pos > 0) ? pos : 0;
}

int64_t MediaFileUtils::GetVirtualIdByType(int32_t id, MediaType type)
{
    switch (type) {
        case MediaType::MEDIA_TYPE_IMAGE:
        case MediaType::MEDIA_TYPE_VIDEO: {
            return (int64_t) id * VIRTUAL_ID_DIVIDER - PHOTO_VIRTUAL_IDENTIFIER;
        }
        case MediaType::MEDIA_TYPE_AUDIO: {
            return (int64_t) id * VIRTUAL_ID_DIVIDER - AUDIO_VIRTUAL_IDENTIFIER;
        }
        default: {
            return (int64_t)id * VIRTUAL_ID_DIVIDER - FILE_VIRTUAL_IDENTIFIER;
        }
    }
}

double MediaFileUtils::GetRealIdByTable(int32_t virtualId, const string &tableName)
{
    if (tableName == PhotoColumn::PHOTOS_TABLE) {
        return (double) (virtualId + PHOTO_VIRTUAL_IDENTIFIER) / VIRTUAL_ID_DIVIDER;
    } else if (tableName == AudioColumn::AUDIOS_TABLE) {
        return (double) (virtualId + AUDIO_VIRTUAL_IDENTIFIER) / VIRTUAL_ID_DIVIDER;
    } else {
        return (double) (virtualId + FILE_VIRTUAL_IDENTIFIER) / VIRTUAL_ID_DIVIDER;
    }
}

string MediaFileUtils::GetVirtualUriFromRealUri(const string &uri, const string &extrUri)
{
    if ((uri.find(PhotoColumn::PHOTO_TYPE_URI) != string::npos) ||
        (uri.find(AudioColumn::AUDIO_TYPE_URI) != string::npos) ||
        (uri.find(PhotoColumn::HIGHTLIGHT_COVER_URI) != string::npos) ||
        (uri.find(URI_MTP_OPERATION) != string::npos)) {
        return uri;
    }

    string pureUri = uri;
    string suffixUri;
    size_t questionMaskPoint = uri.rfind('?');
    size_t hashKeyPoint = uri.rfind('#');
    if (questionMaskPoint != string::npos) {
        suffixUri = uri.substr(questionMaskPoint);
        pureUri = uri.substr(0, questionMaskPoint);
    } else if (hashKeyPoint != string::npos) {
        suffixUri = uri.substr(hashKeyPoint);
        pureUri = uri.substr(0, hashKeyPoint);
    }

    MediaFileUri fileUri(pureUri);
    string fileId = fileUri.GetFileId();
    if (!all_of(fileId.begin(), fileId.end(), ::isdigit)) {
        return uri;
    }
    int32_t id;
    if (!StrToInt(fileId, id)) {
        MEDIA_ERR_LOG("invalid fileuri %{private}s", uri.c_str());
        return uri;
    }
    int64_t virtualId;
    MediaType type;
    if ((pureUri.find(MEDIALIBRARY_TYPE_IMAGE_URI) != string::npos)) {
        type = MediaType::MEDIA_TYPE_IMAGE;
        virtualId = GetVirtualIdByType(id, MediaType::MEDIA_TYPE_IMAGE);
    } else if (pureUri.find(MEDIALIBRARY_TYPE_VIDEO_URI) != string::npos) {
        type = MediaType::MEDIA_TYPE_VIDEO;
        virtualId = GetVirtualIdByType(id, MediaType::MEDIA_TYPE_VIDEO);
    } else if ((pureUri.find(MEDIALIBRARY_TYPE_AUDIO_URI) != string::npos)) {
        type = MediaType::MEDIA_TYPE_AUDIO;
        virtualId = GetVirtualIdByType(id, MediaType::MEDIA_TYPE_AUDIO);
    } else {
        type = MediaType::MEDIA_TYPE_FILE;
        virtualId = GetVirtualIdByType(id, MediaType::MEDIA_TYPE_FILE);
    }
    MediaFileUri virtualUri(type, to_string(virtualId), fileUri.GetNetworkId(),
        (fileUri.IsApi10() ? MEDIA_API_VERSION_V10 : MEDIA_API_VERSION_V9),
        (fileUri.IsApi10() ? extrUri : ""));

    return suffixUri.empty() ? virtualUri.ToString() : virtualUri.ToString() + suffixUri;
}

void GetExtrParamFromUri(const std::string &uri, std::string &displayName)
{
    if (uri.find(PATH_PARA) != string::npos) {
        size_t lastSlashPosition = uri.rfind('/');
        if (lastSlashPosition != string::npos) {
            displayName = uri.substr(lastSlashPosition + 1);
        }
    }
}

void InitPureAndSuffixUri(string &pureUri, string &suffixUri, const string &uri)
{
    size_t questionMaskPoint = uri.rfind('?');
    size_t hashKeyPoint = uri.rfind('#');
    if (questionMaskPoint != string::npos) {
        suffixUri = uri.substr(questionMaskPoint);
        pureUri = uri.substr(0, questionMaskPoint);
    } else if (hashKeyPoint != string::npos) {
        suffixUri = uri.substr(hashKeyPoint);
        pureUri = uri.substr(0, hashKeyPoint);
    }
}

string MediaFileUtils::GetRealUriFromVirtualUri(const string &uri)
{
    if ((uri.find(PhotoColumn::PHOTO_TYPE_URI) != string::npos) ||
        (uri.find(AudioColumn::AUDIO_TYPE_URI) != string::npos) ||
        (uri.find(PhotoColumn::HIGHTLIGHT_COVER_URI) != string::npos) ||
        (uri.find(URI_MTP_OPERATION) != string::npos) ||
        (uri.find(MEDIA_FILEOPRN_OPEN_DEBUG_DB) != string::npos)) {
        return uri;
    }

    string pureUri = uri;
    string suffixUri;
    InitPureAndSuffixUri(pureUri, suffixUri, uri);
    MediaFileUri fileUri(pureUri);
    string fileId = fileUri.GetFileId();
    if (!all_of(fileId.begin(), fileId.end(), ::isdigit)) {
        return uri;
    }
    int32_t id;
    if (!StrToInt(fileId, id)) {
        MEDIA_ERR_LOG("invalid fileuri %{private}s", uri.c_str());
        return uri;
    }
    int32_t realId = 0;
    MediaType type;
    if ((pureUri.find(MEDIALIBRARY_TYPE_IMAGE_URI) != string::npos)) {
        type = MediaType::MEDIA_TYPE_IMAGE;
        realId = static_cast<int32_t>(GetRealIdByTable(id, PhotoColumn::PHOTOS_TABLE));
    } else if (pureUri.find(MEDIALIBRARY_TYPE_VIDEO_URI) != string::npos) {
        type = MediaType::MEDIA_TYPE_VIDEO;
        realId = static_cast<int32_t>(GetRealIdByTable(id, PhotoColumn::PHOTOS_TABLE));
    } else if ((pureUri.find(MEDIALIBRARY_TYPE_AUDIO_URI) != string::npos)) {
        type = MediaType::MEDIA_TYPE_AUDIO;
        realId = static_cast<int32_t>(GetRealIdByTable(id, AudioColumn::AUDIOS_TABLE));
    } else {
        type = MediaType::MEDIA_TYPE_FILE;
        realId = static_cast<int32_t>(GetRealIdByTable(id, MEDIALIBRARY_TABLE));
    }
    string extrUri;
    if (fileUri.IsApi10()) {
        string displayName;
        GetExtrParamFromUri(pureUri, displayName);
        extrUri = GetExtraUri(displayName, fileUri.GetFilePath(), false);
    }

    MediaFileUri realUri(type, to_string(realId), fileUri.GetNetworkId(),
        (fileUri.IsApi10() ? MEDIA_API_VERSION_V10 : MEDIA_API_VERSION_V9), (fileUri.IsApi10() ? extrUri : ""));

    if (suffixUri.empty()) {
        return realUri.ToString();
    }
    return realUri.ToString() + suffixUri;
}

#ifdef MEDIALIBRARY_COMPATIBILITY
string MediaFileUtils::GetTableFromVirtualUri(const std::string &virtualUri)
{
    MediaFileUri uri(virtualUri);
    if (!uri.IsValid()) {
        MEDIA_ERR_LOG("virtual uri:%{private}s is invalid", virtualUri.c_str());
        return "";
    }
    string virtualId = uri.GetFileId();
    if (virtualId.size() > VIRTUAL_ID_MAX_SIZE) {
        MEDIA_ERR_LOG("virtualId too long");
        return "";
    }
    if (std::all_of(virtualId.begin(), virtualId.end(), ::isdigit)) {
        int64_t id = stol(virtualId);
        int64_t remainNumber = id % VIRTUAL_ID_DIVIDER;
        switch (remainNumber) {
            case VIRTUAL_ID_DIVIDER - PHOTO_VIRTUAL_IDENTIFIER:
                return PhotoColumn::PHOTOS_TABLE;
            case VIRTUAL_ID_DIVIDER - AUDIO_VIRTUAL_IDENTIFIER:
                return AudioColumn::AUDIOS_TABLE;
            case VIRTUAL_ID_DIVIDER - FILE_VIRTUAL_IDENTIFIER:
                return MEDIALIBRARY_TABLE;
            default:
                MEDIA_ERR_LOG("virtualId:%{public}ld is wrong", (long) id);
                return "";
        }
    } else {
        MEDIA_ERR_LOG("virtual uri:%{private}s is invalid, can not get id", virtualUri.c_str());
        return "";
    }
}
#endif

bool MediaFileUtils::IsUriV10(const string &mediaType)
{
    return mediaType == URI_TYPE_PHOTO ||
        mediaType == URI_TYPE_PHOTO_ALBUM ||
        mediaType == URI_TYPE_AUDIO_V10;
}

bool MediaFileUtils::IsFileTablePath(const string &path)
{
    if (path.empty() || path.size() <= ROOT_MEDIA_DIR.size()) {
        return false;
    }

    if (path.find(ROOT_MEDIA_DIR) == string::npos) {
        return false;
    }

    string relativePath = path.substr(ROOT_MEDIA_DIR.size());
    if ((relativePath.find(DOCS_PATH) == 0)) {
        return true;
    }
    return false;
}

bool MediaFileUtils::IsPhotoTablePath(const string &path)
{
    if (path.empty() || path.size() <= ROOT_MEDIA_DIR.size()) {
        return false;
    }

    if (path.find(ROOT_MEDIA_DIR) == string::npos) {
        return false;
    }

    string relativePath = path.substr(ROOT_MEDIA_DIR.size());

    const vector<string> photoPathVector = {
        PHOTO_BUCKET, PIC_DIR_VALUES, VIDEO_DIR_VALUES, CAMERA_DIR_VALUES
    };
    for (auto &photoPath : photoPathVector) {
        if (relativePath.find(photoPath) == 0) {
            return true;
        }
    }
    return false;
}

bool MediaFileUtils::StartsWith(const std::string &str, const std::string &prefix)
{
    return str.compare(0, prefix.size(), prefix) == 0;
}

bool MediaFileUtils::EndsWith(const std::string &str, const std::string &suffix)
{
    if (str.length() < suffix.length()) {
        return false;
    }
    return str.rfind(suffix) == str.length() - suffix.length();
}

void MediaFileUtils::ReplaceAll(std::string &str, const std::string &from, const std::string &to)
{
    size_t startPos = 0;
    while ((startPos = str.find(from, startPos)) != std::string::npos) {
        str.replace(startPos, from.length(), to);
        startPos += to.length();
    }
}

void MediaFileUtils::UriAppendKeyValue(string &uri, const string &key, std::string value)
{
    string uriKey = key + '=';
    if (uri.find(uriKey) != string::npos) {
        return;
    }

    char queryMark = (uri.find('?') == string::npos) ? '?' : '&';
    string append = queryMark + key + '=' + value;

    size_t pos = uri.find('#');
    if (pos == string::npos) {
        uri += append;
    } else {
        uri.insert(pos, append);
    }
}

string MediaFileUtils::GetExtraUri(const string &displayName, const string &path, const bool isNeedEncode)
{
    string extraUri = "/" + GetTitleFromDisplayName(GetFileName(path)) + "/" + displayName;
    if (!isNeedEncode) {
        return extraUri;
    }
    return MediaFileUtils::Encode(extraUri);
}

string MediaFileUtils::GetUriByExtrConditions(const string &prefix, const string &fileId, const string &suffix)
{
    return prefix + fileId + suffix;
}

std::string MediaFileUtils::GetUriWithoutDisplayname(const string &uri)
{
    if (uri.empty()) {
        return uri;
    }

    auto index = uri.rfind("/");
    if (index == string::npos) {
        return uri;
    }

    return uri.substr(0, index + 1);
}

string MediaFileUtils::Encode(const string &uri)
{
    const unordered_set<char> uriCompentsSet = {
        ';', ',', '/', '?', ':', '@', '&',
        '=', '+', '$', '-', '_', '.', '!',
        '~', '*', '(', ')'
    };
    const int32_t encodeLen = 2;
    ostringstream outPutStream;
    outPutStream.fill('0');
    outPutStream << std::hex;

    for (unsigned char tmpChar : uri) {
        if (std::isalnum(tmpChar) || uriCompentsSet.find(tmpChar) != uriCompentsSet.end()) {
            outPutStream << tmpChar;
        } else {
            outPutStream << std::uppercase;
            outPutStream << '%' << std::setw(encodeLen) << static_cast<unsigned int>(tmpChar);
            outPutStream << std::nouppercase;
        }
    }

    return outPutStream.str();
}

string MediaFileUtils::AddDocsToRelativePath(const string &relativePath)
{
    if (MediaFileUtils::StartsWith(relativePath, DOC_DIR_VALUES) ||
        MediaFileUtils::StartsWith(relativePath, DOWNLOAD_DIR_VALUES)) {
        return DOCS_PATH + relativePath;
    }
    return relativePath;
}

string MediaFileUtils::RemoveDocsFromRelativePath(const string &relativePath)
{
    if (MediaFileUtils::StartsWith(relativePath, DOCS_PATH)) {
        return relativePath.substr(DOCS_PATH.size());
    }
    return relativePath;
}

int64_t MediaFileUtils::Timespec2Millisecond(const struct timespec &time)
{
    return time.tv_sec * MSEC_TO_SEC + time.tv_nsec / MSEC_TO_NSEC;
}

string MediaFileUtils::GetTempMovingPhotoVideoPath(const string &imagePath)
{
    size_t splitIndex = imagePath.find_last_of('.');
    size_t lastSlashIndex = imagePath.find_last_of('/');
    if (splitIndex == string::npos || (lastSlashIndex != string::npos && lastSlashIndex > splitIndex)) {
        return "";
    }
    return imagePath.substr(0, lastSlashIndex + 1) + "_temp" +
        imagePath.substr(lastSlashIndex + 1, splitIndex - lastSlashIndex - 1) + ".mp4";
}

string MediaFileUtils::GetMovingPhotoVideoPath(const string &imagePath)
{
    size_t splitIndex = imagePath.find_last_of('.');
    size_t lastSlashIndex = imagePath.find_last_of('/');
    if (splitIndex == string::npos || (lastSlashIndex != string::npos && lastSlashIndex > splitIndex)) {
        return "";
    }
    return imagePath.substr(0, splitIndex) + ".mp4";
}

string MediaFileUtils::GetOriMovingPhotoVideoPath(const string &imagePath)
{
    return MEDIA_EDIT_DATA_DIR + imagePath.substr(ROOT_MEDIA_DIR.length())+ "/source.mp4";
}

string MediaFileUtils::GetTempOriMovingPhotoVideoPath(const string &imagePath)
{
    string outPath = MEDIA_EDIT_DATA_DIR + imagePath.substr(ROOT_MEDIA_DIR.length()) + "/source.mp4";
    size_t lastSlashIndex = outPath.find_last_of('/');
    return outPath.substr(0, lastSlashIndex + 1) + "temp_" + outPath.substr(lastSlashIndex + 1);
}

bool MediaFileUtils::CheckMovingPhotoExtension(const string &extension)
{
    return IsMovingPhotoMimeType(MimeTypeUtils::GetMimeTypeFromExtension(
        extension, MediaMapConstUtils::GetMimeTypeMap()));
}

bool MediaFileUtils::IsMovingPhotoMimeType(const string &mimeType)
{
    // image of moving photo must be image/jpeg, image/heif or image/heic
    return mimeType == "image/jpeg" || mimeType == "image/heif" || mimeType == "image/heic";
}

bool MediaFileUtils::CheckMovingPhotoVideoExtension(const string &extension)
{
    // video of moving photo must be video/mp4
    return MimeTypeUtils::GetMimeTypeFromExtension(extension, MediaMapConstUtils::GetMimeTypeMap()) == "video/mp4";
}

bool MediaFileUtils::CheckMovingPhotoImage(const string &path)
{
    return CheckMovingPhotoExtension(GetExtensionFromPath(path));
}

bool MediaFileUtils::CheckMovingPhotoVideo(const string &path)
{
    string absFilePath;
    if (!PathToRealPath(path, absFilePath)) {
        MEDIA_ERR_LOG("Failed to get real path, path: %{private}s", path.c_str());
        return false;
    }
    if (absFilePath.empty()) {
        MEDIA_ERR_LOG("Failed to check path for %{private}s, errno: %{public}d", path.c_str(), errno);
        return false;
    }

    string extension = GetExtensionFromPath(absFilePath);
    if (!CheckMovingPhotoVideoExtension(extension)) {
        MEDIA_ERR_LOG("Failed to check extension (%{public}s) of moving photo video", extension.c_str());
        return false;
    }

    UniqueFd uniqueFd(open(absFilePath.c_str(), O_RDONLY));
    return CheckMovingPhotoVideo(uniqueFd);
}

bool MediaFileUtils::CheckMovingPhotoVideo(const UniqueFd &uniqueFd)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaFileUtils::CheckMovingPhotoVideo");

    if (uniqueFd.Get() <= 0) {
        MEDIA_ERR_LOG("Failed to open video of moving photo, errno = %{public}d", errno);
        return false;
    }
    struct stat64 st;
    if (fstat64(uniqueFd.Get(), &st) != 0) {
        MEDIA_ERR_LOG("Failed to get file state, errno = %{public}d", errno);
        return false;
    }

    shared_ptr<AVMetadataHelper> avMetadataHelper = AVMetadataHelperFactory::CreateAVMetadataHelper();
    if (avMetadataHelper == nullptr) {
        MEDIA_WARN_LOG("Failed to create AVMetadataHelper, ignore checking duration");
        return true;
    }

    int32_t err = avMetadataHelper->SetSource(uniqueFd.Get(), 0,
        static_cast<int64_t>(st.st_size), AV_META_USAGE_META_ONLY);
    if (err != 0) {
        MEDIA_ERR_LOG("SetSource failed for the given file descriptor, err = %{public}d", err);
        return false;
    }

    unordered_map<int32_t, string> resultMap = avMetadataHelper->ResolveMetadata();
    if (resultMap.find(AV_KEY_DURATION) == resultMap.end()) {
        MEDIA_ERR_LOG("AV_KEY_DURATION does not exist");
        return false;
    }
    string durationStr = resultMap.at(AV_KEY_DURATION);
    int32_t duration = std::atoi(durationStr.c_str());
    if (!CheckMovingPhotoVideoDuration(duration)) {
        MEDIA_ERR_LOG("Failed to check duration of moving photo video: %{public}d ms", duration);
        return false;
    }
    return true;
}

std::string MediaFileUtils::GetTableNameByDisplayName(const std::string &displayName)
{
    std::string extension;
    string::size_type currentPos = displayName.rfind('.');
    if (currentPos != std::string::npos) {
        extension = displayName.substr(currentPos + 1);
    }

    auto myTypeName = MimeTypeUtils::GetMimeTypeFromExtension(extension, MediaMapConstUtils::GetMimeTypeMap());
    MediaType type = MimeTypeUtils::GetMediaTypeFromMimeType(myTypeName);
    if (type == MEDIA_TYPE_AUDIO) {
        return AudioColumn::AUDIOS_TABLE;
    } else if (type == MEDIA_TYPE_IMAGE || type == MEDIA_TYPE_VIDEO) {
        return PhotoColumn::PHOTOS_TABLE;
    }
    return "";
}

bool MediaFileUtils::GetDateModified(const string &path, int64_t &dateModified)
{
    struct stat statInfo {};
    if (stat(path.c_str(), &statInfo) != E_OK) {
        MEDIA_ERR_LOG("stat error of %{private}s, errno: %{public}d", path.c_str(), errno);
        return false;
    }
    dateModified = Timespec2Millisecond(statInfo.st_mtim);
    return true;
}

bool MediaFileUtils::CheckMovingPhotoVideoDuration(int32_t duration)
{
    // duration of moving photo video must be 0~10 s
    constexpr int32_t MIN_DURATION_MS = 0;
    constexpr int32_t MAX_DURATION_MS = 10000;
    return duration > MIN_DURATION_MS && duration <= MAX_DURATION_MS;
}

bool MediaFileUtils::CheckMovingPhotoEffectMode(int32_t effectMode)
{
    return (effectMode >= static_cast<int32_t>(MovingPhotoEffectMode::EFFECT_MODE_START) &&
        effectMode <= static_cast<int32_t>(MovingPhotoEffectMode::EFFECT_MODE_END)) ||
        effectMode == static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY);
}

bool MediaFileUtils::GetFileSize(const std::string& filePath, size_t& size)
{
    struct stat statbuf;
    if (lstat(filePath.c_str(), &statbuf) == -1) {
        MEDIA_WARN_LOG("Failed to get file size, errno: %{public}d, path: %{private}s", errno, filePath.c_str());
        size = 0;
        return false;
    }
    if (statbuf.st_size < 0) {
        MEDIA_WARN_LOG("File size is negative, path: %{public}s", filePath.c_str());
        size = 0;
        return false;
    }
    size = static_cast<size_t>(statbuf.st_size);
    return true;
}

bool MediaFileUtils::SplitMovingPhotoUri(const std::string& uri, std::vector<std::string>& ret)
{
    const std::string split(MOVING_PHOTO_URI_SPLIT);
    if (uri.empty() || IsMediaLibraryUri(uri)) {
        MEDIA_ERR_LOG("Failed to split moving photo uri, uri=%{public}s", uri.c_str());
        return false;
    }
    std::string temp = uri;
    size_t pos = temp.find(split);
    uint32_t step = split.size();
    ret.push_back(temp.substr(0, pos));
    ret.push_back(temp.substr(pos + step));
    return true;
}

bool MediaFileUtils::IsMediaLibraryUri(const std::string& uri)
{
    return !uri.empty() && uri.find(MOVING_PHOTO_URI_SPLIT) == uri.npos;
}

void MediaFileUtils::CheckDirStatus(const std::unordered_set<std::string> &dirCheckSet, const std::string &dir)
{
    if (dirCheckSet.count(dir) == 0) {
        return;
    }
    PrintStatInformation(dir);
}

int32_t MediaFileUtils::CreateDirectoryAndCopyFiles(const std::string &srcDir, const std::string &dstDir)
{
    if (!MediaFileUtils::IsFileExists(srcDir)) {
        MEDIA_WARN_LOG("%{public}s doesn't exist, skip.", srcDir.c_str());
        return E_OK;
    }
    if (!MediaFileUtils::IsDirectory(srcDir)) {
        MEDIA_WARN_LOG("%{public}s is not an directory, skip.", srcDir.c_str());
        return E_OK;
    }
    if (!MediaFileUtils::CreateDirectory(dstDir)) {
        MEDIA_ERR_LOG("Create dstDir %{public}s failed", dstDir.c_str());
        return E_FAIL;
    }
    for (const auto &dirEntry : std::filesystem::directory_iterator{srcDir}) {
        std::string srcFilePath = dirEntry.path();
        std::string tmpFilePath = srcFilePath;
        std::string dstFilePath = tmpFilePath.replace(0, srcDir.length(), dstDir);
        if (!MediaFileUtils::CopyFileUtil(srcFilePath, dstFilePath)) {
            MEDIA_ERR_LOG("Copy file from %{public}s to %{public}s failed.",
                srcFilePath.c_str(),
                dstFilePath.c_str());
            return E_FAIL;
        }
    }
    return E_OK;
}

void MediaFileUtils::ModifyFile(const std::string path, int64_t modifiedTime)
{
    if (modifiedTime <= 0) {
        MEDIA_ERR_LOG("ModifyTime error!");
        return;
    }
    struct utimbuf buf;
    buf.actime = modifiedTime; // second
    buf.modtime = modifiedTime; // second
    int ret = utime(path.c_str(), &buf);
    if (ret != 0) {
        MEDIA_ERR_LOG("Modify file failed: %{public}d", ret);
    }
}

bool MediaFileUtils::CheckSupportedWatermarkType(int32_t watermarkType)
{
    return watermarkType >= static_cast<int32_t>(WatermarkType::BRAND_COMMON) &&
        watermarkType <= static_cast<int32_t>(WatermarkType::BRAND);
}

int32_t MediaFileUtils::CopyDirectory(const std::string &srcDir, const std::string &dstDir)
{
    if (srcDir.empty() || dstDir.empty()) {
        MEDIA_ERR_LOG("Failed to copy directory, srcDir:%{public}s or newPath:%{public}s is empty!",
            DesensitizePath(srcDir).c_str(), DesensitizePath(dstDir).c_str());
        return E_MODIFY_DATA_FAIL;
    }
    if (!IsFileExists(srcDir)) {
        MEDIA_ERR_LOG("SrcDir:%{public}s is not exist", DesensitizePath(srcDir).c_str());
        return E_NO_SUCH_FILE;
    }
    if (!IsDirectory(srcDir)) {
        MEDIA_ERR_LOG("SrcDir:%{public}s is not directory", DesensitizePath(srcDir).c_str());
        return E_FAIL;
    }
    if (IsFileExists(dstDir)) {
        MEDIA_ERR_LOG("DstDir:%{public}s exists", DesensitizePath(dstDir).c_str());
        return E_FILE_EXIST;
    }
    if (!CreateDirectory(dstDir)) {
        MEDIA_ERR_LOG("Create dstDir:%{public}s failed", DesensitizePath(dstDir).c_str());
        return E_FAIL;
    }
 
    for (const auto& entry : std::filesystem::recursive_directory_iterator(srcDir)) {
        std::string srcFilePath = entry.path();
        std::string tmpFilePath = srcFilePath;
        std::string dstFilePath = tmpFilePath.replace(0, srcDir.length(), dstDir);
        if (entry.is_directory()) {
            if (!CreateDirectory(dstFilePath)) {
                MEDIA_ERR_LOG("Create dir:%{public}s failed", DesensitizePath(dstFilePath).c_str());
                return E_FAIL;
            }
        } else if (entry.is_regular_file()) {
            CHECK_AND_CONTINUE(!IsTranscodeFile(srcFilePath));
            if (!CopyFileUtil(srcFilePath, dstFilePath)) {
                MEDIA_ERR_LOG("Copy file from %{public}s to %{public}s failed.",
                    DesensitizePath(srcFilePath).c_str(), DesensitizePath(dstFilePath).c_str());
                return E_FAIL;
            }
        } else {
            MEDIA_ERR_LOG("Unhandled path type, path:%{public}s", DesensitizePath(srcFilePath).c_str());
            return E_FAIL;
        }
    }
    return E_OK;
}

int32_t MediaFileUtils::MoveDirectory(const std::string &srcDir, const std::string &dstDir)
{
    CHECK_AND_RETURN_RET_LOG(!srcDir.empty() && !dstDir.empty(),
        E_MODIFY_DATA_FAIL,
        "Failed to copy directory, srcDir:%{public}s or newPath:%{public}s is empty!",
        DesensitizePath(srcDir).c_str(),
        DesensitizePath(dstDir).c_str());
    CHECK_AND_RETURN_RET_LOG(
        IsFileExists(srcDir), E_NO_SUCH_FILE, "SrcDir:%{public}s is not exist", DesensitizePath(srcDir).c_str());
    CHECK_AND_RETURN_RET_LOG(
        IsDirectory(srcDir), E_FAIL, "SrcDir:%{public}s is not directory", DesensitizePath(srcDir).c_str());
    CHECK_AND_RETURN_RET_LOG(
        !IsFileExists(dstDir), E_FILE_EXIST, "DstDir:%{public}s exists", DesensitizePath(dstDir).c_str());
    CHECK_AND_RETURN_RET_LOG(
        CreateDirectory(dstDir), E_FAIL, "Create dstDir:%{public}s failed", DesensitizePath(dstDir).c_str());

    for (const auto &entry : std::filesystem::recursive_directory_iterator(srcDir)) {
        std::string srcFilePath = entry.path();
        std::string tmpFilePath = srcFilePath;
        std::string dstFilePath = tmpFilePath.replace(0, srcDir.length(), dstDir);
        if (entry.is_directory()) {
            CHECK_AND_RETURN_RET_LOG(CreateDirectory(dstFilePath),
                E_FAIL,
                "Create dir:%{public}s failed",
                DesensitizePath(dstFilePath).c_str());
        } else if (entry.is_regular_file()) {
            if (srcFilePath.find("editdata_camera") != std::string::npos ||
                srcFilePath.find("source_back") != std::string::npos) {
                CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CopyFileUtil(srcFilePath, dstFilePath),
                    E_FAIL, "Copy editdata_camera from %{public}s to %{public}s failed.",
                    DesensitizePath(srcFilePath).c_str(),
                    DesensitizePath(dstFilePath).c_str());
                continue;
            }
            CHECK_AND_RETURN_RET_LOG(MoveFile(srcFilePath, dstFilePath),
                E_FAIL,
                "Copy file from %{public}s to %{public}s failed.",
                DesensitizePath(srcFilePath).c_str(),
                DesensitizePath(dstFilePath).c_str());
        } else {
            MEDIA_ERR_LOG("Unhandled path type, path:%{public}s", DesensitizePath(srcFilePath).c_str());
            return E_FAIL;
        }
    }
    return E_OK;
}

bool MediaFileUtils::CheckCompositeDisplayMode(int32_t compositeDisplayMode)
{
    return compositeDisplayMode >= static_cast<int32_t>(CompositeDisplayMode::DEFAULT) &&
        compositeDisplayMode <= static_cast<int32_t>(CompositeDisplayMode::CLOUD_ENHANCEMENT);
}

bool MediaFileUtils::IsCalledBySelf()
{
    if (IPCSkeleton::GetCallingFullTokenID() == IPCSkeleton::GetSelfTokenID()) {
        return E_OK;
    }
    return E_FAIL;
}

bool MediaFileUtils::GenerateKvStoreKey(const std::string &fileId, const std::string &dateKey, std::string &key)
{
    if (fileId.empty() || dateKey.empty()) {
        MEDIA_ERR_LOG("FileId:%{public}s or dateKey:%{public}s is empty", fileId.c_str(), dateKey.c_str());
        return false;
    }

    if (fileId.length() > KVSTORE_FILE_ID_TEMPLATE.length() ||
        dateKey.length() > KVSTORE_DATE_KEY_TEMPLATE.length()) {
        MEDIA_ERR_LOG("FileId:%{public}s or dateKey:%{public}s is too long", fileId.c_str(), dateKey.c_str());
        return false;
    }
    key = KVSTORE_DATE_KEY_TEMPLATE.substr(dateKey.length()) + dateKey +
          KVSTORE_FILE_ID_TEMPLATE.substr(fileId.length()) + fileId;
    return true;
}

bool MediaFileUtils::IsValidInteger(const std::string &value)
{
    MEDIA_DEBUG_LOG("KeyWord is:%{public}s", value.c_str());
    std::string unsignedStr = value;
    while (unsignedStr.size() > 0 && unsignedStr[0] == '-') {
        unsignedStr = unsignedStr.substr(1);
    }
    for (size_t i = 0; i < unsignedStr.size(); i++) {
        if (!std::isdigit(unsignedStr[i])) {
            MEDIA_INFO_LOG("KeyWord invalid char of:%{public}c", unsignedStr[i]);
            unsignedStr = unsignedStr.substr(0, i);
            break;
        }
    }
    if (unsignedStr.size() == 0) {
        MEDIA_ERR_LOG("KeyWord Invalid argument");
        return false;
    } else if (unsignedStr.size() < INTEGER_MAX_LENGTH) {
        return true;
    } else if (unsignedStr.size() == INTEGER_MAX_LENGTH) {
        return unsignedStr < MAX_INTEGER;
    } else {
        MEDIA_ERR_LOG("KeyWord is out length!");
        return false;
    }
}

static int64_t GetRoundSize(int64_t size)
{
    uint64_t val = 1;
    int64_t multple = UNIT;
    int64_t stdMultiple = STD_UNIT;
    while (static_cast<int64_t>(val) * stdMultiple < size) {
        val <<= 1;
        if (val > THRESHOLD) {
            val = 1;
            multple *= UNIT;
            stdMultiple *= STD_UNIT;
        }
    }
    return static_cast<int64_t>(val) * multple;
}

int64_t MediaFileUtils::GetTotalSize()
{
    struct statvfs diskInfo;
    int ret = statvfs(DATA_PATH.c_str(), &diskInfo);
    CHECK_AND_RETURN_RET_LOG(ret == 0, E_ERR, "Get total size failed, errno:%{public}d", errno);
    int64_t totalSize = static_cast<long long>(diskInfo.f_bsize) * static_cast<long long>(diskInfo.f_blocks);
    CHECK_AND_RETURN_RET_LOG(totalSize > 0, E_ERR, "Get total size failed, totalSize:%{public}" PRId64, totalSize);
    totalSize = GetRoundSize(totalSize);
    return totalSize;
}

int64_t MediaFileUtils::GetFreeSize()
{
    struct statvfs diskInfo;
    int ret = statvfs(DATA_PATH.c_str(), &diskInfo);
    CHECK_AND_RETURN_RET_LOG(ret == 0, E_ERR, "Get free size failed, errno:%{public}d", errno);
    int64_t freeSize = static_cast<int64_t>(diskInfo.f_bsize) * static_cast<int64_t>(diskInfo.f_bfree);
    return freeSize;
}

void MediaFileUtils::StatDirSize(const std::string& rootPath, size_t& totalSize)
{
    std::stack<std::string> dirStack;
    dirStack.push(rootPath);

    while (!dirStack.empty()) {
        std::string currentPath = dirStack.top();
        dirStack.pop();

        DIR* dir = opendir(currentPath.c_str());
        if (dir == nullptr) {
            MEDIA_ERR_LOG("Failed to open directory.");
            continue;
        }

        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            std::string fullPath = currentPath + "/" + entry->d_name;
            struct stat statBuf;
            if (stat(fullPath.c_str(), &statBuf) == -1) {
                MEDIA_ERR_LOG("Failed to get file status.");
                continue;
            }

            if (S_ISDIR(statBuf.st_mode)) {
                dirStack.push(fullPath);
            } else if (S_ISREG(statBuf.st_mode)) {
                size_t fileSize = 0;
                MediaFileUtils::GetFileSize(fullPath, fileSize);
                MEDIA_DEBUG_LOG("GetFileSize, file: %{public}s, size: %{public}lld bytes",
                    fullPath.c_str(), static_cast<long long>(fileSize));
                totalSize += fileSize;
            }
        }

        closedir(dir);
    }

    MEDIA_INFO_LOG("Directory size: %{public}lld bytes", static_cast<long long>(totalSize));
}

std::string MediaFileUtils::GetMimeTypeFromDisplayName(const std::string &displayName)
{
    std::string mimeType = "";
    CHECK_AND_RETURN_RET_LOG(!displayName.empty(), mimeType, "displayName is empty.");
    std::string extension;
    string::size_type currentPos = displayName.rfind('.');
    if (currentPos != std::string::npos) {
        extension = displayName.substr(currentPos + 1);
    }
    CHECK_AND_RETURN_RET_LOG(!extension.empty(), mimeType, "extension is empty.");
    mimeType = MimeTypeUtils::GetMimeTypeFromExtension(extension, MediaMapConstUtils::GetMimeTypeMap());
    return mimeType;
}

std::string MediaFileUtils::DesensitizeUri(const std::string &fileUri)
{
    string result = fileUri;
    size_t slashIndex = result.rfind('/');
    CHECK_AND_RETURN_RET(slashIndex != std::string::npos, result);
    return result.replace(slashIndex + 1, result.length() - slashIndex - 1, "*");
}

bool MediaFileUtils::DeleteFileOrFolder(const std::string &path, bool isFile)
{
    CHECK_AND_RETURN_RET(MediaFileUtils::IsFileExists(path), true);
    return isFile ? MediaFileUtils::DeleteFile(path) : MediaFileUtils::DeleteDir(path);
}

std::string MediaFileUtils::GetReplacedPathByPrefix(const std::string srcPrefix, const std::string dstPrefix,
    const std::string &path)
{
    std::string replacedPath = path;
    replacedPath.replace(0, srcPrefix.length(), dstPrefix);
    return replacedPath;
}

int64_t MediaFileUtils::GetFileModificationTime(const std::string &path)
{
    struct stat fileStat;

    if (stat(path.c_str(), &fileStat) == -1) {
        MEDIA_ERR_LOG("stat failed. File path: %{public}s, err: %{public}s", path.c_str(), strerror(errno));
        return 0;
    }
    struct tm* timeInfo = localtime(&fileStat.st_mtime);
    if (timeInfo == nullptr) {
        return 0;
    }
    return static_cast<int64_t>(fileStat.st_mtime);
}

bool MediaFileUtils::IsDirExists(const std::string &path)
{
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    return false;
}

int64_t MediaFileUtils::StrToInt64(const std::string &value)
{
    char* pEnd = nullptr;
    errno = 0;
    // check whether convert success
    int64_t res = std::strtoll(value.c_str(), &pEnd, 10);
    if (errno == ERANGE || pEnd == value.c_str() || *pEnd != '\0') {
        MEDIA_ERR_LOG("%{public}s:convert err.", __func__);
        return 0;
    }
    return res;
}

int32_t MediaFileUtils::UpdateModifyTimeInMsec(const std::string &localPath, int64_t localMtimeInMsec)
{
    CHECK_AND_RETURN_RET_LOG(localMtimeInMsec > 0, E_VIOLATION_PARAMETERS,
        "ModifyTime %{public}" PRId64 "<= 0, localPath: %{public}s",
        localMtimeInMsec, DesensitizePath(localPath).c_str());
    CHECK_AND_RETURN_RET_LOG(access(localPath.c_str(), F_OK) == 0, E_NO_SUCH_FILE,
        "File not exist, localPath: %{public}s", DesensitizePath(localPath).c_str());

    struct timeval times[2];
    // set atime
    times[0].tv_sec = static_cast<time_t>(localMtimeInMsec / MSEC_TO_SEC);
    times[0].tv_usec = static_cast<suseconds_t>((localMtimeInMsec % MSEC_TO_SEC) * MSEC_TO_SEC);
    // set mtime
    times[1] = times[0];

    CHECK_AND_RETURN_RET_LOG(utimes(localPath.c_str(), times) >= 0, errno,
        "utimes failed %{public}d, localPath: %{public}s", errno, DesensitizePath(localPath).c_str());
    return E_OK;
}

double MediaFileUtils::CalculateAspectRatio(int32_t height, int32_t width)
{
    if (height <= 0 || width <= 0) {
        return ASPECT_RATIO_UNSUPPORT;
    }

    double aspect_ratio = static_cast<double>(width) / height;
    if (aspect_ratio > ASPECT_RATIO_MAX) {
        return ASPECT_RATIO_MAX;
    } else if (aspect_ratio < ASPECT_RATIO_MIN) {
        return ASPECT_RATIO_MIN;
    } else {
        return std::round(aspect_ratio * ASPECT_RATIO_PRECISION) / ASPECT_RATIO_PRECISION;
    }
}
} // namespace OHOS::Media