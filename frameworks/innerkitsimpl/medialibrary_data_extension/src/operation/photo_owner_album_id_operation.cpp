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
#define MLOG_TAG "PhotoOwnerAlbumIdOperation"

#include "photo_owner_album_id_operation.h"

#include <sstream>

#include "album_accurate_refresh.h"
#include "album_plugin_config.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "userfile_manager_types.h"
#include "result_set_utils.h"
#include "media_file_utils.h"
#include "medialibrary_tracer.h"
#include "refresh_business_name.h"
#include "medialibrary_data_manager_utils.h"
#include "medialibrary_unistore_manager.h"

namespace OHOS::Media {
using MediaData = PhotoOwnerAlbumIdOperation::MediaData;
std::string PhotoOwnerAlbumIdOperation::ToStringWithComma(const std::vector<NativeRdb::ValueObject> &bindArgs) const
{
    std::string tempStr;
    std::stringstream os;
    for (size_t i = 0; i < bindArgs.size(); ++i) {
        bindArgs[i].GetString(tempStr);
        os << tempStr;
        if (i != bindArgs.size() - 1) {
            os << ",";
        }
    }
    return os.str();
}

std::string PhotoOwnerAlbumIdOperation::ToStringWithComma(const std::vector<std::string> &fileIds) const
{
    std::stringstream os;
    for (size_t i = 0; i < fileIds.size(); ++i) {
        os << fileIds[i];
        if (i != fileIds.size() - 1) {
            os << ",";
        }
    }
    return os.str();
}

std::string PhotoOwnerAlbumIdOperation::ToStringWithCommaAndQuote(const std::vector<std::string> &fileIds) const
{
    std::stringstream os;
    for (size_t i = 0; i < fileIds.size(); ++i) {
        os << "'" << fileIds[i] << "'";
        if (i != fileIds.size() - 1) {
            os << ",";
        }
    }
    return os.str();
}

std::string PhotoOwnerAlbumIdOperation::ToString(const MediaData &albumInfo) const
{
    std::stringstream os;
    os << "{"
       << "albumId:" << albumInfo.albumId << ","
       << "albumType:" << albumInfo.albumType << ","
       << "albumSubType:" << albumInfo.albumSubType << ","
       << "albumName:" << albumInfo.albumName << ","
       << "bundleName:" << albumInfo.bundleName << ","
       << "lPath:" << albumInfo.lPath << ","
       << "priority:" << albumInfo.priority << ","
       << "fileId:" << albumInfo.fileId << "}";
    return os.str();
}

std::string PhotoOwnerAlbumIdOperation::FillParams(const std::string &sql, const std::vector<std::string> &bindArgs)
{
    std::stringstream os;
    std::string flag;
    const std::string leftBrace = "{";
    const std::string rightBrace = "}";
    std::string val;
    std::string result = sql;
    for (size_t i = 0; i < bindArgs.size(); i++) {
        flag = leftBrace + std::to_string(i) + rightBrace;
        val = bindArgs[i];
        size_t pos = result.find(flag);
        while (pos != std::string::npos) {
            os.str("");
            os << result.substr(0, pos) << bindArgs[i];
            os << (pos + flag.length() <= result.length() ? result.substr(pos + flag.length()) : "");
            result = os.str();
            os.str("");
            pos = result.find(flag);
        }
    }
    return result;
}

std::vector<std::string> PhotoOwnerAlbumIdOperation::GetFileIdsWithoutAlbum(
    const std::string &fileIdWithComma, bool &containsScreenVideo)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoOwnerAlbumIdOperation::GetFileIdsWithoutAlbum");
    std::vector<std::string> result;
    bool conn = this->rdbStore_ == nullptr || fileIdWithComma.empty();
    CHECK_AND_RETURN_RET_LOG(!conn, result, "Media_Operation: rdbStore_ is null or fileIdWithComma is empty.");

    std::string sql = this->FillParams(this->SQL_NO_ALBUM_FILE_IDS, {fileIdWithComma});
    auto resultSet = this->rdbStore_->QuerySql(sql);
    conn = resultSet == nullptr;
    CHECK_AND_RETURN_RET_LOG(!conn, result, "Media_Operation: QuerySql failed, sql: %{public}s.", sql.c_str());

