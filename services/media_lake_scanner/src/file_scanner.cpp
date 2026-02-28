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

#define MLOG_TAG "FileScanner"

#include "file_scanner.h"

#include <algorithm>
#include <filesystem>

#include "global_scanner.h"
#include "media_log.h"
#include "medialibrary_rdb_utils.h"
#include "media_column.h"
#include "medialibrary_errno.h"
#include "medialibrary_notify.h"
#include "media_file_utils.h"
#include "thumbnail_service.h"

using namespace std;

namespace OHOS::Media {

FileScanner::FileScanner(LakeScanMode scanMode) : scanMode_(scanMode) {}

int32_t FileScanner::Run(vector<MediaLakeNotifyInfo> fileInfos)
{
    MEDIA_WARN_LOG("FileScanner::Run enter");
    CHECK_AND_RETURN_RET_WARN_LOG(!fileInfos.empty(), RET_SUCCESS, "empty files");
    if (IsIncrementScanConflict(fileInfos)) {
        return RET_SUCCESS;
    }

    // 文件排序
    sort(fileInfos.begin(), fileInfos.end(), [](const MediaLakeNotifyInfo &a, const MediaLakeNotifyInfo &b) {
        return a.afterPath > b.afterPath;
    });

    for (auto &fileInfo : fileInfos) {
        auto afterPath = fileInfo.afterPath;
        MEDIA_WARN_LOG("afterPath: %{private}s", afterPath.c_str());
        if (afterPath.empty()) {
            MEDIA_WARN_LOG("empty afterPath");
            continue;
        }

        FileParser fileParser(fileInfo, scanMode_);
        auto updateType = fileParser.GetFileUpdateType();
        if (!fileParser.IsFileValidAsset()) {
            RefreshAssetInfoForSkipFile(fileParser, updateType);
            MEDIA_WARN_LOG("invalid file");
            continue;
        }

        error_code ec;
        if (!filesystem::is_regular_file(afterPath, ec) || FolderScannerUtils::IsSkipCurrentFile(afterPath)) {
            RefreshAssetInfoForSkipFile(fileParser, updateType);
            MEDIA_WARN_LOG("skip file: %{private}s", afterPath.c_str ());
            continue;
        }

        switch (updateType) {
            case FileUpdateType::UPDATE:
                RefreshUpdateAssetInfo(fileParser);
                break;
            case FileUpdateType::INSERT:
                GetInsertAssetInfo(fileInfo, fileParser);
                break;
            case FileUpdateType::UPDATE_ALBUM:
                RefreshUpdateAssetAlbumInfo(fileInfo, fileParser);
                break;
            default:
                MEDIA_INFO_LOG("no change");
                break;
        }
    }
    RefreshInsertAssetInfo();
    UpdateAlbum();
    return RET_SUCCESS;
}

void FileScanner::RefreshUpdateAssetInfo(FileParser &fileParser)
{
    auto fileInfo = fileParser.GetFileInfo();
    fileParser.UpdateAssetInfo();
    albumIds_.push_back(to_string(fileInfo.ownerAlbumId));
    MEDIA_INFO_LOG("update album Id: %{public}d", fileInfo.ownerAlbumId);
    auto watch = MediaLibraryNotify::GetInstance();
    auto notifyUri = fileParser.GetFileAssetUri();
    watch->Notify(notifyUri, NotifyType::NOTIFY_UPDATE);
    MEDIA_INFO_LOG("send update notify: %{public}s", MediaFileUtils::DesensitizeUri(notifyUri).c_str());
}

void FileScanner::RefreshUpdateAssetAlbumInfo(MediaLakeNotifyInfo &notifyFileInfo, FileParser &fileParser)
{
    if (!CheckUpdateFolderParserInfo(notifyFileInfo)) {
        RefreshAssetInfoForSkipFile(fileParser, FileUpdateType::UPDATE_ALBUM);
        MEDIA_WARN_LOG("skip file: %{private}s", notifyFileInfo.afterPath.c_str());
        return;
    }
    auto folderParser = folderParserInfo_.folderParser_;
    LakeAlbumInfo albumInfo = folderParser->GetAlbumInfo();
    auto fileInfo = fileParser.GetFileInfo();
    auto oldAlbumId = fileInfo.ownerAlbumId;
    fileParser.UpdateAssetInfo(albumInfo.albumId, albumInfo.bundleName, albumInfo.albumName);
    albumIds_.push_back(to_string(oldAlbumId));
    MEDIA_INFO_LOG("old album Id: %{public}d", oldAlbumId);
    albumIds_.push_back(to_string(albumInfo.albumId));
    MEDIA_INFO_LOG("new album Id: %{public}d", albumInfo.albumId);
    auto watch = MediaLibraryNotify::GetInstance();
    auto notifyUri = fileParser.GetFileAssetUri();
    watch->Notify(notifyUri, NotifyType::NOTIFY_UPDATE);
    MEDIA_INFO_LOG("send update notify: %{public}s", MediaFileUtils::DesensitizeUri(notifyUri).c_str());
}

void FileScanner::GetInsertAssetInfo(MediaLakeNotifyInfo &fileInfo, FileParser &fileParser)
{
    if (!CheckUpdateFolderParserInfo(fileInfo)) {
        MEDIA_WARN_LOG("skip file: %{private}s", fileInfo.afterPath.c_str());
        return;
    }
    auto folderParser = folderParserInfo_.folderParser_;
    auto albumInfo = folderParser->GetAlbumInfo();
    // 获取插入信息
    auto valueBucket = fileParser.TransFileInfoToBucket(albumInfo.albumId, albumInfo.bundleName, albumInfo.albumName);
    if (valueBucket.IsEmpty()) {
        MEDIA_ERR_LOG("Fail to insert AssetBucket");
        return;
    }
    insertFileInfos_.push_back(valueBucket);
    auto innerFileInfo = fileParser.GetFileInfo();
    inodes_.push_back(innerFileInfo.inode);
    albumIds_.push_back(to_string(albumInfo.albumId));
    MEDIA_INFO_LOG("insert album Id: %{public}d inode: %{public}s", albumInfo.albumId, innerFileInfo.inode.c_str());
}

bool FileScanner::CheckUpdateFolderParserInfo(MediaLakeNotifyInfo &fileInfo)
{
    auto file = fileInfo.afterPath;
    auto folderPath = GetFolder(file);
    if (folderPath.empty()) {
        MEDIA_WARN_LOG("empty folderPath, skip");
        return false;
    }
    if (FolderScannerUtils::IsSkipDirectory(folderPath)) {
        MEDIA_WARN_LOG("skip folderPath: %{private}s", folderPath.c_str());
        return false;
    }
    if (folderParserInfo_.currentFolderPath_ == folderPath) {
        return !folderParserInfo_.isSkipFolder;
    }

    folderParserInfo_.currentFolderPath_ = folderPath;
    folderParserInfo_.folderParser_ = make_shared<FolderParser>(folderPath, scanMode_);
    folderParserInfo_.currentOptType_ = folderParserInfo_.folderParser_->PreProcessFolder();
    folderParserInfo_.isSkipFolder = false;
    if (folderParserInfo_.currentOptType_ == FolderOperationType::INSERT) {
        folderParserInfo_.folderParser_->InsertAlbumInfo();
    } else if (folderParserInfo_.currentOptType_ == FolderOperationType::SKIP) {
        folderParserInfo_.isSkipFolder = true;
        MEDIA_WARN_LOG("Need skip folderPath: %{private}s", folderPath.c_str());
        return false;
    }
    MEDIA_INFO_LOG("new folderPath parser: %{private}s", folderPath.c_str());
    return true;
}

string FileScanner::GetFolder(string file)
{
    filesystem::path filePath(file);
    auto parentPath = filePath.parent_path();
    return parentPath.string();
}

void FileScanner::RefreshInsertAssetInfo()
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
    assetRefresh.Notify();
    auto watch = MediaLibraryNotify::GetInstance();
    MEDIA_WARN_LOG("RefreshInsertAssetInfo");
    insertFileInfos_.clear();
    // 触发缩略图生成
    auto uris = FileParser::GenerateThumbnail(scanMode_, inodes_);
    for (auto notifyUri : uris) {
        watch->Notify(notifyUri, NotifyType::NOTIFY_ADD);
        MEDIA_INFO_LOG("send add notify: %{public}s", MediaFileUtils::DesensitizeUri(notifyUri).c_str());
    }
    MEDIA_WARN_LOG("GenerateThumbnail scan:%{public}d", scanMode_);
}

