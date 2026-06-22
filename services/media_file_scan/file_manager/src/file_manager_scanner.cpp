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

#define MLOG_TAG "FileManagerScanner"

#include "file_manager_folder_parser.h"

#include "file_manager_scanner.h"
#include "file_scan_utils.h"
#include "medialibrary_notify.h"
#include "medialibrary_tracer.h"

using namespace std;

namespace OHOS::Media {
namespace fs = std::filesystem;
// LCOV_EXCL_START
FileManagerScanner::FileManagerScanner(ScanMode scanMode)
    : FileScanner(scanMode)
{
}

void FileManagerScanner::HandleFiles(MediaNotifyInfo& fileInfo)
{
    FileManagerParser fileManagerParser(fileInfo, scanMode_);
    auto updateType = fileManagerParser.GetFileUpdateType();
    if (!fileManagerParser.IsFileValidAsset()) {
        RefreshAssetInfoForSkipFile(fileManagerParser, updateType);
        MEDIA_WARN_LOG("invalid file manager file");
        return;
    }

    error_code ec;
    if (!filesystem::is_regular_file(fileInfo.afterPath, ec) || IsSkipCurrentFile(fileInfo.afterPath)) {
        RefreshAssetInfoForSkipFile(fileManagerParser, updateType);
        MEDIA_WARN_LOG("skip file manager file: %{private}s", fileInfo.afterPath.c_str ());
        return;
    }

    switch (updateType) {
        case FileUpdateType::UPDATE:
            RefreshUpdateAssetInfo(fileManagerParser);
            break;
        case FileUpdateType::INSERT:
            GetInsertAssetInfo(fileInfo, fileManagerParser);
            break;
        case FileUpdateType::UPDATE_ALBUM:
            RefreshUpdateAssetAlbumInfo(fileInfo, fileManagerParser);
            break;
        case FileUpdateType::TRASH:
            RefreshTrashedAssetInfo(fileManagerParser);
            break;
        case FileUpdateType::RECOVER:
            RefreshNeedReoverAssetInfo(fileManagerParser);
            break;
        default:
            MEDIA_INFO_LOG("no change");
            break;
    }
}

bool FileManagerScanner::IsSkipCurrentFile(const std::string &filePath)
{
    std::string fileName = fs::path(filePath).filename().string();
    if (fileName.empty() || fileName.find("..") != std::string::npos) {
        MEDIA_INFO_LOG("Error file manager file: %{public}s", FileScanUtils::GarbleFile(fileName).c_str());
        return true;
    }
    return false;
}

bool FileManagerScanner::IsIncrementScanConflict(std::vector<MediaNotifyInfo> fileInfos)
{
    return false;
}

bool FileManagerScanner::IsSkipDirectory(const std::string &dir)
{
    std::string currentDir = dir;
    while (currentDir != FILE_MANAGER_SCAN_DIR) {
        MEDIA_INFO_LOG("check path:%{public}s", FileScanUtils::GarbleFilePath(currentDir).c_str());
        if (IsSkipFileManagerDirectory(currentDir)) {
            return true;
        }
        currentDir = fs::path(currentDir).parent_path().string();
    }
    return false;
}

std::shared_ptr<FolderParser> FileManagerScanner::BuildFolderParser(const std::string &path)
{
    return make_shared<FileManagerFolderParser>(path, scanMode_);
}

bool FileManagerScanner::IsSkipFileManagerDirectory(const std::string &currentDir)
{
    if (currentDir.empty()) {
        MEDIA_INFO_LOG("empty path: %{public}s", FileScanUtils::GarbleFilePath(currentDir).c_str());
        return true;
    }

    fs::path currentPath(currentDir);
    std::string folderName = currentPath.filename().string();
    if (folderName.empty() || FILE_MANAGER_BLOCKED_DIRS.count(folderName) > 0) {
        MEDIA_INFO_LOG("need blocked currentDir: %{public}s", FileScanUtils::GarbleFilePath(currentDir).c_str());
        return true;
    }
    return false;
}

void FileManagerScanner::RefreshTrashedAssetInfo(FileManagerParser &fileManagerParser)
{
    MediaLibraryTracer tracer;
    tracer.Start("FileManagerScanner::RefreshTrashedAssetInfo");
    auto fileInfo = fileManagerParser.GetFileInfo();
    fileManagerParser.UpdateTrashedAssetinfo();
    MEDIA_INFO_LOG("update album Id: %{public}d", fileInfo.ownerAlbumId);
}

void FileManagerScanner::RefreshNeedReoverAssetInfo(FileManagerParser &fileManagerParser)
{
    MediaLibraryTracer tracer;
    tracer.Start("FileManagerScanner::RefreshNeedReoverAssetInfo");
    auto fileInfo = fileManagerParser.GetFileInfo();
    fileManagerParser.UpdateRecoverAssetinfo();
    MEDIA_INFO_LOG("update album Id: %{public}d", fileInfo.ownerAlbumId);
}

void FileManagerScanner::RefreshInsertAssetInfo()
{
    if (insertFileInfos_.empty()) {
        MEDIA_INFO_LOG("no insert assets");
        return;
    }
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "rdbStore is null.");
    int64_t insertNum = 0;
    int rdbError = 0;
    AccurateRefresh::AssetAccurateRefresh assetRefresh;
    int32_t ret = assetRefresh.BatchInsert(insertNum, PhotoColumn::PHOTOS_TABLE, insertFileInfos_, rdbError,
        NativeRdb::ConflictResolution::ON_CONFLICT_REPLACE);
    CHECK_AND_RETURN_LOG(ret == E_OK, "Batch insert assets failed.");
    assetRefresh.RefreshAlbum();
    assetRefresh.Notify();
    auto watch = MediaLibraryNotify::GetInstance();
    MEDIA_WARN_LOG("RefreshInsertAssetInfo");
    insertFileInfos_.clear();
    // 触发缩略图生成（支持功耗管控）
    auto uris = FileManagerParser::GenerateThumbnailWithPowerControl(scanMode_, inodes_);
    for (auto notifyUri : uris) {
        watch->Notify(notifyUri, NotifyType::NOTIFY_ADD);
        MEDIA_INFO_LOG("send add notify: %{public}s", MediaFileUtils::DesensitizeUri(notifyUri).c_str());
    }
    std::vector<std::string> fileIds = FileScanUtils::GetFileIdsFromUris(uris);
    FileScanUtils::UpdateAndNotifyAnalysisAlbum(fileIds);
    MEDIA_WARN_LOG("GenerateThumbnailWithPowerControl scan:%{public}d", scanMode_);
}
// LCOV_EXCL_STOP
}