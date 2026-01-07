/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define MLOG_TAG "AssetAccurateRefreshTest"
#include "asset_accurate_refresh_test.h"

#include <chrono>
#include <thread>

#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_type_const.h"

#include "accurate_debug_log.h"
#define protected public
#define private public
#include "asset_accurate_refresh.h"
#include "multi_thread_asset_change_info_mgr.h"
#undef protected
#undef private

#include "accurate_refresh_test_util.h"

namespace OHOS {
namespace Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace AccurateRefresh;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;

static constexpr int32_t SLEEP_FIVE_SECONDS = 5;

namespace {
// 新增
// 普通资产（收藏、视频、隐藏、图片）
    
const int32_t ASSET_FILE_ID = 0; // 数据库插入后才有值
const string ASSET_URI = "uri"; // 跟file_id关联
const string ASSET_DATE_DAY = "20250525";
const int64_t ASSET_DATE_TRASH = 123451234567;
const int32_t ASSET_MEDIA_TYPE_VIDEO = static_cast<int32_t>(MEDIA_TYPE_VIDEO);
const int32_t ASSET_MEDIA_TYPE_IMAGE = static_cast<int32_t>(MEDIA_TYPE_IMAGE);
const int32_t ASSET_STRONG_ASSOCIATION_NORMAL = static_cast<int32_t>(StrongAssociationType::NORMAL);
const int32_t ASSET_STRONG_ASSOCIATION_CLOUD = static_cast<int32_t>(StrongAssociationType::CLOUD_ENHANCEMENT);
const int32_t ASSET_THUMBNAIL_VISIBLE = 1;
const int64_t ASSET_DATE_ADDED = 123456;
const int64_t ASSET_DATE_TAKEN = 123456;
const int32_t ASSET_SUBTYPE_DEFAULT = static_cast<int32_t>(PhotoSubType::DEFAULT);
const int32_t ASSET_SYNC_STATUS_VISIBLE = static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE);
const int32_t ASSET_CLEAN_FLAG_NO = static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN);
const int32_t ASSET_BURST_COVER_LEVEL = static_cast<int32_t>(BurstCoverLevelType::COVER);
const string ASSET_DISPLAY_NAME = "asset_display_name";
const string ASSET_PATH = "asset_path";
const string OWNER_ALBUM_URI_ = "file://media/PhotoAlbum/";
const int64_t ASSET_HIDDEN_TIME = 54321;

const int32_t FAVORITE_VIDEO_ASSET_ALBUM_ID = 100;
const int32_t FAVORITE_VIDEO_ASSET_FILE_ID = 10000;

const int32_t VIDEO_ASSET_ALBUM_ID = 200;
const int32_t VIDEO_ASSET_FILE_ID = 2000;

const int32_t FAVORITE_IMAGE_ASSET_ALBUM_ID = 300;
const int32_t FAVORITE_IMAGE_ASSET_FILE_ID = 30000;

const int32_t IMAGE_ASSET_ALBUM_ID = 400;
const int32_t IMAGE_ASSET_FILE_ID = 40000;

const int32_t FAVORITE_VIDEO_HIDDEN_ASSET_ALBUM_ID = 500;
const int32_t FAVORITE_VIDEO_HIDDEN_ASSET_FILE_ID = 50000;

const int32_t VIDEO_HIDDEN_ASSET_ALBUM_ID = 600;

const int32_t FAVORITE_IMAGE_HIDDEN_ASSET_ALBUM_ID = 700;

const int32_t IMAGE_HIDDEN_ASSET_ALBUM_ID = 800;

const int32_t IMAGE_CLOUD_ASSET_ALBUM_ID = 900;
const int32_t IMAGE_CLOUD_ASSET_FILE_ID = 90000;

const int32_t IMAGE_TRASH_ASSET_ALBUM_ID = 1000;
const int32_t IMAGE_TRASH_ASSET_FILE_ID = 100000;

const PhotoAssetChangeInfo NORMAL_ASSET = { ASSET_FILE_ID, ASSET_URI, ASSET_DATE_DAY,
    "uri", // owner album uri
    false, // isFavorite
    ASSET_MEDIA_TYPE_IMAGE, // default image
    false, // isHidden
    0,  // dateTrash
    ASSET_STRONG_ASSOCIATION_NORMAL,
    ASSET_THUMBNAIL_VISIBLE, ASSET_DATE_ADDED, ASSET_DATE_TAKEN, ASSET_SUBTYPE_DEFAULT, ASSET_SYNC_STATUS_VISIBLE,
    ASSET_CLEAN_FLAG_NO,
    0, // timePending
    false, // isTemp
    ASSET_BURST_COVER_LEVEL,
    0, // owner album id
    0, // hidden time
    0,
    ASSET_DISPLAY_NAME,
    ASSET_PATH,
    1, // local
    666, //size
    0 //fileSourceType
};

void SetTables()
{
    // 创建Photos/PhotoAlbum表
    vector<string> createTableSqlList = {
        PhotoAlbumColumns::CREATE_TABLE,
        PhotoColumn::CREATE_PHOTO_TABLE,
    };
    for (auto &createTableSql : createTableSqlList) {
        if (g_rdbStore == nullptr) {
            MEDIA_ERR_LOG("can not get g_rdbstore");
            return;
        }
        int32_t ret = g_rdbStore->ExecuteSql(createTableSql);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("Execute sql %{private}s failed", createTableSql.c_str());
            return;
        }
        MEDIA_INFO_LOG("Execute sql %{private}s success", createTableSql.c_str());
    }
    MEDIA_INFO_LOG("SetTables");
}

void PrepareAlbumData()
{
    vector<ValuesBucket> values;
    values.push_back(GetFavoriteInsertAlbum());
    auto timestamp = AlbumAccurateRefreshManager::GetInstance().GetCurrentRefreshTag();
    AlbumRefreshTimestamp albumTimestamp(timestamp, timestamp);
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(FAVORITE_ALBUM_ID, true, albumTimestamp);
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(FAVORITE_ALBUM_ID, false, albumTimestamp);

    values.push_back(GetHiddenInsertAlbum());
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(HIDDEN_ALBUM_ID, true, albumTimestamp);
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(HIDDEN_ALBUM_ID, false, albumTimestamp);

    values.push_back(GetTrashInsertAlbum());
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(TRASH_ALBUM_ID, true, albumTimestamp);
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(TRASH_ALBUM_ID, false, albumTimestamp);

    values.push_back(GetVideoInsertAlbum());
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(VIDEO_ALBUM_ID, true, albumTimestamp);
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(VIDEO_ALBUM_ID, false, albumTimestamp);

    values.push_back(GetImageInsertAlbum());
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(IMAGE_ALBUM_ID, true, albumTimestamp);
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(IMAGE_ALBUM_ID, false, albumTimestamp);

    values.push_back(GetCloudEnhancementInsertAlbum());
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(CLOUD_ENHANCEMENT_ALBUM_ID, true, albumTimestamp);
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(CLOUD_ENHANCEMENT_ALBUM_ID, false, albumTimestamp);

    values.push_back(GetUserInsertAlbum(FAVORITE_VIDEO_ASSET_ALBUM_ID));
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(FAVORITE_VIDEO_ASSET_ALBUM_ID, true, albumTimestamp);
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(FAVORITE_VIDEO_ASSET_ALBUM_ID, false,
        albumTimestamp);

    values.push_back(GetUserInsertAlbum(VIDEO_ASSET_ALBUM_ID));
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(VIDEO_ASSET_ALBUM_ID, true, albumTimestamp);
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(VIDEO_ASSET_ALBUM_ID, false, albumTimestamp);

    values.push_back(GetUserInsertAlbum(FAVORITE_IMAGE_ASSET_ALBUM_ID));
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(FAVORITE_IMAGE_ASSET_ALBUM_ID, true, albumTimestamp);
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(FAVORITE_IMAGE_ASSET_ALBUM_ID, false,
        albumTimestamp);

    values.push_back(GetUserInsertAlbum(IMAGE_ASSET_ALBUM_ID));
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(IMAGE_ASSET_ALBUM_ID, true, albumTimestamp);
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(IMAGE_ASSET_ALBUM_ID, false, albumTimestamp);

    values.push_back(GetUserInsertAlbum(FAVORITE_VIDEO_HIDDEN_ASSET_ALBUM_ID));
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(FAVORITE_VIDEO_HIDDEN_ASSET_ALBUM_ID, true,
        albumTimestamp);
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(FAVORITE_VIDEO_HIDDEN_ASSET_ALBUM_ID, false,
        albumTimestamp);

    values.push_back(GetUserInsertAlbum(VIDEO_HIDDEN_ASSET_ALBUM_ID));
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(VIDEO_HIDDEN_ASSET_ALBUM_ID, true, albumTimestamp);
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(VIDEO_HIDDEN_ASSET_ALBUM_ID, false, albumTimestamp);

    values.push_back(GetUserInsertAlbum(FAVORITE_IMAGE_HIDDEN_ASSET_ALBUM_ID));
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(FAVORITE_IMAGE_HIDDEN_ASSET_ALBUM_ID, true,
        albumTimestamp);
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(FAVORITE_IMAGE_HIDDEN_ASSET_ALBUM_ID, false,
        albumTimestamp);

    values.push_back(GetUserInsertAlbum(IMAGE_HIDDEN_ASSET_ALBUM_ID));
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(IMAGE_HIDDEN_ASSET_ALBUM_ID, true, albumTimestamp);
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(IMAGE_HIDDEN_ASSET_ALBUM_ID, false, albumTimestamp);

    values.push_back(GetUserInsertAlbum(IMAGE_CLOUD_ASSET_ALBUM_ID));
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(IMAGE_CLOUD_ASSET_ALBUM_ID, true, albumTimestamp);
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(IMAGE_CLOUD_ASSET_ALBUM_ID, false, albumTimestamp);

    values.push_back(GetUserInsertAlbum(IMAGE_TRASH_ASSET_ALBUM_ID));
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(IMAGE_TRASH_ASSET_ALBUM_ID, true, albumTimestamp);
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(IMAGE_TRASH_ASSET_ALBUM_ID, false, albumTimestamp);

    int64_t insertNums = 0;
    auto ret = g_rdbStore->BatchInsert(insertNums, PhotoAlbumColumns::TABLE, values);
    ACCURATE_DEBUG("ret: %{public}d, insert values: %{public}" PRId64, ret, insertNums);
}

void CleanTestTables()
{
    vector<string> dropTableList = {
        PhotoAlbumColumns::TABLE,
        PhotoColumn::PHOTOS_TABLE,
    };
    for (auto &dropTable : dropTableList) {
        string dropSql = "DROP TABLE " + dropTable + ";";
        int32_t ret = g_rdbStore->ExecuteSql(dropSql);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("Drop %{public}s table failed", dropTable.c_str());
            return;
        }
        MEDIA_INFO_LOG("Drop %{public}s table success", dropTable.c_str());
    }
}

// 收藏、视频、非隐藏
PhotoAssetChangeInfo GetFavoriteVideoAsset()
{
    PhotoAssetChangeInfo assetChangeInfo = NORMAL_ASSET;
    assetChangeInfo.isFavorite_ = true;
    assetChangeInfo.fileId_ = FAVORITE_VIDEO_ASSET_FILE_ID;
    assetChangeInfo.ownerAlbumId_ = FAVORITE_VIDEO_ASSET_ALBUM_ID;
    assetChangeInfo.ownerAlbumUri_ = OWNER_ALBUM_URI_ + to_string(FAVORITE_VIDEO_ASSET_ALBUM_ID);
    assetChangeInfo.mediaType_ = ASSET_MEDIA_TYPE_VIDEO;

    return assetChangeInfo;
}

