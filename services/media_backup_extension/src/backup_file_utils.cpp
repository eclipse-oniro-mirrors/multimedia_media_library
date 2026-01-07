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

#define MLOG_TAG "MediaLibraryBackupUtils"

#include "backup_file_utils.h"

#include <utime.h>
#include <sys/types.h>

#include "backup_const_column.h"
#include "datashare_helper.h"
#include "scanner_utils.h"
#include "media_file_uri.h"
#include "metadata_extractor.h"
#include "mimetype_utils.h"
#include "medialibrary_errno.h"
#include "media_log.h"
#include "media_file_utils.h"
#include "media_image_framework_utils.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_data_manager_utils.h"
#include "moving_photo_file_utils.h"
#include "post_proc.h"
#include "thumbnail_utils.h"
#include "ohos_account_kits.h"

namespace OHOS {
namespace Media {
const string DEFAULT_IMAGE_NAME = "IMG_";
const string DEFAULT_VIDEO_NAME = "VID_";
const string DEFAULT_AUDIO_NAME = "AUD_";
const string LOW_QUALITY_PATH = "Documents/cameradata/";
const size_t INVALID_RET = -1;
const int32_t APP_TWIN_DATA_START = 128;
const int32_t APP_TWIN_DATA_END = 147;
const int32_t CROSS_POLICY_ERR = 18;

constexpr int ASSET_MAX_COMPLEMENT_ID = 999;
std::shared_ptr<DataShare::DataShareHelper> BackupFileUtils::sDataShareHelper_ = nullptr;
std::shared_ptr<FileAccessHelper> BackupFileUtils::fileAccessHelper_ = std::make_shared<FileAccessHelper>();

const std::string BackupFileUtils::IMAGE_FORMAT = "image/jpeg";
const std::string BackupFileUtils::LCD_FILE_NAME = "LCD.jpg";
const std::string BackupFileUtils::THM_FILE_NAME = "THM.jpg";
const uint8_t BackupFileUtils::IMAGE_QUALITY = 90;
const uint32_t BackupFileUtils::IMAGE_NUMBER_HINT = 1;
const int32_t BackupFileUtils::IMAGE_MIN_BUF_SIZE = 8192;
static const std::string CLOUD_BASE_URI = "datashareproxy://generic.cloudstorage";
static const std::string CLOUD_SYNC_SWITCH_URI = CLOUD_BASE_URI + "/sync_switch";
static const std::string MOBILE_NETWORK_STATUS_ON = "1";

bool FileAccessHelper::GetValidPath(string &filePath)
{
    if (access(filePath.c_str(), F_OK) == 0) {
        return true;
    }

    string resultPath = filePath;
    size_t pos = 0;
    while ((pos = resultPath.find("/", pos + 1)) != string::npos) {
        string curPath = resultPath.substr(0, pos);
        if (!ConvertCurrentPath(curPath, resultPath)) {
            MEDIA_ERR_LOG("convert fail, path: %{public}s", MediaFileUtils::DesensitizePath(filePath).c_str());
            return false;
        }
    }

    string curPath = resultPath;
    if (!ConvertCurrentPath(curPath, resultPath)) {
        MEDIA_ERR_LOG("convert fail, path: %{public}s", MediaFileUtils::DesensitizePath(filePath).c_str());
        return false;
    }

    filePath = resultPath;
    return true;
}

bool FileAccessHelper::ConvertCurrentPath(string &curPath, string &resultPath)
{
    if (access(curPath.c_str(), F_OK) == 0) {
        return true;
    }

    string parentDir = filesystem::path(curPath).parent_path().string();
    string lowerCurPath = curPath;
    transform(lowerCurPath.begin(), lowerCurPath.end(), lowerCurPath.begin(), ::tolower);
    {
        std::lock_guard<std::mutex> guard(mapMutex);
        if (pathMap.find(lowerCurPath) != pathMap.end()) {
            resultPath.replace(0, curPath.length(), pathMap[lowerCurPath]);
            return true;
        }
    }
    if (!MediaFileUtils::IsFileExists(parentDir)) {
        MEDIA_WARN_LOG("%{public}s doesn't exist, skip.",
            BackupFileUtils::GarbleFilePath(parentDir, DEFAULT_RESTORE_ID).c_str());
        return false;
    }
    std::error_code ec;
    auto iter = filesystem::directory_iterator(parentDir,
        std::filesystem::directory_options::skip_permission_denied, ec);
    if (ec) {
        MEDIA_WARN_LOG("directory_iterator error for %{public}s, skip. ec: %{public}s",
            BackupFileUtils::GarbleFilePath(parentDir, DEFAULT_RESTORE_ID).c_str(), ec.message().c_str());
        return false;
    }

    for (const auto &entry : iter) {
        string entryPath = entry.path().string();
        string lowerEntryPath = entryPath;
        transform(lowerEntryPath.begin(), lowerEntryPath.end(), lowerEntryPath.begin(), ::tolower);
        if (lowerEntryPath == lowerCurPath) {
            resultPath.replace(0, curPath.length(), entry.path().string());
            {
                std::lock_guard<std::mutex> guard(mapMutex);
                pathMap[lowerCurPath] = entry.path().string();
            }
            return true;
        }
    }

    return false;
}

int32_t BackupFileUtils::FillMetadata(std::unique_ptr<Metadata> &data)
{
    int32_t err = GetFileMetadata(data);
    if (err != E_OK) {
        MEDIA_ERR_LOG("failed to get file metadata");
        return err;
    }
    if (data->GetFileMediaType() == MEDIA_TYPE_IMAGE) {
        err = MetadataExtractor::ExtractImageMetadata(data);
    } else {
        err = MetadataExtractor::ExtractAVMetadata(data, Scene::AV_META_SCENE_CLONE);
        MEDIA_INFO_LOG("Extract av metadata end");
    }
    if (err != E_OK) {
        MEDIA_ERR_LOG("failed to extension data");
        return err;
    }
    return E_OK;
}

string BackupFileUtils::ConvertLowQualityPath(int32_t sceneCode, const std::string &filePath,
    const string &relativePath)
{
    string result = filePath;
    size_t displayNameIndex = result.rfind("/");
    if (displayNameIndex == string::npos) {
        return result;
    }
    std::string displayName = result.substr(displayNameIndex + 1);
    size_t dotPos = displayName.find_last_of(".");
    if (dotPos != string::npos) {
        displayName.replace(dotPos, displayName.length() - dotPos, ".camera");
    }
    size_t pos = result.find(relativePath);
    CHECK_AND_RETURN_RET(pos != string::npos, result);
    string publicPath = result.substr(0, pos + 1);
    result = publicPath + LOW_QUALITY_PATH + displayName;
    return result;
}

void BackupFileUtils::ParseResolution(const std::string &resolution, int32_t &width,
    int32_t &height)
{
    //default 0
    width = 0, height = 0;
    CHECK_AND_RETURN(!resolution.empty());
    size_t delimiter_pos = resolution.find('x');
    CHECK_AND_RETURN(delimiter_pos != std::string::npos);
    std::string width_str = resolution.substr(0, delimiter_pos);
    std::string height_str = resolution.substr(delimiter_pos + 1);
    width = std::atoi(width_str.c_str());
    height = std::atoi(height_str.c_str());
    bool cond = (width == 0 || height == 0);
    CHECK_AND_EXECUTE(!cond,  height = width = 0);
}

int32_t BackupFileUtils::GetFileMetadata(std::unique_ptr<Metadata> &data)
{
    string extension = ScannerUtils::GetFileExtension(data->GetFileName()); // in case when trashed or hidden
    string mimeType = MimeTypeUtils::GetMimeTypeFromExtension(extension);
    data->SetFileExtension(extension);
    data->SetFileMimeType(mimeType);
    std::string path = data->GetFilePath();
    struct stat statInfo {};
    if (stat(path.c_str(), &statInfo) != 0) {
        MEDIA_ERR_LOG("stat syscall err %{public}d", errno);
        return E_FAIL;
    }
    data->SetFileSize(statInfo.st_size);
    auto dateModified = static_cast<int64_t>(MediaFileUtils::Timespec2Millisecond(statInfo.st_mtim));
    if (dateModified == 0) {
        dateModified = MediaFileUtils::UTCTimeMilliSeconds();
        MEDIA_WARN_LOG("Invalid dateModified from st_mtim, use current time instead: %{public}lld",
            static_cast<long long>(dateModified));
    }
    if (dateModified != 0 && data->GetFileDateModified() == 0) {
        data->SetFileDateModified(dateModified);
    }
    return E_OK;
}

string BackupFileUtils::GarbleFilePath(const std::string &filePath, int32_t sceneCode, std::string cloneFilePath)
{
    if (filePath.empty()) {
        return filePath;
    }
    size_t displayNameIndex = filePath.rfind("/");
    if (displayNameIndex == string::npos || displayNameIndex + 1 >= filePath.size()) {
        return filePath;
    }
    std::string garbleDisplayName = GarbleFileName(filePath.substr(displayNameIndex + 1));
    std::string path;
    if (sceneCode == UPGRADE_RESTORE_ID) {
        path = filePath.substr(0, displayNameIndex).replace(0, UPGRADE_FILE_DIR.length(), GARBLE);
    } else if (sceneCode == DUAL_FRAME_CLONE_RESTORE_ID) {
        path = filePath.substr(0, displayNameIndex).replace(0, GARBLE_DUAL_FRAME_CLONE_DIR.length(), GARBLE);
    } else if (sceneCode == CLONE_RESTORE_ID) {
        path = filePath.substr(0, displayNameIndex).replace(0, cloneFilePath.length(), GARBLE);
    } else if (sceneCode == I_PHONE_CLONE_RESTORE) {
        path = filePath.substr(0, displayNameIndex).replace(0, OTHER_CLONE_PATH.length(), GARBLE);
    } else if (sceneCode == OTHERS_PHONE_CLONE_RESTORE) {
        path = filePath.substr(0, displayNameIndex).replace(0, OTHER_CLONE_PATH.length(), GARBLE);
    } else if (sceneCode == LITE_PHONE_CLONE_RESTORE) {
        path = filePath.substr(0, displayNameIndex).replace(0, OTHER_CLONE_PATH.length(), GARBLE);
    } else {
        path = filePath.substr(0, displayNameIndex).replace(0, DEFAULT_PATH_PREFIX.length(), GARBLE);
    }
    path += "/" + garbleDisplayName;
    return path;
}

string BackupFileUtils::GarbleFileName(const std::string &fileName)
{
    if (fileName.empty()) {
        return fileName;
    }
    if (fileName.find("Screenshot_") == 0 || fileName.find("IMG_") == 0 || fileName.find("VID_") == 0 ||
        fileName.find("SVID_") == 0) {
        return fileName;
    }
    size_t titleIndex = fileName.rfind(".");
    if (titleIndex == string::npos) {
        titleIndex = fileName.size();
    }
    if (titleIndex <= GARBLE.size() * GARBLE_UNIT) {
        return fileName;
    }
    return GARBLE + fileName.substr(GARBLE.size());
}

int32_t BackupFileUtils::CreateAssetPathById(int32_t fileId, int32_t mediaType, const string &extension,
    string &filePath)
{
    int32_t bucketNum = 0;
    int32_t errCode = MediaFileUri::CreateAssetBucket(fileId, bucketNum);
    if (errCode != E_OK) {
        return errCode;
    }

    string realName;
    errCode = CreateAssetRealName(fileId, mediaType, extension, realName);
    if (errCode != E_OK) {
        return errCode;
    }

    string dirPath = (mediaType == MediaType::MEDIA_TYPE_AUDIO ? RESTORE_AUDIO_CLOUD_DIR : RESTORE_CLOUD_DIR) + "/" +
        to_string(bucketNum);
    string localDirPath = GetReplacedPathByPrefixType(PrefixType::CLOUD, PrefixType::LOCAL, dirPath);
    if (!MediaFileUtils::IsFileExists(localDirPath)) {
        bool ret = MediaFileUtils::CreateDirectory(localDirPath);
        errCode = ret? E_OK: E_CHECK_DIR_FAIL;
    }
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Create Dir Failed! localDirPath=%{private}s",
        localDirPath.c_str());
    filePath = dirPath + "/" + realName;
    return E_OK;
}

int32_t BackupFileUtils::CreateAssetRealName(int32_t fileId, int32_t mediaType,
    const string &extension, string &name)
{
    string fileNumStr = to_string(fileId);
    if (fileId <= ASSET_MAX_COMPLEMENT_ID) {
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
    name = mediaTypeStr + to_string(MediaFileUtils::UTCTimeSeconds()) + "_" + fileNumStr + "." + extension;
    return E_OK;
}

std::string BackupFileUtils::GetFullPathByPrefixType(PrefixType prefixType, const std::string &relativePath)
{
    std::string fullPath;
    auto it = PREFIX_MAP.find(prefixType);
    if (it == PREFIX_MAP.end()) {
        MEDIA_ERR_LOG("Get path prefix failed: %{public}d", prefixType);
        return fullPath;
    }
    fullPath = it->second + relativePath;
    return fullPath;
}

int32_t BackupFileUtils::CreatePath(int32_t mediaType, const std::string &displayName, std::string &path)
{
    int32_t uniqueId = MediaLibraryAssetOperations::CreateAssetUniqueId(mediaType);
    int32_t errCode = BackupFileUtils::CreateAssetPathById(uniqueId, mediaType,
        MediaFileUtils::GetExtensionFromPath(displayName), path);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("Create path failed, errCode: %{public}d", errCode);
        path.clear();
        return errCode;
    }
    return E_OK;
}

int32_t BackupFileUtils::PreparePath(const std::string &path)
{
    size_t index = path.rfind("/");
    bool cond = (index == std::string::npos || index == path.length() - 1);
    CHECK_AND_RETURN_RET_LOG(!cond, E_CHECK_DIR_FAIL, "Parse directory path failed: %{private}s", path.c_str());
    std::string dirPath = path.substr(0, index);
    cond = (!MediaFileUtils::IsFileExists(dirPath) && !MediaFileUtils::CreateDirectory(dirPath));
    CHECK_AND_RETURN_RET_LOG(!cond, E_CHECK_DIR_FAIL,
        "Directory path doesn't exist and was created failed: %{public}s", dirPath.c_str());
    return E_OK;
}

int32_t BackupFileUtils::MoveFile(const string &oldPath, const string &newPath, int32_t sceneCode)
{
    bool errRet = false;
    if (!MediaFileUtils::IsFileExists(oldPath)) {
        MEDIA_ERR_LOG("old path: %{public}s is not exists.", GarbleFilePath(oldPath, sceneCode).c_str());
        return E_NO_SUCH_FILE;
    } else if (MediaFileUtils::IsFileExists(newPath)) {
        MEDIA_ERR_LOG("new path: %{public}s is exists.", GarbleFilePath(newPath, sceneCode).c_str());
        return E_FILE_EXIST;
    }
    int ret = rename(oldPath.c_str(), newPath.c_str());
    if (ret < 0 && errno == CROSS_POLICY_ERR) {
        ret = MediaFileUtils::CopyFileAndDelSrc(oldPath, newPath) ? 0 : -1;
        MEDIA_INFO_LOG("sendfile result: %{public}d, old path:%{public}s, new path: %{public}s",
            ret, GarbleFilePath(oldPath, sceneCode).c_str(), GarbleFilePath(newPath, sceneCode).c_str());
    }
    return ret;
}

std::string BackupFileUtils::GetReplacedPathByPrefixType(PrefixType srcPrefixType, PrefixType dstPrefixType,
    const std::string &path)
{
    std::string replacedPath;
    if (PREFIX_MAP.count(srcPrefixType) == 0 || PREFIX_MAP.count(dstPrefixType) == 0) {
        MEDIA_ERR_LOG("Get source or destination prefix failed: %{public}d, %{public}d", srcPrefixType, dstPrefixType);
        return replacedPath;
    }
    std::string srcPrefix = PREFIX_MAP.at(srcPrefixType);
    std::string dstPrefix = PREFIX_MAP.at(dstPrefixType);
    replacedPath = path;
    replacedPath.replace(0, srcPrefix.length(), dstPrefix);
    return replacedPath;
}

void BackupFileUtils::ModifyFile(const std::string path, int64_t modifiedTime)
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

string BackupFileUtils::GetFileNameFromPath(const string &path)
{
    size_t pos = GetLastSlashPosFromPath(path);
    if (pos == INVALID_RET || pos + 1 >= path.size()) {
        MEDIA_ERR_LOG("Failed to obtain file name because pos is invalid or out of range, path: %{public}s, "
            "size: %{public}zu, pos: %{public}zu", GarbleFilePath(path, DEFAULT_RESTORE_ID).c_str(), path.size(), pos);
        return "";
    }
    return path.substr(pos + 1);
}

string BackupFileUtils::GetFileTitle(const string &displayName)
{
    string::size_type pos = displayName.find_last_of('.');
    return (pos == string::npos) ? displayName : displayName.substr(0, pos);
}

int32_t BackupFileUtils::IsLowQualityImage(std::string &filePath, int32_t sceneCode,
    string relativePath, bool hasLowQualityImage)
{
    struct stat statInfo {};
    std::string garbledFilePath = BackupFileUtils::GarbleFilePath(filePath, sceneCode);
    if (!hasLowQualityImage) {
        MEDIA_ERR_LOG("Invalid file (%{public}s), no low quality image, err: %{public}d", garbledFilePath.c_str(),
            errno);
        return E_FAIL;
    }
    string realPath = ConvertLowQualityPath(sceneCode, filePath, relativePath);
    if (stat(realPath.c_str(), &statInfo) != E_SUCCESS) {
        MEDIA_ERR_LOG("Invalid Low quality image! file:%{public}s, err:%{public}d", garbledFilePath.c_str(), errno);
        return E_NO_SUCH_FILE;
    }
    MEDIA_INFO_LOG("Low quality image! file: %{public}s", garbledFilePath.c_str());
    filePath = realPath;
    bool cond = (statInfo.st_mode & S_IFDIR);
    CHECK_AND_RETURN_RET_LOG(!cond, E_FAIL, "Invalid file (%{public}s), is a directory", garbledFilePath.c_str());
    CHECK_AND_RETURN_RET_LOG(statInfo.st_size > 0, E_FAIL, "Invalid file (%{public}s), get size (%{public}lld) <= 0",
        garbledFilePath.c_str(), (long long)statInfo.st_size);
    return E_OK;
}

int32_t BackupFileUtils::IsFileValid(std::string &filePath, int32_t sceneCode,
    string relativePath, bool hasLowQualityImage)
{
    std::string garbledFilePath = BackupFileUtils::GarbleFilePath(filePath, sceneCode);
    struct stat statInfo {};
    if (stat(filePath.c_str(), &statInfo) != E_SUCCESS) {
        bool res = false;
        if (fileAccessHelper_ != nullptr) {
            res = fileAccessHelper_->GetValidPath(filePath);
        }
        if (stat(filePath.c_str(), &statInfo) != E_SUCCESS) {
            return hasLowQualityImage ? IsLowQualityImage(filePath, sceneCode, relativePath, hasLowQualityImage) :
                E_NO_SUCH_FILE;
        }
    }
    if (statInfo.st_mode & S_IFDIR) {
        MEDIA_ERR_LOG("Invalid file (%{public}s), is a directory", garbledFilePath.c_str());
        return E_FAIL;
    }
    CHECK_AND_RETURN_RET_LOG(statInfo.st_size > 0, E_FAIL, "Invalid file (%{public}s), get size (%{public}lld) <= 0",
        garbledFilePath.c_str(), (long long)statInfo.st_size);
    return E_OK;
}

std::string BackupFileUtils::GetDetailsPath(int32_t sceneCode, const std::string &type,
    const std::unordered_map<std::string, FailedFileInfo> &failedFiles, size_t limit)
{
    std::string detailsPath = RESTORE_FAILED_FILES_PATH + "_" + type + ".txt";
    bool cond = (MediaFileUtils::IsFileExists(detailsPath) && !MediaFileUtils::DeleteFile(detailsPath));
    CHECK_AND_RETURN_RET_LOG(!cond, "", "%{public}s exists and delete failed", detailsPath.c_str());
    cond = (failedFiles.empty() || limit == 0);
    CHECK_AND_RETURN_RET(!cond, "");
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateAsset(detailsPath) == E_SUCCESS, "",
        "Create %{public}s failed", detailsPath.c_str());

