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

using ChangeType = OHOS::AAFwk::ChangeInfo::ChangeType;
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
    MEDIA_INFO_LOG("OnFetchRecords enter");
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
            MEDIA_INFO_LOG("OnFetchRecords add to new record");
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

int32_t CloudMediaAlbumService::HandleLPathRecords(PhotoAlbumDto &record,
    const std::map<std::string, int> &lpathRowIdMap, const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
    ChangeType &changeType, OnFetchRecordsAlbumRespBody &resp)
{
    MEDIA_INFO_LOG("HandleLPathRecords enter");
    int ret = E_OK;
    if ((lpathRowIdMap.find(record.lPath) == lpathRowIdMap.end()) && !record.isDelete) {
        changeType = ChangeType::INSERT;
        ret = this->albumDao_.InsertCloudByLPath(record);
        MEDIA_INFO_LOG("HandleLPathRecords insert %{public}s, %{public}d", record.cloudId.c_str(), ret);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("HandleLPathRecords InsertCloudByLPath error");
            resp.failedRecords.emplace_back(record.cloudId);
            this->albumDao_.InsertAlbumInsertFailedRecord(record.lPath);
        }
        resp.stats[StatsIndex::NEW_RECORDS_COUNT]++;
    } else if (lpathRowIdMap.find(record.lPath) != lpathRowIdMap.end()) {
        ret = resultSet->GoToRow(lpathRowIdMap.at(record.lPath));
        int32_t dirty = GetLocalMatchDirty(*resultSet);
        if (dirty == static_cast<int32_t>(Media::DirtyType::TYPE_MDIRTY) ||
            dirty == static_cast<int32_t>(Media::DirtyType::TYPE_DELETED)) {
            MEDIA_INFO_LOG("HandleLPathRecords lpath is dirty skip %{public}s", record.cloudId.c_str());
            return E_OK;
        } else if (record.isDelete) {
            /* delete */
            ret = this->albumDao_.DeleteCloudAlbum(PhotoAlbumColumns::ALBUM_LPATH, record.lPath);
            MEDIA_INFO_LOG("HandleLPathRecords lpath delete %{public}s, %{public}d", record.cloudId.c_str(), ret);
            if (ret != E_OK) {
                resp.failedRecords.emplace_back(record.cloudId);
            }
            resp.stats[StatsIndex::DELETE_RECORDS_COUNT]++;
        } else {
            /* update */
            changeType = ChangeType::UPDATE;
            ret = this->albumDao_.UpdateCloudAlbum(record, PhotoAlbumColumns::ALBUM_LPATH, record.lPath);
            MEDIA_INFO_LOG("HandleLPathRecords lpath update %{public}s, %{public}d", record.cloudId.c_str(), ret);
            if (ret != E_OK) {
                resp.failedRecords.emplace_back(record.cloudId);
            }
            resp.stats[StatsIndex::META_MODIFY_RECORDS_COUNT]++;
        }
    } else {
        MEDIA_ERR_LOG("album sync lpath recordId %s has multiple file in db!", record.cloudId.c_str());
    }
    return ret;
}

