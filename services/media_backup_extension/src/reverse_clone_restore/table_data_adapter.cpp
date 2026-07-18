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

#include "table_data_adapter.h"
#include "backup_database_utils.h"
#include "backup_file_utils.h"
#include "backup_const.h"
#include "media_file_utils.h"
#include "medialibrary_errno.h"
#include <mutex>

namespace OHOS {
namespace Media {

// 表处理类型配置
static const std::unordered_map<std::string, TableProcessType> TABLE_PROCESS_CONFIG = {
    // DROP_AND_INSERT: 直接drop再create，并继承新机的数据
    {"album_plugin", TableProcessType::DROP_AND_INSERT},
    {"Audios", TableProcessType::DROP_AND_INSERT},
    {"BundlePermission", TableProcessType::DROP_AND_INSERT},
    {"CategorySmartAlbumMap", TableProcessType::DROP_AND_INSERT},
    {"Device", TableProcessType::DROP_AND_INSERT},
    {"download_resources_task_records", TableProcessType::DROP_AND_INSERT},
    {"Error", TableProcessType::DROP_AND_INSERT},
    {"Files", TableProcessType::DROP_AND_INSERT},
    {"FormMap", TableProcessType::DROP_AND_INSERT},
    {"LakeAlbum", TableProcessType::DROP_AND_INSERT},
    {"MediaTypeDirectory", TableProcessType::DROP_AND_INSERT},
    {"OrderBackAlbum", TableProcessType::DROP_AND_INSERT},
    {"PhotoMap", TableProcessType::DROP_AND_INSERT},
    {"RefreshAlbum", TableProcessType::DROP_AND_INSERT},
    {"RemoteThumbnailMap", TableProcessType::DROP_AND_INSERT},
    {"selected_node_asset_map", TableProcessType::DROP_AND_INSERT},
    {"SmartAlbum", TableProcessType::DROP_AND_INSERT},
    {"SmartMap", TableProcessType::DROP_AND_INSERT},
    {"tab_analysis_highlight_events", TableProcessType::DROP_AND_INSERT},
    {"tab_analysis_pet_face", TableProcessType::DROP_AND_INSERT},
    {"tab_analysis_pet_tag", TableProcessType::DROP_AND_INSERT},
    {"tab_analysis_progress", TableProcessType::DROP_AND_INSERT},
    {"tab_analysis_similar_face", TableProcessType::DROP_AND_INSERT},
    {"tab_analysis_UG", TableProcessType::DROP_AND_INSERT},
    {"tab_analysis_video_aesthetics_score", TableProcessType::DROP_AND_INSERT},
    {"tab_asset_and_album_operation", TableProcessType::DROP_AND_INSERT},
    {"tab_custom_records", TableProcessType::DROP_AND_INSERT},
    {"tab_facard_photos", TableProcessType::DROP_AND_INSERT},
    {"tab_medialibrary_business_record", TableProcessType::DROP_AND_INSERT},
    {"tab_node_description", TableProcessType::DROP_AND_INSERT},
    {"tab_old_photos", TableProcessType::DROP_AND_INSERT},
    {"tab_tag_node_index", TableProcessType::DROP_AND_INSERT},
    {"tab_trailer_story_album", TableProcessType::DROP_AND_INSERT},
    {"tab_user_photography_info", TableProcessType::DROP_AND_INSERT},
    {"UniqueNumber", TableProcessType::DROP_AND_INSERT},
    {"UriPermission", TableProcessType::DROP_AND_INSERT},
    {"UriSensitive", TableProcessType::DROP_AND_INSERT},

    // DROP_AND_CREATE: 仅drop再create，不继承新机的数据（创建空表）
    {"tab_old_albums", TableProcessType::DROP_AND_CREATE},
    {"tab_cloned_old_photos", TableProcessType::DROP_AND_CREATE},

    // DROP_ONLY: 仅删除表，不重建
    {"ddms_data_search_aux_config", TableProcessType::DROP_ONLY},
    {"naturalbase_rdb_aux_metadata", TableProcessType::DROP_ONLY},
    {"PhotosAlbumBackupForSaveAnalysisData", TableProcessType::DROP_ONLY},

    // SKIP: 跳过，不用处理的表
    {"AnalysisAlbum", TableProcessType::SKIP},
    {"AnalysisPhotoMap", TableProcessType::SKIP},
    {"ConfigInfo", TableProcessType::SKIP},
    {"PhotoAlbum", TableProcessType::SKIP},
    {"Photos", TableProcessType::SKIP},
    {"tab_analysis_aesthetics_score", TableProcessType::SKIP},
    {"tab_analysis_affective", TableProcessType::SKIP},
    {"tab_analysis_ai_retouch", TableProcessType::SKIP},
    {"tab_analysis_album_asset_map", TableProcessType::SKIP},
    {"tab_analysis_asset_sd_map", TableProcessType::SKIP},
    {"tab_analysis_atom_event", TableProcessType::SKIP},
    {"tab_analysis_composition", TableProcessType::SKIP},
    {"tab_analysis_crop", TableProcessType::SKIP},
    {"tab_analysis_dedup", TableProcessType::SKIP},
    {"tab_analysis_face_tag", TableProcessType::SKIP},
    {"tab_analysis_geo_dictionary", TableProcessType::SKIP},
    {"tab_analysis_geo_knowledge", TableProcessType::SKIP},
    {"tab_analysis_head", TableProcessType::SKIP},
    {"tab_analysis_image_face", TableProcessType::SKIP},
    {"tab_analysis_label", TableProcessType::SKIP},
    {"tab_analysis_object", TableProcessType::SKIP},
    {"tab_analysis_ocr", TableProcessType::SKIP},
    {"tab_analysis_pose", TableProcessType::SKIP},
    {"tab_analysis_profile", TableProcessType::SKIP},
    {"tab_analysis_recommendation", TableProcessType::SKIP},
    {"tab_analysis_saliency_detect", TableProcessType::SKIP},
    {"tab_analysis_segmentation", TableProcessType::SKIP},
    {"tab_analysis_selection", TableProcessType::SKIP},
    {"tab_analysis_video_face", TableProcessType::SKIP},
    {"tab_analysis_video_label", TableProcessType::SKIP},
    {"tab_analysis_video_total", TableProcessType::SKIP},
    {"tab_highlight_album", TableProcessType::SKIP},
    {"tab_highlight_cover_info", TableProcessType::SKIP},
    {"tab_highlight_play_info", TableProcessType::SKIP},
    {"tab_analysis_search_index", TableProcessType::SKIP},
    {"tab_analysis_total", TableProcessType::SKIP},
    {"tab_photos_ext", TableProcessType::SKIP},
    {"tab_map_photo_map", TableProcessType::SKIP},
};

// TableSchemaHandler 实现
// LCOV_EXCL_START
TableSchema TableSchemaHandler::GetTableSchema(
    std::shared_ptr<NativeRdb::RdbStore> rdbStore, const std::string &tableName)
{
    TableSchema schema;
    schema.tableName = tableName;

    if (!GetTableCreateSql(rdbStore, tableName, schema)) {
        return schema;
    }

    GetTableColumns(rdbStore, tableName, schema);
    GetTableIndexes(rdbStore, tableName, schema);
    GetTableTriggers(rdbStore, tableName, schema);

    MEDIA_INFO_LOG("GetTableSchema: table %{public}s, columns=%{public}zu, indexes=%{public}zu, triggers=%{public}zu",
        tableName.c_str(), schema.columns.size(), schema.indexSqls.size(), schema.triggerSqls.size());

    return schema;
}

bool TableSchemaHandler::GetTableCreateSql(std::shared_ptr<NativeRdb::RdbStore> rdbStore,
    const std::string &tableName, TableSchema &schema)
{
    std::string sql = "SELECT sql FROM sqlite_master WHERE type='table' AND name=?";
    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(rdbStore, sql, {tableName});
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("GetTableCreateSql: failed to query schema for table %{public}s", tableName.c_str());
        return false;
    }