// 非收藏、视频、非隐藏
PhotoAssetChangeInfo GetVideoAsset()
{
    PhotoAssetChangeInfo assetChangeInfo = NORMAL_ASSET;
    assetChangeInfo.fileId_ = VIDEO_ASSET_FILE_ID;
    assetChangeInfo.ownerAlbumId_ = VIDEO_ASSET_ALBUM_ID;
    assetChangeInfo.ownerAlbumUri_ = OWNER_ALBUM_URI_ + to_string(VIDEO_ASSET_ALBUM_ID);
    assetChangeInfo.mediaType_ = ASSET_MEDIA_TYPE_VIDEO;
    return assetChangeInfo;
}

// 收藏、图片(MEDIA_TYPE_IMAGE)、非隐藏
PhotoAssetChangeInfo GetFavoriteImageAsset()
{
    PhotoAssetChangeInfo assetChangeInfo = NORMAL_ASSET;
    assetChangeInfo.isFavorite_ = true;
    assetChangeInfo.fileId_ = FAVORITE_IMAGE_ASSET_FILE_ID;
    assetChangeInfo.ownerAlbumId_ = FAVORITE_IMAGE_ASSET_ALBUM_ID;
    assetChangeInfo.ownerAlbumUri_ = OWNER_ALBUM_URI_ + to_string(FAVORITE_IMAGE_ASSET_ALBUM_ID);
    return assetChangeInfo;
}

// 非收藏、图片、非隐藏
PhotoAssetChangeInfo GetImageAsset()
{
    PhotoAssetChangeInfo assetChangeInfo = NORMAL_ASSET;
    assetChangeInfo.fileId_ = IMAGE_ASSET_FILE_ID;
    assetChangeInfo.ownerAlbumId_ = IMAGE_ASSET_ALBUM_ID;
    assetChangeInfo.ownerAlbumUri_ = OWNER_ALBUM_URI_ + to_string(IMAGE_ASSET_ALBUM_ID);
    return assetChangeInfo;
}

// 收藏、视频、隐藏
PhotoAssetChangeInfo GetFavoriteVideoHiddenAsset()
{
    PhotoAssetChangeInfo assetChangeInfo = NORMAL_ASSET;
    assetChangeInfo.isFavorite_ = true;
    assetChangeInfo.fileId_ = FAVORITE_VIDEO_HIDDEN_ASSET_FILE_ID;
    assetChangeInfo.ownerAlbumId_ = FAVORITE_VIDEO_HIDDEN_ASSET_ALBUM_ID;
    assetChangeInfo.ownerAlbumUri_ = OWNER_ALBUM_URI_ + to_string(FAVORITE_VIDEO_HIDDEN_ASSET_ALBUM_ID);
    assetChangeInfo.mediaType_ = ASSET_MEDIA_TYPE_VIDEO;
    assetChangeInfo.isHidden_ = true;
    assetChangeInfo.hiddenTime_ = ASSET_HIDDEN_TIME;
    return assetChangeInfo;
}

// 云同步资产
PhotoAssetChangeInfo GetCloudAsset()
{
    PhotoAssetChangeInfo assetChangeInfo = NORMAL_ASSET;
    assetChangeInfo.fileId_ = IMAGE_CLOUD_ASSET_FILE_ID;
    assetChangeInfo.ownerAlbumId_ = IMAGE_CLOUD_ASSET_ALBUM_ID;
    assetChangeInfo.ownerAlbumUri_ = OWNER_ALBUM_URI_ + to_string(IMAGE_HIDDEN_ASSET_ALBUM_ID);
    assetChangeInfo.strongAssociation_ = ASSET_STRONG_ASSOCIATION_CLOUD;
    return assetChangeInfo;
}

// 回收站资产
PhotoAssetChangeInfo GetTrashAsset()
{
    PhotoAssetChangeInfo assetChangeInfo = NORMAL_ASSET;
    assetChangeInfo.fileId_ = IMAGE_TRASH_ASSET_FILE_ID;
    assetChangeInfo.ownerAlbumId_ = IMAGE_TRASH_ASSET_ALBUM_ID;
    assetChangeInfo.ownerAlbumUri_ = OWNER_ALBUM_URI_ + to_string(IMAGE_HIDDEN_ASSET_ALBUM_ID);
    assetChangeInfo.dateTrashedMs_ = ASSET_DATE_TRASH;
    return assetChangeInfo;
}

ValuesBucket GetAssetInsertValue(PhotoAssetChangeInfo assetInfo)
{
    ValuesBucket value;
    value.PutInt(PhotoColumn::MEDIA_ID, assetInfo.fileId_);
    value.PutString(PhotoColumn::PHOTO_DATE_DAY, assetInfo.dateDay_);
    value.PutInt(PhotoColumn::MEDIA_IS_FAV, static_cast<int32_t>(assetInfo.isFavorite_));
    value.PutInt(PhotoColumn::MEDIA_TYPE, assetInfo.mediaType_);
    value.PutInt(PhotoColumn::MEDIA_HIDDEN, assetInfo.isHidden_);
    value.PutLong(PhotoColumn::MEDIA_DATE_TRASHED, assetInfo.dateTrashedMs_);
    value.PutInt(PhotoColumn::PHOTO_STRONG_ASSOCIATION, assetInfo.strongAssociation_);
    value.PutInt(PhotoColumn::PHOTO_THUMBNAIL_VISIBLE, assetInfo.thumbnailVisible_);
    value.PutLong(PhotoColumn::MEDIA_DATE_ADDED, assetInfo.dateAddedMs_);
    value.PutLong(PhotoColumn::MEDIA_DATE_TAKEN, assetInfo.dateTakenMs_);
    value.PutInt(PhotoColumn::PHOTO_SUBTYPE, assetInfo.subType_);
    value.PutInt(PhotoColumn::PHOTO_SYNC_STATUS, assetInfo.syncStatus_);
    value.PutInt(PhotoColumn::PHOTO_CLEAN_FLAG, assetInfo.cleanFlag_);
    value.PutInt(PhotoColumn::MEDIA_TIME_PENDING, assetInfo.timePending_);
    value.PutInt(PhotoColumn::PHOTO_IS_TEMP, assetInfo.isTemp_);
    value.PutInt(PhotoColumn::PHOTO_BURST_COVER_LEVEL, assetInfo.burstCoverLevel_);
    value.PutInt(PhotoColumn::PHOTO_OWNER_ALBUM_ID, assetInfo.ownerAlbumId_);
    value.PutLong(PhotoColumn::PHOTO_HIDDEN_TIME, assetInfo.hiddenTime_);
    value.PutString(PhotoColumn::MEDIA_NAME, assetInfo.displayName_);
    value.PutString(PhotoColumn::MEDIA_FILE_PATH, assetInfo.path_);
    return value;
}

PhotoAssetChangeInfo GetAssetInfo(int32_t fileId = 0)
{
    RdbPredicates queryPredicates(PhotoColumn::PHOTOS_TABLE);
    queryPredicates.EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));

    auto resultSet = g_rdbStore->QueryByStep(queryPredicates, PhotoAssetChangeInfo::GetPhotoAssetColumns());
    EXPECT_TRUE(resultSet != nullptr);
    auto assetInfos = PhotoAssetChangeInfo::GetInfoFromResult(resultSet, PhotoAssetChangeInfo::GetPhotoAssetColumns());
    if (assetInfos.size() == 1) {
        ACCURATE_DEBUG("assetInfo: %{public}s", assetInfos[0].ToString().c_str());
        return assetInfos[0];
    }
    ACCURATE_DEBUG("asset info wrong: %{public}zu.", assetInfos.size());
    return PhotoAssetChangeInfo();
}

bool CheckAssetEqual(const PhotoAssetChangeInfo &changeInfo1, const PhotoAssetChangeInfo &changeInfo2)
{
    ACCURATE_DEBUG("%{public}s", changeInfo1.ToString().c_str());
    ACCURATE_DEBUG("%{public}s", changeInfo2.ToString().c_str());
    return changeInfo1.fileId_ == changeInfo2.fileId_;
}

bool CheckAssetChangeData(const PhotoAssetChangeData &changeData1, const PhotoAssetChangeData &changeData2)
{
    if (changeData1.isContentChanged_ != changeData2.isContentChanged_) {
        MEDIA_ERR_LOG("isContentChanged_(%{public}d/%{public}d) wrong", changeData1.isContentChanged_,
            changeData2.isContentChanged_);
        return false;
    }

    if (changeData1.isDelete_ != changeData2.isDelete_) {
        MEDIA_ERR_LOG("isDelete_(%{public}d/%{public}d) wrong", changeData1.isDelete_, changeData2.isDelete_);
        return false;
    }

    if (changeData1.operation_ != changeData2.operation_) {
        MEDIA_ERR_LOG("operation_(%{public}d/%{public}d) wrong", changeData1.operation_, changeData2.operation_);
        return false;
    }

    if (changeData1.infoBeforeChange_.ownerAlbumId_ != changeData2.infoBeforeChange_.ownerAlbumId_) {
        MEDIA_ERR_LOG("ownerAlbumId_(%{public}d/%{public}d) wrong", changeData1.infoBeforeChange_.ownerAlbumId_,
            changeData2.infoBeforeChange_.ownerAlbumId_);
        return false;
    }

    if (changeData1.infoAfterChange_.ownerAlbumId_ != changeData2.infoAfterChange_.ownerAlbumId_) {
        MEDIA_ERR_LOG("ownerAlbumId_(%{public}d/%{public}d) wrong", changeData1.infoAfterChange_.ownerAlbumId_,
            changeData2.infoAfterChange_.ownerAlbumId_);
        return false;
    }
    return true;
}

bool CheckInsertAlbumInfo(const AlbumChangeInfo &refreshAlbumInfo, AlbumChangeInfo initAlbumInfo,
    const PhotoAssetChangeInfo &queryAssetInfo, map<int32_t, AlbumChangeData> &albumChangeDatas)
{
    // 数据信息
    auto expectedAlbumInfo = initAlbumInfo;
    if (!queryAssetInfo.isHidden_) {
        // 普通资产数量更新
        expectedAlbumInfo.count_++;
        if (queryAssetInfo.mediaType_ == static_cast<int32_t>(MEDIA_TYPE_VIDEO)) {
            expectedAlbumInfo.videoCount_++;
        } else if (queryAssetInfo.mediaType_ == static_cast<int32_t>(MEDIA_TYPE_IMAGE)) {
            expectedAlbumInfo.imageCount_++;
        }
        // 封面更新
        // 视频和图片为date_added，其它为dateTaken
        bool isDateAdded = initAlbumInfo.albumId_ == IMAGE_ALBUM_ID || initAlbumInfo.albumId_ == VIDEO_ALBUM_ID;
        auto queryAssetCoverDateTime = isDateAdded ? queryAssetInfo.dateAddedMs_ : queryAssetInfo.dateTakenMs_;
        if (queryAssetCoverDateTime > initAlbumInfo.coverDateTime_) {
            expectedAlbumInfo.coverDateTime_ = queryAssetCoverDateTime;
            expectedAlbumInfo.coverUri_ = queryAssetInfo.uri_;
            if (!CheckAssetEqual(refreshAlbumInfo.coverInfo_, queryAssetInfo)) {
                MEDIA_ERR_LOG("cover info wrong");
                return false;
            }
        } else {
            if (!CheckAssetEqual(refreshAlbumInfo.coverInfo_, initAlbumInfo.coverInfo_)) {
                MEDIA_ERR_LOG("cover info wrong");
                return false;
            }
        }
    } else {
        // 隐藏资产数量更新
        expectedAlbumInfo.hiddenCount_++;
        // 隐藏资产封面更新
        if (queryAssetInfo.hiddenTime_ > initAlbumInfo.hiddenCoverDateTime_) {
            expectedAlbumInfo.hiddenCoverDateTime_ = queryAssetInfo.hiddenTime_;
            expectedAlbumInfo.hiddenCoverUri_ = queryAssetInfo.uri_;
            if (!CheckAssetEqual(refreshAlbumInfo.hiddenCoverInfo_, queryAssetInfo)) {
                MEDIA_ERR_LOG("hidden cover info wrong");
                return false;
            }
        } else {
            if (!CheckAssetEqual(refreshAlbumInfo.hiddenCoverInfo_, initAlbumInfo.hiddenCoverInfo_)) {
                MEDIA_ERR_LOG("hidden cover info wrong");
                return false;
            }
        }
    }
    if (!IsEqualAlbumInfo(refreshAlbumInfo, expectedAlbumInfo)) {
        MEDIA_ERR_LOG("album info wrong");
        return false;
    }
    AlbumChangeData albumChangeData;
    albumChangeData.infoBeforeChange_ = initAlbumInfo;
    albumChangeData.infoAfterChange_ = expectedAlbumInfo;
    albumChangeData.isDelete_ = false;
    albumChangeDatas.emplace(initAlbumInfo.albumId_, albumChangeData);
    
    // 数据库信息
    auto queryAlbumInfo = GetAlbumInfo(refreshAlbumInfo.albumId_, g_rdbStore);
    if (!IsEqualAlbumInfo(refreshAlbumInfo, queryAlbumInfo)) {
        MEDIA_ERR_LOG("database album info wrong");
        return false;
    }
    return true;
}

