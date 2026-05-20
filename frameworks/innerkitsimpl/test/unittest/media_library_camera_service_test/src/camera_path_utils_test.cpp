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

#define MLOG_TAG "CameraPathUtilsTest"

#include "camera_path_utils_test.h"

#include "camera_path_utils.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_upgrade.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_errno.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "nlohmann/json.hpp"

using namespace std;
using namespace OHOS;
using namespace testing::ext;
using namespace testing;

namespace OHOS {
namespace Media {
static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;

const std::string SOURCE_PATH = "/storage/cloud/files/Photo/16/IMG_4619378_002.jpg";
const std::string EDITDATA_CAMERA_PATH = "/storage/cloud/files/.editData/Photo/16/IMG_4619378_002.jpg";

static void CleanTestTables()
{
    std::vector<std::string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
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

static void SetTables()
{
    std::vector<std::string> createTableSqlList = {
        PhotoUpgrade::CREATE_PHOTO_TABLE,
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

static void ClearAndRestart()
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    system("rm -rf /storage/cloud/files/*");
    system("rm -rf /storage/cloud/files/.thumbs");
    system("rm -rf /storage/cloud/files/.editData");
    for (const auto &dir : TEST_ROOT_DIRS) {
        std::string ROOT_PATH = "/storage/cloud/100/files/";
        bool ret = MediaFileUtils::CreateDirectory(ROOT_PATH + dir + "/");
        CHECK_AND_PRINT_LOG(ret, "make %{public}s dir failed, ret=%{public}d", dir.c_str(), ret);
    }
    CleanTestTables();
    SetTables();
}

void CameraPathUtilsTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    SetTables();
}

void CameraPathUtilsTest::TearDownTestCase(void)
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    system("rm -rf /storage/cloud/files/*");
    ClearAndRestart();
    g_rdbStore = nullptr;
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    this_thread::sleep_for(chrono::seconds(SLEEP_FIVE_SECONDS));
    MEDIA_INFO_LOG("Clean is finish");
}

// SetUp:Execute before each test case
void CameraPathUtilsTest::SetUp()
{
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    ClearAndRestart();
    system("mkdir -p /storage/cloud/files/.editData/");
}

void CameraPathUtilsTest::TearDown(void)
{
    system("rm -rf /storage/cloud/files/*");
    system("rm -rf /storage/cloud/files/.thumbs");
    system("rm -rf /storage/cloud/files/.editData");
}

/**
 * @tc.name: CameraPathUtils_SaveEditDataCameraByString_test01
 * @tc.desc: [允许 string 写入 editdata_camera]
 *           [1] 需要包含3个部分: compatible_format、format_version、edit_data
 *           [2] 即使 input 中包含 app_id 也不会使用, 根据 bundle_name 重新写入
 */
HWTEST_F(CameraPathUtilsTest, CameraPathUtils_SaveEditDataCameraByString_test01, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter NewImagePipeline_SaveEditDataCamera_test01");

    ASSERT_EQ(MediaFileUtils::IsFileExists("/storage/cloud/files/.editData/"), true);
    ASSERT_EQ(MediaFileUtils::IsFileExists("/storage/cloud/files/.editData/Photo/"), false);
    ASSERT_EQ(MediaFileUtils::IsFileExists(EDITDATA_CAMERA_PATH), false);

    // 执行
    CameraPathUtils::SaveEditDataCameraByString(SOURCE_PATH, EDITDATA_CAMERA_STRING_VALID, BUNDLE_NAME_TEST);
    ASSERT_EQ(MediaFileUtils::IsFileExists(EDITDATA_CAMERA_PATH), true);

    // value 符合预期
    std::string editdata;
    int32_t ret = CameraPathUtils::ReadEditdataCameraFromFile(SOURCE_PATH, false, editdata);
    ASSERT_EQ(ret, E_OK);
    ASSERT_EQ(nlohmann::json::accept(editdata), true);

    nlohmann::json dataJson = nlohmann::json::parse(editdata);
    ASSERT_EQ(dataJson.contains(CONST_COMPATIBLE_FORMAT) && dataJson[CONST_COMPATIBLE_FORMAT].is_string(), true);
    ASSERT_EQ(dataJson.contains(CONST_FORMAT_VERSION) && dataJson[CONST_FORMAT_VERSION].is_string(), true);
    ASSERT_EQ(dataJson.contains(CONST_EDIT_DATA) && dataJson[CONST_EDIT_DATA].is_string(), true);
    ASSERT_EQ(dataJson.contains(CONST_APP_ID) && dataJson[CONST_APP_ID].is_string(), true);

    ASSERT_EQ(dataJson.at(CONST_COMPATIBLE_FORMAT), COMPATIBLE_FORMAT);
    ASSERT_EQ(dataJson.at(CONST_FORMAT_VERSION), FORMAT_VERSION);
    ASSERT_EQ(dataJson.at(CONST_APP_ID), BUNDLE_NAME_CAMERA);
}

/**
 * @tc.name: CameraPathUtils_SaveEditDataCameraByString_test02
 * @tc.desc: [允许 string 写入 editdata_camera]
 *           [1] 以下3个部分均[不包含]: compatible_format、format_version、edit_data
 *           [2] app_id 会被写入
 */
HWTEST_F(CameraPathUtilsTest, CameraPathUtils_SaveEditDataCameraByString_test02, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter CameraPathUtils_SaveEditDataCameraByString_test02");

    ASSERT_EQ(MediaFileUtils::IsFileExists("/storage/cloud/files/.editData/"), true);
    ASSERT_EQ(MediaFileUtils::IsFileExists("/storage/cloud/files/.editData/Photo/"), false);
    ASSERT_EQ(MediaFileUtils::IsFileExists(EDITDATA_CAMERA_PATH), false);

    // 执行
    CameraPathUtils::SaveEditDataCameraByString(SOURCE_PATH, EDITDATA_CAMERA_NULL_STRING, BUNDLE_NAME_TEST);
    ASSERT_EQ(MediaFileUtils::IsFileExists(EDITDATA_CAMERA_PATH), true);

    // value 符合预期
    std::string editdata;
    int32_t ret = CameraPathUtils::ReadEditdataCameraFromFile(SOURCE_PATH, false, editdata);
    ASSERT_EQ(ret, E_OK);
    ASSERT_EQ(nlohmann::json::accept(editdata), true);

    nlohmann::json dataJson = nlohmann::json::parse(editdata);
    ASSERT_EQ(dataJson.contains(CONST_COMPATIBLE_FORMAT), true);
    ASSERT_EQ(dataJson.at(CONST_COMPATIBLE_FORMAT), BUNDLE_NAME_TEST);
    ASSERT_EQ(dataJson.contains(CONST_FORMAT_VERSION), true);
    ASSERT_EQ(dataJson.at(CONST_FORMAT_VERSION), "");
    ASSERT_EQ(dataJson.contains(CONST_EDIT_DATA), true);
    ASSERT_EQ(dataJson.at(CONST_EDIT_DATA), "");
    ASSERT_EQ(dataJson.contains(CONST_APP_ID) && dataJson[CONST_APP_ID].is_string(), true);
    ASSERT_EQ(dataJson.at(CONST_APP_ID), BUNDLE_NAME_TEST);
}

/**
 * @tc.name: CameraPathUtils_SaveEditDataCameraByString_test03
 * @tc.desc: editdata 为 empty, 不创建editdata_camera
 */
HWTEST_F(CameraPathUtilsTest, CameraPathUtils_SaveEditDataCameraByString_test03, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter CameraPathUtils_SaveEditDataCameraByString_test03");

    ASSERT_EQ(MediaFileUtils::IsFileExists("/storage/cloud/files/.editData/"), true);
    ASSERT_EQ(MediaFileUtils::IsFileExists("/storage/cloud/files/.editData/Photo/"), false);
    ASSERT_EQ(MediaFileUtils::IsFileExists(EDITDATA_CAMERA_PATH), false);

    // 执行
    CameraPathUtils::SaveEditDataCameraByString(SOURCE_PATH, "", BUNDLE_NAME_TEST);
    ASSERT_EQ(MediaFileUtils::IsFileExists(EDITDATA_CAMERA_PATH), false);
}

/**
 * @tc.name: CameraPathUtils_SaveEditDataCameraByString_test04
 * @tc.desc: 传入路径为 empty, 不创建editdata_camera
 */
HWTEST_F(CameraPathUtilsTest, CameraPathUtils_SaveEditDataCameraByString_test04, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter CameraPathUtils_SaveEditDataCameraByString_test04");

    ASSERT_EQ(MediaFileUtils::IsFileExists("/storage/cloud/files/.editData/"), true);
    ASSERT_EQ(MediaFileUtils::IsFileExists("/storage/cloud/files/.editData/Photo/"), false);
    ASSERT_EQ(MediaFileUtils::IsFileExists(EDITDATA_CAMERA_PATH), false);

    // 执行
    CameraPathUtils::SaveEditDataCameraByString("", EDITDATA_CAMERA_STRING_VALID, BUNDLE_NAME_TEST);
    ASSERT_EQ(MediaFileUtils::IsFileExists(EDITDATA_CAMERA_PATH), false);
}

/**
 * @tc.name: CameraPathUtils_SaveEditDataCameraByString_test05
 * @tc.desc: 传入路径异常, 不创建editdata_camera
 */
HWTEST_F(CameraPathUtilsTest, CameraPathUtils_SaveEditDataCameraByString_test05, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter CameraPathUtils_SaveEditDataCameraByString_test05");

    ASSERT_EQ(MediaFileUtils::IsFileExists("/storage/cloud/files/.editData/"), true);
    ASSERT_EQ(MediaFileUtils::IsFileExists("/storage/cloud/files/.editData/Photo/"), false);
    ASSERT_EQ(MediaFileUtils::IsFileExists(EDITDATA_CAMERA_PATH), false);

    // 执行
    CameraPathUtils::SaveEditDataCameraByString("/invalid/path", EDITDATA_CAMERA_STRING_VALID, BUNDLE_NAME_TEST);
    ASSERT_EQ(MediaFileUtils::IsFileExists(EDITDATA_CAMERA_PATH), false);
}

/**
 * @tc.name: CameraPathUtils_SaveEditDataCameraByString_test06
 * @tc.desc: 如果解析失败[无 compatible_format], 则不会创建editdata_camera文件, 内容为空
 */
HWTEST_F(CameraPathUtilsTest, CameraPathUtils_SaveEditDataCameraByString_test06, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter CameraPathUtils_SaveEditDataCameraByString_test06");

    ASSERT_EQ(MediaFileUtils::IsFileExists("/storage/cloud/files/.editData/"), true);
    ASSERT_EQ(MediaFileUtils::IsFileExists("/storage/cloud/files/.editData/Photo/"), false);
    ASSERT_EQ(MediaFileUtils::IsFileExists(EDITDATA_CAMERA_PATH), false);

    // 执行
    CameraPathUtils::SaveEditDataCameraByString(SOURCE_PATH, EDITDATA_CAMERA_STRING_WITHOUT_COMPATIBLE_FORMAT,
        BUNDLE_NAME_TEST);
    ASSERT_EQ(MediaFileUtils::IsFileExists(EDITDATA_CAMERA_PATH), false);

    // value 符合预期
    std::string editdata;
    int32_t ret = CameraPathUtils::ReadEditdataCameraFromFile(SOURCE_PATH, false, editdata);
    ASSERT_EQ(ret < 0, true);
    ASSERT_EQ(editdata.empty(), true);
}

/**
 * @tc.name: CameraPathUtils_SaveEditDataCameraByString_test07
 * @tc.desc: 如果解析失败[无 format_version], 则不会创建editdata_camera文件, 内容为空
 */
HWTEST_F(CameraPathUtilsTest, CameraPathUtils_SaveEditDataCameraByString_test07, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter CameraPathUtils_SaveEditDataCameraByString_test07");

    ASSERT_EQ(MediaFileUtils::IsFileExists("/storage/cloud/files/.editData/"), true);
    ASSERT_EQ(MediaFileUtils::IsFileExists("/storage/cloud/files/.editData/Photo/"), false);
    ASSERT_EQ(MediaFileUtils::IsFileExists(EDITDATA_CAMERA_PATH), false);

    // 执行
    CameraPathUtils::SaveEditDataCameraByString(SOURCE_PATH, EDITDATA_CAMERA_STRING_WITHOUT_FORMAT_VERSION,
        BUNDLE_NAME_TEST);
    ASSERT_EQ(MediaFileUtils::IsFileExists(EDITDATA_CAMERA_PATH), false);

    // value 符合预期
    std::string editdata;
    int32_t ret = CameraPathUtils::ReadEditdataCameraFromFile(SOURCE_PATH, false, editdata);
    ASSERT_EQ(ret < 0, true);
    ASSERT_EQ(editdata.empty(), true);
}

/**
 * @tc.name: CameraPathUtils_SaveEditDataCameraByString_test08
 * @tc.desc: 如果解析失败[无 edit_data], 则不会创建editdata_camera文件, 内容为空
 */
HWTEST_F(CameraPathUtilsTest, CameraPathUtils_SaveEditDataCameraByString_test08, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter CameraPathUtils_SaveEditDataCameraByString_test08");

    ASSERT_EQ(MediaFileUtils::IsFileExists("/storage/cloud/files/.editData/"), true);
    ASSERT_EQ(MediaFileUtils::IsFileExists("/storage/cloud/files/.editData/Photo/"), false);
    ASSERT_EQ(MediaFileUtils::IsFileExists(EDITDATA_CAMERA_PATH), false);

    // 执行
    CameraPathUtils::SaveEditDataCameraByString(SOURCE_PATH, EDITDATA_CAMERA_STRING_WITHOUT_EDIT_DATA,
        BUNDLE_NAME_TEST);
    ASSERT_EQ(MediaFileUtils::IsFileExists(EDITDATA_CAMERA_PATH), false);

    // value 符合预期
    std::string editdata;
    int32_t ret = CameraPathUtils::ReadEditdataCameraFromFile(SOURCE_PATH, false, editdata);
    ASSERT_EQ(ret < 0, true);
    ASSERT_EQ(editdata.empty(), true);
}

/**
 * @tc.name: CameraPathUtils_SaveEditDataCameraByStruct_test01
 * @tc.desc: [允许 struct 写入 editdata_camera]
 *           [1] 需要包含3个部分: compatible_format、format_version、edit_data
 *           [2] 补充写入 bundle_name
 */
HWTEST_F(CameraPathUtilsTest, CameraPathUtils_SaveEditDataCameraByStruct_test01, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter CameraPathUtils_SaveEditDataCameraByStruct_test01");

    ASSERT_EQ(MediaFileUtils::IsFileExists("/storage/cloud/files/.editData/"), true);
    ASSERT_EQ(MediaFileUtils::IsFileExists("/storage/cloud/files/.editData/Photo/"), false);
    ASSERT_EQ(MediaFileUtils::IsFileExists(EDITDATA_CAMERA_PATH), false);

    // 执行
    CameraPathUtils::SaveEditDataCameraByStruct(SOURCE_PATH, COMPATIBLE_FORMAT, FORMAT_VERSION, EDIT_DATA_FOR_TEST,
        BUNDLE_NAME_CAMERA);
    ASSERT_EQ(MediaFileUtils::IsFileExists(EDITDATA_CAMERA_PATH), true);

    // value 符合预期
    std::string editdata;
    int32_t ret = CameraPathUtils::ReadEditdataCameraFromFile(SOURCE_PATH, false, editdata);
    ASSERT_EQ(ret, E_OK);
    ASSERT_EQ(nlohmann::json::accept(editdata), true);

    nlohmann::json dataJson = nlohmann::json::parse(editdata);
    ASSERT_EQ(dataJson.contains(CONST_COMPATIBLE_FORMAT) && dataJson[CONST_COMPATIBLE_FORMAT].is_string(), true);
    ASSERT_EQ(dataJson.contains(CONST_FORMAT_VERSION) && dataJson[CONST_FORMAT_VERSION].is_string(), true);
    ASSERT_EQ(dataJson.contains(CONST_EDIT_DATA) && dataJson[CONST_EDIT_DATA].is_string(), true);
    ASSERT_EQ(dataJson.contains(CONST_APP_ID) && dataJson[CONST_APP_ID].is_string(), true);

    ASSERT_EQ(dataJson.at(CONST_COMPATIBLE_FORMAT), COMPATIBLE_FORMAT);
    ASSERT_EQ(dataJson.at(CONST_FORMAT_VERSION), FORMAT_VERSION);
    ASSERT_EQ(dataJson.at(CONST_APP_ID), BUNDLE_NAME_CAMERA);
}

/**
 * @tc.name: CameraPathUtils_SaveEditDataCameraByStruct_test02
 * @tc.desc: [允许 struct 写入 editdata_camera]
 *           [1] 以下3个部分均[不包含]: compatible_format、format_version、edit_data
 *           [2] app_id = bundle_name, compatible_format 也会补充写入 bundle_name
 */
HWTEST_F(CameraPathUtilsTest, CameraPathUtils_SaveEditDataCameraByStruct_test02, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter CameraPathUtils_SaveEditDataCameraByStruct_test02");

    ASSERT_EQ(MediaFileUtils::IsFileExists("/storage/cloud/files/.editData/"), true);
    ASSERT_EQ(MediaFileUtils::IsFileExists("/storage/cloud/files/.editData/Photo/"), false);
    ASSERT_EQ(MediaFileUtils::IsFileExists(EDITDATA_CAMERA_PATH), false);

    // 执行
    CameraPathUtils::SaveEditDataCameraByStruct(SOURCE_PATH, "", "", "", BUNDLE_NAME_CAMERA);
    ASSERT_EQ(MediaFileUtils::IsFileExists(EDITDATA_CAMERA_PATH), true);

    // value 符合预期
    std::string editdata;
    int32_t ret = CameraPathUtils::ReadEditdataCameraFromFile(SOURCE_PATH, false, editdata);
    ASSERT_EQ(ret, E_OK);
    ASSERT_EQ(nlohmann::json::accept(editdata), true);

    nlohmann::json dataJson = nlohmann::json::parse(editdata);
    ASSERT_EQ(dataJson.contains(CONST_COMPATIBLE_FORMAT), true);
    ASSERT_EQ(dataJson.at(CONST_COMPATIBLE_FORMAT), BUNDLE_NAME_CAMERA);
    ASSERT_EQ(dataJson.contains(CONST_FORMAT_VERSION), true);
    ASSERT_EQ(dataJson.at(CONST_FORMAT_VERSION), "");
    ASSERT_EQ(dataJson.contains(CONST_EDIT_DATA), true);
    ASSERT_EQ(dataJson.at(CONST_EDIT_DATA), "");
    ASSERT_EQ(dataJson.contains(CONST_APP_ID) && dataJson[CONST_APP_ID].is_string(), true);
    ASSERT_EQ(dataJson.at(CONST_APP_ID), BUNDLE_NAME_CAMERA);
}

/**
 * @tc.name: CameraPathUtils_SaveEditDataCameraByStruct_test03
 * @tc.desc: 传入路径异常, 不创建editdata_camera
 */
HWTEST_F(CameraPathUtilsTest, CameraPathUtils_SaveEditDataCameraByStruct_test03, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter CameraPathUtils_SaveEditDataCameraByStruct_test03");

    ASSERT_EQ(MediaFileUtils::IsFileExists("/storage/cloud/files/.editData/"), true);
    ASSERT_EQ(MediaFileUtils::IsFileExists("/storage/cloud/files/.editData/Photo/"), false);
    ASSERT_EQ(MediaFileUtils::IsFileExists(EDITDATA_CAMERA_PATH), false);

    // 执行
    CameraPathUtils::SaveEditDataCameraByStruct("", COMPATIBLE_FORMAT, FORMAT_VERSION, EDIT_DATA_FOR_TEST,
        BUNDLE_NAME_CAMERA);
    ASSERT_EQ(MediaFileUtils::IsFileExists(EDITDATA_CAMERA_PATH), false);
}

/**
 * @tc.name: CameraPathUtils_SaveEditDataCameraByStruct_test04
 * @tc.desc: 传入路径为 empty, 不创建editdata_camera
 */
HWTEST_F(CameraPathUtilsTest, CameraPathUtils_SaveEditDataCameraByStruct_test04, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter CameraPathUtils_SaveEditDataCameraByStruct_test04");

    ASSERT_EQ(MediaFileUtils::IsFileExists("/storage/cloud/files/.editData/"), true);
    ASSERT_EQ(MediaFileUtils::IsFileExists("/storage/cloud/files/.editData/Photo/"), false);
    ASSERT_EQ(MediaFileUtils::IsFileExists(EDITDATA_CAMERA_PATH), false);

    // 执行
    CameraPathUtils::SaveEditDataCameraByStruct("/invalid/path", COMPATIBLE_FORMAT, FORMAT_VERSION, EDIT_DATA_FOR_TEST,
        BUNDLE_NAME_CAMERA);
    ASSERT_EQ(MediaFileUtils::IsFileExists(EDITDATA_CAMERA_PATH), false);
}

/**
 * @tc.name: CameraPathUtils_SaveEditDataCameraByStruct_test05
 * @tc.desc: 如果解析失败[compatible_format = null], 则不会创建editdata_camera文件, 内容为空
 */
HWTEST_F(CameraPathUtilsTest, CameraPathUtils_SaveEditDataCameraByStruct_test05, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter CameraPathUtils_SaveEditDataCameraByStruct_test05");

    ASSERT_EQ(MediaFileUtils::IsFileExists("/storage/cloud/files/.editData/"), true);
    ASSERT_EQ(MediaFileUtils::IsFileExists("/storage/cloud/files/.editData/Photo/"), false);
    ASSERT_EQ(MediaFileUtils::IsFileExists(EDITDATA_CAMERA_PATH), false);

    // 执行
    CameraPathUtils::SaveEditDataCameraByStruct(SOURCE_PATH, "", FORMAT_VERSION, EDIT_DATA_FOR_TEST,
        BUNDLE_NAME_CAMERA);
    ASSERT_EQ(MediaFileUtils::IsFileExists(EDITDATA_CAMERA_PATH), false);

    // value 符合预期
    std::string editdata;
    int32_t ret = CameraPathUtils::ReadEditdataCameraFromFile(SOURCE_PATH, false, editdata);
    ASSERT_EQ(ret < 0, true);
    ASSERT_EQ(editdata.empty(), true);
}

/**
 * @tc.name: CameraPathUtils_SaveEditDataCameraByStruct_test06
 * @tc.desc: 如果解析失败[format_version = null], 则不会创建editdata_camera文件, 内容为空
 */
HWTEST_F(CameraPathUtilsTest, CameraPathUtils_SaveEditDataCameraByStruct_test06, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter CameraPathUtils_SaveEditDataCameraByStruct_test06");

    ASSERT_EQ(MediaFileUtils::IsFileExists("/storage/cloud/files/.editData/"), true);
    ASSERT_EQ(MediaFileUtils::IsFileExists("/storage/cloud/files/.editData/Photo/"), false);
    ASSERT_EQ(MediaFileUtils::IsFileExists(EDITDATA_CAMERA_PATH), false);

    // 执行
    CameraPathUtils::SaveEditDataCameraByStruct(SOURCE_PATH, COMPATIBLE_FORMAT, "", EDIT_DATA_FOR_TEST,
        BUNDLE_NAME_CAMERA);
    ASSERT_EQ(MediaFileUtils::IsFileExists(EDITDATA_CAMERA_PATH), false);

    // value 符合预期
    std::string editdata;
    int32_t ret = CameraPathUtils::ReadEditdataCameraFromFile(SOURCE_PATH, false, editdata);
    ASSERT_EQ(ret < 0, true);
    ASSERT_EQ(editdata.empty(), true);
}

/**
 * @tc.name: CameraPathUtils_SaveEditDataCameraByStruct_test07
 * @tc.desc: 如果解析失败[edit_data = null], 则不会创建editdata_camera文件, 内容为空
 */
HWTEST_F(CameraPathUtilsTest, CameraPathUtils_SaveEditDataCameraByStruct_test07, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter CameraPathUtils_SaveEditDataCameraByStruct_test07");

    ASSERT_EQ(MediaFileUtils::IsFileExists("/storage/cloud/files/.editData/"), true);
    ASSERT_EQ(MediaFileUtils::IsFileExists("/storage/cloud/files/.editData/Photo/"), false);
    ASSERT_EQ(MediaFileUtils::IsFileExists(EDITDATA_CAMERA_PATH), false);

    // 执行
    CameraPathUtils::SaveEditDataCameraByStruct(SOURCE_PATH, COMPATIBLE_FORMAT, FORMAT_VERSION, "", BUNDLE_NAME_CAMERA);
    ASSERT_EQ(MediaFileUtils::IsFileExists(EDITDATA_CAMERA_PATH), false);

    // value 符合预期
    std::string editdata;
    int32_t ret = CameraPathUtils::ReadEditdataCameraFromFile(SOURCE_PATH, false, editdata);
    ASSERT_EQ(ret < 0, true);
    ASSERT_EQ(editdata.empty(), true);
}
} // namespace Media
} // namespace OHOS