/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#define MLOG_TAG "FileExtUnitTest"

#include "medialibrary_object_test.h"

#include "ability_context_impl.h"
#include "file_asset.h"
#include "media_file_utils.h"
#include "medialibrary_data_manager.h"
#define private public
#include "medialibrary_object_utils.h"
#undef private
#include "medialibrary_unittest_utils.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;

namespace OHOS {
namespace Media {

void MediaLibraryObjectTest::SetUpTestCase(void) {}

void MediaLibraryObjectTest::TearDownTestCase(void) {}

// SetUp:Execute before each test case
void MediaLibraryObjectTest::SetUp() {}

void MediaLibraryObjectTest::TearDown(void) {}

HWTEST_F(MediaLibraryObjectTest, medialib_CreateDirWithPath_test_001, TestSize.Level1)
{
    string dirPath = "";
    int32_t ret = MediaLibraryObjectUtils::CreateDirWithPath(dirPath);
    EXPECT_EQ(ret, E_INVALID_PATH);
    MediaLibraryUnitTestUtils::InitUnistore();
    dirPath = "system/CreateDirWithPath/storage/cloud/data";
    ret = MediaLibraryObjectUtils::CreateDirWithPath(dirPath);
    EXPECT_EQ(ret, E_INVALID_PATH);
    system("rm -rf /system/CreateDirWithPath");
    dirPath = "system/CreateDirWithPath.exe";
    ret = MediaLibraryObjectUtils::CreateDirWithPath(dirPath);
    EXPECT_EQ(ret, E_INVALID_PATH);
    system("rm -rf /system/CreateDirWithPath.exe");
    dirPath = "/storage/cloud/files/medialib_CreateDirWithPath_test_001";
    ret = MediaLibraryObjectUtils::CreateDirWithPath(dirPath);
    EXPECT_GT(ret, 0);
    ret = MediaFileUtils::DeleteFile(dirPath);
    EXPECT_EQ(ret, true);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibraryObjectTest, medialib_GetDirAsset_test_001, TestSize.Level1)
{
    string path = "";
    NativeAlbumAsset dirAsset;
    dirAsset = MediaLibraryObjectUtils::GetDirAsset(path);
    EXPECT_EQ(dirAsset.GetAlbumId(), E_INVALID_PATH);
    path = "system/medialib_GetDirAsset/";
    dirAsset = MediaLibraryObjectUtils::GetDirAsset(path);
    EXPECT_EQ(dirAsset.GetAlbumId(), E_HAS_DB_ERROR);
    MediaLibraryUnitTestUtils::InitUnistore();
    dirAsset = MediaLibraryObjectUtils::GetDirAsset(path);
    EXPECT_EQ(dirAsset.GetAlbumId(), E_INVALID_PATH);
    system("rm -rf /system/medialib_GetDirAsset");
    path = "//data/app/el1";
    dirAsset = MediaLibraryObjectUtils::GetDirAsset(path);
    EXPECT_EQ(dirAsset.GetAlbumId(), E_INVALID_PATH);
    path = "/storage/cloud/files/medialib_GetDirAsset_test_001";
    dirAsset = MediaLibraryObjectUtils::GetDirAsset(path);
    EXPECT_GT(dirAsset.GetAlbumId(), 0);
    bool ret = MediaFileUtils::DeleteFile(path);
    EXPECT_EQ(ret, true);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibraryObjectTest, medialib_CreateFileObj_test_001, TestSize.Level1)
{
    MediaLibraryUnitTestUtils::InitUnistore();
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::CREATE);
    int32_t ret = MediaLibraryObjectUtils::CreateFileObj(cmd);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    cmd.SetTableName(MEDIALIBRARY_TABLE);
    NativeRdb::ValuesBucket values;
    string name = "medialib_CreateFileObj_test_001";
    values.PutString(MEDIA_DATA_DB_NAME, name);
    cmd.SetValueBucket(values);
    ret = MediaLibraryObjectUtils::CreateFileObj(cmd);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    values.PutInt(MEDIA_DATA_DB_MEDIA_TYPE, MEDIA_TYPE_FILE);
    cmd.SetValueBucket(values);
    string path = "//storage/cloud/files/CreateFileObj_test_001";
    values.PutString(MEDIA_DATA_DB_RELATIVE_PATH, path);
    cmd.SetValueBucket(values);
    ret = MediaLibraryObjectUtils::CreateFileObj(cmd);
    EXPECT_NE(ret, E_SUCCESS);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibraryObjectTest, medialib_CreateDirObj_test_001, TestSize.Level1)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::CREATE);
    int64_t rowId = 0;
    int32_t ret = MediaLibraryObjectUtils::CreateDirObj(cmd, rowId);
    EXPECT_EQ(ret, E_INVALID_PATH);
    cmd.SetTableName(MEDIALIBRARY_TABLE);
    NativeRdb::ValuesBucket values;
    string path = "/storage/cloud/files/medialib_CreateDirObj_test_001";
    values.PutString(MEDIA_DATA_DB_FILE_PATH, path);
    cmd.SetValueBucket(values);
    ret = MediaLibraryObjectUtils::CreateDirObj(cmd, rowId);
    EXPECT_LT(ret, 0);
}

