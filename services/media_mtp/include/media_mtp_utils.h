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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MTP_SERVICE_INCLUDE_MEDIA_MTP_UTILS_H_
#define FRAMEWORKS_SERVICES_MEDIA_MTP_SERVICE_INCLUDE_MEDIA_MTP_UTILS_H_
#include <stdint.h>
#include <string>
#include "medialibrary_errno.h"

namespace OHOS {
namespace Media {
const uint16_t UNDEFINED_CONTAINER_TYPE = 0;
const uint16_t COMMAND_CONTAINER_TYPE = 1;
const uint16_t DATA_CONTAINER_TYPE = 2;
const uint16_t RESPONSE_CONTAINER_TYPE = 3;
const uint16_t EVENT_CONTAINER_TYPE = 4;

const uint32_t PACKET_HEADER_LENGETH = 12;
const uint32_t READ_DATA_BUFFER_MAX_SIZE = 16384;
const uint32_t READ_BUFFER_MAX_SIZE = 512;
constexpr uint32_t COMMON_PHOTOS_OFFSET = 100000000;
constexpr uint32_t EDITED_PHOTOS_OFFSET = 200000000;
constexpr uint32_t COMMON_MOVING_OFFSET = 300000000;
constexpr uint32_t EDITED_MOVING_OFFSET = 400000000;
const std::string DEFAULT_PRODUCT_NAME = "OpenHarmony Device";
const std::string DEFAULT_PRODUCT_MODEL = "ohos";
const std::string DEFAULT_PRODUCT_MANUFACTURER = "default";
const std::string DEFAULT_PRODUCT_HARDWARE_VERSION = "default";
const std::string DEFAULT_PRODUCT_SOFTWARE_VERSION = "OpenHarmony 3.2";
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MTP_SERVICE_INCLUDE_MEDIA_MTP_UTILS_H_
