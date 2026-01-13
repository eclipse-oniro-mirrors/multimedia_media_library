/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#define MLOG_TAG "PhotoEditTest"

#include "medialibrary_photo_edit_test.h"

#include <chrono>
#include <cstdint>
#include <fcntl.h>
#include <fstream>
#include <thread>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>

#include "abs_rdb_predicates.h"
#include "fetch_result.h"
#include "file_asset.h"
#include "fcntl.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_command.h"
#include "medialibrary_common_utils.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "medialibrary_photo_operations.h"
#include "medialibrary_inotify.h"
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
using OHOS::DataShare::DataSharePredicates;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;

static const uint8_t BUF[] = {
    255, 216, 255, 224, 0, 16, 74, 70, 73, 70, 0, 1, 1, 1, 0, 96, 0, 96, 0, 0, 255, 219, 0, 67, 0, 8, 6, 6, 7, 6, 5, 8,
    7, 7, 7, 9, 9, 8, 10, 12, 20, 13, 12, 11, 11, 12, 25, 18, 19, 15, 20, 29, 26, 31, 30, 29, 26, 28, 28, 32, 36, 46,
    39, 32, 34, 44, 35, 28, 28, 40, 55, 41, 44, 48, 49, 52, 52, 52, 31, 39, 57, 61, 56, 50, 60, 46, 51, 52, 50, 255,
    219, 0, 67, 1, 9, 9, 9, 12, 11, 12, 24, 13, 13, 24, 50, 33, 28, 33, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
    50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
    50, 50, 50, 50, 50, 50, 50, 50, 50, 255, 192, 0, 17, 8, 0, 132, 0, 132, 3, 1, 34, 0, 2, 17, 1, 3, 17, 1, 255, 196,
    0, 31, 0, 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 255, 196, 0, 181,
    16, 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 125, 1, 2, 3, 0, 4, 17, 5, 18, 33, 49, 65, 6, 19, 81, 97, 7, 34,
    113, 20, 50, 129, 145, 161, 8, 35, 66, 177, 193, 21, 82, 209, 240, 36, 51, 98, 114, 130, 9, 10, 22, 23, 24, 25, 26,
    37, 38, 39, 40, 41, 42, 52, 53, 54, 55, 56, 57, 58, 67, 68, 69, 70, 71, 72, 73, 74, 83, 84, 85, 86, 87, 88, 89, 90,
    99, 100, 101, 102, 103, 104, 105, 106, 115, 116, 117, 118, 119, 120, 121, 122, 131, 132, 133, 134, 135, 136, 137,
    138, 146, 147, 148, 149, 150, 151, 152, 153, 154, 162, 163, 164, 165, 166, 167, 168, 169, 170, 178, 179, 180, 181,
    182, 183, 184, 185, 186, 194, 195, 196, 197, 198, 199, 200, 201, 202, 210, 211, 212, 213, 214, 215, 216, 217, 218,
    225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 255, 196, 0, 31,
    1, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 255, 196, 0, 181, 17, 0,
    2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 119, 0, 1, 2, 3, 17, 4, 5, 33, 49, 6, 18, 65, 81, 7, 97, 113, 19, 34, 50,
    129, 8, 20, 66, 145, 161, 177, 193, 9, 35, 51, 82, 240, 21, 98, 114, 209, 10, 22, 36, 52, 225, 37, 241, 23, 24, 25,
    26, 38, 39, 40, 41, 42, 53, 54, 55, 56, 57, 58, 67, 68, 69, 70, 71, 72, 73, 74, 83, 84, 85, 86, 87, 88, 89, 90, 99,
    100, 101, 102, 103, 104, 105, 106, 115, 116, 117, 118, 119, 120, 121, 122, 130, 131, 132, 133, 134, 135, 136, 137,
    138, 146, 147, 148, 149, 150, 151, 152, 153, 154, 162, 163, 164, 165, 166, 167, 168, 169, 170, 178, 179, 180, 181,
    182, 183, 184, 185, 186, 194, 195, 196, 197, 198, 199, 200, 201, 202, 210, 211, 212, 213, 214, 215, 216, 217, 218,
    226, 227, 228, 229, 230, 231, 232, 233, 234, 242, 243, 244, 245, 246, 247, 248, 249, 250, 255, 218, 0, 12, 3, 1, 0,
    2, 17, 3, 17, 0, 63, 0, 244, 74, 40, 162, 191, 35, 62, 148, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40,
    162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0,
    40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138,
    0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162,
    138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40,
    162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0,
    40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138,
    0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162,
    138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40,
    162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0,
    40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138,
    0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162,
    138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 255, 217
};

