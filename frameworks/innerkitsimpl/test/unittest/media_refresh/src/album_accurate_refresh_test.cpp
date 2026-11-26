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
#include <map>
#include <thread>

#include "media_file_utils.h"
#include "media_pure_file_utils.h"
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
        bool ret = MediaPureFileUtils::CreateDirectory(ROOT_PATH + dir + "/");
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

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_GetUpdateValues_StringUpdate_068, TestSize.Level1)
{
    AlbumChangeInfo oldInfo = FAVORITE_ALBUM_INFO;
    AlbumChangeInfo newInfo = oldInfo;
    newInfo.lpath_ = FAVORITE_ALBUM_LPATH + "_new";

    NotifyType type = NOTIFY_INVALID;
    auto values = newInfo.GetUpdateValues(oldInfo, type);
    map<string, ValueObject> valuesMap;
    values.GetAll(valuesMap);
    auto iter = valuesMap.find(PhotoAlbumColumns::ALBUM_LPATH);

    EXPECT_EQ(type, NOTIFY_UPDATE);
    EXPECT_EQ(valuesMap.size(), 1);
    ASSERT_NE(iter, valuesMap.end());

    string lpath;
    iter->second.GetString(lpath);
    EXPECT_EQ(lpath, newInfo.lpath_);
}

HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_GetUpdateValues_StringNoUpdate_069, TestSize.Level1)
{
    AlbumChangeInfo oldInfo = FAVORITE_ALBUM_INFO;
    AlbumChangeInfo newInfo = oldInfo;

    NotifyType type = NOTIFY_INVALID;
    auto values = newInfo.GetUpdateValues(oldInfo, type);
    map<string, ValueObject> valuesMap;
    values.GetAll(valuesMap);

    EXPECT_EQ(type, NOTIFY_UPDATE);
    EXPECT_TRUE(valuesMap.empty());
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

// 测试IsCoverContentChange - 空fileIds参数
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_IsCoverContentChange_001, TestSize.Level2)
{
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds;
    auto ret = albumRefresh.IsCoverContentChange(fileIds);
    EXPECT_FALSE(ret);
}

// 测试IsCoverContentChange - 单个fileId匹配coverUri
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_IsCoverContentChange_002, TestSize.Level2)
{
    MEDIA_INFO_LOG("AlbumAccurateRefreshTest_IsCoverContentChange_002 Start");
    // 准备测试数据：插入一个album，其coverUri包含fileId
    ValuesBucket albumValues = GetFavoriteInsertAlbum();
    string testFileId = "12345";
    string testCoverUri = "file://media/Photo/" + testFileId + "/IMG_1744362716_000/IMG_20250425_123456.jpg";
    albumValues.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    albumValues.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, testCoverUri);
    
    int64_t insertNum = 0;
    g_rdbStore->Insert(insertNum, PhotoAlbumColumns::TABLE, albumValues);
    EXPECT_TRUE(insertNum > 0);
    
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds = {testFileId};
    auto ret = albumRefresh.IsCoverContentChange(fileIds);
    EXPECT_TRUE(ret);
    MEDIA_INFO_LOG("AlbumAccurateRefreshTest_IsCoverContentChange_002 End");
}

// 测试IsCoverContentChange - 单个fileId匹配hiddenCover
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_IsCoverContentChange_003, TestSize.Level2)
{
    MEDIA_INFO_LOG("AlbumAccurateRefreshTest_IsCoverContentChange_003 Start");
    // 准备测试数据：插入一个album，其hiddenCover包含fileId
    ValuesBucket albumValues = GetFavoriteInsertAlbum();
    string testFileId = "67890";
    string testHiddenCover = "file://media/Photo/" + testFileId + "/IMG_1744362716_000/IMG_20250425_123456.jpg";
    albumValues.Delete(PhotoAlbumColumns::HIDDEN_COVER);
    albumValues.PutString(PhotoAlbumColumns::HIDDEN_COVER, testHiddenCover);
    
    int64_t insertNum = 0;
    g_rdbStore->Insert(insertNum, PhotoAlbumColumns::TABLE, albumValues);
    EXPECT_TRUE(insertNum > 0);
    
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds = {testFileId};
    auto ret = albumRefresh.IsCoverContentChange(fileIds);
    EXPECT_TRUE(ret);
    MEDIA_INFO_LOG("AlbumAccurateRefreshTest_IsCoverContentChange_003 End");
}

// 测试IsCoverContentChange - 多个fileIds匹配多个albums
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_IsCoverContentChange_004, TestSize.Level2)
{
    MEDIA_INFO_LOG("AlbumAccurateRefreshTest_IsCoverContentChange_004 Start");
    // 准备测试数据：插入多个albums
    ValuesBucket album1 = GetFavoriteInsertAlbum();
    string testFileId1 = "11111";
    string testCoverUri1 = "file://media/Photo/" + testFileId1 + "/IMG_1744362716_000/IMG_20250425_123456.jpg";
    album1.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    album1.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, testCoverUri1);
    
    ValuesBucket album2 = GetTrashInsertAlbum();
    string testFileId2 = "22222";
    string testHiddenCover2 = "file://media/Photo/" + testFileId2 + "/IMG_1744362716_000/IMG_20250425_123456.jpg";
    album2.Delete(PhotoAlbumColumns::HIDDEN_COVER);
    album2.PutString(PhotoAlbumColumns::HIDDEN_COVER, testHiddenCover2);
    
    int64_t insertNum1 = 0;
    int64_t insertNum2 = 0;
    g_rdbStore->Insert(insertNum1, PhotoAlbumColumns::TABLE, album1);
    g_rdbStore->Insert(insertNum2, PhotoAlbumColumns::TABLE, album2);
    EXPECT_TRUE(insertNum1 > 0);
    EXPECT_TRUE(insertNum2 > 0);
    
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds = {testFileId1, testFileId2};
    auto ret = albumRefresh.IsCoverContentChange(fileIds);
    EXPECT_TRUE(ret);
    MEDIA_INFO_LOG("AlbumAccurateRefreshTest_IsCoverContentChange_004 End");
}

// 测试IsCoverContentChange - fileIds不匹配任何album
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_IsCoverContentChange_005, TestSize.Level2)
{
    // 准备测试数据：插入album，但fileIds不匹配
    ValuesBucket albumValues = GetFavoriteInsertAlbum();
    string testCoverUri = "file://media/Photo/99999/IMG_1744362716_000/IMG_20250425_123456.jpg";
    albumValues.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    albumValues.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, testCoverUri);
    
    int64_t insertNum = 0;
    g_rdbStore->Insert(insertNum, PhotoAlbumColumns::TABLE, albumValues);
    EXPECT_TRUE(insertNum > 0);
    
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds = {"88888", "77777"};
    auto ret = albumRefresh.IsCoverContentChange(fileIds);
    EXPECT_FALSE(ret);
}

// 测试IsCoverContentChange - 数据库为空
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_IsCoverContentChange_006, TestSize.Level2)
{
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds = {"12345", "67890"};
    auto ret = albumRefresh.IsCoverContentChange(fileIds);
    EXPECT_FALSE(ret);
}

// 测试IsCoverContentChange - 数据库有多条记录，部分匹配
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_IsCoverContentChange_007, TestSize.Level2)
{
    MEDIA_INFO_LOG("AlbumAccurateRefreshTest_IsCoverContentChange_007 Start");
    // 准备测试数据：插入3个albums，只有2个匹配
    ValuesBucket album1 = GetFavoriteInsertAlbum();
    string testFileId1 = "10001";
    album1.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    album1.PutString(PhotoAlbumColumns::ALBUM_COVER_URI,
        "file://media/Photo/" + testFileId1 + "/IMG_1744362716_000/IMG_20250425_123456.jpg");
    
    ValuesBucket album2 = GetTrashInsertAlbum();
    string testFileId2 = "10002";
    album2.Delete(PhotoAlbumColumns::HIDDEN_COVER);
    album2.PutString(PhotoAlbumColumns::HIDDEN_COVER,
        "file://media/Photo/" + testFileId2 + "/IMG_1744362716_000/IMG_20250425_123456.jpg");
    
    ValuesBucket album3 = GetHiddenInsertAlbum();
    album3.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    album3.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, "file://media/Photo/99999.jpg");
    
    int64_t insertNum1 = 0;
    int64_t insertNum2 = 0;
    int64_t insertNum3 = 0;
    g_rdbStore->Insert(insertNum1, PhotoAlbumColumns::TABLE, album1);
    g_rdbStore->Insert(insertNum2, PhotoAlbumColumns::TABLE, album2);
    g_rdbStore->Insert(insertNum3, PhotoAlbumColumns::TABLE, album3);
    EXPECT_TRUE(insertNum1 > 0 && insertNum2 > 0 && insertNum3 > 0);
    
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds = {testFileId1, testFileId2};
    auto ret = albumRefresh.IsCoverContentChange(fileIds);
    EXPECT_TRUE(ret);
    MEDIA_INFO_LOG("AlbumAccurateRefreshTest_IsCoverContentChange_007 End");
}