void FileScanner::UpdateAlbum()
{
    if (albumIds_.empty()) {
        MEDIA_INFO_LOG("no update album ids");
        return;
    }
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "rdbStore is null.");
    MediaLibraryRdbUtils::UpdateCommonAlbumInternal(rdbStore, albumIds_, true, true);
    MediaLibraryRdbUtils::UpdateCommonAlbumHiddenState(rdbStore, albumIds_);
    albumIds_.clear();

    // 文件扫描触发的场景需要更新的系统相册
    // 1、新增：favorite、trash、hidden、云增强为数据库属性，从文件中无法得到，不涉及
    // 2、更新：文件有不包含favorite、trash、hidden、云增强属性，不涉及
    // 3、删除：不涉及文件扫描场景
    MediaLibraryRdbUtils::UpdateSystemAlbumInternal(rdbStore, { to_string(PhotoAlbumSubType::VIDEO),
        to_string(PhotoAlbumSubType::IMAGE) }, true);
    MediaLibraryRdbUtils::UpdateSysAlbumHiddenState(rdbStore, { to_string(PhotoAlbumSubType::VIDEO),
        to_string(PhotoAlbumSubType::IMAGE) });

    // 智慧相册是否需要更新
    MEDIA_WARN_LOG("UpdateAlbum");
}

