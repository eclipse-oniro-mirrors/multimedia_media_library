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

#define MLOG_TAG "MediaCloudSync"

#include "share_album_cloud_sync_test_utils.h"

#include <chrono>
#include <thread>

#include "cloud_media_sync_const.h"
#include "media_library_database.h"
#include "media_log.h"
#include "medialibrary_errno.h"

namespace OHOS::Media::CloudSync {
using namespace OHOS::Media::ORM;
using namespace OHOS::Media::TestUtils;

// 以下常量全部取自真实抓取的共享相册/共享资产记录
const std::string ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_CLOUD_ID =
    "md5dfe8e3778468ba0208b7d72bda1e5e7c78e4dfdf9fbc29716ac7f74022e9739f0";
const std::string ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_LPATH = "/Photoshare/1qqqq/1789955243197";
const std::string ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_NAME = "1qqqq";
const std::string ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_OWNER = "10086000872009183";
const std::string ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_CLOUD_ID =
    "ff323ac729f4482d9301f2cc9e1015bdcb25b293a8b646c083ed41eb4e6cd4ca";
const std::string ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_DISPLAY_NAME = "IMG_20260921_093126.heic";
const std::string ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_DATA_FILE_NAME = "IMG_1790222372_62219.heic";
const std::string ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_OWNER_INFO = "10086000872009183";
const std::string ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_UPLINK_CLOUD_ID =
    "415284596df342798e420670bbbc8c5c8b22afc561dc40b98bcc322fcd49d194";
const std::string ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_META_CLOUD_ID =
    "8d1ea44e257844cfa345a53d4e80467ae84c93deac524a5ab189d9f90d9488f2";
const std::string ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_UPDATED_NAME = "1qqqqwp";
const std::string ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_MOVED_LPATH =
    "/Photoshare/1qqqq_moved/1789955243197";
const std::string ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_JSON =
    "/data/test/cloudsync/share_album/share_album_records.json";
const std::string ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_UPDATE_JSON =
    "/data/test/cloudsync/share_album/share_album_update_records.json";
const std::string ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_UPDATE_LPATH_JSON =
    "/data/test/cloudsync/share_album/share_album_update_lpath_records.json";
const std::string ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_DELETE_JSON =
    "/data/test/cloudsync/share_album/share_album_delete_records.json";
const std::string ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_JSON =
    "/data/test/cloudsync/share_album/share_photo_records.json";
const std::string ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_DELETE_JSON =
    "/data/test/cloudsync/share_album/share_photo_delete_records.json";
const std::string ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_META_JSON =
    "/data/test/cloudsync/share_album/share_photo_meta_records.json";

namespace {
// 异步清理等待参数, 单位毫秒
constexpr int32_t ASYNC_WAIT_MAX_TIMES = 100;
constexpr int32_t ASYNC_WAIT_INTERVAL_MS = 100;
}  // namespace

std::shared_ptr<NativeRdb::RdbStore> ShareAlbumCloudSyncTestUtils::GetRdbStore()
{
    int32_t errorCode = 0;
    return MediaLibraryDatabase().GetRdbStore(errorCode);
}

std::shared_ptr<CloudMediaDataHandler> ShareAlbumCloudSyncTestUtils::MakeHandler(
    const std::string &tableName, int32_t sceneType)
{
    return std::make_shared<CloudMediaDataHandler>(tableName, CLOUD_TYPE, USER_ID, sceneType);
}

std::shared_ptr<CloudMediaDataHandler> ShareAlbumCloudSyncTestUtils::MakeShareAlbumHandler()
{
    return MakeHandler("PhotoAlbum", SCENE_TYPE_SHARE);
}

std::shared_ptr<CloudMediaDataHandler> ShareAlbumCloudSyncTestUtils::MakeSharePhotoHandler()
{
    return MakeHandler("Photos", SCENE_TYPE_SHARE);
}

bool ShareAlbumCloudSyncTestUtils::GetAlbumByCloudId(const std::string &cloudId, PhotoAlbumPo &album)
{
    AlbumDao dao;
    std::vector<PhotoAlbumPo> albumList = dao.QueryByCloudIds({cloudId});
    CHECK_AND_RETURN_RET(albumList.size() > 0, false);
    album = albumList.front();
    return true;
}

bool ShareAlbumCloudSyncTestUtils::GetPhotoByCloudId(const std::string &cloudId, PhotosPo &photo)
{
    PhotosDao dao;
    std::vector<PhotosPo> photosList = dao.QueryPhotosByCloudId(cloudId);
    CHECK_AND_RETURN_RET(photosList.size() > 0, false);
    photo = photosList.front();
    return true;
}

bool ShareAlbumCloudSyncTestUtils::GetAlbumById(int32_t albumId, PhotoAlbumPo &album)
{
    auto rdbStore = GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "GetAlbumById rdbStore is null");
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, albumId);
    std::vector<std::string> columns = {" * "};
    auto resultSet = rdbStore->Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "GetAlbumById query failed, albumId: %{public}d", albumId);
    std::vector<PhotoAlbumPo> albumList = ResultSetReader<PhotoAlbumPoWriter, PhotoAlbumPo>(resultSet).ReadRecords();
    resultSet->Close();
    CHECK_AND_RETURN_RET(albumList.size() > 0, false);
    album = albumList.front();
    return true;
}