    std::string fileId;
    std::string sourcePath;
    std::string lPath;
    int32_t mediaType;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        fileId = GetStringVal("file_id", resultSet);
        sourcePath = GetStringVal("source_path", resultSet);
        mediaType = GetInt32Val("media_type", resultSet);
        lPath = this->ParseSourcePathToLPath(sourcePath);
        containsScreenVideo = containsScreenVideo || (mediaType == MEDIA_TYPE_VIDEO && lPath == LPATH_SCREEN_SHOTS);
        result.emplace_back(fileId);
    }
    return result;
}

std::vector<std::string> PhotoOwnerAlbumIdOperation::GetFileIdsWithAlbum(
    const std::string &fileIdWithComma, bool &containsScreenVideo)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoOwnerAlbumIdOperation::GetFileIdsWithAlbum");
    std::vector<std::string> result;
    bool conn = this->rdbStore_ == nullptr || fileIdWithComma.empty();
    CHECK_AND_RETURN_RET_LOG(!conn, result, "Media_Operation: rdbStore_ is null or fileIdWithComma is empty.");

    std::string sql = this->FillParams(this->SQL_HAS_ALBUM_FILE_IDS, {fileIdWithComma});
    auto resultSet = this->rdbStore_->QuerySql(sql);
    conn = resultSet == nullptr;
    CHECK_AND_RETURN_RET_LOG(!conn, result, "Media_Operation: QuerySql failed, sql: %{public}s.", sql.c_str());

    std::string fileId;
    int32_t mediaType;
    std::string lPath;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        fileId = GetStringVal("file_id", resultSet);
        mediaType = GetInt32Val("media_type", resultSet);
        lPath = GetStringVal("lpath", resultSet);
        containsScreenVideo = containsScreenVideo || (mediaType == MEDIA_TYPE_VIDEO && lPath == LPATH_SCREEN_SHOTS);
        result.emplace_back(fileId);
    }
    return result;
}

int32_t PhotoOwnerAlbumIdOperation::FixPhotoRelation()
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoOwnerAlbumIdOperation::FixPhotoRelation");
    bool conn = this->fileIds_.empty();
    CHECK_AND_RETURN_RET_LOG(!conn, E_OK, "Media_Operation: fileIds_ is empty.");

    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    // Distinguish between files with and without albums
    std::string fileIdWithComma = this->ToStringWithComma(this->fileIds_);
    bool containsScreenVideo = false;
    std::vector<std::string> fileIdsWithoutAlbum = this->GetFileIdsWithoutAlbum(fileIdWithComma, containsScreenVideo);
    std::vector<std::string> fileIdsWithAlbum = this->GetFileIdsWithAlbum(fileIdWithComma, containsScreenVideo);
    MEDIA_INFO_LOG("Media_Operation: fileId size: %{public}zu, fileIdsWithoutAlbum size: %{public}zu, "
                   "fileIdsWithAlbum size: %{public}zu, containsScreenVideo: %{public}d.",
        this->fileIds_.size(),
        fileIdsWithoutAlbum.size(),
        fileIdsWithAlbum.size(),
        containsScreenVideo);
    conn = this->fileIds_.size() != fileIdsWithoutAlbum.size() + fileIdsWithAlbum.size();
    CHECK_AND_PRINT_LOG(!conn, "Media_Operation: fileIds size not match.");

    conn = fileIdsWithoutAlbum.empty() && fileIdsWithAlbum.empty();
    CHECK_AND_RETURN_RET_LOG(!conn, E_ERR, "Media_Operation: fileIdsWithoutAlbum and fileIdsWithAlbum is empty.");

    // Scenario 1, some files are in the album.
    int32_t err = this->FixPhotoRelationForHasAlbum(fileIdsWithAlbum);
    conn = err != E_OK;
    CHECK_AND_RETURN_RET_LOG(!conn, err, "Media_Operation: FixPhotoRelationForHasAlbum failed, err: %{public}d.", err);

    // Scenario 2, some files are not in the album.
    err = this->FixPhotoRelationForNoAlbum(fileIdsWithoutAlbum);
    conn = err != E_OK;
    CHECK_AND_RETURN_RET_LOG(!conn, err, "Media_Operation: FixPhotoRelationForNoAlbum failed, err: %{public}d.", err);

    // Scenario 3, some files are screen video.
    if (containsScreenVideo) {
        err = this->FixScreenVideoRelation();
        conn = err != E_OK;
        CHECK_AND_PRINT_LOG(!conn, "Media_Operation: FixScreenVideoRelation failed, err: %{public}d.", err);
    }
    int64_t costTime = MediaFileUtils::UTCTimeMilliSeconds() - startTime;
    MEDIA_INFO_LOG("Media_Operation: FixPhotoRelation cost time: %{public}s ms.", std::to_string(costTime).c_str());
    return E_OK;
}