HWTEST_F(MediaLibraryObjectTest, medialib_RenameFileObj_test_001, TestSize.Level1)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::CREATE);
    string srcFilePath = "";
    string dstFilePath = "";
    int32_t ret = MediaLibraryObjectUtils::RenameFileObj(cmd, srcFilePath, dstFilePath);
    EXPECT_EQ(ret, E_INVALID_PATH);
    srcFilePath = "/storage/cloud/files";
    dstFilePath = "/storage/cloud/files";
    ret = MediaLibraryObjectUtils::RenameFileObj(cmd, srcFilePath, dstFilePath);
    EXPECT_EQ(ret, E_SUCCESS);
    dstFilePath = "RenameFileObj.test/media";
    cmd.SetTableName(MEDIALIBRARY_TABLE);
    NativeRdb::ValuesBucket values;
    string name = "RenameFileObj_test_001.nofile";
    string path = "/storage/media";
    values.PutString(MEDIA_DATA_DB_NAME, name);
    values.PutString(MEDIA_DATA_DB_RELATIVE_PATH, path);
    cmd.SetValueBucket(values);
    ret = MediaLibraryObjectUtils::RenameFileObj(cmd, srcFilePath, dstFilePath);
    EXPECT_EQ(ret, E_INVALID_ARGUMENTS);
}

HWTEST_F(MediaLibraryObjectTest, medialib_RenameDirObj_test_001, TestSize.Level1)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::CREATE);
    string srcFilePath = "";
    string dstFilePath = "";
    int32_t ret = MediaLibraryObjectUtils::RenameDirObj(cmd, srcFilePath, dstFilePath);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MediaLibraryUnitTestUtils::InitUnistore();
    ret = MediaLibraryObjectUtils::RenameDirObj(cmd, srcFilePath, dstFilePath);
    EXPECT_EQ(ret, E_INVALID_PATH);
    srcFilePath = "/storage/cloud/files/medialib_GetDirAsset_test_001";
    dstFilePath = "/storage/cloud/files";
    ret = MediaLibraryObjectUtils::RenameDirObj(cmd, srcFilePath, dstFilePath);
    EXPECT_EQ(ret, E_HAS_FS_ERROR);
    srcFilePath = "//data/test";
    dstFilePath = "//data/test";
    ret = MediaLibraryObjectUtils::RenameDirObj(cmd, srcFilePath, dstFilePath);
    EXPECT_EQ(ret, E_INVALID_FILEID);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibraryObjectTest, medialib_OpenFile_test_001, TestSize.Level1)
{
    MediaLibraryUnitTestUtils::InitUnistore();
    string mode = "rw";
    Uri uri("datashare:///media/local/files");
    MediaLibraryCommand cmd(uri);
    int32_t ret = MediaLibraryObjectUtils::OpenFile(cmd, mode);
    EXPECT_EQ(ret, E_INVALID_URI);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibraryObjectTest, medialib_OpenFile_test_002, TestSize.Level1)
{
    MediaLibraryUnitTestUtils::InitUnistore();
    string destDir = "/data/storage/el2/log/logpack";
    string path = destDir + "/media_library_21.db.zip";
    MediaFileUtils::CreateDirectory(destDir);
    MediaFileUtils::CreateAsset(path);

    string mode = "r";
    string uriString = "file://media/open_debug_db/21";
    Uri uri(uriString);
    MediaLibraryCommand cmd(uri, OperationType::OPEN);
    int32_t ret = MediaLibraryObjectUtils::OpenFile(cmd, mode);
    EXPECT_GE(ret, E_CHECK_SYSTEMAPP_FAIL);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibraryObjectTest, medialib_CloseFile_test_001, TestSize.Level1)
{
    Uri uri("//data/test");
    MediaLibraryCommand cmd(uri, OperationType::CLOSE);
    cmd.SetOprnAssetId("//data/test");
    int ret = MediaLibraryObjectUtils::CloseFile(cmd);
    EXPECT_EQ(ret, E_INVALID_FILEID);
    MediaLibraryUnitTestUtils::InitUnistore();
    cmd.SetOprnAssetId("medialib_CloseFile_test_001");
    ret = MediaLibraryObjectUtils::CloseFile(cmd);
    EXPECT_EQ(ret, E_INVALID_FILEID);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}


