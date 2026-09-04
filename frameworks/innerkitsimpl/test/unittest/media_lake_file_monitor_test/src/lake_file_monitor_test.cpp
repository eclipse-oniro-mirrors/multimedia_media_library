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
 
#define MLOG_TAG "LakeFileMonitorTest"

#include "check_scene_helper.h"
#include "lake_file_monitor_test.h"

#include "media_log.h"
#include "media_file_notify_info.h"
#include "media_file_change_manager.h"
#include "media_file_change_processor.h"
#include "media_lake_clone_event_manager.h"
#include "file_monitor_interface.h"
#include "file_scan_utils.h"
#include "common_event_support.h"
#include "want.h"
 
namespace OHOS {
namespace Media {
using namespace testing::ext;

void LakeFileMonitorTest::SetUpTestCase() {}

void LakeFileMonitorTest::TearDownTestCase() {}

void LakeFileMonitorTest::SetUp() {}

void LakeFileMonitorTest::TearDown() {}

/**
 * @tc.number    : media_file_change_manager_test_001
 * @tc.name      : MediaFileChangeManager test
 * @tc.desc      : MediaFileChangeManager test
 */
HWTEST_F(LakeFileMonitorTest, media_file_change_manager_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("media_file_change_manager_test_001 start");

    auto manager = MediaFileChangeManager::GetInstance();
    auto processor = MediaFileChangeProcessor::GetInstance();
    EXPECT_NE(processor->fileMonitorProxy_, nullptr);
    manager->StartProcessChangeData();
    EXPECT_TRUE(processor->shouldProcessMsg_);
    manager->StopProcessChangeData();
    EXPECT_FALSE(processor->shouldProcessMsg_);

    MEDIA_INFO_LOG("media_file_change_manager_test_001 end");
}

/**
 * @tc.number    : media_file_change_processor_test_001
 * @tc.name      : MediaFileChangeProcessor::ProcessSingleFileChange test
 * @tc.desc      : MediaFileChangeProcessor::ProcessSingleFileChange test
 */
HWTEST_F(LakeFileMonitorTest, media_file_change_processor_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("media_file_change_processor_test_001 start");

    auto manager = MediaFileChangeManager::GetInstance();
    auto processor = MediaFileChangeProcessor::GetInstance();
    string logicPathPrefixInLake = FileScanUtils::GetCurrentInLakeLogicPrefix();
    string uri = logicPathPrefixInLake + "/Pictures/test";
    FileMonitorService::FileMsgModel model;
    

    processor->StartProcessMsgs();
    EXPECT_TRUE(processor->shouldProcessMsg_);
    model.isFile = true;
    model.opType = -1;
    processor->ProcessSingleFileChange(model);

    model.opType = 0;
    processor->ProcessSingleFileChange(model);

    model.isFile = false;
    model.opType = 2;
    model.fileUri = uri;
    processor->ProcessSingleFileChange(model);

    model.opType = 1;
    processor->ProcessSingleFileChange(model);

    model.opType = 1;
    model.fileUri = "";
    processor->ProcessSingleFileChange(model);
    processor->StopProcessMsgs();
    EXPECT_FALSE(processor->shouldProcessMsg_);

    MEDIA_INFO_LOG("media_file_change_processor_test_001 end");
}

/**
 * @tc.number    : media_lake_clone_event_manager_test_001
 * @tc.name      : MediaLakeCloneEventManager test
 * @tc.desc      : MediaLakeCloneEventManager test
 */
HWTEST_F(LakeFileMonitorTest, media_lake_clone_event_manager_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("media_lake_clone_event_manager_test_001 start");
 
    AAFwk::Want want;
    string action = EventFwk::CommonEventSupport::COMMON_EVENT_RESTORE_START;
    want = want.SetAction(action);
    string bundleName = "com.ohos.medialibrary.medialibrarydata";
    want.SetBundle(bundleName);
    want = want.SetParam("bundleName", bundleName);
 
    bool ret = MediaLakeCloneEventManager::IsRestoreEvent(want);
    EXPECT_TRUE(ret);
    ret = MediaLakeCloneEventManager::GetInstance().IsRestoring();
    EXPECT_FALSE(ret);
 
    MEDIA_INFO_LOG("media_lake_clone_event_manager_test_001 end");
}
 
/**
 * @tc.number    : media_lake_clone_event_manager_test_002
 * @tc.name      : MediaLakeCloneEventManager test
 * @tc.desc      : MediaLakeCloneEventManager test
 */
HWTEST_F(LakeFileMonitorTest, media_lake_clone_event_manager_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("media_lake_clone_event_manager_test_002 start");
 
    AAFwk::Want want;
    string action = EventFwk::CommonEventSupport::COMMON_EVENT_RESTORE_START;
    want = want.SetAction(action);
    string bundleName = "com.ohos.medialibrary.medialibrarydata";
    want = want.SetParam("bundleName", bundleName);
 
    MediaLakeCloneEventManager::GetInstance().HandleRestoreEvent(want);
    EXPECT_NE(MediaLakeCloneEventManager::GetInstance().currentRestoreStatusBitMap_, 0);
    MediaLakeCloneEventManager::GetInstance().HandleDeathRecipient();
    EXPECT_EQ(MediaLakeCloneEventManager::GetInstance().currentRestoreStatusBitMap_, 0);
 
    MEDIA_INFO_LOG("media_lake_clone_event_manager_test_002 end");
}
 
/**
 * @tc.number    : media_lake_clone_event_manager_test_003
 * @tc.name      : MediaLakeCloneEventManager test
 * @tc.desc      : MediaLakeCloneEventManager test
 */
HWTEST_F(LakeFileMonitorTest, media_lake_clone_event_manager_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("media_lake_clone_event_manager_test_003 start");
 
    MediaLakeCloneDeathRecipient recipient;
    wptr<IRemoteObject> object;
    recipient.OnRemoteDied(object);
    AAFwk::Want want;
    string action = EventFwk::CommonEventSupport::COMMON_EVENT_RESTORE_START;
    want = want.SetAction(action);
    string bundleName = "com.huawei.hmos.filemanager";
    want = want.SetParam("bundleName", bundleName);
 
    MediaLakeCloneEventManager::GetInstance().HandleRestoreEvent(want);
    EXPECT_NE(MediaLakeCloneEventManager::GetInstance().currentRestoreStatusBitMap_, 0);
    action = EventFwk::CommonEventSupport::COMMON_EVENT_RESTORE_END;
    want.SetAction(action);
    MediaLakeCloneEventManager::GetInstance().HandleRestoreEvent(want);
    EXPECT_EQ(MediaLakeCloneEventManager::GetInstance().currentRestoreStatusBitMap_, 0);
 
    MEDIA_INFO_LOG("media_lake_clone_event_manager_test_003 end");
}

HWTEST_F(LakeFileMonitorTest, file_scan_utils_is_path_under_root_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("file_scan_utils_is_path_under_root_test_001 start");

    // empty rootPath should return false
    EXPECT_FALSE(FileScanUtils::IsPathUnderRoot("/var/log/file", ""));

    MEDIA_INFO_LOG("file_scan_utils_is_path_under_root_test_001 end");
}

HWTEST_F(LakeFileMonitorTest, file_scan_utils_is_path_under_root_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("file_scan_utils_is_path_under_root_test_002 start");

    // rootPath ends with '/', path under root should return true
    EXPECT_TRUE(FileScanUtils::IsPathUnderRoot("/var/log/file", "/var/log/"));

    MEDIA_INFO_LOG("file_scan_utils_is_path_under_root_test_002 end");
}

HWTEST_F(LakeFileMonitorTest, file_scan_utils_is_path_under_root_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("file_scan_utils_is_path_under_root_test_003 start");

    // rootPath without trailing '/', path under root should return true
    EXPECT_TRUE(FileScanUtils::IsPathUnderRoot("/var/log/file", "/var/log"));

    MEDIA_INFO_LOG("file_scan_utils_is_path_under_root_test_003 end");
}

HWTEST_F(LakeFileMonitorTest, file_scan_utils_is_path_under_root_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("file_scan_utils_is_path_under_root_test_004 start");

    // path equals rootPath should return true
    EXPECT_TRUE(FileScanUtils::IsPathUnderRoot("/var/log", "/var/log"));

    MEDIA_INFO_LOG("file_scan_utils_is_path_under_root_test_004 end");
}

HWTEST_F(LakeFileMonitorTest, file_scan_utils_is_path_under_root_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("file_scan_utils_is_path_under_root_test_005 start");

    // path not under root should return false
    EXPECT_FALSE(FileScanUtils::IsPathUnderRoot("/var/tmp/file", "/var/log"));

    MEDIA_INFO_LOG("file_scan_utils_is_path_under_root_test_005 end");
}

HWTEST_F(LakeFileMonitorTest, file_scan_utils_is_path_under_root_test_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("file_scan_utils_is_path_under_root_test_006 start");

    // case insensitive matching should return true
    EXPECT_TRUE(FileScanUtils::IsPathUnderRoot("/VAR/LOG/file", "/var/log"));

    MEDIA_INFO_LOG("file_scan_utils_is_path_under_root_test_006 end");
}

HWTEST_F(LakeFileMonitorTest, file_scan_utils_is_path_under_root_test_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("file_scan_utils_is_path_under_root_test_007 start");

    // rootPath ends with '/' but path not under root should return false
    EXPECT_FALSE(FileScanUtils::IsPathUnderRoot("/var/tmp/file", "/var/log/"));

    MEDIA_INFO_LOG("file_scan_utils_is_path_under_root_test_007 end");
}

void TestResolveSceneByPath(const std::string &path, CheckScene expectedScene)
{
    EXPECT_EQ(CheckSceneHelper::ResolveSceneByPath(path), expectedScene);
}

HWTEST_F(LakeFileMonitorTest, ResolveSceneByPath_test_001, TestSize.Level1)
{
    TestResolveSceneByPath("/storage/media/local/files/Docs/HO_DATA_EXT_MISC/test/photo.jpg", CheckScene::LAKE);
    TestResolveSceneByPath("/storage/media/local/files/Docs/document.pdf", CheckScene::FILE_MANAGER);
    TestResolveSceneByPath("/data/local/tmp/other_file.txt", CheckScene::UNKNOWN);
}
} // namespace Media
} // namespace OHOS