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
#ifndef FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_HEADER_DATA_H_
#define FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_HEADER_DATA_H_
#include <stdint.h>
#include <vector>
#include "mtp_operation_context.h"
namespace OHOS {
namespace Media {
class HeaderData {
public:
    HeaderData(std::shared_ptr<MtpOperationContext> &context);
    HeaderData(uint16_t containerType, uint16_t code, uint32_t transactionID);
    ~HeaderData();
    int Parser(std::vector<uint8_t> &buffer, uint32_t readSize);
    int Maker(std::vector<uint8_t> &outBuffer);
    uint16_t GetCode() const;
    void SetCode(uint16_t code);
    uint32_t GetContainerLength() const;
    void SetContainerLength(uint32_t containerLength);
    uint16_t GetContainerType() const;
    void SetContainerType(uint16_t containerType);
    uint32_t GetTransactionId() const;
    void SetTransactionId(uint32_t transactionId);
    void Reset();
    
    static uint32_t sTransactionID_;
private:
    std::shared_ptr<MtpOperationContext> context_;
    uint32_t containerLength_ {0};
    uint16_t containerType_ {0};
    uint16_t code_ {0};
    uint32_t transactionID_ {0};
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_PACKET_HEADER_H_
