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

#define MLOG_TAG "Media_Reverse_Restore"
#include "file_id_migrator.h"
#include "media_log.h"
#include <regex>
#include <algorithm>
#include <sstream>

#include <medialibrary_data_manager_utils.h>

namespace OHOS {
namespace Media {

using namespace NativeRdb;

// 公共入口
// LCOV_EXCL_START
bool FileIdMigrator::Migrate(std::shared_ptr<RdbStore> oldDb,
                             std::shared_ptr<RdbStore> newDb)
{
    MEDIA_INFO_LOG("FileIdMigrator::Migrate begin");
    if (!oldDb || !newDb) {
        MEDIA_ERR_LOG("FileIdMigrator: invalid db handles");
        return false;
    }

    // 先执行 file_id 偏移 (设计步骤 1~5)
    if (!MigrateFileIds(oldDb, newDb)) {
        MEDIA_ERR_LOG("FileIdMigrator: file_id migration failed");
        return false;
    }

    // 再执行 album_id 偏移 (设计步骤 7~10)
    if (!MigrateAlbumIds(oldDb, newDb)) {
        MEDIA_ERR_LOG("FileIdMigrator: album_id migration failed");
        return false;
    }

    // 最后执行智慧相册 album_id 偏移
    if (!MigrateSmartAlbumIds(oldDb, newDb)) {
        MEDIA_ERR_LOG("FileIdMigrator: smart album_id migration failed");
        return false;
    }

    MEDIA_INFO_LOG("FileIdMigrator::Migrate success");
    return true;
}

int64_t FileIdMigrator::GetMaxFileIdFromAllTables(std::shared_ptr<RdbStore> db)
{
    int64_t maxId = 0;

    struct Query {
        std::string sql;
        std::string description;
        std::vector<ValueObject> args;
    };

    std::vector<Query> queries = {
        {"SELECT MAX(file_id) FROM Photos", "Photos.file_id", {}},
        {"SELECT MAX(file_id) FROM tab_analysis_total", "tab_analysis_total.file_id", {}},
        {"SELECT MAX(file_id) FROM tab_analysis_video_total", "tab_analysis_video_total.file_id", {}},
        {"SELECT MAX(file_id) FROM tab_analysis_search_index", "tab_analysis_search_index.file_id", {}},
        {"SELECT MAX(file_id) FROM tab_map_photo_map", "tab_map_photo_map.file_id", {}},
        {"SELECT MAX(group_id_rep) FROM tab_analysis_dedup", "tab_analysis_dedup.group_id_rep", {}},
        {"SELECT MAX(group_id_sim) FROM tab_analysis_dedup", "tab_analysis_dedup.group_id_sim", {}},
        {"SELECT seq FROM sqlite_sequence WHERE name = ?", "sqlite_sequence.Photos", {ValueObject("Photos")}}};

    for (const auto &query : queries) {
        auto resultSet = db->QuerySql(query.sql, query.args);
        if (resultSet == nullptr || resultSet->GoToNextRow() != E_OK) {
            if (resultSet != nullptr) {
                resultSet->Close();
            }
            if (query.description.find("sqlite_sequence") == std::string::npos) {
                MEDIA_WARN_LOG("FileIdMigrator: query %{public}s failed", query.description.c_str());
            }
            continue;
        }

        int64_t currentMax = 0;
        resultSet->GetLong(0, currentMax);
        resultSet->Close();

        if (currentMax > maxId) {
            maxId = currentMax;
        }
    }

    MEDIA_INFO_LOG("FileIdMigrator: GetMaxFileIdFromAllTables result=%{public}lld", static_cast<long long>(maxId));
    return maxId;
}

int64_t FileIdMigrator::GetMaxAlbumIdFromAllTables(std::shared_ptr<RdbStore> db)
{
    int64_t maxId = 0;

    struct Query {
        std::string sql;
        std::string description;
        std::vector<ValueObject> args;
    };

    std::vector<Query> queries = {
        {"SELECT MAX(album_id) FROM PhotoAlbum", "PhotoAlbum.album_id", {}},
        {"SELECT seq FROM sqlite_sequence WHERE name = ?", "sqlite_sequence.PhotoAlbum", {ValueObject("PhotoAlbum")}}};

    for (const auto &query : queries) {
        auto resultSet = db->QuerySql(query.sql, query.args);
        if (resultSet == nullptr || resultSet->GoToNextRow() != E_OK) {
            if (resultSet != nullptr) {
                resultSet->Close();
            }
            if (query.description.find("sqlite_sequence") == std::string::npos) {
                MEDIA_WARN_LOG("FileIdMigrator: query %{public}s failed", query.description.c_str());
            }
            continue;
        }

        int64_t currentMax = 0;
        resultSet->GetLong(0, currentMax);
        resultSet->Close();

        if (currentMax > maxId) {
            maxId = currentMax;
        }
    }

    MEDIA_INFO_LOG("FileIdMigrator: GetMaxAlbumIdFromAllTables result=%{public}lld", static_cast<long long>(maxId));
    return maxId;
}

bool FileIdMigrator::UpdateSqliteSequenceForPhotos(std::shared_ptr<RdbStore> db)
{
    MEDIA_INFO_LOG("FileIdMigrator::UpdateSqliteSequenceForPhotos start");

    // 获取所有表中的最大 file_id
    int64_t maxFileId = GetMaxFileIdFromAllTables(db);
    if (maxFileId <= 0) {
        MEDIA_WARN_LOG("FileIdMigrator: maxFileId is invalid (%{public}lld), skip update",
                       static_cast<long long>(maxFileId));
        return false;
    }

    // 更新 sqlite_sequence 表中 Photos 表的 seq 值
    std::string updateSeqSql = "UPDATE sqlite_sequence SET seq = ? WHERE name = 'Photos';";
    std::vector<ValueObject> updateSeqArgs;
    updateSeqArgs.emplace_back(ValueObject(maxFileId));

    int32_t ret = db->ExecuteSql(updateSeqSql, updateSeqArgs);
    if (ret != E_OK) {
        MEDIA_ERR_LOG(
            "FileIdMigrator: update sqlite_sequence.Photos seq failed, maxFileId=%{public}lld, ret=%{public}d",
            static_cast<long long>(maxFileId), ret);
        return false;
    }

    MEDIA_INFO_LOG("FileIdMigrator: updated sqlite_sequence.Photos seq to %{public}lld",
                   static_cast<long long>(maxFileId));
    MEDIA_INFO_LOG("FileIdMigrator::UpdateSqliteSequenceForPhotos end");
    return true;
}

// file_id 偏移实现

bool FileIdMigrator::MigrateFileIds(std::shared_ptr<RdbStore> oldDb,
                                    std::shared_ptr<RdbStore> newDb)
{
    // 步骤 1: 获取原始最大 file_id
    int64_t newMax = GetMaxFileIdFromAllTables(newDb);
    int64_t oldMax = GetMaxFileIdFromAllTables(oldDb);
    // 新机无数据场景：需要将旧机的所有照片记录到 tab_cloned_old_photos
    if (newMax <= 0) {
        if (oldMax <= 0) {
            // 新机和旧机都没有照片，直接返回
            MEDIA_INFO_LOG("FileIdMigrator: both new and old have no photos, no need clone");
            return true;
        }
        // 新机无照片，旧机有照片：将旧机所有照片记录到 tab_cloned_old_photos
        const std::string insertUnoffsetSql =
            "INSERT INTO tab_cloned_old_photos (file_id, data, old_file_id, old_data, clone_sequence) "
            "SELECT file_id, data, file_id, data, 1 FROM Photos;";
        std::vector<ValueObject> insertArgs;
        if (!ExecuteSql(oldDb, insertUnoffsetSql, insertArgs)) {
            MEDIA_ERR_LOG("FileIdMigrator: insert all old photos to tab_cloned_old_photos failed");
            return false;
        }
        MEDIA_INFO_LOG("FileIdMigrator: inserted all old photos to tab_cloned_old_photos");
        return true;
    }
 
    // 旧机无数据场景，直接返回
    if (oldMax <= 0) {
        MEDIA_INFO_LOG("FileIdMigrator: old has no photos, no need clone");
        return true;
    }

    // 步骤 2: 给 new_max_file_id 加余量 (10%, 限制 1000~10000)
    newMaxExtended_ = AddFileIdOffset(newMax);
    MEDIA_INFO_LOG(
        "FileIdMigrator: file_id newMax=%{public}lld oldMax=%{public}lld extended=%{public}lld (from all tables)",
        static_cast<long long>(newMax), static_cast<long long>(oldMax), static_cast<long long>(newMaxExtended_));

    // 步骤 3: 更新 Photos 表自身的 file_id
    oldMax = fmax(newMax, oldMax);
    if (!UpdatePhotosFileId(oldDb, newMaxExtended_, oldMax)) {
        MEDIA_ERR_LOG("FileIdMigrator: UpdatePhotosFileId failed");
        return false;
    }

    // 步骤 4: 更新其他直接引用 file_id 的表
    if (!UpdateDirectFileIdTables(oldDb, newMaxExtended_, oldMax)) {
        MEDIA_ERR_LOG("FileIdMigrator: UpdateDirectFileIdTables failed");
        return false;
    }

    // 步骤 5: 更新嵌入在字符串中的 file_id
    if (!UpdateEmbeddedFileIds(oldDb, newMaxExtended_, oldMax)) {
        MEDIA_ERR_LOG("FileIdMigrator: UpdateEmbeddedFileIds failed");
        return false;
    }

    return true;
}

int64_t FileIdMigrator::AddFileIdOffset(int64_t newMaxFileId)
{
    // 步骤 2 中的余量计算: offset = newMax * 10%, 限制在 [1000, 10000]
    int64_t offset = newMaxFileId * 10 / 100;
    if (offset < 1000) offset = 1000;
    if (offset > 10000) offset = 10000;
    return newMaxFileId + offset;
}

bool FileIdMigrator::UpdatePhotosFileId(std::shared_ptr<RdbStore> db,
                                        int64_t newMaxExtended, int64_t oldMax)
{
    // 步骤 2.5: 先将未偏移的照片记录到 tab_cloned_old_photos
    // 未偏移的条件: file_id > newMaxExtended
    const std::string insertUnoffsetSql =
        "INSERT INTO tab_cloned_old_photos (file_id, data, old_file_id, old_data, clone_sequence) "
        "SELECT file_id, data, file_id, data, 1 FROM Photos WHERE file_id > ?;";
    std::vector<ValueObject> insertArgs;
    insertArgs.emplace_back(ValueObject(newMaxExtended));
    if (!ExecuteSql(db, insertUnoffsetSql, insertArgs)) {
        MEDIA_ERR_LOG("FileIdMigrator: insert unoffset photos to tab_cloned_old_photos failed");
        return false;
    }
    MEDIA_INFO_LOG("FileIdMigrator: inserted unoffset photos to tab_cloned_old_photos");

    const std::string createTrigger =
        "CREATE TEMP TRIGGER IF NOT EXISTS photos_file_id_update_trigger"
        " AFTER UPDATE ON Photos"
        " WHEN OLD.file_id != NEW.file_id"
        " BEGIN"
        " INSERT INTO tab_cloned_old_photos (file_id, data, old_file_id, old_data, clone_sequence)"
        " VALUES (NEW.file_id, NEW.data, OLD.file_id, OLD.data, 1);"
        " END;";
    std::vector<ValueObject> triggerArgs;
    if (!ExecuteSql(db, createTrigger, triggerArgs)) {
        return false;
    }

    // 步骤 3: UPDATE Photos SET file_id = file_id + old_max WHERE file_id <= new_max_extended
    const std::string sql = "UPDATE Photos SET file_id = file_id + ? WHERE file_id <= ?;";
    std::vector<ValueObject> args;
    args.emplace_back(ValueObject(oldMax));
    args.emplace_back(ValueObject(newMaxExtended));
    if (!ExecuteSql(db, sql, args)) {
        return false;
    }

    // 插入并删除空数据以触发 sqlite_sequence 更新
    int64_t targetId = oldMax + newMaxExtended + 1;
    MEDIA_INFO_LOG("FileIdMigrator: InsertAndDeleteEmptyRecord for Photos, targetId=%{public}lld",
                   static_cast<long long>(targetId));
    if (!InsertAndDeleteEmptyRecord(db, "Photos", "file_id", targetId)) {
        MEDIA_ERR_LOG("FileIdMigrator: insert/delete empty record for Photos failed");
        return false;
    }

    const std::string dropTrigger = "DROP TRIGGER IF EXISTS photos_file_id_update_trigger;";
    return ExecuteSql(db, dropTrigger, triggerArgs);
}

bool FileIdMigrator::UpdateDirectFileIdTables(std::shared_ptr<RdbStore> db,
                                              int64_t newMaxExtended, int64_t oldMax)
{
    // 步骤 4: 依次更新所有直接引用 file_id 的表
    // 表结构: {表名, 字段名, 条件模板 (需包含 " <= ?" 部分)}

    // 所有需要更新的表（与设计文档一致）
    std::vector<DirectUpdate> updates = {
        {"Photos", "associate_file_id", "associate_file_id > 0 AND associate_file_id <= ?"},
        {"PhotoMap", "map_asset", "map_asset > 0 AND map_asset <= ?"},
        {"AnalysisPhotoMap", "map_asset", "map_asset > 0 AND map_asset <= ?"},
        {"tab_analysis_album_asset_map", "map_asset", "map_asset > 0 AND map_asset <= ?"},
        {"tab_analysis_asset_sd_map", "map_asset_source", "map_asset_source > 0 AND map_asset_source <= ?"},
        {"tab_analysis_asset_sd_map", "map_asset_destination",
            "map_asset_destination > 0 AND map_asset_destination <= ?"},
        {"tab_analysis_aesthetics_score", "file_id", "file_id > 0 AND file_id <= ?"},
        {"tab_analysis_affective", "file_id", "file_id > 0 AND file_id <= ?"},
        {"tab_analysis_crop", "file_id", "file_id > 0 AND file_id <= ?"},
        {"tab_analysis_dedup", "file_id", "file_id > 0 AND file_id <= ?"},
        {"tab_analysis_dedup", "group_id_rep", "group_id_rep > 0 AND group_id_rep <= ?"},
        {"tab_analysis_dedup", "group_id_sim", "group_id_sim > 0 AND group_id_sim <= ?"},
        {"tab_analysis_geo_knowledge", "file_id", "file_id > 0 AND file_id <= ?"},
        {"tab_analysis_head", "file_id", "file_id > 0 AND file_id <= ?"},
        {"tab_analysis_image_face", "file_id", "file_id > 0 AND file_id <= ?"},
        {"tab_analysis_label", "file_id", "file_id > 0 AND file_id <= ?"},
        {"tab_analysis_object", "file_id", "file_id > 0 AND file_id <= ?"},
        {"tab_analysis_ocr", "file_id", "file_id > 0 AND file_id <= ?"},
        {"tab_analysis_pose", "file_id", "file_id > 0 AND file_id <= ?"},
        {"tab_analysis_profile", "file_id", "file_id > 0 AND file_id <= ?"},
        {"tab_analysis_recommendation", "file_id", "file_id > 0 AND file_id <= ?"},
        {"tab_analysis_saliency_detect", "file_id", "file_id > 0 AND file_id <= ?"},
        {"tab_analysis_search_index", "file_id", "file_id > 0 AND file_id <= ?"},
        {"tab_analysis_segmentation", "file_id", "file_id > 0 AND file_id <= ?"},
        {"tab_analysis_selection", "file_id", "file_id > 0 AND file_id <= ?"},
        {"tab_analysis_video_face", "file_id", "file_id > 0 AND file_id <= ?"},
        {"tab_analysis_video_label", "file_id", "file_id > 0 AND file_id <= ?"},
        {"tab_analysis_video_total", "file_id", "file_id > 0 AND file_id <= ?"},
        {"tab_analysis_total", "file_id", "file_id > 0 AND file_id <= ?"},
        {"tab_analysis_ai_retouch", "file_id", "file_id > 0 AND file_id <= ?"},
        {"tab_analysis_caption", "file_id", "file_id > 0 AND file_id <= ?"},
        {"tab_photos_ext", "photo_id", "photo_id > 0 AND photo_id <= ?"},
        {"tab_analysis_watermark", "file_id", "file_id > 0 AND file_id <= ?"},
        {"tab_map_photo_map", "file_id", "file_id > 0 AND file_id <= ?"}
    };

    for (const auto &upd : updates) {
        // 使用 PRAGMA table_info 检查表是否存在，避免因表缺失导致错误
        if (!TableExists(db, upd.table)) {
            MEDIA_INFO_LOG("FileIdMigrator: table %{public}s not exist, skip", upd.table.c_str());
            continue;  // 表不存在，跳过
        }

        std::string sql = "UPDATE " + upd.table + " SET " + upd.column +
                          " = " + upd.column + " + ? WHERE " + upd.condition + ";";
        std::vector<ValueObject> args;
        args.emplace_back(ValueObject(oldMax));          // 偏移量
        args.emplace_back(ValueObject(newMaxExtended));  // 上限值

        if (!ExecuteSql(db, sql, args)) {
            MEDIA_ERR_LOG("FileIdMigrator: update %{public}s.%{public}s failed",
                          upd.table.c_str(), upd.column.c_str());
            return false;  // 表存在但更新失败，立即终止
        }

        // 对有自增主键的表，插入并删除空数据以触发 sqlite_sequence 更新
        if (upd.table == "Photos" || upd.table == "tab_analysis_total" ||
            upd.table == "tab_analysis_search_index" || upd.table == "tab_analysis_ocr" ||
            upd.table == "tab_analysis_label") {
            int64_t targetId = oldMax + newMaxExtended + 1;
            MEDIA_INFO_LOG("FileIdMigrator: InsertAndDeleteEmptyRecord for %{public}s, targetId=%{public}lld",
                           upd.table.c_str(), static_cast<long long>(targetId));
            if (!InsertAndDeleteEmptyRecord(db, upd.table, upd.column, targetId)) {
                MEDIA_ERR_LOG("FileIdMigrator: insert/delete empty record for %{public}s failed",
                              upd.table.c_str());
                return false;
            }
        }
    }
    return true;
}

bool FileIdMigrator::UpdateEmbeddedFileIds(std::shared_ptr<RdbStore> db,
                                           int64_t newMaxExtended, int64_t oldMax)
{
    // 步骤 5: 处理包含 file_id 的字符串字段
    static const std::vector<std::tuple<std::string, std::string, std::string>> tables = {
        {"PhotoAlbum", "album_id", "cover_uri"},
        {"AnalysisAlbum", "album_id", "cover_uri"},
        {"tab_highlight_cover_info", "album_id", "cover_key"}
    };

    for (const auto &[table, idCol, valueCol] : tables) {
        if (!TableExists(db, table)) {
            MEDIA_INFO_LOG("FileIdMigrator: table %{public}s not exist, skip embedded update", table.c_str());
            continue;
        }
        if (!UpdateEmbeddedBatch(table, idCol, valueCol, db, newMaxExtended, oldMax)) {
            MEDIA_ERR_LOG("FileIdMigrator: embedded update failed for %{public}s", table.c_str());
            return false;
        }
    }
    return true;
}

bool FileIdMigrator::UpdateEmbeddedBatch(const std::string &table,
                                         const std::string &idCol,
                                         const std::string &valueCol,
                                         std::shared_ptr<RdbStore> db,
                                         int64_t newMaxExtended, int64_t oldMax)
{
    const int64_t kPageSize = 400;  // 每次读取 400 条记录
    int64_t lastMaxId = 0;
    std::string selectSql = "SELECT " + idCol + ", " + valueCol + " FROM " + table +
                            " WHERE " + idCol + " > ? ORDER BY " + idCol + " ASC LIMIT " +
                            std::to_string(kPageSize) + ";";
    std::string updateSql = "UPDATE " + table + " SET " + valueCol + " = ? WHERE " + idCol + " = ?;";

    while (true) {
        std::vector<ValueObject> selArgs { ValueObject(lastMaxId) };
        auto resultSet = db->QuerySql(selectSql, selArgs);
        if (resultSet == nullptr) {
            MEDIA_ERR_LOG("FileIdMigrator: query %{public}s failed", table.c_str());
            return false;
        }

        bool dataFound = false;
        while (resultSet->GoToNextRow() == E_OK) {
            dataFound = true;
            int64_t id = 0;
            std::string value;
            resultSet->GetLong(0, id);
            resultSet->GetString(1, value);

            // 使用正则提取 file_id: 匹配模式 file://media/Photo/数字/ 或 Video/数字/
            std::smatch match;
            if (std::regex_search(value, match, regexPattern_)) {
                std::string fileIdStr = match[2].str();  // 捕获组2是数字
                if (fileIdStr.empty() || !MediaLibraryDataManagerUtils::IsNumber(fileIdStr)) {
                    MEDIA_ERR_LOG("invalid fileIdStr: %{public}s", fileIdStr.c_str());
                    lastMaxId = id;  // 更新游标
                    continue;
                }
                int64_t fileId = std::stoll(fileIdStr);
                if (fileId > 0 && fileId <= newMaxExtended) {
                    // 需要偏移：生成新的 file_id 并替换
                    int64_t newFileId = fileId + oldMax;
                    std::string newValue = std::regex_replace(value, regexPattern_,
                        "file://media/$1/" + std::to_string(newFileId) + "/",
                        std::regex_constants::format_first_only);
                    std::vector<ValueObject> updArgs { ValueObject(newValue), ValueObject(id) };
                    if (!ExecuteSql(db, updateSql, updArgs)) {
                        MEDIA_ERR_LOG("FileIdMigrator: update %{public}s id=%{public}lld failed",
                                      table.c_str(), static_cast<long long>(id));
                        resultSet->Close();
                        return false;
                    }
                }
            }
            lastMaxId = id;  // 更新游标
        }
        resultSet->Close();
        if (!dataFound) break;  // 无更多数据，结束循环
    }
    return true;
}

// album_id 偏移实现

bool FileIdMigrator::MigrateAlbumIds(std::shared_ptr<RdbStore> oldDb,
                                     std::shared_ptr<RdbStore> newDb)
{
    // 步骤 7: 获取最大 album_id
    int64_t newMax = GetMaxAlbumIdFromAllTables(newDb);
    int64_t oldMax = GetMaxAlbumIdFromAllTables(oldDb);
    if (newMax <= 0 || oldMax <= 0) {
        MEDIA_ERR_LOG("FileIdMigrator:max album_id new:%{public}lld old:%{public}lld, no need clone",
                      static_cast<long long>(newMax), static_cast<long long>(oldMax));
        return true;
    }

    // 步骤 8: 加余量 (10%, 限制 10~100)
    int64_t newMaxExtended = AddAlbumIdOffset(newMax);
    MEDIA_INFO_LOG("FileIdMigrator: album_id newMax=%{public}lld oldMax=%{public}lld extended=%{public}lld",
                   static_cast<long long>(newMax), static_cast<long long>(oldMax),
                   static_cast<long long>(newMaxExtended));

    // 步骤 9: 更新 PhotoAlbum 表的 album_id
    oldMax = fmax(newMax, oldMax);
    if (!UpdatePhotoAlbumAlbumId(oldDb, newMaxExtended, oldMax)) {
        MEDIA_ERR_LOG("FileIdMigrator: UpdatePhotoAlbumAlbumId failed");
        return false;
    }

    // 步骤 10: 更新 Photos 表的 owner_album_id
    if (!UpdatePhotosOwnerAlbumId(oldDb, newMaxExtended, oldMax)) {
        MEDIA_ERR_LOG("FileIdMigrator: UpdatePhotosOwnerAlbumId failed");
        return false;
    }

    return true;
}

int64_t FileIdMigrator::AddAlbumIdOffset(int64_t newMaxAlbumId)
{
    int64_t offset = newMaxAlbumId * 10 / 100;
    if (offset < 10) offset = 10;
    if (offset > 100) offset = 100;
    return newMaxAlbumId + offset;
}

bool FileIdMigrator::UpdatePhotoAlbumAlbumId(std::shared_ptr<RdbStore> db,
                                             int64_t newMaxExtended, int64_t oldMax)
{
    const std::string createTrigger =
        "CREATE TEMP TRIGGER IF NOT EXISTS photo_album_id_update_trigger"
        " AFTER UPDATE ON PhotoAlbum"
        " WHEN OLD.album_id != NEW.album_id"
        " BEGIN"
        " UPDATE tab_old_albums"
        " SET album_id = NEW.album_id"
        " WHERE album_id = OLD.album_id AND album_type <> 4096;"
        " END;";
    std::vector<ValueObject> triggerArgs;
    if (!ExecuteSql(db, createTrigger, triggerArgs)) {
        return false;
    }

    std::string sql = "UPDATE PhotoAlbum SET album_id = album_id + ? WHERE album_id <= ?;";
    std::vector<ValueObject> args;
    args.emplace_back(ValueObject(oldMax));
    args.emplace_back(ValueObject(newMaxExtended));
    if (!ExecuteSql(db, sql, args)) {
        return false;
    }

    // 插入并删除空数据以触发 sqlite_sequence 更新
    int64_t targetId = oldMax + newMaxExtended + 1;
    if (!InsertAndDeleteEmptyRecord(db, "PhotoAlbum", "album_id", targetId)) {
        MEDIA_ERR_LOG("FileIdMigrator: insert/delete empty record for PhotoAlbum failed");
        return false;
    }

    const std::string dropTrigger = "DROP TRIGGER IF EXISTS photo_album_id_update_trigger;";
    return ExecuteSql(db, dropTrigger, triggerArgs);
}

bool FileIdMigrator::UpdatePhotosOwnerAlbumId(std::shared_ptr<RdbStore> db,
                                              int64_t newMaxExtended, int64_t oldMax)
{
    std::string sql = "UPDATE Photos SET owner_album_id = owner_album_id + ? WHERE owner_album_id <= ?;";
    std::vector<ValueObject> args;
    args.emplace_back(ValueObject(oldMax));
    args.emplace_back(ValueObject(newMaxExtended));
    return ExecuteSql(db, sql, args);
}

// 通用工具函数

bool FileIdMigrator::TableExists(std::shared_ptr<RdbStore> db, const std::string &tableName)
{
    std::string query = "PRAGMA table_info([" + tableName + "]);";
    auto resultSet = db->QueryByStep(query);
    if (resultSet == nullptr) return false;
    bool exists = (resultSet->GoToNextRow() == E_OK);
    resultSet->Close();
    return exists;
}

bool FileIdMigrator::ExecuteSql(std::shared_ptr<RdbStore> db, const std::string &sql,
                                const std::vector<ValueObject> &args)
{
    int32_t ret = db->ExecuteSql(sql, args);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("FileIdMigrator: ExecuteSql failed, ret=%{public}d, sql=%{public}s", ret, sql.c_str());
        return false;
    }
    return true;
}

bool FileIdMigrator::InsertAndDeleteEmptyRecord(std::shared_ptr<RdbStore> db, const std::string &table,
                                                const std::string &idColumn, int64_t idValue)
{
    std::string insertSql;
    std::vector<ValueObject> insertArgs;

    // 根据表名选择合适的插入方式
    if (table == "Photos") {
        // Photos 表：data 是必需字段，其他都有默认值
        insertSql = "INSERT INTO " + table + " (data) VALUES ('');";
        insertArgs.clear();
    } else if (table == "PhotoAlbum") {
        // PhotoAlbum 表：所有字段都有默认值
        insertSql = "INSERT INTO " + table + " DEFAULT VALUES;";
        insertArgs.clear();
    } else {
        // 其他分析表：file_id 是 UNIQUE 字段，其他字段都有默认值
        insertSql = "INSERT INTO " + table + " (" + idColumn + ") VALUES (?);";
        insertArgs.emplace_back(ValueObject(idValue));
    }

    if (!ExecuteSql(db, insertSql, insertArgs)) {
        MEDIA_ERR_LOG("FileIdMigrator: insert empty record into %{public}s failed", table.c_str());
        return false;
    }

    // 删除刚插入的数据
    std::string deleteSql;
    std::vector<ValueObject> deleteArgs;

    if (table == "Photos" || table == "PhotoAlbum" || table == "tab_analysis_total" ||
        table == "tab_analysis_search_index" || table == "tab_analysis_ocr" || table == "tab_analysis_label") {
        // 自增主键表，删除最后插入的记录
        deleteSql = "DELETE FROM " + table + " WHERE rowid = (SELECT MAX(rowid) FROM " + table + ");";
        deleteArgs.clear();
    } else {
        deleteSql = "DELETE FROM " + table + " WHERE " + idColumn + " = ?;";
        deleteArgs.emplace_back(ValueObject(idValue));
    }

    if (!ExecuteSql(db, deleteSql, deleteArgs)) {
        MEDIA_ERR_LOG("FileIdMigrator: delete empty record from %{public}s failed", table.c_str());
        return false;
    }

    return true;
}

bool FileIdMigrator::UpdateFaceTableFileIds(std::shared_ptr<RdbStore> db,
    const std::unordered_map<int32_t, int32_t>& fileIdMap)
{
    MEDIA_INFO_LOG("FileIdMigrator::UpdateFaceTableFileIds begin, map size=%{public}zu", fileIdMap.size());

    if (!db || fileIdMap.empty()) {
        MEDIA_ERR_LOG("FileIdMigrator: invalid db or empty fileIdMap");
        return false;
    }

    // 需要更新的表列表
    const std::vector<std::string> tables = {
        "tab_analysis_image_face",
        "tab_analysis_video_face"
    };

    // 构建 CASE WHEN 语句用于批量更新
    std::ostringstream caseSql;
    std::ostringstream inClause;
    bool first = true;
    caseSql << "CASE file_id";
    for (const auto& [oldFileId, newFileId] : fileIdMap) {
        if (oldFileId == newFileId) continue;
        caseSql << " WHEN " << oldFileId << " THEN " << newFileId;
        if (!first) {
            inClause << ",";
        }
        inClause << oldFileId;
        first = false;
    }
    caseSql << " ELSE file_id END";

    std::string inClauseStr = inClause.str();
    if (inClauseStr.empty()) {
        MEDIA_INFO_LOG("FileIdMigrator: inClause is empty, no data to update");
        return true;
    }

    // 更新每个表
    for (const auto& table : tables) {
        std::string updateSql = "UPDATE " + table + " SET file_id = " + caseSql.str() +
                                " WHERE file_id IN (" + inClauseStr + ");";

        if (!ExecuteSql(db, updateSql, {})) {
            MEDIA_ERR_LOG("FileIdMigrator: update %{public}s failed", table.c_str());
            return false;
        }

        MEDIA_INFO_LOG("FileIdMigrator: updated %{public}s successfully", table.c_str());
    }

    MEDIA_INFO_LOG("FileIdMigrator::UpdateFaceTableFileIds success");
    return true;
}

bool FileIdMigrator::UpdateAnalysisTotalFields(std::shared_ptr<RdbStore> db,
    const std::unordered_map<int32_t, int32_t>& fileIdMap)
{
    const std::vector<std::string> fields = {"face", "selection"};
    return UpdateAnalysisTotalFieldsByConfig(db, fileIdMap, fields);
}

bool FileIdMigrator::UpdateAnalysisTotalFieldsByConfig(std::shared_ptr<RdbStore> db,
    const std::unordered_map<int32_t, int32_t>& fileIdMap,
    const std::vector<std::string>& fields)
{
    MEDIA_INFO_LOG("FileIdMigrator::UpdateAnalysisTotalFieldsByConfig begin, map size=%{public}zu, fields=%{public}zu",
        fileIdMap.size(), fields.size());

    if (!db || fileIdMap.empty() || fields.empty()) {
        MEDIA_ERR_LOG("FileIdMigrator: invalid db, empty fileIdMap or fields");
        return false;
    }

    // 为每个字段构建 CASE WHEN 语句
    std::map<std::string, std::string> fieldCaseSqls;
    std::ostringstream inClause;
    bool first = true;

    for (const auto& [oldFileId, newFileId] : fileIdMap) {
        if (oldFileId == newFileId) continue;
        if (!first) {
            inClause << ",";
        }
        inClause << newFileId;
        first = false;
    }

    std::string inClauseStr = inClause.str();
    CHECK_AND_RETURN_RET_INFO_LOG(!inClauseStr.empty(), true,
        "FileIdMigrator: inClause is empty, no data to update");

    for (const auto& field : fields) {
        std::ostringstream caseSql;
        caseSql << "CASE file_id";
        for (const auto& [oldFileId, newFileId] : fileIdMap) {
            if (oldFileId == newFileId) continue;
            caseSql << " WHEN " << newFileId << " THEN (SELECT " << field
                    << " FROM tab_analysis_total WHERE file_id = " << oldFileId << ")";
        }
        caseSql << " ELSE " << field << " END";
        fieldCaseSqls[field] = caseSql.str();
    }

    // 构建 SET 子句
    std::ostringstream setClause;
    first = true;
    for (const auto& [field, caseSql] : fieldCaseSqls) {
        if (!first) {
            setClause << ", ";
        }
        setClause << field << " = " << caseSql;
        first = false;
    }

    // 执行更新
    std::string updateSql = "UPDATE tab_analysis_total SET " + setClause.str() +
                            " WHERE file_id IN (" + inClauseStr + ");";

    CHECK_AND_RETURN_RET_LOG(ExecuteSql(db, updateSql, {}), false,
        "FileIdMigrator: update tab_analysis_total fields failed");

    MEDIA_INFO_LOG("FileIdMigrator::UpdateAnalysisTotalFieldsByConfig success");
    return true;
}

// 智慧相册 album_id 偏移实现

bool FileIdMigrator::MigrateSmartAlbumIds(std::shared_ptr<RdbStore> oldDb,
    std::shared_ptr<RdbStore> newDb)
{
    MEDIA_INFO_LOG("FileIdMigrator::MigrateSmartAlbumIds begin");
    if (!oldDb || !newDb) {
        MEDIA_ERR_LOG("FileIdMigrator: invalid db handles for smart album migration");
        return false;
    }

    // 获取新机和旧机中智慧相册相关表的最大 album_id
    int64_t newMax = GetMaxSmartAlbumId(newDb);
    int64_t oldMax = GetMaxSmartAlbumId(oldDb);

    if (newMax <= 0) {
        MEDIA_INFO_LOG("FileIdMigrator: newMax smart album_id is 0, no need migration");
        return true;
    }
    if (oldMax <= 0) {
        MEDIA_INFO_LOG("FileIdMigrator: oldMax smart album_id is 0, no need migration");
        return true;
    }

    // 加余量 (10%, 限制 10~100)
    int64_t newMaxExtended = AddAlbumIdOffset(newMax);
    MEDIA_INFO_LOG("FileIdMigrator: smart album_id newMax=%{public}lld oldMax=%{public}lld extended=%{public}lld",
        static_cast<long long>(newMax), static_cast<long long>(oldMax),
        static_cast<long long>(newMaxExtended));

    // 更新智慧相册相关表的 album_id
    oldMax = fmax(newMax, oldMax);
    if (!UpdateSmartAlbumTables(oldDb, newMaxExtended, oldMax)) {
        MEDIA_ERR_LOG("FileIdMigrator: UpdateSmartAlbumTables failed");
        return false;
    }

    MEDIA_INFO_LOG("FileIdMigrator::MigrateSmartAlbumIds success");
    return true;
}

int64_t FileIdMigrator::GetMaxSmartAlbumId(std::shared_ptr<RdbStore> db)
{
    int64_t maxId = 0;

    // 查询 AnalysisAlbum 表的最大 album_id
    auto resultSet = db->QuerySql("SELECT MAX(album_id) FROM AnalysisAlbum");
    if (resultSet != nullptr && resultSet->GoToNextRow() == E_OK) {
        resultSet->GetLong(0, maxId);
        resultSet->Close();
    }

    // 查询 AnalysisPhotoMap 表的最大 map_album
    resultSet = db->QuerySql("SELECT MAX(map_album) FROM AnalysisPhotoMap");
    if (resultSet != nullptr && resultSet->GoToNextRow() == E_OK) {
        int64_t currentMax = 0;
        resultSet->GetLong(0, currentMax);
        maxId = fmax(maxId, currentMax);
        resultSet->Close();
    }

    // 查询 tab_highlight_album 表的最大 album_id
    resultSet = db->QuerySql("SELECT MAX(album_id) FROM tab_highlight_album");
    if (resultSet != nullptr && resultSet->GoToNextRow() == E_OK) {
        int64_t currentMax = 0;
        resultSet->GetLong(0, currentMax);
        maxId = fmax(maxId, currentMax);
        resultSet->Close();
    }

    return maxId;
}

bool FileIdMigrator::UpdateSmartAlbumTables(std::shared_ptr<RdbStore> db,
    int64_t newMaxExtended, int64_t oldMax)
{
    if (!CreateAlbumIdUpdateTrigger(db)) {
        return false;
    }

    bool success = UpdateAllSmartAlbumTables(db, newMaxExtended, oldMax);

    if (!DropAlbumIdUpdateTrigger(db)) {
        return false;
    }

    return success;
}

bool FileIdMigrator::CreateAlbumIdUpdateTrigger(std::shared_ptr<RdbStore> db)
{
    const std::string createTrigger =
        "CREATE TEMP TRIGGER IF NOT EXISTS analysis_album_id_update_trigger"
        " AFTER UPDATE ON AnalysisAlbum"
        " WHEN OLD.album_id != NEW.album_id"
        " BEGIN"
        " UPDATE tab_old_albums"
        " SET album_id = NEW.album_id"
        " WHERE album_id = OLD.album_id AND album_type = 4096;"
        " END;";
    std::vector<ValueObject> triggerArgs;
    return ExecuteSql(db, createTrigger, triggerArgs);
}

bool FileIdMigrator::UpdateAllSmartAlbumTables(std::shared_ptr<RdbStore> db,
    int64_t newMaxExtended, int64_t oldMax)
{
    std::vector<DirectUpdate> updates = {
        {"AnalysisAlbum", "album_id", "album_id > 0 AND album_id <= ?", true},
        {"AnalysisPhotoMap", "map_album", "map_album > 0 AND map_album <= ?", false},
        {"tab_highlight_album", "album_id", "album_id > 0 AND album_id <= ?", false},
        {"tab_highlight_album", "ai_album_id", "ai_album_id > 0 AND ai_album_id <= ?", false},
    };

    for (const auto &upd : updates) {
        if (!UpdateSmartAlbumTable(db, upd, newMaxExtended, oldMax)) {
            return false;
        }
    }

    return true;
}

bool FileIdMigrator::UpdateSmartAlbumTable(std::shared_ptr<RdbStore> db,
    const DirectUpdate& upd, int64_t newMaxExtended, int64_t oldMax)
{
    if (!TableExists(db, upd.table)) {
        MEDIA_INFO_LOG("FileIdMigrator: table %{public}s not exist, skip", upd.table.c_str());
        return true;
    }

    std::string sql = "UPDATE " + upd.table + " SET " + upd.column +
                      " = " + upd.column + " + ? WHERE " + upd.condition + ";";
    std::vector<ValueObject> args;
    args.emplace_back(ValueObject(oldMax));
    args.emplace_back(ValueObject(newMaxExtended));

    if (!ExecuteSql(db, sql, args)) {
        MEDIA_ERR_LOG("FileIdMigrator: update %{public}s.%{public}s failed",
                      upd.table.c_str(), upd.column.c_str());
        return false;
    }

    if (upd.needSequenceUpdate) {
        int64_t targetId = oldMax + newMaxExtended + 1;
        MEDIA_INFO_LOG("FileIdMigrator: InsertAndDeleteEmptyRecord for %{public}s, targetId=%{public}lld",
                       upd.table.c_str(), static_cast<long long>(targetId));
        if (!InsertAndDeleteEmptyRecord(db, upd.table, upd.column, targetId)) {
            MEDIA_ERR_LOG("FileIdMigrator: insert/delete empty record for %{public}s failed",
                          upd.table.c_str());
            return false;
        }
    }

    return true;
}

bool FileIdMigrator::DropAlbumIdUpdateTrigger(std::shared_ptr<RdbStore> db)
{
    const std::string dropTrigger = "DROP TRIGGER IF EXISTS analysis_album_id_update_trigger;";
    std::vector<ValueObject> triggerArgs;
    return ExecuteSql(db, dropTrigger, triggerArgs);
}

bool FileIdMigrator::MigrateAnalysisTotalScore(std::shared_ptr<RdbStore> oldDb,
    std::shared_ptr<RdbStore> newDb, const std::unordered_map<int32_t, int32_t>& fileIdMap)
{
    MEDIA_INFO_LOG("FileIdMigrator::MigrateAnalysisTotalScore begin, map size=%{public}zu", fileIdMap.size());

    if (!oldDb || !newDb || fileIdMap.empty()) {
        MEDIA_ERR_LOG("FileIdMigrator: invalid db handles or empty fileIdMap");
        return false;
    }

    if (!SetBit20InTotalScore(oldDb)) {
        return false;
    }

    if (!CopyTotalScoreFromNewDb(oldDb, newDb)) {
        return false;
    }

    if (!UpdateTotalScoreByMapping(oldDb, fileIdMap)) {
        return false;
    }

    MEDIA_INFO_LOG("FileIdMigrator::MigrateAnalysisTotalScore success");
    return true;
}

bool FileIdMigrator::SetBit20InTotalScore(std::shared_ptr<RdbStore> oldDb)
{
    const std::string sql = "UPDATE tab_analysis_total SET total_score = total_score | 1048576;";
    if (!ExecuteSql(oldDb, sql, {})) {
        MEDIA_ERR_LOG("FileIdMigrator: SetBit20InTotalScore failed");
        return false;
    }
    MEDIA_INFO_LOG("FileIdMigrator: SetBit20InTotalScore completed");
    return true;
}

bool FileIdMigrator::CopyTotalScoreFromNewDb(std::shared_ptr<RdbStore> oldDb,
    std::shared_ptr<RdbStore> newDb)
{
    std::string querySql = "SELECT file_id, total_score FROM tab_analysis_total WHERE status <> -1;";
    auto resultSet = newDb->QuerySql(querySql);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("FileIdMigrator: CopyTotalScoreFromNewDb - query failed");
        return false;
    }