bool CheckInsertHiddenAlbum(const AlbumChangeInfo &refreshHiddenAlbumInfo, AlbumChangeInfo initHiddenAlbumInfo,
    const PhotoAssetChangeInfo &queryAssetInfo, map<int32_t, AlbumChangeData> &albumChangeDatas)
{
    AlbumChangeInfo expectedAlbumInfo = initHiddenAlbumInfo;
    if (queryAssetInfo.isHidden_) {
        // 隐藏相册更新
        expectedAlbumInfo.count_++;
        expectedAlbumInfo.hiddenCount_ = expectedAlbumInfo.count_;
        if (queryAssetInfo.mediaType_ == static_cast<int32_t>(MEDIA_TYPE_VIDEO)) {
            expectedAlbumInfo.videoCount_++;
        } else {
            expectedAlbumInfo.imageCount_++;
        }
        if (refreshHiddenAlbumInfo.hiddenCoverDateTime_ < queryAssetInfo.hiddenTime_) {
            expectedAlbumInfo.hiddenCoverDateTime_ = queryAssetInfo.hiddenTime_;
            expectedAlbumInfo.hiddenCoverUri_ = queryAssetInfo.uri_;
            if (!CheckAssetEqual(refreshHiddenAlbumInfo.coverInfo_, queryAssetInfo)) {
                MEDIA_ERR_LOG("hidden cover info not refresh");
                return false;
            }
        } else {
            if (!CheckAssetEqual(refreshHiddenAlbumInfo.coverInfo_, initHiddenAlbumInfo.coverInfo_)) {
                MEDIA_ERR_LOG("hidden cover info wrong");
                return false;
            }
        }
    } else {
        MEDIA_ERR_LOG("not hidden asset.");
        return false;
    }
    if (!IsEqualAlbumInfo(refreshHiddenAlbumInfo, expectedAlbumInfo)) {
        return false;
    }
    AlbumChangeData albumChangeData;
    albumChangeData.infoBeforeChange_ = initHiddenAlbumInfo;
    albumChangeData.infoAfterChange_ = expectedAlbumInfo;
    albumChangeData.isDelete_ = false;
    albumChangeDatas.emplace(initHiddenAlbumInfo.albumId_, albumChangeData);
    return true;
}

bool CheckInsertAlbumInfos(const AssetAccurateRefresh &assetRefresh, const vector<AlbumChangeInfo> &initAlbumInfos,
    const PhotoAssetChangeInfo &queryAssetInfo, map<int32_t, AlbumChangeData> &albumChangeDatas)
{
    // 刷新相册ID
    EXPECT_TRUE(assetRefresh.albumRefreshExe_.forceRefreshAlbums_.empty());
    auto forceRefreshHiddenAlbums = assetRefresh.albumRefreshExe_.forceRefreshHiddenAlbums_;
    EXPECT_TRUE(forceRefreshHiddenAlbums.empty());

    // 刷新信息和数据库信息
    auto refreshAlbumsMap = assetRefresh.albumRefreshExe_.refreshAlbums_;
    for (auto &albumInfo : initAlbumInfos) {
        auto albumId = albumInfo.albumId_;
        if (refreshAlbumsMap.find(albumId) == refreshAlbumsMap.end()) {
            MEDIA_ERR_LOG("no album id, albumId:%{public}d", albumId);
            return false;
        }
        auto albumInfoIter = refreshAlbumsMap.find(albumId);
        if (albumInfoIter == refreshAlbumsMap.end()) {
            MEDIA_ERR_LOG("no album info, albumId:%{public}d", albumId);
            return false;
        }
        // 隐藏相册计算方式不同
        if (albumId == HIDDEN_ALBUM_ID) {
            if (!CheckInsertHiddenAlbum(albumInfoIter->second.second, albumInfo, queryAssetInfo, albumChangeDatas)) {
                MEDIA_ERR_LOG("hidden album info wrong, albumId:%{public}d", albumId);
                return false;
            }
            continue;
        }
        if (!CheckInsertAlbumInfo(albumInfoIter->second.second, albumInfo, queryAssetInfo, albumChangeDatas)) {
            MEDIA_ERR_LOG("album info wrong, albumId:%{public}d", albumId);
            return false;
        }
        ACCURATE_DEBUG("refrehInfo: %{public}s", albumInfoIter->second.second.ToString().c_str());
    }
    ACCURATE_DEBUG("return true");
    return true;
}

bool CheckAlbumChangeDatas(const AlbumChangeData &notifyChangeData, const AlbumChangeData &expectedChangeData)
{
    return CheckAlbumChangeData(notifyChangeData, expectedChangeData.operation_, expectedChangeData.infoBeforeChange_,
        expectedChangeData.infoAfterChange_, expectedChangeData.isDelete_);
}

bool CheckInsertAssetNotifyInfo(pair<Notification::AssetRefreshOperation, vector<PhotoAssetChangeData>> notifyInfos,
    const PhotoAssetChangeData &assetChangeData, Notification::AssetRefreshOperation operation)
{
    auto assetOperation = notifyInfos.first;
    if (assetOperation != operation) {
        MEDIA_ERR_LOG("notify asset operation wrong: %{public}d", assetOperation);
        return false;
    }
    auto photoAssetChangeDataVec = notifyInfos.second;
    if (photoAssetChangeDataVec.size() != 1) {
        MEDIA_ERR_LOG("notify asset change info size wrong: %{public}zu", photoAssetChangeDataVec.size());
        return false;
    }
    if (!CheckAssetChangeData(photoAssetChangeDataVec[0], assetChangeData)) {
        MEDIA_ERR_LOG("asset change data wrong.");
        return false;
    }
    return true;
}

bool CheckInsertNotifyAlbumInfos(map<Notification::AlbumRefreshOperation, vector<AlbumChangeData>> notifyAlbumInfos,
    Notification::AlbumRefreshOperation operation, const map<int32_t, AlbumChangeData> albumChangeDatas,
    int64_t notifySize)
{
    auto iter = notifyAlbumInfos.find(operation);
    if (iter == notifyAlbumInfos.end()) {
        MEDIA_ERR_LOG("notify album operation wrong: %{public}d", operation);
        return false;
    }
    
    auto albumChangeDataVec = iter->second;
    if (notifySize != albumChangeDataVec.size()) {
        MEDIA_ERR_LOG("notify size wrong");
        return false;
    }
    for (auto &albumChangeData : albumChangeDataVec) {
        auto albumId = albumChangeData.infoAfterChange_.albumId_;
        auto expectedAlbumChangeIter = albumChangeDatas.find(albumId);
        if (expectedAlbumChangeIter == albumChangeDatas.end()) {
            MEDIA_ERR_LOG("no albumId:%{public}d", albumId);
            return false;
        }
        if (!CheckAlbumChangeDatas(albumChangeData, expectedAlbumChangeIter->second)) {
            MEDIA_ERR_LOG("album change data wrong: %{public}d", albumId);
            return false;
        }
    }
    return true;
}

bool CheckInsertNotifyInfos(const AssetAccurateRefresh &assetRefresh, const PhotoAssetChangeData &assetChangeData,
    const map<int32_t, AlbumChangeData> albumChangeDatas)
{
    auto notifyAssetInfos = assetRefresh.notifyExe_.notifyInfos_;
    if (notifyAssetInfos.size() != 1) {
        MEDIA_ERR_LOG("notify asset change type size wrong: %{public}zu", notifyAssetInfos.size());
        return false;
    }
    if (!CheckInsertAssetNotifyInfo(*(notifyAssetInfos.begin()), assetChangeData, Notification::ASSET_OPERATION_ADD)) {
        MEDIA_ERR_LOG("asset notify info wrong.");
        return false;
    }

    auto notifyAlbumInfos = assetRefresh.albumRefreshExe_.albumRefresh_.notifyExe_.notifyInfos_;
    if (notifyAlbumInfos.size() != 1) {
        MEDIA_ERR_LOG("notify album change type size wrong: %{public}zu", notifyAlbumInfos.size());
        return false;
    }
    if (!CheckInsertNotifyAlbumInfos(notifyAlbumInfos, Notification::ALBUM_OPERATION_UPDATE, albumChangeDatas,
        albumChangeDatas.size())) {
        MEDIA_ERR_LOG("notify album info wrong.");
        return false;
    }

    return true;
}

bool CheckAlbumInfo(const unordered_map<int32_t, pair<AlbumRefreshInfo, AlbumChangeInfo>> &refreshAlbums_,
    const AlbumChangeInfo &initAlbumInfo,
    const AlbumChangeInfo &expectedAlbumInfo, RdbOperation operation, map<int32_t, AlbumChangeData> &albumChangeDatas)
{
    auto refreshIter = refreshAlbums_.find(expectedAlbumInfo.albumId_);
    if (refreshIter == refreshAlbums_.end()) {
        MEDIA_ERR_LOG("No refresh album id: %{public}d", expectedAlbumInfo.albumId_);
        return false;
    }
    if (!IsEqualAlbumInfo(expectedAlbumInfo, refreshIter->second.second)) {
        MEDIA_ERR_LOG("refresh album info wrong");
        return false;
    }
    AlbumChangeData albumChangeData;
    albumChangeData.infoBeforeChange_ = initAlbumInfo;
    albumChangeData.infoAfterChange_ = expectedAlbumInfo;
    albumChangeData.operation_ = operation;
    albumChangeData.isDelete_ = false;
    albumChangeDatas.emplace(expectedAlbumInfo.albumId_, albumChangeData);
    return true;
}
bool CheckAssetNotifyInfo(const map<Notification::AssetRefreshOperation, vector<PhotoAssetChangeData>> &notifyInfos,
    Notification::AssetRefreshOperation operation, const PhotoAssetChangeData &assetChangeData)
{
    auto iter = notifyInfos.find(operation);
    if (iter == notifyInfos.end()) {
        MEDIA_ERR_LOG("no operation: %{public}d", operation);
        return false;
    }
    auto assetChangeDatas = iter->second;
    for (auto assetChangeData : assetChangeDatas) {
        if (CheckAssetChangeData(assetChangeData, assetChangeData)) {
            return true;
        }
    }
    MEDIA_ERR_LOG("no equal asset change data.");
    return false;
}
void PrepareNormalAssets()
{
    vector<ValuesBucket> values;
    values.push_back(GetAssetInsertValue(GetFavoriteImageAsset()));
    values.push_back(GetAssetInsertValue(GetFavoriteVideoAsset()));
    values.push_back(GetAssetInsertValue(GetImageAsset()));
    int64_t insertNums = 0;
    auto ret = g_rdbStore->BatchInsert(insertNums, PhotoColumn::PHOTOS_TABLE, values);
    ACCURATE_DEBUG("ret: %{public}d, insert values: %{public}" PRId64, ret, insertNums);
}

