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
#include "photos_count_statistic.h"

#include "media_log.h"
#include "backup_database_utils.h"
#include "backup_const.h"
#include "media_backup_report_data_type.h"
#include "statistic_processor.h"
#include "media_file_utils.h"

namespace OHOS::Media {
std::vector<AlbumMediaStatisticInfo> PhotosCountStatistic::Load()
{
    if (this->mediaLibraryRdb_ == nullptr) {
        MEDIA_ERR_LOG("mediaLibraryRdb_ is nullptr, Maybe init failed.");
        return {};
    }
    std::vector<AlbumMediaStatisticInfo> infoList = {
        // data-transfer statistic info.
        this->GetAllStatInfo(),
        this->GetAllImageStatInfo(),
        this->GetAllVideoStatInfo(),
        this->GetAllRestoreStatInfo(),
        this->GetAllRestoreImageStatInfo(),
        this->GetAllRestoreVideoStatInfo(),
        // other statistic info.
        this->GetLiveStatInfo(),
        this->GetGalleryAlbumCountInfo(),
        this->GetGalleryDeletedAlbumCountInfo(),
        // statistic info.
        this->GetImageAlbumInfo(),
        this->GetVideoAlbumInfo(),
        // folder info.
        this->GetFavoriteAlbumStatInfo(),
        this->GetTrashedAlbumStatInfo(),
        this->GetHiddenAlbumStatInfo(),
    };
    // key album of album_plugin.
    std::vector<AlbumMediaStatisticInfo> cameraAlbumList = this->GetAlbumInfoByName("相机");
    infoList.insert(infoList.end(), cameraAlbumList.begin(), cameraAlbumList.end());
    std::vector<AlbumMediaStatisticInfo> screenShotAlbumList = this->GetAlbumInfoByName("截图");
    infoList.insert(infoList.end(), screenShotAlbumList.begin(), screenShotAlbumList.end());
    std::vector<AlbumMediaStatisticInfo> screenRecordAlbumList = this->GetAlbumInfoByName("屏幕录制");
    infoList.insert(infoList.end(), screenRecordAlbumList.begin(), screenRecordAlbumList.end());
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

int32_t PhotosCountStatistic::GetCount(const std::string &query, const std::vector<NativeRdb::ValueObject> &args)
{
    return BackupDatabaseUtils::QueryInt(this->mediaLibraryRdb_, query, CUSTOM_COUNT, args);
}

/**
 * @brief Get the row count of media_library storage in SD card.
 * @param mediaType - 0 all, 1 picture, 3 video
 */
int32_t PhotosCountStatistic::QueryTotalCount(SearchCondition searchCondition)
{
    CHECK_AND_RETURN_RET_LOG(this->mediaLibraryRdb_ != nullptr, 0, "Media_Restore: mediaLibraryRdb_ is null.");
    int32_t mediaType = searchCondition.GetMediaType();
    int32_t hiddenType = searchCondition.GetHiddenType();
    int32_t trashedType = searchCondition.GetTrashedType();
    int32_t cloudType = searchCondition.GetCloudType();
    int32_t favoriteType = searchCondition.GetFavoriteType();
    int32_t burstType = searchCondition.GetBurstType();
    int32_t lakeType = searchCondition.GetLakeType();
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
        burstType,
        lakeType,
        lakeType};
    return this->GetCount(this->SQL_PHOTOS_ALL_TOTAL_COUNT, params);
}

/**
 * @brief Get the row count of media_library storage in SD card.
 * @param mediaType - 0 all, 1 picture, 3 video
 */
int32_t PhotosCountStatistic::QueryAllRestoreCount(SearchCondition searchCondition)
{
    CHECK_AND_RETURN_RET_LOG(this->mediaLibraryRdb_ != nullptr, 0, "Media_Restore: mediaLibraryRdb_ is null.");
    int32_t mediaType = searchCondition.GetMediaType();
    int32_t hiddenType = searchCondition.GetHiddenType();
    int32_t trashedType = searchCondition.GetTrashedType();
    int32_t cloudType = searchCondition.GetCloudType();
    int32_t favoriteType = searchCondition.GetFavoriteType();
    int32_t burstType = searchCondition.GetBurstType();
    int32_t lakeType = searchCondition.GetLakeType();
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
        burstType,
        lakeType,
        lakeType};
    return this->GetCount(this->SQL_PHOTOS_ALL_RESTORE_COUNT, params);
}

/**
 * @brief Get the row count of media_library storage in SD card.
 * @param mediaType - 0 all, 1 picture, 3 video
 */
