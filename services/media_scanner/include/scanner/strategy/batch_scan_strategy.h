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
 
#ifndef BATCH_SCAN_STRATEGY_H
#define BATCH_SCAN_STRATEGY_H
 
#include <memory>
 
#include "i_scan_strategy.h"
 
namespace OHOS {
namespace Media {
 
#define EXPORT __attribute__ ((visibility ("default")))
 
class BatchScannerObj;
struct ScanTaskContext;
 
class EXPORT BatchScanStrategy : public IScanStrategy {
public:
    int32_t Scan(const std::shared_ptr<ScanTaskContext> &context) override;
    ScanStrategyType GetStrategyType() const override;
 
private:
    bool ValidateBatchContext(const std::shared_ptr<ScanTaskContext> &context);
    std::unique_ptr<BatchScannerObj> CreateScannerObj(const std::shared_ptr<ScanTaskContext> &context);
};
 
} // namespace Media
} // namespace OHOS
 
#endif // BATCH_SCAN_STRATEGY_H