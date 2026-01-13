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
#ifndef FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_PACKET_PAYLOAD_FACTORY_H_
#define FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_PACKET_PAYLOAD_FACTORY_H_
#include <stdint.h>
#include "payload_data.h"
namespace OHOS {
namespace Media {
class PacketPayloadFactory {
public:
    PacketPayloadFactory();
    ~PacketPayloadFactory();

    static std::shared_ptr<PayloadData> CreatePayload(std::shared_ptr<MtpOperationContext> &context,
        const uint16_t code, const uint16_t stage);

private:
    static std::shared_ptr<PayloadData> CreatePayloadMore(std::shared_ptr<MtpOperationContext> &context,
        const uint16_t code, const uint16_t stage);
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_PACKET_PAYLOAD_FACTORY_H_