int32_t PhotosCountStatistic::QueryPicturesTotalCount(SearchCondition searchCondition)
{
    CHECK_AND_RETURN_RET_LOG(this->mediaLibraryRdb_ != nullptr, 0, "Media_Restore: mediaLibraryRdb_ is null.");
    int32_t mediaType = searchCondition.GetMediaType();
    int32_t hiddenType = searchCondition.GetHiddenType();
    int32_t trashedType = searchCondition.GetTrashedType();
    int32_t cloudType = searchCondition.GetCloudType();
    int32_t favoriteType = searchCondition.GetFavoriteType();
    int32_t burstType = searchCondition.GetBurstType();
    int32_t lakeType = searchCondition.GetLakeType();
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
        burstType,
        lakeType,
        lakeType};
    return this->GetCount(this->SQL_PHOTOS_PICTURES_TOTAL_COUNT, params);
}

/**
 * @brief Get the row count of media_library.
 * @param searchType - 0 all, 1 cloud
 * @param mediaType - 0 all, 1 picture, 2 video
 */
std::vector<AlbumStatisticInfo> PhotosCountStatistic::QueryAlbumCountByName(
    const std::string &albumName, SearchCondition searchCondition)
{
    CHECK_AND_RETURN_RET_LOG(this->mediaLibraryRdb_ != nullptr, {}, "Media_Restore: mediaLibraryRdb_ is null.");
    int32_t mediaType = searchCondition.GetMediaType();
    int32_t hiddenType = searchCondition.GetHiddenType();
    int32_t trashedType = searchCondition.GetTrashedType();
    int32_t cloudType = searchCondition.GetCloudType();
    int32_t favoriteType = searchCondition.GetFavoriteType();
    int32_t burstType = searchCondition.GetBurstType();
    int32_t lakeType = searchCondition.GetLakeType();
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
        burstType,
        lakeType,
        lakeType};
    auto resultSet = this->mediaLibraryRdb_->QuerySql(this->SQL_PHOTOS_COUNT_BY_ALBUM_NAME, params);
    CHECK_AND_RETURN_RET(resultSet != nullptr, {});
    std::vector<AlbumStatisticInfo> infoList;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        AlbumStatisticInfo info;
        info.albumName = GetStringVal("albumName", resultSet);
        info.count = GetInt32Val("count", resultSet);
        info.lPath = GetStringVal("lpath", resultSet);
        infoList.emplace_back(info);
    }
    resultSet->Close();
    return infoList;
}

/**
 * @brief Get the row count of media_library.
 * @param mediaType - 0 all, 2 cloud
 */
int32_t PhotosCountStatistic::QueryLiveCount(int32_t searchType)
{
    std::vector<NativeRdb::ValueObject> params = {searchType};
    CHECK_AND_RETURN_RET_LOG(this->mediaLibraryRdb_ != nullptr, 0, "Media_Restore: mediaLibraryRdb_ is null.");
    return this->GetCount(this->SQL_PHOTOS_LIVE_COUNT, params);
}

AlbumMediaStatisticInfo PhotosCountStatistic::GetAllStatInfo()
{
    AlbumMediaStatisticInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    // build the statistic info.
    info.totalCount = this->QueryTotalCount(SearchCondition());
    info.imageCount = this->QueryTotalCount(SearchCondition().SetMediaType(SINGLE_MEDIA_TYPE_IMAGE));
    info.videoCount = this->QueryTotalCount(SearchCondition().SetMediaType(SINGLE_MEDIA_TYPE_VIDEO));
    info.hiddenCount = this->QueryTotalCount(SearchCondition().SetHiddenType(SINGLE_HIDDEN_TYPE_HIDDEN));
    info.trashedCount = this->QueryTotalCount(SearchCondition().SetTrashedType(SINGLE_TRASHED_TYPE_TRASHED));
    info.cloudCount = this->QueryTotalCount(SearchCondition().SetCloudType(SINGLE_CLOUD_TYPE_CLOUD));
    info.favoriteCount = this->QueryTotalCount(SearchCondition().SetFavoriteType(SINGLE_FAVORITE_TYPE_FAVORITE));
    info.burstTotalCount = this->QueryTotalCount(SearchCondition().SetBurstType(SINGLE_BURST_TYPE_ALL));
    info.burstCoverCount = this->QueryTotalCount(SearchCondition().SetBurstType(SINGLE_BURST_TYPE_COVER));
    info.innerEastLakeCount = this->QueryTotalCount(SearchCondition().SetLakeType(SINGLE_LAKE_TYPE_INNER));
    // build the album name.
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    std::string albumName = "ALL";
    std::string lPath = "";
    int32_t period = this->period_;  // 0 - BEFORE, 1 - AFTER
    int32_t dbType = 1;              // 0 - GALLERY, 1 - MEDIA
    info.albumName = AlbumNameInfo()
                         .SetAlbumName(albumName)
                         .SetLPath(lPath)
                         .SetCostTime(costTime)
                         .SetPeriod(period)
                         .SetDbType(dbType)
                         .ToString();
    return info;
}