    std::unordered_map<int32_t, int32_t> scoreMap;
    while (resultSet->GoToNextRow() == E_OK) {
        int32_t fileId = 0;
        int32_t totalScore = 0;
        resultSet->GetInt(0, fileId);
        resultSet->GetInt(1, totalScore);
        scoreMap[fileId] = totalScore;
    }
    resultSet->Close();
    MEDIA_INFO_LOG("FileIdMigrator: CopyTotalScoreFromNewDb - queried %{public}zu records", scoreMap.size());

    if (scoreMap.empty()) {
        return true;
    }

    std::ostringstream caseSql;
    std::ostringstream inClause;
    bool first = true;
    caseSql << "CASE file_id";

    for (const auto& [fileId, totalScore] : scoreMap) {
        caseSql << " WHEN " << fileId << " THEN " << totalScore;
        if (!first) {
            inClause << ",";
        }
        inClause << fileId;
        first = false;
    }
    caseSql << " ELSE total_score END";

    std::string inClauseStr = inClause.str();
    if (inClauseStr.empty()) {
        MEDIA_INFO_LOG("FileIdMigrator: inClause is empty, no data to update");
        return true;
    }

    const std::string updateSql = "UPDATE tab_analysis_total SET total_score = " + caseSql.str() +
        " WHERE file_id IN (" + inClauseStr + ");";

