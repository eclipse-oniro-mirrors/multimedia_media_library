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

#define MLOG_TAG "audioOperationsTest"

#include "../include/medialibrary_audio_operations_test.h"

#include <chrono>
#include <fstream>
#include <thread>
#include <unistd.h>
#include <unordered_map>

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
#include "medialibrary_audio_operations.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "result_set_utils.h"
#include "uri.h"
#include "userfile_manager_types.h"
#include "values_bucket.h"

namespace OHOS {
namespace Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace OHOS::DataShare;
using OHOS::DataShare::DataShareValuesBucket;
using OHOS::DataShare::DataSharePredicates;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;

using ExceptIntFunction = void (*) (int32_t);
using ExceptLongFunction = void (*) (int64_t);
using ExceptBoolFunction = void (*) (bool);
using ExceptStringFunction = void (*) (const string&);

const unordered_map<string, int> FILEASSET_MEMBER_MAP = {
    { MediaColumn::MEDIA_ID, MEMBER_TYPE_INT32 },
    { MediaColumn::MEDIA_URI, MEMBER_TYPE_STRING },
    { MediaColumn::MEDIA_FILE_PATH, MEMBER_TYPE_STRING },
    { MediaColumn::MEDIA_SIZE, MEMBER_TYPE_INT64 },
    { MediaColumn::MEDIA_TITLE, MEMBER_TYPE_STRING },
    { MediaColumn::MEDIA_NAME, MEMBER_TYPE_STRING },
    { MediaColumn::MEDIA_TYPE, MEMBER_TYPE_INT32 },
    { MediaColumn::MEDIA_MIME_TYPE, MEMBER_TYPE_STRING },
    { MediaColumn::MEDIA_OWNER_PACKAGE, MEMBER_TYPE_STRING },
    { MediaColumn::MEDIA_DEVICE_NAME, MEMBER_TYPE_STRING },
    { MediaColumn::MEDIA_DATE_ADDED, MEMBER_TYPE_INT64 },
    { MediaColumn::MEDIA_DATE_MODIFIED, MEMBER_TYPE_INT64 },
    { MediaColumn::MEDIA_DATE_TAKEN, MEMBER_TYPE_INT64 },
    { MediaColumn::MEDIA_DATE_DELETED, MEMBER_TYPE_INT64 },
    { MediaColumn::MEDIA_TIME_VISIT, MEMBER_TYPE_INT64 },
    { MediaColumn::MEDIA_DURATION, MEMBER_TYPE_INT32 },
    { MediaColumn::MEDIA_TIME_PENDING, MEMBER_TYPE_INT64 },
    { MediaColumn::MEDIA_IS_FAV, MEMBER_TYPE_INT32 },
    { MediaColumn::MEDIA_DATE_TRASHED, MEMBER_TYPE_INT64 },
    { MediaColumn::MEDIA_HIDDEN, MEMBER_TYPE_INT32 },
    { MediaColumn::MEDIA_PARENT_ID, MEMBER_TYPE_INT32 },
    { MediaColumn::MEDIA_RELATIVE_PATH, MEMBER_TYPE_STRING },
    { PhotoColumn::PHOTO_ORIENTATION, MEMBER_TYPE_INT32 },
    { PhotoColumn::PHOTO_LATITUDE, MEMBER_TYPE_DOUBLE },
    { PhotoColumn::PHOTO_LONGITUDE, MEMBER_TYPE_DOUBLE },
    { PhotoColumn::PHOTO_HEIGHT, MEMBER_TYPE_INT32 },
    { PhotoColumn::PHOTO_WIDTH, MEMBER_TYPE_INT32 },
    { PhotoColumn::PHOTO_LCD_VISIT_TIME, MEMBER_TYPE_INT64 },
    { AudioColumn::AUDIO_ALBUM, MEMBER_TYPE_STRING },
    { AudioColumn::AUDIO_ARTIST, MEMBER_TYPE_STRING },
    { DocumentColumn::DOCUMENT_LCD, MEMBER_TYPE_STRING },
    { DocumentColumn::DOCUMENT_LCD_VISIT_TIME, MEMBER_TYPE_INT64 }
};

namespace {
void CleanTestTables()
{
    vector<string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
        AudioColumn::AUDIOS_TABLE,
        DocumentColumn::DOCUMENTS_TABLE,
        ASSET_UNIQUE_NUMBER_TABLE
        // todo: album tables
    };
    for (auto &dropTable : dropTableList) {
        string dropSql = "DROP TABLE " + dropTable + ";";
        int32_t ret = g_rdbStore->ExecuteSql(dropSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Drop %{public}s table failed", dropTable.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Drop %{public}s table success", dropTable.c_str());
    }
}

struct UniqueMemberValuesBucket {
    string assetMediaType;
    int32_t startNumber;
};

void PrepareUniqueNumberTable()
{
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("can not get g_rdbstore");
        return;
    }
    auto store = g_rdbStore->GetRaw();
    if (store == nullptr) {
        MEDIA_ERR_LOG("can not get store");
        return;
    }
    string queryRowSql = "SELECT COUNT(*) as count FROM " + ASSET_UNIQUE_NUMBER_TABLE;
    auto resultSet = store->QuerySql(queryRowSql);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get AssetUniqueNumberTable count");
        return;
    }
    if (GetInt32Val("count", resultSet) != 0) {
        MEDIA_DEBUG_LOG("AssetUniqueNumberTable is already inited");
        return;
    }

    UniqueMemberValuesBucket imageBucket = { IMAGE_ASSET_TYPE, 1 };
    UniqueMemberValuesBucket videoBucket = { VIDEO_ASSET_TYPE, 1 };
    UniqueMemberValuesBucket audioBucket = { AUDIO_ASSET_TYPE, 1 };

    vector<UniqueMemberValuesBucket> uniqueNumberValueBuckets = {
        imageBucket, videoBucket, audioBucket
    };

    for (const auto& uniqueNumberValueBucket : uniqueNumberValueBuckets) {
        ValuesBucket valuesBucket;
        valuesBucket.PutString(ASSET_MEDIA_TYPE, uniqueNumberValueBucket.assetMediaType);
        valuesBucket.PutInt(UNIQUE_NUMBER, uniqueNumberValueBucket.startNumber);
        int64_t outRowId = -1;
        int32_t insertResult = store->Insert(outRowId, ASSET_UNIQUE_NUMBER_TABLE, valuesBucket);
        if (insertResult != NativeRdb::E_OK || outRowId <= 0) {
            MEDIA_ERR_LOG("Prepare smartAlbum failed");
        }
    }
}

