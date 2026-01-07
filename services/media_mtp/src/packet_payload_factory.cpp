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
#include "packet_payload_factory.h"
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
#include "payload_data/open_session_data.h"
#include "payload_data/send_object_data.h"
#include "payload_data/send_object_info_data.h"
#include "payload_data/set_device_prop_value_data.h"
#include "payload_data/set_object_prop_value_data.h"
#include "payload_data/set_object_references_data.h"

using namespace std;
namespace OHOS {
namespace Media {
PacketPayloadFactory::PacketPayloadFactory()
{
}

PacketPayloadFactory::~PacketPayloadFactory()
{
}

std::shared_ptr<PayloadData> PacketPayloadFactory::CreatePayload(std::shared_ptr<MtpOperationContext> &context,
    const uint16_t code, const uint16_t stage)
{
    shared_ptr<PayloadData> payloadData;
    switch (code) {
        case MTP_OPERATION_GET_DEVICE_INFO_CODE:
            payloadData = make_shared<GetDeviceInfoData>(context);
            break;
        case MTP_OPERATION_OPEN_SESSION_CODE:
            payloadData = make_shared<OpenSessionData>(context);
            break;
        case MTP_OPERATION_RESET_DEVICE_CODE:
        case MTP_OPERATION_CLOSE_SESSION_CODE:
            payloadData = make_shared<CloseSessionData>(context);
            break;
        case MTP_OPERATION_GET_OBJECT_PROPS_SUPPORTED_CODE:
            payloadData = make_shared<GetObjectPropsSupportedData>(context);
            break;
        case MTP_OPERATION_GET_OBJECT_HANDLES_CODE:
            payloadData = make_shared<GetObjectHandlesData>(context);
            break;
        case MTP_OPERATION_GET_NUM_OBJECTS_CODE:
            payloadData = make_shared<GetNumObjectsData>(context);
            break;
        case MTP_OPERATION_GET_OBJECT_INFO_CODE:
            payloadData = make_shared<GetObjectInfoData>(context);
            break;
        case MTP_OPERATION_GET_OBJECT_PROP_DESC_CODE:
            payloadData = make_shared<GetObjectPropDescData>(context);
            break;
        case MTP_OPERATION_GET_OBJECT_PROP_VALUE_CODE:
            payloadData = make_shared<GetObjectPropValueData>(context);
            break;
        case MTP_OPERATION_SET_OBJECT_PROP_VALUE_CODE:
            payloadData = make_shared<SetObjectPropValueData>(context);
            break;
        case MTP_OPERATION_GET_OBJECT_PROP_LIST_CODE:
            payloadData = make_shared<GetObjectPropListData>(context);
            break;
        case MTP_OPERATION_GET_OBJECT_REFERENCES_CODE:
            payloadData = make_shared<GetObjectReferencesData>(context);
            break;
        case MTP_OPERATION_SET_OBJECT_REFERENCES_CODE:
            payloadData = make_shared<SetObjectReferencesData>(context);
            break;
        case MTP_OPERATION_DELETE_OBJECT_CODE:
            payloadData = make_shared<DeleteObjectData>(context);
            break;
        default:
            payloadData = CreatePayloadMore(context, code, stage);
            break;
    }
    return payloadData;
}

std::shared_ptr<PayloadData> PacketPayloadFactory::CreatePayloadMore(std::shared_ptr<MtpOperationContext> &context,
    const uint16_t code, const uint16_t stage)
{
    shared_ptr<PayloadData> payloadData;
    switch (code) {
        case MTP_OPERATION_MOVE_OBJECT_CODE:
            payloadData = make_shared<MoveObjectData>(context);
            break;
        case MTP_OPERATION_COPY_OBJECT_CODE:
            payloadData = make_shared<CopyObjectData>(context);
            break;
        case MTP_OPERATION_GET_OBJECT_CODE:
            payloadData = make_shared<GetObjectData>(context);
            break;
        case MTP_OPERATION_SEND_OBJECT_CODE:
            payloadData = make_shared<SendObjectData>(context);
            break;
        case MTP_OPERATION_GET_THUMB_CODE:
            payloadData = make_shared<GetThumbData>(context);
            break;
        case MTP_OPERATION_SEND_OBJECT_INFO_CODE:
            payloadData = make_shared<SendObjectInfoData>(context);
            break;
        case MTP_OPERATION_GET_PARTIAL_OBJECT_CODE:
            payloadData = make_shared<GetPartialObjectData>(context);
            break;
        case MTP_OPERATION_GET_STORAGE_IDS_CODE:
            payloadData = make_shared<GetStorageIdsData>(context);
            break;
        case MTP_OPERATION_GET_STORAGE_INFO_CODE:
            payloadData = make_shared<GetStorageInfoData>(context);
            break;
        case MTP_OPERATION_GET_DEVICE_PROP_DESC_CODE: // 0x1004 device_prop_desc
            payloadData = make_shared<GetDevicePropDescData>(context);
            break;
        case MTP_OPERATION_GET_DEVICE_PROP_VALUE_CODE:
            payloadData = make_shared<GetDevicePropValueData>(context);
            break;
        case MTP_OPERATION_SET_DEVICE_PROP_VALUE_CODE:
            payloadData = make_shared<SetDevicePropValueData>(context);
            break;
        default:
            break;
    }
    return payloadData;
}
} // namespace Media
} // namespace OHOS