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

#include "mtp_ptp_proxy_test.h"
#include "mtp_ptp_proxy.h"
#include "medialibrary_errno.h"
#include "iservice_registry.h"
#include "mtp_manager.h"
#include "mtp_ptp_const.h"
#include "mtp_media_library.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {

static constexpr int STORAGE_MANAGER_UID_TEST = 5003;

void MtpPtpProxyTest::SetUpTestCase(void)
{
    std::shared_ptr<MtpMediaLibrary> g_mtpMediaLibrary = MtpMediaLibrary::GetInstance();
    g_mtpMediaLibrary->Clear();
}

void MtpPtpProxyTest::TearDownTestCase(void)
{
    std::shared_ptr<MtpMediaLibrary> g_mtpMediaLibrary = MtpMediaLibrary::GetInstance();
    g_mtpMediaLibrary->Clear();
}

void MtpPtpProxyTest::SetUp() {}
void MtpPtpProxyTest::TearDown(void) {}

/*
 * Feature: MTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetHandles
 */
HWTEST_F(MtpPtpProxyTest, MtpPtpProxy_Test_001, TestSize.Level1)
{
    MtpPtpProxy mtpPtpProxy = OHOS::Media::MtpPtpProxy::GetInstance();

    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(saManager, nullptr);
    auto token = saManager->GetSystemAbility(STORAGE_MANAGER_UID_TEST);

    Context context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    mtpPtpProxy.Init(token, context);

    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;

    std::shared_ptr<UInt32List> outHandles = std::make_shared<UInt32List>();
    ASSERT_NE(outHandles, nullptr);
    bool isMac = false;

    context->parent = DEFAULT_PARENT_ROOT;
    int32_t res = mtpPtpProxy.GetHandles(context, outHandles, isMac);
    EXPECT_EQ(res, MTP_ERROR_STORE_NOT_AVAILABLE);

    context->parent = 1;
    res = mtpPtpProxy.GetHandles(context, outHandles, isMac);
    EXPECT_EQ(res, MTP_SUCCESS);

    context->parent = 2 * PTP_IN_MTP_ID;
    res = mtpPtpProxy.GetHandles(context, outHandles, isMac);
    EXPECT_EQ(res, MTP_ERROR_STORE_NOT_AVAILABLE);
}

/*
 * Feature: MTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetObjectInfo
 */
HWTEST_F(MtpPtpProxyTest, MtpPtpProxy_Test_002, TestSize.Level1)
{
    MtpPtpProxy mtpPtpProxy = OHOS::Media::MtpPtpProxy::GetInstance();

    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(saManager, nullptr);
    auto token = saManager->GetSystemAbility(STORAGE_MANAGER_UID_TEST);

    Context context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    mtpPtpProxy.Init(token, context);

    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;

    std::shared_ptr<ObjectInfo> objectInfo = std::make_shared<ObjectInfo>(context->handle);
    ASSERT_NE(objectInfo, nullptr);

    context->handle = 0;
    int32_t res = mtpPtpProxy.GetObjectInfo(context, objectInfo);
    EXPECT_EQ(res, MTP_ERROR_INVALID_OBJECTHANDLE);

    context->handle = PTP_IN_MTP_ID;
    res = mtpPtpProxy.GetObjectInfo(context, objectInfo);
    EXPECT_EQ(res, MTP_SUCCESS);

    context->handle = 2 * PTP_IN_MTP_ID;
    res = mtpPtpProxy.GetObjectInfo(context, objectInfo);
    EXPECT_EQ(res, MTP_ERROR_STORE_NOT_AVAILABLE);
}

/*
 * Feature: MTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetObjectPropValue
 */