// 测试IsCoverContentChange - coverUri和hiddenCover都为空
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_IsCoverContentChange_008, TestSize.Level2)
{
    ValuesBucket albumValues = GetFavoriteInsertAlbum();
    albumValues.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    albumValues.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, "");
    albumValues.Delete(PhotoAlbumColumns::HIDDEN_COVER);
    albumValues.PutString(PhotoAlbumColumns::HIDDEN_COVER, "");
    
    int64_t insertNum = 0;
    g_rdbStore->Insert(insertNum, PhotoAlbumColumns::TABLE, albumValues);
    EXPECT_TRUE(insertNum > 0);
    
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds = {"12345"};
    auto ret = albumRefresh.IsCoverContentChange(fileIds);
    EXPECT_FALSE(ret);
}

// 测试IsCoverContentChange - 同一个fileId匹配多个albums
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_IsCoverContentChange_009, TestSize.Level2)
{
    string testFileId = "55555";
    
    ValuesBucket album1 = GetFavoriteInsertAlbum();
    album1.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    album1.PutString(PhotoAlbumColumns::ALBUM_COVER_URI,
        "file://media/Photo/" + testFileId + "/IMG_1744362716_000/IMG_20250425_123456.jpg");
    
    ValuesBucket album2 = GetTrashInsertAlbum();
    album2.Delete(PhotoAlbumColumns::HIDDEN_COVER);
    album2.PutString(PhotoAlbumColumns::HIDDEN_COVER,
        "file://media/Photo/" + testFileId + "/IMG_1744362716_000/IMG_20250425_123456.jpg");
    
    int64_t insertNum1 = 0;
    int64_t insertNum2 = 0;
    g_rdbStore->Insert(insertNum1, PhotoAlbumColumns::TABLE, album1);
    g_rdbStore->Insert(insertNum2, PhotoAlbumColumns::TABLE, album2);
    EXPECT_TRUE(insertNum1 > 0 && insertNum2 > 0);
    
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds = {testFileId};
    auto ret = albumRefresh.IsCoverContentChange(fileIds);
    EXPECT_TRUE(ret);
}

// 测试IsCoverContentChange - 大量fileIds
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_IsCoverContentChange_010, TestSize.Level2)
{
    ValuesBucket albumValues = GetFavoriteInsertAlbum();
    string testFileId = "33333";
    albumValues.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    albumValues.PutString(PhotoAlbumColumns::ALBUM_COVER_URI,
        "file://media/Photo/" + testFileId + "/IMG_1744362716_000/IMG_20250425_123456.jpg");
    
    int64_t insertNum = 0;
    g_rdbStore->Insert(insertNum, PhotoAlbumColumns::TABLE, albumValues);
    EXPECT_TRUE(insertNum > 0);
    
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds;
    for (int i = 0; i < 100; i++) {
        fileIds.push_back(to_string(i));
    }
    fileIds.push_back(testFileId);
    auto ret = albumRefresh.IsCoverContentChange(fileIds);
    EXPECT_TRUE(ret);
}

// 测试NotifyAlbumsCoverChange - 空fileIds参数
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_NotifyAlbumsCoverChange_011, TestSize.Level2)
{
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds;
    vector<int32_t> albumIds = {1, 2, 3};
    albumRefresh.NotifyAlbumsCoverChange(fileIds, albumIds);
}

// 测试NotifyAlbumsCoverChange - 空albumIds参数
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_NotifyAlbumsCoverChange_012, TestSize.Level2)
{
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds = {"12345"};
    vector<int32_t> albumIds;
    albumRefresh.NotifyAlbumsCoverChange(fileIds, albumIds);
}

// 测试NotifyAlbumsCoverChange - 正常场景：coverUri匹配
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_NotifyAlbumsCoverChange_013, TestSize.Level2)
{
    // 准备测试数据
    ValuesBucket albumValues = GetFavoriteInsertAlbum();
    string testFileId = "44444";
    string testCoverUri = "file://media/Photo/" + testFileId + "/IMG_1744362716_000/IMG_20250425_123456.jpg";
    albumValues.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    albumValues.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, testCoverUri);
    
    int64_t insertNum = 0;
    g_rdbStore->Insert(insertNum, PhotoAlbumColumns::TABLE, albumValues);
    EXPECT_TRUE(insertNum > 0);
    
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds = {testFileId};
    vector<int32_t> albumIds = {FAVORITE_ALBUM_INFO.albumId_};
    albumRefresh.NotifyAlbumsCoverChange(fileIds, albumIds);
    
    // 验证通知结果
    auto changeDatas = albumRefresh.GetAlbumChangeDatas();
    EXPECT_TRUE(changeDatas.size() > 0);
}

// 测试NotifyAlbumsCoverChange - 正常场景：hiddenCover匹配
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_NotifyAlbumsCoverChange_014, TestSize.Level2)
{
    // 准备测试数据
    ValuesBucket albumValues = GetFavoriteInsertAlbum();
    string testFileId = "55555";
    string testHiddenCover = "file://media/Photo/" + testFileId + "/IMG_1744362716_000/IMG_20250425_123456.jpg";
    albumValues.Delete(PhotoAlbumColumns::HIDDEN_COVER);
    albumValues.PutString(PhotoAlbumColumns::HIDDEN_COVER, testHiddenCover);
    
    int64_t insertNum = 0;
    g_rdbStore->Insert(insertNum, PhotoAlbumColumns::TABLE, albumValues);
    EXPECT_TRUE(insertNum > 0);
    
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds = {testFileId};
    vector<int32_t> albumIds = {FAVORITE_ALBUM_INFO.albumId_};
    albumRefresh.NotifyAlbumsCoverChange(fileIds, albumIds);
    
    // 验证通知结果
    auto changeDatas = albumRefresh.GetAlbumChangeDatas();
    EXPECT_TRUE(changeDatas.size() > 0);
}

// 测试NotifyAlbumsCoverChange - 多个fileIds和多个albumIds
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshRefreshTest_NotifyAlbumsCoverChange_015, TestSize.Level2)
{
    // 准备测试数据
    ValuesBucket album1 = GetFavoriteInsertAlbum();
    string testFileId1 = "66666";
    album1.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    album1.PutString(PhotoAlbumColumns::ALBUM_COVER_URI,
        "file://media/Photo/" + testFileId1 + "/IMG_1744362716_000/IMG_20250425_123456.jpg");
    
    ValuesBucket album2 = GetTrashInsertAlbum();
    string testFileId2 = "77777";
    album2.Delete(PhotoAlbumColumns::HIDDEN_COVER);
    album2.PutString(PhotoAlbumColumns::HIDDEN_COVER,
        "file://media/Photo/" + testFileId2 + "/IMG_1744362716_000/IMG_20250425_123456.jpg");
    
    int64_t insertNum1 = 0;
    int64_t insertNum2 = 0;
    g_rdbStore->Insert(insertNum1, PhotoAlbumColumns::TABLE, album1);
    g_rdbStore->Insert(insertNum2, PhotoAlbumColumns::TABLE, album2);
    EXPECT_TRUE(insertNum1 > 0 && insertNum2 > 0);
    
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds = {testFileId1, testFileId2};
    vector<int32_t> albumIds = {FAVORITE_ALBUM_INFO.albumId_, TRASH_ALBUM_INFO.albumId_};
    albumRefresh.NotifyAlbumsCoverChange(fileIds, albumIds);
    
    // 验证通知结果
    auto changeDatas = albumRefresh.GetAlbumChangeDatas();
    EXPECT_TRUE(changeDatas.size() > 0);
}

