/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#include "medialibrary_queryperf_test.h"

#include <thread>
#include "datashare_helper.h"
#include "get_self_permissions.h"
#include "iservice_registry.h"

#include "medialibrary_db_const.h"
#include "medialibrary_tracer.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_mock_tocken.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "mimetype_utils.h"
#include "rdb_utils.h"
#include "result_set_utils.h"
#include "scanner_utils.h"
#include "medialibrary_data_manager.h"
#include "userfilemgr_uri.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;
using namespace OHOS::DataShare;
using namespace OHOS::NativeRdb;
using namespace OHOS::AppExecFwk;

namespace OHOS {
namespace Media {
constexpr int STORAGE_MANAGER_ID = 5003;
std::shared_ptr<DataShare::DataShareHelper> sDataShareHelper_ = nullptr;
const int DATA_COUNT = 1000;
const int S2MS = 1000;
const int MS2NS = 1000000;
static uint64_t g_shellToken = 0;
static MediaLibraryMockHapToken* mockToken = nullptr;

void MakeTestData()
{
    DataShareValuesBucket datashareValues;
    datashareValues.Put(CONST_MEDIA_DATA_DB_MEDIA_TYPE, MEDIA_TYPE_IMAGE);
    datashareValues.Put(CONST_MEDIA_DATA_DB_URI, MediaFileUtils::GetMediaTypeUri(MEDIA_TYPE_IMAGE));
    string displayName = "test.jpg";
    string extension = ScannerUtils::GetFileExtension(displayName);
    datashareValues.Put(CONST_MEDIA_DATA_DB_MIME_TYPE, MimeTypeUtils::GetMimeTypeFromExtension(extension));
    datashareValues.Put(CONST_MEDIA_DATA_DB_RELATIVE_PATH, PIC_DIR_VALUES);
    datashareValues.Put(CONST_MEDIA_DATA_DB_NAME, displayName);
    datashareValues.Put(CONST_MEDIA_DATA_DB_TITLE, MediaFileUtils::GetTitleFromDisplayName(displayName));
    datashareValues.Put(CONST_MEDIA_DATA_DB_SIZE, 0);
    datashareValues.Put(CONST_MEDIA_DATA_DB_DATE_ADDED, MediaFileUtils::UTCTimeMilliSeconds());
    datashareValues.Put(CONST_MEDIA_DATA_DB_DATE_MODIFIED, MediaFileUtils::UTCTimeMilliSeconds());
    datashareValues.Put(CONST_MEDIA_DATA_DB_FILE_PATH, ROOT_MEDIA_DIR + PIC_DIR_VALUES + displayName);
    datashareValues.Put(CONST_MEDIA_DATA_DB_BUCKET_ID, 1);
    datashareValues.Put(CONST_MEDIA_DATA_DB_PARENT_ID, 1);
    datashareValues.Put(CONST_MEDIA_DATA_DB_BUCKET_NAME, PIC_DIR_VALUES);
    ValuesBucket values = RdbDataShareAdapter::RdbUtils::ToValuesBucket(datashareValues);

    Uri uri(MEDIALIBRARY_DATA_URI);
    for (int i = 0; i < DATA_COUNT; i++) {
        int64_t rowId = -1;
        MediaLibraryDataManager::GetInstance()->rdbStore_->Insert(rowId, CONST_MEDIALIBRARY_TABLE, values);
    }
}

void UriAppendKeyValue(string &uri, const string &key, std::string value)
{
    string uriKey = key + '=';
    if (uri.find(uriKey) != string::npos) {
        return;
    }

    char queryMark = (uri.find('?') == string::npos) ? '?' : '&';
    string append = queryMark + key + '=' + value;

    size_t pos = uri.find('#');
    if (pos == string::npos) {
        uri += append;
    } else {
        uri.insert(pos, append);
    }
}

void MediaLibraryQueryPerfUnitTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    // 获取shell的tokenId 并保存
    g_shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(g_shellToken);

