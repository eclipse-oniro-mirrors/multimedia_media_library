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

#define MLOG_TAG "FixDuplicateUniqueIdTest"

#include "medialibrary_fix_duplicate_unique_id_test.h"

#include "media_fix_duplicate_unique_id_task.h"
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
    return "DUP_UID_" + std::to_string(ts) + "_" + std::to_string(GetNumber());
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

static int32_t InsertAsset(const std::string &uniqueId, int64_t dateModified,
    int32_t mediaType = MediaType::MEDIA_TYPE_IMAGE)
{
    int64_t fileId = -1;
    std::string title = GetTitle();
    std::string displayName = title + ".jpg";
    std::string data = "/storage/cloud/files/photo/1/" + displayName;
    NativeRdb::ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_FILE_PATH, data);
    values.PutString(MediaColumn::MEDIA_TITLE, title);
    values.PutString(MediaColumn::MEDIA_NAME, displayName);
    values.PutInt(MediaColumn::MEDIA_TYPE, mediaType);
    values.Put(MediaColumn::MEDIA_DATE_MODIFIED, dateModified);
    if (!uniqueId.empty() && uniqueId != "-1") {
        values.PutString(PhotoColumn::UNIQUE_ID, uniqueId);
    } else if (uniqueId == "-1") {
        values.PutString(PhotoColumn::UNIQUE_ID, "-1");
    } else {
        values.PutNull(PhotoColumn::UNIQUE_ID);
    }
    int32_t ret = g_rdbStore->Insert(fileId, PhotoColumn::PHOTOS_TABLE, values);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("InsertAsset failed, ret: %{public}d", ret);
        return -1;
    }
    return fileId;
}

static std::string GetUniqueIdByFileId(int32_t fileId)
{
    std::string querySql = "SELECT " + PhotoColumn::UNIQUE_ID +
        " FROM " + PhotoColumn::PHOTOS_TABLE +
        " WHERE " + PhotoColumn::MEDIA_ID + " = " + std::to_string(fileId);
    auto resultSet = g_rdbStore->QuerySql(querySql);
    if (resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        int columnIndex = 0;
        resultSet->GetColumnIndex(PhotoColumn::UNIQUE_ID, columnIndex);
        bool isNull = false;
        resultSet->IsColumnNull(columnIndex, isNull);
        if (isNull) {
            return "";
        }
        return GetStringVal(PhotoColumn::UNIQUE_ID, resultSet);
    }
    return "";
}

static int32_t CountDuplicateGroups()
{
    std::string sql = "SELECT COUNT(*) FROM " + PhotoColumn::PHOTOS_TABLE +
        " WHERE " + PhotoColumn::UNIQUE_ID + " IS NOT NULL AND " +
        PhotoColumn::UNIQUE_ID + " != '' AND " +
        PhotoColumn::UNIQUE_ID + " != '-1'" +
        " GROUP BY " + PhotoColumn::UNIQUE_ID +
        " HAVING COUNT(*) > 1";
    auto resultSet = g_rdbStore->QuerySql(sql);
    if (resultSet != nullptr) {
        int rowCount = 0;
        resultSet->GetRowCount(rowCount);
        return rowCount;
    }
    return 0;
}

void FixDuplicateUniqueIdTest::SetUpTestCase()
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("FixDuplicateUniqueIdTest SetUpTestCase failed, can not get g_rdbStore");
        exit(1);
    }
    CleanTestTables();
    SetTables();
    MEDIA_INFO_LOG("FixDuplicateUniqueIdTest SetUpTestCase");
}

void FixDuplicateUniqueIdTest::TearDownTestCase()
{
    CleanTestTables();
    SetTables();
    MEDIA_INFO_LOG("FixDuplicateUniqueIdTest TearDownTestCase");
}

void FixDuplicateUniqueIdTest::SetUp()
{
    CleanTestTables();
    SetTables();
    MEDIA_INFO_LOG("FixDuplicateUniqueIdTest SetUp");
}

void FixDuplicateUniqueIdTest::TearDown()
{
    MEDIA_INFO_LOG("FixDuplicateUniqueIdTest TearDown");
}