HWTEST_F(MediaLibraryObjectTest, medialib_UpdateDateModified_test_001, TestSize.Level1)
{
    string uriStr = "";
    int32_t ret = MediaLibraryObjectUtils::UpdateDateModified(uriStr);
    EXPECT_EQ(ret, E_INVALID_PATH);
    uriStr = "medialib_UpdateDateModified_test_001";
    ret = MediaLibraryObjectUtils::UpdateDateModified(uriStr);
    EXPECT_EQ(ret, E_SUCCESS);
    MediaLibraryUnitTestUtils::InitUnistore();
    ret = MediaLibraryObjectUtils::UpdateDateModified(uriStr);
    EXPECT_EQ(ret, E_SUCCESS);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibraryObjectTest, medialib_GetIdByPathFromDb_test_001, TestSize.Level1)
{
    string path = "";
    int32_t ret = MediaLibraryObjectUtils::GetIdByPathFromDb(path);
    EXPECT_EQ(ret, E_INVALID_PATH);
    path = "medialib_GetIdByPathFromDb_test_001";
    ret = MediaLibraryObjectUtils::GetIdByPathFromDb(path);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MediaLibraryUnitTestUtils::InitUnistore();
    string dirPath = "/storage/cloud/files/medialib_GetIdByPathFromDb_test_001";
    ret = MediaLibraryObjectUtils::CreateDirWithPath(dirPath);
    EXPECT_GT(ret, 0);
    ret = MediaLibraryObjectUtils::GetIdByPathFromDb(dirPath);
    EXPECT_GT(ret, 0);
    ret = MediaFileUtils::DeleteFile(dirPath);
    EXPECT_EQ(ret, true);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibraryObjectTest, medialib_InsertInDb_test_001, TestSize.Level1)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::CREATE);
    int32_t ret = MediaLibraryObjectUtils::InsertInDb(cmd);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MediaLibraryUnitTestUtils::InitUnistore();
    ret = MediaLibraryObjectUtils::InsertInDb(cmd);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    NativeRdb::ValuesBucket values;
    string name = "";
    values.PutString(MEDIA_DATA_DB_NAME, name);
    cmd.SetValueBucket(values);
    ret = MediaLibraryObjectUtils::InsertInDb(cmd);
    EXPECT_NE(ret, E_SUCCESS);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibraryObjectTest, medialib_ModifyInfoByIdInDb_test_001, TestSize.Level1)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::CREATE);
    string fileId = "";
    int32_t ret = MediaLibraryObjectUtils::ModifyInfoByIdInDb(cmd, fileId);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MediaLibraryUnitTestUtils::InitUnistore();
    ret = MediaLibraryObjectUtils::ModifyInfoByIdInDb(cmd, fileId);
    EXPECT_EQ(ret, E_INVALID_FILEID);
    fileId = "medialib_ModifyInfoByIdInDb_test_001";
    cmd.GetAbsRdbPredicates()->SetWhereClause("1 <> 0");
    ret = MediaLibraryObjectUtils::ModifyInfoByIdInDb(cmd, fileId);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibraryObjectTest, medialib_QueryWithCondition_test_001, TestSize.Level1)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::CREATE);
    vector<string> columns;
    string conditionColumn = "";
    auto resultSetPtr = MediaLibraryObjectUtils::QueryWithCondition(cmd, columns, conditionColumn);
    EXPECT_EQ(resultSetPtr, nullptr);
    MediaLibraryUnitTestUtils::InitUnistore();
    string condition = "medialib_QueryWithCondition_test_001";
    resultSetPtr = MediaLibraryObjectUtils::QueryWithCondition(cmd, columns, condition);
    EXPECT_EQ(resultSetPtr, nullptr);
    resultSetPtr = MediaLibraryObjectUtils::QueryWithCondition(cmd, columns, conditionColumn);
    EXPECT_NE(resultSetPtr, nullptr);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibraryObjectTest, medialib_IsColumnValueExist_test_001, TestSize.Level1)
{
    string value = "";
    string column = "";
    bool ret = MediaLibraryObjectUtils::IsColumnValueExist(value, column);
    EXPECT_EQ(ret, false);
    value = "medialib_IsColumnValueExist_test_001";
    column = "medialib_IsColumnValueExist_test_001";
    ret = MediaLibraryObjectUtils::IsColumnValueExist(value, column);
    EXPECT_EQ(ret, false);
    MediaLibraryUnitTestUtils::InitUnistore();
    ret = MediaLibraryObjectUtils::IsColumnValueExist(value, column);
    EXPECT_EQ(ret, false);
    column = MEDIA_DATA_DB_PARENT_ID;
    ret = MediaLibraryObjectUtils::IsColumnValueExist(value, column);
    EXPECT_EQ(ret, false);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibraryObjectTest, medialib_DeleteInfoByIdInDb_test_001, TestSize.Level1)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::CREATE);
    string fileId = "";
    int32_t ret = MediaLibraryObjectUtils::DeleteInfoByIdInDb(cmd, fileId);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MediaLibraryUnitTestUtils::InitUnistore();
    ret = MediaLibraryObjectUtils::DeleteInfoByIdInDb(cmd, fileId);
    EXPECT_EQ(ret, E_INVALID_FILEID);
    cmd.GetAbsRdbPredicates()->SetWhereClause("1 <> 0");
    ret = MediaLibraryObjectUtils::DeleteInfoByIdInDb(cmd, fileId);
    EXPECT_GT(ret, 0);
    fileId = "0";
    vector<std::string> whereArgs{"medialib_GetDirAsset"};
    cmd.SetTableName(MEDIALIBRARY_TABLE);
    cmd.GetAbsRdbPredicates()->SetWhereArgs(whereArgs);
    cmd.GetAbsRdbPredicates()->SetWhereClause("medialib_GetDirAsset");
    ret = MediaLibraryObjectUtils::DeleteInfoByIdInDb(cmd, fileId);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibraryObjectTest, medialib_IsFileExistInDb_test_001, TestSize.Level1)
{
    MediaLibraryUnitTestUtils::InitUnistore();
    string dirPath = "/storage/cloud/files/medialib_IsFileExistInDb_test_001";
    int32_t ret = MediaLibraryObjectUtils::CreateDirWithPath(dirPath);
    EXPECT_GT(ret, 0);
    string path = "";
    ret = MediaLibraryObjectUtils::IsFileExistInDb(path);
    EXPECT_EQ(ret, false);
    path = "medialib_IsFileExistInDb";
    ret = MediaLibraryObjectUtils::IsFileExistInDb(path);
    EXPECT_EQ(ret, false);
    ret = MediaLibraryObjectUtils::IsFileExistInDb(dirPath);
    EXPECT_EQ(ret, true);
    ret = MediaFileUtils::DeleteFile(dirPath);
    EXPECT_EQ(ret, true);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibraryObjectTest, medialib_CopyAsset_test_001, TestSize.Level1)
{
    shared_ptr<FileAsset> srcFileAsset = nullptr;
    string relativePath = "";
    int32_t ret = MediaLibraryObjectUtils::CopyAsset(srcFileAsset, relativePath);
    EXPECT_EQ(ret, E_INVALID_URI);
    relativePath = "medialib_CopyAsset_test_001";
    shared_ptr<FileAsset> srcFileAssetPtr = MediaLibraryObjectUtils::GetFileAssetFromUri(relativePath);
    ret = MediaLibraryObjectUtils::CopyAsset(srcFileAssetPtr, relativePath);
    EXPECT_TRUE(ret < 0);
}