int32_t PhotoOwnerAlbumIdOperation::FixPhotoRelationForHasAlbum(const std::vector<std::string> &fileIds)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoOwnerAlbumIdOperation::FixPhotoRelationForHasAlbum");
    bool conn = fileIds.empty();
    CHECK_AND_RETURN_RET_LOG(!conn, E_OK, "Media_Operation: fileIds is empty.");

    std::vector<std::string> ownerAlbumIds = this->GetOwnerAlbumIds(fileIds);
    conn = ownerAlbumIds.empty();
    CHECK_AND_RETURN_RET_LOG(!conn, E_OK, "Media_Operation: ownerAlbumIds is empty.");

    return this->ResetAlbumDirty(ownerAlbumIds);
}

int32_t PhotoOwnerAlbumIdOperation::FixPhotoRelationForNoAlbum(const std::vector<std::string> &fileIds)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoOwnerAlbumIdOperation::FixPhotoRelationForNoAlbum");
    bool conn = fileIds.empty();
    CHECK_AND_RETURN_RET_LOG(!conn, E_OK, "Media_Operation: fileIds is empty.");

    // query MediaData list, including lPath, sourcePath, fileId
    std::unordered_set<std::string> lPathSet;
    std::unordered_map<std::string, std::vector<MediaData>> photoTargetlPaths = this->GetPhotolPath(fileIds, lPathSet);
    conn = photoTargetlPaths.empty() || lPathSet.empty();
    CHECK_AND_RETURN_RET_LOG(!conn, E_OK, "Media_Operation: photolPaths or lPathSet is empty.");

    int32_t err = this->CreateAlbums(lPathSet);
    conn = err != E_OK;
    CHECK_AND_RETURN_RET_LOG(!conn, err, "Media_Operation: CreateAlbums failed, err: %{public}d.", err);

    // query albumInfo from db, using lPath, to get albumId
    std::unordered_map<std::string, MediaData> albumInfos = this->GetPhotoAlbums(lPathSet);
    conn = albumInfos.empty();
    CHECK_AND_RETURN_RET_LOG(!conn, E_ERR, "Media_Operation: albumInfos is empty.");

    // update photo owner_album_id
    return this->BatchUpdatePhotoOwnerAlbumId(photoTargetlPaths, albumInfos);
}

std::vector<std::string> PhotoOwnerAlbumIdOperation::GetOwnerAlbumIds(const std::vector<std::string> &fileIds)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoOwnerAlbumIdOperation::GetOwnerAlbumIds");
    std::vector<std::string> result;
    bool conn = this->rdbStore_ == nullptr || fileIds.empty();
    CHECK_AND_RETURN_RET_LOG(!conn, result, "Media_Operation: rdbStore_ is null or fileIds is empty.");

    std::string sql = this->FillParams(this->SQL_PHOTO_OWNER_ALBUM_ID_QUERY, {this->ToStringWithComma(fileIds)});
    auto resultSet = this->rdbStore_->QuerySql(sql);
    conn = resultSet == nullptr;
    CHECK_AND_RETURN_RET_LOG(!conn, result, "Media_Operation: QuerySql failed, sql: %{public}s.", sql.c_str());

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string albumId = GetStringVal("owner_album_id", resultSet);
        result.emplace_back(albumId);
    }
    return result;
}

