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

#ifndef OHOS_MEDIA_DFX_COLLECTOR_H
#define OHOS_MEDIA_DFX_COLLECTOR_H

#include <map>
#include <mutex>
#include <string>

#include "dfx_const.h"

namespace OHOS {
namespace Media {
class DfxCollector {
public:
    DfxCollector();
    ~DfxCollector();
    void CollectThumbnailError(const std::string &path, int32_t method, int32_t errorCode);
    std::unordered_map<std::string, ThumbnailErrorInfo> GetThumbnailError();

private:
    std::mutex thumbnailErrorLock_;
    std::unordered_map<std::string, ThumbnailErrorInfo> thumbnailErrorMap_;
};
} // namespace Media
} // namespace OHOS

#endif  // OHOS_MEDIA_DFX_COLLECTOR_H