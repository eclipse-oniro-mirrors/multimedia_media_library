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

#ifndef OHOS_MEDIA_BACKUP_LOG_CONST_H
#define OHOS_MEDIA_BACKUP_LOG_CONST_H

#include <string>
#include <unordered_map>

#include "backup_const.h"

namespace OHOS::Media {
const uint32_t ON_PROCESS_INTV = 5;
const uint32_t LOG_PROGRESS_INTV = 2 * 60; // 2min
const uint32_t LOG_TIMEOUT_INTV = 60 * 60; // 1h
} // namespace OHOS::Media

#endif  // OHOS_MEDIA_BACKUP_LOG_CONST_H