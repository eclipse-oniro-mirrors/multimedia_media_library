/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MediaLibraryPtpOperationsUnitTest"

#include "medialibrary_ptp_operations_test.h"

#include <cstdlib>
#include <thread>
#include <fcntl.h>

#include "medialibrary_ptp_operations.h"
#include "media_column.h"
#include "photo_album_column.h"
#include "get_self_permissions.h"
#include "base_data_uri.h"
#include "system_ability_definition.h"
#include "datashare_helper.h"
#include "fetch_result.h"
#include "file_asset.h"
#include "file_uri.h"
#include "hilog/log.h"
#include "userfilemgr_uri.h"
#include "iservice_registry.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "media_file_utils.h"
#include "media_library_manager.h"
#include "medialibrary_data_manager.h"
#include "media_log.h"
#include "media_volume.h"
#include "scanner_utils.h"
#include "userfile_client.h"
#include "media_asset_impl.h"
#include "directory_ex.h"
#include "ptp_medialibrary_manager_uri.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"

using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::NativeRdb;
using namespace OHOS::AppExecFwk;
namespace OHOS {
namespace Media {
static std::shared_ptr<DataShare::DataShareHelper> g_DataShareHelper = nullptr;
static shared_ptr<MediaLibraryRdbStore> g_RdbStore = nullptr;

int32_t ExecSqls(const vector<string> &sqls)
{
    EXPECT_NE((g_RdbStore == nullptr), true);
    int32_t err = E_OK;
    for (const auto &sql : sqls) {
        err = g_RdbStore->ExecuteSql(sql);
        MEDIA_INFO_LOG("exec sql: %{public}s result: %{public}d", sql.c_str(), err);
        EXPECT_EQ(err, E_OK);
    }
    return E_OK;
}

void ClearTables()
{
    string clearPhoto = "DELETE FROM " + PhotoColumn::PHOTOS_TABLE;
    string clearAlbum = "DELETE FROM " + PhotoAlbumColumns::TABLE;
    vector<string> executeSqls;
    executeSqls.push_back(clearPhoto);
    executeSqls.push_back(clearAlbum);
    MEDIA_INFO_LOG("start clear data");
    ExecSqls(executeSqls);
}

void MediaLibraryPtpOperationsUnitTest::SetUpTestCase(void)
{
    std::vector<string> perms;
    perms.push_back("ohos.permission.READ_IMAGEVIDEO");
    perms.push_back("ohos.permission.WRITE_IMAGEVIDEO");
    uint64_t tokenId = 0;
    PermissionUtilsUnitTest::SetAccessTokenPermission("MediaLibraryPtpOperationsUnitTest", perms, tokenId);
    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (saManager == nullptr) {
        MEDIA_ERR_LOG("GetSystemAbilityManager Service Failed.");
        return;
    }
    auto remoteObj = saManager->GetSystemAbility(5003);
    if (remoteObj == nullptr) {
        MEDIA_ERR_LOG("GetSystemAbility Service Failed.");
        return;
    }
    if (g_DataShareHelper == nullptr) {
        const sptr<IRemoteObject> &token = remoteObj;
        g_DataShareHelper = DataShare::DataShareHelper::Creator(token, MEDIALIBRARY_DATA_URI);
    }
    if (g_DataShareHelper == nullptr) {
        MEDIA_ERR_LOG("g_DataShareHelper Get Failed.");
        exit(1);
    }
    MediaLibraryUnitTestUtils::Init();
    g_RdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_RdbStore == nullptr) {
        MEDIA_ERR_LOG("g_RdbStore Get Failed.");
        exit(1);
    }
}

void MediaLibraryPtpOperationsUnitTest::TearDownTestCase(void)
{
    ClearTables();
}

void MediaLibraryPtpOperationsUnitTest::SetUp()
{
    ClearTables();
}

void MediaLibraryPtpOperationsUnitTest::TearDown(void) {}

static void InsertPtpTestData(NativeRdb::ValuesBucket valuesBucket)
{
    MEDIA_INFO_LOG("InsertPtpTestData start");
    int64_t outRow = -1;
    Uri uri(MEDIALIBRARY_DATA_URI + "/" + CONST_PTP_OPERATION + "/" + OPRN_CREATE);
    int32_t ret = g_RdbStore->Insert(outRow, PhotoColumn::PHOTOS_TABLE, valuesBucket);
    MEDIA_INFO_LOG("insert test data outRow:%{public}d", ret);
}

static void InsertPtpAlbumTestData(NativeRdb::ValuesBucket valuesBucket)
{
    MEDIA_INFO_LOG("InsertPtpAlbumTestData start");
    int64_t outRow = -1;
    Uri uri(MEDIALIBRARY_DATA_URI + "/" + CONST_PTP_ALBUM_OPERATION + "/" + OPRN_CREATE);
    int32_t ret = g_RdbStore->Insert(outRow, PhotoAlbumColumns::TABLE, valuesBucket);
    MEDIA_INFO_LOG("insert album test data ret:%{public}d", ret);
}

HWTEST_F(MediaLibraryPtpOperationsUnitTest, mediaLibraryPtpOperationsUnitTest_001, TestSize.Level1)
{
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, "1");
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.Put(PhotoColumn::MEDIA_ID, 1);
    InsertPtpTestData(valuesBucket);
    NativeRdb::RdbPredicates rdbPredicates = RdbDataShareAdapter::RdbUtils::ToPredicates(predicates,
        PhotoColumn::PHOTOS_TABLE);
    int32_t ret = MediaLibraryPtpOperations::DeletePtpPhoto(rdbPredicates);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaLibraryPtpOperationsUnitTest, mediaLibraryPtpOperationsUnitTest_002, TestSize.Level1)
{
    OHOS::DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, "100");
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.Put(PhotoAlbumColumns::ALBUM_ID, 100);
    InsertPtpAlbumTestData(valuesBucket);
    NativeRdb::RdbPredicates rdbPredicates = RdbDataShareAdapter::RdbUtils::ToPredicates(predicates,
        PhotoAlbumColumns::TABLE);
    int32_t ret = MediaLibraryPtpOperations::DeletePtpAlbum(rdbPredicates);
    EXPECT_EQ(ret, E_OK);
}
} // namespace Media
} // namespace OHOS