static const std::string EDITDATA_VALUE = "{\"imageEffect\":{\"filters\":[{\"name\":\"InplaceSticker\","
    "\"values\":{\"RESOURCE_DIRECTORY\":\"/sys_prod/resource/camera\"}}],\"name\":\"brandWaterMark\"}}";
static const std::string COMPATIBLE_FORMAT_VALUE = "com.demo.test";
static const std::string FORMAT_VERSION_VALUE = "0";
static const std::string ROOT_DIR = "/storage/cloud/files/";
static const std::string WATER_MARK_DIR = "/sys_prod/resource/camera/watermark";
static const std::string CAMERA_BUNDLE_NAME = "com.huawei.hmos.camera";

struct PhotoMode {
    string path;
    int64_t editTime;
};

struct TakePhotoResult {
    int32_t fileId;
    string path;
};

string GetFileName()
{
    return to_string(MediaFileUtils::UTCTimeNanoSeconds()) + ".jpg";
}

bool IsWaterMarkExists()
{
    return MediaFileUtils::IsFileExists(WATER_MARK_DIR);
}

int32_t WriteFile(int destfd)
{
    MEDIA_INFO_LOG("WriteFile enter");
    write(destfd, BUF, sizeof(BUF));
    return E_OK;
}

unique_ptr<FileAsset> QueryPhotoAsset(const string &columnName, const string &value)
{
    string querySql = "SELECT * FROM " + PhotoColumn::PHOTOS_TABLE + " WHRE " +
        columnName + "='" + value + "';";

    MEDIA_DEBUG_LOG("querySql: %{public}s", querySql.c_str());
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("g_rdbStore is nullptr");
        return nullptr;
    }
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

void WriteDefferedPhoto(std::shared_ptr<FileAsset> &fileAsset)
{
    MEDIA_INFO_LOG("WriteDefferedPhoto start");
    int ret = MediaLibraryPhotoOperations::ProcessMultistagesPhoto(fileAsset, BUF, sizeof(BUF));
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("WriteDefferedPhoto end");
}

string CreateCacheFile()
{
    MEDIA_INFO_LOG("CreateCacheFile enter");
    string cacheFileName = GetFileName();
    string uri = PhotoColumn::PHOTO_CACHE_URI_PREFIX + cacheFileName;
    MediaFileUtils::UriAppendKeyValue(uri, URI_PARAM_API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri openCacheUri(uri);
    MediaLibraryCommand cmd(openCacheUri);
    int32_t cacheFileFd = MediaLibraryDataManager::GetInstance()->OpenFile(cmd, "w");
    EXPECT_GE(cacheFileFd, 0);
    WriteFile(cacheFileFd);
    close(cacheFileFd);
    return cacheFileName;
}

PhotoMode GetPhotoMode(int fileId)
{
    MEDIA_INFO_LOG("GetPhotoMode start");
    PhotoMode photoMode;
    EXPECT_EQ((fileId > 0), true);
    vector<string> columns = { PhotoColumn::MEDIA_FILE_PATH, PhotoColumn::PHOTO_EDIT_TIME };
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY,
        MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    EXPECT_EQ((g_rdbStore == nullptr), false);
    auto resultSet = g_rdbStore->Query(cmd, columns);
    EXPECT_EQ((resultSet == nullptr), false);
    EXPECT_EQ((resultSet->GoToFirstRow()), NativeRdb::E_OK);
    photoMode.path = GetStringVal(PhotoColumn::MEDIA_FILE_PATH, resultSet);
    photoMode.editTime = GetInt64Val(PhotoColumn::PHOTO_EDIT_TIME, resultSet);
    MEDIA_INFO_LOG("GetPhotoMode end");
    return photoMode;
}

string GetFilePath(int fileId)
{
    PhotoMode mode = GetPhotoMode(fileId);
    return mode.path;
}

bool IsEdited(int fileId)
{
    PhotoMode mode = GetPhotoMode(fileId);
    return mode.editTime > 0;
}

int32_t CreatePhotoAsset()
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE,
        MediaLibraryApi::API_10);
    OHOS::NativeRdb::ValuesBucket values;
    string displayName = GetFileName();
    values.PutString(MediaColumn::MEDIA_NAME, displayName);
    values.PutInt(MediaColumn::MEDIA_TYPE, MediaType::MEDIA_TYPE_IMAGE);
    cmd.SetValueBucket(values);
    int32_t ret = MediaLibraryPhotoOperations::Create(cmd);
    MEDIA_INFO_LOG("CreatePhotoAsset, fileId=%{public}d", ret);
    if (ret < 0) {
        MEDIA_ERR_LOG("Create Photo failed, errCode=%{public}d", ret);
        return ret;
    }
    return ret;
}

