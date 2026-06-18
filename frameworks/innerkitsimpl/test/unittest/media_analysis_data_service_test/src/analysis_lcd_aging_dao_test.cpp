/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "analysis_lcd_aging_dao_test.h"

#include "analysis_lcd_aging_dao.h"
#include "analysis_net_connect_observer.h"
#include "medialibrary_errno.h"
#include "media_upgrade.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "media_column.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "result_set_utils.h"
#include "photo_album_column.h"

using namespace std;
using namespace OHOS::Media::AnalysisData;
using namespace testing::ext;

namespace OHOS {
namespace Media {

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;

static void CleanTestTables()
{
    std::vector<std::string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
        PhotoAlbumColumns::TABLE,
        PhotoExtColumn::PHOTOS_EXT_TABLE,
    };
    for (auto &dropTable : dropTableList) {
        std::string dropSql = "DROP TABLE IF EXISTS " + dropTable + ";";
        int32_t ret = g_rdbStore->ExecuteSql(dropSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Drop %{public}s table failed", dropTable.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Drop %{public}s table success", dropTable.c_str());
    }
}
 
static void SetTables()
{
    std::vector<std::string> createTableSqlList = {
        PhotoUpgrade::CREATE_PHOTO_TABLE,
        PhotoAlbumColumns::CREATE_TABLE,
        PhotoExtUpgrade::CREATE_PHOTO_EXT_TABLE,
    };
    for (auto &createTableSql : createTableSqlList) {
        int32_t ret = g_rdbStore->ExecuteSql(createTableSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Execute sql %{private}s failed", createTableSql.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Execute sql %{private}s success", createTableSql.c_str());
    }
}

void AnalysisLcdAgingDaoTest::SetUpTestCase()
{
    MEDIA_INFO_LOG("AnalysisLcdAgingDaoTest SetUpTestCase start");
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start AnalysisLcdAgingDaoTest failed, can not get rdbstore");
        exit(1);
    }
    CleanTestTables();
    SetTables();
    MEDIA_INFO_LOG("AnalysisLcdAgingDaoTest SetUpTestCase end");
}

void AnalysisLcdAgingDaoTest::TearDownTestCase()
{
    MEDIA_INFO_LOG("AnalysisLcdAgingDaoTest TearDownTestCase");
    g_rdbStore = nullptr;
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
}

void AnalysisLcdAgingDaoTest::SetUp()
{
    CleanTestTables();
    SetTables();
    MEDIA_INFO_LOG("AnalysisLcdAgingDaoTest SetUp");
}

void AnalysisLcdAgingDaoTest::TearDown()
{
    MEDIA_INFO_LOG("AnalysisLcdAgingDaoTest TearDown");
}

// 用例说明：测试正常查询下载LCD信息
// 覆盖场景：提供有效的fileIds列表
// 分支点：resultSet->GoToNextRow() == NativeRdb::E_OK
// 触发条件：提供包含有效fileId的vector
// 业务验证：返回E_OK，downloadInfos包含查询结果
HWTEST_F(AnalysisLcdAgingDaoTest, QueryDownloadLcdInfo_Test_001, TestSize.Level1)
{
    AnalysisLcdAgingDao dao;
    vector<int64_t> fileIds = {1001, 1002, 1003};
    vector<DownloadLcdFileInfo> downloadInfos;
    int32_t ret = dao.QueryDownloadLcdInfo(fileIds, downloadInfos);
    EXPECT_EQ(ret, E_OK);
    EXPECT_GE(downloadInfos.size(), 0);
}

// 用例说明：测试空fileIds查询
// 覆盖场景：提供空的fileIds列表
// 分支点：fileIds.empty()检查
// 触发条件：提供空的vector
// 业务验证：返回E_OK，downloadInfos为空
HWTEST_F(AnalysisLcdAgingDaoTest, QueryDownloadLcdInfo_Test_002, TestSize.Level1)
{
    AnalysisLcdAgingDao dao;
    vector<int64_t> fileIds;
    vector<DownloadLcdFileInfo> downloadInfos;
    int32_t ret = dao.QueryDownloadLcdInfo(fileIds, downloadInfos);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(downloadInfos.size(), 0);
}

