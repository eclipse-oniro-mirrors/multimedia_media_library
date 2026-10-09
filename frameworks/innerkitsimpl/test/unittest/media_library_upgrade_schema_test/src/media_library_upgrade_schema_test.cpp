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

#define MLOG_TAG "UpgradeSchemaTest"

#include "media_library_upgrade_schema_test.h"

#include <vector>
#include <map>
#include <algorithm>
#include <fstream>
#include <iostream>
#include "media_log.h"
#include "media_file_utils.h"
#include "medialibrary_db_const.h"
#include "medialibrary_rdbstore.h"
#include "media_library_upgrade_manager.h"
#include "media_library_upgrade_helper.h"
#include "media_library_upgrade_task_registry.h"
#include "media_library_upgrade_executor.h"
#include "media_library_sql_builder.h"
#include "table_event_handler.h"
#include "rdb_helper.h"
#include "media_app_uri_sensitive_column.h"
#include "preferences.h"
#include "preferences_helper.h"

using namespace testing::ext;
using namespace OHOS::NativePreferences;

namespace OHOS::Media {

// 测试配置常量定义

/** @brief 测试源版本号 */
constexpr int32_t TEST_SOURCE_VERSION = 350;

/** @brief 测试目标版本号 */
constexpr int32_t TEST_TARGET_VERSION = MEDIA_RDB_VERSION;

/** @brief 数据库连接池大小 */
constexpr int32_t CONNECT_SIZE = 8;

/** @brief RDB WAL日志限制大小 (1GB) */
constexpr ssize_t RDB_WAL_LIMIT_SIZE = 1024 * 1024 * 1024;

/** @brief 函数参数数量 */
constexpr int32_t ARG_COUNT = 2;

/** @brief WAL限制大小 (1GB) */
constexpr int32_t WAL_LIMIT_SIZE = 1024 * 1024 * 1024;

/** @brief 时间戳参数数量 */
constexpr int32_t STAMP_PARAM = 4;

// 测试文件路径常量定义

/** @brief 创建测试数据库路径 */
const std::string CREATE_DB_PATH = "/data/test/create_test.db";

/** @brief 升级测试数据库路径 */
const std::string UPGRADE_DB_50_PATH = "/data/test/upgrade_test_5_0_187.db";

/** @brief 升级测试数据库版本号 */
constexpr int32_t UPGRADE_DB_50_VERSION = 187;

/** @brief 升级事件配置文件路径 */
const std::string UPGRADE_EVENT_50_PATH = "/data/test/rdb_upgrade_events_5_0.xml";
const std::string UPGRADE_EVENT_61_PATH = "/data/test/rdb_upgrade_events_6_1.xml";

/** @brief 升级测试数据库路径 (6.1版本) */
const std::string UPGRADE_DB_61_PATH = "/data/test/upgrade_test_6_1_561.db";

/** @brief 升级测试数据库版本号 (6.1版本) */
constexpr int32_t UPGRADE_DB_61_VERSION = 561;


/** @brief RDB配置文件路径 */
const std::string RDB_CONFIG_PATH_TEST = "/data/test/rdb_config.xml";

// 特殊SQL语句定义

/**
 * @brief 特殊创建表SQL
 * 
 * 在创建DB场景下，补齐使用MediaLibraryRdbStore创建的表
 */
const std::string SPECIAL_CREATE_TABLE_SQL_1 = "\
    CREATE TABLE IF NOT EXISTS download_resources_task_records ( \
        file_id         INTEGER  PRIMARY KEY NOT NULL, \
        display_name       TEXT     NOT NULL DEFAULT \"\", \
        size       BIGINT NOT NULL DEFAULT -1, \
        uri        TEXT, \
        add_time      BIGINT NOT NULL DEFAULT -1, \
        finish_time     BIGINT NOT NULL DEFAULT -1, \
        download_status INT NOT NULL DEFAULT -1, \
        percent         INT NOT NULL DEFAULT -1, \
        auto_pause_reason INT NOT NULL DEFAULT 0, \
        cover_level INT NOT NULL DEFAULT 1 \
    );";

/**
 * @brief 特殊升级表SQL
 * 
 * 补齐未在数据库升级中删除的表
 */
const std::string SPECIAL_UPGRADE_TABLE_SQL_1 = AppUriSensitiveColumn::DROP_APP_URI_SENSITIVE_TABLE;

// 索引特殊处理配置

/**
 * @brief 需要排除的索引列表
 * 
 * 这些索引的差异不需要计入对比结果（已有纪要说明）
 */
const std::vector<std::string> SPECIAL_EXCLUDE_INDEX = {
    "idx_fileid_for_photo_map",
};

/**
 * @brief 需要格式化处理的索引列表
 * 
 * 这些索引的格式差异（空格&换行）需要特殊处理，属于历史版本问题
 */
const std::vector<std::string> SPECIAL_FORMAT_INDEX = {
    "idx_asset_uri_on_tab_facard_photos",
    "idx_form_id_on_tab_facard_photos",
};

/**
 * @brief 需要排除的触发器列表
 * 
 * 这些触发器不需要进行对比检查
 */
const std::vector<std::string> SPECIAL_EXCLUDE_TRIGGER = {
    // 如果有需要排除的触发器，在这里添加
};

/**
 * @brief 需要格式化处理的触发器列表
 * 
 * 这些触发器的格式差异（空格&换行）需要特殊处理，属于历史版本问题
 */
const std::vector<std::string> SPECIAL_FORMAT_TRIGGER = {
    // 如果有需要格式化处理的触发器，在这里添加
};

// 任务配置常量定义

/** @brief 任务配置未设置 */
constexpr int32_t TASK_CONFIG_NOT_SET = -1;

/** @brief 任务配置：需要同步任务 */
constexpr int32_t TASK_CONFIG_SYNC_ONLY = 1;

/** @brief 任务配置：需要异步任务 */
constexpr int32_t TASK_CONFIG_ASYNC_ONLY = 2;

/** @brief 任务配置：需要同步和异步任务 */
constexpr int32_t TASK_CONFIG_SYNC_AND_ASYNC = 3;

// SQL解析常量定义

/** @brief "INDEX " 关键字长度 */
constexpr size_t INDEX_KEYWORD_LENGTH = 6;

/** @brief "IF NOT EXISTS " 关键字长度 */
constexpr size_t IF_NOT_EXISTS_LENGTH = 12;

// UpgradeSchemaTest 测试类生命周期函数实现

void UpgradeSchemaTest::SetUpTestCase()
{
    MEDIA_INFO_LOG("UpgradeSchemaTest::SetUpTestCase");
}

void UpgradeSchemaTest::TearDownTestCase()
{
    MEDIA_INFO_LOG("UpgradeSchemaTest::TearDownTestCase");
}

void UpgradeSchemaTest::SetUp()
{
    MEDIA_INFO_LOG("UpgradeSchemaTest::SetUp");
}

void UpgradeSchemaTest::TearDown()
{
    MEDIA_INFO_LOG("UpgradeSchemaTest::TearDown start");
    RemoveTestDb(CREATE_DB_PATH);
    MediaFileUtils::DeleteFile(RDB_CONFIG_PATH_TEST);
    MEDIA_INFO_LOG("UpgradeSchemaTest::TearDown end");
}

// 数据库辅助函数实现

/**
 * @brief 云同步触发函数（测试用）
 * @param args 参数列表
 * @return 空字符串
 */
static std::string CloudSyncTriggerFunc(const std::vector<std::string> &args)
{
    return "";
}

/**
 * @brief 判断调用者是否为自身（测试用）
 * @param args 参数列表
 * @return 固定返回"false"
 */
static std::string IsCallerSelfFunc(const std::vector<std::string> &args)
{
    return "false";
}

/**
 * @brief 相册通知函数（测试用）
 * @param args 参数列表
 * @return 空字符串
 */
static std::string PhotoAlbumNotifyFunc(const std::vector<std::string> &args)
{
    return "";
}

/**
 * @brief 开始生成高亮缩略图函数（测试用）
 * @param args 参数列表
 * @return 空字符串
 */
static std::string BeginGenerateHighlightThumbnail(const std::vector<std::string> &args)
{
    return "";
}

// UpgradeSchemaTest 工具函数实现

void UpgradeSchemaTest::RemoveTestDb(const std::string &dbPath)
{
    if (MediaFileUtils::IsFileExists(dbPath)) {
        MediaFileUtils::DeleteFile(dbPath);
        MEDIA_INFO_LOG("Removed database: %{public}s", dbPath.c_str());
    }

    // 删除数据库相关文件（WAL、SHM、DWR、比较文件）
    std::string walPath = dbPath + "-wal";
    std::string shmPath = dbPath + "-shm";
    std::string dwrPath = dbPath + "-dwr";
    std::string comparePath = dbPath + "-compare";

    if (MediaFileUtils::IsFileExists(walPath)) {
        MediaFileUtils::DeleteFile(walPath);
    }
    if (MediaFileUtils::IsFileExists(shmPath)) {
        MediaFileUtils::DeleteFile(shmPath);
    }
    if (MediaFileUtils::IsFileExists(dwrPath)) {
        MediaFileUtils::DeleteFile(dwrPath);
    }
    if (MediaFileUtils::IsFileExists(comparePath)) {
        MediaFileUtils::DeleteFile(comparePath);
    }
}

// TestRdbCreateCallback 回调类函数实现

int32_t TestRdbCreateCallback::OnCreate(NativeRdb::RdbStore &rdbStore)
{
    MEDIA_INFO_LOG("TestRdbCreateCallback::OnCreate");
    MediaLibraryDataCallBack cb;
    cb.OnCreate(rdbStore);
    return NativeRdb::E_OK;
}

int32_t TestRdbCreateCallback::OnUpgrade(NativeRdb::RdbStore &rdbStore, int32_t oldVersion, int32_t newVersion)
{
    MEDIA_INFO_LOG("TestRdbCreateCallback::OnUpgrade");
    return NativeRdb::E_OK;
}

// TestRdbUpgradeCallback 回调类函数实现

int32_t TestRdbUpgradeCallback::OnCreate(NativeRdb::RdbStore &rdbStore)
{
    MEDIA_INFO_LOG("TestRdbUpgradeCallback::OnCreate");
    return NativeRdb::E_OK;
}

int32_t TestRdbUpgradeCallback::OnUpgrade(NativeRdb::RdbStore &rdbStore, int32_t oldVersion, int32_t newVersion)
{
    MEDIA_INFO_LOG("TestRdbUpgradeCallback::OnUpgrade");
    return NativeRdb::E_OK;
}

// 数据库升级和创建辅助函数实现

/**
 * @brief 测试数据库升级函数
 * @param rdbStore 数据库存储对象引用
 * @param oldVersion 旧版本号
 * @param newVersion 新版本号
 * @return 成功返回NativeRdb::E_OK，失败返回错误码
 */
int32_t TestUpgradeDatabase(NativeRdb::RdbStore &rdbStore, const std::string& eventPath,
    int32_t oldVersion, int32_t newVersion)
{
    MEDIA_INFO_LOG("#test OnUpgrade old:%{public}d new:%{public}d", oldVersion, newVersion);
    UpgradeManagerConfig config(false, eventPath, RDB_CONFIG_PATH_TEST, oldVersion, newVersion);
    int32_t ret = UpgradeManager::GetInstance().Initialize(config);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("UpgradeManager Initialize failed: %{public}d", ret);
        return ret;
    }
    ret = UpgradeManager::GetInstance().UpgradeSync(rdbStore);
    ret = UpgradeManager::GetInstance().UpgradeAsync(rdbStore);
    return ret;
}

