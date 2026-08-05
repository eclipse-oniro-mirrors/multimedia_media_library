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
#define MLOG_TAG "CustomRestoreUtils"
 
#include "custom_restore_utils.h"
 
#include <cerrno>
#include <sys/stat.h>
 
#include "custom_restore_types.h"
#include "custom_restore_info.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_time_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_unistore_manager.h"
#include "metadata.h"
#include "metadata_extractor.h"
#include "photo_album_column.h"
#include "photo_file_utils.h"
#include "result_set_utils.h"
#include "scanner_utils.h"
#include "values_bucket.h"
 
namespace OHOS {
namespace Media {
using namespace std;
 
bool CustomRestoreUtils::IsDuplication(const CustomRestoreInfo &info, const RestoreFileInfo &fileInfo)
{
    bool cond = (!info.GetIsDeduplication() || info.GetAlbumId() == 0);
    CHECK_AND_RETURN_RET(!cond, false);
    if (info.GetHasPhotoCache()) {
        return IsDuplicateInCache(info, fileInfo);
    }
 
    const string querySql =
        "SELECT COUNT(1) as count FROM " + PhotoColumn::PHOTOS_TABLE + " WHERE " + PhotoColumn::PHOTO_OWNER_ALBUM_ID +
        "=" + to_string(info.GetAlbumId()) + " AND " + MediaColumn::MEDIA_NAME + "='" + fileInfo.fileName +
        "' AND " + MediaColumn::MEDIA_SIZE + "=" + to_string(fileInfo.size) + " AND " + MediaColumn::MEDIA_TYPE + "=" +
        to_string(fileInfo.mediaType) + " AND " + PhotoColumn::PHOTO_ORIENTATION + "=" +
        to_string(fileInfo.orientation) + ";";
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "IsDuplication: get rdb store fail!");
 
    auto resultSet = rdbStore->QuerySql(querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "IsDuplication: query PhotoAlbum failed!");
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("IsDuplication first row empty.");
        resultSet->Close();
        return false;
    }
    int32_t count = GetInt32Val("count", resultSet);
    resultSet->Close();
    return count > 0;
}
 
bool CustomRestoreUtils::IsDuplicateInCache(const CustomRestoreInfo &info, const RestoreFileInfo &fileInfo)
{
    string photoId = fileInfo.fileName + "_" + to_string(fileInfo.size) + "_" + to_string(fileInfo.mediaType) +
                     "_" + to_string(fileInfo.orientation);
    return info.GetPhotoCache().count(photoId) > 0;
}
 
unordered_set<string> CustomRestoreUtils::BatchQueryDuplicates(const CustomRestoreInfo &info,
    const vector<RestoreFileInfo> &fileInfos)
{
    unordered_set<string> dupKeys;
    bool cond = (!info.GetIsDeduplication() || info.GetAlbumId() == 0);
    if (cond) {
        return dupKeys;
    }
 
    if (info.GetHasPhotoCache()) {
        for (const auto &fileInfo : fileInfos) {
            if (IsDuplicateInCache(info, fileInfo)) {
                string key = fileInfo.fileName + "_" + to_string(fileInfo.size) + "_" +
                             to_string(fileInfo.mediaType) + "_" + to_string(fileInfo.orientation);
                dupKeys.insert(key);
            }
        }
        return dupKeys;
    }
 
    // Batch DB query: build parameterized SQL with OR clauses
    if (fileInfos.empty()) {
        return dupKeys;
    }
    string sql = "SELECT " + MediaColumn::MEDIA_NAME + ", " + MediaColumn::MEDIA_SIZE + ", " +
                 MediaColumn::MEDIA_TYPE + ", " + PhotoColumn::PHOTO_ORIENTATION +
                 " FROM " + PhotoColumn::PHOTOS_TABLE +
                 " WHERE " + PhotoColumn::PHOTO_OWNER_ALBUM_ID + "=?";
    vector<NativeRdb::ValueObject> params;
    params.emplace_back(info.GetAlbumId());
 
    for (size_t i = 0; i < fileInfos.size(); i++) {
        if (i == 0) {
            sql += " AND (";
        } else {
            sql += " OR ";
        }
        sql += "(" + MediaColumn::MEDIA_NAME + "=? AND " + MediaColumn::MEDIA_SIZE + "=? AND " +
               MediaColumn::MEDIA_TYPE + "=? AND " + PhotoColumn::PHOTO_ORIENTATION + "=?)";
        params.emplace_back(fileInfos[i].fileName);
        params.emplace_back(static_cast<int64_t>(fileInfos[i].size));
        params.emplace_back(fileInfos[i].mediaType);
        params.emplace_back(fileInfos[i].orientation);
    }
    sql += ")";
 
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("BatchQueryDuplicates: get rdb store fail!");
        return dupKeys;
    }
 
    auto resultSet = rdbStore->QuerySql(sql, params);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("BatchQueryDuplicates: query failed!");
        return dupKeys;
    }
 
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        string name = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
        int64_t size = GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet);
        int32_t type = GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet);
        int32_t orient = GetInt32Val(PhotoColumn::PHOTO_ORIENTATION, resultSet);
        string key = name + "_" + to_string(size) + "_" + to_string(type) + "_" + to_string(orient);
        dupKeys.insert(key);
    }
    resultSet->Close();
    return dupKeys;
}
 
