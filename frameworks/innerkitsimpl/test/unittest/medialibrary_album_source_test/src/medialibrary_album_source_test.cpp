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

#define MLOG_TAG "SourceAlbumTest"

#include "medialibrary_album_source_test.h"

#include <chrono>

#include "abs_rdb_predicates.h"
#include "fetch_result.h"
#include "file_asset.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_command.h"
#include "medialibrary_common_utils.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_data_manager_utils.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "medialibrary_inotify.h"
#include "medialibrary_photo_operations.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "photo_album_column.h"
#include "result_set_utils.h"
#include "source_album.h"
#include "uri.h"
#include "userfile_manager_types.h"
#include "values_bucket.h"
#include "vision_column.h"
#include "media_upgrade.h"

namespace OHOS {
namespace Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace OHOS::DataShare;
using OHOS::DataShare::DataShareValuesBucket;
using OHOS::DataShare::DataSharePredicates;
static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static std::atomic<int> number(0);
const int ZERO = 0;
const string RECORD_BUNDLE_NAME = "com.ohos.app";
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;

struct InsertResult {
    int64_t fileId;
    string title;
    string displayName;
};

int32_t ExecSqls(const vector<string> &sqls)
{
    EXPECT_NE((g_rdbStore == nullptr), true);
    int32_t err = E_OK;
    for (const auto &sql : sqls) {
        err = g_rdbStore->ExecuteSql(sql);
        MEDIA_INFO_LOG("exec sql: %{public}s result: %{public}d", sql.c_str(), err);
        EXPECT_EQ(err, E_OK);
    }
    return E_OK;
}

void InitSourceAlbumTrigger()
{
    static const vector<string> executeSqlStrs = {
        INSERT_PHOTO_INSERT_SOURCE_ALBUM,
        INSERT_PHOTO_UPDATE_SOURCE_ALBUM,
        CREATE_SOURCE_ALBUM_INDEX,
    };
    MEDIA_INFO_LOG("start add source album trigger");
    ExecSqls(executeSqlStrs);
}

void ClearData()
{
    string clearPhotoSql = "DELETE FROM " + PhotoColumn::PHOTOS_TABLE;
    string clearSourceAlbumSql = "DELETE FROM " + PhotoAlbumColumns::TABLE + " WHERE " +
        PhotoAlbumColumns::ALBUM_TYPE + " = " + to_string(PhotoAlbumType::SOURCE) + " AND " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + " = " + to_string(PhotoAlbumSubType::SOURCE_GENERIC);
    string initSystemAlbumSql = "UPDATE " + PhotoAlbumColumns::TABLE +
        " SET " + PhotoAlbumColumns::ALBUM_COVER_URI + " = '', " +
        PhotoAlbumColumns::ALBUM_COUNT + " = 0" +
        " WHERE " + PhotoAlbumColumns::ALBUM_TYPE + " = " + to_string(PhotoAlbumType::SYSTEM);
    vector<string> executeSqlStrs = {
        clearPhotoSql,
        clearSourceAlbumSql,
        initSystemAlbumSql,
        DROP_INSERT_PHOTO_INSERT_SOURCE_ALBUM,
        DROP_INSERT_PHOTO_UPDATE_SOURCE_ALBUM,
        DROP_UPDATE_PHOTO_UPDATE_SOURCE_ALBUM,
        DROP_DELETE_PHOTO_UPDATE_SOURCE_ALBUM,
        DROP_SOURCE_ALBUM_INDEX,
        DROP_INSERT_PHOTO_UPDATE_ALBUM_BUNDLENAME,
    };
    MEDIA_INFO_LOG("start clear data");
    ExecSqls(executeSqlStrs);
}

int GetNumber()
{
    return ++number;
}

int64_t GetTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    return seconds.count() + GetNumber();
}

string GetCoverUri(InsertResult result)
{
    return PhotoColumn::PHOTO_URI_PREFIX + to_string(result.fileId) + "/" +
        result.title + "/" + result.displayName;
}

inline void CheckColumn(shared_ptr<OHOS::NativeRdb::ResultSet> &resultSet, const string &column,
    ResultSetDataType type, const variant<int32_t, string, int64_t, double> &expected)
{
    EXPECT_EQ(ResultSetUtils::GetValFromColumn(column, resultSet, type), expected);
}

string GetTitle(int64_t &timestamp)
{
    return "IMG_" + to_string(timestamp) + "_" + to_string(GetNumber());
}

string GetDisplayName(string &title)
{
    return title + ".png";
}

