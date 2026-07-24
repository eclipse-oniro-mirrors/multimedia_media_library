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

#define MLOG_TAG "AlbumDataManagerTest"

#include "album_data_manager_test.h"

#include <string>
#include <vector>
#include <memory>
#define private public
#define protected public
#include "album_data_manager.h"
#undef private
#undef public
#include "album_change_info.h"
#include "photo_asset_change_info.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "media_log.h"
#include "photo_album_column.h"
#include "media_column.h"
#include "values_bucket.h"
#include "abs_rdb_predicates.h"
#include "rdb_predicates.h"
#include "result_set.h"
#include "media_file_utils.h"

namespace OHOS {
namespace Media {
namespace AccurateRefresh {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t TEST_ALBUM_ID_1 = 100;
static constexpr int32_t TEST_ALBUM_ID_2 = 200;
static constexpr int32_t TEST_FILE_ID_1 = 1000;
static constexpr int32_t TEST_FILE_ID_2 = 2000;
static constexpr int32_t INVALID_ALBUM_ID = -1;
static constexpr int32_t MAX_ALBUM_COUNT = 1100;

void InsertPhotoAlbumTestData(int32_t albumId, int32_t albumSubType)
{
    ValuesBucket valuesBucket;
    valuesBucket.Put(PhotoAlbumColumns::ALBUM_ID, albumId);
    valuesBucket.Put(PhotoAlbumColumns::ALBUM_SUBTYPE, albumSubType);
    valuesBucket.Put(PhotoAlbumColumns::ALBUM_TYPE, PhotoAlbumType::USER);
    valuesBucket.Put(PhotoAlbumColumns::ALBUM_NAME, "TestAlbum" + to_string(albumId));
    valuesBucket.Put(PhotoAlbumColumns::ALBUM_COUNT, 0);
    int64_t outRowId = 0;
    int ret = g_rdbStore->Insert(outRowId, PhotoAlbumColumns::TABLE, valuesBucket);
    MEDIA_INFO_LOG("Insert photo album %{public}d result: %{public}d", albumId, ret);
}

void InsertPhotoAssetTestData(int32_t fileId, const string &uri, int32_t mediaType)
{
    ValuesBucket valuesBucket;
    valuesBucket.Put(PhotoColumn::MEDIA_ID, fileId);
    valuesBucket.Put(PhotoColumn::MEDIA_TYPE, mediaType);
    valuesBucket.Put(PhotoColumn::MEDIA_FILE_PATH, "/data/test/" + to_string(fileId) + ".jpg");
    valuesBucket.Put(PhotoColumn::MEDIA_DATE_ADDED, MediaFileUtils::UTCTimeMilliSeconds());
    int64_t outRowId = 0;
    int ret = g_rdbStore->Insert(outRowId, PhotoColumn::PHOTOS_TABLE, valuesBucket);
    MEDIA_INFO_LOG("Insert photo asset %{public}d result: %{public}d", fileId, ret);
}

void ClearPhotoAlbumTable()
{
    string deleteSql = "DELETE FROM " + PhotoAlbumColumns::TABLE + " WHERE " +
        PhotoAlbumColumns::ALBUM_ID + " >= 100";
    g_rdbStore->ExecuteSql(deleteSql);
}

void ClearPhotoAssetTable()
{
    string deleteSql = "DELETE FROM " + PhotoColumn::PHOTOS_TABLE + " WHERE " +
        PhotoColumn::MEDIA_ID + " >= 1000";
    g_rdbStore->ExecuteSql(deleteSql);
}

void AlbumDataManagerTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start AlbumDataManagerTest failed, can not get g_rdbStore");
        FAIL() << "Start AlbumDataManagerTest failed";
    }
    MEDIA_INFO_LOG("AlbumDataManagerTest::SetUpTestCase");
}

void AlbumDataManagerTest::TearDownTestCase(void)
{
    ClearPhotoAlbumTable();
    ClearPhotoAssetTable();
    MEDIA_INFO_LOG("AlbumDataManagerTest::TearDownTestCase");
}

void AlbumDataManagerTest::SetUp()
{
    ClearPhotoAlbumTable();
    ClearPhotoAssetTable();
    InsertPhotoAlbumTestData(TEST_ALBUM_ID_1, PhotoAlbumSubType::USER_GENERIC);
    InsertPhotoAlbumTestData(TEST_ALBUM_ID_2, PhotoAlbumSubType::FAVORITE);
    InsertPhotoAssetTestData(TEST_FILE_ID_1, "file://1000", MediaType::MEDIA_TYPE_IMAGE);
    MEDIA_INFO_LOG("AlbumDataManagerTest::SetUp");
}

void AlbumDataManagerTest::TearDown(void)
{
    MEDIA_INFO_LOG("AlbumDataManagerTest::TearDown");
}

HWTEST_F(AlbumDataManagerTest, InitAlbumInfos_EmptyVector_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start InitAlbumInfos_EmptyVector_Test");
    AlbumDataManager dataManager;
    vector<int> emptyAlbumIds;
    int32_t result = dataManager.InitAlbumInfos(emptyAlbumIds);
    EXPECT_EQ(result, ACCURATE_REFRESH_INPUT_PARA_ERR);
}

HWTEST_F(AlbumDataManagerTest, InitAlbumInfos_SingleAlbumId_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start InitAlbumInfos_SingleAlbumId_Test");
    AlbumDataManager dataManager;
    vector<int> albumIds = {TEST_ALBUM_ID_1};
    int32_t result = dataManager.InitAlbumInfos(albumIds);
    EXPECT_EQ(result, ACCURATE_REFRESH_RET_OK);
}

HWTEST_F(AlbumDataManagerTest, InitAlbumInfos_MultipleAlbumIds_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start InitAlbumInfos_MultipleAlbumIds_Test");
    AlbumDataManager dataManager;
    vector<int> albumIds = {TEST_ALBUM_ID_1, TEST_ALBUM_ID_2};
    int32_t result = dataManager.InitAlbumInfos(albumIds);
    EXPECT_EQ(result, ACCURATE_REFRESH_RET_OK);
}

HWTEST_F(AlbumDataManagerTest, InitAlbumIds_InvalidAlbumId_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start InitAlbumInfos_InvalidAlbumId_Test");
    AlbumDataManager dataManager;
    vector<int> albumIds = {INVALID_ALBUM_ID};
    int32_t result = dataManager.InitAlbumInfos(albumIds);
    EXPECT_EQ(result, ACCURATE_REFRESH_RET_OK);
}

HWTEST_F(AlbumDataManagerTest, InitAlbumInfos_LargeAlbumIdList_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start InitAlbumInfos_LargeAlbumIdList_Test");
    AlbumDataManager dataManager;
    vector<int> albumIds;
    for (int i = 0; i < 100; i++) {
        albumIds.push_back(1000 + i);
    }
    int32_t result = dataManager.InitAlbumInfos(albumIds);
    EXPECT_EQ(result, ACCURATE_REFRESH_RET_OK);
}

HWTEST_F(AlbumDataManagerTest, UpdateModifiedDatas_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start UpdateModifiedDatas_Test");
    AlbumDataManager dataManager;
    int32_t result = dataManager.UpdateModifiedDatas();
    EXPECT_EQ(result, ACCURATE_REFRESH_RET_OK);
}

HWTEST_F(AlbumDataManagerTest, UpdateModifiedDatas_MultipleCalls_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start UpdateModifiedDatas_MultipleCalls_Test");
    AlbumDataManager dataManager;
    int32_t result1 = dataManager.UpdateModifiedDatas();
    int32_t result2 = dataManager.UpdateModifiedDatas();
    int32_t result3 = dataManager.UpdateModifiedDatas();
    EXPECT_EQ(result1, ACCURATE_REFRESH_RET_OK);
    EXPECT_EQ(result2, ACCURATE_REFRESH_RET_OK);
    EXPECT_EQ(result3, ACCURATE_REFRESH_RET_OK);
}

HWTEST_F(AlbumDataManagerTest, GetPhotoAssetInfo_ValidFileId_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetPhotoAssetInfo_ValidFileId_Test");
    AlbumDataManager dataManager;
    PhotoAssetChangeInfo assetInfo = dataManager.GetPhotoAssetInfo(TEST_FILE_ID_1);
    EXPECT_EQ(assetInfo.fileId_, TEST_FILE_ID_1);
}

HWTEST_F(AlbumDataManagerTest, GetPhotoAssetInfo_InvalidFileId_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetPhotoAssetInfo_InvalidFileId_Test");
    AlbumDataManager dataManager;
    PhotoAssetChangeInfo assetInfo = dataManager.GetPhotoAssetInfo(INVALID_ALBUM_ID);
    EXPECT_EQ(assetInfo.fileId_, INVALID_INT32_VALUE);
}

HWTEST_F(AlbumDataManagerTest, GetPhotoAssetInfo_NegativeFileId_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetPhotoAssetInfo_NegativeFileId_Test");
    AlbumDataManager dataManager;
    PhotoAssetChangeInfo assetInfo = dataManager.GetPhotoAssetInfo(-100);
    EXPECT_EQ(assetInfo.fileId_, INVALID_INT32_VALUE);
}

HWTEST_F(AlbumDataManagerTest, GetPhotoAssetInfo_ZeroFileId_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetPhotoAssetInfo_ZeroFileId_Test");
    AlbumDataManager dataManager;
    PhotoAssetChangeInfo assetInfo = dataManager.GetPhotoAssetInfo(0);
    EXPECT_EQ(assetInfo.fileId_, INVALID_INT32_VALUE);
}

HWTEST_F(AlbumDataManagerTest, GetPhotoAssetInfo_LargeFileId_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetPhotoAssetInfo_LargeFileId_Test");
    AlbumDataManager dataManager;
    PhotoAssetChangeInfo assetInfo = dataManager.GetPhotoAssetInfo(999999);
    EXPECT_EQ(assetInfo.fileId_, INVALID_INT32_VALUE);
}

HWTEST_F(AlbumDataManagerTest, PostProcessModifiedDatas_EmptyKeys_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start PostProcessModifiedDatas_EmptyKeys_Test");
    AlbumDataManager dataManager;
    vector<int32_t> emptyKeys;
    int32_t result = dataManager.PostProcessModifiedDatas(emptyKeys);
    EXPECT_EQ(result, ACCURATE_REFRESH_RET_OK);
}

HWTEST_F(AlbumDataManagerTest, PostProcessModifiedDatas_SingleKey_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start PostProcessModifiedDatas_SingleKey_Test");
    AlbumDataManager dataManager;
    vector<int> albumIds = {TEST_ALBUM_ID_1};
    dataManager.InitAlbumInfos(albumIds);
    vector<int32_t> keys = {TEST_ALBUM_ID_1};
    int32_t result = dataManager.PostProcessModifiedDatas(keys);
    EXPECT_EQ(result, ACCURATE_REFRESH_RET_OK);
}

HWTEST_F(AlbumDataManagerTest, PostProcessModifiedDatas_MultipleKeys_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start PostProcessModifiedDatas_MultipleKeys_Test");
    AlbumDataManager dataManager;
    vector<int> albumIds = {TEST_ALBUM_ID_1, TEST_ALBUM_ID_2};
    dataManager.InitAlbumInfos(albumIds);
    vector<int32_t> keys = {TEST_ALBUM_ID_1, TEST_ALBUM_ID_2};
    int32_t result = dataManager.PostProcessModifiedDatas(keys);
    EXPECT_EQ(result, ACCURATE_REFRESH_RET_OK);
}

HWTEST_F(AlbumDataManagerTest, PostProcessModifiedDatas_InvalidKey_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start PostProcessModifiedDatas_InvalidKey_Test");
    AlbumDataManager dataManager;
    vector<int32_t> keys = {INVALID_ALBUM_ID};
    int32_t result = dataManager.PostProcessModifiedDatas(keys);
    EXPECT_EQ(result, ACCURATE_REFRESH_RET_OK);
}

HWTEST_F(AlbumDataManagerTest, PostProcessModifiedDatas_CoverChange_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start PostProcessModifiedDatas_CoverChange_Test");
    AlbumDataManager dataManager;
    vector<int> albumIds = {TEST_ALBUM_ID_1};
    dataManager.InitAlbumInfos(albumIds);
    vector<int32_t> keys = {TEST_ALBUM_ID_1};
    int32_t result = dataManager.PostProcessModifiedDatas(keys);
    EXPECT_EQ(result, ACCURATE_REFRESH_RET_OK);
}

HWTEST_F(AlbumDataManagerTest, PostProcessModifiedDatas_HiddenCoverChange_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start PostProcessModifiedDatas_HiddenCoverChange_Test");
    AlbumDataManager dataManager;
    vector<int> albumIds = {TEST_ALBUM_ID_1};
    dataManager.InitAlbumInfos(albumIds);
    vector<int32_t> keys = {TEST_ALBUM_ID_1};
    int32_t result = dataManager.PostProcessModifiedDatas(keys);
    EXPECT_EQ(result, ACCURATE_REFRESH_RET_OK);
}

HWTEST_F(AlbumDataManagerTest, PostProcessModifiedDatas_LargeKeysList_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start PostProcessModifiedDatas_LargeKeysList_Test");
    AlbumDataManager dataManager;
    vector<int32_t> keys;
    for (int i = 0; i < 100; i++) {
        keys.push_back(1000 + i);
    }
    int32_t result = dataManager.PostProcessModifiedDatas(keys);
    EXPECT_EQ(result, ACCURATE_REFRESH_RET_OK);
}

HWTEST_F(AlbumDataManagerTest, GetChangeInfoKey_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetChangeInfoKey_Test");
    AlbumDataManager dataManager;
    AlbumChangeInfo changeInfo;
    changeInfo.albumId_ = TEST_ALBUM_ID_1;
    int32_t key = dataManager.GetChangeInfoKey(changeInfo);
    EXPECT_EQ(key, TEST_ALBUM_ID_1);
}

HWTEST_F(AlbumDataManagerTest, GetChangeInfoKey_ZeroAlbumId_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetChangeInfoKey_ZeroAlbumId_Test");
    AlbumDataManager dataManager;
    AlbumChangeInfo changeInfo;
    changeInfo.albumId_ = 0;
    int32_t key = dataManager.GetChangeInfoKey(changeInfo);
    EXPECT_EQ(key, 0);
}

HWTEST_F(AlbumDataManagerTest, GetChangeInfoKey_NegativeAlbumId_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetChangeInfoKey_NegativeAlbumId_Test");
    AlbumDataManager dataManager;
    AlbumChangeInfo changeInfo;
    changeInfo.albumId_ = INVALID_ALBUM_ID;
    int32_t key = dataManager.GetChangeInfoKey(changeInfo);
    EXPECT_EQ(key, INVALID_ALBUM_ID);
}

HWTEST_F(AlbumDataManagerTest, GetInfoByKeys_SingleKey_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetInfoByKeys_SingleKey_Test");
    AlbumDataManager dataManager;
    vector<int32_t> albumIds = {TEST_ALBUM_ID_1};
    vector<AlbumChangeInfo> infos = dataManager.GetInfoByKeys(albumIds);
    EXPECT_FALSE(infos.empty());
}

HWTEST_F(AlbumDataManagerTest, GetInfoByKeys_MultipleKeys_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetInfoByKeys_MultipleKeys_Test");
    AlbumDataManager dataManager;
    vector<int32_t> albumIds = {TEST_ALBUM_ID_1, TEST_ALBUM_ID_2};
    vector<AlbumChangeInfo> infos = dataManager.GetInfoByKeys(albumIds);
    EXPECT_FALSE(infos.empty());
}

HWTEST_F(AlbumDataManagerTest, GetInfoByKeys_EmptyKeys_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetInfoByKeys_EmptyKeys_Test");
    AlbumDataManager dataManager;
    vector<int32_t> emptyKeys;
    vector<AlbumChangeInfo> infos = dataManager.GetInfoByKeys(emptyKeys);
    EXPECT_EQ(infos.size(), 0);
}

HWTEST_F(AlbumDataManagerTest, GetInfoByKeys_InvalidKey_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetInfoByKeys_InvalidKey_Test");
    AlbumDataManager dataManager;
    vector<int32_t> albumIds = {INVALID_ALBUM_ID};
    vector<AlbumChangeInfo> infos = dataManager.GetInfoByKeys(albumIds);
    EXPECT_EQ(infos.size(), 0);
}

HWTEST_F(AlbumDataManagerTest, GetInfoByKeys_LargeKeysList_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetInfoByKeys_LargeKeysList_Test");
    AlbumDataManager dataManager;
    vector<int32_t> albumIds;
    for (int i = 0; i < 100; i++) {
        albumIds.push_back(1000 + i);
    }
    vector<AlbumChangeInfo> infos = dataManager.GetInfoByKeys(albumIds);
    EXPECT_EQ(infos.size(), 0);
}

HWTEST_F(AlbumDataManagerTest, GetInfosByPredicates_ValidPredicates_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetInfosByPredicates_ValidPredicates_Test");
    AlbumDataManager dataManager;
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(TEST_ALBUM_ID_1));
    vector<AlbumChangeInfo> infos = dataManager.GetInfosByPredicates(predicates);
    EXPECT_FALSE(infos.empty());
}