AlbumMediaStatisticInfo PhotosCountStatistic::GetAllImageStatInfo()
{
    AlbumMediaStatisticInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    // build the statistic info.
    SearchCondition defaultCondition = SearchCondition().SetMediaType(SINGLE_MEDIA_TYPE_IMAGE);
    info.totalCount = this->QueryTotalCount(SearchCondition(defaultCondition));
    info.imageCount = this->QueryTotalCount(SearchCondition(defaultCondition));
    info.videoCount = 0;
    info.hiddenCount =
        this->QueryTotalCount(SearchCondition(defaultCondition).SetHiddenType(SINGLE_HIDDEN_TYPE_HIDDEN));
    info.trashedCount =
        this->QueryTotalCount(SearchCondition(defaultCondition).SetTrashedType(SINGLE_TRASHED_TYPE_TRASHED));
    info.cloudCount = this->QueryTotalCount(SearchCondition(defaultCondition).SetCloudType(SINGLE_CLOUD_TYPE_CLOUD));
    info.favoriteCount =
        this->QueryTotalCount(SearchCondition(defaultCondition).SetFavoriteType(SINGLE_FAVORITE_TYPE_FAVORITE));
    info.burstTotalCount = this->QueryTotalCount(SearchCondition(defaultCondition).SetBurstType(SINGLE_BURST_TYPE_ALL));
    info.burstCoverCount =
        this->QueryTotalCount(SearchCondition(defaultCondition).SetBurstType(SINGLE_BURST_TYPE_COVER));
    info.innerEastLakeCount =
        this->QueryTotalCount(SearchCondition(defaultCondition).SetLakeType(SINGLE_LAKE_TYPE_INNER));
    // build the album name.
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    std::string albumName = "ALL_IMAGE";
    std::string lPath = "";
    int32_t period = this->period_;  // 0 - BEFORE, 1 - AFTER
    int32_t dbType = 1;              // 0 - GALLERY, 1 - MEDIA
    info.albumName = AlbumNameInfo()
                         .SetAlbumName(albumName)
                         .SetLPath(lPath)
                         .SetCostTime(costTime)
                         .SetPeriod(period)
                         .SetDbType(dbType)
                         .ToString();
    return info;
}

AlbumMediaStatisticInfo PhotosCountStatistic::GetAllVideoStatInfo()
{
    AlbumMediaStatisticInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    // build the statistic info.
    SearchCondition defaultCondition = SearchCondition().SetMediaType(SINGLE_MEDIA_TYPE_VIDEO);
    info.totalCount = this->QueryTotalCount(SearchCondition(defaultCondition));
    info.imageCount = 0;
    info.videoCount = this->QueryTotalCount(SearchCondition(defaultCondition));
    info.hiddenCount =
        this->QueryTotalCount(SearchCondition(defaultCondition).SetHiddenType(SINGLE_HIDDEN_TYPE_HIDDEN));
    info.trashedCount =
        this->QueryTotalCount(SearchCondition(defaultCondition).SetTrashedType(SINGLE_TRASHED_TYPE_TRASHED));
    info.cloudCount = this->QueryTotalCount(SearchCondition(defaultCondition).SetCloudType(SINGLE_CLOUD_TYPE_CLOUD));
    info.favoriteCount =
        this->QueryTotalCount(SearchCondition(defaultCondition).SetFavoriteType(SINGLE_FAVORITE_TYPE_FAVORITE));
    info.burstTotalCount = 0;
    info.burstCoverCount = 0;
    info.innerEastLakeCount =
        this->QueryTotalCount(SearchCondition(defaultCondition).SetLakeType(SINGLE_LAKE_TYPE_INNER));
    // build the album name.
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    std::string albumName = "ALL_VIDEO";
    std::string lPath = "";
    int32_t period = this->period_;  // 0 - BEFORE, 1 - AFTER
    int32_t dbType = 1;              // 0 - GALLERY, 1 - MEDIA
    info.albumName = AlbumNameInfo()
                         .SetAlbumName(albumName)
                         .SetLPath(lPath)
                         .SetCostTime(costTime)
                         .SetPeriod(period)
                         .SetDbType(dbType)
                         .ToString();
    return info;
}

