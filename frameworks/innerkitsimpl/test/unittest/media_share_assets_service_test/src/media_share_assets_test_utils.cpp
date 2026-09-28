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

#define MLOG_TAG "MediaShareAssetsServiceTest"

#include "media_share_assets_test_utils.h"

#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>

#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_upgrade.h"
#include "medialibrary_errno.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "photo_album_column.h"
#include "rdb_predicates.h"
#include "share_member_column.h"
#include "userfile_manager_types.h"

using namespace std;
using namespace OHOS::NativeRdb;

namespace OHOS::Media {

const std::string MediaShareAssetsTestUtils::DELETED_DISPLAY_NAME = "cloud_media_asset_deleted";

namespace {
// 与产品一致的照片云侧路径前缀(见 medialibrary_photo_operations.cpp 中的 "/storage/cloud/files/Photo/")
const std::string TEST_PHOTO_DIR = "/storage/cloud/files/Photo/100";
const std::string TEST_SHARE_RETAIN_PREF_DIR = "/data/storage/el2/base/preferences";
constexpr int32_t SHARE_ALBUM_TYPE = static_cast<int32_t>(PhotoAlbumType::SHARE);
constexpr int32_t USER_ALBUM_TYPE = static_cast<int32_t>(PhotoAlbumType::USER);
constexpr int32_t SHARE_ALBUM_SUBTYPE = static_cast<int32_t>(PhotoAlbumSubType::SHARE_GENERIC);
constexpr int32_t USER_ALBUM_SUBTYPE = static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC);
constexpr int32_t SHARED_ASSET_FLAG = 1;
constexpr int32_t NOT_SHARED_ASSET_FLAG = 0;
constexpr int64_t TEST_PHOTO_SIZE = 1024;
// 用例内固定时间戳(2023-11-15 06:13:20 UTC), 避免用例结果依赖当前时间
constexpr int64_t TEST_PHOTO_DATE_MS = 1700000000000;
} // namespace

std::shared_ptr<MediaLibraryRdbStore> &MediaShareAssetsTestUtils::GetRdbStore()
{
    static std::shared_ptr<MediaLibraryRdbStore> rdbStore = nullptr;
    return rdbStore;
}

void MediaShareAssetsTestUtils::InitEnvironment()
{
    auto &rdbStore = GetRdbStore();
    if (rdbStore == nullptr) {
        MediaLibraryUnitTestUtils::InitUnistore();
        rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    }
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("InitEnvironment failed, rdbStore is null");
        return;
    }
    std::vector<std::string> createTableSqlList = {
        PhotoUpgrade::CREATE_PHOTO_TABLE,
        PhotoAlbumColumns::CREATE_TABLE,
        SQL_CREATE_TAB_SHARE_ALBUM_MEMBER,
    };
    MediaLibraryUnitTestUtils::CreateTestTables(rdbStore, createTableSqlList);
    // 照片物理文件目录: 保证 InsertPhoto 写入的 file_path 真实可用, 从而真正走到删文件分支
    if (access(TEST_PHOTO_DIR.c_str(), F_OK) != 0) {
        MediaFileUtils::CreateDirectory(TEST_PHOTO_DIR + "/");
    }
}

void MediaShareAssetsTestUtils::ReleaseEnvironment()
{
    auto &rdbStore = GetRdbStore();
    if (rdbStore == nullptr) {
        return;
    }
    std::vector<std::string> testTables = {
        PhotoColumn::PHOTOS_TABLE,
        PhotoAlbumColumns::TABLE,
        ShareMemberColumn::TABLE_NAME,
    };
    MediaLibraryUnitTestUtils::CleanTestTables(rdbStore, testTables, true);
    MediaLibraryUnitTestUtils::StopUnistore();
    rdbStore = nullptr;
}

void MediaShareAssetsTestUtils::CleanTables()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        return;
    }
    std::vector<std::string> testTables = {
        PhotoColumn::PHOTOS_TABLE,
        PhotoAlbumColumns::TABLE,
        ShareMemberColumn::TABLE_NAME,
    };
    for (const auto &table : testTables) {
        std::string deleteSql = "DELETE FROM " + table + ";";
        rdbStore->ExecuteSql(deleteSql);
    }
}

void MediaShareAssetsTestUtils::PrepareShareRetainPreferences()
{
    if (access(TEST_SHARE_RETAIN_PREF_DIR.c_str(), F_OK) == 0) {
        return;
    }
    MediaFileUtils::CreateDirectory(TEST_SHARE_RETAIN_PREF_DIR + "/");
}

