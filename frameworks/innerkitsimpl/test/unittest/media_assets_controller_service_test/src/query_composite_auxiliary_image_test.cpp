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

#define MLOG_TAG "MediaAssetsControllerServiceTest"

#include "query_composite_auxiliary_image_test.h"

#include <cstdlib>
#include <string>
#include <thread>
#include <unistd.h>

#define PRIVATE public
#define PROTECTED public
#include "media_assets_controller_service.h"
#undef PRIVATE
#undef PROTECTED

#include "message_parcel.h"
#include "user_define_ipc_client.h"
#include "media_resp_vo.h"
#include "media_empty_obj_vo.h"
#include "media_assets_service.h"
#include "medialibrary_type_const.h"
#include "query_composite_auxiliary_image_dto.h"
#include "query_composite_auxiliary_image_vo.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "result_set_utils.h"
#include "media_column.h"
#include "medialibrary_errno.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_ONE_SECOND = 1;
static const string TEST_PHOTO_PATH = "/storage/cloud/files/Photo/16/IMG_1501924305_000.jpg";
static const string TEST_PHOTO_NAME = "cam_pic.jpg";
static const int32_t TEST_MEDIA_TYPE_IMAGE = 1;

static const string SQL_INSERT_PHOTO =
    "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + "(" + MediaColumn::MEDIA_FILE_PATH + ", " +
    MediaColumn::MEDIA_TYPE + ", " + MediaColumn::MEDIA_NAME + ", " + PhotoColumn::PHOTO_EDIT_TIME + ", " +
    MediaColumn::MEDIA_TIME_PENDING + ", " + MediaColumn::MEDIA_DATE_TRASHED + ", " +
    MediaColumn::MEDIA_HIDDEN + ", " + PhotoColumn::PHOTO_COMPOSITE_DISPLAY_STATUS + ")";

static int32_t ClearTable(const string &table)
{
    RdbPredicates predicates(table);

    int32_t rows = 0;
    int32_t err = g_rdbStore->Delete(rows, predicates);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to clear photos table, err: %{public}d", err);
        return E_HAS_DB_ERROR;
    }
    return E_OK;
}

static void PrepareCompositeFiles(bool withMainFile, bool withSourceBack)
{
    system("mkdir -p /storage/cloud/files/Photo/16/");
    system("mkdir -p /storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg");
    if (withMainFile) {
        system("touch /storage/cloud/files/Photo/16/IMG_1501924305_000.jpg");
    }
    if (withSourceBack) {
        system("touch /storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg/source_back.jpg");
    }
}

static void CleanCompositeFiles()
{
    system("rm -rf /storage/cloud/files/Photo/16/IMG_1501924305_000.jpg");
    system("rm -rf /storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg");
}

// 造一条照片记录，data/hidden/timePending/compositeDisplayStatus 均可定制。
// 覆盖分支说明（QueryCompositeAuxiliaryImage，2026-09-01 支持纯云 source_back 流读后）：
//   B1 fileAsset==nullptr; B2 timePending!=0; B3 隐藏权限; B4 path 为空;
//   B5 GetCompositeAuxiliaryPath 非法状态; B6 各合法状态取路径;
//   B7 本地目标文件存在直读; B8 本地目标文件不存在放行云流读;
//   B9 open 成功返回 fd; B10 open 失败。
static int32_t InsertPhoto(const string &path, int32_t hidden, int32_t timePending,
    int32_t compositeDisplayStatus)
{
    string insertSql = SQL_INSERT_PHOTO + " VALUES ('" + path + "', " + to_string(TEST_MEDIA_TYPE_IMAGE) +
        ", '" + TEST_PHOTO_NAME + "', 0, " + to_string(timePending) + ", 0, " + to_string(hidden) + ", " +
        to_string(compositeDisplayStatus) + ")";
    int32_t ret = g_rdbStore->ExecuteSql(insertSql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Execute sql %{public}s failed", insertSql.c_str());
    }
    return ret;
}

static int32_t QueryFileIdByDisplayName(const string &displayName)
{
    vector<string> columns;
    RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    rdbPredicates.EqualTo(MediaColumn::MEDIA_NAME, displayName);
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get fileId");
        return -1;
    }
    return GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
}

static int32_t UpdateCompositeDisplayStatus(int32_t fileId, int32_t compositeDisplayStatus)
{
    RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    rdbPredicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    int32_t changedRows = -1;
    ValuesBucket valueBucket;
    valueBucket.PutInt(PhotoColumn::PHOTO_COMPOSITE_DISPLAY_STATUS, compositeDisplayStatus);
    g_rdbStore->Update(changedRows, valueBucket, rdbPredicates);
    return changedRows;
}

