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

#define MLOG_TAG "MediaDatashareExtAbilityUnitTest"

#include "media_datashare_ext_ability_test.h"

#include <chrono>
#include <thread>

#include "ability_context_impl.h"
#include "data_ability_observer_interface.h"
#include "datashare_business_error.h"
#include "datashare_helper.h"
#include "datashare_predicates.h"
#include "datashare_values_bucket.h"
#include "get_self_permissions.h"
#include "media_datashare_ext_ability.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_db_const.h"
#include "medialibrary_db_const_sqls.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "media_column.h"
#include "media_datashare_stub_impl.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "runtime.h"
#include "userfilemgr_uri.h"
#include "medialibrary_data_manager.h"
#include "cloud_media_asset_uri.h"
#include "mediatool_uri.h"
#include "cloud_enhancement_uri.h"
#include "album_operation_uri.h"
#include "data_secondary_directory_uri.h"
#include "medialibrary_upgrade_utils.h"
#include "media_upgrade.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::AbilityRuntime;
using namespace OHOS::DataShare;
namespace OHOS {
namespace Media {
static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static const int32_t SLEEP_FOR_FIFTY_MS = 50;
static const std::vector<std::string> EXTENSION_URI_LISTS = {
    // API9 compat photo operations constants
    CONST_URI_CREATE_PHOTO,
    CONST_URI_CLOSE_PHOTO,
    CONST_URI_UPDATE_PHOTO,
    CONST_URI_QUERY_PHOTO,

    // API9 compat audio operations constants
    CONST_URI_QUERY_AUDIO,
    CONST_URI_CLOSE_AUDIO,
    CONST_URI_UPDATE_AUDIO,
    CONST_URI_CREATE_AUDIO,
    CONST_URI_CLOSE_FILE,
    CONST_URI_UPDATE_FILE,
    CONST_URI_CREATE_FILE,

    // Photo album operations constants
    CONST_URI_QUERY_PHOTO_ALBUM,
    CONST_URI_DELETE_PHOTOS,
    CONST_URI_COMPAT_DELETE_PHOTOS,

    // Photo map operations constants
    CONST_URI_QUERY_PHOTO_MAP,

    // Scanner tool operation constants
    CONST_URI_SCANNER,

    // Mediatool delete operation constants
    CONST_URI_DELETE_TOOL,

    // UserFileManager photo operation constants
    CONST_UFM_CREATE_PHOTO,
    CONST_UFM_CREATE_PHOTO_COMPONENT,
    CONST_UFM_CLOSE_PHOTO,
    CONST_UFM_UPDATE_PHOTO,
    CONST_UFM_QUERY_PHOTO,
    CONST_UFM_SET_USER_COMMENT,
    CONST_UFM_GET_INDEX,
    CONST_UFM_HIDE_PHOTO,

    // UserFileManager audio operation constants
    CONST_UFM_CREATE_AUDIO,
    CONST_UFM_CREATE_AUDIO_COMPONENT,
    CONST_UFM_CLOSE_AUDIO,
    CONST_UFM_QUERY_AUDIO,
    CONST_UFM_UPDATE_AUDIO,
    CONST_URI_DELETE_AUDIO,

    // UserFileManager album operation constants
    CONST_UFM_CREATE_PHOTO_ALBUM,
    CONST_UFM_DELETE_PHOTO_ALBUM,
    CONST_UFM_UPDATE_PHOTO_ALBUM,
    CONST_UFM_QUERY_PHOTO_ALBUM,
    CONST_UFM_QUERY_HIDDEN_ALBUM,
    CONST_UFM_PHOTO_ALBUM_ADD_ASSET,
    CONST_UFM_PHOTO_ALBUM_REMOVE_ASSET,
    CONST_UFM_QUERY_PHOTO_MAP,
    CONST_UFM_RECOVER_PHOTOS,
    CONST_UFM_DELETE_PHOTOS,

    // PhotoAccessHelper photo operation constants
    CONST_PAH_CREATE_PHOTO,
    CONST_PAH_CREATE_PHOTO_COMPONENT,
    CONST_PAH_CLOSE_PHOTO,
    CONST_PAH_UPDATE_PHOTO,
    CONST_PAH_UPDATE_PHOTO_COMPONENT,
    CONST_PAH_TRASH_PHOTO,
    CONST_PAH_QUERY_PHOTO,
    CONST_PAH_EDIT_USER_COMMENT_PHOTO,
    CONST_PAH_HIDE_PHOTOS,
    CONST_PAH_SUBMIT_CACHE,
    CONST_PAH_ADD_FILTERS,
    CONST_PAH_BATCH_UPDATE_FAVORITE,
    CONST_PAH_BATCH_UPDATE_USER_COMMENT,
    CONST_PAH_BATCH_UPDATE_OWNER_ALBUM_ID,
    CONST_PAH_GET_ANALYSIS_INDEX,
    CONST_PAH_DISCARD_CAMERA_PHOTO,
    CONST_PAH_SAVE_CAMERA_PHOTO,
    CONST_PAH_SCAN_WITHOUT_ALBUM_UPDATE,
    CONST_PATH_SAVE_PICTURE,

    // MultiStages capture related operation uri
    CONST_PAH_SET_PHOTO_QUALITY,
    CONST_PAH_PROCESS_IMAGE,
    CONST_PAH_ADD_IMAGE,
    CONST_PAH_SET_LOCATION,
    CONST_PAH_CANCEL_PROCESS_IMAGE,
    CONST_PAH_REMOVE_MSC_TASK,

    // Generate thumbnails in batches operation uri
    CONST_PAH_START_GENERATE_THUMBNAILS,
    CONST_PAH_STOP_GENERATE_THUMBNAILS,
    CONST_PAH_ADD_LOWQUALITY_IMAGE,

    // PhotoAccessHelper album operation constants
    CONST_PAH_CREATE_PHOTO_ALBUM,
    CONST_PAH_DELETE_PHOTO_ALBUM,
    CONST_PAH_UPDATE_PHOTO_ALBUM,
    CONST_PAH_SET_PHOTO_ALBUM_NAME,
    CONST_PAH_QUERY_PHOTO_ALBUM,
    CONST_PAH_QUERY_HIDDEN_ALBUM,
    CONST_PAH_PHOTO_ALBUM_ADD_ASSET,
    CONST_PAH_PHOTO_ALBUM_REMOVE_ASSET,
    CONST_PAH_QUERY_PHOTO_MAP,
    CONST_PAH_RECOVER_PHOTOS,
    CONST_PAH_DELETE_PHOTOS,
    CONST_PAH_ORDER_ALBUM,
    CONST_PAH_COMMIT_EDIT_PHOTOS,
    CONST_PAH_REVERT_EDIT_PHOTOS,
    CONST_PAH_PORTRAIT_DISPLAY_LEVLE,
    CONST_PAH_PORTRAIT_IS_ME,
    CONST_PAH_PORTRAIT_ANAALBUM_ALBUM_NAME,
    CONST_PAH_PORTRAIT_MERGE_ALBUM,
    CONST_PAH_DISMISS_ASSET,
    CONST_PAH_PORTRAIT_ANAALBUM_COVER_URI,
    CONST_PAH_GROUP_ANAALBUM_DISMISS,
    CONST_PAH_GROUP_ANAALBUM_ALBUM_NAME,
    CONST_PAH_GROUP_ANAALBUM_COVER_URI,

    CONST_PAH_QUERY_ANA_PHOTO_ALBUM,
    CONST_PAH_QUERY_ANA_PHOTO_MAP,
    CONST_PAH_INSERT_ANA_PHOTO_ALBUM,
    CONST_PAH_UPDATE_ANA_PHOTO_ALBUM,
    CONST_PAH_INSERT_ANA_PHOTO_MAP,

    CONST_PAH_QUERY_ANA_OCR,
    CONST_PAH_QUERY_ANA_ATTS,
    CONST_PAH_QUERY_ANA_LABEL,
    CONST_PAH_QUERY_ANA_VIDEO_LABEL,
    CONST_PAH_QUERY_ANA_FACE,
    CONST_PAH_QUERY_ANA_FACE_TAG,
    CONST_PAH_QUERY_ANA_OBJECT,
    CONST_PAH_QUERY_ANA_RECOMMENDATION,
    CONST_PAH_QUERY_ANA_SEGMENTATION,
    CONST_PAH_QUERY_ANA_COMPOSITION,
    CONST_PAH_QUERY_ANA_HEAD,
    CONST_PAH_QUERY_ANA_POSE,
    CONST_PAH_STORE_FORM_MAP,
    CONST_PAH_REMOVE_FORM_MAP,
    CONST_PAH_QUERY_ANA_SAL,
    CONST_PAH_QUERY_ANA_ADDRESS,
    CONST_PAH_QUERY_GEO_PHOTOS,
    CONST_PAH_QUERY_HIGHLIGHT_COVER,
    CONST_PAH_QUERY_HIGHLIGHT_PLAY,
    CONST_PAH_QUERY_ANA_TOTAL,
    CONST_PAH_QUERY_MULTI_CROP,
    CONST_PAH_UPDATE_ANA_FACE,

    // PhotoAccessHelper moving photo
    CONST_PAH_MOVING_PHOTO_SCAN,

    // PhotoAccessHelper cloud enhancement
    CONST_PAH_CLOUD_ENHANCEMENT_ADD,
    CONST_PAH_CLOUD_ENHANCEMENT_PRIORITIZE,
    CONST_PAH_CLOUD_ENHANCEMENT_CANCEL,
    CONST_PAH_CLOUD_ENHANCEMENT_CANCEL_ALL,
    CONST_PAH_CLOUD_ENHANCEMENT_SYNC,
    CONST_PAH_CLOUD_ENHANCEMENT_QUERY,
    CONST_PAH_CLOUD_ENHANCEMENT_GET_PAIR,

    // mediatool operation constants
    CONST_TOOL_CREATE_PHOTO,
    CONST_TOOL_CREATE_AUDIO,
    CONST_TOOL_CLOSE_PHOTO,
    CONST_TOOL_CLOSE_AUDIO,
    CONST_TOOL_QUERY_PHOTO,
    CONST_TOOL_QUERY_AUDIO,
    CONST_TOOL_LIST_PHOTO,
    CONST_TOOL_LIST_AUDIO,
    CONST_TOOL_UPDATE_PHOTO,
    CONST_TOOL_UPDATE_AUDIO,
    CONST_TOOL_DELETE_PHOTO,
    CONST_TOOL_DELETE_AUDIO,

    // Miscellaneous operation constants
    CONST_LOG_MOVING_PHOTO,
    CONST_PAH_FINISH_REQUEST_PICTURE,

    CONST_MEDIALIBRARY_DIRECTORY_URI,
    CONST_MEDIALIBRARY_BUNDLEPERM_URI,

    CONST_MEDIALIBRARY_CHECK_URIPERM_URI,
    CONST_MEDIALIBRARY_GRANT_URIPERM_URI,

    CONST_MEDIALIBRARY_AUDIO_URI,
    CONST_MEDIALIBRARY_VIDEO_URI,
    CONST_MEDIALIBRARY_IMAGE_URI,
    CONST_MEDIALIBRARY_FILE_URI,
    CONST_MEDIALIBRARY_ALBUM_URI,
    CONST_MEDIALIBRARY_SMARTALBUM_CHANGE_URI,
    CONST_MEDIALIBRARY_DEVICE_URI,
    CONST_MEDIALIBRARY_SMART_URI,
    CONST_MEDIALIBRARY_REMOTEFILE_URI,
};

void CleanTestTables()
{
    vector<string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
        CONST_MEDIALIBRARY_TABLE
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

void SetTables()
{
    vector<string> createTableSqlList = {
        PhotoUpgrade::CREATE_PHOTO_TABLE,
        CREATE_MEDIA_TABLE,
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

void ClearAndRestart()
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    system("rm -rf /storage/cloud/files/*");
    system("rm -rf /storage/cloud/files/.thumbs");
    system("rm -rf /storage/cloud/files/.editData");
    for (const auto &dir : TEST_ROOT_DIRS) {
        string ROOT_PATH = "/storage/cloud/100/files/";
        bool ret = MediaFileUtils::CreateDirectory(ROOT_PATH + dir + "/");
        CHECK_AND_PRINT_LOG(ret, "make %{public}s dir failed, ret=%{public}d", dir.c_str(), ret);
    }
    CleanTestTables();
    SetTables();
}

static inline MediaDataShareExtAbility Init()
{
    const std::unique_ptr<AbilityRuntime::Runtime> runtime;
    return {(*runtime)};
}

void MediaDatashareExtAbilityTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    SetTables();
    MEDIA_INFO_LOG("MediaLibraryManagerTest::SetUpTestCase:: Finish");
}

void MediaDatashareExtAbilityTest::TearDownTestCase(void)
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    system("rm -rf /storage/cloud/files/*");
    ClearAndRestart();
    g_rdbStore = nullptr;
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    MEDIA_INFO_LOG("Clean is finish");
    MEDIA_ERR_LOG("TearDownTestCase finish");
}

// SetUp:Execute before each test case
void MediaDatashareExtAbilityTest::SetUp()
{
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryCloudAssetDownloadTest failed, can not get rdbstore");
        exit(1);
    }
    ClearAndRestart();
}

void MediaDatashareExtAbilityTest::TearDown(void) {}

struct PhotoResult {
    int64_t fileId;
    int32_t mediaType;
    string path;
    string displayName;
};

string ReturnUri(string uriType, string mainUri, string subUri = "")
{
    if (subUri.empty()) {
        return (uriType + "/" + mainUri);
    } else {
        return (uriType + "/" + mainUri + "/" + subUri);
    }
}

void InsertAsset(PhotoResult &result)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    EXPECT_NE(rdbStore, nullptr);

    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutInt(MediaColumn::MEDIA_TYPE, result.mediaType);
    valuesBucket.PutString(MediaColumn::MEDIA_NAME, result.displayName);
    valuesBucket.PutString(MediaColumn::MEDIA_FILE_PATH, result.path);

