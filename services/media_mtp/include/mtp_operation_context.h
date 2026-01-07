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
#ifndef FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_CONTEXT_H_
#define FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_CONTEXT_H_
#include <memory>
#include <stdint.h>
#include "mtp_constants.h"
#include "storage.h"
#include "mtp_driver.h"
namespace OHOS {
namespace Media {
struct MtpOperationContext {
    uint16_t operationCode {0};
    uint32_t transactionID {0};
    uint32_t devicePropertyCode {0};
    uint32_t storageID {0};
    uint16_t format {0};
    uint32_t parent {0};
    uint32_t handle {0};
    uint32_t property {0};
    uint32_t groupCode {0};
    uint32_t depth {0};
    int properType = MTP_TYPE_UNDEFINED_CODE;
    std::string properStrValue;
    int64_t properIntValue {0};
    std::shared_ptr<UInt32List> handles;
    uint32_t sendObjectFileSize {0};
    std::string name;
    std::string created;
    std::string modified;
    uint64_t offset {0};
    uint32_t length {0};

    bool indata {false};
    uint32_t storageInfoID {0};

    bool sessionOpen {false};
    uint32_t sessionID {0};
    std::shared_ptr<MtpDriver> mtpDriver;
    uint32_t tempSessionID {0};
    uint32_t eventHandle {0};
    uint32_t eventProperty {0};
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_CONTEXT_H_
