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
 
#define MLOG_TAG "MediaDuplicateCheckerUtils"
 
#include "media_duplicate_checker_utils.h"
 
#include <unistd.h>
#include <cstdio>
#include <regex>
#include "media_log.h"
#include "media_column.h"
#include "medialibrary_errno.h"
#include "photo_album_column.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_rdbstore.h"
#include "result_set_utils.h"
#include "media_file_utils.h"
#include "medialibrary_bundle_manager.h"
#include "values_bucket.h"

using namespace std;

namespace OHOS {
namespace Media {

const std::string PATH_PREFIX = "/storage/media/local/files/Docs";
const std::string BUNDLENAME_PHOTOS = "com.huawei.hmos.photos";
const std::string BUNDLENAME_FILEMANAGER = "com.huawei.hmos.filemanager";
const std::string ROOT_LPATH = "/FromDocs/";
const std::string ROOT_FROM_DOCS_PATH = "/FromDocs";
const int32_t MAX_RENAME_ATTEMPTS = 1000;

// 图库正则校验规则
bool MediaDuplicateCheckerUtils::checkNameValidForMediaLibrary(const string &name)
{
    MEDIA_INFO_LOG("checkNameValidForMediaLibrary enter, name:%{public}s", name.c_str());
    if (name.empty()) {
        MEDIA_ERR_LOG("name is empty.");
        return -EINVAL;
    }

    // static const string REGEX_CHECK = R"([\.\\/:*?"'`<>|{}\[\]])";
    std::string CheckName = name;
    std::regex pattern(R"([\.\\/:*?"'`<>|{}\[\]])");
    std::smatch result;
    return std::regex_search(name, result, pattern);
}

// 根据相册id获取相册实际路径
int32_t MediaDuplicateCheckerUtils::getAlbumLpathByAlbumId(const std::string &albumId, std::string &path)
{
    CHECK_AND_RETURN_RET_LOG(!albumId.empty(), E_ERR, "albumId is null.");
    MEDIA_INFO_LOG("enter getAlbumLpathByAlbumId, albumId:%{public}s", albumId.c_str());
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "rdbStore null.");
    NativeRdb::AbsRdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, albumId);
    predicates.Limit(1);
    auto resultSet = rdbStore->Query(predicates, {PhotoAlbumColumns::ALBUM_LPATH});
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "query album by albumId err");
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string lpath = GetStringVal(PhotoAlbumColumns::ALBUM_LPATH, resultSet);
        CHECK_AND_RETURN_RET_LOG(!lpath.empty(), false, "get lpath is empty");
        path = lpath;
        MEDIA_INFO_LOG("getAlbumLpathByAlbumId lPath:%{public}s", path.c_str());
        return E_OK;
    }
    return E_ERR;
}

int32_t MediaDuplicateCheckerUtils::getAlbumActualPathByAlbumId(const std::string &albumId, std::string &actualPath)
{
    CHECK_AND_RETURN_RET_LOG(!albumId.empty(), E_ERR, "albumId is null.");
    MEDIA_INFO_LOG("enter getAlbumActualPathByAlbumId, albumId:%{public}s", albumId.c_str());
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "rdbStore null.");
    string lpath;
    CHECK_AND_RETURN_RET_LOG(getAlbumLpathByAlbumId(albumId, lpath) == E_OK, E_ERR, "get lpath failed.");
    CHECK_AND_RETURN_RET_LOG(!lpath.empty(), E_ERR, "lpath is null");
    string docPrefix = "/FromDocs/";
    string pathPrefix = "/storage/media/local/files/Docs/";
    if (lpath == ROOT_FROM_DOCS_PATH) {
        actualPath = pathPrefix;
    } else {
        CHECK_AND_RETURN_RET_LOG(lpath.find(docPrefix) == 0, E_OK, "is not file manager album, no need to modify");
        string path = lpath.substr(docPrefix.length());
        actualPath = pathPrefix + path;
    }
    return E_OK;
}

// 根据fileId获取displayName和storagePath
int32_t MediaDuplicateCheckerUtils::getAlbumPathAndDisplayNameByFileId(
    const std::string &fileId, std::string &actualPath, std::string &displayName)
{
    MEDIA_INFO_LOG("getAlbumPathAndDisplayNameByFileId enter, fileId:%{public}s", fileId.c_str());
    CHECK_AND_RETURN_RET_LOG(!fileId.empty(), E_ERR, "fileId null");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "rdbStore null.");
    string sqlStr = "SELECT " + PhotoAlbumColumns::ALBUM_LPATH + "," + MediaColumn::MEDIA_NAME + " FROM " +
        PhotoAlbumColumns::TABLE + " a INNER JOIN " + PhotoColumn::PHOTOS_TABLE + " p ON " + "p." +
        PhotoColumn::PHOTO_OWNER_ALBUM_ID + " = a." + PhotoAlbumColumns::ALBUM_ID + " where p." +
        PhotoColumn::MEDIA_ID + " = " + fileId.c_str() + ";";
    auto resultSet = rdbStore->QuerySql(sqlStr);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "query lpath by fileId err");
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string lpath = GetStringVal(PhotoAlbumColumns::ALBUM_LPATH, resultSet);
        CHECK_AND_RETURN_RET_LOG(!lpath.empty(), false, "get lpath is empty");
        actualPath = PATH_PREFIX + lpath;
        displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
        MEDIA_INFO_LOG("getAlbumPathAndDisplayNameByFileId actualPath:%{public}s, displayName:%{public}s",
            actualPath.c_str(), displayName.c_str());
        return E_OK;
    }
    return E_ERR;
}

