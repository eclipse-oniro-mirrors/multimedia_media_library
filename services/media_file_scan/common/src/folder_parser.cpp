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
#define MLOG_TAG "FolderParser"

#include "folder_parser.h"

#include <filesystem>
#include <sys/stat.h>
#include <chrono>
#include <cctype>
#include "album_accurate_refresh.h"
#include "media_log.h"
#include "photo_album_column.h"
#include "result_set_utils.h"
#include "file_scan_utils.h"
#include "photo_album_upload_status_operation.h"
#include "media_string_utils.h"
#include "settings_data_manager.h"
#include "medialibrary_tracer.h"

using namespace std;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {
const std::string PATH_PREFIX = "/storage/media/local/files/Docs/HO_DATA_EXT_MISC";
const int32_t PHOTO_ALBUM_EXIST = 1;
const int32_t ALBUM_PLUGIN_NOT_EXIST = 2;
// LCOV_EXCL_START
FolderParser::FolderParser(const std::string &storagePath, ScanMode scanMode) : scanMode_(scanMode)
{
    MEDIA_INFO_LOG("FolderParser init, storagePath is: %{public}s",
        FileScanUtils::GarbleFilePath(storagePath).c_str());
    storagePath_ = storagePath;
    mediaLibraryRdb_ = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
}

int32_t FolderParser::GetAlbumPhotoInfo(CommonAlbumInfo &photoAlbumInfo)
{
    CHECK_AND_RETURN_RET_LOG(mediaLibraryRdb_ != nullptr, E_ERR, "GetAlbumPhotoInfo: rdb is nullptr");
    std::vector<NativeRdb::ValueObject> bindArgs = {commonAlbumInfo_.lpath};
    MEDIA_DEBUG_LOG("lpath is: %{public}s",
        FileScanUtils::GarbleFilePath(commonAlbumInfo_.lpath).c_str());
    std::string querySql = SQL_PHOTO_ALBUM_SELECT_BY_LPATH;
    auto resultSet = mediaLibraryRdb_->QuerySql(querySql, bindArgs);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Query failed");
        if (resultSet != nullptr) {
            resultSet->Close();
        }
        photoAlbumInfo.isValid = false;
        return E_ERR;
    }
    photoAlbumInfo.isValid = true;
    photoAlbumInfo.lpath = GetStringVal(PhotoAlbumColumns::ALBUM_LPATH, resultSet);
    photoAlbumInfo.albumName = GetStringVal(PhotoAlbumColumns::ALBUM_NAME, resultSet);
    photoAlbumInfo.bundleName = GetStringVal(PhotoAlbumColumns::ALBUM_BUNDLE_NAME, resultSet);
    photoAlbumInfo.albumId = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSet);
    resultSet->Close();
    MEDIA_DEBUG_LOG("album_name is: %{public}s",
        FileScanUtils::GarbleFile(photoAlbumInfo.albumName).c_str());
    return E_OK;
}

int32_t FolderParser::GetAlbumPluginInfo(AlbumPluginInfo &albumPluginInfo)
{
    CHECK_AND_RETURN_RET_LOG(mediaLibraryRdb_ != nullptr, E_ERR, "GetAlbumPluginInfo: rdb is nullptr");
    std::vector<NativeRdb::ValueObject> bindArgs = {commonAlbumInfo_.lpath};
    MEDIA_DEBUG_LOG("lpath is: %{public}s",
        FileScanUtils::GarbleFilePath(commonAlbumInfo_.lpath).c_str());
    std::string querySql = SQL_QUERY_ALBUM_NAME_FROM_ALBUM_PLUGIN_ONLY;
    auto resultSet = mediaLibraryRdb_->QuerySql(querySql, bindArgs);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Query failed");
        if (resultSet != nullptr) {
            resultSet->Close();
        }
        albumPluginInfo.isValid = false;
        return E_ERR;
    }
    albumPluginInfo.isValid = true;
    albumPluginInfo.lpath = GetStringVal("lpath", resultSet);
    albumPluginInfo.albumName = GetStringVal("album_name", resultSet);
    albumPluginInfo.bundleName = GetStringVal("bundle_name", resultSet);
    albumPluginInfo.albumNameEn = GetStringVal("album_name_en", resultSet);
    MEDIA_DEBUG_LOG("lpath is: %{public}s",
        FileScanUtils::GarbleFilePath(albumPluginInfo.lpath).c_str());
    resultSet->Close();
    return E_OK;
}

// 不区分大小写的字符比较函数
bool CaseInsensitiveCharCompare(char a, char b)
{
    return std::tolower(static_cast<unsigned char>(a)) ==
           std::tolower(static_cast<unsigned char>(b));
}

