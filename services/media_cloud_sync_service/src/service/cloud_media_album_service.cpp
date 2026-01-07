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

#define MLOG_TAG "Media_Cloud_Service"

#include "cloud_media_album_service.h"

#include <string>
#include <vector>

#include "cloud_media_sync_utils.h"
#include "dataobs_mgr_changeinfo.h"
#include "media_column.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "on_fetch_records_vo.h"
#include "photos_dto.h"
#include "result_set_utils.h"
#include "media_gallery_sync_notify.h"
#include "cloud_media_sync_const.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_data_manager_utils.h"
#include "media_file_utils.h"

namespace OHOS::Media::CloudSync {
const std::unordered_map<std::string, std::vector<std::string>> screensMap = {
    {"/Pictures/Screenshots", {"截图", "com.huawei.hmos.screenshot"}},
    {"/Pictures/Screenrecords", {"屏幕录制", "com.huawei.hmos.screenrecorder"}},
};

static bool IsDoubleScreenshot(const std::string &lpath, const std::string &recordId)
{
    return lpath == DEFAULT_SCREENSHOT_LPATH_EN && recordId == DEFAULT_SCREENSHOT_CLOUDID;
}

static int32_t GetLocalMatchDirty(NativeRdb::ResultSet &resultSet)
{
    int32_t dirty = GetInt32Val(PhotoAlbumColumns::ALBUM_DIRTY, &resultSet);
    if (dirty < static_cast<int32_t>(DirtyType::TYPE_SYNCED)) {
        MEDIA_ERR_LOG("fail to get album dirty value");
        /* assume dirty so that no follow-up actions */
        return static_cast<int32_t>(Media::DirtyType::TYPE_MDIRTY);
    }
    return dirty;
}

int32_t CloudMediaAlbumService::ConvertToSingleScreenshots(PhotoAlbumDto &album, std::vector<PhotoAlbumDto> &records)
{
    for (const auto &it : screensMap) {
        MEDIA_DEBUG_LOG(
            "ConvertToSingleScreenshots Key : %{public}s, Value: %{public}s", it.first.c_str(), it.second[0].c_str());
        album.lPath = it.first;
        album.albumName = it.second[0];
        album.bundleName = it.second[1];
        records.emplace_back(album);
    }
    return E_OK;
}

int32_t CloudMediaAlbumService::OnFetchRecords(
    std::vector<PhotoAlbumDto> &albumDtoList, OnFetchRecordsAlbumRespBody &resp)
{
    std::vector<PhotoAlbumDto> oldRecords;
    std::vector<PhotoAlbumDto> lpathRecords;
    int32_t ret = E_OK;
    for (auto &album : albumDtoList) {
        ret = this->albumDao_.HandleLPathAndAlbumType(album);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_STOP, "OnFetchRecords HandleLPathAndAlbumType Error");
        if (IsDoubleScreenshot(album.lPath, album.cloudId)) {
            MEDIA_INFO_LOG("OnFetchRecords IsDoubleScreenshot");
            ConvertToSingleScreenshots(album, lpathRecords);
            continue;
        }
        if (!album.lPath.empty()) {
            MEDIA_DEBUG_LOG("OnFetchRecords add to new record");
            lpathRecords.emplace_back(album);
            continue;
        }
        MEDIA_INFO_LOG("OnFetchRecords add to old record");
        oldRecords.emplace_back(album);
    }
    ret = OnFetchOldRecords(oldRecords, resp);
    if (ret == E_STOP) {
        MEDIA_ERR_LOG("OnFetchRecords OnFetchOldRecords Error");
        return ret;
    }
    return OnFetchLPathRecords(lpathRecords, resp);
}

int32_t CloudMediaAlbumService::HandleFetchOldRecordNew(
    PhotoAlbumDto &record, ChangeType &changeType, OnFetchRecordsAlbumRespBody &resp)
{
    if (!record.isDelete) {
        /* insert */
        std::shared_ptr<AccurateRefresh::AlbumAccurateRefresh> albumRefresh =
            std::make_shared<AccurateRefresh::AlbumAccurateRefresh>();
        CHECK_AND_RETURN_RET_LOG(albumRefresh != nullptr, E_RDB_STORE_NULL,
            "Delete Cloud Album Failed to get albumRefresh.");
        int32_t ret = this->albumDao_.InsertCloudByCloudId(record, albumRefresh);
        if (ret != E_OK) {
            MEDIA_ERR_LOG(
                "HandleFetchOldRecord InsertCloudByCloudId error %{public}s, %{public}d", record.cloudId.c_str(), ret);
            resp.failedRecords.emplace_back(record.cloudId);
        } else {
            /* notify */
            ret = albumRefresh->Notify();
            CHECK_AND_RETURN_RET_LOG(ret == AccurateRefresh::ACCURATE_REFRESH_RET_OK, E_ERR,
                "fail to notify, ret = %{public}d", ret);
        }
        MEDIA_INFO_LOG("HandleFetchOldRecord insert %{public}s, %{public}d", record.cloudId.c_str(), ret);
        resp.stats[StatsIndex::NEW_RECORDS_COUNT]++;
        MEDIA_INFO_LOG("HandleFetchOldRecord insert stats %{public}d", resp.stats[StatsIndex::NEW_RECORDS_COUNT]);
        changeType = ChangeType::INSERT;
    }
    return E_OK;
}