    int32_t columnIndex = 0;
    if (resultSet->GetColumnIndex("sql", columnIndex) == NativeRdb::E_OK) {
        resultSet->GetString(columnIndex, schema.sqlCreate);
    }
    resultSet->Close();

    if (schema.sqlCreate.empty()) {
        MEDIA_ERR_LOG("GetTableCreateSql: empty sql for table %{public}s", tableName.c_str());
        return false;
    }

    return true;
}

void TableSchemaHandler::GetTableColumns(std::shared_ptr<NativeRdb::RdbStore> rdbStore,
    const std::string &tableName, TableSchema &schema)
{
    std::string sql = "PRAGMA table_info(" + tableName + ")";
    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(rdbStore, sql, {});
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("GetTableColumns: failed to query pragma for table %{public}s", tableName.c_str());
        return;
    }

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t nameIndex = 0;
        int32_t typeIndex = 0;
        if (resultSet->GetColumnIndex("name", nameIndex) == NativeRdb::E_OK &&
            resultSet->GetColumnIndex("type", typeIndex) == NativeRdb::E_OK) {
            std::string columnName;
            std::string columnType;
            resultSet->GetString(nameIndex, columnName);
            resultSet->GetString(typeIndex, columnType);

            schema.columns.push_back(columnName);
            schema.columnTypes[columnName] = columnType;
        }
    }
    resultSet->Close();
}

void TableSchemaHandler::GetTableIndexes(std::shared_ptr<NativeRdb::RdbStore> rdbStore,
    const std::string &tableName, TableSchema &schema)
{
    std::string sql = "SELECT sql FROM sqlite_master WHERE type='index' AND tbl_name=? AND sql IS NOT NULL";
    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(rdbStore, sql, {tableName});
    if (resultSet == nullptr) {
        return;
    }

    int32_t columnIndex = 0;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        if (resultSet->GetColumnIndex("sql", columnIndex) == NativeRdb::E_OK) {
            std::string indexSql;
            resultSet->GetString(columnIndex, indexSql);
            if (!indexSql.empty()) {
                schema.indexSqls.push_back(indexSql);
            }
        }
    }
    resultSet->Close();
}

