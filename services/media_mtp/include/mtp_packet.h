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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_PACKET_H_
#define FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_PACKET_H_

#include <memory>
#include <stdint.h>
#include <vector>

#include "header_data.h"
#include "mtp_driver.h"
#include "payload_data.h"

namespace OHOS {
namespace Media {
class MtpPacket {
public:
    explicit MtpPacket(std::shared_ptr<MtpOperationContext> &context);
    MtpPacket(std::shared_ptr<MtpOperationContext> &context, const std::shared_ptr<MtpDriver> &mtpDriver);
    ~MtpPacket();
    int Parser();
    int Maker(bool isPayload);

    int Read();
    int Write(int32_t &result);
    void Init(std::shared_ptr<HeaderData> &headerData);
    void Init(std::shared_ptr<HeaderData> &headerData, std::shared_ptr<PayloadData> &payloadData);
    void Reset();
    void Stop();

    uint16_t GetOperationCode();
    uint32_t GetTransactionId();
    uint32_t GetSessionID();

    static bool IsNeedDataPhase(uint16_t operationCode);
    static bool IsI2R(uint16_t operationCode);
private:
    int ParserHead();
    int ParserPayload();
    int ParserPayloaddata();
    int MakeHead();
    int MakerPayload();

    std::shared_ptr<MtpOperationContext> context_;
    uint32_t readSize_ {0};
    uint32_t readBufSize_ {0};
    uint32_t writeSize_ {0};
    std::shared_ptr<HeaderData> headerData_;
    std::shared_ptr<PayloadData> payloadData_;
    std::shared_ptr<MtpDriver> mtpDriver_;
    std::vector<uint8_t> readBuffer_;
    std::vector<uint8_t> writeBuffer_;
};
} // namespace Media
} // namespace OHOS

#endif  // FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_PACKET_H_
