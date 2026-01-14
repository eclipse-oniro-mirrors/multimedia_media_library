/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "medialibrary_mtp_unit_test.h"
#include "medialibrary_errno.h"
#include "mtp_constants.h"
#include "mtp_storage_manager.h"
#include "property.h"
#include "payload_data/close_session_data.h"
#include "payload_data/copy_object_data.h"
#include "payload_data/delete_object_data.h"
#include "payload_data/get_device_info_data.h"
#include "payload_data/get_device_prop_desc_data.h"
#include "payload_data/get_device_prop_value_data.h"
#include "payload_data/get_num_objects_data.h"
#include "payload_data/get_object_data.h"
#include "payload_data/get_object_handles_data.h"
#include "payload_data/get_object_info_data.h"
#include "payload_data/get_object_prop_desc_data.h"
#include "payload_data/get_object_prop_list_data.h"
#include "payload_data/get_object_prop_value_data.h"
#include "payload_data/get_object_props_supported_data.h"
#include "payload_data/get_object_references_data.h"
#include "payload_data/get_partial_object_data.h"
#include "payload_data/get_storage_ids_data.h"
#include "payload_data/get_storage_info_data.h"
#include "payload_data/get_thumb_data.h"
#include "payload_data/move_object_data.h"
#include "payload_data/object_event_data.h"
#include "payload_data/open_session_data.h"
#include "payload_data/resp_common_data.h"
#include "payload_data/send_object_data.h"
#include "payload_data/send_object_info_data.h"
#include "payload_data/set_device_prop_value_data.h"
#include "payload_data/set_object_references_data.h"

using namespace std;
using namespace testing::ext;
static constexpr int COUNT_NUM_BIG = 30;
static constexpr int COUNT_NUM_SMALL = 16;
const int PARAM_INDEX_1 = 1;
const int PARAM_INDEX_2 = 2;
const int PARAM_INDEX_3 = 3;
const int PARAM_INDEX_4 = 4;
const int PARAM_INDEX_5 = 5;
const int DATA_COUNT_1 = 4;
const int DATA_COUNT_2 = 6;
const int DATA_COUNT_3 = 8;
const int DATA_COUNT_4 = 38;
const int DATA_COUNT_5 = 42;
const int DATA_COUNT_6 = 44;
const int DATA_COUNT_7 = 48;
const int DATA_COUNT_8 = 60;
const int IMAGEINFO_COUNT_1 = 2;
const int IMAGEINFO_COUNT_2 = 6;
const int IMAGEINFO_COUNT_3 = 10;
const int IMAGEINFO_COUNT_4 = 14;
const int IMAGEINFO_COUNT_5 = 22;

