/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include "gallery_media_count_statistic.h"

#include "media_log.h"
#include "backup_database_utils.h"
#include "backup_const.h"
#include "media_backup_report_data_type.h"
#include "statistic_processor.h"
#include "media_file_utils.h"

namespace OHOS::Media {
std::vector<AlbumMediaStatisticInfo> GalleryMediaCountStatistic::Load()
{
    CHECK_AND_RETURN_RET_LOG(this->galleryRdb_ != nullptr, {}, "galleryRdb_ is nullptr, Maybe init failed.");
    std::vector<AlbumMediaStatisticInfo> infoList = {
        // data-transfer statistic info.
        this->GetAllStatInfo(),
        this->GetAllImageStatInfo(),
        this->GetAllVideoStatInfo(),
        this->GetAllRestoreStatInfo(),
        this->GetAllRestoreImageStatInfo(),
        this->GetAllRestoreVideoStatInfo(),
        // other statistic info.
        this->GetSdCardStatInfo(),
        this->GetDuplicateStatInfo(),
        this->GetAppTwinStatInfo(),
        this->GetOnlyHDCInfo(),
        this->GetSizeUnnormalInfo(),
        this->GetLiveStatInfo(),
        this->GetTempInfo(),
        this->GetNotSyncInfo(),
        this->GetGalleryAlbumCountInfo(),
        // statistic info.
        this->GetImageAlbumInfo(),
        this->GetVideoAlbumInfo(),
        // folder info.
        this->GetFavoriteAlbumStatInfo(),
        this->GetTrashedAlbumStatInfo(),
    };
    // key album statistic info.
    std::vector<AlbumMediaStatisticInfo> importsAlbumList = this->GetAlbumInfoByLPath("/Pictures/cloud/Imports");
    infoList.insert(infoList.end(), importsAlbumList.begin(), importsAlbumList.end());
    std::vector<AlbumMediaStatisticInfo> hiddenAlbumList = this->GetAlbumInfoByLPath("/Pictures/hiddenAlbum");
    infoList.insert(infoList.end(), hiddenAlbumList.begin(), hiddenAlbumList.end());
    // key album of album_plugin.
    std::vector<AlbumMediaStatisticInfo> cameraAlbumList = this->GetAlbumInfoByName("相机");
    infoList.insert(infoList.end(), cameraAlbumList.begin(), cameraAlbumList.end());
    std::vector<AlbumMediaStatisticInfo> screenShotAlbumList = this->GetAlbumInfoByName("截图");
    infoList.insert(infoList.end(), screenShotAlbumList.begin(), screenShotAlbumList.end());
    std::vector<AlbumMediaStatisticInfo> weiXinAlbumList = this->GetAlbumInfoByName("微信");
    infoList.insert(infoList.end(), weiXinAlbumList.begin(), weiXinAlbumList.end());
    std::vector<AlbumMediaStatisticInfo> weiboAlbumList = this->GetAlbumInfoByName("微博");
    infoList.insert(infoList.end(), weiboAlbumList.begin(), weiboAlbumList.end());
    std::vector<AlbumMediaStatisticInfo> shareAlbumList = this->GetAlbumInfoByName("华为分享");
    infoList.insert(infoList.end(), shareAlbumList.begin(), shareAlbumList.end());
    std::vector<AlbumMediaStatisticInfo> otherAlbumList = this->GetAlbumInfoByName("其它");
    infoList.insert(infoList.end(), otherAlbumList.begin(), otherAlbumList.end());
    return infoList;
}

int32_t GalleryMediaCountStatistic::GetCount(const std::string &query, const std::vector<NativeRdb::ValueObject> &args)
{
    return BackupDatabaseUtils::QueryInt(this->galleryRdb_, query, CUSTOM_COUNT, args);
}

int32_t GalleryMediaCountStatistic::QueryGalleryAllCount(SearchCondition searchCondition)
{
    CHECK_AND_RETURN_RET_LOG(this->galleryRdb_ != nullptr, 0, "Media_Restore: galleryRdb_ is null.");
    int32_t mediaType = searchCondition.GetMediaType();
    int32_t hiddenType = searchCondition.GetHiddenType();
    int32_t trashedType = searchCondition.GetTrashedType();
    int32_t cloudType = searchCondition.GetCloudType();
    int32_t favoriteType = searchCondition.GetFavoriteType();
    int32_t burstType = searchCondition.GetBurstType();
    std::vector<NativeRdb::ValueObject> params = {mediaType,
        mediaType,
        hiddenType,
        hiddenType,
        hiddenType,
        trashedType,
        trashedType,
        trashedType,
        cloudType,
        cloudType,
        cloudType,
        favoriteType,
        favoriteType,
        favoriteType,
        burstType,
        burstType,
        burstType};
    return this->GetCount(this->SQL_QUERY_ALL_GALLERY_COUNT, params);
}

int32_t GalleryMediaCountStatistic::QueryAlbumGalleryCount(SearchCondition searchCondition)
{
    CHECK_AND_RETURN_RET_LOG(this->galleryRdb_ != nullptr, 0, "Media_Restore: galleryRdb_ is null.");
    int32_t mediaType = searchCondition.GetMediaType();
    int32_t hiddenType = searchCondition.GetHiddenType();
    int32_t trashedType = searchCondition.GetTrashedType();
    int32_t cloudType = searchCondition.GetCloudType();
    int32_t favoriteType = searchCondition.GetFavoriteType();
    int32_t burstType = searchCondition.GetBurstType();
    std::vector<NativeRdb::ValueObject> params = {mediaType,
        mediaType,
        hiddenType,
        hiddenType,
        hiddenType,
        trashedType,
        trashedType,
        trashedType,
        cloudType,
        cloudType,
        cloudType,
        favoriteType,
        favoriteType,
        favoriteType,
        burstType,
        burstType,
        burstType};
    return this->GetCount(this->SQL_QUERY_ALBUM_GALLERY_COUNT, params);
}

/**
 * @brief Get the row count of gallery_media.
 * @param mediaType - 0 all, 1 picture, 3 video
 */
int32_t GalleryMediaCountStatistic::GetGalleryMediaAllRestoreCount(SearchCondition searchCondition)
{
    CHECK_AND_RETURN_RET_LOG(this->galleryRdb_ != nullptr, 0, "Media_Restore: galleryRdb_ is null.");
    int32_t mediaType = searchCondition.GetMediaType();
    int32_t hiddenType = searchCondition.GetHiddenType();
    int32_t trashedType = searchCondition.GetTrashedType();
    int32_t cloudType = searchCondition.GetCloudType();
    int32_t favoriteType = searchCondition.GetFavoriteType();
    int32_t burstType = searchCondition.GetBurstType();
    bool hasLowQualityImage = this->HasLowQualityImage();
    int32_t hasLowQualityImageFlag = hasLowQualityImage ? 1 : 0;
    int32_t shouldIncludeSdFlag = this->shouldIncludeSd_ ? 1 : 0;
    std::vector<NativeRdb::ValueObject> params = {hasLowQualityImageFlag,
        shouldIncludeSdFlag,
        mediaType,
        mediaType,
        hiddenType,
        hiddenType,
        hiddenType,
        trashedType,
        trashedType,
        trashedType,
        cloudType,
        cloudType,
        cloudType,
        favoriteType,
        favoriteType,
        favoriteType,
        burstType,
        burstType,
        burstType};
    return this->GetCount(this->SQL_GALLERY_MEDIA_QUERY_COUNT, params);
}

int32_t GalleryMediaCountStatistic::QueryGalleryCloneCount()
{
    static string QUERY_GALLERY_CLONE_COUNT =
        string("SELECT count(1) AS count FROM gallery_media WHERE local_media_id = -3 AND _size > 0 ") +
        "AND COALESCE(storage_id, 0) IN (0, 65537) AND relative_bucket_id NOT IN ( " +
        "SELECT DISTINCT relative_bucket_id FROM garbage_album WHERE type = 1)";
    return BackupDatabaseUtils::QueryInt(this->galleryRdb_, QUERY_GALLERY_CLONE_COUNT, CUSTOM_COUNT);
}

/**
 * @brief Get the row count of gallery_media storage in SD card.
 * @param mediaType - 0 all, 1 picture, 3 video
 */
int32_t GalleryMediaCountStatistic::QueryGallerySdCardCount(SearchCondition searchCondition)
{
    CHECK_AND_RETURN_RET_LOG(this->galleryRdb_ != nullptr, 0, "Media_Restore: galleryRdb_ is null.");
    int32_t mediaType = searchCondition.GetMediaType();
    int32_t hiddenType = searchCondition.GetHiddenType();
    int32_t trashedType = searchCondition.GetTrashedType();
    int32_t cloudType = searchCondition.GetCloudType();
    int32_t favoriteType = searchCondition.GetFavoriteType();
    int32_t burstType = searchCondition.GetBurstType();
    std::vector<NativeRdb::ValueObject> params = {mediaType,
        mediaType,
        hiddenType,
        hiddenType,
        hiddenType,
        trashedType,
        trashedType,
        trashedType,
        cloudType,
        cloudType,
        cloudType,
        favoriteType,
        favoriteType,
        favoriteType,
        burstType,
        burstType,
        burstType};
    return this->GetCount(this->SQL_QUERY_GALLERY_SD_CARD_COUNT, params);
}

/**
 * @brief Get the row count of gallery_media.
 * @param searchType - 0 all, 1 cloud
 * @param mediaType - 0 all, 1 picture, 3 video
 */
int32_t GalleryMediaCountStatistic::QueryAlbumAllVideoCount(SearchCondition searchCondition)
{
    CHECK_AND_RETURN_RET_LOG(this->galleryRdb_ != nullptr, 0, "Media_Restore: galleryRdb_ is null.");
    int32_t mediaType = searchCondition.GetMediaType();
    int32_t hiddenType = searchCondition.GetHiddenType();
    int32_t trashedType = searchCondition.GetTrashedType();
    int32_t cloudType = searchCondition.GetCloudType();
    int32_t favoriteType = searchCondition.GetFavoriteType();
    int32_t burstType = searchCondition.GetBurstType();
    std::vector<NativeRdb::ValueObject> params = {mediaType,
        mediaType,
        hiddenType,
        hiddenType,
        hiddenType,
        trashedType,
        trashedType,
        trashedType,
        cloudType,
        cloudType,
        cloudType,
        favoriteType,
        favoriteType,
        favoriteType,
        burstType,
        burstType,
        burstType};
    return this->GetCount(this->SQL_QUERY_GALLERY_MEDIA_ALL_VIDEO_COUNT, params);
}

/**
 * @brief Get the row count of gallery_media.
 * @param searchType - 0 all, 1 cloud
 * @param mediaType - 0 all, 1 picture, 3 video
 */
int32_t GalleryMediaCountStatistic::QueryLiveCount(int32_t searchType, int32_t mediaType)
{
    CHECK_AND_RETURN_RET_LOG(this->galleryRdb_ != nullptr, 0, "Media_Restore: galleryRdb_ is null.");
    std::vector<NativeRdb::ValueObject> params = {searchType, mediaType, mediaType};
    return this->GetCount(this->SQL_QUERY_LIVE_COUNT, params);
}

/**
 * @brief Get the row count of gallery_media.
 * @param searchType - 0 all, 1 cloud
 * @param mediaType - 0 all, 1 picture, 3 video
 */
int32_t GalleryMediaCountStatistic::QueryTempCount(int32_t searchType, int32_t mediaType)
{
    CHECK_AND_RETURN_RET_LOG(this->galleryRdb_ != nullptr, 0, "Media_Restore: galleryRdb_ is null.");
    std::vector<NativeRdb::ValueObject> params = {searchType, mediaType, mediaType};
    return this->GetCount(this->SQL_QUERY_TEMP_COUNT, params);
}

/**
 * @brief Get the row count of gallery_media.
 * @param searchType - 0 all, 1 cloud
 * @param mediaType - 0 all, 1 picture, 3 video
 */
std::vector<AlbumStatisticInfo> GalleryMediaCountStatistic::QueryAlbumCountByName(
    const std::string &albumName, SearchCondition searchCondition)
{
    CHECK_AND_RETURN_RET_LOG(this->galleryRdb_ != nullptr, {}, "Media_Restore: galleryRdb_ is null.");
    int32_t mediaType = searchCondition.GetMediaType();
    int32_t hiddenType = searchCondition.GetHiddenType();
    int32_t trashedType = searchCondition.GetTrashedType();
    int32_t cloudType = searchCondition.GetCloudType();
    int32_t favoriteType = searchCondition.GetFavoriteType();
    int32_t burstType = searchCondition.GetBurstType();
    std::vector<NativeRdb::ValueObject> params = {albumName,
        mediaType,
        mediaType,
        hiddenType,
        hiddenType,
        hiddenType,
        trashedType,
        trashedType,
        trashedType,
        cloudType,
        cloudType,
        cloudType,
        favoriteType,
        favoriteType,
        favoriteType,
        burstType,
        burstType,
        burstType};
    auto resultSet = this->galleryRdb_->QuerySql(this->SQL_QUERY_ALBUM_COUNT_BY_NAME, params);
    CHECK_AND_RETURN_RET(resultSet != nullptr, {});
    std::vector<AlbumStatisticInfo> infoList;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        AlbumStatisticInfo info;
        info.count = GetInt32Val("count", resultSet);
        info.lPath = GetStringVal("lPath", resultSet);
        info.albumName = GetStringVal("albumName", resultSet);
        infoList.emplace_back(info);
    }
    resultSet->Close();
    return infoList;
}