void TableSchemaHandler::GetTableTriggers(std::shared_ptr<NativeRdb::RdbStore> rdbStore,
    const std::string &tableName, TableSchema &schema)
{
    std::string sql = "SELECT sql FROM sqlite_master WHERE type='trigger' AND tbl_name=? AND sql IS NOT NULL";
    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(rdbStore, sql, {tableName});
    if (resultSet == nullptr) {
        return;
    }

    int32_t columnIndex = 0;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        if (resultSet->GetColumnIndex("sql", columnIndex) == NativeRdb::E_OK) {
            std::string triggerSql;
            resultSet->GetString(columnIndex, triggerSql);
            if (!triggerSql.empty()) {
                schema.triggerSqls.push_back(triggerSql);
            }
        }
    }
    resultSet->Close();
}

int32_t TableSchemaHandler::DropTable(std::shared_ptr<NativeRdb::RdbStore> rdbStore, const std::string &tableName)
{
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("DropTable: rdbStore is null for table %{public}s", tableName.c_str());
        return E_ERR;
    }

    std::string sql = "DROP TABLE IF EXISTS " + tableName;
    int32_t ret = rdbStore->ExecuteSql(sql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("DropTable: failed to drop table %{public}s, ret=%{public}d", tableName.c_str(), ret);
        return E_ERR;
    }

    MEDIA_INFO_LOG("DropTable: dropped table %{public}s", tableName.c_str());
    return E_OK;
}

int32_t TableSchemaHandler::CreateTable(std::shared_ptr<NativeRdb::RdbStore> rdbStore, const TableSchema &schema)
{
    if (schema.sqlCreate.empty()) {
        MEDIA_ERR_LOG("CreateTable: empty sql for table %{public}s", schema.tableName.c_str());
        return E_ERR;
    }

    // 1. 创建表
    int32_t ret = rdbStore->ExecuteSql(schema.sqlCreate);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("CreateTable: failed to create table %{public}s, ret=%{public}d", schema.tableName.c_str(), ret);
        return E_ERR;
    }

    MEDIA_INFO_LOG("CreateTable: created table %{public}s", schema.tableName.c_str());

    // 2. 创建索引
    for (const auto &indexSql : schema.indexSqls) {
        ret = rdbStore->ExecuteSql(indexSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_WARN_LOG("CreateTable: failed to create index for table %{public}s, ret=%{public}d, sql=%{public}s",
                schema.tableName.c_str(), ret, indexSql.c_str());
        }
    }

    // 3. 创建触发器
    for (const auto &triggerSql : schema.triggerSqls) {
        ret = rdbStore->ExecuteSql(triggerSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_WARN_LOG("CreateTable: failed to create trigger for table %{public}s, ret=%{public}d, sql=%{public}s",
                schema.tableName.c_str(), ret, triggerSql.c_str());
        }
    }

    MEDIA_INFO_LOG("CreateTable: created %{public}zu indexes and %{public}zu triggers for table %{public}s",
        schema.indexSqls.size(), schema.triggerSqls.size(), schema.tableName.c_str());

    return E_OK;
}

int32_t TableSchemaHandler::RecreateTableFromDst(std::shared_ptr<NativeRdb::RdbStore> srcDb,
    std::shared_ptr<NativeRdb::RdbStore> dstDb, const std::string &tableName)
{
    // 1. 从dstdb获取表结构
    TableSchema schema = GetTableSchema(dstDb, tableName);
    if (schema.sqlCreate.empty()) {
        MEDIA_ERR_LOG("RecreateTableFromDst: failed to get schema for table %{public}s", tableName.c_str());
        return E_ERR;
    }

    // 2. 在srcdb中创建表
    int32_t ret = CreateTable(srcDb, schema);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("RecreateTableFromDst: failed to create table %{public}s", tableName.c_str());
        return ret;
    }

    return E_OK;
}

TableSchema TableSchemaHandler::ParseSchemaFromResultSet(
    std::shared_ptr<NativeRdb::ResultSet> resultSet, const std::string &tableName)
{
    TableSchema schema;
    schema.tableName = tableName;
    // 解析逻辑已在GetTableSchema中实现
    return schema;
}

// TableDataCopier 实现