    int32_t ret = rdbStore->Insert(result.fileId, PhotoColumn::PHOTOS_TABLE, valuesBucket);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaDatashareExtAbilityTest, Extension_Insert_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Extension_Insert_001::Start");
    auto extension = Init();
    extension.InitPermissionHandler();

    Uri insertUri(CONST_URI_CREATE_FILE);
    DataShare::DataShareValuesBucket valuesBucket;
    string relativePath = "Pictures/";
    string displayName = "CreateAsset_Test_001.jpg";
    MediaType mediaType = MEDIA_TYPE_IMAGE;
    valuesBucket.Put(MediaColumn::MEDIA_TYPE, mediaType);
    valuesBucket.Put(MediaColumn::MEDIA_NAME, displayName);
    valuesBucket.Put(MediaColumn::MEDIA_FILE_PATH, relativePath);
    auto retVal = extension.Insert(insertUri, valuesBucket);
    EXPECT_EQ((retVal > 0), true);
}

HWTEST_F(MediaDatashareExtAbilityTest, Extension_Update_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Extension_Update_001::Start");
    auto extension = Init();
    extension.InitPermissionHandler();

    Uri updateUri(CONST_CMAM_CLOUD_MEDIA_ASSET_TASK_START_FORCE);
    DataShare::DataSharePredicates predicates;
    DataShare::DataShareValuesBucket valuesBucket;
    auto ret = extension.Update(updateUri, predicates, valuesBucket);
    EXPECT_EQ((ret > 0), false);
}

