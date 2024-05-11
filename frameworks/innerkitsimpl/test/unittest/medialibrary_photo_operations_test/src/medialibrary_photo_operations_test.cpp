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

#define MLOG_TAG "PhotoOperationsTest"

#include "medialibrary_photo_operations_test.h"

#include <chrono>
#include <fcntl.h>
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
#include "medialibrary_db_const_sqls.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "medialibrary_inotify.h"
#include "medialibrary_photo_operations.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "media_file_ext_ability.h"
#include "media_file_extention_utils.h"
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

const string COMMON_PREFIX = "datashare:///media/";
const string ROOT_URI = "root";

using ExceptIntFunction = void (*) (int32_t);
using ExceptLongFunction = void (*) (int64_t);
using ExceptBoolFunction = void (*) (bool);
using ExceptStringFunction = void (*) (const string&);

namespace {
void CleanTestTables()
{
    vector<string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
        AudioColumn::AUDIOS_TABLE,
        MEDIALIBRARY_TABLE,
        ASSET_UNIQUE_NUMBER_TABLE
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

class ArkJsRuntime : public AbilityRuntime::JsRuntime {
public:
    ArkJsRuntime() {};

    ~ArkJsRuntime() {};

    void StartDebugMode(const DebugOption debugOption) {};
    void FinishPreload() {};
    bool LoadRepairPatch(const string& patchFile, const string& baseFile)
    {
        return true;
    };
    bool NotifyHotReloadPage()
    {
        return true;
    };
    bool UnLoadRepairPatch(const string& patchFile)
    {
        return true;
    };
    bool RunScript(const string& path, const string& hapPath, bool useCommonChunk = false)
    {
        return true;
    };
};

#ifdef FILEEXT
void DisplayFileList(const vector<FileAccessFwk::FileInfo> &fileList)
{
    for (auto t : fileList) {
        MEDIA_DEBUG_LOG("medialib_ListFile_test_001 file.uri: %s, file.fileName: %s, file.mode: %d, file.mimeType: %s",
            t.uri.c_str(), t.fileName.c_str(), t.mode, t.mimeType.c_str());
    }
}
#endif

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
        CREATE_MEDIA_TABLE,
        CREATE_ASSET_UNIQUE_NUMBER_TABLE
        // todo: album tables
    };
    for (auto &createTableSql : createTableSqlList) {
        if (g_rdbStore == nullptr) {
            MEDIA_ERR_LOG("can not get g_rdbstore");
            return;
        }
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

unique_ptr<FileAsset> QueryPhotoAsset(const string &columnName, const string &value)
{
    string querySql = "SELECT * FROM " + PhotoColumn::PHOTOS_TABLE + " WHERE " +
        columnName + "='" + value + "';";

    MEDIA_DEBUG_LOG("querySql: %{public}s", querySql.c_str());
    auto resultSet = g_rdbStore->QuerySql(querySql);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Get resultSet failed");
        return nullptr;
    }

    int32_t resultSetCount = 0;
    int32_t ret = resultSet->GetRowCount(resultSetCount);
    if (ret != NativeRdb::E_OK || resultSetCount <= 0) {
        MEDIA_ERR_LOG("resultSet row count is 0");
        return nullptr;
    }

    shared_ptr<FetchResult<FileAsset>> fetchFileResult = make_shared<FetchResult<FileAsset>>();
    if (fetchFileResult == nullptr) {
        MEDIA_ERR_LOG("Get fetchFileResult failed");
        return nullptr;
    }
    auto fileAsset = fetchFileResult->GetObjectFromRdb(resultSet, 0);
    if (fileAsset == nullptr || fileAsset->GetId() < 0) {
        return nullptr;
    }
    return fileAsset;
}

bool QueryAndVerifyPhotoAsset(const string &columnName, const string &value,
    const unordered_map<string, string> &verifyColumnAndValuesMap)
{
    auto fileAsset = QueryPhotoAsset(columnName, value);
    if (fileAsset != nullptr) {
        for (const auto &iter : verifyColumnAndValuesMap) {
            string resStr = GetFileAssetValueToStr(*fileAsset, iter.first);
            if (resStr.empty()) {
                MEDIA_ERR_LOG("verify failed! Param %{public}s is empty", iter.first.c_str());
                return false;
            }
            if (resStr != iter.second) {
                MEDIA_ERR_LOG("verify failed! Except %{public}s param %{public}s, actually %{public}s",
                    iter.first.c_str(), iter.second.c_str(), resStr.c_str());
                return false;
            }
        }
    } else {
        MEDIA_ERR_LOG("Query failed! Can not find this file");
        return false;
    }
    return true;
}

inline int32_t CreatePhotoApi9(int mediaType, const string &displayName, const string &relativePath)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE,
        MediaLibraryApi::API_OLD);
    ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_NAME, displayName);
    values.PutInt(MediaColumn::MEDIA_TYPE, mediaType);
    values.PutString(MediaColumn::MEDIA_RELATIVE_PATH, relativePath);
    cmd.SetValueBucket(values);
    int32_t ret = MediaLibraryPhotoOperations::Create(cmd);
    if (ret < 0) {
        MEDIA_ERR_LOG("Create Photo failed, errCode=%{public}d", ret);
        return ret;
    }
    return ret;
}

inline int32_t CreatePhotoApi10(int mediaType, const string &displayName)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE,
        MediaLibraryApi::API_10);
    ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_NAME, displayName);
    values.PutInt(MediaColumn::MEDIA_TYPE, mediaType);
    cmd.SetValueBucket(values);
    int32_t ret = MediaLibraryPhotoOperations::Create(cmd);
    if (ret < 0) {
        MEDIA_ERR_LOG("Create Photo failed, errCode=%{public}d", ret);
        return ret;
    }
    return ret;
}

string GetFilePath(int fileId)
{
    if (fileId < 0) {
        MEDIA_ERR_LOG("this file id %{private}d is invalid", fileId);
        return "";
    }

    vector<string> columns = { PhotoColumn::MEDIA_FILE_PATH };
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY,
        MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("can not get rdbstore");
        return "";
    }
    auto resultSet = g_rdbStore->Query(cmd, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get file Path");
        return "";
    }
    string path = GetStringVal(PhotoColumn::MEDIA_FILE_PATH, resultSet);
    return path;
}

int32_t MakePhotoUnpending(int fileId)
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
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    ValuesBucket values;
    values.PutLong(PhotoColumn::MEDIA_TIME_PENDING, 0);
    cmd.SetValueBucket(values);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    int32_t changedRows = -1;
    errCode = g_rdbStore->Update(cmd, changedRows);
    if (errCode != E_OK || changedRows <= 0) {
        MEDIA_ERR_LOG("Update pending failed, errCode = %{public}d, changeRows = %{public}d",
            errCode, changedRows);
        return errCode;
    }

    return E_OK;
}

int32_t SetPendingOnly(int32_t pendingTime, int64_t fileId)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    ValuesBucket values;
    values.PutLong(PhotoColumn::MEDIA_TIME_PENDING, pendingTime);
    cmd.SetValueBucket(values);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    int32_t changedRows = -1;
    int32_t errCode = g_rdbStore->Update(cmd, changedRows);
    if (errCode != E_OK || changedRows <= 0) {
        MEDIA_ERR_LOG("Update pending failed, errCode = %{public}d, changeRows = %{public}d",
            errCode, changedRows);
        return errCode;
    }

    return E_OK;
}

int32_t SetDefaultPhotoApi9(int mediaType, const string &displayName, const string &relativePath)
{
    int fileId = CreatePhotoApi9(mediaType, displayName, relativePath);
    if (fileId < 0) {
        MEDIA_ERR_LOG("create photo failed, res=%{public}d", fileId);
        return fileId;
    }
    int32_t errCode = MakePhotoUnpending(fileId);
    if (errCode != E_OK) {
        return errCode;
    }
    return fileId;
}

int32_t SetDefaultPhotoApi10(int mediaType, const string &displayName)
{
    int fileId = CreatePhotoApi10(mediaType, displayName);
    if (fileId < 0) {
        MEDIA_ERR_LOG("create photo failed, res=%{public}d", fileId);
        return fileId;
    }
    int32_t errCode = MakePhotoUnpending(fileId);
    if (errCode != E_OK) {
        return errCode;
    }
    return fileId;
}

int32_t GetPhotoAssetCountIndb(const string &key, const string &value)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    cmd.GetAbsRdbPredicates()->EqualTo(key, value);
    vector<string> columns;
    auto resultSet = g_rdbStore->Query(cmd, columns);
    int count = -1;
    if (resultSet->GetRowCount(count) != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get result set count");
        return -1;
    }
    MEDIA_DEBUG_LOG("Get PhotoAsset count in db, key=%{public}s, value=%{public}s, count=%{public}d",
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

    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY, api);
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

void TestPhotoCreateParamsApi9(const string &displayName, int32_t type, const string &relativePath,
    int32_t result)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE,
        MediaLibraryApi::API_OLD);
    ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_NAME, displayName);
    values.PutInt(MediaColumn::MEDIA_TYPE, type);
    values.PutString(MediaColumn::MEDIA_RELATIVE_PATH, relativePath);
    cmd.SetValueBucket(values);
    int32_t ret = MediaLibraryPhotoOperations::Create(cmd);
    EXPECT_EQ(ret, result);
}

void TestPhotoCreateParamsApi10(const string &displayName, int32_t type, int32_t result)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE,
        MediaLibraryApi::API_10);
    ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_NAME, displayName);
    values.PutInt(MediaColumn::MEDIA_TYPE, type);
    cmd.SetValueBucket(values);
    int32_t ret = MediaLibraryPhotoOperations::Create(cmd);
    EXPECT_EQ(ret, result);
}

void TestPhotoCreateWithExtApi10(const string &extention, int32_t type, int32_t result)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE,
        MediaLibraryApi::API_10);
    ValuesBucket values;
    values.PutString("extention", extention);
    values.PutInt(MediaColumn::MEDIA_TYPE, type);
    cmd.SetValueBucket(values);
    int32_t ret = MediaLibraryPhotoOperations::Create(cmd);
    EXPECT_EQ(ret, result);
}

void TestPhotoDeleteParamsApi10(OperationObject oprnObject, int32_t fileId, ExceptIntFunction func)
{
    MediaLibraryCommand cmd(oprnObject, OperationType::DELETE, MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    int32_t ret = MediaLibraryPhotoOperations::Delete(cmd);
    func(ret);
}

void TestPhotoUpdateParamsApi9(const string &predicateColumn, const string &predicateValue,
    const unordered_map<string, string> &updateColumns, ExceptIntFunction func)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE,
        MediaLibraryApi::API_OLD);
    ValuesBucket values;
    for (auto &iter : updateColumns) {
        SetValuesBucketInUpdate(iter.first, iter.second, values);
    }
    cmd.SetValueBucket(values);
    cmd.GetAbsRdbPredicates()->EqualTo(predicateColumn, predicateValue);
    int32_t ret = MediaLibraryPhotoOperations::Update(cmd);
    func(ret);
}

void TestPhotoUpdateParamsApi10(const string &predicateColumn, const string &predicateValue,
    const unordered_map<string, string> &updateColumns, ExceptIntFunction func)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE,
        MediaLibraryApi::API_10);
    ValuesBucket values;
    for (auto &iter : updateColumns) {
        SetValuesBucketInUpdate(iter.first, iter.second, values);
    }
    cmd.SetValueBucket(values);
    cmd.GetAbsRdbPredicates()->EqualTo(predicateColumn, predicateValue);
    int32_t ret = MediaLibraryPhotoOperations::Update(cmd);
    func(ret);
}

void TestPhotoUpdateByQuery(const string &predicateColumn, const string &predicateValue,
    const unordered_map<string, string> &checkColumns, int32_t result)
{
    for (auto &iter : checkColumns) {
        int32_t errCode = TestQueryAsset(predicateColumn, predicateValue, iter.first, iter.second,
            MediaLibraryApi::API_OLD);
        EXPECT_EQ(errCode, result);
    }
}

void TestPhotoUpdateParamsVerifyFunctionFailed(const string &predicateColumn, const string &predicateValue,
    const unordered_map<string, string> &updateColumns)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE,
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

void TestPhotoOpenParamsApi10(int32_t fileId, const string &mode, ExceptIntFunction func)
{
    string uriString = MediaFileUtils::GetMediaTypeUriV10(MediaType::MEDIA_TYPE_IMAGE);
    uriString += "/" + to_string(fileId);
    Uri uri(uriString);
    MediaLibraryCommand cmd(uri);
    int32_t fd = MediaLibraryPhotoOperations::Open(cmd, mode);
    func(fd);
    if (fd > 0) {
        close(fd);
        MediaLibraryInotify::GetInstance()->RemoveByFileUri(cmd.GetUriStringWithoutSegment(),
            MediaLibraryApi::API_10);
    }
}

void TestPhotoOpenEditParamsApi10(int32_t fileId, const string &addKey,
    const std::string &mode, ExceptIntFunction func)
{
    string uriString = MediaFileUtils::GetMediaTypeUriV10(MediaType::MEDIA_TYPE_IMAGE);
    uriString += "/" + to_string(fileId) + "?" + addKey;
    Uri uri(uriString);
    MediaLibraryCommand cmd(uri);
    int32_t fd = MediaLibraryPhotoOperations::Open(cmd, mode);
    func(fd);
    if (fd > 0) {
        close(fd);
        MediaLibraryInotify::GetInstance()->RemoveByFileUri(cmd.GetUriStringWithoutSegment(),
            MediaLibraryApi::API_10);
    }
}

void TestPhotoCloseParamsApi10(string uri, ExceptIntFunction func)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CLOSE);
    ValuesBucket values;
    values.PutString(MEDIA_DATA_DB_URI, uri);
    cmd.SetValueBucket(values);
    int32_t ret = MediaLibraryPhotoOperations::Close(cmd);
    func(ret);
}

int64_t GetPhotoPendingStatus(int32_t fileId)
{
    MediaLibraryCommand queryCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY,
        MediaLibraryApi::API_10);
    queryCmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    vector<string> columns = { PhotoColumn::MEDIA_TIME_PENDING };
    auto resultSet = g_rdbStore->Query(queryCmd, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get AssetUniqueNumberTable count");
        return E_HAS_DB_ERROR;
    }
    return GetInt64Val(PhotoColumn::MEDIA_TIME_PENDING, resultSet);
}

int32_t SetPhotoPendingStatus(int32_t pendingStatus, int32_t fileId)
{
    MediaLibraryCommand setPendingCloseCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE_PENDING,
        MediaLibraryApi::API_10);
    setPendingCloseCmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    ValuesBucket setPendingCloseValues;
    setPendingCloseValues.Put(PhotoColumn::MEDIA_TIME_PENDING, pendingStatus);
    setPendingCloseCmd.SetValueBucket(setPendingCloseValues);
    return MediaLibraryPhotoOperations::Update(setPendingCloseCmd);
}

