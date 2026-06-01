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

#define MLOG_TAG "DefaultScanStrategy"

#include "default_scan_strategy.h"

#include "converter/scanner_context_converter.h"
#include "media_log.h"
#include "media_scanner.h"
#include "medialibrary_errno.h"
#include "scan_config.h"

namespace OHOS {
namespace Media {

DefaultScanStrategy::DefaultScanStrategy()
{
    MEDIA_INFO_LOG("DefaultScanStrategy created");
}

DefaultScanStrategy::~DefaultScanStrategy()
{
    MEDIA_INFO_LOG("DefaultScanStrategy destroyed");
}

int32_t DefaultScanStrategy::Scan(const std::shared_ptr<ScanTaskContext>& context)
{
    if (!ValidateContext(context)) {
        MEDIA_ERR_LOG("Scan: context validation failed");
        return E_ERR;
    }

    MEDIA_INFO_LOG("validated (fileId %{public}d, path %{private}s)",
        context->config.GetFileId(), context->config.GetFilePath().c_str());

    auto scanner = CreateScanner(context);
    if (scanner == nullptr) {
        MEDIA_ERR_LOG("Scan: scanner creation failed");
        return E_ERR;
    }

    scanner->Scan();

    MEDIA_INFO_LOG("completed (fileId %{public}d)", context->config.GetFileId());
    return E_OK;
}

ScanStrategyType DefaultScanStrategy::GetStrategyType() const
{
    return ScanStrategyType::DEFAULT_SCAN;
}

bool DefaultScanStrategy::ValidateContext(const std::shared_ptr<ScanTaskContext>& context)
{
    if (context == nullptr) {
        MEDIA_ERR_LOG("ValidateContext: context is null");
        return false;
    }

    if (context->config.GetFilePath().empty()) {
        MEDIA_ERR_LOG("ValidateContext: filePath is empty");
        return false;
    }

    return true;
}

std::unique_ptr<MediaScannerObj> DefaultScanStrategy::CreateScanner(
    const std::shared_ptr<ScanTaskContext>& context)
{
    auto scanner = ScannerContextConverter::Convert(*context);
    if (scanner == nullptr) {
        MEDIA_ERR_LOG("CreateScanner: converter failed");
        return nullptr;
    }

    MEDIA_DEBUG_LOG("CreateScanner: scanner created");
    return scanner;
}

} // namespace Media
} // namespace OHOS