HWTEST_F(MediaDatashareExtAbilityTest, Extension_Update_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Extension_Update_002::Start");
    auto extension = Init();
    extension.InitPermissionHandler();

    struct PhotoResult photoAsset = {-1, static_cast<int32_t>(MEDIA_TYPE_IMAGE), "UpdateTest_002.jpg", "Pictures/"};
    InsertAsset(photoAsset);

    Uri updateUri(CONST_PAH_UPDATE_PHOTO);
    string updateDisplayName = "Modify_001.jpg";
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(MediaColumn::MEDIA_NAME, updateDisplayName);

    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(MediaColumn::MEDIA_ID, photoAsset.fileId);
    auto ret = extension.Update(updateUri, predicates, valuesBucket);
    EXPECT_EQ(ret, E_PERMISSION_DENIED);
}

HWTEST_F(MediaDatashareExtAbilityTest, Extension_Delete_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Extension_Delete_001::Start");
    auto extension = Init();
    extension.InitPermissionHandler();

    struct PhotoResult photoAsset = {-1, static_cast<int32_t>(MEDIA_TYPE_IMAGE), "DeleteTest_001.jpg", "Pictures/"};
    InsertAsset(photoAsset);

    Uri deleteUri(CONST_PAH_TRASH_PHOTO);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(MediaColumn::MEDIA_ID, photoAsset.fileId);
    auto ret = extension.Delete(deleteUri, predicates);
    EXPECT_EQ(ret, E_PERMISSION_DENIED);
}