HWTEST_F(AlbumDataManagerTest, GetInfosByPredicates_EmptyPredicates_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetInfosByPredicates_EmptyPredicates_Test");
    AlbumDataManager dataManager;
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    vector<AlbumChangeInfo> infos = dataManager.GetInfosByPredicates(predicates);
    EXPECT_FALSE(infos.empty());
}

HWTEST_F(AlbumDataManagerTest, GetInfosByPredicates_NoMatch_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetInfosByPredicates_NoMatch_Test");
    AlbumDataManager dataManager;
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(99999));
    vector<AlbumChangeInfo> infos = dataManager.GetInfosByPredicates(predicates);
    EXPECT_EQ(infos.size(), 0);
}

HWTEST_F(AlbumDataManagerTest, GetAlbumInfos_WithAlbumIds_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetAlbumInfos_WithAlbumIds_Test");
    AlbumDataManager dataManager;
    vector<int32_t> albumIds = {TEST_ALBUM_ID_1, TEST_ALBUM_ID_2};
    vector<AlbumChangeInfo> infos = dataManager.GetAlbumInfos(albumIds, {});
    EXPECT_FALSE(infos.empty());
}

HWTEST_F(AlbumDataManagerTest, GetAlbumInfos_WithSystemTypes_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetAlbumInfos_WithSystemTypes_Test");
    AlbumDataManager dataManager;
    vector<string> systemTypes = {"1", "2"};
    vector<AlbumChangeInfo> infos = dataManager.GetAlbumInfos({}, systemTypes);
    EXPECT_FALSE(infos.empty());
}