/**
 * @brief Get the row count of gallery_media.
 * @param searchType - 0 all, 1 cloud
 * @param mediaType - 0 all, 1 picture, 3 video
 */
std::vector<AlbumStatisticInfo> GalleryMediaCountStatistic::QueryAlbumCountByLPath(
    const std::string &lPath, SearchCondition searchCondition)
{
    CHECK_AND_RETURN_RET_LOG(this->galleryRdb_ != nullptr, {}, "Media_Restore: galleryRdb_ is null.");
    int32_t mediaType = searchCondition.GetMediaType();
    int32_t hiddenType = searchCondition.GetHiddenType();
    int32_t trashedType = searchCondition.GetTrashedType();
    int32_t cloudType = searchCondition.GetCloudType();
    int32_t favoriteType = searchCondition.GetFavoriteType();
    int32_t burstType = searchCondition.GetBurstType();
    std::vector<NativeRdb::ValueObject> params = {lPath,
        mediaType,
        mediaType,
        hiddenType,
        hiddenType,
        hiddenType,
        trashedType,
        trashedType,
        trashedType,
        cloudType,
        cloudType,
        cloudType,
        favoriteType,
        favoriteType,
        favoriteType,
        burstType,
        burstType,
        burstType};
    auto resultSet = this->galleryRdb_->QuerySql(this->SQL_QUERY_ALBUM_COUNT_BY_LPATH, params);
    CHECK_AND_RETURN_RET(resultSet != nullptr, {});
    std::vector<AlbumStatisticInfo> infoList;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        AlbumStatisticInfo info;
        info.count = GetInt32Val("count", resultSet);
        info.albumName = GetStringVal("albumName", resultSet);
        info.lPath = GetStringVal("lPath", resultSet);
        infoList.emplace_back(info);
    }
    resultSet->Close();
    return infoList;
}

