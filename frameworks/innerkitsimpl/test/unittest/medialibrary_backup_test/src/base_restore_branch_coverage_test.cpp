/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

#define MLOG_TAG "BaseRestoreBranchCoverageTest"

#include "base_restore_branch_coverage_test.h"

#include <fstream>

#define private public
#define protected public
#include "base_restore.h"
#include "media_file_utils.h"
#include "photos_dao.h"
#include "result_set.h"
#include "values_bucket.h"
#undef private
#undef protected

#include "media_pure_file_utils.h"
#include "medialibrary_errno.h"

using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace {
const std::string TEST_DIR = "/data/test/backup/base_restore_branch_coverage";

class TestBaseRestore final : public BaseRestore {
public:
    int32_t Init(const std::string &backupRestorePath, const std::string &upgradePath, bool isUpgrade) override
    {
        return E_OK;
    }

    NativeRdb::ValuesBucket GetInsertValue(const FileInfo &fileInfo, const std::string &newPath,
        int32_t sourceType) override
    {
        NativeRdb::ValuesBucket values;
        values.PutString(MediaColumn::MEDIA_FILE_PATH, newPath);
        return values;
    }

    bool ParseResultSet(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, FileInfo &info,
        std::string dbName = "") override
    {
        return false;
    }

    bool ParseResultSetForAudio(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, FileInfo &info) override
    {
        return false;
    }

    void AnalyzeSource() override {}
    void RestorePhoto() override {}
    void RestoreAudio() override {}
    void HandleRestData() override {}
};

FileInfo BuildBaseInfo()
{
    FileInfo info;
    info.fileIdOld = 1;
    info.filePath = TEST_DIR + "/src.jpg";
    info.cloudPath = TEST_DIR + "/dst.jpg";
    info.needMove = true;
    info.isNew = true;
    info.position = static_cast<int32_t>(PhotoPositionType::LOCAL);
    return info;
}

void PrepareDir()
{
    if (!MediaPureFileUtils::IsDirectory(TEST_DIR)) {
        (void)MediaPureFileUtils::CreateDirectory(TEST_DIR);
    }
}

void WriteFile(const std::string &path, const std::string &content)
{
    std::ofstream ofs(path, std::ios::trunc | std::ios::binary);
    ofs << content;
}
} // namespace

void BaseRestoreBranchCoverageTest::SetUpTestCase()
{
    PrepareDir();
}

void BaseRestoreBranchCoverageTest::TearDownTestCase()
{
    (void)MediaFileUtils::DeleteDir(TEST_DIR);
}

void BaseRestoreBranchCoverageTest::SetUp()
{
    PrepareDir();
}

void BaseRestoreBranchCoverageTest::TearDown()
{
    (void)MediaFileUtils::DeleteDir(TEST_DIR);
    PrepareDir();
}

// 验证 ExtraCheckForCloneSameFile 在 clean 云端记录场景下会触发更新路径。
HWTEST_F(BaseRestoreBranchCoverageTest, ExtraCheckForCloneSameFile_CleanCloud_001, TestSize.Level1)
{
    TestBaseRestore restore;
    FileInfo info = BuildBaseInfo();
    PhotosDao::PhotosRowData rowData;
    rowData.fileId = 100;
    rowData.data = "/storage/cloud/files/Photo/1/clean_001.jpg";
    rowData.cleanFlag = 1;
    rowData.position = static_cast<int32_t>(PhotoPositionType::CLOUD);
    bool ret = restore.ExtraCheckForCloneSameFile(info, rowData);
    EXPECT_FALSE(ret);
    EXPECT_TRUE(info.needUpdate);
    EXPECT_FALSE(info.isNew);
    EXPECT_EQ(info.fileIdNew, 100);
}

