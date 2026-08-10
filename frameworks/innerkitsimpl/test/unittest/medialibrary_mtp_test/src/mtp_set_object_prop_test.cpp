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
#include <iomanip>
#include "medialibrary_errno.h"
#include "mtp_storage_manager.h"
#include "medialibrary_mtp_unit_test.h"
#include "payload_data/set_object_prop_value_data.h"
using namespace std;
using namespace testing::ext;
static constexpr int32_t SIZE_NUM_BIG = 30;

namespace OHOS {
namespace Media {
HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_parser_test_001, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    SetObjectPropValueData setObjectPropValueData(context);
    vector<uint8_t> buffer(SIZE_NUM_BIG, 0b00);
    auto mtpStorageManager = MtpStorageManager::GetInstance();
    auto storage = make_shared<Storage>();
    mtpStorageManager->AddStorage(storage);
    int ret = setObjectPropValueData.Parser(buffer, 0);
    EXPECT_EQ(ret, MTP_ERROR_PACKET_INCORRECT);
    ret = setObjectPropValueData.Parser(buffer, 24);
    EXPECT_EQ(ret, MTP_OBJECTPROP_NOT_SUPPORTED_CODE);
    buffer.at(MTP_CONTAINER_HEADER_SIZE + sizeof(uint32_t)) = static_cast<uint8_t>(0x02);
    buffer.at(MTP_CONTAINER_HEADER_SIZE + sizeof(uint32_t) + sizeof(uint8_t)) = static_cast<uint8_t>(0xDC);
    ret = setObjectPropValueData.Parser(buffer, 24);
    EXPECT_EQ(ret, MTP_SUCCESS);
    mtpStorageManager->RemoveStorage(storage);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_parser_test_002, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    context->indata = true;
    context->properType = MTP_TYPE_INT64_CODE;
    context->properIntValue = 0;
    SetObjectPropValueData setObjectPropValueData(context);
    vector<uint8_t> buffer;
    auto mtpStorageManager = MtpStorageManager::GetInstance();
    auto storage = make_shared<Storage>();
    mtpStorageManager->AddStorage(storage);
    for (int i = 0; i < SIZE_NUM_BIG; i++)
    {
        buffer.push_back(0);
    }
    int ret = setObjectPropValueData.Parser(buffer, 24);
    EXPECT_EQ(ret, MTP_SUCCESS);
    mtpStorageManager->RemoveStorage(storage);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_parser_test_003, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    context->indata = true;
    context->properType = 10;
    context->properIntValue = 0;
    SetObjectPropValueData setObjectPropValueData(context);
    vector<uint8_t> buffer;
    auto mtpStorageManager = MtpStorageManager::GetInstance();
    auto storage = make_shared<Storage>();
    mtpStorageManager->AddStorage(storage);
    buffer.push_back(0);
    int ret = setObjectPropValueData.Parser(buffer, 24);
    EXPECT_EQ(ret, MTP_INVALID_OBJECTPROP_FORMAT_CODE);
    mtpStorageManager->RemoveStorage(storage);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_parser_test_004, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    context->indata = true;
    context->properType = MTP_TYPE_STRING_CODE;
    SetObjectPropValueData setObjectPropValueData(context);
    vector<uint8_t> buffer;
    auto mtpStorageManager = MtpStorageManager::GetInstance();
    auto storage = make_shared<Storage>();
    mtpStorageManager->AddStorage(storage);
    for (int i = 0; i < SIZE_NUM_BIG; i++)
    {
        buffer.push_back(0);
    }
    int ret = setObjectPropValueData.Parser(buffer, 24);
    EXPECT_EQ(ret, MTP_SUCCESS);
    mtpStorageManager->RemoveStorage(storage);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_maker_test_001, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    auto mtpStorageManager = MtpStorageManager::GetInstance();
    vector<uint8_t> outBuffer;
    SetObjectPropValueData setObjectPropValueData(context);
    auto storage = make_shared<Storage>();
    mtpStorageManager->AddStorage(storage);
    int ret = setObjectPropValueData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_INVALID_OBJECTHANDLE_CODE);
    setObjectPropValueData.SetResult(0);
    ret = setObjectPropValueData.Maker(outBuffer);
    EXPECT_EQ(ret, MTP_SUCCESS);
    mtpStorageManager->RemoveStorage(storage);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_calculateSize_test_001, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    auto mtpStorageManager = MtpStorageManager::GetInstance();
    SetObjectPropValueData setObjectPropValueData(context);
    auto storage = make_shared<Storage>();
    mtpStorageManager->AddStorage(storage);
    uint32_t ret = setObjectPropValueData.CalculateSize();
    EXPECT_EQ(ret, MTP_INVALID_OBJECTHANDLE_CODE);
    setObjectPropValueData.SetResult(0);
    ret = setObjectPropValueData.CalculateSize();
    EXPECT_EQ(ret, MTP_SUCCESS);
    mtpStorageManager->RemoveStorage(storage);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_setResult_test_001, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    SetObjectPropValueData setObjectPropValueData(context);
    bool ret = setObjectPropValueData.SetResult(0);
    EXPECT_EQ(ret, true);
    ret = setObjectPropValueData.SetResult(0);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_readInt8Value_test_001, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    vector<uint8_t> buffer;
    size_t offset;
    int type = 0;
    int64_t int64Value = 0;
    bool ret = SetObjectPropValueData::ReadInt8Value(buffer, offset, type, int64Value);
    EXPECT_EQ(ret, false);
    ret = SetObjectPropValueData::ReadInt8Value(buffer, offset, MTP_TYPE_INT8_CODE, int64Value);
    EXPECT_EQ(ret, false);
    ret = SetObjectPropValueData::ReadInt8Value(buffer, offset, MTP_TYPE_UINT8_CODE, int64Value);
    EXPECT_EQ(ret, false);
    buffer.push_back(0);
    buffer.push_back(0);
    ret = SetObjectPropValueData::ReadInt8Value(buffer, offset, MTP_TYPE_INT8_CODE, int64Value);
    EXPECT_EQ(ret, true);
    ret = SetObjectPropValueData::ReadInt8Value(buffer, offset, MTP_TYPE_UINT8_CODE, int64Value);
    EXPECT_EQ(ret, true);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_readInt16Value_test_001, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    vector<uint8_t> buffer;
    size_t offset = 0;
    int type = 0;
    int64_t int64Value = 0;
    bool ret = SetObjectPropValueData::ReadInt16Value(buffer, offset, type, int64Value);
    EXPECT_EQ(ret, false);
    ret = SetObjectPropValueData::ReadInt16Value(buffer, offset, MTP_TYPE_INT16_CODE, int64Value);
    EXPECT_EQ(ret, false);
    ret = SetObjectPropValueData::ReadInt16Value(buffer, offset, MTP_TYPE_UINT16_CODE, int64Value);
    EXPECT_EQ(ret, false);
    for (int i = 0; i < SIZE_NUM_BIG; i++)
    {
        buffer.push_back(0);
    }
    ret = SetObjectPropValueData::ReadInt16Value(buffer, offset, MTP_TYPE_INT16_CODE, int64Value);
    EXPECT_EQ(ret, true);
    ret = SetObjectPropValueData::ReadInt16Value(buffer, offset, MTP_TYPE_UINT16_CODE, int64Value);
    EXPECT_EQ(ret, true);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_readInt32Value_test_001, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    vector<uint8_t> buffer;
    size_t offset = 0;
    int type = 0;
    int64_t int64Value = 0;
    bool ret = SetObjectPropValueData::ReadInt32Value(buffer, offset, type, int64Value);
    EXPECT_EQ(ret, false);
    ret = SetObjectPropValueData::ReadInt32Value(buffer, offset, MTP_TYPE_INT32_CODE, int64Value);
    EXPECT_EQ(ret, false);
    ret = SetObjectPropValueData::ReadInt32Value(buffer, offset, MTP_TYPE_UINT32_CODE, int64Value);
    EXPECT_EQ(ret, false);
    for (int i = 0; i < SIZE_NUM_BIG; i++)
    {
        buffer.push_back(0);
    }
    ret = SetObjectPropValueData::ReadInt32Value(buffer, offset, MTP_TYPE_INT32_CODE, int64Value);
    EXPECT_EQ(ret, true);
    ret = SetObjectPropValueData::ReadInt32Value(buffer, offset, MTP_TYPE_UINT32_CODE, int64Value);
    EXPECT_EQ(ret, true);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_readInt64Value_test_001, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    vector<uint8_t> buffer;
    size_t offset = 0;
    int type = 0;
    int64_t int64Value = 0;
    bool ret = SetObjectPropValueData::ReadInt64Value(buffer, offset, type, int64Value);
    EXPECT_EQ(ret, false);
    ret = SetObjectPropValueData::ReadInt64Value(buffer, offset, MTP_TYPE_INT64_CODE, int64Value);
    EXPECT_EQ(ret, false);
    ret = SetObjectPropValueData::ReadInt64Value(buffer, offset, MTP_TYPE_UINT64_CODE, int64Value);
    EXPECT_EQ(ret, false);
    for (int i = 0; i < SIZE_NUM_BIG; i++)
    {
        buffer.push_back(0);
    }
    ret = SetObjectPropValueData::ReadInt64Value(buffer, offset, MTP_TYPE_INT64_CODE, int64Value);
    EXPECT_EQ(ret, true);
    ret = SetObjectPropValueData::ReadInt64Value(buffer, offset, MTP_TYPE_UINT64_CODE, int64Value);
    EXPECT_EQ(ret, true);
}