// 检查 str 是否以 prefix 开头（忽略大小写）
bool FolderParser::StartsWithIgnoreCase(const std::string& str, const std::string& prefix)
{
    if (prefix.length() > str.length()) {
        return false;
    }
    return std::equal(prefix.begin(), prefix.end(), str.begin(), CaseInsensitiveCharCompare);
}

std::string to_lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

int32_t FolderParser::PreProcessUpdate()
{
    commonAlbumInfo_.albumName = albumPluginInfo_.albumName;
    commonAlbumInfo_.lpath = albumPluginInfo_.lpath;
    return E_OK;
}

FolderOperationType FolderParser::PreProcessFolder()
{
    if (!hasConverted && GetConvertedLpath(storagePath_, commonAlbumInfo_.lpath) == E_OK) {
        isLpathValid = true;
        GetAlbumPhotoInfo(commonAlbumInfo_);
        GetAlbumPluginInfo(albumPluginInfo_);
    }
    hasConverted = true;
    if (!isLpathValid) {
        MEDIA_INFO_LOG("input storagePath is invalid, check input param");
        return FolderOperationType::SKIP;
    }
    if (IsFolderSkip()) {
        return FolderOperationType::SKIP;
    }
    MEDIA_DEBUG_LOG("albumPluginInfo_.isValid is: %{public}d",
        static_cast<int>(albumPluginInfo_.isValid));

    if (commonAlbumInfo_.isValid) {
        if (!albumPluginInfo_.isValid) {
            // 表中有，白名单中没有，不需要处理
            return FolderOperationType::CONTINUE;
        }
        if (to_lower(commonAlbumInfo_.albumName) == to_lower(albumPluginInfo_.albumName)) {
            // 表中相册名和白名单相册名一致，不需要修改
            return FolderOperationType::CONTINUE;
        }
        return FolderOperationType::UPDATE;
    }
    return hasInsert ? FolderOperationType::CONTINUE : FolderOperationType::INSERT;
}

CommonAlbumInfo FolderParser::GetAlbumInfo()
{
    return commonAlbumInfo_;
}

int32_t FolderParser::InsertAlbumInfo()
{
    MEDIA_INFO_LOG("get or create album operation start");
    CHECK_AND_RETURN_RET_LOG(!commonAlbumInfo_.lpath.empty(), E_ERR, "input lpath is empty, check input param");

    // 3. Get whether the album in the PhotoAlnumb table already exists according to lpath.
    // If it exists return PHOTO_ALBUM_EXIST.
    CHECK_AND_RETURN_RET_LOG(GetMediaLibraryRdb() != nullptr, E_ERR, "InsertAlbumInfo rdb is null.");

    CHECK_AND_RETURN_RET_LOG(PreProcessFolder() == FolderOperationType::INSERT, E_ERR, "do not pass the preprocess");

    // 4. get the displayName
    GetDisplayName(commonAlbumInfo_.lpath, commonAlbumInfo_.displayName);

    // 5. Query album_plugin(whitelist) in the lake according to lPath, get the album name in the lake,
    // if there is no album name, use displayName
    if (GetAlbumName(commonAlbumInfo_) == E_ERR) {
        return E_ERR;
    }
    MEDIA_INFO_LOG("GetAlbumName end2, albumName is : %{public}s",
        FileScanUtils::GarbleFile(commonAlbumInfo_.albumName).c_str());
    // 6. Query the data in the PhotoAlbum table where the album name and lPath are different,
    // and check whether the album name already exists.
    std::string albumName = commonAlbumInfo_.albumName;
    if (albumPluginInfo_.isValid) {
        albumName = albumPluginInfo_.albumName;
    } else {
        GetUniqueAlbumName(albumName);
    }
    MEDIA_INFO_LOG("GetAlbumName end3, albumName is : %{public}s",
        FileScanUtils::GarbleFile(albumName).c_str());
    commonAlbumInfo_.albumName = albumName;

    // 7. insert
    int32_t res = InsertAlbum(commonAlbumInfo_);
    if (res == E_OK) {
        hasInsert = true;
    }
    return res;
}

int32_t FolderParser::UpdatePhotoAlbum()
{
    MEDIA_INFO_LOG("update album operation start");
    CHECK_AND_RETURN_RET_LOG(!storagePath_.empty(), E_ERR, "storagePath is empty, check input param");
    CHECK_AND_RETURN_RET_LOG(isLpathValid, E_ERR, "input storagePath is invalid, check input param");
    CHECK_AND_RETURN_RET_LOG(PreProcessFolder() == FolderOperationType::UPDATE, E_ERR, "do not pass the preprocess");
    CommonAlbumInfo lakeAlbumInfoBackUp = commonAlbumInfo_;
    PreProcessUpdate();
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_LPATH, commonAlbumInfo_.lpath);
    int32_t ret = UpdateAlbum(commonAlbumInfo_, predicates);
    if (ret != E_OK) {
        commonAlbumInfo_ = lakeAlbumInfoBackUp;
    }
    return ret;
}

