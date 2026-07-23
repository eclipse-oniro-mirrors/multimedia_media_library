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
#define MLOG_TAG "FolderScanner"

#include <algorithm>
#include <dirent.h>
#include <filesystem>
#include <sys/stat.h>

#include "folder_scanner.h"

#include "check_scene_helper.h"
#include "file_manager_parser.h"
#include "global_scanner.h"
#include "file_scan_utils.h"
#include "medialibrary_errno.h"
#include "media_lake_clone_event_manager.h"

namespace OHOS::Media {
namespace fs = std::filesystem;

const size_t BATCH_SIZE = 32;
// LCOV_EXCL_START
FolderScanner::FolderScanner(const std::string &path, ScanMode scanMode)
    : fsInitialType_(FolderScannerInitialType::PATH), scanMode_(scanMode)
{
    rootPath_ = path;
    scene_ = CheckSceneHelper::ResolveSceneByPath(rootPath_);
    folderParser_ = CheckSceneHelper::CreateFolderParser(scene_, rootPath_, scanMode);
}

FolderScanner::FolderScanner(const MediaNotifyInfo &notifyFolderInfo, ScanMode scanMode)
    : notifyFolderInfo_(notifyFolderInfo), fsInitialType_(FolderScannerInitialType::NOTIFY_INFO), scanMode_(scanMode)
{
    rootPath_ = notifyFolderInfo_.afterPath;
    scene_ = CheckSceneHelper::ResolveSceneByPath(rootPath_);
    folderParser_ = CheckSceneHelper::CreateFolderParser(scene_, rootPath_, scanMode);
}

FolderScanner::~FolderScanner() = default;

int32_t FolderScanner::Run()
{
    std::error_code errorCode;
    bool isDirectory = fs::exists(rootPath_, errorCode) && fs::is_directory(rootPath_, errorCode);
    CHECK_AND_RETURN_RET_WARN_LOG(isDirectory, ERR_INCORRECT_PATH, "invalid directory[%{public}s], error[%{public}s]",
        FileScanUtils::GarbleFilePath(rootPath_).c_str(), errorCode.message().c_str());

    const auto &scanRule = CheckSceneHelper::GetScanRuleConfig(scene_);
    queue<std::string> dirQueue;
    dirQueue.push(rootPath_);
    bool isRootDir = true;
    while (!dirQueue.empty()) {
        std::string currentDir = dirQueue.front();
        dirQueue.pop();

        // 目录过长
        size_t len = currentDir.length();
        CHECK_AND_CONTINUE_ERR_LOG(len > 0 && len < FILENAME_MAX - 1, "dir[%{public}s] error.",
            FileScanUtils::GarbleFilePath(currentDir).c_str());

        bool shouldScan = FolderScannerUtils::ShouldScanDirectory(currentDir, scanRule);
        CHECK_AND_CONTINUE_ERR_LOG(shouldScan, "Not scan dir: %{public}s",
            FileScanUtils::GarbleFilePath(currentDir).c_str());

        // 根目录需要轮询它的父目录
        if (isRootDir) {
            bool isSkipDirectory = FolderScannerUtils::IsSkipDirectory(currentDir, scanRule);
            isRootDir = false;
            MEDIA_INFO_LOG("call IsSkipDirectory");
            CHECK_AND_RETURN_RET_WARN_LOG(!isSkipDirectory, ERR_SUCCESS,
                "Skip dir path: %{public}s", FileScanUtils::GarbleFilePath(currentDir).c_str());
        } else {
            bool isSkipDirectory = FolderScannerUtils::IsSkipCurrentDirectory(currentDir, scanRule);
            MEDIA_INFO_LOG("call IsSkipCurrentDirectory");
            CHECK_AND_CONTINUE_ERR_LOG(!isSkipDirectory, "Skip dir path: %{public}s",
                FileScanUtils::GarbleFilePath(currentDir).c_str());
        }

        FolderScanner folderScanner = BuildSubDirFolderScanner(currentDir);
        int32_t ret = folderScanner.ScanCurrentDirectory(dirQueue);

        bool isRestore = MediaLakeCloneEventManager::GetInstance().IsRestoring();
        CHECK_AND_RETURN_RET_WARN_LOG(!isRestore, ERR_SUCCESS,
            "Global scan is due to the Restore process stopping at dir[%{public}s] scanning",
            FileScanUtils::GarbleFilePath(currentDir).c_str());

        CHECK_AND_CONTINUE_ERR_LOG(ret == ERR_SUCCESS, "Scan dir[%{public}s] failed",
            FileScanUtils::GarbleFilePath(currentDir).c_str());
    }
    return ERR_SUCCESS;
}

FolderScanner FolderScanner::BuildSubDirFolderScanner(const std::string &currentSubDir)
{
    if (fsInitialType_ == FolderScannerInitialType::PATH) {
        return FolderScanner(currentSubDir, scanMode_);
    }

    if (fsInitialType_ == FolderScannerInitialType::NOTIFY_INFO) {
        MediaNotifyInfo notifyInfo;
        BuildNotifyInfo(currentSubDir, notifyInfo);
        return FolderScanner(notifyInfo, scanMode_);
    }

    MEDIA_ERR_LOG("Fail to build SubDirFolderScanner");
    return FolderScanner("", scanMode_);
}

int32_t FolderScanner::ScanCurrentDirectory(queue<std::string> &subDirQueue)
{
    MEDIA_DEBUG_LOG("ScanCurrentDirectory enter dir: %{public}s", FileScanUtils::GarbleFilePath(rootPath_).c_str());
    CHECK_AND_RETURN_RET(!IsIncrementScanConflict(), ERR_SUCCESS);

    const auto &scanRule = CheckSceneHelper::GetScanRuleConfig(scene_);
    CHECK_AND_RETURN_RET_LOG(folderParser_ != nullptr, ERR_FAIL, "folderParser_ is nullptr");
    FolderOperationType type = folderParser_->PreProcessFolder();
    CHECK_AND_RETURN_RET_INFO_LOG(type != FolderOperationType::SKIP, ERR_SUCCESS,
        "Skip current directory: %{public}s", FileScanUtils::GarbleFilePath(rootPath_).c_str());

    // 性能优化，是否跳过当前文件夹下所有文件扫描
    CheckSetFileScannerSkip(type);

    // 扫描文件夹下所有文件和子文件夹
    DIR *dirPath = opendir(rootPath_.c_str());
    CHECK_AND_RETURN_RET_LOG(dirPath != nullptr, ERR_NOT_ACCESSIBLE,
        "Failed to opendir %{public}s, errno: %{public}d", FileScanUtils::GarbleFilePath(rootPath_).c_str(), errno);
    struct dirent *currentFile = nullptr;
    auto assetRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    while ((currentFile = readdir(dirPath)) != nullptr) {
        std::string currentFilePath = rootPath_ + "/" + currentFile->d_name;

        bool isRestore = MediaLakeCloneEventManager::GetInstance().IsRestoring();
        if (isRestore) {
            closedir(dirPath);
            dirPath = nullptr;
            MEDIA_WARN_LOG("Current folder scan stop at file[%{public}s] scanning",
                FileScanUtils::GarbleFilePath(currentFilePath).c_str());
            return ERR_SUCCESS;
        }

        struct stat statInfo;
        if (lstat(currentFilePath.c_str(), &statInfo) != 0) {
            MEDIA_INFO_LOG("lstat error: %{public}s, errno: %{public}d",
                FileScanUtils::GarbleFilePath(currentFilePath).c_str(), errno);
            continue;
        }
        if (S_ISDIR(statInfo.st_mode)) {
            subDirQueue.push(currentFilePath);
        } else if (IsScanFolderFile() && S_ISREG(statInfo.st_mode)) {
            if (FolderScannerUtils::IsSkipCurrentFile(currentFilePath, scanRule)) {
                MEDIA_INFO_LOG("Skip file path: %{public}s", FileScanUtils::GarbleFilePath(currentFilePath).c_str());
                continue;
            }
            CHECK_AND_PRINT_LOG(HandleFiles(currentFilePath, assetRefresh) == ERR_SUCCESS,
                "Failed to handle file %{public}s ", FileScanUtils::GarbleFilePath(currentFilePath).c_str());
        }
    }
    closedir(dirPath);
    dirPath = nullptr;

    // 更新湖内相册文件夹date_modified
    CheckUpdateFolderModified();

    // 批量插入资产
    BatchInsertAssets(assetRefresh);

    // 更新相册发送通知
    assetRefresh->Notify();
    UpdateAndNotifyAlbumInfos(true);
    return ERR_SUCCESS;
}

int32_t FolderScanner::HandleNeedUpdateAssets(FileParser &fileParser)
{
    InnerFileInfo fileInfos = fileParser.GetFileInfo();
    int32_t ret = fileParser.UpdateAssetInfo(albumInfos_.albumId, albumInfos_.bundleName, albumInfos_.albumName);

    CHECK_AND_RETURN_RET_LOG(ret == ERR_SUCCESS, ERR_FAIL, "Update assets failed");
    notifyAlbumIds_.insert(to_string(fileInfos.ownerAlbumId));
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_RET_LOG(watch != nullptr, ERR_FAIL, "Can not get MediaLibraryNotify Instance");
    auto notifyUri = fileParser.GetFileAssetUri();
    watch->Notify(notifyUri, NotifyType::NOTIFY_UPDATE);
    FileScanUtils::UpdateAndNotifyAnalysisAlbum({to_string(fileInfos.fileId)});
    MEDIA_INFO_LOG("send update notify: %{public}s", notifyUri.c_str());
    return ERR_SUCCESS;
}

int32_t FolderScanner::HandleNeedInsertedAssets(FileParser &fileParser,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh)
{
    InnerFileInfo fileInfos = fileParser.GetFileInfo();
    NativeRdb::ValuesBucket value = fileParser.TransFileInfoToBucket(albumInfos_.albumId, albumInfos_.bundleName,
        albumInfos_.albumName);
    if (value.IsEmpty()) {
        MEDIA_ERR_LOG("Fail to insert AssetBucket");
        return ERR_FAIL;
    }
    notifyAlbumIds_.insert(to_string(albumInfos_.albumId));
    insertFileBuckets_.emplace_back(value);
    inodes_.push_back(fileInfos.inode);
    if (insertFileBuckets_.size() >= BATCH_SIZE) {
        BatchInsertAssets(assetRefresh);
    }
    return ERR_SUCCESS;
}

std::unique_ptr<FileParser> FolderScanner::BuildFileParser(const std::string &currentFilePath)
{
    if (fsInitialType_ == FolderScannerInitialType::PATH) {
        return CheckSceneHelper::CreateFileParser(scene_, currentFilePath, scanMode_);
    }

    if (fsInitialType_ == FolderScannerInitialType::NOTIFY_INFO) {
        MediaNotifyInfo notifyInfo;
        BuildNotifyInfo(currentFilePath, notifyInfo);
        return CheckSceneHelper::CreateFileParser(scene_, notifyInfo, scanMode_);
    }

    MEDIA_ERR_LOG("Fail to initialize fileParser");
    return CheckSceneHelper::CreateFileParser(scene_, "", scanMode_);
}

int32_t FolderScanner::HandleParentFolderByType(FolderOperationType type)
{
    CHECK_AND_RETURN_RET_LOG(type != FolderOperationType::SKIP, ERR_FAIL, "Skip current directory");
    if (type == FolderOperationType::UPDATE) {
        CHECK_AND_RETURN_RET_LOG(folderParser_->UpdatePhotoAlbum() == ERR_SUCCESS, ERR_FAIL,
            "The directory update failed, skip current directory");
        notifyAlbumIds_.insert(to_string(albumInfos_.albumId));
        albumUpdateCount_++;
    }
    if (type == FolderOperationType::INSERT) {
        CHECK_AND_RETURN_RET_LOG(folderParser_->InsertAlbumInfo() != E_ERR, ERR_FAIL,
            "The current file failed to be inserted due to an incorrect album id");
        albumInfos_ = folderParser_->GetAlbumInfo();
        MEDIA_INFO_LOG("album insert, albumId[%{public}d]", albumInfos_.albumId);
        albumAddCount_++;
    }
    return ERR_SUCCESS;
}

int32_t FolderScanner::HandleFiles(std::string &currentFilePath,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh)
{
    std::unique_ptr<FileParser> fileParser = BuildFileParser(currentFilePath);
    CHECK_AND_RETURN_RET_LOG(fileParser != nullptr && fileParser->IsFileValidAsset(), ERR_FAIL,
        "Current file is invalid");

    // 首次插入场景
    FolderOperationType type = folderParser_->PreProcessFolder();
    CHECK_AND_RETURN_RET(HandleParentFolderByType(type) == ERR_SUCCESS, ERR_FAIL);

    MEDIA_INFO_LOG("HandleFiles, type[%{public}d], albumId[%{public}d]", type, albumInfos_.albumId);
    FileUpdateType updateType = fileParser->GetFileUpdateType();
    if (updateType == FileUpdateType::INSERT && GlobalScanner::GetInstance().IsLakeCloneRestoring()) {
        int32_t isDeleteForCloneRestore = fileParser->IsExistSameFileForCloneRestore(albumInfos_.albumId);
        CHECK_AND_EXECUTE(isDeleteForCloneRestore != E_OK, updateType = FileUpdateType::DELETE);
    }
    MEDIA_INFO_LOG("Current file update type is %{public}d", static_cast<int32_t>(updateType));
    switch (updateType) {
        case FileUpdateType::UPDATE:
        case FileUpdateType::UPDATE_ALBUM:
            CHECK_AND_RETURN_RET_LOG(HandleUpdateAsset(*fileParser) == ERR_SUCCESS, ERR_FAIL,
                "Update photo assets failed");
            break;
        case FileUpdateType::INSERT:
            CHECK_AND_RETURN_RET_LOG(HandleInsertAsset(*fileParser, assetRefresh) == ERR_SUCCESS, ERR_FAIL,
                "Batch insert photo assets failed");
            break;
        case FileUpdateType::NO_CHANGE:
            MEDIA_INFO_LOG("Current file has no change");
            fileIds_.push_back(fileParser->GetFileInfo().fileId);
            break;
        case FileUpdateType::DELETE:
            CHECK_AND_RETURN_RET_LOG(HandleDeleteAsset(currentFilePath) == ERR_SUCCESS, ERR_FAIL,
                "Delete file failed");
            break;
        case FileUpdateType::TRASH:
            CHECK_AND_RETURN_RET_LOG(HandleTrashAsset(*fileParser) == ERR_SUCCESS, ERR_FAIL,
                "Trash file failed");
            break;
        case FileUpdateType::RECOVER:
            CHECK_AND_RETURN_RET_LOG(HandleRecoverAsset(*fileParser) == ERR_SUCCESS, ERR_FAIL,
                "Recover file failed");
            break;
        default:
            MEDIA_ERR_LOG("The current file update type cannot be determined");
            return ERR_FAIL;
    }
    return ERR_SUCCESS;
}

int32_t FolderScanner::HandleUpdateAsset(FileParser &fileParser)
{
    CHECK_AND_RETURN_RET(HandleNeedUpdateAssets(fileParser) == ERR_SUCCESS, ERR_FAIL);
    fileIds_.push_back(fileParser.GetFileInfo().fileId);
    updateCount_++;
    return ERR_SUCCESS;
}

int32_t FolderScanner::HandleInsertAsset(FileParser &fileParser,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh)
{
    CHECK_AND_RETURN_RET(HandleNeedInsertedAssets(fileParser, assetRefresh) == ERR_SUCCESS, ERR_FAIL);
    addCount_++;
    return ERR_SUCCESS;
}

int32_t FolderScanner::HandleDeleteAsset(const std::string &currentFilePath)
{
    MEDIA_INFO_LOG("LakeClone: delete file, filePath = %{private}s", currentFilePath.c_str());
    MediaFileUtils::DeleteFile(currentFilePath);
    deleteCountForCloneRestore_++;
    return ERR_SUCCESS;
}

int32_t FolderScanner::HandleTrashAsset(FileParser &fileParser)
{
    if (scene_ == CheckScene::FILE_MANAGER) {
        auto* fmParser = static_cast<FileManagerParser*>(&fileParser);
        fmParser->UpdateTrashedAssetinfo();
    }
    return ERR_SUCCESS;
}

int32_t FolderScanner::HandleRecoverAsset(FileParser &fileParser)
{
    if (scene_ == CheckScene::FILE_MANAGER) {
        auto* fmParser = static_cast<FileManagerParser*>(&fileParser);
        fmParser->UpdateRecoverAssetinfo();
    }
    return ERR_SUCCESS;
}

void FolderScanner::UpdateAndNotifyAlbumInfos(bool isNeedUpdateSystemAlbum)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();