    if (!ExecuteSql(oldDb, updateSql, {})) {
        MEDIA_ERR_LOG("FileIdMigrator: CopyTotalScoreFromNewDb - update failed");
        return false;
    }
    MEDIA_INFO_LOG("FileIdMigrator: CopyTotalScoreFromNewDb completed");
    return true;
}

bool FileIdMigrator::UpdateTotalScoreByMapping(std::shared_ptr<RdbStore> oldDb,
    const std::unordered_map<int32_t, int32_t>& fileIdMap)
{
    std::ostringstream inClause;
    std::ostringstream caseSql;
    bool first = true;
    caseSql << "CASE file_id";

    for (const auto& [oldFileId, newFileId] : fileIdMap) {
        if (oldFileId == newFileId) continue;
        caseSql << " WHEN " << oldFileId << " THEN ((SELECT total_score FROM tab_analysis_total"
                 << " WHERE file_id = " << oldFileId << ") & 4 | 1048576) | (SELECT total_score FROM tab_analysis_total"
                 << " WHERE file_id = " << newFileId << ")";
        if (!first) {
            inClause << ",";
        }
        inClause << oldFileId;
        first = false;
    }
    caseSql << " ELSE total_score END";

    std::string inClauseStr = inClause.str();
    if (inClauseStr.empty()) {
        MEDIA_INFO_LOG("FileIdMigrator: No file IDs to update, skipping UpdateTotalScoreByMapping");
        return true;
    }

    const std::string updateSql = "UPDATE tab_analysis_total SET total_score = " + caseSql.str() +
                                  " WHERE file_id IN (" + inClauseStr + ");";

    if (!ExecuteSql(oldDb, updateSql, {})) {
        MEDIA_ERR_LOG("FileIdMigrator: UpdateTotalScoreByMapping failed");
        return false;
    }
    MEDIA_INFO_LOG("FileIdMigrator: UpdateTotalScoreByMapping completed");
    return true;
}
// LCOV_EXCL_STOP
} // namespace Media
} // namespace OHOS