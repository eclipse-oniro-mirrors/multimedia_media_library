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
#include <sys/stat.h>
 
#include "folder_scanner_helper.h"
#include "lake_file_utils.h"
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
    MEDIA_INFO_LOG("albumId: %{public}d, path: %{public}s", albumId, LakeFileUtils::GarbleFilePath(path).c_str());
    albumId_ = albumId;
    storagePath_ = path;
    InitLakeAlbumInfo();
    InitFolderInfo();
}
 
bool FolderScannerHelper::IsFolderModified()
{
    return folderDateModified_ != albumInfoDateModified_;
}
 
bool FolderScannerHelper::IsSkipFolderFile()
{
    return !IsFolderModified();
}
 
void FolderScannerHelper::UpdateFolderModified()
{
    MEDIA_INFO_LOG("albumId: %{public}d", albumId_);
    if (isInsert_) {
        InsertFolderModified();
        return;
    }
    if (!IsFolderModified() && albumInfoPath_ == storagePath_) {
        MEDIA_INFO_LOG("no change path[%{public}s], dateModified[%{public}lld]",
            LakeFileUtils::GarbleFilePath(storagePath_).c_str(), folderDateModified_);
        return;
    }
    NativeRdb::ValuesBucket values;
    if (albumInfoPath_ != storagePath_) {
        values.Put(LAKE_ALBUM_LPATH, storagePath_);
    }

    if (IsFolderModified()) {
        values.Put(LAKE_FOLDER_MODIFIED, folderDateModified_);
    }

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "RdbStore is nullptr!");
    NativeRdb::RdbPredicates predicates(LAKE_ALBUM_TABLE);
    predicates.EqualTo(LAKE_ALBUM_ID, to_string(albumId_));
    int32_t changeRows = 0;
    int32_t ret = rdbStore->Update(changeRows, values, predicates);
    CHECK_AND_RETURN_LOG(ret == E_OK, "update ret error");
    MEDIA_INFO_LOG("update change path[%{public}s], dateModified[%{public}lld]",
        LakeFileUtils::GarbleFilePath(storagePath_).c_str(), folderDateModified_);
}
 
void FolderScannerHelper::InsertFolderModified()
{
    if (albumId_ < 0) {
        MEDIA_ERR_LOG("invalid album id.");
        return;
    }
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "RdbStore is nullptr!");
    NativeRdb::ValuesBucket values;
    values.Put(LAKE_ALBUM_ID, albumId_);
    values.Put(LAKE_ALBUM_LPATH, storagePath_);
    values.Put(LAKE_FOLDER_MODIFIED, folderDateModified_);
    int64_t changeId = 0;
    int32_t ret = rdbStore->Insert(changeId, LAKE_ALBUM_TABLE, values);
    MEDIA_INFO_LOG("insert albumId[%{public}d] path[%{public}s], dateModified[%{public}lld]", albumId_,
        LakeFileUtils::GarbleFilePath(storagePath_).c_str(), folderDateModified_);
}
 
void FolderScannerHelper::InitLakeAlbumInfo()
{
    // 查询数据库
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "RdbStore is nullptr!");
    NativeRdb::RdbPredicates predicates(LAKE_ALBUM_TABLE);
    predicates.EqualTo(LAKE_ALBUM_ID, to_string(albumId_));
    auto resultSet = rdbStore->Query(predicates, {LAKE_ALBUM_LPATH, LAKE_FOLDER_MODIFIED});
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "resultSet is nullptr!");
    int32_t count;
    int32_t ret = resultSet->GetRowCount(count);
    // 找不到需要刷新
    if (ret == E_OK) {
        if (count == 0) {
            MEDIA_INFO_LOG("insert path %{public}s", LakeFileUtils::GarbleFilePath(storagePath_).c_str());
            isInsert_ = true;
            return;
        } else if (count > 1) {
            MEDIA_ERR_LOG("count[%{public}d] error", count);
            return;
        }
    } else {
        MEDIA_ERR_LOG("get result error: %{public}d", ret);
        return;
    }
    while (resultSet->GoToNextRow() == E_OK) {
        albumInfoPath_ = get<string>(ResultSetUtils::GetValFromColumn(LAKE_ALBUM_LPATH, resultSet, TYPE_STRING));
        albumInfoDateModified_ = get<int64_t>(ResultSetUtils::GetValFromColumn(
            LAKE_FOLDER_MODIFIED, resultSet, TYPE_INT64));
    }
    resultSet->Close();
    MEDIA_INFO_LOG("lake album info: path[%{public}s], dateModified[%{public}" PRId64"] ",
        LakeFileUtils::GarbleFilePath(albumInfoPath_).c_str(), albumInfoDateModified_);
}
 
void FolderScannerHelper::InitFolderInfo()
{
    struct stat dirStat;
    const int MILLISECOND_PER_SECOND = 1000;
    const int MICROSECOND_PER_SECOND = 1000000;
    if (stat(storagePath_.c_str(), &dirStat) == 0) {
        struct timespec ctim = dirStat.st_ctim;
        folderDateModified_ = ctim.tv_sec * MILLISECOND_PER_SECOND + ctim.tv_nsec / MICROSECOND_PER_SECOND;
        MEDIA_INFO_LOG("folder modified: %{public}lld", folderDateModified_);
    } else {
        MEDIA_INFO_LOG("get folder stat error");
    }
}
 
}