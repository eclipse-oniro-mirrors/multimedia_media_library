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

#ifndef SCANNER_CONTEXT_CONVERTER_H
#define SCANNER_CONTEXT_CONVERTER_H

#include <memory>

#include "media_scanner.h"
#include "scan_task_deduplicator.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class EXPORT ScannerContextConverter {
public:
    static std::unique_ptr<MediaScannerObj> Convert(const ScanTaskContext& context);
    static MediaScannerObj::ScanType DetermineScanType(const ScanTaskContext& context);
};
} // namespace Media
} // namespace OHOS
#endif // SCANNER_CONTEXT_CONVERTER_H