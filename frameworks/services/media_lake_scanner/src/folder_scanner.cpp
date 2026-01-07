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

#include "folder_scanner.h"
#include "global_scanner.h"
#include "lake_file_utils.h"
#include "medialibrary_errno.h"

namespace OHOS::Media {
namespace fs = std::filesystem;

const size_t BATCH_SIZE = 32;

FolderScanner::FolderScanner(const std::string &path, LakeScanMode scanMode) :
    fsInitialType_(FolderScannerInitialType::PATH),
    folderParser_(path, scanMode),
    scanMode_(scanMode)
{
    rootPath_ = path;
}

FolderScanner::FolderScanner(const MediaLakeNotifyInfo &notifyFolderInfo, LakeScanMode scanMode) :
    notifyFolderInfo_(notifyFolderInfo),
    fsInitialType_(FolderScannerInitialType::NOTIFY_INFO),
    folderParser_(notifyFolderInfo.afterPath, scanMode),
    scanMode_(scanMode)
{
    rootPath_ = notifyFolderInfo_.afterPath;
}

FolderScanner::~FolderScanner() = default;

int32_t FolderScanner::Run()
{
    std::error_code errorCode;
    bool isDirectory = fs::exists(rootPath_, errorCode) && fs::is_directory(rootPath_, errorCode);
    CHECK_AND_RETURN_RET_WARN_LOG(isDirectory, ERR_INCORRECT_PATH, "invalid directory[%{public}s], error[%{public}s]",
        LakeFileUtils::GarbleFilePath(rootPath_).c_str(), errorCode.message().c_str());

    queue<std::string> dirQueue;
    dirQueue.push(rootPath_);
    bool isRootDir = true;
    while (!dirQueue.empty()) {
        std::string currentDir = dirQueue.front();
        dirQueue.pop();

        // 目录过长
        size_t len = currentDir.length();
        CHECK_AND_CONTINUE_ERR_LOG(len > 0 && len < FILENAME_MAX - 1, "dir[%{public}s] error.",
            LakeFileUtils::GarbleFilePath(currentDir).c_str());

        bool shouldScan = FolderScannerUtils::shouldScanDirectory(currentDir);
        CHECK_AND_CONTINUE_ERR_LOG(shouldScan, "Not scan dir: %{public}s",
            LakeFileUtils::GarbleFilePath(currentDir).c_str());

        // 根目录需要轮询它的父目录
        if (isRootDir) {
            bool isSkipDirectory = FolderScannerUtils::IsSkipDirectory(currentDir);
            isRootDir = false;
            MEDIA_INFO_LOG("call IsSkipDirectory");
            CHECK_AND_RETURN_RET_WARN_LOG(!isSkipDirectory, ERR_SUCCESS,
                "Skip dir path: %{public}s", LakeFileUtils::GarbleFilePath(currentDir).c_str());
        } else {
            bool isSkipDirectory = FolderScannerUtils::IsSkipCurrentDirectory(currentDir);
            MEDIA_INFO_LOG("call IsSkipCurrentDirectory");
            CHECK_AND_CONTINUE_ERR_LOG(!isSkipDirectory, "Skip dir path: %{public}s",
                LakeFileUtils::GarbleFilePath(currentDir).c_str());
        }

        FolderScanner folderScanner = BuildSubDirFolderScanner(currentDir);
        int32_t ret = folderScanner.ScanCurrentDirectory(dirQueue);
        CHECK_AND_CONTINUE_ERR_LOG(ret == ERR_SUCCESS, "Scan dir[%{public}s] failed",
            LakeFileUtils::GarbleFilePath(currentDir).c_str());
    }
    return ERR_SUCCESS;
}

FolderScanner FolderScanner::BuildSubDirFolderScanner(const std::string &currentSubDir)
{
    if (fsInitialType_ == FolderScannerInitialType::PATH) {
        return FolderScanner(currentSubDir, scanMode_);
    }

    if (fsInitialType_ == FolderScannerInitialType::NOTIFY_INFO) {
        MediaLakeNotifyInfo notifyInfo;
        BuildNotifyInfo(currentSubDir, notifyInfo);
        return FolderScanner(notifyInfo, scanMode_);
    }

    MEDIA_ERR_LOG("Fail to build SubDirFolderScanner");
    return FolderScanner("", scanMode_);
}

int32_t FolderScanner::ScanCurrentDirectory(queue<std::string> &subDirQueue)
{
    if (IsIncrementScanConflict()) {
        return ERR_SUCCESS;
    }

    FolderOperationType type = folderParser_.PreProcessFolder();
    if (type == FolderOperationType::SKIP) {
        MEDIA_INFO_LOG("Skip current directory: %{public}s", LakeFileUtils::GarbleFilePath(rootPath_).c_str());
        return ERR_SUCCESS;
    }

    // 性能优化，是否跳过当前文件夹下所有文件扫描
    CheckSetFileScannerSkip(type);

    // 扫描文件夹下所有文件和子文件夹
    DIR *dirPath = opendir(rootPath_.c_str());
    CHECK_AND_RETURN_RET_LOG(dirPath != nullptr, ERR_NOT_ACCESSIBLE,
        "Failed to opendir %{public}s, errno: %{public}d", LakeFileUtils::GarbleFilePath(rootPath_).c_str(), errno);
    struct dirent *currentFile = nullptr;
    auto assetRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    while ((currentFile = readdir(dirPath)) != nullptr) {
        std::string fileName = currentFile->d_name;
        std::string currentFilePath = rootPath_ + "/" + fileName;
        struct stat statInfo;
        if (lstat(currentFilePath.c_str(), &statInfo) != 0) {
            MEDIA_INFO_LOG("lstat error: %{public}s, errno: %{public}d",
                LakeFileUtils::GarbleFilePath(currentFilePath).c_str(), errno);
            continue;
        }
        if (S_ISDIR(statInfo.st_mode)) {
            subDirQueue.push(currentFilePath);
        } else if (IsScanFolderFile() && S_ISREG(statInfo.st_mode)) {
            bool isSkipFile = FolderScannerUtils::IsSkipCurrentFile(currentFilePath);
            if (isSkipFile) {
                MEDIA_INFO_LOG("Skip file path: %{public}s", LakeFileUtils::GarbleFilePath(currentFilePath).c_str());
                continue;
            }
            int32_t ret = HandleFiles(currentFilePath, assetRefresh);
            CHECK_AND_PRINT_LOG(ret == ERR_SUCCESS,
                "Failed to handle file %{public}s ", LakeFileUtils::GarbleFilePath(currentFilePath).c_str());
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

FileParser FolderScanner::BuildFileParser(const std::string &currentFilePath)
{
    if (fsInitialType_ == FolderScannerInitialType::PATH) {
        return FileParser(currentFilePath, scanMode_);
    }

    if (fsInitialType_ == FolderScannerInitialType::NOTIFY_INFO) {
        MediaLakeNotifyInfo notifyInfo;
        BuildNotifyInfo(currentFilePath, notifyInfo);
        return FileParser(notifyInfo, scanMode_);
    }

    MEDIA_ERR_LOG("Fail to initialize fileParser");
    return FileParser("", scanMode_);
}

int32_t FolderScanner::HandleFiles(std::string &currentFilePath,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh)
{
    FileParser fileParser = BuildFileParser(currentFilePath);
    CHECK_AND_RETURN_RET_LOG(fileParser.IsFileValidAsset(), ERR_FAIL, "Current file is invalid");
 
    // 首次插入场景
    FolderOperationType type = folderParser_.PreProcessFolder();
    CHECK_AND_RETURN_RET_LOG(type != FolderOperationType::SKIP, ERR_FAIL, "Skip current directory");
    if (type == FolderOperationType::UPDATE) {
        CHECK_AND_RETURN_RET_LOG(folderParser_.UpdatePhotoAlbum() == ERR_SUCCESS, ERR_FAIL,
            "The directory update failed, skip current directory");
        notifyAlbumIds_.insert(to_string(albumInfos_.albumId));
    }
    if (type == FolderOperationType::INSERT) {
        CHECK_AND_RETURN_RET_LOG(folderParser_.InsertAlbumInfo() != E_ERR, ERR_FAIL,
            "The current file failed to be inserted due to an incorrect album id");
        albumInfos_ = folderParser_.GetAlbumInfo();
        MEDIA_INFO_LOG("album insert, albumId[%{public}d]", albumInfos_.albumId);
    }
 
    MEDIA_INFO_LOG("HandleFiles, type[%{public}d], albumId[%{public}d]", type, albumInfos_.albumId);
 
    FileUpdateType updateType = fileParser.GetFileUpdateType();
    MEDIA_INFO_LOG("Current file update type is %{public}d", static_cast<int32_t>(updateType));
    switch (updateType) {
        case FileUpdateType::UPDATE:
        case FileUpdateType::UPDATE_ALBUM:
            CHECK_AND_RETURN_RET_LOG(HandleNeedUpdateAssets(fileParser) == ERR_SUCCESS, ERR_FAIL,
                "Update photo assets failed");
            fileIds_.push_back(fileParser.GetFileInfo().fileId);
            updateCount_++;
            break;
        case FileUpdateType::INSERT:
            CHECK_AND_RETURN_RET_LOG(HandleNeedInsertedAssets(fileParser, assetRefresh) == ERR_SUCCESS, ERR_FAIL,
                "Batch insert photo assets failed");
            addCount_++;
            break;
        case FileUpdateType::NO_CHANGE:
            MEDIA_INFO_LOG("Current file has no change");
            fileIds_.push_back(fileParser.GetFileInfo().fileId);
            break;
        default:
            MEDIA_ERR_LOG("The current file update type cannot be determined");
            return ERR_FAIL;
    }
    return ERR_SUCCESS;
}

void FolderScanner::UpdateAndNotifyAlbumInfos(bool isNeedUpdateSystemAlbum)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();

    if (notifyFileUris_.size() > 0) {
        std::vector<std::string> fileUris(notifyFileUris_.begin(), notifyFileUris_.end());
        auto watch = MediaLibraryNotify::GetInstance();
        CHECK_AND_RETURN_LOG(watch != nullptr, "Can not get MediaLibraryNotify Instance");
        for (auto fileUri : fileUris) {
            watch->Notify(fileUri, NotifyType::NOTIFY_ADD);
            MEDIA_INFO_LOG("send add notify: %{public}s", fileUri.c_str());
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
        albumInfos_ = folderParser_.GetAlbumInfo();
        folderScannerHelperPtr = make_shared<FolderScannerHelper>(albumInfos_.albumId, rootPath_);
        MEDIA_INFO_LOG("album update type[%{public}d], albumId[%{public}d], skip[%{public}d]", type,
            albumInfos_.albumId, folderScannerHelperPtr->IsSkipFolderFile());
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
    if (folderScannerHelperPtr != nullptr) {
        folderScannerHelperPtr->UpdateFolderModified();
    } else {
        // 新增场景
        FolderScannerHelper folderScannerHelper(albumInfos_.albumId, rootPath_);
        folderScannerHelper.UpdateFolderModified();
    }
    MEDIA_INFO_LOG("update folder modified time");
}
 
bool FolderScanner::IsScanFolderFile()
{
    return folderScannerHelperPtr == nullptr || folderScannerHelperPtr->IsFolderModified();
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
    struct MediaLakeNotifyInfo notifyInfo = notifyFolderInfo_;
    if (fsInitialType_ != FolderScannerInitialType::NOTIFY_INFO) {
        notifyInfo.afterPath = rootPath_;
    }
    if (scanMode_ == LakeScanMode::INCREMENT &&
        GlobalScanner::GetInstance().IsGlobalScanning({notifyInfo}, ScanTaskType::Folder)) {
        MEDIA_INFO_LOG("Global scan in progress, %{public}s scan later",
            LakeFileUtils::GarbleFilePath(rootPath_).c_str());
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

void FolderScanner::BuildNotifyInfo(const std::string &targetPath, MediaLakeNotifyInfo &notifyInfo)
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

}