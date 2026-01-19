/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "HeifTranscodingCheckUtilsTest"

#include "heif_transcoding_check_utils.h"
#include "heif_transcoding_check_utils_test.h"

#include <sstream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <thread>
#include <sys/stat.h>
#include <algorithm>

#include "heif_bundle_info_cache.h"
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
const std::string HEIF_TRANSCODING_CHECK_LIST_JSON_LOCAL_NAME_PATH_TEST =
    "/system/etc/com.ohos.medialibrary.medialibrarydata/heif_transcoding/"
    "heif_transcoding_checklist.json";
const std::string DUE_INSTALL_DIR_TEST =
    "/data/service/el1/public/update/param_service/install/system/etc/"
    "com.ohos.medialibrary.medialibrarydata/heif_transcoding/"
    "heif_transcoding_checklist.json";

void HeifTranscodingCheckUtilsTest::SetUpTestCase() {}

void HeifTranscodingCheckUtilsTest::TearDownTestCase() {}

void HeifTranscodingCheckUtilsTest::SetUp() {}

void HeifTranscodingCheckUtilsTest::TearDown() {}

enum OperationType {
    ADD_FIELD = 1,
    REMOVE_FIELD = 2,
    REMOVE_LISTSTRATEGY = 3,
    ADD_LISTSTRATEGY = 4
};

bool ProcessJsonOperation(json& listJson, const std::string& fieldName, int opType)
{
    switch (opType) {
        case ADD_FIELD:
            if (!listJson.contains(fieldName)) {
                listJson[fieldName] = json::array();
                MEDIA_INFO_LOG("Field %{public}s added as empty array", fieldName.c_str());
                return true;
            } else {
                if (listJson[fieldName].is_array()) {
                    MEDIA_INFO_LOG("Field %{public}s already exists and is an array", fieldName.c_str());
                }
            }
            break;
        case REMOVE_FIELD:
            if (listJson.contains(fieldName) && listJson[fieldName].is_array()) {
                listJson.erase(fieldName);
                MEDIA_INFO_LOG("Array field %{public}s removed", fieldName.c_str());
                return true;
            } else {
                MEDIA_INFO_LOG("Field %{public}s does not exist or is not an array, skip remove", fieldName.c_str());
            }
            break;
        case REMOVE_LISTSTRATEGY:
            if (listJson.contains("listStrategy") && listJson["listStrategy"].is_string()) {
                listJson.erase("listStrategy");
                MEDIA_INFO_LOG("string field %{public}s removed", fieldName.c_str());
                return true;
            } else {
                MEDIA_INFO_LOG("Field %{public}s does not exist or is not a string, skip remove", fieldName.c_str());
                }
            break;
        case ADD_LISTSTRATEGY:
            if (!listJson.contains("listStrategy")) {
                listJson["listStrategy"] = "whiteList";
                MEDIA_INFO_LOG("Field %{public}s added as string", fieldName.c_str());
                return true;
            } else {
                if (listJson[fieldName].is_string()) {
                MEDIA_INFO_LOG("Field %{public}s already exists and is an string", fieldName.c_str());
                } else {
                    MEDIA_INFO_LOG("Field %{public}s already exists and is not a string", fieldName.c_str());
                }
            }
            break;
        default:
            MEDIA_ERR_LOG("Unknown opType: %d", opType);
    }
    return false;
}

bool JsonFieldOperation(const std::string& filePath, const std::string& fieldName, int opType)
{
    std::ifstream input(filePath);
    if (!input.is_open()) {
        MEDIA_ERR_LOG("File not found: %{public}s", filePath.c_str());
        return false;
    }
    json listJson;
    input >> listJson;
    input.close();
    bool operationSuccess = false;
    operationSuccess = ProcessJsonOperation(listJson, fieldName, opType);
    if (operationSuccess) {
        std::ofstream output(filePath);
        if (!output.is_open()) {
            MEDIA_ERR_LOG("Failed to open file for writing: %{public}s", filePath.c_str());
            return false;
        }
        const int jsonIndent = 4;
        output << listJson.dump(jsonIndent);
        if (!output.good()) {
            MEDIA_ERR_LOG("Failed to write data to file: %{public}s", filePath.c_str());
            output.close();
            return false;
        }
        output.close();
        MEDIA_INFO_LOG("File updated successfully: %{public}s", filePath.c_str());
    }
    return operationSuccess;
}