int32_t TableDataCopier::CopyTableData(std::shared_ptr<NativeRdb::RdbStore> destDb,
    std::shared_ptr<NativeRdb::RdbStore> sourceDb, const std::string &tableName)
{
    MEDIA_INFO_LOG("CopyTableData: start copying table %{public}s", tableName.c_str());

    // 添加数据库连接有效性检查
    if (destDb == nullptr || sourceDb == nullptr) {
        MEDIA_ERR_LOG("CopyTableData: destDb or sourceDb is null for table %{public}s", tableName.c_str());
        return E_ERR;
    }

    // 1. 获取表的所有列名
    std::vector<std::string> columns = GetColumnNames(sourceDb, tableName);
    if (columns.empty()) {
        MEDIA_ERR_LOG("CopyTableData: failed to get columns for table %{public}s", tableName.c_str());
        return E_ERR;
    }

    // 2. 从sourceDb查询所有数据
    std::string columnsStr = "";
    for (size_t i = 0; i < columns.size(); i++) {
        columnsStr += columns[i];
        if (i < columns.size() - 1) {
            columnsStr += ", ";
        }
    }

    std::string querySql = "SELECT " + columnsStr + " FROM " + tableName;
    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(sourceDb, querySql);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("CopyTableData: query result is null for table %{public}s", tableName.c_str());
        return E_ERR;
    }

    // 4. 先将所有数据读取到内存中，避免长时间持有ResultSet（修复多线程并发问题）
    std::vector<NativeRdb::ValuesBucket> valuesList;
    int32_t rowCount = 0;

    while (true) {
        // 添加ResultSet有效性检查
        if (resultSet == nullptr) {
            MEDIA_ERR_LOG(
                "CopyTableData: resultSet became null during iteration for table %{public}s", tableName.c_str());
            break;
        }

        int32_t ret = resultSet->GoToNextRow();
        if (ret != NativeRdb::E_OK) {
            // 正常结束遍历
            break;
        }

        NativeRdb::ValuesBucket values;
        bool rowValid = true;

        for (const auto &column : columns) {
            std::string columnType = "TEXT";  // 默认类型
            int32_t ret = GetValueFromResultSet(resultSet.get(), column, columnType, values);
            if (ret != E_OK) {
                MEDIA_WARN_LOG("CopyTableData: failed to get value for column %{public}s in table %{public}s",
                    column.c_str(),
                    tableName.c_str());
                // 继续处理其他列
            }
        }

        if (rowValid) {
            valuesList.push_back(values);
            rowCount++;
        }
    }

    // 安全关闭ResultSet
    if (resultSet != nullptr) {
        resultSet->Close();
        resultSet.reset();
    }

    MEDIA_INFO_LOG("CopyTableData: read %{public}d rows from source table %{public}s", rowCount, tableName.c_str());

    // 5. 批量插入到destDb（此时ResultSet已关闭，降低并发风险）
    if (!valuesList.empty()) {
        int32_t ret = BatchInsert(destDb, tableName, valuesList);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("CopyTableData: failed to insert data for table %{public}s", tableName.c_str());
            return ret;
        }
    }

    MEDIA_INFO_LOG("CopyTableData: successfully copied %{public}zu rows for table %{public}s",
        valuesList.size(),
        tableName.c_str());

    return E_OK;
}

std::vector<std::string> TableDataCopier::GetColumnNames(
    std::shared_ptr<NativeRdb::RdbStore> rdbStore, const std::string &tableName)
{
    std::vector<std::string> columns;

    std::string sql = "PRAGMA table_info(" + tableName + ")";
    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(rdbStore, sql, {});
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("GetColumnNames: failed to query pragma for table %{public}s", tableName.c_str());
        return columns;
    }

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t nameIndex = 0;
        if (resultSet->GetColumnIndex("name", nameIndex) == NativeRdb::E_OK) {
            std::string columnName;
            resultSet->GetString(nameIndex, columnName);
            columns.push_back(columnName);
        }
    }
    resultSet->Close();

    return columns;
}

int32_t TableDataCopier::BatchInsert(std::shared_ptr<NativeRdb::RdbStore> rdbStore, const std::string &tableName,
    const std::vector<NativeRdb::ValuesBucket> &values)
{
    if (values.empty()) {
        return E_OK;
    }

    // 使用RdbStore提供的批量插入接口
    int64_t insertNum = 0;
    int32_t ret = rdbStore->BatchInsert(insertNum, tableName, values);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("BatchInsert: failed to insert rows into table %{public}s, ret=%{public}d",
            tableName.c_str(), ret);
        return E_ERR;
    }

    MEDIA_INFO_LOG("BatchInsert: inserted %{public}lld rows into table %{public}s",
        static_cast<long long>(insertNum), tableName.c_str());
    return E_OK;
}