    if (notifyFileUris_.size() > 0) {
        std::vector<std::string> fileUris(notifyFileUris_.begin(), notifyFileUris_.end());
        std::vector<std::string> fileIds = FileScanUtils::GetFileIdsFromUris(fileUris);
        FileScanUtils::UpdateAndNotifyAnalysisAlbum(fileIds);

        auto watch = MediaLibraryNotify::GetInstance();
        CHECK_AND_RETURN_LOG(watch != nullptr, "Can not get MediaLibraryNotify Instance");
        for (auto fileUri : fileUris) {
            watch->Notify(fileUri, NotifyType::NOTIFY_ADD);
            MEDIA_INFO_LOG("send add notify: %{public}s", MediaFileUtils::DesensitizeUri(fileUri).c_str());
        }
        notifyFileUris_.clear();
    }

    if (isNeedUpdateSystemAlbum) {
        MediaLibraryRdbUtils::UpdateSystemAlbumInternal(rdbStore, {
            to_string(PhotoAlbumSubType::IMAGE),
            to_string(PhotoAlbumSubType::VIDEO)
        });
    }
    if (notifyAlbumIds_.size() > 0) {
        std::vector<std::string> albumIds(notifyAlbumIds_.begin(), notifyAlbumIds_.end());
        MediaLibraryRdbUtils::UpdateUserAlbumInternal(rdbStore, albumIds, true, true);
        MediaLibraryRdbUtils::UpdateSourceAlbumInternal(rdbStore, albumIds, true, true);
        notifyAlbumIds_.clear();
    }
}