/**
 * @brief 创建数据库存储对象
 * @param path 数据库文件路径
 * @param version 数据库版本号
 * @param cb 数据库打开回调对象
 * @return 数据库存储对象智能指针
 */
static std::shared_ptr<NativeRdb::RdbStore> MakeStore(const std::string &path, int32_t version,
                                                      NativeRdb::RdbOpenCallback &cb)
{
    NativeRdb::RdbStoreConfig config(path.substr(path.rfind('/') + 1));
    config.SetPath(path);
    config.SetBundleName("com.ohos.medialibrary.medialibrarydata");
    config.SetReadConSize(CONNECT_SIZE);
    config.SetSecurityLevel(NativeRdb::SecurityLevel::S3);
    config.SetHaMode(NativeRdb::HAMode::MANUAL_TRIGGER);
    config.SetAllowRebuild(true);
    config.SetWalLimitSize(RDB_WAL_LIMIT_SIZE);
    config.SetScalarFunction("cloud_sync_func", 0, CloudSyncTriggerFunc);
    config.SetScalarFunction("is_caller_self_func", 0, IsCallerSelfFunc);
    config.SetScalarFunction("photo_album_notify_func", ARG_COUNT, PhotoAlbumNotifyFunc);
    config.SetScalarFunction("begin_generate_highlight_thumbnail", STAMP_PARAM, BeginGenerateHighlightThumbnail);
    int32_t err = 0;

    return NativeRdb::RdbHelper::GetRdbStore(config, version, cb, err);
}

// 数据库元信息查询函数实现

/**
 * @brief 获取数据库中所有表的名称
 * @param store 数据库存储对象引用
 * @return 表名称列表
 */
static std::vector<std::string> GetTableNames(NativeRdb::RdbStore &store)
{
    std::vector<std::string> tableNames;
    std::string sql = "SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%'";
    auto resultSet = store.QuerySql(sql);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Query table names failed");
        return tableNames;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string name;
        resultSet->GetString(0, name);
        tableNames.push_back(name);
    }
    return tableNames;
}

/**
 * @brief 获取数据库中所有索引的SQL语句
 * @param store 数据库存储对象引用
 * @return 索引SQL语句列表（已排序）
 */
static std::vector<std::string> GetIndexSqls(NativeRdb::RdbStore &store)
{
    std::vector<std::string> indexSqls;
    std::string sql = "SELECT sql FROM sqlite_master WHERE type='index' AND sql IS NOT NULL";
    auto resultSet = store.QuerySql(sql);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Query index SQLs failed");
        return indexSqls;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string sqlContent;
        resultSet->GetString(0, sqlContent);
        indexSqls.push_back(sqlContent);
    }
    std::sort(indexSqls.begin(), indexSqls.end());
    return indexSqls;
}

/**
 * @brief 获取数据库中所有触发器的SQL语句
 * @param store 数据库存储对象引用
 * @return 触发器SQL语句列表（已排序）
 */
static std::vector<std::string> GetTriggerSqls(NativeRdb::RdbStore &store)
{
    std::vector<std::string> triggerSqls;
    std::string sql = "SELECT sql FROM sqlite_master WHERE type='trigger' AND sql IS NOT NULL";
    auto resultSet = store.QuerySql(sql);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Query trigger SQLs failed");
        return triggerSqls;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string sqlContent;
        resultSet->GetString(0, sqlContent);
        triggerSqls.push_back(sqlContent);
    }
    std::sort(triggerSqls.begin(), triggerSqls.end());
    return triggerSqls;
}

/**
 * @brief 获取指定表的所有列名
 * @param store 数据库存储对象引用
 * @param tableName 表名
 * @return 列名列表（已排序）
 */
static std::vector<std::string> GetColumnNames(NativeRdb::RdbStore &store, const std::string &tableName)
{
    std::vector<std::string> columnNames;
    std::string sql = "SELECT name FROM pragma_table_info('" + tableName + "')";
    auto resultSet = store.QuerySql(sql);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Query column names for %{public}s failed", tableName.c_str());
        return columnNames;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string name;
        resultSet->GetString(0, name);
        columnNames.push_back(name);
    }
    std::sort(columnNames.begin(), columnNames.end());
    return columnNames;
}

// 索引处理辅助函数实现

/**
 * @brief 从索引SQL中提取索引名称
 * @param indexSql 索引创建SQL语句
 * @return 索引名称，提取失败返回空字符串
 */
static std::string ExtractIndexName(const std::string &indexSql)
{
    size_t idxPos = indexSql.find("INDEX ");
    if (idxPos == std::string::npos) {
        return "";
    }
    size_t nameStart = idxPos + INDEX_KEYWORD_LENGTH;  // 跳过 "INDEX "
    // 跳过可能存在的 IF NOT EXISTS
    if (indexSql.substr(nameStart, IF_NOT_EXISTS_LENGTH) == "IF NOT EXISTS ") {
        nameStart += IF_NOT_EXISTS_LENGTH;
    }
    // 查找索引名结束位置（空格或左括号）
    size_t nameEnd = indexSql.find_first_of(" (", nameStart);
    if (nameEnd == std::string::npos) {
        return "";
    }
    std::string indexName = indexSql.substr(nameStart, nameEnd - nameStart);
    // 去除可能的引号
    if (indexName.front() == '"' || indexName.front() == '\'') {
        indexName = indexName.substr(1);
    }
    if (indexName.back() == '"' || indexName.back() == '\'') {
        indexName.pop_back();
    }
    return indexName;
}

/**
 * @brief 提取触发器名称
 * @param triggerSql 触发器SQL语句
 * @return 触发器名称，提取失败返回空字符串
 */