void AddUniqueItems(json& targetList, const std::vector<std::string>& newItems,
    const std::string& listName)
{
    for (const auto& item : newItems) {
        if (!std::any_of(targetList.begin(), targetList.end(),
            [&](const json& existing) { return existing.get<std::string>() == item; })) {
            targetList.push_back(item);
            MEDIA_INFO_LOG("Added to %{public}s: %{public}s", listName.c_str(), item.c_str());
        }
    }
}

bool UpdateHeifChecklist(const std::string& filePath, const std::vector<std::string>& newWhitelist,
    const std::vector<std::string>& newDenyList, const std::string& newStrategy)
{
    std::ifstream inputFile(filePath);
    if (!inputFile.is_open()) {
        return false;
    }
    json listJson;
    inputFile >> listJson;
    inputFile.close();
    if (!listJson.contains("whiteList") || !listJson["whiteList"].is_array() ||
        !listJson.contains("denyList") || !listJson["denyList"].is_array()) {
        return false;
    }
    json& whiteList = listJson["whiteList"];
    json& denyList = listJson["denyList"];

    AddUniqueItems(whiteList, newWhitelist, "whiteList");
    AddUniqueItems(denyList, newDenyList, "denyList");

    listJson["listStrategy"] = newStrategy;
    std::ofstream outputFile(filePath);
    if (!outputFile.is_open()) {
        return false;
    }
    const int jsonIndent = 4;
    outputFile << listJson.dump(jsonIndent);
    outputFile.close();
    return true;
}

std::string GetBackupFilePath(const std::string& originalPath)
{
    return originalPath + ".bak";
}

bool BackupFileIfExists(const std::string& originalPath)
{
    if (fs::exists(originalPath) && fs::is_regular_file(originalPath)) {
        std::string backupPath = GetBackupFilePath(originalPath);
        if (fs::exists(backupPath)) {
            fs::remove(backupPath);
            MEDIA_INFO_LOG("Removed old backup file: %{public}s", backupPath.c_str());
        }
        fs::rename(originalPath, backupPath);
        MEDIA_INFO_LOG("File backed up to: %{public}s", backupPath.c_str());
    }
    return true;
}

bool RestoreFileFromBackup(const std::string& originalPath)
{
    std::string backupPath = GetBackupFilePath(originalPath);
    if (!fs::exists(backupPath) || !fs::is_regular_file(backupPath)) {
        MEDIA_ERR_LOG("Backup file not found: %{public}s", backupPath.c_str());
        return false;
    }
    if (fs::exists(originalPath)) {
        fs::remove(originalPath);
        MEDIA_INFO_LOG("Removed existing file: %{public}s", originalPath.c_str());
    }
    fs::rename(backupPath, originalPath);
    MEDIA_INFO_LOG("File restored from backup: %{public}s", originalPath.c_str());
    return true;
}

bool IsFileOpenable(const std::string& filename)
{
    std::ifstream jfile;
    jfile.open(filename);
    if (!jfile.is_open()) {
        return false;
    }
    jfile.close();
    return true;
}

HWTEST_F(HeifTranscodingCheckUtilsTest, InitCheckList_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd InitCheckList_test_001");
    if (IsFileOpenable(DUE_INSTALL_DIR_TEST)) {
        auto ret = BackupFileIfExists(DUE_INSTALL_DIR_TEST);
        EXPECT_EQ(ret, true);
    }
    BackupFileIfExists(HEIF_TRANSCODING_CHECK_LIST_JSON_LOCAL_NAME_PATH_TEST);
    auto ret1 = HeifTranscodingCheckUtils::InitCheckList();
    if (IsFileOpenable(DUE_INSTALL_DIR_TEST)) {
        auto ret2 = RestoreFileFromBackup(DUE_INSTALL_DIR_TEST);
        EXPECT_EQ(ret2, true);
    }
    RestoreFileFromBackup(HEIF_TRANSCODING_CHECK_LIST_JSON_LOCAL_NAME_PATH_TEST);
    EXPECT_EQ(ret1, E_FAIL);
    MEDIA_INFO_LOG("end tdd InitCheckList_test_001");
}