void ModifyAssetDateTime(int64_t dateTaken)
{
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.NotEqualTo(PhotoColumn::MEDIA_ID, 0);
    ValuesBucket value;
    value.PutInt(PhotoColumn::MEDIA_DATE_TAKEN, dateTaken);
    value.PutInt(PhotoColumn::MEDIA_DATE_ADDED, dateTaken);
    int32_t changedRows = 0;
    auto ret = g_rdbStore->Update(changedRows, value, predicates);
    ACCURATE_DEBUG("ret: %{public}d, insert values: %{public}d", ret, changedRows);
}

void ClearAndRestart()
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    system("rm -rf /storage/cloud/files/*");
    system("rm -rf /storage/cloud/files/.thumbs");
    system("rm -rf /storage/cloud/files/.editData");
    system("rm -rf /storage/cloud/files/.cache");
    for (const auto &dir : TEST_ROOT_DIRS) {
        string ROOT_PATH = "/storage/cloud/100/files/";
        bool ret = MediaFileUtils::CreateDirectory(ROOT_PATH + dir + "/");
        CHECK_AND_PRINT_LOG(ret, "make %{public}s dir failed, ret=%{public}d", dir.c_str(), ret);
    }
    CleanTestTables();
    SetTables();
}

} // namespace

void AssetAccurateRefreshTest::SetUpTestCase()
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    SetTables();
}

void AssetAccurateRefreshTest::SetUp()
{
    ClearAndRestart();
    PrepareAlbumData();
}

void AssetAccurateRefreshTest::TearDownTestCase()
{
    CleanTestTables();
    g_rdbStore = nullptr;
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    this_thread::sleep_for(chrono::seconds(SLEEP_FIVE_SECONDS));
    MEDIA_INFO_LOG("Clean is finish");
}

void AssetAccurateRefreshTest::TearDown()
{
    MEDIA_INFO_LOG("TearDown start");
    RdbPredicates alumPredicates(PhotoAlbumColumns::TABLE);
    alumPredicates.NotEqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(0));
    
    int deleteRows = 0;
    g_rdbStore->Delete(deleteRows, alumPredicates);
    MEDIA_INFO_LOG("delete album: %{public}d", deleteRows);

    RdbPredicates photosPredicates(PhotoColumn::PHOTOS_TABLE);
    photosPredicates.GreaterThan(PhotoColumn::MEDIA_ID, to_string(0));
    g_rdbStore->Delete(deleteRows, photosPredicates);
    MEDIA_INFO_LOG("delete Photos: %{public}d", deleteRows);
    MEDIA_INFO_LOG("TearDown end");
}

HWTEST_F(AssetAccurateRefreshTest, Init_001, TestSize.Level2)
{
    std::shared_ptr<TransactionOperations> trans = make_shared<TransactionOperations>("Init_001");
    AssetAccurateRefresh assetRefresh(trans);
    EXPECT_TRUE(assetRefresh.trans_ != nullptr);
    EXPECT_TRUE(assetRefresh.dataManager_.trans_ != nullptr);
}

HWTEST_F(AssetAccurateRefreshTest, Insert_002, TestSize.Level2)
{
    AssetAccurateRefresh assetRefresh("Insert_002");
    int64_t outRowId = 0;
    auto assetInfo = GetFavoriteVideoAsset();
    auto value = GetAssetInsertValue(assetInfo);
    auto ret = assetRefresh.Insert(outRowId, PhotoColumn::PHOTOS_TABLE, value);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(outRowId > 0);
    
    // 数据库
    auto queryAssetInfo = GetAssetInfo(assetInfo.fileId_);
    EXPECT_TRUE(CheckAssetEqual(assetInfo, queryAssetInfo));

    // data manager
    auto changeDatasMap = assetRefresh.dataManager_.changeDatas_;
    PhotoAssetChangeData assetChangeData;
    assetChangeData.operation_ = RDB_OPERATION_ADD;
    assetChangeData.infoAfterChange_ = queryAssetInfo;
    assetChangeData.isContentChanged_ = false;
    assetChangeData.thumbnailChangeStatus_ = ThumbnailChangeStatus::THUMBNAIL_NOT_EXISTS;
    assetChangeData.isDelete_ = false;
    EXPECT_TRUE(changeDatasMap.size() == 1);
    auto iter = changeDatasMap.begin();
    EXPECT_TRUE(CheckAssetChangeData(iter->second, assetChangeData));

    // album 刷新
    assetRefresh.RefreshAlbum();
    // 相册刷新信息和数据库信息、通知信息
    vector<AlbumChangeInfo> initAlbumInfos =
        { FAVORITE_ALBUM_INFO, VIDEO_ALBUM_INFO, GetUserInsertInfo(FAVORITE_VIDEO_ASSET_ALBUM_ID)};
    map<int32_t, AlbumChangeData> albumChangeDatas;
    EXPECT_TRUE(CheckInsertAlbumInfos(assetRefresh, initAlbumInfos, queryAssetInfo, albumChangeDatas));

    // Notify
    assetRefresh.Notify();
    EXPECT_TRUE(CheckInsertNotifyInfos(assetRefresh, assetChangeData, albumChangeDatas));
}

HWTEST_F(AssetAccurateRefreshTest, Insert_003, TestSize.Level2)
{
    AssetAccurateRefresh assetRefresh("Insert_003");
    int64_t outRowId = 0;
    auto assetInfo = GetVideoAsset();
    auto value = GetAssetInsertValue(assetInfo);
    auto ret = assetRefresh.Insert(outRowId, PhotoColumn::PHOTOS_TABLE, value);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(outRowId > 1);
    // 数据库
    auto queryAssetInfo = GetAssetInfo(assetInfo.fileId_);
    EXPECT_TRUE(CheckAssetEqual(assetInfo, queryAssetInfo));

    // data manager
    auto changeDatasMap = assetRefresh.dataManager_.changeDatas_;
    PhotoAssetChangeData assetChangeData;
    assetChangeData.operation_ = RDB_OPERATION_ADD;
    assetChangeData.infoAfterChange_ = queryAssetInfo;
    EXPECT_TRUE(changeDatasMap.size() == 1);
    auto iter = changeDatasMap.begin();
    EXPECT_TRUE(CheckAssetChangeData(iter->second, assetChangeData));

    // album 刷新
    assetRefresh.RefreshAlbum();
    // 相册刷新信息和数据库信息、通知信息
    vector<AlbumChangeInfo> initAlbumInfos = { VIDEO_ALBUM_INFO, GetUserInsertInfo(assetInfo.ownerAlbumId_)};
    map<int32_t, AlbumChangeData> albumChangeDatas;
    EXPECT_TRUE(CheckInsertAlbumInfos(assetRefresh, initAlbumInfos, queryAssetInfo, albumChangeDatas));

    // Notify
    assetRefresh.Notify();
    EXPECT_TRUE(CheckInsertNotifyInfos(assetRefresh, assetChangeData, albumChangeDatas));
}

HWTEST_F(AssetAccurateRefreshTest, Insert_004, TestSize.Level2)
{
    AssetAccurateRefresh assetRefresh("Insert_004");
    int64_t outRowId = 0;
    auto assetInfo = GetFavoriteImageAsset();
    auto value = GetAssetInsertValue(assetInfo);
    auto ret = assetRefresh.Insert(outRowId, PhotoColumn::PHOTOS_TABLE, value);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(outRowId > 1);
    // 数据库
    auto queryAssetInfo = GetAssetInfo(assetInfo.fileId_);
    EXPECT_TRUE(CheckAssetEqual(assetInfo, queryAssetInfo));

    // data manager
    auto changeDatasMap = assetRefresh.dataManager_.changeDatas_;
    PhotoAssetChangeData assetChangeData;
    assetChangeData.operation_ = RDB_OPERATION_ADD;
    assetChangeData.infoAfterChange_ = queryAssetInfo;
    EXPECT_TRUE(changeDatasMap.size() == 1);
    auto iter = changeDatasMap.begin();
    EXPECT_TRUE(CheckAssetChangeData(iter->second, assetChangeData));

    // album 刷新
    assetRefresh.RefreshAlbum();
    // 相册刷新信息和数据库信息、通知信息
    vector<AlbumChangeInfo> initAlbumInfos =
        { FAVORITE_ALBUM_INFO, IMAGE_ALBUM_INFO, GetUserInsertInfo(assetInfo.ownerAlbumId_)};
    map<int32_t, AlbumChangeData> albumChangeDatas;
    EXPECT_TRUE(CheckInsertAlbumInfos(assetRefresh, initAlbumInfos, queryAssetInfo, albumChangeDatas));

    // Notify
    assetRefresh.Notify();
    EXPECT_TRUE(CheckInsertNotifyInfos(assetRefresh, assetChangeData, albumChangeDatas));
}

HWTEST_F(AssetAccurateRefreshTest, Insert_005, TestSize.Level2)
{
    AssetAccurateRefresh assetRefresh("Insert_005");
    int64_t outRowId = 0;
    auto assetInfo = GetImageAsset();
    auto value = GetAssetInsertValue(assetInfo);
    auto ret = assetRefresh.Insert(outRowId, PhotoColumn::PHOTOS_TABLE, value);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(outRowId > 1);
    // 数据库
    auto queryAssetInfo = GetAssetInfo(assetInfo.fileId_);
    EXPECT_TRUE(CheckAssetEqual(assetInfo, queryAssetInfo));

    // data manager
    auto changeDatasMap = assetRefresh.dataManager_.changeDatas_;
    PhotoAssetChangeData assetChangeData;
    assetChangeData.operation_ = RDB_OPERATION_ADD;
    assetChangeData.infoAfterChange_ = queryAssetInfo;
    EXPECT_TRUE(changeDatasMap.size() == 1);
    auto iter = changeDatasMap.begin();
    EXPECT_TRUE(CheckAssetChangeData(iter->second, assetChangeData));

    // album 刷新
    assetRefresh.RefreshAlbum();
    // 相册刷新信息和数据库信息、通知信息
    vector<AlbumChangeInfo> initAlbumInfos = { IMAGE_ALBUM_INFO, GetUserInsertInfo(assetInfo.ownerAlbumId_)};
    map<int32_t, AlbumChangeData> albumChangeDatas;
    EXPECT_TRUE(CheckInsertAlbumInfos(assetRefresh, initAlbumInfos, queryAssetInfo, albumChangeDatas));

    // Notify
    assetRefresh.Notify();
    EXPECT_TRUE(CheckInsertNotifyInfos(assetRefresh, assetChangeData, albumChangeDatas));
}

HWTEST_F(AssetAccurateRefreshTest, Insert_006, TestSize.Level2)
{
    AssetAccurateRefresh assetRefresh("Insert_006");
    int64_t outRowId = 0;
    auto assetInfo = GetFavoriteVideoHiddenAsset();
    auto value = GetAssetInsertValue(assetInfo);
    auto ret = assetRefresh.Insert(outRowId, PhotoColumn::PHOTOS_TABLE, value);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(outRowId > 1);
    // 资产数据库
    auto queryAssetInfo = GetAssetInfo(assetInfo.fileId_);
    EXPECT_TRUE(CheckAssetEqual(assetInfo, queryAssetInfo));

    // 资产data manager信息
    auto changeDatasMap = assetRefresh.dataManager_.changeDatas_;
    PhotoAssetChangeData assetChangeData;
    assetChangeData.operation_ = RDB_OPERATION_ADD;
    assetChangeData.infoAfterChange_ = queryAssetInfo;
    EXPECT_TRUE(changeDatasMap.size() == 1);
    auto iter = changeDatasMap.begin();
    EXPECT_TRUE(CheckAssetChangeData(iter->second, assetChangeData));

    // album 刷新
    assetRefresh.RefreshAlbum();
    // 相册刷新信息和数据库信息、通知信息
    vector<AlbumChangeInfo> initAlbumInfos =
        { FAVORITE_ALBUM_INFO, VIDEO_ALBUM_INFO, HIDDEN_ALBUM_INFO, GetUserInsertInfo(assetInfo.ownerAlbumId_)};
    map<int32_t, AlbumChangeData> albumChangeDatas;
    EXPECT_TRUE(CheckInsertAlbumInfos(assetRefresh, initAlbumInfos, queryAssetInfo, albumChangeDatas));
    
    // Notify
    assetRefresh.Notify();
    // 资产通知
    auto notifyAssetInfos = assetRefresh.notifyExe_.notifyInfos_;
    EXPECT_TRUE(notifyAssetInfos.size() == 1);
    EXPECT_TRUE(CheckInsertAssetNotifyInfo(*(notifyAssetInfos.begin()), assetChangeData,
        Notification::ASSET_OPERATION_ADD_HIDDEN));
    
    auto notifyAlbumInfos = assetRefresh.albumRefreshExe_.albumRefresh_.notifyExe_.notifyInfos_;
    EXPECT_TRUE(notifyAlbumInfos.size() == 1);
    EXPECT_TRUE(CheckInsertNotifyAlbumInfos(notifyAlbumInfos, Notification::ALBUM_OPERATION_UPDATE_HIDDEN,
        albumChangeDatas, 4));
}