void SetTables()
{
    vector<string> createTableSqlList = {
        PhotoColumn::CREATE_PHOTO_TABLE,
        AudioColumn::CREATE_AUDIO_TABLE,
        DocumentColumn::CREATE_DOCUMENT_TABLE,
        CREATE_ASSET_UNIQUE_NUMBER_TABLE
        // todo: album tables
    };
    for (auto &createTableSql : createTableSqlList) {
        int32_t ret = g_rdbStore->ExecuteSql(createTableSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Execute sql %{private}s failed", createTableSql.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Execute sql %{private}s success", createTableSql.c_str());
    }
    PrepareUniqueNumberTable();
}

void ClearAndRestart()
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    system("rm -rf /storage/media/local/files/*");
    for (const auto &dir : TEST_ROOT_DIRS) {
        string ROOT_PATH = "/storage/media/100/local/files/";
        bool ret = MediaFileUtils::CreateDirectory(ROOT_PATH + dir + "/");
        CHECK_AND_PRINT_LOG(ret, "make %{public}s dir failed, ret=%{public}d", dir.c_str(), ret);
    }
    CleanTestTables();
    SetTables();
}

string GetFileAssetValueToStr(FileAsset &fileAsset, const string &column)
{
    // judge type
    auto member = fileAsset.GetMemberValue(column);
    int type = -1;
    if (get_if<int32_t>(&member)) {
        type = MEMBER_TYPE_INT32;
    } else if (get_if<int64_t>(&member)) {
        type = MEMBER_TYPE_INT64;
    } else if (get_if<string>(&member)) {
        type = MEMBER_TYPE_STRING;
    } else {
        MEDIA_ERR_LOG("Can not find this type");
        return "";
    }

    auto res = fileAsset.GetMemberValue(column);
    switch (type) {
        case MEMBER_TYPE_INT32: {
            int32_t resInt = get<int32_t>(res);
            if (resInt != DEFAULT_INT32) {
                return to_string(resInt);
            }
            break;
        }
        case MEMBER_TYPE_INT64: {
            int64_t resLong = get<int64_t>(res);
            if (resLong != DEFAULT_INT64) {
                return to_string(resLong);
            }
            break;
        }
        case MEMBER_TYPE_STRING: {
            string resStr = get<string>(res);
            if (!resStr.empty()) {
                return resStr;
            }
            return "";
        }
        default: {
            return "";
        }
    }
    return "0";
}

bool QueryAndVerifyAudioAsset(const string &columnName, const string &value,
    const unordered_map<string, string> &verifyColumnAndValuesMap)
{
    string querySql = "SELECT * FROM " + AudioColumn::AUDIOS_TABLE + " WHERE " +
        columnName + "='" + value + "';";

    MEDIA_DEBUG_LOG("querySql: %{public}s", querySql.c_str());
    auto resultSet = g_rdbStore->QuerySql(querySql);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Get resultSet failed");
        return false;
    }

    int32_t resultSetCount = 0;
    int32_t ret = resultSet->GetRowCount(resultSetCount);
    if (ret != NativeRdb::E_OK || resultSetCount <= 0) {
        MEDIA_ERR_LOG("resultSet row count is 0");
        return false;
    }

    shared_ptr<FetchResult<FileAsset>> fetchFileResult = make_shared<FetchResult<FileAsset>>();
    if (fetchFileResult == nullptr) {
        MEDIA_ERR_LOG("Get fetchFileResult failed");
        return false;
    }
    auto fileAsset = fetchFileResult->GetObjectFromRdb(resultSet, 0);
    if (fileAsset == nullptr || fileAsset->GetId() < 0) {
        return false;
    }
    if (fileAsset != nullptr) {
        for (const auto &iter : verifyColumnAndValuesMap) {
            string resStr = GetFileAssetValueToStr(*fileAsset, iter.first);
            if (resStr.empty()) {
                MEDIA_ERR_LOG("verify failed! Param %{public}s is empty", resStr.c_str());
                return false;
            }
            if (resStr != iter.second) {
                MEDIA_ERR_LOG("verify failed! Except %{public}s param %{public}s, actually %{public}s",
                    iter.first.c_str(), iter.second.c_str(), resStr.c_str());
                return false;
            }
        }
    }
    return true;
}

inline int32_t CreateAudioApi10(int mediaType, const string &displayName)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_AUDIO, OperationType::CREATE,
        MediaLibraryApi::API_10);
    ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_NAME, displayName);
    values.PutInt(MediaColumn::MEDIA_TYPE, mediaType);
    cmd.SetValueBucket(values);
    int32_t ret = MediaLibraryAudioOperations::Create(cmd);
    if (ret < 0) {
        MEDIA_ERR_LOG("Create Audio failed, errCode=%{public}d", ret);
        return ret;
    }
    return ret;
}

string GetFilePath(int fileId)
{
    if (fileId < 0) {
        MEDIA_ERR_LOG("this file id %{public}d is invalid", fileId);
        return "";
    }

    vector<string> columns = { AudioColumn::MEDIA_FILE_PATH };
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_AUDIO, OperationType::QUERY,
        MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->EqualTo(AudioColumn::MEDIA_ID, to_string(fileId));
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("can not get rdbstore");
        return "";
    }
    auto resultSet = g_rdbStore->Query(cmd, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get file Path");
        return "";
    }
    string path = GetStringVal(AudioColumn::MEDIA_FILE_PATH, resultSet);
    return path;
}

