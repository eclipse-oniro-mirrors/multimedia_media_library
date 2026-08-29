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

#define MLOG_TAG "Media_Cloud_Service"

#include "cloud_media_share_album_service.h"

#include <optional>
#include <string>
#include <vector>

#include "album_accurate_refresh.h"
#include "cloud_file_error.h"
#include "cloud_media_sync_const.h"
#include "media_gallery_sync_notify.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_type_const.h"
#include "photo_album_column.h"
#include "photo_album_po_writer.h"
#include "photos_dto.h"
#include "result_set_reader.h"

namespace OHOS::Media::CloudSync {

int32_t CloudMediaShareAlbumService::OnFetchRecords(
    std::vector<PhotoAlbumDto> &albumDtoList, std::vector<int32_t> &stats, std::vector<std::string> &failedRecords)
{
    for (auto &record : albumDtoList) {
        // 先按 lPath/cloudId 匹配本地相册，填充 record.localAlbumInfo
        int32_t ret = FindAlbumInfo(record);
        CHECK_AND_CONTINUE_ERR_LOG(ret == E_OK, "GetPhotoAlbum failed, ret: %{public}d, record: %{public}s",
            ret, record.ToString().c_str());
        // 再处理记录
        ChangeType changeType = ChangeType::INVAILD;
        ret = HandleRecord(record, changeType, stats, failedRecords);
        /* 检查 ret */
        if (ret != E_OK) {
            MEDIA_INFO_LOG("OnFetchRecords recordId %{public}s error %{public}d", record.cloudId.c_str(), ret);
            /* might need specific error type */
            if (ret == FileManagement::E_STOP || ret == E_RDB) {
                MediaGallerySyncNotify::GetInstance().FinalNotify();
                return FileManagement::E_STOP;
            }
            continue;
        } else {
            if (changeType != ChangeType::INVAILD && changeType != ChangeType::DELETE) {
                MediaGallerySyncNotify::GetInstance().AddNotify(
                    PhotoAlbumColumns::ALBUM_GALLERY_CLOUD_URI_PREFIX, changeType, record.cloudId);
            }
        }
    }
    MediaGallerySyncNotify::GetInstance().FinalNotify();
    return E_OK;
}

int32_t CloudMediaShareAlbumService::FindAlbumInfo(PhotoAlbumDto &record)
{
    CHECK_AND_RETURN_RET_INFO_LOG(!record.localAlbumInfo.has_value(), E_OK,
        "FindAlbumInfo record already has localAlbumInfo, cloudId: %{public}s", record.cloudId.c_str());

    this->albumDao_.GetPhotoAlbum(record.lPath, record.localAlbumInfo);
    CHECK_AND_RETURN_RET_INFO_LOG(!record.localAlbumInfo.has_value(), E_OK,
        "GetPhotoAlbum record already has localAlbumInfo, cloudId: %{public}s", record.cloudId.c_str());

    return this->commonDao_.QueryPhotoAlbumByCloudId(record.cloudId, record.localAlbumInfo);
}

int32_t CloudMediaShareAlbumService::HandleRecord(
    PhotoAlbumDto &record, ChangeType &changeType, std::vector<int32_t> &stats, std::vector<std::string> &failedRecords)
{
    int32_t ret = E_OK;
    const bool insertFlag = !record.localAlbumInfo.has_value() && !record.isDelete;
    const bool updateFlag = record.localAlbumInfo.has_value() && !record.isDelete;
    const bool deleteFlag = record.localAlbumInfo.has_value() && record.isDelete;
    CHECK_AND_EXECUTE(!insertFlag, ret = this->PullInsert(record, changeType, stats, failedRecords));
    CHECK_AND_EXECUTE(!updateFlag, ret = this->PullUpdate(record, changeType, stats, failedRecords));
    CHECK_AND_EXECUTE(!deleteFlag, ret = this->PullDelete(record, changeType, stats, failedRecords));
    return ret;
}

int32_t CloudMediaShareAlbumService::PullInsert(
    const PhotoAlbumDto &record, ChangeType &changeType, std::vector<int32_t> &stats,
    std::vector<std::string> &failedRecords)
{
    std::shared_ptr<AccurateRefresh::AlbumAccurateRefresh> albumRefresh =
        std::make_shared<AccurateRefresh::AlbumAccurateRefresh>();
    CHECK_AND_RETURN_RET_LOG(albumRefresh != nullptr, E_RDB_STORE_NULL, "failed to get albumRefresh.");
    // Check
    const bool insertFlag = !record.localAlbumInfo.has_value() && !record.isDelete;
    CHECK_AND_RETURN_RET_LOG(insertFlag, E_INVALID_VALUES, "invalid data");
    // Process
    changeType = ChangeType::INSERT;
    int32_t ret = this->albumDao_.InsertCloudByLPath(record, albumRefresh);
    if (ret != E_OK) {
        MEDIA_ERR_LOG(
            "InsertCloudByLPath failed, ret: %{public}d, albumInfo: %{public}s", ret, record.ToString().c_str());
        failedRecords.emplace_back(record.cloudId);
        this->albumDao_.InsertAlbumInsertFailedRecord(record.lPath);
        return ret;
    }
    stats[StatsIndex::NEW_RECORDS_COUNT]++;
    // Notify
    ret = albumRefresh->Notify();
    CHECK_AND_RETURN_RET_LOG(
        ret == E_OK, ret, "fail to notify, ret: %{public}d, cloudId: %{public}s", ret, record.cloudId.c_str());
    MEDIA_INFO_LOG("PullInsert completed, ret: %{public}d, cloudId: %{public}s", ret, record.cloudId.c_str());
    return E_OK;
}

int32_t CloudMediaShareAlbumService::PullUpdate(
    const PhotoAlbumDto &record, ChangeType &changeType, std::vector<int32_t> &stats,
    std::vector<std::string> &failedRecords)
{
    std::shared_ptr<AccurateRefresh::AlbumAccurateRefresh> albumRefresh =
        std::make_shared<AccurateRefresh::AlbumAccurateRefresh>();
    CHECK_AND_RETURN_RET_LOG(albumRefresh != nullptr, E_RDB_STORE_NULL, "failed to get albumRefresh.");
    // Check
    const bool updateFlag = record.localAlbumInfo.has_value() && !record.isDelete;
    CHECK_AND_RETURN_RET_LOG(updateFlag, E_INVALID_VALUES, "invalid data");
    const PhotoAlbumPo &albumInfo = record.localAlbumInfo.value();

    const int32_t dirty = albumInfo.dirty.value_or(static_cast<int32_t>(DirtyType::TYPE_MDIRTY));
    bool isValid = dirty != static_cast<int32_t>(Media::DirtyType::TYPE_MDIRTY) &&
                   dirty != static_cast<int32_t>(Media::DirtyType::TYPE_DELETED);
    CHECK_AND_RETURN_RET_WARN_LOG(isValid, E_OK, "lpath is dirty, skip. cloudId: %{public}s", record.cloudId.c_str());
    // Process
    changeType = ChangeType::UPDATE;
    const std::string albumIdStr = std::to_string(albumInfo.albumId.value_or(0));
    int32_t ret = this->albumDao_.UpdateCloudAlbum(record, PhotoAlbumColumns::ALBUM_ID, albumIdStr, albumRefresh);
    if (ret != E_OK) {
        MEDIA_ERR_LOG(
            "UpdateCloudAlbum failed, ret: %{public}d, albumInfo: %{public}s", ret, record.ToString().c_str());
        failedRecords.emplace_back(record.cloudId);
        return ret;
    }
    stats[StatsIndex::META_MODIFY_RECORDS_COUNT]++;
    // Notify
    ret = albumRefresh->Notify();
    CHECK_AND_RETURN_RET_LOG(
        ret == E_OK, ret, "fail to notify, ret: %{public}d, cloudId: %{public}s", ret, record.cloudId.c_str());
    MEDIA_INFO_LOG("PullUpdate completed, ret: %{public}d, cloudId: %{public}s", ret, record.cloudId.c_str());
    return E_OK;
}

int32_t CloudMediaShareAlbumService::PullDelete(
    const PhotoAlbumDto &record, ChangeType &changeType, std::vector<int32_t> &stats,
    std::vector<std::string> &failedRecords)
{
    std::shared_ptr<AccurateRefresh::AlbumAccurateRefresh> albumRefresh =
        std::make_shared<AccurateRefresh::AlbumAccurateRefresh>();
    CHECK_AND_RETURN_RET_LOG(albumRefresh != nullptr, E_RDB_STORE_NULL, "failed to get albumRefresh.");
    // Check
    const bool deleteFlag = record.localAlbumInfo.has_value() && record.isDelete;
    CHECK_AND_RETURN_RET_LOG(deleteFlag, E_INVALID_VALUES, "invalid data");
    int32_t dirty = record.localAlbumInfo.value().dirty.value_or(static_cast<int32_t>(DirtyType::TYPE_MDIRTY));
    bool isValid = dirty != static_cast<int32_t>(Media::DirtyType::TYPE_MDIRTY) &&
                   dirty != static_cast<int32_t>(Media::DirtyType::TYPE_DELETED);
    CHECK_AND_RETURN_RET_WARN_LOG(isValid, E_OK, "lpath is dirty, skip. cloudId: %{public}s", record.cloudId.c_str());
    // Process
    changeType = ChangeType::DELETE;
    int32_t ret = this->albumDao_.DeleteCloudAlbum(PhotoAlbumColumns::ALBUM_CLOUD_ID, record.cloudId, albumRefresh);
    if (ret != E_OK) {
        MEDIA_ERR_LOG(
            "DeleteCloudAlbum failed, ret: %{public}d, albumInfo: %{public}s", ret, record.ToString().c_str());
        failedRecords.emplace_back(record.cloudId);
        return ret;
    }
    stats[StatsIndex::DELETE_RECORDS_COUNT]++;
    MEDIA_INFO_LOG("PullDelete completed, ret: %{public}d, cloudId: %{public}s", ret, record.cloudId.c_str());
    return E_OK;
}


}  // namespace OHOS::Media::CloudSync
