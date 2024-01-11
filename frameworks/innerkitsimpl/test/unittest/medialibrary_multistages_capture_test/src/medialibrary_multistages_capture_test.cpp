/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MultiStagesCaptureUnitTest"

#include "medialibrary_multistages_capture_test.h"

#include <chrono>
#include <thread>

#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_command.h"
#include "medialibrary_common_utils.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_errno.h"
#include "medialibrary_photo_operations.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "result_set_utils.h"
#include "values_bucket.h"
#define private public
#define protected public
#include "multistages_capture_dfx_first_visit.h"
#include "multistages_capture_dfx_result.h"
#include "multistages_capture_dfx_total_time.h"
#include "multistages_capture_dfx_request_policy.h"
#include "multistages_capture_dfx_trigger_ratio.h"
#undef private
#undef protected

using namespace std;
using namespace OHOS;
using namespace OHOS::NativeRdb;
using namespace testing::ext;

namespace OHOS {
namespace Media {
static shared_ptr<MediaLibraryRdbStore> g_rdbStore;

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

struct UniqueMemberValuesBucket {
    string assetMediaType;
    int32_t startNumber;
};

void PrepareUniqueNumberTable()
{
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("can not get g_rdbStore");
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
            MEDIA_ERR_LOG("Prepare PrepareUniqueNumberTable failed");
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

    system("rm -rf /storage/cloud/files/*");
    system("rm -rf /storage/cloud/files/.thumbs");
    system("rm -rf /storage/cloud/files/.editData");
    for (const auto &dir : TEST_ROOT_DIRS) {
        string ROOT_PATH = "/storage/cloud/100/files/";
        bool ret = MediaFileUtils::CreateDirectory(ROOT_PATH + dir + "/");
        CHECK_AND_PRINT_LOG(ret, "make %{public}s dir failed, ret=%{public}d", dir.c_str(), ret);
    }
    CleanTestTables();
    SetTables();
}

inline int32_t CreatePhotoApi10(int mediaType, const string &displayName)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE,
        MediaLibraryApi::API_10);
    ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_NAME, displayName);
    values.PutInt(MediaColumn::MEDIA_TYPE, mediaType);
    cmd.SetValueBucket(values);
    return MediaLibraryPhotoOperations::Create(cmd);
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

const string PHOTO_ID_FOR_TEST = "202312071614";

int32_t PrepareForFirstVisit()
{
    auto fileId = SetDefaultPhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "MultiStagesCaptureTest001.jpg");
    EXPECT_GT(fileId, 0);

    // update multi-stages capture db info
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE, MediaLibraryApi::API_10);
    ValuesBucket values;
    values.Put(PhotoColumn::PHOTO_QUALITY, 1);
    values.Put(PhotoColumn::PHOTO_ID, PHOTO_ID_FOR_TEST);
    values.Put(PhotoColumn::PHOTO_DEFERRED_PROC_TYPE, 1);
    cmd.SetValueBucket(values);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    EXPECT_GT(MediaLibraryPhotoOperations::Update(cmd), E_OK);

    return fileId;
}
} // namespace

void MediaLibraryMultiStagesCaptureTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStoreRaw();
    if (g_rdbStore == nullptr || g_rdbStore->GetRaw() == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    SetTables();
}

void MediaLibraryMultiStagesCaptureTest::TearDownTestCase(void)
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

