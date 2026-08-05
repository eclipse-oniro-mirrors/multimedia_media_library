/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "CustomRestoreStrategy"

#include "custom_restore_strategy.h"

#include "custom_restore_scanner_obj.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "scan_task_context.h"

// LCOV_EXCL_START
namespace OHOS::Media {

ScanStrategyType CustomRestoreStrategy::GetStrategyType() const
{
    return ScanStrategyType::CUSTOM_RESTORE_SCAN;
}

bool CustomRestoreStrategy::ValidateCustomRestoreContext(const std::shared_ptr<ScanTaskContext> &context)
{
    if (context == nullptr) {
        MEDIA_ERR_LOG("ValidateCustomRestoreContext: context is nullptr");
        return false;
    }
    if (context->config.GetCustomRestoreInfo().GetFilePaths().empty()) {
        MEDIA_ERR_LOG("ValidateCustomRestoreContext: filePaths is empty");
        return false;
    }
    return true;
}

int32_t CustomRestoreStrategy::Scan(const std::shared_ptr<ScanTaskContext> &context)
{
    if (!ValidateCustomRestoreContext(context)) {
        return E_ERR;
    }

    auto scannerObj = CreateScannerObj(context);
    if (scannerObj == nullptr) {
        MEDIA_ERR_LOG("Scan: scannerObj creation failed");
        return E_ERR;
    }

    return scannerObj->Execute();
}

std::unique_ptr<CustomRestoreScannerObj> CustomRestoreStrategy::CreateScannerObj(
    const std::shared_ptr<ScanTaskContext> &context)
{
    return std::make_unique<CustomRestoreScannerObj>(context->config.GetCustomRestoreInfo());
}

} // namespace OHOS::Media
// LCOV_EXCL_STOP
