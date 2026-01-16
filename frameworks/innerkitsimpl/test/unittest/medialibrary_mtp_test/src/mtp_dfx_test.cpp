/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "mtp_dfx_test.h"

#include <new>
#include <numeric>
#include <unistd.h>

#include "media_log.h"
#include "medialibrary_errno.h"
#include "mock_usbfn_mtp_impl.h"
#include "mtp_dfx_reporter.h"
#include "mtp_manager.h"
#include "mtp_test.h"
#include "mtp_operation.h"
#include "mtp_driver.h"
#include "mtp_packet.h"
#include "payload_data/close_session_data.h"
#include "ptp_media_sync_observer.h"
#include "storage.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
const uint32_t MTP_PHOTO_COUNT = 20;
const int32_t RESPONSE_CODE = -200;
const int32_t E_RESPONSE_CODE = -202;
const int32_t OPERATION_READ_MODE = 1;
const int32_t MTP_MODE = 1;
const int32_t PTP_MODE = 2;
const int32_t NONE_MODE = 0;
const int32_t FILE_COUNT = 60;
void MtpDfxTest::SetUpTestCase(void) {}
void MtpDfxTest::TearDownTestCase(void) {}
void MtpDfxTest::SetUp() {}
void MtpDfxTest::TearDown(void) {}
/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: SendMakeResponsePacket
 */
