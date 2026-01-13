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
#ifndef FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_OPERATION_H_
#define FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_OPERATION_H_
#include <memory>
#include <vector>
#include "mtp_driver.h"
#include "mtp_operation_context.h"
#include "mtp_operation_utils.h"
#include "mtp_packet.h"
#include "payload_data.h"
namespace OHOS {
namespace Media {

class MtpOperation {
public:
    MtpOperation();
    ~MtpOperation() = default;
    int32_t Execute();
    void Stop();

private:
    void Init();
    void ResetOperation();
    uint16_t GetPayloadData(std::shared_ptr<MtpOperationContext> &context,
        std::shared_ptr<PayloadData> &data, uint16_t containerType, int &errorCode);
    uint16_t GetPayloadDataSub(std::shared_ptr<MtpOperationContext> &context,
        std::shared_ptr<PayloadData> &data, uint16_t containerType, int &errorCode);
    uint16_t GetPayloadDataMore(std::shared_ptr<MtpOperationContext> &context,
        std::shared_ptr<PayloadData> &data, uint16_t containerType, int &errorCode);
    void ReceiveRequestPacket(int &errorCode);
    void SendObjectData(int &errorCode);
    void RecevieObjectData(int &errorCode);
    void SendMakeResponsePacket(int &errorCode);
    void ReceiveI2Rdata(int &errorCode);
    void SendR2Idata(int &errorCode);
    void AddStorage(std::shared_ptr<Storage> &storage);
    void RemoveStorage(std::shared_ptr<Storage> &storage);
    void DealRequest(uint16_t operationCode, int &errorCode);
    
private:
    std::shared_ptr<MtpOperationContext> mtpContextPtr_;
    std::shared_ptr<MtpPacket> requestPacketPtr_;
    std::shared_ptr<MtpPacket> dataPacketPtr_;
    std::shared_ptr<MtpPacket> responsePacketPtr_;
    std::shared_ptr<MtpOperationUtils> operationUtils_;
    std::shared_ptr<MtpDriver> mtpDriver_;
    std::shared_ptr<PayloadData> dataPayloadData_;
    uint16_t responseCode_;
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_OPERATION_H_