HWTEST_F(MediaLibraryObjectTest, medialib_CopyDir_test_001, TestSize.Level1)
{
    shared_ptr<FileAsset> srcDirAsset = nullptr;
    string relativePath = "";
    int32_t id = 1;
    int32_t ret = MediaLibraryObjectUtils::CopyDir(srcDirAsset, relativePath);
    EXPECT_EQ(ret, E_INVALID_URI);
    relativePath = "medialib_CopyDir_test_001";
    shared_ptr<FileAsset> srcDirAssetPtr = make_shared<FileAsset>();
    srcDirAssetPtr->SetDisplayName(MEDIA_NO_FILE);
    srcDirAssetPtr->SetId(id);
    relativePath = "/storage/cloud/files/";
    ret = MediaLibraryObjectUtils::CopyDir(srcDirAssetPtr, relativePath);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
}

HWTEST_F(MediaLibraryObjectTest, medialib_IsSmartAlbumExistInDb_test_001, TestSize.Level1)
{
    int32_t id = 0;
    bool ret = MediaLibraryObjectUtils::IsSmartAlbumExistInDb(id);
    EXPECT_EQ(ret, false);
    MediaLibraryUnitTestUtils::InitUnistore();
    id = 1;
    ret = MediaLibraryObjectUtils::IsSmartAlbumExistInDb(id);
    EXPECT_EQ(ret, true);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibraryObjectTest, medialib_IsParentSmartAlbum_test_001, TestSize.Level1)
{
    int32_t id = 0;
    bool isInclude = false;
    bool ret = MediaLibraryObjectUtils::IsParentSmartAlbum(id, isInclude);
    EXPECT_EQ(ret, false);
    isInclude = true;
    ret = MediaLibraryObjectUtils::IsParentSmartAlbum(id, isInclude);
    EXPECT_EQ(ret, false);
    id = 1;
    ret = MediaLibraryObjectUtils::IsParentSmartAlbum(id, isInclude);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryObjectTest, medialib_DeleteFileObj_test_001, TestSize.Level1)
{
    string dirPath = "";
    std::shared_ptr<FileAsset> fileAsset = make_shared<FileAsset>();
    fileAsset->SetPath(dirPath);
    fileAsset->SetId(0);
    int32_t ret = MediaLibraryObjectUtils::DeleteFileObj(fileAsset);
    EXPECT_EQ(ret, E_HAS_FS_ERROR);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibraryObjectTest, medialib_DeleteMisc_test_001, TestSize.Level1)
{
    MediaLibraryUnitTestUtils::InitUnistore();
    string path = "/storage/cloud/files/medialib_DeleteMisc_test_001";
    int32_t fileId = 1;
    int32_t parentId = 1;
    int32_t ret = MediaLibraryObjectUtils::DeleteMisc(fileId, path, parentId);
    EXPECT_EQ(ret, E_SUCCESS);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibraryObjectTest, medialib_CheckDirExtension_test_001, TestSize.Level1)
{
    string relativePath = "";
    string displayName = "";
    int32_t ret = MediaLibraryObjectUtils::CheckDirExtension(relativePath, displayName);
    EXPECT_EQ(ret, E_INVALID_ARGUMENTS);
    displayName = MEDIA_NO_FILE;
    relativePath = "medialib_CheckDirExtension_test_001";
    ret = MediaLibraryObjectUtils::CheckDirExtension(relativePath, displayName);
    EXPECT_EQ(ret, E_SUCCESS);
    displayName = "medialib_CheckDirExtension_test_001.jpg";
    relativePath = "/storage/cloud/files/medialib_CheckDirExtension_test_001";
    ret = MediaLibraryObjectUtils::CheckDirExtension(relativePath, displayName);
    EXPECT_EQ(ret, E_CHECK_EXTENSION_FAIL);
    string name(256, 'a');
    ret = MediaLibraryObjectUtils::CheckDirExtension(relativePath, name);
    EXPECT_EQ(ret, E_FILE_NAME_INVALID);
}

