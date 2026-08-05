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
 
#ifndef CUSTOM_RESTORE_SCANNER_OBJ_H
#define CUSTOM_RESTORE_SCANNER_OBJ_H
 
#include <memory>
#include <vector>
 
#include "custom_restore_types.h"
#include "custom_restore_info.h"
#include "metadata.h"
#include "values_bucket.h"
 
namespace OHOS {
namespace Media {
 
class CustomRestoreScannerObj {
 
 
public:
    explicit CustomRestoreScannerObj(CustomRestoreInfo& info);
    ~CustomRestoreScannerObj() = default;
 
    int32_t Execute();
 
private:
    // Pipeline steps
    int32_t ResolveMetadata();
    int32_t Deduplicate();
    int32_t ConvertToValues();
    int32_t Insert();
    void PostProcess();
 
    // Intermediate item for pipeline
    struct CustomRestoreItem {
        RestoreFileInfo fileInfo;
        std::unique_ptr<Metadata> metadata;
        NativeRdb::ValuesBucket values;
        bool isDuplicate = false;
    };
 
    std::vector<CustomRestoreItem> items_;
    CustomRestoreInfo& customRestoreInfo_;
};
 
} // namespace Media
} // namespace OHOS
 
#endif // CUSTOM_RESTORE_SCANNER_OBJ_H