bool GalleryMediaCountStatistic::HasLowQualityImage()
{
    std::string sql = "SELECT count(1) AS count FROM gallery_media WHERE (local_media_id != -1) AND \
        (COALESCE(storage_id, 0) IN (0, 65537)) AND relative_bucket_id NOT IN (SELECT DISTINCT relative_bucket_id FROM \
        garbage_album WHERE type = 1) AND _size = 0 AND photo_quality = 0";
    int count = BackupDatabaseUtils::QueryInt(galleryRdb_, sql, CUSTOM_COUNT);
    MEDIA_INFO_LOG("HasLowQualityImage count:%{public}d", count);
    return count > 0;
}

int32_t GalleryMediaCountStatistic::QueryGalleryAppTwinDataCount()
{
    std::string sql =
        "SELECT count(1) AS count FROM gallery_media WHERE _data LIKE '/storage/emulated/%' AND "
        "CAST (substr(_data, length('/storage/emulated/') + 1, 3) AS INTEGER) BETWEEN 128 AND 147 AND _size > 0";
    return BackupDatabaseUtils::QueryInt(galleryRdb_, sql, CUSTOM_COUNT);
}

int32_t GalleryMediaCountStatistic::QueryGalleryOnlyHDCDataCount(SearchCondition searchCondition)
{
    int32_t mediaType = searchCondition.GetMediaType();
    std::vector<NativeRdb::ValueObject> params = {mediaType, mediaType};
    return this->GetCount(this->SQL_ONLY_HDC_META_QUERY_COUNT, params);
}

int32_t GalleryMediaCountStatistic::QueryGallerySizeUnnormalDataCount()
{
    return BackupDatabaseUtils::QueryInt(galleryRdb_, SQL_SIZE_UNNORMAL_META_QUERY_COUNT, CUSTOM_COUNT);
}

AlbumMediaStatisticInfo GalleryMediaCountStatistic::GetAllStatInfo()
{
    AlbumMediaStatisticInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    // build the statistic info.
    info.totalCount = this->QueryGalleryAllCount(SearchCondition());
    info.imageCount = this->QueryGalleryAllCount(SearchCondition().SetMediaType(DUAL_MEDIA_TYPE_IMAGE));
    info.videoCount = this->QueryGalleryAllCount(SearchCondition().SetMediaType(DUAL_MEDIA_TYPE_VIDEO));
    info.hiddenCount = this->QueryGalleryAllCount(SearchCondition().SetHiddenType(DUAL_HIDDEN_TYPE_HIDDEN));
    info.trashedCount = this->QueryGalleryAllCount(SearchCondition().SetTrashedType(DUAL_TRASHED_TYPE_TRASHED));
    info.cloudCount = this->QueryGalleryAllCount(SearchCondition().SetCloudType(DUAL_CLOUD_TYPE_CLOUD));
    info.favoriteCount = this->QueryGalleryAllCount(SearchCondition().SetFavoriteType(DUAL_FAVORITE_TYPE_FAVORITE));
    info.burstTotalCount = this->QueryGalleryAllCount(SearchCondition().SetBurstType(DUAL_BURST_TYPE_ALL));
    info.burstCoverCount = this->QueryGalleryAllCount(SearchCondition().SetBurstType(DUAL_BURST_TYPE_COVER));
    // build the album name.
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    std::string albumName = "ALL";
    std::string lPath = "";
    int32_t period = 0;  // 0 - BEFORE, 1 - AFTER
    int32_t dbType = 0;  // 0 - GALLERY, 1 - MEDIA
    info.albumName = AlbumNameInfo()
                         .SetAlbumName(albumName)
                         .SetLPath(lPath)
                         .SetCostTime(costTime)
                         .SetPeriod(period)
                         .SetDbType(dbType)
                         .ToString();
    return info;
}