HWTEST_F(HeifTranscodingCheckUtilsTest, InitCheckList_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd InitCheckList_test_002");
    if (IsFileOpenable(DUE_INSTALL_DIR_TEST)) {
        auto ret = BackupFileIfExists(DUE_INSTALL_DIR_TEST);
        EXPECT_EQ(ret, true);
    }
    auto ret1 = HeifTranscodingCheckUtils::InitCheckList();
    if (IsFileOpenable(DUE_INSTALL_DIR_TEST)) {
        auto ret2 = RestoreFileFromBackup(DUE_INSTALL_DIR_TEST);
        EXPECT_EQ(ret2, true);
    }
    HeifTranscodingCheckUtils::UnsubscribeCotaUpdatedEvent();
    EXPECT_EQ(ret1, E_OK);
    MEDIA_INFO_LOG("end tdd InitCheckList_test_002");
}

HWTEST_F(HeifTranscodingCheckUtilsTest, InitCheckList_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd InitCheckList_test_003");
    std::string filePath;
    if (IsFileOpenable(DUE_INSTALL_DIR_TEST)) {
        filePath = DUE_INSTALL_DIR_TEST;
    } else {
        filePath = HEIF_TRANSCODING_CHECK_LIST_JSON_LOCAL_NAME_PATH_TEST;
    }
    std::string fieldName = "listStrategy";
    auto ret = JsonFieldOperation(filePath, fieldName, REMOVE_LISTSTRATEGY);
    EXPECT_EQ(ret, true);
    auto ret1 = HeifTranscodingCheckUtils::InitCheckList();
    auto ret2 = JsonFieldOperation(filePath, fieldName, ADD_LISTSTRATEGY);
    EXPECT_EQ(ret2, true);
    EXPECT_EQ(ret1, E_FAIL);
    MEDIA_INFO_LOG("end tdd InitCheckList_test_003");
}

HWTEST_F(HeifTranscodingCheckUtilsTest, InitCheckList_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd InitCheckList_test_004");
    std::string filePath;
    if (IsFileOpenable(DUE_INSTALL_DIR_TEST)) {
        filePath = DUE_INSTALL_DIR_TEST;
    } else {
        filePath = HEIF_TRANSCODING_CHECK_LIST_JSON_LOCAL_NAME_PATH_TEST;
    }
    std::vector<std::string> newWhitelist = {
        "xixi_test_001@1.0.0",
        "xixi_test_001@2.0.0",
        "xixi_test_001@1.9.9",
        "@1.0.0",
        "xixi_test_002@",
        "xixi_test_003",
    };
    std::vector<std::string> newDenyList = {};
    std::string newStrategy = "whiteList";
    auto ret = UpdateHeifChecklist(filePath, newWhitelist, newDenyList, newStrategy);
    EXPECT_EQ(ret, true);
    auto ret1 = HeifTranscodingCheckUtils::InitCheckList();
    HeifTranscodingCheckUtils::UnsubscribeCotaUpdatedEvent();
    EXPECT_EQ(ret1, E_OK);
    MEDIA_INFO_LOG("end tdd InitCheckList_test_004");
}

HWTEST_F(HeifTranscodingCheckUtilsTest, InitCheckList_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd InitCheckList_test_005");
    std::string filePath;
    if (IsFileOpenable(DUE_INSTALL_DIR_TEST)) {
        filePath = DUE_INSTALL_DIR_TEST;
    } else {
        filePath = HEIF_TRANSCODING_CHECK_LIST_JSON_LOCAL_NAME_PATH_TEST;
    }
    std::string fieldName = "whiteList";
    auto ret = JsonFieldOperation(filePath, fieldName, REMOVE_FIELD);
    EXPECT_EQ(ret, true);
    auto ret1 = HeifTranscodingCheckUtils::InitCheckList();
    auto ret2 = JsonFieldOperation(filePath, fieldName, ADD_FIELD);
    EXPECT_EQ(ret2, true);
    EXPECT_EQ(ret1, E_FAIL);
    MEDIA_INFO_LOG("end tdd InitCheckList_test_005");
}

