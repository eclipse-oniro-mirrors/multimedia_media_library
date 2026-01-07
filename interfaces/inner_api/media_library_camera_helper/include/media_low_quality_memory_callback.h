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
 
#ifndef MEDIA_LIBRARY_LOW_QUALITY_MEMORY_CALLBACK_H_
#define MEDIA_LIBRARY_LOW_QUALITY_MEMORY_CALLBACK_H_
 
#include "datashare_helper.h"
 
namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
using LowQualityMemoryNumHandler = std::function<void(int32_t)>;
class MediaLowQualityMemoryCallback : public RefBase {
public:
    EXPORT MediaLowQualityMemoryCallback() = default;
    ~MediaLowQualityMemoryCallback();
 
    // 查询低质量图内存个数
    int32_t RegisterPhotoStateCallback(
        std::shared_ptr<DataShare::DataShareHelper> dataShareHelper, const LowQualityMemoryNumHandler &func);
    int32_t UnregisterPhotoStateCallback(std::shared_ptr<DataShare::DataShareHelper> dataShareHelper);
 
private:
    int32_t RegisterLowQualityMemoryNumObserver(std::shared_ptr<DataShare::DataShareHelper> &dataShareHelper);
    int32_t UnregisterLowQualityMemoryNumObserver(std::shared_ptr<DataShare::DataShareHelper> &dataShareHelper);
};
} // namespace Media
} // namespace OHOS
#endif // MEDIA_LIBRARY_LOW_QUALITY_MEMORY_CALLBACK_H_