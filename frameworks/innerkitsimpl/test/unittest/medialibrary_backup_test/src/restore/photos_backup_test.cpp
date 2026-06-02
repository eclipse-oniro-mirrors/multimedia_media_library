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

#define MLOG_TAG "PhotosBackupTest"

#include "photos_backup_test.h"

#include <string>

#include "backup_const.h"
#include "json_utils.h"
#include "media_log.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "nlohmann/json.hpp"
#include "photos_backup.h"
#include "userfile_manager_types.h"
#include "media_string_utils.h"
#include "media_file_utils.h"

using namespace testing::ext;

namespace OHOS::Media {
const size_t EXPECTED_BACKUP_INFO_SIZE = 12;
static std::shared_ptr<MediaLibraryRdbStore> g_rdbStore;

static int64_t GetIntFromJsonByKey(const nlohmann::json &jsonObj, const std::string &key)
{
    for (const auto &item : jsonObj) {
        if (item.contains("backupInfo") && item["backupInfo"] == key &&
            item.contains("number") && item["number"].is_number()) {
            return item["number"].get<int64_t>();
        }
    }
    return -1;
}

void PhotosBackupTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("SetUpTestCase");
    MEDIA_INFO_LOG("Start Init");
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    MediaFileUtils::CreateDirectory(CLONE_RESTORE_BACKUP_DIR);
}

void PhotosBackupTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("TearDownTestCase");
}

void PhotosBackupTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
}

void PhotosBackupTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}

HWTEST_F(PhotosBackupTest, BackupInfo_ToString_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("BackupInfo_ToString_Test_001 start");
    PhotosBackup::BackupInfo backupInfo;
    backupInfo.suffix = "";
    backupInfo.photoCount = 10;
    backupInfo.videoCount = 5;
    backupInfo.cloudPhotoCount = 3;
    backupInfo.cloudVideoCount = 2;
    backupInfo.audioCount = 7;
    backupInfo.totalSize = 1024000;
    backupInfo.lakePhotoCount = 15;
    backupInfo.lakeVideoCount = 8;
    backupInfo.lakeTotalSize = 2048000;
    backupInfo.fileManagerPhotoCount = 20;
    backupInfo.fileManagerVideoCount = 10;
    backupInfo.fileManagerTotalSize = 3072000;

    std::string result = backupInfo.ToString();
    EXPECT_FALSE(result.empty());

    nlohmann::json jsonObject = nlohmann::json::parse(result);
    EXPECT_TRUE(jsonObject.is_array());
    EXPECT_GE(jsonObject.size(), EXPECTED_BACKUP_INFO_SIZE);
    MEDIA_INFO_LOG("BackupInfo_ToString_Test_001 end");
}

HWTEST_F(PhotosBackupTest, BackupInfo_ToString_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("BackupInfo_ToString_Test_002 start");
    PhotosBackup::BackupInfo backupInfo;
    backupInfo.suffix = "_anco";
    backupInfo.photoCount = 0;
    backupInfo.videoCount = 0;
    backupInfo.cloudPhotoCount = 0;
    backupInfo.cloudVideoCount = 0;
    backupInfo.audioCount = 0;
    backupInfo.totalSize = 0;
    
    std::string result = backupInfo.ToString();
    EXPECT_FALSE(result.empty());
    
    nlohmann::json jsonObject = nlohmann::json::parse(result);
    EXPECT_TRUE(jsonObject.is_array());
    MEDIA_INFO_LOG("BackupInfo_ToString_Test_002 end");
}

HWTEST_F(PhotosBackupTest, PhotosBackup_Constructor_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("PhotosBackup_Constructor_Test_001 start");
    int32_t sceneCode = 1001;
    std::string taskId = "task_001";
    
    PhotosBackup photosBackup(sceneCode, taskId, g_rdbStore->GetRaw());
    
    std::string backupInfo = photosBackup.GetBackupInfo();
    EXPECT_FALSE(backupInfo.empty());
    MEDIA_INFO_LOG("PhotosBackup_Constructor_Test_001 end");
}

HWTEST_F(PhotosBackupTest, PhotosBackup_Constructor_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("PhotosBackup_Constructor_Test_002 start");
    int32_t sceneCode = 0;
    std::string taskId = "";
    
    PhotosBackup photosBackup(sceneCode, taskId, g_rdbStore->GetRaw());
    
    std::string backupInfo = photosBackup.GetBackupInfo();
    EXPECT_FALSE(backupInfo.empty());
    MEDIA_INFO_LOG("PhotosBackup_Constructor_Test_002 end");
}