HWTEST_F(MediaDatashareExtAbilityTest, Extension_Query_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Extension_Query_001::Start");
    auto extension = Init();
    extension.InitPermissionHandler();

    struct PhotoResult photoAsset = {-1, static_cast<int32_t>(MEDIA_TYPE_IMAGE), "QueryTest_001.jpg", "Pictures/"};
    InsertAsset(photoAsset);

    Uri queryUri(CONST_PAH_QUERY_PHOTO);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(MediaColumn::MEDIA_ID, photoAsset.fileId);

    vector<string> columns = {MediaColumn::MEDIA_ID, MediaColumn::MEDIA_NAME};
    DataShare::DatashareBusinessError businessError;
    auto result = extension.Query(queryUri, predicates, columns, businessError);
    EXPECT_EQ(result, nullptr);
}

HWTEST_F(MediaDatashareExtAbilityTest, Extension_GetType_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Extension_GetType_001::Start");
    auto extension = Init();
    extension.InitPermissionHandler();

    Uri getTypeUri(CONST_CMAM_CLOUD_MEDIA_ASSET_TASK_STATUS_QUERY);
    string result = extension.GetType(getTypeUri);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaDatashareExtAbilityTest, Extension_RegisterObserver_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Extension_RegisterObserver_001::Start");
    auto extension = Init();
    extension.InitPermissionHandler();

    Uri registerObserverUri(CONST_MEDIALIBRARY_IMAGE_URI);
    sptr<AAFwk::IDataAbilityObserver> dataObserver;
    bool ret = extension.RegisterObserver(registerObserverUri, dataObserver);
    EXPECT_EQ(dataObserver, nullptr);

    ret = extension.UnregisterObserver(registerObserverUri, dataObserver);
    EXPECT_EQ(dataObserver, nullptr);
}

