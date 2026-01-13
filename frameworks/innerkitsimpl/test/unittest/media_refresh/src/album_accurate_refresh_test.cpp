/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define MLOG_TAG "AlbumAccurateRefreshTest"

#include "album_accurate_refresh_test.h"

#include <chrono>
#include <thread>

#include "media_file_utils.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_unittest_utils.h"
#include "userfile_manager_types.h"
#include "values_bucket.h"
#include "uri.h"
#include "medialibrary_rdb_transaction.h"

#include "media_log.h"
#define protected public
#define private public
#include "album_accurate_refresh.h"
#undef protected
#undef private

#include "album_change_info.h"
#include "result_set_utils.h"
#include "abs_rdb_predicates.h"
#include "accurate_common_data.h"
#include "accurate_debug_log.h"
#include "accurate_refresh_test_util.h"

namespace OHOS {
namespace Media {

using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace AccurateRefresh;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;

static constexpr int32_t SLEEP_FIVE_SECONDS = 5;
static constexpr int32_t NUMBER_TWO = 2;
static constexpr int32_t NUMBER_ONE = 1;

namespace {


const string insertSql = "INSERT INTO " + PhotoAlbumColumns::TABLE + "( \
            album_id, album_type, album_subtype, count, image_count, video_count, cover_uri, hidden_count, \
            hidden_cover, cover_date_time, hidden_cover_date_time, dirty) \
            VALUES (" +
            to_string(FAVORITE_ALBUM_INFO.albumId_) + " , " +
            to_string(PhotoAlbumType::SYSTEM) + " , " +
            to_string(PhotoAlbumSubType::FAVORITE) + " , " +
            to_string(FAVORITE_ALBUM_INFO.count_) + " , " +
            to_string(FAVORITE_ALBUM_INFO.imageCount_) + " , " +
            to_string(FAVORITE_ALBUM_INFO.videoCount_) + " , '" +
            FAVORITE_ALBUM_INFO.coverUri_ + "' , " +
            to_string(FAVORITE_ALBUM_INFO.hiddenCount_) + " , '" +
            FAVORITE_ALBUM_INFO.hiddenCoverUri_ + "' , " +
            to_string(FAVORITE_ALBUM_INFO.coverDateTime_) + " , " +
            to_string(FAVORITE_ALBUM_INFO.hiddenCoverDateTime_) + " , " +
            to_string(FAVORITE_ALBUM_INFO.dirty_) + " )";
const string insertSqlWithArgs = "INSERT INTO " + PhotoAlbumColumns::TABLE + "( \
            album_id, album_type, album_subtype, count, image_count, video_count, cover_uri, hidden_count, \
            hidden_cover, cover_date_time, hidden_cover_date_time, dirty) \
            VALUES (" +
            to_string(FAVORITE_ALBUM_INFO.albumId_) + " , " +
            to_string(PhotoAlbumType::SYSTEM) + " , " +
            to_string(PhotoAlbumSubType::FAVORITE) + " , " +
            "?" + " , " +
            to_string(FAVORITE_ALBUM_INFO.imageCount_) + " , " +
            to_string(FAVORITE_ALBUM_INFO.videoCount_) + " , '" +
            FAVORITE_ALBUM_INFO.coverUri_ + "' , " +
            to_string(FAVORITE_ALBUM_INFO.hiddenCount_) + " , '" +
            FAVORITE_ALBUM_INFO.hiddenCoverUri_ + "' , " +
            to_string(FAVORITE_ALBUM_INFO.coverDateTime_) + " , " +
            to_string(FAVORITE_ALBUM_INFO.hiddenCoverDateTime_) + " , " +
            to_string(FAVORITE_ALBUM_INFO.dirty_) + " )";

vector<ValuesBucket> GetBatchInsertValues()
{
    vector<ValuesBucket> values;
    values.push_back(GetFavoriteInsertAlbum());
    values.push_back(GetTrashInsertAlbum());
    return values;
}

int32_t GetPhotoAlbumTotalCount()
{
    RdbPredicates queryPredicates(PhotoAlbumColumns::TABLE);

    int32_t count;
    auto resultSet = g_rdbStore->QueryByStep(queryPredicates, { "COUNT(1)" });
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int colIndex = 0;
        resultSet->GetInt(colIndex, count);
        ACCURATE_DEBUG("count: %{public}d", count);
    }
    return count;
}

void PrepareAlbumData()
{
    auto values = GetBatchInsertValues();
    int64_t insertNums = 0;
    auto ret = g_rdbStore->BatchInsert(insertNums, PhotoAlbumColumns::TABLE, values);
    ACCURATE_DEBUG("ret: %{public}d, insert values: %{public}" PRId64, ret, insertNums);
}

void PrepareHiddenData()
{
    auto values = GetHiddenInsertAlbum();
    int64_t insertNums = 0;
    auto ret = g_rdbStore->Insert(insertNums, PhotoAlbumColumns::TABLE, values);
    ACCURATE_DEBUG("ret: %{public}d, insert values: %{public}" PRId64, ret, insertNums);
}

bool CheckInsertChangeData(const map<int32_t, AlbumChangeData> albumChangeDatas, const AlbumChangeInfo &albumInfo)
{
    auto iter = albumChangeDatas.find(albumInfo.albumId_);
    if (iter == albumChangeDatas.end()) {
        MEDIA_ERR_LOG("no album ID");
        return false;
    }

    auto dataManagerChangeData = iter->second;
    return CheckAlbumChangeData(dataManagerChangeData, RDB_OPERATION_ADD, AlbumChangeInfo(), albumInfo);
}

bool CheckInsertResult(const AlbumAccurateRefresh &albumRefresh, const AlbumChangeInfo &albumInfo)
{
    auto dataManagerPtr = albumRefresh.dataManager_;
    auto &albumChangeDatas = dataManagerPtr.changeDatas_;
    if (albumChangeDatas.size() != 1) {
        MEDIA_ERR_LOG("data size error");
        return false;
    }
    if (!CheckInsertChangeData(albumChangeDatas, albumInfo)) {
        MEDIA_ERR_LOG("changeData wrong");
        return false;
    }
    return true;
}

bool CheckInsertNotify(const AlbumAccurateRefresh &albumRefresh, const AlbumChangeInfo &albumInfo)
{
    auto notifyInfos = albumRefresh.notifyExe_.notifyInfos_;
    if (notifyInfos.size() != NUMBER_ONE && notifyInfos.size() != NUMBER_TWO) {
        MEDIA_ERR_LOG("notify size error.");
        return false;
    }
    
    auto iter = notifyInfos.begin();
    if (iter->first != Notification::AlbumRefreshOperation::ALBUM_OPERATION_ADD) {
        MEDIA_ERR_LOG("operation type error.");
        return false;
    }

    auto albumChangeDatas = iter->second;
    if (albumChangeDatas.size() != 1) {
        MEDIA_ERR_LOG("albumChangeDatas size error.");
        return false;
    }

    return CheckAlbumChangeData(albumChangeDatas[0], RDB_OPERATION_ADD, AlbumChangeInfo(), albumInfo);
}

bool CheckBatchInsertResult(const AlbumAccurateRefresh &albumRefresh, const AlbumChangeInfo &favoriteAlbumInfo,
    const AlbumChangeInfo &trashAlbumInfo)
{
    auto dataManagerPtr = albumRefresh.dataManager_;
    auto &albumChangeDatas = dataManagerPtr.changeDatas_;
    if (albumChangeDatas.size() != NUMBER_TWO) {
        MEDIA_ERR_LOG("data size error");
        return false;
    }
    
    if (!CheckInsertChangeData(albumChangeDatas, favoriteAlbumInfo)) {
        MEDIA_ERR_LOG("favorite album changeData wrong");
        return false;
    }

    if (!CheckInsertChangeData(albumChangeDatas, trashAlbumInfo)) {
        MEDIA_ERR_LOG("trash album changeData wrong");
        return false;
    }
    return true;
}

bool CheckUpdateResult(const AlbumAccurateRefresh &albumRefresh, const AlbumChangeInfo &infoBefore,
    const AlbumChangeInfo &infoAfter)
{
    auto dataManagerPtr = albumRefresh.dataManager_;
    auto &albumChangeDatas = dataManagerPtr.changeDatas_;
    if (albumChangeDatas.size() != 1) {
        MEDIA_ERR_LOG("data size error");
        return false;
    }
    auto iter = albumChangeDatas.begin();
    return CheckAlbumChangeData(iter->second, RDB_OPERATION_UPDATE, infoBefore, infoAfter);
}

bool CheckDeleteResult(const AlbumAccurateRefresh &albumRefresh, RdbOperation operation,
    const AlbumChangeInfo &infoBefore, const AlbumChangeInfo &infoAfter)
{
    auto dataManagerPtr = albumRefresh.dataManager_;
    auto &albumChangeDatas = dataManagerPtr.changeDatas_;
    if (albumChangeDatas.size() != 1) {
        MEDIA_ERR_LOG("data size error");
        return false;
    }
    auto iter = albumChangeDatas.begin();
    return CheckAlbumChangeData(iter->second, operation, infoBefore, infoAfter, operation == RDB_OPERATION_REMOVE);
}

bool CheckBatchInsertNotify(const AlbumAccurateRefresh &albumRefresh, const AlbumChangeInfo &favoriteAlbumInfo,
    const AlbumChangeInfo &trashAlbumInfo)
{
    auto notifyInfos = albumRefresh.notifyExe_.notifyInfos_;
    if (notifyInfos.size() != NUMBER_ONE && notifyInfos.size() != NUMBER_TWO) {
        MEDIA_ERR_LOG("notify size error.");
        return false;
    }
    
    auto iter = notifyInfos.begin();
    if (iter->first != Notification::AlbumRefreshOperation::ALBUM_OPERATION_ADD) {
        MEDIA_ERR_LOG("operation type error.");
        return false;
    }

    auto albumChangeDatas = iter->second;
    if (albumChangeDatas.size() != NUMBER_TWO) {
        MEDIA_ERR_LOG("albumChangeDatas size error.");
        return false;
    }

    for (auto const &changeData : albumChangeDatas) {
        if (changeData.infoAfterChange_.albumId_ == favoriteAlbumInfo.albumId_) {
            if (!CheckAlbumChangeData(changeData, RDB_OPERATION_ADD, AlbumChangeInfo(), favoriteAlbumInfo)) {
                MEDIA_ERR_LOG("favorite album info error.");
                return false;
            }
        } else if (changeData.infoAfterChange_.albumId_ == trashAlbumInfo.albumId_) {
            if (!CheckAlbumChangeData(changeData, RDB_OPERATION_ADD, AlbumChangeInfo(), trashAlbumInfo)) {
                MEDIA_ERR_LOG("trash album info error.");
                return false;
            }
        } else {
            MEDIA_ERR_LOG("no album Id.");
            return false;
        }
    }
    return true;
}

bool CheckUpdateNotify(const AlbumAccurateRefresh &albumRefresh, Notification::AlbumRefreshOperation operation,
    const AlbumChangeInfo &infoBefore, const AlbumChangeInfo &infoAfter)
{
    auto notifyInfos = albumRefresh.notifyExe_.notifyInfos_;
    if (notifyInfos.size() != 1) {
        MEDIA_ERR_LOG("notify size error.");
        return false;
    }
    
    auto iter = notifyInfos.begin();
    if (iter->first != operation) {
        MEDIA_ERR_LOG("operation type error.");
        return false;
    }

    auto albumChangeDatas = iter->second;
    if (albumChangeDatas.size() != 1) {
        MEDIA_ERR_LOG("albumChangeDatas size error.");
        return false;
    }

    return CheckAlbumChangeData(albumChangeDatas[0], RDB_OPERATION_UPDATE, infoBefore, infoAfter);
}

bool CheckDeleteNotify(const AlbumAccurateRefresh &albumRefresh, Notification::AlbumRefreshOperation operation,
    const AlbumChangeInfo &infoBefore, const AlbumChangeInfo &infoAfter)
{
    auto notifyInfos = albumRefresh.notifyExe_.notifyInfos_;
    if (notifyInfos.size() != 1) {
        MEDIA_ERR_LOG("notify size error.");
        return false;
    }
    
    auto iter = notifyInfos.begin();
    if (iter->first != operation) {
        MEDIA_ERR_LOG("operation type error.");
        return false;
    }

    auto albumChangeDatas = iter->second;
    if (albumChangeDatas.size() != 1) {
        MEDIA_ERR_LOG("albumChangeDatas size error.");
        return false;
    }

    return CheckAlbumChangeData(albumChangeDatas[0], RDB_OPERATION_UNDEFINED, infoBefore, infoAfter, true);
}

void SetTables()
{
    // 创建Album表
    vector<string> createTableSqlList = {
        PhotoAlbumColumns::CREATE_TABLE
    };
    for (auto &createTableSql : createTableSqlList) {
        if (g_rdbStore == nullptr) {
            MEDIA_ERR_LOG("can not get g_rdbstore");
            return;
        }
        int32_t ret = g_rdbStore->ExecuteSql(createTableSql);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("Execute sql %{private}s failed", createTableSql.c_str());
            return;
        }
        MEDIA_INFO_LOG("Execute sql %{private}s success", createTableSql.c_str());
    }
    MEDIA_INFO_LOG("SetTables");
}

void CleanTestTables()
{
    vector<string> dropTableList = {
        PhotoAlbumColumns::TABLE,
    };
    for (auto &dropTable : dropTableList) {
        string dropSql = "DROP TABLE " + dropTable + ";";
        int32_t ret = g_rdbStore->ExecuteSql(dropSql);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("Drop %{public}s table failed", dropTable.c_str());
            return;
        }
        MEDIA_INFO_LOG("Drop %{public}s table success", dropTable.c_str());
    }
}

int32_t GetAlbumCount(PhotoAlbumSubType subType)
{
    return AccurateRefresh::GetAlbumCount(subType, g_rdbStore);
}

