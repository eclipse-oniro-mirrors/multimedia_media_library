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
#include <fcntl.h>
#include <sys/stat.h>
 
#include "custom_restore_types.h"
#include "custom_restore_info.h"
#include "directory_ex.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_time_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"
#include "metadata.h"
#include "metadata_extractor.h"
#include "moving_photo_file_utils.h"
#include "photo_album_column.h"
#include "photo_file_utils.h"
#include "result_set_utils.h"
#include "scanner_utils.h"
#include "unique_fd.h"
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
 
string CustomRestoreUtils::MakeDuplicateKey(const string &name, int64_t size,
    int32_t type, int32_t orient)
{
    return name + "_" + to_string(size) + "_" + to_string(type) + "_" + to_string(orient);
}

unordered_set<string> CustomRestoreUtils::BatchQueryDuplicates(const CustomRestoreInfo &info,
    const vector<RestoreFileInfo> &fileInfos)
{
    unordered_set<string> dupKeys;
    if (!info.GetIsDeduplication() || info.GetAlbumId() == 0) {
        return dupKeys;
    }
    if (info.GetHasPhotoCache()) {
        for (const auto &fileInfo : fileInfos) {
            if (IsDuplicateInCache(info, fileInfo)) {
                dupKeys.insert(MakeDuplicateKey(fileInfo.fileName, fileInfo.size,
                    fileInfo.mediaType, fileInfo.orientation));
            }
        }
        return dupKeys;
    }
    if (fileInfos.empty()) {
        return dupKeys;
    }

    string sql;
    vector<NativeRdb::ValueObject> params;
    BuildDuplicateQuerySql(info, fileInfos, sql, params);

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
    CollectDuplicateKeys(resultSet, dupKeys);
    resultSet->Close();
    return dupKeys;
}

void CustomRestoreUtils::BuildDuplicateQuerySql(const CustomRestoreInfo &info,
    const vector<RestoreFileInfo> &fileInfos, string &sql,
    vector<NativeRdb::ValueObject> &params)
{
    sql = "SELECT " + MediaColumn::MEDIA_NAME + ", " + MediaColumn::MEDIA_SIZE + ", " +
          MediaColumn::MEDIA_TYPE + ", " + PhotoColumn::PHOTO_ORIENTATION +
          " FROM " + PhotoColumn::PHOTOS_TABLE +
          " WHERE " + PhotoColumn::PHOTO_OWNER_ALBUM_ID + "=?";
    params.emplace_back(info.GetAlbumId());

    for (size_t i = 0; i < fileInfos.size(); i++) {
        sql += (i == 0 ? " AND (" : " OR ");
        sql += "(" + MediaColumn::MEDIA_NAME + "=? AND " + MediaColumn::MEDIA_SIZE +
               "=? AND " + MediaColumn::MEDIA_TYPE + "=? AND " +
               PhotoColumn::PHOTO_ORIENTATION + "=?)";
        params.emplace_back(fileInfos[i].fileName);
        params.emplace_back(static_cast<int64_t>(fileInfos[i].size));
        params.emplace_back(fileInfos[i].mediaType);
        params.emplace_back(fileInfos[i].orientation);
    }
    sql += ")";
}

void CustomRestoreUtils::CollectDuplicateKeys(const shared_ptr<NativeRdb::ResultSet> &resultSet,
    unordered_set<string> &dupKeys)
{
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        string name = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
        int64_t size = GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet);
        int32_t type = GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet);
        int32_t orient = GetInt32Val(PhotoColumn::PHOTO_ORIENTATION, resultSet);
        dupKeys.insert(MakeDuplicateKey(name, size, type, orient));
    }
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
    // parse moving photo cover position
    uint64_t coverPosition = 0;
    uint32_t version = 0;
    uint32_t frameIndex = 0;
    bool hasCinemagraphInfo = false;
    string extraDataPath = MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(fileInfo.originFilePath);
    string videoPath = MovingPhotoFileUtils::GetMovingPhotoVideoPath(fileInfo.originFilePath);
    string absExtraDataPath;
    if (!PathToRealPath(extraDataPath, absExtraDataPath)) {
        MEDIA_WARN_LOG("file is not real path: %{private}s, errno: %{public}d", extraDataPath.c_str(), errno);
    } else {
        UniqueFd extraDataFd(open(absExtraDataPath.c_str(), O_RDONLY));
        (void)MovingPhotoFileUtils::GetVersionAndFrameNum(extraDataFd.Get(), version, frameIndex, hasCinemagraphInfo);
        (void)MovingPhotoFileUtils::GetCoverPosition(videoPath, frameIndex, coverPosition);
    }
    data->SetCoverPosition(static_cast<int64_t>(coverPosition));
    if (version == LIVE_PHOTO_4D_VERSION) {
        data->SetLivePhoto4dStatus(static_cast<int32_t>(LivePhoto4dStatusType::TYPE_LIVEPHOTO_4D));
    }
    if (data->GetFileMediaType() == MEDIA_TYPE_IMAGE) {
        err = MetadataExtractor::ExtractImageMetadata(data);
    } else {
        err = MetadataExtractor::ExtractAVMetadata(data, Scene::AV_META_SCENE_CLONE);
    }
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "failed to extension data");
    return E_OK;
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