HWTEST_F(MediaDatashareExtAbilityTest, Extension_NotifyChange_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Extension_NotifyChange_001::Start");
    auto extension = Init();
    extension.InitPermissionHandler();

    bool ret = false;
    for (int i = 0; i < EXTENSION_URI_LISTS.size(); i++) {
        Uri notifyChangeUri(EXTENSION_URI_LISTS[i]);
        ret = extension.NotifyChange(notifyChangeUri);
    }
    EXPECT_EQ(ret, true);
}

HWTEST_F(MediaDatashareExtAbilityTest, DataManager_Update_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("DataManager_Update_001::Start");
    struct PhotoResult photoAsset = {-1, static_cast<int32_t>(MEDIA_TYPE_IMAGE), "UpdateTest_002.jpg", "Pictures/"};
    InsertAsset(photoAsset);

    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(MediaColumn::MEDIA_ID, photoAsset.fileId);
    DataShare::DataShareValuesBucket dataShareValue;
    for (int i = 0; i < EXTENSION_URI_LISTS.size(); i++) {
        Uri uri(EXTENSION_URI_LISTS[i]);
        MediaLibraryCommand cmd(uri);
        auto updateRet = MediaLibraryDataManager::GetInstance()->Update(cmd, dataShareValue, predicates);
        EXPECT_LT(updateRet, 2);
        this_thread::sleep_for(chrono::milliseconds(SLEEP_FOR_FIFTY_MS));
    }
    MEDIA_INFO_LOG("DataManager_Update_001::End");
}

HWTEST_F(MediaDatashareExtAbilityTest, DataManager_Query_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("DataManager_Query_001::Start");
    int err = 0;
    vector<string> columns = {MediaColumn::MEDIA_ID, MediaColumn::MEDIA_NAME};
    struct PhotoResult photoAsset = {-1, static_cast<int32_t>(MEDIA_TYPE_IMAGE), "UpdateTest_002.jpg", "Pictures/"};
    InsertAsset(photoAsset);

    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(MediaColumn::MEDIA_ID, photoAsset.fileId);
    for (int i = 0; i < EXTENSION_URI_LISTS.size(); i++) {
        if (EXTENSION_URI_LISTS[i] == CONST_PAH_PROCESS_IMAGE ||
            EXTENSION_URI_LISTS[i] == CONST_PAH_REMOVE_MSC_TASK) {
            continue;
        }
        Uri uri(EXTENSION_URI_LISTS[i]);
        MediaLibraryCommand cmd(uri);
        auto queryRet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, err);
        this_thread::sleep_for(chrono::milliseconds(SLEEP_FOR_FIFTY_MS));
    }
    EXPECT_LT(err, 2);
    MEDIA_INFO_LOG("DataManager_Query_001::End");
}