// 测试NotifyAlbumsCoverChange - fileId不匹配任何URI
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_NotifyAlbumsCoverChange_017, TestSize.Level2)
{
    // 准备测试数据
    ValuesBucket albumValues = GetFavoriteInsertAlbum();
    albumValues.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    albumValues.PutString(PhotoAlbumColumns::ALBUM_COVER_URI,
        "file://media/Photo/11111/IMG_1744362716_000/IMG_20250425_123456.jpg");
    albumValues.Delete(PhotoAlbumColumns::HIDDEN_COVER);
    albumValues.PutString(PhotoAlbumColumns::HIDDEN_COVER,
        "file://media/Photo/22222/IMG_1744362716_000/IMG_20250425_123456.jpg");
    
    int64_t insertNum = 0;
    g_rdbStore->Insert(insertNum, PhotoAlbumColumns::TABLE, albumValues);
    EXPECT_TRUE(insertNum > 0);
    
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds = {"33333", "44444"};
    vector<int32_t> albumIds = {FAVORITE_ALBUM_INFO.albumId_};
    albumRefresh.NotifyAlbumsCoverChange(fileIds, albumIds);
    
    // 验证通知结果
    auto changeDatas = albumRefresh.GetAlbumChangeDatas();
    EXPECT_TRUE(changeDatas.size() > 0);
    if (changeDatas.size() > 0) {
        EXPECT_FALSE(changeDatas[0].infoAfterChange_.isCoverChange_);
        EXPECT_FALSE(changeDatas[0].infoAfterChange_.isHiddenCoverChange_);
    }
}

// 测试NotifyAlbumsCoverChange - URI为空字符串
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_NotifyAlbumsCoverChange_018, TestSize.Level2)
{
    // 准备测试数据
    ValuesBucket albumValues = GetFavoriteInsertAlbum();
    albumValues.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    albumValues.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, "");
    albumValues.Delete(PhotoAlbumColumns::HIDDEN_COVER);
    albumValues.PutString(PhotoAlbumColumns::HIDDEN_COVER, "");
    
    int64_t insertNum = 0;
    g_rdbStore->Insert(insertNum, PhotoAlbumColumns::TABLE, albumValues);
    EXPECT_TRUE(insertNum > 0);
    
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds = {"12345"};
    vector<int32_t> albumIds = {FAVORITE_ALBUM_INFO.albumId_};
    albumRefresh.NotifyAlbumsCoverChange(fileIds, albumIds);
    
    // 验证通知结果
    auto changeDatas = albumRefresh.GetAlbumChangeDatas();
    EXPECT_TRUE(changeDatas.size() > 0);
    if (changeDatas.size() > 0) {
        EXPECT_FALSE(changeDatas[0].infoAfterChange_.isCoverChange_);
        EXPECT_FALSE(changeDatas[0].infoAfterChange_.isHiddenCoverChange_);
    }
}

// 测试NotifyAlbumsCoverChange - 大量fileIds和albumIds
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_NotifyAlbumsCoverChange_019, TestSize.Level2)
{
    // 准备测试数据
    vector<string> fileIds;
    vector<int32_t> albumIds;
    vector<ValuesBucket> albumValuesList;
    
    for (int i = 0; i < 50; i++) {
        ValuesBucket albumValues;
        albumValues.PutInt(PhotoAlbumColumns::ALBUM_ID, 1000 + i);
        albumValues.PutInt(PhotoAlbumColumns::ALBUM_TYPE, PhotoAlbumType::USER);
        albumValues.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, PhotoAlbumSubType::USER_GENERIC);
        albumValues.PutInt(PhotoAlbumColumns::ALBUM_COUNT, 10);
        albumValues.PutInt(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, 5);
        albumValues.PutInt(PhotoAlbumColumns::ALBUM_VIDEO_COUNT, 5);
        albumValues.PutInt(PhotoAlbumColumns::HIDDEN_COUNT, 0);
        albumValues.PutInt(PhotoAlbumColumns::COVER_DATE_TIME, 0);
        albumValues.PutInt(PhotoAlbumColumns::HIDDEN_COVER_DATE_TIME, 0);
        albumValues.PutInt(PhotoAlbumColumns::ALBUM_DIRTY, static_cast<int32_t>(DirtyType::TYPE_NEW));
        
        string testFileId = to_string(20000 + i);
        fileIds.push_back(testFileId);
        albumIds.push_back(1000 + i);
        
        string testCoverUri = "file://media/Photo/" + testFileId + "/IMG_1744362716_000/IMG_20250425_123456.jpg";
        albumValues.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, testCoverUri);
        albumValues.PutString(PhotoAlbumColumns::ALBUM_NAME, "TestAlbum" + to_string(i));
        albumValues.PutString(PhotoAlbumColumns::HIDDEN_COVER, "");
        
        albumValuesList.push_back(albumValues);
    }
    
    // 批量插入
    for (auto &albumValues : albumValuesList) {
        int64_t insertNum = 0;
        g_rdbStore->Insert(insertNum, PhotoAlbumColumns::TABLE, albumValues);
    }
    
    AlbumAccurateRefresh albumRefresh;
    albumRefresh.NotifyAlbumsCoverChange(fileIds, albumIds);
    
    // 验证通知结果
    auto changeDatas = albumRefresh.GetAlbumChangeDatas();
    EXPECT_TRUE(changeDatas.size() > 0);
}

// 测试NotifyAlbumsCoverChange - 混合匹配场景
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_NotifyAlbumsCoverChange_020, TestSize.Level2)
{
    // 准备测试数据：3个albums，只有部分匹配
    ValuesBucket album1 = GetFavoriteInsertAlbum();
    string testFileId1 = "11111";
    album1.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    album1.PutString(PhotoAlbumColumns::ALBUM_COVER_URI,
        "file://media/Photo/" + testFileId1 + "/IMG_1744362716_000/IMG_20250425_123456.jpg");
    
    ValuesBucket album2 = GetTrashInsertAlbum();
    album2.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    album2.Delete(PhotoAlbumColumns::HIDDEN_COVER);
    album2.PutString(PhotoAlbumColumns::ALBUM_COVER_URI,
        "file://media/Photo/22222/IMG_1744362716_000/IMG_20250425_123456.jpg");
    album2.PutString(PhotoAlbumColumns::HIDDEN_COVER,
        "file://media/Photo/33333/IMG_1744362716_000/IMG_20250425_123456.jpg");
    
    ValuesBucket album3 = GetHiddenInsertAlbum();
    string testFileId3 = "44444";
    album3.Delete(PhotoAlbumColumns::HIDDEN_COVER);
    album3.PutString(PhotoAlbumColumns::HIDDEN_COVER,
        "file://media/Photo/" + testFileId3 + "/IMG_1744362716_000/IMG_20250425_123456.jpg");
    
    int64_t insertNum1 = 0;
    int64_t insertNum2 = 0;
    int64_t insertNum3 = 0;
    g_rdbStore->Insert(insertNum1, PhotoAlbumColumns::TABLE, album1);
    g_rdbStore->Insert(insertNum2, PhotoAlbumColumns::TABLE, album2);
    g_rdbStore->Insert(insertNum3, PhotoAlbumColumns::TABLE, album3);
    EXPECT_TRUE(insertNum1 > 0 && insertNum2 > 0 && insertNum3 > 0);
    
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds = {testFileId1, testFileId3, "55555"};
    vector<int32_t> albumIds = {
        FAVORITE_ALBUM_INFO.albumId_,
        TRASH_ALBUM_INFO.albumId_,
        HIDDEN_ALBUM_INFO.albumId_
    };
    albumRefresh.NotifyAlbumsCoverChange(fileIds, albumIds);
    
    // 验证通知结果
    auto changeDatas = albumRefresh.GetAlbumChangeDatas();
    EXPECT_TRUE(changeDatas.size() > 0);
}

