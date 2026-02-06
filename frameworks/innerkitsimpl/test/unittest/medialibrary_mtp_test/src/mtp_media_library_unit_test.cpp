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

#include <cstdlib>
#include <thread>
#include <unistd.h>
#include "mtp_media_library_unit_test.h"
#include "mtp_media_library.h"
#include "medialibrary_errno.h"
#include "media_log.h"
#include "mtp_storage_manager.h"
#include "object_info.h"
#include "mtp_constants.h"
#include "mtp_ptp_const.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
// storage file
const std::string STORAGE_FILE = "/storage/media/local/files/Docs";
// file path
const std::string FILE_PATH = "/storage/media/local/files/Docs/Desktop";
// file real path
const std::string REAL_FILE_PATH = "/storage/media/100/local/files/Docs/Desktop";
// storage real path
const std::string REAL_STORAGE_FILE = "/storage/media/100/local/files/Docs";
// document real path
const std::string REAL_DOCUMENT_FILE = "/storage/media/100/local/files/Docs/Document";
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;
const std::shared_ptr<MtpMediaLibrary> mtpMediaLib_ = MtpMediaLibrary::GetInstance();
void MtpMediaLibraryUnitTest::SetUpTestCase(void) {}

void MtpMediaLibraryUnitTest::TearDownTestCase(void)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
}

