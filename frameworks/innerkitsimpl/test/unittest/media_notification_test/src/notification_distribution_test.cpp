/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#define MLOG_TAG "NotificationDistributionTest"

#include "notification_distribution_test.h"
#include "notification_merging_test.h"
#include "notification_distribution.h"
#include "media_log.h"
#include "media_change_info.h"
#include "photo_asset_change_info.h"
#include "album_change_info.h"
#include "notify_info.h"
#include "media_observer_manager.h"
#include "medialibrary_errno.h"
#include "notification_test_data.h"
#include "get_self_permissions.h"
#include "media_datashare_stub_impl.h"
#include "data_ability_observer_interface.h"
#include "observer_info.h"
#include "data_ability_observer_stub.h"
#include "notify_task_worker.h"
#include "user_define_notify_info.h"

#include <string>
#include <unordered_set>


using namespace testing::ext;
using namespace OHOS::AbilityRuntime;
using namespace OHOS::AAFwk;
using namespace OHOS::Media::Notification;

namespace OHOS {
namespace Media {

void NotificationDistributionTest::SetUpTestCase(void)
{
    Notification::NotificationTestData::SetHapPermission();
}

void NotificationDistributionTest::TearDownTestCase(void)
{}

void NotificationDistributionTest::SetUp()
{}

void NotificationDistributionTest::TearDown(void)
{
    MEDIA_INFO_LOG("NotificationDistributionTest TearDown start");
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    observerManager->observers_.clear();
    MEDIA_INFO_LOG("NotificationDistributionTest TearDown end");
}

HWTEST_F(NotificationDistributionTest, medialib_distribution_test001, TestSize.Level1)
{
    // 一个PHOTO_URI类型的observer
    MEDIA_INFO_LOG("enter medialib_distribution_test001");
    std::unordered_map<Notification::NotifyUriType, std::vector<Notification::ObserverInfo>> observerMap;
    Notification::NotifyUriType uri = Notification::NotifyUriType::PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    observerManager->AddObserver(uri, dataObserver);
    std::vector<Notification::ObserverInfo> observerInfos = observerManager->FindObserver(uri);
    if (observerInfos.size() < 0) {
        MEDIA_INFO_LOG("observerInfos is nullptr");
    }
    observerMap[uri] = observerInfos;
    std::vector<Notification::NotifyInfo> notifyInfos =
        Notification::NotificationTestData::buildAssetsNotifyInfo("test001", observerMap);
    EXPECT_TRUE(!notifyInfos.empty());
    int32_t ret = Media::Notification::NotificationDistribution::DistributeNotifyInfo(notifyInfos);
    EXPECT_TRUE(ret == E_OK);
    MEDIA_INFO_LOG("end medialib_distribution_test001");
}

HWTEST_F(NotificationDistributionTest, medialib_distribution_test002, TestSize.Level1)
{
    // 一个HIDDEN_PHOTO_URI类型的observer
    MEDIA_INFO_LOG("enter medialib_distribution_test002");
    std::unordered_map<Notification::NotifyUriType, std::vector<Notification::ObserverInfo>> observerMap;
    Notification::NotifyUriType uri = Notification::NotifyUriType::HIDDEN_PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    observerManager->AddObserver(uri, dataObserver);
    std::vector<Notification::ObserverInfo> observerInfos = observerManager->FindObserver(uri);
    observerMap[uri] = observerInfos;
    std::vector<Notification::NotifyInfo> notifyInfos =
        Notification::NotificationTestData::buildAssetsNotifyInfo("test002", observerMap);
    EXPECT_TRUE(!notifyInfos.empty());
    int32_t ret = Media::Notification::NotificationDistribution::DistributeNotifyInfo(notifyInfos);
    EXPECT_TRUE(ret == E_OK);
    MEDIA_INFO_LOG("end medialib_distribution_test002");
}
HWTEST_F(NotificationDistributionTest, medialib_distribution_test003, TestSize.Level1)
{
    // 一个TRASH_PHOTO_URI类型的observer
    MEDIA_INFO_LOG("enter medialib_distribution_test003");
    std::unordered_map<Notification::NotifyUriType, std::vector<Notification::ObserverInfo>> observerMap;
    Notification::NotifyUriType uri = Notification::NotifyUriType::TRASH_PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    observerManager->AddObserver(uri, dataObserver);
    std::vector<Notification::ObserverInfo> observerInfos = observerManager->FindObserver(uri);
    observerMap[uri] = observerInfos;
    std::vector<Notification::NotifyInfo> notifyInfos =
        Notification::NotificationTestData::buildAssetsNotifyInfo("test003", observerMap);
    int32_t ret = Media::Notification::NotificationDistribution::DistributeNotifyInfo(notifyInfos);
    EXPECT_TRUE(ret == E_OK);
    MEDIA_INFO_LOG("end medialib_distribution_test003");
}

HWTEST_F(NotificationDistributionTest, medialib_distribution_test004, TestSize.Level1)
{
    // 一个PHOTO_ALBUM_URI类型的observer
    MEDIA_INFO_LOG("enter medialib_distribution_test004");
    std::unordered_map<Notification::NotifyUriType, std::vector<Notification::ObserverInfo>> observerMap;
    Notification::NotifyUriType uri = Notification::NotifyUriType::PHOTO_ALBUM_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    observerManager->AddObserver(uri, dataObserver);
    std::vector<Notification::ObserverInfo> observerInfos = observerManager->FindObserver(uri);
    observerMap[uri] = observerInfos;
    std::vector<Notification::NotifyInfo> notifyInfos =
        Notification::NotificationTestData::buildAssetsNotifyInfo("test004", observerMap);
    int32_t ret = Media::Notification::NotificationDistribution::DistributeNotifyInfo(notifyInfos);
    EXPECT_TRUE(ret == E_OK);
    MEDIA_INFO_LOG("end medialib_distribution_test004");
}


HWTEST_F(NotificationDistributionTest, medialib_distribution_test005, TestSize.Level1)
{
    // 一个HIDDEN_ALBUM_URI类型的observer
    MEDIA_INFO_LOG("enter medialib_distribution_test005");
    std::unordered_map<Notification::NotifyUriType, std::vector<Notification::ObserverInfo>> observerMap;
    Notification::NotifyUriType uri = Notification::NotifyUriType::HIDDEN_ALBUM_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    observerManager->AddObserver(uri, dataObserver);
    std::vector<Notification::ObserverInfo> observerInfos = observerManager->FindObserver(uri);
    observerMap[uri] = observerInfos;
    std::vector<Notification::NotifyInfo> notifyInfos =
        Notification::NotificationTestData::buildAssetsNotifyInfo("test005", observerMap);
    int32_t ret = Media::Notification::NotificationDistribution::DistributeNotifyInfo(notifyInfos);
    EXPECT_TRUE(ret == E_OK);
    MEDIA_INFO_LOG("end medialib_distribution_test005");
}

HWTEST_F(NotificationDistributionTest, medialib_distribution_test006, TestSize.Level1)
{
    // 一个TRASH_ALBUM_URI类型的observer
    MEDIA_INFO_LOG("enter medialib_distribution_test006");
    std::unordered_map<Notification::NotifyUriType, std::vector<Notification::ObserverInfo>> observerMap;
    Notification::NotifyUriType uri = Notification::NotifyUriType::TRASH_ALBUM_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    observerManager->AddObserver(uri, dataObserver);
    std::vector<Notification::ObserverInfo> observerInfos = observerManager->FindObserver(uri);
    observerMap[uri] = observerInfos;
    std::vector<Notification::NotifyInfo> notifyInfos =
        Notification::NotificationTestData::buildAssetsNotifyInfo("test006", observerMap);
    int32_t ret = Media::Notification::NotificationDistribution::DistributeNotifyInfo(notifyInfos);
    EXPECT_TRUE(ret == E_OK);
    MEDIA_INFO_LOG("end medialib_distribution_test006");
}
HWTEST_F(NotificationDistributionTest, medialib_distribution_test007, TestSize.Level1)
{
    // HIDDEN_ALBUM_URI、TRASH_ALBUM_URI类型的observer
    MEDIA_INFO_LOG("enter medialib_distribution_test007");
    std::unordered_map<Notification::NotifyUriType, std::vector<Notification::ObserverInfo>> observerMap;

    Notification::NotifyUriType hiddenUri = Notification::NotifyUriType::HIDDEN_ALBUM_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    observerManager->AddObserver(hiddenUri, dataObserver);
    std::vector<Notification::ObserverInfo> observerInfos = observerManager->FindObserver(hiddenUri);
    observerMap[hiddenUri] = observerInfos;

    Notification::NotifyUriType trashUri = Notification::NotifyUriType::TRASH_ALBUM_URI;
    sptr<IDataAbilityObserver> secondDataObserver = new (std::nothrow) IDataAbilityObserverTest();
    observerManager->AddObserver(trashUri, secondDataObserver);
    std::vector<Notification::ObserverInfo> secondObserverInfos = observerManager->FindObserver(trashUri);
    observerMap[trashUri] = secondObserverInfos;

    std::vector<Notification::NotifyInfo> notifyInfos =
        Notification::NotificationTestData::buildAssetsNotifyInfo("test007", observerMap);
    int32_t ret = Media::Notification::NotificationDistribution::DistributeNotifyInfo(notifyInfos);
    EXPECT_TRUE(ret == E_OK);
    MEDIA_INFO_LOG("end medialib_distribution_test007");
}

HWTEST_F(NotificationDistributionTest, medialib_distribution_test008, TestSize.Level1)
{
    // 两个PHOTO_URI类型的observer
    MEDIA_INFO_LOG("enter medialib_distribution_test008");
    std::unordered_map<Notification::NotifyUriType, std::vector<Notification::ObserverInfo>> observerMap;
    Notification::NotifyUriType uri = Notification::NotifyUriType::PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    sptr<IDataAbilityObserver> secondDataObserver = new (std::nothrow) IDataAbilityObserverTest();
    observerManager->AddObserver(uri, dataObserver);
    observerManager->AddObserver(uri, secondDataObserver);
    std::vector<Notification::ObserverInfo> observerInfos = observerManager->FindObserver(uri);
    observerMap[uri] = observerInfos;
    std::vector<Notification::NotifyInfo> notifyInfos =
        Notification::NotificationTestData::buildAssetsNotifyInfo("test008", observerMap);
    int32_t ret = Media::Notification::NotificationDistribution::DistributeNotifyInfo(notifyInfos);
    EXPECT_TRUE(ret == E_OK);
    MEDIA_INFO_LOG("end medialib_distribution_test008");
}

HWTEST_F(NotificationDistributionTest, medialib_distribution_test009, TestSize.Level1)
{
    //存在其中一个notify的isForRecheck为true
    MEDIA_INFO_LOG("enter medialib_distribution_test009");
    std::unordered_map<Notification::NotifyUriType, std::vector<Notification::ObserverInfo>> observerMap;
    Notification::NotifyUriType uri = Notification::NotifyUriType::TRASH_PHOTO_URI;
    Notification::NotifyUriType secondUri = Notification::NotifyUriType::PHOTO_ALBUM_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    sptr<IDataAbilityObserver> secondDataObserver = new (std::nothrow) IDataAbilityObserverTest();
    observerManager->AddObserver(uri, dataObserver);
    observerManager->AddObserver(secondUri, secondDataObserver);
    std::vector<Notification::ObserverInfo> observerInfos = observerManager->FindObserver(uri);
    std::vector<Notification::ObserverInfo> secondObserverInfos = observerManager->FindObserver(secondUri);
    observerMap[uri] = observerInfos;
    observerMap[secondUri] = secondObserverInfos;
    std::vector<Notification::NotifyInfo> notifyInfos =
        Notification::NotificationTestData::buildAssetsNotifyInfo("test009", observerMap);
    int32_t ret = Media::Notification::NotificationDistribution::DistributeNotifyInfo(notifyInfos);
    EXPECT_TRUE(ret == E_OK);
    MEDIA_INFO_LOG("end medialib_distribution_test009");
}

HWTEST_F(NotificationDistributionTest, medialib_distribution_test010, TestSize.Level1)
{
    // 存在changeInfos是空
    MEDIA_INFO_LOG("enter medialib_distribution_test010");
    std::unordered_map<Notification::NotifyUriType, std::vector<Notification::ObserverInfo>> observerMap;
    Notification::NotifyUriType uri = Notification::NotifyUriType::HIDDEN_PHOTO_URI;
    Notification::NotifyUriType photoUri = Notification::NotifyUriType::PHOTO_URI;
    Notification::NotifyUriType trashPhotoUri = Notification::NotifyUriType::TRASH_PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    observerManager->AddObserver(uri, dataObserver);
    observerManager->AddObserver(photoUri, dataObserver);
    observerManager->AddObserver(trashPhotoUri, dataObserver);
    std::vector<Notification::ObserverInfo> observerInfos = observerManager->FindObserver(uri);
    std::vector<Notification::ObserverInfo> photoUriObserverInfos = observerManager->FindObserver(photoUri);
    std::vector<Notification::ObserverInfo> trashPhotoUriObserverInfos = observerManager->FindObserver(trashPhotoUri);
    observerMap[uri] = observerInfos;
    observerMap[photoUri] = photoUriObserverInfos;
    observerMap[trashPhotoUri] = trashPhotoUriObserverInfos;
    std::vector<Notification::NotifyInfo> notifyInfos =
        Notification::NotificationTestData::buildAssetsNotifyInfo("test010", observerMap);
    int32_t ret = Media::Notification::NotificationDistribution::DistributeNotifyInfo(notifyInfos);
    EXPECT_TRUE(ret == E_OK);
    MEDIA_INFO_LOG("end medialib_distribution_test010");
}

HWTEST_F(NotificationDistributionTest, medialib_distribution_test011, TestSize.Level1)
{
    // 存在多个changeInfos
    MEDIA_INFO_LOG("enter medialib_distribution_test011");
    std::unordered_map<Notification::NotifyUriType, std::vector<Notification::ObserverInfo>> observerMap;
    Notification::NotifyUriType uri = Notification::NotifyUriType::PHOTO_URI;
    Notification::NotifyUriType photoUri = Notification::NotifyUriType::HIDDEN_ALBUM_URI;
    Notification::NotifyUriType trashPhotoUri = Notification::NotifyUriType::TRASH_ALBUM_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    observerManager->AddObserver(uri, dataObserver);
    observerManager->AddObserver(photoUri, dataObserver);
    observerManager->AddObserver(trashPhotoUri, dataObserver);
    std::vector<Notification::ObserverInfo> observerInfos = observerManager->FindObserver(uri);
    observerMap[uri] = observerInfos;
    observerMap[photoUri] = observerManager->FindObserver(photoUri);
    observerMap[trashPhotoUri] = observerManager->FindObserver(trashPhotoUri);
    std::vector<Notification::NotifyInfo> notifyInfos =
        Notification::NotificationTestData::buildAssetsNotifyInfo("test011", observerMap);
    int32_t ret = Media::Notification::NotificationDistribution::DistributeNotifyInfo(notifyInfos);
    EXPECT_TRUE(ret == E_OK);
    MEDIA_INFO_LOG("end medialib_distribution_test011");
}

HWTEST_F(NotificationDistributionTest, medialib_distribution_test012, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_distribution_test012");
    std::unordered_map<Notification::NotifyUriType, std::vector<Notification::ObserverInfo>> observerMap;
    Notification::NotifyUriType uri = Notification::NotifyUriType::PHOTO_URI;
    Notification::NotifyUriType trashPhotoUri = Notification::NotifyUriType::TRASH_PHOTO_URI;
    Notification::NotifyUriType photoAlbumUri = Notification::NotifyUriType::PHOTO_ALBUM_URI;
    Notification::NotifyUriType trashAlbumUri = Notification::NotifyUriType::TRASH_ALBUM_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    observerManager->AddObserver(uri, dataObserver);
    observerManager->AddObserver(trashPhotoUri, dataObserver);
    observerManager->AddObserver(photoAlbumUri, dataObserver);
    observerManager->AddObserver(trashAlbumUri, dataObserver);
    std::vector<Notification::ObserverInfo> observerInfos = observerManager->FindObserver(uri);
    std::vector<Notification::ObserverInfo> trashPhotoUriObs = observerManager->FindObserver(trashPhotoUri);
    std::vector<Notification::ObserverInfo> photoAlbumUriObs = observerManager->FindObserver(photoAlbumUri);
    std::vector<Notification::ObserverInfo> trashAlbumUriObs = observerManager->FindObserver(trashAlbumUri);
    observerMap[uri] = observerInfos;
    observerMap[trashPhotoUri] = trashPhotoUriObs;
    observerMap[photoAlbumUri] = photoAlbumUriObs;
    observerMap[trashAlbumUri] = trashAlbumUriObs;
    std::vector<Notification::NotifyInfo> notifyInfos =
        Notification::NotificationTestData::buildAssetsNotifyInfo("test012", observerMap);
    int32_t ret = Media::Notification::NotificationDistribution::DistributeNotifyInfo(notifyInfos);
    EXPECT_TRUE(ret == E_OK);
    MEDIA_INFO_LOG("end medialib_distribution_test012");
}

HWTEST_F(NotificationDistributionTest, medialib_distribution_test013, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_distribution_test013");
    std::vector<Notification::NotifyInfo> notifyInfos =
        Notification::NotificationTestData::buildAssetsNotifyInfo("test013", {});
    int32_t ret = Media::Notification::NotificationDistribution::DistributeNotifyInfo(notifyInfos);
    EXPECT_TRUE(ret == E_OK);
    MEDIA_INFO_LOG("end medialib_distribution_test013");
}

/**
 * @tc.name: medialib_ProcessUserDefineNotifyInfo_test01
 * @tc.desc: 该接口仅支持 USER_DEFINE_NOTIFY_URI，否则失败
 */
HWTEST_F(NotificationDistributionTest, medialib_ProcessUserDefineNotifyInfo_test01, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_ProcessUserDefineNotifyInfo_test01");
    UserDefineNotifyInfo notifyInfo;
    notifyInfo.notifyUri_ = NotifyUriType::PHOTO_URI;
    int32_t ret = NotificationDistribution::ProcessUserDefineNotifyInfo(notifyInfo);
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("end medialib_ProcessUserDefineNotifyInfo_test01");
}
 
/**
 * @tc.name: medialib_ProcessUserDefineNotifyInfo_test02
 * @tc.desc: 该接口必须定义body部分的类型，否则失败
 */
HWTEST_F(NotificationDistributionTest, medialib_ProcessUserDefineNotifyInfo_test02, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_ProcessUserDefineNotifyInfo_test02");
    UserDefineNotifyInfo notifyInfo;
    notifyInfo.notifyUri_ = NotifyUriType::USER_DEFINE_NOTIFY_URI;
    int32_t ret = NotificationDistribution::ProcessUserDefineNotifyInfo(notifyInfo);
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("end medialib_ProcessUserDefineNotifyInfo_test02");
}
 
/**
 * @tc.name: medialib_ProcessUserDefineNotifyInfo_test03
 * @tc.desc: 该接口需要在客户端注册之后才可使用，否则失败
 */
HWTEST_F(NotificationDistributionTest, medialib_ProcessUserDefineNotifyInfo_test03, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_ProcessUserDefineNotifyInfo_test03");
    UserDefineNotifyInfo notifyInfo;
    notifyInfo.notifyUri_ = NotifyUriType::USER_DEFINE_NOTIFY_URI;
    notifyInfo.notifyUserDefineType_ = NotifyForUserDefineType::MULTISTAGES_CAPTURE;
    int32_t ret = NotificationDistribution::ProcessUserDefineNotifyInfo(notifyInfo);
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("end medialib_ProcessUserDefineNotifyInfo_test03");
}
 
/**
 * @tc.name: medialib_ProcessUserDefineNotifyInfo_test04
 * @tc.desc: mock客户端注册 ，服务端可以正常发送通知
 */
HWTEST_F(NotificationDistributionTest, medialib_ProcessUserDefineNotifyInfo_test04, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_ProcessUserDefineNotifyInfo_test04");
    std::unordered_map<Notification::NotifyUriType, std::vector<Notification::ObserverInfo>> observerMap;
    Notification::NotifyUriType userDefineUri = Notification::NotifyUriType::USER_DEFINE_NOTIFY_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    observerManager->AddObserver(userDefineUri, dataObserver);
 
    UserDefineNotifyInfo notifyInfo;
    notifyInfo.notifyUri_ = NotifyUriType::USER_DEFINE_NOTIFY_URI;
    notifyInfo.notifyUserDefineType_ = NotifyForUserDefineType::MULTISTAGES_CAPTURE;
    int32_t ret = Media::Notification::NotificationDistribution::ProcessUserDefineNotifyInfo(notifyInfo);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end medialib_ProcessUserDefineNotifyInfo_test04");
}
} // namespace Media
} // namespace OHOS