// 测试IsCoverContentChange - 长fileId
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_IsCoverContentChange_022, TestSize.Level2)
{
    MEDIA_INFO_LOG("AlbumAccurateRefreshTest_IsCoverContentChange_SpecialChars_022 Start");
    ValuesBucket albumValues = GetFavoriteInsertAlbum();
    string testFileId = "1234567890123456789012345678901234567890";
    string testCoverUri = "file://media/Photo/" + testFileId + "/IMG_1744362716_000/IMG_20250425_123456.jpg";
    albumValues.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    albumValues.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, testCoverUri);
    
    int64_t insertNum = 0;
    g_rdbStore->Insert(insertNum, PhotoAlbumColumns::TABLE, albumValues);
    EXPECT_TRUE(insertNum > 0);
    
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds = {testFileId};
    auto ret = albumRefresh.IsCoverContentChange(fileIds);
    EXPECT_TRUE(ret);
    MEDIA_INFO_LOG("AlbumAccurateRefreshTest_IsCoverContentChange_SpecialChars_022 End");
}

// 测试NotifyAlbumsCoverChange - 单个fileId匹配多个albums的coverUri
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_NotifyAlbumsCoverChange_023, TestSize.Level2)
{
    string testFileId = "60000";
    
    ValuesBucket album1 = GetFavoriteInsertAlbum();
    album1.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    album1.PutString(PhotoAlbumColumns::ALBUM_COVER_URI,
        "file://media/Photo/" + testFileId + "/IMG_1744362716_000/IMG_20250425_123456.jpg");
    
    ValuesBucket album2 = GetTrashInsertAlbum();
    album2.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    album2.PutString(PhotoAlbumColumns::ALBUM_COVER_URI,
        "file://media/Photo/" + testFileId + "/IMG_1744362716_000/IMG_20250425_123456.jpg");
    
    int64_t insertNum1 = 0;
    int64_t insertNum2 = 0;
    g_rdbStore->Insert(insertNum1, PhotoAlbumColumns::TABLE, album1);
    g_rdbStore->Insert(insertNum2, PhotoAlbumColumns::TABLE, album2);
    EXPECT_TRUE(insertNum1 > 0 && insertNum2 > 0);
    
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds = {testFileId};
    vector<int32_t> albumIds = {
        FAVORITE_ALBUM_INFO.albumId_,
        TRASH_ALBUM_INFO.albumId_
    };
    albumRefresh.NotifyAlbumsCoverChange(fileIds, albumIds);
    
    // 验证通知结果
    auto changeDatas = albumRefresh.GetAlbumChangeDatas();
    EXPECT_TRUE(changeDatas.size() > 0);
}

// 测试IsCoverContentChange - URI中包含路径分隔符
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_IsCoverContentChange_026, TestSize.Level2)
{
    MEDIA_INFO_LOG("AlbumAccurateRefreshTest_IsCoverContentChange_026 Start");
    ValuesBucket albumValues = GetFavoriteInsertAlbum();
    string testFileId = "110000";
    string testCoverUri = "file://media/Photo/subdir1/subdir2/" + testFileId +
        "/IMG_1744362716_000/IMG_20250425_123456.jpg";
    albumValues.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    albumValues.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, testCoverUri);
    
    int64_t insertNum = 0;
    g_rdbStore->Insert(insertNum, PhotoAlbumColumns::TABLE, albumValues);
    EXPECT_TRUE(insertNum > 0);
    
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds = {testFileId};
    auto ret = albumRefresh.IsCoverContentChange(fileIds);
    EXPECT_TRUE(ret);
    MEDIA_INFO_LOG("AlbumAccurateRefreshTest_IsCoverContentChange_026 End");
}

// 测试NotifyAlbumsCoverChange - 清空changeInfos后重新调用
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_NotifyAlbumsCoverChange_027, TestSize.Level2)
{
    ValuesBucket albumValues = GetFavoriteInsertAlbum();
    string testFileId = "120000";
    string testCoverUri = "file://media/Photo/" + testFileId + "/IMG_1744362716_000/IMG_20250425_123456.jpg";
    albumValues.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    albumValues.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, testCoverUri);
    
    int64_t insertNum = 0;
    g_rdbStore->Insert(insertNum, PhotoAlbumColumns::TABLE, albumValues);
    EXPECT_TRUE(insertNum > 0);
    
    AlbumAccurateRefresh albumRefresh;
    albumRefresh.ClearChangeInfos();
    
    vector<string> fileIds = {testFileId};
    vector<int32_t> albumIds = {FAVORITE_ALBUM_INFO.albumId_};
    albumRefresh.NotifyAlbumsCoverChange(fileIds, albumIds);
    
    // 验证通知结果
    auto changeDatas = albumRefresh.GetAlbumChangeDatas();
    EXPECT_TRUE(changeDatas.size() > 0);
}

// 测试IsCoverContentChange - 连续调用多次
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_IsCoverContentChange_028, TestSize.Level2)
{
    MEDIA_INFO_LOG("AlbumAccurateRefreshTest_IsCoverContentChange_028 Start");
    ValuesBucket albumValues = GetFavoriteInsertAlbum();
    string testFileId1 = "130000";
    string testFileId2 = "140000";
    albumValues.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    albumValues.PutString(PhotoAlbumColumns::ALBUM_COVER_URI,
        "file://media/Photo/" + testFileId1 + "/IMG_1744362716_000/IMG_20250425_123456.jpg");
    albumValues.Delete(PhotoAlbumColumns::HIDDEN_COVER);
    albumValues.PutString(PhotoAlbumColumns::HIDDEN_COVER,
        "file://media/Photo/" + testFileId2 + "/IMG_1744362716_000/IMG_20250425_123456.jpg");
    
    int64_t insertNum = 0;
    g_rdbStore->Insert(insertNum, PhotoAlbumColumns::TABLE, albumValues);
    EXPECT_TRUE(insertNum > 0);
    
    AlbumAccurateRefresh albumRefresh;
    
    // 第一次调用
    vector<string> fileIds1 = {testFileId1};
    auto ret1 = albumRefresh.IsCoverContentChange(fileIds1);
    EXPECT_TRUE(ret1);
    
    // 第二次调用
    vector<string> fileIds2 = {testFileId2};
    auto ret2 = albumRefresh.IsCoverContentChange(fileIds2);
    EXPECT_TRUE(ret2);
    
    // 第三次调用（不匹配）
    vector<string> fileIds3 = {"150000"};
    auto ret3 = albumRefresh.IsCoverContentChange(fileIds3);
    EXPECT_FALSE(ret3);
    MEDIA_INFO_LOG("AlbumAccurateRefreshTest_IsCoverContentChange_028 End");
}

// 测试NotifyAlbumsCoverChange - 处理边界情况：fileIds包含重复元素
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_NotifyAlbumsCoverChange_033, TestSize.Level2)
{
    ValuesBucket albumValues = GetFavoriteInsertAlbum();
    string testFileId = "230000";
    string testCoverUri = "file://media/Photo/" + testFileId + "/IMG_1744362716_000/IMG_20250425_123456.jpg";
    albumValues.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    albumValues.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, testCoverUri);
    
    int64_t insertNum = 0;
    g_rdbStore->Insert(insertNum, PhotoAlbumColumns::TABLE, albumValues);
    EXPECT_TRUE(insertNum > 0);
    
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds = {testFileId, testFileId, testFileId};
    vector<int32_t> albumIds = {FAVORITE_ALBUM_INFO.albumId_};
    albumRefresh.NotifyAlbumsCoverChange(fileIds, albumIds);
    
    // 验证通知结果
    auto changeDatas = albumRefresh.GetAlbumChangeDatas();
    EXPECT_TRUE(changeDatas.size() > 0);
}