int32_t PhotoOwnerAlbumIdOperation::ResetAlbumDirty(const std::vector<std::string> &ownerAlbumIds)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoOwnerAlbumIdOperation::ResetAlbumDirty");
    bool conn = this->rdbStore_ == nullptr || ownerAlbumIds.empty();
    CHECK_AND_RETURN_RET_LOG(!conn, E_OK, "Media_Operation: rdbStore_ is null or ownerAlbumIds is empty.");

    std::string sql = this->FillParams(this->SQL_PHOTO_ALBUM_DIRTY_UPDATE, {this->ToStringWithComma(ownerAlbumIds)});
    int32_t err = this->rdbStore_->ExecuteSql(sql);
    conn = err != NativeRdb::E_OK;
    CHECK_AND_RETURN_RET_LOG(
        !conn, err, "Media_Operation: album dirty failed, err: %{public}d, sql: %{public}s.", err, sql.c_str());

    return E_OK;
}

std::unordered_map<std::string, std::vector<MediaData>> PhotoOwnerAlbumIdOperation::GetPhotolPath(
    const std::vector<std::string> &fileIds, std::unordered_set<std::string> &lPathSet)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoOwnerAlbumIdOperation::GetPhotolPath");
    std::unordered_map<std::string, std::vector<MediaData>> result;
    bool conn = this->rdbStore_ == nullptr || fileIds.empty();
    CHECK_AND_RETURN_RET_LOG(!conn, result, "Media_Operation: rdbStore_ is null or fileIds is empty.");

    std::string sql = this->FillParams(this->SQL_PHOTOS_SOURCE_PATH_QUERY, {this->ToStringWithComma(fileIds)});
    auto resultSet = this->rdbStore_->QuerySql(sql);
    conn = resultSet == nullptr;
    CHECK_AND_RETURN_RET_LOG(!conn, result, "Media_Operation: QuerySql failed, sql: %{public}s.", sql.c_str());

    std::string sourcePath;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        MediaData mediaData;
        mediaData.fileId = GetStringVal("file_id", resultSet);
        sourcePath = GetStringVal("source_path", resultSet);
        mediaData.lPath = this->ParseSourcePathToLPath(sourcePath);
        lPathSet.insert(mediaData.lPath);
        if (result.count(mediaData.lPath) <= 0) {
            result[mediaData.lPath] = {};
        }
        result[mediaData.lPath].emplace_back(mediaData);
    }
    return result;
}

std::unordered_map<std::string, MediaData> PhotoOwnerAlbumIdOperation::GetPhotoAlbums(
    const std::unordered_set<std::string> &lPathSet)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoOwnerAlbumIdOperation::GetPhotoAlbums");
    std::unordered_map<std::string, MediaData> albumInfos;  // key: lPath, value: MediaData
    for (const auto &lPath : lPathSet) {
        if (albumInfos.count(lPath) > 0) {
            continue;
        }
        MediaData albumInfo = this->GetPhotoAlbum(lPath);
        if (albumInfo.albumId.empty() || albumInfo.lPath.empty()) {
            MEDIA_ERR_LOG(
                "Media_Operation: GetPhotoAlbum failed, albumInfo: %{public}s.", this->ToString(albumInfo).c_str());
            continue;
        }
        albumInfos[lPath] = albumInfo;
    }
    return albumInfos;
}

MediaData PhotoOwnerAlbumIdOperation::GetPhotoAlbum(const std::string &lPath)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoOwnerAlbumIdOperation::GetPhotoAlbum");
    MediaData result;
    bool conn = this->rdbStore_ == nullptr || lPath.empty();
    CHECK_AND_RETURN_RET_LOG(!conn, result, "Media_Operation: rdbStore_ is null or lPath is empty.");

    std::string sql = SQL_PHOTO_ALBUM_QUERY;
    std::vector<NativeRdb::ValueObject> params = {lPath};
    auto resultSet = this->rdbStore_->QuerySql(sql, params);
    conn = resultSet == nullptr || resultSet->GoToNextRow() != NativeRdb::E_OK;
    CHECK_AND_RETURN_RET_LOG(!conn, result, "Media_Operation: QuerySql failed, sql: %{public}s.", sql.c_str());

    result.albumId = GetStringVal("album_id", resultSet);
    result.lPath = GetStringVal("lpath", resultSet);
    return result;
}

