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
#ifndef FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_SEND_OBJECT_EVENT_H_
#define FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_SEND_OBJECT_EVENT_H_
#include "payload_data.h"
namespace OHOS {
namespace Media {
class ObjectEventData : public PayloadData {
public:
    explicit ObjectEventData(std::shared_ptr<MtpOperationContext> &context);
    ObjectEventData();
    ~ObjectEventData() override;

    int Parser(const std::vector<uint8_t> &buffer, int32_t readSize) override;
    int Maker(std::vector<uint8_t> &outBuffer) override;
    void SetPayload(const int32_t payload);
    uint32_t CalculateSize() override;
private:
    int32_t payload_ {0};
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_SEND_OBJECT_EVENT_H_
