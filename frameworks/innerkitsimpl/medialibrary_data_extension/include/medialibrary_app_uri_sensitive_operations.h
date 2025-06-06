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
 * See the License for the specific language governing Sensitives and
 * limitations under the License.
 */

#ifndef MEDIALIBRARY_APP_URI_SENSITIVE_OPERATIONS
#define MEDIALIBRARY_APP_URI_SENSITIVE_OPERATIONS

#include <string>
#include <unordered_map>
#include "media_app_uri_sensitive_column.h"
#include "media_app_uri_permission_column.h"
#include "file_asset.h"
#include "medialibrary_command.h"
#include "rdb_predicates.h"
#include "result_set.h"
#include "datashare_values_bucket.h"

namespace OHOS {
namespace Media {

EXPORT const std::unordered_map<std::string, int> APP_URI_SENSITIVE_MEMBER_MAP = {
    {AppUriSensitiveColumn::ID, MEMBER_TYPE_INT32},
    {AppUriSensitiveColumn::APP_ID, MEMBER_TYPE_STRING},
    {AppUriSensitiveColumn::FILE_ID, MEMBER_TYPE_INT32},
    {AppUriSensitiveColumn::URI_TYPE, MEMBER_TYPE_INT32},
    {AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE, MEMBER_TYPE_INT32},
    {AppUriSensitiveColumn::IS_FORCE_SENSITIVE, MEMBER_TYPE_INT32},
    {AppUriSensitiveColumn::DATE_MODIFIED, MEMBER_TYPE_INT64},
    {AppUriSensitiveColumn::SOURCE_TOKENID, MEMBER_TYPE_INT64},
    {AppUriSensitiveColumn::TARGET_TOKENID, MEMBER_TYPE_INT64}};

class MediaLibraryAppUriSensitiveOperations {
public:
    EXPORT static const int ERROR;
    EXPORT static const int SUCCEED;
    EXPORT static const int ALREADY_EXIST;
    EXPORT static const int NO_DATA_EXIST;

    EXPORT static int32_t HandleInsertOperation(MediaLibraryCommand &cmd);
    EXPORT static int32_t BatchInsert(MediaLibraryCommand &cmd,
        const std::vector<DataShare::DataShareValuesBucket> &values);
    EXPORT static int32_t DeleteOperation(NativeRdb::RdbPredicates &predicates);
    EXPORT static std::shared_ptr<OHOS::NativeRdb::ResultSet> QueryOperation(
        DataShare::DataSharePredicates &predicates, std::vector<std::string> &fetchColumns);
    EXPORT static bool BeForceSensitive(MediaLibraryCommand &cmd,
        const std::vector<DataShare::DataShareValuesBucket> &values);
private:
    /**
     * query newData before insert, use this method.
     * @param resultFlag ERROR: query newData error.
     *                   NO_DATA_EXIST: newData not exist in database.
     *                   ALREADY_EXIST: newData already exist in database.
     */
    static std::shared_ptr<OHOS::NativeRdb::ResultSet> QueryNewData(
        OHOS::NativeRdb::ValuesBucket &valueBucket, int &resultFlag);
    /**
     * get the value of the int type corresponding to {@code column} from {@code valueBucket}.
     * @param result target value
     * @return true: Successfully obtained the value.
     *         false: failed to get the value.
     */
    static bool GetIntFromValuesBucket(OHOS::NativeRdb::ValuesBucket &valueBucket,
        const std::string &column, int &result);
    
    /**
     * get the value of the int type corresponding to {@code column} from {@code valueBucket}.
     * @param result target value
     * @return true: Successfully obtained the value.
     *         false: failed to get the value.
     */
    static bool GetStringFromValuesBucket(OHOS::NativeRdb::ValuesBucket &valueBucket,
        const std::string &column, std::string &result);
    
    /**
     * @param resultSetDB must contain id value.
     * @param valueBucketParam must contain SensitiveType value.
     */
    static int UpdateSensitiveType(std::shared_ptr<OHOS::NativeRdb::ResultSet> &resultSetDB,
        int &sensitiveTypeParam);
    
    static int UpdateSensitiveTypeAndForceHideSensitive(std::shared_ptr<OHOS::NativeRdb::ResultSet> &resultSetDB,
        int &sensitiveTypeParam, OHOS::NativeRdb::ValuesBucket &valueBucket);
    static bool IsValidSensitiveType(int &sensitiveType);
};
} // namespace Media
} // namespace OHOS

#endif // MEDIALIBRARY_APP_URI_SENSITIVE_OPERATIONS