int32_t MakeAudioUnpending(int fileId)
{
    if (fileId < 0) {
        MEDIA_ERR_LOG("this file id %{private}d is invalid", fileId);
        return E_INVALID_FILEID;
    }

    string path = GetFilePath(fileId);
    if (path.empty()) {
        MEDIA_ERR_LOG("Get path failed");
        return E_INVALID_VALUES;
    }
    int32_t errCode = MediaFileUtils::CreateAsset(path);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("Can not create asset");
        return errCode;
    }

    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("can not get rdbstore");
        return E_HAS_DB_ERROR;
    }
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_AUDIO, OperationType::UPDATE);
    ValuesBucket values;
    values.PutLong(AudioColumn::MEDIA_TIME_PENDING, 0);
    cmd.SetValueBucket(values);
    cmd.GetAbsRdbPredicates()->EqualTo(AudioColumn::MEDIA_ID, to_string(fileId));
    int32_t changedRows = -1;
    errCode = g_rdbStore->Update(cmd, changedRows);
    if (errCode != E_OK || changedRows <= 0) {
        MEDIA_ERR_LOG("Update pending failed, errCode = %{public}d, changeRows = %{public}d",
            errCode, changedRows);
        return errCode;
    }

    return E_OK;
}

int32_t SetDefaultAudioApi10(int mediaType, const string &displayName)
{
    int fileId = CreateAudioApi10(mediaType, displayName);
    if (fileId < 0) {
        MEDIA_ERR_LOG("create audio failed, res=%{public}d", fileId);
        return fileId;
    }
    int32_t errCode = MakeAudioUnpending(fileId);
    if (errCode != E_OK) {
        return errCode;
    }
    return fileId;
}

int32_t GetAudioAssetCountIndb(const string &key, const string &value)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_AUDIO, OperationType::QUERY);
    cmd.GetAbsRdbPredicates()->EqualTo(key, value);
    vector<string> columns;
    auto resultSet = g_rdbStore->Query(cmd, columns);
    int count = -1;
    if (resultSet->GetRowCount(count) != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get result set count");
        return -1;
    }
    MEDIA_DEBUG_LOG("Get AudioAsset count in db, key=%{public}s, value=%{public}s, count=%{public}d",
        key.c_str(), value.c_str(), count);
    return count;
}

void SetValuesBucketInUpdate(const string &columnKey, const string &columnValue,
    ValuesBucket &values)
{
    if (FILEASSET_MEMBER_MAP.find(columnKey) == FILEASSET_MEMBER_MAP.end()) {
        MEDIA_ERR_LOG("this columnKey %{public}s is not excepted", columnKey.c_str());
        return;
    }
    int type = FILEASSET_MEMBER_MAP.at(columnKey);
    switch (type) {
        case MEMBER_TYPE_INT32:
            values.PutInt(columnKey, stoi(columnValue));
            break;
        case MEMBER_TYPE_INT64:
            values.PutLong(columnKey, stol(columnValue));
            break;
        case MEMBER_TYPE_STRING:
            values.PutString(columnKey, columnValue);
            break;
        case MEMBER_TYPE_DOUBLE:
            values.PutDouble(columnKey, stod(columnValue));
            break;
        default:
            MEDIA_ERR_LOG("this column type %{public}s is not excepted", columnKey.c_str());
    }
}

static int32_t TestQueryAssetIntParams(int32_t intValue, const string &columnValue)
{
    int32_t columnIntValue = atoi(columnValue.c_str());
    if (columnIntValue == intValue) {
        return E_OK;
    } else {
        MEDIA_ERR_LOG("TestQueryAssetIntParams failed, intValue=%{public}d, columnValue=%{public}s",
            intValue, columnValue.c_str());
        return E_INVALID_VALUES;
    }
}

static int32_t TestQueryAssetLongParams(int64_t longValue, const string &columnValue)
{
    int64_t columnLongValue = atol(columnValue.c_str());
    if (columnLongValue == longValue) {
        return E_OK;
    } else {
        MEDIA_ERR_LOG("TestQueryAssetLongParams failed, intValue=%{public}ld, columnValue=%{public}s",
            static_cast<long>(longValue), columnValue.c_str());
        return E_INVALID_VALUES;
    }
}

static int32_t TestQueryAssetDoubleParams(double doubleValue, const string &columnValue)
{
    double columnDoubleValue = stod(columnValue);
    if (columnDoubleValue == doubleValue) {
        return E_OK;
    } else {
        MEDIA_ERR_LOG("TestQueryAssetDoubleParams failed, intValue=%{public}lf, columnValue=%{public}s",
            doubleValue, columnValue.c_str());
        return E_INVALID_VALUES;
    }
}

static int32_t TestQueryAssetStringParams(const string &stringValue, const string &columnValue)
{
    if (columnValue == stringValue) {
        return E_OK;
    } else {
        MEDIA_ERR_LOG("TestQueryAssetStringParams failed, stringValue=%{public}s, columnValue=%{public}s",
            stringValue.c_str(), columnValue.c_str());
        return E_INVALID_VALUES;
    }
}

int32_t TestQueryAsset(const string &queryKey, const string &queryValue, const string &columnKey,
    const string &columnValue, MediaLibraryApi api)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStoreRaw();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr!");
        return E_HAS_DB_ERROR;
    }

    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_AUDIO, OperationType::QUERY, api);
    cmd.GetAbsRdbPredicates()->EqualTo(queryKey, queryValue);
    vector<string> columns;
    columns.push_back(columnKey);
    auto queryResultSet = rdbStore->Query(cmd, columns);
    if (queryResultSet != nullptr && queryResultSet->GoToFirstRow() == NativeRdb::E_OK) {
        int type = FILEASSET_MEMBER_MAP.at(columnKey);
        switch (type) {
            case MEMBER_TYPE_INT32: {
                int intValue = GetInt32Val(columnKey, queryResultSet);
                return TestQueryAssetIntParams(intValue, columnValue);
            }
            case MEMBER_TYPE_INT64: {
                long longValue = GetInt64Val(columnKey, queryResultSet);
                return TestQueryAssetLongParams(longValue, columnValue);
            }
            case MEMBER_TYPE_STRING: {
                string value = GetStringVal(columnKey, queryResultSet);
                return TestQueryAssetStringParams(value, columnValue);
            }
            case MEMBER_TYPE_DOUBLE: {
                double doubleValue = get<double>(ResultSetUtils::GetValFromColumn(columnKey,
                    queryResultSet, TYPE_DOUBLE));
                return TestQueryAssetDoubleParams(doubleValue, columnValue);
            }
            default: {
                MEDIA_ERR_LOG("this column type %{public}s is not excepted", columnKey.c_str());
                return E_INVALID_VALUES;
            }
        }
    } else {
        MEDIA_ERR_LOG("Query Failed");
        return E_HAS_DB_ERROR;
    }
}
} // namespace