int32_t TableDataCopier::GetValueFromResultSet(NativeRdb::ResultSet *resultSet, const std::string &columnName,
    const std::string &columnTypeStr, NativeRdb::ValuesBucket &values)
{
    int32_t columnIndex = 0;
    if (resultSet->GetColumnIndex(columnName, columnIndex) != NativeRdb::E_OK) {
        MEDIA_WARN_LOG("GetValueFromResultSet: failed to get column index for %{public}s", columnName.c_str());
        return E_ERR;
    }

    // 检查是否为NULL
    bool isNull = false;
    if (resultSet->IsColumnNull(columnIndex, isNull) != NativeRdb::E_OK || isNull) {
        values.PutNull(columnName);
        return E_OK;
    }

    // 根据列类型获取值
    NativeRdb::ColumnType columnType = NativeRdb::ColumnType::TYPE_STRING;
    if (resultSet->GetColumnType(columnIndex, columnType) != NativeRdb::E_OK) {
        // 默认按字符串处理
        std::string strValue;
        if (resultSet->GetString(columnIndex, strValue) == NativeRdb::E_OK) {
            values.PutString(columnName, strValue);
        }
        return E_OK;
    }

    switch (columnType) {
        case NativeRdb::ColumnType::TYPE_INTEGER: {
            int64_t intValue = 0;
            if (resultSet->GetLong(columnIndex, intValue) == NativeRdb::E_OK) {
                values.PutLong(columnName, intValue);
            }
            break;
        }
        case NativeRdb::ColumnType::TYPE_FLOAT: {
            double doubleValue = 0.0;
            if (resultSet->GetDouble(columnIndex, doubleValue) == NativeRdb::E_OK) {
                values.PutDouble(columnName, doubleValue);
            }
            break;
        }
        case NativeRdb::ColumnType::TYPE_STRING:
        case NativeRdb::ColumnType::TYPE_BLOB: {
            std::string strValue;
            if (resultSet->GetString(columnIndex, strValue) == NativeRdb::E_OK) {
                values.PutString(columnName, strValue);
            }
            break;
        }
        default:
            // 默认按字符串处理
            std::string strValue;
            if (resultSet->GetString(columnIndex, strValue) == NativeRdb::E_OK) {
                values.PutString(columnName, strValue);
            }
            break;
    }

    return E_OK;
}

// TableDataAdapter 实现

int32_t TableDataAdapter::ClearRedundantData(
    std::shared_ptr<NativeRdb::RdbStore> destRdb, std::shared_ptr<NativeRdb::RdbStore> sourceRdb)
{
    MEDIA_INFO_LOG("START STEP 10: Clear redundant data");
    failedTable_.clear();

    // 1. 初始化表处理配置
    InitTableProcessConfig();

    // 2. 获取需要处理的表列表（DROP_AND_INSERT、DROP_AND_CREATE和DROP_ONLY）
    std::vector<std::string> tablesToRecreate = GetTablesByType(TableProcessType::DROP_AND_INSERT);
    std::vector<std::string> tablesToCreate = GetTablesByType(TableProcessType::DROP_AND_CREATE);
    std::vector<std::string> tablesToDropOnly = GetTablesByType(TableProcessType::DROP_ONLY);

    MEDIA_INFO_LOG("Tables to drop and insert: %{public}zu", tablesToRecreate.size());
    MEDIA_INFO_LOG("Tables to drop and create: %{public}zu", tablesToCreate.size());
    MEDIA_INFO_LOG("Tables to drop only: %{public}zu", tablesToDropOnly.size());

    // 3. 处理DROP_AND_INSERT类型的表（drop + create + copy data）
    for (const auto &tableName : tablesToRecreate) {
        int32_t ret = ProcessTableByRecreate(tableName, destRdb, sourceRdb);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("ClearRedundantData: failed to process table %{public}s", tableName.c_str());
            failedTable_ = tableName;
            return E_ERR;
        }
    }

    // 4. 处理DROP_AND_CREATE类型的表（drop + create only）
    for (const auto &tableName : tablesToCreate) {
        int32_t ret = ProcessTableByCreateOnly(tableName, destRdb, sourceRdb);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("ClearRedundantData: failed to process table %{public}s", tableName.c_str());
            failedTable_ = tableName;
            return E_ERR;
        }
    }

    // 5. 处理DROP_ONLY类型的表（仅删除，不重建）
    for (const auto &tableName : tablesToDropOnly) {
        int32_t ret = ProcessTableByDropOnly(tableName, destRdb);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("ClearRedundantData: failed to drop table %{public}s", tableName.c_str());
            failedTable_ = tableName;
            return E_ERR;
        }
    }

    MEDIA_INFO_LOG("END STEP 10: Clear redundant data");
    return E_OK;
}