int32_t CloudMediaAlbumService::HandleFetchOldRecord(
    PhotoAlbumDto &record, bool &bContinue, ChangeType &changeType, OnFetchRecordsAlbumRespBody &resp)
{
    int32_t ret = E_OK;
    auto [resultSet, rowCount] = this->albumDao_.QueryLocalMatchAlbum(record.cloudId);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("HandleFetchOldRecord QueryLocalMatchAlbum error");
        bContinue = true;
        return ret;
    }
    /* need to handle album cover uri */
    if (rowCount == 0) {
        this->HandleFetchOldRecordNew(record, changeType, resp);
    } else if (rowCount == 1) {
        resultSet->GoToNextRow();
        int32_t dirty = GetLocalMatchDirty(*resultSet);
        std::shared_ptr<AccurateRefresh::AlbumAccurateRefresh> albumRefresh =
            std::make_shared<AccurateRefresh::AlbumAccurateRefresh>();
        CHECK_AND_RETURN_RET_LOG(albumRefresh != nullptr, E_RDB_STORE_NULL,
            "Delete Cloud Album Failed to get albumRefresh.");
        if (dirty != static_cast<int32_t>(Media::DirtyType::TYPE_SYNCED)) {
            /* local dirty */
            MEDIA_INFO_LOG("HandleFetchOldRecord is dirty skip %{public}s", record.cloudId.c_str());
            bContinue = true;
            return ret;
        } else if (record.isDelete) {
            /* delete */
            ret = this->albumDao_.DeleteCloudAlbum(PhotoAlbumColumns::ALBUM_CLOUD_ID, record.cloudId, albumRefresh);
            MEDIA_INFO_LOG("HandleFetchOldRecorddelete %{public}s, %{public}d", record.cloudId.c_str(), ret);
            if (ret != E_OK) {
                MEDIA_ERR_LOG("HandleFetchOldRecord DeleteCloudAlbum error %{public}s", record.cloudId.c_str());
                resp.failedRecords.emplace_back(record.cloudId);
            }
            resp.stats[StatsIndex::DELETE_RECORDS_COUNT]++;
        } else {
            /* update */
            ret = this->albumDao_.UpdateCloudAlbum(record, PhotoAlbumColumns::ALBUM_CLOUD_ID,
                record.cloudId, albumRefresh);
            MEDIA_INFO_LOG("HandleFetchOldRecord update %{public}s, %{public}d", record.cloudId.c_str(), ret);
            changeType = ChangeType::UPDATE;
            if (ret != E_OK) {
                MEDIA_ERR_LOG("HandleFetchOldRecord UpdateCloudAlbum error %{public}s", record.cloudId.c_str());
                resp.failedRecords.emplace_back(record.cloudId);
            }
            resp.stats[StatsIndex::META_MODIFY_RECORDS_COUNT]++;
        }
        if (ret == E_OK && changeType != ChangeType::INVAILD) {
            int32_t notifyRet = albumRefresh->Notify();
            CHECK_AND_RETURN_RET_LOG(notifyRet == AccurateRefresh::ACCURATE_REFRESH_RET_OK, E_ERR,
                "fail to notify, ret = %{public}d", notifyRet);
        }
    } else {
        /* invalid cases */
        MEDIA_ERR_LOG("HandleFetchOldRecord recordId %s rowCount %{public}d", record.cloudId.c_str(), rowCount);
        ret = E_DATA;
    }
    return ret;
}

int32_t CloudMediaAlbumService::OnFetchOldRecords(
    std::vector<PhotoAlbumDto> &records, OnFetchRecordsAlbumRespBody &resp)
{
    MEDIA_INFO_LOG("OnFetchOldRecords enter %{public}zu", records.size());
    int32_t ret = E_OK;
    for (auto &record : records) {
        bool bContinue = false;
        ChangeType changeType = ChangeType::INVAILD;
        ret = HandleFetchOldRecord(record, bContinue, changeType, resp);
        if (bContinue) {
            MEDIA_INFO_LOG("OnFetchOldRecords HandleFetchOldRecord next record");
            continue;
        }

        /* check ret */
        if (ret != E_OK) {
            MEDIA_ERR_LOG("OnFetchOldRecords recordId %{public}s error %{public}d", record.cloudId.c_str(), ret);
            /* might need specific error type */
            if (ret == E_STOP || ret == E_RDB) {
                MediaGallerySyncNotify::GetInstance().FinalNotify();
                return E_STOP;
            }
            continue;
        } else {
            if (changeType != ChangeType::INVAILD) {
                MediaGallerySyncNotify::GetInstance().AddNotify(
                    PhotoAlbumColumns::ALBUM_GALLERY_CLOUD_URI_PREFIX, changeType, record.cloudId);
            }
        }
    }
    MediaGallerySyncNotify::GetInstance().FinalNotify();
    return E_OK;
}