void TestAudioCreateParamsApi10(const string &displayName, int32_t type, int32_t result)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_AUDIO, OperationType::CREATE,
        MediaLibraryApi::API_10);
    ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_NAME, displayName);
    values.PutInt(MediaColumn::MEDIA_TYPE, type);
    cmd.SetValueBucket(values);
    int32_t ret = MediaLibraryAudioOperations::Create(cmd);
    EXPECT_EQ(ret, result);
}

void TestAudioDeleteParamsApi10(OperationObject oprnObject, int32_t fileId, ExceptIntFunction func)
{
    MediaLibraryCommand cmd(oprnObject, OperationType::DELETE, MediaLibraryApi::API_10);
    ValuesBucket values;
    values.PutInt(AudioColumn::MEDIA_ID, fileId);
    cmd.SetValueBucket(values);
    int32_t ret = MediaLibraryAudioOperations::Delete(cmd);
    func(ret);
}

void TestAudioUpdateParamsApi10(const string &predicateColumn, const string &predicateValue,
    const unordered_map<string, string> &updateColumns, ExceptIntFunction func)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_AUDIO, OperationType::UPDATE,
        MediaLibraryApi::API_10);
    ValuesBucket values;
    for (auto &iter : updateColumns) {
        SetValuesBucketInUpdate(iter.first, iter.second, values);
    }
    cmd.SetValueBucket(values);
    cmd.GetAbsRdbPredicates()->EqualTo(predicateColumn, predicateValue);
    int32_t ret = MediaLibraryAudioOperations::Update(cmd);
    func(ret);
}

void TestAudioUpdateByQueryApi10(const string &predicateColumn, const string &predicateValue,
    const unordered_map<string, string> &checkColumns, int32_t result)
{
    for (auto &iter : checkColumns) {
        int32_t errCode = TestQueryAsset(predicateColumn, predicateValue, iter.first, iter.second,
            MediaLibraryApi::API_OLD);
        EXPECT_EQ(errCode, result);
    }
}

void TestAudioUpdateParamsVerifyFunctionFailed(const string &predicateColumn, const string &predicateValue,
    const unordered_map<string, string> &updateColumns)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_AUDIO, OperationType::UPDATE,
        MediaLibraryApi::API_10);
    ValuesBucket values;
    for (auto &iter : updateColumns) {
        SetValuesBucketInUpdate(iter.first, iter.second, values);
    }
    cmd.SetValueBucket(values);
    cmd.GetAbsRdbPredicates()->EqualTo(predicateColumn, predicateValue);
    int32_t ret = MediaLibraryAssetOperations::UpdateOperation(cmd);
    MEDIA_INFO_LOG("column:%{public}s, predicates:%{public}s, ret:%{public}d",
        predicateColumn.c_str(), predicateValue.c_str(), ret);
    EXPECT_EQ(ret, E_INVALID_VALUES);
}

void TestAudioOpenParamsApi10(int32_t fileId, const string &mode, ExceptIntFunction func)
{
    string uriString = MediaFileUtils::GetMediaTypeUriV10(MediaType::MEDIA_TYPE_AUDIO);
    uriString += "/" + to_string(fileId);
    Uri uri(uriString);
    MediaLibraryCommand cmd(uri);
    int32_t fd = MediaLibraryAudioOperations::Open(cmd, mode);
    func(fd);
    if (fd > 0) {
        close(fd);
        MediaLibraryInotify::GetInstance()->RemoveByFileUri(cmd.GetUriStringWithoutSegment(),
            MediaLibraryApi::API_10);
    }
}

void TestAudioCloseParamsApi10(int32_t fileId, ExceptIntFunction func)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_AUDIO, OperationType::CLOSE);
    ValuesBucket values;
    values.PutInt(AudioColumn::MEDIA_ID, fileId);
    cmd.SetValueBucket(values);
    int32_t ret = MediaLibraryAudioOperations::Close(cmd);
    func(ret);
}

void MediaLibraryAudioOperationsTest::SetUpTestCase()
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStoreRaw();
    if (g_rdbStore == nullptr || g_rdbStore->GetRaw() == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryAudioOperationsTest failed, can not get rdbstore");
        exit(1);
    }
}

void MediaLibraryAudioOperationsTest::TearDownTestCase()
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    system("rm -rf /storage/media/local/files/*");
    ClearAndRestart();
    g_rdbStore = nullptr;
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    this_thread::sleep_for(chrono::seconds(1));
    MEDIA_INFO_LOG("Clean is finish");
}

void MediaLibraryAudioOperationsTest::SetUp()
{
    if (g_rdbStore == nullptr || g_rdbStore->GetRaw() == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryAudioOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    ClearAndRestart();
}

void MediaLibraryAudioOperationsTest::TearDown()
{}

const string CHAR256_ENGLISH =
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
const string CHAR256_CHINESE =
    "中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中"
    "中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中";

HWTEST_F(MediaLibraryAudioOperationsTest, audio_oprn_create_api10_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd audio_oprn_create_api10_test_001");
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_AUDIO, OperationType::CREATE,
        MediaLibraryApi::API_10);
    string name = "audio.mp3";
    ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_NAME, name);
    values.PutInt(MediaColumn::MEDIA_TYPE, MediaType::MEDIA_TYPE_AUDIO);
    cmd.SetValueBucket(values);
    int32_t ret = MediaLibraryAudioOperations::Create(cmd);
    EXPECT_GE(ret, 0);
    unordered_map<string, string> verifyMap = {
        { AudioColumn::MEDIA_TITLE, "audio" },
        { AudioColumn::MEDIA_TYPE, to_string(MediaType::MEDIA_TYPE_AUDIO) },
        { AudioColumn::MEDIA_TIME_PENDING, to_string(UNCREATE_FILE_TIMEPENDING)}
    };
    bool res = QueryAndVerifyAudioAsset(AudioColumn::MEDIA_NAME, name, verifyMap);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("end tdd audio_oprn_create_api10_test_001");
}