int64_t GetPhotoLastVisitTime(int32_t fileId)
{
    MediaLibraryCommand queryCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY,
        MediaLibraryApi::API_10);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    queryCmd.SetDataSharePred(predicates);
    vector<string> columns = { PhotoColumn::PHOTO_LAST_VISIT_TIME };
    auto resultSet = g_rdbStore->Query(queryCmd, columns);
    if (resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        int64_t lastVisitTime = GetInt64Val(PhotoColumn::PHOTO_LAST_VISIT_TIME, resultSet);
        return lastVisitTime;
    } else {
        MEDIA_ERR_LOG("Test getPhotoLastVisitTime tdd Query failed");
        return 0L;
    }
}

static int32_t QueryPhotoIdByDisplayName(const string& displayName)
{
    MediaLibraryCommand queryCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY,
        MediaLibraryApi::API_10);
    DataSharePredicates predicates;
    predicates.EqualTo(MediaColumn::MEDIA_NAME, displayName);
    queryCmd.SetDataSharePred(predicates);
    vector<string> columns = { MediaColumn::MEDIA_ID };
    auto resultSet = g_rdbStore->Query(queryCmd, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Read moving photo query photo id by display name failed");
        return -1;
    }
    int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
    return fileId;
}

void MediaLibraryPhotoOperationsTest::SetUpTestCase()
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStoreRaw();
    if (g_rdbStore == nullptr || g_rdbStore->GetRaw() == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    SetTables();
}

void MediaLibraryPhotoOperationsTest::TearDownTestCase()
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    system("rm -rf /storage/cloud/files/*");
    ClearAndRestart();
    g_rdbStore = nullptr;
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    this_thread::sleep_for(chrono::seconds(1));
    MEDIA_INFO_LOG("Clean is finish");
}

void MediaLibraryPhotoOperationsTest::SetUp()
{
    if (g_rdbStore == nullptr || g_rdbStore->GetRaw() == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    ClearAndRestart();
}

void MediaLibraryPhotoOperationsTest::TearDown()
{}

const string CHAR256_ENGLISH =
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
const string CHAR256_CHINESE =
    "中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中"
    "中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中";

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_create_api10_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_create_api10_test_001");
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE,
        MediaLibraryApi::API_10);
    string name = "photo.jpg";
    ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_NAME, name);
    values.PutInt(MediaColumn::MEDIA_TYPE, MediaType::MEDIA_TYPE_IMAGE);
    cmd.SetValueBucket(values);
    int32_t ret = MediaLibraryPhotoOperations::Create(cmd);
    EXPECT_GE(ret, 0);
    unordered_map<string, string> verifyMap = {
        { PhotoColumn::MEDIA_TITLE, "photo" },
        { PhotoColumn::MEDIA_TYPE, to_string(MediaType::MEDIA_TYPE_IMAGE) },
        { PhotoColumn::MEDIA_TIME_PENDING, to_string(UNCREATE_FILE_TIMEPENDING)}
    };
    bool res = QueryAndVerifyPhotoAsset(PhotoColumn::MEDIA_NAME, name, verifyMap);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("end tdd photo_oprn_create_api10_test_001");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_create_api10_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_create_api10_test_002");
    string defaultRelativePath = "Pictures/1/";
    TestPhotoCreateParamsApi10("", MediaType::MEDIA_TYPE_IMAGE, E_INVALID_DISPLAY_NAME);
    TestPhotoCreateParamsApi10("photo\"\".jpg", MediaType::MEDIA_TYPE_IMAGE, E_INVALID_DISPLAY_NAME);
    TestPhotoCreateParamsApi10(".photo.jpg", MediaType::MEDIA_TYPE_IMAGE, E_INVALID_DISPLAY_NAME);
    string englishLongString = CHAR256_ENGLISH + ".jpg";
    TestPhotoCreateParamsApi10(englishLongString, MediaType::MEDIA_TYPE_IMAGE,
        E_INVALID_DISPLAY_NAME);
    string chineseLongString = CHAR256_CHINESE + ".jpg";
    TestPhotoCreateParamsApi10(chineseLongString, MediaType::MEDIA_TYPE_IMAGE,
        E_INVALID_DISPLAY_NAME);

    TestPhotoCreateParamsApi10("photo", MediaType::MEDIA_TYPE_IMAGE, E_INVALID_DISPLAY_NAME);
    TestPhotoCreateParamsApi10("photo.", MediaType::MEDIA_TYPE_IMAGE, E_INVALID_DISPLAY_NAME);
    TestPhotoCreateParamsApi10("photo.abc", MediaType::MEDIA_TYPE_IMAGE,
        E_CHECK_MEDIATYPE_MATCH_EXTENSION_FAIL);
    TestPhotoCreateParamsApi10("photo.mp3", MediaType::MEDIA_TYPE_IMAGE,
        E_CHECK_MEDIATYPE_MATCH_EXTENSION_FAIL);
    MEDIA_INFO_LOG("end tdd photo_oprn_create_api10_test_002");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_create_api10_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_create_api10_test_003");
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE,
        MediaLibraryApi::API_10);
    string name = "photo.jpg";
    ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_NAME, name);
    values.PutInt(MediaColumn::MEDIA_TYPE, MediaType::MEDIA_TYPE_IMAGE);
    cmd.SetValueBucket(values);
    int32_t ret = MediaLibraryPhotoOperations::Create(cmd);
    EXPECT_GE(ret, 0);
    MediaLibraryPhotoOperations::Create(cmd);
    MediaLibraryPhotoOperations::Create(cmd);
#ifdef FILEEXT
    shared_ptr<MediaFileExtAbility> mediaFileExtAbility;
    MediaLibraryUnitTestUtils::Init();
    ArkJsRuntime runtime;
    mediaFileExtAbility = make_shared<MediaFileExtAbility>(runtime);
    const int64_t offset = 0;
    const int64_t maxCount = 100;
    FileAccessFwk::FileFilter filter;
    FileAccessFwk::FileInfo rootInfo;
    rootInfo.uri = COMMON_PREFIX + ROOT_URI + MEDIALIBRARY_TYPE_IMAGE_URI;
    rootInfo.mimeType = DEFAULT_IMAGE_MIME_TYPE_PREFIX;
    vector<FileAccessFwk::FileInfo> rootFileList;
    ret = mediaFileExtAbility->ListFile(rootInfo, offset, maxCount, filter, rootFileList);
    EXPECT_EQ(ret, E_SUCCESS);
    EXPECT_EQ(rootFileList.size(), 3);
    DisplayFileList(rootFileList);
#endif
    MEDIA_INFO_LOG("end tdd photo_oprn_create_api10_test_003");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_create_api10_test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_create_api10_test_004");
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE,
        MediaLibraryApi::API_10);
    string name = "photo.mp4";
    ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_NAME, name);
    values.PutInt(MediaColumn::MEDIA_TYPE, MediaType::MEDIA_TYPE_VIDEO);
    cmd.SetValueBucket(values);
    int32_t ret = MediaLibraryPhotoOperations::Create(cmd);
    EXPECT_GE(ret, 0);
    MediaLibraryPhotoOperations::Create(cmd);
    MediaLibraryPhotoOperations::Create(cmd);
#ifdef FILEEXT
    shared_ptr<MediaFileExtAbility> mediaFileExtAbility;
    MediaLibraryUnitTestUtils::Init();
    ArkJsRuntime runtime;
    mediaFileExtAbility = make_shared<MediaFileExtAbility>(runtime);
    const int64_t offset = 0;
    const int64_t maxCount = 100;
    FileAccessFwk::FileFilter filter;
    FileAccessFwk::FileInfo rootInfo;
    rootInfo.uri = COMMON_PREFIX + ROOT_URI + MEDIALIBRARY_TYPE_VIDEO_URI;
    rootInfo.mimeType = DEFAULT_VIDEO_MIME_TYPE_PREFIX;
    vector<FileAccessFwk::FileInfo> rootFileList;
    ret = mediaFileExtAbility->ListFile(rootInfo, offset, maxCount, filter, rootFileList);
    EXPECT_EQ(ret, E_SUCCESS);
    EXPECT_EQ(rootFileList.size(), 3);
    DisplayFileList(rootFileList);