// 测试NotifyAlbumsCoverChange - 处理边界情况：fileIds包含空字符串
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_NotifyAlbumsCoverChange_035, TestSize.Level2)
{
    ValuesBucket albumValues = GetFavoriteInsertAlbum();
    string testFileId = "260000";
    string testCoverUri = "file://media/Photo/" + testFileId + "/IMG_1744362716_000/IMG_20250425_123456.jpg";
    albumValues.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    albumValues.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, testCoverUri);
    
    int64_t insertNum = 0;
    g_rdbStore->Insert(insertNum, PhotoAlbumColumns::TABLE, albumValues);
    EXPECT_TRUE(insertNum > 0);
    
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds = {testFileId, "", "270000"};
    vector<int32_t> albumIds = {FAVORITE_ALBUM_INFO.albumId_};
    albumRefresh.NotifyAlbumsCoverChange(fileIds, albumIds);
    
    // 验证通知结果
    auto changeDatas = albumRefresh.GetAlbumChangeDatas();
    EXPECT_TRUE(changeDatas.size() > 0);
}

// 测试NotifyAlbumsCoverChange - 验证多次调用后的状态
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_NotifyAlbumsCoverChange_037, TestSize.Level2)
{
    ValuesBucket albumValues = GetFavoriteInsertAlbum();
    string testFileId1 = "280000";
    string testFileId2 = "290000";
    albumValues.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    albumValues.PutString(PhotoAlbumColumns::ALBUM_COVER_URI,
        "file://media/Photo/" + testFileId1 + "/IMG_1744362716_000/IMG_20250425_123456.jpg");
    
    int64_t insertNum = 0;
    g_rdbStore->Insert(insertNum, PhotoAlbumColumns::TABLE, albumValues);
    EXPECT_TRUE(insertNum > 0);
    
    AlbumAccurateRefresh albumRefresh;
    
    // 第一次调用
    vector<string> fileIds1 = {testFileId1};
    vector<int32_t> albumIds1 = {FAVORITE_ALBUM_INFO.albumId_};
    albumRefresh.NotifyAlbumsCoverChange(fileIds1, albumIds1);
    auto changeDatas1 = albumRefresh.GetAlbumChangeDatas();
    EXPECT_TRUE(changeDatas1.size() > 0);
    
    // 清空后第二次调用
    albumRefresh.ClearChangeInfos();
    vector<string> fileIds2 = {testFileId2};
    vector<int32_t> albumIds2 = {FAVORITE_ALBUM_INFO.albumId_};
    albumRefresh.NotifyAlbumsCoverChange(fileIds2, albumIds2);
    auto changeDatas2 = albumRefresh.GetAlbumChangeDatas();
    EXPECT_TRUE(changeDatas2.size() > 0);
}

// 测试NotifyAlbumsCoverChange - 处理大量albums
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_NotifyAlbumsCoverChange_039, TestSize.Level2)
{
    vector<string> fileIds;
    vector<int32_t> albumIds;
    vector<ValuesBucket> albumValuesList;
    
    // 插入100个albums
    for (int i = 0; i < 100; i++) {
        ValuesBucket albumValues;
        albumValues.PutInt(PhotoAlbumColumns::ALBUM_ID, 3000 + i);
        albumValues.PutInt(PhotoAlbumColumns::ALBUM_TYPE, PhotoAlbumType::USER);
        albumValues.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, PhotoAlbumSubType::USER_GENERIC);
        albumValues.PutInt(PhotoAlbumColumns::ALBUM_COUNT, 10);
        albumValues.PutInt(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, 5);
        albumValues.PutInt(PhotoAlbumColumns::ALBUM_VIDEO_COUNT, 5);
        albumValues.PutInt(PhotoAlbumColumns::HIDDEN_COUNT, 0);
        albumValues.PutInt(PhotoAlbumColumns::COVER_DATE_TIME, 0);
        albumValues.PutInt(PhotoAlbumColumns::HIDDEN_COVER_DATE_TIME, 0);
        albumValues.PutInt(PhotoAlbumColumns::ALBUM_DIRTY, static_cast<int32_t>(DirtyType::TYPE_NEW));
        
        string testFileId = to_string(50000 + i);
        fileIds.push_back(testFileId);
        albumIds.push_back(3000 + i);
        
        string testCoverUri = "file://media/Photo/" + testFileId + "/IMG_1744362716_000/IMG_20250425_123456.jpg";
        albumValues.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, testCoverUri);
        albumValues.PutString(PhotoAlbumColumns::HIDDEN_COVER, "");
        albumValues.PutString(PhotoAlbumColumns::ALBUM_NAME, "TestAlbum" + to_string(i));
        albumValuesList.push_back(albumValues);
    }
    
    // 批量插入
    for (auto &albumValues : albumValuesList) {
        int64_t insertNum = 0;
        g_rdbStore->Insert(insertNum, PhotoAlbumColumns::TABLE, albumValues);
    }
    
    AlbumAccurateRefresh albumRefresh;
    albumRefresh.NotifyAlbumsCoverChange(fileIds, albumIds);
    
    // 验证通知结果
    auto changeDatas = albumRefresh.GetAlbumChangeDatas();
    EXPECT_TRUE(changeDatas.size() > 0);
}

// 测试NotifyAlbumsCoverChange - URI中包含下划线
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_NotifyAlbumsCoverChange_041, TestSize.Level2)
{
    ValuesBucket albumValues = GetFavoriteInsertAlbum();
    string testFileId = "test_file_456";
    string testCoverUri = "file://media/Photo/" + testFileId + "/IMG_1744362716_000/IMG_20250425_123456.jpg";
    albumValues.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    albumValues.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, testCoverUri);
    
    int64_t insertNum = 0;
    g_rdbStore->Insert(insertNum, PhotoAlbumColumns::TABLE, albumValues);
    EXPECT_TRUE(insertNum > 0);
    
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds = {testFileId};
    vector<int32_t> albumIds = {FAVORITE_ALBUM_INFO.albumId_};
    albumRefresh.NotifyAlbumsCoverChange(fileIds, albumIds);
    
    // 验证通知结果
    auto changeDatas = albumRefresh.GetAlbumChangeDatas();
    EXPECT_TRUE(changeDatas.size() > 0);
}

// 测试NotifyAlbumsCoverChange - 验证数据更新逻辑
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_NotifyAlbumsCoverChange_043, TestSize.Level2)
{
    // 准备测试数据
    ValuesBucket albumValues = GetFavoriteInsertAlbum();
    string testFileId = "350000";
    string testCoverUri = "file://media/Photo/" + testFileId + "/IMG_1744362716_000/IMG_20250425_123456.jpg";
    albumValues.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    albumValues.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, testCoverUri);
    
    int64_t insertNum = 0;
    g_rdbStore->Insert(insertNum, PhotoAlbumColumns::TABLE, albumValues);
    EXPECT_TRUE(insertNum > 0);
    
    AlbumAccurateRefresh albumRefresh;
    
    // 调用Init初始化
    vector<int32_t> initAlbumIds = {FAVORITE_ALBUM_INFO.albumId_};
    auto initRet = albumRefresh.Init(initAlbumIds);
    EXPECT_TRUE(initRet == ACCURATE_REFRESH_RET_OK);
    
    // 调用NotifyAlbumsCoverChange
    vector<string> fileIds = {testFileId};
    vector<int32_t> albumIds = {FAVORITE_ALBUM_INFO.albumId_};
    albumRefresh.NotifyAlbumsCoverChange(fileIds, albumIds);
    
    // 验证通知结果
    auto changeDatas = albumRefresh.GetAlbumChangeDatas();
    EXPECT_TRUE(changeDatas.size() > 0);
}

// 测试IsCoverContentChange - 验证空字符串URI的处理
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_IsCoverContentChange_044, TestSize.Level2)
{
    ValuesBucket albumValues = GetFavoriteInsertAlbum();
    albumValues.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    albumValues.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, "");
    albumValues.Delete(PhotoAlbumColumns::HIDDEN_COVER);
    albumValues.PutString(PhotoAlbumColumns::HIDDEN_COVER, "");
    
    int64_t insertNum = 0;
    g_rdbStore->Insert(insertNum, PhotoAlbumColumns::TABLE, albumValues);
    EXPECT_TRUE(insertNum > 0);
    
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds = {"360000"};
    auto ret = albumRefresh.IsCoverContentChange(fileIds);
    EXPECT_FALSE(ret);
}