/**
 * @brief Parse the sourcePath to lPath.
 * example, sourcePath=/storage/emulated/0/DCIM/Camera/IMG_20240829_072213.jpg, lPath=/DCIM/Camera
 * if the sourcePath can not be parsed, return /Pictures/其它.
 */
std::string PhotoOwnerAlbumIdOperation::ParseSourcePathToLPath(const std::string &sourcePath)
{
    size_t start_pos = sourcePath.find(GALLERT_ROOT_PATH);
    size_t end_pos = sourcePath.find_last_of("/");

    std::string result = "/Pictures/其它";
    if (start_pos != std::string::npos && end_pos != std::string::npos) {
        start_pos += GALLERT_ROOT_PATH.length();
        result = sourcePath.substr(start_pos, end_pos - start_pos);
        start_pos = result.find_first_of("/");
        if (start_pos != std::string::npos) {
            result = result.substr(start_pos);
        } else {
            result = FILE_SEPARATOR;
        }
    }
    result = result == AlbumPlugin::LPATH_HIDDEN_ALBUM ? AlbumPlugin::LPATH_RECOVER : result;
    return result;
}

/**
 * @brief Build MediaData from lPath.
 */
MediaData PhotoOwnerAlbumIdOperation::BuildAlbumInfoByLPath(const std::string &lPath)
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
    return this->BuildAlbumInfoByLPath(lPath, albumType, albumSubType);
}

int32_t PhotoOwnerAlbumIdOperation::CheckAndUpdateAlbumName(std::string &albumName, const int32_t albumType)
{
    CHECK_AND_RETURN_RET(albumType == static_cast<int32_t>(PhotoAlbumType::SOURCE), E_OK);
    CHECK_AND_RETURN_RET_LOG(this->rdbStore_ != nullptr, E_ERR, "Media_Operation: rdbStore_ is null.");
    bool isUnique = false;
    std::string val = albumName;
    int32_t sequence = 1;
    const int32_t MAX_ALBUM_NAME_SEQUENCE = 1000;
    while (!isUnique && sequence < MAX_ALBUM_NAME_SEQUENCE) {
        std::vector<NativeRdb::ValueObject> bindArgs = {val};
        std::string sql = this->SQL_QUERY_ALBUM_NAME_UNIQUE;
        auto resultSet = this->rdbStore_->QuerySql(sql, bindArgs);
        if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            val = albumName + " " + std::to_string(sequence);
            sequence++;
            CHECK_AND_RETURN_RET_LOG(sequence < MAX_ALBUM_NAME_SEQUENCE, E_ERR, "sequence exceed the limit");
        } else {
            albumName = val;
            isUnique = true;
            MEDIA_INFO_LOG("albumName is %{private}s", albumName.c_str());
        }
    }
    return E_OK;
}

/**
 * @brief Build MediaData from lPath.
 */
MediaData PhotoOwnerAlbumIdOperation::BuildAlbumInfoByLPath(
    const std::string &lPath, const int32_t albumType, const int32_t albumSubType)
{
    MediaData albumInfo;
    // find albumName from lPath
    std::string albumName = "其它";
    std::string albumlPath = lPath;
    int32_t albumTypeTmp = albumType;
    int32_t albumSubTypeTmp = albumSubType;
    size_t fileIndex = albumlPath.find_last_of(FILE_SEPARATOR);
    if (fileIndex != string::npos) {
        albumName = albumlPath.substr(fileIndex + 1);
        int32_t ret = CheckAndUpdateAlbumName(albumName, albumType);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, albumInfo, "fail to update albumName");
    } else {
        albumlPath = "/Pictures/其它";
        albumTypeTmp = static_cast<int32_t>(PhotoAlbumType::SOURCE);
        albumSubTypeTmp = static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC);
    }
    albumInfo.albumName = albumName;
    albumInfo.lPath = albumlPath;
    albumInfo.albumType = albumTypeTmp;
    albumInfo.albumSubType = albumSubTypeTmp;
    albumInfo.priority = 1;
    return albumInfo;
}

