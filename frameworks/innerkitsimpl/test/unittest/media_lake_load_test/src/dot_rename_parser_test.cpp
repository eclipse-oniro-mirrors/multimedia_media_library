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

#define MLOG_TAG "DotRenameParserTest"

#include "dot_rename_parser_test.h"

#include "lake_file_parser.h"
#include "file_manager_parser.h"
#include "media_column.h"
#include "media_file_notify_info.h"
#include "media_file_utils.h"
#include "media_pure_file_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "photo_album_column.h"
#include "rdb_predicates.h"
#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {
using namespace testing::ext;
using namespace NativeRdb;

// 该用例直接调用 LakeFileParser 的点号重命名判定，避免在 dot_rename_test.cpp 中复制实现逻辑。
// media_lake_load_test 目标带 -fno-access-control，可直接访问 private 方法。
static const std::string TEST_DIR_PATH = "/data/test/dot_rename_parser_test";
static const std::string TEST_NORMAL_PATH = TEST_DIR_PATH + "/hi.jpg";
static const std::string TEST_DOT_PATH = TEST_DIR_PATH + "/.hi.jpg";

void DotRenameParserTest::SetUpTestCase()
{
    MediaPureFileUtils::CreateDirectory(TEST_DIR_PATH);
}

void DotRenameParserTest::TearDownTestCase()
{
}

void DotRenameParserTest::SetUp()
{
}

void DotRenameParserTest::TearDown()
{
}

static MediaNotifyInfo BuildDotRenameNotifyInfo(const std::string &beforePath, const std::string &afterPath)
{
    MediaNotifyInfo info;
    info.objType = FileNotifyObjectType::FILE;
    info.optType = FileNotifyOperationType::MOD;
    info.beforePath = beforePath;
    info.afterPath = afterPath;
    return info;
}

// hi.jpg -> .hi.jpg：释放本地
HWTEST_F(DotRenameParserTest, IsDotRenameRelease_001, TestSize.Level1)
{
    LakeFileParser parser(BuildDotRenameNotifyInfo(TEST_NORMAL_PATH, TEST_DOT_PATH), ScanMode::INCREMENT);
    EXPECT_TRUE(parser.IsDotRenameRelease());
    EXPECT_FALSE(parser.IsDotRenameRecover());
}

// .hi.jpg -> hi.jpg：恢复入库
HWTEST_F(DotRenameParserTest, IsDotRenameRecover_001, TestSize.Level1)
{
    LakeFileParser parser(BuildDotRenameNotifyInfo(TEST_DOT_PATH, TEST_NORMAL_PATH), ScanMode::INCREMENT);
    EXPECT_FALSE(parser.IsDotRenameRelease());
    EXPECT_TRUE(parser.IsDotRenameRecover());
}

// 普通改名：两个方向都不成立
HWTEST_F(DotRenameParserTest, IsDotRename_NormalRename_001, TestSize.Level1)
{
    LakeFileParser parser(BuildDotRenameNotifyInfo(TEST_NORMAL_PATH,
        TEST_DIR_PATH + "/hi1.jpg"), ScanMode::INCREMENT);
    EXPECT_FALSE(parser.IsDotRenameRelease());
    EXPECT_FALSE(parser.IsDotRenameRecover());
}

// 隐藏文件之间改名（.a.jpg -> .b.jpg）：都不成立
HWTEST_F(DotRenameParserTest, IsDotRename_BothDot_001, TestSize.Level1)
{
    LakeFileParser parser(BuildDotRenameNotifyInfo(TEST_DIR_PATH + "/.a.jpg",
        TEST_DIR_PATH + "/.b.jpg"), ScanMode::INCREMENT);
    EXPECT_FALSE(parser.IsDotRenameRelease());
    EXPECT_FALSE(parser.IsDotRenameRecover());
}

// 空路径：都不成立
HWTEST_F(DotRenameParserTest, IsDotRename_EmptyPath_001, TestSize.Level1)
{
    LakeFileParser parser(BuildDotRenameNotifyInfo("", ""), ScanMode::INCREMENT);
    EXPECT_FALSE(parser.IsDotRenameRelease());
    EXPECT_FALSE(parser.IsDotRenameRecover());
}

// 跨目录改名：只按文件名首字符判定
HWTEST_F(DotRenameParserTest, IsDotRename_CrossDir_001, TestSize.Level1)
{
    LakeFileParser parser(BuildDotRenameNotifyInfo("/data/test/a/hi.jpg",
        "/data/test/b/.hi.jpg"), ScanMode::INCREMENT);
    EXPECT_TRUE(parser.IsDotRenameRelease());
    EXPECT_FALSE(parser.IsDotRenameRecover());
}

// ---- GetDotRenameUpdateType 三分支：需要 Photos 表记录参与判定，故独立 fixture 初始化 RDB ----
static std::shared_ptr<MediaLibraryRdbStore> g_dotRenameRdbStore = nullptr;
static const std::string TEST_DOT_RENAME_ROOT = "/storage/media/local/files/Docs/dot_rename_parser_test";
static const std::string TEST_DOT_RENAME_NORMAL_PATH = TEST_DOT_RENAME_ROOT + "/hi.jpg";
static const std::string TEST_DOT_RENAME_HIDDEN_PATH = TEST_DOT_RENAME_ROOT + "/.hi.jpg";
static std::vector<int32_t> g_dotRenameInsertedIds;

