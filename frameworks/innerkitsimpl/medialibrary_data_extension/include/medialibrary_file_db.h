/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIALIBRARY_FILE_DB_H
#define OHOS_MEDIALIBRARY_FILE_DB_H

#include <string>
#include <sys/stat.h>

#include "media_data_ability_const.h"
#include "rdb_store.h"
#include "rdb_errno.h"
#include "values_bucket.h"
#include "datashare_predicates.h"
#include "datashare_values_bucket.h"
#include "datashare_abs_result_set.h"
#include "datashare_abstract_result_set.h"

namespace OHOS {
namespace Media {
class MediaLibraryFileDb {
public:
    MediaLibraryFileDb() = default;
    ~MediaLibraryFileDb() = default;

    int32_t Insert(const OHOS::DataShare::DataShareValuesBucket &values,
                   const std::shared_ptr<OHOS::NativeRdb::RdbStore> &rdbStore);
    int32_t Delete(const std::string &strRow, const std::shared_ptr<OHOS::NativeRdb::RdbStore> &rdbStore);
    int32_t Modify(const std::string &rowNum, const std::string &dstPath,
                   const int &bucketId, const std::string &bucketName,
                   const std::shared_ptr<OHOS::NativeRdb::RdbStore> &rdbStore);
};
} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIALIBRARY_FILE_DB_H