int32_t PhotoOwnerAlbumIdOperation::CreateAlbums(const std::unordered_set<std::string> &lPathSet)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoOwnerAlbumIdOperation::CreateAlbums");
    MediaData albumInfo;
    int32_t err = E_OK;
    bool conn;
    // create album for each node.key (lPath)
    for (const auto &lPath : lPathSet) {
        albumInfo = this->BuildAlbumInfoByLPath(lPath);
        err = this->CreateAlbum(albumInfo);
        conn = err != E_OK;
        CHECK_AND_RETURN_RET_LOG(!conn,
            err,
            "Media_Operation: CreateAlbum failed, err: %{public}d, albumInfo: %{public}s.",
            err,
            this->ToString(albumInfo).c_str());
    }
    return E_OK;
}

int32_t PhotoOwnerAlbumIdOperation::CreateAlbum(const MediaData &albumInfo)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoOwnerAlbumIdOperation::CreateAlbum");
    AccurateRefresh::AlbumAccurateRefresh albumRefresh(AccurateRefresh::CREATE_PHOTO_TABLE_BUSSINESS_NAME);

    std::string sql = this->SQL_PHOTO_ALBUM_INSERT;
    std::vector<NativeRdb::ValueObject> params = {albumInfo.albumType,
        albumInfo.albumSubType,
        albumInfo.albumName,
        albumInfo.bundleName,
        albumInfo.lPath,
        albumInfo.priority};
    int32_t err = albumRefresh.ExecuteSql(sql, params, AccurateRefresh::RdbOperation::RDB_OPERATION_ADD);
    bool conn = err != AccurateRefresh::ACCURATE_REFRESH_RET_OK;
    CHECK_AND_RETURN_RET_LOG(!conn,
        err,
        "Media_Operation: insert photo album failed, err: %{public}d, params: %{public}s.",
        err,
        this->ToStringWithComma(params).c_str());
    albumRefresh.Notify();
    return E_OK;
}

std::vector<std::string> PhotoOwnerAlbumIdOperation::GetFileIds(const std::vector<MediaData> &fileIds)
{
    std::vector<std::string> result;
    for (const auto &fileId : fileIds) {
        result.emplace_back(fileId.fileId);
    }
    return result;
}

int32_t PhotoOwnerAlbumIdOperation::BatchUpdatePhotoOwnerAlbumId(
    const std::unordered_map<std::string, std::vector<MediaData>> &photoTargetlPaths,
    const std::unordered_map<std::string, MediaData> &albumInfos)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoOwnerAlbumIdOperation::BatchUpdatePhotoOwnerAlbumId");
    std::vector<std::string> fileIdsForUpdate;
    std::string albumId;
    int32_t err;
    bool conn;
    for (const auto &iter : photoTargetlPaths) {  // key: lPath, value: MediaData list
        if (albumInfos.count(iter.first) <= 0) {
            continue;
        }
        albumId = albumInfos.at(iter.first).albumId;
        fileIdsForUpdate = this->GetFileIds(iter.second);
        err = this->UpdatePhotoOwnerAlbumId(fileIdsForUpdate, albumId);
        conn = err != E_OK;
        // continue to next lPath
        CHECK_AND_PRINT_LOG(!conn, "Media_Operation: BatchUpdatePhotoOwnerAlbumId failed, err: %{public}d.", err);
    }
    return E_OK;
}

int32_t PhotoOwnerAlbumIdOperation::UpdatePhotoOwnerAlbumId(
    const std::vector<std::string> &fileIds, const std::string &ownerAlbumId)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoOwnerAlbumIdOperation::UpdatePhotoOwnerAlbumId");
    bool conn = this->rdbStore_ == nullptr || fileIds.empty() || ownerAlbumId.empty();
    CHECK_AND_RETURN_RET_LOG(!conn,
        E_OK,
        "Media_Operation: rdbStore_ is null or fileIds: %{public}zu or ownerAlbumId: %{public}s is empty.",
        fileIds.size(),
        ownerAlbumId.c_str());

    std::string sql = this->FillParams(this->SQL_PHOTOS_OWNER_ALBUM_ID_UPDATE, {this->ToStringWithComma(fileIds)});
    std::vector<NativeRdb::ValueObject> params = {ownerAlbumId};
    int32_t err = this->rdbStore_->ExecuteSql(sql, params);
    conn = err != NativeRdb::E_OK;
    CHECK_AND_RETURN_RET_LOG(!conn,
        err,
        "Media_Operation: update photo owner album id failed, err: %{public}d, params: %{public}s.",
        err,
        this->ToStringWithComma(params).c_str());

    return E_OK;
}