HWTEST_F(AssetAccurateRefreshTest, Insert_007, TestSize.Level2)
{
    AssetAccurateRefresh assetRefresh("Insert_007");
    int64_t outRowId = 0;
    auto assetInfo = GetCloudAsset();
    auto value = GetAssetInsertValue(assetInfo);
    auto ret = assetRefresh.Insert(outRowId, PhotoColumn::PHOTOS_TABLE, value);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(outRowId > 1);
    // 数据库
    auto queryAssetInfo = GetAssetInfo(assetInfo.fileId_);
    EXPECT_TRUE(CheckAssetEqual(assetInfo, queryAssetInfo));

    // data manager
    auto changeDatasMap = assetRefresh.dataManager_.changeDatas_;
    PhotoAssetChangeData assetChangeData;
    assetChangeData.operation_ = RDB_OPERATION_ADD;
    assetChangeData.infoAfterChange_ = queryAssetInfo;
    EXPECT_TRUE(changeDatasMap.size() == 1);
    auto iter = changeDatasMap.begin();
    EXPECT_TRUE(CheckAssetChangeData(iter->second, assetChangeData));

    // album 刷新
    assetRefresh.RefreshAlbum();
    // 相册刷新信息和数据库信息、通知信息
    vector<AlbumChangeInfo> initAlbumInfos =
        { IMAGE_ALBUM_INFO, CLOUD_ENHANCEMENT_ALBUM_INFO, GetUserInsertInfo(assetInfo.ownerAlbumId_)};
    map<int32_t, AlbumChangeData> albumChangeDatas;
    EXPECT_TRUE(CheckInsertAlbumInfos(assetRefresh, initAlbumInfos, queryAssetInfo, albumChangeDatas));
    
    // Notify
    assetRefresh.Notify();
    // 资产通知
    auto notifyAssetInfos = assetRefresh.notifyExe_.notifyInfos_;
    EXPECT_TRUE(notifyAssetInfos.size() == 1);
    EXPECT_TRUE(CheckInsertAssetNotifyInfo(*(notifyAssetInfos.begin()), assetChangeData,
        Notification::ASSET_OPERATION_ADD));
    
    auto notifyAlbumInfos = assetRefresh.albumRefreshExe_.albumRefresh_.notifyExe_.notifyInfos_;
    EXPECT_TRUE(notifyAlbumInfos.size() == 1);
    EXPECT_TRUE(
        CheckInsertNotifyAlbumInfos(notifyAlbumInfos, Notification::ALBUM_OPERATION_UPDATE, albumChangeDatas, 3));
}

HWTEST_F(AssetAccurateRefreshTest, Insert_008, TestSize.Level2)
{
    AssetAccurateRefresh assetRefresh("Insert_008");
    int64_t outRowId = 0;
    auto assetInfo = GetTrashAsset();
    auto value = GetAssetInsertValue(assetInfo);
    auto ret = assetRefresh.Insert(outRowId, PhotoColumn::PHOTOS_TABLE, value);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(outRowId > 1);
    // 数据库
    auto queryAssetInfo = GetAssetInfo(assetInfo.fileId_);
    EXPECT_TRUE(CheckAssetEqual(assetInfo, queryAssetInfo));

    // data manager
    auto changeDatasMap = assetRefresh.dataManager_.changeDatas_;
    PhotoAssetChangeData assetChangeData;
    assetChangeData.operation_ = RDB_OPERATION_ADD;
    assetChangeData.infoAfterChange_ = queryAssetInfo;
    EXPECT_TRUE(changeDatasMap.size() == 1);
    auto iter = changeDatasMap.begin();
    EXPECT_TRUE(CheckAssetChangeData(iter->second, assetChangeData));

    // album 刷新
    assetRefresh.RefreshAlbum();
    // 相册刷新信息和数据库信息、通知信息
    vector<AlbumChangeInfo> initAlbumInfos = { TRASH_ALBUM_INFO };
    map<int32_t, AlbumChangeData> albumChangeDatas;
    EXPECT_TRUE(CheckInsertAlbumInfos(assetRefresh, initAlbumInfos, queryAssetInfo, albumChangeDatas));
    
    // Notify
    assetRefresh.Notify();
    // 资产通知
    auto notifyAssetInfos = assetRefresh.notifyExe_.notifyInfos_;
    EXPECT_TRUE(notifyAssetInfos.size() == 1);
    EXPECT_TRUE(CheckInsertAssetNotifyInfo(*(notifyAssetInfos.begin()), assetChangeData,
        Notification::ASSET_OPERATION_ADD_TRASH));
    
    auto notifyAlbumInfos = assetRefresh.albumRefreshExe_.albumRefresh_.notifyExe_.notifyInfos_;
    EXPECT_TRUE(notifyAlbumInfos.size() == 1);
    EXPECT_TRUE(CheckInsertNotifyAlbumInfos(notifyAlbumInfos, Notification::ALBUM_OPERATION_UPDATE_TRASH,
        albumChangeDatas, 1));
}

// trash 非封面
HWTEST_F(AssetAccurateRefreshTest, Update_009, TestSize.Level2)
{
    PrepareNormalAssets();
    // 修改1个
    ModifyAssetDateTime(12345); // dateTaken时间小于所有的相册coverDateTime

    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, FAVORITE_IMAGE_ASSET_FILE_ID);
    ValuesBucket value;
    int64_t dataTrashTime = 1000000;
    value.PutInt(PhotoColumn::MEDIA_DATE_TRASHED, dataTrashTime);
    AssetAccurateRefresh assetRefresh("Update_009");
    int32_t changedRow = 0;
    auto ret = assetRefresh.Update(changedRow, value, predicates);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changedRow == 1);
    // 数据库
    auto queryAssetInfo = GetAssetInfo(FAVORITE_IMAGE_ASSET_FILE_ID);
    EXPECT_TRUE(queryAssetInfo.dateTrashedMs_ == dataTrashTime);

    // data manager
    auto changeDatasMap = assetRefresh.dataManager_.changeDatas_;
    PhotoAssetChangeData assetChangeData;
    assetChangeData.operation_ = RDB_OPERATION_UPDATE;
    assetChangeData.infoBeforeChange_ = GetFavoriteImageAsset();
    assetChangeData.infoAfterChange_ = queryAssetInfo;
    EXPECT_TRUE(changeDatasMap.size() == 1);
    auto iter = changeDatasMap.begin();
    EXPECT_TRUE(CheckAssetChangeData(iter->second, assetChangeData));

    // album 刷新
    assetRefresh.RefreshAlbum();
    // 相册刷新信息和数据库信息、通知信息
    vector<AlbumChangeInfo> initAlbumInfos = { FAVORITE_ALBUM_INFO, IMAGE_ALBUM_INFO, TRASH_ALBUM_INFO,
        GetUserInsertInfo(FAVORITE_IMAGE_ASSET_ALBUM_ID)};
    map<int32_t, AlbumChangeData> albumChangeDatas;
    
    // 刷新信息和数据库信息
    auto refreshAlbumsMap = assetRefresh.albumRefreshExe_.refreshAlbums_;
    AlbumChangeInfo refreshAlbumInfo = FAVORITE_ALBUM_INFO;
    refreshAlbumInfo.count_--;
    refreshAlbumInfo.imageCount_--;
    CheckAlbumInfo(refreshAlbumsMap, FAVORITE_ALBUM_INFO, refreshAlbumInfo, RDB_OPERATION_UPDATE, albumChangeDatas);

    refreshAlbumInfo = IMAGE_ALBUM_INFO;
    refreshAlbumInfo.count_--;
    refreshAlbumInfo.imageCount_--;
    CheckAlbumInfo(refreshAlbumsMap, IMAGE_ALBUM_INFO, refreshAlbumInfo, RDB_OPERATION_UPDATE, albumChangeDatas);
    
    refreshAlbumInfo = GetUserInsertInfo(FAVORITE_IMAGE_ASSET_ALBUM_ID);
    refreshAlbumInfo.count_--;
    refreshAlbumInfo.imageCount_--;
    CheckAlbumInfo(refreshAlbumsMap, GetUserInsertInfo(FAVORITE_IMAGE_ASSET_ALBUM_ID), refreshAlbumInfo,
        RDB_OPERATION_UPDATE, albumChangeDatas);
    // trash不更新封面
    refreshAlbumInfo = TRASH_ALBUM_INFO;
    refreshAlbumInfo.count_++;
    refreshAlbumInfo.imageCount_++;
    CheckAlbumInfo(refreshAlbumsMap, TRASH_ALBUM_INFO, refreshAlbumInfo, RDB_OPERATION_UPDATE, albumChangeDatas);
    
    // Notify
    assetRefresh.Notify();
    // 资产通知
    auto notifyAssetInfos = assetRefresh.notifyExe_.notifyInfos_;
    EXPECT_TRUE(notifyAssetInfos.size() == 2);
    CheckAssetNotifyInfo(notifyAssetInfos, Notification::ASSET_OPERATION_UPDATE_ADD_TRASH, assetChangeData);
    CheckAssetNotifyInfo(notifyAssetInfos, Notification::ASSET_OPERATION_UPDATE_REMOVE_NORMAL, assetChangeData);

    auto notifyAlbumInfos = assetRefresh.albumRefreshExe_.albumRefresh_.notifyExe_.notifyInfos_;
    EXPECT_TRUE(notifyAlbumInfos.size() == 2);
    EXPECT_TRUE(
        CheckInsertNotifyAlbumInfos(notifyAlbumInfos, Notification::ALBUM_OPERATION_UPDATE_TRASH, albumChangeDatas, 1));
    EXPECT_TRUE(
        CheckInsertNotifyAlbumInfos(notifyAlbumInfos, Notification::ALBUM_OPERATION_UPDATE, albumChangeDatas, 3));
}

HWTEST_F(AssetAccurateRefreshTest, Update_Exceed_010, TestSize.Level2)
{
    PrepareNormalAssets();
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, FAVORITE_IMAGE_ASSET_FILE_ID);
    ValuesBucket value;
    int64_t dataTrashTime = 1000000;
    value.PutInt(PhotoColumn::MEDIA_DATE_TRASHED, dataTrashTime);
    AssetAccurateRefresh assetRefresh("Update_Exceed_010");
    int32_t changedRow = 0;
    auto ret = assetRefresh.Update(changedRow, value, predicates);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changedRow == 1);

    // 修改changeDatas_的数量
    auto &changeDatasMap = assetRefresh.dataManager_.changeDatas_;
    PhotoAssetChangeData changeData;
    // 总共1000条
    for (int i = 0; i < 999; ++i) {
        changeDatasMap.insert_or_assign(1000000 + i, changeData);
    }
    ValuesBucket newValue;
    newValue.PutInt(PhotoColumn::MEDIA_DATE_TRASHED, 0);
    ret = assetRefresh.Update(changedRow, newValue, predicates);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changedRow == 1);
    // 总共1000条
    EXPECT_TRUE(assetRefresh.dataManager_.CheckIsExceed());
    EXPECT_TRUE(assetRefresh.dataManager_.changeDatas_.empty());
    EXPECT_TRUE(assetRefresh.RefreshAlbum() == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(assetRefresh.Notify() == ACCURATE_REFRESH_RET_OK);
}