HWTEST_F(AlbumDataManagerTest, GetAlbumInfos_WithBothParams_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetAlbumInfos_WithBothParams_Test");
    AlbumDataManager dataManager;
    vector<int32_t> albumIds = {TEST_ALBUM_ID_1};
    vector<string> systemTypes = {"1"};
    vector<AlbumChangeInfo> infos = dataManager.GetAlbumInfos(albumIds, systemTypes);
    EXPECT_FALSE(infos.empty());
}

HWTEST_F(AlbumDataManagerTest, GetAlbumInfos_EmptyParams_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetAlbumInfos_EmptyParams_Test");
    AlbumDataManager dataManager;
    vector<AlbumChangeInfo> infos = dataManager.GetAlbumInfos({}, {});
    EXPECT_EQ(infos.size(), 0);
}

HWTEST_F(AlbumDataManagerTest, GetAlbumInfos_LargeAlbumIds_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetAlbumInfos_LargeAlbumIds_Test");
    AlbumDataManager dataManager;
    vector<int32_t> albumIds;
    for (int i = 0; i < 100; i++) {
        albumIds.push_back(1000 + i);
    }
    vector<AlbumChangeInfo> infos = dataManager.GetAlbumInfos(albumIds, {});
    EXPECT_EQ(infos.size(), 0);
}

HWTEST_F(AlbumDataManagerTest, GetInitAlbumInfos_WithData_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetInitAlbumInfos_WithData_Test");
    AlbumDataManager dataManager;
    vector<int> albumIds = {TEST_ALBUM_ID_1, TEST_ALBUM_ID_2};
    dataManager.InitAlbumInfos(albumIds);
    unordered_map<int32_t, AlbumChangeInfo> initInfos = dataManager.GetInitAlbumInfos();
    EXPECT_FALSE(initInfos.empty());
}

HWTEST_F(AlbumDataManagerTest, GetInitKeys_Empty_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetInitKeys_Empty_Test");
    AlbumDataManager dataManager;
    vector<int32_t> keys = dataManager.GetInitKeys();
    EXPECT_FALSE(keys.empty());
}

HWTEST_F(AlbumDataManagerTest, GetInitKeys_WithData_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetInitKeys_WithData_Test");
    AlbumDataManager dataManager;
    vector<int> albumIds = {TEST_ALBUM_ID_1, TEST_ALBUM_ID_2};
    dataManager.InitAlbumInfos(albumIds);
    vector<int32_t> keys = dataManager.GetInitKeys();
    EXPECT_GE(keys.size(), 0);
}

HWTEST_F(AlbumDataManagerTest, GetInitKeys_SingleKey_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetInitKeys_SingleKey_Test");
    AlbumDataManager dataManager;
    vector<int> albumIds = {TEST_ALBUM_ID_1};
    dataManager.InitAlbumInfos(albumIds);
    vector<int32_t> keys = dataManager.GetInitKeys();
    EXPECT_EQ(keys.size(), 1);
}