HWTEST_F(MtpPtpProxyTest, MtpPtpProxy_Test_003, TestSize.Level1)
{
    MtpPtpProxy mtpPtpProxy = OHOS::Media::MtpPtpProxy::GetInstance();

    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(saManager, nullptr);
    auto token = saManager->GetSystemAbility(STORAGE_MANAGER_UID_TEST);

    Context context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    mtpPtpProxy.Init(token, context);

    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;

    uint64_t intVal = 0;
    uint128_t longVal = {0};
    std::string strVal = "";

    context->handle = 0;
    int32_t res = mtpPtpProxy.GetObjectPropValue(context, intVal, longVal, strVal);
    EXPECT_EQ(res, MTP_ERROR_INVALID_OBJECTHANDLE);

    context->handle = PTP_IN_MTP_ID;
    res = mtpPtpProxy.GetObjectPropValue(context, intVal, longVal, strVal);
    EXPECT_EQ(res, MTP_SUCCESS);

    context->handle = 2 * PTP_IN_MTP_ID;
    res = mtpPtpProxy.GetObjectPropValue(context, intVal, longVal, strVal);
    EXPECT_EQ(res, MTP_ERROR_INVALID_OBJECTHANDLE);
}

/*
 * Feature: MTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: SetObjectPropValue
 */
HWTEST_F(MtpPtpProxyTest, MtpPtpProxy_Test_004, TestSize.Level1)
{
    MtpPtpProxy mtpPtpProxy = OHOS::Media::MtpPtpProxy::GetInstance();

    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(saManager, nullptr);
    auto token = saManager->GetSystemAbility(STORAGE_MANAGER_UID_TEST);

    Context context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    mtpPtpProxy.Init(token, context);

    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;

    context->handle = 0;
    int32_t res = mtpPtpProxy.SetObjectPropValue(context);
    EXPECT_EQ(res, MTP_ERROR_ACCESS_DENIED);

    context->handle = PTP_IN_MTP_ID;
    res = mtpPtpProxy.SetObjectPropValue(context);
    EXPECT_EQ(res, MTP_ERROR_ACCESS_DENIED);

    context->handle = 2 * PTP_IN_MTP_ID;
    res = mtpPtpProxy.SetObjectPropValue(context);
    EXPECT_EQ(res, MTP_ERROR_INVALID_OBJECTPROP_VALUE);
}

/*
 * Feature: MTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetObjectPropList
 */
HWTEST_F(MtpPtpProxyTest, MtpPtpProxy_Test_005, TestSize.Level1)
{
    MtpPtpProxy mtpPtpProxy = OHOS::Media::MtpPtpProxy::GetInstance();

    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(saManager, nullptr);
    auto token = saManager->GetSystemAbility(STORAGE_MANAGER_UID_TEST);

    Context context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    mtpPtpProxy.Init(token, context);

    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;

    std::shared_ptr<std::vector<Property>> outProps = std::make_shared<std::vector<Property>>();
    ASSERT_NE(outProps, nullptr);

    context->handle = 0;
    int32_t res = mtpPtpProxy.GetObjectPropList(context, outProps);
    EXPECT_EQ(res, MTP_ERROR_PARAMETER_NOT_SUPPORTED);

    context->handle = PTP_IN_MTP_ID;
    res = mtpPtpProxy.GetObjectPropList(context, outProps);
    EXPECT_EQ(res, MTP_SUCCESS);

    context->handle = 2 * PTP_IN_MTP_ID;
    res = mtpPtpProxy.GetObjectPropList(context, outProps);
    EXPECT_EQ(res, MTP_ERROR_PARAMETER_NOT_SUPPORTED);
}

/*
 * Feature: MTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: IsMtpExistObject
 */
HWTEST_F(MtpPtpProxyTest, MtpPtpProxy_Test_006, TestSize.Level1)
{
    MtpPtpProxy mtpPtpProxy = OHOS::Media::MtpPtpProxy::GetInstance();

    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(saManager, nullptr);
    auto token = saManager->GetSystemAbility(STORAGE_MANAGER_UID_TEST);

    Context context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    mtpPtpProxy.Init(token, context);

    context->handle = 0;
    bool res = mtpPtpProxy.IsMtpExistObject(context);
    EXPECT_TRUE(res);

    context->handle = 2 * PTP_IN_MTP_ID;
    res = mtpPtpProxy.IsMtpExistObject(context);
    EXPECT_FALSE(res);
}