AlbumMediaStatisticInfo PhotosCountStatistic::GetAllRestoreStatInfo()
{
    AlbumMediaStatisticInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    // build the statistic info.
    info.totalCount = this->QueryAllRestoreCount(SearchCondition());
    info.imageCount = this->QueryAllRestoreCount(SearchCondition().SetMediaType(SINGLE_MEDIA_TYPE_IMAGE));
    info.videoCount = this->QueryAllRestoreCount(SearchCondition().SetMediaType(SINGLE_MEDIA_TYPE_VIDEO));
    info.hiddenCount = this->QueryAllRestoreCount(SearchCondition().SetHiddenType(SINGLE_HIDDEN_TYPE_HIDDEN));
    info.trashedCount = this->QueryAllRestoreCount(SearchCondition().SetTrashedType(SINGLE_TRASHED_TYPE_TRASHED));
    info.cloudCount = 0;
    info.favoriteCount = this->QueryAllRestoreCount(SearchCondition().SetFavoriteType(SINGLE_FAVORITE_TYPE_FAVORITE));
    info.burstTotalCount = this->QueryAllRestoreCount(SearchCondition().SetBurstType(SINGLE_BURST_TYPE_ALL));
    info.burstCoverCount = this->QueryAllRestoreCount(SearchCondition().SetBurstType(SINGLE_BURST_TYPE_COVER));
    info.innerEastLakeCount = this->QueryAllRestoreCount(SearchCondition().SetLakeType(SINGLE_LAKE_TYPE_INNER));
    // build the album name.
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    std::string albumName = "ALL_RESTORE";
    std::string lPath = "";
    int32_t period = this->period_;  // 0 - BEFORE, 1 - AFTER
    int32_t dbType = 1;              // 0 - GALLERY, 1 - MEDIA
    info.albumName = AlbumNameInfo()
                         .SetAlbumName(albumName)
                         .SetLPath(lPath)
                         .SetCostTime(costTime)
                         .SetPeriod(period)
                         .SetDbType(dbType)
                         .ToString();
    return info;
}

AlbumMediaStatisticInfo PhotosCountStatistic::GetAllRestoreImageStatInfo()
{
    AlbumMediaStatisticInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    // build the statistic info.
    SearchCondition defaultCondition = SearchCondition().SetMediaType(SINGLE_MEDIA_TYPE_IMAGE);
    info.totalCount = this->QueryAllRestoreCount(SearchCondition(defaultCondition));
    info.imageCount = this->QueryAllRestoreCount(SearchCondition(defaultCondition));
    info.videoCount = 0;
    info.hiddenCount =
        this->QueryAllRestoreCount(SearchCondition(defaultCondition).SetHiddenType(SINGLE_HIDDEN_TYPE_HIDDEN));
    info.trashedCount =
        this->QueryAllRestoreCount(SearchCondition(defaultCondition).SetTrashedType(SINGLE_TRASHED_TYPE_TRASHED));
    info.cloudCount = 0;
    info.favoriteCount =
        this->QueryAllRestoreCount(SearchCondition(defaultCondition).SetFavoriteType(SINGLE_FAVORITE_TYPE_FAVORITE));
    info.burstTotalCount =
        this->QueryAllRestoreCount(SearchCondition(defaultCondition).SetBurstType(SINGLE_BURST_TYPE_ALL));
    info.burstCoverCount =
        this->QueryAllRestoreCount(SearchCondition(defaultCondition).SetBurstType(SINGLE_BURST_TYPE_COVER));
    info.innerEastLakeCount =
        this->QueryAllRestoreCount(SearchCondition(defaultCondition).SetLakeType(SINGLE_LAKE_TYPE_INNER));
    // build the album name.
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    std::string albumName = "ALL_RESTORE_IMAGE";
    std::string lPath = "";
    int32_t period = this->period_;  // 0 - BEFORE, 1 - AFTER
    int32_t dbType = 1;              // 0 - GALLERY, 1 - MEDIA
    info.albumName = AlbumNameInfo()
                         .SetAlbumName(albumName)
                         .SetLPath(lPath)
                         .SetCostTime(costTime)
                         .SetPeriod(period)
                         .SetDbType(dbType)
                         .ToString();
    return info;
}

