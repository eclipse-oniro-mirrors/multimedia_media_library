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
#define MLOG_TAG "NotifyTest"

#include <thread>
#include <memory>
#include <vector>
#include <algorithm>
#include <string>

#include "notify_test.h"

#include "ability_context_impl.h"
#include "fetch_result.h"
#include "get_self_permissions.h"
#include "iservice_registry.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#define private public
#include "medialibrary_notify.h"
#include "cloud_sync_notify_handler.h"
#undef private
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "photo_album_column.h"
#include "photo_map_column.h"
#include "rdb_predicates.h"
#include "result_set_utils.h"
#include "system_ability_definition.h"
#include "analysis_handler.h"
#include "notify_handler.h"
#include "uri_convert_handler.h"
#include "userfile_manager_types.h"
#include "userfilemgr_uri.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace OHOS::DataShare;
using OHOS::DataShare::DataShareValuesBucket;
using OHOS::DataShare::DataSharePredicates;
using ChangeType = DataShare::DataShareObserver::ChangeType;

enum DefaultAlbumId: int32_t {
    FAVORITE_ALBUM = 1,
    VIDEO_ALBUM,
    HIDDEN_ALBUM,
    TRASH_ALBUM,
    SCREENSHOTS_ALBUM,
    CAMERA_ALBUM,
};
static constexpr int STORAGE_MANAGER_ID = 5003;
static constexpr int OBS_TMP_ID = 1;
static constexpr int OBS_TMP_ALBUM_ID = 10;
static constexpr int LIST_SIZE = 1;
static constexpr int64_t DATE_ADD = 6666666;
static constexpr int64_t DATE_MODIFY = 6666667;
shared_ptr<DataShare::DataShareHelper> sDataShareHelper_ = nullptr;
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;

const std::array<CloudSyncErrType, 7> allErrorTypes = {
    CloudSyncErrType::OTHER_ERROR,
    CloudSyncErrType::CONTENT_NOT_FOUND,
    CloudSyncErrType::THM_NOT_FOUND,
    CloudSyncErrType::LCD_NOT_FOUND,
    CloudSyncErrType::LCD_SIZE_IS_TOO_LARGE,
    CloudSyncErrType::CONTENT_SIZE_IS_ZERO,
    CloudSyncErrType::ALBUM_NOT_FOUND
};

void CheckGetAlbumIdBySubType(PhotoAlbumSubType photoAlbumSubType, DefaultAlbumId defaultAlbumId)
{
    auto watch = MediaLibraryNotify::GetInstance();
    int albumId = watch->GetAlbumIdBySubType(photoAlbumSubType);
    EXPECT_EQ(defaultAlbumId, albumId);
}

void CheckFileNotify(NotifyType notifyType)
{
    string uriStr = PhotoColumn::PHOTO_URI_PREFIX + to_string(OBS_TMP_ID);
    Uri uri(uriStr);
    shared_ptr<TestObserver> obs = make_shared<TestObserver>();
    if (sDataShareHelper_ == nullptr) {
        return;
    }
    sDataShareHelper_->RegisterObserverExt(uri, obs, true);
    MediaLibraryNotify::GetInstance()->Notify(uriStr, notifyType);
    {
        unique_lock<mutex> lock(obs->mutex_);
        if (obs->condition_.wait_for(lock, 2s) == cv_status::no_timeout) {
            EXPECT_EQ(obs->changeInfo_.changeType_, static_cast<DataShareObserver::ChangeType>(notifyType));
            EXPECT_EQ(obs->changeInfo_.uris_.size(), 1);
            EXPECT_EQ(obs->changeInfo_.uris_.begin()->ToString(), uriStr);
        } else {
            EXPECT_TRUE(false);
        }
    }
}