int32_t CustomRestoreUtils::FillMetadata(const unordered_map<string, TimeInfo> &timeInfoMap,
    const RestoreFileInfo &fileInfo, std::unique_ptr<Metadata> &data)
{
    data->SetFilePath(fileInfo.originFilePath);
    data->SetFileName(fileInfo.fileName);
    data->SetFileMediaType(fileInfo.mediaType);
    bool isMovingPhoto = false;
    if (fileInfo.isLivePhoto) {
        data->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
        isMovingPhoto = true;
    }
    if (timeInfoMap.find(fileInfo.fileName) != timeInfoMap.end()) {
        auto timeInfo = timeInfoMap.at(fileInfo.fileName);
        data->SetFileDateAdded(timeInfo.dateAdded);
        data->SetDateTaken(timeInfo.dateTaken);
        data->SetDetailTime(timeInfo.detailTime);
    }
    int32_t err = GetFileMetadata(data);
    if (err != E_OK) {
        MEDIA_ERR_LOG("failed to get file metadata");
        return err;
    }
    return MetadataExtractor::Extract(data, isMovingPhoto, Scene::AV_META_SCENE_CLONE);
}
 
int32_t CustomRestoreUtils::GetFileMetadata(std::unique_ptr<Metadata> &data)
{
    struct stat statInfo {};
    if (stat(data->GetFilePath().c_str(), &statInfo) != 0) {
        MEDIA_ERR_LOG("stat syscall err %{public}d", errno);
        return E_FAIL;
    }
    data->SetFileSize(statInfo.st_size);
    data->SetLocalAssetSize(statInfo.st_size);
    auto dateModified = static_cast<int64_t>(MediaFileUtils::Timespec2Millisecond(statInfo.st_mtim));
    if (dateModified == 0) {
        dateModified = MediaTimeUtils::UTCTimeMilliSeconds();
        MEDIA_WARN_LOG("Invalid dateModified from st_mtim, use current time instead: %{public}lld",
            static_cast<long long>(dateModified));
    }
    if (dateModified != 0 && data->GetFileDateModified() == 0) {
        data->SetFileDateModified(dateModified);
    }
    string extension = ScannerUtils::GetFileExtension(data->GetFileName());
    data->SetFileExtension(extension);
    return E_OK;
}
 
void CustomRestoreUtils::SetTimeInfo(
    const std::unique_ptr<Metadata> &data, NativeRdb::ValuesBucket &value)
{
    int64_t dateModified =
        PhotoFileUtils::NormalizeTimestamp(data->GetFileDateModified(), MediaTimeUtils::UTCTimeMilliSeconds());
    int64_t dateAdded = PhotoFileUtils::NormalizeTimestamp(data->GetFileDateAdded(), dateModified);
    int64_t dateTaken = PhotoFileUtils::NormalizeTimestamp(data->GetDateTaken(), min(dateAdded, dateModified));
 
    value.Put(MediaColumn::MEDIA_DATE_ADDED, dateAdded);
    value.Put(MediaColumn::MEDIA_DATE_MODIFIED, dateModified);
    value.Put(MediaColumn::MEDIA_DATE_TAKEN, dateTaken);
 
    std::string detailTime = data->GetDetailTime();
    const auto [normalizeDateTaken, normalizeDetailTime] =
        PhotoFileUtils::ExtractTimeInfo(detailTime, PhotoColumn::PHOTO_DETAIL_TIME_FORMAT);
    if (normalizeDateTaken < MIN_MILSEC_TIMESTAMP || normalizeDateTaken > MAX_MILSEC_TIMESTAMP ||
        abs(dateTaken - normalizeDateTaken) > MAX_TIMESTAMP_DIFF) {
        MEDIA_ERR_LOG("invalid detailTime: %{public}s, dateTaken: %{public}lld",
            detailTime.c_str(),
            static_cast<long long>(dateTaken));
        detailTime = MediaTimeUtils::StrCreateTimeByMilliseconds(PhotoColumn::PHOTO_DETAIL_TIME_FORMAT, dateTaken);
    } else {
        detailTime = normalizeDetailTime;
    }
 
    value.Put(PhotoColumn::PHOTO_DETAIL_TIME, detailTime);
 
    const auto [dateYear, dateMonth, dateDay] = PhotoFileUtils::ExtractYearMonthDay(detailTime);
    value.Put(PhotoColumn::PHOTO_DATE_YEAR, dateYear);
    value.Put(PhotoColumn::PHOTO_DATE_MONTH, dateMonth);
    value.Put(PhotoColumn::PHOTO_DATE_DAY, dateDay);
}
 
} // namespace Media
} // namespace OHOS