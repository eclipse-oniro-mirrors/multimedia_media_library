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

#ifndef OHOS_MEDIALIBRARY_UNISTORE_H
#define OHOS_MEDIALIBRARY_UNISTORE_H

#include "fa_ability_context.h"
#include "medialibrary_command.h"
#include "rdb_errno.h"
#include "rdb_helper.h"
#include "rdb_open_callback.h"
#include "rdb_store.h"
#include "rdb_store_config.h"
#include "rdb_types.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class MediaLibraryUnistore {
public:
    EXPORT MediaLibraryUnistore() = default;
    EXPORT virtual ~MediaLibraryUnistore() = default;

    EXPORT virtual int32_t Init() = 0;
    EXPORT virtual void Stop() = 0;

    EXPORT virtual int32_t Insert(MediaLibraryCommand &cmd, int64_t &rowId) = 0;
    EXPORT virtual int32_t Delete(MediaLibraryCommand &cmd, int32_t &rowId) = 0;
    EXPORT virtual int32_t Update(MediaLibraryCommand &cmd, int32_t &rowId) = 0;
    EXPORT virtual int32_t BatchInsert(MediaLibraryCommand &cmd, int64_t& outInsertNum,
        std::vector<NativeRdb::ValuesBucket>& values) = 0;
    EXPORT virtual std::shared_ptr<NativeRdb::ResultSet> Query(MediaLibraryCommand &cmd,
        const std::vector<std::string> &columns)
    {
        return nullptr;
    }
    
    EXPORT virtual bool SyncPullTable(const std::string &bundleName, const std::string &tableName,
        int32_t rowId, std::vector<std::string> &devices)
    {
        return false;
    }

    EXPORT virtual bool SyncPushTable(const std::string &bundleName, const std::string &tableName,
        int32_t rowId, std::vector<std::string> &devices, bool isBlock = false)
    {
        return false;
    }

    EXPORT virtual int32_t ExecuteSql(const std::string &sql)
    {
        return NativeRdb::E_NOT_SUPPORT;
    }

    EXPORT virtual std::shared_ptr<NativeRdb::ResultSet> QuerySql(const std::string &sql,
        const std::vector<std::string> &selectionArgs = std::vector<std::string>())
    {
        return nullptr;
    }
};
} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIALIBRARY_UNISTORE_H
