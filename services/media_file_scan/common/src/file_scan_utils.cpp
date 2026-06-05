/*
* Copyright (C) 2026 Huawei Device Co., Ltd.
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
#include "file_scan_utils.h"

#include <regex>
#include <safe_map.h>
#include <uuid.h>
#include <sys/sendfile.h>

#include "dfx_utils.h"
#include "medialibrary_errno.h"
#include "media_file_uri.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "mimetype_utils.h"
#include "metadata_extractor.h"
#include "rdb_predicates.h"
#include "scanner_utils.h"
#include "userfile_manager_types.h"
#include "medialibrary_rdbstore.h"
#include "directory_ex.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_notify.h"
#include "moving_photo_file_utils.h"

namespace OHOS::Media {
static SafeMap<std::string, std::string> groupHashMap_;
static SafeMap<std::string, int32_t> objectHashMap_;

const char EXTENSION_DOT = '.';
const char GARBLE_MARKER = '*';
const size_t GARBLE_SIZE_DEFAULT = 3;
const size_t GARBLE_SIZE_RATIO = 2;
const std::string MEDIALIBRARY_ZERO_BUCKET_PATH = "/storage/cloud/files/Photo/0";
constexpr int32_t CROSS_POLICY_ERR = 18;
constexpr int32_t BASE_USER_RANGE = 200000;
static const mode_t CHOWN_RO_USR_GRP = 0644;
static const int UUID_STR_LENGTH = 37;
const int32_t OH_DEFAULT_USER_ID = 100;
const std::unordered_map<PrefixType, std::string> PREFIX_MAP = {
    { PrefixType::CLOUD, "/storage/cloud/files" },
    { PrefixType::LOCAL, "/storage/media/local/files" },
    { PrefixType::CLOUD_EDIT_DATA, "/storage/cloud/files/.editData" },
    { PrefixType::LOCAL_EDIT_DATA, "/storage/media/local/files/.editData" },
    { PrefixType::CLOUD_THUMB, "/storage/cloud/files/.thumbs" },
};

// LCOV_EXCL_START
int32_t FileScanUtils::GetFileMetadata(std::unique_ptr<Metadata> &data)
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

int32_t FileScanUtils::FillMetadata(std::unique_ptr<Metadata> &data)
{
    int32_t err = GetFileMetadata(data);
    if (err != E_OK) {
        MEDIA_ERR_LOG("failed to get file metadata");
        return err;
    }
    if (data->GetFileMediaType() == MEDIA_TYPE_IMAGE) {
        err = MetadataExtractor::ExtractImageMetadata(data);
        if (data->GetPhotoSubType() == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)) {
            int64_t coverPosition = 0;
            int32_t err = MovingPhotoFileUtils::GetLivePhotoCoverPosition(data->GetFilePath(), coverPosition);
            if (err == E_OK) {
                data->SetCoverPosition(coverPosition);
            } else {
                MEDIA_ERR_LOG("failed to get cover position");
            }
        }
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

std::string FileScanUtils::GetFileTitle(const std::string &displayName)
{
    string::size_type pos = displayName.find_last_of('.');
    return (pos == string::npos) ? displayName : displayName.substr(0, pos);
}

int32_t FileScanUtils::CreateAssetRealName(int32_t fileId, int32_t mediaType,
    const std::string &extension, std::string &name)
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

std::string FileScanUtils::GetReplacedPathByPrefixType(PrefixType srcPrefixType, PrefixType dstPrefixType,
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

int32_t FileScanUtils::CreateAssetPathById(int32_t fileId, int32_t mediaType, const std::string &extension,
    std::string &filePath)
{
    int32_t bucketNum = 0;
    string realName;
    int32_t errCode = CreateAssetRealName(fileId, mediaType, extension, realName);
    if (errCode != E_OK) {
        return errCode;
    }

    string dirPath = RESTORE_CLOUD_DIR + "/" + to_string(bucketNum);
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

std::string FileScanUtils::FindObjectHash(InnerFileInfo &fileInfo)
{
    return std::to_string(fileInfo.ownerAlbumId) + "#" + FindTitlePrefix(fileInfo) + "#" + fileInfo.displayName;
}

std::string FileScanUtils::GenerateUuid()
{
    uuid_t uuid;
    uuid_generate(uuid);
    char str[UUID_STR_LENGTH] = {};
    uuid_unparse(uuid, str);
    return str;
}

int32_t FileScanUtils::FindGroupIndex(InnerFileInfo &fileInfo)
{
    std::string objectHash = FindObjectHash(fileInfo);
    int32_t groupIndex = 1;
    if (objectHashMap_.Find(objectHash, groupIndex)) {
        groupIndex++;
    }
    objectHashMap_.EnsureInsert(objectHash, groupIndex);
    return groupIndex;
}

std::string FileScanUtils::FindTitlePrefix(InnerFileInfo &fileInfo)
{
    std::string displayName = fileInfo.displayName;
    auto pos = displayName.find(TITLE_KEY_WORDS_OF_BURST);
    if (pos == std::string::npos) {
        MEDIA_ERR_LOG("Media_Restore: cannot find _BURST. Object: %{public}s",
            MediaFileUtils::DesensitizeName(fileInfo.displayName).c_str());
        return "";
    }
    return displayName.substr(0, std::min<int32_t>(pos, DISPLAY_NAME_PREFIX_LENGTH) + 1);
}

std::string CleanBurstFileName(const std::string& fileName)
{
    // 核心正则：
    // 匹配 BURST + 数字 + 可选(括号数字) + 可选_COVER
    // 捕获括号内的内容，保留！删除数字和COVER
    const std::regex reg(R"(BURST\d+(\(\d+\))?(_COVER)?)");
    // 替换为 BURST + 捕获的括号内容（无则为空）
    return std::regex_replace(fileName, reg, "BURST$1");
}

std::string FileScanUtils::FindGroupHash(InnerFileInfo &fileInfo)
{
    return  std::to_string(fileInfo.ownerAlbumId) + "#" + CleanBurstFileName(fileInfo.displayName) + "#" +
        std::to_string(FindGroupIndex(fileInfo));
}

void FileScanUtils::SetBurstKey(InnerFileInfo &fileInfo)
{
    if (fileInfo.isBurst != IsBurstType::BURST_COVER_TYPE &&
        fileInfo.isBurst != IsBurstType::BURST_MEMBER_TYPE) {
        fileInfo.burstKey = "";
        return;
    }
    std::string groupHash = FindGroupHash(fileInfo);
    std::string burstKey;
    if (!groupHashMap_.Find(groupHash, burstKey)) {
        burstKey = GenerateUuid();
        groupHashMap_.Insert(groupHash, burstKey);
    }
    MEDIA_DEBUG_LOG("FileParser: Media_Restore: burst photo, objectHash: %{public}s,"
        "groupHash: %{public}s, burstKey: %{public}s",
        FindObjectHash(fileInfo).c_str(),
        groupHash.c_str(),
        burstKey.c_str());
    fileInfo.burstKey = burstKey;
}

int32_t FileScanUtils::FindSubtype(const InnerFileInfo &fileInfo)
{
    CHECK_AND_RETURN_RET(fileInfo.burstKey.size() <= 0, static_cast<int32_t>(PhotoSubType::BURST));
    if (MovingPhotoFileUtils::IsLivePhoto(fileInfo.filePath)) {
        return static_cast<int32_t>(PhotoSubType::MOVING_PHOTO);
    }
    return static_cast<int32_t>(PhotoSubType::DEFAULT);
}

static bool IsZeroBucketPath(const std::string &path)
{
    return path.find(MEDIALIBRARY_ZERO_BUCKET_PATH) != string::npos;
}

std::string FileScanUtils::GetAssetRealPath(const std::string &path)
{
    if (path.empty()) {
        return path;
    }
    if (!IsZeroBucketPath(path)) {
        return path;
    }
    NativeRdb::RdbPredicates rdbPredicate(PhotoColumn::PHOTOS_TABLE);
    rdbPredicate.EqualTo(MediaColumn::MEDIA_FILE_PATH, path);
    vector<string> columns { PhotoColumn::PHOTO_STORAGE_PATH, PhotoColumn::PHOTO_FILE_SOURCE_TYPE };
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicate, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Failed to get resultSet");
        return "";
    }
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to GoToFirstRow");
        return "";
    }
    // 湖内资产删除隐藏后移动到湖外
    if (MediaLibraryRdbStore::GetInt(resultSet, PhotoColumn::PHOTO_FILE_SOURCE_TYPE) == static_cast<int32_t>(MEDIA)) {
        return path;
    }
    return MediaLibraryRdbStore::GetString(resultSet, PhotoColumn::PHOTO_STORAGE_PATH);
}

int32_t FileScanUtils::OpenFile(const string &filePath, int flags)
{
    string tmpPath = GetAssetRealPath(filePath);
    if (tmpPath.size() >= PATH_MAX) {
        MEDIA_ERR_LOG("File path too long %{public}d", static_cast<int>(tmpPath.size()));
        return -1;
    }
    MEDIA_DEBUG_LOG("File path is %{private}s", tmpPath.c_str());
    string absFilePath;
    if (!PathToRealPath(tmpPath, absFilePath)) {
        MEDIA_ERR_LOG("file is not real path, file path: %{private}s", tmpPath.c_str());
        return -1;
    }
    if (absFilePath.empty()) {
        MEDIA_ERR_LOG("Failed to obtain the canonical path for source path:%{public}s %{public}d",
                      DfxUtils::GetSafePath(tmpPath).c_str(), errno);
        return -1;
    }

    return open(absFilePath.c_str(), flags);
}

int32_t FileScanUtils::CopyFile(const std::string &srcPath, std::string &targetPath)
{
    string tmpPath = GetAssetRealPath(srcPath);
    if (srcPath.empty() || !MediaFileUtils::IsFileExists((tmpPath)) || !MediaFileUtils::IsFileValid(tmpPath)) {
        MEDIA_ERR_LOG("FileScanUtils: source file invalid! srcPath: %{public}s",
            DfxUtils::GetSafePath(tmpPath).c_str());
        return E_INVALID_PATH;
    }
    if (targetPath.empty()) {
        MEDIA_ERR_LOG("FileScanUtils: target file invalid! targetPath: %{public}s",
            DfxUtils::GetSafePath(targetPath).c_str());
        return E_INVALID_PATH;
    }
    return MediaFileUtils::CopyFileUtil(tmpPath, targetPath);
}

bool FileScanUtils::DeleteFile(const string &fileName)
{
    string tmpPath = GetAssetRealPath(fileName);
    return (remove(tmpPath.c_str()) == E_SUCCESS);
}

bool FileScanUtils::CoverLakeFile(const string &filePath, const string &newPath)
{
    struct stat fst{};
    bool errCode = false;
    if (filePath.size() >= PATH_MAX) {
        MEDIA_ERR_LOG("File path too long %{public}d", static_cast<int>(filePath.size()));
        return errCode;
    }
    MEDIA_INFO_LOG("File path is %{public}s", MediaFileUtils::DesensitizePath(filePath).c_str());
    string absFilePath;
    if (!PathToRealPath(filePath, absFilePath)) {
        MEDIA_ERR_LOG("file is not real path, file path: %{public}s",
            MediaFileUtils::DesensitizePath(filePath).c_str());
        return errCode;
    }
    if (absFilePath.empty()) {
        MEDIA_ERR_LOG("Failed to obtain the canonical path for source path:%{public}s %{public}d",
            MediaFileUtils::DesensitizePath(filePath).c_str(), errno);
        return errCode;
    }

    int32_t source = open(absFilePath.c_str(), O_RDONLY);
    if (source == -1) {
        MEDIA_ERR_LOG("Open failed for source file, errno: %{public}d", errno);
        return errCode;
    }
    string tmpPath;
    if (!PathToRealPath(newPath, tmpPath) || tmpPath.empty()) {
        MEDIA_ERR_LOG("Failed to obtain the canonical path: %{public}s, errno: %{public}d",
            DfxUtils::GetSafePath(newPath).c_str(), errno);
        close(source);
        return errCode;
    }
    int32_t dest = open(tmpPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, CHOWN_RO_USR_GRP);
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

int32_t FileScanUtils::MoveFileInEditScene(const string &oldPath, const string &newPath)
{
    CHECK_AND_RETURN_RET_LOG(!newPath.empty(), false, "Empty destPath");
    string srcPath = GetAssetRealPath(oldPath);
    string destPath = GetAssetRealPath(newPath);
    MEDIA_INFO_LOG("MoveFile src: %{private}s, dest: %{private}s", srcPath.c_str(), destPath.c_str());

    if (!MediaFileUtils::IsFileExists(srcPath)) {
        MEDIA_ERR_LOG("Source file does not exist: %{private}s", srcPath.c_str());
        return E_ERR;
    }

    bool destExists = MediaFileUtils::IsFileExists(destPath);
    bool isSuccess = false;

    if (rename(srcPath.c_str(), destPath.c_str()) == 0) {
        return E_OK;
    }
    MEDIA_WARN_LOG("Rename failed, errno: %{public}d", errno);
    if (errno == CROSS_POLICY_ERR) {
        if (destPath.find("HO_DATA_EXT_MISC") != std::string::npos) {
            isSuccess = CoverLakeFile(srcPath, destPath);
            if (isSuccess) {
                CHECK_AND_PRINT_LOG(DeleteFile(srcPath),
                    "Delete srcFile:%{private}s failed, errno: %{public}d", srcPath.c_str(), errno);
            }
        } else {
            if (destExists) {
                return E_ERR;
            } else {
                isSuccess = MediaFileUtils::CopyFileUtil(srcPath, destPath);
            }
        }
    }

    return isSuccess ? E_OK : E_ERR;
}

int32_t FileScanUtils::RenameFileCrossPolicy(const string &oldPath, const string &newPath, bool deleteOld)
{
    CHECK_AND_RETURN_RET_LOG(!newPath.empty(), E_ERR, "Empty destPath");
    MEDIA_INFO_LOG("MoveFile src: %{private}s, dest: %{private}s", oldPath.c_str(), newPath.c_str());

    if (!MediaFileUtils::IsFileExists(oldPath)) {
        MEDIA_ERR_LOG("Source file does not exist: %{private}s", oldPath.c_str());
        return E_ERR;
    }

    if (rename(oldPath.c_str(), newPath.c_str()) == 0) {
        return E_OK;
    }

    bool success = false;
    MEDIA_WARN_LOG("Rename failed, errno: %{public}d", errno);
    if (errno == CROSS_POLICY_ERR) {
        success = CoverLakeFile(oldPath, newPath);
        if (deleteOld) {
            CHECK_AND_PRINT_LOG(DeleteFile(oldPath),
                "Delete srcFile:%{private}s failed, errno: %{public}d", oldPath.c_str(), errno);
        }
    }
    return success ? E_OK : E_ERR;
}

std::string FileScanUtils::GarbleFilePath(const std::string &filePath)
{
    std::filesystem::path inputPath(filePath);
    std::filesystem::path outputPath(filePath);
    for (auto iter = inputPath.begin(); iter != inputPath.end(); iter++) {
        outputPath /= GarbleFile(iter->string());
    }
    return outputPath;
}

std::string FileScanUtils::GarbleFile(const std::string &file)
{
    return HasExtension(file) ? GarbleFileWithExtension(file) : GarbleFileWithoutExtension(file);
}

bool FileScanUtils::HasExtension(const std::string &file)
{
    return file.find(EXTENSION_DOT) != std::string::npos;
}

std::string FileScanUtils::GarbleFileWithExtension(const std::string &file)
{
    size_t pos = file.find_last_of(EXTENSION_DOT);
    CHECK_AND_RETURN_RET_LOG(pos != std::string::npos, "", "file.path not cotain EXTENSION_DOT");
    std::string name = file.substr(0, pos);
    std::string extension = file.substr(pos);
    return GarbleFileWithoutExtension(name) + extension;
}

std::string FileScanUtils::GarbleFileWithoutExtension(const std::string &file)
{
    size_t garbleSize = GetGarbleSize(file);
    std::string result(file);
    result.replace(0, garbleSize, garbleSize, GARBLE_MARKER);
    return result;
}

size_t FileScanUtils::GetGarbleSize(const std::string &file)
{
    return file.size() >= GARBLE_SIZE_DEFAULT * GARBLE_SIZE_RATIO ? GARBLE_SIZE_DEFAULT :
        file.size() / GARBLE_SIZE_RATIO;
}

int32_t FileScanUtils::BuildLakeFilePath(
    const std::string &displayName, const int32_t mediaType, std::string &targetPath)
{
    std::shared_ptr<TransactionOperations> trans = make_shared<TransactionOperations>(__func__);
    int32_t uniqueId;
    std::function<int(void)> tryReuseDeleted = [&]() -> int {
        uniqueId = MediaLibraryAssetOperations::CreateAssetUniqueId(mediaType, trans);
        return FileScanUtils::CreateAssetPathById(
            uniqueId, mediaType, MediaFileUtils::GetExtensionFromPath(displayName), targetPath);
    };
    int ret = trans->RetryTrans(tryReuseDeleted);
    MEDIA_INFO_LOG("BuildLakeFilePath, ret: %{public}d, uniqueId:%{public}d, targetPath: %{private}s",
        ret,
        uniqueId,
        targetPath.c_str());
    return ret;
}

int32_t FileScanUtils::GetCurrentAccountId()
{
    int32_t uid = static_cast<int32_t>(getuid());
    int32_t currentUserId = uid / BASE_USER_RANGE;
    MEDIA_DEBUG_LOG("current uid is %{public}d, userId is %{public}d", uid, currentUserId);
    return currentUserId;
}

std::string FileScanUtils::GetCurrentInLakeLogicPrefix()
{
    std::string userId = to_string(GetCurrentAccountId());
    return "/data/service/el2/" + userId + "/hmdfs/account/files/Docs/HO_DATA_EXT_MISC/";
}

bool FileScanUtils::IsDefaultAccount()
{
    int32_t userId = GetCurrentAccountId();
    return userId == OH_DEFAULT_USER_ID;
}

std::vector<std::string> FileScanUtils::GetFileIdsFromUris(const std::vector<std::string> &uris)
{
    std::vector<std::string> fileIds;
    CHECK_AND_RETURN_RET_LOG(!uris.empty(), fileIds, "uris is empty");
    
    string fileId;
    for (const auto &uri : uris) {
        fileId = MediaFileUtils::GetIdFromUri(uri);
        fileIds.emplace_back(fileId);
    }
    return fileIds;
}

void NotifyAnalysisAlbum(const std::vector<std::string>& albumIds)
{
    if (albumIds.size() <= 0) {
        return;
    }
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_LOG(watch != nullptr, "Can not get MediaLibraryNotify Instance");
    for (const auto& albumId : albumIds) {
        watch->Notify(MediaFileUtils::GetUriByExtrConditions(
            PhotoAlbumColumns::ANALYSIS_ALBUM_URI_PREFIX, albumId), NotifyType::NOTIFY_UPDATE);
    }
}

void FileScanUtils::UpdateAndNotifyAnalysisAlbum(const std::vector<std::string>& fileIds)
{
    CHECK_AND_RETURN_LOG(!fileIds.empty(), "fileIds is empty.");
    
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "Failed to get rdbStore.");
    
    std::set<std::string> analysisAlbumIds;
    MediaLibraryRdbUtils::QueryAnalysisAlbumIdOfAssets(fileIds, analysisAlbumIds);

    std::vector<std::string> albumIds(analysisAlbumIds.begin(), analysisAlbumIds.end());
    if (albumIds.size() > 0 && rdbStore != nullptr) {
        MediaLibraryRdbUtils::UpdateAnalysisAlbumInternal(rdbStore, albumIds);
        NotifyAnalysisAlbum(albumIds);
    }
}
// LCOV_EXCL_STOP
}