HWTEST_F(MediaLibraryMTPUnitTest, medialibrary_readIntValue_test_001, TestSize.Level1)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    vector<uint8_t> buffer;
    size_t offset = 0;
    int type = 0;
    int64_t int64Value = 0;
    bool ret = SetObjectPropValueData::ReadIntValue(buffer, offset, type, int64Value);
    EXPECT_EQ(ret, false);
    ret = SetObjectPropValueData::ReadIntValue(buffer, offset, MTP_TYPE_INT8_CODE, int64Value);
    EXPECT_EQ(ret, false);
    ret = SetObjectPropValueData::ReadIntValue(buffer, offset, MTP_TYPE_INT16_CODE, int64Value);
    EXPECT_EQ(ret, false);
    ret = SetObjectPropValueData::ReadIntValue(buffer, offset, MTP_TYPE_INT32_CODE, int64Value);
    EXPECT_EQ(ret, false);
    ret = SetObjectPropValueData::ReadIntValue(buffer, offset, MTP_TYPE_INT64_CODE, int64Value);
    EXPECT_EQ(ret, false);
    for (int i = 0; i < SIZE_NUM_BIG; i++)
    {
        buffer.push_back(0);
    }
    ret = SetObjectPropValueData::ReadIntValue(buffer, offset, MTP_TYPE_INT8_CODE, int64Value);
    EXPECT_EQ(ret, true);
    ret = SetObjectPropValueData::ReadIntValue(buffer, offset, MTP_TYPE_INT16_CODE, int64Value);
    EXPECT_EQ(ret, true);
    ret = SetObjectPropValueData::ReadIntValue(buffer, offset, MTP_TYPE_INT32_CODE, int64Value);
    EXPECT_EQ(ret, true);
    ret = SetObjectPropValueData::ReadIntValue(buffer, offset, MTP_TYPE_INT64_CODE, int64Value);
    EXPECT_EQ(ret, true);
}
} // namespace Media
} // namespace OHOS