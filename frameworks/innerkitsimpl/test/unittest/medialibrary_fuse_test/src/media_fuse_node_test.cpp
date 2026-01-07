/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#define MLOG_TAG "MediaFuseNodeTest"
#include <gtest/gtest.h>
#include "medialibrary_errno.h"
#include "media_fuse_node.h"
#include "media_log.h"

using namespace testing;
using namespace std;
using namespace testing::ext;
namespace OHOS {
namespace Media {
class MediaFuseNodeTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void MediaFuseNodeTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("SetUpTestCase");
}

void MediaFuseNodeTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("TearDownTestCase");
}

void MediaFuseNodeTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
}

void MediaFuseNodeTest::TearDown(void)
{
    MediaFuseNode::ReleaseAllNodes();
    MEDIA_INFO_LOG("TearDown");
}

HWTEST_F(MediaFuseNodeTest, MediaLibrary_CreateNode_test_001, Level1)
{
    const char *fileName = nullptr;
    fuse_ino_t parent = 1;
    ino_t srcIno = 1;
    auto ret = MediaFuseNode::CreateNode(fileName, parent, srcIno);
    EXPECT_EQ(ret, FUSE_INVALID_INO);
}

HWTEST_F(MediaFuseNodeTest, MediaLibrary_CreateNode_test_002, Level1)
{
    const char *fileName = "";
    fuse_ino_t parent = 1;
    ino_t srcIno = 1;
    auto ret = MediaFuseNode::CreateNode(fileName, parent, srcIno);
    EXPECT_EQ(ret, FUSE_INVALID_INO);
}

HWTEST_F(MediaFuseNodeTest, MediaLibrary_CreateNode_test_003, Level1)
{
    const char *fileName = "testFile";
    fuse_ino_t parent = 1;
    ino_t srcIno = 1;
    auto ret = MediaFuseNode::CreateNode(fileName, parent, srcIno);
    EXPECT_NE(ret, FUSE_INVALID_INO);
}

HWTEST_F(MediaFuseNodeTest, MediaLibrary_RemoveNode_test_001, Level1)
{
    const char *fileName = "testFile";
    fuse_ino_t parent = 1;
    ino_t srcIno = 1;
    auto nodeId = MediaFuseNode::CreateNode(fileName, parent, srcIno);
    EXPECT_NE(nodeId, FUSE_INVALID_INO);
    MediaFuseNode::RemoveNode(nodeId);
    auto &node = MediaFuseNode::GetNodeById(nodeId);
    EXPECT_EQ(node.parent, FUSE_INVALID_INO);
}

HWTEST_F(MediaFuseNodeTest, MediaLibrary_GetNodeById_test_001, Level1)
{
    const char *fileName = "testFile";
    fuse_ino_t parent = 1;
    ino_t srcIno = 1;
    auto nodeId = MediaFuseNode::CreateNode(fileName, parent, srcIno);
    EXPECT_NE(nodeId, FUSE_INVALID_INO);
    auto &node = MediaFuseNode::GetNodeById(nodeId);
    EXPECT_EQ(node.parent, parent);
    EXPECT_EQ(node.fileName, string(fileName));
}

HWTEST_F(MediaFuseNodeTest, MediaLibrary_GetNodeById_test_002, Level1)
{
    fuse_ino_t nodeId = 9999;
    auto &node = MediaFuseNode::GetNodeById(nodeId);
    EXPECT_EQ(node.parent, FUSE_INVALID_INO);
}

HWTEST_F(MediaFuseNodeTest, MediaLibrary_GetNodeFullPath_test_001, Level1)
{
    fuse_ino_t nodeId = FUSE_INVALID_INO;
    auto ret = MediaFuseNode::GetNodeFullPath(nodeId);
    EXPECT_EQ(ret, "");
}

HWTEST_F(MediaFuseNodeTest, MediaLibrary_GetNodeFullPath_test_002, Level1)
{
    fuse_ino_t nodeId = FUSE_ROOT_INO;
    auto ret = MediaFuseNode::GetNodeFullPath(nodeId);
    EXPECT_EQ(ret, "/");
}