HWTEST_F(MediaLibraryObjectTest, medialib_ModifyInfoByPathInDb_test_001, TestSize.Level1)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::CREATE);
    int32_t ret = MediaLibraryObjectUtils::ModifyInfoByPathInDb(cmd, "");
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MediaLibraryUnitTestUtils::InitUnistore();
    string path = "/storage/cloud/files/";
    ret = MediaLibraryObjectUtils::ModifyInfoByPathInDb(cmd, path);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibraryObjectTest, medialib_DeleteInfoByPathInDb_test_001, TestSize.Level1)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::CREATE);
    int32_t ret = MediaLibraryObjectUtils::DeleteInfoByPathInDb(cmd, "");
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MediaLibraryUnitTestUtils::InitUnistore();
    string path = "/storage/cloud/files/";
    ret = MediaLibraryObjectUtils::DeleteInfoByPathInDb(cmd, path);
    EXPECT_EQ(ret, E_OK);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibraryObjectTest, medialib_GetStringColumnByIdFromDb_test_001, TestSize.Level1)
{
    string value = MediaLibraryObjectUtils::GetStringColumnByIdFromDb("", "", false);
    EXPECT_EQ(value, "");
    MediaLibraryUnitTestUtils::InitUnistore();
    value = MediaLibraryObjectUtils::GetStringColumnByIdFromDb("", "", false);
    EXPECT_EQ(value, "");
    string id = "0";
    value = MediaLibraryObjectUtils::GetStringColumnByIdFromDb(id, MEDIA_DATA_DB_RECYCLE_PATH, false);
    EXPECT_EQ(value, "");
    value = MediaLibraryObjectUtils::GetStringColumnByIdFromDb(id, MEDIA_DATA_DB_RECYCLE_PATH, true);
    EXPECT_EQ(value, "");
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibraryObjectTest, medialib_InsertFileInDb_test_001, TestSize.Level1)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::CREATE);
    FileAsset fileAsset;
    NativeAlbumAsset dirAsset;
    int32_t ret = MediaLibraryObjectUtils::InsertFileInDb(cmd, fileAsset, dirAsset);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MediaLibraryUnitTestUtils::InitUnistore();
    ret = MediaLibraryObjectUtils::InsertFileInDb(cmd, fileAsset, dirAsset);
    EXPECT_GT(ret, 0);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibraryObjectTest, medialib_DeleteInvalidRowInDb_test_001, TestSize.Level1)
{
    int32_t ret = MediaLibraryObjectUtils::DeleteInvalidRowInDb("");
    EXPECT_EQ(ret, E_SUCCESS);
    MediaLibraryUnitTestUtils::InitUnistore();
    string path ="/storage/cloud/files/";
    ret = MediaLibraryObjectUtils::DeleteInvalidRowInDb(path);
    EXPECT_EQ(ret, E_SUCCESS);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibraryObjectTest, medialib_GetLastDirExistInDb_test_001, TestSize.Level1)
{
    NativeAlbumAsset dirAsset = MediaLibraryObjectUtils::GetLastDirExistInDb("");
    EXPECT_EQ(dirAsset.GetAlbumPath(), "");
    string dirPath = "/storage/cloud/files/test";
    dirAsset = MediaLibraryObjectUtils::GetLastDirExistInDb(dirPath);
    EXPECT_EQ(dirAsset.GetAlbumPath(), "/storage/cloud/files");
}