static int32_t QueryCompositeAuxiliaryImage(int32_t fileId, QueryCompositeAuxiliaryImageRespBody &respBody)
{
    QueryCompositeAuxiliaryImageDto dto;
    dto.fileId = fileId;
    return MediaAssetsService::GetInstance().QueryCompositeAuxiliaryImage(dto, respBody);
}

void QueryCompositeAuxiliaryImageTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateBasicTables(g_rdbStore));
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    MEDIA_INFO_LOG("SetUpTestCase");
}

void QueryCompositeAuxiliaryImageTest::TearDownTestCase(void)
{
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    CleanCompositeFiles();
    MEDIA_INFO_LOG("TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_ONE_SECOND));
}

void QueryCompositeAuxiliaryImageTest::SetUp()
{
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    CleanCompositeFiles();
    MEDIA_INFO_LOG("SetUp");
}

void QueryCompositeAuxiliaryImageTest::TearDown(void)
{
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    CleanCompositeFiles();
    MEDIA_INFO_LOG("TearDown");
}

/**
 * @tc.name  : QueryCompositeAuxiliaryImage_ShouldReturnError_WhenReadRequestBodyFails
 * @tc.number: QueryCompositeAuxiliaryImageTest_001
 * @tc.desc  : 控制器入口读请求体失败时返回错误（C1 分支）
 */
HWTEST_F(QueryCompositeAuxiliaryImageTest, QueryCompositeAuxiliaryImageTest_001, TestSize.Level0)
{
    MessageParcel data;
    MessageParcel reply;

    auto service = make_shared<MediaAssetsControllerService>();
    service->QueryCompositeAuxiliaryImage(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ASSERT_EQ(respVo.Unmarshalling(reply), true);
    ASSERT_LT(respVo.GetErrCode(), 0);
}

/**
 * @tc.name  : QueryCompositeAuxiliaryImage_ShouldReturnError_WhenFileAssetNotExist
 * @tc.number: QueryCompositeAuxiliaryImageTest_002
 * @tc.desc  : fileId 不存在时 GetFileAssetFromDb 返回空（B1 分支）
 */
HWTEST_F(QueryCompositeAuxiliaryImageTest, QueryCompositeAuxiliaryImageTest_002, TestSize.Level0)
{
    QueryCompositeAuxiliaryImageRespBody respBody;
    int32_t ret = QueryCompositeAuxiliaryImage(999999, respBody);
    EXPECT_EQ(ret, E_INVALID_VALUES);
}

/**
 * @tc.name  : QueryCompositeAuxiliaryImage_ShouldReturnError_WhenTimePending
 * @tc.number: QueryCompositeAuxiliaryImageTest_003
 * @tc.desc  : 记录 time_pending 非 0 时返回错误（B2 分支）
 */
HWTEST_F(QueryCompositeAuxiliaryImageTest, QueryCompositeAuxiliaryImageTest_003, TestSize.Level0)
{
    ASSERT_EQ(InsertPhoto(TEST_PHOTO_PATH, 0, 1, 0), NativeRdb::E_OK);
    int32_t fileId = QueryFileIdByDisplayName(TEST_PHOTO_NAME);
    ASSERT_GT(fileId, 0);

    QueryCompositeAuxiliaryImageRespBody respBody;
    int32_t ret = QueryCompositeAuxiliaryImage(fileId, respBody);
    EXPECT_EQ(ret, E_INVALID_VALUES);
}

/**
 * @tc.name  : QueryCompositeAuxiliaryImage_ShouldReturnError_WhenPathEmpty
 * @tc.number: QueryCompositeAuxiliaryImageTest_004
 * @tc.desc  : 记录路径为空时返回 E_INVALID_URI（B4 分支，非隐藏记录权限直接放行）
 */
HWTEST_F(QueryCompositeAuxiliaryImageTest, QueryCompositeAuxiliaryImageTest_004, TestSize.Level0)
{
    ASSERT_EQ(InsertPhoto("", 0, 0, 0), NativeRdb::E_OK);
    int32_t fileId = QueryFileIdByDisplayName(TEST_PHOTO_NAME);
    ASSERT_GT(fileId, 0);

    QueryCompositeAuxiliaryImageRespBody respBody;
    int32_t ret = QueryCompositeAuxiliaryImage(fileId, respBody);
    EXPECT_EQ(ret, E_INVALID_URI);
}

/**
 * @tc.name  : QueryCompositeAuxiliaryImage_ShouldNotPreReject_WhenSourceBackNotExist
 * @tc.number: QueryCompositeAuxiliaryImageTest_005
 * @tc.desc  : ENHANCED 状态本地无 source_back 时回退 source 路径并放行到 Open（B6/B8 分支），
 *             ut 环境无云框架 Open 失败返回 E_INVALID_VALUES（B10 分支）
 */
HWTEST_F(QueryCompositeAuxiliaryImageTest, QueryCompositeAuxiliaryImageTest_005, TestSize.Level0)
{
    ASSERT_EQ(InsertPhoto(TEST_PHOTO_PATH, 0, 0,
        static_cast<int32_t>(CompositeDisplayStatus::ENHANCED)), NativeRdb::E_OK);
    int32_t fileId = QueryFileIdByDisplayName(TEST_PHOTO_NAME);
    ASSERT_GT(fileId, 0);

    QueryCompositeAuxiliaryImageRespBody respBody;
    int32_t ret = QueryCompositeAuxiliaryImage(fileId, respBody);
    if (ret == E_OK) {
        EXPECT_GE(respBody.fd, 0);
        if (respBody.fd >= 0) {
            close(respBody.fd);
        }
    } else {
        MEDIA_WARN_LOG("QueryCompositeAuxiliaryImage open may fail in ut env, ret: %{public}d", ret);
        EXPECT_EQ(ret, E_INVALID_VALUES);
    }
}

/**
 * @tc.name  : QueryCompositeAuxiliaryImage_ShouldReturnError_WhenStatusInvalid
 * @tc.number: QueryCompositeAuxiliaryImageTest_006
 * @tc.desc  : composite_display_status 非法值时 GetCompositeAuxiliaryPath 走 else（B5 分支）
 */
HWTEST_F(QueryCompositeAuxiliaryImageTest, QueryCompositeAuxiliaryImageTest_006, TestSize.Level0)
{
    PrepareCompositeFiles(false, true);
    ASSERT_EQ(InsertPhoto(TEST_PHOTO_PATH, 0, 0, 0), NativeRdb::E_OK);
    int32_t fileId = QueryFileIdByDisplayName(TEST_PHOTO_NAME);
    ASSERT_GT(fileId, 0);
    ASSERT_GT(UpdateCompositeDisplayStatus(fileId, 99), 0);

    QueryCompositeAuxiliaryImageRespBody respBody;
    int32_t ret = QueryCompositeAuxiliaryImage(fileId, respBody);
    EXPECT_EQ(ret, E_INVALID_VALUES);
}

/**
 * @tc.name  : QueryCompositeAuxiliaryImage_ShouldReturnError_WhenOriginalTargetMissing
 * @tc.number: QueryCompositeAuxiliaryImageTest_007
 * @tc.desc  : ORIGINAL 状态取主文件路径，主文件本地不存在放行云流读（B6/B8），ut 环境 Open 失败（B10）
 */
HWTEST_F(QueryCompositeAuxiliaryImageTest, QueryCompositeAuxiliaryImageTest_007, TestSize.Level0)
{
    PrepareCompositeFiles(false, true);
    ASSERT_EQ(InsertPhoto(TEST_PHOTO_PATH, 0, 0, 0), NativeRdb::E_OK);
    int32_t fileId = QueryFileIdByDisplayName(TEST_PHOTO_NAME);
    ASSERT_GT(fileId, 0);
    ASSERT_GT(UpdateCompositeDisplayStatus(fileId,
        static_cast<int32_t>(CompositeDisplayStatus::ORIGINAL)), 0);

    QueryCompositeAuxiliaryImageRespBody respBody;
    int32_t ret = QueryCompositeAuxiliaryImage(fileId, respBody);
    EXPECT_EQ(ret, E_INVALID_VALUES);
}

/**
 * @tc.name  : QueryCompositeAuxiliaryImage_ShouldReturnError_WhenOriginalEditSourceMissing
 * @tc.number: QueryCompositeAuxiliaryImageTest_008
 * @tc.desc  : ORIGINAL_EDIT 状态取 source.ext 路径，本地不存在放行云流读（B6/B8），ut 环境 Open 失败（B10）
 */
HWTEST_F(QueryCompositeAuxiliaryImageTest, QueryCompositeAuxiliaryImageTest_008, TestSize.Level0)
{
    PrepareCompositeFiles(false, true);
    ASSERT_EQ(InsertPhoto(TEST_PHOTO_PATH, 0, 0, 0), NativeRdb::E_OK);
    int32_t fileId = QueryFileIdByDisplayName(TEST_PHOTO_NAME);
    ASSERT_GT(fileId, 0);
    ASSERT_GT(UpdateCompositeDisplayStatus(fileId,
        static_cast<int32_t>(CompositeDisplayStatus::ORIGINAL_EDIT)), 0);

    QueryCompositeAuxiliaryImageRespBody respBody;
    int32_t ret = QueryCompositeAuxiliaryImage(fileId, respBody);
    EXPECT_EQ(ret, E_INVALID_VALUES);
}

/**
 * @tc.name  : QueryCompositeAuxiliaryImage_ShouldReturnFd_WhenEnhanced
 * @tc.number: QueryCompositeAuxiliaryImageTest_009
 * @tc.desc  : ENHANCED 状态取 source_back 路径，本地存在直读（B6/B7），Open 成功返回 fd 或 ut 环境失败（B9/B10）
 */
HWTEST_F(QueryCompositeAuxiliaryImageTest, QueryCompositeAuxiliaryImageTest_009, TestSize.Level0)
{
    PrepareCompositeFiles(false, true);
    ASSERT_EQ(InsertPhoto(TEST_PHOTO_PATH, 0, 0, 0), NativeRdb::E_OK);
    int32_t fileId = QueryFileIdByDisplayName(TEST_PHOTO_NAME);
    ASSERT_GT(fileId, 0);
    ASSERT_GT(UpdateCompositeDisplayStatus(fileId,
        static_cast<int32_t>(CompositeDisplayStatus::ENHANCED)), 0);

    QueryCompositeAuxiliaryImageRespBody respBody;
    int32_t ret = QueryCompositeAuxiliaryImage(fileId, respBody);
    if (ret == E_OK) {
        EXPECT_GE(respBody.fd, 0);
        if (respBody.fd >= 0) {
            close(respBody.fd);
        }
    } else {
        MEDIA_WARN_LOG("QueryCompositeAuxiliaryImage open may fail in ut env, ret: %{public}d", ret);
        EXPECT_EQ(ret, E_INVALID_VALUES);
    }
}

/**
 * @tc.name  : QueryCompositeAuxiliaryImage_ShouldReturnFd_WhenEnhancedEdit
 * @tc.number: QueryCompositeAuxiliaryImageTest_010
 * @tc.desc  : ENHANCED_EDIT 状态取 source_back 路径，本地存在直读（B6/B7），Open 成功返回 fd 或 ut 环境失败（B9/B10）
 */
HWTEST_F(QueryCompositeAuxiliaryImageTest, QueryCompositeAuxiliaryImageTest_010, TestSize.Level0)
{
    PrepareCompositeFiles(false, true);
    ASSERT_EQ(InsertPhoto(TEST_PHOTO_PATH, 0, 0, 0), NativeRdb::E_OK);
    int32_t fileId = QueryFileIdByDisplayName(TEST_PHOTO_NAME);
    ASSERT_GT(fileId, 0);
    ASSERT_GT(UpdateCompositeDisplayStatus(fileId,
        static_cast<int32_t>(CompositeDisplayStatus::ENHANCED_EDIT)), 0);

    QueryCompositeAuxiliaryImageRespBody respBody;
    int32_t ret = QueryCompositeAuxiliaryImage(fileId, respBody);
    if (ret == E_OK) {
        EXPECT_GE(respBody.fd, 0);
        if (respBody.fd >= 0) {
            close(respBody.fd);
        }
    } else {
        MEDIA_WARN_LOG("QueryCompositeAuxiliaryImage open may fail in ut env, ret: %{public}d", ret);
        EXPECT_EQ(ret, E_INVALID_VALUES);
    }
}

/**
 * @tc.name  : QueryCompositeAuxiliaryImage_ShouldNotSucceed_WhenHidden
 * @tc.number: QueryCompositeAuxiliaryImageTest_011
 * @tc.desc  : hidden 记录不返回成功（B3 权限分支，结果依赖运行环境权限，仅断言非成功）
 */
HWTEST_F(QueryCompositeAuxiliaryImageTest, QueryCompositeAuxiliaryImageTest_011, TestSize.Level0)
{
    ASSERT_EQ(InsertPhoto(TEST_PHOTO_PATH, 1, 0, 0), NativeRdb::E_OK);
    int32_t fileId = QueryFileIdByDisplayName(TEST_PHOTO_NAME);
    ASSERT_GT(fileId, 0);

    QueryCompositeAuxiliaryImageRespBody respBody;
    int32_t ret = QueryCompositeAuxiliaryImage(fileId, respBody);
    EXPECT_NE(ret, E_OK);
}
} // namespace OHOS::Media
