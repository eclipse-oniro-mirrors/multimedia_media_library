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

#ifndef FILE_ID_MIGRATOR_H
#define FILE_ID_MIGRATOR_H

#include <memory>
#include <string>
#include <vector>
#include <regex>
#include <map>
#include "rdb_store.h"

namespace OHOS {
namespace Media {

/**
 * @brief 用于直接更新表字段的配置结构
 */
struct DirectUpdate {
    std::string table;
    std::string column;
    std::string condition;
    bool needSequenceUpdate = false;
};

/**
 * @brief 负责对旧设备数据库中的 file_id 和 album_id 进行偏移，
 *        确保其不与新设备数据库中的 ID 冲突。
 *
 * 整体流程分为两个独立部分：
 *   - file_id 偏移 (设计步骤 1~5)
 *   - album_id 偏移 (设计步骤 7~10)
 *
 * 支持二次迁移：当数据库转正后若新设备又产生了更大的 ID，
 * 可再次调用 Migrate 进行补充偏移。
 */
class FileIdMigrator {
public:
    FileIdMigrator()
        : regexPattern_(R"(file://media/(Photo|Video)/([0-9]+)/)", std::regex::optimize) {}
    ~FileIdMigrator() = default;

    /**
     * @brief 执行完整的 ID 偏移（含 file_id 和 album_id）。
     * @param oldDb 需要偏移的数据库（旧设备数据库）。
     * @param newDb 用于查询最大 ID 的数据库（新设备数据库）。
     * @return true 成功，false 关键步骤失败。
     */
    bool Migrate(std::shared_ptr<NativeRdb::RdbStore> oldDb,
                 std::shared_ptr<NativeRdb::RdbStore> newDb);

    // 查询指定数据库所有相关表的最大 file_id（包括 sqlite_sequence）
    static int64_t GetMaxFileIdFromAllTables(std::shared_ptr<NativeRdb::RdbStore> db);

    // 查询指定数据库 PhotoAlbum 表的最大 album_id（包括 sqlite_sequence，辅助判断是否需要二次迁移）
    static int64_t GetMaxAlbumIdFromAllTables(std::shared_ptr<NativeRdb::RdbStore> db);

    /**
     * @brief 更新 sqlite_sequence 表中 Photos 表的 seq 值
     *        将 sqlite_sequence.Photos 的 seq 更新为 GetMaxFileIdFromAllTables(db) 的结果
     * @param db 数据库句柄
     * @return true 成功，false 失败
     */
    static bool UpdateSqliteSequenceForPhotos(std::shared_ptr<NativeRdb::RdbStore> db);

    /**
     * @brief 批量更新人脸识别表中的 file_id
     * @param db 数据库句柄
     * @param fileIdMap file_id 映射表 {old_file_id: new_file_id}
     * @return true 成功，false 失败
     */
    bool UpdateFaceTableFileIds(std::shared_ptr<NativeRdb::RdbStore> db,
        const std::unordered_map<int32_t, int32_t>& fileIdMap);

    bool UpdateAnalysisTotalFields(std::shared_ptr<NativeRdb::RdbStore> db,
        const std::unordered_map<int32_t, int32_t>& fileIdMap);

    bool UpdateAnalysisTotalFieldsByConfig(std::shared_ptr<NativeRdb::RdbStore> db,
        const std::unordered_map<int32_t, int32_t>& fileIdMap,
        const std::vector<std::string>& fields);

    bool MigrateAnalysisTotalScore(std::shared_ptr<NativeRdb::RdbStore> oldDb,
        std::shared_ptr<NativeRdb::RdbStore> newDb,
        const std::unordered_map<int32_t, int32_t>& fileIdMap);