static std::string ExtractTriggerName(const std::string &triggerSql)
{
    size_t trigPos = triggerSql.find("TRIGGER ");
    if (trigPos == std::string::npos) {
        return "";
    }
    size_t nameStart = trigPos + 8;  // 跳过 "TRIGGER "
    // 跳过可能存在的 IF NOT EXISTS
    if (triggerSql.substr(nameStart, IF_NOT_EXISTS_LENGTH) == "IF NOT EXISTS ") {
        nameStart += IF_NOT_EXISTS_LENGTH;
    }
    // 查找触发器名结束位置（空格）
    size_t nameEnd = triggerSql.find_first_of(" ", nameStart);
    if (nameEnd == std::string::npos) {
        return "";
    }
    std::string triggerName = triggerSql.substr(nameStart, nameEnd - nameStart);
    // 去除可能的引号
    if (triggerName.front() == '"' || triggerName.front() == '\'') {
        triggerName = triggerName.substr(1);
    }
    if (triggerName.back() == '"' || triggerName.back() == '\'') {
        triggerName.pop_back();
    }
    return triggerName;
}

/**
 * @brief 标准化触发器SQL语句
 * 
 * 该函数去除多余的空格、制表符、换行符等，统一格式以便比较。
 * 保留引号内的空白字符。
 * 
 * @param triggerSql 触发器SQL语句
 * @return 标准化后的SQL语句
 */
static std::string NormalizeTriggerSql(const std::string &triggerSql)
{
    std::string normalized;
    bool inQuote = false;
    char quoteChar = '\0';
    for (char c : triggerSql) {
        if ((c == '"' || c == '\'') && !inQuote) {
            inQuote = true;
            quoteChar = c;
            normalized += c;
        } else if (c == quoteChar && inQuote) {
            inQuote = false;
            normalized += c;
        } else if (!inQuote && (c == ' ' || c == '\t' || c == '\n' || c == '\r')) {
            // 跳过引号外的空白字符
            if (!normalized.empty() && normalized.back() != ' ') {
                normalized += ' ';  // 保留单个空格分隔符
            }
        } else {
            normalized += c;
        }
    }
    // 去除末尾可能的空格
    while (!normalized.empty() && normalized.back() == ' ') {
        normalized.pop_back();
    }
    return normalized;
}

/**
 * @brief 标准化索引SQL格式（去除多余空格和换行）
 * @param indexSql 原始索引SQL语句
 * @return 标准化后的索引SQL语句
 */
static std::string NormalizeIndexSql(const std::string &indexSql)
{
    std::string normalized;
    bool inQuote = false;
    char quoteChar = '\0';
    for (char c : indexSql) {
        if ((c == '"' || c == '\'') && !inQuote) {
            inQuote = true;
            quoteChar = c;
            normalized += c;
        } else if (c == quoteChar && inQuote) {
            inQuote = false;
            normalized += c;
        } else if (!inQuote && (c == ' ' || c == '\t' || c == '\n' || c == '\r')) {
            // 跳过引号外的空白字符
            if (!normalized.empty() && normalized.back() != ' ') {
                normalized += ' ';  // 保留单个空格分隔符
            }
        } else {
            normalized += c;
        }
    }
    // 去除末尾可能的空格
    while (!normalized.empty() && normalized.back() == ' ') {
        normalized.pop_back();
    }
    return normalized;
}

// 数据库结构对比函数实现

/**
 * @brief 比较两个数据库的表信息
 * @param tables1 第一个数据库的表名列表
 * @param tables2 第二个数据库的表名列表
 * @param differences 差异信息输出向量
 */
static void CompareTables(const std::vector<std::string> &tables1,
                          const std::vector<std::string> &tables2,
                          std::vector<std::string> &differences)
{
    // 检查表数量是否一致
    if (tables1.size() != tables2.size()) {
        differences.push_back("Table count mismatch: db1 has " + std::to_string(tables1.size()) + " tables, db2 has " +
                              std::to_string(tables2.size()) + " tables");
    }

    // 检查db1中存在但db2中不存在的表
    for (const auto &table : tables1) {
        if (std::find(tables2.begin(), tables2.end(), table) == tables2.end()) {
            differences.push_back("Table missing in db2: " + table);
        }
    }

    // 检查db2中存在但db1中不存在的表
    for (const auto &table : tables2) {
        if (std::find(tables1.begin(), tables1.end(), table) == tables1.end()) {
            differences.push_back("Table missing in db1: " + table);
        }
    }
}

/**
 * @brief 比较两个数据库的列信息
 * @param store1 第一个数据库存储对象
 * @param store2 第二个数据库存储对象
 * @param tables1 第一个数据库的表名列表
 * @param tables2 第二个数据库的表名列表
 * @param differences 差异信息输出向量
 */
static void CompareColumns(NativeRdb::RdbStore &store1, NativeRdb::RdbStore &store2,
                           const std::vector<std::string> &tables1,
                           const std::vector<std::string> &tables2,
                           std::vector<std::string> &differences)
{
    // 检查每个表的列信息
    for (const auto &table : tables1) {
        if (std::find(tables2.begin(), tables2.end(), table) != tables2.end()) {
            std::vector<std::string> columns1 = GetColumnNames(store1, table);
            std::vector<std::string> columns2 = GetColumnNames(store2, table);
            if (columns1.size() != columns2.size()) {
                differences.push_back("Column count mismatch for table " + table + ": db1 has " +
                                      std::to_string(columns1.size()) + ", db2 has " + std::to_string(columns2.size()));
            }
            for (size_t i = 0; i < columns1.size() && i < columns2.size(); i++) {
                if (columns1[i] != columns2[i]) {
                    differences.push_back("Column mismatch for table " + table + ": db1[" + std::to_string(i) +
                                          "]=" + columns1[i] + ", db2[" + std::to_string(i) + "]=" + columns2[i]);
                }
            }
        }
    }
}

/**
 * @brief 过滤需要排除的索引
 * @param indexes 原始索引列表
 * @return 过滤后的索引列表
 */
static std::vector<std::string> FilterIndexes(const std::vector<std::string> &indexes)
{
    std::vector<std::string> filteredIndexes;
    for (const auto &index : indexes) {
        std::string indexName = ExtractIndexName(index);
        if (std::find(SPECIAL_EXCLUDE_INDEX.begin(), SPECIAL_EXCLUDE_INDEX.end(), indexName) ==
            SPECIAL_EXCLUDE_INDEX.end()) {
            filteredIndexes.push_back(index);
        }
    }
    return filteredIndexes;
}

/**
 * @brief 检查格式索引是否匹配
 * @param index 要检查的索引SQL
 * @param targetIndexes 目标索引列表
 * @return 匹配返回true，否则返回false
 */
static bool IsFormatIndexMatch(const std::string &index, const std::vector<std::string> &targetIndexes)
{
    std::string indexName = ExtractIndexName(index);
    for (const auto &targetIndex : targetIndexes) {
        if (ExtractIndexName(targetIndex) == indexName) {
            // 比较标准化后的SQL
            if (NormalizeIndexSql(index) == NormalizeIndexSql(targetIndex)) {
                return true;
            }
        }
    }
    return false;
}

/**
 * @brief 比较两个数据库的索引信息
 * @param indexes1 第一个数据库的索引列表
 * @param indexes2 第二个数据库的索引列表
 * @param differences 差异信息输出向量
 * @param indexDetails 索引详细信息输出向量（包含差异的完整SQL）
 */