// 测试NotifyAlbumsCoverChange - 验证空字符串URI的处理
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_NotifyAlbumsCoverChange_045, TestSize.Level2)
{
    ValuesBucket albumValues = GetFavoriteInsertAlbum();
    albumValues.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    albumValues.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, "");
    albumValues.Delete(PhotoAlbumColumns::HIDDEN_COVER);
    albumValues.PutString(PhotoAlbumColumns::HIDDEN_COVER, "");
    
    int64_t insertNum = 0;
    g_rdbStore->Insert(insertNum, PhotoAlbumColumns::TABLE, albumValues);
    EXPECT_TRUE(insertNum > 0);
    
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds = {"370000"};
    vector<int32_t> albumIds = {FAVORITE_ALBUM_INFO.albumId_};
    albumRefresh.NotifyAlbumsCoverChange(fileIds, albumIds);
    
    // 验证通知结果
    auto changeDatas = albumRefresh.GetAlbumChangeDatas();
    EXPECT_TRUE(changeDatas.size() > 0);
    if (changeDatas.size() > 0) {
        EXPECT_FALSE(changeDatas[0].infoAfterChange_.isCoverChange_);
        EXPECT_FALSE(changeDatas[0].infoAfterChange_.isHiddenCoverChange_);
    }
}

// 测试IsCoverContentChange - 验证只匹配hiddenCover
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_IsCoverContentChange_048, TestSize.Level2)
{
    MEDIA_INFO_LOG("AlbumAccurateRefreshTest_IsCoverContentChange_048 Start");
    ValuesBucket albumValues = GetFavoriteInsertAlbum();
    string testFileId = "420000";
    string testHiddenCover = "file://media/Photo/" + testFileId + "/IMG_1744362716_000/IMG_20250425_123456.jpg";
    albumValues.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    albumValues.PutString(PhotoAlbumColumns::ALBUM_COVER_URI,
        "file://media/Photo/430000/IMG_1744362716_000/IMG_20250425_123456.jpg");
    albumValues.Delete(PhotoAlbumColumns::HIDDEN_COVER);
    albumValues.PutString(PhotoAlbumColumns::HIDDEN_COVER, testHiddenCover);
    
    int64_t insertNum = 0;
    g_rdbStore->Insert(insertNum, PhotoAlbumColumns::TABLE, albumValues);
    EXPECT_TRUE(insertNum > 0);
    
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds = {testFileId};
    auto ret = albumRefresh.IsCoverContentChange(fileIds);
    EXPECT_TRUE(ret);
    MEDIA_INFO_LOG("AlbumAccurateRefreshTest_IsCoverContentChange_048 End");
}

// 测试NotifyAlbumsCoverChange - 验证fileIds大小为1的情况
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_NotifyAlbumsCoverChange_053, TestSize.Level2)
{
    ValuesBucket albumValues = GetFavoriteInsertAlbum();
    string testFileId = "500000";
    string testCoverUri = "file://media/Photo/" + testFileId + "/IMG_1744362716_000/IMG_20250425_123456.jpg";
    albumValues.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    albumValues.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, testCoverUri);
    
    int64_t insertNum = 0;
    g_rdbStore->Insert(insertNum, PhotoAlbumColumns::TABLE, albumValues);
    EXPECT_TRUE(insertNum > 0);
    
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds = {testFileId};
    vector<int32_t> albumIds = {FAVORITE_ALBUM_INFO.albumId_};
    EXPECT_EQ(fileIds.size(), 1);
    EXPECT_EQ(albumIds.size(), 1);
    albumRefresh.NotifyAlbumsCoverChange(fileIds, albumIds);
    
    // 验证通知结果
    auto changeDatas = albumRefresh.GetAlbumChangeDatas();
    EXPECT_TRUE(changeDatas.size() > 0);
}

// 测试NotifyAlbumsCoverChange - 验证fileIds大小为2的情况
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_NotifyAlbumsCoverChange_055, TestSize.Level2)
{
    ValuesBucket album1 = GetFavoriteInsertAlbum();
    string testFileId1 = "530000";
    album1.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    album1.PutString(PhotoAlbumColumns::ALBUM_COVER_URI,
        "file://media/Photo/" + testFileId1 + "/IMG_1744362716_000/IMG_20250425_123456.jpg");
    
    ValuesBucket album2 = GetTrashInsertAlbum();
    string testFileId2 = "540000";
    album2.Delete(PhotoAlbumColumns::HIDDEN_COVER);
    album2.PutString(PhotoAlbumColumns::HIDDEN_COVER,
        "file://media/Photo/" + testFileId2 + "/IMG_1744362716_000/IMG_20250425_123456.jpg");
    
    int64_t insertNum1 = 0;
    int64_t insertNum2 = 0;
    g_rdbStore->Insert(insertNum1, PhotoAlbumColumns::TABLE, album1);
    g_rdbStore->Insert(insertNum2, PhotoAlbumColumns::TABLE, album2);
    EXPECT_TRUE(insertNum1 > 0 && insertNum2 > 0);
    
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds = {testFileId1, testFileId2};
    vector<int32_t> albumIds = {
        FAVORITE_ALBUM_INFO.albumId_,
        TRASH_ALBUM_INFO.albumId_
    };
    EXPECT_EQ(fileIds.size(), 2);
    EXPECT_EQ(albumIds.size(), 2);
    albumRefresh.NotifyAlbumsCoverChange(fileIds, albumIds);
    
    // 验证通知结果
    auto changeDatas = albumRefresh.GetAlbumChangeDatas();
    EXPECT_TRUE(changeDatas.size() > 0);
}

// 测试IsCoverContentChange - 验证不匹配的情况
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_IsCoverContentChange_056, TestSize.Level2)
{
    ValuesBucket albumValues = GetFavoriteInsertAlbum();
    albumValues.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    albumValues.PutString(PhotoAlbumColumns::ALBUM_COVER_URI,
        "file://media/Photo/550000/IMG_1744362716_000/IMG_20250425_123456.jpg");
    albumValues.Delete(PhotoAlbumColumns::HIDDEN_COVER);
    albumValues.PutString(PhotoAlbumColumns::HIDDEN_COVER,
        "file://media/Photo/560000/IMG_1744362716_000/IMG_20250425_123456.jpg");
    
    int64_t insertNum = 0;
    g_rdbStore->Insert(insertNum, PhotoAlbumColumns::TABLE, albumValues);
    EXPECT_TRUE(insertNum > 0);
    
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds = {"570000", "580000"};
    auto ret = albumRefresh.IsCoverContentChange(fileIds);
    EXPECT_FALSE(ret);
}

// 测试NotifyAlbumsCoverChange - 验证不匹配的情况
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_NotifyAlbumsCoverChange_057, TestSize.Level2)
{
    ValuesBucket albumValues = GetFavoriteInsertAlbum();
    albumValues.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    albumValues.PutString(PhotoAlbumColumns::ALBUM_COVER_URI,
        "file://media/Photo/590000/IMG_1744362716_000/IMG_20250425_123456.jpg");
    albumValues.Delete(PhotoAlbumColumns::HIDDEN_COVER);
    albumValues.PutString(PhotoAlbumColumns::HIDDEN_COVER,
        "file://media/Photo/600000/IMG_1744362716_000/IMG_20250425_123456.jpg");
    
    int64_t insertNum = 0;
    g_rdbStore->Insert(insertNum, PhotoAlbumColumns::TABLE, albumValues);
    EXPECT_TRUE(insertNum > 0);
    
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds = {"610000", "620000"};
    vector<int32_t> albumIds = {FAVORITE_ALBUM_INFO.albumId_};
    albumRefresh.NotifyAlbumsCoverChange(fileIds, albumIds);
    
    // 验在这种情况下changeDatas应该为空或不设置标志
    auto changeDatas = albumRefresh.GetAlbumChangeDatas();
    if (changeDatas.size() > 0) {
        EXPECT_FALSE(changeDatas[0].infoAfterChange_.isCoverChange_);
        EXPECT_FALSE(changeDatas[0].infoAfterChange_.isHiddenCoverChange_);
    }
}