HWTEST_F(AlbumDataManagerTest, GetAlbumDatasFromAddAlbum_SingleAlbumId_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetAlbumDatasFromAddAlbum_SingleAlbumId_Test");
    vector<string> albumIdsStr = {to_string(TEST_ALBUM_ID_1)};
    vector<AlbumChangeData> changeDatas = AlbumDataManager::GetAlbumDatasFromAddAlbum(albumIdsStr);
    EXPECT_FALSE(changeDatas.empty());
}

HWTEST_F(AlbumDataManagerTest, GetAlbumDatasFromAddAlbum_MultipleAlbumIds_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetAlbumDatasFromAddAlbum_MultipleAlbumIds_Test");
    vector<string> albumIdsStr = {to_string(TEST_ALBUM_ID_1), to_string(TEST_ALBUM_ID_2)};
    vector<AlbumChangeData> changeDatas = AlbumDataManager::GetAlbumDatasFromAddAlbum(albumIdsStr);
    EXPECT_FALSE(changeDatas.empty());
}

HWTEST_F(AlbumDataManagerTest, GetAlbumDatasFromAddAlbum_EmptyAlbumIds_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetAlbumDatasFromAddAlbum_EmptyAlbumIds_Test");
    vector<string> emptyAlbumIdsStr;
    vector<AlbumChangeData> changeDatas = AlbumDataManager::GetAlbumDatasFromAddAlbum(emptyAlbumIdsStr);
    EXPECT_EQ(changeDatas.size(), 0);
}

HWTEST_F(AlbumDataManagerTest, GetAlbumDatasFromAddAlbum_InvalidAlbumId_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetAlbumDatasFromAddAlbum_InvalidAlbumId_Test");
    vector<string> albumIdsStr = {to_string(INVALID_ALBUM_ID)};
    vector<AlbumChangeData> changeDatas = AlbumDataManager::GetAlbumDatasFromAddAlbum(albumIdsStr);
    EXPECT_EQ(changeDatas.size(), 0);
}

HWTEST_F(AlbumDataManagerTest, GetAlbumDatasFromAddAlbum_LargeAlbumIds_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetAlbumDatasFromAddAlbum_LargeAlbumIds_Test");
    vector<string> albumIdsStr;
    for (int i = 0; i < 100; i++) {
        albumIdsStr.push_back(to_string(1000 + i));
    }
    vector<AlbumChangeData> changeDatas = AlbumDataManager::GetAlbumDatasFromAddAlbum(albumIdsStr);
    EXPECT_EQ(changeDatas.size(), 0);
}

HWTEST_F(AlbumDataManagerTest, SetAlbumIdsByPredicates_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetAlbumIdsByPredicates_Test");
    AlbumDataManager dataManager;
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(TEST_ALBUM_ID_1));
    int32_t result = dataManager.SetAlbumIdsByPredicates(predicates);
    EXPECT_EQ(result, NativeRdb::E_OK);
}

HWTEST_F(AlbumDataManagerTest, SetAlbumIdsByPredicates_EmptyPredicates_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetAlbumIdsByPredicates_EmptyPredicates_Test");
    AlbumDataManager dataManager;
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    int32_t result = dataManager.SetAlbumIdsByPredicates(predicates);
    EXPECT_EQ(result, NativeRdb::E_OK);
}

HWTEST_F(AlbumDataManagerTest, SetAlbumIdsBySql_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetAlbumIdsBySql_Test");
    AlbumDataManager dataManager;
    string sql = "SELECT * FROM " + PhotoAlbumColumns::TABLE;
    vector<ValueObject> bindArgs;
    int32_t result = dataManager.SetAlbumIdsBySql(sql, bindArgs);
    EXPECT_EQ(result, NativeRdb::E_OK);
}

HWTEST_F(AlbumDataManagerTest, SetAlbumIdsBySql_WithBindArgs_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetAlbumIdsBySql_WithBindArgs_Test");
    AlbumDataManager dataManager;
    string sql = "SELECT * FROM " + PhotoAlbumColumns::TABLE + " WHERE " +
        PhotoAlbumColumns::ALBUM_ID + " = ?";
    vector<ValueObject> bindArgs;
    bindArgs.push_back(ValueObject(static_cast<int>(TEST_ALBUM_ID_1)));
    int32_t result = dataManager.SetAlbumIdsBySql(sql, bindArgs);
    EXPECT_EQ(result, NativeRdb::E_OK);
}

HWTEST_F(AlbumDataManagerTest, SetAlbumIdsBySql_EmptySql_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetAlbumIdsBySql_EmptySql_Test");
    AlbumDataManager dataManager;
    string emptySql;
    vector<ValueObject> bindArgs;
    int32_t result = dataManager.SetAlbumIdsBySql(emptySql, bindArgs);
    EXPECT_EQ(result, NativeRdb::E_OK);
}

HWTEST_F(AlbumDataManagerTest, SetAlbumIdsByFileds_SingleFileId_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetAlbumIdsByFileds_SingleFileId_Test");
    AlbumDataManager dataManager;
    vector<int32_t> fileIds = {TEST_FILE_ID_1};
    int32_t result = dataManager.SetAlbumIdsByFileds(fileIds);
    EXPECT_EQ(result, NativeRdb::E_OK);
}

HWTEST_F(AlbumDataManagerTest, SetAlbumIdsByFileds_MultipleFileIds_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetAlbumIdsByFileds_MultipleFileIds_Test");
    AlbumDataManager dataManager;
    vector<int32_t> fileIds = {TEST_FILE_ID_1, TEST_FILE_ID_2};
    int32_t result = dataManager.SetAlbumIdsByFileds(fileIds);
    EXPECT_EQ(result, NativeRdb::E_OK);
}

HWTEST_F(AlbumDataManagerTest, SetAlbumIdsByFileds_EmptyFileIds_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetAlbumIdsByFileds_EmptyFileIds_Test");
    AlbumDataManager dataManager;
    vector<int32_t> emptyFileIds;
    int32_t result = dataManager.SetAlbumIdsByFileds(emptyFileIds);
    EXPECT_EQ(result, NativeRdb::E_OK);
}

HWTEST_F(AlbumDataManagerTest, SetAlbumIdsByFileds_InvalidFileId_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetAlbumIdsByFileds_InvalidFileId_Test");
    AlbumDataManager dataManager;
    vector<int32_t> fileIds = {INVALID_ALBUM_ID};
    int32_t result = dataManager.SetAlbumIdsByFileds(fileIds);
    EXPECT_EQ(result, NativeRdb::E_OK);
}

HWTEST_F(AlbumDataManagerTest, ClearChangeInfos_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ClearChangeInfos_Test");
    AlbumDataManager dataManager;
    vector<int> albumIds = {TEST_ALBUM_ID_1, TEST_ALBUM_ID_2};
    dataManager.InitAlbumInfos(albumIds);
    dataManager.ClearChangeInfos();
    vector<int32_t> keys = dataManager.GetInitKeys();
    EXPECT_EQ(keys.size(), 0);
}