namespace OHOS {
namespace Media {
HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parser_test_001, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context;
    GetObjectPropValueData getObjectPropValueDataOne(context);
    vector<uint8_t> buffer;
    int ret = getObjectPropValueDataOne.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_FAIL);
    auto mtpStorageManager = MtpStorageManager::GetInstance();
    EXPECT_NE(mtpStorageManager, nullptr);
    auto storage = make_shared<Storage>();
    EXPECT_NE(storage, nullptr);
    mtpStorageManager->AddStorage(storage);
    context = make_shared<MtpOperationContext>();
    GetObjectPropValueData getObjectPropValueData(context);
    ret = getObjectPropValueData.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_INVALID_PARAMETER_CODE);
    for (int i = 0; i < COUNT_NUM_SMALL; i++) {
        buffer.push_back(0);
    }
    ret = getObjectPropValueData.Parser(buffer, 24);
    EXPECT_EQ(ret, MTP_SUCCESS);
    mtpStorageManager->RemoveStorage(storage);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_maker_test_001, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetObjectPropValueData getObjectPropValueData(context);
    vector<uint8_t> outBuffer;
    auto mtpStorageManager = MtpStorageManager::GetInstance();
    EXPECT_NE(mtpStorageManager, nullptr);
    auto storage = make_shared<Storage>();
    EXPECT_NE(storage, nullptr);
    mtpStorageManager->AddStorage(storage);
    int ret = getObjectPropValueData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_INVALID_OBJECTHANDLE_CODE);
    uint128_t int128Value = {1, 2, 3, 4};
    string strValue = "Maker";
    bool retTest = getObjectPropValueData.SetPropValue(MTP_TYPE_INT8_CODE, 0, int128Value, strValue);
    EXPECT_EQ(retTest, true);
    ret = getObjectPropValueData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
    mtpStorageManager->RemoveStorage(storage);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_maker_test_002, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetObjectPropValueData getObjectPropValueData(context);
    uint128_t int128Value = {1, 2, 3, 4};
    string strValue = "Maker";
    bool retTest = getObjectPropValueData.SetPropValue(MTP_TYPE_INT16_CODE, 0, int128Value, strValue);
    EXPECT_EQ(retTest, true);
    vector<uint8_t> outBuffer;
    auto mtpStorageManager = MtpStorageManager::GetInstance();
    EXPECT_NE(mtpStorageManager, nullptr);
    auto storage = make_shared<Storage>();
    EXPECT_NE(storage, nullptr);
    mtpStorageManager->AddStorage(storage);
    int ret = getObjectPropValueData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
    mtpStorageManager->RemoveStorage(storage);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_maker_test_003, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetObjectPropValueData getObjectPropValueData(context);
    uint128_t int128Value = {1, 2, 3, 4};
    string strValue = "Maker";
    bool retTest = getObjectPropValueData.SetPropValue(MTP_TYPE_INT32_CODE, 0, int128Value, strValue);
    EXPECT_EQ(retTest, true);
    vector<uint8_t> outBuffer;
    auto mtpStorageManager = MtpStorageManager::GetInstance();
    EXPECT_NE(mtpStorageManager, nullptr);
    auto storage = make_shared<Storage>();
    EXPECT_NE(storage, nullptr);
    mtpStorageManager->AddStorage(storage);
    int ret = getObjectPropValueData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
    mtpStorageManager->RemoveStorage(storage);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_maker_test_004, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetObjectPropValueData getObjectPropValueData(context);
    uint128_t int128Value = {1, 2, 3, 4};
    string strValue = "Maker";
    bool retTest = getObjectPropValueData.SetPropValue(MTP_TYPE_INT64_CODE, 0, int128Value, strValue);
    EXPECT_EQ(retTest, true);
    vector<uint8_t> outBuffer;
    auto mtpStorageManager = MtpStorageManager::GetInstance();
    EXPECT_NE(mtpStorageManager, nullptr);
    auto storage = make_shared<Storage>();
    EXPECT_NE(storage, nullptr);
    mtpStorageManager->AddStorage(storage);
    int ret = getObjectPropValueData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
    mtpStorageManager->RemoveStorage(storage);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_maker_test_005, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetObjectPropValueData getObjectPropValueData(context);
    uint128_t int128Value = {1, 2, 3, 4};
    string strValue = "Maker";
    bool retTest = getObjectPropValueData.SetPropValue(MTP_TYPE_INT128_CODE, 0, int128Value, strValue);
    EXPECT_EQ(retTest, true);
    vector<uint8_t> outBuffer;
    auto mtpStorageManager = MtpStorageManager::GetInstance();
    EXPECT_NE(mtpStorageManager, nullptr);
    auto storage = make_shared<Storage>();
    EXPECT_NE(storage, nullptr);
    mtpStorageManager->AddStorage(storage);
    int ret = getObjectPropValueData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
    mtpStorageManager->RemoveStorage(storage);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_maker_test_006, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetObjectPropValueData getObjectPropValueData(context);
    uint128_t int128Value = {1, 2, 3, 4};
    string strValue = "Maker";
    bool retTest = getObjectPropValueData.SetPropValue(MTP_TYPE_STRING_CODE, 0, int128Value, strValue);
    EXPECT_EQ(retTest, true);
    vector<uint8_t> outBuffer;
    auto mtpStorageManager = MtpStorageManager::GetInstance();
    EXPECT_NE(mtpStorageManager, nullptr);
    auto storage = make_shared<Storage>();
    EXPECT_NE(storage, nullptr);
    mtpStorageManager->AddStorage(storage);
    int ret = getObjectPropValueData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
    mtpStorageManager->RemoveStorage(storage);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_maker_test_007, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetObjectPropValueData getObjectPropValueData(context);
    uint128_t int128Value = {1, 2, 3, 4};
    string strValue = "Maker";
    bool retTest = getObjectPropValueData.SetPropValue(MTP_TYPE_UNDEFINED_CODE, 0, int128Value, strValue);
    EXPECT_EQ(retTest, true);
    vector<uint8_t> outBuffer;
    auto mtpStorageManager = MtpStorageManager::GetInstance();
    EXPECT_NE(mtpStorageManager, nullptr);
    auto storage = make_shared<Storage>();
    EXPECT_NE(storage, nullptr);
    mtpStorageManager->AddStorage(storage);
    int ret = getObjectPropValueData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_INVALID_OBJECTPROP_FORMAT_CODE);
    mtpStorageManager->RemoveStorage(storage);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_calculateSize_test_001, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetObjectPropValueData getObjectPropValueData(context);
    uint128_t int128Value = {1, 2, 3, 4};
    string strValue = "CalculateSize";
    bool retTest = getObjectPropValueData.SetPropValue(MTP_TYPE_UNDEFINED_CODE, 0, int128Value, strValue);
    EXPECT_EQ(retTest, true);
    auto mtpStorageManager = MtpStorageManager::GetInstance();
    EXPECT_NE(mtpStorageManager, nullptr);
    auto storage = make_shared<Storage>();
    EXPECT_NE(storage, nullptr);
    mtpStorageManager->AddStorage(storage);
    int ret = getObjectPropValueData.CalculateSize();
    EXPECT_EQ(ret, MTP_INVALID_OBJECTPROP_FORMAT_CODE);
    mtpStorageManager->RemoveStorage(storage);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_calculateSize_test_002, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetObjectPropValueData getObjectPropValueData(context);
    uint128_t int128Value = {1, 2, 3, 4};
    string strValue = "CalculateSize";
    bool retTest = getObjectPropValueData.SetPropValue(MTP_TYPE_INT8_CODE, 0, int128Value, strValue);
    EXPECT_EQ(retTest, true);
    retTest = getObjectPropValueData.SetPropValue(MTP_TYPE_INT8_CODE, 0, int128Value, strValue);
    EXPECT_EQ(retTest, false);
    int ret = getObjectPropValueData.CalculateSize();
    EXPECT_NE(ret, MTP_INVALID_OBJECTPROP_FORMAT_CODE);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parser_test_002, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetNumObjectsData getNumObjectsData(context);
    vector<uint8_t> buffer;
    int ret = getNumObjectsData.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_INVALID_PARAMETER_CODE);
    GetNumObjectsData getNumObjectsDataOne;
    ret = getNumObjectsDataOne.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_ERROR_CONTEXT_IS_NULL);
    auto mtpStorageManager = MtpStorageManager::GetInstance();
    EXPECT_NE(mtpStorageManager, nullptr);
    auto storage = make_shared<Storage>();
    EXPECT_NE(storage, nullptr);
    mtpStorageManager->AddStorage(storage);
    for (int i = 0; i < COUNT_NUM_SMALL; i++) {
        buffer.push_back(0);
    }
    ret = getNumObjectsData.Parser(buffer, 24);
    EXPECT_EQ(ret, MTP_SUCCESS);
    mtpStorageManager->RemoveStorage(storage);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parser_test_003, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetNumObjectsData getNumObjectsData(context);
    vector<uint8_t> buffer;
    for (int i = 0; i < COUNT_NUM_SMALL; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    int ret = getNumObjectsData.Parser(buffer, 24);
    EXPECT_EQ(ret, MTP_ERROR_INVALID_STORAGE_ID);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_maker_test_008, TestSize.Level1)
{
    GetNumObjectsData getNumObjectsData;
    vector<uint8_t> outBuffer;
    int ret = getNumObjectsData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_INVALID_OBJECTHANDLE_CODE);
    bool retTest = getNumObjectsData.SetNum(0);
    EXPECT_EQ(retTest, true);
    ret = getNumObjectsData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_OK_CODE);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_calculateSize_test_003, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetNumObjectsData getNumObjectsData(context);
    uint32_t ret = getNumObjectsData.CalculateSize();
    EXPECT_EQ(ret, MTP_INVALID_OBJECTHANDLE_CODE);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_getNum_test_001, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetNumObjectsData getNumObjectsData(context);
    int ret = getNumObjectsData.GetNum();
    EXPECT_EQ(ret, -1);
    bool retTest = getNumObjectsData.SetNum(0);
    EXPECT_EQ(retTest, true);
    ret = getNumObjectsData.GetNum();
    EXPECT_EQ(ret, 0);
    retTest = getNumObjectsData.SetNum(0);
    EXPECT_EQ(retTest, false);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parser_test_004, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context;
    GetObjectReferencesData getObjectReferencesDataOne(context);
    vector<uint8_t> buffer;
    int ret = getObjectReferencesDataOne.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_ERROR_SESSION_NOT_OPEN);
    context = make_shared<MtpOperationContext>();
    GetObjectReferencesData getObjectReferencesData(context);
    ret = getObjectReferencesData.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_ERROR_PACKET_INCORRECT);
    for (int i = 0; i < COUNT_NUM_SMALL; i++) {
        buffer.push_back(0);
    }
    ret = getObjectReferencesData.Parser(buffer, 24);
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_maker_test_009, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetObjectReferencesData getObjectReferencesData(context);
    vector<uint8_t> outBuffer;
    int ret = getObjectReferencesData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SESSION_NOT_OPEN_CODE);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_maker_test_010, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    context->sessionOpen = true;
    GetObjectReferencesData getObjectReferencesData(context);
    vector<uint8_t> outBuffer;
    auto mtpStorageManager = MtpStorageManager::GetInstance();
    EXPECT_NE(mtpStorageManager, nullptr);
    auto storage = make_shared<Storage>();
    EXPECT_NE(storage, nullptr);
    mtpStorageManager->AddStorage(storage);
    int ret = getObjectReferencesData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
    mtpStorageManager->RemoveStorage(storage);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_calculateSize_test_004, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetObjectReferencesData getObjectReferencesData(context);
    int ret = getObjectReferencesData.CalculateSize();
    EXPECT_EQ(ret, MTP_SESSION_NOT_OPEN_CODE);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_calculateSize_test_005, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    context->sessionOpen = true;
    GetObjectReferencesData getObjectReferencesData(context);
    int ret = getObjectReferencesData.CalculateSize();
    EXPECT_GT(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_setObjectHandles_test_001, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    shared_ptr<UInt32List> objectHandles = make_shared<UInt32List>();
    GetObjectReferencesData getObjectReferencesData(context);
    auto retTest = getObjectReferencesData.GetObjectHandles();
    EXPECT_EQ(retTest, nullptr);
    bool ret = getObjectReferencesData.SetObjectHandles(objectHandles);
    EXPECT_EQ(ret, true);
    ret = getObjectReferencesData.SetObjectHandles(objectHandles);
    EXPECT_EQ(ret, false);
    retTest = getObjectReferencesData.GetObjectHandles();
    EXPECT_NE(retTest, nullptr);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parser_test_005, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    SetObjectReferencesData setObjectReferencesData(context);
    vector<uint8_t> buffer;
    int ret = setObjectReferencesData.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_SESSION_NOT_OPEN_CODE);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parser_test_006, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    context->sessionOpen = true;
    SetObjectReferencesData setObjectReferencesData(context);
    vector<uint8_t> buffer;
    auto mtpStorageManager = MtpStorageManager::GetInstance();
    EXPECT_NE(mtpStorageManager, nullptr);
    auto storage = make_shared<Storage>();
    EXPECT_NE(storage, nullptr);
    mtpStorageManager->AddStorage(storage);
    int ret = setObjectReferencesData.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_INVALID_PARAMETER_CODE);
    for (int i = 0; i < COUNT_NUM_BIG; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    ret = setObjectReferencesData.Parser(buffer, 24);
    EXPECT_EQ(ret, MTP_SUCCESS);
    mtpStorageManager->RemoveStorage(storage);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_maker_test_011, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    SetObjectReferencesData setObjectReferencesData(context);
    vector<uint8_t> outBuffer;
    auto mtpStorageManager = MtpStorageManager::GetInstance();
    EXPECT_NE(mtpStorageManager, nullptr);
    auto storage = make_shared<Storage>();
    EXPECT_NE(storage, nullptr);
    mtpStorageManager->AddStorage(storage);
    int ret = setObjectReferencesData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_INVALID_OBJECTHANDLE_CODE);
    mtpStorageManager->RemoveStorage(storage);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_maker_test_012, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    SetObjectReferencesData setObjectReferencesData(context);
    bool retTest = setObjectReferencesData.SetResult(0);
    EXPECT_EQ(retTest, true);
    vector<uint8_t> outBuffer;
    auto mtpStorageManager = MtpStorageManager::GetInstance();
    EXPECT_NE(mtpStorageManager, nullptr);
    auto storage = make_shared<Storage>();
    EXPECT_NE(storage, nullptr);
    mtpStorageManager->AddStorage(storage);
    int ret = setObjectReferencesData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
    retTest = setObjectReferencesData.SetResult(0);
    EXPECT_EQ(retTest, false);
    mtpStorageManager->RemoveStorage(storage);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parser_test_007, TestSize.Level1)
{
    vector<uint8_t> buffer;
    CopyObjectData copyObjectDataOne;
    int ret = copyObjectDataOne.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_ERROR_CONTEXT_IS_NULL);
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    CopyObjectData copyObjectData(context);
    auto mtpStorageManager = MtpStorageManager::GetInstance();
    EXPECT_NE(mtpStorageManager, nullptr);
    auto storage = make_shared<Storage>();
    EXPECT_NE(storage, nullptr);
    mtpStorageManager->AddStorage(storage);
    ret = copyObjectData.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_ERROR_PACKET_INCORRECT);
    for (int i = 0; i < COUNT_NUM_BIG; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    ret = copyObjectData.Parser(buffer, 24);
    EXPECT_EQ(ret, MTP_SUCCESS);
    mtpStorageManager->RemoveStorage(storage);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_maker_test_013, TestSize.Level1)
{
    CopyObjectData copyObjectData;
    vector<uint8_t> outBuffer;
    int ret = copyObjectData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_ERROR_INVALID_OBJECTHANDLE);
    bool retTest = copyObjectData.SetObjectHandle(0);
    EXPECT_EQ(retTest, true);
    ret = copyObjectData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
    retTest = copyObjectData.SetObjectHandle(0);
    EXPECT_EQ(retTest, false);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_calculateSize_test_008, TestSize.Level1)
{
    CopyObjectData copyObjectData;
    int ret = copyObjectData.CalculateSize();
    EXPECT_EQ(ret, MTP_ERROR_INVALID_OBJECTHANDLE);
    copyObjectData.SetObjectHandle(0);
    ret = copyObjectData.CalculateSize();
    EXPECT_GT(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parser_test_008, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetDevicePropValueData getDevicePropValueData(context);
    vector<uint8_t> buffer;
    for (int i = 0; i < COUNT_NUM_BIG; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    int ret = getDevicePropValueData.Parser(buffer, 24);
    EXPECT_EQ(ret, MTP_SUCCESS);
    GetDevicePropValueData getDevicePropValueDataOne;
    ret = getDevicePropValueDataOne.Parser(buffer, 24);
    EXPECT_EQ(ret, MTP_ERROR_CONTEXT_IS_NULL);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_maker_test_014, TestSize.Level1)
{
    GetDevicePropValueData getDevicePropValueData;
    vector<uint8_t> outBuffer;
    int ret = getDevicePropValueData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_ERROR_DEVICEPROP_NOT_SUPPORTED);
    uint16_t type = MTP_TYPE_INT8_CODE;
    shared_ptr<Property::Value> value = make_shared<Property::Value>();
    getDevicePropValueData.SetValue(type, value);
    ret = getDevicePropValueData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_calculateSize_test_009, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetDevicePropValueData getDevicePropValueData(context);
    uint32_t ret = getDevicePropValueData.CalculateSize();
    EXPECT_EQ(ret, MTP_ERROR_DEVICEPROP_NOT_SUPPORTED);
    uint16_t type = MTP_TYPE_INT8_CODE;
    shared_ptr<Property::Value> value = make_shared<Property::Value>();
    getDevicePropValueData.SetValue(type, value);
    ret = getDevicePropValueData.CalculateSize();
    EXPECT_NE(ret, MTP_ERROR_DEVICEPROP_NOT_SUPPORTED);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_writeValue_test_001, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetDevicePropValueData getDevicePropValueData(context);
    vector<uint8_t> buffer;
    Property::Value value;
    int ret = getDevicePropValueData.WriteValue(buffer, MTP_TYPE_INT8_CODE, value);
    EXPECT_EQ(ret, MTP_SUCCESS);
    ret = getDevicePropValueData.WriteValue(buffer, MTP_TYPE_AINT8_CODE, value);
    EXPECT_EQ(ret, MTP_SUCCESS);
    ret = getDevicePropValueData.WriteValue(buffer, MTP_TYPE_UINT8_CODE, value);
    EXPECT_EQ(ret, MTP_SUCCESS);
    ret = getDevicePropValueData.WriteValue(buffer, MTP_TYPE_AUINT8_CODE, value);
    EXPECT_EQ(ret, MTP_SUCCESS);
    ret = getDevicePropValueData.WriteValue(buffer, MTP_TYPE_INT16_CODE, value);
    EXPECT_EQ(ret, MTP_SUCCESS);
    ret = getDevicePropValueData.WriteValue(buffer, MTP_TYPE_AINT16_CODE, value);
    EXPECT_EQ(ret, MTP_SUCCESS);
    ret = getDevicePropValueData.WriteValue(buffer, MTP_TYPE_UINT16_CODE, value);
    EXPECT_EQ(ret, MTP_SUCCESS);
    ret = getDevicePropValueData.WriteValue(buffer, MTP_TYPE_AUINT16_CODE, value);
    EXPECT_EQ(ret, MTP_SUCCESS);
    ret = getDevicePropValueData.WriteValue(buffer, MTP_TYPE_INT32_CODE, value);
    EXPECT_EQ(ret, MTP_SUCCESS);
    ret = getDevicePropValueData.WriteValue(buffer, MTP_TYPE_AINT32_CODE, value);
    EXPECT_EQ(ret, MTP_SUCCESS);
    ret = getDevicePropValueData.WriteValue(buffer, MTP_TYPE_UINT32_CODE, value);
    EXPECT_EQ(ret, MTP_SUCCESS);
    ret = getDevicePropValueData.WriteValue(buffer, MTP_TYPE_AUINT32_CODE, value);
    EXPECT_EQ(ret, MTP_SUCCESS);
    ret = getDevicePropValueData.WriteValue(buffer, MTP_TYPE_UINT128_CODE, value);
    EXPECT_EQ(ret, MTP_ERROR_DEVICEPROP_NOT_SUPPORTED);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parser_test_009, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    RespCommonData respCommonData(context);
    vector<uint8_t> buffer;
    int ret = respCommonData.Parser(buffer, 24);
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_maker_test_015, TestSize.Level1)
{
    RespCommonData respCommonData;
    vector<uint8_t> outBuffer;
    int ret = respCommonData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
    uint32_t value = 1;
    respCommonData.SetParam(PARAM_INDEX_1, value);
    ret = respCommonData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
    respCommonData.SetParam(PARAM_INDEX_2, value);
    ret = respCommonData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
    respCommonData.SetParam(PARAM_INDEX_3, value);
    ret = respCommonData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
    respCommonData.SetParam(PARAM_INDEX_4, value);
    ret = respCommonData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
    respCommonData.SetParam(PARAM_INDEX_5, value);
    ret = respCommonData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
    respCommonData.SetParam(6, value);
    ret = respCommonData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_calculateSize_test_010, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    RespCommonData respCommonData(context);
    int ret = respCommonData.CalculateSize();
    EXPECT_EQ(ret, 0);
    uint32_t value = 1;
    respCommonData.SetParam(PARAM_INDEX_1, value);
    ret = respCommonData.CalculateSize();
    EXPECT_EQ(ret, 4);
    respCommonData.SetParam(PARAM_INDEX_2, value);
    ret = respCommonData.CalculateSize();
    EXPECT_EQ(ret, 8);
    respCommonData.SetParam(PARAM_INDEX_3, value);
    ret = respCommonData.CalculateSize();
    EXPECT_EQ(ret, 12);
    respCommonData.SetParam(PARAM_INDEX_4, value);
    ret = respCommonData.CalculateSize();
    EXPECT_EQ(ret, 16);
    respCommonData.SetParam(PARAM_INDEX_5, value);
    ret = respCommonData.CalculateSize();
    EXPECT_EQ(ret, 20);
    respCommonData.SetParam(6, value);
    ret = respCommonData.CalculateSize();
    EXPECT_EQ(ret, 20);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parser_test_010, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context;
    GetPartialObjectData getPartialObjectDataOne(context);
    vector<uint8_t> buffer;
    int ret = getPartialObjectDataOne.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_INVALID_OBJECTHANDLE_CODE);
    context = make_shared<MtpOperationContext>();
    GetPartialObjectData getPartialObjectData(context);
    auto mtpStorageManager = MtpStorageManager::GetInstance();
    EXPECT_NE(mtpStorageManager, nullptr);
    auto storage = make_shared<Storage>();
    EXPECT_NE(storage, nullptr);
    mtpStorageManager->AddStorage(storage);
    ret = getPartialObjectData.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_INVALID_PARAMETER_CODE);
    for (int i = 0; i < COUNT_NUM_BIG; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    ret = getPartialObjectData.Parser(buffer, 24);
    EXPECT_EQ(ret, MTP_SUCCESS);
    mtpStorageManager->RemoveStorage(storage);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_maker_test_016, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetPartialObjectData getPartialObjectData(context);
    vector<uint8_t> outBuffer;
    int ret = getPartialObjectData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_INVALID_OBJECTHANDLE_CODE);
    bool retTest = getPartialObjectData.SetLength(1);
    EXPECT_EQ(retTest, true);
    ret = getPartialObjectData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
    retTest = getPartialObjectData.SetLength(1);
    EXPECT_EQ(retTest, false);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_calculateSize_test_011, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetPartialObjectData getPartialObjectData(context);
    uint32_t ret = getPartialObjectData.CalculateSize();
    EXPECT_EQ(ret, MTP_INVALID_OBJECTHANDLE_CODE);
    getPartialObjectData.SetLength(1);
    ret = getPartialObjectData.CalculateSize();
    EXPECT_GT(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parser_test_011, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    SetDevicePropValueData setDevicePropValueData(context);
    vector<uint8_t> buffer;
    int ret = setDevicePropValueData.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_ERROR_PACKET_INCORRECT);
    SetDevicePropValueData setDevicePropValueDataOne;
    ret = setDevicePropValueDataOne.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_ERROR_INVALID_OBJECTHANDLE);
    for (int i = 0; i < COUNT_NUM_BIG; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    ret = setDevicePropValueData.Parser(buffer, 24);
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parser_test_012, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    context->indata = true;
    SetDevicePropValueData setDevicePropValueData(context);
    vector<uint8_t> buffer;
    for (int i = 0; i < COUNT_NUM_BIG; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    int ret = setDevicePropValueData.Parser(buffer, 24);
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_maker_test_017, TestSize.Level1)
{
    SetDevicePropValueData setDevicePropValueData;
    vector<uint8_t> outBuffer;
    int ret = setDevicePropValueData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_calculateSize_test_012, TestSize.Level1)
{
    SetDevicePropValueData setDevicePropValueData;
    int ret = setDevicePropValueData.CalculateSize();
    EXPECT_GT(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_paserPropValue_test_001, TestSize.Level1)
{
    SetDevicePropValueData setDevicePropValueData;
    vector<uint8_t> buffer;
    for (int i = 0; i < COUNT_NUM_BIG; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    size_t offset = 10;
    setDevicePropValueData.PaserPropValue(buffer, offset, MTP_DEVICE_PROPERTY_DEVICE_FRIENDLY_NAME_CODE);
    EXPECT_GT(buffer.size(), MTP_SUCCESS);
    setDevicePropValueData.PaserPropValue(buffer, offset, MTP_DEVICE_PROPERTY_SYNCHRONIZATION_PARTNER_CODE);
    setDevicePropValueData.PaserPropValue(buffer, offset, MTP_DEVICE_PROPERTY_SESSION_INITIATOR_VERSION_INFO_CODE);
    setDevicePropValueData.PaserPropValue(buffer, offset, 0xD405);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parser_test_013, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    SendObjectInfoData sendObjectInfoData(context);
    vector<uint8_t> buffer;
    int ret = sendObjectInfoData.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_ERROR_PACKET_INCORRECT);
    SendObjectInfoData sendObjectInfoDataOne;
    ret = sendObjectInfoDataOne.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_ERROR_INVALID_OBJECTHANDLE);
    for (int i = 0; i < COUNT_NUM_BIG; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    ret = sendObjectInfoData.Parser(buffer, 24);
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parser_test_014, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    context->indata = true;
    SendObjectInfoData sendObjectInfoData(context);
    vector<uint8_t> buffer;
    buffer.push_back(COUNT_NUM_SMALL);
    int ret = sendObjectInfoData.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_ERROR_PACKET_INCORRECT);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_maker_test_018, TestSize.Level1)
{
    SendObjectInfoData sendObjectInfoData;
    vector<uint8_t> outBuffer;
    int ret = sendObjectInfoData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_ERROR_INVALID_OBJECTHANDLE);
    bool retTest = sendObjectInfoData.SetSetParam(0, 0, 0);
    EXPECT_EQ(retTest, true);
    ret = sendObjectInfoData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
    retTest = sendObjectInfoData.SetSetParam(0, 0, 0);
    EXPECT_EQ(retTest, false);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_calculateSize_test_013, TestSize.Level1)
{
    SendObjectInfoData sendObjectInfoData;
    int ret = sendObjectInfoData.CalculateSize();
    EXPECT_EQ(ret, MTP_ERROR_INVALID_OBJECTHANDLE);
    sendObjectInfoData.SetSetParam(0, 0, 0);
    ret = sendObjectInfoData.CalculateSize();
    EXPECT_EQ(ret, 12);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parserData_test_001, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    SendObjectInfoData sendObjectInfoData(context);
    vector<uint8_t> buffer;
    size_t offset = 1;
    buffer.push_back(COUNT_NUM_SMALL);
    int ret = sendObjectInfoData.ParserData(buffer, offset);
    EXPECT_EQ(ret, MTP_ERROR_PACKET_INCORRECT);
    for (size_t i = 0; i < COUNT_NUM_BIG; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    ret = sendObjectInfoData.ParserData(buffer, offset);
    EXPECT_EQ(ret, MTP_ERROR_PACKET_INCORRECT);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parserData_test_002, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    SendObjectInfoData sendObjectInfoData(context);
    vector<uint8_t> buffer;
    size_t offset = 0;
    for (size_t i = 0; i < DATA_COUNT_8; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    int ret = sendObjectInfoData.ParserData(buffer, offset);
    EXPECT_EQ(ret, MTP_ERROR_PACKET_INCORRECT);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parserData_test_003, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    SendObjectInfoData sendObjectInfoData(context);
    vector<uint8_t> buffer;
    size_t offset = 0;
    for (int i = 0; i < DATA_COUNT_1; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    int ret = sendObjectInfoData.ParserData(buffer, offset);
    EXPECT_EQ(ret, MTP_ERROR_PACKET_INCORRECT);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parserData_test_004, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    SendObjectInfoData sendObjectInfoData(context);
    vector<uint8_t> buffer;
    size_t offset = 0;
    for (int i = 0; i < DATA_COUNT_2; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    int ret = sendObjectInfoData.ParserData(buffer, offset);
    EXPECT_EQ(ret, MTP_ERROR_PACKET_INCORRECT);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parserData_test_005, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    SendObjectInfoData sendObjectInfoData(context);
    vector<uint8_t> buffer;
    size_t offset = 0;
    for (int i = 0; i < DATA_COUNT_3; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    int ret = sendObjectInfoData.ParserData(buffer, offset);
    EXPECT_EQ(ret, MTP_ERROR_PACKET_INCORRECT);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parserData_test_006, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    SendObjectInfoData sendObjectInfoData(context);
    vector<uint8_t> buffer;
    size_t offset = 0;
    for (int i = 0; i < DATA_COUNT_4; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    int ret = sendObjectInfoData.ParserData(buffer, offset);
    EXPECT_EQ(ret, MTP_ERROR_PACKET_INCORRECT);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parserData_test_007, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    SendObjectInfoData sendObjectInfoData(context);
    vector<uint8_t> buffer;
    size_t offset = 0;
    for (int i = 0; i < DATA_COUNT_5; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    int ret = sendObjectInfoData.ParserData(buffer, offset);
    EXPECT_EQ(ret, MTP_ERROR_PACKET_INCORRECT);
}
HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parserData_test_008, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    SendObjectInfoData sendObjectInfoData(context);
    vector<uint8_t> buffer;
    size_t offset = 0;
    for (int i = 0; i < DATA_COUNT_6; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    int ret = sendObjectInfoData.ParserData(buffer, offset);
    EXPECT_EQ(ret, MTP_ERROR_PACKET_INCORRECT);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parserData_test_009, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    SendObjectInfoData sendObjectInfoData(context);
    vector<uint8_t> buffer;
    size_t offset = 0;
    for (int i = 0; i < DATA_COUNT_7; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    int ret = sendObjectInfoData.ParserData(buffer, offset);
    EXPECT_EQ(ret, MTP_ERROR_PACKET_INCORRECT);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parserDataForImageInfo_test_001, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    SendObjectInfoData sendObjectInfoData(context);
    vector<uint8_t> buffer;
    size_t offset = 1;
    buffer.push_back(COUNT_NUM_SMALL);
    int ret = sendObjectInfoData.ParserDataForImageInfo(buffer, offset);
    EXPECT_EQ(ret, MTP_ERROR_PACKET_INCORRECT);
    for (size_t i = 0; i < COUNT_NUM_BIG; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    ret = sendObjectInfoData.ParserDataForImageInfo(buffer, offset);
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parserDataForImageInfo_test_002, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    SendObjectInfoData sendObjectInfoData(context);
    vector<uint8_t> buffer;
    size_t offset = 0;
    for (size_t i = 0; i < IMAGEINFO_COUNT_1; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    int ret = sendObjectInfoData.ParserDataForImageInfo(buffer, offset);
    EXPECT_EQ(ret, MTP_ERROR_PACKET_INCORRECT);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parserDataForImageInfo_test_003, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    SendObjectInfoData sendObjectInfoData(context);
    vector<uint8_t> buffer;
    size_t offset = 0;
    for (size_t i = 0; i < IMAGEINFO_COUNT_2; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    int ret = sendObjectInfoData.ParserDataForImageInfo(buffer, offset);
    EXPECT_EQ(ret, MTP_ERROR_PACKET_INCORRECT);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parserDataForImageInfo_test_004, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    SendObjectInfoData sendObjectInfoData(context);
    vector<uint8_t> buffer;
    size_t offset = 0;
    for (size_t i = 0; i < IMAGEINFO_COUNT_3; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    int ret = sendObjectInfoData.ParserDataForImageInfo(buffer, offset);
    EXPECT_EQ(ret, MTP_ERROR_PACKET_INCORRECT);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parserDataForImageInfo_test_005, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    SendObjectInfoData sendObjectInfoData(context);
    vector<uint8_t> buffer;
    size_t offset = 0;
    for (size_t i = 0; i < IMAGEINFO_COUNT_4; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    int ret = sendObjectInfoData.ParserDataForImageInfo(buffer, offset);
    EXPECT_EQ(ret, MTP_ERROR_PACKET_INCORRECT);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parserDataForImageInfo_test_006, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    SendObjectInfoData sendObjectInfoData(context);
    vector<uint8_t> buffer;
    size_t offset = 0;
    for (size_t i = 0; i < IMAGEINFO_COUNT_5; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    int ret = sendObjectInfoData.ParserDataForImageInfo(buffer, offset);
    EXPECT_EQ(ret, MTP_ERROR_PACKET_INCORRECT);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parserDataForFileInfo_test_001, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    SendObjectInfoData sendObjectInfoData(context);
    vector<uint8_t> buffer;
    size_t offset = 1;
    buffer.push_back(COUNT_NUM_SMALL);
    int ret = sendObjectInfoData.ParserDataForFileInfo(buffer, offset);
    EXPECT_EQ(ret, MTP_ERROR_PACKET_INCORRECT);
    for (size_t i = 0; i < COUNT_NUM_BIG; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    ret = sendObjectInfoData.ParserDataForFileInfo(buffer, offset);
    EXPECT_EQ(ret, MTP_ERROR_PACKET_INCORRECT);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parserDataForFileInfo_test_002, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    SendObjectInfoData sendObjectInfoData(context);
    std::vector<uint8_t> buffer = {
        0x08,
        0x74, 0x00, 0x65, 0x00, 0x73, 0x00, 0x74, 0x00,
        0x2E, 0x00, 0x6A, 0x00, 0x70, 0x00, 0x67, 0x00
    };
    size_t offset = 0;
    int ret = sendObjectInfoData.ParserDataForFileInfo(buffer, offset);
    EXPECT_EQ(ret, MTP_ERROR_PACKET_INCORRECT);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parserDataForFileInfo_test_003, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    SendObjectInfoData sendObjectInfoData(context);
    std::vector<uint8_t> buffer = {
        0x01, 0x61, 0x00,
        0x01, 0x31, 0x00,
        0x01, 0x32, 0x00
    };
    size_t offset = 0;
    int ret = sendObjectInfoData.ParserDataForFileInfo(buffer, offset);
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parser_test_015, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    CloseSessionData closeSessionData(context);
    vector<uint8_t> buffer;
    int ret = closeSessionData.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_SESSION_NOT_OPEN_CODE);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parser_test_016, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    context->sessionOpen = true;
    CloseSessionData closeSessionData(context);
    vector<uint8_t> buffer;
    int ret = closeSessionData.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_OK_CODE);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_maker_test_019, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    CloseSessionData closeSessionData(context);
    vector<uint8_t> outBuffer;
    int ret = closeSessionData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_OK_CODE);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_calculateSize_test_014, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    CloseSessionData closeSessionData(context);
    int ret = closeSessionData.CalculateSize();
    EXPECT_GT(ret, 0);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parser_test_017, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context;
    GetObjectPropListData getObjectPropListDataOne(context);
    vector<uint8_t> buffer;
    int ret = getObjectPropListDataOne.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_FAIL);
    context = make_shared<MtpOperationContext>();
    GetObjectPropListData getObjectPropListData(context);
    auto mtpStorageManager = MtpStorageManager::GetInstance();
    EXPECT_NE(mtpStorageManager, nullptr);
    auto storage = make_shared<Storage>();
    EXPECT_NE(storage, nullptr);
    mtpStorageManager->AddStorage(storage);
    ret = getObjectPropListData.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_INVALID_PARAMETER_CODE);
    for (int i = 0; i < COUNT_NUM_BIG; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    ret = getObjectPropListData.Parser(buffer, 40);
    EXPECT_EQ(ret, MTP_SUCCESS);
    mtpStorageManager->RemoveStorage(storage);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_maker_test_020, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetObjectPropListData getObjectPropListData(context);
    vector<uint8_t> outBuffer;
    auto mtpStorageManager = MtpStorageManager::GetInstance();
    EXPECT_NE(mtpStorageManager, nullptr);
    auto storage = make_shared<Storage>();
    EXPECT_NE(storage, nullptr);
    mtpStorageManager->AddStorage(storage);
    int ret = getObjectPropListData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_INVALID_OBJECTHANDLE_CODE);
    shared_ptr<vector<Property>> props = make_shared<vector<Property>>();
    bool retTest = getObjectPropListData.SetProps(props);
    EXPECT_EQ(retTest, true);
    ret = getObjectPropListData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
    retTest = getObjectPropListData.SetProps(props);
    EXPECT_EQ(retTest, false);
    mtpStorageManager->RemoveStorage(storage);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_calculateSize_test_015, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetObjectPropListData getObjectPropListData(context);
    auto mtpStorageManager = MtpStorageManager::GetInstance();
    EXPECT_NE(mtpStorageManager, nullptr);
    auto storage = make_shared<Storage>();
    EXPECT_NE(storage, nullptr);
    mtpStorageManager->AddStorage(storage);
    int ret = getObjectPropListData.CalculateSize();
    EXPECT_EQ(ret, MTP_INVALID_OBJECTHANDLE_CODE);
    shared_ptr<vector<Property>> props = make_shared<vector<Property>>();
    getObjectPropListData.SetProps(props);
    ret = getObjectPropListData.CalculateSize();
    EXPECT_EQ(ret, 4);
    vector<uint8_t> outBuffer;
    Property prop;
    getObjectPropListData.WriteProperty(outBuffer, prop);
    Property propOne;
    propOne.currentValue = make_shared<Property::Value>();
    getObjectPropListData.WriteProperty(outBuffer, propOne);
    Property propTwo;
    propOne.type_ = MTP_TYPE_STRING_CODE;
    getObjectPropListData.WriteProperty(outBuffer, propTwo);
    getObjectPropListData.WritePropertyStrValue(outBuffer, prop);
    propTwo.currentValue = make_shared<Property::Value>();
    propTwo.type_ = MTP_TYPE_STRING_CODE;
    getObjectPropListData.WritePropertyStrValue(outBuffer, propTwo);
    propTwo.currentValue->str_ = make_shared<string>();
    getObjectPropListData.WritePropertyStrValue(outBuffer, propTwo);
    prop.type_ = MTP_TYPE_INT8_CODE;
    prop.currentValue = make_shared<Property::Value>();
    prop.type_ = MTP_TYPE_INT32_CODE;
    getObjectPropListData.WritePropertyIntValue(outBuffer, prop);
    prop.type_ = MTP_TYPE_UINT32_CODE;
    getObjectPropListData.WritePropertyIntValue(outBuffer, prop);
    prop.type_ = MTP_TYPE_INT64_CODE;
    getObjectPropListData.WritePropertyIntValue(outBuffer, prop);
    prop.type_ = MTP_TYPE_UINT64_CODE;
    getObjectPropListData.WritePropertyIntValue(outBuffer, prop);
    prop.type_ = MTP_TYPE_INT128_CODE;
    getObjectPropListData.WritePropertyIntValue(outBuffer, prop);
    prop.type_ = MTP_TYPE_UINT128_CODE;
    getObjectPropListData.WritePropertyIntValue(outBuffer, prop);
    prop.type_ = MTP_TYPE_AUINT128_CODE;
    getObjectPropListData.WritePropertyIntValue(outBuffer, prop);
    mtpStorageManager->RemoveStorage(storage);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parser_test_018, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context;
    vector<uint8_t> buffer;
    SendObjectData sendObjectDataOne(context);
    int ret = sendObjectDataOne.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_INVALID_OBJECTHANDLE_CODE);
    context = make_shared<MtpOperationContext>();
    SendObjectData sendObjectData(context);
    auto mtpStorageManager = MtpStorageManager::GetInstance();
    EXPECT_NE(mtpStorageManager, nullptr);
    auto storage = make_shared<Storage>();
    EXPECT_NE(storage, nullptr);
    mtpStorageManager->AddStorage(storage);
    ret = sendObjectData.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_SUCCESS);
    mtpStorageManager->RemoveStorage(storage);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_maker_test_021, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    SendObjectData sendObjectData(context);
    vector<uint8_t> outBuffer;
    int ret = sendObjectData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_calculateSize_test_016, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    SendObjectData sendObjectData(context);
    uint32_t ret = sendObjectData.CalculateSize();
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parser_test_019, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context;
    DeleteObjectData deleteObjectDataOne(context);
    vector<uint8_t> buffer;
    int ret = deleteObjectDataOne.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_ERROR_CONTEXT_IS_NULL);
    context = make_shared<MtpOperationContext>();
    DeleteObjectData deleteObjectData(context);
    auto mtpStorageManager = MtpStorageManager::GetInstance();
    EXPECT_NE(mtpStorageManager, nullptr);
    auto storage = make_shared<Storage>();
    EXPECT_NE(storage, nullptr);
    mtpStorageManager->AddStorage(storage);
    ret = deleteObjectData.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_ERROR_PACKET_INCORRECT);
    for (int i = 0; i < COUNT_NUM_BIG; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    ret = deleteObjectData.Parser(buffer, 24);
    EXPECT_EQ(ret, MTP_SUCCESS);
    mtpStorageManager->RemoveStorage(storage);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parser_test_020, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    DeleteObjectData deleteObjectData(context);
    vector<uint8_t> buffer;
    auto mtpStorageManager = MtpStorageManager::GetInstance();
    EXPECT_NE(mtpStorageManager, nullptr);
    auto storage = make_shared<Storage>();
    EXPECT_NE(storage, nullptr);
    mtpStorageManager->AddStorage(storage);
    int ret = deleteObjectData.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_ERROR_PACKET_INCORRECT);
    for (int i = 0; i < COUNT_NUM_BIG; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    ret = deleteObjectData.Parser(buffer, 24);
    EXPECT_EQ(ret, MTP_SUCCESS);
    mtpStorageManager->RemoveStorage(storage);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_maker_test_022, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    DeleteObjectData deleteObjectData(context);
    vector<uint8_t> outBuffer;
    int ret = deleteObjectData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_calculateSize_test_017, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    DeleteObjectData deleteObjectData(context);
    int ret = deleteObjectData.CalculateSize();
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parser_test_021, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetDeviceInfoData getDeviceInfoData(context);
    vector<uint8_t> buffer;
    int ret = getDeviceInfoData.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_maker_test_023, TestSize.Level1)
{
    GetDeviceInfoData getDeviceInfoData;
    vector<uint8_t> outBuffer;
    int ret = getDeviceInfoData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_calculateSize_test_018, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetDeviceInfoData getDeviceInfoData(context);
    int ret = getDeviceInfoData.CalculateSize();
    EXPECT_GT(ret, MTP_SUCCESS);
    string manufacturer = "GetDeviceInfoData";
    getDeviceInfoData.SetManufacturer(manufacturer);
    getDeviceInfoData.SetModel(manufacturer);
    getDeviceInfoData.SetVersion(manufacturer);
    getDeviceInfoData.SetSerialNum(manufacturer);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parser_test_022, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetDevicePropDescData getDevicePropDescData(context);
    vector<uint8_t> buffer;
    for (int i = 0; i < COUNT_NUM_BIG; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    int ret = getDevicePropDescData.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_SUCCESS);
    GetDevicePropDescData getDevicePropDescDataOne;
    ret = getDevicePropDescDataOne.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_ERROR_CONTEXT_IS_NULL);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_maker_test_024, TestSize.Level1)
{
    GetDevicePropDescData getDevicePropDescData;
    vector<uint8_t> outBuffer;
    int ret = getDevicePropDescData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_ERROR_DEVICEPROP_NOT_SUPPORTED);
    shared_ptr<Property> property = make_shared<Property>();
    getDevicePropDescData.SetProperty(property);
    ret = getDevicePropDescData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_calculateSize_test_019, TestSize.Level1)
{
    GetDevicePropDescData getDevicePropDescData;
    int ret = getDevicePropDescData.CalculateSize();
    EXPECT_EQ(ret, MTP_ERROR_DEVICEPROP_NOT_SUPPORTED);
    shared_ptr<Property> property = make_shared<Property>();
    getDevicePropDescData.SetProperty(property);
    ret = getDevicePropDescData.CalculateSize();
    EXPECT_GT(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parser_test_023, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetObjectData getObjectData(context);
    vector<uint8_t> buffer;
    GetObjectData getObjectDataOne;
    int ret = getObjectDataOne.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_INVALID_OBJECTHANDLE_CODE);
    auto mtpStorageManager = MtpStorageManager::GetInstance();
    EXPECT_NE(mtpStorageManager, nullptr);
    auto storage = make_shared<Storage>();
    EXPECT_NE(storage, nullptr);
    mtpStorageManager->AddStorage(storage);
    ret = getObjectData.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_INVALID_PARAMETER_CODE);
    for (int i = 0; i < COUNT_NUM_BIG; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    ret = getObjectData.Parser(buffer, 24);
    EXPECT_EQ(ret, MTP_SUCCESS);
    mtpStorageManager->RemoveStorage(storage);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_maker_test_025, TestSize.Level1)
{
    GetObjectData getObjectData;
    vector<uint8_t> outBuffer;
    int ret = getObjectData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_calculateSize_test_020, TestSize.Level1)
{
    GetObjectData getObjectData;
    int ret = getObjectData.CalculateSize();
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_setResult_test_019, TestSize.Level1)
{
    GetObjectData getObjectData;
    bool ret = getObjectData.SetResult(0);
    EXPECT_EQ(ret, true);
    ret = getObjectData.SetResult(0);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parser_test_024, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetObjectHandlesData getObjectHandlesData(context);
    vector<uint8_t> buffer;
    int ret = getObjectHandlesData.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_ERROR_PACKET_INCORRECT);
    GetObjectHandlesData getObjectHandlesDataOne;
    ret = getObjectHandlesDataOne.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_ERROR_CONTEXT_IS_NULL);
    for (int i = 0; i < COUNT_NUM_BIG; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    ret = getObjectHandlesData.Parser(buffer, 24);
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_maker_test_026, TestSize.Level1)
{
    GetObjectHandlesData getObjectHandlesData;
    vector<uint8_t> outBuffer;
    int ret = getObjectHandlesData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_ERROR_INVALID_OBJECTHANDLE);
    shared_ptr<UInt32List> objectHandles = make_shared<UInt32List>();
    bool retTest = getObjectHandlesData.SetObjectHandles(objectHandles);
    EXPECT_EQ(retTest, true);
    ret = getObjectHandlesData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
    retTest = getObjectHandlesData.SetObjectHandles(objectHandles);
    EXPECT_EQ(retTest, false);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_calculateSize_test_021, TestSize.Level1)
{
    GetObjectHandlesData getObjectHandlesData;
    vector<uint8_t> outBuffer;
    int ret = getObjectHandlesData.CalculateSize();
    EXPECT_EQ(ret, MTP_ERROR_INVALID_OBJECTHANDLE);
    shared_ptr<UInt32List> objectHandles = make_shared<UInt32List>();
    getObjectHandlesData.SetObjectHandles(objectHandles);
    ret = getObjectHandlesData.CalculateSize();
    EXPECT_GT(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_getObjectHandles_test_028, TestSize.Level1)
{
    GetObjectHandlesData getObjectHandlesData;
    vector<uint8_t> outBuffer;
    auto ret = getObjectHandlesData.GetObjectHandles();
    EXPECT_EQ(ret, nullptr);
    shared_ptr<UInt32List> objectHandles = make_shared<UInt32List>();
    getObjectHandlesData.SetObjectHandles(objectHandles);
    ret = getObjectHandlesData.GetObjectHandles();
    EXPECT_NE(ret, nullptr);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parser_test_025, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetObjectInfoData getObjectInfoData(context);
    vector<uint8_t> buffer;
    int ret = getObjectInfoData.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_ERROR_PACKET_INCORRECT);
    GetObjectInfoData getObjectInfoDataOne;
    ret = getObjectInfoDataOne.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_ERROR_CONTEXT_IS_NULL);
    for (int i = 0; i < COUNT_NUM_BIG; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    ret = getObjectInfoData.Parser(buffer, 24);
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_maker_test_027, TestSize.Level1)
{
    GetObjectInfoData getObjectInfoData;
    vector<uint8_t> outBuffer;
    int ret = getObjectInfoData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_INVALID_OBJECTHANDLE_CODE);
    shared_ptr<ObjectInfo> objectInfo = make_shared<ObjectInfo>(0);
    bool retTest = getObjectInfoData.SetObjectInfo(objectInfo);
    EXPECT_EQ(retTest, true);
    ret = getObjectInfoData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
    retTest = getObjectInfoData.SetObjectInfo(objectInfo);
    EXPECT_EQ(retTest, false);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_calculateSize_test_022, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetObjectInfoData getObjectInfoData(context);
    int ret = getObjectInfoData.CalculateSize();
    EXPECT_EQ(ret, MTP_INVALID_OBJECTHANDLE_CODE);
    shared_ptr<ObjectInfo> objectInfo = make_shared<ObjectInfo>(0);
    getObjectInfoData.SetObjectInfo(objectInfo);
    ret = getObjectInfoData.CalculateSize();
    EXPECT_GT(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_getObjectInfo_test_021, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetObjectInfoData getObjectInfoData(context);
    auto ret = getObjectInfoData.GetObjectInfo();
    EXPECT_EQ(ret, nullptr);
    shared_ptr<ObjectInfo> objectInfo = make_shared<ObjectInfo>(0);
    getObjectInfoData.SetObjectInfo(objectInfo);
    ret = getObjectInfoData.GetObjectInfo();
    EXPECT_NE(ret, nullptr);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parser_test_026, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context;
    vector<uint8_t> buffer;
    GetObjectPropDescData getObjectPropDescDataOne(context);
    int ret = getObjectPropDescDataOne.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_FAIL);
    context = make_shared<MtpOperationContext>();
    GetObjectPropDescData getObjectPropDescData(context);
    ret = getObjectPropDescData.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_INVALID_PARAMETER_CODE);
    for (int i = 0; i < COUNT_NUM_BIG; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    ret = getObjectPropDescData.Parser(buffer, 24);
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_maker_test_028, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetObjectPropDescData getObjectPropDescData(context);
    vector<uint8_t> outBuffer;
    int ret = getObjectPropDescData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_INVALID_OBJECTHANDLE_CODE);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_maker_test_029, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    context->property = MTP_PROPERTY_OBJECT_FORMAT_CODE;
    GetObjectPropDescData getObjectPropDescData(context);
    vector<uint8_t> outBuffer;
    int ret = getObjectPropDescData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_calculateSize_test_023, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetObjectPropDescData getObjectPropDescData(context);
    int ret = getObjectPropDescData.CalculateSize();
    EXPECT_EQ(ret, MTP_INVALID_OBJECTHANDLE_CODE);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_calculateSize_test_024, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    context->property = MTP_PROPERTY_OBJECT_FORMAT_CODE;
    GetObjectPropDescData getObjectPropDescData(context);
    int ret = getObjectPropDescData.CalculateSize();
    EXPECT_GT(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_getProp_test_001, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetObjectPropDescData getObjectPropDescData(context);
    auto ret = getObjectPropDescData.GetProp();
    EXPECT_EQ(ret, nullptr);
    context->property = MTP_PROPERTY_OBJECT_FORMAT_CODE;
    GetObjectPropDescData getObjectPropDescDataOne(context);
    ret = getObjectPropDescDataOne.GetProp();
    EXPECT_NE(ret, nullptr);
    context->property = MTP_PROPERTY_BITRATE_TYPE_CODE;
    GetObjectPropDescData getObjectPropDescDataTwo(context);
    ret = getObjectPropDescDataTwo.GetProp();
    EXPECT_NE(ret, nullptr);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_getPropInt_test_001, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetObjectPropDescData getObjectPropDescData(context);
    auto ret = getObjectPropDescData.GetPropInt();
    EXPECT_EQ(ret, nullptr);
    context->property = MTP_PROPERTY_OBJECT_FORMAT_CODE;
    GetObjectPropDescData getObjectPropDescDataOne(context);
    ret = getObjectPropDescDataOne.GetPropInt();
    EXPECT_NE(ret, nullptr);
    context->property = MTP_PROPERTY_PROTECTION_STATUS_CODE;
    GetObjectPropDescData getObjectPropDescDataTwo(context);
    ret = getObjectPropDescDataTwo.GetPropInt();
    EXPECT_NE(ret, nullptr);
    context->property = MTP_PROPERTY_STORAGE_ID_CODE;
    GetObjectPropDescData getObjectPropDescDataThree(context);
    ret = getObjectPropDescDataThree.GetPropInt();
    EXPECT_NE(ret, nullptr);
    context->property = MTP_PROPERTY_OBJECT_SIZE_CODE;
    GetObjectPropDescData getObjectPropDescDataFpur(context);
    ret = getObjectPropDescDataFpur.GetPropInt();
    EXPECT_NE(ret, nullptr);
    context->property = MTP_PROPERTY_PERSISTENT_UID_CODE;
    GetObjectPropDescData getObjectPropDescDataFive(context);
    ret = getObjectPropDescDataFive.GetPropInt();
    EXPECT_NE(ret, nullptr);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_getPropStr_test_001, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetObjectPropDescData getObjectPropDescData(context);
    auto ret = getObjectPropDescData.GetPropStr();
    EXPECT_EQ(ret, nullptr);
    context->property = MTP_PROPERTY_NAME_CODE;
    GetObjectPropDescData getObjectPropDescDataOne(context);
    ret = getObjectPropDescDataOne.GetPropStr();
    EXPECT_NE(ret, nullptr);
    context->property = MTP_PROPERTY_DATE_MODIFIED_CODE;
    GetObjectPropDescData getObjectPropDescDataTwo(context);
    ret = getObjectPropDescDataTwo.GetPropStr();
    EXPECT_NE(ret, nullptr);
    context->property = MTP_PROPERTY_OBJECT_FILE_NAME_CODE;
    GetObjectPropDescData getObjectPropDescDataThree(context);
    ret = getObjectPropDescDataThree.GetPropStr();
    EXPECT_NE(ret, nullptr);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_getPropForm_test_001, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetObjectPropDescData getObjectPropDescData(context);
    auto ret = getObjectPropDescData.GetPropForm();
    EXPECT_EQ(ret, nullptr);
    context->property = MTP_PROPERTY_BITRATE_TYPE_CODE;
    GetObjectPropDescData getObjectPropDescDataOne(context);
    ret = getObjectPropDescDataOne.GetPropForm();
    EXPECT_NE(ret, nullptr);
    context->property = MTP_PROPERTY_AUDIO_BITRATE_CODE;
    GetObjectPropDescData getObjectPropDescDataTwo(context);
    ret = getObjectPropDescDataTwo.GetPropForm();
    EXPECT_NE(ret, nullptr);
    context->property = MTP_PROPERTY_NUMBER_OF_CHANNELS_CODE;
    GetObjectPropDescData getObjectPropDescDataThree(context);
    ret = getObjectPropDescDataThree.GetPropForm();
    EXPECT_NE(ret, nullptr);
    context->property = MTP_PROPERTY_SAMPLE_RATE_CODE;
    GetObjectPropDescData getObjectPropDescDataFour(context);
    ret = getObjectPropDescDataFour.GetPropForm();
    EXPECT_NE(ret, nullptr);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parser_test_027, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetObjectPropsSupportedData getObjectPropsSupportedData(context);
    vector<uint8_t> buffer;
    int ret = getObjectPropsSupportedData.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_SESSION_NOT_OPEN_CODE);
    context->sessionOpen = true;
    GetObjectPropsSupportedData getObjectPropsSupportedDataOne(context);
    ret = getObjectPropsSupportedDataOne.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_INVALID_PARAMETER_CODE);
    for (int i = 0; i < COUNT_NUM_BIG; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    ret = getObjectPropsSupportedData.Parser(buffer, 24);
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_maker_test_030, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetObjectPropsSupportedData getObjectPropsSupportedData(context);
    vector<uint8_t> outBuffer;
    int ret = getObjectPropsSupportedData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_calculateSize_test_025, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    context->format = MTP_FORMAT_EXIF_JPEG_CODE;
    GetObjectPropsSupportedData getObjectPropsSupportedData(context);
    int ret = getObjectPropsSupportedData.CalculateSize();
    EXPECT_GT(ret, MTP_SUCCESS);
    UInt16List properties;
    getObjectPropsSupportedData.GetObjectProps(properties);
    context->format = MTP_FORMAT_MP3_CODE;
    GetObjectPropsSupportedData getObjectPropsSupportedDataOne(context);
    getObjectPropsSupportedDataOne.GetObjectProps(properties);
    context->format = MTP_FORMAT_MPEG_CODE;
    GetObjectPropsSupportedData getObjectPropsSupportedDataTwo(context);
    getObjectPropsSupportedDataTwo.GetObjectProps(properties);
    context->format = MTP_FORMAT_UNDEFINED_FIRMWARE_CODE;
    GetObjectPropsSupportedData getObjectPropsSupportedDataThree(context);
    getObjectPropsSupportedDataThree.GetObjectProps(properties);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parser_test_028, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetStorageIdsData getStorageIdsData(context);
    vector<uint8_t> buffer;
    int ret = getStorageIdsData.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_maker_test_031, TestSize.Level1)
{
    GetStorageIdsData getStorageIdsData;
    vector<std::shared_ptr<Storage>> storages;
    getStorageIdsData.SetStorages(storages);
    vector<uint8_t> outBuffer;
    int ret = getStorageIdsData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_calculateSize_test_026, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetStorageIdsData getStorageIdsData(context);
    int ret = getStorageIdsData.CalculateSize();
    EXPECT_GT(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parser_test_029, TestSize.Level1)
{
    GetStorageInfoData getStorageInfoData;
    vector<uint8_t> buffer;
    int ret = getStorageInfoData.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_ERROR_CONTEXT_IS_NULL);
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetStorageInfoData getStorageInfoDataOne(context);
    ret = getStorageInfoDataOne.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_ERROR_PACKET_INCORRECT);
    for (int i = 0; i < COUNT_NUM_BIG; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    ret = getStorageInfoDataOne.Parser(buffer, 24);
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_maker_test_032, TestSize.Level1)
{
    GetStorageInfoData getStorageInfoData;
    vector<uint8_t> outBuffer;
    int ret = getStorageInfoData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
    shared_ptr<Storage> storage = make_shared<Storage>();
    getStorageInfoData.SetStorage(storage);
    ret = getStorageInfoData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_calculateSize_test_027, TestSize.Level1)
{
    GetStorageInfoData getStorageInfoData;
    int ret = getStorageInfoData.CalculateSize();
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parser_test_030, TestSize.Level1)
{
    GetThumbData getThumbData;
    vector<uint8_t> buffer;
    int ret = getThumbData.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_ERROR_CONTEXT_IS_NULL);
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetThumbData getThumbDataOne(context);
    ret = getThumbDataOne.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_ERROR_PACKET_INCORRECT);
    for (int i = 0; i < COUNT_NUM_BIG; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    ret = getThumbDataOne.Parser(buffer, 24);
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_maker_test_033, TestSize.Level1)
{
    GetThumbData getThumbData;
    vector<uint8_t> outBuffer;
    int ret = getThumbData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_ERROR_INVALID_OBJECTHANDLE);
    shared_ptr<UInt8List> thumb = make_shared<UInt8List>();
    bool retTest = getThumbData.SetThumb(thumb);
    EXPECT_EQ(retTest, true);
    ret = getThumbData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
    retTest = getThumbData.SetThumb(thumb);
    EXPECT_EQ(retTest, false);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_calculateSize_test_028, TestSize.Level1)
{
    GetThumbData getThumbData;
    int ret = getThumbData.CalculateSize();
    EXPECT_EQ(ret, MTP_ERROR_INVALID_OBJECTHANDLE);
    shared_ptr<UInt8List> thumb = make_shared<UInt8List>();
    getThumbData.SetThumb(thumb);
    ret = getThumbData.CalculateSize();
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parser_test_031, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    MoveObjectData moveObjectData(context);
    vector<uint8_t> buffer;
    auto mtpStorageManager = MtpStorageManager::GetInstance();
    EXPECT_NE(mtpStorageManager, nullptr);
    auto storage = make_shared<Storage>();
    EXPECT_NE(storage, nullptr);
    mtpStorageManager->AddStorage(storage);
    int ret = moveObjectData.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_ERROR_PACKET_INCORRECT);
    for (int i = 0; i < COUNT_NUM_BIG; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    ret = moveObjectData.Parser(buffer, 24);
    EXPECT_EQ(ret, MTP_SUCCESS);
    mtpStorageManager->RemoveStorage(storage);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_maker_test_034, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    MoveObjectData moveObjectData(context);
    vector<uint8_t> outBuffer;
    int ret = moveObjectData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_calculateSize_test_029, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    MoveObjectData moveObjectData(context);
    int ret = moveObjectData.CalculateSize();
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parser_test_032, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    ObjectEventData objectEventData(context);
    vector<uint8_t> buffer;
    int ret = objectEventData.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_maker_test_035, TestSize.Level1)
{
    ObjectEventData objectEventData;
    vector<uint8_t> outBuffer;
    int ret = objectEventData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_calculateSize_test_030, TestSize.Level1)
{
    ObjectEventData objectEventData;
    objectEventData.SetPayload(0);
    int ret = objectEventData.CalculateSize();
    EXPECT_GT(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_parser_test_033, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context;
    OpenSessionData OpenSessionDataOne(context);
    context = make_shared<MtpOperationContext>();
    OpenSessionData OpenSessionData(context);
    vector<uint8_t> buffer;
    int ret = OpenSessionData.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_FAIL);
    ret = OpenSessionDataOne.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_FAIL);
    for (int i = 0; i < COUNT_NUM_BIG; i++) {
        buffer.push_back(COUNT_NUM_SMALL);
    }
    ret = OpenSessionData.Parser(buffer, 24);
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_maker_test_036, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    OpenSessionData OpenSessionData(context);
    vector<uint8_t> outBuffer;
    OpenSessionData.SetSessionId(5);
    int ret = OpenSessionData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_calculateSize_test_031, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    OpenSessionData OpenSessionData(context);
    OpenSessionData.SetSessionId(5);
    int ret = OpenSessionData.CalculateSize();
    EXPECT_GT(ret, MTP_SUCCESS);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_mtp_calculateSize_test_032, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    GetObjectPropListData getObjectPropListData(context);
    auto mtpStorageManager = MtpStorageManager::GetInstance();
    EXPECT_NE(mtpStorageManager, nullptr);
    auto storage = make_shared<Storage>();
    EXPECT_NE(storage, nullptr);
    mtpStorageManager->AddStorage(storage);
    int ret = getObjectPropListData.CalculateSize();
    EXPECT_EQ(ret, MTP_INVALID_OBJECTHANDLE_CODE);
    shared_ptr<vector<Property>> props = make_shared<vector<Property>>();
    getObjectPropListData.SetProps(props);
    ret = getObjectPropListData.CalculateSize();
    EXPECT_EQ(ret, 4);
    vector<uint8_t> outBuffer;
    Property prop;
    getObjectPropListData.WriteProperty(outBuffer, prop);
    Property propOne;
    propOne.currentValue = make_shared<Property::Value>();
    getObjectPropListData.WriteProperty(outBuffer, propOne);
    Property propTwo;
    propOne.type_ = MTP_TYPE_STRING_CODE;
    getObjectPropListData.WriteProperty(outBuffer, propTwo);
    getObjectPropListData.WritePropertyStrValue(outBuffer, prop);
    propTwo.currentValue = make_shared<Property::Value>();
    propTwo.type_ = MTP_TYPE_STRING_CODE;
    getObjectPropListData.WritePropertyStrValue(outBuffer, propTwo);
    propTwo.currentValue->str_ = make_shared<string>();
    getObjectPropListData.WritePropertyStrValue(outBuffer, propTwo);
    prop.type_ = MTP_TYPE_INT8_CODE;
    prop.currentValue = make_shared<Property::Value>();
    getObjectPropListData.WritePropertyIntValue(outBuffer, prop);
    prop.type_ = MTP_TYPE_UINT8_CODE;
    getObjectPropListData.WritePropertyIntValue(outBuffer, prop);
    prop.type_ = MTP_TYPE_INT16_CODE;
    getObjectPropListData.WritePropertyIntValue(outBuffer, prop);
    prop.type_ = MTP_TYPE_UINT16_CODE;
    getObjectPropListData.WritePropertyIntValue(outBuffer, prop);
    mtpStorageManager->RemoveStorage(storage);
}
} // namespace Media
} // namespace OHOS