void SubmitCache(DataShareValuesBucket &valuesBucket, bool isAddWater, bool isEdited)
{
    MEDIA_INFO_LOG("SubmitCache enter");
    MediaLibraryCommand submitCacheCmd(OperationObject::FILESYSTEM_PHOTO,
        OperationType::SUBMIT_CACHE, MediaLibraryApi::API_10);
    if (isAddWater && !isEdited) {
        submitCacheCmd.SetBundleName(CAMERA_BUNDLE_NAME);
    }
    string assetUri;
    int32_t ret = MediaLibraryDataManager::GetInstance()->InsertExt(submitCacheCmd, valuesBucket, assetUri);
    MEDIA_INFO_LOG("SubmitCache, ret=%{public}d", ret);
    EXPECT_GT(ret, 0);
}

DataShareValuesBucket GetValuesBucket(int32_t fileId, std::string cacheFileName, bool isSetEditData)
{
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(PhotoColumn::MEDIA_ID, fileId);
    valuesBucket.Put(CACHE_FILE_NAME, cacheFileName);
    if (isSetEditData) {
        valuesBucket.Put(EDIT_DATA, EDITDATA_VALUE);
        valuesBucket.Put(COMPATIBLE_FORMAT, COMPATIBLE_FORMAT_VALUE);
        valuesBucket.Put(FORMAT_VERSION, FORMAT_VERSION_VALUE);
    }
    return valuesBucket;
}

string GetEditDataDirPath(std::string &path)
{
    return ROOT_DIR + ".editData/" + path.substr(ROOT_DIR.length());
}

bool ValidSourceFile(std::string &path)
{
    string sourcePath = GetEditDataDirPath(path) + "/source.jpg";
    return MediaFileUtils::IsFileExists(sourcePath);
}

bool ValidEditdata(std::string &path)
{
    string editdataPath = GetEditDataDirPath(path) + "/editdata";
    return MediaFileUtils::IsFileExists(editdataPath);
}

bool ValidEditdataCamera(std::string &path)
{
    string editdataCameraPath = GetEditDataDirPath(path) + "/editdata_camera";
    return MediaFileUtils::IsFileExists(editdataCameraPath);
}

bool ValidPhoto(std::string &path)
{
    return MediaFileUtils::IsFileExists(path);
}

TakePhotoResult TakePhotoMock(bool isAddWater)
{
    TakePhotoResult result;
    string cacheFileName = CreateCacheFile();
    int32_t fileId = CreatePhotoAsset();
    string path = GetFilePath(fileId);
    result.fileId = fileId;
    result.path = path;
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(PhotoColumn::MEDIA_ID, fileId);
    valuesBucket.Put(CACHE_FILE_NAME, cacheFileName);
    if (!isAddWater) {
        SubmitCache(valuesBucket, isAddWater, false);
        EXPECT_EQ(ValidSourceFile(path), false);
        EXPECT_EQ(ValidEditdata(path), false);
        EXPECT_EQ(ValidEditdataCamera(path), false);
        EXPECT_EQ(ValidPhoto(path), true);
    } else {
        valuesBucket.Put(EDIT_DATA, EDITDATA_VALUE);
        valuesBucket.Put(COMPATIBLE_FORMAT, COMPATIBLE_FORMAT_VALUE);
        valuesBucket.Put(FORMAT_VERSION, FORMAT_VERSION_VALUE);
        SubmitCache(valuesBucket, isAddWater, false);
        EXPECT_EQ(ValidSourceFile(path), true);
        EXPECT_EQ(ValidEditdata(path), false);
        EXPECT_EQ(ValidEditdataCamera(path), true);
        EXPECT_EQ(ValidPhoto(path), true);
    }
    return result;
}

void EditPhoto(TakePhotoResult result, bool isSetEditData, bool isAddWater)
{
    string cacheFileName = CreateCacheFile();
    DataShareValuesBucket valuesBucket = GetValuesBucket(result.fileId, cacheFileName, isSetEditData);
    SubmitCache(valuesBucket, isAddWater, true);
    EXPECT_EQ(ValidSourceFile(result.path), true);
    EXPECT_EQ(ValidEditdata(result.path), true);
    EXPECT_EQ(ValidPhoto(result.path), true);
    EXPECT_EQ(ValidEditdataCamera(result.path), isAddWater);
}