int32_t TableDataAdapter::CleanInvalidPhotosFromOldDb(
    std::shared_ptr<NativeRdb::RdbStore> oldDbTempStore)
{
    MEDIA_INFO_LOG("CleanInvalidPhotosFromOldDb: start cleaning invalid photos");

    if (oldDbTempStore == nullptr) {
        MEDIA_ERR_LOG("CleanInvalidPhotosFromOldDb: oldDbTempStore is null");
        return E_ERR;
    }

    std::vector<InvalidPhotoInfo> invalidPhotos = QueryInvalidPhotos(oldDbTempStore);

    auto [errCode, transaction] = oldDbTempStore->CreateTransaction(OHOS::NativeRdb::Transaction::DEFERRED);
    if (errCode != NativeRdb::E_OK || transaction == nullptr) {
        MEDIA_ERR_LOG("CleanInvalidPhotosFromOldDb: failed to create transaction, err=%{public}d", errCode);
        return E_ERR;
    }

    int32_t deleteFileSuccessCount = 0;
    int32_t deleteFileFailCount = 0;
    ProcessInvalidPhotosWithTransaction(invalidPhotos, transaction, deleteFileSuccessCount, deleteFileFailCount);

    MEDIA_INFO_LOG("CleanInvalidPhotosFromOldDb: deleted files for %{public}d photos, "
        "failed for %{public}d photos", deleteFileSuccessCount, deleteFileFailCount);

    int32_t ret = transaction->Commit();
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("CleanInvalidPhotosFromOldDb: failed to commit transaction, ret=%{public}d", ret);
        transaction->Rollback();
        return E_ERR;
    }

    MEDIA_INFO_LOG("CleanInvalidPhotosFromOldDb: successfully deleted invalid photos");
    return E_OK;
}

void TableDataAdapter::ProcessInvalidPhotosWithTransaction(
    const std::vector<InvalidPhotoInfo> &invalidPhotos,
    std::shared_ptr<NativeRdb::Transaction> &transaction,
    int32_t &deleteFileSuccessCount, int32_t &deleteFileFailCount)
{
    for (const auto &photo : invalidPhotos) {
        bool allFilesDeleted = DeletePhotoFiles(photo);
        if (allFilesDeleted) {
            bool dbDeleted = DeletePhotoRecord(transaction, photo.fileId);
            if (dbDeleted) {
                deleteFileSuccessCount++;
            } else {
                deleteFileFailCount++;
            }
        } else {
            deleteFileFailCount++;
            LogDeleteFileFailure(photo);
        }
    }
}

bool TableDataAdapter::DeletePhotoFiles(const InvalidPhotoInfo &photo)
{
    bool deleteFileRet = MediaFileUtils::DeleteFileOrFolder(photo.cloudPath, true);
    std::string thumbsFolder = BackupFileUtils::GetReplacedPathByPrefixType(
        PrefixType::CLOUD, PrefixType::CLOUD_THUMB, photo.cloudPath);
    bool deleteThumbsRet = MediaFileUtils::DeleteFileOrFolder(thumbsFolder, false);
    std::string editDataFolder = BackupFileUtils::GetReplacedPathByPrefixType(
        PrefixType::CLOUD, PrefixType::CLOUD_EDIT_DATA, photo.cloudPath);
    bool deleteEditDataRet = MediaFileUtils::DeleteFileOrFolder(editDataFolder, false);
    bool deleteVideoRet = DeleteMovingPhotoVideo(photo);

    return deleteFileRet && deleteThumbsRet && deleteEditDataRet && deleteVideoRet;
}

bool TableDataAdapter::DeleteMovingPhotoVideo(const InvalidPhotoInfo &photo)
{
    bool isMovingPhoto = (photo.subtype == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO) ||
        photo.movingPhotoEffectMode == static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY));
    if (!isMovingPhoto) {
        return true;
    }

    std::string videoPath = MediaFileUtils::GetMovingPhotoVideoPath(photo.cloudPath);
    if (videoPath.empty()) {
        return true;
    }

    return MediaFileUtils::DeleteFileOrFolder(videoPath, false);
}

bool TableDataAdapter::DeletePhotoRecord(std::shared_ptr<NativeRdb::Transaction> &transaction, int64_t fileId)
{
    std::string deleteSql = "DELETE FROM Photos WHERE file_id = ?";
    std::vector<NativeRdb::ValueObject> bindArgs = {fileId};
    auto res = transaction->Execute(deleteSql, bindArgs);

    if (res.first != NativeRdb::E_OK) {
        MEDIA_WARN_LOG("CleanInvalidPhotosFromOldDb: failed to delete db record for fileId=%{public}ld, ret=%{public}d",
            fileId, res.first);
        return false;
    }
    return true;
}

void TableDataAdapter::LogDeleteFileFailure(const InvalidPhotoInfo &photo)
{
    bool deleteFileRet = MediaFileUtils::IsFileExists(photo.cloudPath);
    std::string thumbsFolder = BackupFileUtils::GetReplacedPathByPrefixType(
        PrefixType::CLOUD, PrefixType::CLOUD_THUMB, photo.cloudPath);
    bool deleteThumbsRet = MediaFileUtils::IsFileExists(thumbsFolder);
    std::string editDataFolder = BackupFileUtils::GetReplacedPathByPrefixType(
        PrefixType::CLOUD, PrefixType::CLOUD_EDIT_DATA, photo.cloudPath);
    bool deleteEditDataRet = MediaFileUtils::IsFileExists(editDataFolder);

    bool isMovingPhoto = (photo.subtype == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO) ||
        photo.movingPhotoEffectMode == static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY));
    bool deleteVideoRet = true;
    if (isMovingPhoto) {
        std::string videoPath = MediaFileUtils::GetMovingPhotoVideoPath(photo.cloudPath);
        if (!videoPath.empty()) {
            deleteVideoRet = MediaFileUtils::IsFileExists(videoPath);
        }
    }

    MEDIA_WARN_LOG("CleanInvalidPhotosFromOldDb: failed to delete files for fileId=%{public}ld, "
        "fileExists=%{public}d, thumbsExists=%{public}d, editDataExists=%{public}d, videoExists=%{public}d",
        photo.fileId, deleteFileRet, deleteThumbsRet, deleteEditDataRet, deleteVideoRet);
}

