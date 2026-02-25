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

#include "medialibrary_thumbnail_acl_task_test.h"

#include "medialibrary_errno.h"
#include "media_log.h"

#include "media_thumbnail_acl_task.h"

using namespace testing::ext;
using namespace std;

namespace OHOS::Media::Background {

void MediaLibraryThumbnailAclTaskTest::SetUpTestCase(void) {}

void MediaLibraryThumbnailAclTaskTest::TearDownTestCase(void) {}

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
} // namespace OHOS::Media::Background