    /**
     * @brief 获取迁移时计算的 newMaxExtended 值
     * @return newMaxExtended 值，用于判重时排除吸收的数据范围
     */
    int64_t GetNewMaxExtended() const { return newMaxExtended_; }

private:
    bool SetBit20InTotalScore(std::shared_ptr<NativeRdb::RdbStore> oldDb);
    bool CopyTotalScoreFromNewDb(std::shared_ptr<NativeRdb::RdbStore> oldDb,
        std::shared_ptr<NativeRdb::RdbStore> newDb);
    bool UpdateTotalScoreByMapping(std::shared_ptr<NativeRdb::RdbStore> oldDb,
        const std::unordered_map<int32_t, int32_t>& fileIdMap);

private:
    // file_id 偏移 (步骤 1~5)
    bool MigrateFileIds(std::shared_ptr<NativeRdb::RdbStore> oldDb,
                        std::shared_ptr<NativeRdb::RdbStore> newDb);
    int64_t AddFileIdOffset(int64_t newMaxFileId);
    bool UpdatePhotosFileId(std::shared_ptr<NativeRdb::RdbStore> db,
                            int64_t newMaxExtended, int64_t oldMax);
    bool UpdateDirectFileIdTables(std::shared_ptr<NativeRdb::RdbStore> db,
                                  int64_t newMaxExtended, int64_t oldMax);
    bool UpdateEmbeddedFileIds(std::shared_ptr<NativeRdb::RdbStore> db,
                               int64_t newMaxExtended, int64_t oldMax);
    bool UpdateEmbeddedBatch(const std::string &table,
                             const std::string &idCol,
                             const std::string &valueCol,
                             std::shared_ptr<NativeRdb::RdbStore> db,
                             int64_t newMaxExtended, int64_t oldMax);

    // album_id 偏移 (步骤 7~10)
    bool MigrateAlbumIds(std::shared_ptr<NativeRdb::RdbStore> oldDb,
                         std::shared_ptr<NativeRdb::RdbStore> newDb);
    int64_t AddAlbumIdOffset(int64_t newMaxAlbumId);
    bool UpdatePhotoAlbumAlbumId(std::shared_ptr<NativeRdb::RdbStore> db,
                                 int64_t newMaxExtended, int64_t oldMax);
    bool UpdatePhotosOwnerAlbumId(std::shared_ptr<NativeRdb::RdbStore> db,
                                  int64_t newMaxExtended, int64_t oldMax);

    // 智慧相册 album_id 偏移
    bool MigrateSmartAlbumIds(std::shared_ptr<NativeRdb::RdbStore> oldDb,
                              std::shared_ptr<NativeRdb::RdbStore> newDb);
    int64_t GetMaxSmartAlbumId(std::shared_ptr<NativeRdb::RdbStore> db);
    bool UpdateSmartAlbumTables(std::shared_ptr<NativeRdb::RdbStore> db,
                                int64_t newMaxExtended, int64_t oldMax);
    bool CreateAlbumIdUpdateTrigger(std::shared_ptr<NativeRdb::RdbStore> db);
bool UpdateAllSmartAlbumTables(std::shared_ptr<NativeRdb::RdbStore> db,
                                   int64_t newMaxExtended, int64_t oldMax);
    bool UpdateSmartAlbumTable(std::shared_ptr<NativeRdb::RdbStore> db,
                                const DirectUpdate& upd,
                                int64_t newMaxExtended, int64_t oldMax);
    bool DropAlbumIdUpdateTrigger(std::shared_ptr<NativeRdb::RdbStore> db);

    // 通用工具
    bool TableExists(std::shared_ptr<NativeRdb::RdbStore> db, const std::string &tableName);
    bool ExecuteSql(std::shared_ptr<NativeRdb::RdbStore> db, const std::string &sql,
                    const std::vector<NativeRdb::ValueObject> &args);
    bool InsertAndDeleteEmptyRecord(std::shared_ptr<NativeRdb::RdbStore> db,
                                   const std::string &table,
                                   const std::string &idColumn,
                                   int64_t idValue);

    int64_t newMaxExtended_ = 0;  // 迁移时 file_id 偏移后的上限值（用于判重）
    std::regex regexPattern_;
};

} // namespace Media
} // namespace OHOS
#endif