int32_t CloudMediaAlbumService::OnFetchLPathRecords(
    std::vector<PhotoAlbumDto> &records, OnFetchRecordsAlbumRespBody &resp)
{
    MEDIA_INFO_LOG("OnFetchLPathRecords enter %{public}zu", records.size());
    int32_t ret = E_OK;
    for (auto &record : records) {
        std::string lpath = record.lPath;
        ChangeType changeType = ChangeType::INVAILD;
        this->albumDao_.GetPhotoAlbum(record.lPath, record.localAlbumInfo);
        ret = this->HandleLPathRecords(record, changeType, resp);
        /* check ret */
        if (ret != E_OK) {
            MEDIA_INFO_LOG("OnFetchLPathRecords recordId %{public}s error %{public}d", record.cloudId.c_str(), ret);
            /* might need specific error type */
            if (ret == E_STOP || ret == E_RDB) {
                MediaGallerySyncNotify::GetInstance().FinalNotify();
                return E_STOP;
            }
            continue;
        } else {
            if (changeType != ChangeType::INVAILD) {
                MediaGallerySyncNotify::GetInstance().AddNotify(
                    PhotoAlbumColumns::ALBUM_GALLERY_CLOUD_URI_PREFIX, changeType, record.cloudId);
            }
        }
    }
    MediaGallerySyncNotify::GetInstance().FinalNotify();
    return E_OK;
}

int32_t CloudMediaAlbumService::OnDentryFileInsert()
{
    return 0;
}

int32_t CloudMediaAlbumService::OnDeleteAlbums(std::vector<std::string> &failedAlbumIds)
{
    MEDIA_INFO_LOG("CloudMediaAlbumService::OnDeleteAlbums enter");
    return this->albumDao_.OnDeleteAlbums(failedAlbumIds);
}

int32_t CloudMediaAlbumService::GetCreatedRecords(
    int32_t size, const bool isCloudSpaceFull, std::vector<PhotoAlbumPo> &albumInfoList)
{
    MEDIA_INFO_LOG("CloudMediaAlbumService::GetCreatedRecords enter");
    int32_t tempSize = size;
    std::vector<PhotoAlbumPo> photoAlbumList;
    int32_t ret = this->albumDao_.GetCreatedRecords(tempSize, photoAlbumList);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "failed to get createdAlbum record");
    ret = this->GetCreatedRecordsWithCondition(size, isCloudSpaceFull, photoAlbumList, albumInfoList);
    MEDIA_INFO_LOG("GetCreatedRecords completed, "
                   "ret: %{public}d, size: %{public}d, isCloudSpaceFull: %{public}d, "
                   "db-size: %{public}zu, result-size: %{public}zu",
        ret,
        size,
        isCloudSpaceFull,
        photoAlbumList.size(),
        albumInfoList.size());
    return E_OK;
}

std::vector<PhotoAlbumPo> CloudMediaAlbumService::GetAlbumMetaModifiedRecords(int32_t size)
{
    std::vector<PhotoAlbumPo> photoAlbumList;
    int32_t ret = this->albumDao_.GetMetaModifiedAlbum(size, photoAlbumList);
    CHECK_AND_PRINT_LOG(ret == E_OK, "failed to get metaModifiedAlbum record");
    return photoAlbumList;
}

std::vector<PhotoAlbumPo> CloudMediaAlbumService::GetAlbumFileModifiedRecords(int32_t size)
{
    // nothing to do
    std::vector<PhotoAlbumPo> photoAlbumList;
    return photoAlbumList;
}
std::vector<PhotoAlbumPo> CloudMediaAlbumService::GetAlbumDeletedRecords(int32_t size)
{
    MEDIA_INFO_LOG("CloudMediaAlbumService::GetAlbumDeletedRecords enter %{public}d", size);
    std::vector<PhotoAlbumPo> photoAlbumList;
    int32_t ret = this->albumDao_.GetDeletedRecordsAlbum(size, photoAlbumList);
    CHECK_AND_PRINT_LOG(ret == E_OK, "failed to get metaModifiedAlbum record");
    return photoAlbumList;
}
std::vector<PhotoAlbumPo> CloudMediaAlbumService::GetAlbumCopyRecords(int32_t size)
{
    // nothing to do
    std::vector<PhotoAlbumPo> photoAlbumList;
    return photoAlbumList;
}

int32_t CloudMediaAlbumService::HandleCloudAlbumNotFound(const PhotoAlbumDto &album)
{
    return albumDao_.HandleNotExistAlbumRecord(album);
}