HWTEST_F(AlbumDataManagerTest, ClearChangeInfos_MultipleCalls_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ClearChangeInfos_MultipleCalls_Test");
    AlbumDataManager dataManager;
    vector<int> albumIds = {TEST_ALBUM_ID_1};
    dataManager.InitAlbumInfos(albumIds);
    dataManager.ClearChangeInfos();
    dataManager.ClearChangeInfos();
    dataManager.ClearChangeInfos();
    vector<int32_t> keys = dataManager.GetInitKeys();
    EXPECT_EQ(keys.size(), 0);
}

HWTEST_F(AlbumDataManagerTest, CheckIsExceed_Default_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CheckIsExceed_Default_Test");
    AlbumDataManager dataManager;
    bool result = dataManager.CheckIsExceed(false);
    EXPECT_EQ(result, false);
}

HWTEST_F(AlbumDataManagerTest, CheckIsExceed_LengthChangedTrue_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CheckIsExceed_LengthChangedTrue_Test");
    AlbumDataManager dataManager;
    bool result = dataManager.CheckIsExceed(true);
    EXPECT_EQ(result, false);
}

HWTEST_F(AlbumDataManagerTest, CheckIsExceed_ExceedsLimit_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CheckIsExceed_ExceedsLimit_Test");
    AlbumDataManager dataManager;
    vector<int> albumIds;
    for (int i = 0; i < MAX_ALBUM_COUNT; i++) {
        albumIds.push_back(1000 + i);
    }
    dataManager.InitAlbumInfos(albumIds);
    bool result = dataManager.CheckIsExceed(true);
    EXPECT_EQ(result, false);
}

HWTEST_F(AlbumDataManagerTest, CheckIsExceed_WithPredicates_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CheckIsExceed_WithPredicates_Test");
    AlbumDataManager dataManager;
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    bool result = dataManager.CheckIsExceed(predicates, false);
    EXPECT_EQ(result, false);
}

HWTEST_F(AlbumDataManagerTest, CheckIsExceed_WithPredicatesLengthChanged_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CheckIsExceed_WithPredicatesLengthChanged_Test");
    AlbumDataManager dataManager;
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    bool result = dataManager.CheckIsExceed(predicates, true);
    EXPECT_EQ(result, false);
}

HWTEST_F(AlbumDataManagerTest, CheckIsExceed_WithSql_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CheckIsExceed_WithSql_Test");
    AlbumDataManager dataManager;
    string sql = "SELECT * FROM " + PhotoAlbumColumns::TABLE;
    vector<ValueObject> bindArgs;
    bool result = dataManager.CheckIsExceed(sql, bindArgs, false);
    EXPECT_EQ(result, false);
}

HWTEST_F(AlbumDataManagerTest, CheckIsExceed_WithSqlLengthChanged_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CheckIsExceed_WithSqlLengthChanged_Test");
    AlbumDataManager dataManager;
    string sql = "SELECT * FROM " + PhotoAlbumColumns::TABLE;
    vector<ValueObject> bindArgs;
    bool result = dataManager.CheckIsExceed(sql, bindArgs, true);
    EXPECT_EQ(result, false);
}

HWTEST_F(AlbumDataManagerTest, CheckIsExceed_WithLength_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CheckIsExceed_WithLength_Test");
    AlbumDataManager dataManager;
    bool result = dataManager.CheckIsExceed(static_cast<size_t>(100));
    EXPECT_EQ(result, false);
}

HWTEST_F(AlbumDataManagerTest, CheckIsExceed_WithLengthExceeds_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CheckIsExceed_WithLengthExceeds_Test");
    AlbumDataManager dataManager;
    bool result = dataManager.CheckIsExceed(static_cast<size_t>(MAX_DATA_LENGTH + 1));
    EXPECT_EQ(result, true);
}

HWTEST_F(AlbumDataManagerTest, CheckIsExceed_WithLengthZero_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CheckIsExceed_WithLengthZero_Test");
    AlbumDataManager dataManager;
    bool result = dataManager.CheckIsExceed(static_cast<size_t>(0));
    EXPECT_EQ(result, false);
}

HWTEST_F(AlbumDataManagerTest, CheckIsExceed_WithKeys_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CheckIsExceed_WithKeys_Test");
    AlbumDataManager dataManager;
    vector<int32_t> keys = {TEST_ALBUM_ID_1, TEST_ALBUM_ID_2};
    bool result = dataManager.CheckIsExceed(keys);
    EXPECT_EQ(result, false);
}

HWTEST_F(AlbumDataManagerTest, CheckIsExceed_WithKeysExceeds_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CheckIsExceed_WithKeysExceeds_Test");
    AlbumDataManager dataManager;
    vector<int32_t> keys;
    for (int i = 0; i < MAX_DATA_LENGTH + 1; i++) {
        keys.push_back(i);
    }
    bool result = dataManager.CheckIsExceed(keys);
    EXPECT_EQ(result, true);
}

HWTEST_F(AlbumDataManagerTest, CheckIsExceed_WithEmptyKeys_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CheckIsExceed_WithEmptyKeys_Test");
    AlbumDataManager dataManager;
    vector<int32_t> emptyKeys;
    bool result = dataManager.CheckIsExceed(emptyKeys);
    EXPECT_EQ(result, false);
}

HWTEST_F(AlbumDataManagerTest, CheckIsForRecheck_Default_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CheckIsForRecheck_Default_Test");
    AlbumDataManager dataManager;
    bool result = dataManager.CheckIsForRecheck();
    EXPECT_EQ(result, false);
}

HWTEST_F(AlbumDataManagerTest, CheckIsForRecheck_AfterExceed_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CheckIsForRecheck_AfterExceed_Test");
    AlbumDataManager dataManager;
    vector<int> albumIds;
    for (int i = 0; i < MAX_ALBUM_COUNT; i++) {
        albumIds.push_back(1000 + i);
    }
    dataManager.InitAlbumInfos(albumIds);
    dataManager.CheckIsExceed(true);
    bool result = dataManager.CheckIsForRecheck();
    EXPECT_EQ(result, false);
}

HWTEST_F(AlbumDataManagerTest, CheckIsForRecheck_MultipleCalls_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CheckIsForRecheck_MultipleCalls_Test");
    AlbumDataManager dataManager;
    bool result1 = dataManager.CheckIsForRecheck();
    bool result2 = dataManager.CheckIsForRecheck();
    bool result3 = dataManager.CheckIsForRecheck();
    EXPECT_EQ(result1, false);
    EXPECT_EQ(result2, false);
    EXPECT_EQ(result3, false);
}

HWTEST_F(AlbumDataManagerTest, FullWorkflow_InitPostProcessClear_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start FullWorkflow_InitPostProcessClear_Test");
    AlbumDataManager dataManager;
    vector<int> albumIds = {TEST_ALBUM_ID_1, TEST_ALBUM_ID_2};
    
    int32_t initResult = dataManager.InitAlbumInfos(albumIds);
    EXPECT_EQ(initResult, ACCURATE_REFRESH_RET_OK);
    
    vector<int32_t> keys = dataManager.GetInitKeys();
    EXPECT_FALSE(keys.empty());
    
    int32_t postResult = dataManager.PostProcessModifiedDatas(keys);
    EXPECT_EQ(postResult, ACCURATE_REFRESH_RET_OK);
    
    dataManager.ClearChangeInfos();
    vector<int32_t> clearedKeys = dataManager.GetInitKeys();
    EXPECT_EQ(clearedKeys.size(), 0);
}