HWTEST_F(MediaLibraryAudioOperationsTest, audio_oprn_create_api10_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd audio_oprn_create_api10_test_002");
    string defaultRelativePath = "Pictures/1/";
    TestAudioCreateParamsApi10("", MediaType::MEDIA_TYPE_AUDIO, E_INVALID_DISPLAY_NAME);
    TestAudioCreateParamsApi10("audio\"\".mp3", MediaType::MEDIA_TYPE_AUDIO, E_INVALID_DISPLAY_NAME);
    TestAudioCreateParamsApi10(".audio.mp3", MediaType::MEDIA_TYPE_AUDIO, E_INVALID_DISPLAY_NAME);
    string englishLongString = CHAR256_ENGLISH + ".mp3";
    TestAudioCreateParamsApi10(englishLongString, MediaType::MEDIA_TYPE_AUDIO,
        E_INVALID_DISPLAY_NAME);
    string chineseLongString = CHAR256_CHINESE + ".mp3";
    TestAudioCreateParamsApi10(chineseLongString, MediaType::MEDIA_TYPE_AUDIO,
        E_INVALID_DISPLAY_NAME);

    TestAudioCreateParamsApi10("audio", MediaType::MEDIA_TYPE_AUDIO, E_INVALID_DISPLAY_NAME);
    TestAudioCreateParamsApi10("audio.", MediaType::MEDIA_TYPE_AUDIO, E_INVALID_DISPLAY_NAME);
    TestAudioCreateParamsApi10("audio.abc", MediaType::MEDIA_TYPE_AUDIO,
        E_CHECK_MEDIATYPE_MATCH_EXTENSION_FAIL);
    TestAudioCreateParamsApi10("audio.mp4", MediaType::MEDIA_TYPE_AUDIO,
        E_CHECK_MEDIATYPE_MATCH_EXTENSION_FAIL);
    MEDIA_INFO_LOG("end tdd audio_oprn_create_api10_test_002");
}

HWTEST_F(MediaLibraryAudioOperationsTest, audio_oprn_delete_api10_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd audio_oprn_delete_api10_test_001");

    // set audio
    int fileId = SetDefaultAudioApi10(MediaType::MEDIA_TYPE_AUDIO, "audio.mp3");
    if (fileId < E_OK) {
        MEDIA_ERR_LOG("Set Default audio failed, ret = %{public}d", fileId);
        return;
    }
    string filePath = GetFilePath(fileId);
    if (filePath.empty()) {
        MEDIA_ERR_LOG("Get filePath failed");
        return;
    }

    EXPECT_EQ(MediaFileUtils::IsFileExists(filePath), true);
    int32_t count = GetAudioAssetCountIndb(AudioColumn::MEDIA_NAME, "audio.mp3");
    EXPECT_EQ(count, 1);

    // test delete
    static constexpr int largeNum = 1000;
    TestAudioDeleteParamsApi10(OperationObject::ASSETMAP, fileId,
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_FILEID); });
    TestAudioDeleteParamsApi10(OperationObject::FILESYSTEM_AUDIO, fileId + largeNum,
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_FILEID); });
    TestAudioDeleteParamsApi10(OperationObject::FILESYSTEM_AUDIO, fileId,
        [] (int32_t result) { EXPECT_GT(result, 0); });

    // test delete result
    EXPECT_EQ(MediaFileUtils::IsFileExists(filePath), false);
    count = GetAudioAssetCountIndb(AudioColumn::MEDIA_NAME, "audio.mp3");
    EXPECT_EQ(count, 0);

    MEDIA_INFO_LOG("end tdd audio_oprn_delete_api10_test_001");
}

HWTEST_F(MediaLibraryAudioOperationsTest, audio_oprn_query_api10_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd audio_oprn_query_api10_test_001");

    int32_t fileId1 = SetDefaultAudioApi10(MediaType::MEDIA_TYPE_AUDIO, "audio1.mp3");
    if (fileId1 < E_OK) {
        MEDIA_ERR_LOG("Set Default audio failed, ret = %{public}d", fileId1);
        return;
    }

    // Query
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_AUDIO, OperationType::QUERY,
        MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->EqualTo(AudioColumn::MEDIA_ID, to_string(fileId1));
    vector<string> columns;
    auto resultSet = MediaLibraryAudioOperations::Query(cmd, columns);
    if (resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        string name = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
        EXPECT_EQ(name, "audio1.mp3");
    } else {
        MEDIA_ERR_LOG("Test first tdd Query failed");
        return;
    }

    MEDIA_INFO_LOG("end tdd audio_oprn_query_api10_test_001");
}

HWTEST_F(MediaLibraryAudioOperationsTest, audio_oprn_query_api10_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd audio_oprn_query_api10_test_002");

    int32_t fileId1 = SetDefaultAudioApi10(MediaType::MEDIA_TYPE_AUDIO, "audio1.mp3");
    if (fileId1 < E_OK) {
        MEDIA_ERR_LOG("Set Default audio failed, ret = %{public}d", fileId1);
        return;
    }
    int32_t fileId2 = SetDefaultAudioApi10(MediaType::MEDIA_TYPE_AUDIO, "audio2.mp3");
    if (fileId2 < E_OK) {
        MEDIA_ERR_LOG("Set Default audio failed, ret = %{public}d", fileId2);
        return;
    }

    // Query
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_AUDIO, OperationType::QUERY,
        MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->BeginWrap()->EqualTo(MediaColumn::MEDIA_ID, to_string(fileId1))->
        Or()->EqualTo(MediaColumn::MEDIA_ID, to_string(fileId2))->EndWrap()->
        OrderByAsc(MediaColumn::MEDIA_ID);
    vector<string> columns;
    auto resultSet = MediaLibraryAudioOperations::Query(cmd, columns);
    if (resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        string name = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
        EXPECT_EQ(name, "audio1.mp3");
    } else {
        MEDIA_ERR_LOG("Test first tdd Query failed");
        return;
    }

    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        string name = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
        EXPECT_EQ(name, "audio2.mp3");
    } else {
        MEDIA_ERR_LOG("Test second tdd Query failed");
        return;
    }

    MEDIA_INFO_LOG("end tdd audio_oprn_query_api10_test_002");
}

