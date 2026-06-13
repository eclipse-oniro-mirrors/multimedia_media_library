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
#define MLOG_TAG "FileManagerAlbumOperations"

#include "file_manager_album_operations.h"

#include <sys/time.h>

#include "dfx_utils.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "medialibrary_unistore_manager.h"
#include "photo_album_column.h"
#include "result_set_utils.h"
#include "media_string_utils.h"
#include "media_file_access_utils.h"
#include "photo_file_utils.h"
#include "media_duplicate_checker_utils.h"
#include "cloud_media_common.h"

using namespace OHOS::NativeRdb;
using namespace OHOS::Media::AccurateRefresh;
namespace OHOS::Media {
const std::string ROOT_PATH_PREFIX = "/storage/media/local/files/Docs/";
const std::string ROOT_LPATH = "/FromDocs/";

static std::string ReplaceLastSegment(const std::string& path, const std::string& newName)
{
    size_t lastSlash = path.find_last_of('/');
    if (lastSlash != std::string::npos) {
        if (lastSlash == 0) {
            return "/" + newName;
        }
        return path.substr(0, lastSlash + 1) + newName;
    }
    return path;
}

static int32_t RenameFilesAndRollbackOnFailure(const int32_t newAlbumId, const std::string &newAlbumName,
    const std::string &newAlbumPath, const std::string &oldAlbumPath)
{
    CHECK_AND_RETURN_RET_LOG(access(newAlbumPath.c_str(), F_OK) != E_OK, E_ERR,
        "the album name already exists in the file management, albumName:%{public}s, newAlbumPath:%{private}s",
        newAlbumName.c_str(), newAlbumPath.c_str());
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateDirectory(newAlbumPath), E_ERR, "create new album path failed");

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "rdbStore null.");

    string selectSqlStr = "SELECT " + PhotoColumn::PHOTO_STORAGE_PATH + "," + MediaColumn::MEDIA_NAME + "," +
        MediaColumn::MEDIA_ID + " FROM " + PhotoColumn::PHOTOS_TABLE + " where " +
        PhotoColumn::PHOTO_FILE_SOURCE_TYPE + " = " + to_string(FileSourceType::FILE_MANAGER) + " AND " +
        PhotoColumn::PHOTO_OWNER_ALBUM_ID + " = " + to_string(newAlbumId);
    auto resultSet = rdbStore->QuerySql(selectSqlStr);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "query lpath by fileId err");

    // 记录成功重命名的文件，用于失败时回滚
    vector<pair<string, string>> renamedFiles; // <oldPath, newPath>
    bool allSuccess = true;

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string displayName = GetStringVal(PhotoColumn::MEDIA_NAME, resultSet);
        std::string oldStoragePath = oldAlbumPath + displayName;
        string newPath = ROOT_PATH_PREFIX + newAlbumName + "/" + displayName;
        renamedFiles.emplace_back(oldStoragePath, newPath);
        if (rename(oldStoragePath.c_str(), newPath.c_str()) != E_OK) {
            MEDIA_ERR_LOG("storage path rename failed, errno:%{public}d, error:%{public}s, path:%{private}s",
                errno, strerror(errno), oldStoragePath.c_str());
            allSuccess = false;
            break;
        }
    }

    if (!allSuccess) {
        MEDIA_ERR_LOG("rename failed, rolling back %{public}zu files", renamedFiles.size());
        for (const auto& [oldPath, newPath] : renamedFiles) {
            if (rename(newPath.c_str(), oldPath.c_str()) != E_OK) {
                MEDIA_ERR_LOG("rollback failed! oldPath:%{private}s, newPath:%{private}s, errno:%{public}d",
                    oldPath.c_str(), newPath.c_str(), errno);
            }
        }
        rmdir(newAlbumPath.c_str());
        return E_ERR;
    }
    return E_OK;
}

int32_t FileManagerAlbumOperations::RenameFileManagerAlbum(
    const string &oldAlbumPath, const int32_t newAlbumId, const string &newAlbumName)
{
    MEDIA_INFO_LOG("enter rename file manager album");
    CHECK_AND_RETURN_RET_LOG(newAlbumId > 0, E_ERR, "Invalid album id:%{public}d", newAlbumId);
    CHECK_AND_RETURN_RET_LOG(!newAlbumName.empty(), E_ERR, "newAlbumName is null");
    CHECK_AND_RETURN_RET_LOG(!oldAlbumPath.empty(), E_ERR, "oldAlbumPath is null");
    string newAlbumPath =  ReplaceLastSegment(oldAlbumPath, newAlbumName);
    if (oldAlbumPath == ROOT_PATH_PREFIX) {
        CHECK_AND_RETURN_RET_LOG(
            RenameFilesAndRollbackOnFailure(newAlbumId, newAlbumName, newAlbumPath, oldAlbumPath) == E_OK,
            E_ERR, "move file failed.");
    } else {
        CHECK_AND_RETURN_RET_LOG(rename(oldAlbumPath.c_str(), newAlbumPath.c_str()) == E_OK, E_ERR,
            "rename failed! oldPath:%{private}s, newPath:%{private}s, errno:%{public}d, error:%{public}s",
            oldAlbumPath.c_str(), newAlbumPath.c_str(), errno, strerror(errno));
    }
    return E_OK;
}
}  // namespace OHOS::Media