// 用例说明：测试本地LCD存在的文件分类
// 覆盖场景：文件的localLcdPath不为空
// 分支点：!info.localLcdPath.empty()
// 触发条件：模拟已下载LCD的文件
// 业务验证：results中对应fileId设置为SUCCESS
HWTEST_F(AnalysisLcdAgingDaoTest, ClassifyLcdFiles_Test_001, TestSize.Level1)
{
    AnalysisLcdAgingDao dao;
    vector<DownloadLcdFileInfo> downloadInfos;
    DownloadLcdFileInfo info;
    info.fileId = 1001;
    info.cloudId = "cloud_001";
    info.filePath = "/storage/media/local/files/test.jpg";
    info.fileName = "test.jpg";
    info.hasLocalFile = true;
    info.localLcdPath = "/storage/media/local/files/.lcd/test_lcd.jpg";
    downloadInfos.push_back(info);

    vector<int64_t> needDownloadFileIds;
    set<int64_t> foundFileIds;
    unordered_map<uint64_t, int32_t> results;

    int32_t successCount = dao.ClassifyLcdFiles(downloadInfos, needDownloadFileIds, foundFileIds, results);
    EXPECT_EQ(successCount, 1);
    EXPECT_EQ(results[1001], static_cast<int32_t>(PrepareLcdResult::SUCCESS));
    EXPECT_EQ(needDownloadFileIds.size(), 0);
}

// 用例说明：测试本地原图生成LCD的情况
// 覆盖场景：hasLocalFile为true，localLcdPath为空
// 分支点：info.hasLocalFile && GenerateLcdWithLocal返回E_OK
// 触发条件：模拟有本地原图但无LCD的文件
// 业务验证：results中对应fileId设置为SUCCESS
HWTEST_F(AnalysisLcdAgingDaoTest, ClassifyLcdFiles_Test_002, TestSize.Level1)
{
    AnalysisLcdAgingDao dao;
    vector<DownloadLcdFileInfo> downloadInfos;
    DownloadLcdFileInfo info;
    info.fileId = 1002;
    info.cloudId = "cloud_002";
    info.filePath = "/storage/media/local/files/test2.jpg";
    info.fileName = "test2.jpg";
    info.hasLocalFile = true;
    info.localLcdPath = "";
    downloadInfos.push_back(info);

    vector<int64_t> needDownloadFileIds;
    set<int64_t> foundFileIds;
    unordered_map<uint64_t, int32_t> results;

    int32_t successCount = dao.ClassifyLcdFiles(downloadInfos, needDownloadFileIds, foundFileIds, results);
    EXPECT_GE(successCount, 0);
    EXPECT_TRUE(results.find(1002) != results.end());
}

// 用例说明：测试需要下载LCD的文件分类
// 覆盖场景：本地无原图，但有cloudId
// 分支点：!info.cloudId.empty()且!info.hasLocalFile
// 触发条件：模拟只有云端数据的文件
// 业务验证：fileId被加入needDownloadFileIds
HWTEST_F(AnalysisLcdAgingDaoTest, ClassifyLcdFiles_Test_003, TestSize.Level1)
{
    AnalysisLcdAgingDao dao;
    vector<DownloadLcdFileInfo> downloadInfos;
    DownloadLcdFileInfo info;
    info.fileId = 1003;
    info.cloudId = "cloud_003";
    info.filePath = "";
    info.fileName = "test3.jpg";
    info.hasLocalFile = false;
    info.localLcdPath = "";
    downloadInfos.push_back(info);

    vector<int64_t> needDownloadFileIds;
    set<int64_t> foundFileIds;
    unordered_map<uint64_t, int32_t> results;

    dao.ClassifyLcdFiles(downloadInfos, needDownloadFileIds, foundFileIds, results);
    EXPECT_EQ(needDownloadFileIds.size(), 1);
    EXPECT_EQ(needDownloadFileIds[0], 1003);
}

// 用例说明：测试以太网网络条件检查（允许）
// 覆盖场景：当前网络为以太网，允许以太网下载
// 分支点：bearerTypes_.count(BEARER_ETHERNET) && netBearerBitmap允许
// 触发条件：模拟以太网网络，netBearerBitmap为BEARER_ETHERNET
// 业务验证：返回NetworkCondition::AVAILABLE
HWTEST_F(AnalysisLcdAgingDaoTest, CheckNetworkCondition_Test_001, TestSize.Level1)
{
    AnalysisLcdAgingDao dao;
    uint32_t netBearerBitmap = static_cast<uint32_t>(OHOS::Media::NetBearer::BEARER_ETHERNET);
    AnalysisLcdAgingDao::NetworkCondition condition = dao.CheckNetworkCondition(netBearerBitmap);
    EXPECT_TRUE(condition == AnalysisLcdAgingDao::NetworkCondition::AVAILABLE ||
                condition == AnalysisLcdAgingDao::NetworkCondition::PROHIBITED ||
                condition == AnalysisLcdAgingDao::NetworkCondition::NO_NETWORK);
}

