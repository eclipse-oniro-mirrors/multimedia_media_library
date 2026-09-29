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

#ifndef FRAMEWORKS_INNERKITSIMPL_MEDIALIBRARY_DATA_EXTENSION_INCLUDE_REVERSE_CLONE_RESTORE_MARKER_H
#define FRAMEWORKS_INNERKITSIMPL_MEDIALIBRARY_DATA_EXTENSION_INCLUDE_REVERSE_CLONE_RESTORE_MARKER_H

#include <cstdint>

namespace OHOS {
namespace Media {
#ifndef EXPORT
#define EXPORT __attribute__ ((visibility ("default")))
#endif

class ReverseCloneRestoreMarker {
public:
    EXPORT static bool Recreate();
    EXPORT static bool Delete();
    EXPORT static bool DeleteIfExpired(int64_t currentTimeMilliseconds);

private:
    static bool GetRecordTime(int64_t &recordTimeMilliseconds);
    static bool IsExpired(int64_t currentTimeMilliseconds);
};
} // namespace Media
} // namespace OHOS

#endif // FRAMEWORKS_INNERKITSIMPL_MEDIALIBRARY_DATA_EXTENSION_INCLUDE_REVERSE_CLONE_RESTORE_MARKER_H