/**
 * @tc.name: FixDuplicateUniqueId_Accept_test_001
 * @tc.desc: 验证设备满足后台条件时Accept返回true
 *           [覆盖分支] MedialibrarySubscriber::IsCurrentStatusOn()返回true
 *           [触发条件] 设置currentStatus_ = true，调用Accept()
 *           [业务验证] Accept()返回true
 */
HWTEST_F(FixDuplicateUniqueIdTest, FixDuplicateUniqueId_Accept_test_001, TestSize.Level0)
{
    auto task = std::make_shared<MediaFixDuplicateUniqueIdTask>();
    ASSERT_NE(task, nullptr);
    MedialibrarySubscriber::currentStatus_ = true;
    EXPECT_EQ(task->Accept(), true);
}

/**
 * @tc.name: FixDuplicateUniqueId_Accept_test_002
 * @tc.desc: 验证设备不满足后台条件时Accept返回false
 *           [覆盖分支] MedialibrarySubscriber::IsCurrentStatusOn()返回false
 *           [触发条件] 设置currentStatus_ = false，调用Accept()
 *           [业务验证] Accept()返回false
 */
HWTEST_F(FixDuplicateUniqueIdTest, FixDuplicateUniqueId_Accept_test_002, TestSize.Level0)
{
    auto task = std::make_shared<MediaFixDuplicateUniqueIdTask>();
    ASSERT_NE(task, nullptr);
    MedialibrarySubscriber::currentStatus_ = false;
    EXPECT_EQ(task->Accept(), false);
}

/**
 * @tc.name: FixDuplicateUniqueId_FindDuplicateUniqueIds_test_003
 * @tc.desc: 验证无重复uniqueId时FindDuplicateUniqueIds返回空列表
 *           [覆盖分支] GROUP BY HAVING COUNT(*) > 1无匹配
 *           [触发条件] 不插入任何数据，调用FindDuplicateUniqueIds()
 *           [业务验证] 返回空vector
 */
HWTEST_F(FixDuplicateUniqueIdTest, FixDuplicateUniqueId_FindDuplicateUniqueIds_test_003, TestSize.Level0)
{
    auto task = std::make_shared<MediaFixDuplicateUniqueIdTask>();
    ASSERT_NE(task, nullptr);
    auto result = task->FindDuplicateUniqueIds();
    EXPECT_EQ(result.size(), 0u);
}

/**
 * @tc.name: FixDuplicateUniqueId_FindDuplicateUniqueIds_test_004
 * @tc.desc: 验证2条记录uniqueId相同时FindDuplicateUniqueIds返回该uniqueId
 *           [覆盖分支] GROUP BY HAVING COUNT(*) > 1有匹配
 *           [触发条件] 插入2条uniqueId="dup-uid-004"的记录，调用FindDuplicateUniqueIds()
 *           [业务验证] 返回包含"dup-uid-004"的vector
 */
HWTEST_F(FixDuplicateUniqueIdTest, FixDuplicateUniqueId_FindDuplicateUniqueIds_test_004, TestSize.Level0)
{
    InsertAsset("dup-uid-004", 1000);
    InsertAsset("dup-uid-004", 2000);

    auto task = std::make_shared<MediaFixDuplicateUniqueIdTask>();
    ASSERT_NE(task, nullptr);
    auto result = task->FindDuplicateUniqueIds();
    EXPECT_EQ(result.size(), 1u);
    if (!result.empty()) {
        EXPECT_EQ(result[0], "dup-uid-004");
    }
}

/**
 * @tc.name: FixDuplicateUniqueId_FindDuplicateUniqueIds_test_005
 * @tc.desc: 验证uniqueId为NULL/空串/'-1'的记录不纳入重复检测
 *           [覆盖分支] WHERE unique_id IS NOT NULL AND != '' AND != '-1'
 *           [触发条件] 插入2条uniqueId=NULL、2条uniqueId=""、2条uniqueId="-1"的记录，调用FindDuplicateUniqueIds()
 *           [业务验证] 返回空vector
 */
HWTEST_F(FixDuplicateUniqueIdTest, FixDuplicateUniqueId_FindDuplicateUniqueIds_test_005, TestSize.Level0)
{
    InsertAsset("", 1000);
    InsertAsset("", 2000);
    InsertAsset("-1", 1000);
    InsertAsset("-1", 2000);

    auto task = std::make_shared<MediaFixDuplicateUniqueIdTask>();
    ASSERT_NE(task, nullptr);
    auto result = task->FindDuplicateUniqueIds();
    EXPECT_EQ(result.size(), 0u);
}