HWTEST_F(PhotosBackupTest, GetBackupInfo_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetBackupInfo_Test_001 start");
    int32_t sceneCode = 1001;
    std::string taskId = "task_001";
    
    PhotosBackup photosBackup(sceneCode, taskId, g_rdbStore->GetRaw());
    
    std::string backupInfo = photosBackup.GetBackupInfo();
    EXPECT_FALSE(backupInfo.empty());
    
    nlohmann::json jsonObject = nlohmann::json::parse(backupInfo);
    EXPECT_TRUE(jsonObject.is_array());
    EXPECT_GE(jsonObject.size(), 1);
    EXPECT_GE(GetIntFromJsonByKey(jsonObject, "fileManagerPhoto"), 0);
    EXPECT_GE(GetIntFromJsonByKey(jsonObject, "fileManagerVideo"), 0);
    MEDIA_INFO_LOG("GetBackupInfo_Test_001 end");
}

HWTEST_F(PhotosBackupTest, GetBackupInfo_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetBackupInfo_Test_002 start");
    int32_t sceneCode = 2001;
    std::string taskId = "task_002";
    
    PhotosBackup photosBackup(sceneCode, taskId, g_rdbStore->GetRaw());
    
    std::string backupInfo = photosBackup.GetBackupInfo();
    EXPECT_FALSE(backupInfo.empty());
    
    nlohmann::json jsonObject = nlohmann::json::parse(backupInfo);
    EXPECT_TRUE(jsonObject.is_array());
    MEDIA_INFO_LOG("GetBackupInfo_Test_002 end");
}

HWTEST_F(PhotosBackupTest, GetBackupInfo_VerifyAllFields_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetBackupInfo_VerifyAllFields_Test_001 start");
    int32_t sceneCode = 1001;
    std::string taskId = "task_001";

    PhotosBackup photosBackup(sceneCode, taskId, g_rdbStore->GetRaw());

    std::string backupInfo = photosBackup.GetBackupInfo();
    EXPECT_FALSE(backupInfo.empty());

    nlohmann::json jsonObject = nlohmann::json::parse(backupInfo, nullptr, false);
    ASSERT_FALSE(jsonObject.is_discarded());
    EXPECT_TRUE(jsonObject.is_array());

    EXPECT_GE(GetIntFromJsonByKey(jsonObject, "photo"), 0);
    EXPECT_GE(GetIntFromJsonByKey(jsonObject, "video"), 0);
    EXPECT_GE(GetIntFromJsonByKey(jsonObject, "cloudPhoto"), 0);
    EXPECT_GE(GetIntFromJsonByKey(jsonObject, "cloudVideo"), 0);
    EXPECT_GE(GetIntFromJsonByKey(jsonObject, "audio"), 0);
    EXPECT_GE(GetIntFromJsonByKey(jsonObject, "totalSize"), 0);
    EXPECT_GE(GetIntFromJsonByKey(jsonObject, "ancoPhoto"), 0);
    EXPECT_GE(GetIntFromJsonByKey(jsonObject, "ancoVideo"), 0);
    EXPECT_GE(GetIntFromJsonByKey(jsonObject, "ancoTotalSize"), 0);
    EXPECT_GE(GetIntFromJsonByKey(jsonObject, "fileManagerPhoto"), 0);
    EXPECT_GE(GetIntFromJsonByKey(jsonObject, "fileManagerVideo"), 0);
    EXPECT_GE(GetIntFromJsonByKey(jsonObject, "fileManagerTotalSize"), 0);

    MEDIA_INFO_LOG("GetBackupInfo_VerifyAllFields_Test_001 end");
}

HWTEST_F(PhotosBackupTest, GetBackupInfoOfMediaFile_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetBackupInfoOfMediaFile_Test_001 start");
    int32_t sceneCode = 1001;
    std::string taskId = "task_001";
    
    PhotosBackup photosBackup(sceneCode, taskId, g_rdbStore->GetRaw());
    
    std::string backupInfo = photosBackup.GetBackupInfo();
    EXPECT_FALSE(backupInfo.empty());
    
    nlohmann::json jsonObject = nlohmann::json::parse(backupInfo);
    EXPECT_TRUE(jsonObject.is_array());
    MEDIA_INFO_LOG("GetBackupInfoOfMediaFile_Test_001 end");
}

HWTEST_F(PhotosBackupTest, GetBackupInfoOfLakeFile_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetBackupInfoOfLakeFile_Test_001 start");
    int32_t sceneCode = 1001;
    std::string taskId = "task_001";
    
    PhotosBackup photosBackup(sceneCode, taskId, g_rdbStore->GetRaw());
    
    std::string backupInfo = photosBackup.GetBackupInfo();
    EXPECT_FALSE(backupInfo.empty());
    
    nlohmann::json jsonObject = nlohmann::json::parse(backupInfo);
    EXPECT_TRUE(jsonObject.is_array());
    MEDIA_INFO_LOG("GetBackupInfoOfLakeFile_Test_001 end");
}

