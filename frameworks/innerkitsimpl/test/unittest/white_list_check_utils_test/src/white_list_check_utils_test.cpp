/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "WhiteListCheckUtilsTest"

#include "permission_whitelist_utils.h"
#include "white_list_check_utils_test.h"

#include <sstream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <thread>
#include <sys/stat.h>
#include <algorithm>

#include "media_log.h"
#include "medialibrary_errno.h"
#include "system_ability_definition.h"
#include "bundle_mgr_proxy.h"
#include "iservice_registry.h"
#include "bundle_constants.h"

using namespace std;
using namespace nlohmann;
using namespace testing::ext;
using std::ifstream;
using json = nlohmann::json;
namespace fs = std::filesystem;

namespace OHOS {
namespace Media {
const std::string MEDIA_KIT_WHITE_LIST_JSON_LOCAL_PATH_TEST =
    "/system/etc/com.ohos.medialibrary.medialibrarydata/medialibrary_kit_whitelist/"
    "medialibrary_kit_whitelist.json";
const string DUE_INSTALL_DIR_TEST =
    "/data/service/el1/public/update/param_service/install/system/etc/"
    "com.ohos.medialibrary.medialibrarydata/medialibrary_kit_whitelist/medialibrary_kit_whitelist.json";

void WhiteListCheckUtilsTest::SetUpTestCase() {}

void WhiteListCheckUtilsTest::TearDownTestCase() {}

void WhiteListCheckUtilsTest::SetUp() {}

void WhiteListCheckUtilsTest::TearDown() {}

std::string GetBackupFilePath(const std::string &originalPath)
{
    return originalPath + ".bak";
}

bool BackupFileIfExists(const std::string &originalPath)
{
    CHECK_AND_RETURN_RET_LOG(fs::exists(originalPath), false,
        "Backup file not found: %{public}s", originalPath.c_str());
    CHECK_AND_RETURN_RET_LOG(fs::is_regular_file(originalPath), false, "Backup file not is regular file");
    std::string backupPath = GetBackupFilePath(originalPath);
    if (fs::exists(backupPath)) {
        fs::remove(backupPath);
        MEDIA_INFO_LOG("Removed old backup file: %{public}s", backupPath.c_str());
    }
    fs::rename(originalPath, backupPath);
    MEDIA_INFO_LOG("File backed up to: %{public}s", backupPath.c_str());
    return true;
}

bool RestoreFileFromBackup(const std::string &originalPath)
{
    std::string backupPath = GetBackupFilePath(originalPath);
    CHECK_AND_RETURN_RET_LOG(fs::exists(backupPath), false, "Backup file not found: %{public}s", backupPath.c_str());
    CHECK_AND_RETURN_RET_LOG(fs::is_regular_file(backupPath), false, "Backup file not is regular file");
    if (fs::exists(originalPath)) {
        fs::remove(originalPath);
        MEDIA_INFO_LOG("Removed existing file: %{public}s", originalPath.c_str());
    }
    fs::rename(backupPath, originalPath);
    MEDIA_INFO_LOG("File restored from backup: %{public}s", originalPath.c_str());
    return true;
}

bool IsFileOpenable(const std::string &filename)
{
    std::ifstream jfile;
    jfile.open(filename);
    CHECK_AND_RETURN_RET_LOG(jfile.is_open(), false, "failed to open jfile");
    jfile.close();
    return true;
}

void AddItems(json &targetList, const std::vector<json> &newItems)
{
    for (const auto &item : newItems) {
        targetList.push_back(item);
    }
}

bool UpdateHeifChecklist(const std::string &filePath, const std::vector<json> &newWhitelist)
{
    std::ifstream inputFile(filePath);
    CHECK_AND_RETURN_RET_LOG(inputFile.is_open(), false,
        "Failed to open inputFile: %{public}s", filePath.c_str());
    json listJson;
    inputFile >> listJson;
    inputFile.close();
    CHECK_AND_RETURN_RET_LOG((listJson.contains("applications") && listJson["applications"].is_array()), false,
        "Invalid or missing 'applications' in json");
    auto &whiteList = listJson["applications"];
    AddItems(whiteList, newWhitelist);

    std::ofstream outputFile(filePath);
    CHECK_AND_RETURN_RET_LOG(outputFile.is_open(), false,
        "Failed to open outputFile: %{public}s", filePath.c_str());
    const int jsonIndent = 4;
    outputFile << listJson.dump(jsonIndent);
    outputFile.close();
    return true;
}

bool CreateTestWhiteListJson(const std::string &filePath, const std::string &version,
    const std::vector<json> &applications)
{
    std::string dirPath = filePath.substr(0, filePath.find_last_of('/'));
    std::filesystem::create_directories(dirPath);
    json whiteListJson;
    whiteListJson["version"] = version;
    whiteListJson["applications"] = applications;
    std::ofstream outputFile(filePath);
    CHECK_AND_RETURN_RET_LOG(outputFile.is_open(), false,
        "Failed to create JSON file: %{public}s", filePath.c_str());
    const int jsonIndent = 4;
    outputFile << whiteListJson.dump(jsonIndent);
    outputFile.close();
    MEDIA_INFO_LOG("Created test JSON file: %{public}s", filePath.c_str());
    return true;
}

HWTEST_F(WhiteListCheckUtilsTest, InitWhiteList_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd InitWhiteList_test_001");
    auto ret = PermissionWhitelistUtils::InitWhiteList();
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end tdd InitWhiteList_test_001");
}

HWTEST_F(WhiteListCheckUtilsTest, InitWhiteList_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd InitWhiteList_test_002");
    if (IsFileOpenable(DUE_INSTALL_DIR_TEST)) {
        auto ret = BackupFileIfExists(MEDIA_KIT_WHITE_LIST_JSON_LOCAL_PATH_TEST);
        EXPECT_EQ(ret, true);
    }
    auto ret1 = PermissionWhitelistUtils::InitWhiteList();
    EXPECT_EQ(ret1, E_OK);
    if (IsFileOpenable(DUE_INSTALL_DIR_TEST)) {
        auto ret2 = RestoreFileFromBackup(MEDIA_KIT_WHITE_LIST_JSON_LOCAL_PATH_TEST);
        EXPECT_EQ(ret2, true);
    }
    MEDIA_INFO_LOG("end tdd InitWhiteList_test_002");
}

HWTEST_F(WhiteListCheckUtilsTest, InitWhiteList_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd InitWhiteList_test_003");
    if (IsFileOpenable(DUE_INSTALL_DIR_TEST)) {
        auto ret = BackupFileIfExists(DUE_INSTALL_DIR_TEST);
        EXPECT_EQ(ret, true);
    }
    std::vector<json> newWhitelist = {
        {{"appIdentifier", "1234567891234567891"}, {"allowedApiVersion", 0}},
        {{"appIdentifier", "1234567891234567891"}, {"allowedApiVersion", 25}},
        {{"appIdentifier", "1234567891234567891"}, {"allowedApiVersion", 0}}
    };
    auto ret1 = UpdateHeifChecklist(MEDIA_KIT_WHITE_LIST_JSON_LOCAL_PATH_TEST, newWhitelist);
    EXPECT_EQ(ret1, true);
    auto ret2 = PermissionWhitelistUtils::InitWhiteList();
    EXPECT_EQ(ret2, E_OK);
    if (IsFileOpenable(DUE_INSTALL_DIR_TEST)) {
        auto ret3 = RestoreFileFromBackup(DUE_INSTALL_DIR_TEST);
        EXPECT_EQ(ret3, true);
    }
    MEDIA_INFO_LOG("end tdd InitWhiteList_test_003");
}

HWTEST_F(WhiteListCheckUtilsTest, CheckWhiteList_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd CheckWhiteList_test_001");
    auto ret = PermissionWhitelistUtils::CheckWhiteList();
    EXPECT_EQ(ret, E_SUCCESS);
    MEDIA_INFO_LOG("end tdd CheckWhiteList_test_001");
}

HWTEST_F(WhiteListCheckUtilsTest, CheckWhiteList_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd CheckWhiteList_test_002");
    std::vector<json> testApplications = {
        {{"appIdentifier", "1234567891234567891"}, {"allowedApiVersion", 0}},
        {{"appIdentifier", "1234567891234567891"}, {"allowedApiVersion", 25}},
        {{"appIdentifier", "1234567891234567891"}, {"allowedApiVersion", 0}}
    };
    auto createlocalJsonRet = CreateTestWhiteListJson(MEDIA_KIT_WHITE_LIST_JSON_LOCAL_PATH_TEST, "invalidVersion",
        testApplications);
    EXPECT_EQ(createlocalJsonRet, true);
    auto createDueJsonRet = CreateTestWhiteListJson(DUE_INSTALL_DIR_TEST, "invalidVersion", testApplications);
    EXPECT_EQ(createDueJsonRet, true);
    auto ret = PermissionWhitelistUtils::InitWhiteList();
    EXPECT_EQ(ret, E_OK);
    createlocalJsonRet = CreateTestWhiteListJson(MEDIA_KIT_WHITE_LIST_JSON_LOCAL_PATH_TEST, "1.0", testApplications);
    EXPECT_EQ(createlocalJsonRet, true);
    ret = PermissionWhitelistUtils::InitWhiteList();
    EXPECT_EQ(ret, E_OK);
    createDueJsonRet = CreateTestWhiteListJson(DUE_INSTALL_DIR_TEST, "1.2", testApplications);
    EXPECT_EQ(createDueJsonRet, true);
    createlocalJsonRet = CreateTestWhiteListJson(MEDIA_KIT_WHITE_LIST_JSON_LOCAL_PATH_TEST, "invalidVersion",
        testApplications);
    EXPECT_EQ(createlocalJsonRet, true);
    ret = PermissionWhitelistUtils::InitWhiteList();
    EXPECT_EQ(ret, E_OK);
    createlocalJsonRet = CreateTestWhiteListJson(MEDIA_KIT_WHITE_LIST_JSON_LOCAL_PATH_TEST, "1.0", testApplications);
    EXPECT_EQ(createlocalJsonRet, true);
    ret = PermissionWhitelistUtils::InitWhiteList();
    EXPECT_EQ(ret, E_OK);
    createlocalJsonRet = CreateTestWhiteListJson(MEDIA_KIT_WHITE_LIST_JSON_LOCAL_PATH_TEST, "1.2", testApplications);
    EXPECT_EQ(createlocalJsonRet, true);
    createDueJsonRet = CreateTestWhiteListJson(DUE_INSTALL_DIR_TEST, "1.0", testApplications);
    EXPECT_EQ(createDueJsonRet, true);
    ret = PermissionWhitelistUtils::InitWhiteList();
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end tdd CheckWhiteList_test_002");
}
}
}