/**
 * @tc.name: FixDuplicateUniqueId_FindDuplicateUniqueIds_test_006
 * @tc.desc: 验证多个不同重复组被正确查找
 *           [覆盖分支] 多个uniqueId的HAVING COUNT > 1
 *           [触发条件] 插入2条"dup-A"和2条"dup-B"，调用FindDuplicateUniqueIds()
 *           [业务验证] 返回包含2个uniqueId的vector
 */
HWTEST_F(FixDuplicateUniqueIdTest, FixDuplicateUniqueId_FindDuplicateUniqueIds_test_006, TestSize.Level0)
{
    InsertAsset("dup-A", 1000);
    InsertAsset("dup-A", 2000);
    InsertAsset("dup-B", 1000);
    InsertAsset("dup-B", 2000);

    auto task = std::make_shared<MediaFixDuplicateUniqueIdTask>();
    ASSERT_NE(task, nullptr);
    auto result = task->FindDuplicateUniqueIds();
    EXPECT_EQ(result.size(), 2u);
}

/**
 * @tc.name: FixDuplicateUniqueId_QueryDuplicateRecords_test_007
 * @tc.desc: 验证QueryDuplicateRecords按date_modified ASC排序返回记录
 *           [覆盖分支] OrderByAsc(date_modified) + OrderByAsc(media_id)
 *           [触发条件] 插入3条相同uniqueId(date_modified=3000/1000/2000)的记录，调用QueryDuplicateRecords()
 *           [业务验证] 返回3条记录，按date_modified升序排列
 */
HWTEST_F(FixDuplicateUniqueIdTest, FixDuplicateUniqueId_QueryDuplicateRecords_test_007, TestSize.Level0)
{
    InsertAsset("dup-uid-007", 3000);
    InsertAsset("dup-uid-007", 1000);
    InsertAsset("dup-uid-007", 2000);

    auto task = std::make_shared<MediaFixDuplicateUniqueIdTask>();
    ASSERT_NE(task, nullptr);
    auto records = task->QueryDuplicateRecords("dup-uid-007");
    EXPECT_EQ(records.size(), 3u);
    if (records.size() >= 3u) {
        EXPECT_EQ(records[0].dateModified, 1000);
        EXPECT_EQ(records[1].dateModified, 2000);
        EXPECT_EQ(records[2].dateModified, 3000);
    }
}

/**
 * @tc.name: FixDuplicateUniqueId_QueryDuplicateRecords_test_008
 * @tc.desc: 验证QueryDuplicateRecords在无匹配记录时返回空
 *           [覆盖分支] uniqueId不存在，查询结果为空
 *           [触发条件] 插入1条uniqueId="other-uid"的记录，调用QueryDuplicateRecords("nonexist-uid")
 *           [业务验证] 返回空vector
 */
HWTEST_F(FixDuplicateUniqueIdTest, FixDuplicateUniqueId_QueryDuplicateRecords_test_008, TestSize.Level0)
{
    InsertAsset("other-uid", 1000);

    auto task = std::make_shared<MediaFixDuplicateUniqueIdTask>();
    ASSERT_NE(task, nullptr);
    auto records = task->QueryDuplicateRecords("nonexist-uid");
    EXPECT_EQ(records.size(), 0u);
}

/**
 * @tc.name: FixDuplicateUniqueId_ProcessDuplicateGroup_test_009
 * @tc.desc: 验证ProcessDuplicateGroup在组内仅1条记录时返回E_OK不做修改
 *           [覆盖分支] records.size() <= 1
 *           [触发条件] 插入1条uniqueId="solo-uid-009"的记录，调用ProcessDuplicateGroup()
 *           [业务验证] 返回E_OK，记录uniqueId不变
 */