HWTEST_F(PhotosBackupTest, GetBackupTotalSizeOfMediaFile_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetBackupTotalSizeOfMediaFile_Test_001 start");
    int32_t sceneCode = 1001;
    std::string taskId = "task_001";
    
    PhotosBackup photosBackup(sceneCode, taskId, g_rdbStore->GetRaw());
    
    std::string backupInfo = photosBackup.GetBackupInfo();
    EXPECT_FALSE(backupInfo.empty());
    
    nlohmann::json jsonObject = nlohmann::json::parse(backupInfo);
    EXPECT_TRUE(jsonObject.is_array());
    MEDIA_INFO_LOG("GetBackupTotalSizeOfMediaFile_Test_001 end");
}

HWTEST_F(PhotosBackupTest, GetBackupTotalSizeOfLakeFile_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetBackupTotalSizeOfLakeFile_Test_001 start");
    int32_t sceneCode = 1001;
    std::string taskId = "task_001";
    
    PhotosBackup photosBackup(sceneCode, taskId, g_rdbStore->GetRaw());
    
    std::string backupInfo = photosBackup.GetBackupInfo();
    EXPECT_FALSE(backupInfo.empty());
    
    nlohmann::json jsonObject = nlohmann::json::parse(backupInfo);
    EXPECT_TRUE(jsonObject.is_array());
    MEDIA_INFO_LOG("GetBackupTotalSizeOfLakeFile_Test_001 end");
}

HWTEST_F(PhotosBackupTest, BackupInfo_LakeFields_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("BackupInfo_LakeFields_Test_001 start");
    PhotosBackup::BackupInfo backupInfo;
    backupInfo.suffix = "";
    backupInfo.photoCount = 10;
    backupInfo.videoCount = 5;
    backupInfo.cloudPhotoCount = 3;
    backupInfo.cloudVideoCount = 2;
    backupInfo.audioCount = 7;
    backupInfo.totalSize = 1024000;
    backupInfo.lakePhotoCount = 15;
    backupInfo.lakeVideoCount = 8;
    backupInfo.lakeTotalSize = 2048000;
    
    std::string result = backupInfo.ToString();
    EXPECT_FALSE(result.empty());
    
    nlohmann::json jsonObject = nlohmann::json::parse(result);
    ASSERT_FALSE(jsonObject.is_discarded());
    EXPECT_TRUE(jsonObject.is_array());
    EXPECT_GE(jsonObject.size(), 9);
    MEDIA_INFO_LOG("BackupInfo_LakeFields_Test_001 end");
}

HWTEST_F(PhotosBackupTest, BackupInfo_LakeFields_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("BackupInfo_LakeFields_Test_002 start");
    PhotosBackup::BackupInfo backupInfo;
    backupInfo.suffix = "_lake";
    backupInfo.lakePhotoCount = 0;
    backupInfo.lakeVideoCount = 0;
    backupInfo.lakeTotalSize = 0;
    
    std::string result = backupInfo.ToString();
    EXPECT_FALSE(result.empty());
    
    nlohmann::json jsonObject = nlohmann::json::parse(result);
    ASSERT_FALSE(jsonObject.is_discarded());
    EXPECT_TRUE(jsonObject.is_array());
    MEDIA_INFO_LOG("BackupInfo_LakeFields_Test_002 end");
}

HWTEST_F(PhotosBackupTest, BackupInfo_FileManagerFields_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("BackupInfo_FileManagerFields_Test_001 start");
    PhotosBackup::BackupInfo backupInfo;
    backupInfo.suffix = "";
    backupInfo.photoCount = 10;
    backupInfo.videoCount = 5;
    backupInfo.cloudPhotoCount = 3;
    backupInfo.cloudVideoCount = 2;
    backupInfo.audioCount = 7;
    backupInfo.totalSize = 1024000;
    backupInfo.lakePhotoCount = 15;
    backupInfo.lakeVideoCount = 8;
    backupInfo.lakeTotalSize = 2048000;
    backupInfo.fileManagerPhotoCount = 20;
    backupInfo.fileManagerVideoCount = 10;
    backupInfo.fileManagerTotalSize = 3072000;

    std::string result = backupInfo.ToString();
    EXPECT_FALSE(result.empty());

    nlohmann::json jsonObject = nlohmann::json::parse(result, nullptr, false);
    ASSERT_FALSE(jsonObject.is_discarded());
    EXPECT_TRUE(jsonObject.is_array());
    EXPECT_GE(jsonObject.size(), 12);
    EXPECT_EQ(GetIntFromJsonByKey(jsonObject, "fileManagerPhoto"), backupInfo.fileManagerPhotoCount);
    EXPECT_EQ(GetIntFromJsonByKey(jsonObject, "fileManagerVideo"), backupInfo.fileManagerVideoCount);
    EXPECT_EQ(GetIntFromJsonByKey(jsonObject, "fileManagerTotalSize"), static_cast<int64_t>(backupInfo.fileManagerTotalSize));
    MEDIA_INFO_LOG("BackupInfo_FileManagerFields_Test_001 end");
}