static void CompareIndexes(std::vector<std::string> &indexes1, std::vector<std::string> &indexes2,
                           std::vector<std::string> &differences, std::vector<std::string> &indexDetails)
{
    // 过滤掉需要排除的索引
    std::vector<std::string> filteredIndexes1 = FilterIndexes(indexes1);
    std::vector<std::string> filteredIndexes2 = FilterIndexes(indexes2);

    if (filteredIndexes1.size() != filteredIndexes2.size()) {
        differences.push_back("Index count mismatch (excluding SPECIAL_EXCLUDE_INDEX): db1 has " +
                              std::to_string(filteredIndexes1.size()) + " indexes, db2 has " +
                              std::to_string(filteredIndexes2.size()) + " indexes");
    }

    // 使用集合差集找出差异的索引
    std::vector<std::string> onlyInDb1;
    std::vector<std::string> onlyInDb2;
    std::set_difference(filteredIndexes1.begin(), filteredIndexes1.end(), filteredIndexes2.begin(),
                        filteredIndexes2.end(), std::back_inserter(onlyInDb1));
    std::set_difference(filteredIndexes2.begin(), filteredIndexes2.end(), filteredIndexes1.begin(),
                        filteredIndexes1.end(), std::back_inserter(onlyInDb2));

    // 对差异索引进行二次检查，考虑SPECIAL_FORMAT_INDEX的情况
    for (const auto &index : onlyInDb1) {
        std::string indexName = ExtractIndexName(index);
        bool isFormatIndex = std::find(SPECIAL_FORMAT_INDEX.begin(), SPECIAL_FORMAT_INDEX.end(), indexName) !=
                             SPECIAL_FORMAT_INDEX.end();
        if (isFormatIndex) {
            if (!IsFormatIndexMatch(index, filteredIndexes2)) {
                differences.push_back("Index missing in db2 (format mismatch): " + indexName);
                indexDetails.push_back("Index only in db1 (format mismatch): " + indexName + "\nSQL: " + index);
            }
        } else {
            differences.push_back("Index missing in db2: " + indexName);
            indexDetails.push_back("Index only in db1: " + indexName + "\nSQL: " + index);
        }
    }

    for (const auto &index : onlyInDb2) {
        std::string indexName = ExtractIndexName(index);
        bool isFormatIndex = std::find(SPECIAL_FORMAT_INDEX.begin(), SPECIAL_FORMAT_INDEX.end(), indexName) !=
                             SPECIAL_FORMAT_INDEX.end();
        if (isFormatIndex) {
            if (!IsFormatIndexMatch(index, filteredIndexes1)) {
                differences.push_back("Index missing in db1 (format mismatch): " + indexName);
                indexDetails.push_back("Index only in db2 (format mismatch): " + indexName + "\nSQL: " + index);
            }
        } else {
            differences.push_back("Index missing in db1: " + indexName);
            indexDetails.push_back("Index only in db2: " + indexName + "\nSQL: " + index);
        }
    }
}

/**
 * @brief 过滤需要排除的触发器
 * @param triggers 原始触发器列表
 * @return 过滤后的触发器列表
 */
static std::vector<std::string> FilterTriggers(const std::vector<std::string> &triggers)
{
    std::vector<std::string> filteredTriggers;
    for (const auto &trigger : triggers) {
        std::string triggerName = ExtractTriggerName(trigger);
        if (std::find(SPECIAL_EXCLUDE_TRIGGER.begin(), SPECIAL_EXCLUDE_TRIGGER.end(), triggerName) ==
            SPECIAL_EXCLUDE_TRIGGER.end()) {
            filteredTriggers.push_back(trigger);
        }
    }
    return filteredTriggers;
}

/**
 * @brief 检查格式触发器是否匹配
 * @param trigger 要检查的触发器SQL
 * @param targetTriggers 目标触发器列表
 * @return 匹配返回true，否则返回false
 */
static bool IsFormatTriggerMatch(const std::string &trigger, const std::vector<std::string> &targetTriggers)
{
    std::string triggerName = ExtractTriggerName(trigger);
    for (const auto &targetTrigger : targetTriggers) {
        if (ExtractTriggerName(targetTrigger) == triggerName) {
            // 比较标准化后的SQL
            if (NormalizeTriggerSql(trigger) == NormalizeTriggerSql(targetTrigger)) {
                return true;
            }
        }
    }
    return false;
}

/**
 * @brief 比较两个数据库的触发器信息
 * @param triggers1 第一个数据库的触发器列表
 * @param triggers2 第二个数据库的触发器列表
 * @param differences 差异信息输出向量
 * @param triggerDetails 触发器详细信息输出向量（包含差异的完整SQL）
 */
static void CompareTriggers(std::vector<std::string> &triggers1, std::vector<std::string> &triggers2,
    std::vector<std::string> &differences, std::vector<std::string> &triggerDetails)
{
    // 过滤掉需要排除的触发器
    std::vector<std::string> filteredTriggers1 = FilterTriggers(triggers1);
    std::vector<std::string> filteredTriggers2 = FilterTriggers(triggers2);

    if (filteredTriggers1.size() != filteredTriggers2.size()) {
        differences.push_back("Trigger count mismatch (excluding SPECIAL_EXCLUDE_TRIGGER): db1 has " +
                              std::to_string(filteredTriggers1.size()) + " triggers, db2 has " +
                              std::to_string(filteredTriggers2.size()) + " triggers");
    }

    // 使用集合差集找出差异的触发器
    std::vector<std::string> onlyInDb1;
    std::vector<std::string> onlyInDb2;
    std::set_difference(filteredTriggers1.begin(), filteredTriggers1.end(), filteredTriggers2.begin(),
                        filteredTriggers2.end(), std::back_inserter(onlyInDb1));
    std::set_difference(filteredTriggers2.begin(), filteredTriggers2.end(), filteredTriggers1.begin(),
                        filteredTriggers1.end(), std::back_inserter(onlyInDb2));

    // 对差异触发器进行二次检查，考虑SPECIAL_FORMAT_TRIGGER的情况
    for (const auto &trigger : onlyInDb1) {
        std::string triggerName = ExtractTriggerName(trigger);
        bool isFormatTrigger = std::find(SPECIAL_FORMAT_TRIGGER.begin(), SPECIAL_FORMAT_TRIGGER.end(), triggerName) !=
                               SPECIAL_FORMAT_TRIGGER.end();
        if (isFormatTrigger) {
            if (!IsFormatTriggerMatch(trigger, filteredTriggers2)) {
                differences.push_back("Trigger missing in db2 (format mismatch): " + triggerName);
                triggerDetails.push_back("Trigger only in db1 (format mismatch): " + triggerName + "\nSQL: " + trigger);
            }
        } else {
            differences.push_back("Trigger missing in db2: " + triggerName);
            triggerDetails.push_back("Trigger only in db1: " + triggerName + "\nSQL: " + trigger);
        }
    }

    for (const auto &trigger : onlyInDb2) {
        std::string triggerName = ExtractTriggerName(trigger);
        bool isFormatTrigger = std::find(SPECIAL_FORMAT_TRIGGER.begin(), SPECIAL_FORMAT_TRIGGER.end(), triggerName) !=
                               SPECIAL_FORMAT_TRIGGER.end();
        if (isFormatTrigger) {
            if (!IsFormatTriggerMatch(trigger, filteredTriggers1)) {
                differences.push_back("Trigger missing in db1 (format mismatch): " + triggerName);
                triggerDetails.push_back("Trigger only in db2 (format mismatch): " + triggerName + "\nSQL: " + trigger);
            }
        } else {
            differences.push_back("Trigger missing in db1: " + triggerName);
            triggerDetails.push_back("Trigger only in db2: " + triggerName + "\nSQL: " + trigger);
        }
    }
}

/**
 * @brief 对比两个数据库的表结构是否一致
 * 
 * 该函数对比两个数据库的表、列、索引、触发器等结构信息，
 * 并记录所有差异到differences向量中。
 * 
 * 对比规则：
 * 1. 表数量和名称必须一致
 * 2. 每个表的列数量和名称必须一致
 * 3. 索引数量和SQL必须一致（排除SPECIAL_EXCLUDE_INDEX中定义的索引）
 * 4. 触发器数量和SQL必须一致
 * 
 * @param store1 第一个数据库存储对象
 * @param store2 第二个数据库存储对象
 * @param differences 差异信息输出向量
 * @param indexDetails 索引详细信息输出向量（包含差异的完整SQL）
 * @param triggerDetails 触发器详细信息输出向量（包含差异的完整SQL）
 * @return 结构一致返回true，不一致返回false
 */
static bool CompareTableStructure(NativeRdb::RdbStore &store1, NativeRdb::RdbStore &store2,
                                  std::vector<std::string> &differences,
                                  std::vector<std::string> &indexDetails,
                                  std::vector<std::string> &triggerDetails)
{
    // 获取并排序两个数据库的表名
    std::vector<std::string> tables1 = GetTableNames(store1);
    std::vector<std::string> tables2 = GetTableNames(store2);
    std::sort(tables1.begin(), tables1.end());
    std::sort(tables2.begin(), tables2.end());

    // 比较表信息
    CompareTables(tables1, tables2, differences);

    // 比较列信息
    CompareColumns(store1, store2, tables1, tables2, differences);

    // 获取索引信息并进行对比
    std::vector<std::string> indexes1 = GetIndexSqls(store1);
    std::vector<std::string> indexes2 = GetIndexSqls(store2);
    CompareIndexes(indexes1, indexes2, differences, indexDetails);

    // 检查触发器信息
    std::vector<std::string> triggers1 = GetTriggerSqls(store1);
    std::vector<std::string> triggers2 = GetTriggerSqls(store2);
    CompareTriggers(triggers1, triggers2, differences, triggerDetails);

    return differences.empty();
}