int32_t CloudMediaAlbumService::HandleDetailcode(ErrorDetailCode &errorCode)
{
    /* Only one record failed, not stop sync */
    return E_UNKNOWN;
}

int32_t CloudMediaAlbumService::OnRecordFailedErrorDetails(const PhotoAlbumDto &album)
{
    ErrorType errorType = album.errorType;
    if (album.errorDetails.size() == 0 && errorType != ErrorType::TYPE_NOT_NEED_RETRY) {
        MEDIA_ERR_LOG("errorDetails is empty and errorType is invalid, errorType:%{public}d", errorType);
        return E_INVAL_ARG;
    } else if (album.errorDetails.size() != 0) {
        auto errorDetailcode = static_cast<ErrorDetailCode>(album.errorDetails[0].detailCode);
        if (errorDetailcode == ErrorDetailCode::SPACE_FULL) {
            /* Stop sync */
            return E_CLOUD_STORAGE_FULL;
        }
        if (errorDetailcode == ErrorDetailCode::BUSINESS_MODEL_CHANGE_DATA_UPLOAD_FORBIDDEN) {
            MEDIA_ERR_LOG("Business Mode Change, Upload Fail");
            /* Stop sync */
            return E_BUSINESS_MODE_CHANGED;
        }
        if (errorDetailcode == ErrorDetailCode::SAME_FILENAME_NOT_ALLOWED) {
            MEDIA_ERR_LOG("Business Mode Change, Same Name Upload Fail");
        }
        if (errorDetailcode == ErrorDetailCode::CONTENT_NOT_FIND) {
            MEDIA_ERR_LOG("Business Mode Change, No Content Upload Fail");
        }
        if (errorType != ErrorType::TYPE_NOT_NEED_RETRY) {
            MEDIA_ERR_LOG("unknown error code record failed, errorDetailcode = %{public}d", errorDetailcode);
            return HandleDetailcode(errorDetailcode);
        }
        MEDIA_ERR_LOG("errorDetailcode = %{public}d, errorType = %{public}d, no need retry",
            errorDetailcode,
            static_cast<int32_t>(errorType));
        return E_STOP;
    } else {
        MEDIA_ERR_LOG("errorType = %{public}d, no need retry", static_cast<int32_t>(errorType));
        return E_STOP;
    }
    return E_UNKNOWN;
}

int32_t CloudMediaAlbumService::OnRecordFailed(const PhotoAlbumDto &album)
{
    int32_t serverErrorCode = album.serverErrorCode;
    if ((static_cast<ServerErrorCode>(serverErrorCode) == ServerErrorCode::NETWORK_ERROR)) {
        MEDIA_ERR_LOG("Network Error or Response Time Out");
        return E_SYNC_FAILED_NETWORK_NOT_AVAILABLE;
    } else if ((static_cast<ServerErrorCode>(serverErrorCode) == ServerErrorCode::UID_EMPTY) ||
               (static_cast<ServerErrorCode>(serverErrorCode) == ServerErrorCode::SWITCH_OFF)) {
        MEDIA_ERR_LOG("switch off or uid empty");
        return E_STOP;
    } else if (static_cast<ServerErrorCode>(serverErrorCode) == ServerErrorCode::INVALID_LOCK_PARAM) {
        MEDIA_ERR_LOG("Invalid lock param ");
        return E_STOP;
    } else if (static_cast<ServerErrorCode>(serverErrorCode) == ServerErrorCode::RESPONSE_TIME_OUT) {
        MEDIA_ERR_LOG("on record failed response time out");
    } else if (static_cast<ServerErrorCode>(serverErrorCode) == ServerErrorCode::RESOURCE_INVALID) {
        MEDIA_ERR_LOG("resource invalid");
        return E_DATA;
    } else if (static_cast<ServerErrorCode>(serverErrorCode) == ServerErrorCode::RENEW_RESOURCE) {
        MEDIA_ERR_LOG("renew resource");
        return E_DATA;
    }
    if ((static_cast<ServerErrorCode>(serverErrorCode) == ServerErrorCode::ALBUM_NOT_EXIST)) {
        MEDIA_ERR_LOG("ALBUM NOT EXIST AlbumService");
        return HandleCloudAlbumNotFound(album);
    }
    return OnRecordFailedErrorDetails(album);
}

