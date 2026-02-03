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

#define MLOG_TAG "MediaCloudSync"

#include "medi_library_cloud_data_client_test.h"

#include <memory>
#include "media_log.h"
#include "medialibrary_errno.h"

#include "mdk_record.h"
#include "mdk_record_photos_data.h"
#define protected public
#define private public
#include "cloud_media_data_client.h"
#undef protected
#undef private

using namespace testing::ext;

namespace OHOS::Media::CloudSync {

void CloudMediaDataClientTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("start CloudMediaDataClientTest::SetUpTestCase");
}

void CloudMediaDataClientTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("start CloudMediaDataClientTest::TearDownTestCase");
}

void CloudMediaDataClientTest::SetUp()
{
    MEDIA_INFO_LOG("setup");
}

void CloudMediaDataClientTest::TearDown() {}

HWTEST_F(CloudMediaDataClientTest, CloudMediaDataClientTraceId_Test, TestSize.Level1)
{
    std::string traceId = "test";
    auto client = std::make_shared<CloudMediaDataClient>(1, 1);
    ASSERT_TRUE(client);

    client->SetTraceId(traceId);
    auto result = client->GetTraceId();
    EXPECT_EQ(result, "test");
}

HWTEST_F(CloudMediaDataClientTest, CloudMediaDataClientVacantHandler_Test, TestSize.Level1)
{
    int32_t failSize;
    int32_t res = 1;
    int32_t userId = 100;
    int32_t cloudType = 0;
    DownloadThumPara param;
    std::vector<std::string> uris;
    std::vector<MDKRecord> records;
    std::vector<std::string> cloudIds;
    std::vector<uint64_t> filePosStat;
    std::vector<CloudMetaData> metaData;
    std::vector<MediaOperateResult> result;
    std::unordered_map<std::string, int32_t> resMap;
    auto client = std::make_shared<CloudMediaDataClient>(1, 1);
    ASSERT_TRUE(client);
    client->dataHandler_ = nullptr;

    client->SetTraceId("test");
    EXPECT_EQ(client->GetTraceId(), "");
    client->SetUserId(userId);
    EXPECT_EQ(client->userId_, userId);
    client->SetCloudType(cloudType);
    EXPECT_EQ(client->cloudType_, cloudType);
    client->UpdateDirty("test", DirtyTypes::TYPE_DELETED);
    EXPECT_EQ(client->UpdatePosition(cloudIds, 0), E_IPC_INVAL_ARG);
    client->UpdateSyncStatus("test", 1);
    client->UpdateThmStatus("test", 2);
    client->GetAgingFile(1, 2, 3, 4, metaData);
    client->GetActiveAgingFile(1, 2, 3, 4, metaData);
    client->GetDownloadAsset(uris, metaData);
    client->GetDownloadThmsByUri(uris, 1, metaData);
    client->OnDownloadAsset(uris, result);
    client->GetDownloadThms(metaData, param);
    client->OnDownloadThms(resMap, failSize);
    client->GetVideoToCache(metaData, 1);
    client->GetFilePosStat(filePosStat);
    client->GetCloudThmStat(filePosStat);
    client->GetDirtyTypeStat(filePosStat);
    client->GetDownloadThmNum(failSize, 1);
    client->UpdateLocalFileDirty(records);
    EXPECT_EQ(client->GetCloudSyncUnPreparedData(res), E_IPC_INVAL_ARG);
    EXPECT_EQ(client->SubmitCloudSyncPreparedDataTask(), E_IPC_INVAL_ARG);
}

HWTEST_F(CloudMediaDataClientTest, CloudMediaDataClientMDKProperty_Test, TestSize.Level1)
{
    auto record = std::make_shared<MDKRecord>();
    ASSERT_TRUE(record);

    MDKRelation relation;
    MDKRecordsResponse resp;
    resp.deviceName = "Okamoto";
    std::vector<MDKRelation> result;
    std::vector<MDKRelation> relations;
    relations.emplace_back(relation);

    record->SetCreateInfo(resp);
    record->SetModifiedInfo(resp);
    record->SetBaseCursor("qualvision");
    record->SetRecordRelations(relations);
    record->SetOwnerId("10001");
    record->SetShared(false);

    record->GetShared();
    record->GetShareUri();
    record->GetNewCreate();
    record->GetPrivilege();
    record->GetSrcRecordId();
    record->GetRecordRelations(result);
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(record->GetOwnerId(), "10001");
    EXPECT_EQ(record->GetRecordCreateInfo().deviceName, "Okamoto");
    EXPECT_EQ(record->GetRecordModifiedInfo().deviceName, "Okamoto");
}

HWTEST_F(CloudMediaDataClientTest, CloudMediaDataClient_ToJsonValue_Test, TestSize.Level1)
{
    auto record = std::make_shared<MDKRecord>();
    ASSERT_TRUE(record);
    record->SetBaseCursor("10001");
    record->SetRecordType("media");
    record->SetShared(true);
    record->SetOwnerId("101");

    std::vector<MDKRecordField> val;
    MDKRecordField filed1;
    MDKRecordField filed2(val);
    std::map<std::string, MDKRecordField> fields;
    fields["001"] = filed1;
    fields["attachments"] = filed2;
    Json::Value value = record->ToJsonValue();
    EXPECT_EQ(value["isShared"], true);
}