HWTEST_F(HeifTranscodingCheckUtilsTest, InitCheckList_test_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd InitCheckList_test_006");
    std::string filePath;
    if (IsFileOpenable(DUE_INSTALL_DIR_TEST)) {
        filePath = DUE_INSTALL_DIR_TEST;
    } else {
        filePath = HEIF_TRANSCODING_CHECK_LIST_JSON_LOCAL_NAME_PATH_TEST;
    }
    std::string fieldName = "denyList";
    std::vector<std::string> newWhitelist = {};
    std::vector<std::string> newDenyList = {};
    std::string newStrategy = "denyList";
    auto ret = UpdateHeifChecklist(filePath, newWhitelist, newDenyList, newStrategy);
    EXPECT_EQ(ret, true);
    auto ret1 = JsonFieldOperation(filePath, fieldName, REMOVE_FIELD);
    EXPECT_EQ(ret1, true);
    auto ret2 = HeifTranscodingCheckUtils::InitCheckList();
    auto ret3 = JsonFieldOperation(filePath, fieldName, ADD_FIELD);
    EXPECT_EQ(ret3, true);
    EXPECT_EQ(ret2, E_FAIL);
    MEDIA_INFO_LOG("end tdd InitCheckList_test_006");
}

HWTEST_F(HeifTranscodingCheckUtilsTest, InitCheckList_test_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd InitCheckList_test_007");
    std::string filePath;
    if (IsFileOpenable(DUE_INSTALL_DIR_TEST)) {
        filePath = DUE_INSTALL_DIR_TEST;
    } else {
        filePath = HEIF_TRANSCODING_CHECK_LIST_JSON_LOCAL_NAME_PATH_TEST;
    }
    std::vector<std::string> newWhitelist = {};
    std::vector<std::string> newDenyList = {
        "haha_test_002@",
    };
    std::string newStrategy = "denyList";
    auto ret = UpdateHeifChecklist(filePath, newWhitelist, newDenyList, newStrategy);
    EXPECT_EQ(ret, true);
    auto ret1 = HeifTranscodingCheckUtils::InitCheckList();
    HeifTranscodingCheckUtils::UnsubscribeCotaUpdatedEvent();
    EXPECT_EQ(ret1, E_OK);
    MEDIA_INFO_LOG("end tdd InitCheckList_test_007");
}

HWTEST_F(HeifTranscodingCheckUtilsTest, CanSupportedCompatibleDuplicate_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd CanSupportedCompatibleDuplicate_test_001");
    std::string filePath;
    if (IsFileOpenable(DUE_INSTALL_DIR_TEST)) {
        filePath = DUE_INSTALL_DIR_TEST;
    } else {
        filePath = HEIF_TRANSCODING_CHECK_LIST_JSON_LOCAL_NAME_PATH_TEST;
    }
    std::vector<std::string> newWhitelist = {
        "xixi_test_007@1.0.0",
    };
    std::vector<std::string> newDenyList = {};
    std::string newStrategy = "whiteList";
    auto ret = UpdateHeifChecklist(filePath, newWhitelist, newDenyList, newStrategy);
    EXPECT_EQ(ret, true);
    auto ret1 = HeifTranscodingCheckUtils::InitCheckList();
    HeifTranscodingCheckUtils::UnsubscribeCotaUpdatedEvent();
    EXPECT_EQ(ret1, E_OK);
    std::string bundleName = "xixi_test_007";
    auto ret3 = HeifTranscodingCheckUtils::CanSupportedCompatibleDuplicate(bundleName);
    EXPECT_EQ(ret3, false);
    MEDIA_INFO_LOG("end tdd CanSupportedCompatibleDuplicate_test_001");
}