int32_t CloudMediaAlbumService::OnCreateRecords(std::vector<PhotoAlbumDto> &albumDtoList, int32_t &failedSize)
{
    MEDIA_INFO_LOG("enter CloudMediaAlbumService::OnCreateRecords %{public}zu", albumDtoList.size());
    if (albumDtoList.size() <= 0) {
        return E_OK;
    }
    int32_t ret = E_OK;
    for (auto &album : albumDtoList) {
        MEDIA_DEBUG_LOG("OnCreateRecords album: %{public}s", album.ToString().c_str());
        int32_t err;
        if (album.isSuccess) {
            err = this->albumDao_.OnCreateRecord(album);
        } else {
            err = OnRecordFailed(album);
            failedSize++;
        }
        if (err != E_OK) {
            this->albumDao_.InsertAlbumCreateFailedRecord(album.cloudId);
            if (album.isSuccess) {
                failedSize++;
            }
            MEDIA_ERR_LOG("OnCreateRecords create record fail: cloudId %{public}s, err %{public}d",
                album.cloudId.c_str(), err);
        }
        if (err == E_SYNC_STOP || err == E_SYNC_FAILED_NETWORK_NOT_AVAILABLE || err == E_CLOUD_STORAGE_FULL ||
            err == E_STOP || err == E_BUSINESS_MODE_CHANGED) {
            ret = err;
        }
    }
    return ret;
}

int32_t CloudMediaAlbumService::OnMdirtyRecords(std::vector<PhotoAlbumDto> &albumDtoList, int32_t &failedSize)
{
    MEDIA_INFO_LOG("enter CloudMediaAlbumService::OnMdirtyRecords %{public}zu", albumDtoList.size());
    int32_t ret = E_OK;
    for (auto album : albumDtoList) {
        MEDIA_DEBUG_LOG("OnCreateRecords album: %{public}s", album.ToString().c_str());
        int32_t err;
        if (album.isSuccess) {
            err = this->albumDao_.OnMdirtyAlbumRecords(album.cloudId);
        } else {
            err = OnRecordFailed(album);
            failedSize++;
        }
        if (err != E_OK) {
            this->albumDao_.InsertAlbumModifyFailedRecord(album.cloudId);
            if (album.isSuccess) {
                failedSize++;
            }
            MEDIA_ERR_LOG("OnMdirtyRecords create record fail: cloudId %{public}s, err %{public}d",
                album.cloudId.c_str(), err);
        }
        if (err == E_SYNC_STOP || err == E_SYNC_FAILED_NETWORK_NOT_AVAILABLE || err == E_CLOUD_STORAGE_FULL ||
            err == E_STOP || err == E_BUSINESS_MODE_CHANGED) {
            ret = err;
        }
    }
    return ret;
}

int32_t CloudMediaAlbumService::OnFdirtyRecords()
{
    return 0;
}
int32_t CloudMediaAlbumService::OnDeleteRecords(std::vector<PhotoAlbumDto> &albumDtoList, int32_t &failSize)
{
    MEDIA_INFO_LOG("enter CloudMediaAlbumService::OnDeleteRecords %{public}zu", albumDtoList.size());
    int32_t ret = E_OK;
    for (auto album : albumDtoList) {
        if (album.isSuccess) {
            ret = this->albumDao_.OnDeleteAlbumRecords(album.cloudId);
        } else {
            this->albumDao_.InsertAlbumModifyFailedRecord(album.cloudId);
            failSize++;
        }
        if (ret != E_OK) {
            failSize++;
        }
    }
    return ret;
}
int32_t CloudMediaAlbumService::OnCopyRecords()
{
    return 0;
}

int32_t CloudMediaAlbumService::OnStartSync()
{
    return this->albumDao_.ClearAlbumFailedRecords();
}

int32_t CloudMediaAlbumService::OnCompleteSync()
{
    CheckAlbumManualCover();
    return 0;
}

int32_t CloudMediaAlbumService::OnCompletePull(const MediaOperateResult &optRet)
{
    CHECK_AND_RETURN_RET_LOG(optRet.errorCode == 0, E_OK, "album OnCompletePull errCode:%{public}d", optRet.errorCode);
    std::vector<std::string> failedAlbumIds;
    int32_t ret = OnDeleteAlbums(failedAlbumIds);
    return ret;
}

int32_t CloudMediaAlbumService::OnCompletePush()
{
    int32_t ret = this->albumDao_.ClearAlbumFailedRecords();
    ret = this->albumDao_.ReportAbnormalLocalRecords();
    return ret;
}

int32_t CloudMediaAlbumService::OnCompleteCheck()
{
    MEDIA_INFO_LOG("enter CloudMediaAlbumService::OnCompleteCheck");
    return 0;
}

void PrepareForNextCloud(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    NativeRdb::ValuesBucket &values, int32_t albumId)
{
    NativeRdb::RdbPredicates predicates(PhotoAlbumColumns::TABLE);

    values.PutInt(PhotoAlbumColumns::COVER_URI_SOURCE, static_cast<int32_t>(CoverUriSource::MANUAL_CLOUD_COVER));
    values.PutInt(PhotoAlbumColumns::ALBUM_DIRTY, static_cast<int32_t>(DirtyType::TYPE_MDIRTY));

    string updateCondition = PhotoAlbumColumns::ALBUM_ID + "=" + to_string(albumId);
    predicates.SetWhereClause(updateCondition);
    int32_t changeRows = -1;
    int32_t ret = rdbStore->Update(changeRows, values, predicates);
    CHECK_AND_RETURN_LOG((ret == E_OK && changeRows > 0),
        "Failed to UpdateDirtyForCloudClone, ret:%{public}d, updateRow:%{public}d", ret, changeRows);
    MEDIA_DEBUG_LOG("PrepareForNextCloud: albumId:%{public}d", albumId);
    CHECK_AND_PRINT_LOG(changeRows >= 0, "PrepareForNextCloud failed:%{public}d", changeRows);
}