HWTEST_F(MediaLibraryObjectTest, medialib_DeleteRows_test_001, TestSize.Level1)
{
    vector<int64_t> rowIds;
    int32_t ret = MediaLibraryObjectUtils::DeleteRows(rowIds);
    EXPECT_EQ(ret, 0);
    rowIds.push_back(1);
    ret = MediaLibraryObjectUtils::DeleteRows(rowIds);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
}

HWTEST_F(MediaLibraryObjectTest, medialib_InsertDirToDbRecursively_test_001, TestSize.Level1)
{
    int64_t rowId = 0;
    int32_t ret = MediaLibraryObjectUtils::InsertDirToDbRecursively("", rowId);
    EXPECT_EQ(ret, E_VIOLATION_PARAMETERS);
    MediaLibraryUnitTestUtils::InitUnistore();
    string dirPathTest = "/storage/media/";
    ret = MediaLibraryObjectUtils::InsertDirToDbRecursively(dirPathTest, rowId);
    EXPECT_EQ(ret, E_INVALID_PATH);
    string dirPath = "/storage/cloud/files/test";
    ret = MediaLibraryObjectUtils::InsertDirToDbRecursively(dirPath, rowId);
    EXPECT_EQ(ret, E_SUCCESS);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibraryObjectTest, medialib_ProcessHiddenDir_test_001, TestSize.Level1)
{
    int32_t ret = MediaLibraryObjectUtils::ProcessHiddenDir("", "");
    EXPECT_EQ(ret, E_INVALID_PATH);
    string dstDirName = ".ProcessHiddenDir";
    string srcDirPath = "/storage/cloud/files/test";
    ret = MediaLibraryObjectUtils::ProcessHiddenDir(dstDirName, srcDirPath);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MediaLibraryUnitTestUtils::InitUnistore();
    ret = MediaLibraryObjectUtils::ProcessHiddenDir(dstDirName, srcDirPath);
    EXPECT_EQ(ret, E_SUCCESS);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibraryObjectTest, medialib_UpdateFileInfoInDb_test_001, TestSize.Level1)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::CREATE);
    int32_t bucketId = 0;
    string bucketName = "UpdateFileInfoInDb";
    int32_t ret = MediaLibraryObjectUtils::UpdateFileInfoInDb(cmd, "", bucketId, bucketName);
    EXPECT_EQ(ret, E_INVALID_PATH);
    string dstPath = "/storage/cloud/files/";
    ret = MediaLibraryObjectUtils::UpdateFileInfoInDb(cmd, dstPath, bucketId, bucketName);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
}