std::shared_ptr<MediaLibraryRdbStore> FolderParser::GetMediaLibraryRdb()
{
    if (mediaLibraryRdb_ == nullptr) {
        mediaLibraryRdb_ = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
        return mediaLibraryRdb_;
    }
    return mediaLibraryRdb_;
}

int32_t FolderParser::GetDisplayName(const std::string &path, std::string &displayName)
{
    std::filesystem::path pth(path);
    displayName = pth.filename().string();
    return E_OK;
}

int32_t FolderParser::GetAlbumName(CommonAlbumInfo &commonAlbumInfo)
{
    std::string displayName = commonAlbumInfo.displayName;
    std::string lpath = commonAlbumInfo.lpath;
    if (displayName.empty()) {
        if (lpath != "/") {
            MEDIA_INFO_LOG("displayName is empty, can not convert to album name");
            return E_ERR;
        }
    }
    if (!albumPluginInfo_.isValid) {
        MEDIA_INFO_LOG("GetAlbumName from displayName");
        commonAlbumInfo.albumName = displayName;
    } else {
        MEDIA_INFO_LOG("GetAlbumName from album");
        commonAlbumInfo.albumName = albumPluginInfo_.albumName;
    }
    commonAlbumInfo.bundleName = albumPluginInfo_.bundleName;
    MEDIA_INFO_LOG("GetAlbumName end, albumName is : %{public}s",
        FileScanUtils::GarbleFile(commonAlbumInfo.albumName).c_str());
    return E_OK;
}

bool FolderParser::CheckAlbumNameUnique(const std::string &albumName)
{
    MEDIA_INFO_LOG("CheckAlbumNameUnique start");
    std::vector<NativeRdb::ValueObject> bindArgs = {albumName};
    std::string querySql = SQL_PHOTO_ALBUM_CHECK_ALBUM_NAME_UNIQUE;
    CHECK_AND_RETURN_RET_LOG(mediaLibraryRdb_ != nullptr, true, "FolderParser::CheckAlbumName: rdb is null.");
    auto resultSet = mediaLibraryRdb_->QuerySql(querySql, bindArgs);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_INFO_LOG("FolderParser::CheckAlbumNameUnique: Query resultSql is null.");
        if (resultSet != nullptr) {
            resultSet->Close();
        }
        return true;
    }
    int32_t count = GetInt32Val("count", resultSet);
    resultSet->Close();
    MEDIA_INFO_LOG("CheckAlbumNameUnique end, count: %{public}d", count);
    return count == 0;
}

long long FolderParser::GetTimestampMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

void BuildAlbumTypeByLPath(const std::string &lPath, NativeRdb::ValuesBucket& value)
{
    int32_t albumType = static_cast<int32_t>(PhotoAlbumType::SOURCE);
    int32_t albumSubType = static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC);

    std::string target = "/Pictures/Users/";
    std::transform(target.begin(), target.end(), target.begin(), ::tolower);
    std::string lPathLower = lPath;
    std::transform(lPathLower.begin(), lPathLower.end(), lPathLower.begin(), ::tolower);
    if (lPathLower.find(target) == 0) {
        albumType = static_cast<int32_t>(PhotoAlbumType::USER);
        albumSubType = static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC);
    }
    target = FILE_MANAGER_LPATH_PREFIX;
    std::transform(target.begin(), target.end(), target.begin(), ::tolower);
    if (lPathLower.find(target) == 0) {
        albumSubType = static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILEMANAGER);
    }
    value.PutInt(PhotoAlbumColumns::ALBUM_TYPE, albumType);
    value.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, albumSubType);
}