HWTEST_F(MediaLibraryAudioOperationsTest, audio_oprn_query_api10_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd audio_oprn_query_api10_test_003");

    int32_t fileId1 = SetDefaultAudioApi10(MediaType::MEDIA_TYPE_AUDIO, "audio1.mp3");
    if (fileId1 < E_OK) {
        MEDIA_ERR_LOG("Set Default audio failed, ret = %{public}d", fileId1);
        return;
    }

    // Query
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_AUDIO, OperationType::QUERY,
        MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->EqualTo(AudioColumn::MEDIA_ID, to_string(fileId1));
    vector<string> columns;
    columns.push_back(MediaColumn::MEDIA_NAME);
    columns.push_back(MediaColumn::MEDIA_DATE_ADDED);
    auto resultSet = MediaLibraryAudioOperations::Query(cmd, columns);
    if (resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        string name = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
        EXPECT_EQ(name, "audio1.mp3");
        int64_t dateAdded = GetInt64Val(MediaColumn::MEDIA_DATE_ADDED, resultSet);
        EXPECT_GE(dateAdded, 0L);
        int64_t dateModified = GetInt64Val(MediaColumn::MEDIA_DATE_MODIFIED, resultSet);
        EXPECT_EQ(dateModified, 0L);
    } else {
        MEDIA_ERR_LOG("Test first tdd Query failed");
        return;
    }
    MEDIA_INFO_LOG("end tdd audio_oprn_query_api10_test_003");
}

HWTEST_F(MediaLibraryAudioOperationsTest, audio_oprn_update_api10_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd audio_oprn_update_api10_test_001");
    int32_t fileId = CreateAudioApi10(MediaType::MEDIA_TYPE_AUDIO, "audio.mp3");
    if (fileId < 0) {
        MEDIA_ERR_LOG("CreateAudio In APi10 failed, ret=%{public}d", fileId);
        return;
    }

    TestAudioUpdateParamsApi10(AudioColumn::MEDIA_ID, to_string(fileId),
        { { AudioColumn::MEDIA_NAME, "aud.mp3" } },
        [] (int32_t result) { EXPECT_GE(result, E_OK); });
    TestAudioUpdateByQueryApi10(AudioColumn::MEDIA_ID, to_string(fileId),
        { { AudioColumn::MEDIA_NAME, "aud.mp3" } }, E_OK);
    MEDIA_INFO_LOG("end tdd audio_oprn_update_api10_test_001");
}

HWTEST_F(MediaLibraryAudioOperationsTest, audio_oprn_update_api10_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd audio_oprn_update_api10_test_002");
    int32_t fileId = CreateAudioApi10(MediaType::MEDIA_TYPE_AUDIO, "audio.mp3");
    if (fileId < 0) {
        MEDIA_ERR_LOG("CreateAudio In APi10 failed, ret=%{public}d", fileId);
        return;
    }

    TestAudioUpdateParamsApi10(AudioColumn::MEDIA_ID, to_string(fileId),
        { { AudioColumn::MEDIA_NAME, "" } },
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_DISPLAY_NAME); });
    TestAudioUpdateParamsApi10(AudioColumn::MEDIA_ID, to_string(fileId),
        { { AudioColumn::MEDIA_NAME, "audio\"\".mp3" } },
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_DISPLAY_NAME); });
    TestAudioUpdateParamsApi10(AudioColumn::MEDIA_ID, to_string(fileId),
        { { AudioColumn::MEDIA_NAME, ".audio.mp3" } },
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_DISPLAY_NAME); });

    string englishLongString = CHAR256_ENGLISH + ".mp3";
    string chineseLongString = CHAR256_CHINESE + ".mp3";
    TestAudioUpdateParamsApi10(AudioColumn::MEDIA_ID, to_string(fileId),
        { { AudioColumn::MEDIA_NAME, englishLongString } },
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_DISPLAY_NAME); });
    TestAudioUpdateParamsApi10(AudioColumn::MEDIA_ID, to_string(fileId),
        { { AudioColumn::MEDIA_NAME, chineseLongString } },
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_DISPLAY_NAME); });

    TestAudioUpdateParamsApi10(AudioColumn::MEDIA_ID, to_string(fileId),
        { { AudioColumn::MEDIA_NAME, "audio" } },
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_DISPLAY_NAME); });
    TestAudioUpdateParamsApi10(AudioColumn::MEDIA_ID, to_string(fileId),
        { { AudioColumn::MEDIA_NAME, "audio." } },
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_DISPLAY_NAME); });
    TestAudioUpdateParamsApi10(AudioColumn::MEDIA_ID, to_string(fileId),
        { { AudioColumn::MEDIA_NAME, "audio.abc" } },
        [] (int32_t result) { EXPECT_EQ(result, E_CHECK_MEDIATYPE_MATCH_EXTENSION_FAIL); });
    TestAudioUpdateParamsApi10(AudioColumn::MEDIA_ID, to_string(fileId),
        { { AudioColumn::MEDIA_NAME, "audio.mp4" } },
        [] (int32_t result) { EXPECT_EQ(result, E_CHECK_MEDIATYPE_MATCH_EXTENSION_FAIL); });

    MEDIA_INFO_LOG("end tdd audio_oprn_update_api10_test_002");
}

HWTEST_F(MediaLibraryAudioOperationsTest, audio_oprn_update_api10_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd audio_oprn_update_api10_test_003");
    int32_t fileId = CreateAudioApi10(MediaType::MEDIA_TYPE_AUDIO, "audio.mp3");
    if (fileId < 0) {
        MEDIA_ERR_LOG("CreateAudio In APi10 failed, ret=%{public}d", fileId);
        return;
    }

    unordered_map<string, string> updateMap1 = {
        { AudioColumn::MEDIA_TITLE, "audio1" },
        { AudioColumn::MEDIA_NAME, "audio2.mp3" }
    };
    TestAudioUpdateParamsApi10(AudioColumn::MEDIA_ID, to_string(fileId), updateMap1,
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_DISPLAY_NAME); });

    unordered_map<string, string> updateMap2 = {
        { AudioColumn::MEDIA_TITLE, "audio2" },
        { AudioColumn::MEDIA_NAME, "audio2.mp3" }
    };
    TestAudioUpdateParamsApi10(AudioColumn::MEDIA_ID, to_string(fileId), updateMap2,
    [] (int32_t result) { EXPECT_GE(result, E_OK); });

    MEDIA_INFO_LOG("end tdd audio_oprn_update_api10_test_003");
}

