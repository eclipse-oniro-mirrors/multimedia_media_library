/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "medialibrary_mtp_datautils_fuzzer.h"

#include <cstdint>
#include <string>
#include <vector>
#include <fuzzer/FuzzedDataProvider.h>

#include "system_ability_definition.h"
#include "iservice_registry.h"
#include "userfilemgr_uri.h"
#include "payload_data.h"
#include "close_session_data.h"
#include "media_log.h"
#include "media_mtp_utils.h"

#define private public
#include "mtp_data_utils.h"
#include "property.h"
#undef private

namespace OHOS {
using namespace std;
using namespace Media;
static const int32_t NUM_BYTES = 1;
static const int32_t MAX_DATA_TYPE = 4;
static const int32_t MAX_SUB_TYPE = 5;
static const string MEDIA_DATA_DB_COMPOSER = "composer";
FuzzedDataProvider *provider = nullptr;

static inline vector<uint16_t> FuzzVectorUInt16()
{
    return {provider->ConsumeIntegral<uint16_t>()};
}

static inline vector<uint32_t> FuzzVectorUInt32()
{
    return {provider->ConsumeIntegral<uint32_t>()};
}

static inline PhotoSubType FuzzPhotoSubType()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0, MAX_SUB_TYPE);
    return static_cast<PhotoSubType>(value);
}

static inline Media::ResultSetDataType FuzzResultSetDataType()
{
    int32_t dataType = provider->ConsumeIntegralInRange<int32_t>(0, MAX_DATA_TYPE);
    return static_cast<Media::ResultSetDataType>(dataType);
}

static MtpOperationContext FuzzMtpOperationContext()
{
    MtpOperationContext context;
    context.operationCode = provider->ConsumeIntegral<uint16_t>();
    context.transactionID = provider->ConsumeIntegral<uint32_t>();
    context.devicePropertyCode = provider->ConsumeIntegral<uint32_t>();
    context.storageID = provider->ConsumeIntegral<uint32_t>();
    context.format = provider->ConsumeIntegral<uint16_t>();
    context.parent = provider->ConsumeIntegral<uint32_t>();
    context.handle = provider->ConsumeIntegral<uint32_t>();
    context.property = provider->ConsumeIntegral<uint32_t>();
    context.groupCode = provider->ConsumeIntegral<uint32_t>();
    context.depth = provider->ConsumeIntegral<uint32_t>();
    context.properStrValue = provider->ConsumeBytesAsString(NUM_BYTES);
    context.properIntValue = provider->ConsumeIntegral<int64_t>();
    context.handles = make_shared<UInt32List>(FuzzVectorUInt32());
    context.name = provider->ConsumeBytesAsString(NUM_BYTES);
    context.created = provider->ConsumeBytesAsString(NUM_BYTES);
    context.modified = provider->ConsumeBytesAsString(NUM_BYTES);
    context.indata = provider->ConsumeBool();
    context.storageInfoID = provider->ConsumeIntegral<uint32_t>();
    context.sessionOpen = provider->ConsumeBool();
    context.sessionID = provider->ConsumeIntegral<uint32_t>();
    context.mtpDriver = make_shared<MtpDriver>();
    context.tempSessionID = provider->ConsumeIntegral<uint32_t>();
    context.eventHandle = provider->ConsumeIntegral<uint32_t>();
    context.eventProperty = provider->ConsumeIntegral<uint32_t>();
    return context;
}

static void SolveHandlesFormatDataTest()
{
    uint16_t format = provider->ConsumeIntegral<uint16_t>();
    MediaType outMediaType = MEDIA_TYPE_FILE;
    string outExtension = provider->ConsumeBytesAsString(NUM_BYTES);
    MtpDataUtils::SolveHandlesFormatData(format, outExtension, outMediaType);

    format = provider->ConsumeBool() ? MTP_FORMAT_UNDEFINED_CODE : provider->ConsumeIntegral<uint16_t>();
    MtpDataUtils::SolveHandlesFormatData(format, outExtension, outMediaType);
}

static void SolveSendObjectFormatDataTest()
{
    uint16_t format = provider->ConsumeIntegral<uint16_t>();
    MediaType outMediaType = MEDIA_TYPE_FILE;
    MtpDataUtils::SolveSendObjectFormatData(format, outMediaType);
}

