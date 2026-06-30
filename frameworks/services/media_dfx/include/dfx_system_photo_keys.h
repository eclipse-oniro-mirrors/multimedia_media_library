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

#ifndef OHOS_MEDIA_DFX_SYSTEM_PHOTO_KEYS_H
#define OHOS_MEDIA_DFX_SYSTEM_PHOTO_KEYS_H

#include <safe_map.h>
#include <string>

namespace OHOS {
namespace Media {
// This class is only for client-side (NAPI/ANI/CJ) use. Do not call from server-side.
class DfxSystemPhotoKeys {
public:
    __attribute__((visibility("default"))) static int32_t ReportIfSystemKey(
        const std::string &interface, const std::string &key);

private:
    static std::string GetBundleName();
    static SafeMap<std::string, int32_t> reportedKeyMap_;
};
}  // namespace Media
}  // namespace OHOS

#endif  // OHOS_MEDIA_DFX_SYSTEM_PHOTO_KEYS_H