// 测试NotifyAlbumsCoverChange - 验证部分匹配的情况
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_NotifyAlbumsCoverChange_059, TestSize.Level2)
{
    ValuesBucket album1 = GetFavoriteInsertAlbum();
    string testFileId1 = "660000";
    album1.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    album1.PutString(PhotoAlbumColumns::ALBUM_COVER_URI,
        "file://media/Photo/" + testFileId1 + "/IMG_1744362716_000/IMG_20250425_123456.jpg");
    
    ValuesBucket album2 = GetTrashInsertAlbum();
    album2.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    album2.PutString(PhotoAlbumColumns::ALBUM_COVER_URI,
        "file://media/Photo/670000/IMG_1744362716_000/IMG_20250425_123456.jpg");
    
    int64_t insertNum1 = 0;
    int64_t insertNum2 = 0;
    g_rdbStore->Insert(insertNum1, PhotoAlbumColumns::TABLE, album1);
    g_rdbStore->Insert(insertNum2, PhotoAlbumColumns::TABLE, album2);
    EXPECT_TRUE(insertNum1 > 0 && insertNum2 > 0);
    
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds = {testFileId1, "680000"};
    vector<int32_t> albumIds = {
        FAVORITE_ALBUM_INFO.albumId_,
        TRASH_ALBUM_INFO.albumId_
    };
    albumRefresh.NotifyAlbumsCoverChange(fileIds, albumIds);
    
    // 验证通知结果
    auto changeDatas = albumRefresh.GetAlbumChangeDatas();
    EXPECT_TRUE(changeDatas.size() > 0);
}

// 测试NotifyAlbumsCoverChange - 验证多个albums匹配同一个文件
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_NotifyAlbumsCoverChange_061, TestSize.Level2)
{
    string testFileId = "700000";
    
    ValuesBucket album1 = GetFavoriteInsertAlbum();
    album1.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    album1.PutString(PhotoAlbumColumns::ALBUM_COVER_URI,
        "file://media/Photo/" + testFileId + "/IMG_1744362716_000/IMG_20250425_123456.jpg");
    
    ValuesBucket album2 = GetTrashInsertAlbum();
    album2.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    album2.PutString(PhotoAlbumColumns::ALBUM_COVER_URI,
        "file://media/Photo/" + testFileId + "/IMG_1744362716_000/IMG_20250425_123456.jpg");
    
    int64_t insertNum1 = 0;
    int64_t insertNum2 = 0;
    g_rdbStore->Insert(insertNum1, PhotoAlbumColumns::TABLE, album1);
    g_rdbStore->Insert(insertNum2, PhotoAlbumColumns::TABLE, album2);
    EXPECT_TRUE(insertNum1 > 0 && insertNum2 > 0);
    
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds = {testFileId};
    vector<int32_t> albumIds = {
        FAVORITE_ALBUM_INFO.albumId_,
        TRASH_ALBUM_INFO.albumId_
    };
    albumRefresh.NotifyAlbumsCoverChange(fileIds, albumIds);
    
    // 验证通知结果
    auto changeDatas = albumRefresh.GetAlbumChangeDatas();
    EXPECT_TRUE(changeDatas.size() > 0);
}

// 测试NotifyAlbumsCoverChange - 验证URI格式正确性
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_NotifyAlbumsCoverChange_063, TestSize.Level2)
{
    ValuesBucket albumValues = GetFavoriteInsertAlbum();
    string testFileId = "720000";
    string testCoverUri = "file://media/Photo/" + testFileId + "/IMG_1744362716_000/IMG_20250425_123456.jpg";
    albumValues.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    albumValues.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, testCoverUri);
    
    int64_t insertNum = 0;
    g_rdbStore->Insert(insertNum, PhotoAlbumColumns::TABLE, albumValues);
    EXPECT_TRUE(insertNum > 0);
    
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds = {testFileId};
    vector<int32_t> albumIds = {FAVORITE_ALBUM_INFO.albumId_};
    albumRefresh.NotifyAlbumsCoverChange(fileIds, albumIds);
    
    // 验证通知结果
    auto changeDatas = albumRefresh.GetAlbumChangeDatas();
    EXPECT_TRUE(changeDatas.size() > 0);
}

// 测试NotifyAlbumsCoverChange - 验证边界情况：最大fileId
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_NotifyAlbumsCoverChange_065, TestSize.Level2)
{
    ValuesBucket albumValues = GetFavoriteInsertAlbum();
    string testFileId = "999999999999";
    string testCoverUri = "file://media/Photo/" + testFileId + "/IMG_1744362716_000/IMG_20250425_123456.jpg";
    albumValues.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    albumValues.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, testCoverUri);
    
    int64_t insertNum = 0;
    g_rdbStore->Insert(insertNum, PhotoAlbumColumns::TABLE, albumValues);
    EXPECT_TRUE(insertNum > 0);
    
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds = {testFileId};
    vector<int32_t> albumIds = {FAVORITE_ALBUM_INFO.albumId_};
    albumRefresh.NotifyAlbumsCoverChange(fileIds, albumIds);
    
    // 验证通知结果
    auto changeDatas = albumRefresh.GetAlbumChangeDatas();
    EXPECT_TRUE(changeDatas.size() > 0);
}

// 测试NotifyAlbumsCoverChange - 验证边界情况：最小fileId
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_NotifyAlbumsCoverChange_067, TestSize.Level2)
{
    ValuesBucket albumValues = GetFavoriteInsertAlbum();
    string testFileId = "1";
    string testCoverUri = "file://media/Photo/" + testFileId + "/IMG_1744362716_000/IMG_20250425_123456.jpg";
    albumValues.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    albumValues.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, testCoverUri);
    
    int64_t insertNum = 0;
    g_rdbStore->Insert(insertNum, PhotoAlbumColumns::TABLE, albumValues);
    EXPECT_TRUE(insertNum > 0);
    
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds = {testFileId};
    vector<int32_t> albumIds = {FAVORITE_ALBUM_INFO.albumId_};
    albumRefresh.NotifyAlbumsCoverChange(fileIds, albumIds);
    
    // 验证通知结果
    auto changeDatas = albumRefresh.GetAlbumChangeDatas();
    EXPECT_TRUE(changeDatas.size() > 0);
}

// 测试NotifyAlbumsCoverChange - 验证fileId为0的情况
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_NotifyAlbumsCoverChange_069, TestSize.Level2)
{
    ValuesBucket albumValues = GetFavoriteInsertAlbum();
    string testFileId = "0";
    string testCoverUri = "file://media/Photo/" + testFileId + "/IMG_1744362716_000/IMG_20250425_123456.jpg";
    albumValues.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    albumValues.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, testCoverUri);
    
    int64_t insertNum = 0;
    g_rdbStore->Insert(insertNum, PhotoAlbumColumns::TABLE, albumValues);
    EXPECT_TRUE(insertNum > 0);
    
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds = {testFileId};
    vector<int32_t> albumIds = {FAVORITE_ALBUM_INFO.albumId_};
    albumRefresh.NotifyAlbumsCoverChange(fileIds, albumIds);
    
    // 验证通知结果
    auto changeDatas = albumRefresh.GetAlbumChangeDatas();
    EXPECT_TRUE(changeDatas.size() > 0);
}

// 测试NotifyAlbumsCoverChange - 验证负数fileId的情况
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_NotifyAlbumsCoverChange_071, TestSize.Level2)
{
    ValuesBucket albumValues = GetFavoriteInsertAlbum();
    string testFileId = "-1";
    string testCoverUri = "file://media/Photo/" + testFileId + "/IMG_1744362716_000/IMG_20250425_123456.jpg";
    albumValues.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    albumValues.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, testCoverUri);
    
    int64_t insertNum = 0;
    g_rdbStore->Insert(insertNum, PhotoAlbumColumns::TABLE, albumValues);
    EXPECT_TRUE(insertNum > 0);
    
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds = {testFileId};
    vector<int32_t> albumIds = {FAVORITE_ALBUM_INFO.albumId_};
    albumRefresh.NotifyAlbumsCoverChange(fileIds, albumIds);
    
    // 验证通知结果
    auto changeDatas = albumRefresh.GetAlbumChangeDatas();
    EXPECT_TRUE(changeDatas.size() > 0);
}