    std::string failedFilesStr = GetFailedFilesStr(sceneCode, failedFiles, limit);
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::WriteStrToFile(detailsPath, failedFilesStr), "",
        "Write to %{public}s failed", detailsPath.c_str());
    return detailsPath;
}

std::string BackupFileUtils::GetFailedFilesStr(int32_t sceneCode,
    const std::unordered_map<std::string, FailedFileInfo> &failedFiles, size_t limit)
{
    std::stringstream failedFilesStream;
    failedFilesStream << "[";
    for (const auto &iter : failedFiles) {
        if (limit == 0) {
            break;
        }
        failedFilesStream << "\n\"";
        failedFilesStream << GetFailedFile(sceneCode, iter.first, iter.second);
        limit > 1 ? failedFilesStream << "\"," : failedFilesStream << "\"";
        limit--;
    }
    failedFilesStream << "\n]";
    return failedFilesStream.str();
}

void BackupFileUtils::CreateDataShareHelper(const sptr<IRemoteObject> &token)
{
    if (token != nullptr) {
        sDataShareHelper_ = DataShare::DataShareHelper::Creator(token, MEDIALIBRARY_DATA_URI);
        CHECK_AND_PRINT_LOG(sDataShareHelper_ != nullptr,
            "generate thumbnails after restore failed, the sDataShareHelper_ is nullptr.");
    }
}

