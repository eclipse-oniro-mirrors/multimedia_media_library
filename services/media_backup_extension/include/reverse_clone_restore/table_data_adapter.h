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

#ifndef OHOS_MEDIA_TABLE_DATA_ADAPTER_H
#define OHOS_MEDIA_TABLE_DATA_ADAPTER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#include "rdb_store.h"
#include "values_bucket.h"
#include "media_log.h"
#include "medialibrary_errno.h"

namespace OHOS {
namespace Media {

// 表处理类型枚举
enum class TableProcessType {
    DROP_AND_INSERT,  // 直接drop再create，并继承新机的数据
    DROP_AND_CREATE,  // 仅drop再create，不继承新机的数据（创建空表）
    DROP_ONLY,        // 仅删除表，不重建
    SKIP,             // 跳过，暂时不用处理
};

// 表结构定义
struct TableSchema {
    std::string tableName;
    std::string sqlCreate;
    std::vector<std::string> columns;
    std::unordered_map<std::string, std::string> columnTypes;
    std::vector<std::string> indexSqls;      // 索引创建SQL
    std::vector<std::string> triggerSqls;     // 触发器创建SQL
};

// 无效照片信息
struct InvalidPhotoInfo {
    int64_t fileId;
    std::string cloudPath;
    int32_t subtype;
    int32_t movingPhotoEffectMode;
};

// 表结构管理类
class TableSchemaHandler {
public:
    TableSchemaHandler() = default;
    ~TableSchemaHandler() = default;

    // 获取表结构
    TableSchema GetTableSchema(std::shared_ptr<NativeRdb::RdbStore> rdbStore, const std::string &tableName);

    // Drop表
    int32_t DropTable(std::shared_ptr<NativeRdb::RdbStore> rdbStore, const std::string &tableName);

    // Create表
    int32_t CreateTable(std::shared_ptr<NativeRdb::RdbStore> rdbStore, const TableSchema &schema);

    // 从dstdb获取表结构并在srcdb中重新创建
    int32_t RecreateTableFromDst(std::shared_ptr<NativeRdb::RdbStore> srcDb, std::shared_ptr<NativeRdb::RdbStore> dstDb,
        const std::string &tableName);

private:
    // 获取建表SQL
    bool GetTableCreateSql(std::shared_ptr<NativeRdb::RdbStore> rdbStore, const std::string &tableName,
        TableSchema &schema);

    // 获取表列信息
    void GetTableColumns(std::shared_ptr<NativeRdb::RdbStore> rdbStore, const std::string &tableName,
        TableSchema &schema);

    // 获取表索引
    void GetTableIndexes(std::shared_ptr<NativeRdb::RdbStore> rdbStore, const std::string &tableName,
        TableSchema &schema);

    // 获取表触发器
    void GetTableTriggers(std::shared_ptr<NativeRdb::RdbStore> rdbStore, const std::string &tableName,
        TableSchema &schema);

    // 从ResultSet解析表结构
    TableSchema ParseSchemaFromResultSet(std::shared_ptr<NativeRdb::ResultSet> resultSet, const std::string &tableName);
};

// 数据复制类
class TableDataCopier {
public:
    TableDataCopier() = default;
    ~TableDataCopier() = default;

    // 复制表数据（sourceDb → destDb）
    int32_t CopyTableData(std::shared_ptr<NativeRdb::RdbStore> destDb, std::shared_ptr<NativeRdb::RdbStore> sourceDb,
        const std::string &tableName);

private:
    // 获取表的所有列名
    std::vector<std::string> GetColumnNames(
        std::shared_ptr<NativeRdb::RdbStore> rdbStore, const std::string &tableName);

    // 批量插入数据
    int32_t BatchInsert(std::shared_ptr<NativeRdb::RdbStore> rdbStore, const std::string &tableName,
        const std::vector<NativeRdb::ValuesBucket> &values);

