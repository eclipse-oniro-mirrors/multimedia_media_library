/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_TAB_OLD_PHOTOS_OPERATIONS_H
#define OHOS_MEDIA_TAB_OLD_PHOTOS_OPERATIONS_H

#include <string>
#include <vector>

#include "abs_shared_result_set.h"
#include "medialibrary_command.h"
#include "rdb_utils.h"

namespace OHOS {
namespace Media {
class MediaLibraryTabOldPhotosOperations {
public:
    std::shared_ptr<NativeRdb::ResultSet> Query(const NativeRdb::RdbPredicates &rdbPredicate,
        const std::vector<std::string> &columns);
};
} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIA_TAB_OLD_PHOTOS_OPERATIONS_H
