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

#define MLOG_TAG "NotificationUtilsTest"

#include "media_notification_utils_test.h"

#include "media_log.h"
#include "media_notification_utils.h"
#include "parameters.h"
#include "../../include/notification_test_data.h"
#include "data_ability_observer_interface.h"
#include "parcel.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;

namespace OHOS {
namespace Media {
const std::string PARAM_NEED_DC_BASE_QUOTA_ANALYSIS = "persist.multimedia.media_analysis.dc_base_quota_analysis";
const std::string NO_NEED_DC_BASE_QUOTA_ANALYSIS = "0";
const std::string NEED_DC_BASE_QUOTA_ANALYSIS = "1";

const std::string PARAM_NEED_DC_EXTRA_QUOTA_ANALYSIS = "persist.multimedia.media_analysis.dc_extra_quota_analysis";
const std::string NO_NEED_DC_EXTRA_QUOTA_ANALYSIS = "0";
const std::string NEED_DC_EXTRA_QUOTA_ANALYSIS = "1";

const std::string PARAM_NEED_DC_PROACTIVE_ANALYSIS = "persist.multimedia.media_analysis.dc_proactive_analysis";
const std::string NO_NEED_DC_PROACTIVE_ANALYSIS = "0";
const std::string NEED_DC_PROACTIVE_ANALYSIS = "1";
const std::string DB_STATUS_UNAVAILABLE = "unavailable";
const std::string DB_REASON_CORRUPTED = "Database corrupted";
const std::string DB_REASON_CLONE_OCCUPIED = "Database occupied by Clone application";

void NotificationUtilsTest::SetUpTestCase(void) {}

void NotificationUtilsTest::TearDownTestCase(void) {}

void NotificationUtilsTest::SetUp()
{
    baseQuotaVal_ = system::GetParameter(PARAM_NEED_DC_BASE_QUOTA_ANALYSIS, NO_NEED_DC_BASE_QUOTA_ANALYSIS);
    extraQuotaVal_ = system::GetParameter(PARAM_NEED_DC_EXTRA_QUOTA_ANALYSIS, NO_NEED_DC_EXTRA_QUOTA_ANALYSIS);
    proactiveVal_ = system::GetParameter(PARAM_NEED_DC_PROACTIVE_ANALYSIS, NO_NEED_DC_PROACTIVE_ANALYSIS);
    system::SetParameter(PARAM_NEED_DC_BASE_QUOTA_ANALYSIS, NO_NEED_DC_BASE_QUOTA_ANALYSIS);
    system::SetParameter(PARAM_NEED_DC_EXTRA_QUOTA_ANALYSIS, NO_NEED_DC_EXTRA_QUOTA_ANALYSIS);
    system::SetParameter(PARAM_NEED_DC_PROACTIVE_ANALYSIS, NO_NEED_DC_PROACTIVE_ANALYSIS);
}

void NotificationUtilsTest::TearDown(void)
{
    system::SetParameter(PARAM_NEED_DC_BASE_QUOTA_ANALYSIS, baseQuotaVal_);
    system::SetParameter(PARAM_NEED_DC_EXTRA_QUOTA_ANALYSIS, extraQuotaVal_);
    system::SetParameter(PARAM_NEED_DC_PROACTIVE_ANALYSIS, proactiveVal_);
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test001");
    NotificationUtils::UpdateNotificationProp();
    EXPECT_EQ(NEED_DC_BASE_QUOTA_ANALYSIS, system::GetParameter(
        PARAM_NEED_DC_BASE_QUOTA_ANALYSIS, NO_NEED_DC_BASE_QUOTA_ANALYSIS));
    EXPECT_EQ(NEED_DC_EXTRA_QUOTA_ANALYSIS, system::GetParameter(
        PARAM_NEED_DC_EXTRA_QUOTA_ANALYSIS, NO_NEED_DC_EXTRA_QUOTA_ANALYSIS));
    EXPECT_EQ(NEED_DC_PROACTIVE_ANALYSIS, system::GetParameter(
        PARAM_NEED_DC_PROACTIVE_ANALYSIS, NO_NEED_DC_PROACTIVE_ANALYSIS));
    MEDIA_INFO_LOG("end medialib_notification_utils_test001");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test002, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test002");
    Parcel parcel;
    parcel.WriteUint16(static_cast<uint16_t>(Notification::NotifyUriType::PHOTO_URI));
    parcel.WriteUint16(static_cast<uint16_t>(Notification::DownloadAssetsNotifyType::DOWNLOAD_PROGRESS));
    parcel.WriteInt32(100);
    parcel.WriteInt32(50);
    parcel.WriteInt32(0);
    
    std::shared_ptr<Notification::AssetManagerNotifyInfo> notifyInfo =
        NotificationUtils::UnmarshalAssetManagerNotify(parcel);
    ASSERT_NE(notifyInfo, nullptr);
    EXPECT_EQ(notifyInfo->notifyUri, Notification::NotifyUriType::PHOTO_URI);
    EXPECT_EQ(notifyInfo->downloadAssetNotifyType, Notification::DownloadAssetsNotifyType::DOWNLOAD_PROGRESS);
    EXPECT_EQ(notifyInfo->fileId, 100);
    EXPECT_EQ(notifyInfo->percent, 50);
    EXPECT_EQ(notifyInfo->autoPauseReason, 0);
    MEDIA_INFO_LOG("end medialib_notification_utils_test002");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test003, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test003");
    Parcel parcel;
    parcel.WriteUint16(static_cast<uint16_t>(Notification::NotifyUriType::BATCH_DOWNLOAD_PROGRESS_URI));
    parcel.WriteUint16(static_cast<uint16_t>(Notification::DownloadAssetsNotifyType::DOWNLOAD_FINISH));
    parcel.WriteInt32(200);
    parcel.WriteInt32(100);
    parcel.WriteInt32(1);
    
    std::shared_ptr<Notification::AssetManagerNotifyInfo> notifyInfo =
        NotificationUtils::UnmarshalAssetManagerNotify(parcel);
    ASSERT_NE(notifyInfo, nullptr);
    EXPECT_EQ(notifyInfo->notifyUri, Notification::NotifyUriType::BATCH_DOWNLOAD_PROGRESS_URI);
    EXPECT_EQ(notifyInfo->downloadAssetNotifyType, Notification::DownloadAssetsNotifyType::DOWNLOAD_FINISH);
    EXPECT_EQ(notifyInfo->fileId, 200);
    EXPECT_EQ(notifyInfo->percent, 100);
    EXPECT_EQ(notifyInfo->autoPauseReason, 1);
    MEDIA_INFO_LOG("end medialib_notification_utils_test003");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test004, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test004");
    Parcel parcel;
    parcel.WriteUint16(static_cast<uint16_t>(Notification::NotifyUriType::USER_DEFINE_NOTIFY_URI));
    parcel.WriteUint16(static_cast<uint16_t>(Notification::DownloadAssetsNotifyType::DOWNLOAD_FAILED));
    parcel.WriteInt32(300);
    parcel.WriteInt32(0);
    parcel.WriteInt32(2);
    
    std::shared_ptr<Notification::AssetManagerNotifyInfo> notifyInfo =
        NotificationUtils::UnmarshalAssetManagerNotify(parcel);
    ASSERT_NE(notifyInfo, nullptr);
    EXPECT_EQ(notifyInfo->notifyUri, Notification::NotifyUriType::USER_DEFINE_NOTIFY_URI);
    EXPECT_EQ(notifyInfo->downloadAssetNotifyType, Notification::DownloadAssetsNotifyType::DOWNLOAD_FAILED);
    EXPECT_EQ(notifyInfo->fileId, 300);
    EXPECT_EQ(notifyInfo->percent, 0);
    EXPECT_EQ(notifyInfo->autoPauseReason, 2);
    MEDIA_INFO_LOG("end medialib_notification_utils_test004");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test005, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test005");
    Parcel parcel;
    parcel.WriteUint16(static_cast<uint16_t>(Notification::NotifyUriType::PHOTO_URI));
    parcel.WriteUint16(static_cast<uint16_t>(Notification::DownloadAssetsNotifyType::DOWNLOAD_AUTO_PAUSE));
    parcel.WriteInt32(400);
    parcel.WriteInt32(75);
    parcel.WriteInt32(3);
    
    std::shared_ptr<Notification::AssetManagerNotifyInfo> notifyInfo =
        NotificationUtils::UnmarshalAssetManagerNotify(parcel);
    ASSERT_NE(notifyInfo, nullptr);
    EXPECT_EQ(notifyInfo->notifyUri, Notification::NotifyUriType::PHOTO_URI);
    EXPECT_EQ(notifyInfo->downloadAssetNotifyType, Notification::DownloadAssetsNotifyType::DOWNLOAD_AUTO_PAUSE);
    EXPECT_EQ(notifyInfo->fileId, 400);
    EXPECT_EQ(notifyInfo->percent, 75);
    EXPECT_EQ(notifyInfo->autoPauseReason, 3);
    MEDIA_INFO_LOG("end medialib_notification_utils_test005");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test006, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test006");
    Parcel parcel;
    parcel.WriteUint16(static_cast<uint16_t>(Notification::NotifyUriType::PHOTO_URI));
    parcel.WriteUint16(static_cast<uint16_t>(Notification::DownloadAssetsNotifyType::DOWNLOAD_AUTO_RESUME));
    parcel.WriteInt32(500);
    parcel.WriteInt32(80);
    parcel.WriteInt32(4);
    
    std::shared_ptr<Notification::AssetManagerNotifyInfo> notifyInfo =
        NotificationUtils::UnmarshalAssetManagerNotify(parcel);
    ASSERT_NE(notifyInfo, nullptr);
    EXPECT_EQ(notifyInfo->notifyUri, Notification::NotifyUriType::PHOTO_URI);
    EXPECT_EQ(notifyInfo->downloadAssetNotifyType, Notification::DownloadAssetsNotifyType::DOWNLOAD_AUTO_RESUME);
    EXPECT_EQ(notifyInfo->fileId, 500);
    EXPECT_EQ(notifyInfo->percent, 80);
    EXPECT_EQ(notifyInfo->autoPauseReason, 4);
    MEDIA_INFO_LOG("end medialib_notification_utils_test006");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test007, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test007");
    Parcel parcel;
    parcel.WriteUint16(static_cast<uint16_t>(Notification::NotifyUriType::PHOTO_URI));
    parcel.WriteUint16(static_cast<uint16_t>(Notification::DownloadAssetsNotifyType::DOWNLOAD_REFRESH));
    parcel.WriteInt32(600);
    parcel.WriteInt32(90);
    parcel.WriteInt32(5);
    
    std::shared_ptr<Notification::AssetManagerNotifyInfo> notifyInfo =
        NotificationUtils::UnmarshalAssetManagerNotify(parcel);
    ASSERT_NE(notifyInfo, nullptr);
    EXPECT_EQ(notifyInfo->notifyUri, Notification::NotifyUriType::PHOTO_URI);
    EXPECT_EQ(notifyInfo->downloadAssetNotifyType, Notification::DownloadAssetsNotifyType::DOWNLOAD_REFRESH);
    EXPECT_EQ(notifyInfo->fileId, 600);
    EXPECT_EQ(notifyInfo->percent, 90);
    EXPECT_EQ(notifyInfo->autoPauseReason, 5);
    MEDIA_INFO_LOG("end medialib_notification_utils_test007");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test008, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test008");
    Parcel parcel;
    parcel.WriteUint16(static_cast<uint16_t>(Notification::NotifyUriType::HIDDEN_PHOTO_URI));
    parcel.WriteUint16(static_cast<uint16_t>(Notification::DownloadAssetsNotifyType::DOWNLOAD_ASSET_DELETE));
    parcel.WriteInt32(700);
    parcel.WriteInt32(0);
    parcel.WriteInt32(6);
    
    std::shared_ptr<Notification::AssetManagerNotifyInfo> notifyInfo =
        NotificationUtils::UnmarshalAssetManagerNotify(parcel);
    ASSERT_NE(notifyInfo, nullptr);
    EXPECT_EQ(notifyInfo->notifyUri, Notification::NotifyUriType::HIDDEN_PHOTO_URI);
    EXPECT_EQ(notifyInfo->downloadAssetNotifyType, Notification::DownloadAssetsNotifyType::DOWNLOAD_ASSET_DELETE);
    EXPECT_EQ(notifyInfo->fileId, 700);
    EXPECT_EQ(notifyInfo->percent, 0);
    EXPECT_EQ(notifyInfo->autoPauseReason, 6);
    MEDIA_INFO_LOG("end medialib_notification_utils_test008");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test009, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test009");
    Parcel parcel;
    parcel.WriteBool(false);
    parcel.WriteUint16(static_cast<uint16_t>(Notification::NotifyUriType::PHOTO_URI));
    parcel.WriteUint16(static_cast<uint16_t>(Notification::AccurateNotifyType::NOTIFY_ASSET_ADD));
    parcel.WriteBool(true);
    parcel.WriteBool(false);
    
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo =
        NotificationUtils::UnmarshalInMultiMode(parcel);
    ASSERT_NE(mediaChangeInfo, nullptr);
    EXPECT_FALSE(mediaChangeInfo->isForRecheck);
    EXPECT_EQ(mediaChangeInfo->notifyUri, Notification::NotifyUriType::PHOTO_URI);
    EXPECT_EQ(mediaChangeInfo->notifyType, Notification::AccurateNotifyType::NOTIFY_ASSET_ADD);
    EXPECT_TRUE(mediaChangeInfo->isSystem);
    EXPECT_TRUE(mediaChangeInfo->changeInfos.empty());
    MEDIA_INFO_LOG("end medialib_notification_utils_test009");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test010, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test010");
    Parcel parcel;
    parcel.WriteBool(true);
    parcel.WriteUint16(static_cast<uint16_t>(Notification::NotifyUriType::TRASH_PHOTO_URI));
    parcel.WriteUint16(static_cast<uint16_t>(Notification::AccurateNotifyType::NOTIFY_ASSET_REMOVE));
    parcel.WriteBool(false);
    parcel.WriteBool(false);
    
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo =
        NotificationUtils::UnmarshalInMultiMode(parcel);
    ASSERT_NE(mediaChangeInfo, nullptr);
    EXPECT_TRUE(mediaChangeInfo->isForRecheck);
    EXPECT_EQ(mediaChangeInfo->notifyUri, Notification::NotifyUriType::TRASH_PHOTO_URI);
    EXPECT_EQ(mediaChangeInfo->notifyType, Notification::AccurateNotifyType::NOTIFY_ASSET_REMOVE);
    EXPECT_FALSE(mediaChangeInfo->isSystem);
    EXPECT_TRUE(mediaChangeInfo->changeInfos.empty());
    MEDIA_INFO_LOG("end medialib_notification_utils_test010");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test011, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test011");
    Parcel parcel;
    parcel.WriteBool(false);
    parcel.WriteUint16(static_cast<uint16_t>(Notification::NotifyUriType::PHOTO_ALBUM_URI));
    parcel.WriteUint16(static_cast<uint16_t>(Notification::AccurateNotifyType::NOTIFY_ALBUM_ADD));
    parcel.WriteBool(true);
    parcel.WriteBool(false);
    
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo =
        NotificationUtils::UnmarshalInMultiMode(parcel);
    ASSERT_NE(mediaChangeInfo, nullptr);
    EXPECT_FALSE(mediaChangeInfo->isForRecheck);
    EXPECT_EQ(mediaChangeInfo->notifyUri, Notification::NotifyUriType::PHOTO_ALBUM_URI);
    EXPECT_EQ(mediaChangeInfo->notifyType, Notification::AccurateNotifyType::NOTIFY_ALBUM_ADD);
    EXPECT_TRUE(mediaChangeInfo->isSystem);
    EXPECT_TRUE(mediaChangeInfo->changeInfos.empty());
    MEDIA_INFO_LOG("end medialib_notification_utils_test011");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test012, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test012");
    Parcel parcel;
    parcel.WriteBool(true);
    parcel.WriteUint16(static_cast<uint16_t>(Notification::NotifyUriType::HIDDEN_ALBUM_URI));
    parcel.WriteUint16(static_cast<uint16_t>(Notification::AccurateNotifyType::NOTIFY_ALBUM_UPDATE));
    parcel.WriteBool(false);
    parcel.WriteBool(false);
    
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo =
        NotificationUtils::UnmarshalInMultiMode(parcel);
    ASSERT_NE(mediaChangeInfo, nullptr);
    EXPECT_TRUE(mediaChangeInfo->isForRecheck);
    EXPECT_EQ(mediaChangeInfo->notifyUri, Notification::NotifyUriType::HIDDEN_ALBUM_URI);
    EXPECT_EQ(mediaChangeInfo->notifyType, Notification::AccurateNotifyType::NOTIFY_ALBUM_UPDATE);
    EXPECT_FALSE(mediaChangeInfo->isSystem);
    EXPECT_TRUE(mediaChangeInfo->changeInfos.empty());
    MEDIA_INFO_LOG("end medialib_notification_utils_test012");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test013, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test013");
    Parcel parcel;
    parcel.WriteBool(false);
    parcel.WriteUint16(static_cast<uint16_t>(Notification::NotifyUriType::TRASH_ALBUM_URI));
    parcel.WriteUint16(static_cast<uint16_t>(Notification::AccurateNotifyType::NOTIFY_ALBUM_REMOVE));
    parcel.WriteBool(true);
    parcel.WriteBool(false);
    
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo =
        NotificationUtils::UnmarshalInMultiMode(parcel);
    ASSERT_NE(mediaChangeInfo, nullptr);
    EXPECT_FALSE(mediaChangeInfo->isForRecheck);
    EXPECT_EQ(mediaChangeInfo->notifyUri, Notification::NotifyUriType::TRASH_ALBUM_URI);
    EXPECT_EQ(mediaChangeInfo->notifyType, Notification::AccurateNotifyType::NOTIFY_ALBUM_REMOVE);
    EXPECT_TRUE(mediaChangeInfo->isSystem);
    EXPECT_TRUE(mediaChangeInfo->changeInfos.empty());
    MEDIA_INFO_LOG("end medialib_notification_utils_test013");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test014, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test014");
    Parcel parcel;
    parcel.WriteBool(true);
    parcel.WriteUint16(static_cast<uint16_t>(Notification::NotifyUriType::ANALYSIS_ALBUM_URI));
    parcel.WriteUint16(static_cast<uint16_t>(Notification::AccurateNotifyType::NOTIFY_ASSET_UPDATE));
    parcel.WriteBool(false);
    parcel.WriteBool(false);
    
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo =
        NotificationUtils::UnmarshalInMultiMode(parcel);
    ASSERT_NE(mediaChangeInfo, nullptr);
    EXPECT_TRUE(mediaChangeInfo->isForRecheck);
    EXPECT_EQ(mediaChangeInfo->notifyUri, Notification::NotifyUriType::ANALYSIS_ALBUM_URI);
    EXPECT_EQ(mediaChangeInfo->notifyType, Notification::AccurateNotifyType::NOTIFY_ASSET_UPDATE);
    EXPECT_FALSE(mediaChangeInfo->isSystem);
    EXPECT_TRUE(mediaChangeInfo->changeInfos.empty());
    MEDIA_INFO_LOG("end medialib_notification_utils_test014");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test015, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test015");
    Parcel parcel;
    parcel.WriteBool(false);
    parcel.WriteUint16(static_cast<uint16_t>(Notification::NotifyUriType::SINGLE_PHOTO_URI));
    parcel.WriteUint16(static_cast<uint16_t>(Notification::AccurateNotifyType::NOTIFY_ASSET_YUV_READY));
    parcel.WriteBool(true);
    parcel.WriteBool(false);
    
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo =
        NotificationUtils::UnmarshalInMultiMode(parcel);
    ASSERT_NE(mediaChangeInfo, nullptr);
    EXPECT_FALSE(mediaChangeInfo->isForRecheck);
    EXPECT_EQ(mediaChangeInfo->notifyUri, Notification::NotifyUriType::SINGLE_PHOTO_URI);
    EXPECT_EQ(mediaChangeInfo->notifyType, Notification::AccurateNotifyType::NOTIFY_ASSET_YUV_READY);
    EXPECT_TRUE(mediaChangeInfo->isSystem);
    EXPECT_TRUE(mediaChangeInfo->changeInfos.empty());
    MEDIA_INFO_LOG("end medialib_notification_utils_test015");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test016, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test016");
    Parcel parcel;
    parcel.WriteBool(true);
    parcel.WriteUint16(static_cast<uint16_t>(Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI));
    parcel.WriteUint16(static_cast<uint16_t>(Notification::AccurateNotifyType::NOTIFY_ASSET_ADD));
    parcel.WriteBool(false);
    parcel.WriteBool(false);
    
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo =
        NotificationUtils::UnmarshalInMultiMode(parcel);
    ASSERT_NE(mediaChangeInfo, nullptr);
    EXPECT_TRUE(mediaChangeInfo->isForRecheck);
    EXPECT_EQ(mediaChangeInfo->notifyUri, Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI);
    EXPECT_EQ(mediaChangeInfo->notifyType, Notification::AccurateNotifyType::NOTIFY_ASSET_ADD);
    EXPECT_FALSE(mediaChangeInfo->isSystem);
    EXPECT_TRUE(mediaChangeInfo->changeInfos.empty());
    MEDIA_INFO_LOG("end medialib_notification_utils_test016");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test017, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test017");
    Parcel parcel;
    parcel.WriteBool(false);
    parcel.WriteUint16(static_cast<uint16_t>(Notification::NotifyUriType::BATCH_DOWNLOAD_PROGRESS_URI));
    parcel.WriteUint16(static_cast<uint16_t>(Notification::AccurateNotifyType::NOTIFY_ASSET_REMOVE));
    parcel.WriteBool(true);
    parcel.WriteBool(false);
    
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo =
        NotificationUtils::UnmarshalInMultiMode(parcel);
    ASSERT_NE(mediaChangeInfo, nullptr);
    EXPECT_FALSE(mediaChangeInfo->isForRecheck);
    EXPECT_EQ(mediaChangeInfo->notifyUri, Notification::NotifyUriType::BATCH_DOWNLOAD_PROGRESS_URI);
    EXPECT_EQ(mediaChangeInfo->notifyType, Notification::AccurateNotifyType::NOTIFY_ASSET_REMOVE);
    EXPECT_TRUE(mediaChangeInfo->isSystem);
    EXPECT_TRUE(mediaChangeInfo->changeInfos.empty());
    MEDIA_INFO_LOG("end medialib_notification_utils_test017");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test018, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test018");
    Parcel parcel;
    parcel.WriteBool(true);
    parcel.WriteUint16(static_cast<uint16_t>(Notification::NotifyUriType::USER_DEFINE_NOTIFY_URI));
    parcel.WriteUint16(static_cast<uint16_t>(Notification::AccurateNotifyType::NOTIFY_ALBUM_UPDATE));
    parcel.WriteBool(false);
    parcel.WriteBool(false);
    
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo =
        NotificationUtils::UnmarshalInMultiMode(parcel);
    ASSERT_NE(mediaChangeInfo, nullptr);
    EXPECT_TRUE(mediaChangeInfo->isForRecheck);
    EXPECT_EQ(mediaChangeInfo->notifyUri, Notification::NotifyUriType::USER_DEFINE_NOTIFY_URI);
    EXPECT_EQ(mediaChangeInfo->notifyType, Notification::AccurateNotifyType::NOTIFY_ALBUM_UPDATE);
    EXPECT_FALSE(mediaChangeInfo->isSystem);
    EXPECT_TRUE(mediaChangeInfo->changeInfos.empty());
    MEDIA_INFO_LOG("end medialib_notification_utils_test018");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test019, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test019");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo =
        std::make_shared<Notification::MediaChangeInfo>();
    mediaChangeInfo->isForRecheck = false;
    mediaChangeInfo->notifyUri = Notification::NotifyUriType::PHOTO_URI;
    mediaChangeInfo->notifyType = Notification::AccurateNotifyType::NOTIFY_ASSET_ADD;
    mediaChangeInfo->isSystem = true;
    
    int32_t result = NotificationUtils::SendNotification(dataObserver, mediaChangeInfo);
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("end medialib_notification_utils_test019");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test020, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test020");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo =
        std::make_shared<Notification::MediaChangeInfo>();
    mediaChangeInfo->isForRecheck = true;
    mediaChangeInfo->notifyUri = Notification::NotifyUriType::TRASH_PHOTO_URI;
    mediaChangeInfo->notifyType = Notification::AccurateNotifyType::NOTIFY_ASSET_REMOVE;
    mediaChangeInfo->isSystem = false;
    
    int32_t result = NotificationUtils::SendNotification(dataObserver, mediaChangeInfo);
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("end medialib_notification_utils_test020");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test021, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test021");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = nullptr;
    
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo =
        std::make_shared<Notification::MediaChangeInfo>();
    mediaChangeInfo->isForRecheck = false;
    mediaChangeInfo->notifyUri = Notification::NotifyUriType::PHOTO_URI;
    mediaChangeInfo->notifyType = Notification::AccurateNotifyType::NOTIFY_ASSET_ADD;
    mediaChangeInfo->isSystem = true;
    
    int32_t result = NotificationUtils::SendNotification(dataObserver, mediaChangeInfo);
    EXPECT_EQ(result, E_ERR);
    MEDIA_INFO_LOG("end medialib_notification_utils_test021");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test022, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test022");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo = nullptr;
    
    int32_t result = NotificationUtils::SendNotification(dataObserver, mediaChangeInfo);
    EXPECT_EQ(result, E_ERR);
    MEDIA_INFO_LOG("end medialib_notification_utils_test022");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test023, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test023");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = nullptr;
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo = nullptr;
    
    int32_t result = NotificationUtils::SendNotification(dataObserver, mediaChangeInfo);
    EXPECT_EQ(result, E_ERR);
    MEDIA_INFO_LOG("end medialib_notification_utils_test023");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test024, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test024");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo =
        std::make_shared<Notification::MediaChangeInfo>();
    mediaChangeInfo->isForRecheck = false;
    mediaChangeInfo->notifyUri = Notification::NotifyUriType::PHOTO_ALBUM_URI;
    mediaChangeInfo->notifyType = Notification::AccurateNotifyType::NOTIFY_ALBUM_ADD;
    mediaChangeInfo->isSystem = true;
    
    int32_t result = NotificationUtils::SendNotification(dataObserver, mediaChangeInfo);
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("end medialib_notification_utils_test024");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test025, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test025");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo =
        std::make_shared<Notification::MediaChangeInfo>();
    mediaChangeInfo->isForRecheck = true;
    mediaChangeInfo->notifyUri = Notification::NotifyUriType::HIDDEN_ALBUM_URI;
    mediaChangeInfo->notifyType = Notification::AccurateNotifyType::NOTIFY_ALBUM_UPDATE;
    mediaChangeInfo->isSystem = false;
    
    int32_t result = NotificationUtils::SendNotification(dataObserver, mediaChangeInfo);
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("end medialib_notification_utils_test025");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test026, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test026");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo =
        std::make_shared<Notification::MediaChangeInfo>();
    mediaChangeInfo->isForRecheck = false;
    mediaChangeInfo->notifyUri = Notification::NotifyUriType::TRASH_ALBUM_URI;
    mediaChangeInfo->notifyType = Notification::AccurateNotifyType::NOTIFY_ALBUM_REMOVE;
    mediaChangeInfo->isSystem = true;
    
    int32_t result = NotificationUtils::SendNotification(dataObserver, mediaChangeInfo);
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("end medialib_notification_utils_test026");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test027, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test027");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo =
        std::make_shared<Notification::MediaChangeInfo>();
    mediaChangeInfo->isForRecheck = true;
    mediaChangeInfo->notifyUri = Notification::NotifyUriType::ANALYSIS_ALBUM_URI;
    mediaChangeInfo->notifyType = Notification::AccurateNotifyType::NOTIFY_ASSET_UPDATE;
    mediaChangeInfo->isSystem = false;
    
    int32_t result = NotificationUtils::SendNotification(dataObserver, mediaChangeInfo);
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("end medialib_notification_utils_test027");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test028, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test028");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo =
        std::make_shared<Notification::MediaChangeInfo>();
    mediaChangeInfo->isForRecheck = false;
    mediaChangeInfo->notifyUri = Notification::NotifyUriType::SINGLE_PHOTO_URI;
    mediaChangeInfo->notifyType = Notification::AccurateNotifyType::NOTIFY_ASSET_YUV_READY;
    mediaChangeInfo->isSystem = true;
    
    int32_t result = NotificationUtils::SendNotification(dataObserver, mediaChangeInfo);
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("end medialib_notification_utils_test028");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test029, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test029");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo =
        std::make_shared<Notification::MediaChangeInfo>();
    mediaChangeInfo->isForRecheck = true;
    mediaChangeInfo->notifyUri = Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI;
    mediaChangeInfo->notifyType = Notification::AccurateNotifyType::NOTIFY_ASSET_ADD;
    mediaChangeInfo->isSystem = false;
    
    int32_t result = NotificationUtils::SendNotification(dataObserver, mediaChangeInfo);
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("end medialib_notification_utils_test029");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test030, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test030");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo =
        std::make_shared<Notification::MediaChangeInfo>();
    mediaChangeInfo->isForRecheck = false;
    mediaChangeInfo->notifyUri = Notification::NotifyUriType::BATCH_DOWNLOAD_PROGRESS_URI;
    mediaChangeInfo->notifyType = Notification::AccurateNotifyType::NOTIFY_ASSET_REMOVE;
    mediaChangeInfo->isSystem = true;
    
    int32_t result = NotificationUtils::SendNotification(dataObserver, mediaChangeInfo);
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("end medialib_notification_utils_test030");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test031, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test031");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo =
        std::make_shared<Notification::MediaChangeInfo>();
    mediaChangeInfo->isForRecheck = true;
    mediaChangeInfo->notifyUri = Notification::NotifyUriType::USER_DEFINE_NOTIFY_URI;
    mediaChangeInfo->notifyType = Notification::AccurateNotifyType::NOTIFY_ALBUM_UPDATE;
    mediaChangeInfo->isSystem = false;
    
    int32_t result = NotificationUtils::SendNotification(dataObserver, mediaChangeInfo);
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("end medialib_notification_utils_test031");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test032, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test032");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<AAFwk::ChangeInfo> changeInfo = std::make_shared<AAFwk::ChangeInfo>();
    uint8_t* data = new (std::nothrow) uint8_t[100];
    ASSERT_NE(data, nullptr);
    for (int i = 0; i < 100; i++) {
        data[i] = static_cast<uint8_t>(i);
    }
    changeInfo->data_ = data;
    changeInfo->size_ = 100;
    
    int32_t result = NotificationUtils::SendDownloadProgressInfoNotification(dataObserver, changeInfo);
    EXPECT_EQ(result, E_OK);
    
    if (changeInfo->data_ != nullptr) {
        delete[] static_cast<uint8_t*>(changeInfo->data_);
        changeInfo->data_ = nullptr;
    }
    MEDIA_INFO_LOG("end medialib_notification_utils_test032");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test033, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test033");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<AAFwk::ChangeInfo> changeInfo = std::make_shared<AAFwk::ChangeInfo>();
    uint8_t* data = new (std::nothrow) uint8_t[256];
    ASSERT_NE(data, nullptr);
    for (int i = 0; i < 256; i++) {
        data[i] = static_cast<uint8_t>(255 - i);
    }
    changeInfo->data_ = data;
    changeInfo->size_ = 256;
    
    int32_t result = NotificationUtils::SendDownloadProgressInfoNotification(dataObserver, changeInfo);
    EXPECT_EQ(result, E_OK);
    
    if (changeInfo->data_ != nullptr) {
        delete[] static_cast<uint8_t*>(changeInfo->data_);
        changeInfo->data_ = nullptr;
    }
    MEDIA_INFO_LOG("end end medialib_notification_utils_test033");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test034, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test034");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = nullptr;
    
    std::shared_ptr<AAFwk::ChangeInfo> changeInfo = std::make_shared<AAFwk::ChangeInfo>();
    uint8_t* data = new (std::nothrow) uint8_t[50];
    ASSERT_NE(data, nullptr);
    changeInfo->data_ = data;
    changeInfo->size_ = 50;
    
    int32_t result = NotificationUtils::SendDownloadProgressInfoNotification(dataObserver, changeInfo);
    EXPECT_EQ(result, E_ERR);
    
    if (changeInfo->data_ != nullptr) {
        delete[] static_cast<uint8_t*>(changeInfo->data_);
        changeInfo->data_ = nullptr;
    }
    MEDIA_INFO_LOG("end medialib_notification_utils_test034");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test035, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test035");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<AAFwk::ChangeInfo> changeInfo = nullptr;
    
    int32_t result = NotificationUtils::SendDownloadProgressInfoNotification(dataObserver, changeInfo);
    EXPECT_EQ(result, E_ERR);
    MEDIA_INFO_LOG("end medialib_notification_utils_test035");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test036, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test036");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = nullptr;
    std::shared_ptr<AAFwk::ChangeInfo> changeInfo = nullptr;
    
    int32_t result = NotificationUtils::SendDownloadProgressInfoNotification(dataObserver, changeInfo);
    EXPECT_EQ(result, E_ERR);
    MEDIA_INFO_LOG("end medialib_notification_utils_test036");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test037, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test037");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<AAFwk::ChangeInfo> changeInfo = std::make_shared<AAFwk::ChangeInfo>();
    uint8_t* data = new (std::nothrow) uint8_t[512];
    ASSERT_NE(data, nullptr);
    for (int i = 0; i < 512; i++) {
        data[i] = static_cast<uint8_t>(i % 256);
    }
    changeInfo->data_ = data;
    changeInfo->size_ = 512;
    
    int32_t result = NotificationUtils::SendDownloadProgressInfoNotification(dataObserver, changeInfo);
    EXPECT_EQ(result, E_OK);
    
    if (changeInfo->data_ != nullptr) {
        delete[] static_cast<uint8_t*>(changeInfo->data_);
        changeInfo->data_ = nullptr;
    }
    MEDIA_INFO_LOG("end medialib_notification_utils_test037");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test038, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test038");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<AAFwk::ChangeInfo> changeInfo = std::make_shared<AAFwk::ChangeInfo>();
    uint8_t* data = new (std::nothrow) uint8_t[1024];
    ASSERT_NE(data, nullptr);
    memset_s(data, 1024, 0xAA, 1024);
    changeInfo->data_ = data;
    changeInfo->size_ = 1024;
    
    int32_t result = NotificationUtils::SendDownloadProgressInfoNotification(dataObserver, changeInfo);
    EXPECT_EQ(result, E_OK);
    
    if (changeInfo->data_ != nullptr) {
        delete[] static_cast<uint8_t*>(changeInfo->data_);
        changeInfo->data_ = nullptr;
    }
    MEDIA_INFO_LOG("end medialib_notification_utils_test038");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test039, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test039");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<AAFwk::ChangeInfo> changeInfo = std::make_shared<AAFwk::ChangeInfo>();
    uint8_t* data = new (std::nothrow) uint8_t[1];
    ASSERT_NE(data, nullptr);
    data[0] = 0xFF;
    changeInfo->data_ = data;
    changeInfo->size_ = 1;
    
    int32_t result = NotificationUtils::SendDownloadProgressInfoNotification(dataObserver, changeInfo);
    EXPECT_EQ(result, E_OK);
    
    if (changeInfo->data_ != nullptr) {
        delete[] static_cast<uint8_t*>(changeInfo->data_);
        changeInfo->data_ = nullptr;
    }
    MEDIA_INFO_LOG("end medialib_notification_utils_test039");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test040, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test040");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<AAFwk::ChangeInfo> changeInfo = std::make_shared<AAFwk::ChangeInfo>();
    uint8_t* data = new (std::nothrow) uint8_t[2048];
    ASSERT_NE(data, nullptr);
    for (int i = 0; i < 2048; i++) {
        data[i] = static_cast<uint8_t>((i * 2) % 256);
    }
    changeInfo->data_ = data;
    changeInfo->size_ = 2048;
    
    int32_t result = NotificationUtils::SendDownloadProgressInfoNotification(dataObserver, changeInfo);
    EXPECT_EQ(result, E_OK);
    
    if (changeInfo->data_ != nullptr) {
        delete[] static_cast<uint8_t*>(changeInfo->data_);
        changeInfo->data_ = nullptr;
    }
    MEDIA_INFO_LOG("end medialib_notification_utils_test040");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test041, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test041");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<AAFwk::ChangeInfo> changeInfo = std::make_shared<AAFwk::ChangeInfo>();
    uint8_t* data = new (std::nothrow) uint8_t[4096];
    ASSERT_NE(data, nullptr);
    memset_s(data, 4096, 0x55, 4096);
    changeInfo->data_ = data;
    changeInfo->size_ = 4096;
    
    int32_t result = NotificationUtils::SendDownloadProgressInfoNotification(dataObserver, changeInfo);
    EXPECT_EQ(result, E_OK);
    
    if (changeInfo->data_ != nullptr) {
        delete[] static_cast<uint8_t*>(changeInfo->data_);
        changeInfo->data_ = nullptr;
    }
    MEDIA_INFO_LOG("end medialib_notification_utils_test041");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test042, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test042");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<AAFwk::ChangeInfo> changeInfo = std::make_shared<AAFwk::ChangeInfo>();
    uint8_t* data = new (std::nothrow) uint8_t[8192];
    ASSERT_NE(data, nullptr);
    for (int i = 0; i < 8192; i++) {
        data[i] = static_cast<uint8_t>(i % 128);
    }
    changeInfo->data_ = data;
    changeInfo->size_ = 8192;
    
    int32_t result = NotificationUtils::SendDownloadProgressInfoNotification(dataObserver, changeInfo);
    EXPECT_EQ(result, E_OK);
    
    if (changeInfo->data_ != nullptr) {
        delete[] static_cast<uint8_t*>(changeInfo->data_);
        changeInfo->data_ = nullptr;
    }
    MEDIA_INFO_LOG("end medialib_notification_utils_test042");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test043, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test043");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<AAFwk::ChangeInfo> changeInfo = std::make_shared<AAFwk::ChangeInfo>();
    uint8_t* data = new (std::nothrow) uint8_t[10];
    ASSERT_NE(data, nullptr);
    for (int i = 0; i < 10; i++) {
        data[i] = static_cast<uint8_t>(i * 10);
    }
    changeInfo->data_ = data;
    changeInfo->size_ = 10;
    
    int32_t result = NotificationUtils::SendUserDefineNotification(dataObserver, changeInfo);
    EXPECT_EQ(result, E_OK);
    
    if (changeInfo->data_ != nullptr) {
        delete[] static_cast<uint8_t*>(changeInfo->data_);
        changeInfo->data_ = nullptr;
    }
    MEDIA_INFO_LOG("end medialib_notification_utils_test043");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test044, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test044");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<AAFwk::ChangeInfo> changeInfo = std::make_shared<AAFwk::ChangeInfo>();
    uint8_t* data = new (std::nothrow) uint8_t[20];
    ASSERT_NE(data, nullptr);
    memset_s(data, 20, 0xBB, 20);
    changeInfo->data_ = data;
    changeInfo->size_ = 20;
    
    int32_t result = NotificationUtils::SendUserDefineNotification(dataObserver, changeInfo);
    EXPECT_EQ(result, E_OK);
    
    if (changeInfo->data_ != nullptr) {
        delete[] static_cast<uint8_t*>(changeInfo->data_);
        changeInfo->data_ = nullptr;
    }
    MEDIA_INFO_LOG("end medialib_notification_utils_test044");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test045, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test045");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = nullptr;
    
    std::shared_ptr<AAFwk::ChangeInfo> changeInfo = std::make_shared<AAFwk::ChangeInfo>();
    uint8_t* data = new (std::nothrow) uint8_t[30];
    ASSERT_NE(data, nullptr);
    changeInfo->data_ = data;
    changeInfo->size_ = 30;
    
    int32_t result = NotificationUtils::SendUserDefineNotification(dataObserver, changeInfo);
    EXPECT_EQ(result, E_ERR);
    
    if (changeInfo->data_ != nullptr) {
        delete[] static_cast<uint8_t*>(changeInfo->data_);
        changeInfo->data_ = nullptr;
    }
    MEDIA_INFO_LOG("end medialib_notification_utils_test045");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test046, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test046");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<AAFwk::ChangeInfo> changeInfo = nullptr;
    
    int32_t result = NotificationUtils::SendUserDefineNotification(dataObserver, changeInfo);
    EXPECT_EQ(result, E_ERR);
    MEDIA_INFO_LOG("end medialib_notification_utils_test046");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test047, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test047");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = nullptr;
    std::shared_ptr<AAFwk::ChangeInfo> changeInfo = nullptr;
    
    int32_t result = NotificationUtils::SendUserDefineNotification(dataObserver, changeInfo);
    EXPECT_EQ(result, E_ERR);
    MEDIA_INFO_LOG("end medialib_notification_utils_test047");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test048, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test048");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<AAFwk::ChangeInfo> changeInfo = std::make_shared<AAFwk::ChangeInfo>();
    uint8_t* data = new (std::nothrow) uint8_t[40];
    ASSERT_NE(data, nullptr);
    for (int i = 0; i < 40; i++) {
        data[i] = static_cast<uint8_t>(i * 3);
    }
    changeInfo->data_ = data;
    changeInfo->size_ = 40;
    
    int32_t result = NotificationUtils::SendUserDefineNotification(dataObserver, changeInfo);
    EXPECT_EQ(result, E_OK);
    
    if (changeInfo->data_ != nullptr) {
        delete[] static_cast<uint8_t*>(changeInfo->data_);
        changeInfo->data_ = nullptr;
    }
    MEDIA_INFO_LOG("end medialib_notification_utils_test048");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test049, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test049");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<AAFwk::ChangeInfo> changeInfo = std::make_shared<AAFwk::ChangeInfo>();
    uint8_t* data = new (std::nothrow) uint8_t[60];
    ASSERT_NE(data, nullptr);
    memset_s(data, 60, 0xCC, 60);
    changeInfo->data_ = data;
    changeInfo->size_ = 60;
    
    int32_t result = NotificationUtils::SendUserDefineNotification(dataObserver, changeInfo);
    EXPECT_EQ(result, E_OK);
    
    if (changeInfo->data_ != nullptr) {
        delete[] static_cast<uint8_t*>(changeInfo->data_);
        changeInfo->data_ = nullptr;
    }
    MEDIA_INFO_LOG("end medialib_notification_utils_test049");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test050, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test050");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<AAFwk::ChangeInfo> changeInfo = std::make_shared<AAFwk::ChangeInfo>();
    uint8_t* data = new (std::nothrow) uint8_t[80];
    ASSERT_NE(data, nullptr);
    for (int i = 0; i < 80; i++) {
        data[i] = static_cast<uint8_t>((i * 5) % 256);
    }
    changeInfo->data_ = data;
    changeInfo->size_ = 80;
    
    int32_t result = NotificationUtils::SendUserDefineNotification(dataObserver, changeInfo);
    EXPECT_EQ(result, E_OK);
    
    if (changeInfo->data_ != nullptr) {
        delete[] static_cast<uint8_t*>(changeInfo->data_);
        changeInfo->data_ = nullptr;
    }
    MEDIA_INFO_LOG("end medialib_notification_utils_test050");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test051, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test051");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<AAFwk::ChangeInfo> changeInfo = std::make_shared<AAFwk::ChangeInfo>();
    uint8_t* data = new (std::nothrow) uint8_t[100];
    ASSERT_NE(data, nullptr);
    for (int i = 0; i < 100; i++) {
        data[i] = static_cast<uint8_t>(i);
    }
    changeInfo->data_ = data;
    changeInfo->size_ = 100;
    
    int32_t result = NotificationUtils::SendUserDefineNotification(dataObserver, changeInfo);
    EXPECT_EQ(result, E_OK);
    
    if (changeInfo->data_ != nullptr) {
        delete[] static_cast<uint8_t*>(changeInfo->data_);
        changeInfo->data_ = nullptr;
    }
    MEDIA_INFO_LOG("end medialib_notification_utils_test051");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test052, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test052");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<AAFwk::ChangeInfo> changeInfo = std::make_shared<AAFwk::ChangeInfo>();
    uint8_t* data = new (std::nothrow) uint8_t[128];
    ASSERT_NE(data, nullptr);
    memset_s(data, 128, 0xDD, 128);
    changeInfo->data_ = data;
    changeInfo->size_ = 128;
    
    int32_t result = NotificationUtils::SendUserDefineNotification(dataObserver, changeInfo);
    EXPECT_EQ(result, E_OK);
    
    if (changeInfo->data_ != nullptr) {
        delete[] static_cast<uint8_t*>(changeInfo->data_);
        changeInfo->data_ = nullptr;
    }
    MEDIA_INFO_LOG("end medialib_notification_utils_test052");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test053, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test053");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<AAFwk::ChangeInfo> changeInfo = std::make_shared<AAFwk::ChangeInfo>();
    uint8_t* data = new (std::nothrow) uint8_t[256];
    ASSERT_NE(data, nullptr);
    for (int i = 0; i < 256; i++) {
        data[i] = static_cast<uint8_t>((i * 2) % 256);
    }
    changeInfo->data_ = data;
    changeInfo->size_ = 256;
    
    int32_t result = NotificationUtils::SendUserDefineNotification(dataObserver, changeInfo);
    EXPECT_EQ(result, E_OK);
    
    if (changeInfo->data_ != nullptr) {
        delete[] static_cast<uint8_t*>(changeInfo->data_);
        changeInfo->data_ = nullptr;
    }
    MEDIA_INFO_LOG("end medialib_notification_utils_test053");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test054, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test054");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<AAFwk::ChangeInfo> changeInfo = std::make_shared<AAFwk::ChangeInfo>();
    uint8_t* data = new (std::nothrow) uint8_t[512];
    ASSERT_NE(data, nullptr);
    memset_s(data, 512, 0xEE, 512);
    changeInfo->data_ = data;
    changeInfo->size_ = 512;
    
    int32_t result = NotificationUtils::SendUserDefineNotification(dataObserver, changeInfo);
    EXPECT_EQ(result, E_OK);
    
    if (changeInfo->data_ != nullptr) {
        delete[] static_cast<uint8_t*>(changeInfo->data_);
        changeInfo->data_ = nullptr;
    }
    MEDIA_INFO_LOG("end medialib_notification_utils_test054");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test055, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test055");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<AAFwk::ChangeInfo> changeInfo = std::make_shared<AAFwk::ChangeInfo>();
    uint8_t* data = new (std::nothrow) uint8_t[1024];
    ASSERT_NE(data, nullptr);
    for (int i = 0; i < 1024; i++) {
        data[i] = static_cast<uint8_t>(255 - i);
    }
    changeInfo->data_ = data;
    changeInfo->size_ = 1024;
    
    int32_t result = NotificationUtils::SendUserDefineNotification(dataObserver, changeInfo);
    EXPECT_EQ(result, E_OK);
    
    if (changeInfo->data_ != nullptr) {
        delete[] static_cast<uint8_t*>(changeInfo->data_);
        changeInfo->data_ = nullptr;
    }
    MEDIA_INFO_LOG("end medialib_notification_utils_test055");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test056, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test056");
    NotificationUtils::UpdateNotificationProp();
    EXPECT_EQ(NEED_DC_BASE_QUOTA_ANALYSIS, system::GetParameter(
        PARAM_NEED_DC_BASE_QUOTA_ANALYSIS, NO_NEED_DC_BASE_QUOTA_ANALYSIS));
    EXPECT_EQ(NEED_DC_EXTRA_QUOTA_ANALYSIS, system::GetParameter(
        PARAM_NEED_DC_EXTRA_QUOTA_ANALYSIS, NO_NEED_DC_EXTRA_QUOTA_ANALYSIS));
    EXPECT_EQ(NEED_DC_PROACTIVE_ANALYSIS, system::GetParameter(
        PARAM_NEED_DC_PROACTIVE_ANALYSIS, NO_NEED_DC_PROACTIVE_ANALYSIS));
    MEDIA_INFO_LOG("end medialib_notification_utils_test056");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test057, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test057");
    system::SetParameter(PARAM_NEED_DC_BASE_QUOTA_ANALYSIS, NEED_DC_BASE_QUOTA_ANALYSIS);
    system::SetParameter(PARAM_NEED_DC_EXTRA_QUOTA_ANALYSIS, NEED_DC_EXTRA_QUOTA_ANALYSIS);
    system::SetParameter(PARAM_NEED_DC_PROACTIVE_ANALYSIS, NEED_DC_PROACTIVE_ANALYSIS);
    
    NotificationUtils::UpdateNotificationProp();
    EXPECT_EQ(NEED_DC_BASE_QUOTA_ANALYSIS, system::GetParameter(
        PARAM_NEED_DC_BASE_QUOTA_ANALYSIS, NO_NEED_DC_BASE_QUOTA_ANALYSIS));
    EXPECT_EQ(NEED_DC_EXTRA_QUOTA_ANALYSIS, system::GetParameter(
        PARAM_NEED_DC_EXTRA_QUOTA_ANALYSIS, NO_NEED_DC_EXTRA_QUOTA_ANALYSIS));
    EXPECT_EQ(NEED_DC_PROACTIVE_ANALYSIS, system::GetParameter(
        PARAM_NEED_DC_PROACTIVE_ANALYSIS, NO_NEED_DC_PROACTIVE_ANALYSIS));
    MEDIA_INFO_LOG("end medialib_notification_utils_test057");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test058, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test058");
    system::SetParameter(PARAM_NEED_DC_BASE_QUOTA_ANALYSIS, NO_NEED_DC_BASE_QUOTA_ANALYSIS);
    system::SetParameter(PARAM_NEED_DC_EXTRA_QUOTA_ANALYSIS, NO_NEED_DC_EXTRA_QUOTA_ANALYSIS);
    system::SetParameter(PARAM_NEED_DC_PROACTIVE_ANALYSIS, NO_NEED_DC_PROACTIVE_ANALYSIS);
    
    NotificationUtils::UpdateNotificationProp();
    EXPECT_EQ(NEED_DC_BASE_QUOTA_ANALYSIS, system::GetParameter(
        PARAM_NEED_DC_BASE_QUOTA_ANALYSIS, NO_NEED_DC_BASE_QUOTA_ANALYSIS));
    EXPECT_EQ(NEED_DC_EXTRA_QUOTA_ANALYSIS, system::GetParameter(
        PARAM_NEED_DC_EXTRA_QUOTA_ANALYSIS, NO_NEED_DC_EXTRA_QUOTA_ANALYSIS));
    EXPECT_EQ(NEED_DC_PROACTIVE_ANALYSIS, system::GetParameter(
        PARAM_NEED_DC_PROACTIVE_ANALYSIS, NO_NEED_DC_PROACTIVE_ANALYSIS));
    MEDIA_INFO_LOG("end medialib_notification_utils_test058");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test059, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test059");
    system::SetParameter(PARAM_NEED_DC_BASE_QUOTA_ANALYSIS, NEED_DC_BASE_QUOTA_ANALYSIS);
    system::SetParameter(PARAM_NEED_DC_EXTRA_QUOTA_ANALYSIS, NO_NEED_DC_EXTRA_QUOTA_ANALYSIS);
    system::SetParameter(PARAM_NEED_DC_PROACTIVE_ANALYSIS, NO_NEED_DC_PROACTIVE_ANALYSIS);
    
    NotificationUtils::UpdateNotificationProp();
    EXPECT_EQ(NEED_DC_BASE_QUOTA_ANALYSIS, system::GetParameter(
        PARAM_NEED_DC_BASE_QUOTA_ANALYSIS, NO_NEED_DC_BASE_QUOTA_ANALYSIS));
    EXPECT_EQ(NEED_DC_EXTRA_QUOTA_ANALYSIS, system::GetParameter(
        PARAM_NEED_DC_EXTRA_QUOTA_ANALYSIS, NO_NEED_DC_EXTRA_QUOTA_ANALYSIS));
    EXPECT_EQ(NEED_DC_PROACTIVE_ANALYSIS, system::GetParameter(
        PARAM_NEED_DC_PROACTIVE_ANALYSIS, NO_NEED_DC_PROACTIVE_ANALYSIS));
    MEDIA_INFO_LOG("end medialib_notification_utils_test059");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test060, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test060");
    system::SetParameter(PARAM_NEED_DC_BASE_QUOTA_ANALYSIS, NO_NEED_DC_BASE_QUOTA_ANALYSIS);
    system::SetParameter(PARAM_NEED_DC_EXTRA_QUOTA_ANALYSIS, NEED_DC_EXTRA_QUOTA_ANALYSIS);
    system::SetParameter(PARAM_NEED_DC_PROACTIVE_ANALYSIS, NO_NEED_DC_PROACTIVE_ANALYSIS);
    
    NotificationUtils::UpdateNotificationProp();
    EXPECT_EQ(NEED_DC_BASE_QUOTA_ANALYSIS, system::GetParameter(
        PARAM_NEED_DC_BASE_QUOTA_ANALYSIS, NO_NEED_DC_BASE_QUOTA_ANALYSIS));
    EXPECT_EQ(NEED_DC_EXTRA_QUOTA_ANALYSIS, system::GetParameter(
        PARAM_NEED_DC_EXTRA_QUOTA_ANALYSIS, NO_NEED_DC_EXTRA_QUOTA_ANALYSIS));
    EXPECT_EQ(NEED_DC_PROACTIVE_ANALYSIS, system::GetParameter(
        PARAM_NEED_DC_PROACTIVE_ANALYSIS, NO_NEED_DC_PROACTIVE_ANALYSIS));
    MEDIA_INFO_LOG("end medialib_notification_utils_test060");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test061, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test061");
    system::SetParameter(PARAM_NEED_DC_BASE_QUOTA_ANALYSIS, NO_NEED_DC_BASE_QUOTA_ANALYSIS);
    system::SetParameter(PARAM_NEED_DC_EXTRA_QUOTA_ANALYSIS, NO_NEED_DC_EXTRA_QUOTA_ANALYSIS);
    system::SetParameter(PARAM_NEED_DC_PROACTIVE_ANALYSIS, NEED_DC_PROACTIVE_ANALYSIS);
    
    NotificationUtils::UpdateNotificationProp();
    EXPECT_EQ(NEED_DC_BASE_QUOTA_ANALYSIS, system::GetParameter(
        PARAM_NEED_DC_BASE_QUOTA_ANALYSIS, NO_NEED_DC_BASE_QUOTA_ANALYSIS));
    EXPECT_EQ(NEED_DC_EXTRA_QUOTA_ANALYSIS, system::GetParameter(
        PARAM_NEED_DC_EXTRA_QUOTA_ANALYSIS, NO_NEED_DC_EXTRA_QUOTA_ANALYSIS));
    EXPECT_EQ(NEED_DC_PROACTIVE_ANALYSIS, system::GetParameter(
        PARAM_NEED_DC_PROACTIVE_ANALYSIS, NO_NEED_DC_PROACTIVE_ANALYSIS));
    MEDIA_INFO_LOG("end medialib_notification_utils_test061");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test062, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test062");
    system::SetParameter(PARAM_NEED_DC_BASE_QUOTA_ANALYSIS, NEED_DC_BASE_QUOTA_ANALYSIS);
    system::SetParameter(PARAM_NEED_DC_EXTRA_QUOTA_ANALYSIS, NEED_DC_EXTRA_QUOTA_ANALYSIS);
    system::SetParameter(PARAM_NEED_DC_PROACTIVE_ANALYSIS, NO_NEED_DC_PROACTIVE_ANALYSIS);
    
    NotificationUtils::UpdateNotificationProp();
    EXPECT_EQ(NEED_DC_BASE_QUOTA_ANALYSIS, system::GetParameter(
        PARAM_NEED_DC_BASE_QUOTA_ANALYSIS, NO_NEED_DC_BASE_QUOTA_ANALYSIS));
    EXPECT_EQ(NEED_DC_EXTRA_QUOTA_ANALYSIS, system::GetParameter(
        PARAM_NEED_DC_EXTRA_QUOTA_ANALYSIS, NO_NEED_DC_EXTRA_QUOTA_ANALYSIS));
    EXPECT_EQ(NEED_DC_PROACTIVE_ANALYSIS, system::GetParameter(
        PARAM_NEED_DC_PROACTIVE_ANALYSIS, NO_NEED_DC_PROACTIVE_ANALYSIS));
    MEDIA_INFO_LOG("end medialib_notification_utils_test062");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test063, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test063");
    system::SetParameter(PARAM_NEED_DC_BASE_QUOTA_ANALYSIS, NEED_DC_BASE_QUOTA_ANALYSIS);
    system::SetParameter(PARAM_NEED_DC_EXTRA_QUOTA_ANALYSIS, NO_NEED_DC_EXTRA_QUOTA_ANALYSIS);
    system::SetParameter(PARAM_NEED_DC_PROACTIVE_ANALYSIS, NEED_DC_PROACTIVE_ANALYSIS);
    
    NotificationUtils::UpdateNotificationProp();
    EXPECT_EQ(NEED_DC_BASE_QUOTA_ANALYSIS, system::GetParameter(
        PARAM_NEED_DC_BASE_QUOTA_ANALYSIS, NO_NEED_DC_BASE_QUOTA_ANALYSIS));
    EXPECT_EQ(NEED_DC_EXTRA_QUOTA_ANALYSIS, system::GetParameter(
        PARAM_NEED_DC_EXTRA_QUOTA_ANALYSIS, NO_NEED_DC_EXTRA_QUOTA_ANALYSIS));
    EXPECT_EQ(NEED_DC_PROACTIVE_ANALYSIS, system::GetParameter(
        PARAM_NEED_DC_PROACTIVE_ANALYSIS, NO_NEED_DC_PROACTIVE_ANALYSIS));
    MEDIA_INFO_LOG("end medialib_notification_utils_test063");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test064, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test064");
    system::SetParameter(PARAM_NEED_DC_BASE_QUOTA_ANALYSIS, NO_NEED_DC_BASE_QUOTA_ANALYSIS);
    system::SetParameter(PARAM_NEED_DC_EXTRA_QUOTA_ANALYSIS, NEED_DC_EXTRA_QUOTA_ANALYSIS);
    system::SetParameter(PARAM_NEED_DC_PROACTIVE_ANALYSIS, NEED_DC_PROACTIVE_ANALYSIS);
    
    NotificationUtils::UpdateNotificationProp();
    EXPECT_EQ(NEED_DC_BASE_QUOTA_ANALYSIS, system::GetParameter(
        PARAM_NEED_DC_BASE_QUOTA_ANALYSIS, NO_NEED_DC_BASE_QUOTA_ANALYSIS));
    EXPECT_EQ(NEED_DC_EXTRA_QUOTA_ANALYSIS, system::GetParameter(
        PARAM_NEED_DC_EXTRA_QUOTA_ANALYSIS, NO_NEED_DC_EXTRA_QUOTA_ANALYSIS));
    EXPECT_EQ(NEED_DC_PROACTIVE_ANALYSIS, system::GetParameter(
        PARAM_NEED_DC_PROACTIVE_ANALYSIS, NO_NEED_DC_PROACTIVE_ANALYSIS));
    MEDIA_INFO_LOG("end medialib_notification_utils_test064");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test065, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test065");
    Parcel parcel;
    parcel.WriteUint16(static_cast<uint16_t>(Notification::NotifyUriType::PHOTO_URI));
    parcel.WriteUint16(static_cast<uint16_t>(Notification::DownloadAssetsNotifyType::DOWNLOAD_PROGRESS));
    parcel.WriteInt32(0);
    parcel.WriteInt32(0);
    parcel.WriteInt32(0);
    
    std::shared_ptr<Notification::AssetManagerNotifyInfo> notifyInfo =
        NotificationUtils::UnmarshalAssetManagerNotify(parcel);
    ASSERT_NE(notifyInfo, nullptr);
    EXPECT_EQ(notifyInfo->notifyUri, Notification::NotifyUriType::PHOTO_URI);
    EXPECT_EQ(notifyInfo->downloadAssetNotifyType, Notification::DownloadAssetsNotifyType::DOWNLOAD_PROGRESS);
    EXPECT_EQ(notifyInfo->fileId, 0);
    EXPECT_EQ(notifyInfo->percent, 0);
    EXPECT_EQ(notifyInfo->autoPauseReason, 0);
    MEDIA_INFO_LOG("end medialib_notification_utils_test065");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test066, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test066");
    Parcel parcel;
    parcel.WriteUint16(static_cast<uint16_t>(Notification::NotifyUriType::PHOTO_URI));
    parcel.WriteUint16(static_cast<uint16_t>(Notification::DownloadAssetsNotifyType::DOWNLOAD_PROGRESS));
    parcel.WriteInt32(-1);
    parcel.WriteInt32(-1);
    parcel.WriteInt32(-1);
    
    std::shared_ptr<Notification::AssetManagerNotifyInfo> notifyInfo =
        NotificationUtils::UnmarshalAssetManagerNotify(parcel);
    ASSERT_NE(notifyInfo, nullptr);
    EXPECT_EQ(notifyInfo->notifyUri, Notification::NotifyUriType::PHOTO_URI);
    EXPECT_EQ(notifyInfo->downloadAssetNotifyType, Notification::DownloadAssetsNotifyType::DOWNLOAD_PROGRESS);
    EXPECT_EQ(notifyInfo->fileId, -1);
    EXPECT_EQ(notifyInfo->percent, -1);
    EXPECT_EQ(notifyInfo->autoPauseReason, -1);
    MEDIA_INFO_LOG("end medialib_notification_utils_test066");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test067, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test067");
    Parcel parcel;
    parcel.WriteUint16(static_cast<uint16_t>(Notification::NotifyUriType::PHOTO_URI));
    parcel.WriteUint16(static_cast<uint16_t>(Notification::DownloadAssetsNotifyType::DOWNLOAD_PROGRESS));
    parcel.WriteInt32(INT32_MAX);
    parcel.WriteInt32(INT32_MAX);
    parcel.WriteInt32(INT32_MAX);
    
    std::shared_ptr<Notification::AssetManagerNotifyInfo> notifyInfo =
        NotificationUtils::UnmarshalAssetManagerNotify(parcel);
    ASSERT_NE(notifyInfo, nullptr);
    EXPECT_EQ(notifyInfo->notifyUri, Notification::NotifyUriType::PHOTO_URI);
    EXPECT_EQ(notifyInfo->downloadAssetNotifyType, Notification::DownloadAssetsNotifyType::DOWNLOAD_PROGRESS);
    EXPECT_EQ(notifyInfo->fileId, INT32_MAX);
    EXPECT_EQ(notifyInfo->percent, INT32_MAX);
    EXPECT_EQ(notifyInfo->autoPauseReason, INT32_MAX);
    MEDIA_INFO_LOG("end medialib_notification_utils_test067");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test068, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test068");
    Parcel parcel;
    parcel.WriteUint16(static_cast<uint16_t>(Notification::NotifyUriType::PHOTO_URI));
    parcel.WriteUint16(static_cast<uint16_t>(Notification::DownloadAssetsNotifyType::DOWNLOAD_PROGRESS));
    parcel.WriteInt32(INT32_MIN);
    parcel.WriteInt32(INT32_MIN);
    parcel.WriteInt32(INT32_MIN);
    
    std::shared_ptr<Notification::AssetManagerNotifyInfo> notifyInfo =
        NotificationUtils::UnmarshalAssetManagerNotify(parcel);
    ASSERT_NE(notifyInfo, nullptr);
    EXPECT_EQ(notifyInfo->notifyUri, Notification::NotifyUriType::PHOTO_URI);
    EXPECT_EQ(notifyInfo->downloadAssetNotifyType, Notification::DownloadAssetsNotifyType::DOWNLOAD_PROGRESS);
    EXPECT_EQ(notifyInfo->fileId, INT32_MIN);
    EXPECT_EQ(notifyInfo->percent, INT32_MIN);
    EXPECT_EQ(notifyInfo->autoPauseReason, INT32_MIN);
    MEDIA_INFO_LOG("end medialib_notification_utils_test068");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test069, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test069");
    Parcel parcel;
    parcel.WriteBool(true);
    parcel.WriteUint16(static_cast<uint16_t>(Notification::NotifyUriType::PHOTO_URI));
    parcel.WriteUint16(static_cast<uint16_t>(Notification::AccurateNotifyType::NOTIFY_ASSET_ADD));
    parcel.WriteBool(true);
    parcel.WriteBool(false);
    
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo =
        NotificationUtils::UnmarshalInMultiMode(parcel);
    ASSERT_NE(mediaChangeInfo, nullptr);
    EXPECT_TRUE(mediaChangeInfo->isForRecheck);
    EXPECT_EQ(mediaChangeInfo->notifyUri, Notification::NotifyUriType::PHOTO_URI);
    EXPECT_EQ(mediaChangeInfo->notifyType, Notification::AccurateNotifyType::NOTIFY_ASSET_ADD);
    EXPECT_TRUE(mediaChangeInfo->isSystem);
    EXPECT_TRUE(mediaChangeInfo->changeInfos.empty());
    MEDIA_INFO_LOG("end medialib_notification_utils_test069");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test070, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test070");
    Parcel parcel;
    parcel.WriteBool(false);
    parcel.WriteUint16(static_cast<uint16_t>(Notification::NotifyUriType::PHOTO_URI));
    parcel.WriteUint16(static_cast<uint16_t>(Notification::AccurateNotifyType::NOTIFY_ASSET_ADD));
    parcel.WriteBool(false);
    parcel.WriteBool(false);
    
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo =
        NotificationUtils::UnmarshalInMultiMode(parcel);
    ASSERT_NE(mediaChangeInfo, nullptr);
    EXPECT_FALSE(mediaChangeInfo->isForRecheck);
    EXPECT_EQ(mediaChangeInfo->notifyUri, Notification::NotifyUriType::PHOTO_URI);
    EXPECT_EQ(mediaChangeInfo->notifyType, Notification::AccurateNotifyType::NOTIFY_ASSET_ADD);
    EXPECT_FALSE(mediaChangeInfo->isSystem);
    EXPECT_TRUE(mediaChangeInfo->changeInfos.empty());
    MEDIA_INFO_LOG("end medialib_notification_utils_test070");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test071, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test071");
    Parcel parcel;
    parcel.WriteBool(true);
    parcel.WriteUint16(static_cast<uint16_t>(Notification::NotifyUriType::PHOTO_URI));
    parcel.WriteUint16(static_cast<uint16_t>(Notification::AccurateNotifyType::NOTIFY_ASSET_ADD));
    parcel.WriteBool(false);
    parcel.WriteBool(false);
    
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo =
        NotificationUtils::UnmarshalInMultiMode(parcel);
    ASSERT_NE(mediaChangeInfo, nullptr);
    EXPECT_TRUE(mediaChangeInfo->isForRecheck);
    EXPECT_EQ(mediaChangeInfo->notifyUri, Notification::NotifyUriType::PHOTO_URI);
    EXPECT_EQ(mediaChangeInfo->notifyType, Notification::AccurateNotifyType::NOTIFY_ASSET_ADD);
    EXPECT_FALSE(mediaChangeInfo->isSystem);
    EXPECT_TRUE(mediaChangeInfo->changeInfos.empty());
    MEDIA_INFO_LOG("end medialib_notification_utils_test071");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test072, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test072");
    Parcel parcel;
    parcel.WriteBool(false);
    parcel.WriteUint16(static_cast<uint16_t>(Notification::NotifyUriType::PHOTO_URI));
    parcel.WriteUint16(static_cast<uint16_t>(Notification::AccurateNotifyType::NOTIFY_ASSET_ADD));
    parcel.WriteBool(true);
    parcel.WriteBool(false);
    
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo =
        NotificationUtils::UnmarshalInMultiMode(parcel);
    ASSERT_NE(mediaChangeInfo, nullptr);
    EXPECT_FALSE(mediaChangeInfo->isForRecheck);
    EXPECT_EQ(mediaChangeInfo->notifyUri, Notification::NotifyUriType::PHOTO_URI);
    EXPECT_EQ(mediaChangeInfo->notifyType, Notification::AccurateNotifyType::NOTIFY_ASSET_ADD);
    EXPECT_TRUE(mediaChangeInfo->isSystem);
    EXPECT_TRUE(mediaChangeInfo->changeInfos.empty());
    MEDIA_INFO_LOG("end medialib_notification_utils_test072");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test073, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test073");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo =
        std::make_shared<Notification::MediaChangeInfo>();
    mediaChangeInfo->isForRecheck = false;
    mediaChangeInfo->notifyUri = Notification::NotifyUriType::PHOTO_URI;
    mediaChangeInfo->notifyType = Notification::AccurateNotifyType::NOTIFY_ASSET_ADD;
    mediaChangeInfo->isSystem = false;
    
    int32_t result = NotificationUtils::SendNotification(dataObserver, mediaChangeInfo);
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("end medialib_notification_utils_test073");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test074, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test074");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo =
        std::make_shared<Notification::MediaChangeInfo>();
    mediaChangeInfo->isForRecheck = true;
    mediaChangeInfo->notifyUri = Notification::NotifyUriType::PHOTO_URI;
    mediaChangeInfo->notifyType = Notification::AccurateNotifyType::NOTIFY_ASSET_ADD;
    mediaChangeInfo->isSystem = true;
    
    int32_t result = NotificationUtils::SendNotification(dataObserver, mediaChangeInfo);
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("end medialib_notification_utils_test074");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test075, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test075");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo =
        std::make_shared<Notification::MediaChangeInfo>();
    mediaChangeInfo->isForRecheck = false;
    mediaChangeInfo->notifyUri = Notification::NotifyUriType::PHOTO_URI;
    mediaChangeInfo->notifyType = Notification::AccurateNotifyType::NOTIFY_ASSET_REMOVE;
    mediaChangeInfo->isSystem = true;
    
    int32_t result = NotificationUtils::SendNotification(dataObserver, mediaChangeInfo);
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("end medialib_notification_utils_test075");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test076, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test076");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo =
        std::make_shared<Notification::MediaChangeInfo>();
    mediaChangeInfo->isForRecheck = true;
    mediaChangeInfo->notifyUri = Notification::NotifyUriType::PHOTO_URI;
    mediaChangeInfo->notifyType = Notification::AccurateNotifyType::NOTIFY_ASSET_REMOVE;
    mediaChangeInfo->isSystem = false;
    
    int32_t result = NotificationUtils::SendNotification(dataObserver, mediaChangeInfo);
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("end medialib_notification_utils_test076");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test077, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test077");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo =
        std::make_shared<Notification::MediaChangeInfo>();
    mediaChangeInfo->isForRecheck = false;
    mediaChangeInfo->notifyUri = Notification::NotifyUriType::PHOTO_URI;
    mediaChangeInfo->notifyType = Notification::AccurateNotifyType::NOTIFY_ASSET_UPDATE;
    mediaChangeInfo->isSystem = true;
    
    int32_t result = NotificationUtils::SendNotification(dataObserver, mediaChangeInfo);
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("end medialib_notification_utils_test077");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test078, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test078");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo =
        std::make_shared<Notification::MediaChangeInfo>();
    mediaChangeInfo->isForRecheck = true;
    mediaChangeInfo->notifyUri = Notification::NotifyUriType::PHOTO_URI;
    mediaChangeInfo->notifyType = Notification::AccurateNotifyType::NOTIFY_ASSET_UPDATE;
    mediaChangeInfo->isSystem = false;
    
    int32_t result = NotificationUtils::SendNotification(dataObserver, mediaChangeInfo);
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("end medialib_notification_utils_test078");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test079, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test079");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo =
        std::make_shared<Notification::MediaChangeInfo>();
    mediaChangeInfo->isForRecheck = false;
    mediaChangeInfo->notifyUri = Notification::NotifyUriType::PHOTO_URI;
    mediaChangeInfo->notifyType = Notification::AccurateNotifyType::NOTIFY_ASSET_YUV_READY;
    mediaChangeInfo->isSystem = true;
    
    int32_t result = NotificationUtils::SendNotification(dataObserver, mediaChangeInfo);
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("end medialib_notification_utils_test079");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test080, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test080");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);
    
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo =
        std::make_shared<Notification::MediaChangeInfo>();
    mediaChangeInfo->isForRecheck = true;
    mediaChangeInfo->notifyUri = Notification::NotifyUriType::PHOTO_URI;
    mediaChangeInfo->notifyType = Notification::AccurateNotifyType::NOTIFY_ASSET_YUV_READY;
    mediaChangeInfo->isSystem = false;
    
    int32_t result = NotificationUtils::SendNotification(dataObserver, mediaChangeInfo);
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("end medialib_notification_utils_test080");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test081, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test081");
    Parcel parcel;
    parcel.WriteUint16(static_cast<uint16_t>(Notification::NotifyUriType::AVAILABILITY_URI));
    parcel.WriteString("available");
    parcel.WriteString("");

    std::shared_ptr<Notification::DbAvailabilityData> info = NotificationUtils::UnmarshalDbAvailabilityData(parcel);
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->notifyType, Notification::NotifyUriType::AVAILABILITY_URI);
    EXPECT_EQ(info->status, "available");
    EXPECT_EQ(info->reason, "");
    MEDIA_INFO_LOG("end medialib_notification_utils_test081");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test082, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test082");
    Parcel parcel;
    parcel.WriteUint16(static_cast<uint16_t>(Notification::NotifyUriType::PHOTO_URI));