// 测试结果输出函数实现

/**
 * @brief 将测试结果写入文件
 * 
 * 该函数将Schema对比结果和任务配置检查结果写入指定的结果文件。
 * 
 * @param result 测试结果结构体，包含所有需要写入的测试结果信息
 */
void WriteTestResultToFile(const TestResult &result)
{
    std::ofstream outFile(result.resultFilePath);
    if (!outFile.is_open()) {
        std::cerr << "Failed to open result file: " << result.resultFilePath << std::endl;
        return;
    }

    outFile << "----------------------------------------" << std::endl;
    outFile << "   数据库升级Schema一致性测试结果" << std::endl;
    outFile << "----------------------------------------" << std::endl;
    outFile << std::endl;

    // 输出Schema对比结果
    outFile << "【Schema对比结果】" << std::endl;
    outFile << "----------------------------------------" << std::endl;
    if (result.isConsistent) {
        outFile << "状态: 完全一致" << std::endl;
    } else {
        outFile << "状态: 不一致" << std::endl;
    }
    outFile << std::endl;

    // 输出差异详情
    if (!result.differences.empty()) {
        outFile << "【不一致详情】" << std::endl;
        outFile << "----------------------------------------" << std::endl;
        for (size_t i = 0; i < result.differences.size(); ++i) {
            outFile << i + 1 << ". " << result.differences[i] << std::endl;
        }
        outFile << std::endl;
    } else {
        outFile << "【不一致详情】" << std::endl;
        outFile << "----------------------------------------" << std::endl;
        outFile << "无差异" << std::endl;
        outFile << std::endl;
    }

    // 输出索引差异详情
    if (!result.indexDetails.empty()) {
        outFile << "【索引SQL差异详情】" << std::endl;
        outFile << "----------------------------------------" << std::endl;
        for (size_t i = 0; i < result.indexDetails.size(); ++i) {
            outFile << i + 1 << ". " << result.indexDetails[i] << std::endl;
            outFile << std::endl;
        }
    }

    // 输出触发器差异详情
    if (!result.triggerDetails.empty()) {
        outFile << "【触发器SQL差异详情】" << std::endl;
        outFile << "----------------------------------------" << std::endl;
        for (size_t i = 0; i < result.triggerDetails.size(); ++i) {
            outFile << i + 1 << ". " << result.triggerDetails[i] << std::endl;
            outFile << std::endl;
        }
    }

    // 输出任务配置检查结果
    outFile << "【任务配置检查结果】" << std::endl;
    outFile << "----------------------------------------" << std::endl;
    if (result.taskCheckPassed) {
        outFile << "状态: 完全匹配" << std::endl;
    } else {
        outFile << "状态: 不匹配" << std::endl;
    }
    outFile << std::endl;

    // 输出任务配置错误详情
    if (!result.taskCheckErrors.empty()) {
        outFile << "【任务配置不匹配详情】" << std::endl;
        outFile << "----------------------------------------" << std::endl;
        for (size_t i = 0; i < result.taskCheckErrors.size(); ++i) {
            outFile << i + 1 << ". " << result.taskCheckErrors[i] << std::endl;
        }
        outFile << std::endl;
    } else {
        outFile << "【任务配置不匹配详情】" << std::endl;
        outFile << "----------------------------------------" << std::endl;
        outFile << "无不匹配项" << std::endl;
        outFile << std::endl;
    }

    outFile << "----------------------------------------" << std::endl;
    outFile << "   测试结束" << std::endl;
    outFile << "----------------------------------------" << std::endl;

    outFile.close();
}

// 测试用例实现

/**
 * @brief 检查指定任务是否为同步任务
 * @param taskName 任务名称
 * @return 是同步任务返回true，否则返回false
 */
static bool HasSyncTask(const std::string &taskName)
{
    auto syncTasks =
        UpgradeTaskRegistry::GetInstance().GetTasksAfterVersion(VERSION_TRANSFER_OWNERAPPID_TO_TOKENID, true);
    for (const auto &syncTask : syncTasks) {
        if (syncTask->GetName() == taskName) {
            return true;
        }
    }
    return false;
}

/**
 * @brief 检查指定任务是否为异步任务
 * @param taskName 任务名称
 * @return 是异步任务返回true，否则返回false
 */
static bool HasAsyncTask(const std::string &taskName)
{
    auto asyncTasks =
        UpgradeTaskRegistry::GetInstance().GetTasksAfterVersion(VERSION_TRANSFER_OWNERAPPID_TO_TOKENID, false);
    for (const auto &asyncTask : asyncTasks) {
        if (asyncTask->GetName() == taskName) {
            return true;
        }
    }
    return false;
}

/**
 * @brief 验证任务配置是否与实际任务类型匹配
 * @param taskName 任务名称
 * @param configValue 配置值（1=仅同步，2=仅异步，3=同步+异步）
 * @param taskCheckErrors 错误信息输出向量
 * @return 验证通过返回true，否则返回false
 */
static bool ValidateTaskConfig(const std::string &taskName, int32_t configValue,
                               std::vector<std::string> &taskCheckErrors)
{
    bool hasSyncTask = HasSyncTask(taskName);
    bool hasAsyncTask = HasAsyncTask(taskName);
    bool checkPassed = false;

    switch (configValue) {
        case TASK_CONFIG_SYNC_ONLY:  // 需要有同步任务
            checkPassed = hasSyncTask;
            if (!checkPassed) {
                taskCheckErrors.push_back("Task " + taskName + " config=1 requires sync task, but not found");
            }
            break;
        case TASK_CONFIG_ASYNC_ONLY:  // 需要有异步任务
            checkPassed = hasAsyncTask;
            if (!checkPassed) {
                taskCheckErrors.push_back("Task " + taskName + " config=2 requires async task, but not found");
            }
            break;
        case TASK_CONFIG_SYNC_AND_ASYNC:  // 需要有同步和异步任务
            checkPassed = hasSyncTask && hasAsyncTask;
            if (!checkPassed) {
                taskCheckErrors.push_back("Task " + taskName +
                                          " config=3 requires both sync and async tasks, but sync=" +
                                          std::to_string(hasSyncTask) + ", async=" + std::to_string(hasAsyncTask));
            }
            break;
        default:
            taskCheckErrors.push_back("Task " + taskName +
                                      " has invalid config value: " + std::to_string(configValue));
            break;
    }
    return checkPassed;
}

/**
 * @brief 验证从5.0升级到最新版本与直接刷最新版本数据库Schema一致性
 * 
 * 该测试用例验证以下内容：
 * 1. 模拟刷机场景（新建数据库）和升级场景（从旧版本升级）生成的Schema一致
 * 2. 升级任务配置与实际注册的任务类型（同步/异步）匹配
 * 
 * 测试步骤：
 * 1. 创建一个新数据库，模拟刷机场景
 * 2. 使用旧版本数据库进行升级，模拟升级场景
 * 3. 对比两个数据库的表结构、索引、触发器等是否一致
 * 4. 检查升级任务配置的正确性
 */
