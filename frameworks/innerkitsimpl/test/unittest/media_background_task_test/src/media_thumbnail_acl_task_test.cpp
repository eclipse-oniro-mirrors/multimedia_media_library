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

#include "media_thumbnail_acl_task_test.h"

#include "medialibrary_errno.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_upgrade.h"

#include "media_thumbnail_acl_task.h"

using namespace testing::ext;
using namespace std;
using namespace OHOS::FileManagement::CloudSync;
using namespace OHOS::NativeRdb;

namespace OHOS::Media::Background {
static std::shared_ptr<MediaLibraryRdbStore> g_rdbStore;

static void CleanTestTables()
{
    std::vector<std::string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
    };
    for (auto &dropTable : dropTableList) {
        std::string dropSql = "DROP TABLE " + dropTable + ";";
        int32_t ret = g_rdbStore->ExecuteSql(dropSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Drop %{public}s table failed", dropTable.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Drop %{public}s table success", dropTable.c_str());
    }
}
 
static void SetTables()
{
    std::vector<std::string> createTableSqlList = {
        PhotoUpgrade::CREATE_PHOTO_TABLE,
    };
    for (auto &createTableSql : createTableSqlList) {
        int32_t ret = g_rdbStore->ExecuteSql(createTableSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Execute sql %{private}s failed", createTableSql.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Execute sql %{private}s success", createTableSql.c_str());
    }
}

void MediaLibraryThumbnailAclTaskTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryVideoModeTest failed, can not get g_rdbStore");
        exit(1);
    }
    CleanTestTables();
    SetTables();

    system("rm -rf /storage/media/local/files/.thumbs/Photo/*");
    std::string localThumbnailPath = "/storage/media/local/files/.thumbs/Photo/";
    bool ret = MediaFileUtils::CreateDirectory(localThumbnailPath);
    CHECK_AND_PRINT_LOG(ret, "Make %{public}s dir failed, ret=%{public}d", localThumbnailPath.c_str(), ret);
    MEDIA_INFO_LOG("MediaLibraryThumbnailAclTaskTest SetUpTestCase");
}

void MediaLibraryThumbnailAclTaskTest::TearDownTestCase(void)
{
    system("rm -rf /storage/media/local/files/.thumbs/Photo/*");
    CleanTestTables();
    SetTables();
    MEDIA_INFO_LOG("MediaLibraryThumbnailAclTaskTest TearDownTestCase");
}

void MediaLibraryThumbnailAclTaskTest::SetUp() {}