static void SolveSetObjectPropValueDataTest()
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>(
        FuzzMtpOperationContext());
    if (context == nullptr) {
        MEDIA_ERR_LOG("context is nullptr");
        return;
    }
    context->property = provider->ConsumeBool() ? MTP_PROPERTY_OBJECT_FILE_NAME_CODE :
        provider->ConsumeIntegral<uint32_t>();
    string outColName = provider->ConsumeBytesAsString(NUM_BYTES);
    variant<int64_t, string> outColVal;
    MtpDataUtils::SolveSetObjectPropValueData(context, outColName, outColVal);
}

static void GetMediaTypeByformatTest()
{
    uint16_t format = provider->ConsumeIntegral<uint16_t>();
    MediaType outMediaType = MEDIA_TYPE_FILE;
    MtpDataUtils::GetMediaTypeByformat(format, outMediaType);
}

static void GetPropListBySetTest()
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>(
        FuzzMtpOperationContext());
    if (context == nullptr) {
        MEDIA_ERR_LOG("context is nullptr");
        return;
    }
    const shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>();
    shared_ptr<vector<Property>> outProps = make_shared<vector<Property>>();
    MtpDataUtils::GetPropListBySet(context, resultSet, outProps);

    context->property = provider->ConsumeBool() ? MTP_PROPERTY_ALL_CODE : provider->ConsumeIntegral<int32_t>();
    MtpDataUtils::GetPropListBySet(context, resultSet, outProps);
}

static void GetPropValueBySetTest()
{
    uint32_t property = provider->ConsumeIntegral<uint32_t>();
    PropertyValue outPropValue;
    const shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>();
    MtpDataUtils::GetPropValueBySet(property, resultSet, outPropValue, false);
}

static void GetMediaTypeByNameTest()
{
    string displayName = provider->ConsumeBytesAsString(NUM_BYTES);
    MediaType outMediaType = MEDIA_TYPE_FILE;
    MtpDataUtils::GetMediaTypeByName(displayName, outMediaType);
}

static void GetPropListTest()
{
    const shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>(
        FuzzMtpOperationContext());
    if (context == nullptr) {
        MEDIA_ERR_LOG("context is nullptr");
        return;
    }
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>();
    shared_ptr<UInt16List> properties = make_shared<UInt16List>(FuzzVectorUInt16());
    shared_ptr<vector<Property>> outProps = make_shared<vector<Property>>();
    MtpDataUtils::GetPropList(context, resultSet, properties, outProps);
}

static void ReturnErrorTest()
{
    string errMsg = "";
    ResultSetDataType type = FuzzResultSetDataType();
    MtpDataUtils::ReturnError(errMsg, type);
}

static void GetMovingOrEnditOneRowPropListTest()
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>(
        FuzzMtpOperationContext());
    if (context == nullptr) {
        MEDIA_ERR_LOG("context is nullptr");
        return;
    }
    shared_ptr<vector<uint16_t>> properties = make_shared<vector<uint16_t>>(FuzzVectorUInt16());
    properties->push_back(MTP_PROPERTY_STORAGE_ID_CODE);
    properties->push_back(MTP_PROPERTY_OBJECT_FORMAT_CODE);
    string path = provider->ConsumeBytesAsString(NUM_BYTES);
    shared_ptr<vector<Property>> outProps = make_shared<vector<Property>>();
    MovingType movingType;
    MtpDataUtils::GetMovingOrEnditOneRowPropList(properties, path, context, outProps, movingType);
}

static void GetFormatTest()
{
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>();
    uint16_t outFormat = provider->ConsumeIntegral<uint16_t>();
    MtpDataUtils::GetFormat(resultSet, outFormat);

    uint32_t handle = provider->ConsumeIntegral<uint32_t>();
    shared_ptr<vector<uint16_t>> properties = make_shared<vector<uint16_t>>(FuzzVectorUInt16());
    properties->push_back(MTP_PROPERTY_STORAGE_ID_CODE);
    properties->push_back(MTP_PROPERTY_OBJECT_FORMAT_CODE);
    shared_ptr<vector<Property>> outProps = make_shared<vector<Property>>();

    MtpDataUtils::GetOneRowPropList(handle, resultSet, properties, outProps);
}