void BackupFileUtils::GenerateThumbnailsAfterRestore(int32_t restoreAstcCount)
{
    CHECK_AND_RETURN(sDataShareHelper_ != nullptr);
    std::string updateUri = PAH_GENERATE_THUMBNAILS_RESTORE;
    MediaFileUtils::UriAppendKeyValue(updateUri, URI_PARAM_API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri uri(updateUri);
    DataShare::DataSharePredicates emptyPredicates;
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(RESTORE_REQUEST_ASTC_GENERATE_COUNT, restoreAstcCount);
    int result = sDataShareHelper_->Update(uri, emptyPredicates, valuesBucket);
    CHECK_AND_PRINT_LOG(result >= 0, "generate thumbnails after restore failed, the sDataShareHelper_ update error");
}

std::vector<std::string> BackupFileUtils::GetFailedFilesList(int32_t sceneCode,
    const std::unordered_map<std::string, FailedFileInfo> &failedFiles, size_t limit)
{
    std::vector<std::string> failedFilesList;
    for (const auto &iter : failedFiles) {
        CHECK_AND_BREAK(limit != 0);
        failedFilesList.push_back(GetFailedFile(sceneCode, iter.first, iter.second));
        limit--;
    }
    return failedFilesList;
}

std::string BackupFileUtils::GetFailedFile(int32_t sceneCode, const std::string &failedFilePath,
    const FailedFileInfo &failedFileInfo)
{
    MEDIA_ERR_LOG("Failed file: %{public}s, %{public}s, %{public}s, %{public}s",
        GarbleFilePath(failedFilePath, sceneCode).c_str(), failedFileInfo.albumName.c_str(),
        GarbleFileName(failedFileInfo.displayName).c_str(), failedFileInfo.errorCode.c_str());
    // format: albumName/displayName
    return failedFileInfo.albumName + "/" + failedFileInfo.displayName;
}

bool BackupFileUtils::GetPathPosByPrefixLevel(int32_t sceneCode, const std::string &path, int32_t prefixLevel,
    size_t &pos)
{
    int32_t count = 0;
    for (size_t index = 0; index < path.length(); index++) {
        if (path[index] != '/') {
            continue;
        }
        count++;
        if (count == prefixLevel) {
            pos = index;
            break;
        }
    }
    if (count < prefixLevel) {
        MEDIA_ERR_LOG("Get position failed for %{public}s, %{public}d < %{public}d",
            GarbleFilePath(path, sceneCode).c_str(), count, prefixLevel);
        return false;
    }
    return true;
}

bool BackupFileUtils::ShouldIncludeSd(const std::string &prefix)
{
    return MediaFileUtils::IsFileExists(prefix + "/" + PHOTO_SD_DB_NAME) ||
        MediaFileUtils::IsFileExists(prefix + "/" + VIDEO_SD_DB_NAME);
}

void BackupFileUtils::DeleteSdDatabase(const std::string &prefix)
{
    std::vector<std::string> sdDbs = { PHOTO_SD_DB_NAME, VIDEO_SD_DB_NAME };
    for (const auto &sdDb : sdDbs) {
        std::string sdDbPath = prefix + "/" + sdDb;
        if (!MediaFileUtils::IsFileExists(sdDbPath)) {
            continue;
        }
        CHECK_AND_PRINT_LOG(MediaFileUtils::DeleteFile(sdDbPath),
            "Delete Sd database %{public}s failed, errno: %{public}d", sdDb.c_str(), errno);
    }
}

bool BackupFileUtils::IsLivePhoto(const FileInfo &fileInfo)
{
    return fileInfo.specialFileType == LIVE_PHOTO_TYPE || fileInfo.specialFileType == LIVE_PHOTO_HDR_TYPE ||
        fileInfo.isLivePhoto;
}

static void addPathSuffix(const string &oldPath, const string &suffix, string &newPath)
{
    if (oldPath.empty() || suffix.empty()) {
        MEDIA_WARN_LOG("oldPath or suffix is empty");
        return;
    }

    newPath = oldPath + suffix;
    while (MediaFileUtils::IsFileExists(newPath)) {
        newPath += ".dup" + suffix;
    }
}

bool BackupFileUtils::ConvertToMovingPhoto(FileInfo &fileInfo)
{
    if (!MediaFileUtils::IsFileExists(fileInfo.filePath)) {
        MEDIA_ERR_LOG("Live photo does not exist, path: %{private}s, errno: %{public}d",
            fileInfo.filePath.c_str(), errno);
        return false;
    }

    string movingPhotoImagePath;
    addPathSuffix(fileInfo.filePath, ".jpg", movingPhotoImagePath);
    addPathSuffix(fileInfo.filePath, ".mp4", fileInfo.movingPhotoVideoPath);
    addPathSuffix(fileInfo.filePath, ".extra", fileInfo.extraDataPath);
    int32_t ret = MovingPhotoFileUtils::ConvertToMovingPhoto(
        fileInfo.filePath, movingPhotoImagePath, fileInfo.movingPhotoVideoPath, fileInfo.extraDataPath);
    if (ret != E_OK) {
        return false;
    }

    if (!MediaFileUtils::DeleteFile(fileInfo.filePath)) {
        MEDIA_WARN_LOG("Failed to delete original live photo: %{private}s, errno: %{public}d",
            fileInfo.filePath.c_str(), errno);
    }
    fileInfo.filePath = movingPhotoImagePath;
    return true;
}

size_t BackupFileUtils::GetLastSlashPosFromPath(const std::string &path)
{
    if (path.empty()) {
        MEDIA_ERR_LOG("Failed to obtain last slash pos because given path is empty");
        return INVALID_RET;
    }
    size_t pos = path.rfind("/");
    if (pos == std::string::npos) {
        MEDIA_ERR_LOG("Failed to obtain last slash pos because / not found");
        return INVALID_RET;
    }
    return pos;
}

std::string BackupFileUtils::GetFileFolderFromPath(const std::string &path, bool shouldStartWithSlash)
{
    size_t endPos = GetLastSlashPosFromPath(path);
    if (endPos == INVALID_RET) {
        MEDIA_ERR_LOG("Failed to obtain file folder, path: %{public}s",
            GarbleFilePath(path, DEFAULT_RESTORE_ID).c_str());
        return "";
    }
    size_t startPos = MediaFileUtils::StartsWith(path, "/") && !shouldStartWithSlash ? 1 : 0;
    if (startPos >= endPos) {
        MEDIA_ERR_LOG("Failed to obtain file folder because start %{public}zu >= end %{public}zu, path: %{public}s",
            startPos, endPos, GarbleFilePath(path, DEFAULT_RESTORE_ID).c_str());
        return "";
    }
    return path.substr(startPos, endPos - startPos);
}

std::string BackupFileUtils::GetExtraPrefixForRealPath(int32_t sceneCode, const std::string &path)
{
    return sceneCode == UPGRADE_RESTORE_ID && IsAppTwinData(path) ? APP_TWIN_DATA_PREFIX : "";
}

bool BackupFileUtils::IsAppTwinData(const std::string &path)
{
    int32_t userId = GetUserId(path);
    return userId >= APP_TWIN_DATA_START && userId <= APP_TWIN_DATA_END;
}

int32_t BackupFileUtils::GetUserId(const std::string &path)
{
    int32_t userId = -1;
    if (!MediaFileUtils::StartsWith(path, INTERNAL_PREFIX)) {
        return userId;
    }
    std::string tmpPath = path.substr(INTERNAL_PREFIX.size());
    if (tmpPath.empty()) {
        MEDIA_ERR_LOG("Get substr failed, path: %{public}s", path.c_str());
        return userId;
    }
    size_t posStart = tmpPath.find_first_of("/");
    if (posStart == std::string::npos) {
        MEDIA_ERR_LOG("Get first / failed, path: %{public}s", path.c_str());
        return userId;
    }
    size_t posEnd = tmpPath.find_first_of("/", posStart + 1);
    if (posEnd == std::string::npos) {
        MEDIA_ERR_LOG("Get second / failed, path: %{public}s", path.c_str());
        return userId;
    }
    std::string userIdStr = tmpPath.substr(posStart + 1, posEnd - posStart -1);
    if (userIdStr.empty() || !MediaLibraryDataManagerUtils::IsNumber(userIdStr)) {
        MEDIA_ERR_LOG("Get userId failed, empty or not number, path: %{public}s", path.c_str());
        return userId;
    }
    return std::atoi(userIdStr.c_str());
}

bool BackupFileUtils::HandleRotateImage(const std::string &sourceFile,
    const std::string &targetPath, int32_t exifRotate, bool isLcd)
{
    uint32_t err = E_OK;
    std::unique_ptr<ImageSource> imageSource = LoadImageSource(sourceFile, err);
    bool cond = (err != E_OK || imageSource == nullptr);
    CHECK_AND_RETURN_RET_LOG(!cond, false, "LoadImageSource error: %{public}d, errno: %{public}d", err, errno);
    if (imageSource->IsHdrImage()) {
        return BackupFileUtils::HandleHdrImage(std::move(imageSource), targetPath, exifRotate, isLcd);
    } else {
        return BackupFileUtils::HandleSdrImage(std::move(imageSource), targetPath, exifRotate, isLcd);
    }
}

unique_ptr<ImageSource> BackupFileUtils::LoadImageSource(const std::string &file, uint32_t &err)
{
    SourceOptions opts;
    unique_ptr<ImageSource> imageSource = ImageSource::CreateImageSource(file, opts, err);
    bool cond = (err != E_OK || !imageSource);
    CHECK_AND_RETURN_RET_LOG(!cond, imageSource,
        "Failed to LoadImageSource, file exists: %{public}d", MediaFileUtils::IsFileExists(file));
    return imageSource;
}

bool BackupFileUtils::HandleHdrImage(std::unique_ptr<ImageSource> imageSource,
    const std::string &targetPath, int32_t exifRotate, bool isLcd)
{
    CHECK_AND_RETURN_RET_LOG(imageSource != nullptr, false, "Hdr imagesource null.");
    DecodingOptionsForPicture pictOpts;
    pictOpts.desireAuxiliaryPictures = {AuxiliaryPictureType::GAINMAP};
    uint32_t err = E_OK;
    auto picturePtr = imageSource->CreatePicture(pictOpts, err);
    bool cond = (err != E_OK || picturePtr == nullptr);
    CHECK_AND_RETURN_RET_LOG(!cond, false, "Failed to CreatePicture failed, err: %{public}d", err);

    std::shared_ptr<Picture> picture = std::move(picturePtr);
    auto pixelMap = picture->GetMainPixel();
    auto gainMap = picture->GetGainmapPixelMap();
    CHECK_AND_RETURN_RET_LOG(pixelMap != nullptr, false, "Failed to CreatePicture, main pixelMap is nullptr.");
    CHECK_AND_RETURN_RET_LOG(gainMap != nullptr, false, "Failed to CreatePicture, gainmap is nullptr.");
    CHECK_AND_RETURN_RET_LOG(pixelMap->GetWidth() * pixelMap->GetHeight() != 0, false,
        "Failed to CreatePicture, pixelMap size invalid, width: %{public}d, height: %{public}d",
        pixelMap->GetWidth(), pixelMap->GetHeight());
    CHECK_AND_RETURN_RET_LOG(MediaImageFrameWorkUtils::FlipAndRotatePixelMap(*(pixelMap.get()), exifRotate),
        false, "Flip and rotate pixelMap failed");
    CHECK_AND_RETURN_RET_LOG(MediaImageFrameWorkUtils::FlipAndRotatePixelMap(*(gainMap.get()), exifRotate),
        false, "Flip and rotate gainMap failed");

    CHECK_AND_RETURN_RET(!isLcd, EncodePicture(*picture, targetPath + "/" + LCD_FILE_NAME));
    return EncodePicture(*picture, targetPath + "/" + THM_FILE_NAME);
}

bool BackupFileUtils::EncodePicture(Picture &picture, const std::string &outFile)
{
    Media::ImagePacker imagePacker;
    PackOption option = {
        .format = IMAGE_FORMAT,
        .quality = IMAGE_QUALITY,
        .numberHint = IMAGE_NUMBER_HINT,
        .desiredDynamicRange = EncodeDynamicRange::AUTO,
        .needsPackProperties = false
    };
    uint32_t err = imagePacker.StartPacking(outFile, option);
    CHECK_AND_RETURN_RET_LOG(err == 0, false, "Failed to StartPacking %{public}d", err);
    err = imagePacker.AddPicture(picture);
    CHECK_AND_RETURN_RET_LOG(err == 0, false, "Failed to AddPicture %{public}d", err);
    err = imagePacker.FinalizePacking();
    CHECK_AND_RETURN_RET_LOG(err == 0, false, "Failed to FinalizePacking %{public}d", err);
    return true;
}

bool BackupFileUtils::HandleSdrImage(std::unique_ptr<ImageSource> imageSource,
    const std::string &targetPath, int32_t exifRotate, bool isLcd)
{
    CHECK_AND_RETURN_RET_LOG(imageSource != nullptr, false, "Sdr imagesource null.");
    DecodeOptions decodeOpts;
    uint32_t err = ERR_OK;
    unique_ptr<PixelMap> pixelMap = imageSource->CreatePixelMap(decodeOpts, err);
    CHECK_AND_RETURN_RET_LOG(pixelMap != nullptr, false, "CreatePixelMap err: %{public}d", err);
    CHECK_AND_RETURN_RET_LOG(MediaImageFrameWorkUtils::FlipAndRotatePixelMap(*(pixelMap.get()), exifRotate),
        false, "Flip and rotate pixelMap failed");
    CHECK_AND_RETURN_RET(!isLcd, EncodePixelMap(*pixelMap, targetPath + "/" + LCD_FILE_NAME));
    return EncodePixelMap(*pixelMap, targetPath + "/" + THM_FILE_NAME);
}

bool BackupFileUtils::ScalePixelMap(PixelMap &pixelMap, ImageSource &imageSource, const std::string &outFile)
{
    ImageInfo imageInfo;
    uint32_t err = imageSource.GetImageInfo(0, imageInfo);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, false, "Failed to Get ImageInfo");

    int targetWidth = imageInfo.size.height;
    int targetHeight = imageInfo.size.width;
    CHECK_AND_RETURN_RET(ThumbnailUtils::ResizeThumb(targetWidth, targetHeight), false);
    Size targetSize{targetWidth, targetHeight};
    PostProc postProc;
    CHECK_AND_RETURN_RET_LOG(postProc.ScalePixelMapEx(targetSize, pixelMap, Media::AntiAliasingOption::HIGH),
        false, "ScalePixelMapEx failed, targetSize: %{public}d * %{public}d",
        targetSize.width, targetSize.height);
    return EncodePixelMap(pixelMap, outFile);
}