#endif
    MEDIA_INFO_LOG("end tdd photo_oprn_create_api10_test_004");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_create_api10_test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_create_api10_test_005");
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE,
        MediaLibraryApi::API_10);
    string extention = "jpg";
    ValuesBucket values;
    values.PutString(ASSET_EXTENTION, extention);
    values.PutInt(MediaColumn::MEDIA_TYPE, MediaType::MEDIA_TYPE_IMAGE);
    values.PutInt(PERMISSION_TABLE_TYPE, static_cast<int32_t>(TableType::TYPE_PHOTOS));
    cmd.SetValueBucket(values);
    int32_t ret = MediaLibraryPhotoOperations::Create(cmd);
    EXPECT_GE(ret, 0);
    unordered_map<string, string> verifyMap = {
        { PhotoColumn::MEDIA_TYPE, to_string(MediaType::MEDIA_TYPE_IMAGE) },
        { PhotoColumn::MEDIA_TIME_PENDING, to_string(UNOPEN_FILE_COMPONENT_TIMEPENDING) },
    };
    bool res = QueryAndVerifyPhotoAsset(PhotoColumn::MEDIA_ID, to_string(ret), verifyMap);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("end tdd photo_oprn_create_api10_test_005");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_create_api10_test_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_create_api10_test_006");
    TestPhotoCreateWithExtApi10("", MediaType::MEDIA_TYPE_IMAGE, E_CHECK_MEDIATYPE_MATCH_EXTENSION_FAIL);
    TestPhotoCreateWithExtApi10(".", MediaType::MEDIA_TYPE_IMAGE, E_CHECK_MEDIATYPE_MATCH_EXTENSION_FAIL);
    TestPhotoCreateWithExtApi10(".jpg", MediaType::MEDIA_TYPE_IMAGE, E_CHECK_MEDIATYPE_MATCH_EXTENSION_FAIL);
    TestPhotoCreateWithExtApi10("mp3", MediaType::MEDIA_TYPE_IMAGE, E_CHECK_MEDIATYPE_MATCH_EXTENSION_FAIL);
    TestPhotoCreateWithExtApi10("abc", MediaType::MEDIA_TYPE_IMAGE, E_CHECK_MEDIATYPE_MATCH_EXTENSION_FAIL);
    int32_t firstId = 1;
    TestPhotoCreateWithExtApi10("jpg", MediaType::MEDIA_TYPE_IMAGE, firstId++);
    TestPhotoCreateWithExtApi10("mov", MediaType::MEDIA_TYPE_VIDEO, firstId++);
    TestPhotoCreateWithExtApi10("mp4", MediaType::MEDIA_TYPE_VIDEO, firstId++);
    TestPhotoCreateWithExtApi10("mkv", MediaType::MEDIA_TYPE_VIDEO, firstId);
    MEDIA_INFO_LOG("end tdd photo_oprn_create_api10_test_006");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_create_api9_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_create_api9_test_001");
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE,
        MediaLibraryApi::API_OLD);
    string name = "photo.jpg";
    ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_NAME, name);
    values.PutInt(MediaColumn::MEDIA_TYPE, MediaType::MEDIA_TYPE_IMAGE);
    values.PutString(MediaColumn::MEDIA_RELATIVE_PATH, "Pictures/123/");
    cmd.SetValueBucket(values);
    int32_t ret = MediaLibraryPhotoOperations::Create(cmd);
    EXPECT_GE(ret, 0);
    unordered_map<string, string> verifyMap = {
        { PhotoColumn::MEDIA_TITLE, "photo" },
        { PhotoColumn::MEDIA_TYPE, to_string(MediaType::MEDIA_TYPE_IMAGE) },
        { PhotoColumn::MEDIA_RELATIVE_PATH, "Pictures/123/" },
        { PhotoColumn::MEDIA_VIRTURL_PATH, "Pictures/123/photo.jpg" }
    };
    bool res = QueryAndVerifyPhotoAsset(PhotoColumn::MEDIA_NAME, name, verifyMap);
    EXPECT_EQ(res, true);

    ret = MediaLibraryPhotoOperations::Create(cmd);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MEDIA_INFO_LOG("end tdd photo_oprn_create_api9_test_001");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_create_api9_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_create_api9_test_002");
    string defaultRelativePath = "Pictures/1/";
    TestPhotoCreateParamsApi9("", MediaType::MEDIA_TYPE_IMAGE, defaultRelativePath,
        E_INVALID_DISPLAY_NAME);
    TestPhotoCreateParamsApi9("photo\"\".jpg", MediaType::MEDIA_TYPE_IMAGE, defaultRelativePath,
        E_INVALID_DISPLAY_NAME);
    TestPhotoCreateParamsApi9(".photo.jpg", MediaType::MEDIA_TYPE_IMAGE, defaultRelativePath,
        E_INVALID_DISPLAY_NAME);
    string englishLongString = CHAR256_ENGLISH + ".jpg";
    TestPhotoCreateParamsApi9(englishLongString, MediaType::MEDIA_TYPE_IMAGE, defaultRelativePath,
        E_INVALID_DISPLAY_NAME);
    string chineseLongString = CHAR256_CHINESE + ".jpg";
    TestPhotoCreateParamsApi9(chineseLongString, MediaType::MEDIA_TYPE_IMAGE, defaultRelativePath,
        E_INVALID_DISPLAY_NAME);

    TestPhotoCreateParamsApi9("photo", MediaType::MEDIA_TYPE_IMAGE, defaultRelativePath,
        E_INVALID_DISPLAY_NAME);
    TestPhotoCreateParamsApi9("photo.", MediaType::MEDIA_TYPE_IMAGE, defaultRelativePath,
        E_INVALID_DISPLAY_NAME);
    TestPhotoCreateParamsApi9("photo.abc", MediaType::MEDIA_TYPE_IMAGE, defaultRelativePath,
        E_CHECK_MEDIATYPE_MATCH_EXTENSION_FAIL);
    TestPhotoCreateParamsApi9("photo.mp3", MediaType::MEDIA_TYPE_IMAGE, defaultRelativePath,
        E_CHECK_MEDIATYPE_MATCH_EXTENSION_FAIL);
    MEDIA_INFO_LOG("end tdd photo_oprn_create_api9_test_002");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_create_api9_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("start photo_oprn_create_api9_test_003");
    string defaultDisplayName = "photo.jpg";
    TestPhotoCreateParamsApi9(defaultDisplayName, MediaType::MEDIA_TYPE_IMAGE, "Pictures//",
        E_INVALID_PATH);
    TestPhotoCreateParamsApi9(defaultDisplayName, MediaType::MEDIA_TYPE_IMAGE, "Pictures/\"/",
        E_INVALID_PATH);
    string longEnglishRelativePath = "Pictures/" + CHAR256_ENGLISH + "/";
    string longChineseRelativePath = "Pictures/" + CHAR256_CHINESE + "/";
    TestPhotoCreateParamsApi9(defaultDisplayName, MediaType::MEDIA_TYPE_IMAGE, longEnglishRelativePath,
        E_INVALID_PATH);
    TestPhotoCreateParamsApi9(defaultDisplayName, MediaType::MEDIA_TYPE_IMAGE, longChineseRelativePath,
        E_INVALID_PATH);
    TestPhotoCreateParamsApi9(defaultDisplayName, MediaType::MEDIA_TYPE_IMAGE, "/",
        E_INVALID_PATH);
    TestPhotoCreateParamsApi9(defaultDisplayName, MediaType::MEDIA_TYPE_IMAGE, "Storage/abc",
        E_CHECK_MEDIATYPE_FAIL);
    TestPhotoCreateParamsApi9(defaultDisplayName, MediaType::MEDIA_TYPE_IMAGE, "Videos/abc",
        E_CHECK_MEDIATYPE_FAIL);
    TestPhotoCreateParamsApi9(defaultDisplayName, MediaType::MEDIA_TYPE_VIDEO, "Pictures/abc",
        E_CHECK_MEDIATYPE_FAIL);
    MEDIA_INFO_LOG("end tdd photo_oprn_create_api9_test_003");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_create_api9_test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_create_api9_test_004");
    MediaLibraryCommand cmd1(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE,
        MediaLibraryApi::API_OLD);
    string name = "photo.jpg";
    ValuesBucket values1;
    values1.PutString(MediaColumn::MEDIA_NAME, name);
    values1.PutInt(MediaColumn::MEDIA_TYPE, MediaType::MEDIA_TYPE_IMAGE);
    values1.PutString(MediaColumn::MEDIA_RELATIVE_PATH, CAMERA_PATH);
    cmd1.SetValueBucket(values1);
    int32_t ret = MediaLibraryPhotoOperations::Create(cmd1);
    EXPECT_GE(ret, 0);
    unordered_map<string, string> verifyMap1 = {
        { PhotoColumn::PHOTO_SUBTYPE, to_string(static_cast<int>(PhotoSubType::CAMERA)) }
    };
    bool res = QueryAndVerifyPhotoAsset(PhotoColumn::MEDIA_NAME, name, verifyMap1);
    EXPECT_EQ(res, true);

    MediaLibraryCommand cmd2(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE,
        MediaLibraryApi::API_OLD);
    string videoName = "shot.mp4";
    ValuesBucket values2;
    values2.PutString(MediaColumn::MEDIA_NAME, videoName);
    values2.PutInt(MediaColumn::MEDIA_TYPE, MediaType::MEDIA_TYPE_VIDEO);
    values2.PutString(MediaColumn::MEDIA_RELATIVE_PATH, SCREEN_RECORD_PATH);
    cmd2.SetValueBucket(values2);
    ret = MediaLibraryPhotoOperations::Create(cmd2);
    EXPECT_GE(ret, 0);
    unordered_map<string, string> verifyMap2 = {
        { PhotoColumn::PHOTO_SUBTYPE, to_string(static_cast<int>(PhotoSubType::SCREENSHOT)) }
    };
    res = QueryAndVerifyPhotoAsset(PhotoColumn::MEDIA_NAME, videoName, verifyMap2);
    EXPECT_EQ(res, true);

    MediaLibraryCommand cmd3(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE,
        MediaLibraryApi::API_OLD);
    string photoName = "shot.jpg";
    ValuesBucket values3;
    values3.PutString(MediaColumn::MEDIA_NAME, photoName);
    values3.PutInt(MediaColumn::MEDIA_TYPE, MediaType::MEDIA_TYPE_IMAGE);
    values3.PutString(MediaColumn::MEDIA_RELATIVE_PATH, SCREEN_SHOT_PATH);
    cmd3.SetValueBucket(values3);
    ret = MediaLibraryPhotoOperations::Create(cmd3);
    EXPECT_GE(ret, 0);
    unordered_map<string, string> verifyMap3 = {
        { PhotoColumn::PHOTO_SUBTYPE, to_string(static_cast<int>(PhotoSubType::SCREENSHOT)) }
    };
    res = QueryAndVerifyPhotoAsset(PhotoColumn::MEDIA_NAME, photoName, verifyMap3);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("end tdd photo_oprn_create_api9_test_004");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_delete_api10_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_delete_api10_test_001");

    // set photo
    int fileId = SetDefaultPhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    if (fileId < E_OK) {
        MEDIA_ERR_LOG("Set Default photo failed, ret = %{public}d", fileId);
        return;
    }
    string filePath = GetFilePath(fileId);
    if (filePath.empty()) {
        MEDIA_ERR_LOG("Get filePath failed");
        return;
    }

    EXPECT_EQ(MediaFileUtils::IsFileExists(filePath), true);
    int32_t count = GetPhotoAssetCountIndb(PhotoColumn::MEDIA_NAME, "photo.jpg");
    EXPECT_EQ(count, 1);

    // test delete
    static constexpr int LARGE_NUM = 1000;
    TestPhotoDeleteParamsApi10(OperationObject::ASSETMAP, fileId,
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_FILEID); });
    TestPhotoDeleteParamsApi10(OperationObject::FILESYSTEM_PHOTO, fileId + LARGE_NUM,
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_FILEID); });
    TestPhotoDeleteParamsApi10(OperationObject::FILESYSTEM_PHOTO, fileId,
        [] (int32_t result) { EXPECT_GT(result, 0); });

    // test delete result
    EXPECT_EQ(MediaFileUtils::IsFileExists(filePath), false);
    count = GetPhotoAssetCountIndb(PhotoColumn::MEDIA_NAME, "photo.jpg");
    EXPECT_EQ(count, 0);

    MEDIA_INFO_LOG("end tdd photo_oprn_delete_api10_test_001");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_query_api10_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_query_api10_test_001");

    int32_t fileId1 = SetDefaultPhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo1.jpg");
    if (fileId1 < E_OK) {
        MEDIA_ERR_LOG("Set Default photo failed, ret = %{public}d", fileId1);
        return;
    }

    // Query
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY,
        MediaLibraryApi::API_10);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId1));
    cmd.SetDataSharePred(predicates);
    vector<string> columns;
    auto resultSet = MediaLibraryPhotoOperations::Query(cmd, columns);
    if (resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        string name = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
        EXPECT_EQ(name, "photo1.jpg");
    } else {
        MEDIA_ERR_LOG("Test first tdd Query failed");
        return;
    }

    MEDIA_INFO_LOG("end tdd photo_oprn_query_api10_test_001");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_query_api10_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_query_api10_test_002");

    int32_t fileId1 = SetDefaultPhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo1.jpg");
    if (fileId1 < E_OK) {
        MEDIA_ERR_LOG("Set Default photo failed, ret = %{public}d", fileId1);
        return;
    }
    int32_t fileId2 = SetDefaultPhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo2.jpg");
    if (fileId2 < E_OK) {
        MEDIA_ERR_LOG("Set Default photo failed, ret = %{public}d", fileId2);
        return;
    }

    // Query
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY,
        MediaLibraryApi::API_10);
    DataSharePredicates predicates;
    predicates.BeginWrap()->EqualTo(MediaColumn::MEDIA_ID, to_string(fileId1))->
        Or()->EqualTo(MediaColumn::MEDIA_ID, to_string(fileId2))->EndWrap()->
        OrderByAsc(MediaColumn::MEDIA_ID);
    cmd.SetDataSharePred(predicates);
    vector<string> columns;
    auto resultSet = MediaLibraryPhotoOperations::Query(cmd, columns);
    if (resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        string name = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
        EXPECT_EQ(name, "photo1.jpg");
    } else {
        MEDIA_ERR_LOG("Test first tdd Query failed");
        return;
    }

    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        string name = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
        EXPECT_EQ(name, "photo2.jpg");
    } else {
        MEDIA_ERR_LOG("Test second tdd Query failed");
        return;
    }

    MEDIA_INFO_LOG("end tdd photo_oprn_query_api10_test_002");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_query_api10_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_query_api10_test_003");

    int32_t fileId1 = SetDefaultPhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo1.jpg");
    if (fileId1 < E_OK) {
        MEDIA_ERR_LOG("Set Default photo failed, ret = %{public}d", fileId1);
        return;
    }

    // Query
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY,
        MediaLibraryApi::API_10);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId1));
    cmd.SetDataSharePred(predicates);
    vector<string> columns;
    columns.push_back(MediaColumn::MEDIA_NAME);
    columns.push_back(MediaColumn::MEDIA_DATE_ADDED);
    auto resultSet = MediaLibraryPhotoOperations::Query(cmd, columns);
    if (resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        string name = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
        EXPECT_EQ(name, "photo1.jpg");
        int64_t dateAdded = GetInt64Val(MediaColumn::MEDIA_DATE_ADDED, resultSet);
        EXPECT_GE(dateAdded, 0L);
        int64_t dateModified = GetInt64Val(MediaColumn::MEDIA_DATE_MODIFIED, resultSet);
        EXPECT_EQ(dateModified, 0L);
    } else {
        MEDIA_ERR_LOG("Test first tdd Query failed");
        return;
    }
    MEDIA_INFO_LOG("end tdd photo_oprn_query_api10_test_003");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_query_api10_test_004, TestSize.Level0)
{
    // Hidden time test 1
    MEDIA_INFO_LOG("start tdd photo_oprn_query_api10_test_004");

    int32_t fileId1 = SetDefaultPhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "hohoho.jpg");
    ASSERT_FALSE(fileId1 < E_OK);

    // Query
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY,
        MediaLibraryApi::API_10);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId1));
    cmd.SetDataSharePred(predicates);
    vector<string> columns;
    columns.push_back(MediaColumn::MEDIA_NAME);
    columns.push_back(PhotoColumn::PHOTO_HIDDEN_TIME);
    auto resultSet = MediaLibraryPhotoOperations::Query(cmd, columns);
    ASSERT_TRUE(resultSet != nullptr);
    ASSERT_TRUE(resultSet->GoToFirstRow() == NativeRdb::E_OK);
    string name = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
    EXPECT_EQ(name, "hohoho.jpg");
    int64_t hidden_time = GetInt64Val(PhotoColumn::PHOTO_HIDDEN_TIME, resultSet);
    EXPECT_EQ(hidden_time, 0L);

    // Update
    MediaLibraryCommand cmd_u(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE,
        MediaLibraryApi::API_10);
    ValuesBucket values;
    SetValuesBucketInUpdate(PhotoColumn::MEDIA_NAME, "phophopho.jpg", values);
    cmd_u.SetValueBucket(values);
    cmd_u.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId1));
    MediaLibraryPhotoOperations::Update(cmd_u);

    // Query again
    resultSet = MediaLibraryPhotoOperations::Query(cmd, columns);
    ASSERT_TRUE(resultSet != nullptr);
    ASSERT_TRUE(resultSet->GoToFirstRow() == NativeRdb::E_OK);
    name = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
    EXPECT_EQ(name, "phophopho.jpg");
    hidden_time = GetInt64Val(PhotoColumn::PHOTO_HIDDEN_TIME, resultSet);
    EXPECT_EQ(hidden_time, 0L);

    MEDIA_INFO_LOG("end tdd photo_oprn_query_api10_test_004");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_query_api10_test_005, TestSize.Level0)
{
    // Last visit time test
    MEDIA_INFO_LOG("start tdd photo_oprn_query_api10_test_005");


    int32_t fileId = SetDefaultPhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photosy.jpg");
    EXPECT_GE(fileId, E_OK);
    int64_t lastVisitTime = GetPhotoLastVisitTime(fileId);
    EXPECT_GT(lastVisitTime, 0L);

    MediaFileUri fileUri(MediaType::MEDIA_TYPE_IMAGE, to_string(fileId), "", MEDIA_API_VERSION_V10);
    Uri uri(fileUri.ToString());
    MediaLibraryCommand openCmd(uri, Media::OperationType::OPEN);

    // Open file
    openCmd.SetOprnObject(OperationObject::FILESYSTEM_PHOTO);
    int32_t fd = MediaLibraryDataManager::GetInstance()->OpenFile(openCmd, "rw");
    EXPECT_GE(fd, 0);
    int64_t openTime = GetPhotoLastVisitTime(fileId);
    EXPECT_GT(openTime, lastVisitTime);


    // Open thumbnail
    openCmd.SetOprnObject(OperationObject::THUMBNAIL);
    MediaLibraryDataManager::GetInstance()->OpenFile(openCmd, "rw");
    int64_t openThumbnailTime = GetPhotoLastVisitTime(fileId);
    EXPECT_EQ(openTime, openThumbnailTime);

    // Update
    MediaLibraryCommand cmd_u(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE,
        MediaLibraryApi::API_10);
    ValuesBucket values;
    SetValuesBucketInUpdate(PhotoColumn::MEDIA_NAME, "photosy1.jpg", values);
    cmd_u.SetValueBucket(values);
    cmd_u.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    MediaLibraryPhotoOperations::Update(cmd_u);

    // Query again
    int64_t upLastVisitTime = GetPhotoLastVisitTime(fileId);
    EXPECT_GT(upLastVisitTime, openThumbnailTime);

    TestPhotoDeleteParamsApi10(OperationObject::FILESYSTEM_PHOTO, fileId,
        [] (int32_t result) { EXPECT_GT(result, 0); });
    MEDIA_INFO_LOG("end tdd photo_oprn_query_api10_test_005");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_query_api10_test_006, TestSize.Level0)
{
    // Hidden time test 2
    MEDIA_INFO_LOG("start tdd photo_oprn_query_api10_test_006");
    int32_t fileId1 = SetDefaultPhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "hoho.jpg");
    ASSERT_FALSE(fileId1 < E_OK);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId1));

    MediaLibraryCommand cmd_hide(OperationObject::FILESYSTEM_PHOTO, OperationType::HIDE,
        MediaLibraryApi::API_10);
    ValuesBucket values;
    SetValuesBucketInUpdate(MediaColumn::MEDIA_HIDDEN, "1", values);
    cmd_hide.SetValueBucket(values);
    cmd_hide.SetDataSharePred(predicates);
    MediaLibraryPhotoOperations::Update(cmd_hide);

    MediaLibraryCommand cmd_q(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY,
        MediaLibraryApi::API_10);
    predicates = DataSharePredicates {};
    predicates.EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId1));
    cmd_q.SetDataSharePred(predicates);
    vector<string> columns;
    columns.push_back(MediaColumn::MEDIA_NAME);
    columns.push_back(PhotoColumn::PHOTO_HIDDEN_TIME);
    auto resultSet = MediaLibraryPhotoOperations::Query(cmd_q, columns);
    ASSERT_TRUE(resultSet != nullptr);
    ASSERT_TRUE(resultSet->GoToFirstRow() == NativeRdb::E_OK);
    string name = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
    EXPECT_EQ(name, "hoho.jpg");
    int64_t hidden_time = GetInt64Val(PhotoColumn::PHOTO_HIDDEN_TIME, resultSet);
    EXPECT_EQ(hidden_time > 0, true);

    MediaLibraryCommand cmd_unhide(OperationObject::FILESYSTEM_PHOTO, OperationType::HIDE,
        MediaLibraryApi::API_10);
    values = ValuesBucket {};
    SetValuesBucketInUpdate(MediaColumn::MEDIA_HIDDEN, "0", values);
    cmd_unhide.SetValueBucket(values);
    predicates = DataSharePredicates {};
    predicates.EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId1));
    cmd_unhide.SetDataSharePred(predicates);
    MediaLibraryPhotoOperations::Update(cmd_unhide);

    resultSet = MediaLibraryPhotoOperations::Query(cmd_q, columns);
    ASSERT_TRUE(resultSet != nullptr);
    ASSERT_TRUE(resultSet->GoToFirstRow() == NativeRdb::E_OK);
    name = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
    EXPECT_EQ(name, "hoho.jpg");
    hidden_time = GetInt64Val(PhotoColumn::PHOTO_HIDDEN_TIME, resultSet);
    EXPECT_EQ(hidden_time, 0L);

    MEDIA_INFO_LOG("end tdd photo_oprn_query_api10_test_006");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_query_api10_test_007, TestSize.Level0)
{
    // location album test
    MEDIA_INFO_LOG("start tdd photo_oprn_query_api10_test_007");

    int32_t fileId = SetDefaultPhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    EXPECT_GE(fileId, E_OK);

    // Update
    MediaLibraryCommand cmd_u(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE,
        MediaLibraryApi::API_10);
    ValuesBucket values;
    SetValuesBucketInUpdate(PhotoColumn::PHOTO_LATITUDE, "29.589475631666667", values);
    SetValuesBucketInUpdate(PhotoColumn::PHOTO_LONGITUDE, "106.31181335444444", values);
    cmd_u.SetValueBucket(values);
    cmd_u.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    MediaLibraryPhotoOperations::Update(cmd_u);

    // Query
    MediaLibraryCommand queryCmd(OperationObject::GEO_PHOTO, OperationType::QUERY,
        MediaLibraryApi::API_10);
    DataSharePredicates predicates;

    predicates.GreaterThanOrEqualTo(PhotoColumn::PHOTO_LATITUDE, "-90.00000030559437");
    predicates.LessThan(PhotoColumn::PHOTO_LONGITUDE, "90.00000030559437");
    predicates.GreaterThanOrEqualTo(PhotoColumn::PHOTO_LATITUDE, "-180.00000061118874");
    predicates.LessThan(PhotoColumn::PHOTO_LONGITUDE, "00000061118874");
    string latitudeIndex = "round((latitude - -90.00000030559437) / 22.5 - 0.5)";
    string longitudeIndex  = "round((longitude - -180.00000061118874) / 22.5 - 0.5)";
    string group = latitudeIndex + "," + longitudeIndex;
    predicates.GroupBy({ group });
    queryCmd.SetDataSharePred(predicates);
    vector<string> columns = { MediaColumn::MEDIA_NAME };
    auto resultSet = g_rdbStore->Query(queryCmd, columns);
    ASSERT_TRUE(resultSet != nullptr);
    ASSERT_TRUE(resultSet->GoToFirstRow() == NativeRdb::E_OK);
    string name = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
    EXPECT_EQ(name, "photo.jpg");

    TestPhotoDeleteParamsApi10(OperationObject::FILESYSTEM_PHOTO, fileId,
        [] (int32_t result) { EXPECT_GT(result, 0); });

    MEDIA_INFO_LOG("end tdd photo_oprn_query_api10_test_007");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_update_api10_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_update_api10_test_001");
    int32_t fileId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    if (fileId < 0) {
        MEDIA_ERR_LOG("CreatePhoto In APi10 failed, ret=%{public}d", fileId);
        return;
    }

    TestPhotoUpdateParamsApi10(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_NAME, "pic.jpg" } },
        [] (int32_t result) { EXPECT_GE(result, E_OK); });
    TestPhotoUpdateByQuery(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_NAME, "pic.jpg" } }, E_OK);
    MEDIA_INFO_LOG("end tdd photo_oprn_update_api10_test_001");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_update_api10_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_update_api10_test_002");
    int32_t fileId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    if (fileId < 0) {
        MEDIA_ERR_LOG("CreatePhoto In APi10 failed, ret=%{public}d", fileId);
        return;
    }

    TestPhotoUpdateParamsApi10(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_NAME, "" } },
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_DISPLAY_NAME); });
    TestPhotoUpdateParamsApi10(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_NAME, "photo\"\".jpg" } },
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_DISPLAY_NAME); });
    TestPhotoUpdateParamsApi10(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_NAME, ".photo.jpg" } },
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_DISPLAY_NAME); });

    string englishLongString = CHAR256_ENGLISH + ".jpg";
    string chineseLongString = CHAR256_CHINESE + ".jpg";
    TestPhotoUpdateParamsApi10(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_NAME, englishLongString } },
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_DISPLAY_NAME); });
    TestPhotoUpdateParamsApi10(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_NAME, chineseLongString } },
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_DISPLAY_NAME); });

    TestPhotoUpdateParamsApi10(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_NAME, "photo" } },
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_DISPLAY_NAME); });
    TestPhotoUpdateParamsApi10(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_NAME, "photo." } },
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_DISPLAY_NAME); });
    TestPhotoUpdateParamsApi10(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_NAME, "photo.abc" } },
        [] (int32_t result) { EXPECT_EQ(result, E_CHECK_MEDIATYPE_MATCH_EXTENSION_FAIL); });
    TestPhotoUpdateParamsApi10(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_NAME, "photo.mp4" } },
        [] (int32_t result) { EXPECT_EQ(result, E_CHECK_MEDIATYPE_MATCH_EXTENSION_FAIL); });

    MEDIA_INFO_LOG("end tdd photo_oprn_update_api10_test_002");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_update_api10_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_update_api10_test_003");
    int32_t fileId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    if (fileId < 0) {
        MEDIA_ERR_LOG("CreatePhoto In APi10 failed, ret=%{public}d", fileId);
        return;
    }

    unordered_map<string, string> updateMap1 = {
        { PhotoColumn::MEDIA_TITLE, "photo1" },
        { PhotoColumn::MEDIA_NAME, "photo2.jpg" }
    };
    TestPhotoUpdateParamsApi10(PhotoColumn::MEDIA_ID, to_string(fileId), updateMap1,
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_DISPLAY_NAME); });

    unordered_map<string, string> updateMap2 = {
        { PhotoColumn::MEDIA_TITLE, "photo2" },
        { PhotoColumn::MEDIA_NAME, "photo2.jpg" }
    };
    TestPhotoUpdateParamsApi10(PhotoColumn::MEDIA_ID, to_string(fileId), updateMap2,
    [] (int32_t result) { EXPECT_GE(result, E_OK); });

    MEDIA_INFO_LOG("end tdd photo_oprn_update_api10_test_003");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_update_api10_test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_update_api10_test_004");
    int32_t fileId1 = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    if (fileId1 < 0) {
        MEDIA_ERR_LOG("CreatePhoto In APi10 failed, ret=%{public}d", fileId1);
        return;
    }

    int32_t fileId2 = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "pic.jpg");
    if (fileId2 < 0) {
        MEDIA_ERR_LOG("CreatePhoto In APi10 failed, ret=%{public}d", fileId2);
        return;
    }

    TestPhotoUpdateParamsApi10(PhotoColumn::MEDIA_ID, to_string(fileId1),
        { { PhotoColumn::MEDIA_TITLE, "photo1" } },
        [] (int32_t result) { EXPECT_GE(result, E_OK); });
    TestPhotoUpdateByQuery(PhotoColumn::MEDIA_ID, to_string(fileId1),
        { { PhotoColumn::MEDIA_NAME, "photo1.jpg" } }, E_OK);

    TestPhotoUpdateParamsApi10(PhotoColumn::MEDIA_ID, to_string(fileId2),
        { { PhotoColumn::MEDIA_NAME, "photo2.jpg" } },
        [] (int32_t result) { EXPECT_GE(result, E_OK); });
    TestPhotoUpdateByQuery(PhotoColumn::MEDIA_ID, to_string(fileId2),
        { { PhotoColumn::MEDIA_TITLE, "photo2" } }, E_OK);

    MEDIA_INFO_LOG("end tdd photo_oprn_update_api10_test_004");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_update_api10_test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_update_api10_test_005");
    int32_t fileId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    if (fileId < 0) {
        MEDIA_ERR_LOG("CreatePhoto In APi10 failed, ret=%{public}d", fileId);
        return;
    }

    TestPhotoUpdateParamsVerifyFunctionFailed(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_ID, "1"} });
    TestPhotoUpdateParamsVerifyFunctionFailed(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_FILE_PATH, ""} });
    TestPhotoUpdateParamsVerifyFunctionFailed(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { { PhotoColumn::MEDIA_FILE_PATH, ""}, { PhotoColumn::MEDIA_TITLE, "123" } } });
    TestPhotoUpdateParamsVerifyFunctionFailed(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_SIZE, "12345"} });
    TestPhotoUpdateParamsVerifyFunctionFailed(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_TITLE, ""} });
    TestPhotoUpdateParamsVerifyFunctionFailed(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_NAME, ""} });
    TestPhotoUpdateParamsVerifyFunctionFailed(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_DATE_MODIFIED, "1000000"} });
    TestPhotoUpdateParamsVerifyFunctionFailed(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_DATE_ADDED, "1000000"} });
    TestPhotoUpdateParamsVerifyFunctionFailed(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_DATE_TAKEN, "1000000"} });
    TestPhotoUpdateParamsVerifyFunctionFailed(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_DURATION, "1000000"} });
    TestPhotoUpdateParamsVerifyFunctionFailed(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_TIME_PENDING, "1000000"}, { PhotoColumn::MEDIA_TITLE, "123" } });
    TestPhotoUpdateParamsVerifyFunctionFailed(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_DATE_TRASHED, "1000000"}, { PhotoColumn::MEDIA_TITLE, "123" } });
    TestPhotoUpdateParamsVerifyFunctionFailed(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_DATE_DELETED, "1000000"}, { PhotoColumn::MEDIA_TITLE, "123" } });
    TestPhotoUpdateParamsVerifyFunctionFailed(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_HIDDEN, "1"}, { PhotoColumn::MEDIA_TITLE, "123" } });
    TestPhotoUpdateParamsVerifyFunctionFailed(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_IS_FAV, "1"}, { PhotoColumn::MEDIA_TITLE, "123" } });

    MEDIA_INFO_LOG("end tdd photo_oprn_update_api10_test_005");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_update_api10_test_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_update_api10_test_006");
    int32_t fileId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    if (fileId < 0) {
        MEDIA_ERR_LOG("CreatePhoto In APi10 failed, ret=%{public}d", fileId);
        return;
    }

    TestPhotoUpdateParamsVerifyFunctionFailed(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::PHOTO_ORIENTATION, "1"} });
    TestPhotoUpdateParamsVerifyFunctionFailed(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::PHOTO_LATITUDE, "1"} });
    TestPhotoUpdateParamsVerifyFunctionFailed(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::PHOTO_LONGITUDE, "1"} });
    TestPhotoUpdateParamsVerifyFunctionFailed(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::PHOTO_HEIGHT, "12345"} });
    TestPhotoUpdateParamsVerifyFunctionFailed(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::PHOTO_WIDTH, "12345"} });
    MEDIA_INFO_LOG("end tdd photo_oprn_update_api10_test_006");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_update_api9_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_update_api9_test_001");
    string displayName = "photo.jpg";
    string relativePath = "Pictures/1/";
    int32_t fileId = SetDefaultPhotoApi9(MediaType::MEDIA_TYPE_IMAGE, displayName, relativePath);
    if (fileId < 0) {
        MEDIA_ERR_LOG("CreatePhoto In APi10 failed, ret=%{public}d", fileId);
        return;
    }

    unordered_map<string, string> updateMap = {
        { PhotoColumn::MEDIA_NAME, "photo1.jpg" },
        { PhotoColumn::MEDIA_RELATIVE_PATH, "Pictures/2" }
    };
    TestPhotoUpdateParamsApi9(PhotoColumn::MEDIA_ID, to_string(fileId), updateMap,
    [] (int32_t result) { EXPECT_GE(result, E_OK); });

    unordered_map<string, string> queryMap = {
        { PhotoColumn::MEDIA_NAME, "photo1.jpg" },
        { PhotoColumn::MEDIA_TITLE, "photo1" },
        { PhotoColumn::MEDIA_RELATIVE_PATH, "Pictures/2/"},
        { PhotoColumn::MEDIA_VIRTURL_PATH, "Pictures/2/photo1.jpg"}
    };
    TestPhotoUpdateByQuery(PhotoColumn::MEDIA_ID, to_string(fileId), queryMap, E_OK);
    MEDIA_INFO_LOG("end tdd photo_oprn_update_api9_test_001");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_update_api9_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_update_api9_test_002");
    string relativePath = "Pictures/1/";
    int32_t fileId = SetDefaultPhotoApi9(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg", relativePath);
    if (fileId < 0) {
        MEDIA_ERR_LOG("CreatePhoto In APi10 failed, ret=%{public}d", fileId);
        return;
    }

    TestPhotoUpdateParamsApi9(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_NAME, "" } },
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_DISPLAY_NAME); });
    TestPhotoUpdateParamsApi9(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_NAME, "photo\"\".jpg" } },
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_DISPLAY_NAME); });
    TestPhotoUpdateParamsApi9(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_NAME, ".photo.jpg" } },
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_DISPLAY_NAME); });

    string englishLongString = CHAR256_ENGLISH + ".jpg";
    string chineseLongString = CHAR256_CHINESE + ".jpg";
    TestPhotoUpdateParamsApi9(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_NAME, englishLongString } },
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_DISPLAY_NAME); });
    TestPhotoUpdateParamsApi9(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_NAME, chineseLongString } },
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_DISPLAY_NAME); });

    TestPhotoUpdateParamsApi9(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_NAME, "photo" } },
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_DISPLAY_NAME); });
    TestPhotoUpdateParamsApi9(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_NAME, "photo." } },
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_DISPLAY_NAME); });
    TestPhotoUpdateParamsApi9(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_NAME, "photo.abc" } },
        [] (int32_t result) { EXPECT_EQ(result, E_CHECK_MEDIATYPE_MATCH_EXTENSION_FAIL); });
    TestPhotoUpdateParamsApi9(PhotoColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_NAME, "photo.mp4" } },
        [] (int32_t result) { EXPECT_EQ(result, E_CHECK_MEDIATYPE_MATCH_EXTENSION_FAIL); });

    MEDIA_INFO_LOG("end tdd photo_oprn_update_api9_test_002");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_update_api9_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("start photo_oprn_update_api9_test_003");
    string relativePath = "Pictures/1/";
    int32_t fileId = SetDefaultPhotoApi9(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg", relativePath);
    if (fileId < 0) {
        MEDIA_ERR_LOG("CreatePhoto In APi10 failed, ret=%{public}d", fileId);
        return;
    }

    string defaultDisplayName = "photo.jpg";
    string longEnglishRelativePath = "Pictures/" + CHAR256_ENGLISH + "/";
    string longChineseRelativePath = "Pictures/" + CHAR256_CHINESE + "/";
    TestPhotoUpdateParamsApi9(MediaColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_RELATIVE_PATH, "Pictures//" } },
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_PATH); });
    TestPhotoUpdateParamsApi9(MediaColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_RELATIVE_PATH, "Pictures/\"/" } },
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_PATH); });
    TestPhotoUpdateParamsApi9(MediaColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_RELATIVE_PATH, longEnglishRelativePath } },
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_PATH); });
    TestPhotoUpdateParamsApi9(MediaColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_RELATIVE_PATH, longChineseRelativePath } },
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_PATH); });
    TestPhotoUpdateParamsApi9(MediaColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_RELATIVE_PATH, "/" } },
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_PATH); });
    TestPhotoUpdateParamsApi9(MediaColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_RELATIVE_PATH, "Storage/abc" } },
        [] (int32_t result) { EXPECT_EQ(result, E_CHECK_MEDIATYPE_FAIL); });
    TestPhotoUpdateParamsApi9(MediaColumn::MEDIA_ID, to_string(fileId),
        { { PhotoColumn::MEDIA_RELATIVE_PATH, "Videos/abc" } },
        [] (int32_t result) { EXPECT_EQ(result, E_CHECK_MEDIATYPE_FAIL); });

    MEDIA_INFO_LOG("end tdd photo_oprn_update_api9_test_003");
}


HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_update_api9_test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_update_api9_test_004");
    string relativePath = "Pictures/1/";
    int32_t fileId1 = SetDefaultPhotoApi9(MediaType::MEDIA_TYPE_IMAGE, "photo1.jpg", relativePath);
    if (fileId1 < 0) {
        MEDIA_ERR_LOG("CreatePhoto In APi10 failed, ret=%{public}d", fileId1);
        return;
    }
    int32_t fileId2 = SetDefaultPhotoApi9(MediaType::MEDIA_TYPE_IMAGE, "photo2.jpg", relativePath);
    if (fileId2 < 0) {
        MEDIA_ERR_LOG("CreatePhoto In APi10 failed, ret=%{public}d", fileId2);
        return;
    }

    TestPhotoUpdateParamsApi9(PhotoColumn::MEDIA_ID, to_string(fileId2),
        { { PhotoColumn::MEDIA_NAME, "photo1.jpg" } },
        [] (int32_t result) { EXPECT_EQ(result, E_HAS_DB_ERROR); });
    MEDIA_INFO_LOG("end tdd photo_oprn_update_api9_test_004");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_update_api9_test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_update_api9_test_005");
    string displayName = "photo.jpg";
    int32_t fileId1 = SetDefaultPhotoApi9(MediaType::MEDIA_TYPE_IMAGE, displayName, "Pictures/1/");
    if (fileId1 < 0) {
        MEDIA_ERR_LOG("CreatePhoto In APi10 failed, ret=%{public}d", fileId1);
        return;
    }
    int32_t fileId2 = SetDefaultPhotoApi9(MediaType::MEDIA_TYPE_IMAGE, displayName, "Pictures/2/");
    if (fileId2 < 0) {
        MEDIA_ERR_LOG("CreatePhoto In APi10 failed, ret=%{public}d", fileId2);
        return;
    }

    TestPhotoUpdateParamsApi9(PhotoColumn::MEDIA_ID, to_string(fileId2),
        { { PhotoColumn::MEDIA_RELATIVE_PATH, "Pictures/1" } },
        [] (int32_t result) { EXPECT_EQ(result, E_HAS_DB_ERROR); });
    TestPhotoUpdateParamsApi9(PhotoColumn::MEDIA_ID, to_string(fileId2),
        { { PhotoColumn::MEDIA_RELATIVE_PATH, "Pictures/1/" } },
        [] (int32_t result) { EXPECT_EQ(result, E_HAS_DB_ERROR); });
    TestPhotoUpdateParamsApi9(PhotoColumn::MEDIA_ID, to_string(fileId2),
        { { PhotoColumn::MEDIA_RELATIVE_PATH, "/Pictures/1" } },
        [] (int32_t result) { EXPECT_EQ(result, E_HAS_DB_ERROR); });
    TestPhotoUpdateParamsApi9(PhotoColumn::MEDIA_ID, to_string(fileId2),
        { { PhotoColumn::MEDIA_RELATIVE_PATH, "/Pictures/1/" } },
        [] (int32_t result) { EXPECT_EQ(result, E_HAS_DB_ERROR); });

    MEDIA_INFO_LOG("end tdd photo_oprn_update_api9_test_005");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_open_api10_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_open_api10_test_001");

    int fileId = SetDefaultPhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    if (fileId < 0) {
        MEDIA_ERR_LOG("Create photo failed error=%{public}d", fileId);
        return;
    }

    TestPhotoOpenParamsApi10(fileId, "",
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_MODE); });
    TestPhotoOpenParamsApi10(fileId, "m",
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_MODE); });
    TestPhotoOpenParamsApi10(fileId, "rw",
        [] (int32_t result) { EXPECT_GE(result, E_OK); });

    MEDIA_INFO_LOG("end tdd photo_oprn_open_api10_test_001");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_open_api10_test_002, TestSize.Level0)
{
    // test function MediaLibraryPhotoOperations::RequestEditData
    MEDIA_INFO_LOG("start tdd photo_oprn_open_api10_test_002");

    int fileId = SetDefaultPhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    if (fileId < 0) {
        MEDIA_ERR_LOG("Create photo failed error=%{public}d", fileId);
        return;
    }

    const static int LARGE_NUM = 1000;
    string requestEditDataStr = MEDIA_OPERN_KEYWORD + "=" + EDIT_DATA_REQUEST;
    TestPhotoOpenEditParamsApi10(fileId, requestEditDataStr, "r",
        [] (int32_t result) { EXPECT_GE(result, E_OK); });
    TestPhotoOpenEditParamsApi10(fileId + LARGE_NUM, requestEditDataStr, "r",
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_URI); });
    TestPhotoOpenEditParamsApi10(-1, requestEditDataStr, "r",
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_URI); });
    TestPhotoOpenEditParamsApi10(fileId + LARGE_NUM, requestEditDataStr, "r",
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_URI); });
    int32_t ret = SetPendingOnly(UNCLOSE_FILE_TIMEPENDING, fileId);
    EXPECT_EQ(ret, 0);
    TestPhotoOpenEditParamsApi10(fileId, requestEditDataStr, "r",
        [] (int32_t result) { EXPECT_EQ(result, E_IS_PENDING_ERROR); });

    MEDIA_INFO_LOG("end tdd photo_oprn_open_api10_test_002");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_open_api10_test_003, TestSize.Level0)
{
    // test function MediaLibraryPhotoOperations::RequestEditSource
    MEDIA_INFO_LOG("start tdd photo_oprn_open_api10_test_003");

    int fileId = SetDefaultPhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    if (fileId < 0) {
        MEDIA_ERR_LOG("Create photo failed error=%{public}d", fileId);
        return;
    }

    const static int LARGE_NUM = 1000;
    string requestEditDataStr = MEDIA_OPERN_KEYWORD + "=" + SOURCE_REQUEST;
    TestPhotoOpenEditParamsApi10(fileId + LARGE_NUM, requestEditDataStr, "r",
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_URI); });
    TestPhotoOpenEditParamsApi10(-1, requestEditDataStr, "r",
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_URI); });
    TestPhotoOpenEditParamsApi10(fileId + LARGE_NUM, requestEditDataStr, "r",
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_URI); });
    int32_t ret = SetPendingOnly(UNCLOSE_FILE_TIMEPENDING, fileId);
    EXPECT_EQ(ret, 0);
    TestPhotoOpenEditParamsApi10(fileId, requestEditDataStr, "r",
        [] (int32_t result) { EXPECT_EQ(result, E_IS_PENDING_ERROR); });

    MEDIA_INFO_LOG("end tdd photo_oprn_open_api10_test_003");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_open_api10_test_004, TestSize.Level0)
{
    // test function MediaLibraryPhotoOperations::CommitEditOpen
    MEDIA_INFO_LOG("start tdd photo_oprn_open_api10_test_004");

    int fileId = SetDefaultPhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    if (fileId < 0) {
        MEDIA_ERR_LOG("Create photo failed error=%{public}d", fileId);
        return;
    }

    const static int LARGE_NUM = 1000;
    string requestEditDataStr = MEDIA_OPERN_KEYWORD + "=" + COMMIT_REQUEST;
    TestPhotoOpenEditParamsApi10(fileId, requestEditDataStr, "r",
        [] (int32_t result) { EXPECT_GE(result, E_OK); });
    TestPhotoOpenEditParamsApi10(fileId + LARGE_NUM, requestEditDataStr, "r",
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_URI); });
    TestPhotoOpenEditParamsApi10(-1, requestEditDataStr, "r",
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_URI); });
    TestPhotoOpenEditParamsApi10(fileId + LARGE_NUM, requestEditDataStr, "r",
        [] (int32_t result) { EXPECT_EQ(result, E_INVALID_URI); });
    int32_t ret = SetPendingOnly(UNCLOSE_FILE_TIMEPENDING, fileId);
    EXPECT_EQ(ret, 0);
    TestPhotoOpenEditParamsApi10(fileId, requestEditDataStr, "r",
        [] (int32_t result) { EXPECT_EQ(result, E_IS_PENDING_ERROR); });

    MEDIA_INFO_LOG("end tdd photo_oprn_open_api10_test_004");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_close_api10_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_close_api10_test_001");

    int fileId = SetDefaultPhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    if (fileId < 0) {
        MEDIA_ERR_LOG("Create photo failed error=%{public}d", fileId);
        return;
    }

    auto fileAssetPtr = QueryPhotoAsset(PhotoColumn::MEDIA_ID, to_string(fileId));
    static constexpr int LARGE_NUM = 1000;
    fileAssetPtr->SetId(fileId + LARGE_NUM);
    string uri = MediaLibraryAssetOperations::CreateExtUriForV10Asset(*fileAssetPtr);
    TestPhotoCloseParamsApi10(uri, [] (int32_t result) { EXPECT_EQ(result, E_INVALID_URI); });

    fileAssetPtr->SetId(fileId);
    uri = MediaLibraryAssetOperations::CreateExtUriForV10Asset(*fileAssetPtr);
    TestPhotoCloseParamsApi10(uri, [] (int32_t result) { EXPECT_GE(result, E_OK); });

    MEDIA_INFO_LOG("end tdd photo_oprn_close_api10_test_001");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_pending_api10_test_001, TestSize.Level0)
{
    // common api10 create -> open -> write -> close
    MEDIA_INFO_LOG("start tdd photo_oprn_pending_api10_test_001");
    MediaLibraryCommand createCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE,
        MediaLibraryApi::API_10);
    string name = "photo.jpg";
    ValuesBucket createValues;
    createValues.PutString(MediaColumn::MEDIA_NAME, name);
    createValues.PutInt(MediaColumn::MEDIA_TYPE, MediaType::MEDIA_TYPE_IMAGE);
    createCmd.SetValueBucket(createValues);
    int32_t fileId = MediaLibraryPhotoOperations::Create(createCmd);
    EXPECT_GE(fileId, 0);
    int64_t pendingStatus = GetPhotoPendingStatus(fileId);
    EXPECT_EQ(pendingStatus, UNCREATE_FILE_TIMEPENDING);

    MediaFileUri fileUri(MediaType::MEDIA_TYPE_IMAGE, to_string(fileId), "", MEDIA_API_VERSION_V10);
    Uri uri(fileUri.ToString());
    MediaLibraryCommand openCmd(uri);
    int32_t fd = MediaLibraryPhotoOperations::Open(openCmd, "rw");
    EXPECT_GE(fd, 0);
    pendingStatus = GetPhotoPendingStatus(fileId);
    EXPECT_EQ(pendingStatus, UNCLOSE_FILE_TIMEPENDING);

    MediaLibraryCommand closeCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CLOSE);
    auto fileAssetPtr = QueryPhotoAsset(PhotoColumn::MEDIA_ID, to_string(fileId));
    string uriString = MediaLibraryAssetOperations::CreateExtUriForV10Asset(*fileAssetPtr);
    ValuesBucket closeValues;
    closeValues.PutString(MEDIA_DATA_DB_URI, uriString);
    closeCmd.SetValueBucket(closeValues);
    int32_t ret = MediaLibraryPhotoOperations::Close(closeCmd);
    EXPECT_EQ(ret, 0);
    pendingStatus = GetPhotoPendingStatus(fileId);
    EXPECT_EQ(pendingStatus, 0);

    MEDIA_INFO_LOG("end tdd photo_oprn_pending_api10_test_001");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_pending_api10_test_002, TestSize.Level0)
{
    // common api10 create -> setPending(true) -> open -> write -> close -> setPending(false)
    MEDIA_INFO_LOG("start tdd photo_oprn_pending_api10_test_002");
    MediaLibraryCommand createCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE,
        MediaLibraryApi::API_10);
    string name = "photo.jpg";
    ValuesBucket createValues;
    createValues.PutString(MediaColumn::MEDIA_NAME, name);
    createValues.PutInt(MediaColumn::MEDIA_TYPE, MediaType::MEDIA_TYPE_IMAGE);
    createCmd.SetValueBucket(createValues);
    int32_t fileId = MediaLibraryPhotoOperations::Create(createCmd);
    EXPECT_GE(fileId, 0);
    int64_t pendingStatus = GetPhotoPendingStatus(fileId);
    EXPECT_EQ(pendingStatus, UNCREATE_FILE_TIMEPENDING);

    int32_t ret = SetPhotoPendingStatus(1, fileId);
    EXPECT_EQ(ret, 0);
    pendingStatus = GetPhotoPendingStatus(fileId);
    EXPECT_GT(pendingStatus, 0);

    MediaFileUri fileUri(MediaType::MEDIA_TYPE_IMAGE, to_string(fileId), "", MEDIA_API_VERSION_V10);
    Uri uri(fileUri.ToString());
    MediaLibraryCommand openCmd(uri);
    int32_t fd = MediaLibraryPhotoOperations::Open(openCmd, "rw");
    EXPECT_GE(fd, 0);
    pendingStatus = GetPhotoPendingStatus(fileId);
    EXPECT_GT(pendingStatus, 0);

    char data = 'A';
    write(fd, &data, 1);

    MediaLibraryCommand closeCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CLOSE,
        MediaLibraryApi::API_10);
    auto fileAssetPtr = QueryPhotoAsset(PhotoColumn::MEDIA_ID, to_string(fileId));
    string uriString = MediaLibraryAssetOperations::CreateExtUriForV10Asset(*fileAssetPtr);
    ValuesBucket closeValues;
    closeValues.PutString(MEDIA_DATA_DB_URI, uriString);
    closeCmd.SetValueBucket(closeValues);
    ret = MediaLibraryPhotoOperations::Close(closeCmd);
    EXPECT_EQ(ret, 0);
    pendingStatus = GetPhotoPendingStatus(fileId);
    EXPECT_GT(pendingStatus, 0);

    ret = SetPhotoPendingStatus(0, fileId);
    EXPECT_EQ(ret, 0);
    pendingStatus = GetPhotoPendingStatus(fileId);
    EXPECT_EQ(pendingStatus, 0);
    MEDIA_INFO_LOG("end tdd photo_oprn_pending_api10_test_002");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_pending_api10_test_003, TestSize.Level0)
{
    // common api10 create -> open -> setPending(true) -> write -> setPending(false) -> close
    MEDIA_INFO_LOG("start tdd photo_oprn_pending_api10_test_003");
    MediaLibraryCommand createCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE,
        MediaLibraryApi::API_10);
    string name = "photo.jpg";
    ValuesBucket createValues;
    createValues.PutString(MediaColumn::MEDIA_NAME, name);
    createValues.PutInt(MediaColumn::MEDIA_TYPE, MediaType::MEDIA_TYPE_IMAGE);
    createCmd.SetValueBucket(createValues);
    int32_t fileId = MediaLibraryPhotoOperations::Create(createCmd);
    EXPECT_GE(fileId, 0);
    int64_t pendingStatus = GetPhotoPendingStatus(fileId);
    EXPECT_EQ(pendingStatus, UNCREATE_FILE_TIMEPENDING);

    MediaFileUri fileUri(MediaType::MEDIA_TYPE_IMAGE, to_string(fileId), "", MEDIA_API_VERSION_V10);
    Uri uri(fileUri.ToString());
    MediaLibraryCommand openCmd(uri);
    int32_t fd = MediaLibraryPhotoOperations::Open(openCmd, "rw");
    EXPECT_GE(fd, 0);
    pendingStatus = GetPhotoPendingStatus(fileId);
    EXPECT_EQ(pendingStatus, UNCLOSE_FILE_TIMEPENDING);

    int32_t ret = SetPhotoPendingStatus(1, fileId);
    EXPECT_EQ(ret, 0);
    pendingStatus = GetPhotoPendingStatus(fileId);
    EXPECT_GT(pendingStatus, 0);

    char data = 'A';
    write(fd, &data, 1);

    ret = SetPhotoPendingStatus(0, fileId);
    EXPECT_EQ(ret, 0);
    pendingStatus = GetPhotoPendingStatus(fileId);
    EXPECT_EQ(pendingStatus, 0);

    MediaLibraryCommand closeCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CLOSE);
    auto fileAssetPtr = QueryPhotoAsset(PhotoColumn::MEDIA_ID, to_string(fileId));
    string uriString = MediaLibraryAssetOperations::CreateExtUriForV10Asset(*fileAssetPtr);
    ValuesBucket closeValues;
    closeValues.PutString(MEDIA_DATA_DB_URI, uriString);
    closeCmd.SetValueBucket(closeValues);
    ret = MediaLibraryPhotoOperations::Close(closeCmd);
    EXPECT_EQ(ret, 0);
    pendingStatus = GetPhotoPendingStatus(fileId);
    EXPECT_EQ(pendingStatus, 0);

    MEDIA_INFO_LOG("end tdd photo_oprn_pending_api10_test_003");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_pending_api9_test_001, TestSize.Level0)
{
    // common api9 create -> open -> write -> close
    MEDIA_INFO_LOG("start tdd photo_oprn_pending_api9_test_001");
    MediaLibraryCommand createCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE,
        MediaLibraryApi::API_OLD);
    string name = "photo.jpg";
    ValuesBucket createValues;
    createValues.PutString(MediaColumn::MEDIA_NAME, name);
    createValues.PutInt(MediaColumn::MEDIA_TYPE, MediaType::MEDIA_TYPE_IMAGE);
    createValues.PutString(PhotoColumn::MEDIA_RELATIVE_PATH, "Pictures/1/");
    createCmd.SetValueBucket(createValues);
    int32_t fileId = MediaLibraryPhotoOperations::Create(createCmd);
    EXPECT_GE(fileId, 0);
    int64_t pendingStatus = GetPhotoPendingStatus(fileId);
    EXPECT_EQ(pendingStatus, 0);

    MediaFileUri fileUri(MediaType::MEDIA_TYPE_IMAGE, to_string(fileId), "", MEDIA_API_VERSION_V9);
    Uri uri(fileUri.ToString());
    MediaLibraryCommand openCmd(uri);
    int32_t fd = MediaLibraryPhotoOperations::Open(openCmd, "rw");
    EXPECT_GE(fd, 0);
    pendingStatus = GetPhotoPendingStatus(fileId);
    EXPECT_EQ(pendingStatus, 0);

    MediaLibraryCommand closeCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CLOSE);
    auto fileAssetPtr = QueryPhotoAsset(PhotoColumn::MEDIA_ID, to_string(fileId));
    string uriString = MediaLibraryAssetOperations::CreateExtUriForV10Asset(*fileAssetPtr);
    ValuesBucket closeValues;
    closeValues.PutString(MEDIA_DATA_DB_URI, uriString);
    closeCmd.SetValueBucket(closeValues);
    int32_t ret = MediaLibraryPhotoOperations::Close(closeCmd);
    EXPECT_EQ(ret, 0);
    pendingStatus = GetPhotoPendingStatus(fileId);
    EXPECT_EQ(pendingStatus, 0);

    MEDIA_INFO_LOG("end tdd photo_oprn_pending_api9_test_001");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_commit_edit_insert_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_commit_edit_insert_test_001");

    int fileId = SetDefaultPhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    if (fileId < 0) {
        MEDIA_ERR_LOG("Create photo failed error=%{public}d", fileId);
        return;
    }

    const static int LARGE_NUM = 1000;
    string editData = "123456";

    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::COMMIT_EDIT,
        MediaLibraryApi::API_10);
    ValuesBucket values1;
    cmd.SetValueBucket(values1);
    EXPECT_EQ(MediaLibraryPhotoOperations::CommitEditInsert(cmd), E_INVALID_VALUES);
    ValuesBucket values2;
    values2.PutString(EDIT_DATA, editData);
    cmd.SetValueBucket(values2);
    EXPECT_EQ(MediaLibraryPhotoOperations::CommitEditInsert(cmd), E_INVALID_VALUES);
    ValuesBucket values3;
    values3.PutInt(PhotoColumn::MEDIA_ID, fileId);
    cmd.SetValueBucket(values3);
    EXPECT_EQ(MediaLibraryPhotoOperations::CommitEditInsert(cmd), E_INVALID_VALUES);
    ValuesBucket values4;
    values4.PutString(EDIT_DATA, editData);
    values4.PutInt(PhotoColumn::MEDIA_ID, fileId + LARGE_NUM);
    cmd.SetValueBucket(values4);
    EXPECT_EQ(MediaLibraryPhotoOperations::CommitEditInsert(cmd), E_INVALID_VALUES);
    int32_t ret = SetPendingOnly(UNCLOSE_FILE_TIMEPENDING, fileId);
    EXPECT_EQ(ret, 0);
    ValuesBucket values5;
    values5.PutString(EDIT_DATA, editData);
    values5.PutInt(PhotoColumn::MEDIA_ID, fileId);
    cmd.SetValueBucket(values5);
    EXPECT_EQ(MediaLibraryPhotoOperations::CommitEditInsert(cmd), E_IS_PENDING_ERROR);

    MEDIA_INFO_LOG("end tdd photo_oprn_commit_edit_insert_test_001");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_commit_edit_insert_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_commit_edit_insert_test_002");

    int fileId = SetDefaultPhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    if (fileId < 0) {
        MEDIA_ERR_LOG("Create photo failed error=%{public}d", fileId);
        return;
    }

    string editData = "123456";
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::COMMIT_EDIT,
        MediaLibraryApi::API_10);
    ValuesBucket values;
    values.PutString(EDIT_DATA, editData);
    values.PutInt(PhotoColumn::MEDIA_ID, fileId);
    cmd.SetValueBucket(values);
    EXPECT_EQ(MediaLibraryPhotoOperations::CommitEditInsert(cmd), E_OK);

    MEDIA_INFO_LOG("end tdd photo_oprn_commit_edit_insert_test_002");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_revert_edit_insert_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_revert_edit_insert_test_001");

    int fileId = SetDefaultPhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    if (fileId < 0) {
        MEDIA_ERR_LOG("Create photo failed error=%{public}d", fileId);
        return;
    }

    const static int LARGE_NUM = 1000;
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::REVERT_EDIT,
        MediaLibraryApi::API_10);
    ValuesBucket values1;
    cmd.SetValueBucket(values1);
    EXPECT_EQ(MediaLibraryPhotoOperations::RevertToOrigin(cmd), E_INVALID_VALUES);
    ValuesBucket values2;
    values2.PutInt(PhotoColumn::MEDIA_ID, fileId + LARGE_NUM);
    cmd.SetValueBucket(values2);
    EXPECT_EQ(MediaLibraryPhotoOperations::RevertToOrigin(cmd), E_INVALID_VALUES);
    int32_t ret = SetPendingOnly(UNCLOSE_FILE_TIMEPENDING, fileId);
    EXPECT_EQ(ret, 0);
    ValuesBucket values3;
    values3.PutInt(PhotoColumn::MEDIA_ID, fileId);
    cmd.SetValueBucket(values3);
    EXPECT_EQ(MediaLibraryPhotoOperations::RevertToOrigin(cmd), E_IS_PENDING_ERROR);
    ret = SetPendingOnly(0, fileId);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(MediaLibraryPhotoOperations::RevertToOrigin(cmd), E_OK);

    MEDIA_INFO_LOG("end tdd photo_oprn_revert_edit_insert_test_001");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_edit_record_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_edit_record_test_001");

    int32_t fileId1 = 1000;
    int32_t fileId2 = 1001;
    auto instance = PhotoEditingRecord::GetInstance();
    EXPECT_EQ(instance->IsInEditOperation(fileId1), false);
    EXPECT_EQ(instance->IsInEditOperation(fileId2), false);
    EXPECT_EQ(instance->StartCommitEdit(fileId1), true);
    EXPECT_EQ(instance->IsInEditOperation(fileId1), true);
    EXPECT_EQ(instance->IsInEditOperation(fileId2), false);
    EXPECT_EQ(instance->StartCommitEdit(fileId1), true);
    EXPECT_EQ(instance->IsInEditOperation(fileId1), true);
    EXPECT_EQ(instance->IsInEditOperation(fileId2), false);
    EXPECT_EQ(instance->StartCommitEdit(fileId2), true);
    EXPECT_EQ(instance->IsInEditOperation(fileId1), true);
    EXPECT_EQ(instance->IsInEditOperation(fileId2), true);
    EXPECT_EQ(instance->IsInRevertOperation(fileId1), false);
    EXPECT_EQ(instance->IsInRevertOperation(fileId2), false);
    EXPECT_EQ(instance->StartRevert(fileId1), false);
    EXPECT_EQ(instance->StartRevert(fileId2), false);
    instance->EndCommitEdit(fileId1);
    EXPECT_EQ(instance->IsInEditOperation(fileId1), false);
    EXPECT_EQ(instance->IsInEditOperation(fileId2), true);
    EXPECT_EQ(instance->StartRevert(fileId1), true);
    EXPECT_EQ(instance->IsInEditOperation(fileId1), true);
    EXPECT_EQ(instance->IsInEditOperation(fileId2), true);
    EXPECT_EQ(instance->IsInRevertOperation(fileId1), true);
    EXPECT_EQ(instance->IsInRevertOperation(fileId2), false);
    EXPECT_EQ(instance->StartRevert(fileId2), false);
    instance->EndCommitEdit(fileId2);
    EXPECT_EQ(instance->IsInEditOperation(fileId1), true);
    EXPECT_EQ(instance->IsInEditOperation(fileId2), false);
    EXPECT_EQ(instance->StartRevert(fileId1), true);
    EXPECT_EQ(instance->StartRevert(fileId2), true);
    EXPECT_EQ(instance->IsInEditOperation(fileId1), true);
    EXPECT_EQ(instance->IsInEditOperation(fileId2), true);
    EXPECT_EQ(instance->IsInRevertOperation(fileId1), true);
    EXPECT_EQ(instance->IsInRevertOperation(fileId2), true);

    instance->EndCommitEdit(fileId1);
    instance->EndCommitEdit(fileId2);
    instance->EndRevert(fileId1);
    instance->EndRevert(fileId2);
    EXPECT_EQ(instance->IsInEditOperation(fileId1), false);
    EXPECT_EQ(instance->IsInEditOperation(fileId2), false);
    EXPECT_EQ(instance->IsInRevertOperation(fileId1), false);
    EXPECT_EQ(instance->IsInRevertOperation(fileId2), false);

    MEDIA_INFO_LOG("end tdd photo_edit_record_test_001");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_edit_record_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_edit_record_test_002");

    int32_t fileId1 = 1000;
    int32_t fileId2 = 1001;

    auto instance = PhotoEditingRecord::GetInstance();
    EXPECT_EQ(instance->IsInEditOperation(fileId1), false);
    EXPECT_EQ(instance->IsInEditOperation(fileId2), false);
    EXPECT_EQ(instance->StartRevert(fileId1), true);
    EXPECT_EQ(instance->IsInEditOperation(fileId1), true);
    EXPECT_EQ(instance->IsInEditOperation(fileId2), false);
    EXPECT_EQ(instance->StartRevert(fileId1), true);
    EXPECT_EQ(instance->StartRevert(fileId2), true);
    EXPECT_EQ(instance->IsInEditOperation(fileId1), true);
    EXPECT_EQ(instance->IsInEditOperation(fileId2), true);
    EXPECT_EQ(instance->StartCommitEdit(fileId1), false);
    EXPECT_EQ(instance->StartCommitEdit(fileId2), false);
    EXPECT_EQ(instance->IsInEditOperation(fileId1), true);
    EXPECT_EQ(instance->IsInEditOperation(fileId2), true);
    instance->EndRevert(fileId1);
    EXPECT_EQ(instance->IsInEditOperation(fileId1), false);
    EXPECT_EQ(instance->IsInEditOperation(fileId2), true);
    EXPECT_EQ(instance->StartCommitEdit(fileId1), true);
    EXPECT_EQ(instance->StartCommitEdit(fileId2), false);
    EXPECT_EQ(instance->IsInEditOperation(fileId1), true);
    EXPECT_EQ(instance->IsInEditOperation(fileId2), true);
    instance->EndRevert(fileId2);
    EXPECT_EQ(instance->IsInEditOperation(fileId1), true);
    EXPECT_EQ(instance->IsInEditOperation(fileId2), false);
    EXPECT_EQ(instance->StartCommitEdit(fileId1), true);
    EXPECT_EQ(instance->StartCommitEdit(fileId2), true);
    EXPECT_EQ(instance->IsInEditOperation(fileId1), true);
    EXPECT_EQ(instance->IsInEditOperation(fileId2), true);

    instance->EndCommitEdit(fileId1);
    instance->EndCommitEdit(fileId2);
    instance->EndRevert(fileId1);
    instance->EndRevert(fileId2);
    EXPECT_EQ(instance->IsInEditOperation(fileId1), false);
    EXPECT_EQ(instance->IsInEditOperation(fileId2), false);

    MEDIA_INFO_LOG("end tdd photo_edit_record_test_002");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, multistages_capture_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd multistages_capture_test_001");

    int32_t fileId = SetDefaultPhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "MultiStagesCaptureTest001.jpg");
    EXPECT_GT(fileId, E_OK);

    // Query
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY, MediaLibraryApi::API_10);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    cmd.SetDataSharePred(predicates);
    vector<string> columns;
    columns.push_back(MediaColumn::MEDIA_NAME);
    columns.push_back(PhotoColumn::PHOTO_ID);
    columns.push_back(PhotoColumn::PHOTO_QUALITY);
    columns.push_back(PhotoColumn::PHOTO_FIRST_VISIT_TIME);
    columns.push_back(PhotoColumn::PHOTO_DEFERRED_PROC_TYPE);
    auto resultSet = MediaLibraryPhotoOperations::Query(cmd, columns);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToFirstRow(), NativeRdb::E_OK);
    string name = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
    EXPECT_EQ(name, "MultiStagesCaptureTest001.jpg");
    int photoQuality = GetInt32Val(PhotoColumn::PHOTO_QUALITY, resultSet);
    EXPECT_EQ(photoQuality, 0);
    string photoId = GetStringVal(PhotoColumn::PHOTO_ID, resultSet);
    EXPECT_EQ(photoId, "");
    int64_t firstVisitTime = GetInt64Val(PhotoColumn::PHOTO_FIRST_VISIT_TIME, resultSet);
    EXPECT_EQ(firstVisitTime, 0);
    int deferredProcType = GetInt32Val(PhotoColumn::PHOTO_DEFERRED_PROC_TYPE, resultSet);
    EXPECT_EQ(deferredProcType, 0);

    MEDIA_INFO_LOG("end tdd multistages_capture_test_001");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, multistages_capture_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd multistages_capture_test_002");

    int32_t fileId = SetDefaultPhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "MultiStagesCaptureTest001.jpg");
    EXPECT_GT(fileId, E_OK);

    // Update
    MediaLibraryCommand cmd_u(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE,
        MediaLibraryApi::API_10);
    ValuesBucket values;
    values.Put(PhotoColumn::PHOTO_QUALITY, 1);
    values.Put(PhotoColumn::PHOTO_ID, "20231031001");
    values.Put(PhotoColumn::PHOTO_FIRST_VISIT_TIME, 20231031002);
    values.Put(PhotoColumn::PHOTO_DEFERRED_PROC_TYPE, 1);
    cmd_u.SetValueBucket(values);
    cmd_u.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    EXPECT_GT(MediaLibraryPhotoOperations::Update(cmd_u), E_OK);

    // check update result
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY, MediaLibraryApi::API_10);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    cmd.SetDataSharePred(predicates);
    vector<string> columns { MediaColumn::MEDIA_NAME, PhotoColumn::PHOTO_ID, PhotoColumn::PHOTO_QUALITY,
        PhotoColumn::PHOTO_FIRST_VISIT_TIME, PhotoColumn::PHOTO_DEFERRED_PROC_TYPE };
    auto resultSet = MediaLibraryPhotoOperations::Query(cmd, columns);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToFirstRow(), NativeRdb::E_OK);
    string name = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
    EXPECT_EQ(name, "MultiStagesCaptureTest001.jpg");
    int photoQuality = GetInt32Val(PhotoColumn::PHOTO_QUALITY, resultSet);
    EXPECT_EQ(photoQuality, 1);
    string photoId = GetStringVal(PhotoColumn::PHOTO_ID, resultSet);
    EXPECT_EQ(photoId, "20231031001");
    int64_t firstVisitTime = GetInt64Val(PhotoColumn::PHOTO_FIRST_VISIT_TIME, resultSet);
    EXPECT_EQ(firstVisitTime, 20231031002);
    int deferredProcType = GetInt32Val(PhotoColumn::PHOTO_DEFERRED_PROC_TYPE, resultSet);
    EXPECT_EQ(deferredProcType, 1);

    MEDIA_INFO_LOG("end tdd multistages_capture_test_002");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_open_cache_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_open_cache_test_001");

    string fileName = to_string(MediaFileUtils::UTCTimeNanoSeconds()) + ".jpg";
    string uri = PhotoColumn::PHOTO_CACHE_URI_PREFIX + fileName;
    MediaFileUtils::UriAppendKeyValue(uri, URI_PARAM_API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri openCacheUri(uri);

    MediaLibraryCommand cmd(openCacheUri);
    cmd.SetOprnObject(OperationObject::FILESYSTEM_PHOTO);
    int32_t fd = MediaLibraryDataManager::GetInstance()->OpenFile(cmd, "w");
    EXPECT_GE(fd, 0);
    close(fd);

    MEDIA_INFO_LOG("end tdd photo_oprn_open_cache_test_001");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_open_cache_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_open_cache_test_002");

    string fileName = "open_cache_test_002.jpg";
    string uri = PhotoColumn::PHOTO_CACHE_URI_PREFIX + fileName;
    MediaFileUtils::UriAppendKeyValue(uri, URI_PARAM_API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri openCacheUri(uri);

    MediaLibraryCommand cmd(openCacheUri);
    cmd.SetOprnObject(OperationObject::FILESYSTEM_PHOTO);
    int32_t fd = MediaLibraryDataManager::GetInstance()->OpenFile(cmd, "w");
    EXPECT_GE(fd, 0);
    close(fd);

    MediaLibraryCommand duplicatedCmd(openCacheUri);
    duplicatedCmd.SetOprnObject(OperationObject::FILESYSTEM_PHOTO);
    int32_t ret = MediaLibraryDataManager::GetInstance()->OpenFile(duplicatedCmd, "w");
    EXPECT_LT(ret, 0);

    MEDIA_INFO_LOG("end tdd photo_oprn_open_cache_test_002");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_open_cache_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_open_cache_test_003");

    string unsupportedFileName = "open_cache_test_003.txt";
    string uri = PhotoColumn::PHOTO_CACHE_URI_PREFIX + unsupportedFileName;
    MediaFileUtils::UriAppendKeyValue(uri, URI_PARAM_API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri openCacheUri(uri);

    MediaLibraryCommand cmd(openCacheUri);
    cmd.SetOprnObject(OperationObject::FILESYSTEM_PHOTO);
    int32_t ret = MediaLibraryDataManager::GetInstance()->OpenFile(cmd, "w");
    EXPECT_LT(ret, 0);

    MEDIA_INFO_LOG("end tdd photo_oprn_open_cache_test_003");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_create_by_cache_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_create_by_cache_test_001");

    // open cache file
    string fileName = to_string(MediaFileUtils::UTCTimeNanoSeconds()) + ".jpg";
    string uri = PhotoColumn::PHOTO_CACHE_URI_PREFIX + fileName;
    MediaFileUtils::UriAppendKeyValue(uri, URI_PARAM_API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri openCacheUri(uri);

    MediaLibraryCommand cmd(openCacheUri);
    cmd.SetOprnObject(OperationObject::FILESYSTEM_PHOTO);
    int32_t fd = MediaLibraryDataManager::GetInstance()->OpenFile(cmd, "w");
    EXPECT_GE(fd, 0);
    close(fd);

    // create by cache
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(MediaColumn::MEDIA_NAME, "create_by_cache.jpg");
    valuesBucket.Put(MediaColumn::MEDIA_TYPE, MediaType::MEDIA_TYPE_IMAGE);
    valuesBucket.Put(CACHE_FILE_NAME, fileName);

    string assetUri;
    MediaLibraryCommand submitCacheCmd(OperationObject::FILESYSTEM_PHOTO,
        OperationType::SUBMIT_CACHE, MediaLibraryApi::API_10);
    int32_t ret = MediaLibraryDataManager::GetInstance()->InsertExt(submitCacheCmd, valuesBucket, assetUri);
    EXPECT_GT(ret, 0);
    EXPECT_EQ(assetUri.empty(), false);

    MEDIA_INFO_LOG("end tdd photo_oprn_create_by_cache_test_001");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_create_by_cache_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_create_by_cache_test_002");

    // open cache file
    string fileName = "create_by_cache_test_002.jpg";
    string uri = PhotoColumn::PHOTO_CACHE_URI_PREFIX + fileName;
    MediaFileUtils::UriAppendKeyValue(uri, URI_PARAM_API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri openCacheUri(uri);

    MediaLibraryCommand cmd(openCacheUri);
    cmd.SetOprnObject(OperationObject::FILESYSTEM_PHOTO);
    int32_t fd = MediaLibraryDataManager::GetInstance()->OpenFile(cmd, "w");
    EXPECT_GE(fd, 0);
    close(fd);

    // create by cache
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(MediaColumn::MEDIA_NAME, "create_by_cache.mp4");
    valuesBucket.Put(MediaColumn::MEDIA_TYPE, MediaType::MEDIA_TYPE_VIDEO);
    valuesBucket.Put(CACHE_FILE_NAME, fileName);

    string assetUri;
    MediaLibraryCommand submitCacheCmd(OperationObject::FILESYSTEM_PHOTO,
        OperationType::SUBMIT_CACHE, MediaLibraryApi::API_10);
    int32_t ret = MediaLibraryDataManager::GetInstance()->InsertExt(submitCacheCmd, valuesBucket, assetUri);
    EXPECT_LT(ret, 0);
    EXPECT_EQ(assetUri.empty(), true);

    MEDIA_INFO_LOG("end tdd photo_oprn_create_by_cache_test_002");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_create_by_cache_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_create_by_cache_test_003");

    // create asset in pending
    int32_t fileId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    if (fileId < 0) {
        MEDIA_ERR_LOG("CreatePhoto In APi10 failed, ret=%{public}d", fileId);
        return;
    }

    // open cache file
    string fileName = to_string(MediaFileUtils::UTCTimeNanoSeconds()) + ".jpg";
    string uri = PhotoColumn::PHOTO_CACHE_URI_PREFIX + fileName;
    MediaFileUtils::UriAppendKeyValue(uri, URI_PARAM_API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri openCacheUri(uri);

    MediaLibraryCommand cmd(openCacheUri);
    cmd.SetOprnObject(OperationObject::FILESYSTEM_PHOTO);
    int32_t fd = MediaLibraryDataManager::GetInstance()->OpenFile(cmd, "w");
    EXPECT_GE(fd, 0);
    close(fd);

    // submit cache
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(MediaColumn::MEDIA_ID, fileId);
    valuesBucket.Put(CACHE_FILE_NAME, fileName);

    MediaLibraryCommand submitCacheCmd(OperationObject::FILESYSTEM_PHOTO,
        OperationType::SUBMIT_CACHE, MediaLibraryApi::API_10);
    int32_t ret = MediaLibraryDataManager::GetInstance()->Insert(submitCacheCmd, valuesBucket);
    EXPECT_EQ(ret, fileId);

    MEDIA_INFO_LOG("end tdd photo_oprn_create_by_cache_test_003");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_create_by_cache_test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_create_by_cache_test_004");

    // open cache file
    string fileName = to_string(MediaFileUtils::UTCTimeNanoSeconds()) + ".jpg";
    string uri = PhotoColumn::PHOTO_CACHE_URI_PREFIX + fileName;
    MediaFileUtils::UriAppendKeyValue(uri, URI_PARAM_API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri openCacheUri(uri);

    MediaLibraryCommand cmd(openCacheUri);
    cmd.SetOprnObject(OperationObject::FILESYSTEM_PHOTO);
    int32_t fd = MediaLibraryDataManager::GetInstance()->OpenFile(cmd, "w");
    EXPECT_GE(fd, 0);
    close(fd);

    string videoFileName = to_string(MediaFileUtils::UTCTimeNanoSeconds()) + ".mp4";
    string videoUri = PhotoColumn::PHOTO_CACHE_URI_PREFIX + videoFileName;
    MediaFileUtils::UriAppendKeyValue(videoUri, URI_PARAM_API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri openVideoCacheUri(videoUri);

    MediaLibraryCommand videoCmd(openVideoCacheUri);
    videoCmd.SetOprnObject(OperationObject::FILESYSTEM_PHOTO);
    int32_t videoFd = MediaLibraryDataManager::GetInstance()->OpenFile(videoCmd, "w");
    EXPECT_GE(videoFd, 0);
    close(videoFd);

    // create by cache
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(MediaColumn::MEDIA_NAME, "moving_photo.jpg");
    valuesBucket.Put(MediaColumn::MEDIA_TYPE, MediaType::MEDIA_TYPE_IMAGE);
    valuesBucket.Put(PhotoColumn::PHOTO_SUBTYPE, static_cast<int>(PhotoSubType::MOVING_PHOTO));
    valuesBucket.Put(CACHE_FILE_NAME, fileName);
    valuesBucket.Put(CACHE_MOVING_PHOTO_VIDEO_NAME, videoFileName);
    MediaLibraryCommand submitCacheCmd(
        OperationObject::FILESYSTEM_PHOTO, OperationType::SUBMIT_CACHE, MediaLibraryApi::API_10);
    int32_t ret = MediaLibraryDataManager::GetInstance()->Insert(submitCacheCmd, valuesBucket);
    EXPECT_LE(ret, 0);

    MEDIA_INFO_LOG("end tdd photo_oprn_create_by_cache_test_004");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_create_by_cache_test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_create_by_cache_test_005");

    // open cache file
    string videoFileName = to_string(MediaFileUtils::UTCTimeNanoSeconds()) + ".mp4";
    string videoUri = PhotoColumn::PHOTO_CACHE_URI_PREFIX + videoFileName;
    MediaFileUtils::UriAppendKeyValue(videoUri, URI_PARAM_API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri openVideoCacheUri(videoUri);

    MediaLibraryCommand videoCmd(openVideoCacheUri);
    videoCmd.SetOprnObject(OperationObject::FILESYSTEM_PHOTO);
    int32_t videoFd = MediaLibraryDataManager::GetInstance()->OpenFile(videoCmd, "w");
    EXPECT_GE(videoFd, 0);
    close(videoFd);

    // create by cache
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(MediaColumn::MEDIA_NAME, "moving_photo.jpg");
    valuesBucket.Put(MediaColumn::MEDIA_TYPE, MediaType::MEDIA_TYPE_IMAGE);
    valuesBucket.Put(CACHE_MOVING_PHOTO_VIDEO_NAME, videoFileName);
    MediaLibraryCommand submitCacheCmd(OperationObject::FILESYSTEM_PHOTO,
        OperationType::SUBMIT_CACHE, MediaLibraryApi::API_10);
    int32_t ret = MediaLibraryDataManager::GetInstance()->Insert(submitCacheCmd, valuesBucket);
    EXPECT_EQ(ret, E_INVALID_VALUES);

    MEDIA_INFO_LOG("end tdd photo_oprn_create_by_cache_test_005");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_edit_by_cache_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_edit_by_cache_test_001");

    // create asset
    int32_t fileId = SetDefaultPhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    if (fileId < 0) {
        MEDIA_ERR_LOG("Create photo failed, ret=%{public}d", fileId);
        return;
    }

    // open cache file
    string fileName = to_string(MediaFileUtils::UTCTimeNanoSeconds()) + ".jpg";
    string uri = PhotoColumn::PHOTO_CACHE_URI_PREFIX + fileName;
    MediaFileUtils::UriAppendKeyValue(uri, URI_PARAM_API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri openCacheUri(uri);

    MediaLibraryCommand cmd(openCacheUri);
    cmd.SetOprnObject(OperationObject::FILESYSTEM_PHOTO);
    int32_t fd = MediaLibraryDataManager::GetInstance()->OpenFile(cmd, "w");
    EXPECT_GE(fd, 0);
    close(fd);

    // edit by cache
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(MediaColumn::MEDIA_ID, fileId);
    valuesBucket.Put(CACHE_FILE_NAME, fileName);
    valuesBucket.Put(COMPATIBLE_FORMAT, "compatibleFormat");
    valuesBucket.Put(FORMAT_VERSION, "formatVersion");
    valuesBucket.Put(EDIT_DATA, "data");

    MediaLibraryCommand submitCacheCmd(OperationObject::FILESYSTEM_PHOTO,
        OperationType::SUBMIT_CACHE, MediaLibraryApi::API_10);
    int32_t ret = MediaLibraryDataManager::GetInstance()->Insert(submitCacheCmd, valuesBucket);
    EXPECT_EQ(ret, fileId);

    MEDIA_INFO_LOG("end tdd photo_oprn_edit_by_cache_test_001");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_edit_by_cache_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_edit_by_cache_test_002");

    // create asset
    int32_t fileId = SetDefaultPhotoApi10(MediaType::MEDIA_TYPE_VIDEO, "photo.mp4");
    if (fileId < 0) {
        MEDIA_ERR_LOG("Create photo failed, ret=%{public}d", fileId);
        return;
    }

    // open cache file
    string fileName = to_string(MediaFileUtils::UTCTimeNanoSeconds()) + ".jpg";
    string uri = PhotoColumn::PHOTO_CACHE_URI_PREFIX + fileName;
    MediaFileUtils::UriAppendKeyValue(uri, URI_PARAM_API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri openCacheUri(uri);

    MediaLibraryCommand cmd(openCacheUri);
    cmd.SetOprnObject(OperationObject::FILESYSTEM_PHOTO);
    int32_t fd = MediaLibraryDataManager::GetInstance()->OpenFile(cmd, "w");
    EXPECT_GE(fd, 0);
    close(fd);

    // edit by cache
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(MediaColumn::MEDIA_ID, fileId);
    valuesBucket.Put(CACHE_FILE_NAME, fileName);
    valuesBucket.Put(COMPATIBLE_FORMAT, "compatibleFormat");
    valuesBucket.Put(FORMAT_VERSION, "formatVersion");
    valuesBucket.Put(EDIT_DATA, "data");

    MediaLibraryCommand submitCacheCmd(OperationObject::FILESYSTEM_PHOTO,
        OperationType::SUBMIT_CACHE, MediaLibraryApi::API_10);
    int32_t ret = MediaLibraryDataManager::GetInstance()->Insert(submitCacheCmd, valuesBucket);
    EXPECT_LT(ret, 0);

    MEDIA_INFO_LOG("end tdd photo_oprn_edit_by_cache_test_002");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_edit_by_cache_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_edit_by_cache_test_003");

    // create asset
    int32_t fileId = SetDefaultPhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    if (fileId < 0) {
        MEDIA_ERR_LOG("Create photo failed, ret=%{public}d", fileId);
        return;
    }

    // open cache file
    string fileName = to_string(MediaFileUtils::UTCTimeNanoSeconds()) + ".jpg";
    string uri = PhotoColumn::PHOTO_CACHE_URI_PREFIX + fileName;
    MediaFileUtils::UriAppendKeyValue(uri, URI_PARAM_API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri openCacheUri(uri);

    MediaLibraryCommand cmd(openCacheUri);
    cmd.SetOprnObject(OperationObject::FILESYSTEM_PHOTO);
    int32_t fd = MediaLibraryDataManager::GetInstance()->OpenFile(cmd, "w");
    EXPECT_GE(fd, 0);
    close(fd);

    // edit by cache with default edit data
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(MediaColumn::MEDIA_ID, fileId);
    valuesBucket.Put(CACHE_FILE_NAME, fileName);

    MediaLibraryCommand submitCacheCmd(OperationObject::FILESYSTEM_PHOTO,
        OperationType::SUBMIT_CACHE, MediaLibraryApi::API_10);
    int32_t ret = MediaLibraryDataManager::GetInstance()->Insert(submitCacheCmd, valuesBucket);
    EXPECT_EQ(ret, fileId);

    MEDIA_INFO_LOG("end tdd photo_oprn_edit_by_cache_test_003");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_edit_by_cache_test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_edit_by_cache_test_004");

    // create asset
    int32_t fileId = SetDefaultPhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    if (fileId < 0) {
        MEDIA_ERR_LOG("Create photo failed, ret=%{public}d", fileId);
        return;
    }

    // open cache file
    string fileName = to_string(MediaFileUtils::UTCTimeNanoSeconds()) + ".jpg";
    string uri = PhotoColumn::PHOTO_CACHE_URI_PREFIX + fileName;
    MediaFileUtils::UriAppendKeyValue(uri, URI_PARAM_API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri openCacheUri(uri);

    MediaLibraryCommand cmd(openCacheUri);
    cmd.SetOprnObject(OperationObject::FILESYSTEM_PHOTO);
    int32_t fd = MediaLibraryDataManager::GetInstance()->OpenFile(cmd, "w");
    EXPECT_GE(fd, 0);
    close(fd);

    // edit by cache with invalid edit data
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(MediaColumn::MEDIA_ID, fileId);
    valuesBucket.Put(CACHE_FILE_NAME, fileName);
    valuesBucket.Put(FORMAT_VERSION, "v1.0");

    MediaLibraryCommand submitCacheCmd(OperationObject::FILESYSTEM_PHOTO,
        OperationType::SUBMIT_CACHE, MediaLibraryApi::API_10);
    int32_t ret = MediaLibraryDataManager::GetInstance()->Insert(submitCacheCmd, valuesBucket);
    EXPECT_LT(ret, 0);

    MEDIA_INFO_LOG("end tdd photo_oprn_edit_by_cache_test_004");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_delete_cache_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_delete_cache_test_001");

    // open cache file
    string fileName = "delete_cache_test_001.jpg";
    string uri = PhotoColumn::PHOTO_CACHE_URI_PREFIX + fileName;
    MediaFileUtils::UriAppendKeyValue(uri, URI_PARAM_API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri openCacheUri(uri);

    MediaLibraryCommand cmd(openCacheUri);
    cmd.SetOprnObject(OperationObject::FILESYSTEM_PHOTO);
    int32_t fd = MediaLibraryDataManager::GetInstance()->OpenFile(cmd, "w");
    EXPECT_GE(fd, 0);

    // delete cache file
    string deleteUri = PhotoColumn::PHOTO_CACHE_URI_PREFIX + fileName;
    MediaFileUtils::UriAppendKeyValue(deleteUri, URI_PARAM_API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri deleteCacheUri(deleteUri);

    MediaLibraryCommand deleteCacheCmd(deleteCacheUri);
    deleteCacheCmd.SetOprnObject(OperationObject::FILESYSTEM_PHOTO);
    DataSharePredicates predicates;
    int32_t ret = MediaLibraryDataManager::GetInstance()->Delete(deleteCacheCmd, predicates);
    EXPECT_EQ(ret, E_OK);

    MEDIA_INFO_LOG("end tdd photo_oprn_delete_cache_test_001");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_delete_cache_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_delete_cache_test_002");

    // open cache file
    string fileName = "delete_cache_test_002.jpg";
    string uri = PhotoColumn::PHOTO_CACHE_URI_PREFIX + fileName;
    MediaFileUtils::UriAppendKeyValue(uri, URI_PARAM_API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri openCacheUri(uri);

    MediaLibraryCommand cmd(openCacheUri);
    cmd.SetOprnObject(OperationObject::FILESYSTEM_PHOTO);
    int32_t fd = MediaLibraryDataManager::GetInstance()->OpenFile(cmd, "w");
    EXPECT_GE(fd, 0);

    // delete cache file
    string deleteUri = PhotoColumn::PHOTO_CACHE_URI_PREFIX + "NONE.jpg";
    MediaFileUtils::UriAppendKeyValue(deleteUri, URI_PARAM_API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri deleteCacheUri(deleteUri);

    MediaLibraryCommand deleteCacheCmd(deleteCacheUri);
    deleteCacheCmd.SetOprnObject(OperationObject::FILESYSTEM_PHOTO);
    DataSharePredicates predicates;
    int32_t ret = MediaLibraryDataManager::GetInstance()->Delete(deleteCacheCmd, predicates);
    EXPECT_LT(ret, 0);

    MEDIA_INFO_LOG("end tdd photo_oprn_delete_cache_test_002");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_batch_update_user_comment_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_batch_update_user_comment_test_001");
    int32_t fileId1 = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo1.jpg");
    if (fileId1 < 0) {
        MEDIA_ERR_LOG("CreatePhoto failed, ret=%{public}d", fileId1);
        return;
    }

    int32_t fileId2 = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo2.jpg");
    if (fileId2 < 0) {
        MEDIA_ERR_LOG("CreatePhoto failed, ret=%{public}d", fileId2);
        return;
    }

    int32_t fileId3 = CreatePhotoApi10(MediaType::MEDIA_TYPE_VIDEO, "photo3.mp4");
    if (fileId3 < 0) {
        MEDIA_ERR_LOG("CreatePhoto failed, ret=%{public}d", fileId3);
        return;
    }

    vector<string> assetsArray = { to_string(fileId1), to_string(fileId2), to_string(fileId3) };
    string newUserComment = "batch update user comment";
    DataSharePredicates predicates;
    predicates.In(PhotoColumn::MEDIA_ID, assetsArray);
    DataShareValuesBucket values;
    values.Put(PhotoColumn::PHOTO_USER_COMMENT, newUserComment);
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO,
        OperationType::BATCH_UPDATE_USER_COMMENT, MediaLibraryApi::API_10);
    int32_t changedRows = MediaLibraryDataManager::GetInstance()->Update(cmd, values, predicates);
    EXPECT_EQ(changedRows, 3);

    MEDIA_INFO_LOG("end tdd photo_oprn_batch_update_user_comment_test_001");
}

HWTEST_F(MediaLibraryPhotoOperationsTest, photo_oprn_read_moving_photo_video_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("start tdd photo_oprn_read_moving_photo_video_test_001");

    // create moving photo
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE, MediaLibraryApi::API_10);
    ValuesBucket values;
    values.PutString(ASSET_EXTENTION, "jpg");
    values.PutString(PhotoColumn::MEDIA_TITLE, "moving_photo");
    values.PutInt(MediaColumn::MEDIA_TYPE, MediaType::MEDIA_TYPE_IMAGE);
    values.PutInt(PhotoColumn::PHOTO_SUBTYPE, static_cast<int>(PhotoSubType::MOVING_PHOTO));
    cmd.SetValueBucket(values);
    cmd.SetBundleName("values");
    MediaLibraryPhotoOperations::Create(cmd);
    int32_t fileId = QueryPhotoIdByDisplayName("moving_photo.jpg");
    ASSERT_GE(fileId, 0);

    // Open and close to set pending time to 0
    MediaFileUri fileUri(MediaType::MEDIA_TYPE_IMAGE, to_string(fileId), "", MEDIA_API_VERSION_V10);
    string fileUriStr = fileUri.ToString();
    Uri uri(fileUriStr);
    MediaLibraryCommand openImageCmd(uri, Media::OperationType::OPEN);
    int32_t fd = MediaLibraryPhotoOperations::Open(openImageCmd, "w");
    ASSERT_GE(fd, 0);

    MediaLibraryCommand closeCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CLOSE);
    ValuesBucket closeValues;
    closeValues.PutString(MEDIA_DATA_DB_URI, fileUriStr);
    closeCmd.SetValueBucket(closeValues);
    MediaLibraryPhotoOperations::Close(closeCmd);

    // read moving photo video
    string videoUriStr = fileUriStr;
    MediaFileUtils::UriAppendKeyValue(videoUriStr, MEDIA_MOVING_PHOTO_OPRN_KEYWORD, OPEN_MOVING_PHOTO_VIDEO);
    Uri videoUri(videoUriStr);
    MediaLibraryCommand readVideoCmd(videoUri, Media::OperationType::OPEN);
    readVideoCmd.SetOprnObject(OperationObject::FILESYSTEM_PHOTO);
    int32_t rfd = MediaLibraryDataManager::GetInstance()->OpenFile(readVideoCmd, "r");
    EXPECT_GE(rfd, 0);
    close(rfd);

    MEDIA_INFO_LOG("end tdd photo_oprn_read_moving_photo_video_test_001");
}
} // namespace Media
} // namespace OHOS