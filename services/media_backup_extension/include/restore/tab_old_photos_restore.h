/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_BACKUP_MEDIA_TAB_OLD_PHOTOS_RESTORE
#define OHOS_BACKUP_MEDIA_TAB_OLD_PHOTOS_RESTORE

#include <string>
#include <vector>

#include "backup_const.h"
#include "rdb_store.h"

namespace OHOS::Media {
class TabOldPhotosRestore {
public:
    int32_t Restore(std::shared_ptr<NativeRdb::RdbStore> rdbStorePtr, const std::vector<FileInfo> &fileInfos);

private:
    std::string ToString(const std::vector<NativeRdb::ValueObject> &values);
};

class TabOldPhotosRestoreHelper {
public:
    void SetPlaceHoldersAndBindArgs(const std::vector<FileInfo> &fileInfos);
    bool IsEmpty();
    int32_t InsertIntoTable(std::shared_ptr<NativeRdb::RdbStore> rdbStorePtr);
    std::string GetInsertSql();
    std::vector<NativeRdb::ValueObject> GetBindArgs();
    size_t GetInsertSize();

private:
    void AddPlaceHolders();
    void AddBindArgs(const FileInfo &fileInfo);
    std::string GetInputTableClause();

    std::vector<std::string> placeHolders_;
    std::vector<NativeRdb::ValueObject> bindArgs_;

    const std::string SQL_PLACEHOLDERS = "(?, ?, ?)";
    const std::string SQL_TAB_OLD_PHOTOS_INSERT = "\
        INSERT INTO tab_old_photos \
        ( \
            file_id, \
            data, \
            old_file_id, \
            old_data \
        ) \
        SELECT Photos.file_id, \
            INPUT.data, \
            INPUT.old_file_id, \
            INPUT.old_data \
        FROM Photos \
            INNER JOIN INPUT \
                ON Photos.data=INPUT.data \
            LEFT JOIN tab_old_photos \
                ON INPUT.old_data=tab_old_photos.old_data \
        WHERE tab_old_photos.old_data IS NULL AND \
            COALESCE(hidden, 0) = 0 AND \
            COALESCE(date_trashed, 0) = 0;";
};
} // namespace OHOS::Media
#endif // OHOS_BACKUP_MEDIA_TAB_OLD_PHOTOS_RESTORE