HWTEST_F(MediaLibraryObjectTest, medialib_CopyAssetByFd_test_001, TestSize.Level1)
{
    int32_t ret = MediaLibraryObjectUtils::CopyAssetByFd(0, 0, 0, 0);
    EXPECT_EQ(ret, E_FILE_OPER_FAIL);
    int32_t srcFd = 1;
    int32_t srcId = 1;
    int32_t destFd = 1;
    int32_t destId = 1;
    string dstPath = "/storage/cloud/files/";
    ret = MediaLibraryObjectUtils::CopyAssetByFd(srcFd, srcId, destFd, destId);
    EXPECT_EQ(ret, E_FILE_OPER_FAIL);
    MediaLibraryObjectUtils::CloseFileById(srcFd);
}

HWTEST_F(MediaLibraryObjectTest, medialib_GetFileResult_test_001, TestSize.Level1)
{
    shared_ptr<NativeRdb::ResultSet> resultSet;
    int count = 0;
    string relativePath = "/storage/cloud/files/";
    string displayName = "GetFileResult";
    int32_t ret = MediaLibraryObjectUtils::GetFileResult(resultSet, count, relativePath, displayName);
    EXPECT_EQ(ret, E_SUCCESS);
}

HWTEST_F(MediaLibraryObjectTest, medialib_ProcessNoMediaFile_test_001, TestSize.Level1)
{
    int32_t ret = MediaLibraryObjectUtils::ProcessNoMediaFile("", "");
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MediaLibraryUnitTestUtils::InitUnistore();
    ret = MediaLibraryObjectUtils::ProcessNoMediaFile("", "");
    EXPECT_EQ(ret, E_INVALID_ARGUMENTS);
    string dstFileName = ".nomedia";
    string dstAlbumPath = "/storage/cloud/files/test";
    ret = MediaLibraryObjectUtils::ProcessNoMediaFile(dstFileName, dstAlbumPath);
    EXPECT_EQ(ret, E_SUCCESS);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibraryObjectTest, medialib_ProcessHiddenFile_test_001, TestSize.Level1)
{
    int32_t ret = MediaLibraryObjectUtils::ProcessHiddenFile("", "");
    EXPECT_EQ(ret, E_INVALID_ARGUMENTS);
    string dstDirName = ".ProcessHiddenFile";
    string srcPath = "/storage/cloud/files/test";
    ret = MediaLibraryObjectUtils::ProcessHiddenFile(dstDirName, srcPath);
    EXPECT_EQ(ret, E_SUCCESS);
}
} // namespace Media
} // namespace OHOS