std::vector<std::string> PhotoOwnerAlbumIdOperation::GetScreenVideoFileIds()
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoOwnerAlbumIdOperation::GetScreenVideoFileIds");
    std::vector<std::string> result;
    bool conn = this->rdbStore_ == nullptr;
    CHECK_AND_RETURN_RET_LOG(!conn, result, "Media_Operation: rdbStore_ is null.");

    std::string sql = SQL_PHOTOS_SCREEN_VIDEO_QUERY;
    auto resultSet = this->rdbStore_->QuerySql(sql);
    conn = resultSet == nullptr;
    CHECK_AND_RETURN_RET(!conn, result);

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string fileId = GetStringVal("file_id", resultSet);
        result.emplace_back(fileId);
    }
    return result;
}

int32_t PhotoOwnerAlbumIdOperation::FixScreenVideoRelation()
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoOwnerAlbumIdOperation::FixScreenVideoRelation");
    const std::vector<std::string> fileIds = this->GetScreenVideoFileIds();
    bool conn = fileIds.empty();
    CHECK_AND_RETURN_RET(!conn, E_OK);

    MEDIA_INFO_LOG("Media_Operation: Need to fix screen video relation. size: %{public}zu.", fileIds.size());
    int32_t err = this->CreateAlbums({LPATH_SCREEN_RECORDS});
    conn = err != E_OK;
    CHECK_AND_RETURN_RET_LOG(!conn, err, "Media_Operation: CreateAlbums failed, err: %{public}d.", err);

    MediaData albumInfo = this->GetPhotoAlbum(LPATH_SCREEN_RECORDS);
    conn = albumInfo.albumId.empty();
    CHECK_AND_RETURN_RET_LOG(!conn, E_ERR, "Media_Operation: albumInfo is empty.");

    return this->UpdatePhotoOwnerAlbumId(fileIds, albumInfo.albumId);
}

int32_t PhotoOwnerAlbumIdOperation::GetPhotoAlbumId(const std::string &lPath)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoOwnerAlbumIdOperation::GetPhotoAlbumId");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    bool conn = rdbStore == nullptr || lPath.empty();
    CHECK_AND_RETURN_RET_LOG(!conn, E_ERR, "Media_Operation: rdbStore is null or lPath is empty.");

    std::string sql = SQL_PHOTO_ALBUM_QUERY;
    std::vector<NativeRdb::ValueObject> params = {lPath};
    auto resultSet = rdbStore->QuerySql(sql, params);
    conn = resultSet == nullptr || resultSet->GoToNextRow() != NativeRdb::E_OK;
    CHECK_AND_RETURN_RET_LOG(!conn, E_ERR, "Media_Operation: QuerySql failed, sql: %{public}s.", sql.c_str());
    return GetInt32Val("album_id", resultSet);
}

int32_t PhotoOwnerAlbumIdOperation::BuildAlbumBySourcePath(const std::string &sourcePath)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoOwnerAlbumIdOperation::BuildAlbumBySourcePath");
    CHECK_AND_RETURN_RET_LOG(!sourcePath.empty(), E_ERR, "Media_Operation: sourcePath is empty.");
    std::string lPath = this->ParseSourcePathToLPath(sourcePath);

    std::unordered_set<std::string> lPathSet = { lPath };
    int32_t err = this->CreateAlbums(lPathSet);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, E_ERR, "Media_Operation: CreateAlbums failed, err: %{public}d.", err);
    int32_t albumId = this->GetPhotoAlbumId(lPath);
    CHECK_AND_RETURN_RET_LOG(albumId > 0, E_ERR, "Media_Operation: failed to get albumInfo.");
    MEDIA_INFO_LOG("Media_Operation: success to build and get album, albumId: %{public}d", albumId);
    return albumId;
}
}  // namespace OHOS::Media