/*
 * Feature: MTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetReadFd
 */
HWTEST_F(MtpPtpProxyTest, MtpPtpProxy_Test_007, TestSize.Level1)
{
    MtpPtpProxy mtpPtpProxy = OHOS::Media::MtpPtpProxy::GetInstance();

    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(saManager, nullptr);
    auto token = saManager->GetSystemAbility(STORAGE_MANAGER_UID_TEST);

    Context context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    mtpPtpProxy.Init(token, context);

    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;

    int32_t fd = -1;

    context->handle = 0;
    int32_t res = mtpPtpProxy.GetReadFd(context, fd);
    EXPECT_EQ(res, MTP_ERROR_STORE_NOT_AVAILABLE);

    context->handle = PTP_IN_MTP_ID;
    res = mtpPtpProxy.GetReadFd(context, fd);
    EXPECT_EQ(res, MTP_ERROR_ACCESS_DENIED);

    context->handle = 2 * PTP_IN_MTP_ID;
    res = mtpPtpProxy.GetReadFd(context, fd);
    EXPECT_EQ(res, MTP_ERROR_STORE_NOT_AVAILABLE);
}

/*
 * Feature: MTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: CloseReadFd
 */
HWTEST_F(MtpPtpProxyTest, MtpPtpProxy_Test_008, TestSize.Level1)
{
    MtpPtpProxy mtpPtpProxy = OHOS::Media::MtpPtpProxy::GetInstance();

    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(saManager, nullptr);
    auto token = saManager->GetSystemAbility(STORAGE_MANAGER_UID_TEST);

    Context context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    mtpPtpProxy.Init(token, context);

    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;

    int32_t fd = -1;

    int32_t res = mtpPtpProxy.CloseReadFd(context, fd);
    EXPECT_EQ(res, E_ERR);
}

/*
 * Feature: MTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: CloseWriteFd
 */
HWTEST_F(MtpPtpProxyTest, MtpPtpProxy_Test_009, TestSize.Level1)
{
    MtpPtpProxy mtpPtpProxy = OHOS::Media::MtpPtpProxy::GetInstance();

    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(saManager, nullptr);
    auto token = saManager->GetSystemAbility(STORAGE_MANAGER_UID_TEST);

    Context context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    mtpPtpProxy.Init(token, context);

    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;

    int32_t fd = -1;

    int32_t res = mtpPtpProxy.CloseWriteFd(context, fd);
    EXPECT_EQ(res, E_ERR);
}

/*
 * Feature: MTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetModifyObjectInfoPathById
 */
HWTEST_F(MtpPtpProxyTest, MtpPtpProxy_Test_010, TestSize.Level1)
{
    MtpPtpProxy mtpPtpProxy = OHOS::Media::MtpPtpProxy::GetInstance();

    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(saManager, nullptr);
    auto token = saManager->GetSystemAbility(STORAGE_MANAGER_UID_TEST);

    Context context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    mtpPtpProxy.Init(token, context);

    int32_t handle = 1;
    std::string path = "";

    int32_t res = mtpPtpProxy.GetModifyObjectInfoPathById(handle, path);
    EXPECT_EQ(res, MTP_SUCCESS);

    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
    res = mtpPtpProxy.GetModifyObjectInfoPathById(handle, path);
    EXPECT_EQ(res, MTP_SUCCESS);
}

/*
 * Feature: MTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetWriteFd
 */