    std::shared_ptr<Notification::DbAvailabilityData> info = NotificationUtils::UnmarshalDbAvailabilityData(parcel);
    EXPECT_EQ(info, nullptr);
    MEDIA_INFO_LOG("end medialib_notification_utils_test082");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test083, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test083");
    std::shared_ptr<AAFwk::ChangeInfo> changeInfo = std::make_shared<AAFwk::ChangeInfo>();
    int32_t result = NotificationUtils::SendDbAvailabilityNotification(nullptr, changeInfo);
    EXPECT_EQ(result, E_ERR);
    MEDIA_INFO_LOG("end medialib_notification_utils_test083");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test084, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test084");
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow)IDataAbilityObserverTest();
    ASSERT_NE(dataObserver, nullptr);

    int32_t result = NotificationUtils::SendDbAvailabilityNotification(dataObserver, nullptr);
    EXPECT_EQ(result, E_ERR);
    MEDIA_INFO_LOG("end medialib_notification_utils_test084");
}

HWTEST_F(NotificationUtilsTest, medialib_notification_utils_test085, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter medialib_notification_utils_test085");
    Notification::DbAvailabilityData data;
    data.notifyType = Notification::NotifyUriType::AVAILABILITY_URI;
    data.status = DB_STATUS_UNAVAILABLE;
    data.reason = DB_REASON_CORRUPTED;

    Parcel parcel;
    ASSERT_TRUE(data.WriteToParcel(parcel));

    Notification::DbAvailabilityData parsed;
    ASSERT_TRUE(parsed.Unmarshalling(parcel));
    EXPECT_EQ(parsed.notifyType, Notification::NotifyUriType::AVAILABILITY_URI);
    EXPECT_EQ(parsed.status, DB_STATUS_UNAVAILABLE);
    EXPECT_EQ(parsed.reason, DB_REASON_CORRUPTED);
    MEDIA_INFO_LOG("end medialib_notification_utils_test085");
}

} // namespace Media
} // namespace OHOS