void Revert(TakePhotoResult result, bool isAddWater)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::REVERT_EDIT,
        MediaLibraryApi::API_10);
    OHOS::NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::MEDIA_ID, result.fileId);
    cmd.SetValueBucket(values);
    EXPECT_EQ(MediaLibraryPhotoOperations::RevertToOrigin(cmd), E_OK);
    if (!isAddWater) {
        EXPECT_EQ(ValidSourceFile(result.path), false);
        EXPECT_EQ(ValidEditdata(result.path), false);
        EXPECT_EQ(ValidPhoto(result.path), true);
    } else {
        EXPECT_EQ(ValidSourceFile(result.path), true);
        EXPECT_EQ(ValidEditdata(result.path), false);
        EXPECT_EQ(ValidPhoto(result.path), true);
        EXPECT_EQ(ValidEditdataCamera(result.path), true);
    }
}

void CleanTables()
{
    string clearPhotoSql = "DELETE FROM " + PhotoColumn::PHOTOS_TABLE;
    vector<string> executeSqlStrs = {
        clearPhotoSql
    };
    int err;
    for (const auto &sql : executeSqlStrs) {
        err = g_rdbStore->ExecuteSql(sql);
        EXPECT_EQ(err, E_OK);
    }
}

void ClearFiles()
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
    CleanTables();
}

void MediaLibraryPhotoEditTest::SetUpTestCase()
{
    MEDIA_INFO_LOG("MediaLibraryPhotoEditTest SetUpTestCase start");
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    MEDIA_INFO_LOG("MediaLibraryPhotoEditTest SetUpTestCase end");
}

void MediaLibraryPhotoEditTest::TearDownTestCase()
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    system("rm -rf /storage/cloud/files/*");
    ClearFiles();
    g_rdbStore = nullptr;
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    this_thread::sleep_for(chrono::seconds(1));
    MEDIA_INFO_LOG("Clean is finish");
    this_thread::sleep_for(chrono::seconds(SLEEP_FIVE_SECONDS));
}

void MediaLibraryPhotoEditTest::SetUp()
{}

void MediaLibraryPhotoEditTest::TearDown()
{}

// 普通拍照无水印无滤镜，只保存编辑后图片，媒体库生成空的editdata
HWTEST_F(MediaLibraryPhotoEditTest, common_photos_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("start common_photos_test_001");
    // 普通拍照无水印
    TakePhotoResult result = TakePhotoMock(false);
    // 只保存编辑后图片
    EditPhoto(result, false, false);
    // 回退
    Revert(result, false);
    MEDIA_INFO_LOG("end common_photos_test_001");
}

// 普通拍照无水印无滤镜，保存编辑后图片和editdata
HWTEST_F(MediaLibraryPhotoEditTest, common_photos_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("start common_photos_test_002");
    // 普通拍照无水印
    TakePhotoResult result = TakePhotoMock(false);
    // 只保存编辑后图片
    EditPhoto(result, true, false);
    // 回退
    Revert(result, false);
    MEDIA_INFO_LOG("end common_photos_test_002");
}

// 普通拍照有水印/滤镜，只保存编辑后图片，媒体库生成空的editdata
HWTEST_F(MediaLibraryPhotoEditTest, common_photos_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("start common_photos_test_003");
    ASSERT_FALSE(IsWaterMarkExists());

    // 普通拍照有水印
    TakePhotoResult result = TakePhotoMock(true);
    // 只保存编辑后图片
    EditPhoto(result, false, true);
    // 回退
    Revert(result, true);
    MEDIA_INFO_LOG("end common_photos_test_003");
}

// 普通拍照有水印/滤镜，保存编辑后图片和editdata
HWTEST_F(MediaLibraryPhotoEditTest, common_photos_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("start common_photos_test_004");
    ASSERT_FALSE(IsWaterMarkExists());

    // 普通拍照有水印
    TakePhotoResult result = TakePhotoMock(true);
    // 保存编辑后图片和编辑数据
    EditPhoto(result, true, true);
    // 回退
    Revert(result, true);
    MEDIA_INFO_LOG("end common_photos_test_004");
}

// 二阶段拍照，不编辑，不加水印滤镜
HWTEST_F(MediaLibraryPhotoEditTest, deferred_photos_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("start deferred_photos_test_001");
    // 一阶段拍照不加水印
    TakePhotoResult result = TakePhotoMock(false);
    // 二阶段落盘
    bool isEdited = IsEdited(result.fileId);
    EXPECT_EQ(isEdited, false);

    auto fileAssetPtr = QueryPhotoAsset(PhotoColumn::MEDIA_ID, to_string(result.fileId));
    shared_ptr<FileAsset> fileAsset = std::move(fileAssetPtr);
    WriteDefferedPhoto(fileAsset);
    MEDIA_INFO_LOG("end deferred_photos_test_001");
}