// SetUp:Execute before each test case
void MediaLibraryMultiStagesCaptureTest::SetUp()
{
    if (g_rdbStore == nullptr || g_rdbStore->GetRaw() == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    ClearAndRestart();
}

void MediaLibraryMultiStagesCaptureTest::TearDown(void) {}

HWTEST_F(MediaLibraryMultiStagesCaptureTest, dfx_result_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("dfx_result_001 Start");
    MultiStagesCaptureDfxResult::Report("123456", 0);

    MEDIA_INFO_LOG("dfx_result_001 End");
}

HWTEST_F(MediaLibraryMultiStagesCaptureTest, dfx_result_invalid_param_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("dfx_result_invalid_param_002 Start");
    MultiStagesCaptureDfxResult::Report("", 0);

    MEDIA_INFO_LOG("dfx_result_invalid_param_002 End");
}

HWTEST_F(MediaLibraryMultiStagesCaptureTest, dfx_total_time_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("dfx_total_time_001 Start");
    string photoId = "1234566";
    MultiStagesCaptureDfxTotalTime::GetInstance().AddStartTime(photoId);
    EXPECT_EQ(MultiStagesCaptureDfxTotalTime::GetInstance().startTimes_.empty(), false);

    // sleep for 1234 milliseconds
    this_thread::sleep_for(chrono::milliseconds(1234));
    MultiStagesCaptureDfxTotalTime::GetInstance().Report(photoId);
    EXPECT_EQ(MultiStagesCaptureDfxTotalTime::GetInstance().startTimes_.empty(), true);

    MEDIA_INFO_LOG("dfx_total_time_001 End");
}

HWTEST_F(MediaLibraryMultiStagesCaptureTest, dfx_total_time_two_start_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("dfx_total_time_two_start_002 Start");
    string photoId = "1234566";

    // test that photo_id is not add start time
    MultiStagesCaptureDfxTotalTime::GetInstance().Report(photoId);
    EXPECT_EQ(MultiStagesCaptureDfxTotalTime::GetInstance().startTimes_.empty(), true);

    string photoId2 = "12345666";
    MultiStagesCaptureDfxTotalTime::GetInstance().AddStartTime(photoId2);
    EXPECT_EQ(MultiStagesCaptureDfxTotalTime::GetInstance().startTimes_.empty(), false);

    // sleep for 1234 milliseconds
    this_thread::sleep_for(chrono::milliseconds(1234));
    MultiStagesCaptureDfxTotalTime::GetInstance().Report(photoId);
    EXPECT_EQ(MultiStagesCaptureDfxTotalTime::GetInstance().startTimes_.empty(), false);

    MultiStagesCaptureDfxTotalTime::GetInstance().Report(photoId2);
    EXPECT_EQ(MultiStagesCaptureDfxTotalTime::GetInstance().startTimes_.empty(), true);

    MEDIA_INFO_LOG("dfx_total_time_two_start_002 End");
}

HWTEST_F(MediaLibraryMultiStagesCaptureTest, dfx_total_time_remove_start_time_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("dfx_total_time_remove_start_time_003 Start");

    string photoId = "1234566";
    MultiStagesCaptureDfxTotalTime::GetInstance().RemoveStartTime(photoId);
    EXPECT_EQ(MultiStagesCaptureDfxTotalTime::GetInstance().startTimes_.empty(), true);

    MultiStagesCaptureDfxTotalTime::GetInstance().AddStartTime(photoId);
    EXPECT_EQ(MultiStagesCaptureDfxTotalTime::GetInstance().startTimes_.empty(), false);
    sleep(1);
    MultiStagesCaptureDfxTotalTime::GetInstance().RemoveStartTime(photoId);
    EXPECT_EQ(MultiStagesCaptureDfxTotalTime::GetInstance().startTimes_.empty(), true);

    MEDIA_INFO_LOG("dfx_total_time_remove_start_time_003 End");
}

HWTEST_F(MediaLibraryMultiStagesCaptureTest, dfx_request_policy_get_count_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("dfx_request_policy_get_count_001 Start");
    RequestCount requestCount { 0, 0, 0 };
    MultiStagesCaptureDfxRequestPolicy::GetInstance().GetCount(RequestPolicy::HIGH_QUALITY, requestCount);
    EXPECT_EQ(requestCount.highQualityCount, 1);

    MultiStagesCaptureDfxRequestPolicy::GetInstance().GetCount(RequestPolicy::BALANCE_QUALITY, requestCount);
    EXPECT_EQ(requestCount.highQualityCount, 1);
    EXPECT_EQ(requestCount.balanceQualityCount, 1);

    MultiStagesCaptureDfxRequestPolicy::GetInstance().GetCount(RequestPolicy::EMERGENCY_QUALITY, requestCount);
    EXPECT_EQ(requestCount.highQualityCount, 1);
    EXPECT_EQ(requestCount.balanceQualityCount, 1);
    EXPECT_EQ(requestCount.emergencyQualityCount, 1);

    MultiStagesCaptureDfxRequestPolicy::GetInstance().GetCount(RequestPolicy::BALANCE_QUALITY, requestCount);
    EXPECT_EQ(requestCount.highQualityCount, 1);
    EXPECT_EQ(requestCount.balanceQualityCount, 2);
    EXPECT_EQ(requestCount.emergencyQualityCount, 1);

    MultiStagesCaptureDfxRequestPolicy::GetInstance().GetCount(static_cast<RequestPolicy>(3), requestCount);
    EXPECT_EQ(requestCount.highQualityCount, 1);
    EXPECT_EQ(requestCount.balanceQualityCount, 2);
    EXPECT_EQ(requestCount.emergencyQualityCount, 1);

    MEDIA_INFO_LOG("dfx_request_policy_get_count_001 End");
}

HWTEST_F(MediaLibraryMultiStagesCaptureTest, dfx_request_policy_set_policy_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("dfx_request_policy_set_policy_002 Start");

    MultiStagesCaptureDfxRequestPolicy &requestPolicyInstance = MultiStagesCaptureDfxRequestPolicy::GetInstance();

    string callingPackageName = "com.examples.photos";
    requestPolicyInstance.SetPolicy(callingPackageName, RequestPolicy::HIGH_QUALITY);
    // It will definitely be reported for the first time
    EXPECT_EQ(requestPolicyInstance.requestCountMap_.size(), 0);

    requestPolicyInstance.SetPolicy(callingPackageName, RequestPolicy::HIGH_QUALITY);
    EXPECT_EQ(requestPolicyInstance.requestCountMap_.size(), 1);
    EXPECT_EQ(requestPolicyInstance.requestCountMap_[callingPackageName].highQualityCount, 1);

    requestPolicyInstance.SetPolicy(callingPackageName, RequestPolicy::HIGH_QUALITY);
    EXPECT_EQ(requestPolicyInstance.requestCountMap_.size(), 1);
    EXPECT_EQ(requestPolicyInstance.requestCountMap_[callingPackageName].highQualityCount, 2);

    requestPolicyInstance.SetPolicy(callingPackageName, RequestPolicy::BALANCE_QUALITY);
    EXPECT_EQ(requestPolicyInstance.requestCountMap_.size(), 1);
    EXPECT_EQ(requestPolicyInstance.requestCountMap_[callingPackageName].highQualityCount, 2);
    EXPECT_EQ(requestPolicyInstance.requestCountMap_[callingPackageName].balanceQualityCount, 1);

    requestPolicyInstance.SetPolicy(callingPackageName, RequestPolicy::EMERGENCY_QUALITY);
    EXPECT_EQ(requestPolicyInstance.requestCountMap_.size(), 1);
    EXPECT_EQ(requestPolicyInstance.requestCountMap_[callingPackageName].highQualityCount, 2);
    EXPECT_EQ(requestPolicyInstance.requestCountMap_[callingPackageName].balanceQualityCount, 1);
    EXPECT_EQ(requestPolicyInstance.requestCountMap_[callingPackageName].emergencyQualityCount, 1);

    // add another caller request
    string callingPackageName2 = "com.examples.camera";
    requestPolicyInstance.SetPolicy(callingPackageName2, RequestPolicy::HIGH_QUALITY);
    EXPECT_EQ(requestPolicyInstance.requestCountMap_.size(), 2);
    EXPECT_EQ(requestPolicyInstance.requestCountMap_[callingPackageName2].highQualityCount, 1);

    // Simulate scenarios exceeding 24 hours
    requestPolicyInstance.lastReportTime_ -= (24 * 60 * 60 * 1000L + 1);
    requestPolicyInstance.SetPolicy(callingPackageName2, RequestPolicy::EMERGENCY_QUALITY);
    EXPECT_EQ(requestPolicyInstance.requestCountMap_.size(), 0);

    requestPolicyInstance.requestCountMap_.clear();
    MEDIA_INFO_LOG("dfx_request_policy_set_policy_002 End");
}

HWTEST_F(MediaLibraryMultiStagesCaptureTest, dfx_first_visit_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("dfx_first_visit_001 Start");
    auto fileId = PrepareForFirstVisit();
    EXPECT_GT(fileId, 0);

    // sleep for 1234 milliseconds
    this_thread::sleep_for(chrono::milliseconds(1234));

    MultiStagesCaptureDfxFirstVisit &instance = MultiStagesCaptureDfxFirstVisit::GetInstance();
    instance.Report(PHOTO_ID_FOR_TEST);

    // report again, it will not report to hiview
    instance.Report(PHOTO_ID_FOR_TEST);

    MEDIA_INFO_LOG("dfx_first_visit_001 End");
}

HWTEST_F(MediaLibraryMultiStagesCaptureTest, dfx_first_visit_invalid_param_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("dfx_first_visit_invalid_param_002 Start");
    auto fileId = PrepareForFirstVisit();
    EXPECT_GT(fileId, 0);

    // sleep for 1234 milliseconds
    this_thread::sleep_for(chrono::milliseconds(1234));

    MultiStagesCaptureDfxFirstVisit &instance = MultiStagesCaptureDfxFirstVisit::GetInstance();
    // photo id is empty
    instance.Report("");

    // photo id is not exist
    instance.Report("1");

    // report success
    instance.Report(PHOTO_ID_FOR_TEST);

    MEDIA_INFO_LOG("dfx_first_visit_invalid_param_002 End");
}

HWTEST_F(MediaLibraryMultiStagesCaptureTest, dfx_trigger_ratio_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("dfx_trigger_ratio_001 Start");
    MultiStagesCaptureDfxTriggerRatio &instance = MultiStagesCaptureDfxTriggerRatio::GetInstance();
    instance.SetTrigger(MultiStagesCaptureTriggerType::AUTO);
    // report for first time
    EXPECT_EQ(instance.autoCount_, 0);

    instance.SetTrigger(MultiStagesCaptureTriggerType::AUTO);
    EXPECT_EQ(instance.autoCount_, 1);
    instance.SetTrigger(MultiStagesCaptureTriggerType::AUTO);
    EXPECT_EQ(instance.autoCount_, 2);
    instance.SetTrigger(MultiStagesCaptureTriggerType::THIRD_PART);
    EXPECT_EQ(instance.autoCount_, 2);
    EXPECT_EQ(instance.thirdPartCount_, 1);

    // Simulate scenarios exceeding 24 hours
    instance.lastReportTime_ -= (24 * 60 * 60 * 1000L + 1);
    instance.SetTrigger(MultiStagesCaptureTriggerType::THIRD_PART);
    EXPECT_EQ(instance.autoCount_, 0);
    EXPECT_EQ(instance.thirdPartCount_, 0);

    MEDIA_INFO_LOG("dfx_trigger_ratio_001 End");
}

}
}