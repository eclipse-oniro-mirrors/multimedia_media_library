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

#define MLOG_TAG "GetClonedAssetUrisInnerTest"

#include "get_cloned_asset_uris_inner_test.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "get_uris_by_old_uris_inner_vo.h"
#include "media_assets_controller_service.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_old_photos_column.h"
#include "media_resp_vo.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "message_parcel.h"
#include "result_set_utils.h"
#include "values_bucket.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static const std::string CLONED_OLD_PHOTOS_TABLE = "tab_cloned_old_photos";

static constexpr int64_t TEST_PHOTO_SIZE = 175258;
static constexpr int64_t TEST_PHOTO_DATE_ADDED = 1501924205218;

static void ClearClonedTables()
{
    if (g_rdbStore == nullptr) {
        return;
    }
    (void)g_rdbStore->ExecuteSql("DELETE FROM " + CLONED_OLD_PHOTOS_TABLE);
    (void)g_rdbStore->ExecuteSql("DELETE FROM " + PhotoColumn::PHOTOS_TABLE);
}

static int32_t InsertPhotoRecord(const std::string &displayName)
{
    NativeRdb::ValuesBucket values;
    std::string data = "/storage/cloud/files/Photo/1/" + displayName;
    values.PutString(MediaColumn::MEDIA_FILE_PATH, data);
    values.PutLong(MediaColumn::MEDIA_SIZE, TEST_PHOTO_SIZE);
    values.PutString(MediaColumn::MEDIA_NAME, displayName);
    values.PutInt(MediaColumn::MEDIA_TYPE, 1);
    values.PutLong(MediaColumn::MEDIA_DATE_ADDED, TEST_PHOTO_DATE_ADDED);

    int64_t outRowId = -1;
    int32_t ret = g_rdbStore->Insert(outRowId, PhotoColumn::PHOTOS_TABLE, values);
    if (ret != NativeRdb::E_OK || outRowId <= 0) {
        MEDIA_ERR_LOG("InsertPhotoRecord failed, ret=%{public}d, outRowId=%{public}lld",
            ret, static_cast<long long>(outRowId));
        return -1;
    }
    return static_cast<int32_t>(outRowId);
}

static bool InjectCloneMapping(int32_t oldFileId, int32_t newFileId, int32_t cloneSequence)
{
    NativeRdb::ValuesBucket values;
    values.PutInt(TabOldPhotosColumn::MEDIA_ID, newFileId);
    values.PutString(TabOldPhotosColumn::MEDIA_FILE_PATH, "data_" + std::to_string(newFileId));
    values.PutInt(TabOldPhotosColumn::MEDIA_OLD_ID, oldFileId);
    values.PutString(TabOldPhotosColumn::MEDIA_OLD_FILE_PATH, "old_data_" + std::to_string(oldFileId));
    values.PutInt(TabOldPhotosColumn::MEDIA_CLONE_SEQUENCE, cloneSequence);

    int64_t outRowId = -1;
    int32_t ret = g_rdbStore->Insert(outRowId, CLONED_OLD_PHOTOS_TABLE, values);
    if (ret != NativeRdb::E_OK || outRowId <= 0) {
        MEDIA_ERR_LOG("InjectCloneMapping failed, ret=%{public}d, outRowId=%{public}lld",
            ret, static_cast<long long>(outRowId));
        return false;
    }
    return true;
}

void GetClonedAssetUrisInnerTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start GetClonedAssetUrisTest failed, can not get g_rdbStore");
        exit(1);
    }
    ClearClonedTables();
    MEDIA_INFO_LOG("SetUpTestCase");
}