HWTEST_F(HeifTranscodingCheckUtilsTest, CanSupportedCompatibleDuplicate_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd CanSupportedCompatibleDuplicate_test_002");
    std::string filePath;
    if (IsFileOpenable(DUE_INSTALL_DIR_TEST)) {
        filePath = DUE_INSTALL_DIR_TEST;
    } else {
        filePath = HEIF_TRANSCODING_CHECK_LIST_JSON_LOCAL_NAME_PATH_TEST;
    }
    std::vector<std::string> newWhitelist = {};
    std::vector<std::string> newDenyList = {
        "haha_test_003",
    };
    std::string newStrategy = "denyList";
    auto ret = UpdateHeifChecklist(filePath, newWhitelist, newDenyList, newStrategy);
    EXPECT_EQ(ret, true);
    auto ret1 = HeifTranscodingCheckUtils::InitCheckList();
    EXPECT_EQ(ret, true);
    EXPECT_EQ(ret1, E_OK);
    std::string bundleName = "haha_test_003";
    auto ret2 = HeifTranscodingCheckUtils::CanSupportedCompatibleDuplicate(bundleName);
    HeifTranscodingCheckUtils::UnsubscribeCotaUpdatedEvent();
    EXPECT_EQ(ret2, true);
    MEDIA_INFO_LOG("end tdd CanSupportedCompatibleDuplicate_test_002");
}

HWTEST_F(HeifTranscodingCheckUtilsTest, CanSupportedCompatibleDuplicate_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd CanSupportedCompatibleDuplicate_test_003");
    std::string filePath;
    if (IsFileOpenable(DUE_INSTALL_DIR_TEST)) {
        filePath = DUE_INSTALL_DIR_TEST;
    } else {
        filePath = HEIF_TRANSCODING_CHECK_LIST_JSON_LOCAL_NAME_PATH_TEST;
    }
    std::vector<std::string> newWhitelist = {};
    std::vector<std::string> newDenyList = {
        "haha_test_004",
    };
    std::string newStrategy = "denyList";
    auto ret = UpdateHeifChecklist(filePath, newWhitelist, newDenyList, newStrategy);
    EXPECT_EQ(ret, true);
    auto ret1 = HeifTranscodingCheckUtils::InitCheckList();
    EXPECT_EQ(ret1, E_OK);
    std::string bundleName = "haha_test_005";
    auto ret2 = HeifTranscodingCheckUtils::CanSupportedCompatibleDuplicate(bundleName);
    HeifTranscodingCheckUtils::UnsubscribeCotaUpdatedEvent();
    EXPECT_EQ(ret2, false);
    MEDIA_INFO_LOG("end tdd CanSupportedCompatibleDuplicate_test_003");
}

HWTEST_F(HeifTranscodingCheckUtilsTest, CanSupportedCompatibleDuplicate_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd CanSupportedCompatibleDuplicate_test_004");
    std::string filePath;
    if (IsFileOpenable(DUE_INSTALL_DIR_TEST)) {
        filePath = DUE_INSTALL_DIR_TEST;
    } else {
        filePath = HEIF_TRANSCODING_CHECK_LIST_JSON_LOCAL_NAME_PATH_TEST;
    }
    std::vector<std::string> newWhitelist = {
        "com.ohos.medialibrary.medialibrarydata@1.0.0.1",
    };
    std::vector<std::string> newDenyList = {};
    std::string newStrategy = "whiteList";
    auto ret = UpdateHeifChecklist(filePath, newWhitelist, newDenyList, newStrategy);
    EXPECT_EQ(ret, true);
    auto ret1 = HeifTranscodingCheckUtils::InitCheckList();
    HeifTranscodingCheckUtils::UnsubscribeCotaUpdatedEvent();
    EXPECT_EQ(ret1, E_OK);
    std::string bundleName = "com.ohos.medialibrary.medialibrarydata";
    auto ret3 = HeifTranscodingCheckUtils::CanSupportedCompatibleDuplicate(bundleName);
    EXPECT_EQ(ret3, false);
    MEDIA_INFO_LOG("end tdd CanSupportedCompatibleDuplicate_test_004");
}


HWTEST_F(HeifTranscodingCheckUtilsTest, CanSupportedCompatibleDuplicate_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd CanSupportedCompatibleDuplicate_test_005");
    std::string filePath;
    if (IsFileOpenable(DUE_INSTALL_DIR_TEST)) {
        filePath = DUE_INSTALL_DIR_TEST;
    } else {
        filePath = HEIF_TRANSCODING_CHECK_LIST_JSON_LOCAL_NAME_PATH_TEST;
    }
    std::vector<std::string> newWhitelist = {
        "com.ohos.medialibrary.medialibrarydata@4.0.10.202",
    };
    std::vector<std::string> newDenyList = {};
    std::string newStrategy = "whiteList";
    auto ret = UpdateHeifChecklist(filePath, newWhitelist, newDenyList, newStrategy);
    EXPECT_EQ(ret, true);
    auto ret1 = HeifTranscodingCheckUtils::InitCheckList();
    HeifTranscodingCheckUtils::UnsubscribeCotaUpdatedEvent();
    EXPECT_EQ(ret1, E_OK);
    std::string bundleName = "com.ohos.medialibrary.medialibrarydata";
    auto ret3 = HeifTranscodingCheckUtils::CanSupportedCompatibleDuplicate(bundleName);
    EXPECT_EQ(ret3, false);
    MEDIA_INFO_LOG("end tdd CanSupportedCompatibleDuplicate_test_005");
}