HWTEST_F(AssetAccurateRefreshTest, Update_Exceed_011, TestSize.Level2)
{
    PrepareNormalAssets();
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, FAVORITE_IMAGE_ASSET_FILE_ID);
    ValuesBucket value;
    int64_t dataTrashTime = 1000000;
    value.PutInt(PhotoColumn::MEDIA_DATE_TRASHED, dataTrashTime);
    AssetAccurateRefresh assetRefresh;
    int32_t changedRow = 0;
    auto ret = assetRefresh.Update(changedRow, value, predicates);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changedRow == 1);

    // 修改changeDatas_的数量
    auto &changeDatasMap = assetRefresh.dataManager_.changeDatas_;
    PhotoAssetChangeData changeData;
    // 总共999条
    for (int i = 0; i < 998; ++i) {
        changeDatasMap.insert_or_assign(1000000 + i, changeData);
    }
    ValuesBucket newValue;
    newValue.PutInt(PhotoColumn::MEDIA_DATE_TRASHED, 0);
    ret = assetRefresh.Update(changedRow, newValue, predicates);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changedRow == 1);
    EXPECT_TRUE(!assetRefresh.dataManager_.CheckIsExceed());
    // 总共999条
    EXPECT_TRUE(assetRefresh.dataManager_.changeDatas_.size() == 999);
}

HWTEST_F(AssetAccurateRefreshTest, Update_MultiThread_012, TestSize.Level2)
{
    PrepareNormalAssets();

    std::thread taskTrash([]() {
        ACCURATE_DEBUG("taskTrash start");
        RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
        predicates.EqualTo(PhotoColumn::MEDIA_ID, FAVORITE_IMAGE_ASSET_FILE_ID);
        ValuesBucket value;
        int64_t dataTrashTime = 1000000;
        value.PutInt(PhotoColumn::MEDIA_DATE_TRASHED, dataTrashTime);
        AssetAccurateRefresh assetRefresh("Update_MultiThread_012");
        int32_t changedRow = 0;
        auto ret = assetRefresh.Update(changedRow, value, predicates);
        EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
        EXPECT_TRUE(changedRow == 1);
        std::this_thread::sleep_for(chrono::milliseconds(300));
        // album 刷新
        assetRefresh.RefreshAlbum();
        ACCURATE_DEBUG("taskTrash end");
        assetRefresh.Notify();
    });

    std::thread taskFavorite([]() {
        ACCURATE_DEBUG("taskFavorite start");
        RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
        predicates.EqualTo(PhotoColumn::MEDIA_ID, FAVORITE_IMAGE_ASSET_FILE_ID);
        ValuesBucket value;
        value.PutInt(PhotoColumn::MEDIA_IS_FAV, 0);
        AssetAccurateRefresh assetRefresh("Update_MultiThread_012");
        int32_t changedRow = 0;
        auto ret = assetRefresh.Update(changedRow, value, predicates);
        EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
        EXPECT_TRUE(changedRow == 1);
        // album 刷新
        std::this_thread::sleep_for(chrono::milliseconds(100));
        assetRefresh.RefreshAlbum();
        ACCURATE_DEBUG("taskFavorite end");
        assetRefresh.Notify();
    });
    taskTrash.join();
    taskFavorite.join();

    auto favoriteAlbumInfo = AccurateRefresh::GetAlbumInfo(FAVORITE_ALBUM_ID, g_rdbStore);
    auto trashAlbumInfo = AccurateRefresh::GetAlbumInfo(TRASH_ALBUM_ID, g_rdbStore);
    auto imageAlbumInfo = AccurateRefresh::GetAlbumInfo(IMAGE_ALBUM_ID, g_rdbStore);
    auto ownerAlbumInfo = AccurateRefresh::GetAlbumInfo(FAVORITE_IMAGE_ASSET_ALBUM_ID, g_rdbStore);
    EXPECT_TRUE(favoriteAlbumInfo.count_ == FAVORITE_ALBUM_COUNT - 1);
    EXPECT_TRUE(trashAlbumInfo.count_ == TRASH_ALBUM_COUNT + 1);
    EXPECT_TRUE(imageAlbumInfo.count_ == IMAGE_ALBUM_COUNT - 1);
    EXPECT_TRUE(ownerAlbumInfo.count_ == USER_ALBUM_COUNT - 1);
}

void InsertAndUpdateMultiThread(int32_t delayTime)
{
    {
        RdbPredicates predicates(PhotoAlbumColumns::TABLE);
        vector<string> albumIds = {
            to_string(FAVORITE_ALBUM_ID),
            to_string(IMAGE_ALBUM_ID),
            to_string(FAVORITE_IMAGE_ASSET_ALBUM_ID)
        };
        predicates.In(PhotoAlbumColumns::ALBUM_ID, albumIds);
        ValuesBucket value;
        value.PutInt(PhotoAlbumColumns::ALBUM_COUNT, 0);
        int32_t changedRows = 0;
        auto ret = g_rdbStore->Update(changedRows, value, predicates);
        EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    }

    bool isRefreshAllInsert = false;
    std::thread taskInsert([&]() {
        AssetAccurateRefresh assetRefresh("InsertAndUpdateMultiThread");
        int64_t outRowId = 0;
        auto assetInfo = GetFavoriteImageAsset();
        auto value = GetAssetInsertValue(assetInfo);
        auto ret = assetRefresh.Insert(outRowId, PhotoColumn::PHOTOS_TABLE, value);
        EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
        isRefreshAllInsert = assetRefresh.dataManager_.isForRecheck_;
        if (isRefreshAllInsert) {
            std::this_thread::sleep_for(chrono::milliseconds(200));
        }
        // album 刷新
        assetRefresh.RefreshAlbum();
        // Notify
        assetRefresh.Notify();
    });

    int changedRow = 0;
    bool isRefreshAllUpdate = false;
    std::thread taskUpdate([&]() {
        RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
        predicates.EqualTo(PhotoColumn::MEDIA_ID, FAVORITE_IMAGE_ASSET_FILE_ID);
        ValuesBucket value;
        value.PutInt(PhotoColumn::MEDIA_IS_FAV, 0);
        AssetAccurateRefresh assetRefresh("InsertAndUpdateMultiThread");
        std::this_thread::sleep_for(chrono::milliseconds(delayTime));
        auto ret = assetRefresh.Update(changedRow, value, predicates);
        EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
        isRefreshAllUpdate = assetRefresh.dataManager_.isForRecheck_;
        // refresh all时，放在最后处理，才能获取准确的结果
        if (isRefreshAllUpdate) {
            std::this_thread::sleep_for(chrono::milliseconds(200));
        }
        // album 刷新
        assetRefresh.RefreshAlbum();
        // Notify
        assetRefresh.Notify();
    });
    taskInsert.join();
    taskUpdate.join();
    auto favoriteAlbumInfo = AccurateRefresh::GetAlbumInfo(FAVORITE_ALBUM_ID, g_rdbStore);
    auto imageAlbumInfo = AccurateRefresh::GetAlbumInfo(IMAGE_ALBUM_ID, g_rdbStore);
    auto ownerAlbumInfo = AccurateRefresh::GetAlbumInfo(FAVORITE_IMAGE_ASSET_ALBUM_ID, g_rdbStore);

    // refresh all时，会把数据都刷成0
    if (isRefreshAllInsert || isRefreshAllUpdate) {
        EXPECT_TRUE(favoriteAlbumInfo.count_ == 0);
        EXPECT_TRUE(imageAlbumInfo.count_ == 0);
        EXPECT_TRUE(ownerAlbumInfo.count_ == 0);
        ACCURATE_DEBUG("refresh all");
        return;
    }

    if (changedRow == 1) {
        EXPECT_TRUE(imageAlbumInfo.count_ == 1);
        EXPECT_TRUE(ownerAlbumInfo.count_ == 1);
        ACCURATE_DEBUG("update");
    } else {
        EXPECT_TRUE(favoriteAlbumInfo.count_ == 1);
        EXPECT_TRUE(imageAlbumInfo.count_ == 1);
        EXPECT_TRUE(ownerAlbumInfo.count_ == 1);
        ACCURATE_DEBUG("no update");
    }
}

HWTEST_F(AssetAccurateRefreshTest, Insert_MultiThread_013, TestSize.Level2)
{
    InsertAndUpdateMultiThread(0);
}

HWTEST_F(AssetAccurateRefreshTest, Insert_MultiThread_014, TestSize.Level2)
{
    InsertAndUpdateMultiThread(5);
}

HWTEST_F(AssetAccurateRefreshTest, Insert_MultiThread_015, TestSize.Level2)
{
    InsertAndUpdateMultiThread(10);
}

HWTEST_F(AssetAccurateRefreshTest, Insert_MultiThread_016, TestSize.Level2)
{
    InsertAndUpdateMultiThread(15);
}

HWTEST_F(AssetAccurateRefreshTest, Update_MultiThread_017, TestSize.Level2)
{
    PrepareNormalAssets();

    std::thread taskTrash([]() {
        ACCURATE_DEBUG("taskTrash start");
        RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
        predicates.EqualTo(PhotoColumn::MEDIA_ID, FAVORITE_IMAGE_ASSET_FILE_ID);
        ValuesBucket value;
        int64_t dataTrashTime = 1000000;
        value.PutLong(PhotoColumn::MEDIA_DATE_TRASHED, dataTrashTime);
        AssetAccurateRefresh assetRefresh("Update_MultiThread_017");
        int32_t changedRow = 0;
        auto ret = assetRefresh.Update(changedRow, value, predicates);
        EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
        EXPECT_TRUE(changedRow == 1);

        ValuesBucket value2;
        value2.PutInt(PhotoColumn::MEDIA_DATE_TRASHED, 0);
        ret = assetRefresh.Update(changedRow, value2, predicates);
        EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
        EXPECT_TRUE(changedRow == 1);

        std::this_thread::sleep_for(chrono::milliseconds(300));
        // album 刷新
        assetRefresh.RefreshAlbum();
        ACCURATE_DEBUG("taskTrash end");
        assetRefresh.Notify();
    });

    std::thread taskFavorite([]() {
        ACCURATE_DEBUG("taskFavorite start");
        RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
        predicates.EqualTo(PhotoColumn::MEDIA_ID, FAVORITE_IMAGE_ASSET_FILE_ID);
        ValuesBucket value;
        value.PutInt(PhotoColumn::MEDIA_IS_FAV, 0);
        AssetAccurateRefresh assetRefresh("Update_MultiThread_017");
        int32_t changedRow = 0;
        auto ret = assetRefresh.Update(changedRow, value, predicates);
        EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
        EXPECT_TRUE(changedRow == 1);
        // album 刷新
        std::this_thread::sleep_for(chrono::milliseconds(100));
        assetRefresh.RefreshAlbum();
        ACCURATE_DEBUG("taskFavorite end");
        assetRefresh.Notify();
    });
    taskTrash.join();
    taskFavorite.join();

    auto favoriteAlbumInfo = AccurateRefresh::GetAlbumInfo(FAVORITE_ALBUM_ID, g_rdbStore);
    auto trashAlbumInfo = AccurateRefresh::GetAlbumInfo(TRASH_ALBUM_ID, g_rdbStore);
    auto imageAlbumInfo = AccurateRefresh::GetAlbumInfo(IMAGE_ALBUM_ID, g_rdbStore);
    auto ownerAlbumInfo = AccurateRefresh::GetAlbumInfo(FAVORITE_IMAGE_ASSET_ALBUM_ID, g_rdbStore);
    EXPECT_TRUE(favoriteAlbumInfo.count_ == FAVORITE_ALBUM_COUNT - 1);
    EXPECT_TRUE(trashAlbumInfo.count_ == TRASH_ALBUM_COUNT);
    EXPECT_TRUE(imageAlbumInfo.count_ == IMAGE_ALBUM_COUNT);
    EXPECT_TRUE(ownerAlbumInfo.count_ == USER_ALBUM_COUNT);
}