// DB中校验新的相冊名是否同名
int32_t MediaDuplicateCheckerUtils::checkAlbumNameDuplicateInDB(const string &newAlbumName)
{
    MEDIA_INFO_LOG("enter checkAlbumNameDuplicateInDB, newAlbumName:%{public}s", newAlbumName.c_str());
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "rdbStore null.");

    NativeRdb::AbsRdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.BeginWrap();

    // 第一部分：(album_type=0 AND album_subtype=1)
    predicates.BeginWrap();
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_TYPE, std::to_string(PhotoAlbumType::USER))->And();
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, std::to_string(PhotoAlbumSubType::USER_GENERIC))->EndWrap();

    // OR 连接第二部分
    predicates.Or()->BeginWrap();
    /*
    SELECT album_name FROM PhotoAlbum WHERE (album_type=2048 and (album_subtype=2049 OR album_subtype=2050)) or
    (album_type=0 AND album_subtype=1)) AND album_name = 'documentsfoshoaj' LIMIT 1
    */
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_TYPE, std::to_string(PhotoAlbumType::SOURCE))->And()->BeginWrap();
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, std::to_string(PhotoAlbumSubType::SOURCE_GENERIC))->Or();
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE,
        std::to_string(PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILE_MANAGER))->EndWrap();

    predicates.EndWrap();  // 结束第二部分
    predicates.EndWrap();  // 结束外层大括号

    predicates.And()->EqualTo(PhotoAlbumColumns::ALBUM_NAME, newAlbumName);
    predicates.Limit(1);
 
    auto resultSet = rdbStore->Query(predicates, {PhotoAlbumColumns::ALBUM_NAME});
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "query album err");
    
    if (resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        MEDIA_ERR_LOG("albumName already exists, newAlbumName:%{public}s", newAlbumName.c_str());
        return E_ERR;
    }

    return E_OK;
}

// 校验相册名称是否重名
int32_t MediaDuplicateCheckerUtils::checkAlbumNameDuplicate(const std::string &albumId, const std::string &newAlbumName)
{
    CHECK_AND_RETURN_RET_LOG(!albumId.empty(), E_ERR, "albumId is empty");
    NativeRdb::AbsRdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, albumId);
    vector<string> columns = {
        PhotoAlbumColumns::ALBUM_NAME,
        PhotoAlbumColumns::ALBUM_SUBTYPE,
        PhotoAlbumColumns::ALBUM_TYPE,
    };
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "rdbStore null.");
    auto resultSet = rdbStore->Query(predicates, {PhotoAlbumColumns::ALBUM_NAME});
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "query album err");
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK, false, "resultSet is empty");
    int32_t albumSubType = GetInt32Val(PhotoAlbumColumns::ALBUM_SUBTYPE, resultSet);
    int32_t albumType = GetInt32Val(PhotoAlbumColumns::ALBUM_TYPE, resultSet);
    // 用户或来源相册
    if ((albumType == PhotoAlbumType::SOURCE && albumSubType == PhotoAlbumSubType::SOURCE_GENERIC) ||
        (albumType == PhotoAlbumType::USER && albumSubType == PhotoAlbumSubType::USER_GENERIC)) {
        CHECK_AND_RETURN_RET_LOG(checkAlbumNameDuplicateInDB(newAlbumName) == E_OK, E_ERR,
            "checkAlbumNameDuplicateInDB failed");
    }
    // 文管相册
    if (albumSubType == PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILE_MANAGER) {
        CHECK_AND_RETURN_RET_LOG(checkAlbumNameDuplicateInDB(newAlbumName) == E_OK, E_ERR,
            "checkAlbumNameDuplicateInDB failed");
        std::string actualPath;
        CHECK_AND_RETURN_RET_LOG(getAlbumActualPathByAlbumId(albumId, actualPath) == E_OK, E_ERR,
            "get actual path failed");
        CHECK_AND_RETURN_RET_LOG(access(actualPath.c_str(), F_OK) != E_OK, E_ERR,
            "the album name already exists in the file management system, albumName:%{public}s, path:%{public}s",
            newAlbumName.c_str(), actualPath.c_str());
    }
    return E_OK;
}

