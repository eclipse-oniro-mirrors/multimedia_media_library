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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_ALBUM_SERVICE_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_ALBUM_SERVICE_H

#include <map>
#include <vector>

#include "media_column.h"
#include "rdb_store.h"
#include "photos_dto.h"
#include "photo_album_po.h"
#include "photo_album_dto.h"
#include "on_fetch_records_album_vo.h"
#include "dataobs_mgr_changeinfo.h"
#include "cloud_media_album_dao.h"
#include "cloud_media_define.h"
#include "media_operate_result.h"

namespace OHOS::Media::CloudSync {
using ChangeType = OHOS::AAFwk::ChangeInfo::ChangeType;
class EXPORT CloudMediaAlbumService {
public:
    int32_t GetCreatedRecords(
        int32_t size, const bool isCloudSpaceFull, std::vector<PhotoAlbumPo> &albumInfoList);
    std::vector<PhotoAlbumPo> GetAlbumMetaModifiedRecords(int32_t size);
    std::vector<PhotoAlbumPo> GetAlbumFileModifiedRecords(int32_t size);  // it's not exist
    std::vector<PhotoAlbumPo> GetAlbumDeletedRecords(int32_t size);
    std::vector<PhotoAlbumPo> GetAlbumCopyRecords(int32_t size);  // it's not exist
    int32_t HandleCloudAlbumNotFound(const PhotoAlbumDto &album);
    int32_t HandleDetailcode(ErrorDetailCode &errorCode);
    int32_t OnRecordFailedErrorDetails(const PhotoAlbumDto &album);
    int32_t OnRecordFailed(const PhotoAlbumDto &album);
    int32_t OnCreateRecords(std::vector<PhotoAlbumDto> &albumDtoList, int32_t &failedSize);
    int32_t OnMdirtyRecords(std::vector<PhotoAlbumDto> &albumDtoList, int32_t &failedSize);
    int32_t OnFdirtyRecords();
    int32_t OnDeleteRecords(std::vector<PhotoAlbumDto> &albumDtoList, int32_t &failSize);
    int32_t OnCopyRecords();
    int32_t OnFetchRecords(std::vector<PhotoAlbumDto> &albumDtoList, OnFetchRecordsAlbumRespBody &resp);
    int32_t OnDentryFileInsert();
    int32_t OnStartSync();
    int32_t OnCompleteSync();
    int32_t OnCompletePull(const MediaOperateResult &optRet);
    int32_t OnCompletePush();
    int32_t OnCompleteCheck();
    void CheckAlbumManualCover();
    bool GetCoverUriFromCoverCloudId(std::string &coverCloudId, std::string &coverUri);
    void HandleWaitPullCover(shared_ptr<NativeRdb::ResultSet> &resultSet,
        const shared_ptr<MediaLibraryRdbStore> rdbStore, NativeRdb::ValuesBucket &values, int32_t albumId);

private:
    int32_t OnDeleteAlbums(std::vector<std::string> &failedAlbumIds);
    int32_t OnFetchOldRecords(std::vector<PhotoAlbumDto> &records, OnFetchRecordsAlbumRespBody &resp);
    int32_t OnFetchLPathRecords(std::vector<PhotoAlbumDto> &records, OnFetchRecordsAlbumRespBody &resp);
    int32_t HandleFetchOldRecord(PhotoAlbumDto &record, bool &bContinue,
        OHOS::AAFwk::ChangeInfo::ChangeType &changeType, OnFetchRecordsAlbumRespBody &resp);
    int32_t HandleLPathRecords(PhotoAlbumDto &record, ChangeType &changeType, OnFetchRecordsAlbumRespBody &resp);
    int32_t ConvertToSingleScreenshots(PhotoAlbumDto &album, std::vector<PhotoAlbumDto> &records);
    int32_t HandleFetchOldRecordNew(
        PhotoAlbumDto &record, AAFwk::ChangeInfo::ChangeType &changeType, OnFetchRecordsAlbumRespBody &resp);
    bool IsSpaceFullAndSkipCreatedAlbum(const int32_t albumId, const bool isCloudSpaceFull);
    int32_t GetCreatedRecordsWithCondition(const int32_t size, const bool isCloudSpaceFull,
        const std::vector<PhotoAlbumPo> &albumInfoList, std::vector<PhotoAlbumPo> &resultList);
    int32_t PullInsert(const PhotoAlbumDto &record, ChangeType &changeType, OnFetchRecordsAlbumRespBody &resp);
    int32_t PullUpdate(const PhotoAlbumDto &record, ChangeType &changeType, OnFetchRecordsAlbumRespBody &resp);
    int32_t PullDelete(const PhotoAlbumDto &record, ChangeType &changeType, OnFetchRecordsAlbumRespBody &resp);

private:
    CloudMediaAlbumDao albumDao_;
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_ALBUM_SERVICE_H