InsertResult InsertPhoto(string &packageName)
{
    MEDIA_INFO_LOG("InsertPhoto packageName is: %{public}s", packageName.c_str());
    int64_t fileId = -1;
    int64_t timestamp = GetTimestamp();
    string title = GetTitle(timestamp);
    string displayName = GetDisplayName(title);
    MEDIA_INFO_LOG("title is: %{public}s, displayName is: %{public}s",
        title.c_str(), displayName.c_str());
    string data = "/storage/cloud/files/photo/1/" + GetDisplayName(title);
    ValuesBucket valuesBucket;
    valuesBucket.PutString(MediaColumn::MEDIA_FILE_PATH, data);
    valuesBucket.PutString(MediaColumn::MEDIA_TITLE, title);
    valuesBucket.PutString(MediaColumn::MEDIA_NAME, displayName);
    if (!packageName.empty()) {
        valuesBucket.PutString(MediaColumn::MEDIA_PACKAGE_NAME, packageName);
        valuesBucket.PutString(MediaColumn::MEDIA_OWNER_PACKAGE, RECORD_BUNDLE_NAME);
    }
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_ADDED, timestamp);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_MODIFIED, timestamp);
    valuesBucket.PutLong(MediaColumn::MEDIA_TIME_PENDING, ZERO);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_TRASHED, ZERO);
    valuesBucket.PutInt(MediaColumn::MEDIA_HIDDEN, ZERO);
    EXPECT_NE((g_rdbStore == nullptr), true);
    int32_t ret = g_rdbStore->Insert(fileId, PhotoColumn::PHOTOS_TABLE, valuesBucket);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("InsertPhoto fileId is %{public}s", to_string(fileId).c_str());
    InsertResult result;
    result.fileId = fileId;
    result.title = title;
    result.displayName = displayName;
    return result;
}

InsertResult InsertPhotoWithEmptyPackageName()
{
    MEDIA_INFO_LOG("InsertPhoto packageName is empty");
    int64_t fileId = -1;
    int64_t timestamp = GetTimestamp();
    string title = GetTitle(timestamp);
    string displayName = GetDisplayName(title);
    MEDIA_INFO_LOG("title is: %{public}s, displayName is: %{public}s",
        title.c_str(), displayName.c_str());
    string data = "/storage/cloud/files/photo/1/" + GetDisplayName(title);
    ValuesBucket valuesBucket;
    valuesBucket.PutString(MediaColumn::MEDIA_FILE_PATH, data);
    valuesBucket.PutString(MediaColumn::MEDIA_TITLE, title);
    valuesBucket.PutString(MediaColumn::MEDIA_NAME, displayName);
    valuesBucket.PutString(MediaColumn::MEDIA_PACKAGE_NAME, "");
    valuesBucket.PutString(MediaColumn::MEDIA_OWNER_PACKAGE, RECORD_BUNDLE_NAME);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_ADDED, timestamp);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_MODIFIED, timestamp);
    valuesBucket.PutLong(MediaColumn::MEDIA_TIME_PENDING, ZERO);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_TRASHED, ZERO);
    valuesBucket.PutInt(MediaColumn::MEDIA_HIDDEN, ZERO);
    EXPECT_NE((g_rdbStore == nullptr), true);
    int32_t ret = g_rdbStore->Insert(fileId, PhotoColumn::PHOTOS_TABLE, valuesBucket);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("InsertPhoto fileId is %{public}s", to_string(fileId).c_str());
    InsertResult result;
    result.fileId = fileId;
    result.title = title;
    result.displayName = displayName;
    return result;
}

void UpdatePhotoTrashed(int64_t &fileId, bool isDelete)
{
    ASSERT_NE(g_rdbStore, nullptr);
    int32_t changedRows = -1;
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    ValuesBucket updateValues;
    if (isDelete) {
        updateValues.PutLong(MediaColumn::MEDIA_DATE_TRASHED, GetTimestamp());
    } else {
        updateValues.PutLong(MediaColumn::MEDIA_DATE_TRASHED, ZERO);
    }
    cmd.SetValueBucket(updateValues);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    int32_t ret = g_rdbStore->Update(cmd, changedRows);
    EXPECT_EQ(ret, E_OK);
}

void HidePhoto(int64_t fileId, int value)
{
    ASSERT_NE(g_rdbStore, nullptr);
    int32_t changedRows = -1;
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    ValuesBucket updateValues;
    updateValues.PutInt(MediaColumn::MEDIA_HIDDEN, value);
    cmd.SetValueBucket(updateValues);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    int32_t ret = g_rdbStore->Update(cmd, changedRows);
    EXPECT_EQ(ret, E_OK);
}

void UpdateDisplayName(int64_t &fileId, string &displayName)
{
    ASSERT_NE(g_rdbStore, nullptr);
    int32_t changedRows = -1;
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    ValuesBucket updateValues;
    updateValues.PutString(MediaColumn::MEDIA_NAME, displayName);
    updateValues.PutLong(MediaColumn::MEDIA_DATE_MODIFIED, GetTimestamp());
    cmd.SetValueBucket(updateValues);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    int32_t ret = g_rdbStore->Update(cmd, changedRows);
    EXPECT_EQ(ret, E_OK);
}