void SolveAlbumNotify(std::shared_ptr<TestObserver> obs, const std::string& assetStr)
{
    std::vector<uint8_t> data(obs->changeInfo_.size_);
    if (data.empty()) {
        return;
    }

    std::copy(static_cast<const char*>(obs->changeInfo_.data_), static_cast<const char*>(obs->changeInfo_.data_) +
        obs->changeInfo_.size_, data.begin());
    std::shared_ptr<MessageParcel> parcel = std::make_shared<MessageParcel>();

    if (parcel->ParseFrom(reinterpret_cast<uintptr_t>(data.data()), obs->changeInfo_.size_)) {
        uint32_t len = 0;
        if (!parcel->ReadUint32(len)) {
            return;
        }

        EXPECT_EQ(len, LIST_SIZE);

        for (uint32_t i = 0; i < len; i++) {
            std::string subUri = parcel->ReadString();
            if (subUri.empty()) {
                return;
            }
            EXPECT_EQ(subUri, assetStr);
        }
    }
}

void CheckAlbumNotify(NotifyType notifyType, DataShareObserver::ChangeType changeType)
{
    string assetStr = PhotoColumn::PHOTO_URI_PREFIX + to_string(OBS_TMP_ID);
    string albumStr = PhotoAlbumColumns::ALBUM_URI_PREFIX  + to_string(OBS_TMP_ALBUM_ID);
    Uri uri(albumStr);
    shared_ptr<TestObserver> obs = make_shared<TestObserver>();
    if (sDataShareHelper_ == nullptr) {
        return;
    }
    sDataShareHelper_->RegisterObserverExt(uri, obs, true);
    MediaLibraryNotify::GetInstance()->Notify(assetStr, notifyType, OBS_TMP_ALBUM_ID);
    {
        unique_lock<mutex> lock(obs->mutex_);
        if (obs->condition_.wait_for(lock, 2s) == cv_status::no_timeout) {
            if (obs->changeInfo_.size_ > 0) {
                SolveAlbumNotify(obs, assetStr);
            } else {
                EXPECT_TRUE(false);
            }
            EXPECT_EQ(obs->changeInfo_.changeType_, changeType);
            EXPECT_EQ(obs->changeInfo_.uris_.size(), LIST_SIZE);
            EXPECT_EQ(obs->changeInfo_.uris_.begin()->ToString(), albumStr);
        } else {
            EXPECT_TRUE(false);
        }
    }
}

void CheckCloseAssetNotify(bool isCreate)
{
    string uriStr = PhotoColumn::PHOTO_URI_PREFIX + to_string(OBS_TMP_ID);
    Uri uri(uriStr);
    shared_ptr<TestObserver> obs = make_shared<TestObserver>();
    if (sDataShareHelper_ == nullptr) {
        return;
    }
    sDataShareHelper_->RegisterObserverExt(uri, obs, true);
    shared_ptr<FileAsset> closeAsset = make_shared<FileAsset>();
    closeAsset->SetId(OBS_TMP_ID);
    closeAsset->SetMediaType(MediaType::MEDIA_TYPE_IMAGE);
    if (isCreate) {
        closeAsset->SetDateAdded(DATE_ADD);
        closeAsset->SetDateModified(0);
    } else {
        closeAsset->SetDateAdded(DATE_ADD);
        closeAsset->SetDateModified(DATE_MODIFY);
    }
    MediaLibraryNotify::GetInstance()->Notify(closeAsset);
    {
        unique_lock<mutex> lock(obs->mutex_);
        if (obs->condition_.wait_for(lock, 2s) == cv_status::no_timeout) {
            if (isCreate) {
                EXPECT_EQ(obs->changeInfo_.changeType_,
                    static_cast<DataShareObserver::ChangeType>(NotifyType::NOTIFY_ADD));
            } else {
                EXPECT_EQ(obs->changeInfo_.changeType_,
                    static_cast<DataShareObserver::ChangeType>(NotifyType::NOTIFY_UPDATE));
            }
            EXPECT_EQ(obs->changeInfo_.uris_.size(), 1);
            EXPECT_EQ(obs->changeInfo_.uris_.begin()->ToString(), uriStr);
        } else {
            EXPECT_TRUE(false);
        }
    }
}