AlbumMediaStatisticInfo GalleryMediaCountStatistic::GetAllImageStatInfo()
{
    AlbumMediaStatisticInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    // build the statistic info.
    info.totalCount = this->QueryGalleryAllCount(SearchCondition().SetMediaType(DUAL_MEDIA_TYPE_IMAGE));
    info.imageCount = this->QueryGalleryAllCount(SearchCondition().SetMediaType(DUAL_MEDIA_TYPE_IMAGE));
    info.videoCount = 0;
    info.hiddenCount = this->QueryGalleryAllCount(
        SearchCondition().SetMediaType(DUAL_MEDIA_TYPE_IMAGE).SetHiddenType(DUAL_HIDDEN_TYPE_HIDDEN));
    info.trashedCount = this->QueryGalleryAllCount(
        SearchCondition().SetMediaType(DUAL_MEDIA_TYPE_IMAGE).SetTrashedType(DUAL_TRASHED_TYPE_TRASHED));
    info.cloudCount = this->QueryGalleryAllCount(
        SearchCondition().SetMediaType(DUAL_MEDIA_TYPE_IMAGE).SetCloudType(DUAL_CLOUD_TYPE_CLOUD));
    info.favoriteCount = this->QueryGalleryAllCount(
        SearchCondition().SetMediaType(DUAL_MEDIA_TYPE_IMAGE).SetFavoriteType(DUAL_FAVORITE_TYPE_FAVORITE));
    info.burstTotalCount = this->QueryGalleryAllCount(
        SearchCondition().SetMediaType(DUAL_MEDIA_TYPE_IMAGE).SetBurstType(DUAL_BURST_TYPE_ALL));
    info.burstCoverCount = this->QueryGalleryAllCount(
        SearchCondition().SetMediaType(DUAL_MEDIA_TYPE_IMAGE).SetBurstType(DUAL_BURST_TYPE_COVER));
    // build the album name.
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    std::string albumName = "ALL_IMAGE";
    std::string lPath = "";
    int32_t period = 0;  // 0 - BEFORE, 1 - AFTER
    int32_t dbType = 0;  // 0 - GALLERY, 1 - MEDIA
    info.albumName = AlbumNameInfo()
                         .SetAlbumName(albumName)
                         .SetLPath(lPath)
                         .SetCostTime(costTime)
                         .SetPeriod(period)
                         .SetDbType(dbType)
                         .ToString();
    return info;
}

AlbumMediaStatisticInfo GalleryMediaCountStatistic::GetAllVideoStatInfo()
{
    AlbumMediaStatisticInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    // build the statistic info.
    info.totalCount = this->QueryGalleryAllCount(SearchCondition().SetMediaType(DUAL_MEDIA_TYPE_VIDEO));
    info.imageCount = 0;
    info.videoCount = this->QueryGalleryAllCount(SearchCondition().SetMediaType(DUAL_MEDIA_TYPE_VIDEO));
    info.hiddenCount = this->QueryGalleryAllCount(
        SearchCondition().SetMediaType(DUAL_MEDIA_TYPE_VIDEO).SetHiddenType(DUAL_HIDDEN_TYPE_HIDDEN));
    info.trashedCount = this->QueryGalleryAllCount(
        SearchCondition().SetMediaType(DUAL_MEDIA_TYPE_VIDEO).SetTrashedType(DUAL_TRASHED_TYPE_TRASHED));
    info.cloudCount = this->QueryGalleryAllCount(
        SearchCondition().SetMediaType(DUAL_MEDIA_TYPE_VIDEO).SetCloudType(DUAL_CLOUD_TYPE_CLOUD));
    info.favoriteCount = this->QueryGalleryAllCount(
        SearchCondition().SetMediaType(DUAL_MEDIA_TYPE_VIDEO).SetFavoriteType(DUAL_FAVORITE_TYPE_FAVORITE));
    info.burstTotalCount = 0;
    info.burstCoverCount = 0;
    // build the album name.
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    std::string albumName = "ALL_VIDEO";
    std::string lPath = "";
    int32_t period = 0;  // 0 - BEFORE, 1 - AFTER
    int32_t dbType = 0;  // 0 - GALLERY, 1 - MEDIA
    info.albumName = AlbumNameInfo()
                         .SetAlbumName(albumName)
                         .SetLPath(lPath)
                         .SetCostTime(costTime)
                         .SetPeriod(period)
                         .SetDbType(dbType)
                         .ToString();
    return info;
}

AlbumMediaStatisticInfo GalleryMediaCountStatistic::GetAllRestoreStatInfo()
{
    AlbumMediaStatisticInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    // build the statistic info.
    info.totalCount = this->GetGalleryMediaAllRestoreCount(SearchCondition());
    info.imageCount = this->GetGalleryMediaAllRestoreCount(SearchCondition().SetMediaType(DUAL_MEDIA_TYPE_IMAGE));
    info.videoCount = this->GetGalleryMediaAllRestoreCount(SearchCondition().SetMediaType(DUAL_MEDIA_TYPE_VIDEO));
    info.hiddenCount = this->GetGalleryMediaAllRestoreCount(SearchCondition().SetHiddenType(DUAL_HIDDEN_TYPE_HIDDEN));
    info.trashedCount =
        this->GetGalleryMediaAllRestoreCount(SearchCondition().SetTrashedType(DUAL_TRASHED_TYPE_TRASHED));
    info.cloudCount = this->GetGalleryMediaAllRestoreCount(SearchCondition().SetCloudType(DUAL_CLOUD_TYPE_CLOUD));
    info.favoriteCount =
        this->GetGalleryMediaAllRestoreCount(SearchCondition().SetFavoriteType(DUAL_FAVORITE_TYPE_FAVORITE));
    info.burstTotalCount = this->GetGalleryMediaAllRestoreCount(SearchCondition().SetBurstType(DUAL_BURST_TYPE_ALL));
    info.burstCoverCount = this->GetGalleryMediaAllRestoreCount(SearchCondition().SetBurstType(DUAL_BURST_TYPE_COVER));
    // build the album name.
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    std::string albumName = "ALL_RESTORE";
    std::string lPath = "";
    int32_t period = 0;  // 0 - BEFORE, 1 - AFTER
    int32_t dbType = 0;  // 0 - GALLERY, 1 - MEDIA
    info.albumName = AlbumNameInfo()
                         .SetAlbumName(albumName)
                         .SetLPath(lPath)
                         .SetCostTime(costTime)
                         .SetPeriod(period)
                         .SetDbType(dbType)
                         .ToString();
    return info;
}

