/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#include "medialibrary_dir_db.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_tracer.h"

using namespace std;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {
static const std::string DIR_DB_COND = std::to_string(CONST_MEDIA_DATA_DB_ID) + " = ?";
int32_t MediaLibraryDirDb::DeleteDirInfo(const int32_t dirId, const shared_ptr<RdbStore> &rdbStore)
{
    CHECK_AND_RETURN_RET_LOG((rdbStore != nullptr) && (dirId > 0), E_DIR_OPER_ERR, "Invalid input");
    int32_t deletedRows(E_ALBUM_OPER_ERR);
    vector<string> whereArgs = { std::to_string(dirId)};
    int32_t deleteResult = rdbStore->Delete(deletedRows, CONST_MEDIALIBRARY_TABLE, DIR_DB_COND, whereArgs);
    CHECK_AND_RETURN_RET_LOG(deleteResult == E_OK, E_DIR_OPER_ERR, "Delete failed");
    return (deletedRows > 0) ? E_SUCCESS : E_FAIL;
}
} // namespace Media
} // namespace OHOS