int32_t GetAlbumDirtyType(PhotoAlbumSubType subType)
{
    return AccurateRefresh::GetAlbumDirtyType(subType, g_rdbStore);
}

AlbumChangeInfo GetAlbumInfo(PhotoAlbumSubType subType)
{
    return AccurateRefresh::GetAlbumInfo(subType, g_rdbStore);
}

void ClearAndRestart()
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    system("rm -rf /storage/cloud/files/*");
    system("rm -rf /storage/cloud/files/.thumbs");
    system("rm -rf /storage/cloud/files/.editData");
    system("rm -rf /storage/cloud/files/.cache");
    for (const auto &dir : TEST_ROOT_DIRS) {
        string ROOT_PATH = "/storage/cloud/100/files/";
        bool ret = MediaFileUtils::CreateDirectory(ROOT_PATH + dir + "/");
        CHECK_AND_PRINT_LOG(ret, "make %{public}s dir failed, ret=%{public}d", dir.c_str(), ret);
    }
    CleanTestTables();
    SetTables();
}

} // namespace

void AlbumAccurateRefreshTest::SetUpTestCase()
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    SetTables();
}

void AlbumAccurateRefreshTest::SetUp()
{
    ClearAndRestart();
}

void AlbumAccurateRefreshTest::TearDownTestCase()
{
    CleanTestTables();
    g_rdbStore = nullptr;
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    this_thread::sleep_for(chrono::seconds(SLEEP_FIVE_SECONDS));
    MEDIA_INFO_LOG("Clean is finish");
}

void AlbumAccurateRefreshTest::TearDown()
{
    MEDIA_INFO_LOG("TearDown start");
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.NotEqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(0));
    
    int deleteRows = 0;
    g_rdbStore->Delete(deleteRows, predicates);
    MEDIA_INFO_LOG("TearDown end");
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Init_001, TestSize.Level2)
{
    std::shared_ptr<TransactionOperations> trans =
        make_shared<TransactionOperations>("AlbumAccurateRefreshTest_Init_001");
    AlbumAccurateRefresh albumRefresh(trans);
    EXPECT_TRUE(albumRefresh.trans_ != nullptr);
    EXPECT_TRUE(albumRefresh.dataManager_.trans_ != nullptr);
}

