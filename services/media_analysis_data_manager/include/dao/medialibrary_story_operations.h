/*
 * Copyright (C) 2024-2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIALIBRARY_STORY_OPERATIOINS_H
#define OHOS_MEDIALIBRARY_STORY_OPERATIOINS_H

#include <string>
#include <unordered_map>

#include "abs_shared_result_set.h"
#include "medialibrary_command.h"

namespace OHOS {
namespace Media {
class MediaLibraryStoryOperations {
public:
    static int32_t InsertOperation(MediaLibraryCommand &cmd);
    static int32_t UpdateOperation(MediaLibraryCommand &cmd);
    static int32_t DeleteOperation(MediaLibraryCommand &cmd);
    static int32_t UpdateSubtitle(MediaLibraryCommand &cmd);

    static std::shared_ptr<NativeRdb::ResultSet> QueryOperation(MediaLibraryCommand &cmd,
        const std::vector<std::string> &columns);
};
} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIALIBRARY_STORY_OPERATIOINS_H