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

#ifndef OHOS_MEDIALIBRARY_RDB_OPERATIONS_H
#define OHOS_MEDIALIBRARY_RDB_OPERATIONS_H

#include <mutex>
#include <string>
#include <vector>

#include "abs_rdb_predicates.h"
#include "medialibrary_rdbstore.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class MediaLibraryRdbOperations {
public:
    static std::shared_ptr<NativeRdb::ResultSet> GetIndexOfUri(const NativeRdb::AbsRdbPredicates &predicates,
        const std::vector<std::string> &columns, const std::string &id);
    static std::shared_ptr<NativeRdb::ResultSet> GetIndexOfUriForPhotos(const NativeRdb::AbsRdbPredicates &predicates,
        const std::vector<std::string> &columns, const std::string &id);

    EXPORT static int32_t UpdateLastVisitTime(const std::string &id);
    EXPORT static int32_t QueryPragma(const std::string &key, int64_t &value);

    EXPORT static std::shared_ptr<NativeRdb::ResultSet> QueryEditDataExists(
        const NativeRdb::AbsRdbPredicates &predicates);
    static std::shared_ptr<NativeRdb::ResultSet> QueryMovingPhotoVideoReady(
        const NativeRdb::AbsRdbPredicates &predicates);
    static void WalCheckPoint();

private:
    static std::mutex walCheckPointMutex_;
};
} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIALIBRARY_RDB_OPERATIONS_H