std::string MediaShareAssetsTestUtils::BuildPhotoPath(const std::string &displayName)
{
    return TEST_PHOTO_DIR + "/" + displayName;
}

bool MediaShareAssetsTestUtils::IsPhotoFileExists(const std::string &displayName)
{
    return MediaFileUtils::IsFileExists(BuildPhotoPath(displayName));
}

int32_t MediaShareAssetsTestUtils::InsertPhoto(int32_t fileId, const std::string &displayName, int32_t ownerAlbumId,
    int32_t isShared)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("InsertPhoto failed, rdbStore is null");
        return E_ERR;
    }
    std::string filePath = BuildPhotoPath(displayName);
    // 造出真实文件, 使 DeletePhoto 真正走删除物理文件的分支
    if (!MediaFileUtils::IsFileExists(filePath) && !MediaLibraryUnitTestUtils::CreateFileFS(filePath)) {
        MEDIA_ERR_LOG("InsertPhoto failed to create photo file: %{public}s", filePath.c_str());
        return E_ERR;
    }
    NativeRdb::ValuesBucket values;
    values.PutInt(MediaColumn::MEDIA_ID, fileId);
    values.PutString(MediaColumn::MEDIA_NAME, displayName);
    values.PutString(MediaColumn::MEDIA_FILE_PATH, filePath);
    values.PutString(MediaColumn::MEDIA_TITLE, displayName);
    values.PutInt(MediaColumn::MEDIA_TYPE, static_cast<int32_t>(MEDIA_TYPE_IMAGE));
    values.PutString(MediaColumn::MEDIA_MIME_TYPE, "image/jpeg");
    values.PutLong(MediaColumn::MEDIA_SIZE, TEST_PHOTO_SIZE);
    values.PutLong(MediaColumn::MEDIA_DATE_ADDED, TEST_PHOTO_DATE_MS);
    values.PutLong(MediaColumn::MEDIA_DATE_MODIFIED, TEST_PHOTO_DATE_MS);
    values.PutLong(MediaColumn::MEDIA_DATE_TAKEN, TEST_PHOTO_DATE_MS);
    values.PutInt(PhotoColumn::PHOTO_OWNER_ALBUM_ID, ownerAlbumId);
    values.PutInt(PhotoColumn::PHOTO_IS_SHARED, isShared);
    values.PutInt(PhotoColumn::PHOTO_SUBTYPE, 0);
    values.PutInt(PhotoColumn::PHOTO_ORIGINAL_SUBTYPE, 0);
    values.PutString(PhotoColumn::PHOTO_CLOUD_ID, "cloud_id_" + std::to_string(fileId));
    values.PutString(MediaColumn::MEDIA_VIRTUAL_PATH, "/Photo/" + std::to_string(fileId));
    int64_t rowId = -1;
    int32_t ret = rdbStore->Insert(rowId, PhotoColumn::PHOTOS_TABLE, values);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("InsertPhoto failed, ret: %{public}d", ret);
    }
    return ret;
}

int32_t MediaShareAssetsTestUtils::InsertShareAlbum(int32_t albumId, const std::string &albumName)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        return E_ERR;
    }
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoAlbumColumns::ALBUM_ID, albumId);
    values.PutInt(PhotoAlbumColumns::ALBUM_TYPE, SHARE_ALBUM_TYPE);
    values.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, SHARE_ALBUM_SUBTYPE);
    values.PutString(PhotoAlbumColumns::ALBUM_NAME, albumName);
    values.PutString(PhotoAlbumColumns::ALBUM_LPATH, "/Photo/" + std::to_string(albumId));
    values.PutString(PhotoAlbumColumns::ALBUM_CLOUD_ID, "album_cloud_id_" + std::to_string(albumId));
    int64_t rowId = -1;
    int32_t ret = rdbStore->Insert(rowId, PhotoAlbumColumns::TABLE, values);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("InsertShareAlbum failed, ret: %{public}d", ret);
    }
    return ret;
}