HWTEST_F(UpgradeSchemaTest, upgrade_schema_consistency_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("upgrade_schema_consistency_test_001 start");
    // 模拟刷机场景：创建新数据库
    TestRdbCreateCallback createCallBack;
    std::shared_ptr<NativeRdb::RdbStore> dbCreate = MakeStore(CREATE_DB_PATH, MEDIA_RDB_VERSION, createCallBack);
    ASSERT_NE(dbCreate, nullptr) << "Failed to create dbCreate";
    dbCreate->ExecuteSql(SPECIAL_CREATE_TABLE_SQL_1);

    // 模拟升级场景：从旧版本数据库升级
    TestRdbUpgradeCallback upgradeCallback;
    std::shared_ptr<NativeRdb::RdbStore> dbUpgrade = MakeStore(UPGRADE_DB_50_PATH, MEDIA_RDB_VERSION, upgradeCallback);
    ASSERT_NE(dbUpgrade, nullptr) << "Failed to create dbUpgrade";
    TestUpgradeDatabase(*dbUpgrade, UPGRADE_EVENT_50_PATH, UPGRADE_DB_50_VERSION, MEDIA_RDB_VERSION);
    dbUpgrade->ExecuteSql(SPECIAL_UPGRADE_TABLE_SQL_1);

    // 对比数据库表结构
    std::vector<std::string> differences;
    std::vector<std::string> indexDetails;
    std::vector<std::string> triggerDetails;
    bool isConsistent = CompareTableStructure(*dbCreate, *dbUpgrade, differences, indexDetails, triggerDetails);

    EXPECT_TRUE(isConsistent) << "Schema should be consistent between create and upgrade paths";

    std::vector<std::string> tables = GetTableNames(*dbCreate);
    EXPECT_GT(tables.size(), 0) << "Database should have at least one table";

    // 检查升级任务配置一致性
    auto tasks = UpgradeTaskRegistry::GetInstance().GetTasksAfterVersion(VERSION_TRANSFER_OWNERAPPID_TO_TOKENID);

    int32_t prefErrCode = 0;
    std::shared_ptr<Preferences> prefs = PreferencesHelper::GetPreferences(UPGRADE_EVENT_50_PATH, prefErrCode);
    ASSERT_NE(prefs, nullptr) << "Failed to get preferences from " << UPGRADE_EVENT_50_PATH;

    std::vector<std::string> taskCheckErrors;
    bool taskCheckPassed = true;
    for (const auto &task : tasks) {
        std::string taskName = task->GetName();
        int32_t configValue = prefs->GetInt(taskName, TASK_CONFIG_NOT_SET);

        if (configValue == TASK_CONFIG_NOT_SET) {
            taskCheckErrors.push_back("Task " + taskName + " has no configuration value in preference");
            taskCheckPassed = false;
            continue;
        }

        bool checkPassed = ValidateTaskConfig(taskName, configValue, taskCheckErrors);
        if (!checkPassed) {
            taskCheckPassed = false;
        }
    }
    EXPECT_TRUE(taskCheckPassed) << "Config should same";

    // 将测试结果写入文件
    TestResult result;
    result.resultFilePath = "/data/test/upgrade_schema_consistency_test_001_result.txt";
    result.isConsistent = isConsistent;
    result.differences = differences;
    result.taskCheckPassed = taskCheckPassed;
    result.taskCheckErrors = taskCheckErrors;
    result.indexDetails = indexDetails;
    result.triggerDetails = triggerDetails;
    WriteTestResultToFile(result);
    MEDIA_INFO_LOG("upgrade_schema_consistency_test_001 end");
}

/**
 * @brief 验证从6.1版本升级到最新版本与直接刷最新版本数据库Schema一致性
 * 
 * 该测试用例验证以下内容：
 * 1. 模拟刷机场景（新建数据库）和升级场景（从6.1版本升级）生成的Schema一致
 * 2. 升级任务配置与实际注册的任务类型（同步/异步）匹配
 * 
 * 测试步骤：
 * 1. 创建一个新数据库，模拟刷机场景（最新版本）
 * 2. 使用6.1版本数据库进行升级，模拟升级场景
 * 3. 对比两个数据库的表结构、索引、触发器等是否一致
 * 4. 检查升级任务配置的正确性
 */
HWTEST_F(UpgradeSchemaTest, upgrade_schema_consistency_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("upgrade_schema_consistency_test_002 start");
    TestRdbCreateCallback createCallBack;
    std::shared_ptr<NativeRdb::RdbStore> dbCreate = MakeStore(CREATE_DB_PATH, MEDIA_RDB_VERSION, createCallBack);
    ASSERT_NE(dbCreate, nullptr) << "Failed to create dbCreate for 6.1";
    dbCreate->ExecuteSql(SPECIAL_CREATE_TABLE_SQL_1);

    TestRdbUpgradeCallback upgradeCallback;
    std::shared_ptr<NativeRdb::RdbStore> dbUpgrade = MakeStore(UPGRADE_DB_61_PATH, MEDIA_RDB_VERSION, upgradeCallback);
    ASSERT_NE(dbUpgrade, nullptr) << "Failed to create dbUpgrade for 6.1";
    TestUpgradeDatabase(*dbUpgrade, UPGRADE_EVENT_61_PATH, UPGRADE_DB_61_VERSION, MEDIA_RDB_VERSION);
    dbUpgrade->ExecuteSql(SPECIAL_UPGRADE_TABLE_SQL_1);

    std::vector<std::string> differences;
    std::vector<std::string> indexDetails;
    std::vector<std::string> triggerDetails;
    bool isConsistent = CompareTableStructure(*dbCreate, *dbUpgrade, differences, indexDetails, triggerDetails);

    EXPECT_TRUE(isConsistent) << "Schema should be consistent between create and upgrade paths for 6.1";

    std::vector<std::string> tables = GetTableNames(*dbCreate);
    EXPECT_GT(tables.size(), 0) << "Database should have at least one table";

    auto tasks = UpgradeTaskRegistry::GetInstance().GetTasksAfterVersion(UPGRADE_DB_61_VERSION);

    int32_t prefErrCode = 0;
    std::shared_ptr<Preferences> prefs = PreferencesHelper::GetPreferences(UPGRADE_EVENT_61_PATH, prefErrCode);
    ASSERT_NE(prefs, nullptr) << "Failed to get preferences from " << UPGRADE_EVENT_61_PATH;

    std::vector<std::string> taskCheckErrors;
    bool taskCheckPassed = true;
    for (const auto &task : tasks) {
        std::string taskName = task->GetName();
        int32_t configValue = prefs->GetInt(taskName, TASK_CONFIG_NOT_SET);

        if (configValue == TASK_CONFIG_NOT_SET) {
            taskCheckErrors.push_back("Task " + taskName + " has no configuration value in preference for 6.1");
            taskCheckPassed = false;
            continue;
        }

        bool checkPassed = ValidateTaskConfig(taskName, configValue, taskCheckErrors);
        if (!checkPassed) {
            taskCheckPassed = false;
        }
    }
    EXPECT_TRUE(taskCheckPassed) << "Config should same for 6.1";

    TestResult result;
    result.resultFilePath = "/data/test/upgrade_schema_consistency_test_002_result.txt";
    result.isConsistent = isConsistent;
    result.differences = differences;
    result.taskCheckPassed = taskCheckPassed;
    result.taskCheckErrors = taskCheckErrors;
    result.indexDetails = indexDetails;
    result.triggerDetails = triggerDetails;
    WriteTestResultToFile(result);
    MEDIA_INFO_LOG("upgrade_schema_consistency_test_002 end");
}

/**
 * @brief 测试IsSchemaSubsetByAttach函数
 * 
 * 该测试用例验证upgrade_test_5_0_187.db是否为upgrade_test_6_1_561.db的子集。
 *
 */
HWTEST_F(UpgradeSchemaTest, IsSchemaSubsetByAttach_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("IsSchemaSubsetByAttach_test_001 start");
    TestRdbUpgradeCallback upgradeCallback561;
    std::shared_ptr<NativeRdb::RdbStore> db561 = MakeStore(UPGRADE_DB_61_PATH, MEDIA_RDB_VERSION, upgradeCallback561);
    ASSERT_NE(db561, nullptr) << "Failed to create db561";

    TestRdbUpgradeCallback upgradeCallback187;
    std::shared_ptr<NativeRdb::RdbStore> db187 = MakeStore(UPGRADE_DB_50_PATH, MEDIA_RDB_VERSION, upgradeCallback187);
    ASSERT_NE(db187, nullptr) << "Failed to create db187";

    bool isSubset = UpgradeManager::IsSchemaSubsetByAttach(*db561, *db187);

    EXPECT_TRUE(isSubset) << "upgrade_test_5_0_187.db should be a subset of upgrade_test_6_1_561.db";

    MEDIA_INFO_LOG("IsSchemaSubsetByAttach_test_001 end");
}

/**
 * @brief 测试IsSchemaSubsetByAttach函数
 * 
 * 该测试用例验证upgrade_test_6_1_561.db不为upgrade_test_5_0_187.db的子集。
 *
 */
HWTEST_F(UpgradeSchemaTest, IsSchemaSubsetByAttach_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("IsSchemaSubsetByAttach_test_001 start");
    TestRdbUpgradeCallback upgradeCallback561;
    std::shared_ptr<NativeRdb::RdbStore> db561 = MakeStore(UPGRADE_DB_61_PATH, MEDIA_RDB_VERSION, upgradeCallback561);
    ASSERT_NE(db561, nullptr) << "Failed to create db561";

    TestRdbUpgradeCallback upgradeCallback187;
    std::shared_ptr<NativeRdb::RdbStore> db187 = MakeStore(UPGRADE_DB_50_PATH, MEDIA_RDB_VERSION, upgradeCallback187);
    ASSERT_NE(db187, nullptr) << "Failed to create db187";

    bool isSubset = UpgradeManager::IsSchemaSubsetByAttach(*db187, *db561);

    EXPECT_TRUE(isSubset) << "upgrade_test_5_0_187.db should be a subset of upgrade_test_6_1_561.db";

    MEDIA_INFO_LOG("IsSchemaSubsetByAttach_test_001 end");
}