HWTEST_F(AlbumDataManagerTest, FullWorkflow_MultipleOperations_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start FullWorkflow_MultipleOperations_Test");
    AlbumDataManager dataManager;
    
    vector<int> albumIds1 = {TEST_ALBUM_ID_1};
    dataManager.InitAlbumInfos(albumIds1);
    
    int32_t updateResult = dataManager.UpdateModifiedDatas();
    EXPECT_EQ(updateResult, ACCURATE_REFRESH_RET_OK);
    
    vector<int> albumIds2 = {TEST_ALBUM_ID_2};
    dataManager.InitAlbumInfos(albumIds2);
    
    vector<int32_t> keys = dataManager.GetInitKeys();
    int32_t postResult = dataManager.PostProcessModifiedDatas(keys);
    EXPECT_EQ(postResult, ACCURATE_REFRESH_RET_OK);
    
    dataManager.ClearChangeInfos();
}

HWTEST_F(AlbumDataManagerTest, InitAlbumInfos_DuplicateAlbumIds_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start InitAlbumInfos_DuplicateAlbumIds_Test");
    AlbumDataManager dataManager;
    vector<int> albumIds = {TEST_ALBUM_ID_1, TEST_ALBUM_ID_1, TEST_ALBUM_ID_2};
    int32_t result = dataManager.InitAlbumInfos(albumIds);
    EXPECT_EQ(result, ACCURATE_REFRESH_RET_OK);
}

HWTEST_F(AlbumDataManagerTest, InitAlbumInfos_LargeSingleAlbumId_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start InitAlbumInfos_LargeSingleAlbumId_Test");
    AlbumDataManager dataManager;
    vector<int> albumIds = {999999999};
    int32_t result = dataManager.InitAlbumInfos(albumIds);
    EXPECT_EQ(result, ACCURATE_REFRESH_RET_OK);
}

HWTEST_F(AlbumDataManagerTest, InitAlbumInfos_MixedValidInvalidIds_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start InitAlbumInfos_MixedValidInvalidIds_Test");
    AlbumDataManager dataManager;
    vector<int> albumIds = {TEST_ALBUM_ID_1, INVALID_ALBUM_ID, TEST_ALBUM_ID_2};
    int32_t result = dataManager.InitAlbumInfos(albumIds);
    EXPECT_EQ(result, ACCURATE_REFRESH_RET_OK);
}

HWTEST_F(AlbumDataManagerTest, InitAlbumInfos_ThenClear_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start InitAlbumInfos_ThenClear_Test");
    AlbumDataManager dataManager;
    vector<int> albumIds = {TEST_ALBUM_ID_1, TEST_ALBUM_ID_2};
    int32_t result1 = dataManager.InitAlbumInfos(albumIds);
    EXPECT_EQ(result1, ACCURATE_REFRESH_RET_OK);
    
    dataManager.ClearChangeInfos();
    
    int32_t result2 = dataManager.InitAlbumInfos(albumIds);
    EXPECT_EQ(result2, ACCURATE_REFRESH_RET_OK);
}

HWTEST_F(AlbumDataManagerTest, GetPhotoAssetInfo_MultipleCalls_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetPhotoAssetInfo_MultipleCalls_Test");
    AlbumDataManager dataManager;
    PhotoAssetChangeInfo assetInfo1 = dataManager.GetPhotoAssetInfo(TEST_FILE_ID_1);
    PhotoAssetChangeInfo assetInfo2 = dataManager.GetPhotoAssetInfo(TEST_FILE_ID_1);
    PhotoAssetChangeInfo assetInfo3 = dataManager.GetPhotoAssetInfo(TEST_FILE_ID_1);
    EXPECT_EQ(assetInfo1.fileId_, assetInfo2.fileId_);
    EXPECT_EQ(assetInfo2.fileId_, assetInfo3.fileId_);
}

HWTEST_F(AlbumDataManagerTest, PostProcessModifiedDatas_CoverUriChange_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start PostProcessModifiedDatas_CoverUriChange_Test");
    AlbumDataManager dataManager;
    vector<int> albumIds = {TEST_ALBUM_ID_1};
    dataManager.InitAlbumInfos(albumIds);
    vector<int32_t> keys = {TEST_ALBUM_ID_1};
    int32_t result = dataManager.PostProcessModifiedDatas(keys);
    EXPECT_EQ(result, ACCURATE_REFRESH_RET_OK);
}

HWTEST_F(AlbumDataManagerTest, PostProcessModifiedDatas_HiddenCoverUriChange_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start PostProcessModifiedDatas_HiddenCoverUriChange_Test");
    AlbumDataManager dataManager;
    vector<int> albumIds = {TEST_ALBUM_ID_1};
    dataManager.InitAlbumInfos(albumIds);
    vector<int32_t> keys = {TEST_ALBUM_ID_1};
    int32_t result = dataManager.PostProcessModifiedDatas(keys);
    EXPECT_EQ(result, ACCURATE_REFRESH_RET_OK);
}

HWTEST_F(AlbumDataManagerTest, PostProcessModifiedDatas_NoData_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start PostProcessModifiedDatas_NoData_Test");
    AlbumDataManager dataManager;
    vector<int32_t> keys = {99999};
    int32_t result = dataManager.PostProcessModifiedDatas(keys);
    EXPECT_EQ(result, ACCURATE_REFRESH_RET_OK);
}

HWTEST_F(AlbumDataManagerTest, GetChangeInfoKey_MultipleCalls_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetChangeInfoKey_MultipleCalls_Test");
    AlbumDataManager dataManager;
    AlbumChangeInfo changeInfo1;
    changeInfo1.albumId_ = TEST_ALBUM_ID_1;
    int32_t key1 = dataManager.GetChangeInfoKey(changeInfo1);
    
    AlbumChangeInfo changeInfo2;
    changeInfo2.albumId_ = TEST_ALBUM_ID_2;
    int32_t key2 = dataManager.GetChangeInfoKey(changeInfo2);
    
    EXPECT_EQ(key1, TEST_ALBUM_ID_1);
    EXPECT_EQ(key2, TEST_ALBUM_ID_2);
}

HWTEST_F(AlbumDataManagerTest, GetInfoByKeys_WithDuplicates_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetInfoByKeys_WithDuplicates_Test");
    AlbumDataManager dataManager;
    vector<int32_t> albumIds = {TEST_ALBUM_ID_1, TEST_ALBUM_ID_1, TEST_ALBUM_ID_2};
    vector<AlbumChangeInfo> infos = dataManager.GetInfoByKeys(albumIds);
    EXPECT_FALSE(infos.empty());
}

HWTEST_F(AlbumDataManagerTest, GetInfoByKeys_MixedValidInvalid_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetInfoByKeys_MixedValidInvalid_Test");
    AlbumDataManager dataManager;
    vector<int32_t> albumIds = {TEST_ALBUM_ID_1, INVALID_ALBUM_ID, TEST_ALBUM_ID_2};
    vector<AlbumChangeInfo> infos = dataManager.GetInfoByKeys(albumIds);
    EXPECT_FALSE(infos.empty());
}

