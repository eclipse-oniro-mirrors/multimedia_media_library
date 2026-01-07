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

#define MLOG_TAG "MediaPermissionCheckTest"

#include "media_permission_check_test.h"

#include "media_log.h"
#include "media_permission_check.h"
#include "medialibrary_errno.h"
#include "media_file_utils.h"
#include "medialibrary_business_code.h"
#include "get_self_permissions.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "result_set_utils.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace OHOS::Security::AccessToken;

namespace OHOS {
namespace Media {
extern bool (*isCalledBySelfPtr)();
static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_SECONDS = 1;
static const string SQL_INSERT_URIPERMISSION =
    "INSERT INTO UriPermission (target_tokenId, file_id, uri_type, permission_type)";
static const string VALUES_END = ") ";

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

static void InsertUriPermissionRecord(
    const uint32_t &tokenId, const int32_t &fileId, const int32_t &uriType, const int32_t &permissionType)
{
    std::string insertSql = SQL_INSERT_URIPERMISSION + " VALUES (" + to_string(tokenId) + "," + to_string(fileId) +
                            "," + to_string(uriType) + "," + to_string(permissionType) + VALUES_END;
    int32_t ret = g_rdbStore->ExecuteSql(insertSql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Execute sql %{public}s failed", insertSql.c_str());
        return;
    }
    MEDIA_INFO_LOG("Execute sql %{public}s success", insertSql.c_str());
}

std::unordered_map<uint32_t, std::vector<std::vector<PermissionType>>>
    MediaPermissionCheckTest::originalBusinessCodeToPermissions;

void MediaPermissionCheckTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start failed, can not get g_rdbStore");
        exit(1);
    }
    ClearTable("UriPermission");
}