bool BackupFileUtils::EncodePixelMap(PixelMap &pixelMap, const std::string &outFile)
{
    PackOption option = {
        .format = IMAGE_FORMAT,
        .quality = IMAGE_QUALITY,
        .numberHint = IMAGE_NUMBER_HINT,
        .desiredDynamicRange = EncodeDynamicRange::SDR,
    };
    ImagePacker imagePacker;
    uint32_t err = imagePacker.StartPacking(outFile, option);
    CHECK_AND_RETURN_RET_LOG(err == 0, false, "Failed to StartPacking %{public}d", err);
    err = imagePacker.AddImage(pixelMap);
    CHECK_AND_RETURN_RET_LOG(err == 0, false, "Failed to AddPicture %{public}d", err);
    err = imagePacker.FinalizePacking();
    CHECK_AND_RETURN_RET_LOG(err == 0, false, "Failed to FinalizePacking %{public}d", err);
    return true;
}

static std::shared_ptr<DataShare::DataShareHelper> GetCloudHelper(const std::string &uri)
{
    if (uri.empty()) {
        MEDIA_ERR_LOG("uri is empty.");
        return nullptr;
    }
    DataShare::CreateOptions options;
    options.enabled_ = true;
    return DataShare::DataShareHelper::Creator(uri, options);
}