// 验证 ExtraCheckForCloneSameFile 在普通重复文件场景下直接判定为重复。
HWTEST_F(BaseRestoreBranchCoverageTest, ExtraCheckForCloneSameFile_NormalDuplicate_001, TestSize.Level1)
{
    TestBaseRestore restore;
    FileInfo info = BuildBaseInfo();
    PhotosDao::PhotosRowData rowData;
    rowData.fileId = 101;
    rowData.data = "/storage/cloud/files/Photo/1/dup_001.jpg";
    rowData.cleanFlag = 0;
    rowData.position = static_cast<int32_t>(PhotoPositionType::LOCAL);
    bool ret = restore.ExtraCheckForCloneSameFile(info, rowData);
    EXPECT_TRUE(ret);
    EXPECT_FALSE(info.needMove);
    EXPECT_FALSE(info.isNew);
    EXPECT_EQ(info.fileIdNew, 101);
}

// 验证 CheckAndDelete 能删除已存在的 ValuesBucket 字段。
HWTEST_F(BaseRestoreBranchCoverageTest, CheckAndDelete_RemoveExistingColumn_001, TestSize.Level1)
{
    TestBaseRestore restore;
    NativeRdb::ValuesBucket values;
    values.PutString(PhotoColumn::PHOTO_USER_COMMENT, "abc");
    ASSERT_TRUE(values.HasColumn(PhotoColumn::PHOTO_USER_COMMENT));
    restore.CheckAndDelete(values, PhotoColumn::PHOTO_USER_COMMENT);
    EXPECT_FALSE(values.HasColumn(PhotoColumn::PHOTO_USER_COMMENT));
}

// 验证 CheckAndDelete 对不存在字段调用时不会引入异常状态。
HWTEST_F(BaseRestoreBranchCoverageTest, CheckAndDelete_NonExistingColumn_001, TestSize.Level1)
{
    TestBaseRestore restore;
    NativeRdb::ValuesBucket values;
    restore.CheckAndDelete(values, PhotoColumn::PHOTO_USER_COMMENT);
    EXPECT_FALSE(values.HasColumn(PhotoColumn::PHOTO_USER_COMMENT));
}

// 验证 InsertDateTime 在 detailTime 非法时仍会补齐日期相关字段。
HWTEST_F(BaseRestoreBranchCoverageTest, InsertDateTime_InvalidDetailTime_001, TestSize.Level1)
{
    TestBaseRestore restore;
    NativeRdb::ValuesBucket values;
    FileInfo info;
    info.dateTaken = 1715000000000;
    info.detailTime = "invalid";
    restore.InsertDateTime(values, info);
    EXPECT_TRUE(values.HasColumn(PhotoColumn::PHOTO_DETAIL_TIME));
    EXPECT_TRUE(values.HasColumn(PhotoColumn::PHOTO_DATE_YEAR));
    EXPECT_TRUE(values.HasColumn(PhotoColumn::PHOTO_DATE_MONTH));
    EXPECT_TRUE(values.HasColumn(PhotoColumn::PHOTO_DATE_DAY));
}

// 验证 InsertDateTime 在 detailTime 合法时会正常写入日期字段。
HWTEST_F(BaseRestoreBranchCoverageTest, InsertDateTime_ValidDetailTime_001, TestSize.Level1)
{
    TestBaseRestore restore;
    NativeRdb::ValuesBucket values;
    FileInfo info;
    info.dateTaken = 1715000000000;
    info.detailTime = "2024:05:06 01:02:03";
    restore.InsertDateTime(values, info);
    EXPECT_TRUE(values.HasColumn(PhotoColumn::PHOTO_DETAIL_TIME));
    EXPECT_TRUE(values.HasColumn(PhotoColumn::PHOTO_DATE_YEAR));
    EXPECT_TRUE(values.HasColumn(PhotoColumn::PHOTO_DATE_MONTH));
    EXPECT_TRUE(values.HasColumn(PhotoColumn::PHOTO_DATE_DAY));
}