HWTEST_F(MtpDfxTest, mtp_dfx_test_001, TestSize.Level1)
{
    MtpManager mtpManager;
    MtpOperation mtpOperation;
    mtpOperation.Init();
    mtpOperation.ResetOperation();
    mtpOperation.mtpContextPtr_->operationCode = MTP_OPERATION_GET_OBJECT_HANDLES_CODE;
    int errorCode = 0;
    mtpOperation.ReceiveRequestPacket(errorCode);
    MtpDriver mtpDriver;
    sptr<OHOS::HDI::Usb::Gadget::Mtp::V1_0::IUsbfnMtpInterface> usbfnMtpInterface = new MockUsbfnMtpImpl();
    ASSERT_NE(usbfnMtpInterface, nullptr);
    mtpOperation.mtpDriver_->usbfnMtpInterface = usbfnMtpInterface;
    mtpOperation.SendMakeResponsePacket(errorCode);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: DoSendResponseResultDfxReporter
 */
HWTEST_F(MtpDfxTest, mtp_dfx_test_002, TestSize.Level1)
{
    MtpDfxReporter::GetInstance().Init();
    uint16_t operationCode = MTP_OPERATION_GET_OBJECT_HANDLES_CODE;
    int32_t operationResult = RESPONSE_CODE;
    uint64_t duration = MTP_PHOTO_COUNT;
    int32_t operationMode = OPERATION_READ_MODE;
    MtpDfxReporter::GetInstance().DoSendResponseResultDfxReporter(operationCode, operationResult, duration, operationMode);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: NotifyDoDfXReporter
 */
HWTEST_F(MtpDfxTest, mtp_dfx_test_003, TestSize.Level1)
{
    MtpDfxReporter::GetInstance().Init();
    int32_t mtpMode = PTP_MODE;
    FileCountInfo fileCountInfo;
    fileCountInfo.burstCount = MTP_PHOTO_COUNT;
    fileCountInfo.livePhotoCount = MTP_PHOTO_COUNT;
    fileCountInfo.burstCount = MTP_PHOTO_COUNT;
    fileCountInfo.burstTotalCount = MTP_PHOTO_COUNT;
    fileCountInfo.onlyInCloudPhotoCount = MTP_PHOTO_COUNT;
    fileCountInfo.normalCount = MTP_PHOTO_COUNT;
    fileCountInfo.pictureCount = MTP_PHOTO_COUNT;
    fileCountInfo.videoCount = MTP_PHOTO_COUNT;
    fileCountInfo.albumName = "这是个测试相册";
    MtpDfxReporter::GetInstance().DoFileCountInfoStatistics(fileCountInfo);
    MtpDfxReporter::GetInstance().DoOperationResultStatistics(MTP_OPERATION_GET_OBJECT_HANDLES_CODE, MTP_OK_CODE);
    MtpDfxReporter::GetInstance().DoOperationResultStatistics(MTP_OPERATION_GET_OBJECT_HANDLES_CODE, RESPONSE_CODE);
    MtpDfxReporter::GetInstance().DoOperationResultStatistics(MTP_OPERATION_GET_OBJECT_CODE, RESPONSE_CODE);
    MtpDfxReporter::GetInstance().DoOperationResultStatistics(MTP_OPERATION_GET_PARTIAL_OBJECT_CODE, RESPONSE_CODE);
    MtpDfxReporter::GetInstance().DoOperationResultStatistics(MTP_OPERATION_SEND_OBJECT_CODE, RESPONSE_CODE);
    MtpDfxReporter::GetInstance().DoOperationResultStatistics(MTP_OPERATION_SET_OBJECT_PROP_VALUE_CODE, RESPONSE_CODE);
    MtpDfxReporter::GetInstance().DoOperationResultStatistics(MTP_OPERATION_MOVE_OBJECT_CODE, RESPONSE_CODE);
    MtpDfxReporter::GetInstance().DoOperationResultStatistics(MTP_OPERATION_COPY_OBJECT_CODE, RESPONSE_CODE);
    MtpDfxReporter::GetInstance().DoOperationResultStatistics(MTP_OPERATION_GET_OBJECT_CODE, MTP_OK_CODE);
    MtpDfxReporter::GetInstance().DoOperationResultStatistics(MTP_OPERATION_GET_PARTIAL_OBJECT_CODE, MTP_OK_CODE);
    MtpDfxReporter::GetInstance().NotifyDoDfXReporter(mtpMode);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: SendMakeResponsePacket
 */
HWTEST_F(MtpDfxTest, mtp_dfx_test_004, TestSize.Level1)
{
    MtpManager mtpManager;
    MtpOperation mtpOperation;
    mtpOperation.Init();
    mtpOperation.ResetOperation();
    mtpOperation.mtpContextPtr_->operationCode = MTP_OPERATION_GET_OBJECT_CODE;
    int errorCode = 0;
    mtpOperation.ReceiveRequestPacket(errorCode);
    MtpDriver mtpDriver;
    sptr<OHOS::HDI::Usb::Gadget::Mtp::V1_0::IUsbfnMtpInterface> usbfnMtpInterface = new MockUsbfnMtpImpl();
    ASSERT_NE(usbfnMtpInterface, nullptr);
    mtpOperation.mtpDriver_->usbfnMtpInterface = usbfnMtpInterface;
    errorCode = MTP_ERROR_INVALID_OBJECTHANDLE;
    mtpOperation.SendMakeResponsePacket(errorCode);
    int32_t mtpMode = PTP_MODE;
    MtpDfxReporter::GetInstance().NotifyDoDfXReporter(mtpMode);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: SendEventPackets
 */
HWTEST_F(MtpDfxTest, mtp_dfx_test_005, TestSize.Level1)
{
    MtpOperation mtpOperation;
    mtpOperation.Init();
    mtpOperation.ResetOperation();
    std::shared_ptr<MediaSyncObserver> mediaSyncObserver = std::make_shared<MediaSyncObserver>();
    ASSERT_NE(mediaSyncObserver, nullptr);
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    mediaSyncObserver->context_ = context;
    mediaSyncObserver->context_->mtpDriver = make_shared<MtpDriver>();
    ASSERT_NE(mediaSyncObserver->context_->mtpDriver, nullptr);
    sptr<OHOS::HDI::Usb::Gadget::Mtp::V1_0::IUsbfnMtpInterface> usbfnMtpInterface = new MockUsbfnMtpImpl();
    ASSERT_NE(usbfnMtpInterface, nullptr);
    mediaSyncObserver->context_->mtpDriver->usbfnMtpInterface = usbfnMtpInterface;
    mediaSyncObserver->SendEventPackets(MTP_EVENT_OBJECT_ADDED_CODE, MTP_OPERATION_MOVE_OBJECT_CODE);
    mediaSyncObserver->SendEventPacketAlbum(MTP_EVENT_OBJECT_ADDED_CODE, MTP_OPERATION_MOVE_OBJECT_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: NotifyDoDfXReporter
 */
HWTEST_F(MtpDfxTest, mtp_dfx_test_006, TestSize.Level1)
{
    MtpDfxReporter::GetInstance().Init();
    int32_t mtpMode = MTP_MODE;
    FileCountInfo fileCountInfo;
    fileCountInfo.burstCount = MTP_PHOTO_COUNT;
    fileCountInfo.livePhotoCount = MTP_PHOTO_COUNT;
    fileCountInfo.burstCount = MTP_PHOTO_COUNT;
    fileCountInfo.burstTotalCount = MTP_PHOTO_COUNT;
    fileCountInfo.onlyInCloudPhotoCount = MTP_PHOTO_COUNT;
    fileCountInfo.normalCount = MTP_PHOTO_COUNT;
    fileCountInfo.pictureCount = MTP_PHOTO_COUNT;
    fileCountInfo.videoCount = MTP_PHOTO_COUNT;
    fileCountInfo.albumName = "这是个测试相册";
    MtpDfxReporter::GetInstance().DoFileCountInfoStatistics(fileCountInfo);
    MtpDfxReporter::GetInstance().DoOperationResultStatistics(MTP_OPERATION_GET_OBJECT_HANDLES_CODE, MTP_OK_CODE);
    MtpDfxReporter::GetInstance().DoOperationResultStatistics(MTP_OPERATION_GET_OBJECT_HANDLES_CODE, RESPONSE_CODE);
    MtpDfxReporter::GetInstance().DoOperationResultStatistics(MTP_OPERATION_GET_OBJECT_CODE, RESPONSE_CODE);
    MtpDfxReporter::GetInstance().DoOperationResultStatistics(MTP_OPERATION_GET_PARTIAL_OBJECT_CODE, RESPONSE_CODE);
    MtpDfxReporter::GetInstance().DoOperationResultStatistics(MTP_OPERATION_SEND_OBJECT_CODE, RESPONSE_CODE);
    MtpDfxReporter::GetInstance().DoOperationResultStatistics(MTP_OPERATION_SET_OBJECT_PROP_VALUE_CODE, RESPONSE_CODE);
    MtpDfxReporter::GetInstance().DoOperationResultStatistics(MTP_OPERATION_MOVE_OBJECT_CODE, RESPONSE_CODE);
    MtpDfxReporter::GetInstance().DoOperationResultStatistics(MTP_OPERATION_COPY_OBJECT_CODE, RESPONSE_CODE);
    MtpDfxReporter::GetInstance().DoOperationResultStatistics(MTP_OPERATION_GET_OBJECT_CODE, MTP_OK_CODE);
    MtpDfxReporter::GetInstance().DoOperationResultStatistics(MTP_OPERATION_GET_PARTIAL_OBJECT_CODE, MTP_OK_CODE);
    MtpDfxReporter::GetInstance().NotifyDoDfXReporter(mtpMode);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: DoFileCountInfoStatistics
 */
HWTEST_F(MtpDfxTest, mtp_dfx_test_007, TestSize.Level1)
{
    MtpDfxReporter::GetInstance().Init();
    FileCountInfo fileCountInfo;
    fileCountInfo.burstCount = MTP_PHOTO_COUNT;
    fileCountInfo.livePhotoCount = MTP_PHOTO_COUNT;
    fileCountInfo.burstCount = MTP_PHOTO_COUNT;
    fileCountInfo.burstTotalCount = MTP_PHOTO_COUNT;
    fileCountInfo.onlyInCloudPhotoCount = MTP_PHOTO_COUNT;
    fileCountInfo.normalCount = MTP_PHOTO_COUNT;
    fileCountInfo.pictureCount = MTP_PHOTO_COUNT;
    fileCountInfo.videoCount = MTP_PHOTO_COUNT;
    fileCountInfo.albumName = "这是个测试相册";
    MtpDfxReporter::GetInstance().DoFileCountInfoStatistics(fileCountInfo);
    MtpDfxReporter::GetInstance().DoFileCountInfoStatistics(fileCountInfo);

    std::string shartalbumName = "a";
    std::string albumNameShart = MtpDfxReporter::GetInstance().GetSafeAlbumNameWhenChinese(shartalbumName);
    ASSERT_EQ(albumNameShart, "*a");
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: DoBatchFileCountInfoDfxReporte
 * CaseDescription: DoSendResponseResultDfxReporter
 */
HWTEST_F(MtpDfxTest, mtp_dfx_test_008, TestSize.Level1)
{
    MtpDfxReporter::GetInstance().Init();
    FileCountInfo fileCountInfo;
    std::string testAlbumName;
    for (int i = 1; i <= FILE_COUNT; i++)
    {
        testAlbumName = "测试" + std::to_string(i);
        fileCountInfo.burstCount = MTP_PHOTO_COUNT;
        fileCountInfo.livePhotoCount = MTP_PHOTO_COUNT;
        fileCountInfo.burstCount = MTP_PHOTO_COUNT;
        fileCountInfo.burstTotalCount = MTP_PHOTO_COUNT;
        fileCountInfo.onlyInCloudPhotoCount = MTP_PHOTO_COUNT;
        fileCountInfo.normalCount = MTP_PHOTO_COUNT;
        fileCountInfo.pictureCount = MTP_PHOTO_COUNT;
        fileCountInfo.videoCount = MTP_PHOTO_COUNT;
        fileCountInfo.albumName = testAlbumName;
        MtpDfxReporter::GetInstance().DoFileCountInfoStatistics(fileCountInfo);
    }
    int32_t mtpMode = NONE_MODE;
    MtpDfxReporter::GetInstance().DoBatchFileCountInfoDfxReporter(mtpMode);

    std::string longalbumName = "longlonglong";
    std::string albumNameLong = MtpDfxReporter::GetInstance().GetSafeAlbumNameWhenChinese(longalbumName);
    ASSERT_EQ(albumNameLong, "*long");

    uint16_t operationCode = MTP_OPERATION_GET_OBJECT_HANDLES_CODE;
    int32_t operationResult = E_RESPONSE_CODE;
    uint64_t duration = MTP_PHOTO_COUNT;
    int32_t operationMode = OPERATION_READ_MODE;
    MtpDfxReporter::GetInstance().DoSendResponseResultDfxReporter(operationCode,
        operationResult, duration, operationMode);
}
} // namespace Media
} // namespace OHOS