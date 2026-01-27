/*
 * Copyright (C) 2021-2022 Huawei Device Co., Ltd.
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
#define MLOG_TAG "DirOperation"

#include "medialibrary_dir_operations.h"

#include <algorithm>

#include "abs_rdb_predicates.h"
#include "datashare_predicates.h"
#include "datashare_result_set.h"
#include "media_log.h"
#include "medialibrary_file_operations.h"
#include "media_file_utils.h"
#include "media_smart_map_column.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_errno.h"
#ifdef MEDIALIBRARY_FEATURE_ANALYSIS_DATA
#include "medialibrary_smartalbum_map_operations.h"
#endif
#include "medialibrary_object_utils.h"
#include "rdb_utils.h"
#include "scanner_utils.h"

using namespace std;
using namespace OHOS::NativeRdb;
using namespace OHOS::RdbDataShareAdapter;

namespace OHOS {
namespace Media {
int32_t MediaLibraryDirOperations::TrashDirOperation(MediaLibraryCommand &cmd)
{
#ifdef MEDIALIBRARY_FEATURE_ANALYSIS_DATA
    ValueObject valueObject;
    if (!cmd.GetValueBucket().GetObject(CONST_MEDIA_DATA_DB_ID, valueObject)) {
        return E_HAS_DB_ERROR;
    }
    int32_t dirId = DEFAULT_ASSETID;
    valueObject.GetInt(dirId);
    if (dirId <= 0) {
        return E_GET_VALUEBUCKET_FAIL;
    }
    ValuesBucket valuesBucket;
    valuesBucket.PutInt(SMARTALBUMMAP_DB_CHILD_ASSET_ID, dirId);
    valuesBucket.PutInt(SMARTALBUMMAP_DB_ALBUM_ID, TRASH_ALBUM_ID_VALUES);
    MediaLibraryCommand smartMapCmd(OperationObject::SMART_ALBUM_MAP, OperationType::CREATE, valuesBucket);
    return MediaLibrarySmartAlbumMapOperations::HandleSmartAlbumMapOperation(smartMapCmd);
#else
    MEDIA_ERR_LOG("not support analysis data");
    return E_ERR;
#endif
}

int32_t MediaLibraryDirOperations::CreateDirOperation(MediaLibraryCommand &cmd)
{
    ValueObject valueObject;
    if (!cmd.GetValueBucket().GetObject(CONST_MEDIA_DATA_DB_RELATIVE_PATH, valueObject)) {
        return E_HAS_DB_ERROR;
    }
    string relativePath;
    valueObject.GetString(relativePath);
    if (relativePath.empty()) {
        return E_GET_VALUEBUCKET_FAIL;
    }
    ValuesBucket values;
    values.PutString(CONST_MEDIA_DATA_DB_NAME, MEDIA_NO_FILE);
    values.PutString(CONST_MEDIA_DATA_DB_RELATIVE_PATH, relativePath);
    values.PutInt(CONST_MEDIA_DATA_DB_MEDIA_TYPE, MEDIA_TYPE_NOFILE);
    cmd.SetValueBucket(values);
    return MediaLibraryObjectUtils::CreateFileObj(cmd);
}

int32_t MediaLibraryDirOperations::HandleDirOperation(MediaLibraryCommand &cmd)
{
    int32_t errCode = E_FAIL;
    switch (cmd.GetOprnType()) {
        case OperationType::CREATE:
            errCode = CreateDirOperation(cmd);
            break;
        case OperationType::TRASH:
            errCode = TrashDirOperation(cmd);
            break;
        default:
            MEDIA_WARN_LOG("Unknown operation type %{private}d", cmd.GetOprnType());
            break;
        }
    if (errCode < 0) {
        MEDIA_ERR_LOG("HandleDirOperations erroCode = %{public}d", errCode);
    }
    return errCode;
}
} // namespace Media
} // namespace OHOS