// 验证 MoveFile 在 rename 失败回落拷贝路径时仍能成功生成目标文件。
HWTEST_F(BaseRestoreBranchCoverageTest, MoveFile_CopyFallback_001, TestSize.Level1)
{
    TestBaseRestore restore;
    std::string src = TEST_DIR + "/src_fallback.txt";
    std::string dst = TEST_DIR + "/sub/dst_fallback.txt";
    (void)MediaPureFileUtils::CreateDirectory(TEST_DIR + "/sub");
    WriteFile(src, "hello-fallback");
    int32_t ret = restore.MoveFile(src, dst);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(MediaPureFileUtils::IsFileExists(dst));
}

// 验证 MoveFile 在源文件不存在时返回失败码。
HWTEST_F(BaseRestoreBranchCoverageTest, MoveFile_SourceNotExist_001, TestSize.Level1)
{
    TestBaseRestore restore;
    std::string src = TEST_DIR + "/not_exist.txt";
    std::string dst = TEST_DIR + "/dst_not_exist.txt";
    int32_t ret = restore.MoveFile(src, dst);
    EXPECT_NE(ret, E_OK);
}

// 验证 ExtraCheckForCloneSameFile 在普通批量重复场景下保持无更新无移动。
HWTEST_F(BaseRestoreBranchCoverageTest, ExtraCheckForCloneSameFile_Batch_001, TestSize.Level1)
{
    TestBaseRestore restore;
    FileInfo info = BuildBaseInfo();
    PhotosDao::PhotosRowData rowData;
    rowData.fileId = 1001;
    rowData.data = "/storage/cloud/files/Photo/1/batch_001.jpg";
    rowData.cleanFlag = 0;
    rowData.position = 1;
    bool ret = restore.ExtraCheckForCloneSameFile(info, rowData);
    EXPECT_TRUE(ret);
    EXPECT_FALSE(info.needUpdate);
    EXPECT_FALSE(info.needMove);
    EXPECT_FALSE(info.isNew);
    EXPECT_EQ(info.fileIdNew, 1001);
}

// 验证 ExtraCheckForCloneSameFile 在 cleanFlag 命中但位置未触发更新时仍按重复文件处理。
HWTEST_F(BaseRestoreBranchCoverageTest, ExtraCheckForCloneSameFile_Batch_004, TestSize.Level1)
{
    TestBaseRestore restore;
    FileInfo info = BuildBaseInfo();
    PhotosDao::PhotosRowData rowData;
    rowData.fileId = 1004;
    rowData.data = "/storage/cloud/files/Photo/1/batch_004.jpg";
    rowData.cleanFlag = 1;
    rowData.position = 1;
    bool ret = restore.ExtraCheckForCloneSameFile(info, rowData);
    EXPECT_TRUE(ret);
    EXPECT_FALSE(info.needUpdate);
    EXPECT_FALSE(info.needMove);
    EXPECT_FALSE(info.isNew);
    EXPECT_EQ(info.fileIdNew, 1004);
}

// 验证 ExtraCheckForCloneSameFile 在 cleanFlag 与云端位置组合场景下会标记更新和移动。
HWTEST_F(BaseRestoreBranchCoverageTest, ExtraCheckForCloneSameFile_Batch_005, TestSize.Level1)
{
    TestBaseRestore restore;
    FileInfo info = BuildBaseInfo();
    PhotosDao::PhotosRowData rowData;
    rowData.fileId = 1005;
    rowData.data = "/storage/cloud/files/Photo/1/batch_005.jpg";
    rowData.cleanFlag = 1;
    rowData.position = 2;
    bool ret = restore.ExtraCheckForCloneSameFile(info, rowData);
    EXPECT_FALSE(ret);
    EXPECT_TRUE(info.needUpdate);
    EXPECT_TRUE(info.needMove);
    EXPECT_FALSE(info.isNew);
    EXPECT_EQ(info.fileIdNew, 1005);
}

} // namespace Media
} // namespace OHOS