// 二阶段拍照，不编辑，加水印滤镜
HWTEST_F(MediaLibraryPhotoEditTest, deferred_photos_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("start deferred_photos_test_002");
    ASSERT_FALSE(IsWaterMarkExists());

    // 一阶段拍照加水印
    TakePhotoResult result = TakePhotoMock(true);
    // 二阶段落盘
    bool isEdited = IsEdited(result.fileId);
    EXPECT_EQ(isEdited, false);

    auto fileAssetPtr = QueryPhotoAsset(PhotoColumn::MEDIA_ID, to_string(result.fileId));
    shared_ptr<FileAsset> fileAsset = std::move(fileAssetPtr);
    WriteDefferedPhoto(fileAsset);
    MEDIA_INFO_LOG("end deferred_photos_test_002");
}

// 二阶段无水印无滤镜，只保存编辑后图片，媒体库生成空的editdata
HWTEST_F(MediaLibraryPhotoEditTest, deferred_photos_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("start deferred_photos_test_003");
    // 一阶段拍照无水印
    TakePhotoResult result = TakePhotoMock(false);
    // 只保存编辑后图片
    EditPhoto(result, false, false);
    // 二阶段落盘
    bool isEdited = IsEdited(result.fileId);
    EXPECT_EQ(isEdited, true);

    auto fileAssetPtr = QueryPhotoAsset(PhotoColumn::MEDIA_ID, to_string(result.fileId));
    shared_ptr<FileAsset> fileAsset = std::move(fileAssetPtr);
    WriteDefferedPhoto(fileAsset);
    // 回退
    Revert(result, false);
    MEDIA_INFO_LOG("end deferred_photos_test_003");
}

// 二阶段无水印无滤镜，保存编辑后图片和editdata
HWTEST_F(MediaLibraryPhotoEditTest, deferred_photos_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("start deferred_photos_test_004");
    // 一阶段拍照无水印
    TakePhotoResult result = TakePhotoMock(false);
    // 保存编辑后图片和编辑数据
    EditPhoto(result, true, false);
    // 二阶段落盘
    bool isEdited = IsEdited(result.fileId);
    EXPECT_EQ(isEdited, true);

    auto fileAssetPtr = QueryPhotoAsset(PhotoColumn::MEDIA_ID, to_string(result.fileId));
    shared_ptr<FileAsset> fileAsset = std::move(fileAssetPtr);
    WriteDefferedPhoto(fileAsset);
    // 回退
    Revert(result, false);
    MEDIA_INFO_LOG("end deferred_photos_test_004");
}

// 二阶段有水印/滤镜，只保存编辑后图片，媒体库生成空的editdata
HWTEST_F(MediaLibraryPhotoEditTest, deferred_photos_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("start deferred_photos_test_005");
    ASSERT_FALSE(IsWaterMarkExists());

    // 一阶段拍照有水印
    TakePhotoResult result = TakePhotoMock(true);
    // 只保存编辑后图片
    EditPhoto(result, false, true);
    // 二阶段落盘
    bool isEdited = IsEdited(result.fileId);
    EXPECT_EQ(isEdited, true);

    auto fileAssetPtr = QueryPhotoAsset(PhotoColumn::MEDIA_ID, to_string(result.fileId));
    shared_ptr<FileAsset> fileAsset = std::move(fileAssetPtr);
    WriteDefferedPhoto(fileAsset);
    // 回退
    Revert(result, true);
    MEDIA_INFO_LOG("end deferred_photos_test_005");
}

// 二阶段有水印/滤镜，保存编辑后图片和editdata
HWTEST_F(MediaLibraryPhotoEditTest, deferred_photos_test_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("start deferred_photos_test_006");
    ASSERT_FALSE(IsWaterMarkExists());

    // 一阶段拍照无水印
    TakePhotoResult result = TakePhotoMock(true);
    // 保存编辑后图片和编辑数据
    EditPhoto(result, true, true);
    // 二阶段落盘
    bool isEdited = IsEdited(result.fileId);
    EXPECT_EQ(isEdited, true);

    auto fileAssetPtr = QueryPhotoAsset(PhotoColumn::MEDIA_ID, to_string(result.fileId));
    shared_ptr<FileAsset> fileAsset = std::move(fileAssetPtr);
    WriteDefferedPhoto(fileAsset);
    // 回退
    Revert(result, true);
    MEDIA_INFO_LOG("end deferred_photos_test_006");
}

}
}