void MediaPermissionCheckTest::TearDownTestCase(void)
{
    ClearTable("UriPermission");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

bool mockIsCalledBySelf()
{
    return E_FAIL;
}

pid_t mockGetCallingUid()
{
    return 123456;
}

pid_t mockGetCallingUidShell()
{
    return 2000;
}

void MediaPermissionCheckTest::SetUp(void)
{
    isCalledBySelfPtr = mockIsCalledBySelf;
    getCallingUidPtr = mockGetCallingUid;
    ClearTable("UriPermission");
}

void MediaPermissionCheckTest::TearDown(void)
{
    isCalledBySelfPtr = MediaFileUtils::IsCalledBySelf;
    getCallingUidPtr = IPCSkeleton::GetCallingUid;
}

static std::unordered_map<uint32_t, std::vector<std::vector<PermissionType>>> testMap = {
    {1, {{PRIVATE_PERM}}},
    {2, {{CLOUDFILE_SYNC}}},
    {3, {{READ_PERM}}},
    {4, {{WRITE_PERM}}},
    {5, {{SYSTEMAPI_PERM}, {}}},
    {6, {{}, {SYSTEMAPI_PERM}}},
    {7, {}},
    {8, {{PRIVATE_PERM, CLOUDFILE_SYNC, READ_PERM, WRITE_PERM}}},
    {9, {{CLOUD_READ}, {CLOUD_WRITE}}},
    {0, {{READ_PERM}, {WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_OPEN), {{READ_PERM, WRITE_PERM}}},  // openfile api
    {11, {{SYSTEMINNERAPI_PERM}}},
};
static int32_t GetTestPermissionPolicy(uint32_t code, std::vector<std::vector<PermissionType>> &permissionPolicy)
{
    auto it = testMap.find(code);
    if (it != testMap.end()) {
        permissionPolicy = it->second;
        return E_SUCCESS;
    }
    return E_FAIL;
}

static int32_t PreparePermissionParam(uint32_t code, int32_t userId, bool isDBBypass,
    std::unordered_map<std::string, std::string> &headerMap, PermissionHeaderReq &data)
{
    std::vector<std::vector<PermissionType>> permissionPolicy;
    if (GetTestPermissionPolicy(code, permissionPolicy) != E_SUCCESS) {
        return E_FAIL;
    }
    data = PermissionHeaderReq::convertToPermissionHeaderReq(headerMap, userId, permissionPolicy, isDBBypass);
    return E_SUCCESS;
}

HWTEST_F(MediaPermissionCheckTest, MediaPermissionCheckTest_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("MediaPermissionCheckTest_001 begin");
    vector<string> perms;
    perms.push_back("ohos.permission.MEDIA_LOCATION");
    uint64_t tokenId = 0;
    PermissionUtilsUnitTest::SetAccessTokenPermission("MediaPermissionCheckTest_001", perms, tokenId);
    ASSERT_TRUE(tokenId != 0);
    MEDIA_INFO_LOG("MediaPermissionCheckTest_001 tokenId=%{public}llu", static_cast<unsigned long long>(tokenId));

    uint32_t businessCode = 1;
    PermissionHeaderReq data;
    std::unordered_map<std::string, std::string> headerMap;
    EXPECT_EQ(PreparePermissionParam(businessCode, -1, false, headerMap, data), E_SUCCESS);
    EXPECT_EQ(PermissionCheck::VerifyPermissions(businessCode, data), E_PERMISSION_DENIED);
    MEDIA_INFO_LOG("MediaPermissionCheckTest_001 end");
}

HWTEST_F(MediaPermissionCheckTest, MediaPermissionCheckTest_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("MediaPermissionCheckTest_002 begin");
    uint32_t businessCode = 2;
    PermissionHeaderReq data;
    std::unordered_map<std::string, std::string> headerMap;
    EXPECT_EQ(PreparePermissionParam(businessCode, -1, false, headerMap, data), E_SUCCESS);
    EXPECT_EQ(PermissionCheck::VerifyPermissions(businessCode, data), E_PERMISSION_DENIED);
    MEDIA_INFO_LOG("MediaPermissionCheckTest_002 end");
}

HWTEST_F(MediaPermissionCheckTest, MediaPermissionCheckTest_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("MediaPermissionCheckTest_003 begin");
    uint32_t businessCode = 3;
    PermissionHeaderReq data;
    std::unordered_map<std::string, std::string> headerMap;
    EXPECT_EQ(PreparePermissionParam(businessCode, -1, false, headerMap, data), E_SUCCESS);
    EXPECT_EQ(PermissionCheck::VerifyPermissions(businessCode, data), E_PERMISSION_DENIED);

    businessCode = 0;
    data = PermissionHeaderReq();
    EXPECT_EQ(PreparePermissionParam(businessCode, -1, false, headerMap, data), E_SUCCESS);
    EXPECT_EQ(PermissionCheck::VerifyPermissions(businessCode, data), E_PERMISSION_DENIED);
    MEDIA_INFO_LOG("MediaPermissionCheckTest_003 end");
}

HWTEST_F(MediaPermissionCheckTest, MediaPermissionCheckTest_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("MediaPermissionCheckTest_004 begin");
    uint32_t businessCode = 4;
    PermissionHeaderReq data;
    std::unordered_map<std::string, std::string> headerMap;
    EXPECT_EQ(PreparePermissionParam(businessCode, -1, false, headerMap, data), E_SUCCESS);
    EXPECT_EQ(PermissionCheck::VerifyPermissions(businessCode, data), E_PERMISSION_DENIED);

    businessCode = 0;
    data = PermissionHeaderReq();
    EXPECT_EQ(PreparePermissionParam(businessCode, -1, false, headerMap, data), E_SUCCESS);
    EXPECT_EQ(PermissionCheck::VerifyPermissions(businessCode, data), E_PERMISSION_DENIED);
    MEDIA_INFO_LOG("MediaPermissionCheckTest_004 end");
}

HWTEST_F(MediaPermissionCheckTest, MediaPermissionCheckTest_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("MediaPermissionCheckTest_005 begin");
    uint32_t businessCode = 5;
    PermissionHeaderReq data;
    std::unordered_map<std::string, std::string> headerMap;
    EXPECT_EQ(PreparePermissionParam(businessCode, -1, false, headerMap, data), E_SUCCESS);
    EXPECT_EQ(PermissionCheck::VerifyPermissions(businessCode, data), -E_CHECK_SYSTEMAPP_FAIL);

    // check db bypass
    uint32_t tokenId = PermissionUtils::GetTokenId();
    int32_t permission_type = 4;
    int32_t file_id = 1;
    int32_t uri_type = 1;
    InsertUriPermissionRecord(tokenId, file_id, uri_type, permission_type);
    businessCode = 5;
    data = PermissionHeaderReq();
    EXPECT_EQ(PreparePermissionParam(businessCode, -1, true, headerMap, data), E_SUCCESS);
    EXPECT_EQ(PermissionCheck::VerifyPermissions(businessCode, data), E_PERMISSION_DB_BYPASS);
    MEDIA_INFO_LOG("MediaPermissionCheckTest_005 end");
}

HWTEST_F(MediaPermissionCheckTest, MediaPermissionCheckTest_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("MediaPermissionCheckTest_006 begin");
    uint32_t businessCode = 6;
    PermissionHeaderReq data;
    std::unordered_map<std::string, std::string> headerMap;
    EXPECT_EQ(PreparePermissionParam(businessCode, -1, false, headerMap, data), E_SUCCESS);
    EXPECT_EQ(PermissionCheck::VerifyPermissions(businessCode, data), -E_CHECK_SYSTEMAPP_FAIL);
    MEDIA_INFO_LOG("MediaPermissionCheckTest_006 end");
}

HWTEST_F(MediaPermissionCheckTest, MediaPermissionCheckTest_007, TestSize.Level0)
{
    MEDIA_INFO_LOG("MediaPermissionCheckTest_007 begin");
    uint32_t businessCode = 7;
    PermissionHeaderReq data;
    std::unordered_map<std::string, std::string> headerMap;
    EXPECT_EQ(PreparePermissionParam(businessCode, -1, false, headerMap, data), E_SUCCESS);
    EXPECT_EQ(PermissionCheck::VerifyPermissions(businessCode, data), E_SUCCESS);
    MEDIA_INFO_LOG("MediaPermissionCheckTest_007 end");
}

HWTEST_F(MediaPermissionCheckTest, MediaPermissionCheckTest_008, TestSize.Level0)
{
    MEDIA_INFO_LOG("MediaPermissionCheckTest_008 begin");
    uint32_t businessCode = 8;
    PermissionHeaderReq data;
    std::unordered_map<std::string, std::string> headerMap;
    EXPECT_EQ(PreparePermissionParam(businessCode, -1, false, headerMap, data), E_SUCCESS);
    EXPECT_EQ(PermissionCheck::VerifyPermissions(businessCode, data), E_PERMISSION_DENIED);
    MEDIA_INFO_LOG("MediaPermissionCheckTest_008 end");
}

HWTEST_F(MediaPermissionCheckTest, MediaPermissionCheckTest_009, TestSize.Level0)
{
    MEDIA_INFO_LOG("MediaPermissionCheckTest_009 begin");
    // invalid fileId
    std::unordered_map<std::string, std::string> headerMap = {
        {PermissionHeaderReq::FILE_ID_KEY, ""},
        {PermissionHeaderReq::URI_TYPE_KEY, "2"},
    };
    uint32_t businessCode = 3;
    PermissionHeaderReq data;
    EXPECT_EQ(PreparePermissionParam(businessCode, -1, false, headerMap, data), E_SUCCESS);
    EXPECT_EQ(PermissionCheck::VerifyPermissions(businessCode, data), E_PERMISSION_DENIED);

    // invalid uriType
    headerMap[PermissionHeaderReq::URI_TYPE_KEY] = "str";
    data = PermissionHeaderReq();
    EXPECT_EQ(PreparePermissionParam(businessCode, -1, false, headerMap, data), E_SUCCESS);
    EXPECT_EQ(PermissionCheck::VerifyPermissions(businessCode, data), E_PERMISSION_DENIED);
    MEDIA_INFO_LOG("MediaPermissionCheckTest_009 end");
}

HWTEST_F(MediaPermissionCheckTest, MediaPermissionCheckTest_010, TestSize.Level0)
{
    MEDIA_INFO_LOG("MediaPermissionCheckTest_010 begin");
    std::unordered_map<std::string, std::string> headerMap = {
        {PermissionHeaderReq::FILE_ID_KEY, "1"},
        {PermissionHeaderReq::URI_TYPE_KEY, "2"},
    };
    uint32_t businessCode = 3;
    PermissionHeaderReq data;
    EXPECT_EQ(PreparePermissionParam(businessCode, -1, false, headerMap, data), E_SUCCESS);
    EXPECT_EQ(PermissionCheck::VerifyPermissions(businessCode, data), E_PERMISSION_DENIED);
    MEDIA_INFO_LOG("MediaPermissionCheckTest_010 end");
}

HWTEST_F(MediaPermissionCheckTest, MediaPermissionCheckTest_011, TestSize.Level0)
{
    MEDIA_INFO_LOG("MediaPermissionCheckTest_011 begin");
    std::unordered_map<std::string, std::string> headerMap = {
        {PermissionHeaderReq::FILE_ID_KEY, "1"},
        {PermissionHeaderReq::URI_TYPE_KEY, "2"},
    };
    uint32_t businessCode = 3;
    PermissionHeaderReq data;
    EXPECT_EQ(PreparePermissionParam(businessCode, 100, false, headerMap, data), E_SUCCESS);
    EXPECT_EQ(PermissionCheck::VerifyPermissions(businessCode, data), E_PERMISSION_DENIED);

    businessCode = 4;
    data = PermissionHeaderReq();
    EXPECT_EQ(PreparePermissionParam(businessCode, 100, false, headerMap, data), E_SUCCESS);
    EXPECT_EQ(PermissionCheck::VerifyPermissions(businessCode, data), E_PERMISSION_DENIED);
    MEDIA_INFO_LOG("MediaPermissionCheckTest_011 end");
}

HWTEST_F(MediaPermissionCheckTest, MediaPermissionCheckTest_012, TestSize.Level0)
{
    MEDIA_INFO_LOG("MediaPermissionCheckTest_012 begin");
    // invalid api code
    uint32_t businessCode = 1000000;
    PermissionHeaderReq data;
    std::unordered_map<std::string, std::string> headerMap;
    EXPECT_EQ(PreparePermissionParam(businessCode, 100, false, headerMap, data), E_FAIL);
    MEDIA_INFO_LOG("MediaPermissionCheckTest_012 end");
}

HWTEST_F(MediaPermissionCheckTest, MediaPermissionCheckTest_013, TestSize.Level0)
{
    MEDIA_INFO_LOG("MediaPermissionCheckTest_013 begin");
    // invalid map
    std::unordered_map<std::string, std::string> headerMap = {
        {PermissionHeaderReq::FILE_ID_KEY, "1"},
    };
    uint32_t businessCode = 3;
    isCalledBySelfPtr = MediaFileUtils::IsCalledBySelf;
    PermissionHeaderReq data;
    EXPECT_EQ(PreparePermissionParam(businessCode, -1, false, headerMap, data), E_SUCCESS);
    EXPECT_EQ(PermissionCheck::VerifyPermissions(businessCode, data), E_SUCCESS);
    MEDIA_INFO_LOG("MediaPermissionCheckTest_013 end");
}

HWTEST_F(MediaPermissionCheckTest, MediaPermissionCheckTest_014, TestSize.Level0)
{
    MEDIA_INFO_LOG("MediaPermissionCheckTest_014 begin");

    uint32_t businessCode = 1;
    PermissionHeaderReq data;
    std::unordered_map<std::string, std::string> headerMap;
    EXPECT_EQ(PreparePermissionParam(businessCode, -1, false, headerMap, data), E_SUCCESS);
    EXPECT_EQ(PermissionCheck::VerifyPermissions(businessCode, data), E_PERMISSION_DENIED);

    businessCode = 2;
    data = PermissionHeaderReq();
    EXPECT_EQ(PreparePermissionParam(businessCode, -1, false, headerMap, data), E_SUCCESS);
    EXPECT_EQ(PermissionCheck::VerifyPermissions(businessCode, data), E_PERMISSION_DENIED);

    businessCode = 4;
    data = PermissionHeaderReq();
    EXPECT_EQ(PreparePermissionParam(businessCode, -1, false, headerMap, data), E_SUCCESS);
    EXPECT_EQ(PermissionCheck::VerifyPermissions(businessCode, data), E_PERMISSION_DENIED);
    MEDIA_INFO_LOG("MediaPermissionCheckTest_014 end");
}

HWTEST_F(MediaPermissionCheckTest, MediaPermissionCheckTest_015, TestSize.Level0)
{
    MEDIA_INFO_LOG("MediaPermissionCheckTest_015 begin");
    getCallingUidPtr = IPCSkeleton::GetCallingUid;

    uint32_t businessCode = 3;
    PermissionHeaderReq data;
    std::unordered_map<std::string, std::string> headerMap;
    EXPECT_EQ(PreparePermissionParam(businessCode, -1, false, headerMap, data), E_SUCCESS);
    EXPECT_EQ(PermissionCheck::VerifyPermissions(businessCode, data), E_PERMISSION_DENIED);

    isCalledBySelfPtr = MediaFileUtils::IsCalledBySelf;
    businessCode = 3;
    data = PermissionHeaderReq();
    EXPECT_EQ(PreparePermissionParam(businessCode, -1, false, headerMap, data), E_SUCCESS);
    EXPECT_EQ(PermissionCheck::VerifyPermissions(businessCode, data), E_SUCCESS);
    MEDIA_INFO_LOG("MediaPermissionCheckTest_015 end");
}

HWTEST_F(MediaPermissionCheckTest, MediaPermissionCheckTest_016, TestSize.Level0)
{
    MEDIA_INFO_LOG("MediaPermissionCheckTest_016 begin");
    getCallingUidPtr = IPCSkeleton::GetCallingUid;

    uint32_t businessCode = 4;
    PermissionHeaderReq data;
    std::unordered_map<std::string, std::string> headerMap;
    EXPECT_EQ(PreparePermissionParam(businessCode, -1, false, headerMap, data), E_SUCCESS);
    EXPECT_EQ(PermissionCheck::VerifyPermissions(businessCode, data), E_PERMISSION_DENIED);

    isCalledBySelfPtr = MediaFileUtils::IsCalledBySelf;
    businessCode = 4;
    data = PermissionHeaderReq();
    EXPECT_EQ(PreparePermissionParam(businessCode, -1, false, headerMap, data), E_SUCCESS);
    EXPECT_EQ(PermissionCheck::VerifyPermissions(businessCode, data), E_SUCCESS);
    MEDIA_INFO_LOG("MediaPermissionCheckTest_016 end");
}

HWTEST_F(MediaPermissionCheckTest, MediaPermissionCheckTest_017, TestSize.Level0)
{
    MEDIA_INFO_LOG("MediaPermissionCheckTest_017 begin");
    uint32_t tokenId = PermissionUtils::GetTokenId();
    int32_t permission_type = 4;
    int32_t file_id = 1;
    int32_t uri_type = 1;
    InsertUriPermissionRecord(tokenId, file_id, uri_type, permission_type);
    std::unordered_map<std::string, std::string> headerMap = {
        {PermissionHeaderReq::FILE_ID_KEY, to_string(file_id)},
        {PermissionHeaderReq::URI_TYPE_KEY, to_string(uri_type)},
    };

    uint32_t businessCode = 3;
    PermissionHeaderReq data;
    EXPECT_EQ(PreparePermissionParam(businessCode, -1, false, headerMap, data), E_SUCCESS);
    EXPECT_EQ(PermissionCheck::VerifyPermissions(businessCode, data), E_DOUBLE_CHECK);
    MEDIA_INFO_LOG("MediaPermissionCheckTest_017 end");
}

HWTEST_F(MediaPermissionCheckTest, MediaPermissionCheckTest_018, TestSize.Level0)
{
    MEDIA_INFO_LOG("MediaPermissionCheckTest_018 begin");
    uint32_t businessCode = 9;
    PermissionHeaderReq data;
    std::unordered_map<std::string, std::string> headerMap;
    EXPECT_EQ(PreparePermissionParam(businessCode, -1, false, headerMap, data), E_SUCCESS);
    EXPECT_EQ(PermissionCheck::VerifyPermissions(businessCode, data), E_PERMISSION_DENIED);
    MEDIA_INFO_LOG("MediaPermissionCheckTest_018 end");
}

HWTEST_F(MediaPermissionCheckTest, MediaPermissionCheckTest_019, TestSize.Level0)
{
    MEDIA_INFO_LOG("MediaPermissionCheckTest_019 begin");
    uint32_t businessCode = 11;
    PermissionHeaderReq data;
    std::unordered_map<std::string, std::string> headerMap;
    EXPECT_EQ(PreparePermissionParam(businessCode, -1, false, headerMap, data), E_SUCCESS);
    EXPECT_EQ(PermissionCheck::VerifyPermissions(businessCode, data), E_SUCCESS);
}

HWTEST_F(MediaPermissionCheckTest, MediaPermissionCheckTest_020, TestSize.Level0)
{
    MEDIA_INFO_LOG("MediaPermissionCheckTest_020 begin");
    uint32_t businessCode = 3;
    PermissionHeaderReq data;

    uint32_t tokenId = PermissionUtils::GetTokenId();
    int32_t permission_type = 4;
    int32_t file_id = 1;
    int32_t uri_type = 1;
    InsertUriPermissionRecord(tokenId, file_id, uri_type, permission_type);
    std::unordered_map<std::string, std::string> headerMap = {
        {PermissionHeaderReq::FILE_ID_KEY, to_string(file_id)},
        {PermissionHeaderReq::URI_TYPE_KEY, to_string(uri_type)},
    };
    data = PermissionHeaderReq();
    EXPECT_EQ(PreparePermissionParam(businessCode, -1, false, headerMap, data), E_SUCCESS);
    EXPECT_EQ(PermissionCheck::VerifyPermissions(businessCode, data), E_DOUBLE_CHECK);
    MEDIA_INFO_LOG("MediaPermissionCheckTest_020 end");
}

HWTEST_F(MediaPermissionCheckTest, MediaPermissionCheckTest_OPENFILE, TestSize.Level0)
{
    MEDIA_INFO_LOG("MediaPermissionCheckTest_OPENFILE begin");
    std::unordered_map<std::string, std::string> headerMap = {{PermissionHeaderReq::FILE_ID_KEY, "1"},
        {PermissionHeaderReq::URI_TYPE_KEY, "2"},
        {PermissionHeaderReq::OPEN_URI_KEY, "file://media/Photo/picture/?file_id=1"},
        {PermissionHeaderReq::OPEN_MODE_KEY, "rw"}};

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_OPEN);
    PermissionHeaderReq data;
    EXPECT_EQ(PreparePermissionParam(businessCode, 1, false, headerMap, data), E_SUCCESS);
    EXPECT_EQ(PermissionCheck::VerifyPermissions(businessCode, data), E_PERMISSION_DENIED);
    MEDIA_INFO_LOG("MediaPermissionCheckTest_OPENFILE end");
}

HWTEST_F(MediaPermissionCheckTest, MediaPermissionCheckTest_OPENFILE_2, TestSize.Level0)
{
    MEDIA_INFO_LOG("MediaPermissionCheckTest_OPENFILE_2 begin");
    std::unordered_map<std::string, std::string> headerMap = {
        {PermissionHeaderReq::OPEN_URI_KEY,
            "file://media/Photo/6/1/1.jpg?operation=thumbnail&width=720&height=720&path=/storage/cloud/files/Photo/1/"
            "1.jpg"},
        {PermissionHeaderReq::OPEN_MODE_KEY, "r"}};
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_OPEN);
    PermissionHeaderReq data;
    EXPECT_EQ(PreparePermissionParam(businessCode, 1, false, headerMap, data), E_SUCCESS);
    EXPECT_EQ(PermissionCheck::VerifyPermissions(businessCode, data), E_PERMISSION_DENIED);
    MEDIA_INFO_LOG("MediaPermissionCheckTest_OPENFILE_2 end");
}