HWTEST_F(MtpPtpProxyTest, MtpPtpProxy_Test_011, TestSize.Level1)
{
    MtpPtpProxy mtpPtpProxy = OHOS::Media::MtpPtpProxy::GetInstance();

    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(saManager, nullptr);
    auto token = saManager->GetSystemAbility(STORAGE_MANAGER_UID_TEST);

    Context context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    mtpPtpProxy.Init(token, context);

    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;

    int32_t fd = -1;

    context->handle = 0;
    int32_t res = mtpPtpProxy.GetWriteFd(context, fd);
    EXPECT_EQ(res, MTP_ERROR_ACCESS_DENIED);

    context->handle = 2 * PTP_IN_MTP_ID;
    res = mtpPtpProxy.GetWriteFd(context, fd);
    EXPECT_EQ(res, MTP_ERROR_STORE_NOT_AVAILABLE);
}

/*
 * Feature: MTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetMtpPathById
 */
HWTEST_F(MtpPtpProxyTest, MtpPtpProxy_Test_012, TestSize.Level1)
{
    MtpPtpProxy mtpPtpProxy = OHOS::Media::MtpPtpProxy::GetInstance();

    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(saManager, nullptr);
    auto token = saManager->GetSystemAbility(STORAGE_MANAGER_UID_TEST);

    Context context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    mtpPtpProxy.Init(token, context);

    int32_t handle = 1;
    std::string outPath = "";

    int32_t res = mtpPtpProxy.GetMtpPathById(handle, outPath);
    EXPECT_EQ(res, E_ERR);
    EXPECT_EQ(outPath, "");
}

/*
 * Feature: MTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: DeleteCanceledObject
 */
HWTEST_F(MtpPtpProxyTest, MtpPtpProxy_Test_013, TestSize.Level1)
{
    MtpPtpProxy mtpPtpProxy = OHOS::Media::MtpPtpProxy::GetInstance();

    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(saManager, nullptr);
    auto token = saManager->GetSystemAbility(STORAGE_MANAGER_UID_TEST);

    Context context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    mtpPtpProxy.Init(token, context);

    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;

    std::string path = "test";
    uint32_t handle = 2 * PTP_IN_MTP_ID;

    mtpPtpProxy.DeleteCanceledObject(path, handle);
}

/*
 * Feature: MTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetThumb
 */
HWTEST_F(MtpPtpProxyTest, MtpPtpProxy_Test_014, TestSize.Level1)
{
    MtpPtpProxy mtpPtpProxy = OHOS::Media::MtpPtpProxy::GetInstance();

    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(saManager, nullptr);
    auto token = saManager->GetSystemAbility(STORAGE_MANAGER_UID_TEST);

    Context context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    mtpPtpProxy.Init(token, context);

    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;

    std::shared_ptr<UInt8List> outThumb = std::make_shared<UInt8List>();
    ASSERT_NE(outThumb, nullptr);

    context->handle = 0;
    int32_t res = mtpPtpProxy.GetThumb(context, outThumb);
    EXPECT_EQ(res, MTP_SUCCESS);

    context->handle = PTP_IN_MTP_ID;
    res = mtpPtpProxy.GetThumb(context, outThumb);
    EXPECT_EQ(res, MTP_ERROR_ACCESS_DENIED);

    context->handle = 2 * PTP_IN_MTP_ID;
    res = mtpPtpProxy.GetThumb(context, outThumb);
    EXPECT_EQ(res, MTP_ERROR_STORE_NOT_AVAILABLE);
}

/*
 * Feature: MTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: SendObjectInfo
 */
HWTEST_F(MtpPtpProxyTest, MtpPtpProxy_Test_015, TestSize.Level1)
{
    MtpPtpProxy mtpPtpProxy = OHOS::Media::MtpPtpProxy::GetInstance();

    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(saManager, nullptr);
    auto token = saManager->GetSystemAbility(STORAGE_MANAGER_UID_TEST);

    Context context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    mtpPtpProxy.Init(token, context);

    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;

    uint32_t storageID = 0;
    uint32_t parent = 1;
    uint32_t handle = 1;

    context->parent = 2 * PTP_IN_MTP_ID;
    int32_t res = mtpPtpProxy.SendObjectInfo(context, storageID, parent, handle);
    EXPECT_EQ(res, MTP_ERROR_STORE_NOT_AVAILABLE);

    context->parent = 1;
    res = mtpPtpProxy.SendObjectInfo(context, storageID, parent, handle);
    EXPECT_EQ(res, MTP_ERROR_ACCESS_DENIED);
}

