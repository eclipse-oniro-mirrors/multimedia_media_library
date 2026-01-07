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
#ifndef FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_PAYLOAD_DATA_H_
#define FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_PAYLOAD_DATA_H_
#include <memory>
#include <stdint.h>
#include <vector>
#include "mtp_operation_context.h"
namespace OHOS {
namespace Media {
class PayloadData {
public:
    explicit PayloadData(std::shared_ptr<MtpOperationContext> &context);
    explicit PayloadData();
    virtual ~PayloadData();

    virtual int Parser(const std::vector<uint8_t> &buffer, int32_t readSize) = 0;
    virtual int Maker(std::vector<uint8_t> &outBuffer) = 0;

    virtual uint32_t CalculateSize() = 0;

protected:
    std::shared_ptr<MtpOperationContext> context_;
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_PAYLOAD_DATA_H_
