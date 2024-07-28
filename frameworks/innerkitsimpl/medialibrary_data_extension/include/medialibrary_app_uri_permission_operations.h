/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MEDIALIBRARY_APP_URI_PERMISSION_OPERATIONS
#define MEDIALIBRARY_APP_URI_PERMISSION_OPERATIONS

#include <string>
#include <unordered_map>
#include "media_app_uri_permission_column.h"
#include "file_asset.h"
#include "medialibrary_command.h"
#include "rdb_predicates.h"
#include "result_set.h"
#include "datashare_values_bucket.h"

namespace OHOS {
namespace Media {

EXPORT const std::unordered_map<std::string, int> APP_URI_PERMISSION_MEMBER_MAP = {
    {AppUriPermissionColumn::ID, MEMBER_TYPE_INT32},
    {AppUriPermissionColumn::APP_ID, MEMBER_TYPE_STRING},
    {AppUriPermissionColumn::FILE_ID, MEMBER_TYPE_INT32},
    {AppUriPermissionColumn::URI_TYPE, MEMBER_TYPE_INT32},
    {AppUriPermissionColumn::PERMISSION_TYPE, MEMBER_TYPE_INT32},
    {AppUriPermissionColumn::DATE_MODIFIED, MEMBER_TYPE_INT64}};

class MediaLibraryAppUriPermissionOperations {
public:
    static const int ERROR;
    static const int SUCCEED;
    static const int ALREADY_EXIST;

    static int32_t HandleInsertOperation(MediaLibraryCommand &cmd);
    static int32_t BatchInsert(MediaLibraryCommand &cmd,
        const std::vector<DataShare::DataShareValuesBucket> &values);
    static int32_t DeleteOperation(NativeRdb::RdbPredicates &predicates);
    static std::shared_ptr<OHOS::NativeRdb::ResultSet> QueryOperation(
        DataShare::DataSharePredicates &predicates, std::vector<std::string> &fetchColumns);
private:
    static std::shared_ptr<OHOS::NativeRdb::ResultSet> QueryNewData(
        OHOS::NativeRdb::ValuesBucket &valueBucket, int &resultFlag);
    static bool GetIntFromValuesBucket(OHOS::NativeRdb::ValuesBucket &valueBucket,
        const std::string column, int &result);
};
} // namespace Media
} // namespace OHOS

#endif // MEDIALIBRARY_APP_URI_PERMISSION_OPERATIONS