int32_t MediaShareAssetsTestUtils::InsertUserAlbum(int32_t albumId, const std::string &albumName)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        return E_ERR;
    }
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoAlbumColumns::ALBUM_ID, albumId);
    values.PutInt(PhotoAlbumColumns::ALBUM_TYPE, USER_ALBUM_TYPE);
    values.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, USER_ALBUM_SUBTYPE);
    values.PutString(PhotoAlbumColumns::ALBUM_NAME, albumName);
    values.PutString(PhotoAlbumColumns::ALBUM_LPATH, "/Photo/" + std::to_string(albumId));
    int64_t rowId = -1;
    int32_t ret = rdbStore->Insert(rowId, PhotoAlbumColumns::TABLE, values);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("InsertUserAlbum failed, ret: %{public}d", ret);
    }
    return ret;
}

int32_t MediaShareAssetsTestUtils::InsertShareMember(int32_t albumId, const std::string &member)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        return E_ERR;
    }
    NativeRdb::ValuesBucket values;
    values.PutInt(ShareMemberColumn::COLUMN_ALBUM_ID, albumId);
    values.PutString(ShareMemberColumn::COLUMN_SHARE_MEMBER, member);
    values.PutInt(ShareMemberColumn::COLUMN_SHARE_MEMBER_STATUS, 1);
    int64_t rowId = -1;
    int32_t ret = rdbStore->Insert(rowId, ShareMemberColumn::TABLE_NAME, values);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("InsertShareMember failed, ret: %{public}d", ret);
    }
    return ret;
}

int32_t MediaShareAssetsTestUtils::CountRows(const std::string &table)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        return -1;
    }
    // 各表主键列名不一致(Photos 为 file_id, PhotoAlbum / share member 不是), 统一用 COUNT(*) 统计
    std::string countSql = "SELECT COUNT(*) FROM " + table + ";";
    auto resultSet = rdbStore->QuerySql(countSql);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("CountRows query failed, table: %{public}s", table.c_str());
        return -1;
    }
    int64_t count = 0;
    resultSet->GetLong(0, count);
    resultSet->Close();
    return static_cast<int32_t>(count);
}

int32_t MediaShareAssetsTestUtils::CountDeletedMarkedAssets()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        return -1;
    }
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_NAME, DELETED_DISPLAY_NAME);
    auto resultSet = rdbStore->Query(predicates, { MediaColumn::MEDIA_ID });
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("CountDeletedMarkedAssets query failed");
        return -1;
    }
    int32_t count = 0;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        count++;
    }
    resultSet->Close();
    return count;
}

std::string MediaShareAssetsTestUtils::QueryPhotoString(const std::string &fileId, const std::string &column)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        return "";
    }
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    auto resultSet = rdbStore->Query(predicates, { column });
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("QueryPhotoString failed, fileId: %{public}s", fileId.c_str());
        return "";
    }
    std::string value;
    resultSet->GetString(0, value);
    resultSet->Close();
    return value;
}

int32_t MediaShareAssetsTestUtils::QueryPhotoInt(const std::string &fileId, const std::string &column)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        return -1;
    }
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    auto resultSet = rdbStore->Query(predicates, { column });
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("QueryPhotoInt failed, fileId: %{public}s", fileId.c_str());
        return -1;
    }
    int32_t value = -1;
    resultSet->GetInt(0, value);
    resultSet->Close();
    return value;
}

bool MediaShareAssetsTestUtils::IsPhotoColumnNull(const std::string &fileId, const std::string &column)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        return false;
    }
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    auto resultSet = rdbStore->Query(predicates, { column });
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("IsPhotoColumnNull failed, fileId: %{public}s", fileId.c_str());
        return false;
    }
    bool isNull = false;
    resultSet->IsColumnNull(0, isNull);
    resultSet->Close();
    return isNull;
}

void MediaShareAssetsServiceTest::SetUpTestCase(void)
{
    MediaShareAssetsTestUtils::InitEnvironment();
    MediaShareAssetsTestUtils::PrepareShareRetainPreferences();
    ASSERT_NE(MediaShareAssetsTestUtils::GetRdbStore(), nullptr);
}

void MediaShareAssetsServiceTest::TearDownTestCase(void)
{
    // 清理用例产生的真实照片文件, 避免污染设备
    std::string clearCmd = "rm -rf " + TEST_PHOTO_DIR;
    system(clearCmd.c_str());
    MediaShareAssetsTestUtils::ReleaseEnvironment();
}

void MediaShareAssetsServiceTest::SetUp(void)
{
    MediaShareAssetsTestUtils::InitEnvironment();
    MediaShareAssetsTestUtils::CleanTables();
}

void MediaShareAssetsServiceTest::TearDown(void)
{
    MediaShareAssetsTestUtils::CleanTables();
}

} // namespace OHOS::Media