/*
 * Feature: MTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: DeleteObject
 */
HWTEST_F(MtpPtpProxyTest, MtpPtpProxy_Test_016, TestSize.Level1)
{
    MtpPtpProxy mtpPtpProxy = OHOS::Media::MtpPtpProxy::GetInstance();

    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(saManager, nullptr);
    auto token = saManager->GetSystemAbility(STORAGE_MANAGER_UID_TEST);

    Context context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    mtpPtpProxy.Init(token, context);

    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;

    context->handle = 2 * PTP_IN_MTP_ID;
    int32_t res = mtpPtpProxy.DeleteObject(context);
    EXPECT_EQ(res, MTP_ERROR_INVALID_OBJECTHANDLE);

    context->handle = 1;
    res = mtpPtpProxy.DeleteObject(context);
    EXPECT_EQ(res, MTP_ERROR_ACCESS_DENIED);
}

/*
 * Feature: MTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: MoveObject
 */
HWTEST_F(MtpPtpProxyTest, MtpPtpProxy_Test_017, TestSize.Level1)
{
    MtpPtpProxy mtpPtpProxy = OHOS::Media::MtpPtpProxy::GetInstance();

    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(saManager, nullptr);
    auto token = saManager->GetSystemAbility(STORAGE_MANAGER_UID_TEST);

    Context context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    mtpPtpProxy.Init(token, context);

    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;

    uint32_t repeatHandle = 1;

    context->handle = 1;
    int32_t res = mtpPtpProxy.MoveObject(context, repeatHandle);
    EXPECT_EQ(res, MTP_ERROR_ACCESS_DENIED);

    context->handle = 2 * PTP_IN_MTP_ID;
    context->parent = 1;
    res = mtpPtpProxy.MoveObject(context, repeatHandle);
    EXPECT_EQ(res, MTP_ERROR_ACCESS_DENIED);

    context->handle = 2 * PTP_IN_MTP_ID;
    context->parent = 2 * PTP_IN_MTP_ID;
    res = mtpPtpProxy.MoveObject(context, repeatHandle);
    EXPECT_EQ(res, MTP_ERROR_STORE_NOT_AVAILABLE);
}

/*
 * Feature: MTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: CopyObject
 */
HWTEST_F(MtpPtpProxyTest, MtpPtpProxy_Test_018, TestSize.Level1)
{
    MtpPtpProxy mtpPtpProxy = OHOS::Media::MtpPtpProxy::GetInstance();

    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(saManager, nullptr);
    auto token = saManager->GetSystemAbility(STORAGE_MANAGER_UID_TEST);

    Context context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    mtpPtpProxy.Init(token, context);

    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;

    uint32_t outObjectHandle = 1;
    uint32_t oldHandle;

    context->handle = PTP_IN_MTP_ID;
    int32_t res = mtpPtpProxy.CopyObject(context, outObjectHandle, oldHandle);
    EXPECT_EQ(res, MTP_ERROR_ACCESS_DENIED);

    context->handle = 2 * PTP_IN_MTP_ID;
    context->parent = 1;
    res = mtpPtpProxy.CopyObject(context, outObjectHandle, oldHandle);
    EXPECT_EQ(res, MTP_ERROR_ACCESS_DENIED);

    context->handle = 2 * PTP_IN_MTP_ID;
    context->parent = 2 * PTP_IN_MTP_ID;
    res = mtpPtpProxy.CopyObject(context, outObjectHandle, oldHandle);
    EXPECT_EQ(res, MTP_ERROR_STORE_NOT_AVAILABLE);

    context->handle = 1;
    context->parent = 2 * PTP_IN_MTP_ID;
    res = mtpPtpProxy.CopyObject(context, outObjectHandle, oldHandle);
    EXPECT_EQ(res, MTP_ERROR_INVALID_OBJECTHANDLE);
}