bool FileScanner::IsIncrementScanConflict(std::vector<MediaLakeNotifyInfo> fileInfos)
{
    if (scanMode_ == LakeScanMode::INCREMENT &&
        GlobalScanner::GetInstance().IsGlobalScanning(fileInfos, ScanTaskType::File)) {
        MEDIA_INFO_LOG("Global scan in progress, files scan later");
        return true;
    }
    return false;
}

void FileScanner::RefreshAssetInfoForSkipFile(FileParser &fileParser, FileUpdateType type)
{
    // 新路径为skip场景下，UPDATE和UPDATE_ALBUM变为删除处理；INSERT不处理
    if (type == FileUpdateType::UPDATE || type == FileUpdateType::UPDATE_ALBUM) {
        auto fileInfo = fileParser.GetFileInfo();
        auto fileId = fileInfo.fileId;
        auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
        CHECK_AND_RETURN_LOG(rdbStore != nullptr, "rdbStore is null.");
        NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
        predicates.EqualTo(PhotoColumn::MEDIA_ID, fileId);
        int32_t deletedRows = -1;
        AccurateRefresh::AssetAccurateRefresh assetRefresh;
        int32_t ret = assetRefresh.Delete(deletedRows, predicates);
        MEDIA_INFO_LOG("for skip file update, delete: fileId[%{public}d], ret[%{public}d], deletedRows[%{public}d]",
            fileId, ret, deletedRows);
        albumIds_.push_back(to_string(fileInfo.ownerAlbumId));
        MEDIA_INFO_LOG("update album Id: %{public}d", fileInfo.ownerAlbumId);
        DeleteRelatedResource(fileInfo);
        assetRefresh.RefreshAlbum();
        assetRefresh.Notify();
        auto watch = MediaLibraryNotify::GetInstance();
        auto notifyUri = fileParser.GetFileAssetUri();
        watch->Notify(notifyUri, NotifyType::NOTIFY_REMOVE);
    }
}

void FileScanner::DeleteRelatedResource(InnerFileInfo &fileInfo)
{
    // 缩略图删除
    ThumbnailService::GetInstance()->DeleteThumbnailDirAndAstc(to_string(fileInfo.fileId), PhotoColumn::PHOTOS_TABLE,
        fileInfo.cloudPath, to_string(fileInfo.dateTaken));

    // 编辑数据删除
    auto data = fileInfo.cloudPath;
    CHECK_AND_RETURN_LOG(data.length() >= ROOT_MEDIA_DIR.length(), "path: %{private}s", data.c_str());
    string editDataDirPath = MEDIA_EDIT_DATA_DIR + data.substr(ROOT_MEDIA_DIR.length());
    CHECK_AND_RETURN_LOG(!editDataDirPath.empty(), "Cannot get editPath, path: %{private}s", data.c_str());
    if (MediaFileUtils::IsFileExists(editDataDirPath)) {
        CHECK_AND_RETURN_LOG(MediaFileUtils::DeleteDir(editDataDirPath),
            "Failed to delete edit data, data: %{private}s", editDataDirPath.c_str());
    }
    MEDIA_INFO_LOG("DeleteRelatedResource success");
}
}