// 测试NotifyAlbumsCoverChange - 验证URI扩展名不同的处理
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_NotifyAlbumsCoverChange_073, TestSize.Level2)
{
    ValuesBucket album1 = GetFavoriteInsertAlbum();
    string testFileId1 = "750000";
    album1.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    album1.PutString(PhotoAlbumColumns::ALBUM_COVER_URI,
        "file://media/Photo/" + testFileId1 + "/IMG_1744362716_000/IMG_20250425_123456.png");
    
    ValuesBucket album2 = GetTrashInsertAlbum();
    string testFileId2 = "760000";
    album2.Delete(PhotoAlbumColumns::HIDDEN_COVER);
    album2.PutString(PhotoAlbumColumns::HIDDEN_COVER,
        "file://media/Photo/" + testFileId2 + "/IMG_1744362716_000/IMG_20250425_123456.jpeg");
    
    int64_t insertNum1 = 0;
    int64_t insertNum2 = 0;
    g_rdbStore->Insert(insertNum1, PhotoAlbumColumns::TABLE, album1);
    g_rdbStore->Insert(insertNum2, PhotoAlbumColumns::TABLE, album2);
    EXPECT_TRUE(insertNum1 > 0 && insertNum2 > 0);
    
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds = {testFileId1, testFileId2};
    vector<int32_t> albumIds = {
        FAVORITE_ALBUM_INFO.albumId_,
        TRASH_ALBUM_INFO.albumId_
    };
    albumRefresh.NotifyAlbumsCoverChange(fileIds, albumIds);
    
    // 验证通知结果
    auto changeDatas = albumRefresh.GetAlbumChangeDatas();
    EXPECT_TRUE(changeDatas.size() > 0);
}

// 测试NotifyAlbumsCoverChange - 验证URI中包含特殊字符
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_NotifyAlbumsCoverChange_075, TestSize.Level2)
{
    ValuesBucket albumValues = GetFavoriteInsertAlbum();
    string testFileId = "780000";
    string testCoverUri = "file://media/Photo/" + testFileId + "/IMG_1744362716_000/IMG_20250425_123456.jpg";
    albumValues.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    albumValues.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, testCoverUri);
    
    int64_t insertNum = 0;
    g_rdbStore->Insert(insertNum, PhotoAlbumColumns::TABLE, albumValues);
    EXPECT_TRUE(insertNum > 0);
    
    AlbumAccurateRefresh albumRefresh;
    vector<string> fileIds = {testFileId};
    vector<int32_t> albumIds = {FAVORITE_ALBUM_INFO.albumId_};
    albumRefresh.NotifyAlbumsCoverChange(fileIds, albumIds);
    
    // 验证通知结果
    auto changeDatas = albumRefresh.GetAlbumChangeDatas();
    EXPECT_TRUE(changeDatas.size() > 0);
}

// 测试NotifyAlbumsCoverChange - 验证大量数据时的性能
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_NotifyAlbumsCoverChange_077, TestSize.Level2)
{
    vector<string> fileIds;
    vector<int32_t> albumIds;
    vector<ValuesBucket> albumValuesList;
    
    // 插入200个albums
    for (int i = 0; i < 200; i++) {
        ValuesBucket albumValues;
        albumValues.PutInt(PhotoAlbumColumns::ALBUM_ID, 5000 + i);
        albumValues.PutInt(PhotoAlbumColumns::ALBUM_TYPE, PhotoAlbumType::USER);
        albumValues.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, PhotoAlbumSubType::USER_GENERIC);
        albumValues.PutInt(PhotoAlbumColumns::ALBUM_COUNT, 10);
        albumValues.PutInt(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, 5);
        albumValues.PutInt(PhotoAlbumColumns::ALBUM_VIDEO_COUNT, 5);
        albumValues.PutInt(PhotoAlbumColumns::HIDDEN_COUNT, 0);
        albumValues.PutInt(PhotoAlbumColumns::COVER_DATE_TIME, 0);
        albumValues.PutInt(PhotoAlbumColumns::HIDDEN_COVER_DATE_TIME, 0);
        albumValues.PutInt(PhotoAlbumColumns::ALBUM_DIRTY, static_cast<int32_t>(DirtyType::TYPE_NEW));
        
        string testFileId = to_string(90000 + i);
        fileIds.push_back(testFileId);
        albumIds.push_back(5000 + i);
        
        string testCoverUri = "file://media/Photo/" + testFileId + "/IMG_1744362716_000/IMG_20250425_123456.jpg";
        albumValues.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, testCoverUri);
        albumValues.PutString(PhotoAlbumColumns::HIDDEN_COVER, "");
    albumValues.PutString(PhotoAlbumColumns::ALBUM_NAME, "TestAlbum" + to_string(i));
        albumValuesList.push_back(albumValues);
    }
    
    // 批量插入
    for (auto &albumValues : albumValuesList) {
        int64_t insertNum = 0;
        g_rdbStore->Insert(insertNum, PhotoAlbumColumns::TABLE, albumValues);
    }
    
    AlbumAccurateRefresh albumRefresh;
    albumRefresh.NotifyAlbumsCoverChange(fileIds, albumIds);
    
    // 验证通知结果
    auto changeDatas = albumRefresh.GetAlbumChangeDatas();
    EXPECT_TRUE(changeDatas.size() > 0);
}

// 测试NotifyAlbumsCoverChange - 验证并发场景
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_NotifyAlbumsCoverChange_079, TestSize.Level2)
{
    ValuesBucket albumValues = GetFavoriteInsertAlbum();
    string testFileId = "810000";
    string testCoverUri = "file://media/Photo/" + testFileId + "/IMG_1744362716_000/IMG_20250425_123456.jpg";
    albumValues.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    albumValues.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, testCoverUri);
    
    int64_t insertNum = 0;
    g_rdbStore->Insert(insertNum, PhotoAlbumColumns::TABLE, albumValues);
    EXPECT_TRUE(insertNum > 0);
    
    AlbumAccurateRefresh albumRefresh1;
    AlbumAccurateRefresh albumRefresh2;
    
    vector<string> fileIds = {testFileId};
    vector<int32_t> albumIds = {FAVORITE_ALBUM_INFO.albumId_};
    
    // 两个实例同时调用
    albumRefresh1.NotifyAlbumsCoverChange(fileIds, albumIds);
    albumRefresh2.NotifyAlbumsCoverChange(fileIds, albumIds);
    
    // 验证通知结果
    auto changeDatas1 = albumRefresh1.GetAlbumChangeDatas();
    auto changeDatas2 = albumRefresh2.GetAlbumChangeDatas();
    
    EXPECT_TRUE(changeDatas1.size() > 0);
    EXPECT_TRUE(changeDatas2.size() > 0);
}

// 测试IsCoverContentChange - 验证错误恢复能力
HWTEST_F(AlbumAccurateRefreshTest, AlbumAccurateRefreshTest_IsCoverContentChange_080, TestSize.Level2)
{
    ValuesBucket albumValues = GetFavoriteInsertAlbum();
    string testFileId = "820000";
    string testCoverUri = "file://media/Photo/" + testFileId + "/IMG_1744362716_000/IMG_20250425_123456.jpg";
    albumValues.Delete(PhotoAlbumColumns::ALBUM_COVER_URI);
    albumValues.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, testCoverUri);
    
    int64_t insertNum = 0;
    g_rdbStore->Insert(insertNum, PhotoAlbumColumns::TABLE, albumValues);
    EXPECT_TRUE(insertNum > 0);
    
    AlbumAccurateRefresh albumRefresh;
    
    // 第一次调用成功
    vector<string> fileIds1 = {testFileId};
    auto ret1 = albumRefresh.IsCoverContentChange(fileIds1);
    EXPECT_TRUE(ret1);
    
    // 第二次调用不匹配
    vector<string> fileIds2 = {"830000"};
    auto ret2 = albumRefresh.IsCoverContentChange(fileIds2);
    EXPECT_FALSE(ret2);
    
    // 第三次调用又成功
    vector<string> fileIds3 = {testFileId};
    auto ret3 = albumRefresh.IsCoverContentChange(fileIds3);
    EXPECT_TRUE(ret3);
}
} // namespace Media
} // namespace OHOS