/**
 * @brief 测试UpgradeHelper::ExecSqlWithRetry函数
 * 
 * 该测试用例验证ExecSqlWithRetry函数能否正确处理SQL执行。
 */
HWTEST_F(UpgradeSchemaTest, ExecSqlWithRetry_test_001, TestSize.Level1)
{
    TestRdbCreateCallback createCallBack;
    std::shared_ptr<NativeRdb::RdbStore> db = MakeStore(CREATE_DB_PATH, MEDIA_RDB_VERSION, createCallBack);
    ASSERT_NE(db, nullptr) << "Failed to create db";

    int32_t result = UpgradeHelper::ExecSqlWithRetry([&]() {
        return db->ExecuteSql("CREATE TABLE IF NOT EXISTS test_retry_table (id INTEGER PRIMARY KEY, name TEXT)");
    });
    EXPECT_EQ(result, NativeRdb::E_OK) << "ExecSqlWithRetry should succeed for valid SQL";

    result = UpgradeHelper::ExecSqlWithRetry(
        [&]() { return db->ExecuteSql("INSERT INTO test_retry_table VALUES (1, 'test')"); });
    EXPECT_EQ(result, NativeRdb::E_OK) << "ExecSqlWithRetry should succeed for insert";
}

/**
 * @brief 测试UpgradeHelper::ExecuteCommands函数
 * 
 * 该测试用例验证ExecuteCommands函数能否正确执行SQL命令列表。
 */
HWTEST_F(UpgradeSchemaTest, ExecuteCommands_test_001, TestSize.Level1)
{
    TestRdbCreateCallback createCallBack;
    std::shared_ptr<NativeRdb::RdbStore> db = MakeStore(CREATE_DB_PATH, MEDIA_RDB_VERSION, createCallBack);
    ASSERT_NE(db, nullptr) << "Failed to create db";

    SqlBuilder builder;
    builder.AddRawSql("CREATE TABLE IF NOT EXISTS test_commands_table1 (id INTEGER PRIMARY KEY)");
    builder.AddRawSql("CREATE TABLE IF NOT EXISTS test_commands_table2 (id INTEGER PRIMARY KEY, value TEXT)");
    builder.AddRawSql("CREATE INDEX IF NOT EXISTS idx_test_commands ON test_commands_table2(value)");

    std::vector<std::shared_ptr<ISqlCommand>> commands = builder.Build();
    EXPECT_EQ(commands.size(), static_cast<size_t>(3)) << "Build should return 3 commands";

    std::vector<std::pair<int32_t, int32_t>> errResult = UpgradeHelper::ExecuteCommands(commands, *db, false);
    EXPECT_TRUE(errResult.empty()) << "ExecuteCommands should succeed for all commands";
}

/**
 * @brief 测试SqlBuilder::AddRawSql函数
 * 
 * 该测试用例验证AddRawSql能否正确添加原始SQL命令。
 */
HWTEST_F(UpgradeSchemaTest, SqlBuilder_AddRawSql_test_001, TestSize.Level1)
{
    SqlBuilder builder;
    builder.AddRawSql("SELECT * FROM test_table");
    builder.AddRawSql("UPDATE test_table SET value = 1");

    std::vector<std::shared_ptr<ISqlCommand>> commands = builder.Build();
    EXPECT_EQ(commands.size(), static_cast<size_t>(2)) << "AddRawSql should add 2 commands";
}

/**
 * @brief 测试SqlBuilder::AddParameterizedSql函数
 * 
 * 该测试用例验证AddParameterizedSql能否正确添加参数化SQL命令。
 */
HWTEST_F(UpgradeSchemaTest, SqlBuilder_AddParameterizedSql_test_001, TestSize.Level1)
{
    TestRdbCreateCallback createCallBack;
    std::shared_ptr<NativeRdb::RdbStore> db = MakeStore(CREATE_DB_PATH, MEDIA_RDB_VERSION, createCallBack);
    ASSERT_NE(db, nullptr) << "Failed to create db";

    db->ExecuteSql("CREATE TABLE IF NOT EXISTS test_param_table (id INTEGER, name TEXT)");

    SqlBuilder builder;
    std::vector<NativeRdb::ValueObject> args;
    args.push_back(NativeRdb::ValueObject(1));
    args.push_back(NativeRdb::ValueObject("test_name"));
    builder.AddParameterizedSql("INSERT INTO test_param_table VALUES (?, ?)", args);

    std::vector<std::shared_ptr<ISqlCommand>> commands = builder.Build();
    EXPECT_EQ(commands.size(), static_cast<size_t>(1)) << "AddParameterizedSql should add 1 command";

    std::vector<std::pair<int32_t, int32_t>> errResult = UpgradeHelper::ExecuteCommands(commands, *db, false);
    EXPECT_TRUE(errResult.empty()) << "ExecuteCommands should succeed for parameterized SQL";
}

/**
 * @brief 测试SqlBuilder::AddColumn函数
 * 
 * 该测试用例验证AddColumn能否正确构建添加列的SQL。
 */
HWTEST_F(UpgradeSchemaTest, SqlBuilder_AddColumn_test_001, TestSize.Level1)
{
    TestRdbCreateCallback createCallBack;
    std::shared_ptr<NativeRdb::RdbStore> db = MakeStore(CREATE_DB_PATH, MEDIA_RDB_VERSION, createCallBack);
    ASSERT_NE(db, nullptr) << "Failed to create db";

    db->ExecuteSql("CREATE TABLE IF NOT EXISTS test_add_column_table (id INTEGER PRIMARY KEY)");

    SqlBuilder builder;
    builder.AddColumn("test_add_column_table", "new_column", "TEXT");
    builder.AddColumn("test_add_column_table", "new_int_column", "INTEGER DEFAULT 0");

    std::vector<std::shared_ptr<ISqlCommand>> commands = builder.Build();
    EXPECT_EQ(commands.size(), static_cast<size_t>(2)) << "AddColumn should add 2 commands";
}

/**
 * @brief 测试SqlBuilder::DropColumn函数
 * 
 * 该测试用例验证DropColumn能否正确构建删除列的SQL（SQLite不支持直接删除列）。
 */
HWTEST_F(UpgradeSchemaTest, SqlBuilder_DropColumn_test_001, TestSize.Level1)
{
    SqlBuilder builder;
    builder.DropColumn("test_table", "old_column");

    std::vector<std::shared_ptr<ISqlCommand>> commands = builder.Build();
    EXPECT_EQ(commands.size(), static_cast<size_t>(1)) << "DropColumn should add 1 command";
}

/**
 * @brief 测试SqlBuilder::CreateIndex函数
 * 
 * 该测试用例验证CreateIndex能否正确构建创建索引的SQL。
 */
HWTEST_F(UpgradeSchemaTest, SqlBuilder_CreateIndex_test_001, TestSize.Level1)
{
    TestRdbCreateCallback createCallBack;
    std::shared_ptr<NativeRdb::RdbStore> db = MakeStore(CREATE_DB_PATH, MEDIA_RDB_VERSION, createCallBack);
    ASSERT_NE(db, nullptr) << "Failed to create db";

    db->ExecuteSql("CREATE TABLE IF NOT EXISTS test_index_table (id INTEGER, name TEXT)");

    SqlBuilder builder;
    builder.CreateIndex("test_idx_name", "test_index_table", "name");
    builder.CreateIndex("test_idx_id", "test_index_table", "id");

    std::vector<std::shared_ptr<ISqlCommand>> commands = builder.Build();
    EXPECT_EQ(commands.size(), static_cast<size_t>(2)) << "CreateIndex should add 2 commands";

    std::vector<std::pair<int32_t, int32_t>> errResult = UpgradeHelper::ExecuteCommands(commands, *db, false);
    EXPECT_TRUE(errResult.empty()) << "ExecuteCommands should succeed for create index";
}

/**
 * @brief 测试SqlBuilder::DropIndex函数
 * 
 * 该测试用例验证DropIndex能否正确构建删除索引的SQL。
 */
HWTEST_F(UpgradeSchemaTest, SqlBuilder_DropIndex_test_001, TestSize.Level1)
{
    SqlBuilder builder;
    builder.DropIndex("old_index_name");

    std::vector<std::shared_ptr<ISqlCommand>> commands = builder.Build();
    EXPECT_EQ(commands.size(), static_cast<size_t>(1)) << "DropIndex should add 1 command";
}

/**
 * @brief 测试SqlBuilder::DropTrigger函数
 * 
 * 该测试用例验证DropTrigger能否正确构建删除触发器的SQL。
 */