std::vector<InvalidPhotoInfo> TableDataAdapter::QueryInvalidPhotos(
    std::shared_ptr<NativeRdb::RdbStore> oldDbTempStore)
{
    std::string querySql = "SELECT file_id, data, subtype, moving_photo_effect_mode FROM Photos WHERE "
        "sync_status <> 0 OR clean_flag <> 0 OR time_pending < 0 OR is_temp <> 0 OR file_source_type = 1";

    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(oldDbTempStore, querySql);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("QueryInvalidPhotos: failed to query invalid photos");
        return {};
    }

    std::vector<InvalidPhotoInfo> invalidPhotos;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int64_t fileId = 0;
        std::string cloudPath;
        int32_t subtype = 0;
        int32_t movingPhotoEffectMode = 0;
        int32_t fileIdIndex = 0;
        int32_t dataColumnIndex = 0;
        int32_t subtypeIndex = 0;
        int32_t effectModeIndex = 0;
        if (resultSet->GetColumnIndex("file_id", fileIdIndex) == NativeRdb::E_OK &&
            resultSet->GetColumnIndex("data", dataColumnIndex) == NativeRdb::E_OK &&
            resultSet->GetColumnIndex("subtype", subtypeIndex) == NativeRdb::E_OK &&
            resultSet->GetColumnIndex("moving_photo_effect_mode", effectModeIndex) == NativeRdb::E_OK) {
            resultSet->GetLong(fileIdIndex, fileId);
            resultSet->GetString(dataColumnIndex, cloudPath);
            resultSet->GetInt(subtypeIndex, subtype);
            resultSet->GetInt(effectModeIndex, movingPhotoEffectMode);
            invalidPhotos.push_back({fileId, cloudPath, subtype, movingPhotoEffectMode});
        }
    }
    resultSet->Close();

    MEDIA_INFO_LOG("QueryInvalidPhotos: found %{public}zu invalid photos", invalidPhotos.size());
    return invalidPhotos;
}

int32_t TableDataAdapter::ProcessTableByRecreate(const std::string &tableName,
    std::shared_ptr<NativeRdb::RdbStore> destRdb, std::shared_ptr<NativeRdb::RdbStore> sourceRdb)
{
    MEDIA_INFO_LOG("ProcessTableByRecreate: processing table (DROP_AND_INSERT): %{public}s", tableName.c_str());

    // 添加数据库连接有效性检查
    if (destRdb == nullptr || sourceRdb == nullptr) {
        MEDIA_ERR_LOG("ProcessTableByRecreate: destRdb or sourceRdb is null for table %{public}s", tableName.c_str());
        return E_ERR;
    }

    // 添加线程同步锁，防止多线程并发操作同一张表
    static std::mutex tableProcessMutex;
    std::lock_guard<std::mutex> lock(tableProcessMutex);

    MEDIA_INFO_LOG("ProcessTableByRecreate: acquired lock for table %{public}s", tableName.c_str());

    // 再次检查数据库连接（防止在等待锁时连接被释放）
    if (destRdb == nullptr || sourceRdb == nullptr) {
        MEDIA_ERR_LOG("ProcessTableByRecreate: destRdb or sourceRdb became null after lock for table %{public}s",
            tableName.c_str());
        return E_ERR;
    }

    // 1. Drop表（从destRdb / srcdb）
    int32_t ret = schemaHandler_.DropTable(destRdb, tableName);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("ProcessTableByRecreate: failed to drop table %{public}s", tableName.c_str());
        return ret;
    }

    // 2. 从sourceRdb (dstdb)获取表结构并在destRdb (srcdb)中重新创建
    ret = schemaHandler_.RecreateTableFromDst(destRdb, sourceRdb, tableName);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("ProcessTableByRecreate: failed to recreate table %{public}s from dst", tableName.c_str());
        return ret;
    }

    // 3. 复制数据（sourceRdb / dstdb → destRdb / srcdb）
    ret = dataCopier_.CopyTableData(destRdb, sourceRdb, tableName);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("ProcessTableByRecreate: failed to copy data for table %{public}s", tableName.c_str());
        return ret;
    }

    MEDIA_INFO_LOG(
        "ProcessTableByRecreate: successfully processed table (DROP_AND_INSERT): %{public}s", tableName.c_str());
    return E_OK;
}