AlbumMediaStatisticInfo PhotosCountStatistic::GetAllRestoreVideoStatInfo()
{
    AlbumMediaStatisticInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    // build the statistic info.
    SearchCondition defaultCondition = SearchCondition().SetMediaType(SINGLE_MEDIA_TYPE_VIDEO);
    info.totalCount = this->QueryAllRestoreCount(SearchCondition(defaultCondition));
    info.imageCount = 0;
    info.videoCount = this->QueryAllRestoreCount(SearchCondition(defaultCondition));
    info.hiddenCount =
        this->QueryAllRestoreCount(SearchCondition(defaultCondition).SetHiddenType(SINGLE_HIDDEN_TYPE_HIDDEN));
    info.trashedCount =
        this->QueryAllRestoreCount(SearchCondition(defaultCondition).SetTrashedType(SINGLE_TRASHED_TYPE_TRASHED));
    info.cloudCount = 0;
    info.favoriteCount =
        this->QueryAllRestoreCount(SearchCondition(defaultCondition).SetFavoriteType(SINGLE_FAVORITE_TYPE_FAVORITE));
    info.burstTotalCount = 0;
    info.burstCoverCount = 0;
    info.innerEastLakeCount =
        this->QueryAllRestoreCount(SearchCondition(defaultCondition).SetLakeType(SINGLE_LAKE_TYPE_INNER));
    // build the album name.
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    std::string albumName = "ALL_RESTORE_VIDEO";
    std::string lPath = "";
    int32_t period = this->period_;  // 0 - BEFORE, 1 - AFTER
    int32_t dbType = 1;              // 0 - GALLERY, 1 - MEDIA
    info.albumName = AlbumNameInfo()
                         .SetAlbumName(albumName)
                         .SetLPath(lPath)
                         .SetCostTime(costTime)
                         .SetPeriod(period)
                         .SetDbType(dbType)
                         .ToString();
    return info;
}

AlbumMediaStatisticInfo PhotosCountStatistic::GetImageAlbumInfo()
{
    AlbumMediaStatisticInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    // build the statistic info.
    SearchCondition defaultCondition = SearchCondition()
                                           .SetMediaType(SINGLE_MEDIA_TYPE_IMAGE)
                                           .SetHiddenType(SINGLE_HIDDEN_TYPE_NOT_HIDDEN)
                                           .SetTrashedType(SINGLE_TRASHED_TYPE_NOT_TRASHED);
    info.totalCount = this->QueryPicturesTotalCount(defaultCondition);
    info.imageCount =
        this->QueryPicturesTotalCount(SearchCondition(defaultCondition).SetMediaType(SINGLE_MEDIA_TYPE_IMAGE));
    info.videoCount = 0;
    info.hiddenCount = 0;
    info.trashedCount = 0;
    info.cloudCount =
        this->QueryPicturesTotalCount(SearchCondition(defaultCondition).SetCloudType(SINGLE_CLOUD_TYPE_CLOUD));
    info.favoriteCount =
        this->QueryPicturesTotalCount(SearchCondition(defaultCondition).SetFavoriteType(SINGLE_FAVORITE_TYPE_FAVORITE));
    info.burstTotalCount =
        this->QueryTotalCount(SearchCondition(defaultCondition).SetBurstType(SINGLE_BURST_TYPE_ALL));
    info.burstCoverCount =
        this->QueryPicturesTotalCount(SearchCondition(defaultCondition).SetBurstType(SINGLE_BURST_TYPE_COVER));
    info.innerEastLakeCount =
        this->QueryPicturesTotalCount(SearchCondition(defaultCondition).SetLakeType(SINGLE_LAKE_TYPE_INNER));
    // build the album name.
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    std::string albumName = "图片";
    std::string lPath = "";
    int32_t period = this->period_;  // 0 - BEFORE, 1 - AFTER
    int32_t dbType = 1;              // 0 - GALLERY, 1 - MEDIA
    info.albumName = AlbumNameInfo()
                         .SetAlbumName(albumName)
                         .SetLPath(lPath)
                         .SetCostTime(costTime)
                         .SetPeriod(period)
                         .SetDbType(dbType)
                         .ToString();
    return info;
}