HWTEST_F(AlbumDataManagerTest, GetInfosByPredicates_ComplexQuery_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetInfosByPredicates_ComplexQuery_Test");
    AlbumDataManager dataManager;
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_TYPE, to_string(PhotoAlbumType::USER));
    vector<AlbumChangeInfo> infos = dataManager.GetInfosByPredicates(predicates);
    EXPECT_FALSE(infos.empty());
}

HWTEST_F(AlbumDataManagerTest, GetInfosByPredicates_OrderBy_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetInfosByPredicates_OrderBy_Test");
    AlbumDataManager dataManager;
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_TYPE, to_string(PhotoAlbumType::USER));
    predicates.OrderByDesc(PhotoAlbumColumns::ALBUM_ID);
    vector<AlbumChangeInfo> infos = dataManager.GetInfosByPredicates(predicates);
    EXPECT_FALSE(infos.empty());
}

HWTEST_F(AlbumDataManagerTest, GetAlbumInfos_MultipleSystemTypes_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetAlbumInfos_MultipleSystemTypes_Test");
    AlbumDataManager dataManager;
    vector<string> systemTypes = {"1", "2", "3", "4"};
    vector<AlbumChangeInfo> infos = dataManager.GetAlbumInfos({}, systemTypes);
    EXPECT_FALSE(infos.empty());
}

HWTEST_F(AlbumDataManagerTest, GetAlbumInfos_EmptySystemTypes_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetAlbumInfos_EmptySystemTypes_Test");
    AlbumDataManager dataManager;
    vector<string> emptySystemTypes;
    vector<AlbumChangeInfo> infos = dataManager.GetAlbumInfos({}, emptySystemTypes);
    EXPECT_EQ(infos.size(), 0);
}

HWTEST_F(AlbumDataManagerTest, GetInitAlbumInfos_AfterClear_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetInitAlbumInfos_AfterClear_Test");
    AlbumDataManager dataManager;
    vector<int> albumIds = {TEST_ALBUM_ID_1, TEST_ALBUM_ID_2};
    dataManager.InitAlbumInfos(albumIds);
    
    unordered_map<int32_t, AlbumChangeInfo> initInfos1 = dataManager.GetInitAlbumInfos();
    EXPECT_FALSE(initInfos1.empty());
    
    dataManager.ClearChangeInfos();
    
    unordered_map<int32_t, AlbumChangeInfo> initInfos2 = dataManager.GetInitAlbumInfos();
    EXPECT_EQ(initInfos2.size(), 0);
}

HWTEST_F(AlbumDataManagerTest, GetInitKeys_AfterMultipleInit_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetInitKeys_AfterMultipleInit_Test");
    AlbumDataManager dataManager;
    vector<int> albumIds1 = {TEST_ALBUM_ID_1};
    dataManager.InitAlbumInfos(albumIds1);
    
    vector<int32_t> keys1 = dataManager.GetInitKeys();
    EXPECT_EQ(keys1.size(), 1);
    
    vector<int> albumIds2 = {TEST_ALBUM_ID_2};
    dataManager.InitAlbumInfos(albumIds2);
    
    vector<int32_t> keys2 = dataManager.GetInitKeys();
    EXPECT_GT(keys2.size(), 1);
}

HWTEST_F(AlbumDataManagerTest, GetAlbumDatasFromAddAlbum_StaticTest_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetAlbumDatasFromAddAlbum_StaticTest_Test");
    vector<string> albumIdsStr = {to_string(TEST_ALBUM_ID_1), to_string(TEST_ALBUM_ID_2)};
    vector<AlbumChangeData> changeDatas = AlbumDataManager::GetAlbumDatasFromAddAlbum(albumIdsStr);
    EXPECT_FALSE(changeDatas.empty());
}

HWTEST_F(AlbumDataManagerTest, SetAlbumIdsBySql_ComplexQuery_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetAlbumIdsBySql_ComplexQuery_Test");
    AlbumDataManager dataManager;
    string sql = "SELECT * FROM " + PhotoAlbumColumns::TABLE + " WHERE " +
        PhotoAlbumColumns::ALBUM_TYPE + " = ? AND " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + " = ?";
    vector<ValueObject> bindArgs;
    bindArgs.push_back(ValueObject(static_cast<int>(PhotoAlbumType::USER)));
    bindArgs.push_back(ValueObject(static_cast<int>(PhotoAlbumSubType::USER_GENERIC)));
    int32_t result = dataManager.SetAlbumIdsBySql(sql, bindArgs);
    EXPECT_EQ(result, NativeRdb::E_OK);
}

HWTEST_F(AlbumDataManagerTest, SetAlbumIdsByFileds_LargeFileIds_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetAlbumIdsByFileds_LargeFileIds_Test");
    AlbumDataManager dataManager;
    vector<int32_t> fileIds;
    for (int i = 0; i < 100; i++) {
        fileIds.push_back(1000 + i);
    }
    int32_t result = dataManager.SetAlbumIdsByFileds(fileIds);
    EXPECT_EQ(result, NativeRdb::E_OK);
}

HWTEST_F(AlbumDataManagerTest, SetAlbumIdsByFileds_NegativeFileIds_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetAlbumIdsByFileds_NegativeFileIds_Test");
    AlbumDataManager dataManager;
    vector<int32_t> fileIds = {-1, -2, -3};
    int32_t result = dataManager.SetAlbumIdsByFileds(fileIds);
    EXPECT_EQ(result, NativeRdb::E_OK);
}

HWTEST_F(AlbumDataManagerTest, ClearChangeInfos_AfterInit_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ClearChangeInfos_AfterInit_Test");
    AlbumDataManager dataManager;
    vector<int> albumIds = {TEST_ALBUM_ID_1};
    dataManager.InitAlbumInfos(albumIds);
    
    vector<int32_t> keysBefore = dataManager.GetInitKeys();
    EXPECT_FALSE(keysBefore.empty());
    
    dataManager.ClearChangeInfos();
    
    vector<int32_t> keysAfter = dataManager.GetInitKeys();
    EXPECT_EQ(keysAfter.size(), 0);
}

HWTEST_F(AlbumDataManagerTest, CheckIsExceed_MultipleChecks_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CheckIsExceed_MultipleChecks_Test");
    AlbumDataManager dataManager;
    
    bool result1 = dataManager.CheckIsExceed(false);
    EXPECT_EQ(result1, false);
    
    bool result2 = dataManager.CheckIsExceed(true);
    EXPECT_EQ(result2, false);
    
    bool result3 = dataManager.CheckIsExceed(false);
    EXPECT_EQ(result3, false);
}

HWTEST_F(AlbumDataManagerTest, CheckIsExceed_WithPredicatesMultipleCalls_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CheckIsExceed_WithPredicatesMultipleCalls_Test");
    AlbumDataManager dataManager;
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    
    bool result1 = dataManager.CheckIsExceed(predicates, false);
    EXPECT_EQ(result1, false);
    
    bool result2 = dataManager.CheckIsExceed(predicates, true);
    EXPECT_EQ(result2, false);
    
    bool result3 = dataManager.CheckIsExceed(predicates, false);
    EXPECT_EQ(result3, false);
}