int32_t CloudMediaAlbumService::HandleFetchOldRecordNew(
    PhotoAlbumDto &record, ChangeType &changeType, OnFetchRecordsAlbumRespBody &resp)
{
    if (!record.isDelete) {
        /* insert */
        int32_t ret = this->albumDao_.InsertCloudByCloudId(record);
        if (ret != E_OK) {
            MEDIA_ERR_LOG(
                "HandleFetchOldRecord InsertCloudByCloudId error %{public}s, %{public}d", record.cloudId.c_str(), ret);
            resp.failedRecords.emplace_back(record.cloudId);
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
    MEDIA_INFO_LOG("HandleFetchOldRecord enter");
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
        if (dirty != static_cast<int32_t>(Media::DirtyType::TYPE_SYNCED)) {
            /* local dirty */
            MEDIA_INFO_LOG("HandleFetchOldRecord is dirty skip %{public}s", record.cloudId.c_str());
            bContinue = true;
            return ret;
        } else if (record.isDelete) {
            /* delete */
            ret = this->albumDao_.DeleteCloudAlbum(PhotoAlbumColumns::ALBUM_CLOUD_ID, record.cloudId);
            MEDIA_INFO_LOG("HandleFetchOldRecorddelete %{public}s, %{public}d", record.cloudId.c_str(), ret);
            if (ret != E_OK) {
                MEDIA_ERR_LOG("HandleFetchOldRecord DeleteCloudAlbum error %{public}s", record.cloudId.c_str());
                resp.failedRecords.emplace_back(record.cloudId);
            }
            resp.stats[StatsIndex::DELETE_RECORDS_COUNT]++;
        } else {
            /* update */
            ret = this->albumDao_.UpdateCloudAlbum(record, PhotoAlbumColumns::ALBUM_CLOUD_ID, record.cloudId);
            MEDIA_INFO_LOG("HandleFetchOldRecord update %{public}s, %{public}d", record.cloudId.c_str(), ret);
            changeType = ChangeType::UPDATE;
            if (ret != E_OK) {
                MEDIA_ERR_LOG("HandleFetchOldRecord UpdateCloudAlbum error %{public}s", record.cloudId.c_str());
                resp.failedRecords.emplace_back(record.cloudId);
            }
            resp.stats[StatsIndex::META_MODIFY_RECORDS_COUNT]++;
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
    auto lpaths = std::vector<std::string>();
    for (auto &record : records) {
        auto lpath = record.lPath;
        lpaths.emplace_back(lpath);
        MEDIA_DEBUG_LOG("OnFetchLPathRecords Record: %{public}s", record.ToString().c_str());
    }
    auto [resultSet, lpathRowIdMap] = this->albumDao_.QueryLocalAlbum(PhotoAlbumColumns::ALBUM_LPATH, lpaths);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RESULT_SET_NULL, "resultset is null");
    for (auto &record : records) {
        std::string lpath = record.lPath;
        ChangeType changeType = ChangeType::INVAILD;
        ret = HandleLPathRecords(record, lpathRowIdMap, resultSet, changeType, resp);
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

std::vector<PhotoAlbumPo> CloudMediaAlbumService::GetCheckRecords(const std::vector<std::string> &cloudIds)
{
    MEDIA_INFO_LOG("CloudMediaAlbumService::GetCheckRecords enter %{public}zu", cloudIds.size());
    std::vector<PhotoAlbumPo> albumsPoList;

    auto [resultSet, recordIdRowIdMap] = this->albumDao_.QueryLocalAlbum(PhotoAlbumColumns::ALBUM_LPATH, cloudIds);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, albumsPoList, "GetCheckRecords fail to QueryLocalAlbum dkRecordIds");
    return albumsPoList;
}

std::vector<PhotoAlbumPo> CloudMediaAlbumService::GetAlbumCreatedRecords(int32_t size)
{
    MEDIA_INFO_LOG("CloudMediaAlbumService::GetAlbumCreatedRecords enter");
    std::vector<PhotoAlbumPo> photoAlbumList;
    int32_t ret = this->albumDao_.GetCreatedAlbum(size, photoAlbumList);
    CHECK_AND_PRINT_LOG(ret == E_OK, "failed to get createdAlbum record");
    return photoAlbumList;
}

std::vector<PhotoAlbumPo> CloudMediaAlbumService::GetAlbumMetaModifiedRecords(int32_t size)
{
    MEDIA_INFO_LOG("CloudMediaAlbumService::GetAlbumMetaModifiedRecords enter");
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

int32_t CloudMediaAlbumService::OnCreateRecords(std::vector<PhotoAlbumDto> &albumDtoList, int32_t &failSize)
{
    MEDIA_INFO_LOG("enter CloudMediaAlbumService::OnCreateRecords %{public}zu", albumDtoList.size());
    if (albumDtoList.size() <= 0) {
        return E_OK;
    }
    return this->albumDao_.OnCreateRecords(albumDtoList, failSize);
}

int32_t CloudMediaAlbumService::OnMdirtyRecords(std::vector<PhotoAlbumDto> &albumDtoList, int32_t &failSize)
{
    MEDIA_INFO_LOG("enter CloudMediaAlbumService::OnMdirtyRecords %{public}zu", albumDtoList.size());
    int32_t ret = E_OK;
    for (auto album : albumDtoList) {
        if (album.isSuccess) {
            ret = this->albumDao_.OnMdirtyAlbumRecords(album.cloudId);
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
    return 0;
}

int32_t CloudMediaAlbumService::OnCompletePull()
{
    std::vector<std::string> failedAlbumIds;
    int32_t ret = OnDeleteAlbums(failedAlbumIds);
    return ret;
}

int32_t CloudMediaAlbumService::OnCompletePush()
{
    return this->albumDao_.ClearAlbumFailedRecords();
}

int32_t CloudMediaAlbumService::OnCompleteCheck()
{
    MEDIA_INFO_LOG("enter CloudMediaAlbumService::OnCompleteCheck");
    return 0;
}
}  // namespace OHOS::Media::CloudSync