    // 根据列类型从ResultSet获取值
    int32_t GetValueFromResultSet(NativeRdb::ResultSet *resultSet, const std::string &columnName,
        const std::string &columnTypeStr, NativeRdb::ValuesBucket &values);
};

// 表数据适配器主控制器
class TableDataAdapter {
public:
    TableDataAdapter() = default;
    ~TableDataAdapter() = default;

    // 步骤10：清除冗余数据
    int32_t ClearRedundantData(
        std::shared_ptr<NativeRdb::RdbStore> destRdb, std::shared_ptr<NativeRdb::RdbStore> sourceRdb);

    // 清理旧db临时副本中Photos表的无效记录及对应文件
    int32_t CleanInvalidPhotosFromOldDb(std::shared_ptr<NativeRdb::RdbStore> oldDbTempStore);

    // 处理单张表：drop再create并复制数据（DROP_AND_INSERT）
    int32_t ProcessTableByRecreate(const std::string &tableName, std::shared_ptr<NativeRdb::RdbStore> destRdb,
        std::shared_ptr<NativeRdb::RdbStore> sourceRdb);

    // 处理单张表：drop再create但不复制数据（DROP_AND_CREATE）
    int32_t ProcessTableByCreateOnly(const std::string &tableName, std::shared_ptr<NativeRdb::RdbStore> destRdb,
        std::shared_ptr<NativeRdb::RdbStore> sourceRdb);

    // 处理单张表：仅删除表不重建（DROP_ONLY）
    int32_t ProcessTableByDropOnly(const std::string &tableName, std::shared_ptr<NativeRdb::RdbStore> destRdb);

    // 处理单个特殊表：drop再create并复制数据
    int32_t ProcessSingleTableByRecreate(const std::string &tableName, std::shared_ptr<NativeRdb::RdbStore> destRdb,
        std::shared_ptr<NativeRdb::RdbStore> sourceRdb);

    // 条件处理单个表：仅当表在源数据库存在时才处理
    int32_t ProcessSingleTableByRecreateIfExists(const std::string &tableName,
        std::shared_ptr<NativeRdb::RdbStore> destRdb, std::shared_ptr<NativeRdb::RdbStore> sourceRdb);

    // 获取失败的表名
    std::string GetFailedTable() const { return failedTable_; }

private:
    // 初始化表处理配置
    void InitTableProcessConfig();

    // 根据类型获取表列表
    std::vector<std::string> GetTablesByType(TableProcessType type);

    // 查询无效照片记录
    std::vector<InvalidPhotoInfo> QueryInvalidPhotos(
        std::shared_ptr<NativeRdb::RdbStore> oldDbTempStore);

    // 在事务中处理无效照片
    void ProcessInvalidPhotosWithTransaction(const std::vector<InvalidPhotoInfo> &invalidPhotos,
        std::shared_ptr<NativeRdb::Transaction> &transaction,
        int32_t &deleteFileSuccessCount, int32_t &deleteFileFailCount);

    // 删除单张照片的所有文件
    bool DeletePhotoFiles(const InvalidPhotoInfo &photo);

    // 删除动图视频文件
    bool DeleteMovingPhotoVideo(const InvalidPhotoInfo &photo);

    // 删除单张照片的数据库记录
    bool DeletePhotoRecord(std::shared_ptr<NativeRdb::Transaction> &transaction, int64_t fileId);

    // 记录文件删除失败日志
    void LogDeleteFileFailure(const InvalidPhotoInfo &photo);

    // 成员变量
    std::unordered_map<std::string, TableProcessType> tableProcessMap_;
    TableSchemaHandler schemaHandler_;
    TableDataCopier dataCopier_;
    std::string failedTable_;  // 记录ClearRedundantData失败的表名
};

}  // namespace Media
}  // namespace OHOS

#endif  // OHOS_MEDIA_TABLE_DATA_ADAPTER_H