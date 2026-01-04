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

#ifndef INTERFACES_KITS__MEDIALIBRARY_INCLUDE_PHOTO_CUSTOM_RECORD_ASSET_MANAGER_ANI_H
#define INTERFACES_KITS__MEDIALIBRARY_INCLUDE_PHOTO_CUSTOM_RECORD_ASSET_MANAGER_ANI_H

#include <string>
#include "ani_error.h"
#include "photo_asset_custom_record.h"
#include "datashare_helper.h"
#include "fetch_result.h"
#include "custom_records_column.h"
#include "base_data_uri.h"
#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {
const std::string CUSTOM_RECORDS_OPERATION = "custom_records";
const std::string CUSTOM_RECORDS_CREATE_URI = CustomRecordsColumns::CUSTOM_RECORDS_URI_PREFIX + "/" + OPRN_CREATE;
const std::string CUSTOM_RECORDS_QUERY_URI = CustomRecordsColumns::CUSTOM_RECORDS_URI_PREFIX + "/" + OPRN_QUERY;
const std::string CUSTOM_RECORDS_DELETE_URI = CustomRecordsColumns::CUSTOM_RECORDS_URI_PREFIX + "/" + OPRN_DELETE;
const std::string CUSTOM_RECORDS_UPDATE_URI = CustomRecordsColumns::CUSTOM_RECORDS_URI_PREFIX + "/" + OPRN_UPDATE;

class AniCustomRecordStr {
    public:
        static const std::string FILE_ID;
        static const std::string SHARE_COUNT;
        static const std::string LCD_JUMP_COUNT;
};

class PhotoAssetCustomRecordManagerAni {
public:
    static ani_status Init(ani_env *env);
    static bool InitUserFileClient(ani_env *env, ani_object aniObject);
private:
    static ani_object Constructor(ani_env *env, ani_class clazz, ani_object context);
    static void CreateCustomRecords(ani_env *env, ani_object aniObject, ani_object customRecords);
    static ani_object AddShareCount(ani_env *env, ani_object aniObject, ani_object ids);
    static ani_object AddLcdJumpCount(ani_env *env, ani_object aniObject, ani_object ids);
    static void RemoveCustomRecords(ani_env *env, ani_object aniObject, ani_object optionCheck);
    static ani_object SetCustomRecords(ani_env *env, ani_object aniObject, ani_object customRecords);
    static ani_object GetCustomRecords(ani_env *env, ani_object aniObject, ani_object optionCheck);
};

struct CustomRecordAsyncAniContext : public AniError {
    OHOS::DataShare::DataSharePredicates predicates;
    std::vector<OHOS::DataShare::DataShareValuesBucket> valuesBuckets;
    std::vector<std::string> fetchColumn;
    int32_t userId_ = -1;
    ResultNapiType resultNapiType;
    std::vector<int32_t> fileIds;
    std::vector<int32_t> failFileIds;
    std::vector<PhotoAssetCustomRecord> updateRecords;
    std::unique_ptr<FetchResult<PhotoAssetCustomRecord>> fetchCustomRecordsResult;
    std::string networkId;
    std::string uri;
};
} // namespace Media
} // namespace OHOS
#endif // INTERFACES_KITS__MEDIALIBRARY_INCLUDE_PHOTO_CUSTOM_RECORD_ASSET_MANAGER_ANI_H