HWTEST_F(PhotosBackupTest, BackupInfo_FileManagerFields_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("BackupInfo_FileManagerFields_Test_002 start");
    PhotosBackup::BackupInfo backupInfo;
    backupInfo.suffix = "_fileManager";
    backupInfo.fileManagerPhotoCount = 0;
    backupInfo.fileManagerVideoCount = 0;
    backupInfo.fileManagerTotalSize = 0;

    std::string result = backupInfo.ToString();
    EXPECT_FALSE(result.empty());

    nlohmann::json jsonObject = nlohmann::json::parse(result, nullptr, false);
    ASSERT_FALSE(jsonObject.is_discarded());
    EXPECT_TRUE(jsonObject.is_array());
    MEDIA_INFO_LOG("BackupInfo_FileManagerFields_Test_002 end");
}

HWTEST_F(PhotosBackupTest, CreateCloneFileInfoDb_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("CreateCloneFileInfoDb_Test_001 start");
    int32_t sceneCode = 1001;
    std::string taskId = "task_001";
    
    PhotosBackup photosBackup(sceneCode, taskId, g_rdbStore->GetRaw());
    
    int32_t result = photosBackup.CreateCloneFileInfoDb(AncoFileListClone::ANCO_FILE_LIST_CLONE_NONE,
        FileManagerFileListClone::FILE_MANAGER_FILE_LIST_CLONE_NONE);
    EXPECT_EQ(result, NativeRdb::E_OK);
    MEDIA_INFO_LOG("CreateCloneFileInfoDb_Test_001 end");
}

HWTEST_F(PhotosBackupTest, CreateCloneFileInfoDb_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("CreateCloneFileInfoDb_Test_002 start");
    int32_t sceneCode = 1001;
    std::string taskId = "task_002";
    
    PhotosBackup photosBackup(sceneCode, taskId, g_rdbStore->GetRaw());
    
    int32_t result = photosBackup.CreateCloneFileInfoDb(AncoFileListClone::ANCO_FILE_LIST_CLONE_SUPPORTED,
        FileManagerFileListClone::FILE_MANAGER_FILE_LIST_CLONE_NONE);
    EXPECT_EQ(result, NativeRdb::E_OK);
    MEDIA_INFO_LOG("CreateCloneFileInfoDb_Test_002 end");
}

HWTEST_F(PhotosBackupTest, CreateCloneFileInfoDb_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("CreateCloneFileInfoDb_Test_003 start");
    int32_t sceneCode = 1001;
    std::string taskId = "task_003";
    
    PhotosBackup photosBackup(sceneCode, taskId, g_rdbStore->GetRaw());
    
    int32_t result = photosBackup.CreateCloneFileInfoDb(AncoFileListClone::ANCO_FILE_LIST_CLONE_NONE,
        FileManagerFileListClone::FILE_MANAGER_FILE_LIST_CLONE_SUPPORTED);
    EXPECT_EQ(result, NativeRdb::E_OK);
    MEDIA_INFO_LOG("CreateCloneFileInfoDb_Test_003 end");
}

HWTEST_F(PhotosBackupTest, CreateCloneFileInfoDb_Test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("CreateCloneFileInfoDb_Test_004 start");
    int32_t sceneCode = 1001;
    std::string taskId = "task_004";
    
    PhotosBackup photosBackup(sceneCode, taskId, g_rdbStore->GetRaw());
    
    int32_t result = photosBackup.CreateCloneFileInfoDb(AncoFileListClone::ANCO_FILE_LIST_CLONE_SUPPORTED,
        FileManagerFileListClone::FILE_MANAGER_FILE_LIST_CLONE_SUPPORTED);
    EXPECT_EQ(result, NativeRdb::E_OK);
    MEDIA_INFO_LOG("CreateCloneFileInfoDb_Test_004 end");
}
}  // namespace OHOS::Media