int32_t BackupFileUtils::IsCloneCloudSyncSwitchOn(int32_t sceneCode)
{
    auto switchStatus = SettingsDataManager::GetPhotosSyncSwitchStatus();
    if (switchStatus != SwitchStatus::NONE) {
        MEDIA_DEBUG_LOG("GetPhotosSyncSwitchStatus success, switchStatus: %{public}d", static_cast<int>(switchStatus));
        return (switchStatus != SwitchStatus::CLOSE) ? CheckSwitchType::SUCCESS_ON : CheckSwitchType::SUCCESS_OFF;
    }
    MEDIA_DEBUG_LOG("GetPhotosSyncSwitchStatus fail, continue query old sync switch");

    if (sceneCode == UPGRADE_RESTORE_ID) {
        return CheckSwitchType::UPGRADE_FAILED_ON;
    }

    std::shared_ptr<DataShare::DataShareHelper> cloudHelper = GetCloudHelper(CLOUD_BASE_URI);
    CHECK_AND_RETURN_RET_LOG(cloudHelper != nullptr, CheckSwitchType::CLOUD_HELPER_NULL, "cloudHelper is null");
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo("bundleName", "generic.cloudstorage");
    Uri cloudUri(CLOUD_SYNC_SWITCH_URI);
    vector<string> columns = { "isSwitchOn" };
    shared_ptr<DataShare::DataShareResultSet> resultSet = cloudHelper->Query(cloudUri, predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, CheckSwitchType::RESULT_NULL, "resultSet is null");

    string switchOn = "0";
    if (resultSet->GoToNextRow() == E_OK) {
        resultSet->GetString(0, switchOn);
    }
    return (switchOn == MOBILE_NETWORK_STATUS_ON) ? CheckSwitchType::SUCCESS_ON : CheckSwitchType::SUCCESS_OFF;
}

