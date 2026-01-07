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
#ifndef OHOS_MEDIA_BACKUP_DATABASE_HELPER
#define OHOS_MEDIA_BACKUP_DATABASE_HELPER

#include <string>

#include "backup_const.h"
#include "rdb_store.h"

namespace OHOS::Media {
class BackupDatabaseHelper {
public:
    enum DbType {
        DEFAULT = 0,
        EXTERNAL,
        PHOTO_CACHE,
        VIDEO_CACHE,
        PHOTO_SD_CACHE,
        VIDEO_SD_CACHE,
    };

    struct DbInfo {
        std::string name;
        std::string path;
        std::shared_ptr<NativeRdb::RdbStore> rdbStore {nullptr};
        DbInfo() = default;
        DbInfo(const std::string &name, const std::string &path) : name(name), path(path) {}
        DbInfo(std::shared_ptr<NativeRdb::RdbStore> rdbStore) : rdbStore(rdbStore) {}
    };

    struct FileQueryInfo {
        int32_t dbType {DbType::DEFAULT};
        std::string tableName;
        std::string columnName;
        std::string path;
        FileQueryInfo() = default;
        FileQueryInfo(int32_t dbType, const std::string &tableName, const std::string &columnName,
            const std::string &path) : dbType(dbType), tableName(tableName), columnName(columnName), path(path) {}
    };

public:
    void Init(int32_t sceneCode, bool shouldIncludeSd, const std::string &prefix);
    void InitDb(int32_t dbType, const std::string &prefix);
    void InitDb(const std::vector<int32_t> &dbTypeList, const std::string &prefix);
    void AddDb(int32_t dbType, std::shared_ptr<NativeRdb::RdbStore> rdbStore);
    void IsFileExist(int32_t sceneCode, const FileInfo &fileInfo, int32_t &dbType, int32_t &dbStatus,
        int32_t &fileStatus);

private:
    bool HasDb(int32_t dbType);
    void GetFileQueryInfo(int32_t sceneCode, const FileInfo &fileInfo, FileQueryInfo &fileQueryInfo);

    std::unordered_map<int32_t, DbInfo> dbInfoMap_;

    const std::unordered_map<int32_t, DbInfo> DB_INFO_MAP = {
        { DbType::PHOTO_CACHE, DbInfo("photo_Cache.db", "/storage/emulated/0/photo_Cache.db") },
        { DbType::VIDEO_CACHE, DbInfo("video_Cache.db", "/storage/emulated/0/video_Cache.db") },
        { DbType::PHOTO_SD_CACHE, DbInfo("photo_sd_Cache.db", "/photo_sd_Cache.db") },
        { DbType::VIDEO_SD_CACHE, DbInfo("video_sd_Cache.db", "/video_sd_Cache.db") },
    };
};
} // namespace OHOS::Media
#endif // OHOS_MEDIA_BACKUP_DATABASE_HELPER