static void SetOneDefaultlPropListTest()
{
    shared_ptr<vector<Property>> outProps = make_shared<vector<Property>>();
    MtpDataUtils::SetOneDefaultlPropList(0, MTP_PROPERTY_PROTECTION_STATUS_CODE, outProps);
    MtpDataUtils::SetOneDefaultlPropList(0, MTP_PROPERTY_PERSISTENT_UID_CODE, outProps);
    MtpDataUtils::SetOneDefaultlPropList(0, MTP_PROPERTY_ALBUM_NAME_CODE, outProps);
    MtpDataUtils::SetOneDefaultlPropList(0, MTP_PROPERTY_STORAGE_ID_CODE, outProps);
    string column = provider->ConsumeBytesAsString(NUM_BYTES);
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>();
    Property prop;
    ResultSetDataType type = TYPE_NULL;
    MtpDataUtils::SetProperty(column, resultSet, type, prop);
    ResultSetDataType typeOne = TYPE_STRING;
    MtpDataUtils::SetProperty(column, resultSet, typeOne, prop);
    ResultSetDataType typeTwo = TYPE_INT32;
    MtpDataUtils::SetProperty(column, resultSet, typeTwo, prop);
    ResultSetDataType typeThree = TYPE_INT64;
    column = provider->ConsumeBool() ? CONST_MEDIA_DATA_DB_DATE_MODIFIED : provider->ConsumeBytesAsString(NUM_BYTES);
    MtpDataUtils::SetProperty(column, resultSet, typeThree, prop);
    ResultSetDataType typeFour = TYPE_DOUBLE;
    MtpDataUtils::SetProperty(column, resultSet, typeFour, prop);
    uint16_t outFormat = 0;
    MtpDataUtils::GetFormatByPath("", outFormat);
    string path = provider->ConsumeBytesAsString(NUM_BYTES);
    MtpDataUtils::GetFormatByPath(path, outFormat);
}

static void GetMtpPropListTest()
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>(
        FuzzMtpOperationContext());
    if (context == nullptr) {
        MEDIA_ERR_LOG("context is nullptr");
        return;
    }
    context->property = MTP_PROPERTY_ALL_CODE;
    shared_ptr<unordered_map<uint32_t, string>> handles = make_shared<unordered_map<uint32_t, string>>();
    unordered_map<std::string, uint32_t> pathHandles;
    shared_ptr<vector<Property>> outPropValue = make_shared<vector<Property>>();
    MtpDataUtils::GetMtpPropList(handles, pathHandles, context, outPropValue);
}

static void GetMtpOneRowPropTest()
{
    shared_ptr<vector<uint16_t>> properties = make_shared<vector<uint16_t>>(FuzzVectorUInt16());
    uint32_t parentId = provider->ConsumeIntegral<uint32_t>();
    std::unordered_map<uint32_t, std::string> umap{
        {provider->ConsumeIntegral<uint32_t>(), provider->ConsumeBytesAsString(NUM_BYTES)}
    };
    shared_ptr<vector<Property>> outProps = make_shared<vector<Property>>();
    int32_t storageId = provider->ConsumeIntegral<int32_t>();
    MtpDataUtils::GetMtpOneRowProp(properties, parentId, umap.begin(), outProps, storageId);
}

static void SetMtpPropertyTest()
{
    string column = CONST_MEDIA_DATA_DB_NAME;
    string path = provider->ConsumeBytesAsString(NUM_BYTES);
    ResultSetDataType type;
    Property prop;
    MtpDataUtils::SetMtpProperty(column, path, type, prop);

    column = CONST_MEDIA_DATA_DB_SIZE;
    MtpDataUtils::SetMtpProperty(column, path, type, prop);

    column = CONST_MEDIA_DATA_DB_DATE_MODIFIED;
    MtpDataUtils::SetMtpProperty(column, path, type, prop);

    column = CONST_MEDIA_DATA_DB_DATE_ADDED;
    MtpDataUtils::SetMtpProperty(column, path, type, prop);

    column = CONST_MEDIA_DATA_DB_DESCRIPTION;
    MtpDataUtils::SetMtpProperty(column, path, type, prop);

    column = CONST_MEDIA_DATA_DB_DURATION;
    MtpDataUtils::SetMtpProperty(column, path, type, prop);

    column = CONST_MEDIA_DATA_DB_ARTIST;
    MtpDataUtils::SetMtpProperty(column, path, type, prop);

    column = CONST_MEDIA_DATA_DB_ALBUM_NAME;
    MtpDataUtils::SetMtpProperty(column, path, type, prop);

    column = MEDIA_DATA_DB_COMPOSER;
    MtpDataUtils::SetMtpProperty(column, path, type, prop);
}

