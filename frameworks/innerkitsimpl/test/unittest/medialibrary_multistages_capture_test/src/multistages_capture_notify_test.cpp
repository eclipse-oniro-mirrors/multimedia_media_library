/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MultiStagesCaptureNotifyUnitTest"

#include "multistages_capture_notify_test.h"

#include "media_log.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_operation.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "multistages_capture_dao.h"
#include "multistages_capture_notify.h"
#include "userfile_manager_types.h"
#include "media_upgrade.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;
using namespace testing;
using namespace OHOS::Media::Notification;

namespace OHOS {
namespace Media {
static std::shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;

void SetTables()
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

void CleanTestTables()
{
    std::vector<std::string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
    };
    for (auto &dropTable : dropTableList) {
        string dropSql = "DROP TABLE " + dropTable + ";";
        int32_t ret = g_rdbStore->ExecuteSql(dropSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Drop %{public}s table failed", dropTable.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Drop %{public}s table success", dropTable.c_str());
    }
}

void ClearAndRestart()
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    CleanTestTables();
    SetTables();
}

void MultiStagesCaptureNotifyTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    SetTables();
}

void MultiStagesCaptureNotifyTest::TearDownTestCase(void)
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    system("rm -rf /storage/cloud/files/*");
    ClearAndRestart();
    g_rdbStore = nullptr;
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    this_thread::sleep_for(chrono::seconds(SLEEP_FIVE_SECONDS));
    MEDIA_INFO_LOG("Clean is finish");
}

// SetUp:Execute before each test case
void MultiStagesCaptureNotifyTest::SetUp()
{
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    ClearAndRestart();
}

void MultiStagesCaptureNotifyTest::TearDown(void) {}

void InsertAsset(PhotoAsset &asset)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    EXPECT_NE(rdbStore, nullptr);

    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutString(MediaColumn::MEDIA_FILE_PATH, asset.path);
    valuesBucket.PutString(MediaColumn::MEDIA_NAME, asset.displayName);
    valuesBucket.PutInt(MediaColumn::MEDIA_TYPE, asset.mediaType);

    int32_t ret = rdbStore->Insert(asset.fileId, PhotoColumn::PHOTOS_TABLE, valuesBucket);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: NotifyOnProcess_test01
 * @tc.desc: OnProcess通知，resultSet不能为空
 */
HWTEST_F(MultiStagesCaptureNotifyTest, NotifyOnProcess_test01, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter NotifyOnProcess_test01");
    shared_ptr<FileAsset> fileAsset = nullptr;
    int32_t ret = MultistagesCaptureNotify::NotifyOnProcess(
        fileAsset, MultistagesCaptureNotifyType::ON_PROCESS_IMAGE_DONE);
    ASSERT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("end NotifyOnProcess_test01");
}

/**
 * @tc.name: NotifyOnProcess_test02
 * @tc.desc: OnProcess通知，ObserverType不能是未定义状态
 */
HWTEST_F(MultiStagesCaptureNotifyTest, NotifyOnProcess_test02, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter NotifyOnProcess_test02");
    PhotoAsset asset = {
        .path = "/storage/cloud/files/Photo/1/IMG_1764748333_000.jpg",
        .displayName = "test.jpg",
        .mediaType = static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE),
    };
    InsertAsset(asset);
    EXPECT_GT(asset.fileId, -1);

    const std::vector<std::string> columns = {
        MediaColumn::MEDIA_ID,
        MediaColumn::MEDIA_FILE_PATH,
        MediaColumn::MEDIA_NAME,
        MediaColumn::MEDIA_TYPE,
    };
    auto fileAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(
        MediaColumn::MEDIA_ID, std::to_string(asset.fileId), OperationObject::FILESYSTEM_PHOTO, columns);
    ASSERT_NE(fileAsset, nullptr);

    int32_t ret = MultistagesCaptureNotify::NotifyOnProcess(fileAsset, MultistagesCaptureNotifyType::UNDEFINED);
    ASSERT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("end NotifyOnProcess_test02");
}

/**
 * @tc.name: NotifyOnProcess_test03
 * @tc.desc: OnProcess通知
 */
HWTEST_F(MultiStagesCaptureNotifyTest, NotifyOnProcess_test03, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter NotifyOnProcess_test03");
    PhotoAsset asset = {
        .path = "/storage/cloud/files/Photo/1/IMG_1764748333_000.jpg",
        .displayName = "test.jpg",
        .mediaType = static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE),
    };
    InsertAsset(asset);
    EXPECT_GT(asset.fileId, -1);

    const std::vector<std::string> columns = {
        MediaColumn::MEDIA_ID,
        MediaColumn::MEDIA_FILE_PATH,
        MediaColumn::MEDIA_NAME,
        MediaColumn::MEDIA_TYPE,
    };
    auto fileAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(
        MediaColumn::MEDIA_ID, std::to_string(asset.fileId), OperationObject::FILESYSTEM_PHOTO, columns);
    ASSERT_NE(fileAsset, nullptr);

    int32_t ret = MultistagesCaptureNotify::NotifyOnProcess(
        fileAsset, MultistagesCaptureNotifyType::ON_PROCESS_IMAGE_DONE);
    ASSERT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end NotifyOnProcess_test03");
}
} // namespace Media
} // namespace OHOS