int32_t ShareAlbumCloudSyncTestUtils::GetAlbumIdByCloudId(const std::string &cloudId)
{
    PhotoAlbumPo album;
    CHECK_AND_RETURN_RET_LOG(GetAlbumByCloudId(cloudId, album), E_ERR,
        "GetAlbumIdByCloudId failed, cloudId: %{public}s", cloudId.c_str());
    return album.albumId.value_or(E_ERR);
}

int32_t ShareAlbumCloudSyncTestUtils::CountAlbumsByCloudId(const std::string &cloudId)
{
    auto rdbStore = GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "CountAlbumsByCloudId rdbStore is null");
    std::string sql = "SELECT COUNT(*) AS count FROM " + PhotoAlbumColumns::TABLE + " WHERE " +
                      PhotoAlbumColumns::ALBUM_CLOUD_ID + " = ?;";
    std::vector<NativeRdb::ValueObject> bindArgs = {cloudId};
    auto resultSet = rdbStore->QuerySql(sql, bindArgs);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "CountAlbumsByCloudId query failed");
    int32_t count = 0;
    if (resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        resultSet->GetInt(0, count);
    }
    resultSet->Close();
    return count;
}

int32_t ShareAlbumCloudSyncTestUtils::CountShareAssetsOfAlbum(int32_t albumId)
{
    auto rdbStore = GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "CountShareAssetsOfAlbum rdbStore is null");
    std::string sql = "SELECT COUNT(*) AS count FROM " + PhotoColumn::PHOTOS_TABLE + " WHERE " +
                      PhotoColumn::PHOTO_OWNER_ALBUM_ID + " = ? AND " + PhotoColumn::PHOTO_IS_SHARED + " = ?;";
    std::vector<NativeRdb::ValueObject> bindArgs = {albumId, IS_SHARED_TRUE};
    auto resultSet = rdbStore->QuerySql(sql, bindArgs);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "CountShareAssetsOfAlbum query failed");
    int32_t count = 0;
    if (resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        resultSet->GetInt(0, count);
    }
    resultSet->Close();
    return count;
}