static void SetPtpPropertyTest()
{
    string column = CONST_MEDIA_DATA_DB_NAME;
    string path = provider->ConsumeBytesAsString(NUM_BYTES);
    MovingType movingType;
    movingType.displayName = provider->ConsumeBytesAsString(NUM_BYTES);
    Property prop;
    prop.currentValue = make_shared<Property::Value>();
    MtpDataUtils::SetPtpProperty(column, path, movingType, prop);

    column = CONST_MEDIA_DATA_DB_PARENT_ID;
    MtpDataUtils::SetPtpProperty(column, path, movingType, prop);

    column = CONST_MEDIA_DATA_DB_SIZE;
    MtpDataUtils::SetPtpProperty(column, path, movingType, prop);

    column = CONST_MEDIA_DATA_DB_DATE_MODIFIED;
    MtpDataUtils::SetPtpProperty(column, path, movingType, prop);

    column = CONST_MEDIA_DATA_DB_DATE_ADDED;
    MtpDataUtils::SetPtpProperty(column, path, movingType, prop);
}

static void GetMovingOrEnditSourcePathTest()
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>(
        FuzzMtpOperationContext());
    if (context == nullptr) {
        MEDIA_ERR_LOG("context is nullptr");
        return;
    }
    string path = provider->ConsumeBytesAsString(NUM_BYTES);
    int32_t subtype = static_cast<int32_t>(FuzzPhotoSubType());
    MtpDataUtils::GetMovingOrEnditSourcePath(path, subtype, context);

    context->handle = EDITED_PHOTOS_OFFSET;
    MtpDataUtils::GetMovingOrEnditSourcePath(path, subtype, context);

    context->handle = COMMON_MOVING_OFFSET;
    MtpDataUtils::GetMovingOrEnditSourcePath(path, subtype, context);

    context->handle = EDITED_MOVING_OFFSET;
    MtpDataUtils::GetMovingOrEnditSourcePath(path, subtype, context);
}

static void GetMtpPropValueTest()
{
    string path = provider->ConsumeBytesAsString(NUM_BYTES);
    uint32_t property = MTP_PROPERTY_OBJECT_FILE_NAME_CODE;
    uint16_t format = provider->ConsumeIntegral<uint16_t>();
    PropertyValue outPropValue;
    MtpDataUtils::GetMtpPropValue(path, property, format, outPropValue);

    property = MTP_PROPERTY_OBJECT_SIZE_CODE;
    MtpDataUtils::GetMtpPropValue(path, property, format, outPropValue);

    property = MTP_PROPERTY_DATE_MODIFIED_CODE;
    MtpDataUtils::GetMtpPropValue(path, property, format, outPropValue);

    property = MTP_PROPERTY_DATE_ADDED_CODE;
    MtpDataUtils::GetMtpPropValue(path, property, format, outPropValue);
}

static void SetMtpOneDefaultlPropListTest()
{
    uint32_t handle = provider->ConsumeIntegral<uint32_t>();
    uint16_t property = MTP_PROPERTY_PROTECTION_STATUS_CODE;
    shared_ptr<vector<Property>> outProps = make_shared<vector<Property>>();
    int32_t storageId = provider->ConsumeIntegral<int32_t>();
    MtpDataUtils::SetMtpOneDefaultlPropList(handle, property, outProps, storageId);

    property = MTP_PROPERTY_PERSISTENT_UID_CODE;
    MtpDataUtils::SetMtpOneDefaultlPropList(handle, property, outProps, storageId);

    property = MTP_PROPERTY_ALBUM_NAME_CODE;
    MtpDataUtils::SetMtpOneDefaultlPropList(handle, property, outProps, storageId);

    property = MTP_PROPERTY_STORAGE_ID_CODE;
    MtpDataUtils::SetMtpOneDefaultlPropList(handle, property, outProps, storageId);
}

static void MtpDataUtilsTest()
{
    SolveHandlesFormatDataTest();
    SolveSendObjectFormatDataTest();
    SolveSetObjectPropValueDataTest();
    GetMediaTypeByformatTest();
    GetPropListBySetTest();
    GetPropValueBySetTest();
    GetMediaTypeByNameTest();
    GetPropListTest();
    ReturnErrorTest();
    GetMovingOrEnditOneRowPropListTest();
    GetFormatTest();
    SetOneDefaultlPropListTest();
    GetMtpPropListTest();
    GetMtpOneRowPropTest();
    SetMtpPropertyTest();
    SetPtpPropertyTest();
    GetMovingOrEnditSourcePathTest();
    GetMtpPropValueTest();
    SetMtpOneDefaultlPropListTest();
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    FuzzedDataProvider fdp(data, size);
    OHOS::provider = &fdp;
    if (data == nullptr) {
        return 0;
    }
    OHOS::MtpDataUtilsTest();
    return 0;
}