static void CheckInfo(shared_ptr<TestObserver> obs, const std::string &uriPrefix, const std::string &uriPostfix,
    const DataShareObserver::ChangeType &changeType)
{
    unique_lock<mutex> lock(obs->mutex_);
    if (obs->condition_.wait_for(lock, 1s) == cv_status::no_timeout ||
        obs->bChange_.load()) {
        if (changeType == DataShareObserver::ChangeType::OTHER) {
            EXPECT_EQ(obs->changeInfo_.changeType_,
                static_cast<DataShareObserver::ChangeType>(NotifyType::NOTIFY_REMOVE));
        } else if (changeType == DataShareObserver::ChangeType::INSERT) {
            EXPECT_EQ(obs->changeInfo_.changeType_,
                static_cast<DataShareObserver::ChangeType>(NotifyType::NOTIFY_ADD));
        } else if (changeType == DataShareObserver::ChangeType::UPDATE) {
            EXPECT_EQ(obs->changeInfo_.changeType_,
                static_cast<DataShareObserver::ChangeType>(NotifyType::NOTIFY_UPDATE));
        } else if (changeType == DataShareObserver::ChangeType::DELETE) {
            EXPECT_EQ(obs->changeInfo_.changeType_,
                static_cast<DataShareObserver::ChangeType>(NotifyType::NOTIFY_REMOVE));
        } else {
            EXPECT_EQ(obs->changeInfo_.changeType_, changeType);
        }
        EXPECT_EQ(obs->changeInfo_.uris_.size(), LIST_SIZE);
        if (uriPrefix == PhotoAlbumColumns::ALBUM_CLOUD_URI_PREFIX &&
            changeType == DataShareObserver::ChangeType::OTHER) {
            EXPECT_EQ(obs->changeInfo_.uris_.begin()->ToString(), PhotoAlbumColumns::ALBUM_URI_PREFIX);
        } else if (uriPrefix == PhotoColumn::PHOTO_CLOUD_URI_PREFIX &&
            changeType == DataShareObserver::ChangeType::OTHER) {
            EXPECT_EQ(obs->changeInfo_.uris_.begin()->ToString(), PhotoColumn::PHOTO_URI_PREFIX);
        } else if (uriPrefix == PhotoAlbumColumns::ALBUM_CLOUD_URI_PREFIX) {
            EXPECT_EQ(obs->changeInfo_.uris_.begin()->ToString(), PhotoAlbumColumns::ALBUM_URI_PREFIX + uriPostfix);
        } else if (uriPrefix == PhotoColumn::PHOTO_CLOUD_URI_PREFIX) {
            EXPECT_EQ(obs->changeInfo_.uris_.begin()->ToString(), PhotoColumn::PHOTO_URI_PREFIX + uriPostfix);
        }
    } else {
        EXPECT_NE(obs, nullptr);
    }
}

static void CheckCloudSyncNotify(const std::string &uriPrefix, const std::string &uriPostfix,
    const DataShareObserver::ChangeType &changeType)
{
    auto context = std::make_shared<OHOS::AbilityRuntime::AbilityContextImpl>();
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_TRUE(rdbStore != nullptr);

    string assetStr = "";
    if (uriPrefix == PhotoAlbumColumns::ALBUM_CLOUD_URI_PREFIX) {
        assetStr = PhotoAlbumColumns::ALBUM_URI_PREFIX;
    }
    if (uriPrefix == PhotoColumn::PHOTO_CLOUD_URI_PREFIX) {
        assetStr = PhotoColumn::PHOTO_URI_PREFIX;
    }
    string uriStr = uriPrefix + uriPostfix;
    Uri uri(uriStr);
    shared_ptr<TestObserver> obs = make_shared<TestObserver>();
    if (sDataShareHelper_ == nullptr) {
        return;
    }
    Uri uri2(assetStr);
    sDataShareHelper_->RegisterObserverExt(uri2, obs, true);
    list<Uri> uris;
    uris.push_back(uri);
    CloudSyncNotifyInfo _cloudSyncNotifyInfo;
    _cloudSyncNotifyInfo.uris = uris;
    _cloudSyncNotifyInfo.type = changeType;
    CloudSyncNotifyHandler _cloudSyncNotifyHandler(_cloudSyncNotifyInfo);
    _cloudSyncNotifyHandler.MakeResponsibilityChain();

    CheckInfo(obs, uriPrefix, uriPostfix, changeType);
    sDataShareHelper_->UnregisterObserverExt(uri2, obs);
    obs = nullptr;
}