AlbumMediaStatisticInfo PhotosCountStatistic::GetVideoAlbumInfo()
{
    AlbumMediaStatisticInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    // build the statistic info.
    SearchCondition defaultCondition = SearchCondition()
                                           .SetMediaType(SINGLE_MEDIA_TYPE_VIDEO)
                                           .SetHiddenType(SINGLE_HIDDEN_TYPE_NOT_HIDDEN)
                                           .SetTrashedType(SINGLE_TRASHED_TYPE_NOT_TRASHED);
    info.totalCount = this->QueryPicturesTotalCount(SearchCondition(defaultCondition));
    info.imageCount = 0;
    info.videoCount =
        this->QueryPicturesTotalCount(SearchCondition(defaultCondition).SetMediaType(SINGLE_MEDIA_TYPE_VIDEO));
    info.hiddenCount = 0;
    info.trashedCount = 0;
    info.cloudCount =
        this->QueryPicturesTotalCount(SearchCondition(defaultCondition).SetCloudType(SINGLE_CLOUD_TYPE_CLOUD));
    info.favoriteCount =
        this->QueryPicturesTotalCount(SearchCondition(defaultCondition).SetFavoriteType(SINGLE_FAVORITE_TYPE_FAVORITE));
    info.burstTotalCount = 0;
    info.burstCoverCount = 0;
    info.innerEastLakeCount =
        this->QueryTotalCount(SearchCondition(defaultCondition).SetLakeType(SINGLE_LAKE_TYPE_INNER));
    // build the album name.
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    std::string albumName = "视频";
    std::string lPath = "";
    int32_t period = this->period_;  // 0 - BEFORE, 1 - AFTER
    int32_t dbType = 1;              // 0 - GALLERY, 1 - MEDIA
    info.albumName = AlbumNameInfo()
                         .SetAlbumName(albumName)
                         .SetLPath(lPath)
                         .SetCostTime(costTime)
                         .SetPeriod(period)
                         .SetDbType(dbType)
                         .ToString();
    return info;
}

std::vector<AlbumMediaStatisticInfo> PhotosCountStatistic::GetAlbumInfoByName(const std::string &albumName)
{
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    // build the statistic info.
    SearchCondition defaultCondition =
        SearchCondition().SetHiddenType(SINGLE_HIDDEN_TYPE_NOT_HIDDEN).SetTrashedType(SINGLE_TRASHED_TYPE_NOT_TRASHED);
    std::vector<AlbumStatisticInfo> totalCountInfoList =
        this->QueryAlbumCountByName(albumName, SearchCondition(defaultCondition));
    std::vector<AlbumStatisticInfo> imageCountInfoList =
        this->QueryAlbumCountByName(albumName, SearchCondition(defaultCondition).SetMediaType(SINGLE_MEDIA_TYPE_IMAGE));
    std::vector<AlbumStatisticInfo> videoCountInfoList =
        this->QueryAlbumCountByName(albumName, SearchCondition(defaultCondition).SetMediaType(SINGLE_MEDIA_TYPE_VIDEO));
    std::vector<AlbumStatisticInfo> cloudCountInfoList =
        this->QueryAlbumCountByName(albumName, SearchCondition(defaultCondition).SetCloudType(SINGLE_CLOUD_TYPE_CLOUD));
    std::vector<AlbumStatisticInfo> lakeCountInfoList =
        this->QueryAlbumCountByName(albumName, SearchCondition(defaultCondition).SetLakeType(SINGLE_LAKE_TYPE_INNER));
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    // Parse the statistic info.
    std::unordered_map<std::string, AlbumMediaStatisticInfo> albumInfoMap;
    StatisticProcessor()
        .ParseTotalCount(albumInfoMap, totalCountInfoList)
        .ParseImageCount(albumInfoMap, imageCountInfoList)
        .ParseVideoCount(albumInfoMap, videoCountInfoList)
        .ParseCloudCount(albumInfoMap, cloudCountInfoList)
        .ParseLakeCount(albumInfoMap, lakeCountInfoList);
    std::vector<AlbumMediaStatisticInfo> albumInfoList;
    for (const auto &iter : albumInfoMap) {
        AlbumMediaStatisticInfo info = iter.second;
        info.sceneCode = this->sceneCode_;
        info.taskId = this->taskId_;
        info.albumName = AlbumNameInfo()
                             .SetAlbumName(albumName)
                             .SetLPath(info.lPath)
                             .SetCostTime(costTime)
                             .SetPeriod(this->period_)  // 0 - BEFORE, 1 - AFTER
                             .SetDbType(1)  // 0 - GALLERY, 1 - MEDIA
                             .ToString();
        albumInfoList.emplace_back(info);
    }
    return albumInfoList;
}

