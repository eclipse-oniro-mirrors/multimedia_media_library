/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#ifndef OHOS_MTP_STORE_OBSERVER_H
#define OHOS_MTP_STORE_OBSERVER_H

#include <memory>
#include "common_event_subscriber.h"
#include "mtp_operation_context.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class MtpStoreObserver : public EventFwk::CommonEventSubscriber {
public:
    EXPORT virtual ~MtpStoreObserver() = default;
    EXPORT explicit MtpStoreObserver(const EventFwk::CommonEventSubscribeInfo &subscriberInfo);

    EXPORT static bool StartObserver();
    EXPORT static bool StopObserver();
    EXPORT static void AttachContext(const std::shared_ptr<MtpOperationContext> &context);
    EXPORT void OnReceiveEvent(const EventFwk::CommonEventData &eventData) override;
};
} // namespace Media
} // namespace OHOS
#endif // OHOS_MTP_STORE_OBSERVER_H