void ShareAlbumCloudSyncTestUtils::UpdateAlbumDirty(const std::string &cloudId, int32_t dirty)
{
    auto rdbStore = GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "UpdateAlbumDirty rdbStore is null");
    std::string sql = "UPDATE " + PhotoAlbumColumns::TABLE + " SET " + PhotoAlbumColumns::ALBUM_DIRTY + " = ? WHERE " +
                      PhotoAlbumColumns::ALBUM_CLOUD_ID + " = ?;";
    std::vector<NativeRdb::ValueObject> bindArgs = {dirty, cloudId};
    int32_t ret = rdbStore->ExecuteSql(sql, bindArgs);
    MEDIA_INFO_LOG("UpdateAlbumDirty cloudId: %{public}s, dirty: %{public}d, ret: %{public}d",
        cloudId.c_str(), dirty, ret);
}

int32_t ShareAlbumCloudSyncTestUtils::BindSharePhotosToShareAlbums()
{
    auto rdbStore = GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "BindSharePhotosToShareAlbums rdbStore is null");
    // 前两条资产在 SetUpTestCase 里已落库; 第三条(元数据用例用)在自动归属用例中才落库, 不在此绑定
    std::vector<std::pair<std::string, std::string>> photoAlbumPairs = {
        {SHARE_PHOTO_CLOUD_ID, SHARE_ALBUM_CLOUD_ID},
        {SHARE_PHOTO_UPLINK_CLOUD_ID, SHARE_ALBUM_CLOUD_ID},
    };
    std::string sql = "UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " + PhotoColumn::PHOTO_OWNER_ALBUM_ID +
        " = (SELECT " + PhotoAlbumColumns::ALBUM_ID + " FROM " + PhotoAlbumColumns::TABLE + " WHERE " +
        PhotoAlbumColumns::ALBUM_CLOUD_ID + " = ?) WHERE " + PhotoColumn::PHOTO_CLOUD_ID + " = ?;";
    for (const auto &photoAlbumPair : photoAlbumPairs) {
        std::vector<NativeRdb::ValueObject> bindArgs = {photoAlbumPair.second, photoAlbumPair.first};
        int32_t ret = rdbStore->ExecuteSql(sql, bindArgs);
        MEDIA_INFO_LOG("BindSharePhotosToShareAlbums photo: %{public}s, album: %{public}s, ret: %{public}d",
            photoAlbumPair.first.c_str(), photoAlbumPair.second.c_str(), ret);
        CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, ret,
            "bind share photo owner album failed, photo: %{public}s, ret: %{public}d",
            photoAlbumPair.first.c_str(), ret);
    }
    return E_OK;
}

void ShareAlbumCloudSyncTestUtils::PrepareSharePhotoForUpload(const std::string &cloudId, int32_t dirty)
{
    auto rdbStore = GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "PrepareSharePhotoForUpload rdbStore is null");
    // 待上行要求: dirty 为目标值, 且资产本身已就绪(缩略图/LCD 已访问, 不在回收站/隐藏/未决)
    std::string sql = "UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " + PhotoColumn::PHOTO_DIRTY + " = ?, " +
                      PhotoColumn::PHOTO_THUMBNAIL_READY + " = " + std::to_string(THUMBNAIL_READY_FOR_UPLOAD) +
                      ", " + PhotoColumn::PHOTO_LCD_VISIT_TIME + " = " + std::to_string(LCD_VISIT_TIME_FOR_UPLOAD) +
                      ", " + MediaColumn::MEDIA_TIME_PENDING + " = 0, " + MediaColumn::MEDIA_DATE_TRASHED +
                      " = 0, " + MediaColumn::MEDIA_HIDDEN + " = 0 WHERE " + PhotoColumn::PHOTO_CLOUD_ID + " = ?;";
    std::vector<NativeRdb::ValueObject> bindArgs = {dirty, cloudId};
    int32_t ret = rdbStore->ExecuteSql(sql, bindArgs);
    MEDIA_INFO_LOG("PrepareSharePhotoForUpload cloudId: %{public}s, dirty: %{public}d, ret: %{public}d",
        cloudId.c_str(), dirty, ret);
}