void DotRenameUpdateTypeTest::SetUpTestCase()
{
    MediaLibraryUnitTestUtils::Init();
    g_dotRenameRdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_dotRenameRdbStore, nullptr);
}

void DotRenameUpdateTypeTest::TearDownTestCase()
{
}

void DotRenameUpdateTypeTest::SetUp()
{
    g_dotRenameInsertedIds.clear();
}

void DotRenameUpdateTypeTest::TearDown()
{
    if (g_dotRenameRdbStore == nullptr) {
        return;
    }
    // 清理用例插入的Photos记录, 避免残留影响同进程其他用例
    for (int32_t fileId : g_dotRenameInsertedIds) {
        NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
        predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
        int32_t deletedRows = -1;
        g_dotRenameRdbStore->Delete(deletedRows, predicates);
    }
    g_dotRenameInsertedIds.clear();
}

// 插入一条文管来源记录，storage_path 用于 FindSameFileByStoragePath 命中
static int32_t InsertFileManagerAsset(const std::string &storagePath)
{
    ValuesBucket values;
    values.Put(MediaColumn::MEDIA_FILE_PATH, storagePath);
    values.Put(MediaColumn::MEDIA_NAME, MediaFileUtils::GetFileName(storagePath));
    values.Put(MediaColumn::MEDIA_TYPE, static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE));
    values.Put(PhotoColumn::PHOTO_STORAGE_PATH, storagePath);
    values.Put(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, static_cast<int32_t>(FileSourceType::FILE_MANAGER));
    int64_t rowId = -1;
    int32_t ret = g_dotRenameRdbStore->Insert(rowId, PhotoColumn::PHOTOS_TABLE, values);
    if (ret != E_OK) {
        return -1;
    }
    g_dotRenameInsertedIds.emplace_back(static_cast<int32_t>(rowId));
    return static_cast<int32_t>(rowId);
}

// 释放本地：DB 中存在 beforePath 记录 -> DOT_RENAME_RELEASE
HWTEST_F(DotRenameUpdateTypeTest, GetDotRenameUpdateType_Release_001, TestSize.Level1)
{
    ASSERT_GT(InsertFileManagerAsset(TEST_DOT_RENAME_NORMAL_PATH), 0);
    FileManagerParser parser(BuildDotRenameNotifyInfo(TEST_DOT_RENAME_NORMAL_PATH, TEST_DOT_RENAME_HIDDEN_PATH),
        ScanMode::INCREMENT);
    FileUpdateType updateType = FileUpdateType::NO_CHANGE;
    EXPECT_TRUE(parser.GetDotRenameUpdateType(updateType));
    EXPECT_EQ(updateType, FileUpdateType::DOT_RENAME_RELEASE);
}

// 释放本地：DB 中不存在 beforePath 记录 -> 不判定为点号重命名
HWTEST_F(DotRenameUpdateTypeTest, GetDotRenameUpdateType_ReleaseNoRecord_001, TestSize.Level1)
{
    FileManagerParser parser(BuildDotRenameNotifyInfo(TEST_DOT_RENAME_NORMAL_PATH, TEST_DOT_RENAME_HIDDEN_PATH),
        ScanMode::INCREMENT);
    FileUpdateType updateType = FileUpdateType::NO_CHANGE;
    EXPECT_FALSE(parser.GetDotRenameUpdateType(updateType));
}

// 恢复入库：DB 中存在 afterPath 记录 -> DOT_RENAME_RECOVER
HWTEST_F(DotRenameUpdateTypeTest, GetDotRenameUpdateType_Recover_001, TestSize.Level1)
{
    ASSERT_GT(InsertFileManagerAsset(TEST_DOT_RENAME_NORMAL_PATH), 0);
    FileManagerParser parser(BuildDotRenameNotifyInfo(TEST_DOT_RENAME_HIDDEN_PATH, TEST_DOT_RENAME_NORMAL_PATH),
        ScanMode::INCREMENT);
    FileUpdateType updateType = FileUpdateType::NO_CHANGE;
    EXPECT_TRUE(parser.GetDotRenameUpdateType(updateType));
    EXPECT_EQ(updateType, FileUpdateType::DOT_RENAME_RECOVER);
}

// 纯云图恢复：DB 中不存在 afterPath 记录 -> INSERT 重新入库融合
HWTEST_F(DotRenameUpdateTypeTest, GetDotRenameUpdateType_RecoverAsInsert_001, TestSize.Level1)
{
    FileManagerParser parser(BuildDotRenameNotifyInfo(TEST_DOT_RENAME_HIDDEN_PATH, TEST_DOT_RENAME_NORMAL_PATH),
        ScanMode::INCREMENT);
    FileUpdateType updateType = FileUpdateType::NO_CHANGE;
    EXPECT_TRUE(parser.GetDotRenameUpdateType(updateType));
    EXPECT_EQ(updateType, FileUpdateType::INSERT);
}

} // namespace Media
} // namespace OHOS