HWTEST_F(MediaPermissionCheckTest, MediaPermissionCheckTest_OPENFILE_3, TestSize.Level0)
{
    MEDIA_INFO_LOG("MediaPermissionCheckTest_OPENFILE_3 begin");
    std::unordered_map<std::string, std::string> headerMap = {
        {PermissionHeaderReq::OPEN_URI_KEY, "file://media/Photo/picture/?file_id=1"},
        {PermissionHeaderReq::OPEN_MODE_KEY, "rw"}};
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_OPEN);
    PermissionHeaderReq data;
    EXPECT_EQ(PreparePermissionParam(businessCode, 1, false, headerMap, data), E_SUCCESS);
    EXPECT_EQ(PermissionCheck::VerifyPermissions(businessCode, data), E_PERMISSION_DENIED);
    MEDIA_INFO_LOG("MediaPermissionCheckTest_OPENFILE_3 end");
}

HWTEST_F(MediaPermissionCheckTest, MediaPermissionCheckTest_OPENFILE_4, TestSize.Level0)
{
    MEDIA_INFO_LOG("MediaPermissionCheckTest_OPENFILE_4 begin");
    std::unordered_map<std::string, std::string> headerMap = {
        {PermissionHeaderReq::OPEN_URI_KEY, "file:://media/photo_operation/query"},
        {PermissionHeaderReq::OPEN_MODE_KEY, "rw"}};
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_OPEN);
    PermissionHeaderReq data;
    EXPECT_EQ(PreparePermissionParam(businessCode, 1, false, headerMap, data), E_SUCCESS);
    EXPECT_EQ(PermissionCheck::VerifyPermissions(businessCode, data), E_PERMISSION_DENIED);
    MEDIA_INFO_LOG("MediaPermissionCheckTest_OPENFILE_4 end");
}

HWTEST_F(MediaPermissionCheckTest, MediaPermissionCheckTest_OPENFILE_5, TestSize.Level0)
{
    MEDIA_INFO_LOG("MediaPermissionCheckTest_OPENFILE_5 begin");
    std::unordered_map<std::string, std::string> headerMap = {
        {PermissionHeaderReq::OPEN_URI_KEY,
            "file://media/Photo/6/1/1.jpg?operation=astc&width=720&height=720&path=/storage/cloud/files/Photo/1/1.jpg"},
        {PermissionHeaderReq::OPEN_MODE_KEY, "rw"}};
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_OPEN);
    PermissionHeaderReq data;
    EXPECT_EQ(PreparePermissionParam(businessCode, 1, false, headerMap, data), E_SUCCESS);
    EXPECT_EQ(PermissionCheck::VerifyPermissions(businessCode, data), E_PERMISSION_DENIED);
    MEDIA_INFO_LOG("MediaPermissionCheckTest_OPENFILE_5 end");
}

}  // namespace Media
}  // namespace OHOS