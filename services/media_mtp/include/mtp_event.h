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
#ifndef FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_EVENT_H_
#define FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_EVENT_H_
#include <string>
#include <vector>
#include "mtp_operation_context.h"
#include "mtp_operation_utils.h"
#include "payload_data.h"

namespace OHOS {
namespace Media {
class MtpEvent {
public:
    explicit MtpEvent(const std::shared_ptr<MtpOperationContext> &context);
    ~MtpEvent();
    void SendObjectAdded(const std::string &path);
    void SendObjectRemoved(const std::string &path);
    void SendObjectRemovedByHandle(uint32_t handle);
    void SendObjectInfoChanged(const std::string &path);
    void SendDevicePropertyChanged();
    void SendStoreAdded(const std::string &fsUuid);
    void SendStoreRemoved(const std::string &fsUuid);

private:
    std::shared_ptr<MtpOperationContext> mtpContextPtr_;
    std::shared_ptr<MtpOperationUtils> handleptr_;
    uint16_t EventPayloadData(const uint16_t code, std::shared_ptr<PayloadData> &data);
    void SendEvent(const int32_t &code);
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_EVENT_H_