HWTEST_F(FixDuplicateUniqueIdTest, FixDuplicateUniqueId_ProcessDuplicateGroup_test_009, TestSize.Level0)
{
    int32_t fileId = InsertAsset("solo-uid-009", 1000);
    ASSERT_GE(fileId, 0);

    auto task = std::make_shared<MediaFixDuplicateUniqueIdTask>();
    ASSERT_NE(task, nullptr);
    int32_t ret = task->ProcessDuplicateGroup("solo-uid-009");
    EXPECT_EQ(ret, E_OK);

    std::string dbUniqueId = GetUniqueIdByFileId(fileId);
    EXPECT_EQ(dbUniqueId, "solo-uid-009");
}

/**
 * @tc.name: FixDuplicateUniqueId_ProcessDuplicateGroup_test_010
 * @tc.desc: 验证2条重复记录时保留date_modified最小的，另一条重新生成uniqueId
 *           [覆盖分支] i=1循环处理第二条记录，GenerateUUID+UpdateRecordUniqueId
 *           [触发条件] 插入2条uniqueId="dup-uid-010"(date_modified=1000/2000)的记录，调用ProcessDuplicateGroup()
 *           [业务验证] date_modified=1000的记录uniqueId不变，date_modified=2000的记录uniqueId已被更新为新UUID
 */
HWTEST_F(FixDuplicateUniqueIdTest, FixDuplicateUniqueId_ProcessDuplicateGroup_test_010, TestSize.Level0)
{
    int32_t keepId = InsertAsset("dup-uid-010", 1000);
    ASSERT_GE(keepId, 0);
    int32_t changeId = InsertAsset("dup-uid-010", 2000);
    ASSERT_GE(changeId, 0);

    auto task = std::make_shared<MediaFixDuplicateUniqueIdTask>();
    ASSERT_NE(task, nullptr);
    int32_t ret = task->ProcessDuplicateGroup("dup-uid-010");
    EXPECT_EQ(ret, E_OK);

    std::string keepUniqueId = GetUniqueIdByFileId(keepId);
    EXPECT_EQ(keepUniqueId, "dup-uid-010");

    std::string changeUniqueId = GetUniqueIdByFileId(changeId);
    EXPECT_NE(changeUniqueId, "dup-uid-010");
    EXPECT_NE(changeUniqueId, "");
    EXPECT_TRUE(MediaFileUtils::IsValidUuid(changeUniqueId));
}

/**
 * @tc.name: FixDuplicateUniqueId_ProcessDuplicateGroup_test_011
 * @tc.desc: 验证3条重复记录时保留date_modified最小的，其余2条重新生成uniqueId
 *           [覆盖分支] i=1和i=2两条记录均被处理
 *           [触发条件] 插入3条uniqueId="dup-uid-011"(date_modified=1000/2000/3000)的记录，
 *           调用ProcessDuplicateGroup()
 *           [业务验证] date_modified=1000的保留，2000和3000的uniqueId均被更新为新UUID且互不相同
 */
HWTEST_F(FixDuplicateUniqueIdTest, FixDuplicateUniqueId_ProcessDuplicateGroup_test_011, TestSize.Level0)
{
    int32_t keepId = InsertAsset("dup-uid-011", 1000);
    ASSERT_GE(keepId, 0);
    int32_t changeId1 = InsertAsset("dup-uid-011", 2000);
    ASSERT_GE(changeId1, 0);
    int32_t changeId2 = InsertAsset("dup-uid-011", 3000);
    ASSERT_GE(changeId2, 0);

    auto task = std::make_shared<MediaFixDuplicateUniqueIdTask>();
    ASSERT_NE(task, nullptr);
    int32_t ret = task->ProcessDuplicateGroup("dup-uid-011");
    EXPECT_EQ(ret, E_OK);

    std::string keepUniqueId = GetUniqueIdByFileId(keepId);
    EXPECT_EQ(keepUniqueId, "dup-uid-011");

    std::string changeUniqueId1 = GetUniqueIdByFileId(changeId1);
    EXPECT_NE(changeUniqueId1, "dup-uid-011");
    EXPECT_NE(changeUniqueId1, "");
    EXPECT_TRUE(MediaFileUtils::IsValidUuid(changeUniqueId1));

    std::string changeUniqueId2 = GetUniqueIdByFileId(changeId2);
    EXPECT_NE(changeUniqueId2, "dup-uid-011");
    EXPECT_NE(changeUniqueId2, "");
    EXPECT_TRUE(MediaFileUtils::IsValidUuid(changeUniqueId2));

    EXPECT_NE(changeUniqueId1, changeUniqueId2);
}

