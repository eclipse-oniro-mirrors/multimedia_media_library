/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
#ifndef OHOS_MEDIA_FILE_MANAGER_SCAN_RULE_CONFIG_H
#define OHOS_MEDIA_FILE_MANAGER_SCAN_RULE_CONFIG_H

#include <string_view>

#include "scan_rule_config.h"

namespace OHOS::Media {
constexpr std::string_view FILE_MANAGER_ROOT_PATH = "/storage/media/local/files/Docs";
constexpr std::string_view FILE_MANAGER_SCAN_PATH = "/storage/media/local/files/Docs/Download";
const ScanRuleConfig &GetFileManagerScanRuleConfig();
} // namespace OHOS::Media

#endif // OHOS_MEDIA_FILE_MANAGER_SCAN_RULE_CONFIG_H