void FolderScanner::CheckSetFileScannerSkip(FolderOperationType type)
{
    if (type == FolderOperationType::UPDATE || type == FolderOperationType::CONTINUE) {
        albumInfos_ = folderParser_->GetAlbumInfo();
        folderScannerHelperPtr_ = std::make_unique<FolderScannerHelper>(albumInfos_.albumId, rootPath_);
    }
}

void FolderScanner::CheckUpdateFolderModified()
{
    // 空文件夹，没有生成相册
    if (albumInfos_.albumId <= 0) {
        MEDIA_DEBUG_LOG("no need update album modified");
        return;
    }

    // 更新场景
    if (folderScannerHelperPtr_ != nullptr) {
        folderScannerHelperPtr_->UpdateFolderModified();
    } else {
        // 新增场景
        FolderScannerHelper folderScannerHelper(albumInfos_.albumId, rootPath_);
        folderScannerHelper.UpdateFolderModified();
    }
}

bool FolderScanner::IsScanFolderFile()
{
    return folderScannerHelperPtr_ == nullptr || !folderScannerHelperPtr_->IsSkipFolderFile();
}

int32_t FolderScanner::GetAlbumId()
{
    return albumInfos_.albumId;
}

void FolderScanner::GetFileIds(std::vector<int32_t> &fileIds)
{
    fileIds.swap(fileIds_);
}