bool BackupFileUtils::GetAccountValid(const int32_t sceneCode, const std::string &restoreInfo)
{
    if (sceneCode == UPGRADE_RESTORE_ID) {
        return true;
    }
    string oldId = "";
    string newId = "";
    nlohmann::json json_arr = nlohmann::json::parse(restoreInfo, nullptr, false);
    CHECK_AND_RETURN_RET_LOG(!json_arr.is_discarded(), false, "cloud account parse failed.");

    for (const auto& item : json_arr) {
        if (!item.contains("type") || !item.contains("detail")) {
            continue;
        }
        if (!item["type"].is_string() || !item["detail"].is_string()) {
            continue;
        }
        std::string type = item["type"];
        std::string detail = item["detail"];
        if (type != "dualAccountId") {
            continue;
        } else {
            oldId = item["detail"];
            MEDIA_INFO_LOG("the old is %{public}s", oldId.c_str());
            break;
        }
    }
    std::pair<bool, OHOS::AccountSA::OhosAccountInfo> ret =
        OHOS::AccountSA::OhosAccountKits::GetInstance().QueryOhosAccountInfo();
    if (ret.first) {
        OHOS::AccountSA::OhosAccountInfo& resultInfo = ret.second;
        newId = resultInfo.uid_;
    } else {
        MEDIA_ERR_LOG("new account logins failed.");
        return false;
    }
    MEDIA_INFO_LOG("the old id is %{public}s, new id is %{public}s",
        BackupFileUtils::GarbleFilePath(oldId, sceneCode).c_str(),
        BackupFileUtils::GarbleFilePath(newId, sceneCode).c_str());
    return ((oldId != "") && (oldId == newId));
}

