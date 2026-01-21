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
#ifndef OHOS_MEDIALIBRARY_CUSTOM_RESTORE_NOFITY_H
#define OHOS_MEDIALIBRARY_CUSTOM_RESTORE_NOFITY_H
#define EXPORT __attribute__ ((visibility ("default")))

#include <string>

namespace OHOS {
namespace Media {

struct InnerRestoreResult {
    std::string stage = "";
    int32_t errCode = -1;
    int32_t progress = -1;
    int32_t uriType = -1;
    std::string uri = "";
    int32_t totalNum = -1;
    int32_t successNum = -1;
    int32_t failedNum = -1;
    int32_t sameNum = -1;
    int32_t cancelNum = -1;
};

class CustomRestoreNotify {
public:
    static const std::string NOTIFY_URI_PREFIX;
    CustomRestoreNotify() = default;
    ~CustomRestoreNotify() = default;
    int32_t Notify(std::string notifyUri, const InnerRestoreResult &restoreResult);
};

} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIALIBRARY_CUSTOM_RESTORE_NOFITY_H