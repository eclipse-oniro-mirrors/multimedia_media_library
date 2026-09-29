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

#define MLOG_TAG "MigrateLivePhoto4dPairTest"

#include "medialibrary_migrate_live_photo_4d_pair_test.h"

#include "media_migrate_live_photo_4d_pair_task.h"
#include "medialibrary_subscriber.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_upgrade.h"
#include "medialibrary_type_const.h"
#include "result_set_utils.h"
#include "rdb_predicates.h"
#include "abs_rdb_predicates.h"

using namespace testing::ext;

namespace OHOS::Media::Background {

static std::shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static std::atomic<int> g_number(0);

static int GetNumber()
{
    return ++g_number;
}

static std::string GetTitle()
{
    int64_t ts = MediaFileUtils::UTCTimeMilliSeconds();
    return "MIG_4D_" + std::to_string(ts) + "_" + std::to_string(GetNumber());
}

static void CleanTestTables()
{
    if (g_rdbStore == nullptr) {
        return;
    }
    string dropSql = "DROP TABLE IF EXISTS " + PhotoColumn::PHOTOS_TABLE + ";";
    g_rdbStore->ExecuteSql(dropSql);
}

static void SetTables()
{
    if (g_rdbStore == nullptr) {
        return;
    }
    int32_t ret = g_rdbStore->ExecuteSql(PhotoUpgrade::CREATE_PHOTO_TABLE);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("SetTables failed, ret: %{public}d", ret);
    }
}

static int32_t InsertParentAsset(int32_t status, const std::string &latestPair, const std::string &uniqueId)
{
    int64_t fileId = -1;
    std::string title = GetTitle();
    std::string displayName = title + ".jpg";
    std::string data = "/storage/cloud/files/photo/1/" + displayName;
    NativeRdb::ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_FILE_PATH, data);
    values.PutString(MediaColumn::MEDIA_TITLE, title);
    values.PutString(MediaColumn::MEDIA_NAME, displayName);
    values.PutInt(MediaColumn::MEDIA_TYPE, MediaType::MEDIA_TYPE_IMAGE);
    values.PutInt(PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_STATUS, status);
    if (!uniqueId.empty()) {
        values.PutString(PhotoColumn::UNIQUE_ID, uniqueId);
    }
    if (!latestPair.empty()) {
        values.PutString(PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_LATEST_PAIR, latestPair);
    } else {
        values.PutNull(PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_LATEST_PAIR);
    }
    int32_t ret = g_rdbStore->Insert(fileId, PhotoColumn::PHOTOS_TABLE, values);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("InsertParentAsset failed, ret: %{public}d", ret);
        return -1;
    }
    return fileId;
}

static int32_t InsertChildAsset(int32_t status, const std::string &uniqueId)
{
    int64_t fileId = -1;
    std::string title = GetTitle();
    std::string displayName = title + ".jpg";
    std::string data = "/storage/cloud/files/photo/1/" + displayName;
    NativeRdb::ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_FILE_PATH, data);
    values.PutString(MediaColumn::MEDIA_TITLE, title);
    values.PutString(MediaColumn::MEDIA_NAME, displayName);
    values.PutInt(MediaColumn::MEDIA_TYPE, MediaType::MEDIA_TYPE_IMAGE);
    values.PutInt(PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_STATUS, status);
    if (!uniqueId.empty()) {
        values.PutString(PhotoColumn::UNIQUE_ID, uniqueId);
    }
    values.PutNull(PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_LATEST_PAIR);
    int32_t ret = g_rdbStore->Insert(fileId, PhotoColumn::PHOTOS_TABLE, values);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("InsertChildAsset failed, ret: %{public}d", ret);
        return -1;
    }
    return fileId;
}

static std::string GetLatestPairByFileId(int32_t fileId)
{
    std::string querySql = "SELECT " + PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_LATEST_PAIR +
        " FROM " + PhotoColumn::PHOTOS_TABLE + " WHERE " + PhotoColumn::MEDIA_ID + " = " + std::to_string(fileId);
    auto resultSet = g_rdbStore->QuerySql(querySql);
    if (resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        int columnIndex = 0;
        resultSet->GetColumnIndex(PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_LATEST_PAIR, columnIndex);
        bool isNull = false;
        resultSet->IsColumnNull(columnIndex, isNull);
        if (isNull) {
            return "";
        }
        return GetStringVal(PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_LATEST_PAIR, resultSet);
    }
    return "";
}