int32_t TableDataAdapter::ProcessTableByCreateOnly(const std::string &tableName,
    std::shared_ptr<NativeRdb::RdbStore> destRdb, std::shared_ptr<NativeRdb::RdbStore> sourceRdb)
{
    MEDIA_INFO_LOG("ProcessTableByCreateOnly: processing table (DROP_AND_CREATE): %{public}s", tableName.c_str());

    // 1. Drop表（从destRdb / srcdb）
    int32_t ret = schemaHandler_.DropTable(destRdb, tableName);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("ProcessTableByCreateOnly: failed to drop table %{public}s", tableName.c_str());
        return ret;
    }

    // 2. 从sourceRdb (dstdb)获取表结构并在destRdb (srcdb)中重新创建（不复制数据）
    ret = schemaHandler_.RecreateTableFromDst(destRdb, sourceRdb, tableName);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("ProcessTableByCreateOnly: failed to recreate table %{public}s from dst", tableName.c_str());
        return ret;
    }

    // 注意：不复制数据，只创建空表

    MEDIA_INFO_LOG(
        "ProcessTableByCreateOnly: successfully processed table (DROP_AND_CREATE): %{public}s", tableName.c_str());
    return E_OK;
}

int32_t TableDataAdapter::ProcessTableByDropOnly(const std::string &tableName,
    std::shared_ptr<NativeRdb::RdbStore> destRdb)
{
    MEDIA_INFO_LOG("ProcessTableByDropOnly: processing table (DROP_ONLY): %{public}s", tableName.c_str());

    if (destRdb == nullptr) {
        MEDIA_ERR_LOG("ProcessTableByDropOnly: destRdb is null for table %{public}s", tableName.c_str());
        return E_ERR;
    }

    // 添加线程同步锁，防止多线程并发操作同一张表
    static std::mutex tableProcessMutex;
    std::lock_guard<std::mutex> lock(tableProcessMutex);

    MEDIA_INFO_LOG("ProcessTableByDropOnly: acquired lock for table %{public}s", tableName.c_str());

    // 再次检查数据库连接（防止在等待锁时连接被释放）
    if (destRdb == nullptr) {
        MEDIA_ERR_LOG("ProcessTableByDropOnly: destRdb became null after lock for table %{public}s",
            tableName.c_str());
        return E_ERR;
    }

    // 仅删除表，不重建
    int32_t ret = schemaHandler_.DropTable(destRdb, tableName);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("ProcessTableByDropOnly: failed to drop table %{public}s", tableName.c_str());
        return ret;
    }

    MEDIA_INFO_LOG("ProcessTableByDropOnly: successfully dropped table (DROP_ONLY): %{public}s", tableName.c_str());
    return E_OK;
}

void TableDataAdapter::InitTableProcessConfig()
{
    tableProcessMap_ = TABLE_PROCESS_CONFIG;
    MEDIA_INFO_LOG("InitTableProcessConfig: loaded %{public}zu table configs", tableProcessMap_.size());
}

std::vector<std::string> TableDataAdapter::GetTablesByType(TableProcessType type)
{
    std::vector<std::string> tables;
    for (const auto &entry : tableProcessMap_) {
        if (entry.second == type) {
            tables.push_back(entry.first);
        }
    }
    return tables;
}

int32_t TableDataAdapter::ProcessSingleTableByRecreate(const std::string &tableName,
    std::shared_ptr<NativeRdb::RdbStore> destRdb, std::shared_ptr<NativeRdb::RdbStore> sourceRdb)
{
    MEDIA_INFO_LOG("ProcessSingleTableByRecreate: processing table: %{public}s", tableName.c_str());
    return ProcessTableByRecreate(tableName, destRdb, sourceRdb);
}

int32_t TableDataAdapter::ProcessSingleTableByRecreateIfExists(const std::string &tableName,
    std::shared_ptr<NativeRdb::RdbStore> destRdb, std::shared_ptr<NativeRdb::RdbStore> sourceRdb)
{
    // 检查源数据库中是否存在该表
    std::string checkSql = "SELECT name FROM sqlite_master WHERE type='table' AND name=?";
    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(sourceRdb, checkSql, {tableName});
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("ProcessSingleTableByRecreateIfExists: failed to check table %{public}s", tableName.c_str());
        return E_ERR;
    }

    bool tableExists = (resultSet->GoToFirstRow() == NativeRdb::E_OK);
    resultSet->Close();

    if (!tableExists) {
        MEDIA_INFO_LOG(
            "ProcessSingleTableByRecreateIfExists: table %{public}s does not exist in source, skip", tableName.c_str());
        return E_OK;
    }

    MEDIA_INFO_LOG("ProcessSingleTableByRecreateIfExists: table %{public}s exists, processing", tableName.c_str());
    return ProcessSingleTableByRecreate(tableName, destRdb, sourceRdb);
}
// LCOV_EXCL_STOP
}  // namespace Media
}  // namespace OHOS