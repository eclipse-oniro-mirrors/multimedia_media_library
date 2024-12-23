/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OHOS_MEDIALIBRARY_DUPLICATE_PHOTO_OPERATION_H
#define OHOS_MEDIALIBRARY_DUPLICATE_PHOTO_OPERATION_H

#include <mutex>
#include <unordered_set>

#include "abs_shared_result_set.h"

namespace OHOS {
namespace Media {
class DuplicatePhotoOperation {
public:
    static std::shared_ptr<NativeRdb::ResultSet> GetAllDuplicateAssets(const std::vector<std::string> &columns,
        const int offset, const int limit);
    static std::shared_ptr<NativeRdb::ResultSet> GetCanDelDuplicateAssets(const std::vector<std::string> &columns,
        const int offset, const int limit);

private:
    static std::string GetSelectColumns(const std::unordered_set<std::string> &columns);

private:
    static std::once_flag onceFlag_;
};
} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIALIBRARY_DUPLICATE_PHOTO_OPERATION_H