void ShareAlbumCloudSyncTestUtils::CleanShareTddData()
{
    auto rdbStore = GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "CleanShareTddData rdbStore is null");
    // 共享资产被标记删除时 cloud_id 会被清空, 因此再用真实文件路径与历史前缀兜底一次
    std::vector<NativeRdb::ValueObject> photoArgs = {SHARE_PHOTO_CLOUD_ID, SHARE_PHOTO_UPLINK_CLOUD_ID,
        SHARE_PHOTO_META_CLOUD_ID, "/storage/cloud/files/Photo/20043/IMG_1790222372_62219.heic",
        "/storage/cloud/files/Photo/20044/IMG_1790222372_62220.heic",
        "/storage/cloud/files/Photo/20045/IMG_1790222373_62221.heic", "share_tdd_%"};
    int32_t ret = rdbStore->ExecuteSql("DELETE FROM " + PhotoColumn::PHOTOS_TABLE + " WHERE " +
        PhotoColumn::PHOTO_CLOUD_ID + " IN (?, ?, ?) OR " + MediaColumn::MEDIA_FILE_PATH +
        " IN (?, ?, ?) OR " + PhotoColumn::PHOTO_CLOUD_ID + " LIKE ?;", photoArgs);
    MEDIA_INFO_LOG("CleanShareTddData delete photos ret: %{public}d", ret);
    // 资产按 source_path 回退建相册时会生成不带 cloud_id 的源相册, 必须按 lpath 前缀一并清掉,
    // 否则残留的源相册会占据相册映射里的 lpath 槽位, 影响后续用例的相册归属
    std::vector<NativeRdb::ValueObject> albumArgs = {SHARE_ALBUM_CLOUD_ID, "/photoshare/1qqqq/1789955243197",
        "/photoshare/1qqqq_moved/1789955243197", "/photoshare/0924zx/1790219611993", "/pictures/sharetdd%"};
    ret = rdbStore->ExecuteSql("DELETE FROM " + PhotoAlbumColumns::TABLE + " WHERE " +
        PhotoAlbumColumns::ALBUM_CLOUD_ID + " = ? OR LOWER(" + PhotoAlbumColumns::ALBUM_LPATH +
        ") IN (?, ?, ?) OR LOWER(" + PhotoAlbumColumns::ALBUM_LPATH + ") LIKE ?;", albumArgs);
    MEDIA_INFO_LOG("CleanShareTddData delete albums ret: %{public}d", ret);
}

bool ShareAlbumCloudSyncTestUtils::WaitForShareAssetsRemoved(int32_t albumId)
{
    for (int32_t i = 0; i < ASYNC_WAIT_MAX_TIMES; i++) {
        if (CountShareAssetsOfAlbum(albumId) <= 0) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(ASYNC_WAIT_INTERVAL_MS));
    }
    MEDIA_ERR_LOG("WaitForShareAssetsRemoved timeout, albumId: %{public}d, left: %{public}d",
        albumId, CountShareAssetsOfAlbum(albumId));
    return false;
}

int32_t ShareAlbumCloudSyncTestUtils::DownLinkShareAlbums(std::vector<int32_t> &stats)
{
    return DownLinkShareAlbumRecords(SHARE_ALBUM_JSON, stats);
}

int32_t ShareAlbumCloudSyncTestUtils::DownLinkShareAlbumRecords(
    const std::string &jsonPath, std::vector<int32_t> &stats)
{
    JsonFileReader jsonReader(jsonPath);
    std::vector<MDKRecord> records;
    jsonReader.ConvertToMDKRecordVector(records);
    CHECK_AND_RETURN_RET_LOG(!records.empty(), E_ERR, "DownLinkShareAlbumRecords read json failed: %{public}s",
        jsonPath.c_str());
    std::vector<CloudMetaData> newData;
    std::vector<CloudMetaData> fdirtyData;
    std::vector<std::string> failedRecords;
    int32_t ret = MakeShareAlbumHandler()->OnFetchRecords(records, newData, fdirtyData, failedRecords, stats);
    MEDIA_INFO_LOG("DownLinkShareAlbumRecords json: %{public}s, ret: %{public}d, recordSize: %{public}zu,"
        " newData: %{public}zu, failed: %{public}zu, newCount: %{public}d, metaCount: %{public}d,"
        " deleteCount:%{public}d",
        jsonPath.c_str(), ret, records.size(), newData.size(), failedRecords.size(),
        stats[StatsIndex::NEW_RECORDS_COUNT], stats[StatsIndex::META_MODIFY_RECORDS_COUNT],
        stats[StatsIndex::DELETE_RECORDS_COUNT]);
    return ret;
}