void GetClonedAssetUrisInnerTest::TearDownTestCase(void)
{
    ClearClonedTables();
    g_rdbStore.reset();
    MEDIA_INFO_LOG("TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void GetClonedAssetUrisInnerTest::SetUp()
{
    ClearClonedTables();
}

void GetClonedAssetUrisInnerTest::TearDown()
{
    ClearClonedTables();
}

static int32_t CallGetClonedAssetUris(const std::vector<std::string> &uris,
    GetUrisByOldUrisInnerRespBody &respBody)
{
    MessageParcel data;
    MessageParcel reply;
    GetUrisByOldUrisInnerReqBody reqBody;
    reqBody.uris = uris;
    if (!reqBody.Marshalling(data)) {
        return E_FAIL;
    }

    MediaAssetsControllerService service;
    int32_t ret = service.GetClonedAssetUrisInner(data, reply);
    if (ret != E_OK) {
        return ret;
    }

    IPC::MediaRespVo<GetUrisByOldUrisInnerRespBody> resp;
    if (!resp.Unmarshalling(reply)) {
        return E_FAIL;
    }
    respBody = resp.GetBody();
    return resp.GetErrCode();
}

HWTEST_F(GetClonedAssetUrisInnerTest, GetClonedAssetUris_Test_001, TestSize.Level0)
{
    int32_t newFileId = InsertPhotoRecord("cloned_normal.jpg");
    ASSERT_GT(newFileId, 0);
    int32_t oldFileId = newFileId + 900000;
    ASSERT_TRUE(InjectCloneMapping(oldFileId, newFileId, 1));

    std::string oldUri = "file://media/Photo/" + std::to_string(oldFileId) + "/" + std::to_string(oldFileId);

    GetUrisByOldUrisInnerRespBody respBody;
    int32_t ret = CallGetClonedAssetUris({ oldUri }, respBody);

    EXPECT_EQ(ret, E_OK);
    ASSERT_FALSE(respBody.fileIds.empty());
    ASSERT_FALSE(respBody.oldFileIds.empty());
    ASSERT_EQ(respBody.fileIds.size(), 1U);
    ASSERT_EQ(respBody.oldFileIds.size(), 1U);
    EXPECT_EQ(respBody.fileIds[0], newFileId);
    EXPECT_EQ(respBody.oldFileIds[0], oldFileId);
}

HWTEST_F(GetClonedAssetUrisInnerTest, GetClonedAssetUris_Test_002, TestSize.Level0)
{
    std::vector<std::string> uris;
    uris.reserve(101);
    for (int32_t i = 0; i < 101; i++) {
        uris.push_back("file://media/Photo/" + std::to_string(91000000 + i) + "/" +
            std::to_string(91000000 + i));
    }
    GetUrisByOldUrisInnerRespBody respBody;
    int32_t ret = CallGetClonedAssetUris(uris, respBody);
    EXPECT_EQ(ret, E_INVALID_URI);
}

HWTEST_F(GetClonedAssetUrisInnerTest, GetClonedAssetUris_Test_003, TestSize.Level0)
{
    std::vector<std::string> uris = {
        "cloud-file://media/Photo/1/1",
        "file:/media/Photo/1/1",
        "",
    };
    GetUrisByOldUrisInnerRespBody respBody;
    int32_t ret = CallGetClonedAssetUris(uris, respBody);
    EXPECT_EQ(ret, E_INVALID_URI);
}

HWTEST_F(GetClonedAssetUrisInnerTest, GetClonedAssetUris_Test_004, TestSize.Level0)
{
    std::vector<std::string> uris;
    GetUrisByOldUrisInnerRespBody respBody;
    int32_t ret = CallGetClonedAssetUris(uris, respBody);
    EXPECT_EQ(ret, E_INVALID_URI);
}

HWTEST_F(GetClonedAssetUrisInnerTest, GetClonedAssetUris_Test_005, TestSize.Level0)
{
    std::vector<std::string> uris = {
        "file://media/Photo/abc/abc",
        "file://media/Photo/xyz/xyz",
    };
    GetUrisByOldUrisInnerRespBody respBody;
    int32_t ret = CallGetClonedAssetUris(uris, respBody);
    EXPECT_EQ(ret, E_INVALID_URI);
}

HWTEST_F(GetClonedAssetUrisInnerTest, GetClonedAssetUris_Test_006, TestSize.Level0)
{
    std::vector<std::string> uris = {
        "file://media/Photo/94000001/94000001",
        "file://media/Photo/94000002/94000002",
    };
    GetUrisByOldUrisInnerRespBody respBody;
    int32_t ret = CallGetClonedAssetUris(uris, respBody);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(respBody.fileIds.empty());
    EXPECT_TRUE(respBody.oldFileIds.empty());
}

HWTEST_F(GetClonedAssetUrisInnerTest, GetClonedAssetUris_Test_007, TestSize.Level0)
{
    int32_t newFileId = InsertPhotoRecord("cloned_013_exist.jpg");
    ASSERT_GT(newFileId, 0);
    int32_t existOldFileId = newFileId + 990001;
    ASSERT_TRUE(InjectCloneMapping(existOldFileId, newFileId, 1));

    std::vector<std::string> uris = {
        "file://media/Photo/99500001/99500001",
        "file://media/Photo/99500002/99500002",
    };
    GetUrisByOldUrisInnerRespBody respBody;
    int32_t ret = CallGetClonedAssetUris(uris, respBody);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(respBody.fileIds.empty());
    EXPECT_TRUE(respBody.oldFileIds.empty());
    EXPECT_TRUE(respBody.datas.empty());
    EXPECT_TRUE(respBody.displayNames.empty());
}

HWTEST_F(GetClonedAssetUrisInnerTest, GetClonedAssetUris_Test_008, TestSize.Level0)
{
    int32_t newFileId = InsertPhotoRecord("cloned_014_new.jpg");
    ASSERT_GT(newFileId, 0);
    int32_t oldFileId = newFileId + 995000;
    ASSERT_TRUE(InjectCloneMapping(oldFileId, newFileId, 1));

    std::string newIdUri = "file://media/Photo/" + std::to_string(newFileId) + "/" +
        std::to_string(newFileId);
    GetUrisByOldUrisInnerRespBody respBody;
    int32_t ret = CallGetClonedAssetUris({ newIdUri }, respBody);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(respBody.fileIds.empty());
    EXPECT_TRUE(respBody.oldFileIds.empty());

    std::string oldIdUri = "file://media/Photo/" + std::to_string(oldFileId) + "/" +
        std::to_string(oldFileId);
    respBody = GetUrisByOldUrisInnerRespBody();
    ret = CallGetClonedAssetUris({ oldIdUri }, respBody);
    EXPECT_EQ(ret, E_OK);
    ASSERT_FALSE(respBody.oldFileIds.empty());
    ASSERT_FALSE(respBody.fileIds.empty());
    ASSERT_EQ(respBody.oldFileIds.size(), 1U);
    ASSERT_EQ(respBody.fileIds.size(), 1U);
    EXPECT_EQ(respBody.oldFileIds[0], oldFileId);
    EXPECT_EQ(respBody.fileIds[0], newFileId);
}

HWTEST_F(GetClonedAssetUrisInnerTest, GetClonedAssetUris_Test_009, TestSize.Level0)
{
    int32_t oldVersionFileId = InsertPhotoRecord("cloned_v1.jpg");
    ASSERT_GT(oldVersionFileId, 0);
    int32_t latestNewFileId = InsertPhotoRecord("cloned_v2.jpg");
    ASSERT_GT(latestNewFileId, 0);
    int32_t oldFileId = latestNewFileId + 930000;
    ASSERT_TRUE(InjectCloneMapping(oldFileId, oldVersionFileId, 1));
    ASSERT_TRUE(InjectCloneMapping(oldFileId, latestNewFileId, 3));

    std::string oldUri = "file://media/Photo/" + std::to_string(oldFileId) + "/" +
        std::to_string(oldFileId);
    GetUrisByOldUrisInnerRespBody respBody;
    int32_t ret = CallGetClonedAssetUris({ oldUri }, respBody);
    EXPECT_EQ(ret, E_OK);
    ASSERT_FALSE(respBody.fileIds.empty());
    ASSERT_FALSE(respBody.oldFileIds.empty());
    ASSERT_EQ(respBody.fileIds.size(), 1U);
    ASSERT_EQ(respBody.oldFileIds.size(), 1U);
    EXPECT_EQ(respBody.fileIds[0], latestNewFileId);
    EXPECT_EQ(respBody.oldFileIds[0], oldFileId);
}

HWTEST_F(GetClonedAssetUrisInnerTest, GetClonedAssetUris_Test_010, TestSize.Level0)
{
    const std::string displayName = "cloned_dup.jpg";
    int32_t newFileId = InsertPhotoRecord(displayName);
    ASSERT_GT(newFileId, 0);
    int32_t oldFileId = newFileId + 920000;
    ASSERT_TRUE(InjectCloneMapping(oldFileId, newFileId, 1));

    std::string oldUri = "file://media/Photo/" + std::to_string(oldFileId) + "/" +
        std::to_string(oldFileId);
    GetUrisByOldUrisInnerRespBody respBody;
    int32_t ret = CallGetClonedAssetUris({ oldUri, oldUri, oldUri }, respBody);
    EXPECT_EQ(ret, E_OK);
    ASSERT_FALSE(respBody.fileIds.empty());
    ASSERT_FALSE(respBody.oldFileIds.empty());
    ASSERT_EQ(respBody.fileIds.size(), 1U);
    ASSERT_EQ(respBody.oldFileIds.size(), 1U);
    EXPECT_EQ(respBody.fileIds[0], newFileId);
    EXPECT_EQ(respBody.oldFileIds[0], oldFileId);
}

HWTEST_F(GetClonedAssetUrisInnerTest, GetClonedAssetUris_Test_011, TestSize.Level0)
{
    int32_t newFileId = InsertPhotoRecord("cloned_with_empty.jpg");
    ASSERT_GT(newFileId, 0);
    int32_t oldFileId = newFileId + 970000;
    ASSERT_TRUE(InjectCloneMapping(oldFileId, newFileId, 1));

    std::string validUri = "file://media/Photo/" + std::to_string(oldFileId) + "/" +
        std::to_string(oldFileId);
    GetUrisByOldUrisInnerRespBody respBody;
    int32_t ret = CallGetClonedAssetUris({ validUri, "" }, respBody);
    EXPECT_EQ(ret, E_INVALID_URI);
    EXPECT_TRUE(respBody.fileIds.empty());
    EXPECT_TRUE(respBody.oldFileIds.empty());
}

HWTEST_F(GetClonedAssetUrisInnerTest, GetClonedAssetUris_Test_012, TestSize.Level0)
{
    int32_t newFileId1 = InsertPhotoRecord("cloned_mixed_1.jpg");
    ASSERT_GT(newFileId1, 0);
    int32_t oldFileId1 = newFileId1 + 980000;
    ASSERT_TRUE(InjectCloneMapping(oldFileId1, newFileId1, 1));

    int32_t newFileId2 = InsertPhotoRecord("cloned_mixed_2.jpg");
    ASSERT_GT(newFileId2, 0);
    int32_t oldFileId2 = newFileId2 + 990000;
    ASSERT_TRUE(InjectCloneMapping(oldFileId2, newFileId2, 1));

    std::string validUri1 = "file://media/Photo/" + std::to_string(oldFileId1) + "/" +
        std::to_string(oldFileId1);
    std::string validUri2 = "file://media/Photo/" + std::to_string(oldFileId2) + "/" +
        std::to_string(oldFileId2);

    GetUrisByOldUrisInnerRespBody respBody;
    int32_t ret = CallGetClonedAssetUris(
        { "", validUri1, "", "", validUri2, "" }, respBody);
    EXPECT_EQ(ret, E_INVALID_URI);
    EXPECT_TRUE(respBody.fileIds.empty());
    EXPECT_TRUE(respBody.oldFileIds.empty());
    EXPECT_TRUE(respBody.datas.empty());
    EXPECT_TRUE(respBody.displayNames.empty());
}
} // namespace OHOS::Media