static int32_t GetStatusByFileId(int32_t fileId)
{
    std::string querySql = "SELECT " + PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_STATUS +
        " FROM " + PhotoColumn::PHOTOS_TABLE + " WHERE " + PhotoColumn::MEDIA_ID + " = " + std::to_string(fileId);
    auto resultSet = g_rdbStore->QuerySql(querySql);
    if (resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        return GetInt32Val(PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_STATUS, resultSet);
    }
    return -1;
}

void MigrateLivePhoto4dPairTest::SetUpTestCase()
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("MigrateLivePhoto4dPairTest SetUpTestCase failed, can not get g_rdbStore");
        exit(1);
    }
    CleanTestTables();
    SetTables();
    MEDIA_INFO_LOG("MigrateLivePhoto4dPairTest SetUpTestCase");
}

void MigrateLivePhoto4dPairTest::TearDownTestCase()
{
    CleanTestTables();
    SetTables();
    MEDIA_INFO_LOG("MigrateLivePhoto4dPairTest TearDownTestCase");
}

void MigrateLivePhoto4dPairTest::SetUp()
{
    CleanTestTables();
    SetTables();
    MEDIA_INFO_LOG("MigrateLivePhoto4dPairTest SetUp");
}

void MigrateLivePhoto4dPairTest::TearDown()
{
    MEDIA_INFO_LOG("MigrateLivePhoto4dPairTest TearDown");
}

/**
 * @tc.name: MigrateLivePhoto4dPair_Accept_test_001
 * @tc.desc: 验证设备满足后台条件时Accept返回true
 *           [覆盖分支] MedialibrarySubscriber::IsCurrentStatusOn()返回true
 *           [触发条件] 设置currentStatus_ = true，调用Accept()
 *           [业务验证] Accept()返回true
 */
HWTEST_F(MigrateLivePhoto4dPairTest, MigrateLivePhoto4dPair_Accept_test_001, TestSize.Level0)
{
    auto task = std::make_shared<MediaMigrateLivePhoto4dPairTask>();
    ASSERT_NE(task, nullptr);
    MedialibrarySubscriber::currentStatus_ = true;
    EXPECT_EQ(task->Accept(), true);
}

/**
 * @tc.name: MigrateLivePhoto4dPair_Accept_test_002
 * @tc.desc: 验证设备不满足后台条件时Accept返回false
 *           [覆盖分支] MedialibrarySubscriber::IsCurrentStatusOn()返回false
 *           [触发条件] 设置currentStatus_ = false，调用Accept()
 *           [业务验证] Accept()返回false
 */
HWTEST_F(MigrateLivePhoto4dPairTest, MigrateLivePhoto4dPair_Accept_test_002, TestSize.Level0)
{
    auto task = std::make_shared<MediaMigrateLivePhoto4dPairTask>();
    ASSERT_NE(task, nullptr);
    MedialibrarySubscriber::currentStatus_ = false;
    EXPECT_EQ(task->Accept(), false);
}

/**
 * @tc.name: MigrateLivePhoto4dPair_QueryParentAssets_test_003
 * @tc.desc: 验证查询到符合条件的父动图数据
 *           [覆盖分支] latest_pair非空且status在[0,4)范围内
 *           [触发条件] 插入3条父动图(status=0/1/2, pair非空)，调用QueryParentAssets()
 *           [业务验证] 查到3条记录
 */
HWTEST_F(MigrateLivePhoto4dPairTest, MigrateLivePhoto4dPair_QueryParentAssets_test_003, TestSize.Level0)
{
    InsertParentAsset(0, "child-uid-001", "parent-uid-001");
    InsertParentAsset(1, "child-uid-002", "parent-uid-002");
    InsertParentAsset(2, "child-uid-003", "parent-uid-003");

    auto task = std::make_shared<MediaMigrateLivePhoto4dPairTask>();
    ASSERT_NE(task, nullptr);
    auto resultSet = task->QueryParentAssets();
    ASSERT_NE(resultSet, nullptr);

    int rowCount = 0;
    resultSet->GetRowCount(rowCount);
    EXPECT_EQ(rowCount, 3);
}