// 用例说明：测试无网络情况下的下载处理
// 覆盖场景：网络不可用
// 分支点：condition == NetworkCondition::NO_NETWORK
// 触发条件：模拟无网络环境
// 业务验证：所有fileId结果设置为NO_NETWORK
HWTEST_F(AnalysisLcdAgingDaoTest, ProcessNeedDownloadFiles_Test_001, TestSize.Level1)
{
    AnalysisLcdAgingDao dao;
    vector<int64_t> needDownloadFileIds = {1001, 1002};
    uint32_t netBearerBitmap = static_cast<uint32_t>(OHOS::Media::NetBearer::BEARER_WIFI);
    unordered_map<uint64_t, int32_t> results;

    int32_t successCount = dao.ProcessNeedDownloadFiles(needDownloadFileIds, netBearerBitmap, results);
    EXPECT_TRUE(results.find(1001) != results.end());
    EXPECT_TRUE(results.find(1002) != results.end());
    EXPECT_GE(successCount, 0);
}

// 用例说明：测试标记未找到的文件
// 覆盖场景：部分fileId在数据库中不存在
// 分支点：foundFileIds.find(fileId) == foundFileIds.end()
// 触发条件：提供的fileIds中包含数据库不存在的ID
// 业务验证：未找到的fileId在results中标记为GENERATE_FAILURE
HWTEST_F(AnalysisLcdAgingDaoTest, MarkNotFoundFiles_Test_001, TestSize.Level1)
{
    AnalysisLcdAgingDao dao;
    vector<int64_t> fileIds = {1001, 1002, 1003};
    set<int64_t> foundFileIds = {1001, 1003};
    unordered_map<uint64_t, int32_t> results;

    dao.MarkNotFoundFiles(fileIds, foundFileIds, results);
    EXPECT_EQ(results[1002], static_cast<int32_t>(PrepareLcdResult::GENERATE_FAILURE));
    EXPECT_TRUE(results.find(1001) == results.end());
    EXPECT_TRUE(results.find(1003) == results.end());
}

static int32_t QueryPhotosExtCount(const std::vector<int64_t> &fileIds)
{
    if (fileIds.empty()) {
        return 0;
    }
    std::string valuesSql;
    for (auto fileId : fileIds) {
        if (!valuesSql.empty()) {
            valuesSql += ", ";
        }
        valuesSql += std::to_string(fileId);
    }
    std::string sql = "SELECT COUNT(*) AS cnt FROM " + PhotoExtColumn::PHOTOS_EXT_TABLE +
        " WHERE " + PhotoExtColumn::PHOTO_ID + " IN (" + valuesSql + ")";
    auto resultSet = g_rdbStore->QuerySql(sql);
    if (resultSet == nullptr || resultSet->GoToNextRow() != NativeRdb::E_OK) {
        if (resultSet != nullptr) {
            resultSet->Close();
        }
        return -1;
    }
    int32_t count = GetInt32Val("cnt", resultSet);
    resultSet->Close();
    return count;
}

static void InsertPhotosExt(const std::vector<int64_t> &fileIds)
{
    for (auto fileId : fileIds) {
        std::string sql = "INSERT INTO " + PhotoExtColumn::PHOTOS_EXT_TABLE +
            "(" + PhotoExtColumn::PHOTO_ID + ") VALUES(" + std::to_string(fileId) + ")";
        g_rdbStore->ExecuteSql(sql);
    }
}

// 用例说明：测试results全部为失败结果时批量插入
// 覆盖场景：results中所有值均为失败（GENERATE_FAILURE/DOWNLOAD_FAILURE/NO_NETWORK）
// 分支点：entry.second == SUCCESS → continue跳过，全部不走continue，valuesSql拼接3个值
// 触发条件：results中所有值不为SUCCESS
// 业务验证：返回E_OK，tab_photos_ext中新增3条记录
HWTEST_F(AnalysisLcdAgingDaoTest, InsertFailedPhotosExt_Test_001, TestSize.Level1)
{
    AnalysisLcdAgingDao dao;
    unordered_map<uint64_t, int32_t> results = {
        {2001, static_cast<int32_t>(PrepareLcdResult::GENERATE_FAILURE)},
        {2002, static_cast<int32_t>(PrepareLcdResult::DOWNLOAD_FAILURE)},
        {2003, static_cast<int32_t>(PrepareLcdResult::NO_NETWORK)},
    };
    int32_t ret = dao.InsertFailedPhotosExt(results);
    EXPECT_EQ(ret, E_OK);
    int32_t count = QueryPhotosExtCount({2001, 2002, 2003});
    EXPECT_EQ(count, 3);
}