HWTEST_F(CloudMediaDataClientTest, CloudMediaDataClient_CreateInfo_Test, TestSize.Level1)
{
    Json::Value jvData;
    Json::Value jvCreate;
    auto record = std::make_shared<MDKRecord>();
    ASSERT_TRUE(record);

    record->ParseCreateInfoFromJson(jvData);
    jvData["createInfo"] = "info";
    record->ParseCreateInfoFromJson(jvData);

    jvCreate["appId"] = "testAppId";
    jvCreate["deviceName"] = "ABAX";
    jvCreate["time"] = 1234567890;
    jvData["createInfo"] = jvCreate;
    record->ParseCreateInfoFromJson(jvData);
    EXPECT_EQ(record->GetRecordCreateInfo().deviceName, "ABAX");
}

HWTEST_F(CloudMediaDataClientTest, CloudMediaDataClient_ModifyInfo_Test, TestSize.Level1)
{
    Json::Value jvData;
    Json::Value jvModify;
    auto record = std::make_shared<MDKRecord>();
    ASSERT_TRUE(record);
    record->ParseModifyInfoFromJson(jvModify);

    jvModify["modifiedInfo"] = "modify";
    record->ParseModifyInfoFromJson(jvModify);

    jvData["appId"] = "appid";
    jvData["deviceName"] = "deviceName";
    jvData["time"] = 1121121;
    jvModify["modifiedInfo"] = jvData;
    record->ParseModifyInfoFromJson(jvModify);
    EXPECT_EQ(record->GetRecordModifiedInfo().deviceName, "deviceName");
}

HWTEST_F(CloudMediaDataClientTest, CloudMediaDataClientRelations_Test, TestSize.Level1)
{
    Json::Value jvData;
    Json::Value jvRelations;
    auto record = std::make_shared<MDKRecord>();
    ASSERT_TRUE(record);
    record->ParseRelationsFromJson(jvData);
    jvData["relations"] = "realtions";
    record->ParseRelationsFromJson(jvData);

    Json::Value jvRelation1;
    jvRelation1["relationName"] = "friend";
    jvRelation1["recordType"] = "person";
    jvRelation1["recordId"] = "123";

    Json::Value jvRelation2;
    jvRelation2["relationName"] = "colleague";
    jvRelation2["recordType"] = "person";
    jvRelation2["recordId"] = "456";

    jvRelations.append(jvRelation1);
    jvRelations.append(jvRelation2);
    jvData["relations"] = jvRelations;
    record->ParseRelationsFromJson(jvData);

    std::vector<MDKRelation> relations;
    record->GetRecordRelations(relations);
    EXPECT_EQ(relations.size(), 2);
}

HWTEST_F(CloudMediaDataClientTest, CloudMediaDataClient_ParseFromJsonValue_Test, TestSize.Level1)
{
    MDKSchema schema;
    Json::Value jvData = Json::nullValue;
    auto record = std::make_shared<MDKRecord>();
    ASSERT_TRUE(record);
    bool result = record->ParseFromJsonValue(schema, jvData);
    EXPECT_EQ(result, false);
}

HWTEST_F(CloudMediaDataClientTest, CloudMediaDataClient_MDKRecordProperties_Test, TestSize.Level1)
{
    auto data = std::make_shared<MDKRecordPhotosData>();
    ASSERT_TRUE(data);

    data->SetFileCreateTime("19991110");
    auto createTime = data->GetFileCreateTime();
    EXPECT_EQ(createTime.value(), "19991110");

    data->SetOwnerAlbumId(120);
    auto AlbumId = data->GetOwnerAlbumId();
    EXPECT_EQ(AlbumId.value(), 120);

    data->SetDateAdded(12211);
    auto dataAldded = data->GetDateAdded();
    EXPECT_EQ(dataAldded.value(), 12211);

    data->SetFirstUpdateTime("2011");
    data->SetSourceFileName("video.avi");
    data->SetSourcePath("/usr/local/bin");
    data->SetEditDataCamera("001");
    data->SetEditedTimeMs(10011);

    data->SetThmSize(10086);
    data->SetLcdSize(2);
    data->SetFixVersion(1110011);
    data->SetFilePath("test/");

    data->SetHeight(1);
    data->SetWidth(0);
    data->SetDetailTime("1970111000111");
    data->SetPosition("Earth");

    data->SetRotate(1);
    data->SetOriginalAssetCloudId("0001");
    data->SetCloudId("0001");
    data->SetCloudFileId(121);
}

HWTEST_F(CloudMediaDataClientTest, CloudMediaDataClient_MDKRecord_Test, TestSize.Level1)
{
    auto data = std::make_shared<MDKRecordPhotosData>();
    ASSERT_TRUE(data);

    MDKRecord record;
    record.SetRecordId("001");
    data->SetDKRecord(record);
    MDKRecord records = data->GetDKRecord();
    EXPECT_EQ(records.GetRecordId(), "001");

    data->SetSourcePath("test");
    data->SetFileId(12);
    data->GetDKRecord();
}
}