bool IsCoverUriExistCloudId(string &coverUri, string &coverCloudId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "QueryCoverCloudId Failed to get rdbStore.");
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    string fileId = MediaLibraryDataManagerUtils::GetFileIdFromPhotoUri(coverUri);
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    std::vector<std::string> queryColumns = { PhotoColumn::PHOTO_CLOUD_ID };
    auto resultSet = rdbStore->Query(predicates, queryColumns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "QueryCloudId get nullptr created result.");
    string cloudId;
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        cloudId = get<string>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_CLOUD_ID,
            resultSet, TYPE_STRING));
    }
    coverCloudId += cloudId;
    return cloudId != "";
}

void CloudMediaAlbumService::HandleWaitPullCover(shared_ptr<NativeRdb::ResultSet> &resultSet,
    const shared_ptr<MediaLibraryRdbStore> rdbStore, NativeRdb::ValuesBucket &values, int32_t albumId)
{
    string coverCloudId = get<string>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::COVER_CLOUD_ID,
        resultSet, TYPE_STRING));
    string coverUri;
    bool isCloudIdExist = GetCoverUriFromCoverCloudId(coverCloudId, coverUri);
    if (isCloudIdExist) {
        MEDIA_DEBUG_LOG("CheckAlbumManualCover isCloudIdExist");
        string lPath = get<string>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_LPATH,
            resultSet, TYPE_STRING));
        if (this->albumDao_.ReplaceCoverUriCondition(coverUri, lPath)) {
            values.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, coverUri);
        }
        PrepareForNextCloud(rdbStore, values, albumId);
    }
}

void CloudMediaAlbumService::CheckAlbumManualCover()
{
    MEDIA_DEBUG_LOG("CheckAlbumManualCover enter");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "CheckAlbumManualCover Failed to get rdbStore.");
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::COVER_URI_SOURCE,
        to_string(static_cast<int32_t>(CoverUriSource::MANUAL_LOCAL_COVER)))
        ->Or()
        ->EqualTo(PhotoAlbumColumns::COVER_URI_SOURCE,
        to_string(static_cast<int32_t>(CoverUriSource::MANUAL_WAIT_PULL_COVER)))
        ->BeginWrap()
        ->EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::USER_GENERIC))
        ->Or()
        ->EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::SOURCE_GENERIC))
        ->EndWrap();
    vector<string> queryColumns = { PhotoAlbumColumns::ALBUM_ID, PhotoAlbumColumns::ALBUM_COVER_URI,
        PhotoAlbumColumns::COVER_URI_SOURCE, PhotoAlbumColumns::COVER_CLOUD_ID, PhotoAlbumColumns::ALBUM_LPATH };
    auto resultSet = rdbStore->Query(predicates, queryColumns);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "resultSet is null");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t albumId = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_ID,
            resultSet, TYPE_INT32));
        int32_t coverUriSource = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::COVER_URI_SOURCE,
            resultSet, TYPE_INT32));
        NativeRdb::ValuesBucket values;
        if (coverUriSource == static_cast<int32_t>(CoverUriSource::MANUAL_LOCAL_COVER)) {
            string coverCloudId = get<string>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::COVER_CLOUD_ID,
                resultSet, TYPE_STRING));
            string coverUri = get<string>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_COVER_URI,
                resultSet, TYPE_STRING));
            string fileId = MediaLibraryDataManagerUtils::GetFileIdFromPhotoUri(coverUri);
            if (IsCoverUriExistCloudId(coverUri, coverCloudId)) {
                MEDIA_DEBUG_LOG("CheckAlbumManualCover IsCoverUriExistCloudId");
                values.PutString(PhotoAlbumColumns::COVER_CLOUD_ID, coverCloudId);
                PrepareForNextCloud(rdbStore, values, albumId);
            }
        } else if (coverUriSource == static_cast<int32_t>(CoverUriSource::MANUAL_WAIT_PULL_COVER)) {
            HandleWaitPullCover(resultSet, rdbStore, values, albumId);
        }
    }
    resultSet->Close();
}

static inline string GetCover(const shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    string coverUri;
    int32_t fileId = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::MEDIA_ID, resultSet, TYPE_INT32));
    if (fileId <= 0) {
        return coverUri;
    }

    string extrUri = MediaFileUtils::GetExtraUri(
        get<string>(ResultSetUtils::GetValFromColumn(PhotoColumn::MEDIA_NAME, resultSet, TYPE_STRING)),
        get<string>(ResultSetUtils::GetValFromColumn(PhotoColumn::MEDIA_FILE_PATH, resultSet, TYPE_STRING)));
    return MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX, to_string(fileId), extrUri);
}

