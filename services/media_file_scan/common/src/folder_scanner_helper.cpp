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
#define MLOG_TAG "FolderScannerHelper"
 
#include <filesystem>
#include <ctime>
#include <chrono>
#include <dirent.h>
#include <sstream>
#include <sys/stat.h>
 
#include "folder_scanner_helper.h"

#include "album_scan_info_column.h"
#include "check_scene_helper.h"
#include "file_const.h"
#include "media_log_utils.h"
#include "media_lake_album.h"
#include "medialibrary_errno.h"
#include "media_log.h"
#include "result_set_utils.h"
#include "datashare_values_bucket.h"
#include "medialibrary_unistore_manager.h"
#include "rdb_predicates.h"
 
using namespace std;
 
namespace OHOS::Media {
 
FolderScannerHelper::FolderScannerHelper(int32_t albumId, string path)
{
    albumId_ = albumId;
    storagePath_ = path;
    scene_ = CheckSceneHelper::ResolveSceneByPath(storagePath_);
    InitLakeAlbumInfo();
    InitFolderInfo();
    MEDIA_INFO_LOG("Init %{public}s", ToString().c_str());
}

bool FolderScannerHelper::IsFolderModified()
{
    return folderDateModified_ != databaseDateModified_;
}
 
bool FolderScannerHelper::IsSkipFolderFile()
{
    return !IsFolderModified() && IsLeafFolder();
}
 
void FolderScannerHelper::UpdateFolderModified()
{
    if (isInsert_) {
        InsertFolderModified();
        return;
    }
    if (!IsFolderModified()) {
        MEDIA_INFO_LOG("No change for %{public}s", ToString().c_str());
        return;
    }
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "RdbStore is nullptr!");
    NativeRdb::ValuesBucket values;
    values.Put(LAKE_FOLDER_MODIFIED, folderDateModified_);
    NativeRdb::RdbPredicates predicates(LAKE_ALBUM_TABLE);
    predicates.EqualTo(LAKE_ALBUM_ID, to_string(albumId_));
    predicates.EqualTo(LAKE_ALBUM_LPATH, storagePath_);
    int32_t changeRows = 0;
    int32_t ret = rdbStore->Update(changeRows, values, predicates);
    CHECK_AND_RETURN_LOG(ret == E_OK, "update ret error for %{public}s", ToString().c_str());
    MEDIA_INFO_LOG("Update %{public}s, ret: %{public}d, changeRows: %{public}d", ToString().c_str(), ret, changeRows);
}
 
void FolderScannerHelper::InsertFolderModified()
{
    CHECK_AND_RETURN_LOG(albumId_ > 0, "invalid album id.");

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "RdbStore is nullptr!");
    NativeRdb::ValuesBucket values;
    values.Put(LAKE_ALBUM_ID, albumId_);
    values.Put(LAKE_ALBUM_LPATH, storagePath_);
    values.Put(LAKE_FOLDER_MODIFIED, folderDateModified_);
    int64_t changeId = 0;
    int32_t ret = rdbStore->Insert(changeId, GetTableName(), values);
    MEDIA_INFO_LOG("Insert %{public}s, ret: %{public}d, changeId: %{public}" PRId64, ToString().c_str(), ret,
        changeId);
}
 
void FolderScannerHelper::InitLakeAlbumInfo()
{
    // 查询数据库
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "RdbStore is nullptr!");
    NativeRdb::RdbPredicates predicates(GetTableName());
    predicates.EqualTo(LAKE_ALBUM_ID, to_string(albumId_));
    predicates.EqualTo(LAKE_ALBUM_LPATH, storagePath_);
    predicates.OrderByDesc("rowid");
    predicates.Limit(1);
    auto resultSet = rdbStore->Query(predicates, {LAKE_FOLDER_MODIFIED});
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("resultSet is nullptr!");
        isInsert_ = true;
        return;
    }
    if (resultSet->GoToNextRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("GoToNextRow failed");
        isInsert_ = true;
        resultSet->Close();
        return;
    }
    databaseDateModified_ = get<int64_t>(ResultSetUtils::GetValFromColumn(
        LAKE_FOLDER_MODIFIED, resultSet, TYPE_INT64));
    resultSet->Close();
}
 
void FolderScannerHelper::InitFolderInfo()
{
    struct stat dirStat;
    const int MILLISECOND_PER_SECOND = 1000;
    const int MICROSECOND_PER_SECOND = 1000000;
    if (stat(storagePath_.c_str(), &dirStat) != 0) {
        MEDIA_ERR_LOG("get folder stat error, storagePath: %{public}s",
            MediaLogUtils::GarbleFilePath(storagePath_).c_str());
        return;
    }
    struct timespec ctim = dirStat.st_ctim;
    folderDateModified_ = ctim.tv_sec * MILLISECOND_PER_SECOND + ctim.tv_nsec / MICROSECOND_PER_SECOND;
}

bool FolderScannerHelper::IsLeafFolder()
{
    if (isLeafFolderChecked_) {
        return isLeafFolder_;
    }

    isLeafFolder_ = IsLeafFolderByTraversal();
    isLeafFolderChecked_ = true;
    return isLeafFolder_;
}

bool FolderScannerHelper::IsLeafFolderByTraversal()
{
    DIR* dir = opendir(storagePath_.c_str());
    CHECK_AND_RETURN_RET_LOG(dir != nullptr, false, "Failed to open directory, storagePath: %{public}s",
        MediaLogUtils::GarbleFilePath(storagePath_).c_str());

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        std::string currentPath = storagePath_ + "/" + entry->d_name;
        if (entry->d_type == DT_DIR) {
            MEDIA_WARN_LOG("Path %{public}s is not a leaf folder, has sub-folder %{public}s",
                MediaLogUtils::GarbleFilePath(storagePath_).c_str(),
                MediaLogUtils::GarbleFile(entry->d_name).c_str());
            closedir(dir);
            return false;
        }
        if (entry->d_type == DT_UNKNOWN) {
            struct stat statInfo;
            int32_t ret = lstat(currentPath.c_str(), &statInfo);
            if (ret != 0) {
                MEDIA_ERR_LOG("Path %{public}s lstat failed, ret: %{public}d",
                    MediaLogUtils::GarbleFilePath(storagePath_).c_str(), ret);
                closedir(dir);
                return false;
            }
            if (S_ISDIR(statInfo.st_mode)) {
                MEDIA_WARN_LOG("Path %{public}s is not a leaf folder, has sub-folder %{public}s",
                    MediaLogUtils::GarbleFilePath(storagePath_).c_str(),
                    MediaLogUtils::GarbleFile(entry->d_name).c_str());
                closedir(dir);
                return false;
            }
        }
    }

    closedir(dir);
    return true;
}

std::string FolderScannerHelper::ToString()
{
    stringstream ss;
    ss << "album_id: " << albumId_ << ", "
        << "storagePath: " << MediaLogUtils::GarbleFilePath(storagePath_) << ", "
        << "databaseDateModified: " << databaseDateModified_ << ", "
        << "folderDateModified: " << folderDateModified_;
    return ss.str();
}

std::string FolderScannerHelper::GetTableName()
{
    switch (scene_) {
        case CheckScene::LAKE:
            return LAKE_ALBUM_TABLE;
        default:
            return AlbumScanInfoColumn::TABLE;
    }
}
}