int32_t ShareAlbumCloudSyncTestUtils::InsertSharePhotosByDentry()
{
    return InsertSharePhotosByDentryFile(SHARE_PHOTO_JSON);
}

int32_t ShareAlbumCloudSyncTestUtils::InsertSharePhotosByDentryFile(const std::string &jsonPath)
{
    JsonFileReader jsonReader(jsonPath);
    std::vector<MDKRecord> records;
    jsonReader.ConvertToMDKRecordVector(records);
    CHECK_AND_RETURN_RET_LOG(!records.empty(), E_ERR, "InsertSharePhotosByDentryFile read json failed: %{public}s",
        jsonPath.c_str());
    std::vector<std::string> failedRecords;
    int32_t ret = MakeSharePhotoHandler()->OnDentryFileInsert(records, failedRecords);
    MEDIA_INFO_LOG("InsertSharePhotosByDentryFile json: %{public}s, ret: %{public}d, recordSize: %{public}zu,"
        " failed: %{public}zu", jsonPath.c_str(), ret, records.size(), failedRecords.size());
    return ret;
}

int32_t ShareAlbumCloudSyncTestUtils::DownLinkSharePhotoMeta(
    std::vector<CloudMetaData> &newData, std::vector<int32_t> &stats)
{
    JsonFileReader jsonReader(SHARE_PHOTO_META_JSON);
    std::vector<MDKRecord> records;
    jsonReader.ConvertToMDKRecordVector(records);
    CHECK_AND_RETURN_RET_LOG(!records.empty(), E_ERR, "DownLinkSharePhotoMeta read json failed");
    std::vector<CloudMetaData> fdirtyData;
    std::vector<std::string> failedRecords;
    int32_t ret = MakeSharePhotoHandler()->OnFetchRecords(records, newData, fdirtyData, failedRecords, stats);
    MEDIA_INFO_LOG("DownLinkSharePhotoMeta ret: %{public}d, recordSize: %{public}zu, newData: %{public}zu,"
        " failed: %{public}zu, stats[0]: %{public}d",
        ret, records.size(), newData.size(), failedRecords.size(), stats[StatsIndex::NEW_RECORDS_COUNT]);
    return ret;
}

int32_t ShareAlbumCloudSyncTestUtils::DownLinkDeletedSharePhoto(std::vector<int32_t> &stats)
{
    JsonFileReader jsonReader(SHARE_PHOTO_DELETE_JSON);
    std::vector<MDKRecord> records;
    jsonReader.ConvertToMDKRecordVector(records);
    CHECK_AND_RETURN_RET_LOG(!records.empty(), E_ERR, "DownLinkDeletedSharePhoto read json failed");
    std::vector<CloudMetaData> newData;
    std::vector<CloudMetaData> fdirtyData;
    std::vector<std::string> failedRecords;
    int32_t ret = MakeSharePhotoHandler()->OnFetchRecords(records, newData, fdirtyData, failedRecords, stats);
    MEDIA_INFO_LOG("DownLinkDeletedSharePhoto ret: %{public}d, failed: %{public}zu, stats[4]: %{public}d",
        ret, failedRecords.size(), stats[StatsIndex::DELETE_RECORDS_COUNT]);
    return ret;
}
}  // namespace OHOS::Media::CloudSync