    vector<string> perms;
    perms.push_back("ohos.permission.READ_MEDIA");
    perms.push_back("ohos.permission.WRITE_MEDIA");
    perms.push_back("ohos.permission.READ_IMAGEVIDEO");
    perms.push_back("ohos.permission.WRITE_IMAGEVIDEO");
    perms.push_back("ohos.permission.MEDIA_LOCATION");
    perms.push_back("ohos.permission.READ_AUDIO");
    perms.push_back("ohos.permission.WRITE_AUDIO");
    // mock  tokenID
    mockToken = new MediaLibraryMockHapToken("com.ohos.medialibrary.medialibrarydata", perms);
    for (auto &perm : perms) {
        MediaLibraryMockTokenUtils::GrantPermissionByTest(IPCSkeleton::GetSelfTokenID(), perm, 0);
    }

    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_TRUE(saManager != nullptr);

    auto remoteObj = saManager->GetSystemAbility(STORAGE_MANAGER_ID);
    ASSERT_TRUE(remoteObj != nullptr);

    sDataShareHelper_ = DataShare::DataShareHelper::Creator(remoteObj, MEDIALIBRARY_DATA_URI);
    ASSERT_TRUE(sDataShareHelper_ != nullptr);

    MediaLibraryUnitTestUtils::CleanTestFiles();
    MakeTestData();
}

void MediaLibraryQueryPerfUnitTest::TearDownTestCase(void)
{
    MEDIA_ERR_LOG("TearDownTestCase start");
    if (sDataShareHelper_ != nullptr) {
        sDataShareHelper_->Release();
    }
    // 删除权限 恢复shell的tokenId
    if (mockToken != nullptr) {
        delete mockToken;
        mockToken = nullptr;
    }

    MediaLibraryMockTokenUtils::ResetToken();
    SetSelfTokenID(g_shellToken);
    EXPECT_EQ(g_shellToken, IPCSkeleton::GetSelfTokenID());
    MEDIA_INFO_LOG("TearDownTestCase end");
}

void MediaLibraryQueryPerfUnitTest::SetUp(void) {}

void MediaLibraryQueryPerfUnitTest::TearDown(void) {}

int64_t UTCTimeSeconds()
{
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = 0;
    clock_gettime(CLOCK_REALTIME, &t);
    return static_cast<int64_t>((t.tv_sec * S2MS) + (t.tv_nsec / MS2NS));
}

HWTEST_F(MediaLibraryQueryPerfUnitTest, medialib_RdbQuery_test_001, TestSize.Level1)
{
    AbsRdbPredicates predicates(CONST_MEDIALIBRARY_TABLE);
    predicates.EqualTo(CONST_MEDIA_DATA_DB_MEDIA_TYPE, to_string(MEDIA_TYPE_IMAGE));
    predicates.OrderByAsc(CONST_MEDIA_DATA_DB_DATE_ADDED);
    vector<string> columns;

    int64_t start = UTCTimeSeconds();
    MediaLibraryTracer tracer;
    tracer.Start("QueryALLColumn");
    auto mediaLibraryDataManager = MediaLibraryDataManager::GetInstance();
    EXPECT_NE(mediaLibraryDataManager, nullptr);
    for (int i = 0; i < 50; i++) {
        auto result = mediaLibraryDataManager->rdbStore_->Query(predicates, columns);
    }
    tracer.Finish();
    int64_t end = UTCTimeSeconds();
    GTEST_LOG_(INFO) << "QueryALLColumn Cost: " << ((double)(end - start)/50) << "ms";
}

HWTEST_F(MediaLibraryQueryPerfUnitTest, medialib_RdbQuery_test_002, TestSize.Level1)
{
    AbsRdbPredicates predicates(CONST_MEDIALIBRARY_TABLE);
    predicates.EqualTo(CONST_MEDIA_DATA_DB_MEDIA_TYPE, to_string(MEDIA_TYPE_IMAGE));
    predicates.OrderByAsc(CONST_MEDIA_DATA_DB_DATE_ADDED);
    vector<string> columns {
        CONST_MEDIA_DATA_DB_MEDIA_TYPE,
        CONST_MEDIA_DATA_DB_PARENT_ID,
        CONST_MEDIA_DATA_DB_RELATIVE_PATH,
        CONST_MEDIA_DATA_DB_MIME_TYPE,
        CONST_MEDIA_DATA_DB_NAME,
        CONST_MEDIA_DATA_DB_TITLE,
        CONST_MEDIA_DATA_DB_BUCKET_ID,
        CONST_MEDIA_DATA_DB_BUCKET_NAME,
        CONST_MEDIA_DATA_DB_DATE_ADDED,
        CONST_MEDIA_DATA_DB_DATE_MODIFIED
    };
    int64_t start = UTCTimeSeconds();
    MediaLibraryTracer tracer;
    tracer.Start("Query10Column");
    auto mediaLibraryDataManager = MediaLibraryDataManager::GetInstance();
    EXPECT_NE(mediaLibraryDataManager, nullptr);
    for (int i = 0; i < 50; i++) {
        auto result = mediaLibraryDataManager->rdbStore_->Query(predicates, columns);
    }
    tracer.Finish();
    int64_t end = UTCTimeSeconds();
    GTEST_LOG_(INFO) << "Query10Column Cost: " << ((double)(end - start)/50) << "ms";
}

HWTEST_F(MediaLibraryQueryPerfUnitTest, medialib_RdbQuery_test_003, TestSize.Level1)
{
    AbsRdbPredicates predicates(CONST_MEDIALIBRARY_TABLE);
    predicates.EqualTo(CONST_MEDIA_DATA_DB_MEDIA_TYPE, to_string(MEDIA_TYPE_IMAGE));
    predicates.OrderByAsc(CONST_MEDIA_DATA_DB_DATE_ADDED);
    predicates.Limit(50);
    vector<string> columns {
        CONST_MEDIA_DATA_DB_MEDIA_TYPE,
        CONST_MEDIA_DATA_DB_PARENT_ID,
        CONST_MEDIA_DATA_DB_RELATIVE_PATH,
        CONST_MEDIA_DATA_DB_MIME_TYPE,
        CONST_MEDIA_DATA_DB_NAME,
        CONST_MEDIA_DATA_DB_TITLE,
        CONST_MEDIA_DATA_DB_BUCKET_ID,
        CONST_MEDIA_DATA_DB_BUCKET_NAME,
        CONST_MEDIA_DATA_DB_DATE_ADDED,
        CONST_MEDIA_DATA_DB_DATE_MODIFIED
    };
    int64_t start = UTCTimeSeconds();
    MediaLibraryTracer tracer;
    tracer.Start("Query10Columnlimit50");
    auto mediaLibraryDataManager = MediaLibraryDataManager::GetInstance();
    EXPECT_NE(mediaLibraryDataManager, nullptr);
    for (int i = 0; i < 50; i++) {
        auto result = mediaLibraryDataManager->rdbStore_->Query(predicates, columns);
    }
    tracer.Finish();
    int64_t end = UTCTimeSeconds();
    GTEST_LOG_(INFO) << "Query10Columnlimit50 Cost: " << ((double)(end - start)/50) << "ms";
}

HWTEST_F(MediaLibraryQueryPerfUnitTest, medialib_RdbQuery_test_004, TestSize.Level1)
{
    AbsRdbPredicates predicates(CONST_MEDIALIBRARY_TABLE);
    predicates.EqualTo(CONST_MEDIA_DATA_DB_MEDIA_TYPE, to_string(MEDIA_TYPE_IMAGE));
    predicates.OrderByAsc(CONST_MEDIA_DATA_DB_DATE_ADDED);
    vector<string> columns;

    int32_t mediaType;
    int32_t parentId;
    int32_t bucketId;
    int64_t dateAdded;
    int64_t dateModified;
    string relativePath;
    string mimeType;
    string displayName;
    string title;
    string bucketName;

    int64_t start = UTCTimeSeconds();
    MediaLibraryTracer tracer;
    tracer.Start("GetALLColumn");
    for (int i = 0; i < 50; i++) {
        auto result = MediaLibraryDataManager::GetInstance()->rdbStore_->Query(predicates, columns);
        ASSERT_NE(result, nullptr);
        int count;
        result->GetRowCount(count);
        ASSERT_TRUE(count >= DATA_COUNT);
        result->GoToFirstRow();
        do {
            mediaType = GetInt32Val(CONST_MEDIA_DATA_DB_MEDIA_TYPE, result);
            parentId = GetInt32Val(CONST_MEDIA_DATA_DB_PARENT_ID, result);
            relativePath = GetStringVal(CONST_MEDIA_DATA_DB_RELATIVE_PATH, result);
            mimeType = GetStringVal(CONST_MEDIA_DATA_DB_MIME_TYPE, result);
            displayName = GetStringVal(CONST_MEDIA_DATA_DB_NAME, result);
            title = GetStringVal(CONST_MEDIA_DATA_DB_TITLE, result);
            bucketId = GetInt32Val(CONST_MEDIA_DATA_DB_BUCKET_ID, result);
            bucketName = GetStringVal(CONST_MEDIA_DATA_DB_BUCKET_NAME, result);
            dateAdded = GetInt64Val(CONST_MEDIA_DATA_DB_DATE_ADDED, result);
            dateModified = GetInt64Val(CONST_MEDIA_DATA_DB_DATE_MODIFIED, result);
        } while (!result->GoToNextRow());
    }
    tracer.Finish();
    int64_t end = UTCTimeSeconds();

    GTEST_LOG_(INFO) << "GetALLColumn Cost: " << ((double)(end - start)/50) << "ms";
}

HWTEST_F(MediaLibraryQueryPerfUnitTest, medialib_RdbQuery_test_005, TestSize.Level1)
{
    AbsRdbPredicates predicates(CONST_MEDIALIBRARY_TABLE);
    predicates.EqualTo(CONST_MEDIA_DATA_DB_MEDIA_TYPE, to_string(MEDIA_TYPE_IMAGE));
    predicates.OrderByAsc(CONST_MEDIA_DATA_DB_DATE_ADDED);
    vector<string> columns { CONST_MEDIA_DATA_DB_MEDIA_TYPE, CONST_MEDIA_DATA_DB_PARENT_ID,
        CONST_MEDIA_DATA_DB_RELATIVE_PATH, CONST_MEDIA_DATA_DB_MIME_TYPE, CONST_MEDIA_DATA_DB_NAME,
        CONST_MEDIA_DATA_DB_TITLE, CONST_MEDIA_DATA_DB_BUCKET_ID,
        CONST_MEDIA_DATA_DB_BUCKET_NAME, CONST_MEDIA_DATA_DB_DATE_ADDED, CONST_MEDIA_DATA_DB_DATE_MODIFIED };

    int32_t mediaType;
    int32_t parentId;
    int32_t bucketId;
    int64_t dateAdded;
    int64_t dateModified;
    string relativePath;
    string mimeType;
    string displayName;
    string title;
    string bucketName;

    int64_t start = UTCTimeSeconds();
    MediaLibraryTracer tracer;
    tracer.Start("Get10Column");
    for (int i = 0; i < 50; i++) {
        auto result = MediaLibraryDataManager::GetInstance()->rdbStore_->Query(predicates, columns);
        ASSERT_NE(result, nullptr);
        int count;
        result->GetRowCount(count);
        ASSERT_TRUE(count >= DATA_COUNT);
        result->GoToFirstRow();
        do {
            mediaType = GetInt32Val(CONST_MEDIA_DATA_DB_MEDIA_TYPE, result);
            parentId = GetInt32Val(CONST_MEDIA_DATA_DB_PARENT_ID, result);
            relativePath = GetStringVal(CONST_MEDIA_DATA_DB_RELATIVE_PATH, result);
            mimeType = GetStringVal(CONST_MEDIA_DATA_DB_MIME_TYPE, result);
            displayName = GetStringVal(CONST_MEDIA_DATA_DB_NAME, result);
            title = GetStringVal(CONST_MEDIA_DATA_DB_TITLE, result);
            bucketId = GetInt32Val(CONST_MEDIA_DATA_DB_BUCKET_ID, result);
            bucketName = GetStringVal(CONST_MEDIA_DATA_DB_BUCKET_NAME, result);
            dateAdded = GetInt64Val(CONST_MEDIA_DATA_DB_DATE_ADDED, result);
            dateModified = GetInt64Val(CONST_MEDIA_DATA_DB_DATE_MODIFIED, result);
        } while (!result->GoToNextRow());
    }
    tracer.Finish();
    int64_t end = UTCTimeSeconds();

    GTEST_LOG_(INFO) << "Get10Column Cost: " << ((double)(end - start)/50) << "ms";
}

HWTEST_F(MediaLibraryQueryPerfUnitTest, medialib_RdbQuery_test_006, TestSize.Level1)
{
    AbsRdbPredicates predicates(CONST_MEDIALIBRARY_TABLE);
    predicates.EqualTo(CONST_MEDIA_DATA_DB_MEDIA_TYPE, to_string(MEDIA_TYPE_IMAGE));
    predicates.OrderByAsc(CONST_MEDIA_DATA_DB_DATE_ADDED);
    predicates.Limit(50);
    vector<string> columns { CONST_MEDIA_DATA_DB_MEDIA_TYPE, CONST_MEDIA_DATA_DB_PARENT_ID, CONST_MEDIA_DATA_DB_RELATIVE_PATH,
        CONST_MEDIA_DATA_DB_MIME_TYPE, CONST_MEDIA_DATA_DB_NAME, CONST_MEDIA_DATA_DB_TITLE, CONST_MEDIA_DATA_DB_BUCKET_ID,
        CONST_MEDIA_DATA_DB_BUCKET_NAME, CONST_MEDIA_DATA_DB_DATE_ADDED, CONST_MEDIA_DATA_DB_DATE_MODIFIED };
    int32_t mediaType;
    int32_t parentId;
    int32_t bucketId;
    int64_t dateAdded;
    int64_t dateModified;
    string relativePath;
    string mimeType;
    string displayName;
    string title;
    string bucketName;

    int64_t start = UTCTimeSeconds();
    MediaLibraryTracer tracer;
    tracer.Start("Get10Columnlimit50");
    for (int i = 0; i < 50; i++) {
        auto result = MediaLibraryDataManager::GetInstance()->rdbStore_->Query(predicates, columns);
        ASSERT_NE(result, nullptr);
        int count;
        result->GetRowCount(count);
        ASSERT_TRUE(count <= 50);
        result->GoToFirstRow();
        do {
            mediaType = GetInt32Val(CONST_MEDIA_DATA_DB_MEDIA_TYPE, result);
            parentId = GetInt32Val(CONST_MEDIA_DATA_DB_PARENT_ID, result);
            relativePath = GetStringVal(CONST_MEDIA_DATA_DB_RELATIVE_PATH, result);
            mimeType = GetStringVal(CONST_MEDIA_DATA_DB_MIME_TYPE, result);
            displayName = GetStringVal(CONST_MEDIA_DATA_DB_NAME, result);
            title = GetStringVal(CONST_MEDIA_DATA_DB_TITLE, result);
            bucketId = GetInt32Val(CONST_MEDIA_DATA_DB_BUCKET_ID, result);
            bucketName = GetStringVal(CONST_MEDIA_DATA_DB_BUCKET_NAME, result);
            dateAdded = GetInt64Val(CONST_MEDIA_DATA_DB_DATE_ADDED, result);
            dateModified = GetInt64Val(CONST_MEDIA_DATA_DB_DATE_MODIFIED, result);
        } while (!result->GoToNextRow());
    }
    tracer.Finish();
    int64_t end = UTCTimeSeconds();

    GTEST_LOG_(INFO) << "Get10Columnlimit50 Cost: " << ((double)(end - start)/50) << "ms";
}

HWTEST_F(MediaLibraryQueryPerfUnitTest, medialib_datashareQuery_test_007, TestSize.Level1)
{
    Uri uri(MEDIALIBRARY_DATA_URI);
    DataSharePredicates predicates;
    vector<string> columns;

    int64_t start = UTCTimeSeconds();
    MediaLibraryTracer tracer;
    tracer.Start("DataShareQueryALLColumn");
    for (int i = 0; i < 50; i++) {
        auto result = sDataShareHelper_->Query(uri, predicates, columns);
        ASSERT_NE(result, nullptr);
    }
    tracer.Finish();
    int64_t end = UTCTimeSeconds();

    GTEST_LOG_(INFO) << "DataShareQueryALLColumn Cost: " << ((double)(end - start)/50) << "ms";
}

HWTEST_F(MediaLibraryQueryPerfUnitTest, medialib_datashareQuery_test_008, TestSize.Level1)
{
    Uri uri(MEDIALIBRARY_DATA_URI);
    DataSharePredicates predicates;
    vector<string> columns {
        CONST_MEDIA_DATA_DB_MEDIA_TYPE,
        CONST_MEDIA_DATA_DB_PARENT_ID,
        CONST_MEDIA_DATA_DB_RELATIVE_PATH,
        CONST_MEDIA_DATA_DB_MIME_TYPE,
        CONST_MEDIA_DATA_DB_NAME,
        CONST_MEDIA_DATA_DB_TITLE,
        CONST_MEDIA_DATA_DB_BUCKET_ID,
        CONST_MEDIA_DATA_DB_BUCKET_NAME,
        CONST_MEDIA_DATA_DB_DATE_ADDED,
        CONST_MEDIA_DATA_DB_DATE_MODIFIED
    };

    int64_t start = UTCTimeSeconds();
    MediaLibraryTracer tracer;
    tracer.Start("DataShareQuery10Column");
    for (int i = 0; i < 50; i++) {
        auto result = sDataShareHelper_->Query(uri, predicates, columns);
        ASSERT_NE(result, nullptr);
    }
    tracer.Finish();
    int64_t end = UTCTimeSeconds();

    GTEST_LOG_(INFO) << "DataShareQuery10Column Cost: " << ((double)(end - start)/50) << "ms";
}


HWTEST_F(MediaLibraryQueryPerfUnitTest, medialib_datashareQuery_test_009, TestSize.Level1)
{
    Uri uri(MEDIALIBRARY_DATA_URI);
    DataSharePredicates predicates;
    string selection = string(CONST_MEDIA_DATA_DB_ID) + " <> ? LIMIT 0, 50 ";
    vector<string> selectionArgs = { "0" };
    predicates.SetWhereClause(selection);
    predicates.SetWhereArgs(selectionArgs);
    vector<string> columns {
        CONST_MEDIA_DATA_DB_MEDIA_TYPE,
        CONST_MEDIA_DATA_DB_PARENT_ID,
        CONST_MEDIA_DATA_DB_RELATIVE_PATH,
        CONST_MEDIA_DATA_DB_MIME_TYPE,
        CONST_MEDIA_DATA_DB_NAME,
        CONST_MEDIA_DATA_DB_TITLE,
        CONST_MEDIA_DATA_DB_BUCKET_ID,
        CONST_MEDIA_DATA_DB_BUCKET_NAME,
        CONST_MEDIA_DATA_DB_DATE_ADDED,
        CONST_MEDIA_DATA_DB_DATE_MODIFIED
    };

    int64_t start = UTCTimeSeconds();
    MediaLibraryTracer tracer;
    tracer.Start("DataShareQuery50-10Column");
    for (int i = 0; i < 50; i++) {
        auto result = sDataShareHelper_->Query(uri, predicates, columns);
        ASSERT_NE(result, nullptr);
    }
    tracer.Finish();
    int64_t end = UTCTimeSeconds();

    GTEST_LOG_(INFO) << "DataShareQuery50-10Column Cost: " << ((double)(end - start)/50) << "ms";
}

HWTEST_F(MediaLibraryQueryPerfUnitTest, medialib_datashareQuery_test_010, TestSize.Level1)
{
    Uri uri(MEDIALIBRARY_DATA_URI);
    DataSharePredicates predicates;
    vector<string> columns;

    int32_t mediaType;
    int32_t parentId;
    int32_t bucketId;
    int64_t dateAdded;
    int64_t dateModified;
    string relativePath;
    string mimeType;
    string displayName;
    string title;
    string bucketName;

    int64_t start = UTCTimeSeconds();
    MediaLibraryTracer tracer;
    tracer.Start("DataShareGetALLColumn");
    for (int i = 0; i < 50; i++) {
        auto result = sDataShareHelper_->Query(uri, predicates, columns);
        ASSERT_NE(result, nullptr);
        int count;
        result->GetRowCount(count);
        result->GoToFirstRow();
        do {
            mediaType = GetInt32Val(CONST_MEDIA_DATA_DB_MEDIA_TYPE, result);
            parentId = GetInt32Val(CONST_MEDIA_DATA_DB_PARENT_ID, result);
            relativePath = GetStringVal(CONST_MEDIA_DATA_DB_RELATIVE_PATH, result);
            mimeType = GetStringVal(CONST_MEDIA_DATA_DB_MIME_TYPE, result);
            displayName = GetStringVal(CONST_MEDIA_DATA_DB_NAME, result);
            title = GetStringVal(CONST_MEDIA_DATA_DB_TITLE, result);
            bucketId = GetInt32Val(CONST_MEDIA_DATA_DB_BUCKET_ID, result);
            bucketName = GetStringVal(CONST_MEDIA_DATA_DB_BUCKET_NAME, result);
            dateAdded = GetInt64Val(CONST_MEDIA_DATA_DB_DATE_ADDED, result);
            dateModified = GetInt64Val(CONST_MEDIA_DATA_DB_DATE_MODIFIED, result);
        } while (!result->GoToNextRow());
    }
    tracer.Finish();
    int64_t end = UTCTimeSeconds();

    GTEST_LOG_(INFO) << "DataShareGetALLColumn Cost: " << ((double)(end - start)/50) << "ms";
}

HWTEST_F(MediaLibraryQueryPerfUnitTest, medialib_datashareQuery_test_011, TestSize.Level1)
{
    string queryUri = CONST_PAH_QUERY_PHOTO;
    UriAppendKeyValue(queryUri, "api_version", to_string(MEDIA_API_VERSION_V10));
    Uri uri(queryUri);
    DataSharePredicates predicates;
    vector<string> columns { CONST_MEDIA_DATA_DB_MEDIA_TYPE, CONST_MEDIA_DATA_DB_IS_FAV, CONST_MEDIA_DATA_DB_RELATIVE_PATH,
        CONST_MEDIA_DATA_DB_MIME_TYPE, CONST_MEDIA_DATA_DB_NAME, CONST_MEDIA_DATA_DB_TITLE, CONST_MEDIA_DATA_DB_PARENT_ID,
        CONST_MEDIA_DATA_DB_DATE_TAKEN, CONST_MEDIA_DATA_DB_DATE_ADDED, CONST_MEDIA_DATA_DB_DATE_MODIFIED };

    int32_t mediaType;
    int32_t is_favorite;
    int32_t parent;
    int64_t dateAdded;
    int64_t dateModified;
    string relativePath;
    string mimeType;
    string displayName;
    string title;
    int64_t date_taken;

    int64_t start = UTCTimeSeconds();
    MediaLibraryTracer tracer;
    tracer.Start("DataShareGet10Column");
    for (int i = 0; i < 50; i++) {
        auto result = sDataShareHelper_->Query(uri, predicates, columns);
        ASSERT_NE(result, nullptr);
        int count;
        result->GetRowCount(count);
        result->GoToFirstRow();
        do {
            mediaType = GetInt32Val(CONST_MEDIA_DATA_DB_MEDIA_TYPE, result);
            is_favorite = GetInt32Val(CONST_MEDIA_DATA_DB_IS_FAV, result);
            relativePath = GetStringVal(CONST_MEDIA_DATA_DB_RELATIVE_PATH, result);
            mimeType = GetStringVal(CONST_MEDIA_DATA_DB_MIME_TYPE, result);
            displayName = GetStringVal(CONST_MEDIA_DATA_DB_NAME, result);
            title = GetStringVal(CONST_MEDIA_DATA_DB_TITLE, result);
            parent = GetInt32Val(CONST_MEDIA_DATA_DB_PARENT_ID, result);
            date_taken = GetInt64Val(CONST_MEDIA_DATA_DB_DATE_TAKEN, result);
            dateAdded = GetInt64Val(CONST_MEDIA_DATA_DB_DATE_ADDED, result);
            dateModified = GetInt64Val(CONST_MEDIA_DATA_DB_DATE_MODIFIED, result);
        } while (!result->GoToNextRow());
    }
    tracer.Finish();
    int64_t end = UTCTimeSeconds();

    GTEST_LOG_(INFO) << "DataShareGet10Column Cost: " << ((double)(end - start)/50) << "ms";
}

HWTEST_F(MediaLibraryQueryPerfUnitTest, medialib_datashareQuery_test_012, TestSize.Level1)
{
    Uri uri(MEDIALIBRARY_DATA_URI);
    DataSharePredicates predicates;
    string selection = string(CONST_MEDIA_DATA_DB_ID) + " <> ? LIMIT 0, 50 ";
    vector<string> selectionArgs = { "0" };
    predicates.SetWhereClause(selection);
    predicates.SetWhereArgs(selectionArgs);
    vector<string> columns { CONST_MEDIA_DATA_DB_MEDIA_TYPE, CONST_MEDIA_DATA_DB_PARENT_ID, CONST_MEDIA_DATA_DB_RELATIVE_PATH,
        CONST_MEDIA_DATA_DB_MIME_TYPE, CONST_MEDIA_DATA_DB_NAME, CONST_MEDIA_DATA_DB_TITLE, CONST_MEDIA_DATA_DB_BUCKET_ID,
        CONST_MEDIA_DATA_DB_BUCKET_NAME, CONST_MEDIA_DATA_DB_DATE_ADDED, CONST_MEDIA_DATA_DB_DATE_MODIFIED };

    int32_t mediaType;
    int32_t parentId;
    int32_t bucketId;
    int64_t dateAdded;
    int64_t dateModified;
    string relativePath;
    string mimeType;
    string displayName;
    string title;
    string bucketName;
    int64_t start = UTCTimeSeconds();
    MediaLibraryTracer tracer;
    tracer.Start("DataShareGet10Columnlimit50");
    for (int i = 0; i < 50; i++) {
        auto result = sDataShareHelper_->Query(uri, predicates, columns);
        ASSERT_NE(result, nullptr);
        int count;
        result->GetRowCount(count);
        ASSERT_TRUE(count <= 50);
        result->GoToFirstRow();
        do {
            mediaType = GetInt32Val(CONST_MEDIA_DATA_DB_MEDIA_TYPE, result);
            parentId = GetInt32Val(CONST_MEDIA_DATA_DB_PARENT_ID, result);
            relativePath = GetStringVal(CONST_MEDIA_DATA_DB_RELATIVE_PATH, result);
            mimeType = GetStringVal(CONST_MEDIA_DATA_DB_MIME_TYPE, result);
            displayName = GetStringVal(CONST_MEDIA_DATA_DB_NAME, result);
            title = GetStringVal(CONST_MEDIA_DATA_DB_TITLE, result);
            bucketId = GetInt32Val(CONST_MEDIA_DATA_DB_BUCKET_ID, result);
            bucketName = GetStringVal(CONST_MEDIA_DATA_DB_BUCKET_NAME, result);
            dateAdded = GetInt64Val(CONST_MEDIA_DATA_DB_DATE_ADDED, result);
            dateModified = GetInt64Val(CONST_MEDIA_DATA_DB_DATE_MODIFIED, result);
        } while (!result->GoToNextRow());
    }
    tracer.Finish();
    int64_t end = UTCTimeSeconds();

    GTEST_LOG_(INFO) << "DataShareGet10Columnlimit50 Cost: " << ((double)(end - start)/50) << "ms";
}

HWTEST_F(MediaLibraryQueryPerfUnitTest, medialib_RdbQuery_test_013, TestSize.Level1)
{
    AbsRdbPredicates predicates(CONST_MEDIALIBRARY_TABLE);
    predicates.EqualTo(CONST_MEDIA_DATA_DB_MEDIA_TYPE, to_string(MEDIA_TYPE_IMAGE));
    vector<string> columns;

    int64_t timeSum = 0;
    for (int i = 0; i < 50; i++) {
        auto result = MediaLibraryDataManager::GetInstance()->rdbStore_->Query(predicates, columns);
        ASSERT_NE(result, nullptr);
        int count;
        int64_t start = UTCTimeSeconds();
        result->GetRowCount(count);
        int64_t end = UTCTimeSeconds();
        ASSERT_TRUE(count > 0);
        timeSum += end - start;
    }

    GTEST_LOG_(INFO) << "rdb GetRowCount Cost: " << ((double)(timeSum)/50) << "ms";
}

HWTEST_F(MediaLibraryQueryPerfUnitTest, medialib_datashareQuery_test_014, TestSize.Level1)
{
    Uri uri(MEDIALIBRARY_DATA_URI);
    DataSharePredicates predicates;
    predicates.EqualTo(CONST_MEDIA_DATA_DB_MEDIA_TYPE, to_string(MEDIA_TYPE_IMAGE));
    vector<string> columns;

    int64_t timeSum = 0;
    for (int i = 0; i < 50; i++) {
        auto result = sDataShareHelper_->Query(uri, predicates, columns);
        ASSERT_NE(result, nullptr);
        int count;
        int64_t start = UTCTimeSeconds();
        result->GetRowCount(count);
        int64_t end = UTCTimeSeconds();
        timeSum += end - start;
    }

    GTEST_LOG_(INFO) << "DataShare GetRowCount Cost: " << ((double)(timeSum)/50) << "ms";
}
} // namespace Media
} // namespace OHOS