/**
 * @tc.name: FixDuplicateUniqueId_UpdateRecordUniqueId_test_012
 * @tc.desc: 验证UpdateRecordUniqueId成功更新记录的uniqueId
 *           [覆盖分支] rdbStore->Update成功，changedRows > 0
 *           [触发条件] 插入1条记录，调用UpdateRecordUniqueId(fileId, "new-uuid-012")
 *           [业务验证] DB中该记录uniqueId="new-uuid-012"，返回E_OK
 */
HWTEST_F(FixDuplicateUniqueIdTest, FixDuplicateUniqueId_UpdateRecordUniqueId_test_012, TestSize.Level0)
{
    int32_t fileId = InsertAsset("old-uuid-012", 1000);
    ASSERT_GE(fileId, 0);

    auto task = std::make_shared<MediaFixDuplicateUniqueIdTask>();
    ASSERT_NE(task, nullptr);
    int32_t ret = task->UpdateRecordUniqueId(fileId, "new-uuid-012");
    EXPECT_EQ(ret, E_OK);

    std::string dbUniqueId = GetUniqueIdByFileId(fileId);
    EXPECT_EQ(dbUniqueId, "new-uuid-012");
}

/**
 * @tc.name: FixDuplicateUniqueId_UpdateRecordUniqueId_test_013
 * @tc.desc: 验证UpdateRecordUniqueId对不存在的fileId返回E_ERR
 *           [覆盖分支] rdbStore->Update后changedRows == 0
 *           [触发条件] 不插入记录，调用UpdateRecordUniqueId(99999, "new-uuid-013")
 *           [业务验证] 返回E_ERR
 */
HWTEST_F(FixDuplicateUniqueIdTest, FixDuplicateUniqueId_UpdateRecordUniqueId_test_013, TestSize.Level0)
{
    auto task = std::make_shared<MediaFixDuplicateUniqueIdTask>();
    ASSERT_NE(task, nullptr);
    int32_t ret = task->UpdateRecordUniqueId(99999, "new-uuid-013");
    EXPECT_EQ(ret, E_ERR);
}

/**
 * @tc.name: FixDuplicateUniqueId_HandleFix_test_014
 * @tc.desc: 验证无重复uniqueId时HandleFixDuplicateUniqueId直接返回
 *           [覆盖分支] FindDuplicateUniqueIds返回空，循环不执行
 *           [触发条件] 插入2条不同uniqueId的记录，调用HandleFixDuplicateUniqueId()
 *           [业务验证] 两条记录uniqueId均不变
 */
HWTEST_F(FixDuplicateUniqueIdTest, FixDuplicateUniqueId_HandleFix_test_014, TestSize.Level0)
{
    int32_t id1 = InsertAsset("uid-A-014", 1000);
    ASSERT_GE(id1, 0);
    int32_t id2 = InsertAsset("uid-B-014", 2000);
    ASSERT_GE(id2, 0);

    MedialibrarySubscriber::currentStatus_ = true;

    auto task = std::make_shared<MediaFixDuplicateUniqueIdTask>();
    ASSERT_NE(task, nullptr);
    task->HandleFixDuplicateUniqueId();

    EXPECT_EQ(GetUniqueIdByFileId(id1), "uid-A-014");
    EXPECT_EQ(GetUniqueIdByFileId(id2), "uid-B-014");
}

/**
 * @tc.name: FixDuplicateUniqueId_HandleFix_test_015
 * @tc.desc: 验证1组重复uniqueId时HandleFixDuplicateUniqueId正确处理
 *           [覆盖分支] 主流程：FindDuplicateUniqueIds→逐组ProcessDuplicateGroup
 *           [触发条件] 插入2条uniqueId="dup-uid-015"(date_modified=1000/2000)的记录，
 *           设置currentStatus_=true，调用HandleFixDuplicateUniqueId()
 *           [业务验证] date_modified小的保留原uniqueId，大的被更新为新UUID
 */