void ValidPhotoAlbumValue(string packageName, int exceptResultCount, int exceptPhotoCount,
    string exceptCoverUri)
{
    MediaLibraryRdbUtils::UpdateSourceAlbumInternal(g_rdbStore);
    vector<string> columns = { PhotoAlbumColumns::ALBUM_COUNT, PhotoAlbumColumns::ALBUM_COVER_URI,
        PhotoAlbumColumns::ALBUM_BUNDLE_NAME};
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ALBUM, OperationType::QUERY,
        MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoAlbumColumns::ALBUM_NAME, packageName);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoAlbumColumns::ALBUM_TYPE, to_string(PhotoAlbumType::SOURCE));
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::SOURCE_GENERIC));
    ASSERT_NE(g_rdbStore, nullptr);
    auto resultSet = g_rdbStore->Query(cmd, columns);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToFirstRow(), E_OK);
    int32_t count = -1;
    int32_t ret = resultSet->GetRowCount(count);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(count, exceptResultCount);
    int photoCount = GetInt32Val(PhotoAlbumColumns::ALBUM_COUNT, resultSet);
    string coverURI = GetStringVal(PhotoAlbumColumns::ALBUM_COVER_URI, resultSet);
    string bundleName = GetStringVal(PhotoAlbumColumns::ALBUM_BUNDLE_NAME, resultSet);
    MEDIA_INFO_LOG("validPhotoAlbumValue photoCount is: %{public}d, coverURI is %{public}s",
        photoCount, coverURI.c_str());
    EXPECT_EQ(photoCount, exceptPhotoCount);
    EXPECT_EQ(coverURI, exceptCoverUri);
    EXPECT_EQ(bundleName, RECORD_BUNDLE_NAME);
}

void ValidNullPackageNameSourceAlbum()
{
    vector<string> columns = {};
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ALBUM, OperationType::QUERY,
        MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->BeginWrap();
    cmd.GetAbsRdbPredicates()->IsNull(PhotoAlbumColumns::ALBUM_NAME);
    cmd.GetAbsRdbPredicates()->Or();
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoAlbumColumns::ALBUM_NAME, "");
    cmd.GetAbsRdbPredicates()->EndWrap();
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoAlbumColumns::ALBUM_TYPE, to_string(PhotoAlbumType::SOURCE));
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::SOURCE_GENERIC));
    ASSERT_NE(g_rdbStore, nullptr);
    auto resultSet = g_rdbStore->Query(cmd, columns);
    ASSERT_NE(resultSet, nullptr);
    int32_t count = -1;
    int32_t ret = resultSet->GetRowCount(count);
    MEDIA_INFO_LOG("ValidNullPackageNameSourceAlbum count is: %{public}d", count);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(count, ZERO);
}

void DeletePhoto(int64_t &fileId)
{
    MEDIA_INFO_LOG("DeletePhoto fileId is %{public}s", to_string(fileId).c_str());
    ASSERT_NE(g_rdbStore, nullptr);
    int32_t deletedRows = -1;
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::DELETE);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    g_rdbStore->Delete(cmd, deletedRows);
    MEDIA_INFO_LOG("DeletePhoto deletedRows is %{public}d", deletedRows);
}

void MediaLibraryAlbumSourceTest::SetUpTestCase()
{
    MEDIA_INFO_LOG("MediaLibraryAlbumSourceTest SetUpTestCase start");
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    ClearData();
    InitSourceAlbumTrigger();
    number = 0;
    MEDIA_INFO_LOG("MediaLibraryAlbumSourceTest SetUpTestCase end");
}

void MediaLibraryAlbumSourceTest::TearDownTestCase()
{
    MEDIA_INFO_LOG("MediaLibraryAlbumSourceTest TearDownTestCase");
    ClearData();
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
}

void MediaLibraryAlbumSourceTest::SetUp()
{
    MEDIA_INFO_LOG("MediaLibraryAlbumSourceTest SetUp");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    rdbStore->ExecuteSql(PhotoUpgrade::INDEX_SCTHP_ADDTIME);
}

void MediaLibraryAlbumSourceTest::TearDown()
{
    MEDIA_INFO_LOG("MediaLibraryAlbumSourceTest TearDown");
}

void CheckColumnInTable(const string &columnName, const string &tableName)
{
    EXPECT_NE(g_rdbStore, nullptr);
    string querySql = "SELECT " + MEDIA_COLUMN_COUNT_1 + " FROM pragma_table_info('" + tableName + "') WHERE name = '" +
        columnName + "'";
    auto resultSet = g_rdbStore->QuerySql(querySql);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Get column count failed");
        return;
    }
    int32_t count = GetInt32Val(MEDIA_COLUMN_COUNT_1, resultSet);
    EXPECT_GT(count, 0);
}

} // namespace Media
} // namespace OHOS