AlbumMediaStatisticInfo PhotosCountStatistic::GetFavoriteAlbumStatInfo()
{
    AlbumMediaStatisticInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    // build the statistic info.
    SearchCondition defaultCondition = SearchCondition().SetFavoriteType(SINGLE_FAVORITE_TYPE_FAVORITE);
    info.totalCount = this->QueryPicturesTotalCount(SearchCondition(defaultCondition));
    info.imageCount =
        this->QueryPicturesTotalCount(SearchCondition(defaultCondition).SetMediaType(SINGLE_MEDIA_TYPE_IMAGE));
    info.videoCount =
        this->QueryPicturesTotalCount(SearchCondition(defaultCondition).SetMediaType(SINGLE_MEDIA_TYPE_VIDEO));
    info.hiddenCount = 0;
    info.trashedCount = 0;
    info.cloudCount =
        this->QueryPicturesTotalCount(SearchCondition(defaultCondition).SetCloudType(SINGLE_CLOUD_TYPE_CLOUD));
    info.favoriteCount =
        this->QueryPicturesTotalCount(SearchCondition(defaultCondition).SetFavoriteType(SINGLE_FAVORITE_TYPE_FAVORITE));
    info.burstTotalCount =
        this->QueryTotalCount(SearchCondition(defaultCondition).SetBurstType(SINGLE_BURST_TYPE_ALL));
    info.burstCoverCount =
        this->QueryPicturesTotalCount(SearchCondition(defaultCondition).SetBurstType(SINGLE_BURST_TYPE_COVER));
    info.innerEastLakeCount =
        this->QueryPicturesTotalCount(SearchCondition(defaultCondition).SetLakeType(SINGLE_LAKE_TYPE_INNER));
    // build the album name.
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    std::string albumName = "收藏";
    std::string lPath = "";
    int32_t period = this->period_;  // 0 - BEFORE, 1 - AFTER
    int32_t dbType = 1;              // 0 - GALLERY, 1 - MEDIA
    info.albumName = AlbumNameInfo()
                         .SetAlbumName(albumName)
                         .SetLPath(lPath)
                         .SetCostTime(costTime)
                         .SetPeriod(period)
                         .SetDbType(dbType)
                         .ToString();
    return info;
}

AlbumMediaStatisticInfo PhotosCountStatistic::GetTrashedAlbumStatInfo()
{
    AlbumMediaStatisticInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    // build the statistic info.
    SearchCondition defaultCondition = SearchCondition().SetTrashedType(SINGLE_TRASHED_TYPE_TRASHED);
    info.totalCount = this->QueryPicturesTotalCount(SearchCondition(defaultCondition));
    info.imageCount =
        this->QueryPicturesTotalCount(SearchCondition(defaultCondition).SetMediaType(SINGLE_MEDIA_TYPE_IMAGE));
    info.videoCount =
        this->QueryPicturesTotalCount(SearchCondition(defaultCondition).SetMediaType(SINGLE_MEDIA_TYPE_VIDEO));
    info.hiddenCount =
        this->QueryPicturesTotalCount(SearchCondition(defaultCondition).SetHiddenType(SINGLE_HIDDEN_TYPE_HIDDEN));
    info.trashedCount =
        this->QueryPicturesTotalCount(SearchCondition(defaultCondition).SetTrashedType(SINGLE_TRASHED_TYPE_TRASHED));
    info.cloudCount =
        this->QueryPicturesTotalCount(SearchCondition(defaultCondition).SetCloudType(SINGLE_CLOUD_TYPE_CLOUD));
    info.favoriteCount =
        this->QueryPicturesTotalCount(SearchCondition(defaultCondition).SetFavoriteType(SINGLE_FAVORITE_TYPE_FAVORITE));
    info.burstTotalCount =
        this->QueryTotalCount(SearchCondition(defaultCondition).SetBurstType(SINGLE_BURST_TYPE_ALL));
    info.burstCoverCount =
        this->QueryPicturesTotalCount(SearchCondition(defaultCondition).SetBurstType(SINGLE_BURST_TYPE_COVER));
    info.innerEastLakeCount =
        this->QueryPicturesTotalCount(SearchCondition(defaultCondition).SetLakeType(SINGLE_LAKE_TYPE_INNER));
    // build the album name.
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    std::string albumName = "回收站";
    std::string lPath = "";
    int32_t period = this->period_;  // 0 - BEFORE, 1 - AFTER
    int32_t dbType = 1;              // 0 - GALLERY, 1 - MEDIA
    info.albumName = AlbumNameInfo()
                         .SetAlbumName(albumName)
                         .SetLPath(lPath)
                         .SetCostTime(costTime)
                         .SetPeriod(period)
                         .SetDbType(dbType)
                         .ToString();
    return info;
}

