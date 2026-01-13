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

#ifndef OHOS_MEDIALIBRARY_KVSTORE_UTILS_H
#define OHOS_MEDIALIBRARY_KVSTORE_UTILS_H

#include <string>

#include "medialibrary_kvstore.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

class MediaLibraryKvStoreUtils {
public:
    EXPORT MediaLibraryKvStoreUtils() = default;
    EXPORT ~MediaLibraryKvStoreUtils();

    EXPORT static int32_t CopyAstcDataToKvStoreByType(const KvStoreValueType &type, const std::string &oldKey,
        const std::string &newKey);
};
} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIALIBRARY_KVSTORE_UTILS_H