bool FolderScanner::IsIncrementScanConflict()
{
    struct MediaNotifyInfo notifyInfo = notifyFolderInfo_;
    if (fsInitialType_ != FolderScannerInitialType::NOTIFY_INFO) {
        notifyInfo.afterPath = rootPath_;
    }
    if (scanMode_ == ScanMode::INCREMENT &&
        GlobalScanner::GetInstance().IsGlobalScanning({notifyInfo}, ScanTaskType::Folder)) {
        MEDIA_INFO_LOG("Global scan in progress, %{public}s scan later",
            FileScanUtils::GarbleFilePath(rootPath_).c_str());
        return true;
    }
    return false;
}

void FolderScanner::BatchInsertAssets(std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh)
{
    if (insertFileBuckets_.size() <= 0) {
        return;
    }
    bool ret = FolderScannerUtils::BatchInsertAssets(PhotoColumn::PHOTOS_TABLE, insertFileBuckets_, assetRefresh);
    if (ret != ERR_SUCCESS) {
        MEDIA_ERR_LOG("Batch insert photo assets failed, valueBuckets array size is %{public}zu",
            insertFileBuckets_.size());
    }
    std::vector<string> uris = FileParser::GenerateThumbnail(scanMode_, inodes_);
    notifyFileUris_.insert(uris.begin(), uris.end());
    inodes_.clear();
    insertFileBuckets_.clear();
}

int32_t FolderScanner::GetAddCount()
{
    return addCount_;
}

int32_t FolderScanner::GetUpdateCount()
{
    return updateCount_;
}

int32_t FolderScanner::GetAlbumAddCount()
{
    return albumAddCount_;
}

int32_t FolderScanner::GetAlbumUpdateCount()
{
    return albumUpdateCount_;
}

int32_t FolderScanner::GetDeleteCountForCloneRestore()
{
    return deleteCountForCloneRestore_;
}

void FolderScanner::BuildNotifyInfo(const std::string &targetPath, MediaNotifyInfo &notifyInfo)
{
    notifyInfo.afterPath = targetPath;
    notifyInfo.objType = notifyFolderInfo_.objType;
    notifyInfo.optType = notifyFolderInfo_.optType;

    if (!notifyFolderInfo_.beforePath.empty() &&
        targetPath.find(notifyFolderInfo_.afterPath) == 0) {
        std::string suffix = targetPath.substr(notifyFolderInfo_.afterPath.length());
        notifyInfo.beforePath = notifyFolderInfo_.beforePath + suffix;
    }
}
} // namespace OHOS::Media