AlbumMediaStatisticInfo GalleryMediaCountStatistic::GetAllRestoreImageStatInfo()
{
    AlbumMediaStatisticInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    // build the statistic info.
    info.totalCount = this->GetGalleryMediaAllRestoreCount(SearchCondition().SetMediaType(DUAL_MEDIA_TYPE_IMAGE));
    info.imageCount = this->GetGalleryMediaAllRestoreCount(SearchCondition().SetMediaType(DUAL_MEDIA_TYPE_IMAGE));
    info.videoCount = 0;
    info.hiddenCount = this->GetGalleryMediaAllRestoreCount(
        SearchCondition().SetMediaType(DUAL_MEDIA_TYPE_IMAGE).SetHiddenType(DUAL_HIDDEN_TYPE_HIDDEN));
    info.trashedCount = this->GetGalleryMediaAllRestoreCount(
        SearchCondition().SetMediaType(DUAL_MEDIA_TYPE_IMAGE).SetTrashedType(DUAL_TRASHED_TYPE_TRASHED));
    info.cloudCount = this->GetGalleryMediaAllRestoreCount(
        SearchCondition().SetMediaType(DUAL_MEDIA_TYPE_IMAGE).SetCloudType(DUAL_CLOUD_TYPE_CLOUD));
    info.favoriteCount = this->GetGalleryMediaAllRestoreCount(
        SearchCondition().SetMediaType(DUAL_MEDIA_TYPE_IMAGE).SetFavoriteType(DUAL_FAVORITE_TYPE_FAVORITE));
    info.burstTotalCount = this->GetGalleryMediaAllRestoreCount(
        SearchCondition().SetMediaType(DUAL_MEDIA_TYPE_IMAGE).SetBurstType(DUAL_BURST_TYPE_ALL));
    info.burstCoverCount = this->GetGalleryMediaAllRestoreCount(
        SearchCondition().SetMediaType(DUAL_MEDIA_TYPE_IMAGE).SetBurstType(DUAL_BURST_TYPE_COVER));
    // build the album name.
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    std::string albumName = "ALL_RESTORE_IMAGE";
    std::string lPath = "";
    int32_t period = 0;  // 0 - BEFORE, 1 - AFTER
    int32_t dbType = 0;  // 0 - GALLERY, 1 - MEDIA
    info.albumName = AlbumNameInfo()
                         .SetAlbumName(albumName)
                         .SetLPath(lPath)
                         .SetCostTime(costTime)
                         .SetPeriod(period)
                         .SetDbType(dbType)
                         .ToString();
    return info;
}

AlbumMediaStatisticInfo GalleryMediaCountStatistic::GetAllRestoreVideoStatInfo()
{
    AlbumMediaStatisticInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    // build the statistic info.
    info.totalCount = this->GetGalleryMediaAllRestoreCount(SearchCondition().SetMediaType(DUAL_MEDIA_TYPE_VIDEO));
    info.imageCount = 0;
    info.videoCount = this->GetGalleryMediaAllRestoreCount(SearchCondition().SetMediaType(DUAL_MEDIA_TYPE_VIDEO));
    info.hiddenCount = this->GetGalleryMediaAllRestoreCount(
        SearchCondition().SetMediaType(DUAL_MEDIA_TYPE_VIDEO).SetHiddenType(DUAL_HIDDEN_TYPE_HIDDEN));
    info.trashedCount = this->GetGalleryMediaAllRestoreCount(
        SearchCondition().SetMediaType(DUAL_MEDIA_TYPE_VIDEO).SetTrashedType(DUAL_TRASHED_TYPE_TRASHED));
    info.cloudCount = this->GetGalleryMediaAllRestoreCount(
        SearchCondition().SetMediaType(DUAL_MEDIA_TYPE_VIDEO).SetCloudType(DUAL_CLOUD_TYPE_CLOUD));
    info.favoriteCount = this->GetGalleryMediaAllRestoreCount(
        SearchCondition().SetMediaType(DUAL_MEDIA_TYPE_VIDEO).SetFavoriteType(DUAL_FAVORITE_TYPE_FAVORITE));
    info.burstTotalCount = 0;
    info.burstCoverCount = 0;
    // build the album name.
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    std::string albumName = "ALL_RESTORE_VIDEO";
    std::string lPath = "";
    int32_t period = 0;  // 0 - BEFORE, 1 - AFTER
    int32_t dbType = 0;  // 0 - GALLERY, 1 - MEDIA
    info.albumName = AlbumNameInfo()
                         .SetAlbumName(albumName)
                         .SetLPath(lPath)
                         .SetCostTime(costTime)
                         .SetPeriod(period)
                         .SetDbType(dbType)
                         .ToString();
    return info;
}

AlbumMediaStatisticInfo GalleryMediaCountStatistic::GetSdCardStatInfo()
{
    AlbumMediaStatisticInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    // build the statistic info.
    info.totalCount = this->QueryGallerySdCardCount(SearchCondition());
    info.imageCount = this->QueryGallerySdCardCount(SearchCondition().SetMediaType(DUAL_MEDIA_TYPE_IMAGE));
    info.videoCount = this->QueryGallerySdCardCount(SearchCondition().SetMediaType(DUAL_MEDIA_TYPE_VIDEO));
    info.hiddenCount = this->QueryGallerySdCardCount(SearchCondition().SetHiddenType(DUAL_HIDDEN_TYPE_HIDDEN));
    info.trashedCount = this->QueryGallerySdCardCount(SearchCondition().SetTrashedType(DUAL_TRASHED_TYPE_TRASHED));
    info.cloudCount = this->QueryGallerySdCardCount(SearchCondition().SetCloudType(DUAL_CLOUD_TYPE_CLOUD));
    info.favoriteCount = this->QueryGallerySdCardCount(SearchCondition().SetFavoriteType(DUAL_FAVORITE_TYPE_FAVORITE));
    info.burstTotalCount = this->QueryGallerySdCardCount(SearchCondition().SetBurstType(DUAL_BURST_TYPE_ALL));
    info.burstCoverCount = this->QueryGallerySdCardCount(SearchCondition().SetBurstType(DUAL_BURST_TYPE_COVER));
    // build the album name.
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    std::string albumName = "SD_CARD";
    std::string lPath = "";
    int32_t period = 0;  // 0 - BEFORE, 1 - AFTER
    int32_t dbType = 0;  // 0 - GALLERY, 1 - MEDIA
    info.albumName = AlbumNameInfo()
                         .SetAlbumName(albumName)
                         .SetLPath(lPath)
                         .SetCostTime(costTime)
                         .SetPeriod(period)
                         .SetDbType(dbType)
                         .ToString();
    return info;
}

AlbumMediaStatisticInfo GalleryMediaCountStatistic::GetDuplicateStatInfo()
{
    int32_t duplicateDataCount;
    int32_t duplicateDataTotal;
    BackupDatabaseUtils::QueryGalleryDuplicateDataCount(this->galleryRdb_, duplicateDataCount, duplicateDataTotal);
    AlbumMediaStatisticInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    // build the statistic info.
    info.totalCount = duplicateDataTotal;
    // build the album name.
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    std::string albumName = "DUPLICATE";
    std::string lPath = "";
    int32_t period = 0;  // 0 - BEFORE, 1 - AFTER
    int32_t dbType = 0;  // 0 - GALLERY, 1 - MEDIA
    info.albumName = AlbumNameInfo()
                         .SetAlbumName(albumName)
                         .SetLPath(lPath)
                         .SetCostTime(costTime)
                         .SetPeriod(period)
                         .SetDbType(dbType)
                         .ToString();
    return info;
}