HWTEST_F(HeifTranscodingCheckUtilsTest, CanSupportedCompatibleDuplicate_test_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd CanSupportedCompatibleDuplicate_test_006");
    std::string filePath;
    if (IsFileOpenable(DUE_INSTALL_DIR_TEST)) {
        filePath = DUE_INSTALL_DIR_TEST;
    } else {
        filePath = HEIF_TRANSCODING_CHECK_LIST_JSON_LOCAL_NAME_PATH_TEST;
    }
    std::vector<std::string> newWhitelist = {
        "com.ohos.medialibrary.medialibrarydata@5.0.10.202",
    };
    std::vector<std::string> newDenyList = {};
    std::string newStrategy = "whiteList";
    auto ret = UpdateHeifChecklist(filePath, newWhitelist, newDenyList, newStrategy);
    EXPECT_EQ(ret, true);
    auto ret1 = HeifTranscodingCheckUtils::InitCheckList();
    HeifTranscodingCheckUtils::UnsubscribeCotaUpdatedEvent();
    EXPECT_EQ(ret1, E_OK);
    std::string bundleName = "com.ohos.medialibrary.medialibrarydata";
    auto ret3 = HeifTranscodingCheckUtils::CanSupportedCompatibleDuplicate(bundleName);
    EXPECT_EQ(ret3, true);
    MEDIA_INFO_LOG("end tdd CanSupportedCompatibleDuplicate_test_006");
}

HWTEST_F(HeifTranscodingCheckUtilsTest, InsertBundleCacheInfo_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd InsertBundleCacheInfo_test_001");
    std::string filePath;
    if (IsFileOpenable(DUE_INSTALL_DIR_TEST)) {
        filePath = DUE_INSTALL_DIR_TEST;
    } else {
        filePath = HEIF_TRANSCODING_CHECK_LIST_JSON_LOCAL_NAME_PATH_TEST;
    }
    const uint32_t CAPACITY_BUNDLE_INFO = 51;
    std::string bundleName = "com.ohos.test.cache";
    std::vector<std::string> newWhitelist = {};
    for (uint32_t i = 0; i < CAPACITY_BUNDLE_INFO; i++) {
        newWhitelist.push_back(bundleName + std::to_string(i) + "@1.0.0.1");
    }
    std::vector<std::string> newDenyList = {};
    std::string newStrategy = "whiteList";
    auto ret = UpdateHeifChecklist(filePath, newWhitelist, newDenyList, newStrategy);
    EXPECT_EQ(ret, true);
    auto ret1 = HeifTranscodingCheckUtils::InitCheckList();
    HeifTranscodingCheckUtils::UnsubscribeCotaUpdatedEvent();
    EXPECT_EQ(ret1, E_OK);
    HeifTranscodingCheckUtils::ClearBundleInfoInCache();
    for (uint32_t i = 0; i < CAPACITY_BUNDLE_INFO; i++) {
        HeifBundleInfoCache::InsertBundleCacheInfo(bundleName + std::to_string(i), true);
    }
    auto ret3 = HeifTranscodingCheckUtils::CanSupportedCompatibleDuplicate(bundleName + std::to_string(0));
    EXPECT_EQ(ret3, false);
    for (uint32_t i = 1; i < CAPACITY_BUNDLE_INFO; i++) {
        ret3 = HeifTranscodingCheckUtils::CanSupportedCompatibleDuplicate(bundleName + std::to_string(i));
        EXPECT_EQ(ret3, true);
    }
    MEDIA_INFO_LOG("end tdd InsertBundleCacheInfo_test_001");
}
}
}