/*
 * Feature: MTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetMtpStorageIds
 */
HWTEST_F(MtpPtpProxyTest, MtpPtpProxy_Test_019, TestSize.Level1)
{
    MtpPtpProxy mtpPtpProxy = OHOS::Media::MtpPtpProxy::GetInstance();

    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(saManager, nullptr);
    auto token = saManager->GetSystemAbility(STORAGE_MANAGER_UID_TEST);

    Context context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    mtpPtpProxy.Init(token, context);

    int32_t res = mtpPtpProxy.GetMtpStorageIds();
    EXPECT_EQ(res, MTP_SUCCESS);
}

/*
 * Feature: MTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetIdByPath
 */
HWTEST_F(MtpPtpProxyTest, MtpPtpProxy_Test_020, TestSize.Level1)
{
    MtpPtpProxy mtpPtpProxy = OHOS::Media::MtpPtpProxy::GetInstance();

    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(saManager, nullptr);
    auto token = saManager->GetSystemAbility(STORAGE_MANAGER_UID_TEST);

    Context context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    mtpPtpProxy.Init(token, context);

    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;

    std::string path = "";
    uint32_t outId;

    int32_t res = mtpPtpProxy.GetIdByPath(path, outId);
    EXPECT_EQ(res, E_NO_SUCH_FILE);
}

/*
 * Feature: MTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetPathByHandle
 */
HWTEST_F(MtpPtpProxyTest, MtpPtpProxy_Test_021, TestSize.Level1)
{
    MtpPtpProxy mtpPtpProxy = OHOS::Media::MtpPtpProxy::GetInstance();

    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(saManager, nullptr);
    auto token = saManager->GetSystemAbility(STORAGE_MANAGER_UID_TEST);

    Context context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    mtpPtpProxy.Init(token, context);

    uint32_t handle = 1;
    std::string path = "";
    std::string realPath = "";

    int32_t res = mtpPtpProxy.GetPathByHandle(handle, path, realPath);
    EXPECT_EQ(res, MTP_SUCCESS);

    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;

    res = mtpPtpProxy.GetPathByHandle(handle, path, realPath);
    EXPECT_EQ(res, MTP_SUCCESS);
    EXPECT_EQ(path, "");
    EXPECT_EQ(realPath, "");
}

/*
 * Feature: MTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: MtpTryAddExternalStorage
 */
HWTEST_F(MtpPtpProxyTest, MtpPtpProxy_Test_022, TestSize.Level1)
{
    MtpPtpProxy mtpPtpProxy = OHOS::Media::MtpPtpProxy::GetInstance();

    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(saManager, nullptr);
    auto token = saManager->GetSystemAbility(STORAGE_MANAGER_UID_TEST);

    Context context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    mtpPtpProxy.Init(token, context);

    std::string fsUuid = "test";
    uint32_t storageId = 0;

    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;

    bool res = mtpPtpProxy.MtpTryAddExternalStorage(fsUuid, storageId);
    EXPECT_TRUE(res);
}

/*
 * Feature: MTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: MtpTryRemoveExternalStorage
 */
HWTEST_F(MtpPtpProxyTest, MtpPtpProxy_Test_023, TestSize.Level1)
{
    MtpPtpProxy mtpPtpProxy = OHOS::Media::MtpPtpProxy::GetInstance();

    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(saManager, nullptr);
    auto token = saManager->GetSystemAbility(STORAGE_MANAGER_UID_TEST);

    Context context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    mtpPtpProxy.Init(token, context);

    std::string fsUuid = "test";
    uint32_t storageId = 0;

    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;

    bool res = mtpPtpProxy.MtpTryRemoveExternalStorage(fsUuid, storageId);
    EXPECT_TRUE(res);
}
} // namespace Media
} // namespace OHOS