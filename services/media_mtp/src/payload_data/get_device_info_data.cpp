/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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
#include "payload_data/get_device_info_data.h"
#include "media_log.h"
#include "media_mtp_utils.h"
#include "mtp_constants.h"
#include "mtp_operation_utils.h"
#include "mtp_packet_tools.h"
#include "mtp_manager.h"
#include "playback_formats.h"

using namespace std;

namespace OHOS {
namespace Media {
static const char *EXTENSION_DESC = "microsoft.com: 1.0; openharmony: 1.0;";

static const uint16_t CaptureFormats[] = {};

static const uint16_t DeviceProperties[] = {
    MTP_DEVICE_PROPERTY_SYNCHRONIZATION_PARTNER_CODE,
    MTP_DEVICE_PROPERTY_DEVICE_FRIENDLY_NAME_CODE,
    MTP_DEVICE_PROPERTY_IMAGE_SIZE_CODE,
    MTP_DEVICE_PROPERTY_BATTERY_LEVEL_CODE,
    MTP_DEVICE_PROPERTY_PERCEIVED_DEVICE_TYPE_CODE,
    MTP_DEVICE_PROPERTY_SESSION_INITIATOR_VERSION_INFO_CODE,
};

static const uint16_t Events[] = {
    MTP_EVENT_OBJECT_ADDED_CODE,
    MTP_EVENT_OBJECT_REMOVED_CODE,
    MTP_EVENT_STORE_ADDED_CODE,
    MTP_EVENT_STORE_REMOVED_CODE,
    MTP_EVENT_DEVICE_PROP_CHANGED_CODE,
    MTP_EVENT_OBJECT_INFO_CHANGED_CODE,
};

static const uint16_t Operations[] = {
    MTP_OPERATION_GET_DEVICE_INFO_CODE,
    MTP_OPERATION_OPEN_SESSION_CODE,
    MTP_OPERATION_CLOSE_SESSION_CODE,
    MTP_OPERATION_GET_STORAGE_IDS_CODE,
    MTP_OPERATION_GET_STORAGE_INFO_CODE,
    MTP_OPERATION_GET_NUM_OBJECTS_CODE,
    MTP_OPERATION_GET_OBJECT_HANDLES_CODE,
    MTP_OPERATION_GET_OBJECT_INFO_CODE,
    MTP_OPERATION_GET_OBJECT_CODE,
    MTP_OPERATION_GET_THUMB_CODE,
    MTP_OPERATION_DELETE_OBJECT_CODE,
    MTP_OPERATION_SEND_OBJECT_INFO_CODE,
    MTP_OPERATION_SEND_OBJECT_CODE,
    MTP_OPERATION_RESET_DEVICE_CODE,
    MTP_OPERATION_GET_DEVICE_PROP_DESC_CODE,
    MTP_OPERATION_GET_DEVICE_PROP_VALUE_CODE,
    MTP_OPERATION_SET_DEVICE_PROP_VALUE_CODE,
    MTP_OPERATION_RESET_DEVICE_PROP_VALUE_CODE,
    MTP_OPERATION_MOVE_OBJECT_CODE,
    MTP_OPERATION_COPY_OBJECT_CODE,
    MTP_OPERATION_GET_PARTIAL_OBJECT_CODE,
    MTP_OPERATION_GET_OBJECT_PROPS_SUPPORTED_CODE,
    MTP_OPERATION_GET_OBJECT_PROP_DESC_CODE,
    MTP_OPERATION_GET_OBJECT_PROP_VALUE_CODE,
    MTP_OPERATION_SET_OBJECT_PROP_VALUE_CODE,
    MTP_OPERATION_GET_OBJECT_PROP_LIST_CODE,
};

const int EXTENSION_ID = 6;
GetDeviceInfoData::GetDeviceInfoData(std::shared_ptr<MtpOperationContext> &context)
    : PayloadData(context), functionalMode_(0), serialNum_("0")
{
    standardVersion_ = MTP_STANDARD_VERSION;
    vendorExtensionID_ = EXTENSION_ID;
    vendorExtensionVersion_ = MTP_STANDARD_VERSION;
    vendorExtensionDesc_ = EXTENSION_DESC;
    manufacturer_ = DEFAULT_PRODUCT_MANUFACTURER;
    model_ = DEFAULT_PRODUCT_MODEL;
    version_ = DEFAULT_PRODUCT_SOFTWARE_VERSION;
    Maker(mOutBuffer);
}

GetDeviceInfoData::GetDeviceInfoData()
{
    standardVersion_ = MTP_STANDARD_VERSION;
    vendorExtensionID_ = EXTENSION_ID;
    vendorExtensionVersion_ = MTP_STANDARD_VERSION;
    vendorExtensionDesc_ = EXTENSION_DESC;
    functionalMode_ = 0;
    manufacturer_ = DEFAULT_PRODUCT_MANUFACTURER;
    model_ = DEFAULT_PRODUCT_MODEL;
    version_ = DEFAULT_PRODUCT_SOFTWARE_VERSION;
    serialNum_ = "0";
    Maker(mOutBuffer);
}

GetDeviceInfoData::~GetDeviceInfoData()
{
}

int GetDeviceInfoData::Parser(const std::vector<uint8_t> &buffer, int32_t readSize)
{
    return MTP_SUCCESS;
}

int GetDeviceInfoData::Maker(std::vector<uint8_t> &outBuffer)
{
    MtpPacketTool::PutUInt16(outBuffer, standardVersion_);
    MtpPacketTool::PutUInt32(outBuffer, vendorExtensionID_);
    MtpPacketTool::PutUInt16(outBuffer, vendorExtensionVersion_);
    MtpPacketTool::PutString(outBuffer, vendorExtensionDesc_);
    MtpPacketTool::PutUInt16(outBuffer, functionalMode_);
    MtpPacketTool::PutAUInt16(outBuffer, Operations, sizeof(Operations) / sizeof(uint16_t));
    MtpPacketTool::PutAUInt16(outBuffer, Events, sizeof(Events) / sizeof(uint16_t));
    MtpPacketTool::PutAUInt16(outBuffer, DeviceProperties, sizeof(DeviceProperties) / sizeof(uint16_t));
    MtpPacketTool::PutAUInt16(outBuffer, CaptureFormats, sizeof(CaptureFormats) / sizeof(uint16_t));
    MtpPacketTool::PutAUInt16(outBuffer, PLAYBACK_FORMATS, sizeof(PLAYBACK_FORMATS) / sizeof(uint16_t));
    MtpPacketTool::PutString(outBuffer, manufacturer_);
    MtpPacketTool::PutString(outBuffer, model_);
    MtpPacketTool::PutString(outBuffer, version_);
    MtpPacketTool::PutString(outBuffer, serialNum_);

    return MTP_SUCCESS;
}

uint32_t GetDeviceInfoData::CalculateSize()
{
    std::vector<uint8_t> tmpVar;

    int res = Maker(tmpVar);
    if (res != MTP_SUCCESS) {
        return res;
    }

    return tmpVar.size();
}

void GetDeviceInfoData::SetManufacturer(const std::string &manufacturer)
{
    manufacturer_ = manufacturer;
}

void GetDeviceInfoData::SetModel(const std::string &model)
{
    model_ = model;
}

void GetDeviceInfoData::SetVersion(const std::string &version)
{
    version_ = version;
}

void GetDeviceInfoData::SetSerialNum(const std::string &serialNum)
{
    serialNum_ = serialNum;
}
} // namespace Media
} // namespace OHOS