std::string MediaDuplicateCheckerUtils::replaceFilename(const std::string& path, const std::string& newName)
{
    size_t lastSlash = path.find_last_of('/');
    if (lastSlash == std::string::npos) {
        return path;
    }
    std::string directory = path.substr(0, lastSlash + 1);
    std::string filename = path.substr(lastSlash + 1);

    size_t lastDot = filename.find_last_of('.');
    if (lastDot != std::string::npos) {
        std::string extension = filename.substr(lastDot);
        return directory + newName + extension;
    } else {
        return directory + newName;
    }
}

// 校验同目录是否重名
int32_t MediaDuplicateCheckerUtils::checkDirectoryNameConflict(const NativeRdb::ValuesBucket& newAlbumValues)
{
    int32_t albumSubType = 0;
    NativeRdb::ValueObject tmpObject;
    if (newAlbumValues.GetObject(PhotoAlbumColumns::ALBUM_SUBTYPE, tmpObject)) {
        tmpObject.GetInt(albumSubType);
    }
    CHECK_AND_RETURN_RET_LOG(albumSubType == PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILE_MANAGER,
        E_OK, "not file manager album, no need to process.");

    string newAlbumName;
    string newLPath;
    if (newAlbumValues.GetObject(PhotoAlbumColumns::ALBUM_NAME, tmpObject)) {
        tmpObject.GetString(newAlbumName);
    }
    if (newAlbumValues.GetObject(PhotoAlbumColumns::ALBUM_LPATH, tmpObject)) {
        tmpObject.GetString(newLPath);
    }
    CHECK_AND_RETURN_RET_LOG(!newAlbumName.empty(), E_ERR, "newAlbumName is null");
    CHECK_AND_RETURN_RET_LOG(!newLPath.empty(), E_ERR, "newLPath is null");
    MEDIA_INFO_LOG("enter checkDirectoryNameConflict newAlbumName:%{public}s", newAlbumName.c_str());
    CHECK_AND_RETURN_RET_LOG(newLPath.find(ROOT_FROM_DOCS_PATH) == 0, E_ERR, "invalid lpath");
    // 通过lpath解析处storaPath
    string newAlbumPath;
    if (newLPath == ROOT_FROM_DOCS_PATH) {
        newAlbumPath = PATH_PREFIX + "/" + newAlbumName;
    } else if (newLPath == ROOT_LPATH) {
        newAlbumPath = PATH_PREFIX + newAlbumName;
    } else {
        newAlbumPath = PATH_PREFIX + replaceFilename(newLPath.substr(ROOT_FROM_DOCS_PATH.length()), newAlbumName);
    }
    CHECK_AND_RETURN_RET_LOG(access(newAlbumPath.c_str(), F_OK) != E_OK, E_ERR,
        "the name already exists in the file management system, photoName:%{public}s, newAlbumPath:%{private}s",
        newAlbumName.c_str(), newAlbumPath.c_str());
    return E_OK;
}

// 校验图片名称是否重名
int32_t MediaDuplicateCheckerUtils::checkPhotoNameDuplicate(const std::string &fileId, const std::string &newName)
{
    MEDIA_INFO_LOG("enter checkPhotoNameDuplicate, fileId:%{public}s, name:%{public}s",
        fileId.c_str(), newName.c_str());
    CHECK_AND_RETURN_RET_LOG(!(fileId.empty() || newName.empty()), E_ERR, "fileId or newName is empty");
    NativeRdb::AbsRdbPredicates predicates(PhotoAlbumColumns::TABLE);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "rdbStore null.");
    string sqlStr = "SELECT " + PhotoColumn::PHOTO_STORAGE_PATH + "," + MediaColumn::MEDIA_NAME + " FROM " +
        PhotoColumn::PHOTOS_TABLE + " WHERE " + PhotoColumn::PHOTO_FILE_SOURCE_TYPE + " = " +
        to_string(FileSourceType::FILE_MANAGER) + " AND " + MediaColumn::MEDIA_ID + " = " + fileId.c_str() + ";";
    auto resultSet = rdbStore->QuerySql(sqlStr);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "query photo err.");
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        // 查询出来有值，说明是文管资产
        string oldPath = GetStringVal(PhotoColumn::PHOTO_STORAGE_PATH, resultSet);
        std::string newPhotoPath = replaceFilename(oldPath, newName);
        CHECK_AND_RETURN_RET_LOG(access(newPhotoPath.c_str(), F_OK) != E_OK, E_ERR,
            "the name already exists in the file management system, photoName:%{public}s, newPhotoPath:%{private}s",
            newName.c_str(), newPhotoPath.c_str());
    }
    return E_OK;
}

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
} // namespace Media
} // namespace OHOS