HWTEST_F(MediaFuseNodeTest, MediaLibrary_GetNodeFullPath_test_003, Level1)
{
    const char *fileName = "testFile";
    fuse_ino_t parent = FUSE_ROOT_INO;
    ino_t srcIno = 1;
    auto nodeId = MediaFuseNode::CreateNode(fileName, parent, srcIno);
    EXPECT_NE(nodeId, FUSE_INVALID_INO);
    auto ret = MediaFuseNode::GetNodeFullPath(nodeId);
    EXPECT_EQ(ret, string("/") + string(fileName));
}

HWTEST_F(MediaFuseNodeTest, MediaLibrary_GetNodeFullPath_test_004, Level1)
{
    const char *parentFileName = "parentDir";
    fuse_ino_t parent = MediaFuseNode::CreateNode(parentFileName, FUSE_ROOT_INO, 2);
    EXPECT_NE(parent, FUSE_INVALID_INO);
    const char *fileName = "testFile";
    ino_t srcIno = 1;
    auto nodeId = MediaFuseNode::CreateNode(fileName, parent, srcIno);
    EXPECT_NE(nodeId, FUSE_INVALID_INO);
    auto ret = MediaFuseNode::GetNodeFullPath(nodeId);
    EXPECT_EQ(ret, string("/") + string(parentFileName) + string("/") + string(fileName));
}

HWTEST_F(MediaFuseNodeTest, MediaLibrary_GetChildNodeFullPath_test_001, Level1)
{
    fuse_ino_t parent = FUSE_INVALID_INO;
    const char *fileName = nullptr;
    auto ret = MediaFuseNode::GetChildNodeFullPath(parent, fileName);
    EXPECT_EQ(ret, "");
}

HWTEST_F(MediaFuseNodeTest, MediaLibrary_GetChildNodeFullPath_test_002, Level1)
{
    fuse_ino_t parent = FUSE_INVALID_INO;
    const char *fileName = "";
    auto ret = MediaFuseNode::GetChildNodeFullPath(parent, fileName);
    EXPECT_EQ(ret, "");
}

HWTEST_F(MediaFuseNodeTest, MediaLibrary_GetChildNodeFullPath_test_003, Level1)
{
    fuse_ino_t parent = FUSE_ROOT_INO;
    const char *fileName = "testFile";
    auto ret = MediaFuseNode::GetChildNodeFullPath(parent, fileName);
    EXPECT_EQ(ret, string("/") + string(fileName));
}

HWTEST_F(MediaFuseNodeTest, MediaLibrary_GetChildNodeFullPath_test_004, Level1)
{
    const char *parentFileName = "parentDir";
    fuse_ino_t parent = MediaFuseNode::CreateNode(parentFileName, FUSE_ROOT_INO, 2);
    EXPECT_NE(parent, FUSE_INVALID_INO);
    const char *fileName = "testFile";
    auto ret = MediaFuseNode::GetChildNodeFullPath(parent, fileName);
    EXPECT_EQ(ret, string("/") + string(parentFileName) + string("/") + string(fileName));
}

HWTEST_F(MediaFuseNodeTest, MediaLibrary_GetNodeMutex_test_001, Level1)
{
    fuse_ino_t nodeId = FUSE_INVALID_INO;
    auto ret = MediaFuseNode::GetNodeMutex(nodeId);
    EXPECT_EQ(ret, nullptr);
}

HWTEST_F(MediaFuseNodeTest, MediaLibrary_GetNodeMutex_test_002, Level1)
{
    const char *fileName = "testFile";
    fuse_ino_t parent = 1;
    ino_t srcIno = 1;
    auto nodeId = MediaFuseNode::CreateNode(fileName, parent, srcIno);
    EXPECT_NE(nodeId, FUSE_INVALID_INO);
    auto ret = MediaFuseNode::GetNodeMutex(nodeId);
    EXPECT_NE(ret, nullptr);
}

