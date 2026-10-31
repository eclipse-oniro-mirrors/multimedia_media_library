/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#define MLOG_TAG "KvStoreSwapOperatorTest"

#include "kvstore_swap_operator_test.h"

#include "backup_const.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_pure_file_utils.h"
#include "medialibrary_errno.h"
#include "reverse_clone_restore/kvstore_swap_operator.h"
#include "reverse_clone_restore_marker.h"
#include "settings_data_manager.h"

#include "datashare_errno.h"
#include "datashare_helper.h"
#include "datashare_result_set.h"
#include "datashare_values_bucket.h"

#include <gtest/hwext/gtest-ext.h>
#include <atomic>
#include <functional>
#include <map>
#include <mutex>
#include <thread>
#include <variant>

using namespace testing::ext;

namespace OHOS {
namespace DataShare {
namespace {
std::mutex g_settingsDataMutex;
std::map<std::string, std::string> g_settingsData;
} // namespace

static std::string GetKeyFromUri(const Uri &uri)
{
    const std::string marker = "key=";
    std::string uriStr = uri.ToString();
    size_t pos = uriStr.find(marker);
    if (pos == std::string::npos) {
        return "";
    }
    return uriStr.substr(pos + marker.size();
}

static std::string ValueToString(const DataShareValueObject::Type &type)
{
    auto str = std::get_if<std::string>(&type);
    return str != nullptr ? *str : "";
}

class MockDataShareResultSet : public DataShareResultSet {
public:
    explicit MockDataShareResultSet(const std::string &key) : key_(key) {}
    int GetColumnCount(int &count) override
    {
        count = 1;
        return E_OK;
    }
    int GetColumnIndex(const std::string &columnName, int &columnIndex) override
    {
        if (columnName == "value") {
            columnIndex = 0;
            return E_OK;
        }
        return E_ERROR;
    }
    int GetColumnName(int columnIndex, std::string &columnName) override
    {
        columnName = "value";
        return E_OK;
    }
    int GetRowIndex(int &position) const override
    {
        position = 0;
        return E_OK;
    }
    int GoTo(int offset) override
    {
        return E_OK;
    }
    int GoToFirstRow() override
    {
        return E_OK;
    }
    int GoToLastRow() override
    {
        return E_OK;
    }
    int GoToNextRow() override
    {
        return E_OK;
    }
    int GoToPreviousRow() override
    {
        return E_OK;
    }
    int IsAtFirstRow(bool &result) const override
    {
        result = true;
        return E_OK;
    }
    int IsAtLastRow(bool &result) override
    {
        result = false;
        return E_OK;
    }
    int IsStarted(bool &result) const override
    {
        result = true;
        return E_OK;
    }
    int IsEnded(bool &result) override
    {
        result = false;
        return E_OK;
    }
    bool IsClosed() const override
    {
        return false;
    }
    int GetString(int columnIndex, std::string &value) override
    {
        if (columnIndex != 0) {
            return E_ERROR;
        }
        std::lock_guard<std::mutex> lock(g_settingsDataMutex);
        auto it = g_settingsData.find(key_);
        value = it != g_settingsData.end() ? it->second : "";
        return E_OK;
    }
    int GetRowCount(int &count) override
    {
        std::lock_guard<std::mutex> lock(g_settingsDataMutex);
        count = g_settingsData.count(key_) > 0 ? 1 : 0;
        return E_OK;
    }
    int Close() override
    {
        return E_OK;
    }

private:
    std::string key_;
};

class MockDataShareHelper : public DataShareHelper {
public:
    bool Release() override
    {
        return true;
    }
    std::vector<std::string> GetFileTypes(Uri &uri, const std::string &mimeTypeFilter) override
    {
        return {};
    }
    int OpenFile(Uri &uri, const std::string &mode) override
    {
        return -1;
    }
    int OpenFileWithErrCode(Uri &uri, const std::string &mode, int32_t &errCode) override
    {
        errCode = E_ERROR;
        return -1;
    }
    int OpenRawFile(Uri &uri, const std::string &mode) override
    {
        return -1;
    }
    int Insert(Uri &uri, const DataShareValuesBucket &value) override
    {
        return -1;
    }
    int InsertExt(Uri &uri, const DataShareValuesBucket &value, std::string &result) override
    {
        return -1;
    }
    int Update(Uri &uri, const DataSharePredicates &predicates, const DataShareValuesBucket &value) override
    {
        return -1;
    }
    int BatchUpdate(const UpdateOperations &operations, std::vector<BatchUpdateResult> &results) override
    {
        return -1;
    }
    int Delete(Uri &uri, const DataSharePredicates &predicates) override
    {
        return -1;
    }
    std::shared_ptr<DataShareResultSet> Query(Uri &uri, const DataSharePredicates &predicates,
        std::vector<std::string> &columns, DatashareBusinessError *businessError = nullptr) override
    {
        return std::make_shared<MockDataShareResultSet>(GetKeyFromUri(uri));
    }
    std::string GetType(Uri &uri) override
    {
        return "";
    }
    int BatchInsert(Uri &uri, const std::vector<DataShareValuesBucket> &values) override
    {
        return -1;
    }
    int ExecuteBatch(const std::vector<OperationStatement> &statements, ExecResultSet &result) override
    {
        return -1;
    }
    int RegisterObserver(const Uri &uri, const sptr<AAFwk::IDataAbilityObserver> &dataObserver) override
    {
        return -1;
    }
    int UnregisterObserver(const Uri &uri, const sptr<AAFwk::IDataAbilityObserver> &dataObserver) override
    {
        return -1;
    }
    void NotifyChange(const Uri &uri) override
    {
    }
    int RegisterObserverExtProvider(const Uri &uri, std::shared_ptr<DataShareObserver> dataObserver,
        bool isDesendants) override
    {
        return -1;
    }
    int UnregisterObserverExtProvider(const Uri &uri, std::shared_ptr<DataShareObserver> dataObserver) override
    {
        return -1;
    }
    void NotifyChangeExtProvider(const DataShareObserver::ChangeInfo &changeInfo) override
    {
    }
    Uri NormalizeUri(Uri &uri) override
    {
        return uri;
    }
    Uri DenormalizeUri(Uri &uri) override
    {
        return uri;
    }
    int AddQueryTemplate(const std::string &uri, int64_t subscriberId, Template &tpl) override
    {
        return -1;
    }
    int DelQueryTemplate(const std::string &uri, int64_t subscriberId) override
    {
        return -1;
    }
    std::vector<OperationResult> Publish(const Data &data, const std::string &bundleName) override
    {
        return {};
    }
    Data GetPublishedData(const std::string &bundleName, int &resultCode) override
    {
        resultCode = E_ERROR;
        return {};
    }
    std::vector<OperationResult> SubscribeRdbData(const std::vector<std::string> &uris, const TemplateId &templateId,
        const std::function<void(const RdbChangeNode &changeNode)> &callback) override
    {
        return {};
    }
    std::vector<OperationResult> UnsubscribeRdbData(
        const std::vector<std::string> &uris, const TemplateId &templateId) override
    {
        return {};
    }
    std::vector<OperationResult> EnableRdbSubs(
        const std::vector<std::string> &uris, const TemplateId &templateId) override
    {
        return {};
    }
    std::vector<OperationResult> DisableRdbSubs(
        const std::vector<std::string> &uris, const TemplateId &templateId) override
    {
        return {};
    }
    std::vector<OperationResult> SubscribePublishedData(const std::vector<std::string> &uris, int64_t subscriberId,
        const std::function<void(const PublishedDataChangeNode &)> &callback) override
    {
        return {};
    }
    std::vector<OperationResult> UnsubscribePublishedData(
        const std::vector<std::string> &uris, int64_t subscriberId) override
    {
        return {};
    }
    std::vector<OperationResult> EnablePubSubs(const std::vector<std::string> &uris, int64_t subscriberId) override
    {
        return {};
    }
    std::vector<OperationResult> DisablePubSubs(const std::vector<std::string> &uris, int64_t subscriberId) override
    {
        return {};
    }
    std::pair<int32_t, int32_t> InsertEx(Uri &uri, const DataShareValuesBucket &value) override
    {
        std::lock_guard<std::mutex> lock(g_settingsDataMutex);
        g_settingsData[GetKeyFromUri(uri)] = ValueToString(value.valuesMap.at("VALUE"));
        return { E_OK, 1 };
    }
    std::pair<int32_t, int32_t> UpdateEx(Uri &uri, const DataSharePredicates &predicates,
        const DataShareValuesBucket &value) override
    {
        std::lock_guard<std::mutex> lock(g_settingsDataMutex);
        g_settingsData[GetKeyFromUri(uri)] = ValueToString(value.valuesMap.at("VALUE"));
        return { E_OK, 1 };
    }
};

int DataShareAbsResultSet::GetColumnCount(int &count)
{
    count = 1;
    return E_OK;
}
int DataShareAbsResultSet::GetColumnIndex(const std::string &columnName, int &columnIndex)
{
    if (columnName == "value") {
        columnIndex = 0;
        return E_OK;
    }
    return E_ERROR;
}
int DataShareAbsResultSet::GetColumnName(int columnIndex, std::string &columnName)
{
    columnName = "value";
    return E_OK;
}
int DataShareAbsResultSet::GetRowIndex(int &position) const
{
    position = 0;
    return E_OK;
}
int DataShareAbsResultSet::GoTo(int offset)
{
    return E_OK;
}
int DataShareAbsResultSet::GoToFirstRow()
{
    return E_OK;
}
int DataShareAbsResultSet::GoToLastRow()
{
    return E_OK;
}
int DataShareAbsResultSet::GoToNextRow()
{
    return E_OK;
}
int DataShareAbsResultSet::GoToPreviousRow()
{
    return E_OK;
}
int DataShareAbsResultSet::IsAtFirstRow(bool &result) const
{
    result = true;
    return E_OK;
}
int DataShareAbsResultSet::IsAtLastRow(bool &result)
{
    result = false;
    return E_OK;
}
int DataShareAbsResultSet::IsStarted(bool &result) const
{
    result = true;
    return E_OK;
}
int DataShareAbsResultSet::IsEnded(bool &result)
{
    result = false;
    return E_OK;
}
bool DataShareAbsResultSet::IsClosed() const
{
    return false;
}

std::shared_ptr<DataShareHelper> DataShareHelper::Creator(const std::string &strUri, const CreateOptions &options,
    const std::string &bundleName, const int waitTime, bool isSystem)
{
    return std::make_shared<MockDataShareHelper>();
}
} // namespace DataShare
} // namespace OHOS

namespace OHOS {
namespace Media {

static const std::string BACKUP_ROOT = "/storage/media/local/files/.backup/restore";
static const std::string TEMP_OLD = BACKUP_ROOT + "/old";
static const std::string TEMP_NEW = BACKUP_ROOT + "/new";
static const std::string TEMP_DIR = BACKUP_ROOT + "/temp";
static const std::string HASH_A = "hashA";
static const std::string HASH_B = "hashB";
static const std::string OLD_YEAR_HASH = "5f796bfa83fcd52d829a2304cb7194119a6e6e84b8efdde115b26fbd424b8541";
static const std::string NEW_YEAR_HASH = "d1fceec62f61bcc5d3b48e9b01c80384ba4ab9c91eb68b13a573ccea03803e43";
static const std::string OLD_MONTH_HASH = "1bd52423fd08d9b97e0c93366e3f9747b17e5dc321a3f0e328273cbd3dca611a";
static const std::string NEW_MONTH_HASH = "3ddb6fb8b2fcb38d2f431e86bfb806dab771637860d6e86bb9430fa15df04248";

void KvStoreSwapOperatorTest::SetUpTestCase()
{
    MediaPureFileUtils::CreateDirectory(BACKUP_ROOT);
}

void KvStoreSwapOperatorTest::TearDownTestCase()
{
    MediaFileUtils::DeleteDir(BACKUP_ROOT);
    MediaFileUtils::DeleteDir(REVERSE_KVDB_TEMP_DIR);
}

void KvStoreSwapOperatorTest::SetUp()
{
    MediaFileUtils::DeleteDir(BACKUP_ROOT);
    MediaPureFileUtils::CreateDirectory(BACKUP_ROOT);
    MediaFileUtils::DeleteDir(REVERSE_KVDB_TEMP_DIR);
}

void KvStoreSwapOperatorTest::TearDown()
{
    MediaFileUtils::DeleteDir(BACKUP_ROOT);
    MediaFileUtils::DeleteDir(REVERSE_KVDB_TEMP_DIR);
}

static void CreateSingleVer(const std::string &base, const std::string &hash)
{
    std::string dir = base + "/" + hash + "/single_ver/main";
    MediaPureFileUtils::CreateDirectory(dir);
    FILE *f = fopen((dir + "/gen_natural_store.db").c_str(), "w");
    if (f != nullptr) {
        fputs("test", f);
        fclose(f);
    }
}

static bool FileExists(const std::string &base, const std::string &hash)
{
    return MediaPureFileUtils::IsFileExists(base + "/" + hash + "/single_ver/main/gen_natural_store.db");
}

static void CreateHashDir(const std::string &base, const std::string &hash)
{
    MediaPureFileUtils::CreateDirectory(base + "/" + hash);
}

HWTEST_F(KvStoreSwapOperatorTest, CleanupTempDir_NotExist_001, TestSize.Level1)
{
    KvStoreSwapOperator op;
    EXPECT_TRUE(op.CleanupTempDir());
}

HWTEST_F(KvStoreSwapOperatorTest, CleanupTempDir_Exist_001, TestSize.Level1)
{
    CreateSingleVer(REVERSE_KVDB_TEMP_DIR + "/kvdb", HASH_A);
    KvStoreSwapOperator op;
    EXPECT_TRUE(op.CleanupTempDir());
    EXPECT_FALSE(MediaPureFileUtils::IsFileExists(REVERSE_KVDB_TEMP_DIR));
}

HWTEST_F(KvStoreSwapOperatorTest, Swap_OldDirsEmpty_001, TestSize.Level1)
{
    KvStoreSwapOperator op;
    EXPECT_FALSE(op.Swap(BACKUP_ROOT));
}

HWTEST_F(KvStoreSwapOperatorTest, Swap_NewDirsEmpty_001, TestSize.Level1)
{
    CreateHashDir(BACKUP_ROOT + CLONE_KVDB_BACKUP_DIR + "/kvdb", HASH_A);
    CreateHashDir(BACKUP_ROOT + CLONE_KVDB_BACKUP_DIR + "/kvdb", HASH_B);
    KvStoreSwapOperator op;
    EXPECT_FALSE(op.Swap(BACKUP_ROOT));
}

HWTEST_F(KvStoreSwapOperatorTest, Swap_NoMatch_001, TestSize.Level1)
{
    CreateHashDir(BACKUP_ROOT + CLONE_KVDB_BACKUP_DIR + "/kvdb", HASH_A);
    CreateHashDir(BACKUP_ROOT + CLONE_KVDB_BACKUP_DIR + "/kvdb", HASH_B);
    CreateHashDir(std::string(MEDIA_KVDB_DIR) + "/kvdb", "hashC");
    CreateHashDir(std::string(MEDIA_KVDB_DIR) + "/kvdb", "hashD");
    KvStoreSwapOperator op;
    EXPECT_FALSE(op.Swap(BACKUP_ROOT));
    MediaFileUtils::DeleteDir(std::string(MEDIA_KVDB_DIR) + "/kvdb/hashC");
    MediaFileUtils::DeleteDir(std::string(MEDIA_KVDB_DIR) + "/kvdb/hashD");
}

HWTEST_F(KvStoreSwapOperatorTest, Swap_AllSuccess_001, TestSize.Level1)
{
    const std::string oldBase = BACKUP_ROOT + CLONE_KVDB_BACKUP_DIR + "/kvdb";
    const std::string newBase = std::string(MEDIA_KVDB_DIR) + "/kvdb";
    const std::string tempBase = std::string(REVERSE_KVDB_TEMP_DIR) + "/kvdb";

    int32_t ret = SettingsDataManager::SetKvDbCloseStatus("0");
    ASSERT_EQ(ret, E_OK) << "SettingsDataManager unavailable, skip Swap success test";

    std::atomic<bool> stopFlag{false};
    std::thread simThread([&stopFlag]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        SettingsDataManager::SetKvDbCloseStatus("1");
        while (!stopFlag.load(std::memory_order_relaxed)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });

    CreateSingleVer(oldBase, OLD_YEAR_HASH);
    CreateSingleVer(oldBase, OLD_MONTH_HASH);
    CreateSingleVer(newBase, NEW_YEAR_HASH);
    CreateSingleVer(newBase, NEW_MONTH_HASH);

    KvStoreSwapOperator op;
    bool result = op.Swap(BACKUP_ROOT);

    stopFlag.store(true, std::memory_order_relaxed);
    simThread.join();

    EXPECT_TRUE(result);
    EXPECT_TRUE(FileExists(newBase, NEW_YEAR_HASH));
    EXPECT_TRUE(FileExists(tempBase, OLD_YEAR_HASH));
    EXPECT_FALSE(FileExists(oldBase, OLD_YEAR_HASH));
    EXPECT_TRUE(ReverseCloneRestoreMarker::Exists(ReverseCloneMarkerType::KVDB));

    ReverseCloneRestoreMarker::Delete(ReverseCloneMarkerType::KVDB);
    MediaFileUtils::DeleteDir(newBase + "/" + NEW_YEAR_HASH);
    MediaFileUtils::DeleteDir(newBase + "/" + NEW_MONTH_HASH);
    MediaFileUtils::DeleteDir(tempBase + "/" + OLD_YEAR_HASH);
    MediaFileUtils::DeleteDir(tempBase + "/" + OLD_MONTH_HASH);
    MediaFileUtils::DeleteDir(oldBase);
    SettingsDataManager::SetKvDbCloseStatus("0");
}

HWTEST_F(KvStoreSwapOperatorTest, Rollback_FromNone_Empty_001, TestSize.Level1)
{
    KvStoreSwapOperator op;
    EXPECT_TRUE(op.Rollback());
}

HWTEST_F(KvStoreSwapOperatorTest, Rollback_FromNone_WithTemp_001, TestSize.Level1)
{
    CreateSingleVer(REVERSE_KVDB_TEMP_DIR + "/kvdb", HASH_A);
    KvStoreSwapOperator op;
    EXPECT_TRUE(op.Rollback());
    EXPECT_FALSE(MediaPureFileUtils::IsFileExists(REVERSE_KVDB_TEMP_DIR));
}

static bool DoSwapWithSimulatedHandshake()
{
    if (SettingsDataManager::SetKvDbCloseStatus("0") != E_OK) {
        return false;
    }
    std::atomic<bool> stopFlag{false};
    std::thread simThread([&stopFlag]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        SettingsDataManager::SetKvDbCloseStatus("1");
        while (!stopFlag.load(std::memory_order_relaxed)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });
    KvStoreSwapOperator op;
    bool result = op.Swap(BACKUP_ROOT);
    stopFlag.store(true, std::memory_order_relaxed);
    simThread.join();
    return result;
}

static void CleanupSwapArtifacts()
{
    const std::string oldBase = BACKUP_ROOT + CLONE_KVDB_BACKUP_DIR + "/kvdb";
    const std::string newBase = std::string(MEDIA_KVDB_DIR) + "/kvdb";
    const std::string tempBase = std::string(REVERSE_KVDB_TEMP_DIR) + "/kvdb";
    MediaFileUtils::DeleteDir(newBase + "/" + NEW_YEAR_HASH);
    MediaFileUtils::DeleteDir(newBase + "/" + NEW_MONTH_HASH);
    MediaFileUtils::DeleteDir(tempBase + "/" + OLD_YEAR_HASH);
    MediaFileUtils::DeleteDir(tempBase + "/" + OLD_MONTH_HASH);
    MediaFileUtils::DeleteDir(oldBase + "/" + OLD_YEAR_HASH);
    MediaFileUtils::DeleteDir(oldBase + "/" + OLD_MONTH_HASH);
    ReverseCloneRestoreMarker::Delete(ReverseCloneMarkerType::KVDB);
    SettingsDataManager::SetKvDbCloseStatus("0");
}

HWTEST_F(KvStoreSwapOperatorTest, Rollback_AfterSwap_001, TestSize.Level1)
{
    const std::string oldBase = BACKUP_ROOT + CLONE_KVDB_BACKUP_DIR + "/kvdb";
    const std::string masterBase = std::string(MEDIA_KVDB_DIR) + "/kvdb";
    const std::string tempBase = std::string(REVERSE_KVDB_TEMP_DIR) + "/kvdb";

    CreateSingleVer(oldBase, OLD_YEAR_HASH);
    CreateSingleVer(oldBase, OLD_MONTH_HASH);
    CreateSingleVer(masterBase, NEW_YEAR_HASH);
    CreateSingleVer(masterBase, NEW_MONTH_HASH);

    int32_t ret = SettingsDataManager::SetKvDbCloseStatus("0");
    ASSERT_EQ(ret, E_OK) << "SettingsDataManager unavailable, skip Rollback test";
    std::atomic<bool> stopFlag{false};
    std::thread simThread([&stopFlag]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        SettingsDataManager::SetKvDbCloseStatus("1");
        while (!stopFlag.load(std::memory_order_relaxed)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });

    KvStoreSwapOperator op;
    ASSERT_TRUE(op.Swap(BACKUP_ROOT)) << "Swap failed, skip Rollback test";

    stopFlag.store(true, std::memory_order_relaxed);
    simThread.join();

    EXPECT_FALSE(FileExists(oldBase, OLD_YEAR_HASH));
    EXPECT_TRUE(FileExists(tempBase, OLD_YEAR_HASH));

    EXPECT_TRUE(op.Rollback());
    EXPECT_TRUE(FileExists(oldBase, OLD_YEAR_HASH));
    EXPECT_TRUE(FileExists(masterBase, NEW_YEAR_HASH));
    EXPECT_FALSE(FileExists(tempBase, OLD_YEAR_HASH));

    CleanupSwapArtifacts();
}

HWTEST_F(KvStoreSwapOperatorTest, Probe_SettingsDataStub_001, TestSize.Level1)
{
    int32_t ret = SettingsDataManager::SetKvDbCloseStatus("0");
    EXPECT_EQ(ret, E_OK) << "settingsdata stub write failed";

    std::string value;
    ret = SettingsDataManager::GetKvDbCloseStatus(value);
    EXPECT_EQ(ret, E_OK) << "settingsdata stub read failed";
    EXPECT_EQ(value, "0");

    ret = SettingsDataManager::SetKvDbCloseStatus("1");
    EXPECT_EQ(ret, E_OK);
    ret = SettingsDataManager::GetKvDbCloseStatus(value);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(value, "1");
    SettingsDataManager::SetKvDbCloseStatus("0");
}

HWTEST_F(KvStoreSwapOperatorTest, Probe_SwapWithSettingsDataStub_001, TestSize.Level1)
{
    const std::string oldBase = BACKUP_ROOT + CLONE_KVDB_BACKUP_DIR + "/kvdb";
    const std::string newBase = std::string(MEDIA_KVDB_DIR) + "/kvdb";
    const std::string tempBase = std::string(REVERSE_KVDB_TEMP_DIR) + "/kvdb";

    CreateSingleVer(oldBase, OLD_YEAR_HASH);
    CreateSingleVer(oldBase, OLD_MONTH_HASH);
    CreateSingleVer(newBase, NEW_YEAR_HASH);
    CreateSingleVer(newBase, NEW_MONTH_HASH);

    ASSERT_TRUE(DoSwapWithSimulatedHandshake()) << "Swap failed with settingsdata stub";

    EXPECT_TRUE(FileExists(newBase, NEW_YEAR_HASH));
    EXPECT_TRUE(FileExists(tempBase, OLD_YEAR_HASH));
    EXPECT_FALSE(FileExists(oldBase, OLD_YEAR_HASH));
    EXPECT_TRUE(ReverseCloneRestoreMarker::Exists(ReverseCloneMarkerType::KVDB));

    CleanupSwapArtifacts();
}

HWTEST_F(KvStoreSwapOperatorTest, Resume_AfterSwap_FullRestore_001, TestSize.Level1)
{
    const std::string oldBase = BACKUP_ROOT + CLONE_KVDB_BACKUP_DIR + "/kvdb";
    const std::string newBase = std::string(MEDIA_KVDB_DIR) + "/kvdb";
    const std::string tempBase = std::string(REVERSE_KVDB_TEMP_DIR) + "/kvdb";

    CreateSingleVer(oldBase, OLD_YEAR_HASH);
    CreateSingleVer(oldBase, OLD_MONTH_HASH);
    CreateSingleVer(newBase, NEW_YEAR_HASH);
    CreateSingleVer(newBase, NEW_MONTH_HASH);
    ASSERT_TRUE(DoSwapWithSimulatedHandshake()) << "Swap failed, skip Resume test";

    KvStoreSwapOperator resumeOp;
    EXPECT_TRUE(resumeOp.RestoreOriginalPositionsForResume(BACKUP_ROOT));
    EXPECT_TRUE(FileExists(oldBase, OLD_YEAR_HASH));
    EXPECT_TRUE(FileExists(newBase, NEW_YEAR_HASH));
    EXPECT_FALSE(FileExists(tempBase, OLD_YEAR_HASH));

    CleanupSwapArtifacts();
}

HWTEST_F(KvStoreSwapOperatorTest, Resume_AfterSwap_MasterLost_001, TestSize.Level1)
{
    const std::string oldBase = BACKUP_ROOT + CLONE_KVDB_BACKUP_DIR + "/kvdb";
    const std::string newBase = std::string(MEDIA_KVDB_DIR) + "/kvdb";
    const std::string tempBase = std::string(REVERSE_KVDB_TEMP_DIR) + "/kvdb";

    CreateSingleVer(oldBase, OLD_YEAR_HASH);
    CreateSingleVer(oldBase, OLD_MONTH_HASH);
    CreateSingleVer(newBase, NEW_YEAR_HASH);
    CreateSingleVer(newBase, NEW_MONTH_HASH);
    ASSERT_TRUE(DoSwapWithSimulatedHandshake()) << "Swap failed, skip Resume test";

    MediaFileUtils::DeleteDir(newBase + "/" + NEW_YEAR_HASH);
    MediaFileUtils::DeleteDir(newBase + "/" + NEW_MONTH_HASH);
    KvStoreSwapOperator resumeOp;
    EXPECT_TRUE(resumeOp.RestoreOriginalPositionsForResume(BACKUP_ROOT));
    EXPECT_TRUE(FileExists(newBase, NEW_YEAR_HASH));
    EXPECT_FALSE(FileExists(tempBase, OLD_YEAR_HASH));

    CleanupSwapArtifacts();
}

HWTEST_F(KvStoreSwapOperatorTest, Resume_AfterSwap_AlreadyRestored_001, TestSize.Level1)
{
    const std::string oldBase = BACKUP_ROOT + CLONE_KVDB_BACKUP_DIR + "/kvdb";
    const std::string newBase = std::string(MEDIA_KVDB_DIR) + "/kvdb";

    CreateSingleVer(oldBase, OLD_YEAR_HASH);
    CreateSingleVer(oldBase, OLD_MONTH_HASH);
    CreateSingleVer(newBase, NEW_YEAR_HASH);
    CreateSingleVer(newBase, NEW_MONTH_HASH);
    ASSERT_TRUE(DoSwapWithSimulatedHandshake()) << "Swap failed, skip Resume test";

    CreateSingleVer(oldBase, OLD_YEAR_HASH);
    KvStoreSwapOperator resumeOp;
    EXPECT_TRUE(resumeOp.RestoreOriginalPositionsForResume(BACKUP_ROOT));
    EXPECT_TRUE(FileExists(newBase, NEW_YEAR_HASH));
    EXPECT_TRUE(FileExists(oldBase, OLD_YEAR_HASH));

    CleanupSwapArtifacts();
}

HWTEST_F(KvStoreSwapOperatorTest, Resume_AfterSwap_AllLost_001, TestSize.Level1)
{
    const std::string oldBase = BACKUP_ROOT + CLONE_KVDB_BACKUP_DIR + "/kvdb";
    const std::string newBase = std::string(MEDIA_KVDB_DIR) + "/kvdb";

    CreateSingleVer(oldBase, OLD_YEAR_HASH);
    CreateSingleVer(oldBase, OLD_MONTH_HASH);
    CreateSingleVer(newBase, NEW_YEAR_HASH);
    CreateSingleVer(newBase, NEW_MONTH_HASH);
    ASSERT_TRUE(DoSwapWithSimulatedHandshake()) << "Swap failed, skip Resume test";

    MediaFileUtils::DeleteDir(newBase + "/" + NEW_YEAR_HASH);
    MediaFileUtils::DeleteDir(newBase + "/" + NEW_MONTH_HASH);
    MediaFileUtils::DeleteDir(REVERSE_KVDB_TEMP_DIR);
    KvStoreSwapOperator resumeOp;
    EXPECT_TRUE(resumeOp.RestoreOriginalPositionsForResume(BACKUP_ROOT));

    CleanupSwapArtifacts();
}
} // namespace Media
} // namespace OHOS