AlbumMediaStatisticInfo GalleryMediaCountStatistic::GetAppTwinStatInfo()
{
    AlbumMediaStatisticInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    // build the statistic info.
    info.totalCount = this->QueryGalleryAppTwinDataCount();
    // build the album name.
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    std::string albumName = "APP_TWIN";
    std::string lPath = "";
    int32_t period = 0;  // 0 - BEFORE, 1 - AFTER
    int32_t dbType = 0;  // 0 - GALLERY, 1 - MEDIA
    info.albumName = AlbumNameInfo()
                         .SetAlbumName(albumName)
                         .SetLPath(lPath)
                         .SetCostTime(costTime)
                         .SetPeriod(period)
                         .SetDbType(dbType)
                         .ToString();
    return info;
}

AlbumMediaStatisticInfo GalleryMediaCountStatistic::GetOnlyHDCInfo()
{
    AlbumMediaStatisticInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    // build the statistic info.
    info.totalCount = this->QueryGalleryOnlyHDCDataCount(SearchCondition());
    info.imageCount = this->QueryGalleryOnlyHDCDataCount(SearchCondition().SetMediaType(DUAL_MEDIA_TYPE_IMAGE));
    info.videoCount = this->QueryGalleryOnlyHDCDataCount(SearchCondition().SetMediaType(DUAL_MEDIA_TYPE_VIDEO));
    // build the album name.
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    std::string albumName = "ONLY_HDC";
    std::string lPath = "";
    int32_t period = 0;  // 0 - BEFORE, 1 - AFTER
    int32_t dbType = 0;  // 0 - GALLERY, 1 - MEDIA
    info.albumName = AlbumNameInfo()
                         .SetAlbumName(albumName)
                         .SetLPath(lPath)
                         .SetCostTime(costTime)
                         .SetPeriod(period)
                         .SetDbType(dbType)
                         .ToString();
    return info;
}

AlbumMediaStatisticInfo GalleryMediaCountStatistic::GetSizeUnnormalInfo()
{
    AlbumMediaStatisticInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    // build the statistic info.
    info.totalCount = this->QueryGallerySizeUnnormalDataCount();
    // build the album name.
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    std::string albumName = "SIZE_UNNORMAL";
    std::string lPath = "";
    int32_t period = 0;  // 0 - BEFORE, 1 - AFTER
    int32_t dbType = 0;  // 0 - GALLERY, 1 - MEDIA
    info.albumName = AlbumNameInfo()
                         .SetAlbumName(albumName)
                         .SetLPath(lPath)
                         .SetCostTime(costTime)
                         .SetPeriod(period)
                         .SetDbType(dbType)
                         .ToString();
    return info;
}

AlbumMediaStatisticInfo GalleryMediaCountStatistic::GetImageAlbumInfo()
{
    AlbumMediaStatisticInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    // build the statistic info.
    SearchCondition defaultCondition =
        SearchCondition().SetHiddenType(DUAL_HIDDEN_TYPE_NOT_HIDDEN).SetTrashedType(DUAL_TRASHED_TYPE_NOT_TRASHED);
    info.totalCount = this->QueryAlbumGalleryCount(SearchCondition(defaultCondition));
    info.imageCount =
        this->QueryAlbumGalleryCount(SearchCondition(defaultCondition).SetMediaType(DUAL_MEDIA_TYPE_IMAGE));
    info.videoCount =
        this->QueryAlbumGalleryCount(SearchCondition(defaultCondition).SetMediaType(DUAL_MEDIA_TYPE_VIDEO));
    info.hiddenCount = 0;
    info.trashedCount = 0;
    info.cloudCount =
        this->QueryAlbumGalleryCount(SearchCondition(defaultCondition).SetCloudType(DUAL_CLOUD_TYPE_CLOUD));
    info.favoriteCount =
        this->QueryAlbumGalleryCount(SearchCondition(defaultCondition).SetFavoriteType(DUAL_FAVORITE_TYPE_FAVORITE));
    info.burstTotalCount =
        this->QueryAlbumGalleryCount(SearchCondition(defaultCondition).SetBurstType(DUAL_BURST_TYPE_ALL));
    info.burstCoverCount =
        this->QueryAlbumGalleryCount(SearchCondition(defaultCondition).SetBurstType(DUAL_BURST_TYPE_COVER));
    // build the album name.
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    std::string albumName = "所有照片";
    std::string lPath = "";
    int32_t period = 0;  // 0 - BEFORE, 1 - AFTER
    int32_t dbType = 0;  // 0 - GALLERY, 1 - MEDIA
    info.albumName = AlbumNameInfo()
                         .SetAlbumName(albumName)
                         .SetLPath(lPath)
                         .SetCostTime(costTime)
                         .SetPeriod(period)
                         .SetDbType(dbType)
                         .ToString();
    return info;
}

AlbumMediaStatisticInfo GalleryMediaCountStatistic::GetVideoAlbumInfo()
{
    AlbumMediaStatisticInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    // build the statistic info.
    SearchCondition defaultCondition = SearchCondition()
                                           .SetMediaType(DUAL_MEDIA_TYPE_VIDEO)
                                           .SetHiddenType(DUAL_HIDDEN_TYPE_NOT_HIDDEN)
                                           .SetTrashedType(DUAL_TRASHED_TYPE_NOT_TRASHED);
    info.totalCount = this->QueryAlbumAllVideoCount(SearchCondition(defaultCondition));
    info.imageCount = 0;
    info.videoCount = this->QueryAlbumAllVideoCount(SearchCondition(defaultCondition));
    info.hiddenCount = 0;
    info.trashedCount = 0;
    info.cloudCount =
        this->QueryAlbumAllVideoCount(SearchCondition(defaultCondition).SetCloudType(DUAL_CLOUD_TYPE_CLOUD));
    info.favoriteCount =
        this->QueryAlbumAllVideoCount(SearchCondition(defaultCondition).SetFavoriteType(DUAL_FAVORITE_TYPE_FAVORITE));
    info.burstTotalCount = 0;
    info.burstCoverCount = 0;
    // build the album name.
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    std::string albumName = "视频";
    std::string lPath = "";
    int32_t period = 0;  // 0 - BEFORE, 1 - AFTER
    int32_t dbType = 0;  // 0 - GALLERY, 1 - MEDIA
    info.albumName = AlbumNameInfo()
                         .SetAlbumName(albumName)
                         .SetLPath(lPath)
                         .SetCostTime(costTime)
                         .SetPeriod(period)
                         .SetDbType(dbType)
                         .ToString();
    return info;
}