HWTEST_F(MediaFuseNodeTest, MediaLibrary_FindNodeIdByParent_test_001, Level1)
{
    const char *fileName = nullptr;
    fuse_ino_t parent = 1;
    auto ret = MediaFuseNode::FindNodeIdByParent(fileName, parent);
    EXPECT_EQ(ret, FUSE_INVALID_INO);
}

HWTEST_F(MediaFuseNodeTest, MediaLibrary_FindNodeIdByParent_test_002, Level1)
{
    const char *fileName = "";
    fuse_ino_t parent = 1;
    auto ret = MediaFuseNode::FindNodeIdByParent(fileName, parent);
    EXPECT_EQ(ret, FUSE_INVALID_INO);
}

HWTEST_F(MediaFuseNodeTest, MediaLibrary_FindNodeIdByParent_test_003, Level1)
{
    const char *fileName = "testFile";
    fuse_ino_t parent = 1;
    ino_t srcIno = 1;
    auto nodeId = MediaFuseNode::CreateNode(fileName, parent, srcIno);
    EXPECT_NE(nodeId, FUSE_INVALID_INO);
    auto ret = MediaFuseNode::FindNodeIdByParent(fileName, parent);
    EXPECT_EQ(ret, nodeId);
}

HWTEST_F(MediaFuseNodeTest, MediaLibrary_FindNodeIdByParent_test_004, Level1)
{
    const char *fileName = "noExistFile";
    fuse_ino_t parent = 1;
    auto ret = MediaFuseNode::FindNodeIdByParent(fileName, parent);
    EXPECT_EQ(ret, FUSE_INVALID_INO);
}

HWTEST_F(MediaFuseNodeTest, MediaLibrary_FindNodeIdByStIno_test_001, Level1)
{
    ino_t srcIno = 9999;
    auto ret = MediaFuseNode::FindNodeIdByStIno(srcIno);
    EXPECT_EQ(ret, FUSE_INVALID_INO);
}

HWTEST_F(MediaFuseNodeTest, MediaLibrary_FindNodeIdByStIno_test_002, Level1)
{
    const char *fileName = "testFile";
    fuse_ino_t parent = 1;
    ino_t srcIno = 1;
    auto nodeId = MediaFuseNode::CreateNode(fileName, parent, srcIno);
    EXPECT_NE(nodeId, FUSE_INVALID_INO);
    auto ret = MediaFuseNode::FindNodeIdByStIno(srcIno);
    EXPECT_EQ(ret, nodeId);
}

HWTEST_F(MediaFuseNodeTest, MediaLibrary_UpdateInoByInodeKey_test_001, Level1)
{
    const char *fileName = "testFile";
    fuse_ino_t parent = 1;
    ino_t srcIno = 1;
    auto nodeId = MediaFuseNode::CreateNode(fileName, parent, srcIno);
    EXPECT_NE(nodeId, FUSE_INVALID_INO);
    auto &node = MediaFuseNode::GetNodeById(nodeId);
    EXPECT_EQ(node.parent, parent);
    EXPECT_EQ(node.fileName, string(fileName));

    string newName = "newTestFile";
    MediaFuseNode::UpdateInoByInodeKey(node, parent, newName, nodeId);
    EXPECT_EQ(node.fileName, newName);
}

HWTEST_F(MediaFuseNodeTest, MediaLibrary_ReleaseAllNodes_test_001, Level1)
{
    const char *fileName = "testFile1";
    fuse_ino_t parent = 1;
    ino_t srcIno = 1;
    auto nodeId = MediaFuseNode::CreateNode(fileName, parent, srcIno);
    EXPECT_NE(nodeId, FUSE_INVALID_INO);

    MediaFuseNode::ReleaseAllNodes();

    auto &node = MediaFuseNode::GetNodeById(nodeId);
    EXPECT_EQ(node.parent, FUSE_INVALID_INO);
}
} // namespace Media
} // namespace OHOS