int32_t FolderParser::InsertAlbum(CommonAlbumInfo &commonAlbumInfo)
{
    MediaLibraryTracer tracer;
    tracer.Start("FolderParser::InsertAlbum");
    MEDIA_INFO_LOG("FolderParser: begin insert PhotoAlbum.");
    CHECK_AND_RETURN_RET_LOG(GetMediaLibraryRdb() != nullptr, E_ERR, "InsertAlbum: rdb is null.");
    NativeRdb::ValuesBucket value;
    BuildAlbumTypeByLPath(commonAlbumInfo.lpath, value);
    auto currentTime = this->GetTimestampMs();
    value.PutLong(PhotoAlbumColumns::ALBUM_DATE_ADDED, static_cast<int64_t>(currentTime));
    value.PutLong(PhotoAlbumColumns::ALBUM_DATE_MODIFIED, static_cast<int64_t>(currentTime));
    value.PutString(PhotoAlbumColumns::ALBUM_NAME, commonAlbumInfo.albumName);
    value.PutString(PhotoAlbumColumns::ALBUM_LPATH, commonAlbumInfo.lpath);
    value.PutString(PhotoAlbumColumns::ALBUM_BUNDLE_NAME, commonAlbumInfo.bundleName);
    value.PutInt(PhotoAlbumColumns::UPLOAD_STATUS, PhotoAlbumUploadStatusOperation::GetAlbumUploadStatus());

    int64_t albumId = 0;
    AccurateRefresh::AlbumAccurateRefresh albumRefresh;
    int32_t ret = albumRefresh.Insert(albumId, PhotoAlbumColumns::TABLE, value);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK && albumId > 0, E_ERR,
        "Insert photo albums failed, failed albumId is %{public}lld and lpath is %{public}s",
        albumId, FileScanUtils::GarbleFilePath(commonAlbumInfo.lpath).c_str());
    commonAlbumInfo.albumId = static_cast<int32_t>(albumId);

    // 文管相册设置云开关
    CHECK_AND_EXECUTE(!MediaStringUtils::StartsWith(commonAlbumInfo.lpath, FILE_MANAGER_ROOT_LPATH),
        SettingsDataManager::ComfirmUploadStatus());
    MEDIA_INFO_LOG("FolderParser: end insert PhotoAlbum.");
    return ret;
}

int32_t FolderParser::UpdateAlbum(const CommonAlbumInfo &commonAlbumInfo, const NativeRdb::AbsRdbPredicates &predicates)
{
    MediaLibraryTracer tracer;
    tracer.Start("FolderParser::UpdateAlbum");
    MEDIA_INFO_LOG("FolderParser: begin update PhotoAlbum.");
    CHECK_AND_RETURN_RET_LOG(GetMediaLibraryRdb() != nullptr, E_ERR, "UpdateAlbum: rdb is null.");

    NativeRdb::ValuesBucket value;
    value.PutString(PhotoAlbumColumns::ALBUM_NAME, commonAlbumInfo.albumName);
    int32_t rowId = -1;
    auto ret = mediaLibraryRdb_->Update(rowId, value, predicates);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK && rowId > 0, E_ERR,
        "Update table failed, row_id is : %{public}d", rowId);
    MEDIA_INFO_LOG("FolderParser: end update PhotoAlbum.");
    return E_OK;
}

int32_t FolderParser::DeleteAlbum(const NativeRdb::AbsRdbPredicates &predicates)
{
    MediaLibraryTracer tracer;
    tracer.Start("FolderParser::DeleteAlbum");
    MEDIA_INFO_LOG("FolderParser: begin delete PhotoAlbum.");
    CHECK_AND_RETURN_RET_LOG(GetMediaLibraryRdb() != nullptr, E_ERR, "DeleteAlbum: rdb is null.");
    int32_t deletedRows = -1;
    AccurateRefresh::AlbumAccurateRefresh albumRefresh;
    int32_t ret = albumRefresh.Delete(deletedRows, predicates);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK && deletedRows > 0, E_ERR,
        "Delete table failed, deletedRows is : %{public}d", deletedRows);
    MEDIA_INFO_LOG("FolderParser: end delete PhotoAlbum.");
    return E_OK;
}


std::string FolderParser::ToString(const std::vector<NativeRdb::ValueObject> &bindArgs)
{
    std::string args;
    for (auto &arg : bindArgs) {
        std::string tempStr;
        arg.GetString(tempStr);
        args += tempStr + ", ";
    }
    return args;
}

void FolderParser::GetUniqueAlbumName(std::string &albumName)
{
    std::string uniqueAlbumName = albumName;
    int32_t sequence = 1;
    bool isUnique = CheckAlbumNameUnique(albumName);
    while (!isUnique && sequence < MAX_ALBUM_NAME_SEQUENCE) {
        uniqueAlbumName = albumName + (" " + std::to_string(sequence));
        MEDIA_INFO_LOG("check album sequence: %{public}d, albumName: %{public}s",
            sequence, FileScanUtils::GarbleFile(uniqueAlbumName).c_str());
        sequence++;
        isUnique = CheckAlbumNameUnique(uniqueAlbumName);
    }
    albumName = uniqueAlbumName;
}
// LCOV_EXCL_STOP
}  // namespace Media
}  // namespace OHOS