HWTEST_F(MediaDatashareExtAbilityTest, DataManager_Delete_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("DataManager_Delete_001::Start");
    vector<string> columns = {MediaColumn::MEDIA_ID, MediaColumn::MEDIA_NAME};
    struct PhotoResult photoAsset = {-1, static_cast<int32_t>(MEDIA_TYPE_IMAGE), "UpdateTest_002.jpg", "Pictures/"};
    InsertAsset(photoAsset);

    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(MediaColumn::MEDIA_ID, photoAsset.fileId);
    for (int i = 0; i < EXTENSION_URI_LISTS.size(); i++) {
        Uri uri(EXTENSION_URI_LISTS[i]);
        MediaLibraryCommand cmd(uri);
        auto deleteRet = MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
        EXPECT_LE(deleteRet, 2);
        this_thread::sleep_for(chrono::milliseconds(SLEEP_FOR_FIFTY_MS));
    }
    MEDIA_INFO_LOG("DataManager_Delete_001::End");
}

HWTEST_F(MediaDatashareExtAbilityTest, DataManager_GetType_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("DataManager_GetType_001::Start");
    string result;
    for (int i = 0; i < EXTENSION_URI_LISTS.size(); i++) {
        Uri uri(EXTENSION_URI_LISTS[i]);
        result = MediaLibraryDataManager::GetInstance()->GetType(uri);
        this_thread::sleep_for(chrono::milliseconds(SLEEP_FOR_FIFTY_MS));
    }
    EXPECT_EQ(result, "");
    MEDIA_INFO_LOG("DataManager_GetType_001::End");
}

HWTEST_F(MediaDatashareExtAbilityTest, DataManager_RegisterObserver_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("DataManager_RegisterObserver_001::Start");
    sptr<AAFwk::IDataAbilityObserver> dataObserver;
    auto extension = Init();
    bool ret = false;
    for (int i = 0; i < EXTENSION_URI_LISTS.size(); i++) {
        Uri uri(EXTENSION_URI_LISTS[i]);
        ret = extension.RegisterObserver(uri, dataObserver);
        this_thread::sleep_for(chrono::milliseconds(SLEEP_FOR_FIFTY_MS));
    }
    EXPECT_EQ(dataObserver, nullptr);
    MEDIA_INFO_LOG("DataManager_RegisterObserver_001::End");
}

HWTEST_F(MediaDatashareExtAbilityTest, DataManager_UpgradeUtils_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("DataManager_UpgradeUtils_001::Start");
    bool ret = RdbUpgradeUtils::HasUpgraded(2025, true);
    EXPECT_EQ(ret, false);
    RdbUpgradeUtils::HasUpgraded(VERSION_FIX_DB_UPGRADE_TO_API20, true);
    RdbUpgradeUtils::SetUpgradeStatus(VERSION_FIX_DB_UPGRADE_TO_API20, true);
    RdbUpgradeUtils::HasUpgraded(VERSION_FIX_DB_UPGRADE_TO_API20, false);
    
    RdbUpgradeUtils::SetUpgradeStatus(VERSION_FIX_DB_UPGRADE_TO_API20, false);
    RdbUpgradeUtils::SetUpgradeStatus(VERSION_FIX_DB_UPGRADE_TO_API20, false);
    MEDIA_INFO_LOG("DataManager_UpgradeUtils_001::End");
}
 
HWTEST_F(MediaDatashareExtAbilityTest, DataManager_UpgradeUtils_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("DataManager_UpgradeUtils_002::Start");
    bool ret = RdbUpgradeUtils::HasUpgraded(0, true);
    EXPECT_EQ(ret, false);
    RdbUpgradeUtils::SetUpgradeStatus(0, true);
    RdbUpgradeUtils::SetUpgradeStatus(VERSION_FIX_DB_UPGRADE_TO_API20, true);
    MEDIA_INFO_LOG("DataManager_UpgradeUtils_002::End");
}
} // namespace Media
} // namespace OHOS