bool BackupFileUtils::IsValidFile(const std::string &path)
{
    size_t fileSize = 0;
    MediaFileUtils::GetFileSize(path, fileSize);
    return fileSize > 0 && MediaFileUtils::IsFileExists(path);
}

bool BackupFileUtils::IsMovingPhotoExist(const std::string &path)
{
    string videoPath = MovingPhotoFileUtils::GetMovingPhotoVideoPath(path);
    string extraDataPath = MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(path);
    return IsValidFile(videoPath) && IsValidFile(extraDataPath);
}

bool BackupFileUtils::HasOrientationOrExifRotate(const FileInfo &info)
{
    return info.orientation != 0 || (
        info.exifRotate != 0 && info.exifRotate != static_cast<int32_t>(ExifRotateType::TOP_LEFT));
}

void BackupFileUtils::RestoreInvalidHDCCloudDataPos()
{
    std::string updateUri = PAH_RESTORE_INVALID_HDC_CLOUD_DATA_POS;
    MediaFileUtils::UriAppendKeyValue(updateUri, URI_PARAM_API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri uri(updateUri);
    DataShare::DataSharePredicates emptyPredicates;
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(BACKUP_RESTORE_INVALID_HDC_CLOUD_DATA_POS_FLAG, 0);
    CHECK_AND_RETURN_LOG(sDataShareHelper_, "sDataShareHelper_ is nullptr");
    int result = sDataShareHelper_->Update(uri, emptyPredicates, valuesBucket);
    CHECK_AND_PRINT_LOG(result >= 0, "restore invaildated pos of cloud hdc data failed,"
        "the sDataShareHelper_ update error");
    MEDIA_INFO_LOG("RestoreInvalidHDCCloudDataPos update success");
}


} // namespace Media
} // namespace OHOS