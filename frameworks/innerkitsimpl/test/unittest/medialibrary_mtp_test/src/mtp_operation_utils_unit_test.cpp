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

#include <thread>
#include "mtp_operation_utils_unit_test.h"
#include "mtp_operation_utils.h"
#include "medialibrary_errno.h"
#include "payload_data.h"
#include "mtp_constants.h"
#include "media_mtp_utils.h"
#include "mtp_manager.h"
#include "mtp_driver.h"
#include "parameters.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {

void MtpOperationUtilsUnitTest::SetUpTestCase(void) {}
void MtpOperationUtilsUnitTest::TearDownTestCase(void) {}
void MtpOperationUtilsUnitTest::SetUp() {}
void MtpOperationUtilsUnitTest::TearDown(void) {}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetNumObjects
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_001, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = nullptr;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    uint16_t res = mtpOperationUtils->GetNumObjects(data);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetNumObjects
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_002, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->storageID = MTP_STORAGE_ID_ALL;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    uint16_t res = mtpOperationUtils->GetNumObjects(data);
    EXPECT_EQ(res, MTP_INVALID_STORAGEID_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: HasStorage
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_003, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = nullptr;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    int errcode = 0;
    uint16_t res = mtpOperationUtils->HasStorage(errcode);
    EXPECT_EQ(res, static_cast<uint16_t>(MTP_ERROR_CONTEXT_IS_NULL));
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: HasStorage
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_004, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->sessionOpen = false;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    int errcode = 0;
    uint16_t res = mtpOperationUtils->HasStorage(errcode);
    EXPECT_EQ(res, static_cast<uint16_t>(MTP_ERROR_SESSION_NOT_OPEN));
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: HasStorage
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_005, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->sessionOpen = true;
    context->storageID = MTP_STORAGE_ID_ALL;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    int errcode = 0;
    uint16_t res = mtpOperationUtils->HasStorage(errcode);
    EXPECT_EQ(res, static_cast<uint16_t>(MTP_ERROR_INVALID_STORAGE_ID));
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetObjectPropDesc
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_006, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = nullptr;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    int errcode = 0;
    uint16_t containerType = DATA_CONTAINER_TYPE;
    uint16_t res = mtpOperationUtils->GetObjectPropDesc(data, containerType, errcode);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetObjectPropValue
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_007, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = nullptr;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    int errcode = 0;
    uint16_t containerType = DATA_CONTAINER_TYPE;
    uint16_t res = mtpOperationUtils->GetObjectPropValue(data, containerType, errcode);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: DoSetObjectPropValue
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_008, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = nullptr;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    int errcode = 0;
    mtpOperationUtils->DoSetObjectPropValue(errcode);
    EXPECT_EQ(errcode, MTP_ERROR_CONTEXT_IS_NULL);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: DoSetObjectPropValue
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_009, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    int errcode = 0;
    mtpOperationUtils->DoSetObjectPropValue(errcode);
    EXPECT_EQ(errcode, MTP_ERROR_ACCESS_DENIED);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetObjectPropList
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_010, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = nullptr;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    int errcode = 0;
    uint16_t containerType = DATA_CONTAINER_TYPE;
    uint16_t res = mtpOperationUtils->GetObjectPropList(data, containerType, errcode);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: SetObjectReferences
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_011, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = nullptr;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    uint16_t res = mtpOperationUtils->SetObjectReferences(data);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: RecevieSendObject
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_012, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = nullptr;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    MtpFileRange object;
    int fd = 1;
    int32_t errcode = mtpOperationUtils->RecevieSendObject(object, fd);
    EXPECT_EQ(errcode, MTP_INVALID_PARAMETER_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: RecevieSendObject
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_013, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->mtpDriver = nullptr;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    MtpFileRange object;
    int fd = 1;
    int32_t errcode = mtpOperationUtils->RecevieSendObject(object, fd);
    EXPECT_EQ(errcode, MTP_INVALID_PARAMETER_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: RecevieSendObject PreDealFd
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_014, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->mtpDriver = std::make_shared<MtpDriver>();
    ASSERT_NE(context->mtpDriver, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    MtpFileRange object;
    int fd = 1;
    int32_t errcode = mtpOperationUtils->RecevieSendObject(object, fd);
    bool deal = false;
    mtpOperationUtils->PreDealFd(deal, fd);
    EXPECT_EQ(errcode, 0);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: PreDealFd
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_015, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->mtpDriver = std::make_shared<MtpDriver>();
    ASSERT_NE(context->mtpDriver, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    MtpFileRange object;
    int fd = 0;
    int32_t errcode = mtpOperationUtils->RecevieSendObject(object, fd);
    bool deal = true;
    mtpOperationUtils->PreDealFd(deal, fd);
    EXPECT_EQ(errcode, 0);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: SendObjectInfo
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_017, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = nullptr;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    int32_t errcode = 0;
    uint16_t res = mtpOperationUtils->SendObjectInfo(data, errcode);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetPartialObject
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_018, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = nullptr;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    uint16_t res = mtpOperationUtils->GetPartialObject(data);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetObjectPropsSupported
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_019, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = nullptr;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    uint16_t res = mtpOperationUtils->GetObjectPropsSupported(data);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: DeleteObject
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_020, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = nullptr;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    int errcode = 0;
    uint16_t res = mtpOperationUtils->DeleteObject(data, errcode);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: MoveObject
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_021, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = nullptr;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    int errcode = 0;
    uint16_t res = mtpOperationUtils->MoveObject(data, errcode);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: CopyObject
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_022, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = nullptr;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    int errcode = 0;
    uint16_t res = mtpOperationUtils->CopyObject(data, errcode);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetStorageIDs
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_023, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = nullptr;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    uint16_t containerType = DATA_CONTAINER_TYPE;
    int errcode = 0;
    uint16_t res = mtpOperationUtils->GetStorageIDs(data, containerType, errcode);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetStorageIDs
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_024, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->sessionOpen = false;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    uint16_t containerType = DATA_CONTAINER_TYPE;
    int errcode = 0;
    uint16_t res = mtpOperationUtils->GetStorageIDs(data, containerType, errcode);
    EXPECT_EQ(res, MTP_SESSION_NOT_OPEN_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetStorageIDs
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_025, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->sessionOpen = true;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    uint16_t containerType = DATA_CONTAINER_TYPE;
    int errcode = 0;
    uint16_t res = mtpOperationUtils->GetStorageIDs(data, containerType, errcode);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetStorageInfo
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_026, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->sessionOpen = false;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    uint16_t containerType = DATA_CONTAINER_TYPE;
    int errcode = 0;
    uint16_t res = mtpOperationUtils->GetStorageInfo(data, containerType, errcode);
    EXPECT_EQ(res, MTP_SESSION_NOT_OPEN_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetStorageInfo
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_027, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->sessionOpen = true;
    context->storageInfoID = MTP_STORAGE_ID_ALL;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    uint16_t containerType = DATA_CONTAINER_TYPE;
    int errcode = 0;
    uint16_t res = mtpOperationUtils->GetStorageInfo(data, containerType, errcode);
    EXPECT_EQ(res, MTP_INVALID_STORAGEID_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: CheckErrorCode
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_028, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    int errcode = MTP_ERROR_PACKET_INCORRECT;
    uint16_t res = mtpOperationUtils->CheckErrorCode(errcode);
    EXPECT_EQ(res, MTP_INVALID_PARAMETER_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: CheckErrorCode
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_029, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    int errcode = MTP_ERROR_SESSION_ALREADY_OPEN;
    uint16_t res = mtpOperationUtils->CheckErrorCode(errcode);
    EXPECT_EQ(res, MTP_SESSION_ALREADY_OPEN_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: CheckErrorCode
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_030, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    int errcode = MTP_ERROR_NO_THIS_FILE;
    uint16_t res = mtpOperationUtils->CheckErrorCode(errcode);
    EXPECT_EQ(res, MTP_INVALID_OBJECTHANDLE_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: CheckErrorCode
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_031, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    int errcode = MTP_ERROR_INCOMPLETE_TRANSFER;
    uint16_t res = mtpOperationUtils->CheckErrorCode(errcode);
    EXPECT_EQ(res, MTP_INCOMPLETE_TRANSFER_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: CheckErrorCode
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_032, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    int errcode = MTP_ERROR_SESSION_NOT_OPEN;
    uint16_t res = mtpOperationUtils->CheckErrorCode(errcode);
    EXPECT_EQ(res, MTP_SESSION_NOT_OPEN_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: CheckErrorCode
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_033, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    int errcode = MTP_ERROR_INVALID_STORAGE_ID;
    uint16_t res = mtpOperationUtils->CheckErrorCode(errcode);
    EXPECT_EQ(res, MTP_INVALID_STORAGEID_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: CheckErrorCode
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_034, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    int errcode = MTP_ERROR_INVALID_OBJECTHANDLE;
    uint16_t res = mtpOperationUtils->CheckErrorCode(errcode);
    EXPECT_EQ(res, MTP_INVALID_OBJECTHANDLE_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: CheckErrorCode
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_035, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    int errcode = MTP_ERROR_DEVICEPROP_NOT_SUPPORTED;
    uint16_t res = mtpOperationUtils->CheckErrorCode(errcode);
    EXPECT_EQ(res, MTP_DEVICEPROP_NOT_SUPPORTED_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: CheckErrorCode
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_036, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    int errcode = MTP_ERROR_STORE_NOT_AVAILABLE;
    uint16_t res = mtpOperationUtils->CheckErrorCode(errcode);
    EXPECT_EQ(res, MTP_STORE_NOT_AVAILABLE_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: CheckErrorCode
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_037, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    int errcode = MTP_ERROR_INVALID_PARENTOBJECT;
    uint16_t res = mtpOperationUtils->CheckErrorCode(errcode);
    EXPECT_EQ(res, MTP_INVALID_PARENTOBJECT_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: CheckErrorCode
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_038, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    int errcode = MTP_ERROR_PARAMETER_NOT_SUPPORTED;
    uint16_t res = mtpOperationUtils->CheckErrorCode(errcode);
    EXPECT_EQ(res, MTP_PARAMETER_NOT_SUPPORTED_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: CheckErrorCode
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_039, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    int errcode = MTP_ERROR_INVALID_OBJECTPROP_VALUE;
    uint16_t res = mtpOperationUtils->CheckErrorCode(errcode);
    EXPECT_EQ(res, MTP_INVALID_OBJECTPROP_VALUE_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: CheckErrorCode
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_040, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    int errcode = MTP_ERROR_INVALID_OBJECTPROP_FORMAT;
    uint16_t res = mtpOperationUtils->CheckErrorCode(errcode);
    EXPECT_EQ(res, MTP_INVALID_OBJECTPROP_FORMAT_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: CheckErrorCode
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_041, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    int errcode = MTP_ERROR_INVALID_OBJECTPROPCODE;
    uint16_t res = mtpOperationUtils->CheckErrorCode(errcode);
    EXPECT_EQ(res, MTP_INVALID_OBJECTPROPCODE_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: CheckErrorCode
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_042, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    int errcode = MTP_ERROR_ACCESS_DENIED;
    uint16_t res = mtpOperationUtils->CheckErrorCode(errcode);
    EXPECT_EQ(res, MTP_ACCESS_DENIED_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: CheckErrorCode
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_043, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    int errcode = MTP_ERROR_SPECIFICATION_BY_GROUP_UNSUPPORTED;
    uint16_t res = mtpOperationUtils->CheckErrorCode(errcode);
    EXPECT_EQ(res, MTP_SPECIFICATION_BY_GROUP_UNSUPPORTED_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: CheckErrorCode
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_044, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    int errcode = MTP_ERROR_SPECIFICATION_BY_DEPTH_UNSUPPORTED;
    uint16_t res = mtpOperationUtils->CheckErrorCode(errcode);
    EXPECT_EQ(res, MTP_SPECIFICATION_BY_DEPTH_UNSUPPORTED_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: CheckErrorCode
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_045, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    int errcode = MTP_SUCCESS;
    uint16_t res = mtpOperationUtils->CheckErrorCode(errcode);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetPropDesc
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_046, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->property = MTP_DEVICE_PROPERTY_SYNCHRONIZATION_PARTNER_CODE;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    uint16_t containerType = DATA_CONTAINER_TYPE;
    int errcode = -1;
    uint16_t res = mtpOperationUtils->GetPropDesc(data, containerType, errcode);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetPropDesc
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_047, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->property = MTP_DEVICE_PROPERTY_DEVICE_FRIENDLY_NAME_CODE;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    uint16_t containerType = DATA_CONTAINER_TYPE;
    int errcode = -1;
    uint16_t res = mtpOperationUtils->GetPropDesc(data, containerType, errcode);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetPropDesc
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_048, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->property = MTP_DEVICE_PROPERTY_SESSION_INITIATOR_VERSION_INFO_CODE;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    uint16_t containerType = DATA_CONTAINER_TYPE;
    int errcode = -1;
    uint16_t res = mtpOperationUtils->GetPropDesc(data, containerType, errcode);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetPropDesc
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_049, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->property = MTP_DEVICE_PROPERTY_IMAGE_SIZE_CODE;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    uint16_t containerType = DATA_CONTAINER_TYPE;
    int errcode = -1;
    uint16_t res = mtpOperationUtils->GetPropDesc(data, containerType, errcode);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetPropDesc
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_050, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->property = MTP_DEVICE_PROPERTY_BATTERY_LEVEL_CODE;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    uint16_t containerType = DATA_CONTAINER_TYPE;
    int errcode = -1;
    uint16_t res = mtpOperationUtils->GetPropDesc(data, containerType, errcode);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetPropDesc
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_051, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->property = MTP_DEVICE_PROPERTY_PERCEIVED_DEVICE_TYPE_CODE;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    uint16_t containerType = DATA_CONTAINER_TYPE;
    int errcode = -1;
    uint16_t res = mtpOperationUtils->GetPropDesc(data, containerType, errcode);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetPropDesc
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_052, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->property = MTP_DEVICE_PROPERTY_UNDEFINED_CODE;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    uint16_t containerType = DATA_CONTAINER_TYPE;
    int errcode = -1;
    uint16_t res = mtpOperationUtils->GetPropDesc(data, containerType, errcode);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetPropValue
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_053, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->property = MTP_DEVICE_PROPERTY_SYNCHRONIZATION_PARTNER_CODE;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    uint16_t containerType = DATA_CONTAINER_TYPE;
    int errcode = -1;
    uint16_t res = mtpOperationUtils->GetPropValue(data, containerType, errcode);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetPropValue
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_054, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->property = MTP_DEVICE_PROPERTY_DEVICE_FRIENDLY_NAME_CODE;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    uint16_t containerType = DATA_CONTAINER_TYPE;
    int errcode = -1;
    uint16_t res = mtpOperationUtils->GetPropValue(data, containerType, errcode);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetPropValue
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_055, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->property = MTP_DEVICE_PROPERTY_SESSION_INITIATOR_VERSION_INFO_CODE;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    uint16_t containerType = DATA_CONTAINER_TYPE;
    int errcode = -1;
    uint16_t res = mtpOperationUtils->GetPropValue(data, containerType, errcode);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetPropValue
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_056, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->property = MTP_DEVICE_PROPERTY_IMAGE_SIZE_CODE;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    uint16_t containerType = DATA_CONTAINER_TYPE;
    int errcode = -1;
    uint16_t res = mtpOperationUtils->GetPropValue(data, containerType, errcode);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetPropValue
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_057, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->property = MTP_DEVICE_PROPERTY_BATTERY_LEVEL_CODE;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    uint16_t containerType = DATA_CONTAINER_TYPE;
    int errcode = -1;
    uint16_t res = mtpOperationUtils->GetPropValue(data, containerType, errcode);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetPropValue
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_058, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->property = MTP_DEVICE_PROPERTY_PERCEIVED_DEVICE_TYPE_CODE;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    uint16_t containerType = DATA_CONTAINER_TYPE;
    int errcode = -1;
    uint16_t res = mtpOperationUtils->GetPropValue(data, containerType, errcode);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetPropValue
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_059, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->property = MTP_DEVICE_PROPERTY_UNDEFINED_CODE;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    uint16_t containerType = DATA_CONTAINER_TYPE;
    int errcode = -1;
    uint16_t res = mtpOperationUtils->GetPropValue(data, containerType, errcode);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: ObjectEvent
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_060, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    int32_t payload = 1;
    uint16_t res = mtpOperationUtils->ObjectEvent(data, payload);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetPathByHandle
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_061, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    uint32_t handle = 0;
    string path = "";
    string realPath = "";
    uint16_t res = mtpOperationUtils->GetPathByHandle(handle, path, realPath);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetPathByHandle
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_062, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    uint32_t handle = 1;
    string path = "";
    string realPath = "";
    uint16_t res = mtpOperationUtils->GetPathByHandle(handle, path, realPath);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetPathByHandle
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_063, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    uint32_t handle = 1;
    string path = "";
    string realPath = "";
    uint16_t res = mtpOperationUtils->GetPathByHandle(handle, path, realPath);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetHandleByPaths
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_064, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    uint32_t handle = 1;
    string path = "";
    int32_t res = mtpOperationUtils->GetHandleByPaths(path, handle);
    EXPECT_EQ(res, MTP_UNDEFINED_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetHandleByPaths
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_065, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    uint32_t handle = 1;
    string path = "/storage/media/local/files/Docs/Desktop";
    int32_t res = mtpOperationUtils->GetHandleByPaths(path, handle);
    EXPECT_EQ(res, E_NO_SUCH_FILE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetHandleByPaths
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_066, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    uint32_t handle = 1;
    string path = "/storage/media/local/files/Docs/Desktop/";
    int32_t res = mtpOperationUtils->GetHandleByPaths(path, handle);
    EXPECT_EQ(res, E_NO_SUCH_FILE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetBatteryLevel
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_067, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    int32_t capacity = mtpOperationUtils->GetBatteryLevel();
    EXPECT_GE(capacity, 0);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetPropertyInner
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_068, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::string property = "const.product.name";
    std::string defValue = "";
    std::string res = mtpOperationUtils->GetPropertyInner(property, defValue);
    std::string propertyInner = OHOS::system::GetParameter(property, defValue);
    EXPECT_EQ(res, propertyInner);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: SetPropertyInner
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_069, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::string property = "persist.device.name";
    std::string defValue = "";
    bool res = mtpOperationUtils->SetPropertyInner(property, defValue);
    EXPECT_TRUE(res);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: CheckErrorCode
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_070, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    int errcode = MTP_ERROR_TRANSFER_FAILED;
    uint16_t res = mtpOperationUtils->CheckErrorCode(errcode);
    EXPECT_EQ(res, MTP_STORE_FULL_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetPathByHandle
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_071, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::PTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    uint32_t handle = 1;
    string path = "";
    string realPath = "";
    uint16_t res = mtpOperationUtils->GetPathByHandle(handle, path, realPath);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetHandleByPaths
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_072, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::PTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    uint32_t handle = 1;
    string path = "/storage/media/local/files/Docs/Desktop/";
    int32_t res = mtpOperationUtils->GetHandleByPaths(path, handle);
    EXPECT_EQ(res, MTP_ERROR_STORE_NOT_AVAILABLE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: TryAddExternalStorage
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_073, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    uint32_t storageId = 1;
    string fsUuid = "";
    bool res = mtpOperationUtils->TryAddExternalStorage(fsUuid, storageId);
    EXPECT_EQ(res, false);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: TryAddExternalStorage
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_074, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::PTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    uint32_t storageId = 1;
    string fsUuid = "";
    bool res = mtpOperationUtils->TryAddExternalStorage(fsUuid, storageId);
    EXPECT_EQ(res, false);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: TryRemoveExternalStorage
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_075, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    uint32_t storageId = 1;
    string fsUuid = "";
    bool res = mtpOperationUtils->TryRemoveExternalStorage(fsUuid, storageId);
    EXPECT_EQ(res, false);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: TryRemoveExternalStorage
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_076, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::PTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    uint32_t storageId = 1;
    string fsUuid = "";
    bool res = mtpOperationUtils->TryRemoveExternalStorage(fsUuid, storageId);
    EXPECT_EQ(res, false);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetStorageIDs
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_077, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::PTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->sessionOpen = true;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    uint16_t containerType = DATA_CONTAINER_TYPE;
    int errcode = 0;
    uint16_t res = mtpOperationUtils->GetStorageIDs(data, containerType, errcode);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: CopyObject
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_078, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::PTP_MODE;
    std::shared_ptr<MtpOperationContext> context = nullptr;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    int errcode = 0;
    uint16_t res = mtpOperationUtils->CopyObject(data, errcode);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: MoveObject
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_079, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::PTP_MODE;
    std::shared_ptr<MtpOperationContext> context = nullptr;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    int errcode = 0;
    uint16_t res = mtpOperationUtils->MoveObject(data, errcode);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: DeleteObject
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_080, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::PTP_MODE;
    std::shared_ptr<MtpOperationContext> context = nullptr;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    int errcode = 0;
    uint16_t res = mtpOperationUtils->DeleteObject(data, errcode);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetThumb
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_081, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::PTP_MODE;
    std::shared_ptr<MtpOperationContext> context = nullptr;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    int errcode = 0;
    uint16_t containerType = DATA_CONTAINER_TYPE;
    mtpOperationUtils->GetThumb(data, containerType, errcode);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: PreDealFd
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_082, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->mtpDriver = std::make_shared<MtpDriver>();
    ASSERT_NE(context->mtpDriver, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    int fd = 1;
    bool deal = false;
    mtpOperationUtils->PreDealFd(deal, fd);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: PreDealFd
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_083, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::PTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->mtpDriver = std::make_shared<MtpDriver>();
    ASSERT_NE(context->mtpDriver, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    int fd = 1;
    bool deal = false;
    mtpOperationUtils->PreDealFd(deal, fd);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: RecevieSendObject
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_084, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    MtpFileRange object;
    int fd = 1;
    int32_t errcode = mtpOperationUtils->RecevieSendObject(object, fd);
    EXPECT_EQ(errcode, MTP_INVALID_PARAMETER_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: RecevieSendObject
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_085, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::PTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    MtpFileRange object;
    int fd = 1;
    int32_t errcode = mtpOperationUtils->RecevieSendObject(object, fd);
    EXPECT_EQ(errcode, MTP_INVALID_PARAMETER_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetObjectPropList
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_086, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::PTP_MODE;
    std::shared_ptr<MtpOperationContext> context = nullptr;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    int errcode = 0;
    uint16_t containerType = DATA_CONTAINER_TYPE;
    uint16_t res = mtpOperationUtils->GetObjectPropList(data, containerType, errcode);
    EXPECT_EQ(res, MTP_OK_CODE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: DoSetObjectPropValue
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_087, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::PTP_MODE;
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    int errcode = 0;
    mtpOperationUtils->DoSetObjectPropValue(errcode);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetObjectPropValue
 */
HWTEST_F(MtpOperationUtilsUnitTest, medialibrary_MTP_message_testlevel_0_088, TestSize.Level1)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::PTP_MODE;
    std::shared_ptr<MtpOperationContext> context = nullptr;
    std::shared_ptr<MtpOperationUtils> mtpOperationUtils = std::make_shared<MtpOperationUtils>(context);
    ASSERT_NE(mtpOperationUtils, nullptr);
    std::shared_ptr<PayloadData> data = nullptr;
    int errcode = 0;
    uint16_t containerType = DATA_CONTAINER_TYPE;
    uint16_t res = mtpOperationUtils->GetObjectPropValue(data, containerType, errcode);
    EXPECT_EQ(res, MTP_OK_CODE);
}
} // namespace Media
} // namespace OHOS