void NotifyTest::SetUpTestCase()
{
    MediaLibraryUnitTestUtils::Init();
    vector<string> perms;
    perms.push_back("ohos.permission.READ_MEDIA");
    perms.push_back("ohos.permission.WRITE_MEDIA");
    uint64_t tokenId = 0;
    PermissionUtilsUnitTest::SetAccessTokenPermission("NotifyTest", perms, tokenId);
    ASSERT_TRUE(tokenId != 0);

    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_TRUE(saManager != nullptr);

    auto remoteObj = saManager->GetSystemAbility(STORAGE_MANAGER_ID);
    ASSERT_TRUE(remoteObj != nullptr);

    sDataShareHelper_ = DataShare::DataShareHelper::Creator(remoteObj, MEDIALIBRARY_DATA_URI);
    if (sDataShareHelper_ == nullptr) {
        exit(0);
    }
}

void NotifyTest::TearDownTestCase()
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
}

// SetUp:Execute before each test case
void NotifyTest::SetUp() {}

void NotifyTest::TearDown() {}

/**
 * @tc.name: asset_on_change_001
 * @tc.desc: solve asset and get message
 *           1. RegisterObserverExt called "asset_on_change_001"
 *           2. NotifyType::NOTIFY_ADD
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(NotifyTest, asset_on_change_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("asset_on_change_001 enter");
    CheckFileNotify(NotifyType::NOTIFY_ADD);
    MEDIA_INFO_LOG("asset_on_change_001 exit");
}

/**
 * @tc.name: asset_on_change_002
 * @tc.desc: solve asset and get message
 *           1. RegisterObserverExt called "asset_on_change_002"
 *           2. NotifyType::NOTIFY_UPDATE
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(NotifyTest, asset_on_change_002, TestSize.Level2)
{
    MEDIA_INFO_LOG("asset_on_change_002 enter");
    CheckFileNotify(NotifyType::NOTIFY_UPDATE);
    MEDIA_INFO_LOG("asset_on_change_002 exit");
}

/**
 * @tc.name: asset_on_change_003
 * @tc.desc: solve asset and get message
 *           1. RegisterObserverExt called "asset_on_change_003"
 *           2. NotifyType::NOTIFY_REMOVE
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(NotifyTest, asset_on_change_003, TestSize.Level2)
{
    MEDIA_INFO_LOG("asset_on_change_003 enter");
    CheckFileNotify(NotifyType::NOTIFY_REMOVE);
    MEDIA_INFO_LOG("asset_on_change_003 exit");
}

/**
 * @tc.name: close_asset_on_change_001
 * @tc.desc: solve close asset and get message
 *           1. RegisterObserverExt called "close_asset_on_change_001"
 *           2. isCreate == true
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(NotifyTest, close_asset_on_change_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("close_asset_on_change_001 enter");
    CheckCloseAssetNotify(true);
    MEDIA_INFO_LOG("close_asset_on_change_001 exit");
}

/**
 * @tc.name: close_asset_on_change_002
 * @tc.desc: solve close asset and get message
 *           1. RegisterObserverExt called "close_asset_on_change_002"
 *           2. isCreate == false
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(NotifyTest, close_asset_on_change_002, TestSize.Level2)
{
    MEDIA_INFO_LOG("close_asset_on_change_002 enter");
    CheckCloseAssetNotify(false);
    MEDIA_INFO_LOG("close_asset_on_change_002 exit");
}

/**
 * @tc.name: cloud_notify_001
 * @tc.desc: test cloud notify for ChangeType::UPDATE
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(NotifyTest, cloud_notify_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("cloud_notify_001 enter");
    CheckCloudSyncNotify(PhotoAlbumColumns::ALBUM_CLOUD_URI_PREFIX, "1", DataShareObserver::ChangeType::UPDATE);
    MEDIA_INFO_LOG("cloud_notify_001 exit");
}

/**
 * @tc.name: cloud_notify_002
 * @tc.desc: test cloud notify for ChangeType::INSERT
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(NotifyTest, cloud_notify_002, TestSize.Level2)
{
    MEDIA_INFO_LOG("cloud_notify_002 enter");
    CheckCloudSyncNotify(PhotoAlbumColumns::ALBUM_CLOUD_URI_PREFIX, "2", DataShareObserver::ChangeType::INSERT);
    MEDIA_INFO_LOG("cloud_notify_002 exit");
}

/**
 * @tc.name: cloud_notify_003
 * @tc.desc: test cloud notify for ChangeType::INSERT
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(NotifyTest, cloud_notify_003, TestSize.Level2)
{
    MEDIA_INFO_LOG("cloud_notify_003 enter");
    CheckCloudSyncNotify(PhotoColumn::PHOTO_CLOUD_URI_PREFIX, "3", DataShareObserver::ChangeType::INSERT);
    MEDIA_INFO_LOG("cloud_notify_003 exit");
}

/**
 * @tc.name: cloud_notify_004
 * @tc.desc: test cloud notify for ChangeType::DELETE
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(NotifyTest, cloud_notify_004, TestSize.Level2)
{
    MEDIA_INFO_LOG("cloud_notify_004 enter");
    CheckCloudSyncNotify(PhotoColumn::PHOTO_CLOUD_URI_PREFIX, "4", DataShareObserver::ChangeType::DELETE);
    MEDIA_INFO_LOG("cloud_notify_004 exit");
}

/**
 * @tc.name: cloud_notify_005
 * @tc.desc: test cloud notify for ChangeType::INSERT
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(NotifyTest, cloud_notify_005, TestSize.Level2)
{
    MEDIA_INFO_LOG("cloud_notify_005 enter");
    CheckCloudSyncNotify(PhotoColumn::PHOTO_CLOUD_URI_PREFIX, "5", DataShareObserver::ChangeType::INSERT);
    MEDIA_INFO_LOG("cloud_notify_005 exit");
}

/**
 * @tc.name: cloud_notify_006
 * @tc.desc: test cloud notify for ChangeType::INSERT
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(NotifyTest, cloud_notify_006, TestSize.Level2)
{
    MEDIA_INFO_LOG("cloud_notify_006 enter");
    CheckCloudSyncNotify(PhotoColumn::PHOTO_CLOUD_URI_PREFIX, "6", DataShareObserver::ChangeType::INSERT);
    MEDIA_INFO_LOG("cloud_notify_006 exit");
}

/**
 * @tc.name: cloud_notify_007
 * @tc.desc: test cloud notify for ChangeType::INSERT
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(NotifyTest, cloud_notify_007, TestSize.Level2)
{
    MEDIA_INFO_LOG("cloud_notify_007 enter");
    CheckCloudSyncNotify(PhotoColumn::PHOTO_CLOUD_URI_PREFIX, "", DataShareObserver::ChangeType::INSERT);
    MEDIA_INFO_LOG("cloud_notify_007 exit");
}

/**
 * @tc.name: cloud_notify_008
 * @tc.desc: test cloud notify for ChangeType::DELETE
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(NotifyTest, cloud_notify_008, TestSize.Level2)
{
    MEDIA_INFO_LOG("cloud_notify_008 enter");
    CheckCloudSyncNotify(PhotoColumn::PHOTO_CLOUD_URI_PREFIX, "", DataShareObserver::ChangeType::DELETE);
    MEDIA_INFO_LOG("cloud_notify_008 exit");
}

/**
 * @tc.name: cloud_notify_009
 * @tc.desc: test cloud notify for ChangeType::INSERT
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(NotifyTest, cloud_notify_009, TestSize.Level2)
{
    MEDIA_INFO_LOG("cloud_notify_009 enter");
    CheckCloudSyncNotify(PhotoColumn::PHOTO_CLOUD_URI_PREFIX, "", DataShareObserver::ChangeType::INSERT);
    MEDIA_INFO_LOG("cloud_notify_009 exit");
}

/**
 * @tc.name: cloud_notify_010
 * @tc.desc: test cloud notify for ChangeType::INSERT
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(NotifyTest, cloud_notify_010, TestSize.Level2)
{
    MEDIA_INFO_LOG("cloud_notify_010 enter");
    CheckCloudSyncNotify(PhotoColumn::PHOTO_CLOUD_URI_PREFIX, "", DataShareObserver::ChangeType::INSERT);
    MEDIA_INFO_LOG("cloud_notify_010 exit");
}

/**
 * @tc.name: handle_empty_data_001
 * @tc.desc: test AnalysisHandler Handle
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(NotifyTest, handle_empty_data_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("handle_empty_data_001 enter");
    MediaLibraryUnitTestUtils::InitUnistore();
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_TRUE(rdbStore != nullptr);
    CloudSyncHandleData emptyHandleData;
    emptyHandleData.orgInfo.type = DataShareObserver::ChangeType::OTHER;

    auto periodWorker = MediaLibraryPeriodWorker::GetInstance();
    PeriodTaskData *data = new (std::nothrow) PeriodTaskData();
    if (data == nullptr) {
        MEDIA_ERR_LOG("Failed to new taskdata");
        return;
    }
    periodWorker->StartTask(PeriodTaskType::CLOUD_ANALYSIS_ALBUM, AnalysisHandler::ProcessHandleData, data);
    MEDIA_INFO_LOG("handle_empty_data_001 exit");
}

/**
 * @tc.name: handle_empty_data_002
 * @tc.desc: test UriConvertHandler Handle
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(NotifyTest, handle_empty_data_002, TestSize.Level2)
{
    MEDIA_INFO_LOG("handle_empty_data_002 enter");
    CloudSyncHandleData emptyHandleData;
    emptyHandleData.orgInfo.type = DataShareObserver::ChangeType::OTHER;
    auto handler = make_shared<NotifyHandler>();
    EXPECT_NE(handler, nullptr);
    handler->Handle(emptyHandleData);
    MEDIA_INFO_LOG("handle_empty_data_002 exit");
}

/**
 * @tc.name: handle_empty_data_003
 * @tc.desc: test NotifyHandler Handle
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(NotifyTest, handle_empty_data_003, TestSize.Level2)
{
    MEDIA_INFO_LOG("handle_empty_data_003 enter");
    CloudSyncHandleData emptyHandleData;
    emptyHandleData.orgInfo.type = DataShareObserver::ChangeType::OTHER;
    auto handler = make_shared<NotifyHandler>();
    EXPECT_NE(handler, nullptr);
    handler->Handle(emptyHandleData);
    MEDIA_INFO_LOG("handle_empty_data_003 exit");
}

/**
 * @tc.name: handle_special_change_type_001
 * @tc.desc: test NotifyHandler Handle
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(NotifyTest, handle_special_change_type_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("handle_special_change_type_001 enter");
    CloudSyncHandleData specialHandleData;
    specialHandleData.notifyInfo[static_cast<DataShare::DataShareObserver::ChangeType>(-1)] = {};
    auto handler = make_shared<NotifyHandler>();
    EXPECT_NE(handler, nullptr);
    handler->Handle(specialHandleData);
    MEDIA_INFO_LOG("handle_special_change_type_001 exit");
}

HWTEST_F(NotifyTest, HandleDirtyDataFix_test_empty_list, TestSize.Level2)
{
    MEDIA_INFO_LOG("HandleDirtyDataFix_test_empty_list enter");
    CloudSyncNotifyInfo info;
    auto handler = make_shared<CloudSyncNotifyHandler>(info);
    EXPECT_NE(handler, nullptr);

    std::list<Uri> uris;
    for (const auto& type : allErrorTypes) {
        handler->HandleDirtyDataFix(uris, type);
    }
    MEDIA_INFO_LOG("HandleDirtyDataFix_test_empty_list leave");
}

HWTEST_F(NotifyTest, HandleDirtyDataFix_test_empty_uri, TestSize.Level2)
{
    MEDIA_INFO_LOG("HandleDirtyDataFix_test_empty_uri enter");
    CloudSyncNotifyInfo info;
    auto handler = make_shared<CloudSyncNotifyHandler>(info);
    EXPECT_NE(handler, nullptr);

    string uriStr;
    Uri uri(uriStr);
    std::list<Uri> uris;
    uris.push_back(uri);

    for (const auto& type : allErrorTypes) {
        handler->HandleDirtyDataFix(uris, type);
    }
    MEDIA_INFO_LOG("HandleDirtyDataFix_test_empty_uri leave");
}

HWTEST_F(NotifyTest, HandleDirtyDataFix_test_normal_uri, TestSize.Level2)
{
    MEDIA_INFO_LOG("HandleDirtyDataFix_test_normal_uri enter");
    CloudSyncNotifyInfo info;
    auto handler = make_shared<CloudSyncNotifyHandler>(info);
    EXPECT_NE(handler, nullptr);

    string uriStr = PhotoColumn::PHOTO_URI_PREFIX + to_string(OBS_TMP_ID);
    Uri uri(uriStr);
    std::list<Uri> uris;
    uris.push_back(uri);

    for (const auto& type : allErrorTypes) {
        handler->HandleDirtyDataFix(uris, type);
    }
    MEDIA_INFO_LOG("HandleDirtyDataFix_test_normal_uri leave");
}

HWTEST_F(NotifyTest, MakeResponsibilityChain_test_empty_list, TestSize.Level2)
{
    MEDIA_INFO_LOG("MakeResponsibilityChain_test_empty_list enter");
    CloudSyncNotifyInfo info;
    auto handler = make_shared<CloudSyncNotifyHandler>(info);
    EXPECT_NE(handler, nullptr);
    handler->MakeResponsibilityChain();
    MEDIA_INFO_LOG("MakeResponsibilityChain_test_empty_list leave");
}

HWTEST_F(NotifyTest, MakeResponsibilityChain_test_empty_uri, TestSize.Level2)
{
    MEDIA_INFO_LOG("MakeResponsibilityChain_test_empty_uri enter");
    CloudSyncNotifyInfo info;
    string uriStr;
    Uri uri(uriStr);
    info.uris.push_back(uri);

    auto handler = make_shared<CloudSyncNotifyHandler>(info);
    EXPECT_NE(handler, nullptr);
    handler->MakeResponsibilityChain();
    MEDIA_INFO_LOG("MakeResponsibilityChain_test_empty_uri leave");
}

HWTEST_F(NotifyTest, MakeResponsibilityChain_test, TestSize.Level2)
{
    MEDIA_INFO_LOG("MakeResponsibilityChain_test enter");
    CloudSyncNotifyInfo info;
    string uriStr = "file://cloudsync/Photo/HeightError/1";
    Uri uri1(uriStr);
    info.uris.push_back(uri1);

    auto handler = make_shared<CloudSyncNotifyHandler>(info);
    EXPECT_NE(handler, nullptr);
    handler->MakeResponsibilityChain();

    info.uris.clear();
    uriStr = "file://cloudsync/Photo/DownloadSuccessed/1";
    Uri uri2(uriStr);
    info.uris.push_back(uri2);
    handler->notifyInfo_ = info;
    handler->MakeResponsibilityChain();

    info.uris.clear();
    uriStr = "file://cloudsync/Photo/1";
    Uri uri3(uriStr);
    info.uris.push_back(uri3);
    info.type = ChangeType::INSERT;
    handler->notifyInfo_ = info;
    handler->MakeResponsibilityChain();

    info.uris.clear();
    uriStr = "file://cloudsync/PhotoAlbum/1";
    Uri uri4(uriStr);
    info.uris.push_back(uri4);
    info.type = ChangeType::DELETE;
    handler->notifyInfo_ = info;
    handler->MakeResponsibilityChain();
    MEDIA_INFO_LOG("MakeResponsibilityChain_test enter");
}
} // namespace OHOS::Media