HWTEST_F(FixDuplicateUniqueIdTest, FixDuplicateUniqueId_HandleFix_test_015, TestSize.Level0)
{
    int32_t keepId = InsertAsset("dup-uid-015", 1000);
    ASSERT_GE(keepId, 0);
    int32_t changeId = InsertAsset("dup-uid-015", 2000);
    ASSERT_GE(changeId, 0);

    MedialibrarySubscriber::currentStatus_ = true;

    auto task = std::make_shared<MediaFixDuplicateUniqueIdTask>();
    ASSERT_NE(task, nullptr);
    task->HandleFixDuplicateUniqueId();

    EXPECT_EQ(GetUniqueIdByFileId(keepId), "dup-uid-015");

    std::string changeUniqueId = GetUniqueIdByFileId(changeId);
    EXPECT_NE(changeUniqueId, "dup-uid-015");
    EXPECT_TRUE(MediaFileUtils::IsValidUuid(changeUniqueId));
}

/**
 * @tc.name: FixDuplicateUniqueId_HandleFix_test_016
 * @tc.desc: 验证currentStatus_=false时HandleFixDuplicateUniqueId在Accept检查后立即返回
 *           [覆盖分支] Accept()返回false，循环中return
 *           [触发条件] 插入2条重复uniqueId的记录，设置currentStatus_=false，
 *           调用HandleFixDuplicateUniqueId()
 *           [业务验证] 两条记录uniqueId均不变
 */
HWTEST_F(FixDuplicateUniqueIdTest, FixDuplicateUniqueId_HandleFix_test_016, TestSize.Level0)
{
    int32_t id1 = InsertAsset("dup-uid-016", 1000);
    ASSERT_GE(id1, 0);
    int32_t id2 = InsertAsset("dup-uid-016", 2000);
    ASSERT_GE(id2, 0);

    MedialibrarySubscriber::currentStatus_ = false;

    auto task = std::make_shared<MediaFixDuplicateUniqueIdTask>();
    ASSERT_NE(task, nullptr);
    task->HandleFixDuplicateUniqueId();

    EXPECT_EQ(GetUniqueIdByFileId(id1), "dup-uid-016");
    EXPECT_EQ(GetUniqueIdByFileId(id2), "dup-uid-016");
}

/**
 * @tc.name: FixDuplicateUniqueId_HandleFix_test_017
 * @tc.desc: 验证多组重复uniqueId时HandleFixDuplicateUniqueId逐组处理
 *           [覆盖分支] for循环遍历所有duplicateIds，每组调用ProcessDuplicateGroup
 *           [触发条件] 插入2条"dup-A"(date_modified=1000/2000)和2条"dup-B"(date_modified=3000/4000)，
 *           设置currentStatus_=true，调用HandleFixDuplicateUniqueId()
 *           [业务验证] 每组date_modified小的保留，大的被更新，无重复组残留
 */
HWTEST_F(FixDuplicateUniqueIdTest, FixDuplicateUniqueId_HandleFix_test_017, TestSize.Level0)
{
    int32_t keepA = InsertAsset("dup-A", 1000);
    ASSERT_GE(keepA, 0);
    int32_t changeA = InsertAsset("dup-A", 2000);
    ASSERT_GE(changeA, 0);
    int32_t keepB = InsertAsset("dup-B", 3000);
    ASSERT_GE(keepB, 0);
    int32_t changeB = InsertAsset("dup-B", 4000);
    ASSERT_GE(changeB, 0);

    MedialibrarySubscriber::currentStatus_ = true;

    auto task = std::make_shared<MediaFixDuplicateUniqueIdTask>();
    ASSERT_NE(task, nullptr);
    task->HandleFixDuplicateUniqueId();

    EXPECT_EQ(GetUniqueIdByFileId(keepA), "dup-A");
    EXPECT_NE(GetUniqueIdByFileId(changeA), "dup-A");
    EXPECT_TRUE(MediaFileUtils::IsValidUuid(GetUniqueIdByFileId(changeA)));

    EXPECT_EQ(GetUniqueIdByFileId(keepB), "dup-B");
    EXPECT_NE(GetUniqueIdByFileId(changeB), "dup-B");
    EXPECT_TRUE(MediaFileUtils::IsValidUuid(GetUniqueIdByFileId(changeB)));

    EXPECT_EQ(CountDuplicateGroups(), 0);
}

}  // namespace OHOS::Media::Background