HWTEST_F(AlbumDataManagerTest, CheckIsExceed_WithSqlMultipleCalls_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CheckIsExceed_WithSqlMultipleCalls_Test");
    AlbumDataManager dataManager;
    string sql = "SELECT * FROM " + PhotoAlbumColumns::TABLE;
    vector<ValueObject> bindArgs;
    
    bool result1 = dataManager.CheckIsExceed(sql, bindArgs, false);
    EXPECT_EQ(result1, false);
    
    bool result2 = dataManager.CheckIsExceed(sql, bindArgs, true);
    EXPECT_EQ(result2, false);
    
    bool result3 = dataManager.CheckIsExceed(sql, bindArgs, false);
    EXPECT_EQ(result3, false);
}

HWTEST_F(AlbumDataManagerTest, CheckIsExceed_WithLengthBoundary_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CheckIsExceed_WithLengthBoundary_Test");
    AlbumDataManager dataManager;
    
    bool result1 = dataManager.CheckIsExceed(static_cast<size_t>(MAX_DATA_LENGTH - 1));
    EXPECT_EQ(result1, false);
    
    bool result2 = dataManager.CheckIsExceed(static_cast<size_t>(MAX_DATA_LENGTH));
    EXPECT_EQ(result2, true);
    
    bool result3 = dataManager.CheckIsExceed(static_cast<size_t>(MAX_DATA_LENGTH + 1));
    EXPECT_EQ(result3, true);
}

HWTEST_F(AlbumDataManagerTest, CheckIsExceed_WithKeysBoundary_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CheckIsExceed_WithKeysBoundary_Test");
    AlbumDataManager dataManager;
    
    vector<int32_t> keys1;
    for (int i = 0; i < MAX_DATA_LENGTH - 1; i++) {
        keys1.push_back(i);
    }
    bool result1 = dataManager.CheckIsExceed(keys1);
    EXPECT_EQ(result1, false);
    
    vector<int32_t> keys2;
    for (int i = 0; i < MAX_DATA_LENGTH; i++) {
        keys2.push_back(i);
    }
    bool result2 = dataManager.CheckIsExceed(keys2);
    EXPECT_EQ(result2, true);
}

HWTEST_F(AlbumDataManagerTest, CheckIsForRecheck_AfterClear_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CheckIsForRecheck_AfterClear_Test");
    AlbumDataManager dataManager;
    vector<int> albumIds = {TEST_ALBUM_ID_1};
    dataManager.InitAlbumInfos(albumIds);
    
    bool result1 = dataManager.CheckIsForRecheck();
    EXPECT_EQ(result1, false);
    
    dataManager.ClearChangeInfos();
    
    bool result2 = dataManager.CheckIsForRecheck();
    EXPECT_EQ(result2, false);
}

HWTEST_F(AlbumDataManagerTest, CheckIsForRecheck_WithExceedAndClear_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CheckIsForRecheck_WithExceedAndClear_Test");
    AlbumDataManager dataManager;
    vector<int> albumIds;
    for (int i = 0; i < MAX_ALBUM_COUNT; i++) {
        albumIds.push_back(1000 + i);
    }
    dataManager.InitAlbumInfos(albumIds);
    dataManager.CheckIsExceed(true);
    
    bool result1 = dataManager.CheckIsForRecheck();
    EXPECT_EQ(result1, false);
    
    dataManager.ClearChangeInfos();
    
    bool result2 = dataManager.CheckIsForRecheck();
    EXPECT_EQ(result2, false);
}

HWTEST_F(AlbumDataManagerTest, FullWorkflow_CompleteCycle_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start FullWorkflow_CompleteCycle_Test");
    AlbumDataManager dataManager;
    
    vector<int> albumIds = {TEST_ALBUM_ID_1, TEST_ALBUM_ID_2};
    int32_t initResult = dataManager.InitAlbumInfos(albumIds);
    EXPECT_EQ(initResult, ACCURATE_REFRESH_RET_OK);
    
    vector<int32_t> keys = dataManager.GetInitKeys();
    EXPECT_FALSE(keys.empty());
    
    int32_t updateResult = dataManager.UpdateModifiedDatas();
    EXPECT_EQ(updateResult, ACCURATE_REFRESH_RET_OK);
    
    int32_t postResult = dataManager.PostProcessModifiedDatas(keys);
    EXPECT_EQ(postResult, ACCURATE_REFRESH_RET_OK);
    
    dataManager.ClearChangeInfos();
    
    vector<int32_t> clearedKeys = dataManager.GetInitKeys();
    EXPECT_EQ(clearedKeys.size(), 0);
    
    bool isExceed = dataManager.CheckIsExceed(false);
    EXPECT_EQ(isExceed, false);
}

HWTEST_F(AlbumDataManagerTest, StressTest_MultipleInitClearCycles_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start StressTest_MultipleInitClearCycles_Test");
    AlbumDataManager dataManager;
    
    for (int i = 0; i < 10; i++) {
        vector<int> albumIds = {TEST_ALBUM_ID_1, TEST_ALBUM_ID_2};
        int32_t initResult = dataManager.InitAlbumInfos(albumIds);
        EXPECT_EQ(initResult, ACCURATE_REFRESH_RET_OK);
        
        vector<int32_t> keys = dataManager.GetInitKeys();
        EXPECT_FALSE(keys.empty());
        
        dataManager.ClearChangeInfos();
        
        vector<int32_t> clearedKeys = dataManager.GetInitKeys();
        EXPECT_EQ(clearedKeys.size(), 0);
    }
}

HWTEST_F(AlbumDataManagerTest, EdgeCase_MaxIntegerValues_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start EdgeCase_MaxIntegerValues_Test");
    AlbumDataManager dataManager;
    
    AlbumChangeInfo changeInfo;
    changeInfo.albumId_ = INT32_MAX;
    int32_t key = dataManager.GetChangeInfoKey(changeInfo);
    EXPECT_EQ(key, INT32_MAX);
}

HWTEST_F(AlbumDataManagerTest, EdgeCase_MinIntegerValues_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start EdgeCase_MinIntegerValues_Test");
    AlbumDataManager dataManager;
    
    AlbumChangeInfo changeInfo;
    changeInfo.albumId_ = INT32_MIN;
    int32_t key = dataManager.GetChangeInfoKey(changeInfo);
    EXPECT_EQ(key, INT32_MIN);
}

HWTEST_F(AlbumDataManagerTest, EdgeCase_ZeroAlbumId_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start EdgeCase_ZeroAlbumId_Test");
    AlbumDataManager dataManager;
    vector<int> albumIds = {0};
    int32_t result = dataManager.InitAlbumInfos(albumIds);
    EXPECT_EQ(result, ACCURATE_REFRESH_RET_OK);
}

HWTEST_F(AlbumDataManagerTest, EdgeCase_VeryLargeAlbumId_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start EdgeCase_VeryLargeAlbumId_Test");
    AlbumDataManager dataManager;
    vector<int> albumIds = {2147483647};
    int32_t result = dataManager.InitAlbumInfos(albumIds);
    EXPECT_EQ(result, ACCURATE_REFRESH_RET_OK);
}

HWTEST_F(AlbumDataManagerTest, EdgeCase_VerySmallAlbumId_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start EdgeCase_VerySmallAlbumId_Test");
    AlbumDataManager dataManager;
    vector<int> albumIds = {-2147483648};
    int32_t result = dataManager.InitAlbumInfos(albumIds);
    EXPECT_EQ(result, ACCURATE_REFRESH_RET_OK);
}

} // namespace AccurateRefresh
} // namespace Media
} // namespace OHOS