AlbumMediaStatisticInfo PhotosCountStatistic::GetHiddenAlbumStatInfo()
{
    AlbumMediaStatisticInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    // build the statistic info.
    SearchCondition defaultCondition = SearchCondition().SetHiddenType(SINGLE_HIDDEN_TYPE_HIDDEN);
    info.totalCount = this->QueryPicturesTotalCount(SearchCondition(defaultCondition));
    info.imageCount =
        this->QueryPicturesTotalCount(SearchCondition(defaultCondition).SetMediaType(SINGLE_MEDIA_TYPE_IMAGE));
    info.videoCount =
        this->QueryPicturesTotalCount(SearchCondition(defaultCondition).SetMediaType(SINGLE_MEDIA_TYPE_VIDEO));
    info.hiddenCount =
        this->QueryPicturesTotalCount(SearchCondition(defaultCondition).SetHiddenType(SINGLE_HIDDEN_TYPE_HIDDEN));
    info.trashedCount =
        this->QueryPicturesTotalCount(SearchCondition(defaultCondition).SetTrashedType(SINGLE_TRASHED_TYPE_TRASHED));
    info.cloudCount =
        this->QueryPicturesTotalCount(SearchCondition(defaultCondition).SetCloudType(SINGLE_CLOUD_TYPE_CLOUD));
    info.favoriteCount =
        this->QueryPicturesTotalCount(SearchCondition(defaultCondition).SetFavoriteType(SINGLE_FAVORITE_TYPE_FAVORITE));
    info.burstTotalCount =
        this->QueryTotalCount(SearchCondition(defaultCondition).SetBurstType(SINGLE_BURST_TYPE_ALL));
    info.burstCoverCount =
        this->QueryPicturesTotalCount(SearchCondition(defaultCondition).SetBurstType(SINGLE_BURST_TYPE_COVER));
    info.innerEastLakeCount =
        this->QueryPicturesTotalCount(SearchCondition(defaultCondition).SetLakeType(SINGLE_LAKE_TYPE_INNER));
    // build the album name.
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    std::string albumName = "隐藏";
    std::string lPath = "";
    int32_t period = this->period_;  // 0 - BEFORE, 1 - AFTER
    int32_t dbType = 1;              // 0 - GALLERY, 1 - MEDIA
    info.albumName = AlbumNameInfo()
                         .SetAlbumName(albumName)
                         .SetLPath(lPath)
                         .SetCostTime(costTime)
                         .SetPeriod(period)
                         .SetDbType(dbType)
                         .ToString();
    return info;
}

AlbumMediaStatisticInfo PhotosCountStatistic::GetGalleryAlbumCountInfo()
{
    AlbumMediaStatisticInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    // build the statistic info.
    info.totalCount = this->GetCount(SQL_PHOTO_ALBUM_COUNT);
    // build the album name.
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    std::string albumName = "相册数量";
    std::string lPath = "";
    int32_t period = this->period_;  // 0 - BEFORE, 1 - AFTER
    int32_t dbType = 1;              // 0 - GALLERY, 1 - MEDIA
    info.albumName = AlbumNameInfo()
                         .SetAlbumName(albumName)
                         .SetLPath(lPath)
                         .SetCostTime(costTime)
                         .SetPeriod(period)
                         .SetDbType(dbType)
                         .ToString();
    return info;
}

AlbumMediaStatisticInfo PhotosCountStatistic::GetGalleryDeletedAlbumCountInfo()
{
    AlbumMediaStatisticInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    // build the statistic info.
    std::vector<NativeRdb::ValueObject> params = {static_cast<int32_t>(DirtyType::TYPE_DELETED)};
    info.totalCount = this->GetCount(SQL_PHOTO_DELETED_ALBUM_COUNT, params);
    // build the album name.
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    std::string albumName = "DELETED_ALBUM";
    std::string lPath = "";
    int32_t period = this->period_;  // 0 - BEFORE, 1 - AFTER
    int32_t dbType = 1;              // 0 - GALLERY, 1 - MEDIA
    info.albumName = AlbumNameInfo()
                         .SetAlbumName(albumName)
                         .SetLPath(lPath)
                         .SetCostTime(costTime)
                         .SetPeriod(period)
                         .SetDbType(dbType)
                         .ToString();
    return info;
}

AlbumMediaStatisticInfo PhotosCountStatistic::GetLiveStatInfo()
{
    AlbumMediaStatisticInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    // build the statistic info.
    info.totalCount = this->QueryLiveCount(SINGLE_SEARCH_TYPE_ALL);
    info.cloudCount = this->QueryLiveCount(SINGLE_SEARCH_TYPE_CLOUD);
    // build the album name.
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    std::string albumName = "动态照片";
    std::string lPath = "";
    int32_t period = this->period_;  // 0 - BEFORE, 1 - AFTER
    int32_t dbType = 1;              // 0 - GALLERY, 1 - MEDIA
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