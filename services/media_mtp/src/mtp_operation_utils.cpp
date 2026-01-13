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
#define MLOG_TAG "MtpOperationUtils"
#include "mtp_operation_utils.h"
#include <cinttypes>
#include <sys/time.h>
#include "ability_manager_client.h"
#ifdef HAS_BATTERY_MANAGER_PART
#include "battery_srv_client.h"
#endif
#include "directory_ex.h"
#include "iservice_registry.h"
#include "media_log.h"
#include "media_mtp_utils.h"
#include "mtp_dfx_reporter.h"
#include "mtp_manager.h"
#include "mtp_packet_tools.h"
#include "mtp_operation_context.h"
#include "mtp_ptp_proxy.h"
#include "mtp_storage_manager.h"
#include "mtp_store_observer.h"
#include "payload_data/resp_common_data.h"
#include "payload_data/close_session_data.h"
#include "payload_data/copy_object_data.h"
#include "payload_data/get_device_info_data.h"
#include "payload_data/get_device_prop_desc_data.h"
#include "payload_data/get_device_prop_value_data.h"
#include "payload_data/get_num_objects_data.h"
#include "payload_data/get_object_data.h"
#include "payload_data/get_object_handles_data.h"
#include "payload_data/get_object_info_data.h"
#include "payload_data/get_object_prop_list_data.h"
#include "payload_data/get_object_prop_desc_data.h"
#include "payload_data/get_object_prop_value_data.h"
#include "payload_data/get_object_props_supported_data.h"
#include "payload_data/get_object_references_data.h"
#include "payload_data/get_partial_object_data.h"
#include "payload_data/get_storage_info_data.h"
#include "payload_data/get_storage_ids_data.h"
#include "payload_data/get_thumb_data.h"
#include "payload_data/object_event_data.h"
#include "payload_data/open_session_data.h"
#include "payload_data/send_object_info_data.h"
#include "payload_data/set_object_references_data.h"
#include "parameters.h"
using namespace std;
namespace OHOS {
namespace Media {
#ifdef HAS_BATTERY_MANAGER_PART
static constexpr int MAX_BATTERY = 100;
static constexpr int ERROR_BATTERY = -1;
#endif
static constexpr int EMPTY_BATTERY = 0;
static constexpr int STORAGE_MANAGER_UID = 5003;
static constexpr int RECEVIE_OBJECT_CANCELLED = -20;
static constexpr int RECEVIE_OBJECT_FAILED = -17;
const std::string PUBLIC_DOC = "/storage/media/local/files/Docs";

static constexpr uint32_t HEADER_LEN = 12;
static constexpr uint32_t READ_LEN = 1024;
static constexpr uint32_t SEND_OBJECT_FILE_MAX_SIZE = 0xFFFFFFFF;
constexpr int32_t PATH_TIMEVAL_MAX = 2;
static bool g_isDevicePropSet = false;

MtpOperationUtils::MtpOperationUtils(const shared_ptr<MtpOperationContext> &context, bool isInit) : context_(context)
{
    if (isInit) {
        auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
        CHECK_AND_RETURN_LOG(saManager != nullptr, "GetSystemAbilityManager failed, saManager is null");

        auto token = saManager->GetSystemAbility(STORAGE_MANAGER_UID);
        MtpPtpProxy::GetInstance().Init(token, context);
        g_isDevicePropSet = false;
    }
}

MtpOperationUtils::~MtpOperationUtils()
{
}

uint16_t MtpOperationUtils::GetDeviceInfo(shared_ptr<PayloadData> &data, uint16_t containerType, int &errorCode)
{
    if (containerType != DATA_CONTAINER_TYPE) {
        data = make_shared<RespCommonData>();
        return CheckErrorCode(errorCode);
    }

    shared_ptr<GetDeviceInfoData> getDeviceInfoData = make_shared<GetDeviceInfoData>();
    getDeviceInfoData->SetManufacturer(GetPropertyInner("const.product.manufacturer",
        DEFAULT_PRODUCT_MANUFACTURER));
    getDeviceInfoData->SetModel(GetPropertyInner("const.product.model", DEFAULT_PRODUCT_MODEL));
    getDeviceInfoData->SetVersion(GetPropertyInner("const.product.software.version",
        DEFAULT_PRODUCT_SOFTWARE_VERSION));
    getDeviceInfoData->SetSerialNum(GetPropertyInner("ohos.boot.sn", "0"));
    data = getDeviceInfoData;
    errorCode = MTP_SUCCESS;
    return MTP_OK_CODE;
}

uint16_t MtpOperationUtils::GetNumObjects(shared_ptr<PayloadData> &data)
{
    CHECK_AND_RETURN_RET_LOG(context_ != nullptr, CheckErrorCode(MTP_ERROR_CONTEXT_IS_NULL),
        "GetNumObjects context_ is null");

    CHECK_AND_RETURN_RET(MtpStorageManager::GetInstance()->HasStorage(context_->storageID),
        MTP_INVALID_STORAGEID_CODE);

    shared_ptr<GetNumObjectsData> getNumObjects = make_shared<GetNumObjectsData>();
    data = getNumObjects;
    return CheckErrorCode(MTP_SUCCESS);
}

uint16_t MtpOperationUtils::HasStorage(int &errorCode)
{
    CHECK_AND_RETURN_RET_LOG(context_ != nullptr, MTP_ERROR_CONTEXT_IS_NULL,
        "GetObjectHandles context_ is null");
    CHECK_AND_RETURN_RET_LOG(context_->sessionOpen != false, MTP_ERROR_SESSION_NOT_OPEN,
        "GetObjectHandles session not open");
    CHECK_AND_RETURN_RET_LOG(MtpStorageManager::GetInstance()->HasStorage(context_->storageID),
        MTP_ERROR_INVALID_STORAGE_ID, "GetObjectHandles no this storage");
    return MTP_SUCCESS;
}

uint16_t MtpOperationUtils::GetObjectHandles(shared_ptr<PayloadData> &data, uint16_t containerType, int &errorCode)
{
    if (containerType != DATA_CONTAINER_TYPE) {
        data = make_shared<RespCommonData>();
        return CheckErrorCode(errorCode);
    }

    uint16_t ret = HasStorage(errorCode);
    CHECK_AND_RETURN_RET(ret == MTP_SUCCESS, CheckErrorCode(ret));
    // Determine whether the device is a Mac computer
    // WIN/MAC parent is 0, Linux parent is 0xffffffff
    // WIN set device prop, MAC/Linux do not set device prop
    bool isMac = (context_->parent == 0 && !g_isDevicePropSet) ? true : false;
    if (context_->parent == MTP_ALL_HANDLE_ID) {
        context_->parent = 0;
    }
    shared_ptr<UInt32List> objectHandles = make_shared<UInt32List>();
    errorCode = MtpPtpProxy::GetInstance().GetHandles(context_, objectHandles, isMac);
    CHECK_AND_RETURN_RET_LOG(errorCode == MTP_SUCCESS,
        CheckErrorCode(errorCode), "GetObjectHandles GetHandles fail!");

    shared_ptr<GetObjectHandlesData> getObjectHandles = make_shared<GetObjectHandlesData>();
    getObjectHandles->SetObjectHandles(objectHandles);
    data = getObjectHandles;
    errorCode = MTP_SUCCESS;
    return CheckErrorCode(errorCode);
}

uint16_t MtpOperationUtils::GetObjectInfo(shared_ptr<PayloadData> &data, uint16_t containerType, int &errorCode)
{
    if (containerType != DATA_CONTAINER_TYPE) {
        data = make_shared<RespCommonData>();
        return CheckErrorCode(errorCode);
    }

    uint16_t ret = HasStorage(errorCode);
    CHECK_AND_RETURN_RET(errorCode == MTP_SUCCESS, CheckErrorCode(ret));

    shared_ptr<ObjectInfo> objectInfo = make_shared<ObjectInfo>(context_->handle);
    errorCode = MtpPtpProxy::GetInstance().GetObjectInfo(context_, objectInfo);
    CHECK_AND_RETURN_RET_LOG(errorCode == MTP_SUCCESS,
        CheckErrorCode(errorCode), "GetObjectHandles GetObjectInfo fail!");
    shared_ptr<GetObjectInfoData> getObjectInfo = make_shared<GetObjectInfoData>();
    getObjectInfo->SetObjectInfo(objectInfo);
    data = getObjectInfo;
    errorCode = MTP_SUCCESS;
    return CheckErrorCode(errorCode);
}

uint16_t MtpOperationUtils::GetObjectPropDesc(shared_ptr<PayloadData> &data, uint16_t containerType, int &errorCode)
{
    if (containerType != DATA_CONTAINER_TYPE) {
        data = make_shared<RespCommonData>();
        return CheckErrorCode(errorCode);
    }

    CHECK_AND_RETURN_RET_LOG(context_ != nullptr, CheckErrorCode(MTP_ERROR_CONTEXT_IS_NULL),
        "GetObjectPropDesc context_ is null");

    data = make_shared<GetObjectPropDescData>(context_);
    errorCode = MTP_SUCCESS;
    return CheckErrorCode(errorCode);
}

uint16_t MtpOperationUtils::GetObjectPropValue(shared_ptr<PayloadData> &data, uint16_t containerType, int &errorCode)
{
    if (containerType != DATA_CONTAINER_TYPE) {
        data = make_shared<RespCommonData>();
        return CheckErrorCode(errorCode);
    }

    CHECK_AND_RETURN_RET_LOG(context_ != nullptr, CheckErrorCode(MTP_ERROR_CONTEXT_IS_NULL),
        "GetObjectPropValue context_ is null");

    int type = MTP_TYPE_UNDEFINED_CODE;
    // GetObjectPropValue may have 3 types of return value, using params to return in one function
    uint64_t int64Value = 0;
    uint128_t int128Value = {0};
    string strValue;
    errorCode = MtpPtpProxy::GetInstance().GetObjectPropValue(context_, int64Value, int128Value, strValue);
    shared_ptr<GetObjectPropValueData> getObjectPropValue = make_shared<GetObjectPropValueData>(context_);
    type = MtpPacketTool::GetObjectPropTypeByPropCode(context_->property);
    getObjectPropValue->SetPropValue(type, int64Value, int128Value, strValue);
    data = getObjectPropValue;
    errorCode = MTP_SUCCESS;
    return CheckErrorCode(errorCode);
}

void MtpOperationUtils::DoSetObjectPropValue(int &errorCode)
{
    if (context_ == nullptr) {
        MEDIA_ERR_LOG("SetObjectPropValue context_ is null");
        errorCode = MTP_ERROR_CONTEXT_IS_NULL;
        return;
    }

    errorCode = MtpPtpProxy::GetInstance().SetObjectPropValue(context_);
    SendEventPacket(context_->handle, MTP_EVENT_OBJECT_ADDED_CODE);
}

void MtpOperationUtils::SendEventPacket(uint32_t objectHandle, uint16_t eventCode)
{
    CHECK_AND_RETURN_LOG(context_ != nullptr, "SendEventPacket context_ is null");

    EventMtp event;
    event.length = MTP_CONTAINER_HEADER_SIZE + sizeof(objectHandle);
    vector<uint8_t> outBuffer;
    MtpPacketTool::PutUInt32(outBuffer, event.length);
    MtpPacketTool::PutUInt16(outBuffer, EVENT_CONTAINER_TYPE);
    MtpPacketTool::PutUInt16(outBuffer, eventCode);
    MtpPacketTool::PutUInt32(outBuffer, context_->transactionID);
    MtpPacketTool::PutUInt32(outBuffer, objectHandle);

    event.data = outBuffer;
    CHECK_AND_RETURN_LOG(context_->mtpDriver != nullptr, "SendEventPacket mtpDriver is null");
    auto startTime = std::chrono::high_resolution_clock::now();
    int32_t result = context_->mtpDriver->WriteEvent(event);
    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<uint16_t, std::milli> duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    MtpDfxReporter::GetInstance().DoSendResponseResultDfxReporter(eventCode, result,
        duration.count(), OperateMode::writemode);
}

uint16_t MtpOperationUtils::GetObjectPropList(shared_ptr<PayloadData> &data,
    uint16_t containerType, int &errorCode)
{
    if (containerType != DATA_CONTAINER_TYPE) {
        data = make_shared<RespCommonData>();
        return CheckErrorCode(errorCode);
    }

    CHECK_AND_RETURN_RET_LOG(context_ != nullptr, CheckErrorCode(MTP_ERROR_CONTEXT_IS_NULL),
        "GetObjectPropList context_ is null");

    shared_ptr<vector<Property>> props = make_shared<vector<Property>>();
    errorCode = MtpPtpProxy::GetInstance().GetObjectPropList(context_, props);
    CHECK_AND_RETURN_RET_LOG(errorCode == MTP_SUCCESS, CheckErrorCode(errorCode), "GetObjectPropList fail!");

    shared_ptr<GetObjectPropListData> getObjectPropList = make_shared<GetObjectPropListData>(context_);
    getObjectPropList->SetProps(props);
    data = getObjectPropList;
    errorCode = MTP_SUCCESS;
    return CheckErrorCode(errorCode);
}

uint16_t MtpOperationUtils::GetObjectReferences(shared_ptr<PayloadData> &data, uint16_t containerType, int &errorCode)
{
    if (containerType != DATA_CONTAINER_TYPE) {
        data = make_shared<RespCommonData>();
        return CheckErrorCode(errorCode);
    }

    uint16_t ret = HasStorage(errorCode);
    CHECK_AND_RETURN_RET(ret == MTP_SUCCESS, CheckErrorCode(ret));

    shared_ptr<UInt32List> objectHandles = nullptr;
    shared_ptr<GetObjectReferencesData> getObjectReferences = make_shared<GetObjectReferencesData>(context_);
    getObjectReferences->SetObjectHandles(objectHandles);
    data = getObjectReferences;
    errorCode = MTP_SUCCESS;
    return CheckErrorCode(errorCode);
}

uint16_t MtpOperationUtils::SetObjectReferences(shared_ptr<PayloadData> &data)
{
    CHECK_AND_RETURN_RET_LOG(context_ != nullptr, CheckErrorCode(MTP_ERROR_CONTEXT_IS_NULL),
        "SetObjectReferences context_ is null");

    uint16_t result = MTP_INVALID_OBJECTPROP_FORMAT_CODE;
    shared_ptr<SetObjectReferencesData> setObjectReferences = make_shared<SetObjectReferencesData>(context_);
    setObjectReferences->SetResult(result);
    data = setObjectReferences;
    return CheckErrorCode(MTP_SUCCESS);
}

uint16_t MtpOperationUtils::GetObjectDataDeal()
{
    CHECK_AND_RETURN_RET_LOG(context_ != nullptr, MTP_ERROR_CONTEXT_IS_NULL, "GetObjectDataDeal context_ is null");

    int fd = 0;
    int errorCode = 0;
    if (MtpManager::GetInstance().IsMtpMode() && !MtpPtpProxy::GetInstance().IsMtpExistObject(context_)) {
        SendEventPacket(context_->handle, MTP_EVENT_OBJECT_REMOVED_CODE);
        return MTP_INVALID_OBJECTHANDLE_CODE;
    }
    errorCode = MtpPtpProxy::GetInstance().GetReadFd(context_, fd);
    CHECK_AND_RETURN_RET_LOG(errorCode == MTP_SUCCESS, errorCode, "GetObjectDataDeal GetFd fail!");

    MtpFileRange object;
    object.fd = fd;
    struct stat sstat;
    int result = fstat(object.fd, &sstat);
    PreDealFd(result < 0, fd);
    CHECK_AND_RETURN_RET_LOG(result == MTP_SUCCESS, MTP_ERROR_INCOMPLETE_TRANSFER,
        "GetObjectDataDeal fstat error = %{public}d", errno);

    object.offset = static_cast<loff_t>(context_->offset);
    if (context_->length == 0 || context_->length == MTP_ALL_HANDLE_ID) {
        object.length = sstat.st_size;
    } else {
        if (context_->offset > static_cast<uint64_t>(sstat.st_size)) {
            context_->length = 0;
            MEDIA_WARN_LOG("GetObjectDataDeal offset is larger than file size, set length to 0");
        } else if (context_->offset + context_->length > static_cast<uint64_t>(sstat.st_size)) {
            context_->length = static_cast<uint32_t>(static_cast<uint64_t>(sstat.st_size) - context_->offset);
        }
        object.length = context_->length;
    }
    object.command = context_->operationCode;
    object.transaction_id = context_->transactionID;
    result = context_->mtpDriver->SendObj(object);
    PreDealFd(result < 0, fd);
    CHECK_AND_RETURN_RET_LOG(result >= 0, MTP_ERROR_INCOMPLETE_TRANSFER,
        "GetObjectDataDeal SendObj error!");
    int32_t ret = MtpPtpProxy::GetInstance().CloseReadFd(context_, fd);
    CHECK_AND_RETURN_RET_LOG(ret == MTP_SUCCESS, E_ERR, "DealFd CloseFd fail!");
    return MTP_SUCCESS;
}

uint16_t MtpOperationUtils::GetObject(shared_ptr<PayloadData> &data, int errorCode)
{
    data = make_shared<GetObjectData>();
    return CheckErrorCode(errorCode);
}

void MtpOperationUtils::ModifyObjectInfo()
{
    CHECK_AND_RETURN_LOG(context_ != nullptr, "DoRecevieSendObject context_ is null");

    std::tm tmCreated = {};
    std::tm tmModified = {};
    std::istringstream created(context_->created);
    std::istringstream modified(context_->modified);
    created >> std::get_time(&tmCreated, "%Y%m%dT%H%M%S");
    modified >> std::get_time(&tmModified, "%Y%m%dT%H%M%S");
    bool cond = (created.fail() || modified.fail());
    CHECK_AND_RETURN_LOG(!cond, "get_time failed");

    std::string path;
    MtpPtpProxy::GetInstance().GetModifyObjectInfoPathById(context_->handle, path);

    struct timeval times[PATH_TIMEVAL_MAX] = { { 0, 0 }, { 0, 0 } };
    times[0].tv_sec = mktime(&tmCreated);
    times[1].tv_sec = mktime(&tmModified);
    if (utimes(path.c_str(), times) != 0) {
        MEDIA_WARN_LOG("utimes path:%{public}s failed", path.c_str());
    }
}

int32_t MtpOperationUtils::DoRecevieSendObject()
{
    CHECK_AND_RETURN_RET_LOG(context_ != nullptr, MTP_INVALID_PARAMETER_CODE, "DoRecevieSendObject context_ is null");

    vector<uint8_t> dataBuffer;
    uint32_t temp = READ_LEN;
    int errorCode = context_->mtpDriver->Read(dataBuffer, temp);
    CHECK_AND_RETURN_RET_LOG(errorCode == MTP_SUCCESS, errorCode, "DoRecevieSendObject Read error!");

    int fd = 0;
    errorCode = MtpPtpProxy::GetInstance().GetWriteFd(context_, fd);
    CHECK_AND_RETURN_RET_LOG(errorCode == MTP_SUCCESS, errorCode, "DoRecevieSendObject GetFd fail!");

    uint32_t initialData = dataBuffer.size() < HEADER_LEN  ? 0 : dataBuffer.size() - HEADER_LEN;
    errorCode = write(fd, &dataBuffer[HEADER_LEN], initialData);
    PreDealFd(errorCode < 0, fd);
    CHECK_AND_RETURN_RET_LOG(errorCode >= 0, MTP_ERROR_RESPONSE_GENERAL,
        "DoRecevieSendObject write error = %{public}d", errno);

    MtpFileRange object;
    object.fd = fd;
    object.offset = initialData;
    if (context_->sendObjectFileSize == SEND_OBJECT_FILE_MAX_SIZE) {
        // when file size is over 0xFFFFFFFF, driver will read until it receives a short packet
        object.length = SEND_OBJECT_FILE_MAX_SIZE;
    } else {
        object.length = static_cast<int64_t>(context_->sendObjectFileSize) - static_cast<int64_t>(initialData);
    }
    errorCode = RecevieSendObject(object, fd);
    CHECK_AND_RETURN_RET_LOG(errorCode != MTP_ERROR_TRANSFER_CANCELLED, MTP_ERROR_TRANSFER_CANCELLED,
        "DoRecevieSendObject ReceiveObj Cancelled = %{public}d", MTP_ERROR_TRANSFER_CANCELLED);
    CHECK_AND_RETURN_RET_LOG(errorCode != MTP_ERROR_TRANSFER_FAILED, MTP_ERROR_TRANSFER_FAILED,
        "DoRecevieSendObject ReceiveObj Failed = %{public}d", MTP_ERROR_TRANSFER_FAILED);
    CHECK_AND_RETURN_RET_LOG(errorCode == MTP_SUCCESS, MTP_ERROR_RESPONSE_GENERAL,
        "DoRecevieSendObject ReceiveObj fail errorCode = %{public}d", errorCode);

    ModifyObjectInfo();
    errorCode = fsync(fd);
    PreDealFd(errorCode != MTP_SUCCESS, fd);
    CHECK_AND_RETURN_RET_LOG(errorCode == MTP_SUCCESS, MTP_ERROR_RESPONSE_GENERAL,
        "DoRecevieSendObject fsync eroor = %{public}d", errno);
    struct stat sstat;
    errorCode = fstat(fd, &sstat);
    PreDealFd(errorCode != MTP_SUCCESS, fd);
    CHECK_AND_RETURN_RET_LOG(errorCode == MTP_SUCCESS, MTP_ERROR_RESPONSE_GENERAL,
        "DoRecevieSendObject fstat error = %{public}d", errno);

    errorCode = MtpPtpProxy::GetInstance().CloseWriteFd(context_, fd);
    CHECK_AND_RETURN_RET_LOG(errorCode == MTP_SUCCESS, errorCode, "DoRecevieSendObject CloseFd fail!");

    return MTP_SUCCESS;
}

int32_t MtpOperationUtils::RecevieSendObject(MtpFileRange &object, int fd)
{
    MEDIA_DEBUG_LOG("RecevieSendObject begin");
    CHECK_AND_RETURN_RET_LOG(context_ != nullptr, MTP_INVALID_PARAMETER_CODE, "DoRecevieSendObject context_ is null");
    CHECK_AND_RETURN_RET_LOG(context_->mtpDriver != nullptr, MTP_INVALID_PARAMETER_CODE,
        "DoRecevieSendObject context_->mtpDriver is null");

    int32_t errorCode = context_->mtpDriver->ReceiveObj(object);
    bool cond = (errorCode != RECEVIE_OBJECT_CANCELLED && errorCode != RECEVIE_OBJECT_FAILED);
    CHECK_AND_RETURN_RET(!cond, errorCode);

    PreDealFd(errorCode != MTP_SUCCESS, fd);
    string filePath("");
    if (MtpManager::GetInstance().IsMtpMode()) {
        MtpPtpProxy::GetInstance().GetMtpPathById(context_->handle, filePath);
        CHECK_AND_RETURN_RET_LOG(!filePath.empty(), MTP_ERROR_TRANSFER_CANCELLED,
            "File path is invalid!");
        int ret = unlink(filePath.c_str());
        CHECK_AND_RETURN_RET_LOG(ret == 0, MTP_ERROR_TRANSFER_CANCELLED, "unlink file fail");
    }
    MtpPtpProxy::GetInstance().DeleteCanceledObject(filePath, context_->handle);
    CHECK_AND_RETURN_RET(errorCode != RECEVIE_OBJECT_FAILED, MTP_ERROR_TRANSFER_FAILED);
    return MTP_ERROR_TRANSFER_CANCELLED;
}

void MtpOperationUtils::PreDealFd(const bool deal, const int fd)
{
    if (!deal || fd <= 0) {
        return;
    }
    int32_t ret = MtpPtpProxy::GetInstance().CloseReadFd(context_, fd);
    CHECK_AND_PRINT_LOG(ret == MTP_SUCCESS, "DealFd CloseFd fail!");
}

uint16_t MtpOperationUtils::GetThumb(shared_ptr<PayloadData> &data, uint16_t containerType, int &errorCode)
{
    CHECK_AND_RETURN_RET_LOG(context_ != nullptr, MTP_INVALID_PARAMETER_CODE, "DoRecevieSendObject context_ is null");

    if (containerType != DATA_CONTAINER_TYPE) {
        data = make_shared<RespCommonData>();
        return CheckErrorCode(errorCode);
    }

    uint16_t ret = HasStorage(errorCode);
    CHECK_AND_RETURN_RET(ret == MTP_SUCCESS, CheckErrorCode(ret));

    shared_ptr<UInt8List> thumb = make_shared<UInt8List>();
    errorCode = MtpPtpProxy::GetInstance().GetThumb(context_, thumb);
    if (errorCode != MTP_SUCCESS) {
        data = make_shared<RespCommonData>();
        return CheckErrorCode(errorCode);
    }

    shared_ptr<GetThumbData> getThumb = make_shared<GetThumbData>();
    getThumb->SetThumb(thumb);
    data = getThumb;
    errorCode = MTP_SUCCESS;
    return CheckErrorCode(errorCode);
}

uint16_t MtpOperationUtils::SendObjectInfo(shared_ptr<PayloadData> &data, int &errorCode)
{
    if (context_ == nullptr) {
        MEDIA_ERR_LOG("MtpOperationUtils::SendObjectInfo param is null");
        data = make_shared<RespCommonData>();
        return CheckErrorCode(MTP_ERROR_CONTEXT_IS_NULL);
    }

    auto manager = MtpStorageManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(manager != nullptr, MTP_INVALID_PARAMETER_CODE, "MtpStorageManager instance is nullptr");

    // should reserve the space for the frame of the cmd 0x100d
    if (context_->sendObjectFileSize + READ_LEN > manager->GetFreeSize()) {
        data = make_shared<RespCommonData>();
        MEDIA_DEBUG_LOG("SendObjectInfo run out of memory, sendObjectFileSize %{public}d",
            context_->sendObjectFileSize);
        MEDIA_DEBUG_LOG("SendObjectInfo run out of memory, FreeSpaceInBytes %{public}"
            PRId64, manager->GetFreeSize());
        return MTP_STORE_FULL_CODE;
    }

    uint32_t storageID = 0;
    uint32_t parent = 0;
    uint32_t handle = 0;
    errorCode = MtpPtpProxy::GetInstance().SendObjectInfo(context_, storageID, parent, handle);
    if (errorCode != MTP_SUCCESS) {
        MEDIA_ERR_LOG("MtpOperationUtils::SendObjectInfo fail!");
        data = make_shared<RespCommonData>();
        return CheckErrorCode(errorCode);
    }
    context_->handle = handle;
    shared_ptr<SendObjectInfoData> sendObjectInfo = make_shared<SendObjectInfoData>();
    sendObjectInfo->SetSetParam(storageID, parent, handle);
    data = sendObjectInfo;
    return CheckErrorCode(errorCode);
}

uint16_t MtpOperationUtils::GetRespCommonData(shared_ptr<PayloadData> &data, int errorCode)
{
    data = make_shared<RespCommonData>();
    return CheckErrorCode(errorCode);
}

uint16_t MtpOperationUtils::GetPartialObject(shared_ptr<PayloadData> &data)
{
    CHECK_AND_RETURN_RET_LOG(context_ != nullptr, CheckErrorCode(MTP_ERROR_CONTEXT_IS_NULL),
        "GetPartialObject context_ is null");

    shared_ptr<GetPartialObjectData> getPartialObject = make_shared<GetPartialObjectData>(context_);
    getPartialObject->SetLength(context_->length);
    data = getPartialObject;
    return MTP_OK_CODE;
}

uint16_t MtpOperationUtils::GetObjectPropsSupported(shared_ptr<PayloadData> &data)
{
    CHECK_AND_RETURN_RET_LOG(context_ != nullptr, CheckErrorCode(MTP_ERROR_CONTEXT_IS_NULL),
        "GetObjectPropsSupported context_ is null");

    data = make_shared<GetObjectPropsSupportedData>(context_);
    return MTP_OK_CODE;
}

uint16_t MtpOperationUtils::DeleteObject(shared_ptr<PayloadData> &data, int &errorCode)
{
    if (context_ == nullptr) {
        errorCode = MTP_ERROR_CONTEXT_IS_NULL;
    } else {
        MEDIA_ERR_LOG("MtpOperationUtils::DeleteObject format=%{public}u", context_->format);
        errorCode = MtpPtpProxy::GetInstance().DeleteObject(context_);
    }
    data = make_shared<RespCommonData>();
    return CheckErrorCode(errorCode);
}

uint16_t MtpOperationUtils::MoveObject(shared_ptr<PayloadData> &data, int &errorCode)
{
    bool cond = (context_ == nullptr);
    CHECK_AND_RETURN_RET_LOG(!cond, CheckErrorCode(MTP_ERROR_CONTEXT_IS_NULL), "param is null");

    uint32_t repeatHandle = 0;
    errorCode = MtpPtpProxy::GetInstance().MoveObject(context_, repeatHandle);

    data = make_shared<RespCommonData>();
    if (repeatHandle != 0) {
        SendEventPacket(repeatHandle, MTP_EVENT_OBJECT_REMOVED_CODE);
        MEDIA_INFO_LOG("MTP:Send Event MTP_EVENT_OBJECT_REMOVED_CODE,repeatHandle[%{public}d]", repeatHandle);
    }
    return CheckErrorCode(errorCode);
}

uint16_t MtpOperationUtils::CopyObject(shared_ptr<PayloadData> &data, int &errorCode)
{
    if (context_ == nullptr) {
        MEDIA_ERR_LOG("param is null");
        data = make_shared<RespCommonData>();
        return CheckErrorCode(MTP_ERROR_CONTEXT_IS_NULL);
    }

    uint32_t objectHandle = 0;
    uint32_t oldHandle = 0;
    MtpPtpProxy::GetInstance().CopyObject(context_, objectHandle, oldHandle);

    shared_ptr<CopyObjectData> copyObject = make_shared<CopyObjectData>();
    copyObject->SetObjectHandle(objectHandle);
    data = copyObject;
    if (oldHandle != 0) {
        SendEventPacket(oldHandle, MTP_EVENT_OBJECT_REMOVED_CODE);
        MEDIA_INFO_LOG("MTP:Send Event MTP_EVENT_OBJECT_REMOVED_CODE,oldHandle[%{public}d]", oldHandle);
    }
    return CheckErrorCode(errorCode);
}

uint16_t MtpOperationUtils::GetStorageIDs(shared_ptr<PayloadData> &data, uint16_t containerType, int &errorCode)
{
    if (containerType != DATA_CONTAINER_TYPE || context_ == nullptr) {
        data = make_shared<RespCommonData>();
        return CheckErrorCode(MTP_ERROR_CONTEXT_IS_NULL);
    }

    CHECK_AND_RETURN_RET_LOG(context_->sessionOpen != false, CheckErrorCode(MTP_ERROR_SESSION_NOT_OPEN),
        "session isn't open");
    auto manager = MtpStorageManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(manager != nullptr, MTP_PARAMETER_NOT_SUPPORTED_CODE,
        "MtpStorageManager instance is nullptr");
    if (MtpManager::GetInstance().IsMtpMode()) {
        MtpStoreObserver::AttachContext(context_);
        MtpPtpProxy::GetInstance().GetMtpStorageIds();
    } else {
        auto storage = make_shared<Storage>();
        CHECK_AND_RETURN_RET_LOG(storage != nullptr, E_ERR, "storage is nullptr");
        storage->SetStorageID(DEFAULT_STORAGE_ID);
        storage->SetStorageType(MTP_STORAGE_FIXEDRAM);
        storage->SetFilesystemType(MTP_FILESYSTEM_GENERICHIERARCHICAL);
        storage->SetAccessCapability(MTP_ACCESS_READ_WRITE);
        storage->SetMaxCapacity(manager->GetTotalSize(PUBLIC_DOC));
        storage->SetFreeSpaceInBytes(manager->GetFreeSize(PUBLIC_DOC));
        storage->SetFreeSpaceInObjects(0);
        storage->SetStorageDescription(manager->GetStorageDescription(MTP_STORAGE_FIXEDRAM));
        manager->AddStorage(storage);
    }

    shared_ptr<GetStorageIdsData> getStorageIdsData = make_shared<GetStorageIdsData>();
    getStorageIdsData->SetStorages(manager->GetStorages());
    data = getStorageIdsData;
    errorCode = MTP_SUCCESS;
    return CheckErrorCode(errorCode);
}

uint16_t MtpOperationUtils::GetStorageInfo(shared_ptr<PayloadData> &data, uint16_t containerType, int &errorCode)
{
    if (containerType != DATA_CONTAINER_TYPE) {
        data = make_shared<RespCommonData>();
        return CheckErrorCode(errorCode);
    }

    CHECK_AND_RETURN_RET_LOG(context_->sessionOpen != false, CheckErrorCode(MTP_ERROR_SESSION_NOT_OPEN),
        "session isn't open error");

    shared_ptr<Storage> storage = MtpStorageManager::GetInstance()->GetStorage(context_->storageInfoID);
    CHECK_AND_RETURN_RET_LOG(storage != nullptr, CheckErrorCode(MTP_ERROR_INVALID_STORAGE_ID),
        "invalid storage id error");
    shared_ptr<GetStorageInfoData> getStorageInfoData = make_shared<GetStorageInfoData>();
    getStorageInfoData->SetStorage(storage);
    data = getStorageInfoData;
    errorCode = MTP_SUCCESS;
    return MTP_OK_CODE;
}

uint16_t MtpOperationUtils::GetOpenSession(shared_ptr<PayloadData> &data, int errorCode)
{
    shared_ptr<OpenSessionData> openSessionData = make_shared<OpenSessionData>(context_);
    uint16_t respCode = CheckErrorCode(errorCode);
    if (respCode == MTP_SESSION_ALREADY_OPEN_CODE) {
        openSessionData->SetSessionId(context_->sessionID);
    }
    if (respCode == MTP_OK_CODE) {
        context_->sessionOpen = true;
    }
    data = openSessionData;
    return respCode;
}

uint16_t MtpOperationUtils::CheckErrorCode(int errorCode)
{
    switch (errorCode) {
        case MTP_ERROR_PACKET_INCORRECT:
            return MTP_INVALID_PARAMETER_CODE;
        case MTP_ERROR_SESSION_ALREADY_OPEN:
            return MTP_SESSION_ALREADY_OPEN_CODE;
        case MTP_ERROR_NO_THIS_FILE:
            return MTP_INVALID_OBJECTHANDLE_CODE;
        case MTP_ERROR_INCOMPLETE_TRANSFER:
            return MTP_INCOMPLETE_TRANSFER_CODE;
        case MTP_ERROR_SESSION_NOT_OPEN:
            return MTP_SESSION_NOT_OPEN_CODE;
        case MTP_ERROR_INVALID_STORAGE_ID:
            return MTP_INVALID_STORAGEID_CODE;
        case MTP_ERROR_INVALID_OBJECTHANDLE:
            return MTP_INVALID_OBJECTHANDLE_CODE;
        case MTP_ERROR_DEVICEPROP_NOT_SUPPORTED:
            return MTP_DEVICEPROP_NOT_SUPPORTED_CODE;
        case MTP_ERROR_STORE_NOT_AVAILABLE:
            return MTP_STORE_NOT_AVAILABLE_CODE;
        case MTP_ERROR_INVALID_PARENTOBJECT:
            return MTP_INVALID_PARENTOBJECT_CODE;
        case MTP_ERROR_PARAMETER_NOT_SUPPORTED:
            return MTP_PARAMETER_NOT_SUPPORTED_CODE;
        case MTP_ERROR_INVALID_OBJECTPROP_VALUE:
            return MTP_INVALID_OBJECTPROP_VALUE_CODE;
        case MTP_ERROR_INVALID_OBJECTPROP_FORMAT:
            return MTP_INVALID_OBJECTPROP_FORMAT_CODE;
        case MTP_ERROR_INVALID_OBJECTPROPCODE:
            return MTP_INVALID_OBJECTPROPCODE_CODE;
        case MTP_ERROR_ACCESS_DENIED:
            return MTP_ACCESS_DENIED_CODE;
        case MTP_ERROR_SPECIFICATION_BY_GROUP_UNSUPPORTED:
            return MTP_SPECIFICATION_BY_GROUP_UNSUPPORTED_CODE;
        case MTP_ERROR_SPECIFICATION_BY_DEPTH_UNSUPPORTED:
            return MTP_SPECIFICATION_BY_DEPTH_UNSUPPORTED_CODE;
        case MTP_ERROR_TRANSFER_FAILED:
            return MTP_STORE_FULL_CODE;
        default:
            return MTP_OK_CODE;
    }
}

uint16_t MtpOperationUtils::GetCloseSession(shared_ptr<PayloadData> &data)
{
    data = make_shared<CloseSessionData>(context_);
    return MTP_OK_CODE;
}

uint16_t MtpOperationUtils::GetPropDesc(shared_ptr<PayloadData> &data, uint16_t containerType, int &errorCode)
{
    if (containerType != DATA_CONTAINER_TYPE) {
        data = make_shared<RespCommonData>();
        return CheckErrorCode(errorCode);
    }

    shared_ptr<GetDevicePropDescData> devicePropDescData = make_shared<GetDevicePropDescData>();
    shared_ptr<Property> property = nullptr;
    switch (context_->property) {
        case MTP_DEVICE_PROPERTY_SYNCHRONIZATION_PARTNER_CODE:
            property = make_shared<Property>(context_->property, MTP_DEVICE_PROP_DESC_TYPE_STR, true);
            break;
        case MTP_DEVICE_PROPERTY_DEVICE_FRIENDLY_NAME_CODE:
            property = make_shared<Property>(context_->property, MTP_DEVICE_PROP_DESC_TYPE_STR, true);
            property->currentValue->str_ = make_shared<string>(GetPropertyInner("const.product.name",
                DEFAULT_PRODUCT_NAME));
            break;
        case MTP_DEVICE_PROPERTY_SESSION_INITIATOR_VERSION_INFO_CODE:
            property = make_shared<Property>(context_->property, MTP_DEVICE_PROP_DESC_TYPE_STR, true);
            break;
        case MTP_DEVICE_PROPERTY_IMAGE_SIZE_CODE:
            property = make_shared<Property>(context_->property, MTP_DEVICE_PROP_DESC_TYPE_STR, true);
            break;
        case MTP_DEVICE_PROPERTY_BATTERY_LEVEL_CODE:
            property = make_shared<Property>(context_->property, MTP_DEVICE_PROP_DESC_TYPE_UINT8);
            property->currentValue->bin_.ui8 = (uint8_t)MtpOperationUtils::GetBatteryLevel();
            property->SetFormRange(BATTERY_LEVEL_MIN, BATTERY_LEVEL_MAX, BATTERY_LEVEL_STEP);
            break;
        case MTP_DEVICE_PROPERTY_PERCEIVED_DEVICE_TYPE_CODE:
            property = make_shared<Property>(context_->property, MTP_DEVICE_PROP_DESC_TYPE_UINT32);
            break;
        default:
            MEDIA_INFO_LOG("property do not find");
            break;
    }

    devicePropDescData->SetProperty(property);
    data = devicePropDescData;
    errorCode = MTP_SUCCESS;
    return MTP_OK_CODE;
}

uint16_t MtpOperationUtils::GetPropValue(shared_ptr<PayloadData> &data, uint16_t containerType, int &errorCode)
{
    if (containerType != DATA_CONTAINER_TYPE) {
        data = make_shared<RespCommonData>();
        return CheckErrorCode(errorCode);
    }

    shared_ptr<GetDevicePropValueData> devicePropValueData = make_shared<GetDevicePropValueData>();
    shared_ptr<Property::Value> value = make_shared<Property::Value>();
    uint16_t valueType = MTP_DEVICE_PROP_DESC_TYPE_UNDEFINED;
    switch (context_->property) {
        case MTP_DEVICE_PROPERTY_SYNCHRONIZATION_PARTNER_CODE:
            valueType = MTP_DEVICE_PROP_DESC_TYPE_STR;
            value->str_ = make_shared<string>("");
            break;
        case MTP_DEVICE_PROPERTY_DEVICE_FRIENDLY_NAME_CODE:
            valueType = MTP_DEVICE_PROP_DESC_TYPE_STR;
            value->str_ = make_shared<string>(GetPropertyInner("const.product.name", DEFAULT_PRODUCT_NAME));
            break;
        case MTP_DEVICE_PROPERTY_SESSION_INITIATOR_VERSION_INFO_CODE:
            valueType = MTP_DEVICE_PROP_DESC_TYPE_STR;
            value->str_ = make_shared<string>("");
            break;
        case MTP_DEVICE_PROPERTY_IMAGE_SIZE_CODE:
            valueType = MTP_DEVICE_PROP_DESC_TYPE_STR;
            value->str_ = make_shared<string>("");
            break;
        case MTP_DEVICE_PROPERTY_BATTERY_LEVEL_CODE:
            valueType = MTP_DEVICE_PROP_DESC_TYPE_UINT8;
            value->bin_.ui8 = (uint8_t)MtpOperationUtils::GetBatteryLevel();
            break;
        case MTP_DEVICE_PROPERTY_PERCEIVED_DEVICE_TYPE_CODE:
            valueType = MTP_DEVICE_PROP_DESC_TYPE_UINT32;
            value->bin_.ui32 = MTP_PERCEIVED_DEVICE_TYPE_GENERIC;
            break;
        default:
            MEDIA_INFO_LOG("property do not find");
            break;
    }

    devicePropValueData->SetValue(valueType, value);
    data = devicePropValueData;
    errorCode = MTP_SUCCESS;
    return MTP_OK_CODE;
}

uint16_t MtpOperationUtils::SetDevicePropValueResp(shared_ptr<PayloadData> &data)
{
    data = make_shared<RespCommonData>();
    return MTP_OK_CODE;
}

uint16_t MtpOperationUtils::ResetDevicePropResp(shared_ptr<PayloadData> &data)
{
    CHECK_AND_PRINT_LOG(SetPropertyInner("const.product.name", DEFAULT_PRODUCT_NAME), "SetPropertyInner fail");
    data = make_shared<RespCommonData>();
    return MTP_OK_CODE;
}

uint16_t MtpOperationUtils::ObjectEvent(shared_ptr<PayloadData> &data, const int32_t payload)
{
    std::shared_ptr<ObjectEventData> eventData = make_shared<ObjectEventData>();
    eventData->SetPayload(payload);
    data = eventData;
    return MTP_OK_CODE;
}

uint16_t MtpOperationUtils::GetPathByHandle(const uint32_t &handle, string &path, string &realPath)
{
    MtpPtpProxy::GetInstance().GetPathByHandle(handle, path, realPath);
    return MTP_OK_CODE;
}

int32_t MtpOperationUtils::GetHandleByPaths(string path, uint32_t &handle)
{
    CHECK_AND_RETURN_RET(!path.empty(), MTP_UNDEFINED_CODE);
    if (path.back() == '/') {
        path.pop_back();
    }
    return MtpPtpProxy::GetInstance().GetIdByPath(path, handle);
}

bool MtpOperationUtils::TryAddExternalStorage(const std::string &fsUuid, uint32_t &storageId)
{
    CHECK_AND_RETURN_RET(!MtpManager::GetInstance().IsMtpMode(),
        MtpPtpProxy::GetInstance().MtpTryAddExternalStorage(fsUuid, storageId));
    return false;
}

bool MtpOperationUtils::TryRemoveExternalStorage(const std::string &fsUuid, uint32_t &storageId)
{
    CHECK_AND_RETURN_RET(!MtpManager::GetInstance().IsMtpMode(),
        MtpPtpProxy::GetInstance().MtpTryRemoveExternalStorage(fsUuid, storageId));
    return false;
}

int32_t MtpOperationUtils::GetBatteryLevel()
{
#ifdef HAS_BATTERY_MANAGER_PART
    auto &batterySrvClient = PowerMgr::BatterySrvClient::GetInstance();
    int32_t capacity = batterySrvClient.GetCapacity();
    bool cond = (capacity > MAX_BATTERY || capacity < EMPTY_BATTERY);
    CHECK_AND_RETURN_RET(!cond, ERROR_BATTERY);
    return capacity;
#else
    return EMPTY_BATTERY;
#endif
}

std::string MtpOperationUtils::GetPropertyInner(const std::string &property, const std::string &defValue)
{
    return OHOS::system::GetParameter(property, defValue);
}

bool MtpOperationUtils::SetPropertyInner(const std::string &property, const std::string &defValue)
{
    return OHOS::system::SetParameter(property, defValue);
}

void MtpOperationUtils::SetIsDevicePropSet()
{
    g_isDevicePropSet = true;
}

} // namespace Media
} // namespace OHOS