HWTEST_F(MediaLibraryAudioOperationsTest, audio_oprn_update_api10_test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd audio_oprn_update_api10_test_004");
    int32_t fileId1 = CreateAudioApi10(MediaType::MEDIA_TYPE_AUDIO, "audio.mp3");
    if (fileId1 < 0) {
        MEDIA_ERR_LOG("CreateAudio In APi10 failed, ret=%{public}d", fileId1);
        return;
    }

    int32_t fileId2 = CreateAudioApi10(MediaType::MEDIA_TYPE_AUDIO, "aud.mp3");
    if (fileId2 < 0) {
        MEDIA_ERR_LOG("CreateAudio In APi10 failed, ret=%{public}d", fileId2);
        return;
    }

    TestAudioUpdateParamsApi10(AudioColumn::MEDIA_ID, to_string(fileId1),
        { { AudioColumn::MEDIA_TITLE, "audio1" } },
        [] (int32_t result) { EXPECT_GE(result, E_OK); });
    TestAudioUpdateByQueryApi10(AudioColumn::MEDIA_ID, to_string(fileId1),
        { { AudioColumn::MEDIA_NAME, "audio1.mp3" } }, E_OK);

    TestAudioUpdateParamsApi10(AudioColumn::MEDIA_ID, to_string(fileId2),
        { { AudioColumn::MEDIA_NAME, "audio2.mp3" } },
        [] (int32_t result) { EXPECT_GE(result, E_OK); });
    TestAudioUpdateByQueryApi10(AudioColumn::MEDIA_ID, to_string(fileId2),
        { { AudioColumn::MEDIA_TITLE, "audio2" } }, E_OK);

    MEDIA_INFO_LOG("end tdd audio_oprn_update_api10_test_004");
}

HWTEST_F(MediaLibraryAudioOperationsTest, audio_oprn_update_api10_test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd audio_oprn_update_api10_test_005");
    int32_t fileId = SetDefaultAudioApi10(MediaType::MEDIA_TYPE_AUDIO, "audio.mp3");
    if (fileId < 0) {
        MEDIA_ERR_LOG("CreateAudio In APi10 failed, ret=%{public}d", fileId);
        return;
    }

    MediaLibraryCommand queryPathCmd(OperationObject::FILESYSTEM_AUDIO, OperationType::QUERY);
    queryPathCmd.GetAbsRdbPredicates()->EqualTo(AudioColumn::MEDIA_ID, to_string(fileId));
    vector<string> columns = { AudioColumn::MEDIA_FILE_PATH };
    auto resultSet = g_rdbStore->Query(queryPathCmd, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get resultSet");
        return;
    }
    string path = GetStringVal(AudioColumn::MEDIA_FILE_PATH, resultSet);
    if (path.empty()) {
        MEDIA_ERR_LOG("Get Path failed");
        return;
    }

    string moveToPath = "/storage/media/local/files/Audios/1.mp3";
    static constexpr int largeNum = 1000;
    TestAudioUpdateParamsApi10(AudioColumn::MEDIA_ID, to_string(fileId + largeNum),
        { { AudioColumn::MEDIA_FILE_PATH, moveToPath } },
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_VALUES); });
    EXPECT_EQ(MediaFileUtils::IsFileExists(moveToPath), false);
    EXPECT_EQ(MediaFileUtils::IsFileExists(path), true);
    TestAudioUpdateParamsApi10(AudioColumn::MEDIA_ID, to_string(fileId),
        { { AudioColumn::MEDIA_FILE_PATH, moveToPath } },
        [] (int32_t result) { EXPECT_GE(result, E_OK); });
    EXPECT_EQ(MediaFileUtils::IsFileExists(moveToPath), true);
    EXPECT_EQ(MediaFileUtils::IsFileExists(path), false);

    MEDIA_INFO_LOG("end tdd audio_oprn_update_api10_test_005");
}

HWTEST_F(MediaLibraryAudioOperationsTest, audio_oprn_update_api10_test_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd audio_oprn_update_api10_test_006");
    int32_t fileId = CreateAudioApi10(MediaType::MEDIA_TYPE_AUDIO, "audio.mp3");
    if (fileId < 0) {
        MEDIA_ERR_LOG("CreateAudio In APi10 failed, ret=%{public}d", fileId);
        return;
    }

    TestAudioUpdateParamsVerifyFunctionFailed(AudioColumn::MEDIA_ID, to_string(fileId),
        { { AudioColumn::MEDIA_ID, "1"} });
    TestAudioUpdateParamsVerifyFunctionFailed(AudioColumn::MEDIA_ID, to_string(fileId),
        { { AudioColumn::MEDIA_URI, "datashare:///media/image/1"} });
    TestAudioUpdateParamsVerifyFunctionFailed(AudioColumn::MEDIA_ID, to_string(fileId),
        { { AudioColumn::MEDIA_FILE_PATH, ""} });
    TestAudioUpdateParamsVerifyFunctionFailed(AudioColumn::MEDIA_ID, to_string(fileId),
        { { { AudioColumn::MEDIA_FILE_PATH, ""}, { AudioColumn::MEDIA_TITLE, "123" } } });
    TestAudioUpdateParamsVerifyFunctionFailed(AudioColumn::MEDIA_ID, to_string(fileId),
        { { AudioColumn::MEDIA_SIZE, "12345"} });
    TestAudioUpdateParamsVerifyFunctionFailed(AudioColumn::MEDIA_ID, to_string(fileId),
        { { AudioColumn::MEDIA_TITLE, ""} });
    TestAudioUpdateParamsVerifyFunctionFailed(AudioColumn::MEDIA_ID, to_string(fileId),
        { { AudioColumn::MEDIA_NAME, ""} });
    TestAudioUpdateParamsVerifyFunctionFailed(AudioColumn::MEDIA_ID, to_string(fileId),
        { { AudioColumn::MEDIA_DATE_MODIFIED, "1000000"} });
    TestAudioUpdateParamsVerifyFunctionFailed(AudioColumn::MEDIA_ID, to_string(fileId),
        { { AudioColumn::MEDIA_DATE_ADDED, "1000000"} });
    TestAudioUpdateParamsVerifyFunctionFailed(AudioColumn::MEDIA_ID, to_string(fileId),
        { { AudioColumn::MEDIA_DATE_TAKEN, "1000000"} });
    TestAudioUpdateParamsVerifyFunctionFailed(AudioColumn::MEDIA_ID, to_string(fileId),
        { { AudioColumn::MEDIA_DURATION, "1000000"} });
    TestAudioUpdateParamsVerifyFunctionFailed(AudioColumn::MEDIA_ID, to_string(fileId),
        { { AudioColumn::MEDIA_TIME_PENDING, "1000000"}, { AudioColumn::MEDIA_TITLE, "123" } });
    TestAudioUpdateParamsVerifyFunctionFailed(AudioColumn::MEDIA_ID, to_string(fileId),
        { { AudioColumn::MEDIA_DATE_TRASHED, "1000000"}, { AudioColumn::MEDIA_TITLE, "123" } });
    TestAudioUpdateParamsVerifyFunctionFailed(AudioColumn::MEDIA_ID, to_string(fileId),
        { { AudioColumn::MEDIA_DATE_DELETED, "1000000"}, { AudioColumn::MEDIA_TITLE, "123" } });
    TestAudioUpdateParamsVerifyFunctionFailed(AudioColumn::MEDIA_ID, to_string(fileId),
        { { AudioColumn::MEDIA_HIDDEN, "1"}, { AudioColumn::MEDIA_TITLE, "123" } });
    TestAudioUpdateParamsVerifyFunctionFailed(AudioColumn::MEDIA_ID, to_string(fileId),
        { { AudioColumn::MEDIA_IS_FAV, "1"}, { AudioColumn::MEDIA_TITLE, "123" } });

    MEDIA_INFO_LOG("end tdd audio_oprn_update_api10_test_006");
}