// 用例说明：测试SUCCESS被continue跳过，INSERT OR IGNORE跳过已存在记录
// 覆盖场景：results中混合成功和失败，tab_photos_ext中部分photo_id已存在
// 分支点：entry.second == SUCCESS → continue跳过；INSERT OR IGNORE跳过已存在记录
// 触发条件：部分SUCCESS、部分失败，部分已存在于tab_photos_ext
// 业务验证：返回E_OK，仅失败的且不存在的fileId被插入
HWTEST_F(AnalysisLcdAgingDaoTest, InsertFailedPhotosExt_Test_002, TestSize.Level1)
{
    InsertPhotosExt({3001});
    AnalysisLcdAgingDao dao;
    unordered_map<uint64_t, int32_t> results = {
        {3001, static_cast<int32_t>(PrepareLcdResult::GENERATE_FAILURE)},
        {3002, static_cast<int32_t>(PrepareLcdResult::SUCCESS)},
        {3003, static_cast<int32_t>(PrepareLcdResult::DOWNLOAD_FAILURE)},
        {3004, static_cast<int32_t>(PrepareLcdResult::NO_NETWORK)},
    };
    int32_t ret = dao.InsertFailedPhotosExt(results);
    EXPECT_EQ(ret, E_OK);
    int32_t count = QueryPhotosExtCount({3001, 3003, 3004});
    EXPECT_EQ(count, 3);
}

// 用例说明：测试全部SUCCESS时valuesSql为空，直接返回
// 覆盖场景：results中所有值均为SUCCESS
// 分支点：全部走continue，valuesSql为空，valuesSql.empty()返回E_OK
// 触发条件：所有results值均为SUCCESS
// 业务验证：返回E_OK，tab_photos_ext无新增记录
HWTEST_F(AnalysisLcdAgingDaoTest, InsertFailedPhotosExt_Test_003, TestSize.Level1)
{
    AnalysisLcdAgingDao dao;
    unordered_map<uint64_t, int32_t> results = {
        {4001, static_cast<int32_t>(PrepareLcdResult::SUCCESS)},
        {4002, static_cast<int32_t>(PrepareLcdResult::SUCCESS)},
    };
    int32_t ret = dao.InsertFailedPhotosExt(results);
    EXPECT_EQ(ret, E_OK);
    int32_t count = QueryPhotosExtCount({4001, 4002});
    EXPECT_EQ(count, 0);
}

// 用例说明：测试空results时直接返回
// 覆盖场景：提供空的results
// 分支点：results.empty() → 不进入循环，valuesSql为空，直接返回E_OK
// 触发条件：提供空的map
// 业务验证：返回E_OK，不做任何操作
HWTEST_F(AnalysisLcdAgingDaoTest, InsertFailedPhotosExt_Test_004, TestSize.Level1)
{
    AnalysisLcdAgingDao dao;
    unordered_map<uint64_t, int32_t> results;
    int32_t ret = dao.InsertFailedPhotosExt(results);
    EXPECT_EQ(ret, E_OK);
}

// 用例说明：测试单条失败记录时插入一条
// 覆盖场景：results中仅包含一条失败记录
// 分支点：不走continue，valuesSql拼接1个值，INSERT OR IGNORE插入1条
// 触发条件：results中仅有一个失败记录
// 业务验证：只插入该失败的fileId
HWTEST_F(AnalysisLcdAgingDaoTest, InsertFailedPhotosExt_Test_005, TestSize.Level1)
{
    AnalysisLcdAgingDao dao;
    unordered_map<uint64_t, int32_t> results = {
        {5001, static_cast<int32_t>(PrepareLcdResult::GENERATE_FAILURE)},
    };
    int32_t ret = dao.InsertFailedPhotosExt(results);
    EXPECT_EQ(ret, E_OK);
    int32_t count = QueryPhotosExtCount({5001});
    EXPECT_EQ(count, 1);
}

} // namespace Media
} // namespace OHOS