// SetUp:Execute before each test case
void MtpMediaLibraryUnitTest::SetUp() {}
void MtpMediaLibraryUnitTest::TearDown(void) {}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: AddPathToMap Clear Init
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_001, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddPathToMap(FILE_PATH + "/" + std::to_string(i) + ".txt");
    }

    mtpMediaLib_->Clear();
    EXPECT_EQ(mtpMediaLib_->id_, OHOS::Media::HANDLE_DEFAULT_ID::START_ID);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: ObserverAddPathToMap ScanDirNoDepth GetHandles
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_002, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    //parent file path
    mtpMediaLib_->ObserverAddPathToMap(FILE_PATH);
    //file path
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->ObserverAddPathToMap(FILE_PATH + "/" + std::to_string(i) + ".txt");
    }

    uint32_t parentId = 0;
    mtpMediaLib_->GetIdByPath(FILE_PATH, parentId);
    std::vector<int> outHandles;
    mtpMediaLib_->GetHandles(parentId, outHandles, MEDIA_TYPE_FILE);
    EXPECT_GE(outHandles.size(), 0);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetHandles GetPathById ScanDirNoDepth
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_003, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    //parent file path
    mtpMediaLib_->ObserverAddPathToMap(FILE_PATH);
    //file path
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->ObserverAddPathToMap(FILE_PATH + "/" + std::to_string(i) + ".txt");
    }

    uint32_t parentId = 0;
    std::shared_ptr<UInt32List> outHandles = std::make_shared<UInt32List>();
    ASSERT_NE(outHandles, nullptr);
    mtpMediaLib_->GetIdByPath(FILE_PATH, parentId);
    context->parent = parentId;
    context->storageID = parentId;
    mtpMediaLib_->GetHandles(context, outHandles);
    EXPECT_GE(outHandles->size(), 0);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetFd
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_004, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    context->handle = 2;
    int32_t outFd = 0;
    int32_t errcode = mtpMediaLib_->GetFd(context, outFd);
    EXPECT_EQ(errcode, MTP_ERROR_INVALID_OBJECTHANDLE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: SendObjectInfo
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_007, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->storageID = 2;
    context->parent = 0;
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    uint32_t outStorageID = 0;
    uint32_t outParent = 0;
    uint32_t outHandle = 0;
    mtpMediaLib_->SendObjectInfo(context, outStorageID, outParent, outHandle);
    EXPECT_NE(outStorageID, context->storageID);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetIdByPath
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_008, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    uint32_t outId = 0;
    mtpMediaLib_->GetIdByPath(FILE_PATH + "/2.txt", outId);
    EXPECT_EQ(outId, 2);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: MoveObject
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_009, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->handle = 1;
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    uint32_t repeatHandle = 0;
    int32_t errcode = mtpMediaLib_->MoveObject(context, repeatHandle);
    EXPECT_NE(errcode, 0);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: CopyObject
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_010, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->handle = 1;
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    uint32_t outObjectHandle = 0;
    uint32_t oldHandle = 0;
    int32_t errcode = mtpMediaLib_->CopyObject(context, outObjectHandle, oldHandle);
    EXPECT_NE(errcode, 0);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: DeleteObject
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_011, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->handle = 1;
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    int32_t errcode = mtpMediaLib_->DeleteObject(context);
    EXPECT_EQ(errcode, MTP_SUCCESS);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: SetObjectPropValue
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_012, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->handle = 1;
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    int32_t errcode = mtpMediaLib_->SetObjectPropValue(context);
    EXPECT_NE(errcode, MTP_SUCCESS);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetHandlesMap
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_013, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->property = 1;
    context->depth = MTP_ALL_DEPTH;
    context->handle = 0;
    context->storageID = 1;
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    auto handlesMap = std::make_shared<std::unordered_map<uint32_t, std::string>>();
    handlesMap = mtpMediaLib_->GetHandlesMap(context);
    if (access(STORAGE_FILE.c_str(), R_OK) == 0) {
        EXPECT_FALSE(handlesMap->empty());
    } else {
        EXPECT_TRUE(handlesMap->empty());
    }
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetObjectPropList
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_014, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<std::vector<Property>> outProps = std::make_shared<std::vector<Property>>();
    ASSERT_NE(outProps, nullptr);
    context->property = 1;
    context->depth = MTP_ALL_DEPTH;
    context->handle = 0;
    context->storageID = 1;
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    mtpMediaLib_->GetObjectPropList(context, outProps);
    EXPECT_TRUE(outProps->empty());
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetObjectPropValue
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_015, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    uint64_t outIntVal = 0;
    uint128_t outLongVal = { 0 };
    std::string outStrVal = "";
    context->handle = 1;
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    int32_t errcode = -1;
    errcode = mtpMediaLib_->GetObjectPropValue(context, outIntVal, outLongVal, outStrVal);
    EXPECT_NE(errcode, MTP_SUCCESS);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetRealPath
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_016, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::string outPath = "";
    int32_t errcode = mtpMediaLib_->GetRealPath(FILE_PATH, outPath);
    EXPECT_NE(errcode, E_ERR);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetStorageIds
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_017, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    int errcode = -1;
    errcode = mtpMediaLib_->GetStorageIds();
    EXPECT_EQ(errcode, MTP_SUCCESS);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: DeleteHandlePathMap
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_018, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    uint32_t id = 0;
    mtpMediaLib_->DeleteHandlePathMap(FILE_PATH + "/1.txt", id);
    uint32_t outId = 0;
    int32_t errcode = mtpMediaLib_->GetIdByPath(FILE_PATH + "/1.txt", outId);
    EXPECT_EQ(errcode, E_NO_SUCH_FILE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: ObserverDeletePathToMap
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_019, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    mtpMediaLib_->ObserverDeletePathToMap(FILE_PATH + "/1.txt");
    uint32_t outId = 0;
    int32_t errcode = mtpMediaLib_->GetIdByPath(FILE_PATH + "/1.txt", outId);
    EXPECT_EQ(errcode, E_NO_SUCH_FILE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: AddToHandlePathMap
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_020, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    uint32_t id = 1;
    mtpMediaLib_->AddToHandlePathMap(FILE_PATH, id);
    uint32_t outId = 0;
    mtpMediaLib_->GetIdByPath(FILE_PATH, outId);
    EXPECT_EQ(outId, 1);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: ModifyHandlePathMap
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_021, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    mtpMediaLib_->ModifyHandlePathMap(FILE_PATH + "/1.txt", FILE_PATH + "/11.txt");
    uint32_t outId = 0;
    mtpMediaLib_->GetIdByPath(FILE_PATH + "/11.txt", outId);
    EXPECT_EQ(outId, 1);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: ModifyPathHandleMap
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_022, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    uint32_t id = 12;
    mtpMediaLib_->ModifyPathHandleMap(FILE_PATH + "/1.txt", id);
    uint32_t outId = 0;
    mtpMediaLib_->GetIdByPath(FILE_PATH + "/1.txt", outId);
    EXPECT_EQ(outId, 12);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: StartsWith
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_023, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::string str = "ab";
    std::string prefix = "ab";
    bool ret = false;
    ret = mtpMediaLib_->StartsWith(str, prefix);
    EXPECT_TRUE(ret);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: MoveHandlePathMap
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_024, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    mtpMediaLib_->MoveHandlePathMap(FILE_PATH, STORAGE_FILE);
    uint32_t outId = 0;
    int32_t errcode = mtpMediaLib_->GetIdByPath(FILE_PATH + "/1.txt", outId);
    EXPECT_EQ(errcode, E_NO_SUCH_FILE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetId
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_025, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    uint32_t id = 0;
    id = mtpMediaLib_->GetId();
    EXPECT_EQ(id, OHOS::Media::HANDLE_DEFAULT_ID::START_ID);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetParentId
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_026, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    uint32_t parentId = 1;
    parentId = mtpMediaLib_->GetParentId(FILE_PATH + "/1.txt");
    EXPECT_EQ(parentId, 0);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: AddPathToMap Clear Init
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_027, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    for(int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddPathToMap(FILE_PATH + std::to_string(i) + ".txt");
    }
    uint32_t id = OHOS::Media::HANDLE_DEFAULT_ID::START_ID + 10;
    EXPECT_EQ(mtpMediaLib_->id_, id);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetHandles
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_028, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    //parent file path
    mtpMediaLib_->ObserverAddPathToMap(FILE_PATH);
    //file path
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->ObserverAddPathToMap(FILE_PATH + std::to_string(i) + ".txt");
    }

    uint32_t parentId = 0;
    mtpMediaLib_->GetIdByPath(FILE_PATH, parentId);
    std::vector<int> outHandles;
    mtpMediaLib_->GetHandles(parentId, outHandles, MEDIA_TYPE_FILE);
    EXPECT_GE(outHandles.size(), 0);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetHandles
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_029, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    //parent file path
    mtpMediaLib_->ObserverAddPathToMap(FILE_PATH);
    //file path
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->ObserverAddPathToMap(FILE_PATH + std::to_string(i) + ".txt");
    }
    uint32_t parentId = 0;
    std::shared_ptr<UInt32List> outHandles = std::make_shared<UInt32List>();
    ASSERT_NE(outHandles, nullptr);
    mtpMediaLib_->GetIdByPath(FILE_PATH, parentId);
    context->parent = parentId;
    context->storageID = parentId;

    mtpMediaLib_->GetHandles(context, outHandles);
    EXPECT_GE(outHandles->size(), 0);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: ModifyHandlePathMap
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_031, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    //parent file path
    mtpMediaLib_->ObserverAddPathToMap(FILE_PATH);
    //file path
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->ObserverAddPathToMap(FILE_PATH + std::to_string(i) + ".txt");
    }

    mtpMediaLib_->ModifyHandlePathMap(FILE_PATH + std::to_string(1) +
        ".txt", STORAGE_FILE + std::to_string(1) + ".txt");
    uint32_t handle = 0;
    mtpMediaLib_->GetIdByPath(FILE_PATH + std::to_string(1) + ".txt", handle);
    EXPECT_EQ(handle, 0);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: ModifyPathHandleMap
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_032, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    //parent file path
    mtpMediaLib_->ObserverAddPathToMap(FILE_PATH);
    //file path
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->ObserverAddPathToMap(FILE_PATH + std::to_string(i) + ".txt");
    }

    uint32_t handle = 0;
    mtpMediaLib_->GetIdByPath(FILE_PATH + std::to_string(1) + ".txt", handle);
    mtpMediaLib_->ModifyPathHandleMap(FILE_PATH + std::to_string(1) + ".txt", handle);
    EXPECT_NE(handle, 0);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: MoveHandlePathMap
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_033, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::string str = FILE_PATH;
    std::string prefix = FILE_PATH;
    //storage file path
    mtpMediaLib_->ObserverAddPathToMap(STORAGE_FILE);
    //parent file path
    mtpMediaLib_->ObserverAddPathToMap(FILE_PATH);
    //file path
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->ObserverAddPathToMap(FILE_PATH + std::to_string(i) + ".txt");
    }

    std::string from = FILE_PATH + std::to_string(1) + ".txt";
    std::string to = STORAGE_FILE;
    mtpMediaLib_->MoveHandlePathMap(from, to);
    uint32_t handle = 0;
    int32_t errcode = mtpMediaLib_->GetIdByPath(STORAGE_FILE + std::to_string(1) + ".txt", handle);
    EXPECT_EQ(errcode, E_NO_SUCH_FILE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetIdByPath
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_034, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::string str = FILE_PATH;
    //parent file path
    mtpMediaLib_->ObserverAddPathToMap(FILE_PATH);
    uint32_t handle = 0;
    mtpMediaLib_->GetIdByPath(FILE_PATH, handle);
    mtpMediaLib_->DeleteHandlePathMap(FILE_PATH, handle);
    mtpMediaLib_->GetIdByPath(FILE_PATH, handle);
    EXPECT_NE(handle, 0);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetIdByPath
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_035, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::string str = FILE_PATH;
    //parent file path
    mtpMediaLib_->ObserverAddPathToMap(FILE_PATH);
    mtpMediaLib_->ObserverDeletePathToMap(FILE_PATH);
    uint32_t handle = 0;
    int32_t errcode = mtpMediaLib_->GetIdByPath(FILE_PATH, handle);
    EXPECT_EQ(errcode, E_NO_SUCH_FILE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetSizeFromOfft
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_036, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    const off_t size = 10;
    uint32_t result = mtpMediaLib_->GetSizeFromOfft(size);
    EXPECT_EQ(result, 10);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetFd
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_037, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    //storage file path
    mtpMediaLib_->ObserverAddPathToMap(STORAGE_FILE);
    //parent file path
    mtpMediaLib_->ObserverAddPathToMap(FILE_PATH);
    uint32_t handle = 0;
    mtpMediaLib_->GetIdByPath(FILE_PATH, handle);
    context->handle = handle;
    int32_t outFd = 0;
    int32_t errcode = mtpMediaLib_->GetFd(context, outFd);
    EXPECT_NE(errcode, MTP_ERROR_STORE_NOT_AVAILABLE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetRealPath
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_038, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    mtpMediaLib_->ObserverAddPathToMap(FILE_PATH);
    std::string outPath = "";
    mtpMediaLib_->GetRealPath(FILE_PATH, outPath);
    std::string result = "/storage/media/100/local/files/Docs/Desktop";
    EXPECT_NE(outPath, result);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: CloseFd
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_039, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    //storage file path
    mtpMediaLib_->ObserverAddPathToMap(STORAGE_FILE);
    //parent file path
    mtpMediaLib_->ObserverAddPathToMap(FILE_PATH);
    uint32_t handle = 0;
    mtpMediaLib_->GetIdByPath(FILE_PATH, handle);
    context->handle = handle;
    int32_t outFd = 0;
    mtpMediaLib_->GetFd(context, outFd);
    int32_t result = mtpMediaLib_->CloseFd(context,outFd);
    EXPECT_NE(result, MTP_SUCCESS);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: MoveObject
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_040, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    //storage file path
    mtpMediaLib_->ObserverAddPathToMap(STORAGE_FILE);
    //parent file path
    mtpMediaLib_->ObserverAddPathToMap(FILE_PATH);
    std::string from = STORAGE_FILE + "/Documents";
    mtpMediaLib_->ObserverAddPathToMap(from);
    std::string to = FILE_PATH;
    uint32_t fromId = 0;
    mtpMediaLib_->GetIdByPath(from, fromId);
    uint32_t parentId = 0;
    mtpMediaLib_->GetIdByPath(STORAGE_FILE, parentId);
    context->handle = fromId;
    context->parent = parentId;
    int32_t result = mtpMediaLib_->MoveObject(context,parentId);
    EXPECT_NE(result, MTP_SUCCESS);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: StartsWith
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_041, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    //storage file path
    mtpMediaLib_->ObserverAddPathToMap(STORAGE_FILE);
    //parent file path
    mtpMediaLib_->ObserverAddPathToMap(FILE_PATH);
    uint32_t handle = 0;
    mtpMediaLib_->GetIdByPath(FILE_PATH, handle);
    context->handle = handle;
    int32_t outFd = 0;
    mtpMediaLib_->GetFd(context, outFd);
    int32_t result = mtpMediaLib_->CloseFd(context, outFd);
    EXPECT_NE(result, MTP_SUCCESS);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: StartsWith
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_042, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::string str = FILE_PATH;
    std::string prefix = FILE_PATH;
    bool result = mtpMediaLib_->StartsWith(str, prefix);
    EXPECT_TRUE(result);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: MoveObjectSub
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_046, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    mtpMediaLib_->AddToHandlePathMap(STORAGE_FILE, 11);
    bool isDir = false;
    uint32_t repeatHandle = 0;
    uint32_t errcode = 1;
    errcode = mtpMediaLib_->MoveObjectSub(FILE_PATH, STORAGE_FILE, isDir, repeatHandle);
    EXPECT_EQ(errcode, MTP_SUCCESS);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetThumb
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_047, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<UInt8List> outThumb = std::make_shared<UInt8List>();
    ASSERT_NE(outThumb, nullptr);
    context->handle = 2;
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    int32_t errcode = 1;
    errcode = mtpMediaLib_->GetThumb(context, outThumb);
    EXPECT_EQ(errcode, MTP_SUCCESS);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: CloseFd
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_048, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    uint32_t handle = 0;
    mtpMediaLib_->GetIdByPath(FILE_PATH, handle);
    context->handle = handle;
    int32_t outFd = 0;
    mtpMediaLib_->GetFd(context, outFd);
    int32_t errcode = 1;
    errcode = mtpMediaLib_->CloseFd(context, outFd);
    EXPECT_NE(errcode, MTP_SUCCESS);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetVideoThumb
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_049, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<UInt8List> outThumb = std::make_shared<UInt8List>();
    ASSERT_NE(outThumb, nullptr);
    context->handle = 2;
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    mtpMediaLib_->GetVideoThumb(context, outThumb);
    EXPECT_EQ(outThumb->size(), 0);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetPictureThumb
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_050, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<UInt8List> outThumb = std::make_shared<UInt8List>();
    ASSERT_NE(outThumb, nullptr);
    context->handle = 2;
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    mtpMediaLib_->GetPictureThumb(context, outThumb);
    EXPECT_EQ(outThumb->size(), 0);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: ScanDirWithType
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_051, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<std::unordered_map<uint32_t, std::string>> out =
        std::make_shared<std::unordered_map<uint32_t, std::string>>();
    ASSERT_NE(out, nullptr);
    uint32_t errcode = 1;
    errcode = mtpMediaLib_->ScanDirWithType(STORAGE_FILE, out);
    if (access(STORAGE_FILE.c_str(), R_OK) == 0) {
        EXPECT_EQ(errcode, MTP_SUCCESS);
    } else {
        EXPECT_NE(errcode, MTP_SUCCESS);
    }
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: MoveRepeatDirHandlePathMap
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_052, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    mtpMediaLib_->AddToHandlePathMap(FILE_PATH, 11);
    mtpMediaLib_->AddToHandlePathMap(STORAGE_FILE, 12);
    mtpMediaLib_->MoveRepeatDirHandlePathMap(FILE_PATH, STORAGE_FILE);
    uint32_t outId = 0;
    int32_t errcode = mtpMediaLib_->GetIdByPath(FILE_PATH + "/1.txt", outId);
    EXPECT_EQ(errcode, E_NO_SUCH_FILE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: ScanDirTraverseWithType
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_053, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<std::unordered_map<uint32_t, std::string>> out =
        std::make_shared<std::unordered_map<uint32_t, std::string>>();
    ASSERT_NE(out, nullptr);
    uint32_t errcode = 1;
    errcode = mtpMediaLib_->ScanDirTraverseWithType(STORAGE_FILE, out);
    if (access(STORAGE_FILE.c_str(), R_OK) == 0) {
        EXPECT_EQ(errcode, MTP_SUCCESS);
    } else {
        EXPECT_NE(errcode, MTP_SUCCESS);
    }
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: ErasePathInfo
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_055, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    uint32_t handle = 12;
    mtpMediaLib_->ErasePathInfo(handle, STORAGE_FILE);
    uint32_t outId = 0;
    int32_t errcode = mtpMediaLib_->GetIdByPath(FILE_PATH + "/1.txt", outId);
    EXPECT_EQ(errcode, E_NO_SUCH_FILE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetExternalStorages
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_056, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::string path = FILE_PATH;
    mtpMediaLib_->GetExternalStorages();

    int64_t result = MtpStorageManager::GetInstance()->GetTotalSize(STORAGE_FILE);
    EXPECT_GE(result, 0);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: MoveRepeatDirHandlePathMap
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_057, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    // storage file path
    mtpMediaLib_->ObserverAddPathToMap(STORAGE_FILE);
    // parent file path
    mtpMediaLib_->ObserverAddPathToMap(FILE_PATH);

    std::string from = STORAGE_FILE;
    std::string to = FILE_PATH;
    mtpMediaLib_->MoveRepeatDirHandlePathMap(from, to);

    uint32_t outId = 0;
    std::string resultPath = FILE_PATH + "/Docs";
    mtpMediaLib_->GetIdByPath(FILE_PATH, outId);
    EXPECT_GT(outId, 0);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: MoveObjectSub
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_058, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    // storage file path
    mtpMediaLib_->ObserverAddPathToMap(STORAGE_FILE);
    // parent file path
    mtpMediaLib_->ObserverAddPathToMap(FILE_PATH);
    std::string from = STORAGE_FILE;
    std::string to = FILE_PATH;
    uint32_t repeatHandle = 0;
    uint32_t result = mtpMediaLib_->MoveObjectSub(from, to, false, repeatHandle);

    EXPECT_EQ(result, MTP_SUCCESS);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: ScanDirWithType
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_059, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::string root = FILE_PATH;
    std::shared_ptr<std::unordered_map<uint32_t, std::string>> outMap =
        std::make_shared<std::unordered_map<uint32_t, std::string>>();
    ASSERT_NE(outMap, nullptr);
    mtpMediaLib_->ScanDirWithType(root, outMap);

    EXPECT_GE(outMap->size(), 0);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: ErasePathInfo
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_060, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    // parent file path
    mtpMediaLib_->ObserverAddPathToMap(FILE_PATH);
    uint32_t handle = 0;
    mtpMediaLib_->GetIdByPath(FILE_PATH, handle);
    mtpMediaLib_->ErasePathInfo(handle, FILE_PATH);

    std::string outResult;
    mtpMediaLib_->GetPathById(handle, outResult);
    EXPECT_TRUE(outResult.empty());
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetParentId
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_061, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    uint32_t parentId = 1;
    parentId = mtpMediaLib_->GetParentId(FILE_PATH + "/3.txt");
    EXPECT_EQ(parentId, 0);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetParentId
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_062, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    mtpMediaLib_->AddToHandlePathMap(FILE_PATH, 11);
    uint32_t parentId = 0;
    parentId = mtpMediaLib_->GetParentId(FILE_PATH + "/3.txt");
    EXPECT_EQ(parentId, 11);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: ModifyHandlePathMap
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_063, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    std::string from = FILE_PATH + "/11.txt";
    std::string to = STORAGE_FILE + "/11.txt";
    mtpMediaLib_->ModifyHandlePathMap(from, to);
    uint32_t outId = 0;
    mtpMediaLib_->GetIdByPath(FILE_PATH + "/1.txt", outId);
    EXPECT_EQ(outId, 1);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: ModifyPathHandleMap
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_064, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    std::string path = FILE_PATH;
    uint32_t id = 11;
    mtpMediaLib_->ModifyPathHandleMap(path, id);
    uint32_t outId = 0;
    mtpMediaLib_->GetIdByPath(FILE_PATH + "/1.txt", outId);
    EXPECT_EQ(outId, 1);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: ScanDirNoDepth
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_065, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();

    std::string root = FILE_PATH;
    std::shared_ptr<UInt32List> out = std::make_shared<UInt32List>();
    ASSERT_NE(out, nullptr);
    int32_t errcode = 0;
    errcode = mtpMediaLib_->ScanDirNoDepth(root, out);
    EXPECT_EQ(errcode, E_ERR);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: ScanDirNoDepth
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_066, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();

    std::string root = FILE_PATH;
    std::shared_ptr<UInt32List> out = nullptr;
    int32_t errcode = 0;
    errcode = mtpMediaLib_->ScanDirNoDepth(root, out);
    EXPECT_EQ(errcode, E_ERR);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: StartsWith
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_067, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();

    std::string str = "ab";
    std::string prefix = "abc";
    bool res = mtpMediaLib_->StartsWith(str, prefix);
    EXPECT_FALSE(res);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: StartsWith
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_068, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();

    std::string str = "ab";
    std::string prefix = "";
    bool res = mtpMediaLib_->StartsWith(str, prefix);
    EXPECT_FALSE(res);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: StartsWith
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_069, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();

    std::string str = "ab";
    std::string prefix = "ab";
    bool res = mtpMediaLib_->StartsWith(str, prefix);
    EXPECT_TRUE(res);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: StartsWith
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_070, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();

    std::string str = "ab";
    std::string prefix = "bc";
    bool res = mtpMediaLib_->StartsWith(str, prefix);
    EXPECT_FALSE(res);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: DeleteHandlePathMap
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_071, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    std::string path = FILE_PATH;
    uint32_t id = 2;
    mtpMediaLib_->DeleteHandlePathMap(path, id);
    uint32_t outId = 0;
    int32_t errcode = mtpMediaLib_->GetIdByPath(path, outId);
    EXPECT_EQ(errcode, E_NO_SUCH_FILE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: DeleteHandlePathMap
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_072, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    std::string path = FILE_PATH + "/1.txt";
    uint32_t id = 12;
    mtpMediaLib_->DeleteHandlePathMap(path, id);
    std::string outPath = "";
    int32_t errcode = mtpMediaLib_->GetPathById(id, outPath);
    EXPECT_EQ(errcode, E_ERR);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: ObserverAddPathToMap
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_073, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    std::string path = FILE_PATH + "/11.txt";
    int id = mtpMediaLib_->ObserverAddPathToMap(path);
    EXPECT_EQ(id, OHOS::Media::HANDLE_DEFAULT_ID::START_ID);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: ObserverAddPathToMap
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_074, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    std::string path = FILE_PATH + "/2.txt";
    int id = mtpMediaLib_->ObserverAddPathToMap(path);
    EXPECT_EQ(id, 2);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: ObserverDeletePathToMap
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_075, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();

    std::string path = FILE_PATH;
    mtpMediaLib_->ObserverDeletePathToMap(path);
    uint32_t outId = 0;
    int32_t errcode = mtpMediaLib_->GetIdByPath(path, outId);
    EXPECT_EQ(errcode, E_NO_SUCH_FILE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: MoveHandlePathMap
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_076, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    std::string from = "/storage/media/local/files/Docss";
    std::string to = FILE_PATH;
    mtpMediaLib_->MoveHandlePathMap(from, to);
    uint32_t outId = 0;
    int32_t errcode = mtpMediaLib_->GetIdByPath(from, outId);
    EXPECT_EQ(errcode, E_NO_SUCH_FILE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:GetObjectInfo
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Branch
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_077, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    std::shared_ptr<ObjectInfo> objectInfo = std::make_shared<ObjectInfo>(0);
    ASSERT_NE(context, nullptr);
    ASSERT_NE(objectInfo, nullptr);
    context->handle = 0;

    int32_t result = mtpMediaLib_->GetObjectInfo(context, objectInfo);
    EXPECT_EQ(result, MTP_ERROR_STORE_NOT_AVAILABLE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:GetObjectInfo
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Branch
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_078, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = nullptr;
    std::shared_ptr<ObjectInfo> objectInfo = std::make_shared<ObjectInfo>(0);
    ASSERT_NE(objectInfo, nullptr);

    int32_t result = mtpMediaLib_->GetObjectInfo(context, objectInfo);
    EXPECT_EQ(result, MTP_ERROR_STORE_NOT_AVAILABLE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:GetObjectInfo
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Branch
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_079, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    std::shared_ptr<ObjectInfo> objectInfo = std::make_shared<ObjectInfo>(0);
    ASSERT_NE(context, nullptr);
    ASSERT_NE(objectInfo, nullptr);
    context->handle = 0;

    int32_t result = mtpMediaLib_->GetObjectInfo(context, objectInfo);
    EXPECT_EQ(result, MTP_ERROR_STORE_NOT_AVAILABLE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:GetObjectInfo
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Branch
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_080, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    std::shared_ptr<ObjectInfo> objectInfo = std::make_shared<ObjectInfo>(0);
    ASSERT_NE(context, nullptr);
    ASSERT_NE(objectInfo, nullptr);
    context->handle = 1;

    int32_t result = mtpMediaLib_->GetObjectInfo(context, objectInfo);
    EXPECT_EQ(result, MTP_ERROR_STORE_NOT_AVAILABLE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:GetObjectInfo
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Branch
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_081, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    std::shared_ptr<ObjectInfo> objectInfo = std::make_shared<ObjectInfo>(0);
    ASSERT_NE(context, nullptr);
    ASSERT_NE(objectInfo, nullptr);
    context->handle = 100;
    context->storageID = 1;

    int32_t result = mtpMediaLib_->GetObjectInfo(context, objectInfo);
    EXPECT_EQ(result, MTP_ERROR_STORE_NOT_AVAILABLE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:SendObjectInfo
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Branch
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_082, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    uint32_t outStorageID = 0;
    uint32_t outParent = 0;
    uint32_t outHandle = 0;

    int32_t result = mtpMediaLib_->SendObjectInfo(nullptr, outStorageID, outParent, outHandle);
    EXPECT_EQ(result, MTP_ERROR_STORE_NOT_AVAILABLE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:SendObjectInfo
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Branch
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_083, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->storageID = 2;
    context->parent = MTP_ALL_HANDLE_ID;

    uint32_t outStorageID = 0;
    uint32_t outParent = 0;
    uint32_t outHandle = 0;

    int32_t result = mtpMediaLib_->SendObjectInfo(context, outStorageID, outParent, outHandle);
    EXPECT_EQ(result, MTP_ERROR_STORE_NOT_AVAILABLE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:SendObjectInfo
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Branch
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_084, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    // parent file path
    mtpMediaLib_->ObserverAddPathToMap(FILE_PATH);
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    uint32_t parent = 0;
    mtpMediaLib_->GetIdByPath(FILE_PATH, parent);
    context->storageID = 0;
    context->parent = parent;
    context->format = MTP_FORMAT_TEXT_CODE;
    context->name = "1.txt";

    uint32_t outStorageID = 0;
    uint32_t outParent = 0;
    uint32_t outHandle = 0;

    mtpMediaLib_->SendObjectInfo(context, outStorageID, outParent, outHandle);
    EXPECT_EQ(outStorageID, 0);
}

/*
 * Feature: MediaLibraryMTP
 * Function:SendObjectInfo
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Branch
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_085, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    // storage file path
    mtpMediaLib_->ObserverAddPathToMap(STORAGE_FILE);
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    uint32_t parent = 0;
    mtpMediaLib_->GetIdByPath(FILE_PATH, parent);
    context->storageID = parent;
    context->parent = parent;
    context->format = MTP_FORMAT_ASSOCIATION_CODE;

    uint32_t outStorageID = 0;
    uint32_t outParent = 0;
    uint32_t outHandle = 0;

    mtpMediaLib_->SendObjectInfo(context, outStorageID, outParent, outHandle);
    EXPECT_EQ(outStorageID, 0);
}

/*
 * Feature: MediaLibraryMTP
 * Function:GetObjectPropList
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Branch
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_086, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<std::vector<Property>> outProps = std::make_shared<std::vector<Property>>();
    ASSERT_NE(outProps, nullptr);

    context->property = 0;
    context->groupCode = 0;
    context->depth = MTP_ALL_DEPTH;
    context->handle = 0;
    context->storageID = 1;

    int32_t result = mtpMediaLib_->GetObjectPropList(context, outProps);
    EXPECT_EQ(result, MTP_ERROR_PARAMETER_NOT_SUPPORTED);
}

/*
 * Feature: MediaLibraryMTP
 * Function:GetObjectPropList
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Branch
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_087, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<std::vector<Property>> outProps = std::make_shared<std::vector<Property>>();
    ASSERT_NE(outProps, nullptr);

    context->property = 0;
    context->groupCode = 1;
    context->depth = 0;
    context->handle = 0;
    context->storageID = 1;

    int32_t result = mtpMediaLib_->GetObjectPropList(context, outProps);
    EXPECT_EQ(result, MTP_ERROR_SPECIFICATION_BY_GROUP_UNSUPPORTED);
}

/*
 * Feature: MediaLibraryMTP
 * Function:GetObjectPropList
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Branch
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_088, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    mtpMediaLib_->ObserverAddPathToMap(STORAGE_FILE);
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<std::vector<Property>> outProps = std::make_shared<std::vector<Property>>();
    ASSERT_NE(outProps, nullptr);
    context->property = 1;
    context->groupCode = 1;
    context->depth = 3;
    context->handle = 4;
    context->storageID = 1;

    int32_t result = mtpMediaLib_->GetObjectPropList(context, outProps);
    EXPECT_EQ(result, MTP_ERROR_SPECIFICATION_BY_DEPTH_UNSUPPORTED);
}

/*
 * Feature: MediaLibraryMTP
 * Function:GetThumb
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Branch
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_089, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = nullptr;
    std::shared_ptr<UInt8List> outThumb = std::make_shared<UInt8List>();
    ASSERT_NE(outThumb, nullptr);

    int32_t result = 0;
    result = mtpMediaLib_->GetThumb(context, outThumb);
    EXPECT_EQ(result, MTP_ERROR_CONTEXT_IS_NULL);
}

/*
 * Feature: MediaLibraryMTP
 * Function:GetThumb
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Branch
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_090, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    mtpMediaLib_->ObserverAddPathToMap(REAL_FILE_PATH);
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<UInt8List> outThumb = std::make_shared<UInt8List>();
    ASSERT_NE(outThumb, nullptr);

    uint32_t outHandle = 0;
    mtpMediaLib_->GetIdByPath(REAL_FILE_PATH, outHandle);
    context->handle = outHandle;

    int32_t result = 0;
    result = mtpMediaLib_->GetThumb(context, outThumb);
    EXPECT_EQ(result, MTP_SUCCESS);
}

/*
 * Feature: MediaLibraryMTP
 * Function:GetThumb
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Branch
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_091, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    mtpMediaLib_->ObserverAddPathToMap(REAL_FILE_PATH);
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<UInt8List> outThumb = std::make_shared<UInt8List>();
    ASSERT_NE(outThumb, nullptr);

    uint32_t outHandle = 0;
    mtpMediaLib_->GetIdByPath(REAL_FILE_PATH, outHandle);
    context->handle = outHandle;

    int32_t result = 0;
    result = mtpMediaLib_->GetThumb(context, outThumb);
    EXPECT_EQ(result, MTP_SUCCESS);
}

/*
 * Feature: MediaLibraryMTP
 * Function:GetParentId
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Branch
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_092, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    mtpMediaLib_->ObserverAddPathToMap(FILE_PATH);
    std::string childPath = FILE_PATH + "/1.txt";
    mtpMediaLib_->ObserverAddPathToMap(childPath);

    uint32_t parentId = 0;
    uint32_t result = 0;
    mtpMediaLib_->GetIdByPath(FILE_PATH, result);
    parentId = mtpMediaLib_->GetParentId(childPath);
    EXPECT_EQ(parentId, result);
}

/*
 * Feature: MediaLibraryMTP
 * Function:GetParentId
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Branch
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_093, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::string childPath = FILE_PATH + "/1.txt";
    mtpMediaLib_->ObserverAddPathToMap(childPath);

    uint32_t parentId = 10;
    parentId = mtpMediaLib_->GetParentId(childPath);
    EXPECT_EQ(parentId, 0);
}

/*
 * Feature: MediaLibraryMTP
 * Function:CopyObject
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Branch
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_094, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    mtpMediaLib_->AddToHandlePathMap(REAL_FILE_PATH, 1);
    mtpMediaLib_->AddToHandlePathMap(STORAGE_FILE, 2);
    mtpMediaLib_->AddToHandlePathMap(REAL_DOCUMENT_FILE, 3);
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    uint32_t fileHandle = 0;
    uint32_t storageHandle = 0;
    uint32_t documentHanle = 0;
    mtpMediaLib_->GetIdByPath(REAL_FILE_PATH, fileHandle);
    mtpMediaLib_->GetIdByPath(STORAGE_FILE, storageHandle);
    mtpMediaLib_->GetIdByPath(REAL_DOCUMENT_FILE, documentHanle);

    context->handle = fileHandle;
    context->parent = storageHandle;
    uint32_t outObjectHandle = 0;
    uint32_t oldHandle = 0;
    std::string from("");
    std::string to("");
    mtpMediaLib_->GetPathById(context->handle, from);
    mtpMediaLib_->GetPathByContextParent(context, to);
    bool cond = (!sf::exists(from) || !sf::exists(to));

    int32_t result = mtpMediaLib_->CopyObject(context, outObjectHandle, oldHandle);
    if (cond) {
        EXPECT_EQ(result, MTP_ERROR_INVALID_OBJECTHANDLE);
    } else {
        EXPECT_EQ(result, MTP_SUCCESS);
    }
}

/*
 * Feature: MediaLibraryMTP
 * Function:GetObjectPropList
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Branch
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_095, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    mtpMediaLib_->ObserverAddPathToMap(STORAGE_FILE);
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<std::vector<Property>> outProps = std::make_shared<std::vector<Property>>();
    ASSERT_NE(outProps, nullptr);

    context->property = 1;
    context->groupCode = 1;
    context->depth = 3;
    context->handle = 4;
    context->storageID = 1;

    int32_t result = mtpMediaLib_->GetObjectPropList(context, outProps);
    EXPECT_EQ(result, MTP_ERROR_SPECIFICATION_BY_DEPTH_UNSUPPORTED);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: AddToHandlePathMap
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_097, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    uint32_t id = 11;
    std::string path = FILE_PATH + "/10.txt";
    mtpMediaLib_->AddToHandlePathMap(FILE_PATH, id);
    uint32_t outId = 0;
    mtpMediaLib_->GetIdByPath(FILE_PATH, outId);
    EXPECT_EQ(outId, 11);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: MoveRepeatDirHandlePathMap
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_098, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    std::string from = FILE_PATH;
    std::string to = STORAGE_FILE;
    mtpMediaLib_->MoveRepeatDirHandlePathMap(from, to);
    uint32_t outId = 0;
    int32_t errcode = mtpMediaLib_->GetIdByPath(from, outId);
    EXPECT_EQ(errcode, E_NO_SUCH_FILE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: MoveRepeatDirHandlePathMap
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_099, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    std::string from = REAL_FILE_PATH;
    std::string to = STORAGE_FILE;
    mtpMediaLib_->MoveRepeatDirHandlePathMap(from, to);
    uint32_t outId = 0;
    int32_t errcode = mtpMediaLib_->GetIdByPath(from, outId);
    EXPECT_EQ(errcode, E_NO_SUCH_FILE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetHandles
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_100, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    uint32_t parentId = 0;
    std::vector<int> outHandles;
    int32_t errcode = mtpMediaLib_->GetHandles(parentId, outHandles, MEDIA_TYPE_FILE);
    EXPECT_EQ(errcode, MTP_ERROR_STORE_NOT_AVAILABLE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetHandles
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_101, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = nullptr;
    std::shared_ptr<UInt32List> outHandles = std::make_shared<UInt32List>();
    ASSERT_NE(outHandles, nullptr);
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    int32_t errcode = mtpMediaLib_->GetHandles(context, outHandles);
    EXPECT_EQ(errcode, MTP_ERROR_STORE_NOT_AVAILABLE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetHandles
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_102, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<UInt32List> outHandles = std::make_shared<UInt32List>();
    ASSERT_NE(outHandles, nullptr);
    context->parent = 0;
    context->storageID = 0;
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    int32_t errcode = mtpMediaLib_->GetHandles(context, outHandles);
    EXPECT_EQ(errcode, MTP_ERROR_STORE_NOT_AVAILABLE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetHandles
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_103, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<UInt32List> outHandles = std::make_shared<UInt32List>();
    ASSERT_NE(outHandles, nullptr);
    context->parent = MTP_ALL_HANDLE_ID;
    context->storageID = 1;
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    int32_t res = mtpMediaLib_->GetHandles(context, outHandles);
    EXPECT_EQ(res, MTP_ERROR_STORE_NOT_AVAILABLE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetHandles
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_104, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    mtpMediaLib_->AddToHandlePathMap(FILE_PATH, 1);
    for (int i = 2; i <= 11; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    uint32_t parentId = 1;
    std::vector<int> outHandles;
    int32_t res = mtpMediaLib_->GetHandles(parentId, outHandles, MEDIA_TYPE_FILE);
    EXPECT_EQ(res, MTP_SUCCESS);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetObjectInfo
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_105, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<ObjectInfo> objectInfo = nullptr;
    context->handle = 1;
    int32_t res = mtpMediaLib_->GetObjectInfo(context, objectInfo);
    EXPECT_EQ(res, MTP_ERROR_STORE_NOT_AVAILABLE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetObjectInfo
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_106, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->handle = 1;
    context->storageID = 2;
    std::shared_ptr<ObjectInfo> objectInfo = std::make_shared<ObjectInfo>(0);
    ASSERT_NE(objectInfo, nullptr);
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    mtpMediaLib_->AddToHandlePathMap(FILE_PATH, 11);
    int32_t res = mtpMediaLib_->GetObjectInfo(context, objectInfo);
    EXPECT_NE(res, MTP_SUCCESS);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetFd CondCloseFd
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_107, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = nullptr;
    bool condition = false;
    int fd = 0;
    mtpMediaLib_->CondCloseFd(condition, fd);

    int32_t outFd = 0;
    int32_t errcode = mtpMediaLib_->GetFd(context, outFd);
    EXPECT_EQ(errcode, MTP_ERROR_STORE_NOT_AVAILABLE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetFd CondCloseFd
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_108, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = nullptr;
    bool condition = true;
    int fd = 0;
    mtpMediaLib_->CondCloseFd(condition, fd);

    int32_t outFd = 0;
    int32_t errcode = mtpMediaLib_->GetFd(context, outFd);
    EXPECT_EQ(errcode, MTP_ERROR_STORE_NOT_AVAILABLE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetFd CondCloseFd
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_109, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = nullptr;
    bool condition = false;
    int fd = 1;
    mtpMediaLib_->CondCloseFd(condition, fd);

    int32_t outFd = 0;
    int32_t errcode = mtpMediaLib_->GetFd(context, outFd);
    EXPECT_EQ(errcode, MTP_ERROR_STORE_NOT_AVAILABLE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetGalleryObjectInfo
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_110, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->handle = PTP_IN_MTP_ID;
    std::shared_ptr<ObjectInfo> outObjectInfo = std::make_shared<ObjectInfo>(context->handle);
    ASSERT_NE(outObjectInfo, nullptr);
    std::string name = "test";
    int32_t res = mtpMediaLib_->GetGalleryObjectInfo(context, outObjectInfo, name);
    EXPECT_EQ(res, MTP_SUCCESS);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetGalleryPropValue
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_111, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);

    uint64_t outIntVal = 0;
    uint128_t outLongVal = {0};
    std::string outStrVal = "";
    std::string name = "test";

    context->property = MTP_PROPERTY_PARENT_OBJECT_CODE;
    int32_t res = mtpMediaLib_->GetGalleryPropValue(context, outIntVal, outLongVal, outStrVal, name);
    EXPECT_EQ(outIntVal, DEFAULT_PARENT_ROOT);
    EXPECT_EQ(res, MTP_SUCCESS);

    context->property = MTP_PROPERTY_DISPLAY_NAME_CODE;
    mtpMediaLib_->GetGalleryPropValue(context, outIntVal, outLongVal, outStrVal, name);
    EXPECT_EQ(outStrVal, "test");

    context->property = MTP_PROPERTY_KEYWORDS_CODE;
    res = mtpMediaLib_->GetGalleryPropValue(context, outIntVal, outLongVal, outStrVal, name);
    EXPECT_EQ(res, MTP_SUCCESS);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetGalleryObjectPropList
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_112, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::shared_ptr<std::vector<Property>> outProps = std::make_shared<std::vector<Property>>();
    ASSERT_NE(outProps, nullptr);
    std::string name = "test";

    context->format = MTP_FORMAT_GIF_CODE;
    context->property = MTP_PROPERTY_ALL_CODE;
    int32_t res = mtpMediaLib_->GetGalleryObjectPropList(context, outProps, name);
    EXPECT_EQ(res, MTP_SUCCESS);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: CopyGalleryPhoto
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_113, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    PathMap paths;
    uint32_t outObjectHandle = 0;

    context->parent = 0;
    int32_t res = mtpMediaLib_->CopyGalleryPhoto(context, paths, outObjectHandle);
    EXPECT_EQ(res, MTP_ERROR_INVALID_OBJECTHANDLE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: CopyGalleryAlbum
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_114, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    std::string albumName = "";
    PathMap paths;
    uint32_t outObjectHandle = 0;

    context->parent = 0;
    int32_t res = mtpMediaLib_->CopyGalleryAlbum(context, albumName, paths, outObjectHandle);
    EXPECT_EQ(res, MTP_ERROR_INVALID_OBJECTHANDLE);
}

/*
 * Feature: MediaLibraryMTP
 * Function:SendObjectInfo
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: SendObjectInfo
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_115, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->storageID = 2;
    context->parent = 0;
    context->format = MTP_FORMAT_ASSOCIATION_CODE;
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }
    mtpMediaLib_->AddExternalStorage(FILE_PATH, context->storageID);
    uint32_t outStorageID = 0;
    uint32_t outParent = 0;
    uint32_t outHandle = 0;
    mtpMediaLib_->SendObjectInfo(context, outStorageID, outParent, outHandle);
    EXPECT_NE(outStorageID, context->storageID);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: MoveObjectSub
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_116, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::string path = FILE_PATH;
    uint32_t id = 10;
    mtpMediaLib_->AddToHandlePathMap(path, id);
    mtpMediaLib_->AddToHandlePathMap(path, id);

    bool isDir = true;
    uint32_t repeatHandle = 0;
    uint32_t errcode = 1;
    errcode = mtpMediaLib_->MoveObjectSub(FILE_PATH, STORAGE_FILE, isDir, repeatHandle);
    EXPECT_EQ(errcode, MTP_SUCCESS);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: MoveObjectSub
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_117, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    mtpMediaLib_->AddToHandlePathMap(STORAGE_FILE, 11);
    bool isDir = true;
    uint32_t repeatHandle = 0;
    uint32_t errcode = 1;
    errcode = mtpMediaLib_->MoveObjectSub(FILE_PATH, STORAGE_FILE, isDir, repeatHandle);
    EXPECT_EQ(errcode, MTP_SUCCESS);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetHandlesMap
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_118, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->property = 1;
    context->depth = 0;
    context->handle = 1;
    context->storageID = 1;
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    auto handlesMap = std::make_shared<std::unordered_map<uint32_t, std::string>>();
    handlesMap = mtpMediaLib_->GetHandlesMap(context);
    EXPECT_TRUE(handlesMap->empty());
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetHandlesMap
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_119, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->property = 1;
    context->depth = 1;
    context->handle = 1;
    context->storageID = 1;
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    auto handlesMap = std::make_shared<std::unordered_map<uint32_t, std::string>>();
    handlesMap = mtpMediaLib_->GetHandlesMap(context);
    EXPECT_TRUE(handlesMap->empty());
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: AddExternalStorage and CorrectStorageId
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_120, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->handle = 1;
    context->storageID = 1;
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap("/storage/External" + FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }
    std::string fsUuid = "storage/media/local/files/Docs/Desktop/1.txt";
    uint32_t storageId = 1;
    mtpMediaLib_->AddExternalStorage(fsUuid, storageId);
    bool ret = mtpMediaLib_->AddExternalStorage(fsUuid, storageId);
    EXPECT_TRUE(ret);
    mtpMediaLib_->CorrectStorageId(context);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetObjectPropValue
 */
HWTEST_F(MtpMediaLibraryUnitTest, medialibrary_MTP_message_testlevel_121, TestSize.Level1)
{
    ASSERT_NE(mtpMediaLib_, nullptr);
    mtpMediaLib_->Clear();
    std::shared_ptr<MtpOperationContext> context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    uint64_t outIntVal = 0;
    uint128_t outLongVal = { 0 };
    std::string outStrVal = "";
    context->handle = 1;
    context->property = MTP_PROPERTY_PARENT_OBJECT_CODE;
    for (int i = 1; i <= 10; i++) {
        mtpMediaLib_->AddToHandlePathMap(FILE_PATH + "/" + std::to_string(i) + ".txt", i);
    }

    int32_t errcode = -1;
    errcode = mtpMediaLib_->GetObjectPropValue(context, outIntVal, outLongVal, outStrVal);
    EXPECT_EQ(errcode, MTP_SUCCESS);
}
} // namespace Media
} // namespace OHOS