bool CloudMediaAlbumService::GetCoverUriFromCoverCloudId(string &coverCloudId, string &coverUri)
{
    auto index = coverCloudId.find(',');
    if (index == string::npos) {
        MEDIA_ERR_LOG("GetCoverUriFromCoverCloudId can not find split, coverCloudId:%{public}s",
            coverCloudId.c_str());
        return false;
    }
    string cloudId = coverCloudId.substr(index + 1);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "GetCoverUriFromCoverCloudId Failed to get rdbStore.");
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::PHOTO_CLOUD_ID, cloudId);

    vector<string> queryColumns = { MediaColumn::MEDIA_ID, MediaColumn::MEDIA_FILE_PATH, PhotoColumn::MEDIA_NAME };
    auto resultSet = rdbStore->Query(predicates, queryColumns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "resultset is null");

    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        coverUri = GetCover(resultSet);
    }
    resultSet->Close();
    return coverUri != "";
}

int32_t CloudMediaAlbumService::HandleLPathRecords(
    PhotoAlbumDto &record, ChangeType &changeType, OnFetchRecordsAlbumRespBody &resp)
{
    int32_t ret = E_OK;
    const bool insertFlag = !record.localAlbumInfo.has_value() && !record.isDelete;
    const bool updateFlag = record.localAlbumInfo.has_value() && !record.isDelete;
    const bool deleteFlag = record.localAlbumInfo.has_value() && record.isDelete;
    CHECK_AND_EXECUTE(!insertFlag, ret = this->PullInsert(record, changeType, resp));
    CHECK_AND_EXECUTE(!updateFlag, ret = this->PullUpdate(record, changeType, resp));
    CHECK_AND_EXECUTE(!deleteFlag, ret = this->PullDelete(record, changeType, resp));
    return ret;
}

int32_t CloudMediaAlbumService::PullInsert(
    const PhotoAlbumDto &record, ChangeType &changeType, OnFetchRecordsAlbumRespBody &resp)
{
    std::shared_ptr<AccurateRefresh::AlbumAccurateRefresh> albumRefresh =
        std::make_shared<AccurateRefresh::AlbumAccurateRefresh>();
    CHECK_AND_RETURN_RET_LOG(albumRefresh != nullptr, E_RDB_STORE_NULL, "failed to get albumRefresh.");
    // Check
    const bool insertFlag = !record.localAlbumInfo.has_value() && !record.isDelete;
    CHECK_AND_RETURN_RET_LOG(insertFlag, E_INVAL_ARG, "invalid data");
    // Process
    changeType = ChangeType::INSERT;
    int32_t ret = this->albumDao_.InsertCloudByLPath(record, albumRefresh);
    if (ret != E_OK) {
        MEDIA_ERR_LOG(
            "InsertCloudByLPath failed, ret: %{public}d, albumInfo: %{public}s", ret, record.ToString().c_str());
        resp.failedRecords.emplace_back(record.cloudId);
        this->albumDao_.InsertAlbumInsertFailedRecord(record.lPath);
        return ret;
    }
    resp.stats[StatsIndex::NEW_RECORDS_COUNT]++;
    // Notify
    ret = albumRefresh->Notify();
    CHECK_AND_RETURN_RET_LOG(
        ret == E_OK, ret, "fail to notify, ret: %{public}d, cloudId: %{public}s", ret, record.cloudId.c_str());
    MEDIA_INFO_LOG("PullInsert completed, ret: %{public}d, cloudId: %{public}s", ret, record.cloudId.c_str());
    return E_OK;
}

int32_t CloudMediaAlbumService::PullUpdate(
    const PhotoAlbumDto &record, ChangeType &changeType, OnFetchRecordsAlbumRespBody &resp)
{
    std::shared_ptr<AccurateRefresh::AlbumAccurateRefresh> albumRefresh =
        std::make_shared<AccurateRefresh::AlbumAccurateRefresh>();
    CHECK_AND_RETURN_RET_LOG(albumRefresh != nullptr, E_RDB_STORE_NULL, "failed to get albumRefresh.");
    // Check
    const bool updateFlag = record.localAlbumInfo.has_value() && !record.isDelete;
    CHECK_AND_RETURN_RET_LOG(updateFlag, E_INVAL_ARG, "invalid data");
    int32_t dirty = record.localAlbumInfo.value().dirty.value_or(static_cast<int32_t>(DirtyType::TYPE_MDIRTY));
    bool isValid = dirty != static_cast<int32_t>(Media::DirtyType::TYPE_MDIRTY) &&
                   dirty != static_cast<int32_t>(Media::DirtyType::TYPE_DELETED);
    CHECK_AND_RETURN_RET_WARN_LOG(isValid, E_OK, "lpath is dirty, skip. cloudId: %{public}s", record.cloudId.c_str());
    // Process
    changeType = ChangeType::UPDATE;
    int32_t ret = this->albumDao_.UpdateCloudAlbum(record, PhotoAlbumColumns::ALBUM_LPATH, record.lPath, albumRefresh);
    if (ret != E_OK) {
        MEDIA_ERR_LOG(
            "UpdateCloudAlbum failed, ret: %{public}d, albumInfo: %{public}s", ret, record.ToString().c_str());
        resp.failedRecords.emplace_back(record.cloudId);
        return ret;
    }
    resp.stats[StatsIndex::META_MODIFY_RECORDS_COUNT]++;
    // Notify
    ret = albumRefresh->Notify();
    CHECK_AND_RETURN_RET_LOG(
        ret == E_OK, ret, "fail to notify, ret: %{public}d, cloudId: %{public}s", ret, record.cloudId.c_str());
    MEDIA_INFO_LOG("PullUpdate completed, ret: %{public}d, cloudId: %{public}s", ret, record.cloudId.c_str());
    return E_OK;
}