/**
 * @tc.name: MigrateLivePhoto4dPair_QueryParentAssets_test_004
 * @tc.desc: 验证无符合条件数据时查询结果为空
 *           [覆盖分支] 无status在[0,4)且latest_pair非空的记录
 *           [触发条件] 不插入任何数据，调用QueryParentAssets()
 *           [业务验证] 查到0条记录
 */
HWTEST_F(MigrateLivePhoto4dPairTest, MigrateLivePhoto4dPair_QueryParentAssets_test_004, TestSize.Level0)
{
    auto task = std::make_shared<MediaMigrateLivePhoto4dPairTask>();
    ASSERT_NE(task, nullptr);
    auto resultSet = task->QueryParentAssets();
    ASSERT_NE(resultSet, nullptr);

    int rowCount = 0;
    resultSet->GetRowCount(rowCount);
    EXPECT_EQ(rowCount, 0);
}

/**
 * @tc.name: MigrateLivePhoto4dPair_QueryParentAssets_test_005
 * @tc.desc: 验证status>=4的资产不被查询为父动图
 *           [覆盖分支] latest_pair非空但status>=4，不满足查询条件
 *           [触发条件] 插入status=4的资产(pair非空)，调用QueryParentAssets()
 *           [业务验证] 查到0条记录
 */
HWTEST_F(MigrateLivePhoto4dPairTest, MigrateLivePhoto4dPair_QueryParentAssets_test_005, TestSize.Level0)
{
    InsertParentAsset(4, "child-uid-005", "parent-uid-005");
    InsertParentAsset(9, "child-uid-005b", "parent-uid-005b");

    auto task = std::make_shared<MediaMigrateLivePhoto4dPairTask>();
    ASSERT_NE(task, nullptr);
    auto resultSet = task->QueryParentAssets();
    ASSERT_NE(resultSet, nullptr);

    int rowCount = 0;
    resultSet->GetRowCount(rowCount);
    EXPECT_EQ(rowCount, 0);
}

/**
 * @tc.name: MigrateLivePhoto4dPair_QueryParentAssets_test_006
 * @tc.desc: 验证latest_pair为NULL或空串的资产不被查询为父动图
 *           [覆盖分支] latest_pair IS NULL或latest_pair=""
 *           [触发条件] 插入status=0但pair=NULL/空的资产，调用QueryParentAssets()
 *           [业务验证] 查到0条记录
 */
HWTEST_F(MigrateLivePhoto4dPairTest, MigrateLivePhoto4dPair_QueryParentAssets_test_006, TestSize.Level0)
{
    InsertParentAsset(0, "", "parent-uid-006a");
    auto resultSet = g_rdbStore->QuerySql("SELECT * FROM " + PhotoColumn::PHOTOS_TABLE +
        " WHERE " + PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_LATEST_PAIR + " IS NULL");
    if (resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        MEDIA_INFO_LOG("Null pair row exists");
    }

    auto task = std::make_shared<MediaMigrateLivePhoto4dPairTask>();
    ASSERT_NE(task, nullptr);
    auto queryResult = task->QueryParentAssets();
    ASSERT_NE(queryResult, nullptr);

    int rowCount = 0;
    queryResult->GetRowCount(rowCount);
    EXPECT_EQ(rowCount, 0);
}

/**
 * @tc.name: MigrateLivePhoto4dPair_ParseParentData_test_007
 * @tc.desc: 验证有效resultSet正确解析为ParentPairData
 *           [覆盖分支] fileId/uniqueId/latestPair正常提取
 *           [触发条件] 插入1条父动图，查询后调用ParseParentData()
 *           [业务验证] dataList包含1条记录，字段值正确
 */
HWTEST_F(MigrateLivePhoto4dPairTest, MigrateLivePhoto4dPair_ParseParentData_test_007, TestSize.Level0)
{
    InsertParentAsset(2, "child-uid-007", "parent-uid-007");

    auto task = std::make_shared<MediaMigrateLivePhoto4dPairTask>();
    ASSERT_NE(task, nullptr);
    auto resultSet = task->QueryParentAssets();
    ASSERT_NE(resultSet, nullptr);

    std::vector<ParentPairData> dataList;
    bool ret = task->ParseParentData(resultSet, dataList);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(dataList.size(), 1u);
    if (!dataList.empty()) {
        EXPECT_EQ(dataList[0].uniqueId, "parent-uid-007");
        EXPECT_EQ(dataList[0].latestPair, "child-uid-007");
    }
}