// 测试用例初始化运行时，创建PhotoAlbum表，里面包含系统相册、用户相册、来源相册内容
// Init查询PhotoAlbum表中的数据，进行判断
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Insert_cmd_002, TestSize.Level2)
{
    Uri uri("");
    MediaLibraryCommand cmd(uri);
    cmd.SetTableName(PhotoAlbumColumns::TABLE);
    ValuesBucket values;
    cmd.SetValueBucket(GetFavoriteInsertAlbum());
    AlbumAccurateRefresh albumRefresh;
    int64_t changeRow = 0;
    auto ret = albumRefresh.Insert(cmd, changeRow);

    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRow == 1);

    // 数据库执行结果
    AlbumChangeInfo albumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    EXPECT_TRUE(IsEqualAlbumInfo(albumInfo, FAVORITE_ALBUM_INFO));
    // 操作前后数据结果
    EXPECT_TRUE(CheckInsertResult(albumRefresh, albumInfo));
    
    // 通知结果
    albumRefresh.Notify();
    EXPECT_TRUE(CheckInsertNotify(albumRefresh, albumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Insert_Trans_cmd_003, TestSize.Level2)
{
    Uri uri("");
    MediaLibraryCommand cmd(uri);
    cmd.SetTableName(PhotoAlbumColumns::TABLE);
    cmd.SetValueBucket(GetTrashInsertAlbum());

    std::shared_ptr<TransactionOperations> trans =
        make_shared<TransactionOperations>("AlbumAccurateRefreshTest_Insert_Trans_cmd_003");
    AlbumAccurateRefresh albumRefresh(trans);
    int64_t changeRow = 0;
    std::function<int(void)> transFunc = [&]()->int {
        albumRefresh.Insert(cmd, changeRow);
        return ACCURATE_REFRESH_RET_OK;
    };
    // trans查询
    auto ret = trans->RetryTrans(transFunc);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    
    // 数据库执行结果
    AlbumChangeInfo albumInfo = GetAlbumInfo(PhotoAlbumSubType::TRASH);
    MEDIA_INFO_LOG("albulmInfo: %{public}s", albumInfo.ToString().c_str());
    EXPECT_TRUE(IsEqualAlbumInfo(albumInfo, TRASH_ALBUM_INFO));

    // 操作前后数据结果
    EXPECT_TRUE(CheckInsertResult(albumRefresh, albumInfo));

    // 通知结果
    albumRefresh.Notify();
    EXPECT_TRUE(CheckInsertNotify(albumRefresh, albumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Insert_004, TestSize.Level2)
{
    ValuesBucket values = GetFavoriteInsertAlbum();
    AlbumAccurateRefresh albumRefresh;
    int64_t changeRow = 0;
    auto ret = albumRefresh.Insert(changeRow, PhotoAlbumColumns::TABLE, values);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    
    // 数据库执行结果
    AlbumChangeInfo albumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    EXPECT_TRUE(IsEqualAlbumInfo(albumInfo, FAVORITE_ALBUM_INFO));
    // 操作前后数据结果
    EXPECT_TRUE(CheckInsertResult(albumRefresh, albumInfo));
    
    // 通知结果
    albumRefresh.Notify();
    EXPECT_TRUE(CheckInsertNotify(albumRefresh, albumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Insert_Trans_005, TestSize.Level2)
{
    std::shared_ptr<TransactionOperations> trans =
        make_shared<TransactionOperations>("AlbumAccurateRefreshTest_Insert_Trans_005");
    AlbumAccurateRefresh albumRefresh(trans);
    int64_t changeRow = 0;
    ValuesBucket values = GetFavoriteInsertAlbum();
    std::function<int(void)> transFunc = [&]()->int {
        auto ret = albumRefresh.Insert(changeRow, PhotoAlbumColumns::TABLE, values);
        return ret;
    };
    // trans查询
    auto ret = trans->RetryTrans(transFunc);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    // 数据库执行结果
    AlbumChangeInfo favoriteAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    EXPECT_TRUE(IsEqualAlbumInfo(favoriteAlbumInfo, FAVORITE_ALBUM_INFO));
    // 操作前后数据结果
    EXPECT_TRUE(CheckInsertResult(albumRefresh, favoriteAlbumInfo));
    // 通知结果
    albumRefresh.Notify();
    EXPECT_TRUE(CheckInsertNotify(albumRefresh, favoriteAlbumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_BatchInsert_cmd_006, TestSize.Level2)
{
    Uri uri("");
    MediaLibraryCommand cmd(uri);
    cmd.SetTableName(PhotoAlbumColumns::TABLE);
    int64_t insertNums = 0;
    AlbumAccurateRefresh albumRefresh;
    auto values = GetBatchInsertValues();
    auto ret = albumRefresh.BatchInsert(cmd, insertNums, values);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(insertNums == values.size());

    // 数据库执行结果
    AlbumChangeInfo favoriteAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    EXPECT_TRUE(IsEqualAlbumInfo(favoriteAlbumInfo, FAVORITE_ALBUM_INFO));
    AlbumChangeInfo trashAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::TRASH);
    EXPECT_TRUE(IsEqualAlbumInfo(trashAlbumInfo, TRASH_ALBUM_INFO));
    // 操作前后数据结果
    EXPECT_TRUE(CheckBatchInsertResult(albumRefresh, favoriteAlbumInfo, trashAlbumInfo));
    
    // 通知结果
    albumRefresh.Notify();

    EXPECT_TRUE(CheckBatchInsertNotify(albumRefresh, favoriteAlbumInfo, trashAlbumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_BatchInsert_Trans_cmd_007, TestSize.Level2)
{
    Uri uri("");
    MediaLibraryCommand cmd(uri);
    cmd.SetTableName(PhotoAlbumColumns::TABLE);
    int64_t insertNums = 0;
    std::shared_ptr<TransactionOperations> trans =
        make_shared<TransactionOperations>("AlbumAccurateRefreshTest_BatchInsert_Trans_cmd_007");
    AlbumAccurateRefresh albumRefresh(trans);
    auto values = GetBatchInsertValues();
    std::function<int(void)> transFunc = [&]()->int {
        auto ret = albumRefresh.BatchInsert(cmd, insertNums, values);
        return ret;
    };
    // trans查询
    auto ret = trans->RetryTrans(transFunc);
    
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(insertNums == values.size());
    // 数据库执行结果
    AlbumChangeInfo favoriteAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    EXPECT_TRUE(IsEqualAlbumInfo(favoriteAlbumInfo, FAVORITE_ALBUM_INFO));
    AlbumChangeInfo trashAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::TRASH);
    EXPECT_TRUE(IsEqualAlbumInfo(trashAlbumInfo, TRASH_ALBUM_INFO));
    // 操作前后数据结果
    EXPECT_TRUE(CheckBatchInsertResult(albumRefresh, favoriteAlbumInfo, trashAlbumInfo));
    
    // 通知结果
    albumRefresh.Notify();

    EXPECT_TRUE(CheckBatchInsertNotify(albumRefresh, favoriteAlbumInfo, trashAlbumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_BatchInsert_008, TestSize.Level2)
{
    int64_t insertNums = 0;
    int rdbError = 0;
    AlbumAccurateRefresh albumRefresh;
    auto values = GetBatchInsertValues();
    auto ret = albumRefresh.BatchInsert(insertNums, PhotoAlbumColumns::TABLE, values, rdbError);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(insertNums == values.size());
    // 数据库执行结果
    AlbumChangeInfo favoriteAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    EXPECT_TRUE(IsEqualAlbumInfo(favoriteAlbumInfo, FAVORITE_ALBUM_INFO));
    AlbumChangeInfo trashAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::TRASH);
    EXPECT_TRUE(IsEqualAlbumInfo(trashAlbumInfo, TRASH_ALBUM_INFO));
    // 操作前后数据结果
    EXPECT_TRUE(CheckBatchInsertResult(albumRefresh, favoriteAlbumInfo, trashAlbumInfo));
    
    // 通知结果
    albumRefresh.Notify();

    EXPECT_TRUE(CheckBatchInsertNotify(albumRefresh, favoriteAlbumInfo, trashAlbumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_BatchInsert_Trans_009, TestSize.Level2)
{
    int64_t insertNums = 0;
    int rdbError = 0;
    std::shared_ptr<TransactionOperations> trans =
        make_shared<TransactionOperations>("AlbumAccurateRefreshTest_BatchInsert_Trans_009");
    AlbumAccurateRefresh albumRefresh(trans);
    auto values = GetBatchInsertValues();
    std::function<int(void)> transFunc = [&]()->int {
        auto ret = albumRefresh.BatchInsert(insertNums, PhotoAlbumColumns::TABLE, values, rdbError);
        return ret;
    };
    auto ret = trans->RetryTrans(transFunc);
    
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(insertNums == values.size());
    // 数据库执行结果
    AlbumChangeInfo favoriteAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    EXPECT_TRUE(IsEqualAlbumInfo(favoriteAlbumInfo, FAVORITE_ALBUM_INFO));
    AlbumChangeInfo trashAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::TRASH);
    EXPECT_TRUE(IsEqualAlbumInfo(trashAlbumInfo, TRASH_ALBUM_INFO));
    // 操作前后数据结果
    EXPECT_TRUE(CheckBatchInsertResult(albumRefresh, favoriteAlbumInfo, trashAlbumInfo));
    
    // 通知结果
    albumRefresh.Notify();

    EXPECT_TRUE(CheckBatchInsertNotify(albumRefresh, favoriteAlbumInfo, trashAlbumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Update_cmd_010, TestSize.Level2)
{
    PrepareAlbumData();
    Uri uri("");
    MediaLibraryCommand cmd(uri);
    cmd.SetTableName(PhotoAlbumColumns::TABLE);
    auto predicates = cmd.GetAbsRdbPredicates();
    predicates->SetWhereClause(PhotoAlbumColumns::ALBUM_SUBTYPE + " = ?");
    predicates->SetWhereArgs({ to_string(PhotoAlbumSubType::FAVORITE) });
    ValuesBucket value;
    auto newCount = FAVORITE_ALBUM_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_COUNT, newCount);
    auto imageCount = FAVORITE_ALBUM_IMAGE_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, imageCount);
    cmd.SetValueBucket(value);

    AlbumAccurateRefresh albumRefreshUpdate;
    int32_t changeRows = 0;
    auto ret = albumRefreshUpdate.Update(cmd, changeRows);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);
    // 数据库执行结果
    AlbumChangeInfo favoriteAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    AlbumChangeInfo updateAlbumInfo = FAVORITE_ALBUM_INFO;
    updateAlbumInfo.count_ = newCount;
    updateAlbumInfo.imageCount_ = imageCount;
    EXPECT_TRUE(IsEqualAlbumInfo(favoriteAlbumInfo, updateAlbumInfo));
    EXPECT_TRUE(GetAlbumCount(PhotoAlbumSubType::FAVORITE) == newCount);

    // 操作前后数据结果
    EXPECT_TRUE(CheckUpdateResult(albumRefreshUpdate, FAVORITE_ALBUM_INFO, favoriteAlbumInfo));

    // 通知结果
    albumRefreshUpdate.Notify();

    EXPECT_TRUE(CheckUpdateNotify(albumRefreshUpdate, Notification::ALBUM_OPERATION_UPDATE, FAVORITE_ALBUM_INFO,
        favoriteAlbumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Update_Trans_cmd_011, TestSize.Level2)
{
    PrepareAlbumData();
    Uri uri("");
    MediaLibraryCommand cmd(uri);
    cmd.SetTableName(PhotoAlbumColumns::TABLE);
    auto predicates = cmd.GetAbsRdbPredicates();
    predicates->SetWhereClause(PhotoAlbumColumns::ALBUM_SUBTYPE + " = ?");
    predicates->SetWhereArgs({ to_string(PhotoAlbumSubType::FAVORITE) });
    ValuesBucket value;
    auto newCount = FAVORITE_ALBUM_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_COUNT, newCount);
    auto imageCount = FAVORITE_ALBUM_IMAGE_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, imageCount);
    cmd.SetValueBucket(value);

    std::shared_ptr<TransactionOperations> trans =
        make_shared<TransactionOperations>("AlbumAccurateRefreshTest_Update_Trans_cmd_011");
    AlbumAccurateRefresh albumRefreshUpdate(trans);
    int32_t changeRows = 0;
    std::function<int(void)> transFunc = [&]()->int {
        auto ret = albumRefreshUpdate.Update(cmd, changeRows);
        return ret;
    };
    auto ret = trans->RetryTrans(transFunc);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);
    // 数据库执行结果
    AlbumChangeInfo favoriteAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    AlbumChangeInfo updateAlbumInfo = FAVORITE_ALBUM_INFO;
    updateAlbumInfo.count_ = newCount;
    updateAlbumInfo.imageCount_ = imageCount;
    EXPECT_TRUE(IsEqualAlbumInfo(favoriteAlbumInfo, updateAlbumInfo));
    EXPECT_TRUE(GetAlbumCount(PhotoAlbumSubType::FAVORITE) == newCount);

    // 操作前后数据结果
    EXPECT_TRUE(CheckUpdateResult(albumRefreshUpdate, FAVORITE_ALBUM_INFO, favoriteAlbumInfo));

    // 通知结果
    albumRefreshUpdate.Notify();

    EXPECT_TRUE(CheckUpdateNotify(albumRefreshUpdate, Notification::ALBUM_OPERATION_UPDATE, FAVORITE_ALBUM_INFO,
        favoriteAlbumInfo));
}


HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Update_012, TestSize.Level2)
{
    PrepareAlbumData();
    ValuesBucket value;
    auto newCount = TRASH_ALBUM_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_COUNT, newCount);
    auto imageCount = TRASH_ALBUM_IMAGE_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, imageCount);
    string whereClause = PhotoAlbumColumns::ALBUM_SUBTYPE + " = ?";
    vector<string> args = { to_string(PhotoAlbumSubType::TRASH) };

    AlbumAccurateRefresh albumRefreshUpdate;
    int32_t changeRows = 0;
    auto ret = albumRefreshUpdate.Update(changeRows, PhotoAlbumColumns::TABLE, value, whereClause, args);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);
    // 数据库执行结果
    AlbumChangeInfo trashAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::TRASH);
    AlbumChangeInfo updateAlbumInfo = TRASH_ALBUM_INFO;
    updateAlbumInfo.count_ = newCount;
    updateAlbumInfo.imageCount_ = imageCount;
    EXPECT_TRUE(IsEqualAlbumInfo(trashAlbumInfo, updateAlbumInfo));
    EXPECT_TRUE(GetAlbumCount(PhotoAlbumSubType::TRASH) == newCount);

    // 操作前后数据结果
    EXPECT_TRUE(CheckUpdateResult(albumRefreshUpdate, TRASH_ALBUM_INFO, trashAlbumInfo));

    // 通知结果
    albumRefreshUpdate.Notify();

    EXPECT_TRUE(CheckUpdateNotify(albumRefreshUpdate, Notification::ALBUM_OPERATION_UPDATE_TRASH, TRASH_ALBUM_INFO,
        trashAlbumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Update_Trans_013, TestSize.Level2)
{
    PrepareAlbumData();
    ValuesBucket value;
    auto newCount = TRASH_ALBUM_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_COUNT, newCount);
    auto imageCount = TRASH_ALBUM_IMAGE_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, imageCount);
    string whereClause = PhotoAlbumColumns::ALBUM_SUBTYPE + " = ?";
    vector<string> args = { to_string(PhotoAlbumSubType::TRASH) };

    std::shared_ptr<TransactionOperations> trans =
        make_shared<TransactionOperations>("AlbumAccurateRefreshTest_Update_Trans_cmd_011");
    AlbumAccurateRefresh albumRefreshUpdate(trans);
    int32_t changeRows = 0;
    std::function<int(void)> transFunc = [&]()->int {
        auto ret = albumRefreshUpdate.Update(changeRows, PhotoAlbumColumns::TABLE, value, whereClause, args);
        return ret;
    };

    auto ret = trans->RetryTrans(transFunc);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);
    // 数据库执行结果
    AlbumChangeInfo trashAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::TRASH);
    AlbumChangeInfo updateAlbumInfo = TRASH_ALBUM_INFO;
    updateAlbumInfo.count_ = newCount;
    updateAlbumInfo.imageCount_ = imageCount;
    EXPECT_TRUE(IsEqualAlbumInfo(trashAlbumInfo, updateAlbumInfo));
    EXPECT_TRUE(GetAlbumCount(PhotoAlbumSubType::TRASH) == newCount);

    // 操作前后数据结果
    EXPECT_TRUE(CheckUpdateResult(albumRefreshUpdate, TRASH_ALBUM_INFO, trashAlbumInfo));

    // 通知结果
    albumRefreshUpdate.Notify();

    EXPECT_TRUE(CheckUpdateNotify(albumRefreshUpdate, Notification::ALBUM_OPERATION_UPDATE_TRASH, TRASH_ALBUM_INFO,
        trashAlbumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Update_014, TestSize.Level2)
{
    PrepareHiddenData();
    ValuesBucket value;
    auto newCount = HIDDEN_ALBUM_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_COUNT, newCount);
    auto imageCount = HIDDEN_ALBUM_IMAGE_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, imageCount);
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::HIDDEN));

    AlbumAccurateRefresh albumRefreshUpdate;
    int32_t changeRows = 0;
    auto ret = albumRefreshUpdate.Update(changeRows, value, predicates);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);
    // 数据库执行结果
    AlbumChangeInfo hiddenAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::HIDDEN);
    AlbumChangeInfo updateAlbumInfo = HIDDEN_ALBUM_INFO;
    updateAlbumInfo.count_ = newCount;
    updateAlbumInfo.imageCount_ = imageCount;
    EXPECT_TRUE(IsEqualAlbumInfo(hiddenAlbumInfo, updateAlbumInfo));
    EXPECT_TRUE(GetAlbumCount(PhotoAlbumSubType::HIDDEN) == newCount);

    // 操作前后数据结果
    EXPECT_TRUE(CheckUpdateResult(albumRefreshUpdate, HIDDEN_ALBUM_INFO, hiddenAlbumInfo));

    // 通知结果
    albumRefreshUpdate.Notify();

    EXPECT_TRUE(CheckUpdateNotify(albumRefreshUpdate, Notification::ALBUM_OPERATION_UPDATE_HIDDEN,
        HIDDEN_ALBUM_INFO, hiddenAlbumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Update_Trans_015, TestSize.Level2)
{
    PrepareHiddenData();
    ValuesBucket value;
    auto newCount = HIDDEN_ALBUM_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_COUNT, newCount);
    auto imageCount = HIDDEN_ALBUM_IMAGE_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, imageCount);
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::HIDDEN));

    std::shared_ptr<TransactionOperations> trans =
        make_shared<TransactionOperations>("AlbumAccurateRefreshTest_Update_Trans_015");
    AlbumAccurateRefresh albumRefreshUpdate(trans);
    int32_t changeRows = 0;
    std::function<int(void)> transFunc = [&]()->int {
        auto ret = albumRefreshUpdate.Update(changeRows, value, predicates);
        return ret;
    };

    auto ret = trans->RetryTrans(transFunc);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);

    // 数据库执行结果
    AlbumChangeInfo hiddenAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::HIDDEN);
    AlbumChangeInfo updateAlbumInfo = HIDDEN_ALBUM_INFO;
    updateAlbumInfo.count_ = newCount;
    updateAlbumInfo.imageCount_ = imageCount;
    EXPECT_TRUE(IsEqualAlbumInfo(hiddenAlbumInfo, updateAlbumInfo));
    EXPECT_TRUE(GetAlbumCount(PhotoAlbumSubType::HIDDEN) == newCount);

    // 操作前后数据结果
    EXPECT_TRUE(CheckUpdateResult(albumRefreshUpdate, HIDDEN_ALBUM_INFO, hiddenAlbumInfo));

    // 通知结果
    albumRefreshUpdate.Notify();

    EXPECT_TRUE(CheckUpdateNotify(albumRefreshUpdate, Notification::ALBUM_OPERATION_UPDATE_HIDDEN, HIDDEN_ALBUM_INFO,
        hiddenAlbumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Delete_cmd_016, TestSize.Level2)
{
    PrepareAlbumData();
    Uri uri("");
    MediaLibraryCommand cmd(uri);
    cmd.SetTableName(PhotoAlbumColumns::TABLE);
    auto predicates = cmd.GetAbsRdbPredicates();
    predicates->SetWhereClause(PhotoAlbumColumns::ALBUM_SUBTYPE + " = ?");
    predicates->SetWhereArgs({ to_string(PhotoAlbumSubType::FAVORITE) });

    AlbumAccurateRefresh albumRefreshDel;
    int32_t changeRows = 0;
    auto ret = albumRefreshDel.LogicalDeleteReplaceByUpdate(cmd, changeRows);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);

    // 数据库执行结果
    AlbumChangeInfo favoriteAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    auto updateAlbumInfo = FAVORITE_ALBUM_INFO;
    updateAlbumInfo.dirty_ = static_cast<int32_t>(DirtyType::TYPE_DELETED);
    EXPECT_TRUE(IsEqualAlbumInfo(favoriteAlbumInfo, updateAlbumInfo));

    // 操作前后数据结果
    EXPECT_TRUE(CheckDeleteResult(albumRefreshDel, RDB_OPERATION_REMOVE, FAVORITE_ALBUM_INFO, AlbumChangeInfo()));

    // 通知结果
    albumRefreshDel.Notify();
    EXPECT_TRUE(CheckDeleteNotify(albumRefreshDel, Notification::ALBUM_OPERATION_REMOVE, FAVORITE_ALBUM_INFO,
        AlbumChangeInfo()));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Delete_Trans_cmd_017, TestSize.Level2)
{
    ACCURATE_DEBUG("");
    PrepareAlbumData();
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 2);
    EXPECT_TRUE(GetAlbumDirtyType(PhotoAlbumSubType::FAVORITE) == static_cast<int32_t>(DirtyType::TYPE_NEW));
    Uri uri("");
    MediaLibraryCommand cmd(uri);
    cmd.SetTableName(PhotoAlbumColumns::TABLE);
    auto predicates = cmd.GetAbsRdbPredicates();
    predicates->SetWhereClause(PhotoAlbumColumns::ALBUM_SUBTYPE + " = ?");
    predicates->SetWhereArgs({ to_string(PhotoAlbumSubType::FAVORITE) });

    std::shared_ptr<TransactionOperations> trans =
        make_shared<TransactionOperations>("AlbumAccurateRefreshTest_Delete_Trans_cmd_017");
    AlbumAccurateRefresh albumRefreshDel(trans);
    int32_t changeRows = 0;
    function<int32_t()> transFunc = [&] () -> int32_t {
        auto ret = albumRefreshDel.LogicalDeleteReplaceByUpdate(cmd, changeRows);
        return ret;
    };

    auto ret = trans->RetryTrans(transFunc);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);
    // 数据库执行结果
    AlbumChangeInfo favoriteAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    auto updateAlbumInfo = FAVORITE_ALBUM_INFO;
    updateAlbumInfo.dirty_ = static_cast<int32_t>(DirtyType::TYPE_DELETED);
    EXPECT_TRUE(IsEqualAlbumInfo(favoriteAlbumInfo, updateAlbumInfo));

    // 操作前后数据结果
    EXPECT_TRUE(CheckDeleteResult(albumRefreshDel, RDB_OPERATION_REMOVE, FAVORITE_ALBUM_INFO, AlbumChangeInfo()));

    // 通知结果
    albumRefreshDel.Notify();
    EXPECT_TRUE(CheckDeleteNotify(albumRefreshDel, Notification::ALBUM_OPERATION_REMOVE, FAVORITE_ALBUM_INFO,
        AlbumChangeInfo()));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Delete_018, TestSize.Level2)
{
    PrepareAlbumData();
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 2);
    EXPECT_TRUE(GetAlbumDirtyType(PhotoAlbumSubType::FAVORITE) == static_cast<int32_t>(DirtyType::TYPE_NEW));
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::FAVORITE));

    AlbumAccurateRefresh albumRefreshDel;
    int32_t changeRows = 0;
    auto ret = albumRefreshDel.LogicalDeleteReplaceByUpdate(predicates, changeRows);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 2);

    // 数据库执行结果
    AlbumChangeInfo favoriteAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    auto updateAlbumInfo = FAVORITE_ALBUM_INFO;
    updateAlbumInfo.dirty_ = static_cast<int32_t>(DirtyType::TYPE_DELETED);
    EXPECT_TRUE(IsEqualAlbumInfo(favoriteAlbumInfo, updateAlbumInfo));

    // 操作前后数据结果
    EXPECT_TRUE(CheckDeleteResult(albumRefreshDel, RDB_OPERATION_REMOVE, FAVORITE_ALBUM_INFO, AlbumChangeInfo()));

    // 通知结果
    albumRefreshDel.Notify();
    EXPECT_TRUE(CheckDeleteNotify(albumRefreshDel, Notification::ALBUM_OPERATION_REMOVE, FAVORITE_ALBUM_INFO,
        AlbumChangeInfo()));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Delete_Trans_019, TestSize.Level2)
{
    PrepareAlbumData();
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 2);
    EXPECT_TRUE(GetAlbumDirtyType(PhotoAlbumSubType::FAVORITE) == static_cast<int32_t>(DirtyType::TYPE_NEW));
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::FAVORITE));

    std::shared_ptr<TransactionOperations> trans =
        make_shared<TransactionOperations>("AlbumAccurateRefreshTest_Delete_Trans_019");
    AlbumAccurateRefresh albumRefreshDel(trans);
    int32_t changeRows = 0;
    function<int32_t()> transFunc = [&] () -> int32_t {
        auto ret = albumRefreshDel.LogicalDeleteReplaceByUpdate(predicates, changeRows);
        return ret;
    };

    auto ret = trans->RetryTrans(transFunc);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 2);

    // 数据库执行结果
    AlbumChangeInfo favoriteAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    auto updateAlbumInfo = FAVORITE_ALBUM_INFO;
    updateAlbumInfo.dirty_ = static_cast<int32_t>(DirtyType::TYPE_DELETED);
    EXPECT_TRUE(IsEqualAlbumInfo(favoriteAlbumInfo, updateAlbumInfo));

    // 操作前后数据结果
    EXPECT_TRUE(CheckDeleteResult(albumRefreshDel, RDB_OPERATION_REMOVE, FAVORITE_ALBUM_INFO, AlbumChangeInfo()));

    // 通知结果
    albumRefreshDel.Notify();
    EXPECT_TRUE(CheckDeleteNotify(albumRefreshDel, Notification::ALBUM_OPERATION_REMOVE, FAVORITE_ALBUM_INFO,
        AlbumChangeInfo()));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Delete_020, TestSize.Level2)
{
    PrepareAlbumData();
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 2);
    
    AlbumAccurateRefresh albumRefreshDel;
    int32_t changeRows = 0;
    string whereClause = PhotoAlbumColumns::ALBUM_SUBTYPE + " = ?";
    vector<string> args = { to_string(PhotoAlbumSubType::FAVORITE) };
    auto ret = albumRefreshDel.Delete(changeRows, PhotoAlbumColumns::TABLE, whereClause, args);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 1);

     // 操作前后数据结果
    EXPECT_TRUE(CheckDeleteResult(albumRefreshDel, RDB_OPERATION_REMOVE, FAVORITE_ALBUM_INFO, AlbumChangeInfo()));

    // 通知结果
    albumRefreshDel.Notify();
    EXPECT_TRUE(CheckDeleteNotify(albumRefreshDel, Notification::ALBUM_OPERATION_REMOVE, FAVORITE_ALBUM_INFO,
        AlbumChangeInfo()));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Delete_Trans_021, TestSize.Level2)
{
    PrepareAlbumData();
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 2);

    std::shared_ptr<TransactionOperations> trans =
        make_shared<TransactionOperations>("AlbumAccurateRefreshTest_Delete_Trans_021");
    AlbumAccurateRefresh albumRefreshDel(trans);
    int32_t changeRows = 0;
    string whereClause = PhotoAlbumColumns::ALBUM_SUBTYPE + " = ?";
    vector<string> args = { to_string(PhotoAlbumSubType::FAVORITE) };
    function<int32_t()> transFunc = [&] () -> int32_t {
        auto ret = albumRefreshDel.Delete(changeRows, PhotoAlbumColumns::TABLE, whereClause, args);
        return ret;
    };

    auto ret = trans->RetryTrans(transFunc);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 1);
    // 操作前后数据结果
    EXPECT_TRUE(CheckDeleteResult(albumRefreshDel, RDB_OPERATION_REMOVE, FAVORITE_ALBUM_INFO, AlbumChangeInfo()));

    // 通知结果
    albumRefreshDel.Notify();
    EXPECT_TRUE(CheckDeleteNotify(albumRefreshDel, Notification::ALBUM_OPERATION_REMOVE, FAVORITE_ALBUM_INFO,
        AlbumChangeInfo()));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Delete_022, TestSize.Level2)
{
    PrepareAlbumData();
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 2);

    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::FAVORITE));
    
    AlbumAccurateRefresh albumRefreshDel;
    int32_t changeRows = 0;
    auto ret = albumRefreshDel.Delete(changeRows, predicates);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 1);

    // 操作前后数据结果
    EXPECT_TRUE(CheckDeleteResult(albumRefreshDel, RDB_OPERATION_REMOVE, FAVORITE_ALBUM_INFO, AlbumChangeInfo()));

    // 通知结果
    albumRefreshDel.Notify();
    EXPECT_TRUE(CheckDeleteNotify(albumRefreshDel, Notification::ALBUM_OPERATION_REMOVE, FAVORITE_ALBUM_INFO,
        AlbumChangeInfo()));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Delete_Trans_023, TestSize.Level2)
{
    PrepareAlbumData();
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 2);

    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::FAVORITE));

    std::shared_ptr<TransactionOperations> trans =
        make_shared<TransactionOperations>("AlbumAccurateRefreshTest_Delete_Trans_023");
    AlbumAccurateRefresh albumRefreshDel(trans);
    int32_t changeRows = 0;
    string whereClause = PhotoAlbumColumns::ALBUM_SUBTYPE + " = ?";
    vector<string> args = { to_string(PhotoAlbumSubType::FAVORITE) };
    function<int32_t()> transFunc = [&] () -> int32_t {
        auto ret = albumRefreshDel.Delete(changeRows, predicates);
        return ret;
    };

    auto ret = trans->RetryTrans(transFunc);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 1);
    // 操作前后数据结果
    EXPECT_TRUE(CheckDeleteResult(albumRefreshDel, RDB_OPERATION_REMOVE, FAVORITE_ALBUM_INFO, AlbumChangeInfo()));
    // 通知结果
    albumRefreshDel.Notify();
    EXPECT_TRUE(CheckDeleteNotify(albumRefreshDel, Notification::ALBUM_OPERATION_REMOVE, FAVORITE_ALBUM_INFO,
        AlbumChangeInfo()));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Exe_024, TestSize.Level2)
{
    AlbumAccurateRefresh albumRefreshExe;
    albumRefreshExe.Init();
    auto ret = albumRefreshExe.ExecuteSql(insertSql, RdbOperation::RDB_OPERATION_ADD);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 1);
    EXPECT_TRUE(GetAlbumCount(PhotoAlbumSubType::FAVORITE) == FAVORITE_ALBUM_INFO.count_);

    // 数据库执行结果
    AlbumChangeInfo albumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    EXPECT_TRUE(IsEqualAlbumInfo(albumInfo, FAVORITE_ALBUM_INFO));
    // 操作前后数据结果
    EXPECT_TRUE(CheckInsertResult(albumRefreshExe, albumInfo));
    
    // 通知结果
    albumRefreshExe.Notify();
    EXPECT_TRUE(CheckInsertNotify(albumRefreshExe, albumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Exe_Trans_025, TestSize.Level2)
{
    std::shared_ptr<TransactionOperations> trans =
        make_shared<TransactionOperations>("AlbumAccurateRefreshTest_Exe_Trans_024");
    AlbumAccurateRefresh albumRefreshExe(trans);
    albumRefreshExe.Init();
    function<int32_t()> transFunc = [&]() -> int32_t {
        return albumRefreshExe.ExecuteSql(insertSql, RdbOperation::RDB_OPERATION_ADD);
    };
    auto ret = trans->RetryTrans(transFunc);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 1);
    EXPECT_TRUE(GetAlbumCount(PhotoAlbumSubType::FAVORITE) == FAVORITE_ALBUM_INFO.count_);

    // 数据库执行结果
    AlbumChangeInfo albumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    EXPECT_TRUE(IsEqualAlbumInfo(albumInfo, FAVORITE_ALBUM_INFO));
    // 操作前后数据结果
    EXPECT_TRUE(CheckInsertResult(albumRefreshExe, albumInfo));
    
    // 通知结果
    albumRefreshExe.Notify();
    EXPECT_TRUE(CheckInsertNotify(albumRefreshExe, albumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Exe_026, TestSize.Level2)
{
    vector<ValueObject> args = {to_string(FAVORITE_ALBUM_INFO.count_)};

    AlbumAccurateRefresh albumRefreshExe;
    albumRefreshExe.Init();
    auto changedRowId =
        albumRefreshExe.ExecuteForLastInsertedRowId(insertSqlWithArgs, args, RdbOperation::RDB_OPERATION_ADD);
    ACCURATE_DEBUG("changedRowId: %{public}d", changedRowId);
    EXPECT_TRUE(changedRowId > 0);
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 1);
    EXPECT_TRUE(GetAlbumCount(PhotoAlbumSubType::FAVORITE) == FAVORITE_ALBUM_INFO.count_);

    // 数据库执行结果
    AlbumChangeInfo albumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    EXPECT_TRUE(IsEqualAlbumInfo(albumInfo, FAVORITE_ALBUM_INFO));
    // 操作前后数据结果
    EXPECT_TRUE(CheckInsertResult(albumRefreshExe, albumInfo));
    
    // 通知结果
    albumRefreshExe.Notify();
    EXPECT_TRUE(CheckInsertNotify(albumRefreshExe, albumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Exe_027, TestSize.Level2)
{
    vector<ValueObject> args = {to_string(FAVORITE_ALBUM_INFO.count_)};

    std::shared_ptr<TransactionOperations> trans =
        make_shared<TransactionOperations>("AlbumAccurateRefreshTest_Exe_Trans_024");
    AlbumAccurateRefresh albumRefreshExe(trans);
    albumRefreshExe.Init();
    int32_t changedRowId = 0;
    function<int32_t()> transFunc = [&]() -> int32_t {
        changedRowId =
            albumRefreshExe.ExecuteForLastInsertedRowId(insertSqlWithArgs, args, RdbOperation::RDB_OPERATION_ADD);
        if (changedRowId != E_HAS_DB_ERROR) {
            return ACCURATE_REFRESH_RET_OK;
        } else {
            return E_HAS_DB_ERROR;
        }
    };
    auto ret = trans->RetryTrans(transFunc);
    
    ACCURATE_DEBUG("changedRowId: %{public}d", changedRowId);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changedRowId > 0);
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 1);
    EXPECT_TRUE(GetAlbumCount(PhotoAlbumSubType::FAVORITE) == FAVORITE_ALBUM_INFO.count_);
    // 数据库执行结果
    AlbumChangeInfo albumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    EXPECT_TRUE(IsEqualAlbumInfo(albumInfo, FAVORITE_ALBUM_INFO));
    // 操作前后数据结果
    EXPECT_TRUE(CheckInsertResult(albumRefreshExe, albumInfo));
    
    // 通知结果
    albumRefreshExe.Notify();
    EXPECT_TRUE(CheckInsertNotify(albumRefreshExe, albumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Exe_028, TestSize.Level2)
{
    vector<ValueObject> args = {to_string(FAVORITE_ALBUM_INFO.count_)};

    AlbumAccurateRefresh albumRefreshExe;
    albumRefreshExe.Init();
    auto ret = albumRefreshExe.ExecuteSql(insertSqlWithArgs, args, RdbOperation::RDB_OPERATION_ADD);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 1);
    EXPECT_TRUE(GetAlbumCount(PhotoAlbumSubType::FAVORITE) == FAVORITE_ALBUM_INFO.count_);
    // 数据库执行结果
    AlbumChangeInfo albumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    EXPECT_TRUE(IsEqualAlbumInfo(albumInfo, FAVORITE_ALBUM_INFO));
    // 操作前后数据结果
    EXPECT_TRUE(CheckInsertResult(albumRefreshExe, albumInfo));
    
    // 通知结果
    albumRefreshExe.Notify();
    EXPECT_TRUE(CheckInsertNotify(albumRefreshExe, albumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Exe_029, TestSize.Level2)
{
    vector<ValueObject> args = {to_string(FAVORITE_ALBUM_INFO.count_)};

    std::shared_ptr<TransactionOperations> trans =
        make_shared<TransactionOperations>("AlbumAccurateRefreshTest_Exe_Trans_024");
    AlbumAccurateRefresh albumRefreshExe(trans);
    albumRefreshExe.Init();

    function<int32_t()> transFunc = [&]() -> int32_t {
        auto ret = albumRefreshExe.ExecuteSql(insertSqlWithArgs, args, RdbOperation::RDB_OPERATION_ADD);
        return ret;
    };
    auto ret = trans->RetryTrans(transFunc);

    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 1);
    EXPECT_TRUE(GetAlbumCount(PhotoAlbumSubType::FAVORITE) == FAVORITE_ALBUM_INFO.count_);
    // 数据库执行结果
    AlbumChangeInfo albumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    EXPECT_TRUE(IsEqualAlbumInfo(albumInfo, FAVORITE_ALBUM_INFO));
    // 操作前后数据结果
    EXPECT_TRUE(CheckInsertResult(albumRefreshExe, albumInfo));
    
    // 通知结果
    albumRefreshExe.Notify();
    EXPECT_TRUE(CheckInsertNotify(albumRefreshExe, albumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Exe_030, TestSize.Level2)
{
    vector<ValueObject> args = {to_string(FAVORITE_ALBUM_INFO.count_)};

    AlbumAccurateRefresh albumRefreshExe;
    albumRefreshExe.Init();
    int64_t outValue = 0;
    auto ret =
        albumRefreshExe.ExecuteForChangedRowCount(outValue, insertSqlWithArgs, args, RdbOperation::RDB_OPERATION_ADD);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(outValue == 1);
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 1);
    EXPECT_TRUE(GetAlbumCount(PhotoAlbumSubType::FAVORITE) == FAVORITE_ALBUM_INFO.count_);
    // 数据库执行结果
    AlbumChangeInfo albumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    EXPECT_TRUE(IsEqualAlbumInfo(albumInfo, FAVORITE_ALBUM_INFO));
    // 操作前后数据结果
    EXPECT_TRUE(CheckInsertResult(albumRefreshExe, albumInfo));
    
    // 通知结果
    albumRefreshExe.Notify();
    EXPECT_TRUE(CheckInsertNotify(albumRefreshExe, albumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Exe_031, TestSize.Level2)
{
    vector<ValueObject> args = {to_string(FAVORITE_ALBUM_INFO.count_)};

    std::shared_ptr<TransactionOperations> trans =
        make_shared<TransactionOperations>("AlbumAccurateRefreshTest_Exe_Trans_024");
    AlbumAccurateRefresh albumRefreshExe(trans);
    albumRefreshExe.Init();
    int64_t outValue = 0;
    function<int32_t()> transFunc = [&]() -> int32_t {
        auto ret =albumRefreshExe.ExecuteForChangedRowCount(outValue, insertSqlWithArgs, args,
            RdbOperation::RDB_OPERATION_ADD);
        return ret;
    };
    auto ret = trans->RetryTrans(transFunc);

    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(outValue == 1);
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 1);
    EXPECT_TRUE(GetAlbumCount(PhotoAlbumSubType::FAVORITE) == FAVORITE_ALBUM_INFO.count_);
    // 数据库执行结果
    AlbumChangeInfo albumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    EXPECT_TRUE(IsEqualAlbumInfo(albumInfo, FAVORITE_ALBUM_INFO));
    // 操作前后数据结果
    EXPECT_TRUE(CheckInsertResult(albumRefreshExe, albumInfo));
    
    // 通知结果
    albumRefreshExe.Notify();
    EXPECT_TRUE(CheckInsertNotify(albumRefreshExe, albumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Update_Exceed_032, TestSize.Level2)
{
    PrepareHiddenData();
    ValuesBucket value;
    auto newCount = HIDDEN_ALBUM_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_COUNT, newCount);
    auto imageCount = HIDDEN_ALBUM_IMAGE_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, imageCount);
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::HIDDEN));

    AlbumAccurateRefresh albumRefreshUpdate;
    int32_t changeRows = 0;
    auto ret = albumRefreshUpdate.Update(changeRows, value, predicates);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);
    auto &changeDatasMap = albumRefreshUpdate.dataManager_.changeDatas_;
    AlbumChangeData changeData;
    // 总共1000条
    for (int i = 0; i < 999; ++i) {
        changeDatasMap.insert_or_assign(1000000 + i, changeData);
    }

    ValuesBucket newValue;
    newCount = HIDDEN_ALBUM_COUNT;
    newValue.PutInt(PhotoAlbumColumns::ALBUM_COUNT, newCount);
    imageCount = HIDDEN_ALBUM_IMAGE_COUNT;
    newValue.PutInt(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, imageCount);
    ret = albumRefreshUpdate.Update(changeRows, value, predicates);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);
    // 总共1000条
    EXPECT_TRUE(albumRefreshUpdate.dataManager_.CheckIsExceed());
    EXPECT_TRUE(albumRefreshUpdate.dataManager_.changeDatas_.empty());
    EXPECT_TRUE(albumRefreshUpdate.Notify() == ACCURATE_REFRESH_RET_OK);
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Update_Exceed_033, TestSize.Level2)
{
    PrepareHiddenData();
    ValuesBucket value;
    auto newCount = HIDDEN_ALBUM_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_COUNT, newCount);
    auto imageCount = HIDDEN_ALBUM_IMAGE_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, imageCount);
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::HIDDEN));

    AlbumAccurateRefresh albumRefreshUpdate;
    int32_t changeRows = 0;
    auto ret = albumRefreshUpdate.Update(changeRows, value, predicates);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);
    auto &changeDatasMap = albumRefreshUpdate.dataManager_.changeDatas_;
    AlbumChangeData changeData;
    // 总共999条
    for (int i = 0; i < 998; ++i) {
        changeDatasMap.insert_or_assign(1000000 + i, changeData);
    }

    ValuesBucket newValue;
    newCount = HIDDEN_ALBUM_COUNT;
    newValue.PutInt(PhotoAlbumColumns::ALBUM_COUNT, newCount);
    imageCount = HIDDEN_ALBUM_IMAGE_COUNT;
    newValue.PutInt(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, imageCount);
    ret = albumRefreshUpdate.Update(changeRows, value, predicates);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);
    // 总共999条
    EXPECT_TRUE(!albumRefreshUpdate.dataManager_.CheckIsExceed());
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Init_034, TestSize.Level2)
{
    std::shared_ptr<TransactionOperations> trans =
        make_shared<TransactionOperations>("AlbumAccurateRefreshTest_Init_034");
    AlbumAccurateRefresh albumRefresh("AlbumAccurateRefreshTest_Init_034", trans);
    EXPECT_TRUE(albumRefresh.trans_ != nullptr);
    EXPECT_TRUE(albumRefresh.dataManager_.trans_ != nullptr);
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Insert_cmd_35, TestSize.Level2)
{
    Uri uri("");
    MediaLibraryCommand cmd(uri);
    cmd.SetTableName(PhotoAlbumColumns::TABLE);
    ValuesBucket values;
    cmd.SetValueBucket(GetPhotoAlbumInsertValue(FAVORITE_ALBUM_INFO_TOW));
    AlbumAccurateRefresh albumRefresh("AlbumAccurateRefreshTest_Insert_cmd_35");
    int64_t changeRow = 0;
    auto ret = albumRefresh.Insert(cmd, changeRow);

    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRow == 101);

    // 数据库执行结果
    AlbumChangeInfo albumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    EXPECT_TRUE(IsEqualAlbumInfo(albumInfo, FAVORITE_ALBUM_INFO_TOW));
    // 操作前后数据结果
    EXPECT_TRUE(CheckInsertResult(albumRefresh, albumInfo));
    
    // 通知结果
    albumRefresh.Notify();
    EXPECT_TRUE(CheckInsertNotify(albumRefresh, albumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Insert_Trans_cmd_036, TestSize.Level2)
{
    Uri uri("");
    MediaLibraryCommand cmd(uri);
    cmd.SetTableName(PhotoAlbumColumns::TABLE);
    cmd.SetValueBucket(GetPhotoAlbumInsertValue(TRASH_ALBUM_INFO_TOW));

    std::shared_ptr<TransactionOperations> trans =
        make_shared<TransactionOperations>("AlbumAccurateRefreshTest_Insert_Trans_cmd_036");
    AlbumAccurateRefresh albumRefresh("AlbumAccurateRefreshTest_Insert_Trans_cmd_036", trans);
    int64_t changeRow = 0;
    std::function<int(void)> transFunc = [&]()->int {
        albumRefresh.Insert(cmd, changeRow);
        return ACCURATE_REFRESH_RET_OK;
    };
    // trans查询
    auto ret = trans->RetryTrans(transFunc);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    
    // 数据库执行结果
    AlbumChangeInfo albumInfo = GetAlbumInfo(PhotoAlbumSubType::TRASH);
    MEDIA_INFO_LOG("albulmInfo: %{public}s", albumInfo.ToString().c_str());
    EXPECT_TRUE(IsEqualAlbumInfo(albumInfo, TRASH_ALBUM_INFO_TOW));

    // 操作前后数据结果
    EXPECT_TRUE(CheckInsertResult(albumRefresh, albumInfo));

    // 通知结果
    albumRefresh.Notify();
    EXPECT_TRUE(CheckInsertNotify(albumRefresh, albumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Insert_037, TestSize.Level2)
{
    ValuesBucket values = GetPhotoAlbumInsertValue(FAVORITE_ALBUM_INFO_TOW);
    AlbumAccurateRefresh albumRefresh("AlbumAccurateRefreshTest_Insert_037");
    int64_t changeRow = 0;
    auto ret = albumRefresh.Insert(changeRow, PhotoAlbumColumns::TABLE, values);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    
    // 数据库执行结果
    AlbumChangeInfo albumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    EXPECT_TRUE(IsEqualAlbumInfo(albumInfo, FAVORITE_ALBUM_INFO_TOW));
    // 操作前后数据结果
    EXPECT_TRUE(CheckInsertResult(albumRefresh, albumInfo));
    
    // 通知结果
    albumRefresh.Notify();
    EXPECT_TRUE(CheckInsertNotify(albumRefresh, albumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Insert_Trans_038, TestSize.Level2)
{
    std::shared_ptr<TransactionOperations> trans =
        make_shared<TransactionOperations>("AlbumAccurateRefreshTest_Insert_Trans_038");
    AlbumAccurateRefresh albumRefresh("AlbumAccurateRefreshTest_Insert_Trans_038", trans);
    int64_t changeRow = 0;
    ValuesBucket values = GetPhotoAlbumInsertValue(FAVORITE_ALBUM_INFO_TOW);
    std::function<int(void)> transFunc = [&]()->int {
        auto ret = albumRefresh.Insert(changeRow, PhotoAlbumColumns::TABLE, values);
        return ret;
    };
    // trans查询
    auto ret = trans->RetryTrans(transFunc);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    // 数据库执行结果
    AlbumChangeInfo favoriteAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    EXPECT_TRUE(IsEqualAlbumInfo(favoriteAlbumInfo, FAVORITE_ALBUM_INFO_TOW));
    // 操作前后数据结果
    EXPECT_TRUE(CheckInsertResult(albumRefresh, favoriteAlbumInfo));
    // 通知结果
    albumRefresh.Notify();
    EXPECT_TRUE(CheckInsertNotify(albumRefresh, favoriteAlbumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_BatchInsert_cmd_039, TestSize.Level2)
{
    Uri uri("");
    MediaLibraryCommand cmd(uri);
    cmd.SetTableName(PhotoAlbumColumns::TABLE);
    int64_t insertNums = 0;
    AlbumAccurateRefresh albumRefresh("AlbumAccurateRefreshTest_BatchInsert_cmd_039");
    vector<ValuesBucket> values;
    values.push_back(GetPhotoAlbumInsertValue(FAVORITE_ALBUM_INFO_TOW));
    values.push_back(GetPhotoAlbumInsertValue(TRASH_ALBUM_INFO_TOW));
    auto ret = albumRefresh.BatchInsert(cmd, insertNums, values);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(insertNums == values.size());

    // 数据库执行结果
    AlbumChangeInfo favoriteAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    EXPECT_TRUE(IsEqualAlbumInfo(favoriteAlbumInfo, FAVORITE_ALBUM_INFO_TOW));
    AlbumChangeInfo trashAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::TRASH);
    EXPECT_TRUE(IsEqualAlbumInfo(trashAlbumInfo, TRASH_ALBUM_INFO_TOW));
    // 操作前后数据结果
    EXPECT_TRUE(CheckBatchInsertResult(albumRefresh, favoriteAlbumInfo, trashAlbumInfo));
    
    // 通知结果
    albumRefresh.Notify();

    EXPECT_TRUE(CheckBatchInsertNotify(albumRefresh, favoriteAlbumInfo, trashAlbumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_BatchInsert_Trans_cmd_040, TestSize.Level2)
{
    Uri uri("");
    MediaLibraryCommand cmd(uri);
    cmd.SetTableName(PhotoAlbumColumns::TABLE);
    int64_t insertNums = 0;
    std::shared_ptr<TransactionOperations> trans =
        make_shared<TransactionOperations>("AlbumAccurateRefreshTest_BatchInsert_Trans_cmd_040");
    AlbumAccurateRefresh albumRefresh("AlbumAccurateRefreshTest_BatchInsert_Trans_cmd_040", trans);
    vector<ValuesBucket> values;
    values.push_back(GetPhotoAlbumInsertValue(FAVORITE_ALBUM_INFO_TOW));
    values.push_back(GetPhotoAlbumInsertValue(TRASH_ALBUM_INFO_TOW));
    std::function<int(void)> transFunc = [&]()->int {
        auto ret = albumRefresh.BatchInsert(cmd, insertNums, values);
        return ret;
    };
    // trans查询
    auto ret = trans->RetryTrans(transFunc);
    
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(insertNums == values.size());
    // 数据库执行结果
    AlbumChangeInfo favoriteAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    EXPECT_TRUE(IsEqualAlbumInfo(favoriteAlbumInfo, FAVORITE_ALBUM_INFO_TOW));
    AlbumChangeInfo trashAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::TRASH);
    EXPECT_TRUE(IsEqualAlbumInfo(trashAlbumInfo, TRASH_ALBUM_INFO_TOW));
    // 操作前后数据结果
    EXPECT_TRUE(CheckBatchInsertResult(albumRefresh, favoriteAlbumInfo, trashAlbumInfo));
    
    // 通知结果
    albumRefresh.Notify();

    EXPECT_TRUE(CheckBatchInsertNotify(albumRefresh, favoriteAlbumInfo, trashAlbumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_BatchInsert_041, TestSize.Level2)
{
    int64_t insertNums = 0;
    int rdbError = 0;
    AlbumAccurateRefresh albumRefresh("AlbumAccurateRefreshTest_BatchInsert_041");
    vector<ValuesBucket> values;
    values.push_back(GetPhotoAlbumInsertValue(FAVORITE_ALBUM_INFO_TOW));
    values.push_back(GetPhotoAlbumInsertValue(TRASH_ALBUM_INFO_TOW));
    auto ret = albumRefresh.BatchInsert(insertNums, PhotoAlbumColumns::TABLE, values, rdbError);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(insertNums == values.size());
    // 数据库执行结果
    AlbumChangeInfo favoriteAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    EXPECT_TRUE(IsEqualAlbumInfo(favoriteAlbumInfo, FAVORITE_ALBUM_INFO_TOW));
    AlbumChangeInfo trashAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::TRASH);
    EXPECT_TRUE(IsEqualAlbumInfo(trashAlbumInfo, TRASH_ALBUM_INFO_TOW));
    // 操作前后数据结果
    EXPECT_TRUE(CheckBatchInsertResult(albumRefresh, favoriteAlbumInfo, trashAlbumInfo));
    
    // 通知结果
    albumRefresh.Notify();

    EXPECT_TRUE(CheckBatchInsertNotify(albumRefresh, favoriteAlbumInfo, trashAlbumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_BatchInsert_Trans_042, TestSize.Level2)
{
    int64_t insertNums = 0;
    int rdbError = 0;
    std::shared_ptr<TransactionOperations> trans =
        make_shared<TransactionOperations>("AlbumAccurateRefreshTest_BatchInsert_Trans_042");
    AlbumAccurateRefresh albumRefresh("AlbumAccurateRefreshTest_BatchInsert_Trans_042", trans);
    vector<ValuesBucket> values;
    values.push_back(GetPhotoAlbumInsertValue(FAVORITE_ALBUM_INFO_TOW));
    values.push_back(GetPhotoAlbumInsertValue(TRASH_ALBUM_INFO_TOW));
    std::function<int(void)> transFunc = [&]()->int {
        auto ret = albumRefresh.BatchInsert(insertNums, PhotoAlbumColumns::TABLE, values, rdbError);
        return ret;
    };
    auto ret = trans->RetryTrans(transFunc);
    
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(insertNums == values.size());
    // 数据库执行结果
    AlbumChangeInfo favoriteAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    EXPECT_TRUE(IsEqualAlbumInfo(favoriteAlbumInfo, FAVORITE_ALBUM_INFO_TOW));
    AlbumChangeInfo trashAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::TRASH);
    EXPECT_TRUE(IsEqualAlbumInfo(trashAlbumInfo, TRASH_ALBUM_INFO_TOW));
    // 操作前后数据结果
    EXPECT_TRUE(CheckBatchInsertResult(albumRefresh, favoriteAlbumInfo, trashAlbumInfo));
    
    // 通知结果
    albumRefresh.Notify();

    EXPECT_TRUE(CheckBatchInsertNotify(albumRefresh, favoriteAlbumInfo, trashAlbumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Update_cmd_043, TestSize.Level2)
{
    PrepareAlbumData();
    Uri uri("");
    MediaLibraryCommand cmd(uri);
    cmd.SetTableName(PhotoAlbumColumns::TABLE);
    auto predicates = cmd.GetAbsRdbPredicates();
    predicates->SetWhereClause(PhotoAlbumColumns::ALBUM_SUBTYPE + " = ?");
    predicates->SetWhereArgs({ to_string(PhotoAlbumSubType::FAVORITE) });
    ValuesBucket value;
    auto newCount = FAVORITE_ALBUM_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_COUNT, newCount);
    auto imageCount = FAVORITE_ALBUM_IMAGE_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, imageCount);
    cmd.SetValueBucket(value);

    AlbumAccurateRefresh albumRefreshUpdate("AlbumAccurateRefreshTest_Update_cmd_043");
    int32_t changeRows = 0;
    auto ret = albumRefreshUpdate.Update(cmd, changeRows);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);
    // 数据库执行结果
    AlbumChangeInfo favoriteAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    AlbumChangeInfo updateAlbumInfo = FAVORITE_ALBUM_INFO;
    updateAlbumInfo.count_ = newCount;
    updateAlbumInfo.imageCount_ = imageCount;
    EXPECT_TRUE(IsEqualAlbumInfo(favoriteAlbumInfo, updateAlbumInfo));
    EXPECT_TRUE(GetAlbumCount(PhotoAlbumSubType::FAVORITE) == newCount);

    // 操作前后数据结果
    EXPECT_TRUE(CheckUpdateResult(albumRefreshUpdate, FAVORITE_ALBUM_INFO, favoriteAlbumInfo));

    // 通知结果
    albumRefreshUpdate.Notify();

    EXPECT_TRUE(CheckUpdateNotify(albumRefreshUpdate, Notification::ALBUM_OPERATION_UPDATE, FAVORITE_ALBUM_INFO,
        favoriteAlbumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Update_Trans_cmd_044, TestSize.Level2)
{
    PrepareAlbumData();
    Uri uri("");
    MediaLibraryCommand cmd(uri);
    cmd.SetTableName(PhotoAlbumColumns::TABLE);
    auto predicates = cmd.GetAbsRdbPredicates();
    predicates->SetWhereClause(PhotoAlbumColumns::ALBUM_SUBTYPE + " = ?");
    predicates->SetWhereArgs({ to_string(PhotoAlbumSubType::FAVORITE) });
    ValuesBucket value;
    auto newCount = FAVORITE_ALBUM_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_COUNT, newCount);
    auto imageCount = FAVORITE_ALBUM_IMAGE_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, imageCount);
    cmd.SetValueBucket(value);

    std::shared_ptr<TransactionOperations> trans =
        make_shared<TransactionOperations>("AlbumAccurateRefreshTest_Update_Trans_cmd_044");
    AlbumAccurateRefresh albumRefreshUpdate("AlbumAccurateRefreshTest_Update_Trans_cmd_044", trans);
    int32_t changeRows = 0;
    std::function<int(void)> transFunc = [&]()->int {
        auto ret = albumRefreshUpdate.Update(cmd, changeRows);
        return ret;
    };
    auto ret = trans->RetryTrans(transFunc);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);
    // 数据库执行结果
    AlbumChangeInfo favoriteAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    AlbumChangeInfo updateAlbumInfo = FAVORITE_ALBUM_INFO;
    updateAlbumInfo.count_ = newCount;
    updateAlbumInfo.imageCount_ = imageCount;
    EXPECT_TRUE(IsEqualAlbumInfo(favoriteAlbumInfo, updateAlbumInfo));
    EXPECT_TRUE(GetAlbumCount(PhotoAlbumSubType::FAVORITE) == newCount);

    // 操作前后数据结果
    EXPECT_TRUE(CheckUpdateResult(albumRefreshUpdate, FAVORITE_ALBUM_INFO, favoriteAlbumInfo));

    // 通知结果
    albumRefreshUpdate.Notify();
    EXPECT_TRUE(CheckUpdateNotify(albumRefreshUpdate, Notification::ALBUM_OPERATION_UPDATE, FAVORITE_ALBUM_INFO,
        favoriteAlbumInfo));
}


HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Update_045, TestSize.Level2)
{
    PrepareAlbumData();
    ValuesBucket value;
    auto newCount = TRASH_ALBUM_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_COUNT, newCount);
    auto imageCount = TRASH_ALBUM_IMAGE_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, imageCount);
    string whereClause = PhotoAlbumColumns::ALBUM_SUBTYPE + " = ?";
    vector<string> args = { to_string(PhotoAlbumSubType::TRASH) };

    AlbumAccurateRefresh albumRefreshUpdate("AlbumAccurateRefreshTest_Update_045");
    int32_t changeRows = 0;
    auto ret = albumRefreshUpdate.Update(changeRows, PhotoAlbumColumns::TABLE, value, whereClause, args);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);
    // 数据库执行结果
    AlbumChangeInfo trashAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::TRASH);
    AlbumChangeInfo updateAlbumInfo = TRASH_ALBUM_INFO;
    updateAlbumInfo.count_ = newCount;
    updateAlbumInfo.imageCount_ = imageCount;
    EXPECT_TRUE(IsEqualAlbumInfo(trashAlbumInfo, updateAlbumInfo));
    EXPECT_TRUE(GetAlbumCount(PhotoAlbumSubType::TRASH) == newCount);

    // 操作前后数据结果
    EXPECT_TRUE(CheckUpdateResult(albumRefreshUpdate, TRASH_ALBUM_INFO, trashAlbumInfo));

    // 通知结果
    albumRefreshUpdate.Notify();

    EXPECT_TRUE(CheckUpdateNotify(albumRefreshUpdate, Notification::ALBUM_OPERATION_UPDATE_TRASH, TRASH_ALBUM_INFO,
        trashAlbumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Update_Trans_046, TestSize.Level2)
{
    PrepareAlbumData();
    ValuesBucket value;
    auto newCount = TRASH_ALBUM_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_COUNT, newCount);
    auto imageCount = TRASH_ALBUM_IMAGE_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, imageCount);
    string whereClause = PhotoAlbumColumns::ALBUM_SUBTYPE + " = ?";
    vector<string> args = { to_string(PhotoAlbumSubType::TRASH) };

    std::shared_ptr<TransactionOperations> trans =
        make_shared<TransactionOperations>("AlbumAccurateRefreshTest_Update_Trans_046");
    AlbumAccurateRefresh albumRefreshUpdate("AlbumAccurateRefreshTest_Update_Trans_046", trans);
    int32_t changeRows = 0;
    std::function<int(void)> transFunc = [&]()->int {
        auto ret = albumRefreshUpdate.Update(changeRows, PhotoAlbumColumns::TABLE, value, whereClause, args);
        return ret;
    };

    auto ret = trans->RetryTrans(transFunc);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);
    // 数据库执行结果
    AlbumChangeInfo trashAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::TRASH);
    AlbumChangeInfo updateAlbumInfo = TRASH_ALBUM_INFO;
    updateAlbumInfo.count_ = newCount;
    updateAlbumInfo.imageCount_ = imageCount;
    EXPECT_TRUE(IsEqualAlbumInfo(trashAlbumInfo, updateAlbumInfo));
    EXPECT_TRUE(GetAlbumCount(PhotoAlbumSubType::TRASH) == newCount);

    // 操作前后数据结果
    EXPECT_TRUE(CheckUpdateResult(albumRefreshUpdate, TRASH_ALBUM_INFO, trashAlbumInfo));

    // 通知结果
    albumRefreshUpdate.Notify();
    EXPECT_TRUE(CheckUpdateNotify(albumRefreshUpdate, Notification::ALBUM_OPERATION_UPDATE_TRASH, TRASH_ALBUM_INFO,
        trashAlbumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Update_047, TestSize.Level2)
{
    PrepareHiddenData();
    ValuesBucket value;
    auto newCount = HIDDEN_ALBUM_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_COUNT, newCount);
    auto imageCount = HIDDEN_ALBUM_IMAGE_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, imageCount);
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::HIDDEN));

    AlbumAccurateRefresh albumRefreshUpdate("AlbumAccurateRefreshTest_Update_047");
    int32_t changeRows = 0;
    auto ret = albumRefreshUpdate.Update(changeRows, value, predicates);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);
    // 数据库执行结果
    AlbumChangeInfo hiddenAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::HIDDEN);
    AlbumChangeInfo updateAlbumInfo = HIDDEN_ALBUM_INFO;
    updateAlbumInfo.count_ = newCount;
    updateAlbumInfo.imageCount_ = imageCount;
    EXPECT_TRUE(IsEqualAlbumInfo(hiddenAlbumInfo, updateAlbumInfo));
    EXPECT_TRUE(GetAlbumCount(PhotoAlbumSubType::HIDDEN) == newCount);

    // 操作前后数据结果
    EXPECT_TRUE(CheckUpdateResult(albumRefreshUpdate, HIDDEN_ALBUM_INFO, hiddenAlbumInfo));

    // 通知结果
    albumRefreshUpdate.Notify();

    EXPECT_TRUE(CheckUpdateNotify(albumRefreshUpdate, Notification::ALBUM_OPERATION_UPDATE_HIDDEN,
        HIDDEN_ALBUM_INFO, hiddenAlbumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Update_Trans_048, TestSize.Level2)
{
    PrepareHiddenData();
    ValuesBucket value;
    auto newCount = HIDDEN_ALBUM_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_COUNT, newCount);
    auto imageCount = HIDDEN_ALBUM_IMAGE_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, imageCount);
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::HIDDEN));

    std::shared_ptr<TransactionOperations> trans =
        make_shared<TransactionOperations>("AlbumAccurateRefreshTest_Update_Trans_048");
    AlbumAccurateRefresh albumRefreshUpdate("AlbumAccurateRefreshTest_Update_Trans_048", trans);
    int32_t changeRows = 0;
    std::function<int(void)> transFunc = [&]()->int {
        auto ret = albumRefreshUpdate.Update(changeRows, value, predicates);
        return ret;
    };

    auto ret = trans->RetryTrans(transFunc);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);

    // 数据库执行结果
    AlbumChangeInfo hiddenAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::HIDDEN);
    AlbumChangeInfo updateAlbumInfo = HIDDEN_ALBUM_INFO;
    updateAlbumInfo.count_ = newCount;
    updateAlbumInfo.imageCount_ = imageCount;
    EXPECT_TRUE(IsEqualAlbumInfo(hiddenAlbumInfo, updateAlbumInfo));
    EXPECT_TRUE(GetAlbumCount(PhotoAlbumSubType::HIDDEN) == newCount);

    // 操作前后数据结果
    EXPECT_TRUE(CheckUpdateResult(albumRefreshUpdate, HIDDEN_ALBUM_INFO, hiddenAlbumInfo));

    // 通知结果
    albumRefreshUpdate.Notify();

    EXPECT_TRUE(CheckUpdateNotify(albumRefreshUpdate, Notification::ALBUM_OPERATION_UPDATE_HIDDEN, HIDDEN_ALBUM_INFO,
        hiddenAlbumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Delete_cmd_049, TestSize.Level2)
{
    PrepareAlbumData();
    Uri uri("");
    MediaLibraryCommand cmd(uri);
    cmd.SetTableName(PhotoAlbumColumns::TABLE);
    auto predicates = cmd.GetAbsRdbPredicates();
    predicates->SetWhereClause(PhotoAlbumColumns::ALBUM_SUBTYPE + " = ?");
    predicates->SetWhereArgs({ to_string(PhotoAlbumSubType::FAVORITE) });

    AlbumAccurateRefresh albumRefreshDel("AlbumAccurateRefreshTest_Delete_cmd_049");
    int32_t changeRows = 0;
    auto ret = albumRefreshDel.LogicalDeleteReplaceByUpdate(cmd, changeRows);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);

    // 数据库执行结果
    AlbumChangeInfo favoriteAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    auto updateAlbumInfo = FAVORITE_ALBUM_INFO;
    updateAlbumInfo.dirty_ = static_cast<int32_t>(DirtyType::TYPE_DELETED);
    EXPECT_TRUE(IsEqualAlbumInfo(favoriteAlbumInfo, updateAlbumInfo));

    // 操作前后数据结果
    EXPECT_TRUE(CheckDeleteResult(albumRefreshDel, RDB_OPERATION_REMOVE, FAVORITE_ALBUM_INFO, AlbumChangeInfo()));

    // 通知结果
    albumRefreshDel.Notify();
    EXPECT_TRUE(CheckDeleteNotify(albumRefreshDel, Notification::ALBUM_OPERATION_REMOVE, FAVORITE_ALBUM_INFO,
        AlbumChangeInfo()));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Delete_Trans_cmd_050, TestSize.Level2)
{
    ACCURATE_DEBUG("");
    PrepareAlbumData();
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 2);
    EXPECT_TRUE(GetAlbumDirtyType(PhotoAlbumSubType::FAVORITE) == static_cast<int32_t>(DirtyType::TYPE_NEW));
    Uri uri("");
    MediaLibraryCommand cmd(uri);
    cmd.SetTableName(PhotoAlbumColumns::TABLE);
    auto predicates = cmd.GetAbsRdbPredicates();
    predicates->SetWhereClause(PhotoAlbumColumns::ALBUM_SUBTYPE + " = ?");
    predicates->SetWhereArgs({ to_string(PhotoAlbumSubType::FAVORITE) });

    std::shared_ptr<TransactionOperations> trans =
        make_shared<TransactionOperations>("AlbumAccurateRefreshTest_Delete_Trans_cmd_050");
    AlbumAccurateRefresh albumRefreshDel("AlbumAccurateRefreshTest_Delete_Trans_cmd_050", trans);
    int32_t changeRows = 0;
    function<int32_t()> transFunc = [&] () -> int32_t {
        auto ret = albumRefreshDel.LogicalDeleteReplaceByUpdate(cmd, changeRows);
        return ret;
    };

    auto ret = trans->RetryTrans(transFunc);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);
    // 数据库执行结果
    AlbumChangeInfo favoriteAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    auto updateAlbumInfo = FAVORITE_ALBUM_INFO;
    updateAlbumInfo.dirty_ = static_cast<int32_t>(DirtyType::TYPE_DELETED);
    EXPECT_TRUE(IsEqualAlbumInfo(favoriteAlbumInfo, updateAlbumInfo));

    // 操作前后数据结果
    EXPECT_TRUE(CheckDeleteResult(albumRefreshDel, RDB_OPERATION_REMOVE, FAVORITE_ALBUM_INFO, AlbumChangeInfo()));

    // 通知结果
    albumRefreshDel.Notify();
    EXPECT_TRUE(CheckDeleteNotify(albumRefreshDel, Notification::ALBUM_OPERATION_REMOVE, FAVORITE_ALBUM_INFO,
        AlbumChangeInfo()));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Delete_051, TestSize.Level2)
{
    PrepareAlbumData();
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 2);
    EXPECT_TRUE(GetAlbumDirtyType(PhotoAlbumSubType::FAVORITE) == static_cast<int32_t>(DirtyType::TYPE_NEW));
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::FAVORITE));

    AlbumAccurateRefresh albumRefreshDel("AlbumAccurateRefreshTest_Delete_051");
    int32_t changeRows = 0;
    auto ret = albumRefreshDel.LogicalDeleteReplaceByUpdate(predicates, changeRows);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 2);

    // 数据库执行结果
    AlbumChangeInfo favoriteAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    auto updateAlbumInfo = FAVORITE_ALBUM_INFO;
    updateAlbumInfo.dirty_ = static_cast<int32_t>(DirtyType::TYPE_DELETED);
    EXPECT_TRUE(IsEqualAlbumInfo(favoriteAlbumInfo, updateAlbumInfo));

    // 操作前后数据结果
    EXPECT_TRUE(CheckDeleteResult(albumRefreshDel, RDB_OPERATION_REMOVE, FAVORITE_ALBUM_INFO, AlbumChangeInfo()));

    // 通知结果
    albumRefreshDel.Notify();
    EXPECT_TRUE(CheckDeleteNotify(albumRefreshDel, Notification::ALBUM_OPERATION_REMOVE, FAVORITE_ALBUM_INFO,
        AlbumChangeInfo()));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Delete_Trans_052, TestSize.Level2)
{
    PrepareAlbumData();
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 2);
    EXPECT_TRUE(GetAlbumDirtyType(PhotoAlbumSubType::FAVORITE) == static_cast<int32_t>(DirtyType::TYPE_NEW));
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::FAVORITE));

    std::shared_ptr<TransactionOperations> trans =
        make_shared<TransactionOperations>("AlbumAccurateRefreshTest_Delete_Trans_052");
    AlbumAccurateRefresh albumRefreshDel("AlbumAccurateRefreshTest_Delete_Trans_052", trans);
    int32_t changeRows = 0;
    function<int32_t()> transFunc = [&] () -> int32_t {
        auto ret = albumRefreshDel.LogicalDeleteReplaceByUpdate(predicates, changeRows);
        return ret;
    };

    auto ret = trans->RetryTrans(transFunc);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 2);

    // 数据库执行结果
    AlbumChangeInfo favoriteAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    auto updateAlbumInfo = FAVORITE_ALBUM_INFO;
    updateAlbumInfo.dirty_ = static_cast<int32_t>(DirtyType::TYPE_DELETED);
    EXPECT_TRUE(IsEqualAlbumInfo(favoriteAlbumInfo, updateAlbumInfo));

    // 操作前后数据结果
    EXPECT_TRUE(CheckDeleteResult(albumRefreshDel, RDB_OPERATION_REMOVE, FAVORITE_ALBUM_INFO, AlbumChangeInfo()));

    // 通知结果
    albumRefreshDel.Notify();
    EXPECT_TRUE(CheckDeleteNotify(albumRefreshDel, Notification::ALBUM_OPERATION_REMOVE, FAVORITE_ALBUM_INFO,
        AlbumChangeInfo()));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Delete_053, TestSize.Level2)
{
    PrepareAlbumData();
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 2);
    
    AlbumAccurateRefresh albumRefreshDel("AlbumAccurateRefreshTest_Delete_053");
    int32_t changeRows = 0;
    string whereClause = PhotoAlbumColumns::ALBUM_SUBTYPE + " = ?";
    vector<string> args = { to_string(PhotoAlbumSubType::FAVORITE) };
    auto ret = albumRefreshDel.Delete(changeRows, PhotoAlbumColumns::TABLE, whereClause, args);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 1);

     // 操作前后数据结果
    EXPECT_TRUE(CheckDeleteResult(albumRefreshDel, RDB_OPERATION_REMOVE, FAVORITE_ALBUM_INFO, AlbumChangeInfo()));

    // 通知结果
    albumRefreshDel.Notify();
    EXPECT_TRUE(CheckDeleteNotify(albumRefreshDel, Notification::ALBUM_OPERATION_REMOVE, FAVORITE_ALBUM_INFO,
        AlbumChangeInfo()));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Delete_Trans_054, TestSize.Level2)
{
    PrepareAlbumData();
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 2);

    std::shared_ptr<TransactionOperations> trans =
        make_shared<TransactionOperations>("AlbumAccurateRefreshTest_Delete_Trans_054");
    AlbumAccurateRefresh albumRefreshDel("AlbumAccurateRefreshTest_Delete_Trans_054", trans);
    int32_t changeRows = 0;
    string whereClause = PhotoAlbumColumns::ALBUM_SUBTYPE + " = ?";
    vector<string> args = { to_string(PhotoAlbumSubType::FAVORITE) };
    function<int32_t()> transFunc = [&] () -> int32_t {
        auto ret = albumRefreshDel.Delete(changeRows, PhotoAlbumColumns::TABLE, whereClause, args);
        return ret;
    };

    auto ret = trans->RetryTrans(transFunc);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 1);
    // 操作前后数据结果
    EXPECT_TRUE(CheckDeleteResult(albumRefreshDel, RDB_OPERATION_REMOVE, FAVORITE_ALBUM_INFO, AlbumChangeInfo()));

    // 通知结果
    albumRefreshDel.Notify();
    EXPECT_TRUE(CheckDeleteNotify(albumRefreshDel, Notification::ALBUM_OPERATION_REMOVE, FAVORITE_ALBUM_INFO,
        AlbumChangeInfo()));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Delete_055, TestSize.Level2)
{
    PrepareAlbumData();
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 2);

    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::FAVORITE));
    
    AlbumAccurateRefresh albumRefreshDel("AlbumAccurateRefreshTest_Delete_055");
    int32_t changeRows = 0;
    auto ret = albumRefreshDel.Delete(changeRows, predicates);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 1);

    // 操作前后数据结果
    EXPECT_TRUE(CheckDeleteResult(albumRefreshDel, RDB_OPERATION_REMOVE, FAVORITE_ALBUM_INFO, AlbumChangeInfo()));

    // 通知结果
    albumRefreshDel.Notify();
    EXPECT_TRUE(CheckDeleteNotify(albumRefreshDel, Notification::ALBUM_OPERATION_REMOVE, FAVORITE_ALBUM_INFO,
        AlbumChangeInfo()));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Delete_Trans_056, TestSize.Level2)
{
    PrepareAlbumData();
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 2);

    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::FAVORITE));

    std::shared_ptr<TransactionOperations> trans =
        make_shared<TransactionOperations>("AlbumAccurateRefreshTest_Delete_Trans_056");
    AlbumAccurateRefresh albumRefreshDel("AlbumAccurateRefreshTest_Delete_Trans_056", trans);
    int32_t changeRows = 0;
    string whereClause = PhotoAlbumColumns::ALBUM_SUBTYPE + " = ?";
    vector<string> args = { to_string(PhotoAlbumSubType::FAVORITE) };
    function<int32_t()> transFunc = [&] () -> int32_t {
        auto ret = albumRefreshDel.Delete(changeRows, predicates);
        return ret;
    };

    auto ret = trans->RetryTrans(transFunc);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 1);
    // 操作前后数据结果
    EXPECT_TRUE(CheckDeleteResult(albumRefreshDel, RDB_OPERATION_REMOVE, FAVORITE_ALBUM_INFO, AlbumChangeInfo()));
    // 通知结果
    albumRefreshDel.Notify();
    EXPECT_TRUE(CheckDeleteNotify(albumRefreshDel, Notification::ALBUM_OPERATION_REMOVE, FAVORITE_ALBUM_INFO,
        AlbumChangeInfo()));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Exe_057, TestSize.Level2)
{
    AlbumAccurateRefresh albumRefreshExe("AlbumAccurateRefreshTest_Exe_057");
    albumRefreshExe.Init();
    auto ret = albumRefreshExe.ExecuteSql(insertSql, RdbOperation::RDB_OPERATION_ADD);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 1);
    EXPECT_TRUE(GetAlbumCount(PhotoAlbumSubType::FAVORITE) == FAVORITE_ALBUM_INFO.count_);

    // 数据库执行结果
    AlbumChangeInfo albumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    EXPECT_TRUE(IsEqualAlbumInfo(albumInfo, FAVORITE_ALBUM_INFO));
    // 操作前后数据结果
    EXPECT_TRUE(CheckInsertResult(albumRefreshExe, albumInfo));
    
    // 通知结果
    albumRefreshExe.Notify();
    EXPECT_TRUE(CheckInsertNotify(albumRefreshExe, albumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Exe_Trans_058, TestSize.Level2)
{
    std::shared_ptr<TransactionOperations> trans =
        make_shared<TransactionOperations>("AlbumAccurateRefreshTest_Exe_Trans_058");
    AlbumAccurateRefresh albumRefreshExe("AlbumAccurateRefreshTest_Exe_Trans_058", trans);
    albumRefreshExe.Init();
    function<int32_t()> transFunc = [&]() -> int32_t {
        return albumRefreshExe.ExecuteSql(insertSql, RdbOperation::RDB_OPERATION_ADD);
    };
    auto ret = trans->RetryTrans(transFunc);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 1);
    EXPECT_TRUE(GetAlbumCount(PhotoAlbumSubType::FAVORITE) == FAVORITE_ALBUM_INFO.count_);

    // 数据库执行结果
    AlbumChangeInfo albumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    EXPECT_TRUE(IsEqualAlbumInfo(albumInfo, FAVORITE_ALBUM_INFO));
    // 操作前后数据结果
    EXPECT_TRUE(CheckInsertResult(albumRefreshExe, albumInfo));
    
    // 通知结果
    albumRefreshExe.Notify();
    EXPECT_TRUE(CheckInsertNotify(albumRefreshExe, albumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Exe_059, TestSize.Level2)
{
    vector<ValueObject> args = {to_string(FAVORITE_ALBUM_INFO.count_)};

    AlbumAccurateRefresh albumRefreshExe("AlbumAccurateRefreshTest_Exe_059");
    albumRefreshExe.Init();
    auto changedRowId =
        albumRefreshExe.ExecuteForLastInsertedRowId(insertSqlWithArgs, args, RdbOperation::RDB_OPERATION_ADD);
    ACCURATE_DEBUG("changedRowId: %{public}d", changedRowId);
    EXPECT_TRUE(changedRowId > 0);
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 1);
    EXPECT_TRUE(GetAlbumCount(PhotoAlbumSubType::FAVORITE) == FAVORITE_ALBUM_INFO.count_);

    // 数据库执行结果
    AlbumChangeInfo albumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    EXPECT_TRUE(IsEqualAlbumInfo(albumInfo, FAVORITE_ALBUM_INFO));
    // 操作前后数据结果
    EXPECT_TRUE(CheckInsertResult(albumRefreshExe, albumInfo));
    
    // 通知结果
    albumRefreshExe.Notify();
    EXPECT_TRUE(CheckInsertNotify(albumRefreshExe, albumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Exe_060, TestSize.Level2)
{
    vector<ValueObject> args = {to_string(FAVORITE_ALBUM_INFO.count_)};

    std::shared_ptr<TransactionOperations> trans =
        make_shared<TransactionOperations>("AlbumAccurateRefreshTest_Exe_060");
    AlbumAccurateRefresh albumRefreshExe("AlbumAccurateRefreshTest_Exe_060", trans);
    albumRefreshExe.Init();
    int32_t changedRowId = 0;
    function<int32_t()> transFunc = [&]() -> int32_t {
        changedRowId =
            albumRefreshExe.ExecuteForLastInsertedRowId(insertSqlWithArgs, args, RdbOperation::RDB_OPERATION_ADD);
        if (changedRowId != E_HAS_DB_ERROR) {
            return ACCURATE_REFRESH_RET_OK;
        } else {
            return E_HAS_DB_ERROR;
        }
    };
    auto ret = trans->RetryTrans(transFunc);
    
    ACCURATE_DEBUG("changedRowId: %{public}d", changedRowId);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changedRowId > 0);
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 1);
    EXPECT_TRUE(GetAlbumCount(PhotoAlbumSubType::FAVORITE) == FAVORITE_ALBUM_INFO.count_);
    // 数据库执行结果
    AlbumChangeInfo albumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    EXPECT_TRUE(IsEqualAlbumInfo(albumInfo, FAVORITE_ALBUM_INFO));
    // 操作前后数据结果
    EXPECT_TRUE(CheckInsertResult(albumRefreshExe, albumInfo));
    
    // 通知结果
    albumRefreshExe.Notify();
    EXPECT_TRUE(CheckInsertNotify(albumRefreshExe, albumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Exe_061, TestSize.Level2)
{
    vector<ValueObject> args = {to_string(FAVORITE_ALBUM_INFO.count_)};

    AlbumAccurateRefresh albumRefreshExe("AlbumAccurateRefreshTest_Exe_061");
    albumRefreshExe.Init();
    auto ret = albumRefreshExe.ExecuteSql(insertSqlWithArgs, args, RdbOperation::RDB_OPERATION_ADD);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 1);
    EXPECT_TRUE(GetAlbumCount(PhotoAlbumSubType::FAVORITE) == FAVORITE_ALBUM_INFO.count_);
    // 数据库执行结果
    AlbumChangeInfo albumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    EXPECT_TRUE(IsEqualAlbumInfo(albumInfo, FAVORITE_ALBUM_INFO));
    // 操作前后数据结果
    EXPECT_TRUE(CheckInsertResult(albumRefreshExe, albumInfo));
    
    // 通知结果
    albumRefreshExe.Notify();
    EXPECT_TRUE(CheckInsertNotify(albumRefreshExe, albumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Exe_062, TestSize.Level2)
{
    vector<ValueObject> args = {to_string(FAVORITE_ALBUM_INFO.count_)};

    std::shared_ptr<TransactionOperations> trans =
        make_shared<TransactionOperations>("AlbumAccurateRefreshTest_Exe_062");
    AlbumAccurateRefresh albumRefreshExe("AlbumAccurateRefreshTest_Exe_062", trans);
    albumRefreshExe.Init();

    function<int32_t()> transFunc = [&]() -> int32_t {
        auto ret = albumRefreshExe.ExecuteSql(insertSqlWithArgs, args, RdbOperation::RDB_OPERATION_ADD);
        return ret;
    };
    auto ret = trans->RetryTrans(transFunc);

    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 1);
    EXPECT_TRUE(GetAlbumCount(PhotoAlbumSubType::FAVORITE) == FAVORITE_ALBUM_INFO.count_);
    // 数据库执行结果
    AlbumChangeInfo albumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    EXPECT_TRUE(IsEqualAlbumInfo(albumInfo, FAVORITE_ALBUM_INFO));
    // 操作前后数据结果
    EXPECT_TRUE(CheckInsertResult(albumRefreshExe, albumInfo));
    
    // 通知结果
    albumRefreshExe.Notify();
    EXPECT_TRUE(CheckInsertNotify(albumRefreshExe, albumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Exe_063, TestSize.Level2)
{
    vector<ValueObject> args = {to_string(FAVORITE_ALBUM_INFO.count_)};

    AlbumAccurateRefresh albumRefreshExe("AlbumAccurateRefreshTest_Exe_063");
    albumRefreshExe.Init();
    int64_t outValue = 0;
    auto ret =
        albumRefreshExe.ExecuteForChangedRowCount(outValue, insertSqlWithArgs, args, RdbOperation::RDB_OPERATION_ADD);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(outValue == 1);
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 1);
    EXPECT_TRUE(GetAlbumCount(PhotoAlbumSubType::FAVORITE) == FAVORITE_ALBUM_INFO.count_);
    // 数据库执行结果
    AlbumChangeInfo albumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    EXPECT_TRUE(IsEqualAlbumInfo(albumInfo, FAVORITE_ALBUM_INFO));
    // 操作前后数据结果
    EXPECT_TRUE(CheckInsertResult(albumRefreshExe, albumInfo));
    
    // 通知结果
    albumRefreshExe.Notify();
    EXPECT_TRUE(CheckInsertNotify(albumRefreshExe, albumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Exe_064, TestSize.Level2)
{
    vector<ValueObject> args = {to_string(FAVORITE_ALBUM_INFO.count_)};

    std::shared_ptr<TransactionOperations> trans =
        make_shared<TransactionOperations>("AlbumAccurateRefreshTest_Exe_064");
    AlbumAccurateRefresh albumRefreshExe("AlbumAccurateRefreshTest_Exe_064", trans);
    albumRefreshExe.Init();
    int64_t outValue = 0;
    function<int32_t()> transFunc = [&]() -> int32_t {
        auto ret =albumRefreshExe.ExecuteForChangedRowCount(outValue, insertSqlWithArgs, args,
            RdbOperation::RDB_OPERATION_ADD);
        return ret;
    };
    auto ret = trans->RetryTrans(transFunc);

    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(outValue == 1);
    EXPECT_TRUE(GetPhotoAlbumTotalCount() == 1);
    EXPECT_TRUE(GetAlbumCount(PhotoAlbumSubType::FAVORITE) == FAVORITE_ALBUM_INFO.count_);
    // 数据库执行结果
    AlbumChangeInfo albumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    EXPECT_TRUE(IsEqualAlbumInfo(albumInfo, FAVORITE_ALBUM_INFO));
    // 操作前后数据结果
    EXPECT_TRUE(CheckInsertResult(albumRefreshExe, albumInfo));
    
    // 通知结果
    albumRefreshExe.Notify();
    EXPECT_TRUE(CheckInsertNotify(albumRefreshExe, albumInfo));
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Update_Exceed_065, TestSize.Level2)
{
    PrepareHiddenData();
    ValuesBucket value;
    auto newCount = HIDDEN_ALBUM_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_COUNT, newCount);
    auto imageCount = HIDDEN_ALBUM_IMAGE_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, imageCount);
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::HIDDEN));

    AlbumAccurateRefresh albumRefreshUpdate("AlbumAccurateRefreshTest_Update_Exceed_065");
    int32_t changeRows = 0;
    auto ret = albumRefreshUpdate.Update(changeRows, value, predicates);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);
    auto &changeDatasMap = albumRefreshUpdate.dataManager_.changeDatas_;
    AlbumChangeData changeData;
    // 总共1000条
    for (int i = 0; i < 999; ++i) {
        changeDatasMap.insert_or_assign(1000000 + i, changeData);
    }

    ValuesBucket newValue;
    newCount = HIDDEN_ALBUM_COUNT;
    newValue.PutInt(PhotoAlbumColumns::ALBUM_COUNT, newCount);
    imageCount = HIDDEN_ALBUM_IMAGE_COUNT;
    newValue.PutInt(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, imageCount);
    ret = albumRefreshUpdate.Update(changeRows, value, predicates);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);
    // 总共1000条
    EXPECT_TRUE(albumRefreshUpdate.dataManager_.CheckIsExceed());
    EXPECT_TRUE(albumRefreshUpdate.dataManager_.changeDatas_.empty());
    EXPECT_TRUE(albumRefreshUpdate.Notify() == ACCURATE_REFRESH_RET_OK);
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Update_Exceed_066, TestSize.Level2)
{
    PrepareHiddenData();
    ValuesBucket value;
    auto newCount = HIDDEN_ALBUM_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_COUNT, newCount);
    auto imageCount = HIDDEN_ALBUM_IMAGE_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, imageCount);
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::HIDDEN));

    AlbumAccurateRefresh albumRefreshUpdate("AlbumAccurateRefreshTest_Update_Exceed_066");
    int32_t changeRows = 0;
    auto ret = albumRefreshUpdate.Update(changeRows, value, predicates);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);
    auto &changeDatasMap = albumRefreshUpdate.dataManager_.changeDatas_;
    AlbumChangeData changeData;
    // 总共999条
    for (int i = 0; i < 998; ++i) {
        changeDatasMap.insert_or_assign(1000000 + i, changeData);
    }

    ValuesBucket newValue;
    newCount = HIDDEN_ALBUM_COUNT;
    newValue.PutInt(PhotoAlbumColumns::ALBUM_COUNT, newCount);
    imageCount = HIDDEN_ALBUM_IMAGE_COUNT;
    newValue.PutInt(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, imageCount);
    ret = albumRefreshUpdate.Update(changeRows, value, predicates);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);
    // 总共999条
    EXPECT_TRUE(!albumRefreshUpdate.dataManager_.CheckIsExceed());
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Update_cmd_067, TestSize.Level2)
{
    PrepareAlbumData();
    Uri uri("");
    MediaLibraryCommand cmd(uri);
    cmd.SetTableName(PhotoAlbumColumns::TABLE);
    auto predicates = cmd.GetAbsRdbPredicates();
    predicates->SetWhereClause(PhotoAlbumColumns::ALBUM_SUBTYPE + " = ?");
    predicates->SetWhereArgs({ to_string(PhotoAlbumSubType::FAVORITE) });
    ValuesBucket value;
    auto newCount = FAVORITE_ALBUM_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_COUNT, newCount);
    auto imageCount = FAVORITE_ALBUM_IMAGE_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, imageCount);
    auto changeTime = MediaFileUtils::UTCTimeMilliSeconds();
    value.PutInt(PhotoAlbumColumns::CHANGE_TIME, changeTime);
    cmd.SetValueBucket(value);

    AlbumAccurateRefresh albumRefreshUpdate;
    int32_t changeRows = 0;
    auto ret = albumRefreshUpdate.Update(cmd, changeRows);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);
    // 数据库执行结果
    AlbumChangeInfo favoriteAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::FAVORITE);
    AlbumChangeInfo updateAlbumInfo = FAVORITE_ALBUM_INFO;
    updateAlbumInfo.count_ = newCount;
    updateAlbumInfo.imageCount_ = imageCount;
    EXPECT_TRUE(IsEqualAlbumInfo(favoriteAlbumInfo, updateAlbumInfo));
    EXPECT_TRUE(GetAlbumCount(PhotoAlbumSubType::FAVORITE) == newCount);
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Update_cmd_068, TestSize.Level2)
{
    PrepareAlbumData();
    Uri uri("");
    MediaLibraryCommand cmd(uri);
    cmd.SetTableName(PhotoAlbumColumns::ALBUM_NAME);
    auto predicates = cmd.GetAbsRdbPredicates();
    predicates->SetWhereClause(PhotoAlbumColumns::ALBUM_SUBTYPE + " = ?");
    predicates->SetWhereArgs({ to_string(PhotoAlbumSubType::FAVORITE) });
    ValuesBucket value;
    auto newCount = FAVORITE_ALBUM_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_COUNT, newCount);
    auto imageCount = FAVORITE_ALBUM_IMAGE_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, imageCount);
    auto changeTime = MediaFileUtils::UTCTimeMilliSeconds();
    value.PutInt(PhotoAlbumColumns::CHANGE_TIME, changeTime);
    cmd.SetValueBucket(value);

    AlbumAccurateRefresh albumRefreshUpdate;
    int32_t changeRows = 0;
    auto ret = albumRefreshUpdate.Update(cmd, changeRows);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret > 0);
    EXPECT_TRUE(changeRows == 0);
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Update_069, TestSize.Level2)
{
    PrepareHiddenData();
    ValuesBucket value;
    auto newCount = HIDDEN_ALBUM_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_COUNT, newCount);
    auto imageCount = HIDDEN_ALBUM_IMAGE_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, imageCount);
    auto changeTime = MediaFileUtils::UTCTimeMilliSeconds();
    value.PutInt(PhotoAlbumColumns::CHANGE_TIME, changeTime);
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::HIDDEN));

    AlbumAccurateRefresh albumRefreshUpdate("AlbumAccurateRefreshTest_Update_069");
    int32_t changeRows = 0;
    auto ret = albumRefreshUpdate.Update(changeRows, value, predicates);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret == ACCURATE_REFRESH_RET_OK);
    EXPECT_TRUE(changeRows == 1);
    // 数据库执行结果
    AlbumChangeInfo hiddenAlbumInfo = GetAlbumInfo(PhotoAlbumSubType::HIDDEN);
    AlbumChangeInfo updateAlbumInfo = HIDDEN_ALBUM_INFO;
    updateAlbumInfo.count_ = newCount;
    updateAlbumInfo.imageCount_ = imageCount;
    EXPECT_TRUE(IsEqualAlbumInfo(hiddenAlbumInfo, updateAlbumInfo));
    EXPECT_TRUE(GetAlbumCount(PhotoAlbumSubType::HIDDEN) == newCount);
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_Update_070, TestSize.Level2)
{
    PrepareHiddenData();
    ValuesBucket value;
    auto newCount = HIDDEN_ALBUM_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_COUNT, newCount);
    auto imageCount = HIDDEN_ALBUM_IMAGE_COUNT - 1;
    value.PutInt(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, imageCount);
    auto changeTime = MediaFileUtils::UTCTimeMilliSeconds();
    value.PutInt(PhotoAlbumColumns::CHANGE_TIME, changeTime);
    RdbPredicates predicates(PhotoAlbumColumns::ALBUM_NAME);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::HIDDEN));

    AlbumAccurateRefresh albumRefreshUpdate("AlbumAccurateRefreshTest_Update_069");
    int32_t changeRows = 0;
    auto ret = albumRefreshUpdate.Update(changeRows, value, predicates);
    ACCURATE_DEBUG("ret: %{public}d", ret);
    EXPECT_TRUE(ret > 0);
    EXPECT_TRUE(changeRows == 0);
}
} // namespace Media
} // namespace OHOS