AlbumMediaStatisticInfo GalleryMediaCountStatistic::GetFavoriteAlbumStatInfo()
{
    AlbumMediaStatisticInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    // build the statistic info.
    SearchCondition defaultCondition = SearchCondition()
                                           .SetHiddenType(DUAL_HIDDEN_TYPE_NOT_HIDDEN)
                                           .SetTrashedType(DUAL_TRASHED_TYPE_NOT_TRASHED)
                                           .SetFavoriteType(DUAL_FAVORITE_TYPE_FAVORITE);
    info.totalCount = this->QueryAlbumGalleryCount(SearchCondition(defaultCondition));
    info.imageCount =
        this->QueryAlbumGalleryCount(SearchCondition(defaultCondition).SetMediaType(DUAL_MEDIA_TYPE_IMAGE));
    info.videoCount =
        this->QueryAlbumGalleryCount(SearchCondition(defaultCondition).SetMediaType(DUAL_MEDIA_TYPE_VIDEO));
    info.hiddenCount = 0;
    info.trashedCount = 0;
    info.cloudCount =
        this->QueryAlbumGalleryCount(SearchCondition(defaultCondition).SetCloudType(DUAL_CLOUD_TYPE_CLOUD));
    info.favoriteCount = this->QueryAlbumGalleryCount(SearchCondition(defaultCondition));
    info.burstTotalCount =
        this->QueryAlbumGalleryCount(SearchCondition(defaultCondition).SetBurstType(DUAL_BURST_TYPE_ALL));
    info.burstCoverCount =
        this->QueryAlbumGalleryCount(SearchCondition(defaultCondition).SetBurstType(DUAL_BURST_TYPE_COVER));
    // build the album name.
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    std::string albumName = "收藏";
    std::string lPath = "";
    int32_t period = 0;  // 0 - BEFORE, 1 - AFTER
    int32_t dbType = 0;  // 0 - GALLERY, 1 - MEDIA
    info.albumName = AlbumNameInfo()
                         .SetAlbumName(albumName)
                         .SetLPath(lPath)
                         .SetCostTime(costTime)
                         .SetPeriod(period)
                         .SetDbType(dbType)
                         .ToString();
    return info;
}

AlbumMediaStatisticInfo GalleryMediaCountStatistic::GetTrashedAlbumStatInfo()
{
    AlbumMediaStatisticInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    // build the statistic info.
    SearchCondition defaultCondition =
        SearchCondition().SetHiddenType(DUAL_HIDDEN_TYPE_NOT_HIDDEN).SetTrashedType(DUAL_TRASHED_TYPE_TRASHED);
    info.totalCount = this->QueryAlbumGalleryCount(SearchCondition(defaultCondition));
    info.imageCount =
        this->QueryAlbumGalleryCount(SearchCondition(defaultCondition).SetMediaType(DUAL_MEDIA_TYPE_IMAGE));
    info.videoCount =
        this->QueryAlbumGalleryCount(SearchCondition(defaultCondition).SetMediaType(DUAL_MEDIA_TYPE_VIDEO));
    info.hiddenCount = 0;
    info.trashedCount = this->QueryAlbumGalleryCount(SearchCondition(defaultCondition));
    info.cloudCount =
        this->QueryAlbumGalleryCount(SearchCondition(defaultCondition).SetCloudType(DUAL_CLOUD_TYPE_CLOUD));
    info.favoriteCount = 0;
    info.burstTotalCount =
        this->QueryAlbumGalleryCount(SearchCondition(defaultCondition).SetBurstType(DUAL_BURST_TYPE_ALL));
    info.burstCoverCount =
        this->QueryAlbumGalleryCount(SearchCondition(defaultCondition).SetBurstType(DUAL_BURST_TYPE_COVER));
    // build the album name.
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    std::string albumName = "回收站";
    std::string lPath = "";
    int32_t period = 0;  // 0 - BEFORE, 1 - AFTER
    int32_t dbType = 0;  // 0 - GALLERY, 1 - MEDIA
    info.albumName = AlbumNameInfo()
                         .SetAlbumName(albumName)
                         .SetLPath(lPath)
                         .SetCostTime(costTime)
                         .SetPeriod(period)
                         .SetDbType(dbType)
                         .ToString();
    return info;
}

std::vector<AlbumMediaStatisticInfo> GalleryMediaCountStatistic::GetAlbumInfoByName(const std::string &albumName)
{
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    // build the statistic info.
    SearchCondition defaultCondition =
        SearchCondition().SetHiddenType(DUAL_HIDDEN_TYPE_NOT_HIDDEN).SetTrashedType(DUAL_TRASHED_TYPE_NOT_TRASHED);
    std::vector<AlbumStatisticInfo> totalCountInfoList =
        this->QueryAlbumCountByName(albumName, SearchCondition(defaultCondition));
    std::vector<AlbumStatisticInfo> imageCountInfoList =
        this->QueryAlbumCountByName(albumName, SearchCondition(defaultCondition).SetMediaType(DUAL_MEDIA_TYPE_IMAGE));
    std::vector<AlbumStatisticInfo> videoCountInfoList =
        this->QueryAlbumCountByName(albumName, SearchCondition(defaultCondition).SetMediaType(DUAL_MEDIA_TYPE_VIDEO));
    std::vector<AlbumStatisticInfo> cloudCountInfoList =
        this->QueryAlbumCountByName(albumName, SearchCondition(defaultCondition).SetCloudType(DUAL_CLOUD_TYPE_CLOUD));
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    // Parse the statistic info.
    std::unordered_map<std::string, AlbumMediaStatisticInfo> albumInfoMap;
    StatisticProcessor()
        .ParseTotalCount(albumInfoMap, totalCountInfoList)
        .ParseImageCount(albumInfoMap, imageCountInfoList)
        .ParseVideoCount(albumInfoMap, videoCountInfoList)
        .ParseCloudCount(albumInfoMap, cloudCountInfoList);
    std::vector<AlbumMediaStatisticInfo> albumInfoList;
    for (const auto &iter : albumInfoMap) {
        AlbumMediaStatisticInfo info = iter.second;
        info.sceneCode = this->sceneCode_;
        info.taskId = this->taskId_;
        info.albumName = AlbumNameInfo()
                             .SetAlbumName(info.albumName)
                             .SetLPath(info.lPath)
                             .SetCostTime(costTime)
                             .SetPeriod(0)  // 0 - BEFORE, 1 - AFTER
                             .SetDbType(0)  // 0 - GALLERY, 1 - MEDIA
                             .ToString();
        albumInfoList.emplace_back(info);
    }
    return albumInfoList;
}