HWTEST_F(AssetAccurateRefreshTest, MultiThread_Clear_018, TestSize.Level2)
{
    PrepareNormalAssets();
    std::thread taskTrash([]() {
        ACCURATE_DEBUG("taskTrash start");
        RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
        predicates.EqualTo(PhotoColumn::MEDIA_ID, FAVORITE_IMAGE_ASSET_FILE_ID);
        ValuesBucket value;
        int64_t dataTrashTime = 1000000;
        value.PutLong(PhotoColumn::MEDIA_DATE_TRASHED, dataTrashTime);
        AssetAccurateRefresh assetRefresh("MultiThread_Clear_018");
        int32_t changedRow = 0;
        auto ret = assetRefresh.Update(changedRow, value, predicates);
        EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
        EXPECT_TRUE(changedRow == 1);

        ValuesBucket value2;
        value2.PutInt(PhotoColumn::MEDIA_DATE_TRASHED, 0);
        ret = assetRefresh.Update(changedRow, value2, predicates);
        EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
        EXPECT_TRUE(changedRow == 1);
        auto infos = MultiThreadAssetChangeInfoMgr::GetInstance().GetAssetChangeData(FAVORITE_IMAGE_ASSET_FILE_ID);
        EXPECT_TRUE(infos.first.fileId_ == FAVORITE_IMAGE_ASSET_FILE_ID);
        EXPECT_TRUE(infos.second.fileId_ == FAVORITE_IMAGE_ASSET_FILE_ID);
        auto multiAssetMap = MultiThreadAssetChangeInfoMgr::GetInstance().assetChangeDataMap_;
        auto iter = multiAssetMap.find(FAVORITE_IMAGE_ASSET_FILE_ID);
        EXPECT_TRUE(iter != multiAssetMap.end());
        auto data = iter->second;
        EXPECT_TRUE(data.count_ >= 2);
        EXPECT_TRUE(data.isMultiOperation_);
        ACCURATE_DEBUG("count[%{public}d]", data.count_);
        std::this_thread::sleep_for(chrono::milliseconds(100));
    });

    std::thread taskTrash2([]() {
        ACCURATE_DEBUG("taskTrash start");
        RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
        predicates.EqualTo(PhotoColumn::MEDIA_ID, FAVORITE_IMAGE_ASSET_FILE_ID);
        ValuesBucket value;
        int64_t dataTrashTime = 1000000;
        value.PutLong(PhotoColumn::MEDIA_DATE_TRASHED, dataTrashTime);
        AssetAccurateRefresh assetRefresh("MultiThread_Clear_018");
        int32_t changedRow = 0;
        auto ret = assetRefresh.Update(changedRow, value, predicates);
        EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
        EXPECT_TRUE(changedRow == 1);

        ValuesBucket value2;
        value2.PutInt(PhotoColumn::MEDIA_DATE_TRASHED, 0);
        ret = assetRefresh.Update(changedRow, value2, predicates);
        EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
        EXPECT_TRUE(changedRow == 1);
        auto infos = MultiThreadAssetChangeInfoMgr::GetInstance().GetAssetChangeData(FAVORITE_IMAGE_ASSET_FILE_ID);
        EXPECT_TRUE(infos.first.fileId_ == FAVORITE_IMAGE_ASSET_FILE_ID);
        EXPECT_TRUE(infos.second.fileId_ == FAVORITE_IMAGE_ASSET_FILE_ID);
        auto multiAssetMap = MultiThreadAssetChangeInfoMgr::GetInstance().assetChangeDataMap_;
        auto iter = multiAssetMap.find(FAVORITE_IMAGE_ASSET_FILE_ID);
        EXPECT_TRUE(iter != multiAssetMap.end());
        auto data = iter->second;
        EXPECT_TRUE(data.count_ >= 2);
        EXPECT_TRUE(data.isMultiOperation_);
        ACCURATE_DEBUG("count[%{public}d]", data.count_);
        std::this_thread::sleep_for(chrono::milliseconds(100));
    });

    taskTrash.join();
    taskTrash2.join();
    auto infos = MultiThreadAssetChangeInfoMgr::GetInstance().GetAssetChangeData(FAVORITE_IMAGE_ASSET_FILE_ID);
    EXPECT_TRUE(infos.first.fileId_ == INVALID_INT32_VALUE);
    EXPECT_TRUE(infos.second.fileId_ == INVALID_INT32_VALUE);
}

HWTEST_F(AssetAccurateRefreshTest, MultiThread_Clear_019, TestSize.Level2)
{
    PrepareNormalAssets();
    std::thread taskTrash([]() {
        ACCURATE_DEBUG("taskTrash start");
        RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
        predicates.EqualTo(PhotoColumn::MEDIA_ID, FAVORITE_IMAGE_ASSET_FILE_ID);
        ValuesBucket value;
        int64_t dataTrashTime = 1000000;
        value.PutLong(PhotoColumn::MEDIA_DATE_TRASHED, dataTrashTime);
        shared_ptr<AssetAccurateRefresh> assetRefresh = make_shared<AssetAccurateRefresh>("MultiThread_Clear_019");
        int32_t changedRow = 0;
        auto ret = assetRefresh->Update(changedRow, value, predicates);
        EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
        EXPECT_TRUE(changedRow == 1);

        ValuesBucket value2;
        value2.PutInt(PhotoColumn::MEDIA_DATE_TRASHED, 0);
        ret = assetRefresh->Update(changedRow, value2, predicates);
        EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
        EXPECT_TRUE(changedRow == 1);
        auto infos = MultiThreadAssetChangeInfoMgr::GetInstance().GetAssetChangeData(FAVORITE_IMAGE_ASSET_FILE_ID);
        EXPECT_TRUE(infos.first.fileId_ == FAVORITE_IMAGE_ASSET_FILE_ID);
        EXPECT_TRUE(infos.second.fileId_ == FAVORITE_IMAGE_ASSET_FILE_ID);
        auto multiAssetMap = MultiThreadAssetChangeInfoMgr::GetInstance().assetChangeDataMap_;
        auto iter = multiAssetMap.find(FAVORITE_IMAGE_ASSET_FILE_ID);
        EXPECT_TRUE(iter != multiAssetMap.end());
        auto data = iter->second;
        EXPECT_TRUE(data.count_ >= 2);
        EXPECT_TRUE(data.isMultiOperation_);
        ACCURATE_DEBUG("count[%{public}d]", data.count_);
        std::this_thread::sleep_for(chrono::milliseconds(100));
    });

    std::thread taskTrash2([]() {
        ACCURATE_DEBUG("taskTrash start");
        RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
        predicates.EqualTo(PhotoColumn::MEDIA_ID, FAVORITE_IMAGE_ASSET_FILE_ID);
        ValuesBucket value;
        int64_t dataTrashTime = 1000000;
        value.PutLong(PhotoColumn::MEDIA_DATE_TRASHED, dataTrashTime);
        shared_ptr<AssetAccurateRefresh> assetRefresh = make_shared<AssetAccurateRefresh>("MultiThread_Clear_019");
        int32_t changedRow = 0;
        auto ret = assetRefresh->Update(changedRow, value, predicates);
        EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
        EXPECT_TRUE(changedRow == 1);

        ValuesBucket value2;
        value2.PutInt(PhotoColumn::MEDIA_DATE_TRASHED, 0);
        ret = assetRefresh->Update(changedRow, value2, predicates);
        EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
        EXPECT_TRUE(changedRow == 1);
        auto infos = MultiThreadAssetChangeInfoMgr::GetInstance().GetAssetChangeData(FAVORITE_IMAGE_ASSET_FILE_ID);
        EXPECT_TRUE(infos.first.fileId_ == FAVORITE_IMAGE_ASSET_FILE_ID);
        EXPECT_TRUE(infos.second.fileId_ == FAVORITE_IMAGE_ASSET_FILE_ID);
        auto multiAssetMap = MultiThreadAssetChangeInfoMgr::GetInstance().assetChangeDataMap_;
        auto iter = multiAssetMap.find(FAVORITE_IMAGE_ASSET_FILE_ID);
        EXPECT_TRUE(iter != multiAssetMap.end());
        auto data = iter->second;
        EXPECT_TRUE(data.count_ >= 2);
        EXPECT_TRUE(data.isMultiOperation_);
        ACCURATE_DEBUG("count[%{public}d]", data.count_);
        std::this_thread::sleep_for(chrono::milliseconds(100));
    });

    taskTrash.join();
    taskTrash2.join();
    auto infos = MultiThreadAssetChangeInfoMgr::GetInstance().GetAssetChangeData(FAVORITE_IMAGE_ASSET_FILE_ID);
    EXPECT_TRUE(infos.first.fileId_ == INVALID_INT32_VALUE);
    EXPECT_TRUE(infos.second.fileId_ == INVALID_INT32_VALUE);
}

HWTEST_F(AssetAccurateRefreshTest, AccurateRefreshAlbum_020, TestSize.Level2)
{
    PrepareNormalAssets();

    ACCURATE_DEBUG("taskTrash start");
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, FAVORITE_IMAGE_ASSET_FILE_ID);
    ValuesBucket value;
    int64_t dataTrashTime = 1000000;
    value.PutInt(PhotoColumn::MEDIA_DATE_TRASHED, dataTrashTime);
    AssetAccurateRefresh assetRefresh("AccurateRefreshAlbum_020");
    int32_t changedRow = 0;
    auto ret = assetRefresh.Update(changedRow, value, predicates);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changedRow == 1);
    // album 刷新
    assetRefresh.RefreshAlbum();
    ACCURATE_DEBUG("taskTrash end");
    assetRefresh.Notify();

    auto favoriteAlbumInfo = AccurateRefresh::GetAlbumInfo(FAVORITE_ALBUM_ID, g_rdbStore);
    auto trashAlbumInfo = AccurateRefresh::GetAlbumInfo(TRASH_ALBUM_ID, g_rdbStore);
    auto imageAlbumInfo = AccurateRefresh::GetAlbumInfo(IMAGE_ALBUM_ID, g_rdbStore);
    auto ownerAlbumInfo = AccurateRefresh::GetAlbumInfo(FAVORITE_IMAGE_ASSET_ALBUM_ID, g_rdbStore);
    EXPECT_TRUE(favoriteAlbumInfo.count_ == FAVORITE_ALBUM_COUNT - 1);
    EXPECT_TRUE(trashAlbumInfo.count_ == TRASH_ALBUM_COUNT + 1);
    EXPECT_TRUE(imageAlbumInfo.count_ == IMAGE_ALBUM_COUNT - 1);
    EXPECT_TRUE(ownerAlbumInfo.count_ == USER_ALBUM_COUNT - 1);
}