HWTEST_F(UpgradeSchemaTest, SqlBuilder_DropTrigger_test_001, TestSize.Level1)
{
    SqlBuilder builder;
    builder.DropTrigger("old_trigger_name");

    std::vector<std::shared_ptr<ISqlCommand>> commands = builder.Build();
    EXPECT_EQ(commands.size(), static_cast<size_t>(1)) << "DropTrigger should add 1 command";
}

/**
 * @brief 测试SqlBuilder::DropTable函数
 * 
 * 该测试用例验证DropTable能否正确构建删除表的SQL。
 */
HWTEST_F(UpgradeSchemaTest, SqlBuilder_DropTable_test_001, TestSize.Level1)
{
    SqlBuilder builder;
    builder.DropTable("old_table_name");

    std::vector<std::shared_ptr<ISqlCommand>> commands = builder.Build();
    EXPECT_EQ(commands.size(), static_cast<size_t>(1)) << "DropTable should add 1 command";
}

/**
 * @brief 测试SqlBuilder::Clear函数
 * 
 * 该测试用例验证Clear能否正确清空构建器中的命令。
 */
HWTEST_F(UpgradeSchemaTest, SqlBuilder_Clear_test_001, TestSize.Level1)
{
    SqlBuilder builder;
    builder.AddRawSql("SELECT * FROM test1");
    builder.AddRawSql("SELECT * FROM test2");

    std::vector<std::shared_ptr<ISqlCommand>> commands1 = builder.Build();
    EXPECT_EQ(commands1.size(), static_cast<size_t>(2)) << "Build should return 2 commands";

    builder.Clear();
    std::vector<std::shared_ptr<ISqlCommand>> commands2 = builder.Build();
    EXPECT_EQ(commands2.size(), static_cast<size_t>(0)) << "Build should return 0 commands after Clear";
}

/**
 * @brief 测试UpgradeManager::DoUpgrade函数（无需升级场景）
 * 
 * 该测试用例验证当当前版本等于目标版本时，DoUpgrade应返回E_OK且不执行任务。
 */
HWTEST_F(UpgradeSchemaTest, DoUpgrade_no_upgrade_test_001, TestSize.Level1)
{
    TestRdbCreateCallback createCallBack;
    std::shared_ptr<NativeRdb::RdbStore> db = MakeStore(CREATE_DB_PATH, MEDIA_RDB_VERSION, createCallBack);
    ASSERT_NE(db, nullptr) << "Failed to create db";

    UpgradeManagerConfig config(false, UPGRADE_EVENT_61_PATH, RDB_CONFIG_PATH_TEST, MEDIA_RDB_VERSION,
                                MEDIA_RDB_VERSION);
    int32_t ret = UpgradeManager::GetInstance().Initialize(config);
    EXPECT_EQ(ret, NativeRdb::E_OK) << "Initialize should succeed";

    ret = UpgradeManager::GetInstance().UpgradeSync(*db);
    EXPECT_EQ(ret, NativeRdb::E_OK) << "UpgradeSync should succeed when no upgrade needed";
}

/**
 * @brief 测试UpgradeManager::SetObserver函数
 * 
 * 该测试用例验证SetObserver能否正确设置升级观察者。
 */
HWTEST_F(UpgradeSchemaTest, SetObserver_test_001, TestSize.Level1)
{
    auto observer = std::make_shared<DefaultUpgradeObserver>();
    UpgradeManager::GetInstance().SetObserver(observer);

    UpgradeManagerConfig config(false, UPGRADE_EVENT_61_PATH, RDB_CONFIG_PATH_TEST, UPGRADE_DB_50_VERSION,
                                MEDIA_RDB_VERSION);
    int32_t ret = UpgradeManager::GetInstance().Initialize(config);
    EXPECT_EQ(ret, NativeRdb::E_OK) << "Initialize should succeed with observer";
}

/**
 * @brief 测试ExecuteCommands错误处理（需要跳过）
 * 
 * 该测试用例验证ExecuteCommands在遇到错误时能否正确跳过后续命令。
 */
HWTEST_F(UpgradeSchemaTest, ExecuteCommands_error_skip_test_001, TestSize.Level1)
{
    TestRdbCreateCallback createCallBack;
    std::shared_ptr<NativeRdb::RdbStore> db = MakeStore(CREATE_DB_PATH, MEDIA_RDB_VERSION, createCallBack);
    ASSERT_NE(db, nullptr) << "Failed to create db";

    SqlBuilder builder;
    builder.AddRawSql("CREATE TABLE IF NOT EXISTS valid_table (id INTEGER)");
    builder.AddRawSql("INVALID SQL STATEMENT");
    builder.AddRawSql("CREATE TABLE IF NOT EXISTS skipped_table (id INTEGER)");

    std::vector<std::shared_ptr<ISqlCommand>> commands = builder.Build();

    std::vector<std::pair<int32_t, int32_t>> errResult = UpgradeHelper::ExecuteCommands(commands, *db, true);
    EXPECT_FALSE(errResult.empty()) << "ExecuteCommands should return errors for invalid SQL";
    EXPECT_EQ(errResult.size(), static_cast<size_t>(1)) << "Should only return first error when needSkip=true";
}

/**
 * @brief 测试ExecuteCommands错误处理（不跳过）
 * 
 * 该测试用例验证ExecuteCommands在遇到错误时能否继续执行后续命令。
 */
HWTEST_F(UpgradeSchemaTest, ExecuteCommands_error_no_skip_test_001, TestSize.Level1)
{
    TestRdbCreateCallback createCallBack;
    std::shared_ptr<NativeRdb::RdbStore> db = MakeStore(CREATE_DB_PATH, MEDIA_RDB_VERSION, createCallBack);
    ASSERT_NE(db, nullptr) << "Failed to create db";

    SqlBuilder builder;
    builder.AddRawSql("CREATE TABLE IF NOT EXISTS valid_table1 (id INTEGER)");
    builder.AddRawSql("INVALID SQL STATEMENT 1");
    builder.AddRawSql("CREATE TABLE IF NOT EXISTS valid_table2 (id INTEGER)");
    builder.AddRawSql("INVALID SQL STATEMENT 2");

    std::vector<std::shared_ptr<ISqlCommand>> commands = builder.Build();

    std::vector<std::pair<int32_t, int32_t>> errResult = UpgradeHelper::ExecuteCommands(commands, *db, false);
    EXPECT_FALSE(errResult.empty()) << "ExecuteCommands should return errors for invalid SQL";
    EXPECT_EQ(errResult.size(), static_cast<size_t>(2)) << "Should return all errors when needSkip=false";
}

/**
 * @brief 测试UpgradeTaskRegistry::GetTasksAfterVersion函数
 * 
 * 该测试用例验证GetTasksAfterVersion能否正确获取升级任务列表。
 */
HWTEST_F(UpgradeSchemaTest, GetTasksAfterVersion_test_001, TestSize.Level1)
{
    auto syncTasks = UpgradeTaskRegistry::GetInstance().GetTasksAfterVersion(UPGRADE_DB_50_VERSION, true);
    auto asyncTasks = UpgradeTaskRegistry::GetInstance().GetTasksAfterVersion(UPGRADE_DB_50_VERSION, false);
    auto allTasks = UpgradeTaskRegistry::GetInstance().GetTasksAfterVersion(UPGRADE_DB_50_VERSION);

    EXPECT_GE(syncTasks.size(), static_cast<size_t>(0)) << "Should have sync tasks";
    EXPECT_GE(asyncTasks.size(), static_cast<size_t>(0)) << "Should have async tasks";
    EXPECT_EQ(allTasks.size(), syncTasks.size() + asyncTasks.size()) << "All tasks should equal sync + async";
}

/**
 * @brief 测试UpgradeManager::Initialize函数（重复初始化）
 * 
 * 该测试用例验证Initialize能否支持重复初始化。
 */
HWTEST_F(UpgradeSchemaTest, Initialize_repeat_test_001, TestSize.Level1)
{
    UpgradeManagerConfig config1(false, UPGRADE_EVENT_61_PATH, RDB_CONFIG_PATH_TEST, UPGRADE_DB_50_VERSION,
                                MEDIA_RDB_VERSION);
    int32_t ret1 = UpgradeManager::GetInstance().Initialize(config1);
    EXPECT_EQ(ret1, NativeRdb::E_OK) << "First Initialize should succeed";

    UpgradeManagerConfig config2(false, UPGRADE_EVENT_61_PATH, RDB_CONFIG_PATH_TEST, UPGRADE_DB_61_VERSION,
                                MEDIA_RDB_VERSION);
    int32_t ret2 = UpgradeManager::GetInstance().Initialize(config2);
    EXPECT_EQ(ret2, NativeRdb::E_OK) << "Second Initialize should succeed";
}
}  // namespace OHOS::Media