HWTEST_F(MediaLibraryAudioOperationsTest, audio_oprn_update_api10_test_007, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd audio_oprn_update_api10_test_007");
    int32_t fileId = CreateAudioApi10(MediaType::MEDIA_TYPE_AUDIO, "audio.mp3");
    if (fileId < 0) {
        MEDIA_ERR_LOG("CreateAudio In APi10 failed, ret=%{public}d", fileId);
        return;
    }

    TestAudioUpdateParamsVerifyFunctionFailed(AudioColumn::MEDIA_ID, to_string(fileId),
        { { AudioColumn::AUDIO_ARTIST, "1"} });
    TestAudioUpdateParamsVerifyFunctionFailed(AudioColumn::MEDIA_ID, to_string(fileId),
        { { AudioColumn::AUDIO_ALBUM, "1"} });

    MEDIA_INFO_LOG("end tdd audio_oprn_update_api10_test_007");
}

HWTEST_F(MediaLibraryAudioOperationsTest, audio_oprn_open_api10_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd audio_oprn_open_api10_test_001");

    int fileId = SetDefaultAudioApi10(MediaType::MEDIA_TYPE_AUDIO, "audio.mp3");
    if (fileId < 0) {
        MEDIA_ERR_LOG("Create audio failed error=%{public}d", fileId);
        return;
    }

    static constexpr int largeNum = 1000;
    TestAudioOpenParamsApi10(fileId, "",
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_MODE); });
    TestAudioOpenParamsApi10(fileId, "m",
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_MODE); });
    TestAudioOpenParamsApi10(fileId + largeNum, "rw",
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_URI); });
    TestAudioOpenParamsApi10(fileId, "rw",
        [] (int32_t result) { EXPECT_GE(result, E_OK); });

    MEDIA_INFO_LOG("end tdd audio_oprn_open_api10_test_001");
}

HWTEST_F(MediaLibraryAudioOperationsTest, audio_oprn_close_api10_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd audio_oprn_close_api10_test_001");

    int fileId = SetDefaultAudioApi10(MediaType::MEDIA_TYPE_AUDIO, "audio.mp3");
    if (fileId < 0) {
        MEDIA_ERR_LOG("Create audio failed error=%{public}d", fileId);
        return;
    }

    static constexpr int largeNum = 1000;
    TestAudioCloseParamsApi10(fileId + largeNum,
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_FILEID); });

    MEDIA_INFO_LOG("end tdd audio_oprn_close_api10_test_001");
}

HWTEST_F(MediaLibraryAudioOperationsTest, audio_oprn_pending_api10_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd audio_oprn_pending_api10_test_001");
    MediaLibraryCommand createCmd(OperationObject::FILESYSTEM_AUDIO, OperationType::CREATE,
        MediaLibraryApi::API_10);
    string name = "audio.mp3";
    ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_NAME, name);
    values.PutInt(MediaColumn::MEDIA_TYPE, MediaType::MEDIA_TYPE_AUDIO);
    createCmd.SetValueBucket(values);
    int32_t fileId = MediaLibraryAudioOperations::Create(createCmd);
    EXPECT_GE(fileId, 0);

    string uriString = MediaFileUtils::GetFileMediaTypeUriV10(MediaType::MEDIA_TYPE_AUDIO, "");
    uriString += "/" + to_string(fileId);
    Uri uri(uriString);
    MediaLibraryCommand openCmd(uri);
    int32_t fd = MediaLibraryAudioOperations::Open(openCmd, "rw");
    EXPECT_GE(fd, 0);

    MediaLibraryCommand queryCmd(OperationObject::FILESYSTEM_AUDIO, OperationType::QUERY, MediaLibraryApi::API_10);
    queryCmd.GetAbsRdbPredicates()->EqualTo(AudioColumn::MEDIA_ID, to_string(fileId));
    vector<string> columns = { AudioColumn::MEDIA_TIME_PENDING };
    auto resultSet = g_rdbStore->Query(queryCmd, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get AssetUniqueNumberTable count");
        return;
    }
    int64_t timePending = GetInt64Val(AudioColumn::MEDIA_TIME_PENDING, resultSet);
    EXPECT_GT(timePending, 0);

    MEDIA_INFO_LOG("end tdd audio_oprn_pending_api10_test_001");
}
} // namespace Media
} // namespace OHOS