std::vector<AlbumMediaStatisticInfo> GalleryMediaCountStatistic::GetAlbumInfoByLPath(const std::string &lPath)
{
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    // build the statistic info.
    SearchCondition defaultCondition =
        SearchCondition().SetHiddenType(DUAL_HIDDEN_TYPE_NOT_HIDDEN).SetTrashedType(DUAL_TRASHED_TYPE_NOT_TRASHED);
    std::vector<AlbumStatisticInfo> totalCountInfoList =
        this->QueryAlbumCountByLPath(lPath, SearchCondition(defaultCondition));
    std::vector<AlbumStatisticInfo> imageCountInfoList =
        this->QueryAlbumCountByLPath(lPath, SearchCondition(defaultCondition).SetMediaType(DUAL_MEDIA_TYPE_IMAGE));
    std::vector<AlbumStatisticInfo> videoCountInfoList =
        this->QueryAlbumCountByLPath(lPath, SearchCondition(defaultCondition).SetMediaType(DUAL_MEDIA_TYPE_VIDEO));
    std::vector<AlbumStatisticInfo> cloudCountInfoList =
        this->QueryAlbumCountByLPath(lPath, SearchCondition(defaultCondition).SetCloudType(DUAL_CLOUD_TYPE_CLOUD));
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    // Parse the statistic info.
    std::unordered_map<std::string, AlbumMediaStatisticInfo> albumInfoMap;
    StatisticProcessor()
        .ParseTotalCount(albumInfoMap, totalCountInfoList)
        .ParseImageCount(albumInfoMap, imageCountInfoList)
        .ParseVideoCount(albumInfoMap, videoCountInfoList)
        .ParseCloudCount(albumInfoMap, cloudCountInfoList);
    std::vector<AlbumMediaStatisticInfo> albumInfoList;
    for (const auto &iter : albumInfoMap) {
        AlbumMediaStatisticInfo info = iter.second;
        info.sceneCode = this->sceneCode_;
        info.taskId = this->taskId_;
        info.albumName = AlbumNameInfo()
                             .SetAlbumName(info.albumName)
                             .SetLPath(info.lPath)
                             .SetCostTime(costTime)
                             .SetPeriod(0)  // 0 - BEFORE, 1 - AFTER
                             .SetDbType(0)  // 0 - GALLERY, 1 - MEDIA
                             .ToString();
        albumInfoList.emplace_back(info);
    }
    return albumInfoList;
}

AlbumMediaStatisticInfo GalleryMediaCountStatistic::GetGalleryAlbumCountInfo()
{
    AlbumMediaStatisticInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    // build the statistic info.
    info.totalCount = this->GetCount(this->SQL_QUERY_GALLERY_ALBUM_COUNT);
    // build the album name.
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    std::string albumName = "相册数量";
    std::string lPath = "";
    int32_t period = 0;  // 0 - BEFORE, 1 - AFTER
    int32_t dbType = 0;  // 0 - GALLERY, 1 - MEDIA
    info.albumName = AlbumNameInfo()
                         .SetAlbumName(albumName)
                         .SetLPath(lPath)
                         .SetCostTime(costTime)
                         .SetPeriod(period)
                         .SetDbType(dbType)
                         .ToString();
    return info;
}

AlbumMediaStatisticInfo GalleryMediaCountStatistic::GetLiveStatInfo()
{
    AlbumMediaStatisticInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    // build the statistic info.
    info.totalCount = this->QueryLiveCount(DUAL_SEARCH_TYPE_ALL, DUAL_MEDIA_TYPE_ALL);
    info.cloudCount = this->QueryLiveCount(DUAL_SEARCH_TYPE_CLOUD, DUAL_MEDIA_TYPE_ALL);
    // build the album name.
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    std::string albumName = "动态照片";
    std::string lPath = "";
    int32_t period = 0;  // 0 - BEFORE, 1 - AFTER
    int32_t dbType = 0;  // 0 - GALLERY, 1 - MEDIA
    info.albumName = AlbumNameInfo()
                         .SetAlbumName(albumName)
                         .SetLPath(lPath)
                         .SetCostTime(costTime)
                         .SetPeriod(period)
                         .SetDbType(dbType)
                         .ToString();
    return info;
}

AlbumMediaStatisticInfo GalleryMediaCountStatistic::GetTempInfo()
{
    AlbumMediaStatisticInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    // build the statistic info.
    info.totalCount = this->QueryTempCount(DUAL_SEARCH_TYPE_ALL, DUAL_MEDIA_TYPE_ALL);
    info.imageCount = this->QueryTempCount(DUAL_SEARCH_TYPE_ALL, DUAL_MEDIA_TYPE_IMAGE);
    info.videoCount = this->QueryTempCount(DUAL_SEARCH_TYPE_ALL, DUAL_MEDIA_TYPE_VIDEO);
    info.cloudCount = this->QueryTempCount(DUAL_SEARCH_TYPE_CLOUD, DUAL_MEDIA_TYPE_ALL);
    // build the album name.
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    std::string albumName = "应用缓存文件";
    std::string lPath = "";
    int32_t period = 0;  // 0 - BEFORE, 1 - AFTER
    int32_t dbType = 0;  // 0 - GALLERY, 1 - MEDIA
    info.albumName = AlbumNameInfo()
                         .SetAlbumName(albumName)
                         .SetLPath(lPath)
                         .SetCostTime(costTime)
                         .SetPeriod(period)
                         .SetDbType(dbType)
                         .ToString();
    return info;
}

AlbumMediaStatisticInfo GalleryMediaCountStatistic::GetNotSyncInfo()
{
    AlbumMediaStatisticInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    // build the statistic info.
    info.totalCount = this->GetCount(SQL_QUERY_NOT_SYNC_COUNT);
    // build the album name.
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    std::string albumName = "媒体库未同步";
    std::string lPath = "";
    int32_t period = 0;  // 0 - BEFORE, 1 - AFTER
    int32_t dbType = 0;  // 0 - GALLERY, 1 - MEDIA
    info.albumName = AlbumNameInfo()
                         .SetAlbumName(albumName)
                         .SetLPath(lPath)
                         .SetCostTime(costTime)
                         .SetPeriod(period)
                         .SetDbType(dbType)
                         .ToString();
    return info;
}
}  // namespace OHOS::Media