void MediaLibraryThumbnailAclTaskTest::TearDown(void) {}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsEntriesExpected_test_001, TestSize.Level0)
{
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::vector<AclXattrEntry> aclEntries = {};
    bool res = mediaThumbnailAclTestTask->IsEntriesExpected(aclEntries, false);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsEntriesExpected_test_002, TestSize.Level0)
{
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::USER_OBJ, 7, ACL_UNDEFINED_ID};
    std::vector<AclXattrEntry> aclEntries = {entry, entry, entry, entry, entry};
    bool res = mediaThumbnailAclTestTask->IsEntriesExpected(aclEntries, false);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsEntriesExpected_test_003, TestSize.Level0)
{
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::USER, 7, 1000};
    std::vector<AclXattrEntry> aclEntries = {entry, entry, entry, entry, entry};
    bool res = mediaThumbnailAclTestTask->IsEntriesExpected(aclEntries, false);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsEntriesExpected_test_004, TestSize.Level0)
{
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::GROUP_OBJ, 7, ACL_UNDEFINED_ID};
    std::vector<AclXattrEntry> aclEntries = {entry, entry, entry, entry, entry};
    bool res = mediaThumbnailAclTestTask->IsEntriesExpected(aclEntries, false);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsEntriesExpected_test_005, TestSize.Level0)
{
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::GROUP, 7, 2008};
    std::vector<AclXattrEntry> aclEntries = {entry, entry, entry, entry, entry};
    bool res = mediaThumbnailAclTestTask->IsEntriesExpected(aclEntries, false);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsEntriesExpected_test_006, TestSize.Level0)
{
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::MASK, 7, ACL_UNDEFINED_ID};
    std::vector<AclXattrEntry> aclEntries = {entry, entry, entry, entry, entry};
    bool res = mediaThumbnailAclTestTask->IsEntriesExpected(aclEntries, false);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsEntriesExpected_test_007, TestSize.Level0)
{
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::OTHER, 1, ACL_UNDEFINED_ID};
    std::vector<AclXattrEntry> aclEntries = {entry, entry, entry, entry, entry};
    bool res = mediaThumbnailAclTestTask->IsEntriesExpected(aclEntries, false);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsEntriesExpected_test_008, TestSize.Level0)
{
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::UNDEFINED, 1, ACL_UNDEFINED_ID};
    std::vector<AclXattrEntry> aclEntries = {entry, entry, entry, entry, entry};
    bool res = mediaThumbnailAclTestTask->IsEntriesExpected(aclEntries, false);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsEntriesExpected_test_009, TestSize.Level0)
{
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry1 = {ACL_TAG::USER_OBJ, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry2 = {ACL_TAG::GROUP_OBJ, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry3 = {ACL_TAG::GROUP, 5, 3008};
    AclXattrEntry entry4 = {ACL_TAG::MASK, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry5 = {ACL_TAG::OTHER, 1, ACL_UNDEFINED_ID};
    std::vector<AclXattrEntry> aclEntries = {entry1, entry2, entry3, entry4, entry5};
    bool res = mediaThumbnailAclTestTask->IsEntriesExpected(aclEntries, false);
    EXPECT_EQ(res, true);
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsGroupObjEntryExpected_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsGroupObjEntryExpected_test_001 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::GROUP_OBJ, 7, ACL_UNDEFINED_ID};
    bool res = mediaThumbnailAclTestTask->IsGroupObjEntryExpected(entry, false);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsGroupObjEntryExpected_test_001 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsGroupObjEntryExpected_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsGroupObjEntryExpected_test_002 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::GROUP_OBJ, 7, ACL_UNDEFINED_ID};
    bool res = mediaThumbnailAclTestTask->IsGroupObjEntryExpected(entry, true);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsGroupObjEntryExpected_test_002 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsGroupObjEntryExpected_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsGroupObjEntryExpected_test_003 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::GROUP_OBJ, 6, ACL_UNDEFINED_ID};
    bool res = mediaThumbnailAclTestTask->IsGroupObjEntryExpected(entry, false);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("IsGroupObjEntryExpected_test_003 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsGroupObjEntryExpected_test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsGroupObjEntryExpected_test_004 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::GROUP_OBJ, 5, ACL_UNDEFINED_ID};
    bool res = mediaThumbnailAclTestTask->IsGroupObjEntryExpected(entry, false);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("IsGroupObjEntryExpected_test_004 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsGroupObjEntryExpected_test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsGroupObjEntryExpected_test_005 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::GROUP, 7, 1000};
    bool res = mediaThumbnailAclTestTask->IsGroupObjEntryExpected(entry, false);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("IsGroupObjEntryExpected_test_005 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsGroupEntryExpected_test_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsGroupEntryExpected_test_006 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::GROUP, 5, THUMB_ACL_GROUP};
    bool res = mediaThumbnailAclTestTask->IsGroupEntryExpected(entry, false);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsGroupEntryExpected_test_006 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsGroupEntryExpected_test_007, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsGroupEntryExpected_test_007 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::GROUP, 4, ACL_UNDEFINED_ID};
    bool res = mediaThumbnailAclTestTask->IsGroupEntryExpected(entry, false);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("IsGroupEntryExpected_test_007 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsGroupObjEntryExpected_test_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsGroupObjEntryExpected_test_006 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::GROUP_OBJ, 7, 2008};
    bool res = mediaThumbnailAclTestTask->IsGroupObjEntryExpected(entry, false);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("IsGroupObjEntryExpected_test_006 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsGroupEntryExpected_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsGroupEntryExpected_test_001 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::GROUP, 5, THUMB_ACL_GROUP};
    bool res = mediaThumbnailAclTestTask->IsGroupEntryExpected(entry, false);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsGroupEntryExpected_test_001 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsGroupEntryExpected_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsGroupEntryExpected_test_002 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::GROUP, 5, MEDIA_DB_ACL_GROUP};
    bool res = mediaThumbnailAclTestTask->IsGroupEntryExpected(entry, false);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsGroupEntryExpected_test_002 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsGroupEntryExpected_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsGroupEntryExpected_test_003 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::GROUP, 5, THUMB_ACL_GROUP};
    bool res = mediaThumbnailAclTestTask->IsGroupEntryExpected(entry, true);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsGroupEntryExpected_test_003 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsGroupEntryExpected_test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsGroupEntryExpected_test_004 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::GROUP, 5, MEDIA_DB_ACL_GROUP};
    bool res = mediaThumbnailAclTestTask->IsGroupEntryExpected(entry, true);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsGroupEntryExpected_test_004 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsGroupEntryExpected_test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsGroupEntryExpected_test_005 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::GROUP, 7, THUMB_ACL_GROUP};
    bool res = mediaThumbnailAclTestTask->IsGroupEntryExpected(entry, false);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("IsGroupEntryExpected_test_005 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsGroupEntryExpected_test_008, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsGroupEntryExpected_test_008 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::GROUP, 5, 3000};
    bool res = mediaThumbnailAclTestTask->IsGroupEntryExpected(entry, false);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("IsGroupEntryExpected_test_008 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsOtherEntryExpected_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsOtherEntryExpected_test_001 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::OTHER, 1, ACL_UNDEFINED_ID};
    bool res = mediaThumbnailAclTestTask->IsOtherEntryExpected(entry, false);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsOtherEntryExpected_test_001 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsOtherEntryExpected_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsOtherEntryExpected_test_002 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::OTHER, 1, ACL_UNDEFINED_ID};
    bool res = mediaThumbnailAclTestTask->IsOtherEntryExpected(entry, true);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsOtherEntryExpected_test_002 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsOtherEntryExpected_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsOtherEntryExpected_test_003 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::OTHER, 0, ACL_UNDEFINED_ID};
    bool res = mediaThumbnailAclTestTask->IsOtherEntryExpected(entry, true);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsOtherEntryExpected_test_003 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsOtherEntryExpected_test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsOtherEntryExpected_test_004 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::OTHER, 0, ACL_UNDEFINED_ID};
    bool res = mediaThumbnailAclTestTask->IsOtherEntryExpected(entry, false);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("IsOtherEntryExpected_test_004 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsOtherEntryExpected_test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsOtherEntryExpected_test_005 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::OTHER, 7, ACL_UNDEFINED_ID};
    bool res = mediaThumbnailAclTestTask->IsOtherEntryExpected(entry, false);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("IsOtherEntryExpected_test_005 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsOtherEntryExpected_test_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsOtherEntryExpected_test_006 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::OTHER, 1, 1000};
    bool res = mediaThumbnailAclTestTask->IsOtherEntryExpected(entry, false);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("IsOtherEntryExpected_test_006 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsOtherEntryExpected_test_007, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsOtherEntryExpected_test_007 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::OTHER, 0, 2008};
    bool res = mediaThumbnailAclTestTask->IsOtherEntryExpected(entry, true);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("IsOtherEntryExpected_test_007 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsOtherEntryExpected_test_008, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsOtherEntryExpected_test_008 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::OTHER, 5, ACL_UNDEFINED_ID};
    bool res = mediaThumbnailAclTestTask->IsOtherEntryExpected(entry, true);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("IsOtherEntryExpected_test_008 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsXattrResultsExpected_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsXattrResultsExpected_test_001 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::vector<XattrResult> xattrResults;
    XattrResult result;
    result.isSuccess = false;
    xattrResults.push_back(result);
    XattrResult unexpectedXattr;
    bool res = mediaThumbnailAclTestTask->IsXattrResultsExpected(xattrResults, unexpectedXattr, false);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsXattrResultsExpected_test_001 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsXattrResultsExpected_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsXattrResultsExpected_test_002 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::vector<XattrResult> xattrResults;
    XattrResult result;
    result.isSuccess = false;
    xattrResults.push_back(result);
    XattrResult unexpectedXattr;
    bool res = mediaThumbnailAclTestTask->IsXattrResultsExpected(xattrResults, unexpectedXattr, true);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsXattrResultsExpected_test_002 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsAllXattrExpected_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsAllXattrExpected_test_001 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::vector<XattrResult> dirDefaultAcls;
    std::vector<XattrResult> dirAccessAcls;
    std::vector<XattrResult> fileAccessAcls;
    XattrResult result;
    result.isSuccess = false;
    dirDefaultAcls.push_back(result);
    dirAccessAcls.push_back(result);
    fileAccessAcls.push_back(result);
    XattrResult unexpectedXattr;
    bool res = mediaThumbnailAclTestTask->IsAllXattrExpected(dirDefaultAcls, dirAccessAcls,
        fileAccessAcls, unexpectedXattr);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsAllXattrExpected_test_001 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsAllXattrExpected_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsAllXattrExpected_test_002 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::vector<XattrResult> dirDefaultAcls;
    std::vector<XattrResult> dirAccessAcls;
    std::vector<XattrResult> fileAccessAcls;
    XattrResult result;
    result.isSuccess = false;
    dirDefaultAcls.push_back(result);
    dirAccessAcls.push_back(result);
    dirAccessAcls.push_back(result);
    fileAccessAcls.push_back(result);
    XattrResult unexpectedXattr;
    bool res = mediaThumbnailAclTestTask->IsAllXattrExpected(dirDefaultAcls, dirAccessAcls,
        fileAccessAcls, unexpectedXattr);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsAllXattrExpected_test_002 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsAllXattrExpected_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsAllXattrExpected_test_003 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::vector<XattrResult> dirDefaultAcls;
    std::vector<XattrResult> dirAccessAcls;
    std::vector<XattrResult> fileAccessAcls;
    XattrResult result;
    result.isSuccess = false;
    dirDefaultAcls.push_back(result);
    dirDefaultAcls.push_back(result);
    dirAccessAcls.push_back(result);
    fileAccessAcls.push_back(result);
    XattrResult unexpectedXattr;
    bool res = mediaThumbnailAclTestTask->IsAllXattrExpected(dirDefaultAcls, dirAccessAcls,
        fileAccessAcls, unexpectedXattr);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsAllXattrExpected_test_003 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsAllXattrExpected_test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsAllXattrExpected_test_004 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::vector<XattrResult> dirDefaultAcls;
    std::vector<XattrResult> dirAccessAcls;
    std::vector<XattrResult> fileAccessAcls;
    XattrResult result;
    result.isSuccess = false;
    dirDefaultAcls.push_back(result);
    dirAccessAcls.push_back(result);
    dirAccessAcls.push_back(result);
    fileAccessAcls.push_back(result);
    XattrResult unexpectedXattr;
    bool res = mediaThumbnailAclTestTask->IsAllXattrExpected(dirDefaultAcls, dirAccessAcls,
        fileAccessAcls, unexpectedXattr);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsAllXattrExpected_test_004 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsAllXattrExpected_test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsAllXattrExpected_test_005 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::vector<XattrResult> dirDefaultAcls;
    std::vector<XattrResult> dirAccessAcls;
    std::vector<XattrResult> fileAccessAcls;
    XattrResult result;
    result.isSuccess = false;
    dirDefaultAcls.push_back(result);
    dirAccessAcls.push_back(result);
    fileAccessAcls.push_back(result);
    fileAccessAcls.push_back(result);
    XattrResult unexpectedXattr;
    bool res = mediaThumbnailAclTestTask->IsAllXattrExpected(dirDefaultAcls, dirAccessAcls,
        fileAccessAcls, unexpectedXattr);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsAllXattrExpected_test_005 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, GetLocalThumbnailPath_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetLocalThumbnailPath_test_001 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::string path = "/storage/cloud/files/Photo/test.jpg";
    std::string result = mediaThumbnailAclTestTask->GetLocalThumbnailPath(path);
    EXPECT_EQ(result, "/storage/media/local/files/.thumbs/Photo/test.jpg");
    MEDIA_INFO_LOG("GetLocalThumbnailPath_test_001 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, GetLocalThumbnailPath_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetLocalThumbnailPath_test_002 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::string path = "/storage/cloud/files/Photo/subdir/test.jpg";
    std::string result = mediaThumbnailAclTestTask->GetLocalThumbnailPath(path);
    EXPECT_EQ(result, "/storage/media/local/files/.thumbs/Photo/subdir/test.jpg");
    MEDIA_INFO_LOG("GetLocalThumbnailPath_test_002 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, GetLocalThumbnailPath_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetLocalThumbnailPath_test_003 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::string path = "/storage/cloud/files/Photo/2024/01/01/test.jpg";
    std::string result = mediaThumbnailAclTestTask->GetLocalThumbnailPath(path);
    EXPECT_EQ(result, "/storage/media/local/files/.thumbs/Photo/2024/01/01/test.jpg");
    MEDIA_INFO_LOG("GetLocalThumbnailPath_test_003 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, GetLocalThumbnailPath_test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetLocalThumbnailPath_test_004 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::string path = "/storage/media/local/files/Photo/test.jpg";
    std::string result = mediaThumbnailAclTestTask->GetLocalThumbnailPath(path);
    EXPECT_EQ(result, "");
    MEDIA_INFO_LOG("GetLocalThumbnailPath_test_004 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, GetLocalThumbnailPath_test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetLocalThumbnailPath_test_005 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::string path = "";
    std::string result = mediaThumbnailAclTestTask->GetLocalThumbnailPath(path);
    EXPECT_EQ(result, "");
    MEDIA_INFO_LOG("GetLocalThumbnailPath_test_005 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, GetLocalThumbnailPath_test_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetLocalThumbnailPath_test_006 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::string path = "/storage/cloud/files/Photo";
    std::string result = mediaThumbnailAclTestTask->GetLocalThumbnailPath(path);
    EXPECT_EQ(result, "/storage/media/local/files/.thumbs/Photo");
    MEDIA_INFO_LOG("GetLocalThumbnailPath_test_006 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, GetLocalThumbnailPath_test_007, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetLocalThumbnailPath_test_007 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::string path = "/storage/cloud/files/Photo/";
    std::string result = mediaThumbnailAclTestTask->GetLocalThumbnailPath(path);
    EXPECT_EQ(result, "/storage/media/local/files/.thumbs/Photo/");
    MEDIA_INFO_LOG("GetLocalThumbnailPath_test_007 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, GetLocalThumbnailPath_test_008, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetLocalThumbnailPath_test_008 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::string path = "/storage/cloud/files/Photo/test.png";
    std::string result = mediaThumbnailAclTestTask->GetLocalThumbnailPath(path);
    EXPECT_EQ(result, "/storage/media/local/files/.thumbs/Photo/test.png");
    MEDIA_INFO_LOG("GetLocalThumbnailPath_test_008 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, Accept_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Accept_test_001 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    bool res = mediaThumbnailAclTestTask->Accept();
    MEDIA_INFO_LOG("Accept_test_001 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, Execute_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Execute_test_001 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    mediaThumbnailAclTestTask->Execute();
    MEDIA_INFO_LOG("Execute_test_001 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, Execute_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Execute_test_002 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    mediaThumbnailAclTestTask->Execute();
    mediaThumbnailAclTestTask->Execute();
    MEDIA_INFO_LOG("Execute_test_002 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsEntriesExpected_test_010, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsEntriesExpected_test_010 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry1 = {ACL_TAG::USER_OBJ, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry2 = {ACL_TAG::GROUP_OBJ, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry3 = {ACL_TAG::GROUP, 5, 2008};
    AclXattrEntry entry4 = {ACL_TAG::MASK, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry5 = {ACL_TAG::OTHER, 1, ACL_UNDEFINED_ID};
    std::vector<AclXattrEntry> aclEntries = {entry1, entry2, entry3, entry4, entry5};
    bool res = mediaThumbnailAclTestTask->IsEntriesExpected(aclEntries, true);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsEntriesExpected_test_010 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsEntriesExpected_test_011, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsEntriesExpected_test_011 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry1 = {ACL_TAG::USER_OBJ, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry2 = {ACL_TAG::GROUP_OBJ, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry3 = {ACL_TAG::GROUP, 5, 2008};
    AclXattrEntry entry4 = {ACL_TAG::MASK, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry5 = {ACL_TAG::OTHER, 0, ACL_UNDEFINED_ID};
    std::vector<AclXattrEntry> aclEntries = {entry1, entry2, entry3, entry4, entry5};
    bool res = mediaThumbnailAclTestTask->IsEntriesExpected(aclEntries, true);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsEntriesExpected_test_011 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsEntriesExpected_test_012, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsEntriesExpected_test_012 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry1 = {ACL_TAG::USER_OBJ, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry2 = {ACL_TAG::GROUP_OBJ, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry3 = {ACL_TAG::GROUP, 5, 3008};
    AclXattrEntry entry4 = {ACL_TAG::MASK, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry5 = {ACL_TAG::OTHER, 0, ACL_UNDEFINED_ID};
    std::vector<AclXattrEntry> aclEntries = {entry1, entry2, entry3, entry4, entry5};
    bool res = mediaThumbnailAclTestTask->IsEntriesExpected(aclEntries, true);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsEntriesExpected_test_012 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsEntriesExpected_test_013, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsEntriesExpected_test_013 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry1 = {ACL_TAG::USER_OBJ, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry2 = {ACL_TAG::GROUP_OBJ, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry3 = {ACL_TAG::GROUP, 5, 2008};
    AclXattrEntry entry4 = {ACL_TAG::MASK, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry5 = {ACL_TAG::OTHER, 0, ACL_UNDEFINED_ID};
    std::vector<AclXattrEntry> aclEntries = {entry1, entry2, entry3, entry4, entry5};
    bool res = mediaThumbnailAclTestTask->IsEntriesExpected(aclEntries, false);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("IsEntriesExpected_test_013 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsXattrResultsExpected_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsXattrResultsExpected_test_003 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::vector<XattrResult> xattrResults;
    XattrResult result;
    result.isSuccess = true;
    result.xattrValue = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    xattrResults.push_back(result);
    XattrResult unexpectedXattr;
    bool res = mediaThumbnailAclTestTask->IsXattrResultsExpected(xattrResults, unexpectedXattr, false);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("IsXattrResultsExpected_test_003 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsXattrResultsExpected_test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsXattrResultsExpected_test_004 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::vector<XattrResult> xattrResults;
    XattrResult result;
    result.isSuccess = true;
    result.xattrValue = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    xattrResults.push_back(result);
    XattrResult unexpectedXattr;
    bool res = mediaThumbnailAclTestTask->IsXattrResultsExpected(xattrResults, unexpectedXattr, true);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("IsXattrResultsExpected_test_004 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsXattrResultsExpected_test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsXattrResultsExpected_test_005 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::vector<XattrResult> xattrResults;
    XattrResult result1;
    result1.isSuccess = false;
    xattrResults.push_back(result1);
    XattrResult result2;
    result2.isSuccess = true;
    result2.xattrValue = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    xattrResults.push_back(result2);
    XattrResult unexpectedXattr;
    bool res = mediaThumbnailAclTestTask->IsXattrResultsExpected(xattrResults, unexpectedXattr, false);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("IsXattrResultsExpected_test_005 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsXattrResultsExpected_test_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsXattrResultsExpected_test_006 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::vector<XattrResult> xattrResults;
    XattrResult result1;
    result1.isSuccess = false;
    xattrResults.push_back(result1);
    XattrResult result2;
    result2.isSuccess = false;
    xattrResults.push_back(result2);
    XattrResult unexpectedXattr;
    bool res = mediaThumbnailAclTestTask->IsXattrResultsExpected(xattrResults, unexpectedXattr, false);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsXattrResultsExpected_test_006 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsAllXattrExpected_test_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsAllXattrExpected_test_006 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::vector<XattrResult> dirDefaultAcls;
    std::vector<XattrResult> dirAccessAcls;
    std::vector<XattrResult> fileAccessAcls;
    XattrResult result;
    result.isSuccess = true;
    result.xattrValue = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    dirDefaultAcls.push_back(result);
    dirAccessAcls.push_back(result);
    fileAccessAcls.push_back(result);
    XattrResult unexpectedXattr;
    bool res = mediaThumbnailAclTestTask->IsAllXattrExpected(dirDefaultAcls, dirAccessAcls,
        fileAccessAcls, unexpectedXattr);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("IsAllXattrExpected_test_006 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsAllXattrExpected_test_007, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsAllXattrExpected_test_007 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::vector<XattrResult> dirDefaultAcls;
    std::vector<XattrResult> dirAccessAcls;
    std::vector<XattrResult> fileAccessAcls;
    XattrResult result;
    result.isSuccess = true;
    result.xattrValue = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    dirDefaultAcls.push_back(result);
    result.isSuccess = false;
    dirAccessAcls.push_back(result);
    fileAccessAcls.push_back(result);
    XattrResult unexpectedXattr;
    bool res = mediaThumbnailAclTestTask->IsAllXattrExpected(dirDefaultAcls, dirAccessAcls,
        fileAccessAcls, unexpectedXattr);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("IsAllXattrExpected_test_007 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsAllXattrExpected_test_008, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsAllXattrExpected_test_008 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::vector<XattrResult> dirDefaultAcls;
    std::vector<XattrResult> dirAccessAcls;
    std::vector<XattrResult> fileAccessAcls;
    XattrResult result;
    result.isSuccess = false;
    dirDefaultAcls.push_back(result);
    result.isSuccess = true;
    result.xattrValue = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    dirAccessAcls.push_back(result);
    fileAccessAcls.push_back(result);
    XattrResult unexpectedXattr;
    bool res = mediaThumbnailAclTestTask->IsAllXattrExpected(dirDefaultAcls, dirAccessAcls,
        fileAccessAcls, unexpectedXattr);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("IsAllXattrExpected_test_008 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsAllXattrExpected_test_009, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsAllXattrExpected_test_009 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::vector<XattrResult> dirDefaultAcls;
    std::vector<XattrResult> dirAccessAcls;
    std::vector<XattrResult> fileAccessAcls;
    XattrResult result;
    result.isSuccess = false;
    dirDefaultAcls.push_back(result);
    dirAccessAcls.push_back(result);
    result.isSuccess = true;
    result.xattrValue = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    fileAccessAcls.push_back(result);
    XattrResult unexpectedXattr;
    bool res = mediaThumbnailAclTestTask->IsAllXattrExpected(dirDefaultAcls, dirAccessAcls,
        fileAccessAcls, unexpectedXattr);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("IsAllXattrExpected_test_009 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, HandleThumbnailAcl_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("HandleThumbnailAcl_test_001 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    mediaThumbnailAclTestTask->HandleThumbnailAcl();
    MEDIA_INFO_LOG("HandleThumbnailAcl_test_001 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, HandleThumbnailAcl_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("HandleThumbnailAcl_test_002 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    mediaThumbnailAclTestTask->HandleThumbnailAcl();
    mediaThumbnailAclTestTask->HandleThumbnailAcl();
    MEDIA_INFO_LOG("HandleThumbnailAcl_test_002 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, StartThumbnailAclRemoveTask_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("StartThumbnailAclRemoveTask_test_001 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    mediaThumbnailAclTestTask->StartThumbnailAclRemoveTask();
    MEDIA_INFO_LOG("StartThumbnailAclRemoveTask_test_001 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, StartThumbnailAclRemoveTask_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("StartThumbnailAclRemoveTask_test_002 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    mediaThumbnailAclTestTask->StartThumbnailAclRemoveTask();
    mediaThumbnailAclTestTask->StartThumbnailAclRemoveTask();
    MEDIA_INFO_LOG("StartThumbnailAclRemoveTask_test_002 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, StartRemoveThumbnailDirAcl_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("StartRemoveThumbnailDirAcl_test_001 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    XattrResult thumbnailDirXattrResult;
    thumbnailDirXattrResult.isSuccess = true;
    thumbnailDirXattrResult.xattrValue = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    mediaThumbnailAclTestTask->StartRemoveThumbnailDirAcl(thumbnailDirXattrResult);
    MEDIA_INFO_LOG("StartRemoveThumbnailDirAcl_test_001 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, StartRemoveThumbnailDirAcl_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("StartRemoveThumbnailDirAcl_test_002 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    XattrResult thumbnailDirXattrResult;
    thumbnailDirXattrResult.isSuccess = false;
    mediaThumbnailAclTestTask->StartRemoveThumbnailDirAcl(thumbnailDirXattrResult);
    MEDIA_INFO_LOG("StartRemoveThumbnailDirAcl_test_002 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, GetThumbnailDirDefaultAcl_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetThumbnailDirDefaultAcl_test_001 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    XattrResult xattrResult;
    bool res = mediaThumbnailAclTestTask->GetThumbnailDirDefaultAcl(xattrResult);
    MEDIA_INFO_LOG("GetThumbnailDirDefaultAcl_test_001 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, GetThumbnailDirDefaultAcl_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetThumbnailDirDefaultAcl_test_002 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    XattrResult xattrResult;
    xattrResult.isSuccess = true;
    bool res = mediaThumbnailAclTestTask->GetThumbnailDirDefaultAcl(xattrResult);
    MEDIA_INFO_LOG("GetThumbnailDirDefaultAcl_test_002 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, GetNeedCheckAclPathList_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetNeedCheckAclPathList_test_001 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::vector<std::string> needCheckAclDirList;
    std::vector<std::string> needCheckAclFileList;
    bool res = mediaThumbnailAclTestTask->GetNeedCheckAclPathList(needCheckAclDirList,
        needCheckAclFileList);
    MEDIA_INFO_LOG("GetNeedCheckAclPathList_test_001 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, ParseNeedCheckThumbnailPathWithFilePath_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("ParseNeedCheckThumbnailPathWithFilePath_test_003 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::string path = "/storage/media/local/files/Photo/test.jpg";
    std::vector<std::string> needCheckAclDirList;
    std::vector<std::string> needCheckAclFileList;
    bool res = mediaThumbnailAclTestTask->ParseNeedCheckThumbnailPathWithFilePath(path,
        needCheckAclDirList, needCheckAclFileList);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("ParseNeedCheckThumbnailPathWithFilePath_test_003 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, ParseNeedCheckThumbnailPathWithFilePath_test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("ParseNeedCheckThumbnailPathWithFilePath_test_004 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::string path = "";
    std::vector<std::string> needCheckAclDirList;
    std::vector<std::string> needCheckAclFileList;
    bool res = mediaThumbnailAclTestTask->ParseNeedCheckThumbnailPathWithFilePath(path,
        needCheckAclDirList, needCheckAclFileList);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("ParseNeedCheckThumbnailPathWithFilePath_test_004 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, ContinueThumbnailAclRemoveTask_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("ContinueThumbnailAclRemoveTask_test_001 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    mediaThumbnailAclTestTask->ContinueThumbnailAclRemoveTask();
    MEDIA_INFO_LOG("ContinueThumbnailAclRemoveTask_test_001 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, RemoveThumbPhotoDirAndBucketdirAcl_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("RemoveThumbPhotoDirAndBucketdirAcl_test_001 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    int32_t res = mediaThumbnailAclTestTask->RemoveThumbPhotoDirAndBucketdirAcl();
    MEDIA_INFO_LOG("RemoveThumbPhotoDirAndBucketdirAcl_test_001 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, ContinueRemoveThumbnailAclWithFileId_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("ContinueRemoveThumbnailAclWithFileId_test_001 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::string xattrInfo = "test_xattr_info";
    int32_t fileId = 0;
    mediaThumbnailAclTestTask->ContinueRemoveThumbnailAclWithFileId(xattrInfo, fileId);
    MEDIA_INFO_LOG("ContinueRemoveThumbnailAclWithFileId_test_001 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, ContinueRemoveThumbnailAclWithFileId_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("ContinueRemoveThumbnailAclWithFileId_test_002 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::string xattrInfo = "test_xattr_info";
    int32_t fileId = 100;
    mediaThumbnailAclTestTask->ContinueRemoveThumbnailAclWithFileId(xattrInfo, fileId);
    MEDIA_INFO_LOG("ContinueRemoveThumbnailAclWithFileId_test_002 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, ContinueRemoveThumbnailAclWithFileId_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("ContinueRemoveThumbnailAclWithFileId_test_003 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::string xattrInfo = "";
    int32_t fileId = -1;
    mediaThumbnailAclTestTask->ContinueRemoveThumbnailAclWithFileId(xattrInfo, fileId);
    MEDIA_INFO_LOG("ContinueRemoveThumbnailAclWithFileId_test_003 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, RemoveThumbnailDirAndFileAcl_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("RemoveThumbnailDirAndFileAcl_test_001 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::string path = "/storage/cloud/files/Photo/test.jpg";
    int32_t res = mediaThumbnailAclTestTask->RemoveThumbnailDirAndFileAcl(path);
    MEDIA_INFO_LOG("RemoveThumbnailDirAndFileAcl_test_001 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, RemoveThumbnailDirAndFileAcl_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("RemoveThumbnailDirAndFileAcl_test_002 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::string path = "/storage/cloud/files/Photo/subdir/test.jpg";
    int32_t res = mediaThumbnailAclTestTask->RemoveThumbnailDirAndFileAcl(path);
    MEDIA_INFO_LOG("RemoveThumbnailDirAndFileAcl_test_002 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, RemoveThumbnailDirAndFileAcl_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("RemoveThumbnailDirAndFileAcl_test_003 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::string path = "/storage/media/local/files/Photo/test.jpg";
    int32_t res = mediaThumbnailAclTestTask->RemoveThumbnailDirAndFileAcl(path);
    EXPECT_EQ(res, E_ERR);
    MEDIA_INFO_LOG("RemoveThumbnailDirAndFileAcl_test_003 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, RemoveThumbnailDirAndFileAcl_test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("RemoveThumbnailDirAndFileAcl_test_004 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::string path = "";
    int32_t res = mediaThumbnailAclTestTask->RemoveThumbnailDirAndFileAcl(path);
    EXPECT_EQ(res, E_ERR);
    MEDIA_INFO_LOG("RemoveThumbnailDirAndFileAcl_test_004 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, GetThumbnailAclTaskPhotoInfos_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetThumbnailAclTaskPhotoInfos_test_001 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    std::vector<ThumbnailAclTaskPhotoInfo> infos;
    bool res = mediaThumbnailAclTestTask->GetThumbnailAclTaskPhotoInfos(rdbPredicates, infos);
    MEDIA_INFO_LOG("GetThumbnailAclTaskPhotoInfos_test_001 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, GetThumbnailAclTaskPhotoInfos_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetThumbnailAclTaskPhotoInfos_test_002 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    rdbPredicates.GreaterThan(MediaColumn::MEDIA_ID, 0);
    rdbPredicates.Limit(10);
    std::vector<ThumbnailAclTaskPhotoInfo> infos;
    bool res = mediaThumbnailAclTestTask->GetThumbnailAclTaskPhotoInfos(rdbPredicates, infos);
    MEDIA_INFO_LOG("GetThumbnailAclTaskPhotoInfos_test_002 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, ReportDfxAndFlushRecordEvent_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("ReportDfxAndFlushRecordEvent_test_001 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    int32_t result = 1;
    int32_t isConfigXattr = 1;
    std::string xattrInfo = "test_xattr_info";
    int32_t recordResult = 3;
    int32_t res = mediaThumbnailAclTestTask->ReportDfxAndFlushRecordEvent(result, isConfigXattr,
        xattrInfo, recordResult);
    MEDIA_INFO_LOG("ReportDfxAndFlushRecordEvent_test_001 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, ReportDfxAndFlushRecordEvent_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("ReportDfxAndFlushRecordEvent_test_002 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    int32_t result = 2;
    int32_t isConfigXattr = 0;
    std::string xattrInfo = "";
    int32_t recordResult = 4;
    int32_t res = mediaThumbnailAclTestTask->ReportDfxAndFlushRecordEvent(result, isConfigXattr,
        xattrInfo, recordResult);
    MEDIA_INFO_LOG("ReportDfxAndFlushRecordEvent_test_002 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, FlushReportDfxAndFlushRecordEvent_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("FlushProgressEvent_test_001 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::string xattrInfo = "test_xattr_info";
    int32_t fileId = 100;
    int32_t res = mediaThumbnailAclTestTask->FlushProgressEvent(xattrInfo, fileId);
    MEDIA_INFO_LOG("FlushProgressEvent_test_001 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, FlushProgressEvent_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("FlushProgressEvent_test_002 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::string xattrInfo = "";
    int32_t fileId = -1;
    int32_t res = mediaThumbnailAclTestTask->FlushProgressEvent(xattrInfo, fileId);
    MEDIA_INFO_LOG("FlushProgressEvent_test_002 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, FlushProgressEvent_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("FlushProgressEvent_test_003 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::string xattrInfo = "test_xattr_info";
    int32_t fileId = 0;
    int32_t res = mediaThumbnailAclTestTask->FlushProgressEvent(xattrInfo, fileId);
    MEDIA_INFO_LOG("FlushProgressEvent_test_003 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, FlushProgressEvent_test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("FlushProgressEvent_test_004 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::string xattrInfo = "xattr_data_12345";
    int32_t fileId = 999;
    int32_t res = mediaThumbnailAclTestTask->FlushProgressEvent(xattrInfo, fileId);
    MEDIA_INFO_LOG("FlushProgressEvent_test_004 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsEntriesExpected_test_014, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsEntriesExpected_test_014 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry1 = {ACL_TAG::USER_OBJ, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry2 = {ACL_TAG::GROUP_OBJ, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry3 = {ACL_TAG::GROUP, 5, THUMB_ACL_GROUP};
    AclXattrEntry entry4 = {ACL_TAG::MASK, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry5 = {ACL_TAG::OTHER, 1, ACL_UNDEFINED_ID};
    std::vector<AclXattrEntry> aclEntries = {entry1, entry2, entry3, entry4, entry5};
    bool res = mediaThumbnailAclTestTask->IsEntriesExpected(aclEntries, false);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsEntriesExpected_test_014 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsEntriesExpected_test_015, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsEntriesExpected_test_015 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry1 = {ACL_TAG::USER_OBJ, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry2 = {ACL_TAG::GROUP_OBJ, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry3 = {ACL_TAG::GROUP, 5, MEDIA_DB_ACL_GROUP};
    AclXattrEntry entry4 = {ACL_TAG::MASK, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry5 = {ACL_TAG::OTHER, 1, ACL_UNDEFINED_ID};
    std::vector<AclXattrEntry> aclEntries = {entry1, entry2, entry3, entry4, entry5};
    bool res = mediaThumbnailAclTestTask->IsEntriesExpected(aclEntries, false);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsEntriesExpected_test_015 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsEntriesExpected_test_016, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsEntriesExpected_test_016 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry1 = {ACL_TAG::USER_OBJ, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry2 = {ACL_TAG::GROUP_OBJ, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry3 = {ACL_TAG::GROUP, 5, THUMB_ACL_GROUP};
    AclXattrEntry entry4 = {ACL_TAG::MASK, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry5 = {ACL_TAG::OTHER, 1, ACL_UNDEFINED_ID};
    std::vector<AclXattrEntry> aclEntries = {entry1, entry2, entry3, entry4, entry5};
    bool res = mediaThumbnailAclTestTask->IsEntriesExpected(aclEntries, true);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsEntriesExpected_test_016 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsEntriesExpected_test_017, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsEntriesExpected_test_017 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry1 = {ACL_TAG::USER_OBJ, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry2 = {ACL_TAG::GROUP_OBJ, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry3 = {ACL_TAG::GROUP, 5, MEDIA_DB_ACL_GROUP};
    AclXattrEntry entry4 = {ACL_TAG::MASK, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry5 = {ACL_TAG::OTHER, 0, ACL_UNDEFINED_ID};
    std::vector<AclXattrEntry> aclEntries = {entry1, entry2, entry3, entry4, entry5};
    bool res = mediaThumbnailAclTestTask->IsEntriesExpected(aclEntries, true);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsEntriesExpected_test_017 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsGroupObjEntryExpected_test_007, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsGroupObjEntryExpected_test_007 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::GROUP_OBJ, 7, ACL_UNDEFINED_ID};
    bool res = mediaThumbnailAclTestTask->IsGroupObjEntryExpected(entry, false);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsGroupObjEntryExpected_test_007 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsGroupObjEntryExpected_test_008, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsGroupObjEntryExpected_test_008 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::GROUP_OBJ, 7, ACL_UNDEFINED_ID};
    bool res = mediaThumbnailAclTestTask->IsGroupObjEntryExpected(entry, true);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsGroupObjEntryExpected_test_008 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsGroupEntryExpected_test_009, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsGroupEntryExpected_test_009 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::GROUP, 5, THUMB_ACL_GROUP};
    bool res = mediaThumbnailAclTestTask->IsGroupEntryExpected(entry, false);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsGroupEntryExpected_test_009 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsXattrResultsExpected_test_007, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsXattrResultsExpected_test_007 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::vector<XattrResult> xattrResults;
    XattrResult result;
    result.isSuccess = false;
    xattrResults.push_back(result);
    XattrResult unexpectedXattr;
    bool res = mediaThumbnailAclTestTask->IsXattrResultsExpected(xattrResults, unexpectedXattr, false);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsXattrResultsExpected_test_007 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsAllXattrExpected_test_010, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsAllXattrExpected_test_010 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::vector<XattrResult> dirDefaultAcls;
    std::vector<XattrResult> dirAccessAcls;
    std::vector<XattrResult> fileAccessAcls;
    XattrResult result;
    result.isSuccess = false;
    dirDefaultAcls.push_back(result);
    dirAccessAcls.push_back(result);
    fileAccessAcls.push_back(result);
    XattrResult unexpectedXattr;
    bool res = mediaThumbnailAclTestTask->IsAllXattrExpected(dirDefaultAcls, dirAccessAcls,
        fileAccessAcls, unexpectedXattr);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsAllXattrExpected_test_010 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, GetLocalThumbnailPath_test_009, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetLocalThumbnailPath_test_009 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::string path = "/storage/cloud/files/Photo/2024/12/31/test.jpg";
    std::string result = mediaThumbnailAclTestTask->GetLocalThumbnailPath(path);
    EXPECT_EQ(result, "/storage/media/local/files/.thumbs/Photo/2024/12/31/test.jpg");
    MEDIA_INFO_LOG("GetLocalThumbnailPath_test_009 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, GetLocalThumbnailPath_test_010, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetLocalThumbnailPath_test_010 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::string path = "/storage/cloud/files/Photo/test.gif";
    std::string result = mediaThumbnailAclTestTask->GetLocalThumbnailPath(path);
    EXPECT_EQ(result, "/storage/media/local/files/.thumbs/Photo/test.gif");
    MEDIA_INFO_LOG("GetLocalThumbnailPath_test_010 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, ContinueRemoveThumbnailAclWithFileId_test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("ContinueRemoveThumbnailAclWithFileId_test_004 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::string xattrInfo = "xattr_test_001";
    int32_t fileId = 50;
    mediaThumbnailAclTestTask->ContinueRemoveThumbnailAclWithFileId(xattrInfo, fileId);
    MEDIA_INFO_LOG("ContinueRemoveThumbnailAclWithFileId_test_004 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, ContinueRemoveThumbnailAclWithFileId_test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("ContinueRemoveThumbnailAclWithFileId_test_005 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::string xattrInfo = "xattr_test_002";
    int32_t fileId = 200;
    mediaThumbnailAclTestTask->ContinueRemoveThumbnailAclWithFileId(xattrInfo, fileId);
    MEDIA_INFO_LOG("ContinueRemoveThumbnailAclWithFileId_test_005 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, RemoveThumbnailDirAndFileAcl_test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("RemoveThumbnailDirAndFileAcl_test_005 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::string path = "/storage/cloud/files/Photo/2024/test.jpg";
    int32_t res = mediaThumbnailAclTestTask->RemoveThumbnailDirAndFileAcl(path);
    MEDIA_INFO_LOG("RemoveThumbnailDirAndFileAcl_test_005 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, RemoveThumbnailDirAndFileAcl_test_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("RemoveThumbnailDirAndFileAcl_test_006 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::string path = "/storage/cloud/files/Photo/2024/01/test.jpg";
    int32_t res = mediaThumbnailAclTestTask->RemoveThumbnailDirAndFileAcl(path);
    MEDIA_INFO_LOG("RemoveThumbnailDirAndFileAcl_test_006 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, GetThumbnailAclTaskPhotoInfos_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetThumbnailAclTaskPhotoInfos_test_003 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    rdbPredicates.GreaterThan(MediaColumn::MEDIA_ID, 100);
    rdbPredicates.Limit(50);
    rdbPredicates.OrderByAsc(MediaColumn::MEDIA_ID);
    std::vector<ThumbnailAclTaskPhotoInfo> infos;
    bool res = mediaThumbnailAclTestTask->GetThumbnailAclTaskPhotoInfos(rdbPredicates, infos);
    MEDIA_INFO_LOG("GetThumbnailAclTaskPhotoInfos_test_003 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, ReportDfxAndFlushRecordEvent_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("ReportDfxAndFlushRecordEvent_test_003 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    int32_t result = 3;
    int32_t isConfigXattr = 1;
    std::string xattrInfo = "xattr_test_003";
    int32_t recordResult = 3;
    int32_t res = mediaThumbnailAclTestTask->ReportDfxAndFlushRecordEvent(result, isConfigXattr,
        xattrInfo, recordResult);
    MEDIA_INFO_LOG("ReportDfxAndFlushRecordEvent_test_003 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, ReportDfxAndFlushRecordEvent_test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("ReportDfxAndFlushRecordEvent_test_004 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    int32_t result = 1;
    int32_t isConfigXattr = 0;
    std::string xattrInfo = "xattr_test_004";
    int32_t recordResult = 4;
    int32_t res = mediaThumbnailAclTestTask->ReportDfxAndFlushRecordEvent(result, isConfigXattr,
        xattrInfo, recordResult);
    MEDIA_INFO_LOG("ReportDfxAndFlushRecordEvent_test_004 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, FlushProgressEvent_test_009, TestSize.Level0)
{
    MEDIA_INFO_LOG("FlushProgressEvent_test_009 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::string xattrInfo = "xattr_test_009";
    int32_t fileId = 880;
    int32_t res = mediaThumbnailAclTestTask->FlushProgressEvent(xattrInfo, fileId);
    MEDIA_INFO_LOG("FlushProgressEvent_test_009 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, FlushProgressEvent_test_010, TestSize.Level0)
{
    MEDIA_INFO_LOG("FlushProgressEvent_test_010 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::string xattrInfo = "xattr_test_010";
    int32_t fileId = 2000;
    int32_t res = mediaThumbnailAclTestTask->FlushProgressEvent(xattrInfo, fileId);
    MEDIA_INFO_LOG("FlushProgressEvent_test_010 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsEntriesExpected_test_018, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsEntriesExpected_test_018 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry1 = {ACL_TAG::USER_OBJ, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry2 = {ACL_TAG::GROUP_OBJ, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry3 = {ACL_TAG::GROUP, 5, THUMB_ACL_GROUP};
    AclXattrEntry entry4 = {ACL_TAG::MASK, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry5 = {ACL_TAG::OTHER, 1, ACL_UNDEFINED_ID};
    std::vector<AclXattrEntry> aclEntries = {entry1, entry2, entry3, entry4, entry5};
    bool res = mediaThumbnailAclTestTask->IsEntriesExpected(aclEntries, false);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsEntriesExpected_test_018 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsEntriesExpected_test_019, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsEntriesExpected_test_019 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry1 = {ACL_TAG::USER_OBJ, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry2 = {ACL_TAG::GROUP_OBJ, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry3 = {ACL_TAG::GROUP, 5, MEDIA_DB_ACL_GROUP};
    AclXattrEntry entry4 = {ACL_TAG::MASK, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry5 = {ACL_TAG::OTHER, 1, ACL_UNDEFINED_ID};
    std::vector<AclXattrEntry> aclEntries = {entry1, entry2, entry3, entry4, entry5};
    bool res = mediaThumbnailAclTestTask->IsEntriesExpected(aclEntries, false);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsEntriesExpected_test_019 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsEntriesExpected_test_020, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsEntriesExpected_test_020 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry1 = {ACL_TAG::USER_OBJ, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry2 = {ACL_TAG::GROUP_OBJ, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry3 = {ACL_TAG::GROUP, 5, THUMB_ACL_GROUP};
    AclXattrEntry entry4 = {ACL_TAG::MASK, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry5 = {ACL_TAG::OTHER, 0, ACL_UNDEFINED_ID};
    std::vector<AclXattrEntry> aclEntries = {entry1, entry2, entry3, entry4, entry5};
    bool res = mediaThumbnailAclTestTask->IsEntriesExpected(aclEntries, true);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsEntriesExpected_test_020 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsEntriesExpected_test_021, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsEntriesExpected_test_021 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry1 = {ACL_TAG::USER_OBJ, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry2 = {ACL_TAG::GROUP_OBJ, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry3 = {ACL_TAG::GROUP, 5, MEDIA_DB_ACL_GROUP};
    AclXattrEntry entry4 = {ACL_TAG::MASK, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry5 = {ACL_TAG::OTHER, 0, ACL_UNDEFINED_ID};
    std::vector<AclXattrEntry> aclEntries = {entry1, entry2, entry3, entry4, entry5};
    bool res = mediaThumbnailAclTestTask->IsEntriesExpected(aclEntries, true);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsEntriesExpected_test_021 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsGroupObjEntryExpected_test_011, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsGroupObjEntryExpected_test_011 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::GROUP_OBJ, 7, ACL_UNDEFINED_ID};
    bool res = mediaThumbnailAclTestTask->IsGroupObjEntryExpected(entry, false);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsGroupObjEntryExpected_test_011 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsGroupObjEntryExpected_test_012, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsGroupObjEntryExpected_test_012 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::GROUP_OBJ, 7, ACL_UNDEFINED_ID};
    bool res = mediaThumbnailAclTestTask->IsGroupObjEntryExpected(entry, true);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsGroupObjEntryExpected_test_012 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsGroupEntryExpected_test_011, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsGroupEntryExpected_test_011 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::GROUP, 5, THUMB_ACL_GROUP};
    bool res = mediaThumbnailAclTestTask->IsGroupEntryExpected(entry, false);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsGroupEntryExpected_test_011 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsGroupEntryExpected_test_012, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsGroupEntryExpected_test_012 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::GROUP, 5, MEDIA_DB_ACL_GROUP};
    bool res = mediaThumbnailAclTestTask->IsGroupEntryExpected(entry, false);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsGroupEntryExpected_test_012 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsOtherEntryExpected_test_017, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsOtherEntryExpected_test_017 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::OTHER, 1, ACL_UNDEFINED_ID};
    bool res = mediaThumbnailAclTestTask->IsOtherEntryExpected(entry, false);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsOtherEntryExpected_test_017 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsOtherEntryExpected_test_018, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsOtherEntryExpected_test_018 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::OTHER, 0, ACL_UNDEFINED_ID};
    bool res = mediaThumbnailAclTestTask->IsOtherEntryExpected(entry, true);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsOtherEntryExpected_test_018 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsXattrResultsExpected_test_011, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsXattrResultsExpected_test_011 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::vector<XattrResult> xattrResults;
    XattrResult result;
    result.isSuccess = false;
    xattrResults.push_back(result);
    XattrResult unexpectedXattr;
    bool res = mediaThumbnailAclTestTask->IsXattrResultsExpected(xattrResults, unexpectedXattr, false);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsXattrResultsExpected_test_011 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, GetLocalThumbnailPath_test_017, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetLocalThumbnailPath_test_017 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::string path = "/storage/cloud/files/Photo/2027/03/03/test.jpg";
    std::string result = mediaThumbnailAclTestTask->GetLocalThumbnailPath(path);
    EXPECT_EQ(result, "/storage/media/local/files/.thumbs/Photo/2027/03/03/test.jpg");
    MEDIA_INFO_LOG("GetLocalThumbnailPath_test_017 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, GetLocalThumbnailPath_test_018, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetLocalThumbnailPath_test_018 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::string path = "/storage/cloud/files/Photo/test.svg";
    std::string result = mediaThumbnailAclTestTask->GetLocalThumbnailPath(path);
    EXPECT_EQ(result, "/storage/media/local/files/.thumbs/Photo/test.svg");
    MEDIA_INFO_LOG("GetLocalThumbnailPath_test_018 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, ContinueRemoveThumbnailAclWithFileId_test_010, TestSize.Level0)
{
    MEDIA_INFO_LOG("ContinueRemoveThumbnailAclWithFileId_test_010 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::string xattrInfo = "xattr_test_007";
    int32_t fileId = 99;
    mediaThumbnailAclTestTask->ContinueRemoveThumbnailAclWithFileId(xattrInfo, fileId);
    MEDIA_INFO_LOG("ContinueRemoveThumbnailAclWithFileId_test_010 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, ContinueRemoveThumbnailAclWithFileId_test_011, TestSize.Level0)
{
    MEDIA_INFO_LOG("ContinueRemoveThumbnailAclWithFileId_test_011 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::string xattrInfo = "xattr_test_008";
    int32_t fileId = 350;
    mediaThumbnailAclTestTask->ContinueRemoveThumbnailAclWithFileId(xattrInfo, fileId);
    MEDIA_INFO_LOG("ContinueRemoveThumbnailAclWithFileId_test_011 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, RemoveThumbnailDirAndFileAcl_test_011, TestSize.Level0)
{
    MEDIA_INFO_LOG("RemoveThumbnailDirAndFileAcl_test_011 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::string path = "/storage/cloud/files/Photo/2027/test.jpg";
    int32_t res = mediaThumbnailAclTestTask->RemoveThumbnailDirAndFileAcl(path);
    MEDIA_INFO_LOG("RemoveThumbnailDirAndFileAcl_test_011 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, RemoveThumbnailDirAndFileAcl_test_012, TestSize.Level0)
{
    MEDIA_INFO_LOG("RemoveThumbnailDirAndFileAcl_test_012 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::string path = "/storage/cloud/files/Photo/2027/04/test.jpg";
    int32_t res = mediaThumbnailAclTestTask->RemoveThumbnailDirAndFileAcl(path);
    MEDIA_INFO_LOG("RemoveThumbnailDirAndFileAcl_test_012 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, GetThumbnailAclTaskPhotoInfos_test_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetThumbnailAclTaskPhotoInfos_test_006 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    rdbPredicates.GreaterThan(MediaColumn::MEDIA_ID, 400);
    rdbPredicates.Limit(300);
    rdbPredicates.OrderByAsc(MediaColumn::MEDIA_ID);
    std::vector<ThumbnailAclTaskPhotoInfo> infos;
    bool res = mediaThumbnailAclTestTask->GetThumbnailAclTaskPhotoInfos(rdbPredicates, infos);
    MEDIA_INFO_LOG("GetThumbnailAclTaskPhotoInfos_test_006 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, ReportDfxAndFlushRecordEvent_test_009, TestSize.Level0)
{
    MEDIA_INFO_LOG("ReportDfxAndFlushRecordEvent_test_009 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    int32_t result = 1;
    int32_t isConfigXattr = 1;
    std::string xattrInfo = "xattr_test_009";
    int32_t recordResult = 3;
    int32_t res = mediaThumbnailAclTestTask->ReportDfxAndFlushRecordEvent(result, isConfigXattr,
        xattrInfo, recordResult);
    MEDIA_INFO_LOG("ReportDfxAndFlushRecordEvent_test_009 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, ReportDfxAndFlushRecordEvent_test_010, TestSize.Level0)
{
    MEDIA_INFO_LOG("ReportDfxAndFlushRecordEvent_test_010 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    int32_t result = 3;
    int32_t isConfigXattr = 1;
    std::string xattrInfo = "xattr_test_010";
    int32_t recordResult = 4;
    int32_t res = mediaThumbnailAclTestTask->ReportDfxAndFlushRecordEvent(result, isConfigXattr,
        xattrInfo, recordResult);
    MEDIA_INFO_LOG("ReportDfxAndFlushRecordEvent_test_010 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, FlushProgressEvent_test_011, TestSize.Level0)
{
    MEDIA_INFO_LOG("FlushProgressEvent_test_011 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::string xattrInfo = "xattr_test_011";
    int32_t fileId = 990;
    int32_t res = mediaThumbnailAclTestTask->FlushProgressEvent(xattrInfo, fileId);
    MEDIA_INFO_LOG("FlushProgressEvent_test_011 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, FlushProgressEvent_test_012, TestSize.Level0)
{
    MEDIA_INFO_LOG("FlushProgressEvent_test_012 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::string xattrInfo = "xattr_test_012";
    int32_t fileId = 2500;
    int32_t res = mediaThumbnailAclTestTask->FlushProgressEvent(xattrInfo, fileId);
    MEDIA_INFO_LOG("FlushProgressEvent_test_012 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsEntriesExpected_test_031, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsEntriesExpected_test_031 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry1 = {ACL_TAG::USER_OBJ, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry2 = {ACL_TAG::GROUP_OBJ, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry3 = {ACL_TAG::GROUP, 5, THUMB_ACL_GROUP};
    AclXattrEntry entry4 = {ACL_TAG::MASK, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry5 = {ACL_TAG::OTHER, 1, ACL_UNDEFINED_ID};
    std::vector<AclXattrEntry> aclEntries = {entry1, entry2, entry3, entry4, entry5};
    bool res = mediaThumbnailAclTestTask->IsEntriesExpected(aclEntries, false);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsEntriesExpected_test_031 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsEntriesExpected_test_032, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsEntriesExpected_test_032 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry1 = {ACL_TAG::USER_OBJ, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry2 = {ACL_TAG::GROUP_OBJ, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry3 = {ACL_TAG::GROUP, 5, MEDIA_DB_ACL_GROUP};
    AclXattrEntry entry4 = {ACL_TAG::MASK, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry5 = {ACL_TAG::OTHER, 0, ACL_UNDEFINED_ID};
    std::vector<AclXattrEntry> aclEntries = {entry1, entry2, entry3, entry4, entry5};
    bool res = mediaThumbnailAclTestTask->IsEntriesExpected(aclEntries, true);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsEntriesExpected_test_032 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsEntriesExpected_test_033, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsEntriesExpected_test_033 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry1 = {ACL_TAG::USER_OBJ, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry2 = {ACL_TAG::GROUP_OBJ, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry3 = {ACL_TAG::GROUP, 5, MEDIA_DB_ACL_GROUP};
    AclXattrEntry entry4 = {ACL_TAG::MASK, 7, ACL_UNDEFINED_ID};
    AclXattrEntry entry5 = {ACL_TAG::OTHER, 0, ACL_UNDEFINED_ID};
    std::vector<AclXattrEntry> aclEntries = {entry1, entry2, entry3, entry4, entry5};
    bool res = mediaThumbnailAclTestTask->IsEntriesExpected(aclEntries, true);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsEntriesExpected_test_033 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsGroupObjEntryExpected_test_015, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsGroupObjEntryExpected_test_015 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::GROUP_OBJ, 7, ACL_UNDEFINED_ID};
    bool res = mediaThumbnailAclTestTask->IsGroupObjEntryExpected(entry, false);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsGroupObjEntryExpected_test_015 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsGroupObjEntryExpected_test_016, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsGroupObjEntryExpected_test_016 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::GROUP_OBJ, 7, ACL_UNDEFINED_ID};
    bool res = mediaThumbnailAclTestTask->IsGroupObjEntryExpected(entry, true);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsGroupObjEntryExpected_test_016 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsOtherEntryExpected_test_012, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsOtherEntryExpected_test_012 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::OTHER, 1, ACL_UNDEFINED_ID};
    bool res = mediaThumbnailAclTestTask->IsOtherEntryExpected(entry, false);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsOtherEntryExpected_test_012 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsOtherEntryExpected_test_010, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsOtherEntryExpected_test_010 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::OTHER, 0, ACL_UNDEFINED_ID};
    bool res = mediaThumbnailAclTestTask->IsOtherEntryExpected(entry, true);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsOtherEntryExpected_test_010 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsOtherEntryExpected_test_011, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsOtherEntryExpected_test_011 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    AclXattrEntry entry = {ACL_TAG::OTHER, 0, ACL_UNDEFINED_ID};
    bool res = mediaThumbnailAclTestTask->IsOtherEntryExpected(entry, false);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("IsOtherEntryExpected_test_011 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsAllXattrExpected_test_012, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsAllXattrExpected_test_012 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::vector<XattrResult> dirDefaultAcls;
    std::vector<XattrResult> dirAccessAcls;
    std::vector<XattrResult> fileAccessAcls;
    XattrResult result;
    result.isSuccess = false;
    dirDefaultAcls.push_back(result);
    dirAccessAcls.push_back(result);
    fileAccessAcls.push_back(result);
    XattrResult unexpectedXattr;
    bool res = mediaThumbnailAclTestTask->IsAllXattrExpected(dirDefaultAcls, dirAccessAcls,
        fileAccessAcls, unexpectedXattr);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsAllXattrExpected_test_012 end");
}

HWTEST_F(MediaLibraryThumbnailAclTaskTest, IsAllXattrExpected_test_013, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsAllXattrExpected_test_013 start");
    auto mediaThumbnailAclTestTask = std::make_shared<MediaThumbnailAclTask>();
    ASSERT_NE(mediaThumbnailAclTestTask, nullptr);
    std::vector<XattrResult> dirDefaultAcls;
    std::vector<XattrResult> dirAccessAcls;
    std::vector<XattrResult> fileAccessAcls;
    XattrResult result;
    result.isSuccess = false;
    dirDefaultAcls.push_back(result);
    dirAccessAcls.push_back(result);
    fileAccessAcls.push_back(result);
    XattrResult unexpectedXattr;
    bool res = mediaThumbnailAclTestTask->IsAllXattrExpected(dirDefaultAcls, dirAccessAcls,
        fileAccessAcls, unexpectedXattr);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("IsAllXattrExpected_test_013 end");
}
} // namespace OHOS::Media::Background