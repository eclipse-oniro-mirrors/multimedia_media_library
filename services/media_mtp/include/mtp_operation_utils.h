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
#ifndef FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_OPERATION_UTILS_H_
#define FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_OPERATION_UTILS_H_
#include <memory>
#include <vector>
#include "mtp_operation_context.h"
#include "payload_data.h"
namespace OHOS {
namespace Media {
class MtpOperationUtils {
public:
    explicit MtpOperationUtils(const std::shared_ptr<MtpOperationContext> &context, bool isInit = false);
    ~MtpOperationUtils();

    uint16_t GetRespCommonData(std::shared_ptr<PayloadData> &data, int errorCode);
    uint16_t GetDeviceInfo(std::shared_ptr<PayloadData> &data, uint16_t containerType, int &errorCode);
    uint16_t GetNumObjects(std::shared_ptr<PayloadData> &data);
    uint16_t GetObjectHandles(std::shared_ptr<PayloadData> &data, uint16_t containerType, int &errorCode);
    uint16_t GetObjectInfo(std::shared_ptr<PayloadData> &data, uint16_t containerType, int &errorCode);
    uint16_t GetObjectPropDesc(std::shared_ptr<PayloadData> &data, uint16_t containerType, int &errorCode);
    uint16_t GetObjectPropValue(std::shared_ptr<PayloadData> &data, uint16_t containerType, int &errorCode);
    void DoSetObjectPropValue(int &errorCode);
    uint16_t GetObjectPropList(std::shared_ptr<PayloadData> &data, uint16_t containerType, int &errorCode);
    uint16_t GetObjectReferences(std::shared_ptr<PayloadData> &data, uint16_t containerType, int &errorCode);
    uint16_t SetObjectReferences(std::shared_ptr<PayloadData> &data);
    uint16_t GetObjectDataDeal();
    uint16_t GetObject(std::shared_ptr<PayloadData> &data, int errorCode);
    int32_t DoRecevieSendObject();
    uint16_t GetThumb(std::shared_ptr<PayloadData> &data, uint16_t containerType, int &errorCode);
    uint16_t SendObjectInfo(std::shared_ptr<PayloadData> &data, int &errorCode);
    uint16_t GetPartialObject(std::shared_ptr<PayloadData> &data);
    uint16_t GetObjectPropsSupported(std::shared_ptr<PayloadData> &data);
    uint16_t DeleteObject(std::shared_ptr<PayloadData> &data, int &errorCode);
    uint16_t MoveObject(std::shared_ptr<PayloadData> &data, int &errorCode);
    uint16_t CopyObject(std::shared_ptr<PayloadData> &data, int &errorCode);
    uint16_t GetStorageIDs(std::shared_ptr<PayloadData> &data, uint16_t containerType, int &errorCode);
    uint16_t GetStorageInfo(std::shared_ptr<PayloadData> &data, uint16_t containerType, int &errorCode);
    uint16_t GetOpenSession(std::shared_ptr<PayloadData> &data, int errorCode);
    uint16_t GetCloseSession(std::shared_ptr<PayloadData> &data);
    uint16_t GetPropDesc(std::shared_ptr<PayloadData> &data, uint16_t containerType, int &errorCode);
    uint16_t GetPropValue(std::shared_ptr<PayloadData> &data, uint16_t containerType, int &errorCode);
    uint16_t SetDevicePropValueResp(std::shared_ptr<PayloadData> &data);
    uint16_t ResetDevicePropResp(std::shared_ptr<PayloadData> &data);
    uint16_t ObjectEvent(std::shared_ptr<PayloadData> &data, const int32_t payload);
    uint16_t GetPathByHandle(const uint32_t &handle, std::string &path, std::string &realPath);
    int32_t GetHandleByPaths(std::string path, uint32_t &handle);
    bool TryAddExternalStorage(const std::string &fsUuid, uint32_t &storageId);
    bool TryRemoveExternalStorage(const std::string &fsUuid, uint32_t &storageId);
    static int32_t GetBatteryLevel();
    static std::string GetPropertyInner(const std::string &property, const std::string &defValue);
    static bool SetPropertyInner(const std::string &property, const std::string &value);
    static void SetIsDevicePropSet();

private:
    uint16_t CheckErrorCode(int errorCode);
    void PreDealFd(const bool deal, const int fd);
    void SendEventPacket(uint32_t objectHandle, uint16_t eventCode);
    uint16_t HasStorage(int &errorCode);
    int32_t RecevieSendObject(MtpFileRange &object, int fd);
    void ModifyObjectInfo();

    std::shared_ptr<MtpOperationContext> context_;
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_OPERATION_UTILS_H_