int32_t CloudMediaAlbumService::PullDelete(
    const PhotoAlbumDto &record, ChangeType &changeType, OnFetchRecordsAlbumRespBody &resp)
{
    std::shared_ptr<AccurateRefresh::AlbumAccurateRefresh> albumRefresh =
        std::make_shared<AccurateRefresh::AlbumAccurateRefresh>();
    CHECK_AND_RETURN_RET_LOG(albumRefresh != nullptr, E_RDB_STORE_NULL, "failed to get albumRefresh.");
    // Check
    const bool deleteFlag = record.localAlbumInfo.has_value() && record.isDelete;
    CHECK_AND_RETURN_RET_LOG(deleteFlag, E_INVAL_ARG, "invalid data");
    int32_t dirty = record.localAlbumInfo.value().dirty.value_or(static_cast<int32_t>(DirtyType::TYPE_MDIRTY));
    bool isValid = dirty != static_cast<int32_t>(Media::DirtyType::TYPE_MDIRTY) &&
                   dirty != static_cast<int32_t>(Media::DirtyType::TYPE_DELETED);
    CHECK_AND_RETURN_RET_WARN_LOG(isValid, E_OK, "lpath is dirty, skip. cloudId: %{public}s", record.cloudId.c_str());
    // Process
    changeType = ChangeType::DELETE;
    int32_t ret = this->albumDao_.DeleteCloudAlbum(PhotoAlbumColumns::ALBUM_LPATH, record.lPath, albumRefresh);
    if (ret != E_OK) {
        MEDIA_ERR_LOG(
            "DeleteCloudAlbum failed, ret: %{public}d, albumInfo: %{public}s", ret, record.ToString().c_str());
        resp.failedRecords.emplace_back(record.cloudId);
        return ret;
    }
    resp.stats[StatsIndex::DELETE_RECORDS_COUNT]++;
    // Notify
    ret = albumRefresh->Notify();
    CHECK_AND_RETURN_RET_LOG(
        ret == E_OK, ret, "fail to notify, ret: %{public}d, cloudId: %{public}s", ret, record.cloudId.c_str());
    MEDIA_INFO_LOG("PullDelete completed, ret: %{public}d, cloudId: %{public}s", ret, record.cloudId.c_str());
    return E_OK;
}

bool CloudMediaAlbumService::IsSpaceFullAndSkipCreatedAlbum(const int32_t albumId, const bool isCloudSpaceFull)
{
    CHECK_AND_RETURN_RET(isCloudSpaceFull, false);
    int32_t cloudAssetCount = 0;
    int32_t ret = this->albumDao_.GetAlbumCloudAssetCount(albumId, cloudAssetCount);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, false, "GetAlbumCloudAssetCount fail, ret: %{public}d", ret);
    CHECK_AND_RETURN_RET_LOG(cloudAssetCount == 0, false, "cloudAssetCount: %{public}d", cloudAssetCount);
    return true;
}

int32_t CloudMediaAlbumService::GetCreatedRecordsWithCondition(const int32_t size, const bool isCloudSpaceFull,
    const std::vector<PhotoAlbumPo> &albumInfoList, std::vector<PhotoAlbumPo> &resultList)
{
    int32_t albumId = 0;
    bool isSkip = false;
    for (const auto &albumInfo : albumInfoList) {
        albumId = albumInfo.albumId.value_or(0);
        isSkip = this->IsSpaceFullAndSkipCreatedAlbum(albumId, isCloudSpaceFull);
        CHECK_AND_CONTINUE_INFO_LOG(!isSkip,
            "isSkipFullAndSkipCreatedAlbum, albumId: %{public}d, cloudId: %{public}s",
            albumId,
            albumInfo.cloudId.value_or("").c_str());
        resultList.emplace_back(albumInfo);
        CHECK_AND_BREAK(resultList.size() < size);
    }
    return E_OK;
}
}  // namespace OHOS::Media::CloudSync