HWTEST_F(AssetAccurateRefreshTest, IgnoreRefreshAlbum_021, TestSize.Level2)
{
    PrepareNormalAssets();

    ACCURATE_DEBUG("taskTrash start");
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, FAVORITE_IMAGE_ASSET_FILE_ID);
    ValuesBucket value;
    int64_t dataTrashTime = 1000000;
    value.PutInt(PhotoColumn::MEDIA_DATE_TRASHED, dataTrashTime);
    AssetAccurateRefresh assetRefresh("IgnoreRefreshAlbum_021");
    int32_t changedRow = 0;
    auto ret = assetRefresh.Update(changedRow, value, predicates);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changedRow == 1);

    auto timestamp = AlbumAccurateRefreshManager::GetInstance().GetCurrentRefreshTag();
    AlbumRefreshTimestamp albumTimestamp(timestamp + 100, timestamp + 100);
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(FAVORITE_ALBUM_ID, true, albumTimestamp);
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(FAVORITE_ALBUM_ID, false, albumTimestamp);
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(TRASH_ALBUM_ID, true, albumTimestamp);
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(TRASH_ALBUM_ID, false, albumTimestamp);
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(IMAGE_ALBUM_ID, true, albumTimestamp);
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(IMAGE_ALBUM_ID, false, albumTimestamp);
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(FAVORITE_IMAGE_ASSET_ALBUM_ID, true, albumTimestamp);
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(FAVORITE_IMAGE_ASSET_ALBUM_ID, false,
        albumTimestamp);

    // album 刷新
    assetRefresh.RefreshAlbum();
    ACCURATE_DEBUG("taskTrash end");
    assetRefresh.Notify();

    auto favoriteAlbumInfo = AccurateRefresh::GetAlbumInfo(FAVORITE_ALBUM_ID, g_rdbStore);
    auto trashAlbumInfo = AccurateRefresh::GetAlbumInfo(TRASH_ALBUM_ID, g_rdbStore);
    auto imageAlbumInfo = AccurateRefresh::GetAlbumInfo(IMAGE_ALBUM_ID, g_rdbStore);
    auto ownerAlbumInfo = AccurateRefresh::GetAlbumInfo(FAVORITE_IMAGE_ASSET_ALBUM_ID, g_rdbStore);
    EXPECT_TRUE(favoriteAlbumInfo.count_ == FAVORITE_ALBUM_COUNT);
    EXPECT_TRUE(trashAlbumInfo.count_ == TRASH_ALBUM_COUNT);
    EXPECT_TRUE(imageAlbumInfo.count_ == IMAGE_ALBUM_COUNT);
    EXPECT_TRUE(ownerAlbumInfo.count_ == USER_ALBUM_COUNT);
}

HWTEST_F(AssetAccurateRefreshTest, ForceRefreshAlbum_021, TestSize.Level2)
{
    PrepareNormalAssets();

    ACCURATE_DEBUG("taskTrash start");
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, FAVORITE_IMAGE_ASSET_FILE_ID);
    ValuesBucket value;
    int64_t dataTrashTime = 1000000;
    value.PutInt(PhotoColumn::MEDIA_DATE_TRASHED, dataTrashTime);
    AssetAccurateRefresh assetRefresh("ForceRefreshAlbum_021");
    int32_t changedRow = 0;
    auto ret = assetRefresh.Update(changedRow, value, predicates);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changedRow == 1);

    auto iter = assetRefresh.dataManager_.changeDatas_.find(FAVORITE_IMAGE_ASSET_FILE_ID);
    EXPECT_TRUE(iter != assetRefresh.dataManager_.changeDatas_.end());
    auto changeData = iter->second;
    auto timestampStart = changeData.infoBeforeChange_.timestamp_;
    AlbumRefreshTimestamp albumTimestamp(timestampStart - 100, timestampStart);
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(FAVORITE_ALBUM_ID, true, albumTimestamp);
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(FAVORITE_ALBUM_ID, false, albumTimestamp);

    // album 刷新
    assetRefresh.RefreshAlbum();
    ACCURATE_DEBUG("taskTrash end");
    assetRefresh.Notify();

    auto favoriteAlbumInfo = AccurateRefresh::GetAlbumInfo(FAVORITE_ALBUM_ID, g_rdbStore);
    auto trashAlbumInfo = AccurateRefresh::GetAlbumInfo(TRASH_ALBUM_ID, g_rdbStore);
    auto imageAlbumInfo = AccurateRefresh::GetAlbumInfo(IMAGE_ALBUM_ID, g_rdbStore);
    auto ownerAlbumInfo = AccurateRefresh::GetAlbumInfo(FAVORITE_IMAGE_ASSET_ALBUM_ID, g_rdbStore);
    EXPECT_TRUE(favoriteAlbumInfo.count_ == 0);
    EXPECT_TRUE(trashAlbumInfo.count_ == TRASH_ALBUM_COUNT + 1);
    EXPECT_TRUE(imageAlbumInfo.count_ == IMAGE_ALBUM_COUNT - 1);
    EXPECT_TRUE(ownerAlbumInfo.count_ == USER_ALBUM_COUNT - 1);
}

HWTEST_F(AssetAccurateRefreshTest, ForceRefreshAlbum_022, TestSize.Level2)
{
    PrepareNormalAssets();

    ACCURATE_DEBUG("taskTrash start");
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, FAVORITE_IMAGE_ASSET_FILE_ID);
    ValuesBucket value;
    int64_t dataTrashTime = 1000000;
    value.PutInt(PhotoColumn::MEDIA_DATE_TRASHED, dataTrashTime);
    AssetAccurateRefresh assetRefresh("ForceRefreshAlbum_022");
    int32_t changedRow = 0;
    auto ret = assetRefresh.Update(changedRow, value, predicates);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changedRow == 1);

    auto iter = assetRefresh.dataManager_.changeDatas_.find(FAVORITE_IMAGE_ASSET_FILE_ID);
    EXPECT_TRUE(iter != assetRefresh.dataManager_.changeDatas_.end());
    auto changeData = iter->second;
    auto timestampStart = changeData.infoBeforeChange_.timestamp_;
    auto timestampEnd = changeData.infoAfterChange_.timestamp_;
    AlbumRefreshTimestamp albumTimestamp(timestampStart - 100, timestampEnd + 100);
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(FAVORITE_ALBUM_ID, true, albumTimestamp);
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(FAVORITE_ALBUM_ID, false, albumTimestamp);

    // album 刷新
    assetRefresh.RefreshAlbum();
    ACCURATE_DEBUG("taskTrash end");
    assetRefresh.Notify();

    auto favoriteAlbumInfo = AccurateRefresh::GetAlbumInfo(FAVORITE_ALBUM_ID, g_rdbStore);
    auto trashAlbumInfo = AccurateRefresh::GetAlbumInfo(TRASH_ALBUM_ID, g_rdbStore);
    auto imageAlbumInfo = AccurateRefresh::GetAlbumInfo(IMAGE_ALBUM_ID, g_rdbStore);
    auto ownerAlbumInfo = AccurateRefresh::GetAlbumInfo(FAVORITE_IMAGE_ASSET_ALBUM_ID, g_rdbStore);
    EXPECT_TRUE(favoriteAlbumInfo.count_ == 0);
    EXPECT_TRUE(trashAlbumInfo.count_ == TRASH_ALBUM_COUNT + 1);
    EXPECT_TRUE(imageAlbumInfo.count_ == IMAGE_ALBUM_COUNT - 1);
    EXPECT_TRUE(ownerAlbumInfo.count_ == USER_ALBUM_COUNT - 1);
}

HWTEST_F(AssetAccurateRefreshTest, ForceRefreshAlbum_023, TestSize.Level2)
{
    PrepareNormalAssets();

    ACCURATE_DEBUG("taskTrash start");
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, FAVORITE_IMAGE_ASSET_FILE_ID);
    ValuesBucket value;
    int64_t dataTrashTime = 1000000;
    value.PutInt(PhotoColumn::MEDIA_DATE_TRASHED, dataTrashTime);
    AssetAccurateRefresh assetRefresh("ForceRefreshAlbum_023");
    int32_t changedRow = 0;
    auto ret = assetRefresh.Update(changedRow, value, predicates);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changedRow == 1);

    auto iter = assetRefresh.dataManager_.changeDatas_.find(FAVORITE_IMAGE_ASSET_FILE_ID);
    EXPECT_TRUE(iter != assetRefresh.dataManager_.changeDatas_.end());
    auto changeData = iter->second;
    auto timestampEnd = changeData.infoAfterChange_.timestamp_;
    AlbumRefreshTimestamp albumTimestamp(timestampEnd, timestampEnd + 100);
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(FAVORITE_ALBUM_ID, true, albumTimestamp);
    AlbumAccurateRefreshManager::GetInstance().SetRefreshTimestamp(FAVORITE_ALBUM_ID, false, albumTimestamp);

    // album 刷新
    assetRefresh.RefreshAlbum();
    ACCURATE_DEBUG("taskTrash end");
    assetRefresh.Notify();

    auto favoriteAlbumInfo = AccurateRefresh::GetAlbumInfo(FAVORITE_ALBUM_ID, g_rdbStore);
    auto trashAlbumInfo = AccurateRefresh::GetAlbumInfo(TRASH_ALBUM_ID, g_rdbStore);
    auto imageAlbumInfo = AccurateRefresh::GetAlbumInfo(IMAGE_ALBUM_ID, g_rdbStore);
    auto ownerAlbumInfo = AccurateRefresh::GetAlbumInfo(FAVORITE_IMAGE_ASSET_ALBUM_ID, g_rdbStore);
    EXPECT_TRUE(favoriteAlbumInfo.count_ == 0);
    EXPECT_TRUE(trashAlbumInfo.count_ == TRASH_ALBUM_COUNT + 1);
    EXPECT_TRUE(imageAlbumInfo.count_ == IMAGE_ALBUM_COUNT - 1);
    EXPECT_TRUE(ownerAlbumInfo.count_ == USER_ALBUM_COUNT - 1);
}

HWTEST_F(AssetAccurateRefreshTest, NotifyYuvReady_024, TestSize.Level2)
{
    PrepareNormalAssets();

    ACCURATE_DEBUG("NotifyYuvReady start");
    AssetAccurateRefresh assetRefresh("NotifyYuvReady_024");
    auto ret = ACCURATE_REFRESH_RET_OK;
    ret = assetRefresh.NotifyYuvReady(GetFavoriteImageAsset().fileId_);
    ACCURATE_DEBUG("NotifyYuvReady end");
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
}

HWTEST_F(AssetAccurateRefreshTest, NotifyYuvReady_025, TestSize.Level2)
{
    PrepareNormalAssets();

    ACCURATE_DEBUG("NotifyYuvReady start");
    AssetAccurateRefresh assetRefresh("NotifyYuvReady_025");
    auto ret = ACCURATE_REFRESH_RET_OK;
    ret = assetRefresh.NotifyYuvReady(GetFavoriteImageAsset().fileId_ + 10);
    ACCURATE_DEBUG("NotifyYuvReady end");
    EXPECT_TRUE(ret != ACCURATE_REFRESH_RET_OK);
}

HWTEST_F(AssetAccurateRefreshTest, NotifyYuvReady_026, TestSize.Level2)
{
    PrepareNormalAssets();

    ACCURATE_DEBUG("NotifyYuvReady start");
    AssetAccurateRefresh assetRefresh("NotifyYuvReady_026");

    int32_t ret = ACCURATE_REFRESH_RET_OK;
    const std::vector<int32_t> keys = { GetFavoriteImageAsset().fileId_ };
    assetRefresh.dataManager_.Init(keys);
    assetRefresh.UpdateModifiedDatasInner(keys, RDB_OPERATION_UPDATE);
    ret = assetRefresh.dataManager_.PostProcessModifiedDatas(keys);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    PhotoAssetChangeData changeData;
    ret = assetRefresh.dataManager_.GetChangeDataByKey(GetFavoriteImageAsset().fileId_, changeData);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    assetRefresh.notifyExe_.NotifyYuvReady(changeData);
    ACCURATE_DEBUG("NotifyYuvReady end");
}

HWTEST_F(AssetAccurateRefreshTest, NotifyYuvReady_027, TestSize.Level2)
{
    PrepareNormalAssets();

    ACCURATE_DEBUG("NotifyYuvReady start");
    AssetAccurateRefresh assetRefresh("NotifyYuvReady_027");

    int32_t ret = ACCURATE_REFRESH_RET_OK;
    const std::vector<int32_t> keys = { GetFavoriteImageAsset().fileId_ };
    assetRefresh.dataManager_.Init(keys);
    assetRefresh.UpdateModifiedDatasInner(keys, RDB_OPERATION_UPDATE);
    ret = assetRefresh.dataManager_.PostProcessModifiedDatas(keys);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    PhotoAssetChangeData changeData;
    ret = assetRefresh.dataManager_.GetChangeDataByKey(GetFavoriteImageAsset().fileId_ + 10, changeData);
    EXPECT_TRUE(ret != ACCURATE_REFRESH_RET_OK);
    ACCURATE_DEBUG("NotifyYuvReady end");
}
} // namespace Media
} // namespace OHOS