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
 
#define MLOG_TAG "BatchScanStrategy"
 
#include "batch_scan_strategy.h"
 
#include "batch_scanner_obj.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "scan_task_context.h"
 
// LCOV_EXCL_START
namespace OHOS::Media {
 
ScanStrategyType BatchScanStrategy::GetStrategyType() const
{
    return ScanStrategyType::CUSTOM_RESTORE_SCAN;
}
 
bool BatchScanStrategy::ValidateBatchContext(const std::shared_ptr<ScanTaskContext> &context)
{
    if (context == nullptr) {
        MEDIA_ERR_LOG("ValidateBatchContext: context is nullptr");
        return false;
    }
    if (context->config.GetCustomRestoreInfo().GetFilePaths().empty()) {
        MEDIA_ERR_LOG("ValidateBatchContext: filePaths is empty");
        return false;
    }
    return true;
}
 
int32_t BatchScanStrategy::Scan(const std::shared_ptr<ScanTaskContext> &context)
{
    if (!ValidateBatchContext(context)) {
        return E_ERR;
    }
 
    auto scannerObj = CreateScannerObj(context);
    if (scannerObj == nullptr) {
        MEDIA_ERR_LOG("Scan: scannerObj creation failed");
        return E_ERR;
    }
 
    return scannerObj->Execute();
}
 
std::unique_ptr<BatchScannerObj> BatchScanStrategy::CreateScannerObj(
    const std::shared_ptr<ScanTaskContext> &context)
{
    return std::make_unique<BatchScannerObj>(context->config.GetCustomRestoreInfo());
}
 
} // namespace OHOS::Media
// LCOV_EXCL_STOP