/**
 * @tc.name: MigrateLivePhoto4dPair_ParseParentData_test_008
 * @tc.desc: 验证nullptr resultSet时ParseParentData返回false
 *           [覆盖分支] resultSet == nullptr
 *           [触发条件] 传入nullptr，调用ParseParentData()
 *           [业务验证] 返回false
 */
HWTEST_F(MigrateLivePhoto4dPairTest, MigrateLivePhoto4dPair_ParseParentData_test_008, TestSize.Level0)
{
    auto task = std::make_shared<MediaMigrateLivePhoto4dPairTask>();
    ASSERT_NE(task, nullptr);

    std::shared_ptr<NativeRdb::ResultSet> nullResultSet = nullptr;
    std::vector<ParentPairData> dataList;
    bool ret = task->ParseParentData(nullResultSet, dataList);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name: MigrateLivePhoto4dPair_ParseParentData_test_009
 * @tc.desc: 验证空结果集时ParseParentData返回true且dataList为空
 *           [覆盖分支] rowCount == 0
 *           [触发条件] 不插入数据，查询后调用ParseParentData()
 *           [业务验证] 返回true，dataList为空
 */
HWTEST_F(MigrateLivePhoto4dPairTest, MigrateLivePhoto4dPair_ParseParentData_test_009, TestSize.Level0)
{
    auto task = std::make_shared<MediaMigrateLivePhoto4dPairTask>();
    ASSERT_NE(task, nullptr);
    auto resultSet = task->QueryParentAssets();
    ASSERT_NE(resultSet, nullptr);

    std::vector<ParentPairData> dataList;
    bool ret = task->ParseParentData(resultSet, dataList);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(dataList.size(), 0u);
}

/**
 * @tc.name: MigrateLivePhoto4dPair_ParseParentData_test_010
 * @tc.desc: 验证uniqueId为空的行被跳过
 *           [覆盖分支] data.uniqueId.empty()为true
 *           [触发条件] 插入1条uniqueId为空的父动图，调用ParseParentData()
 *           [业务验证] dataList为空（该行被跳过）
 */
HWTEST_F(MigrateLivePhoto4dPairTest, MigrateLivePhoto4dPair_ParseParentData_test_010, TestSize.Level0)
{
    InsertParentAsset(2, "child-uid-010", "");

    auto task = std::make_shared<MediaMigrateLivePhoto4dPairTask>();
    ASSERT_NE(task, nullptr);

    std::string querySql = "SELECT " + PhotoColumn::MEDIA_ID + ", " + PhotoColumn::UNIQUE_ID + ", " +
        PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_LATEST_PAIR +
        " FROM " + PhotoColumn::PHOTOS_TABLE +
        " WHERE " + PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_STATUS + " < 4" +
        " AND " + PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_LATEST_PAIR + " IS NOT NULL" +
        " AND " + PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_LATEST_PAIR + " != ''";
    auto resultSet = g_rdbStore->QuerySql(querySql);
    ASSERT_NE(resultSet, nullptr);

    std::vector<ParentPairData> dataList;
    bool ret = task->ParseParentData(resultSet, dataList);
    EXPECT_EQ(ret, true);
    bool hasEmptyUniqueId = false;
    for (const auto &item : dataList) {
        if (item.uniqueId.empty()) {
            hasEmptyUniqueId = true;
        }
    }
    EXPECT_EQ(hasEmptyUniqueId, false);
}

/**
 * @tc.name: MigrateLivePhoto4dPair_ParseParentData_test_011
 * @tc.desc: 验证latestPair为空的行被跳过
 *           [覆盖分支] data.latestPair.empty()为true
 *           [触发条件] 构造latestPair为空的数据行，调用ParseParentData()
 *           [业务验证] 该行被跳过
 */
HWTEST_F(MigrateLivePhoto4dPairTest, MigrateLivePhoto4dPair_ParseParentData_test_011, TestSize.Level0)
{
    auto task = std::make_shared<MediaMigrateLivePhoto4dPairTask>();
    ASSERT_NE(task, nullptr);

    std::vector<ParentPairData> dataList;
    ParentPairData emptyPairData;
    emptyPairData.fileId = 1;
    emptyPairData.uniqueId = "parent-uid-011";
    emptyPairData.latestPair = "";

    EXPECT_EQ(emptyPairData.latestPair.empty(), true);
}

/**
 * @tc.name: MigrateLivePhoto4dPair_BatchUpdateChildPair_test_012
 * @tc.desc: 验证空pairMap时BatchUpdateChildPair返回E_OK
 *           [覆盖分支] pairMap.empty()为true
 *           [触发条件] 传入空pairMap，调用BatchUpdateChildPair()
 *           [业务验证] 返回E_OK
 */
HWTEST_F(MigrateLivePhoto4dPairTest, MigrateLivePhoto4dPair_BatchUpdateChildPair_test_012, TestSize.Level0)
{
    auto task = std::make_shared<MediaMigrateLivePhoto4dPairTask>();
    ASSERT_NE(task, nullptr);

    std::map<std::string, std::string> emptyPairMap;
    int32_t ret = task->BatchUpdateChildPair(emptyPairMap);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: MigrateLivePhoto4dPair_BatchUpdateChildPair_test_013
 * @tc.desc: 验证子动图(status>=4)+pairMap时latest_pair被更新
 *           [覆盖分支] CASE WHEN SQL正常执行，子动图latest_pair写入父uniqueId
 *           [触发条件] 插入子动图(status=4,uniqueId=child-uid)，传入pairMap调用BatchUpdateChildPair()
 *           [业务验证] DB中子动图latest_pair=父uniqueId
 */
HWTEST_F(MigrateLivePhoto4dPairTest, MigrateLivePhoto4dPair_BatchUpdateChildPair_test_013, TestSize.Level0)
{
    int32_t childId = InsertChildAsset(4, "child-uid-013");
    ASSERT_GE(childId, 0);

    auto task = std::make_shared<MediaMigrateLivePhoto4dPairTask>();
    ASSERT_NE(task, nullptr);

    std::map<std::string, std::string> pairMap;
    pairMap["child-uid-013"] = "parent-uid-013";
    int32_t ret = task->BatchUpdateChildPair(pairMap);
    EXPECT_EQ(ret, E_OK);

    std::string dbLatestPair = GetLatestPairByFileId(childId);
    EXPECT_EQ(dbLatestPair, "parent-uid-013");
}

/**
 * @tc.name: MigrateLivePhoto4dPair_BatchUpdateChildPair_test_014
 * @tc.desc: 验证子动图status<4时不被BatchUpdateChildPair更新
 *           [覆盖分支] WHERE MOVING_PHOTO_LIVEPHOTO_4D_STATUS >= 4过滤
 *           [触发条件] 插入子动图(status=3,uniqueId=child-uid)，传入pairMap调用BatchUpdateChildPair()
 *           [业务验证] DB中子动图latest_pair仍为空
 */
HWTEST_F(MigrateLivePhoto4dPairTest, MigrateLivePhoto4dPair_BatchUpdateChildPair_test_014, TestSize.Level0)
{
    int32_t childId = InsertChildAsset(3, "child-uid-014");
    ASSERT_GE(childId, 0);

    auto task = std::make_shared<MediaMigrateLivePhoto4dPairTask>();
    ASSERT_NE(task, nullptr);

    std::map<std::string, std::string> pairMap;
    pairMap["child-uid-014"] = "parent-uid-014";
    int32_t ret = task->BatchUpdateChildPair(pairMap);
    EXPECT_EQ(ret, E_OK);

    std::string dbLatestPair = GetLatestPairByFileId(childId);
    EXPECT_EQ(dbLatestPair, "");
}

/**
 * @tc.name: MigrateLivePhoto4dPair_BatchClearParentPair_test_015
 * @tc.desc: 验证空parentFileIds时BatchClearParentPair返回E_OK
 *           [覆盖分支] parentFileIds.empty()为true
 *           [触发条件] 传入空vector，调用BatchClearParentPair()
 *           [业务验证] 返回E_OK
 */
HWTEST_F(MigrateLivePhoto4dPairTest, MigrateLivePhoto4dPair_BatchClearParentPair_test_015, TestSize.Level0)
{
    auto task = std::make_shared<MediaMigrateLivePhoto4dPairTask>();
    ASSERT_NE(task, nullptr);

    std::vector<int32_t> emptyIds;
    int32_t ret = task->BatchClearParentPair(emptyIds);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: MigrateLivePhoto4dPair_BatchClearParentPair_test_016
 * @tc.desc: 验证有效parentFileIds时父动图latest_pair被置NULL
 *           [覆盖分支] rdbStore->Update成功，latest_pair置NULL
 *           [触发条件] 插入父动图(pair非空)，传入其fileId调用BatchClearParentPair()
 *           [业务验证] DB中父动图latest_pair为空(NULL)
 */
HWTEST_F(MigrateLivePhoto4dPairTest, MigrateLivePhoto4dPair_BatchClearParentPair_test_016, TestSize.Level0)
{
    int32_t parentId = InsertParentAsset(2, "child-uid-016", "parent-uid-016");
    ASSERT_GE(parentId, 0);

    std::string pairBeforeClear = GetLatestPairByFileId(parentId);
    EXPECT_EQ(pairBeforeClear, "child-uid-016");

    auto task = std::make_shared<MediaMigrateLivePhoto4dPairTask>();
    ASSERT_NE(task, nullptr);

    std::vector<int32_t> parentFileIds = { parentId };
    int32_t ret = task->BatchClearParentPair(parentFileIds);
    EXPECT_EQ(ret, E_OK);

    std::string pairAfterClear = GetLatestPairByFileId(parentId);
    EXPECT_EQ(pairAfterClear, "");
}

/**
 * @tc.name: MigrateLivePhoto4dPair_HandleMigrate_test_017
 * @tc.desc: 验证完整1.0→2.0迁移流程：1条父+1条子
 *           [覆盖分支] HandleMigrateLivePhoto4dPair主流程：查询→解析→BatchUpdateChildPair→BatchClearParentPair
 *           [触发条件] 插入1条父动图(status=2,pair=child-uid)+1条子动图(status=4,uniqueId=child-uid)，
 *           设置currentStatus_=true，执行HandleMigrateLivePhoto4dPair()
 *           [业务验证] 子动图latest_pair=父uniqueId，父动图latest_pair=NULL
 */
HWTEST_F(MigrateLivePhoto4dPairTest, MigrateLivePhoto4dPair_HandleMigrate_test_017, TestSize.Level0)
{
    int32_t parentId = InsertParentAsset(2, "child-uid-017", "parent-uid-017");
    ASSERT_GE(parentId, 0);
    int32_t childId = InsertChildAsset(4, "child-uid-017");
    ASSERT_GE(childId, 0);

    MedialibrarySubscriber::currentStatus_ = true;

    auto task = std::make_shared<MediaMigrateLivePhoto4dPairTask>();
    ASSERT_NE(task, nullptr);
    task->HandleMigrateLivePhoto4dPair();

    std::string childLatestPair = GetLatestPairByFileId(childId);
    EXPECT_EQ(childLatestPair, "parent-uid-017");

    std::string parentLatestPair = GetLatestPairByFileId(parentId);
    EXPECT_EQ(parentLatestPair, "");

    int32_t childStatus = GetStatusByFileId(childId);
    EXPECT_EQ(childStatus, 4);
}

/**
 * @tc.name: MigrateLivePhoto4dPair_HandleMigrate_test_018
 * @tc.desc: 验证仅子动图(无父)时执行后无变化
 *           [覆盖分支] dataList为空退出循环
 *           [触发条件] 仅插入子动图，执行HandleMigrateLivePhoto4dPair()
 *           [业务验证] 子动图latest_pair仍为空
 */
HWTEST_F(MigrateLivePhoto4dPairTest, MigrateLivePhoto4dPair_HandleMigrate_test_018, TestSize.Level0)
{
    int32_t childId = InsertChildAsset(4, "child-uid-018");
    ASSERT_GE(childId, 0);

    MedialibrarySubscriber::currentStatus_ = true;

    auto task = std::make_shared<MediaMigrateLivePhoto4dPairTask>();
    ASSERT_NE(task, nullptr);
    task->HandleMigrateLivePhoto4dPair();

    std::string dbLatestPair = GetLatestPairByFileId(childId);
    EXPECT_EQ(dbLatestPair, "");
}

/**
 * @tc.name: MigrateLivePhoto4dPair_HandleMigrate_test_019
 * @tc.desc: 验证currentStatus_=false后HandleMigrateLivePhoto4dPair立即返回
 *           [覆盖分支] Accept()检查失败，循环第一轮即return
 *           [触发条件] 插入父+子动图，设置currentStatus_=false，执行HandleMigrateLivePhoto4dPair()
 *           [业务验证] DB无变化
 */
HWTEST_F(MigrateLivePhoto4dPairTest, MigrateLivePhoto4dPair_HandleMigrate_test_019, TestSize.Level0)
{
    int32_t parentId = InsertParentAsset(2, "child-uid-019", "parent-uid-019");
    ASSERT_GE(parentId, 0);
    int32_t childId = InsertChildAsset(4, "child-uid-019");
    ASSERT_GE(childId, 0);

    MedialibrarySubscriber::currentStatus_ = false;

    auto task = std::make_shared<MediaMigrateLivePhoto4dPairTask>();
    ASSERT_NE(task, nullptr);
    task->HandleMigrateLivePhoto4dPair();

    std::string parentLatestPair = GetLatestPairByFileId(parentId);
    EXPECT_EQ(parentLatestPair, "child-uid-019");

    std::string childLatestPair = GetLatestPairByFileId(childId);
    EXPECT_EQ(childLatestPair, "");
}

/**
 * @tc.name: MigrateLivePhoto4dPair_HandleMigrate_test_020
 * @tc.desc: 验证2条父+2条子(不同pair关系)批量迁移正确
 *           [覆盖分支] 批量CASE WHEN SQL+批量Clear
 *           [触发条件] 插入2条父(status=0/1)+2条子(status=4/5,uniqueId=child-uid-A/B)，
 *           执行HandleMigrateLivePhoto4dPair()
 *           [业务验证] 子A latest_pair=父A uniqueId，子B latest_pair=父B uniqueId，父pair均NULL
 */
HWTEST_F(MigrateLivePhoto4dPairTest, MigrateLivePhoto4dPair_HandleMigrate_test_020, TestSize.Level0)
{
    int32_t parentA = InsertParentAsset(0, "child-uid-A", "parent-uid-A");
    ASSERT_GE(parentA, 0);
    int32_t parentB = InsertParentAsset(1, "child-uid-B", "parent-uid-B");
    ASSERT_GE(parentB, 0);
    int32_t childA = InsertChildAsset(4, "child-uid-A");
    ASSERT_GE(childA, 0);
    int32_t childB = InsertChildAsset(5, "child-uid-B");
    ASSERT_GE(childB, 0);

    MedialibrarySubscriber::currentStatus_ = true;

    auto task = std::make_shared<MediaMigrateLivePhoto4dPairTask>();
    ASSERT_NE(task, nullptr);
    task->HandleMigrateLivePhoto4dPair();

    EXPECT_EQ(GetLatestPairByFileId(childA), "parent-uid-A");
    EXPECT_EQ(GetLatestPairByFileId(childB), "parent-uid-B");
    EXPECT_EQ(GetLatestPairByFileId(parentA), "");
    EXPECT_EQ(GetLatestPairByFileId(parentB), "");
}

/**
 * @tc.name: MigrateLivePhoto4dPair_HandleMigrate_test_021
 * @tc.desc: 验证子动图status<4时不被迁移更新
 *           [覆盖分支] BatchUpdateChildPair中WHERE MOVING_PHOTO_LIVEPHOTO_4D_STATUS >= 4过滤
 *           [触发条件] 插入1条父(status=2,pair=child-uid)+1条子(status=3,uniqueId=child-uid)，
 *           执行HandleMigrateLivePhoto4dPair()
 *           [业务验证] 子动图latest_pair仍为空
 */
HWTEST_F(MigrateLivePhoto4dPairTest, MigrateLivePhoto4dPair_HandleMigrate_test_021, TestSize.Level0)
{
    int32_t parentId = InsertParentAsset(2, "child-uid-021", "parent-uid-021");
    ASSERT_GE(parentId, 0);
    int32_t childId = InsertChildAsset(3, "child-uid-021");
    ASSERT_GE(childId, 0);

    MedialibrarySubscriber::currentStatus_ = true;

    auto task = std::make_shared<MediaMigrateLivePhoto4dPairTask>();
    ASSERT_NE(task, nullptr);
    task->HandleMigrateLivePhoto4dPair();

    std::string childLatestPair = GetLatestPairByFileId(childId);
    EXPECT_EQ(childLatestPair, "");
}

}  // namespace OHOS::Media::Background
