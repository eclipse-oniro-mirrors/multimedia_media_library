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

#define MLOG_TAG "CompatibleInfoTest"

#include "compatible_info_test.h"

#include <string>
#include <vector>

#define private public
#define protected public
#include "datashare_result_set.h"
#include "media_assets_controller_service.h"
#include "media_assets_service.h"
#include "rdb_utils.h"
#undef private
#undef protected

#include "compatible_info_vo.h"
#include "user_define_ipc_client.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "result_set_utils.h"
#include "preferred_compatible_mode_check_utils.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace IPC;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_SECONDS = 1;

static int32_t ClearCompatibleInfoTable()
{
    RdbPredicates predicates("CompatibleInfo");
    int32_t rows = 0;
    int32_t err = g_rdbStore->Delete(rows, predicates);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to clear CompatibleInfo table, err: %{public}d", err);
        return E_HAS_DB_ERROR;
    }
    return E_OK;
}

void CompatibleInfoTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start CompatibleInfoTest failed, can not get g_rdbStore");
        exit(1);
    }
    MEDIA_INFO_LOG("CompatibleInfoTest SetUpTestCase");
}

void CompatibleInfoTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("CompatibleInfoTest TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void CompatibleInfoTest::SetUp()
{
    MEDIA_INFO_LOG("CompatibleInfoTest SetUp");
}

void CompatibleInfoTest::TearDown(void)
{
    MEDIA_INFO_LOG("CompatibleInfoTest TearDown");
}

static int32_t SetCompatibleInfo(const string &bundleName, bool highResolution, const vector<string> &mimeTypes)
{
    SetCompatibleInfoReqBody reqBody;
    reqBody.bundleName = bundleName;
    reqBody.supportedHighResolution = highResolution;
    reqBody.supportedMimeTypes = mimeTypes;

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);

    auto service = make_shared<MediaAssetsControllerService>();
    service->SetCompatibleInfo(data, reply);

    MediaRespVo<MediaEmptyObjVo> resp;
    if (resp.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("resp.Unmarshalling failed");
        return -1;
    }

    return resp.GetErrCode();
}

static int32_t GetCompatibleInfo(const string &bundleName, GetCompatibleInfoRespBody &respBody)
{
    GetCompatibleInfoReqBody reqBody;
    reqBody.bundleName = bundleName;

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);

    auto service = make_shared<MediaAssetsControllerService>();
    service->GetCompatibleInfo(data, reply);

    MediaRespVo<GetCompatibleInfoRespBody> resp;
    if (resp.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("resp.Unmarshalling failed");
        return -1;
    }
    respBody = resp.GetBody();
    return resp.GetErrCode();
}

static int32_t GetTranscodeCheckInfo(const string &bundleName, GetTranscodeCheckInfoRespBody &respBody)
{
    GetTranscodeCheckInfoReqBody reqBody;
    reqBody.bundleName = bundleName;

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);

    auto service = make_shared<MediaAssetsControllerService>();
    service->GetTranscodeCheckInfo(data, reply);

    MediaRespVo<GetTranscodeCheckInfoRespBody> resp;
    if (resp.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("resp.Unmarshalling failed");
        return -1;
    }
    respBody = resp.GetBody();
    return resp.GetErrCode();
}

HWTEST_F(CompatibleInfoTest, SetCompatibleInfo_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetCompatibleInfo_001: normal set HEIC+highResolution");
    string bundleName = "com.test.bundle1";
    bool highResolution = true;
    vector<string> mimeTypes = {"image/heic"};

    int32_t ret = SetCompatibleInfo(bundleName, highResolution, mimeTypes);
    EXPECT_EQ(ret, E_SUCCESS);

    GetCompatibleInfoRespBody respBody;
    ret = GetCompatibleInfo(bundleName, respBody);
    EXPECT_EQ(ret, E_SUCCESS);
    EXPECT_EQ(respBody.bundleName, bundleName);
    EXPECT_EQ(respBody.supportedHighResolution, highResolution);
    EXPECT_EQ(respBody.supportedMimeTypes.size(), 1);
    EXPECT_EQ(respBody.supportedMimeTypes[0], "image/heic");
    MEDIA_INFO_LOG("end SetCompatibleInfo_001");
}

HWTEST_F(CompatibleInfoTest, SetCompatibleInfo_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetCompatibleInfo_002: normal set JPEG+lowResolution");
    string bundleName = "com.test.bundle2";
    bool highResolution = false;
    vector<string> mimeTypes = {"image/jpeg"};

    int32_t ret = SetCompatibleInfo(bundleName, highResolution, mimeTypes);
    EXPECT_EQ(ret, E_SUCCESS);

    GetCompatibleInfoRespBody respBody;
    ret = GetCompatibleInfo(bundleName, respBody);
    EXPECT_EQ(ret, E_SUCCESS);
    EXPECT_EQ(respBody.supportedHighResolution, highResolution);
    EXPECT_EQ(respBody.supportedMimeTypes.size(), 1);
    EXPECT_EQ(respBody.supportedMimeTypes[0], "image/jpeg");
    MEDIA_INFO_LOG("end SetCompatibleInfo_002");
}

HWTEST_F(CompatibleInfoTest, SetCompatibleInfo_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetCompatibleInfo_003: set HEIC+JPEG combination");
    string bundleName = "com.test.bundle3";
    bool highResolution = true;
    vector<string> mimeTypes = {"image/heic", "image/jpeg"};

    int32_t ret = SetCompatibleInfo(bundleName, highResolution, mimeTypes);
    EXPECT_EQ(ret, E_SUCCESS);

    GetCompatibleInfoRespBody respBody;
    ret = GetCompatibleInfo(bundleName, respBody);
    EXPECT_EQ(ret, E_SUCCESS);
    EXPECT_EQ(respBody.supportedMimeTypes.size(), 2);
    MEDIA_INFO_LOG("end SetCompatibleInfo_003");
}

HWTEST_F(CompatibleInfoTest, SetCompatibleInfo_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetCompatibleInfo_004: contain invalid MIME types");
    string bundleName = "com.test.bundle4";
    bool highResolution = true;
    vector<string> mimeTypes = {"image/heic", "image/png", "video/mp4"};

    int32_t ret = SetCompatibleInfo(bundleName, highResolution, mimeTypes);
    EXPECT_EQ(ret, E_SUCCESS);

    GetCompatibleInfoRespBody respBody;
    ret = GetCompatibleInfo(bundleName, respBody);
    EXPECT_EQ(ret, E_SUCCESS);
    EXPECT_EQ(respBody.supportedMimeTypes.size(), 1);
    EXPECT_EQ(respBody.supportedMimeTypes[0], "image/heic");
    MEDIA_INFO_LOG("end SetCompatibleInfo_004");
}

HWTEST_F(CompatibleInfoTest, SetCompatibleInfo_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetCompatibleInfo_005: MIME type count exceeds limit");
    string bundleName = "com.test.bundle5";
    bool highResolution = true;
    vector<string> mimeTypes = {"image/heic", "image/jpeg", "image/png"};

    int32_t ret = SetCompatibleInfo(bundleName, highResolution, mimeTypes);
    EXPECT_EQ(ret, E_SUCCESS);
    MEDIA_INFO_LOG("end SetCompatibleInfo_005");
}

HWTEST_F(CompatibleInfoTest, SetCompatibleInfo_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetCompatibleInfo_006: MIME type boundary (2 types)");
    string bundleName = "com.test.bundle6";
    bool highResolution = true;
    vector<string> mimeTypes = {"image/heic", "image/jpeg"};

    int32_t ret = SetCompatibleInfo(bundleName, highResolution, mimeTypes);
    EXPECT_EQ(ret, E_SUCCESS);

    GetCompatibleInfoRespBody respBody;
    ret = GetCompatibleInfo(bundleName, respBody);
    EXPECT_EQ(ret, E_SUCCESS);
    EXPECT_EQ(respBody.supportedMimeTypes.size(), 2);
    MEDIA_INFO_LOG("end SetCompatibleInfo_006");
}

HWTEST_F(CompatibleInfoTest, SetCompatibleInfo_007, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetCompatibleInfo_007: empty bundleName (auto get)");
    bool highResolution = true;
    vector<string> mimeTypes = {"image/heic"};

    int32_t ret = SetCompatibleInfo("", highResolution, mimeTypes);
    EXPECT_EQ(ret, E_INNER_FAIL);
    MEDIA_INFO_LOG("end SetCompatibleInfo_007");
}

HWTEST_F(CompatibleInfoTest, SetCompatibleInfo_008, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetCompatibleInfo_008: set success and verify data");
    string bundleName = "com.test.bundle8";
    bool highResolution = true;
    vector<string> mimeTypes = {"image/heic"};

    int32_t ret = SetCompatibleInfo(bundleName, highResolution, mimeTypes);
    EXPECT_EQ(ret, E_SUCCESS);

    GetCompatibleInfoRespBody respBody;
    ret = GetCompatibleInfo(bundleName, respBody);
    EXPECT_EQ(ret, E_SUCCESS);
    EXPECT_TRUE(respBody.supportedHighResolution);
    MEDIA_INFO_LOG("end SetCompatibleInfo_008");
}

HWTEST_F(CompatibleInfoTest, SetCompatibleInfo_009, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetCompatibleInfo_009: update existing (overwrite)");
    string bundleName = "com.test.bundle9";

    int32_t ret = SetCompatibleInfo(bundleName, true, {"image/heic"});
    EXPECT_EQ(ret, E_SUCCESS);

    ret = SetCompatibleInfo(bundleName, false, {"image/jpeg"});
    EXPECT_EQ(ret, E_SUCCESS);

    GetCompatibleInfoRespBody respBody;
    ret = GetCompatibleInfo(bundleName, respBody);
    EXPECT_EQ(ret, E_SUCCESS);
    EXPECT_FALSE(respBody.supportedHighResolution);
    EXPECT_EQ(respBody.supportedMimeTypes.size(), 1);
    EXPECT_EQ(respBody.supportedMimeTypes[0], "image/jpeg");
    MEDIA_INFO_LOG("end SetCompatibleInfo_009");
}

HWTEST_F(CompatibleInfoTest, SetCompatibleInfo_010, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetCompatibleInfo_010: empty MIME list");
    string bundleName = "com.test.bundle10";
    bool highResolution = true;
    vector<string> mimeTypes = {};

    int32_t ret = SetCompatibleInfo(bundleName, highResolution, mimeTypes);
    EXPECT_EQ(ret, E_SUCCESS);
    MEDIA_INFO_LOG("end SetCompatibleInfo_010");
}

HWTEST_F(CompatibleInfoTest, GetCompatibleInfo_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetCompatibleInfo_001: normal get configured info");
    string bundleName = "com.test.get001";

    int32_t ret = SetCompatibleInfo(bundleName, true, {"image/heic"});
    EXPECT_EQ(ret, E_SUCCESS);

    GetCompatibleInfoRespBody respBody;
    ret = GetCompatibleInfo(bundleName, respBody);
    EXPECT_EQ(ret, E_SUCCESS);
    EXPECT_EQ(respBody.bundleName, bundleName);
    MEDIA_INFO_LOG("end GetCompatibleInfo_001");
}

HWTEST_F(CompatibleInfoTest, GetCompatibleInfo_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetCompatibleInfo_002: get non-existent bundleName");
    string bundleName = "com.test.nonexistent";

    GetCompatibleInfoRespBody respBody;
    int32_t ret = GetCompatibleInfo(bundleName, respBody);
    EXPECT_EQ(ret, E_SUCCESS);
    MEDIA_INFO_LOG("end GetCompatibleInfo_002");
}

HWTEST_F(CompatibleInfoTest, GetCompatibleInfo_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetCompatibleInfo_003: invalid MIME filtered when get");
    string bundleName = "com.test.get003";
    vector<string> mimeTypes = {"image/heic", "image/png", "video/mp4"};

    int32_t ret = SetCompatibleInfo(bundleName, true, mimeTypes);
    EXPECT_EQ(ret, E_SUCCESS);

    GetCompatibleInfoRespBody respBody;
    ret = GetCompatibleInfo(bundleName, respBody);
    EXPECT_EQ(ret, E_SUCCESS);
    EXPECT_EQ(respBody.supportedMimeTypes.size(), 1);
    EXPECT_EQ(respBody.supportedMimeTypes[0], "image/heic");
    MEDIA_INFO_LOG("end GetCompatibleInfo_003");
}

HWTEST_F(CompatibleInfoTest, GetCompatibleInfo_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetCompatibleInfo_004: MIME exceeds limit when get");
    string bundleName = "com.test.get004";
    vector<string> mimeTypes = {"image/heic", "image/jpeg", "image/png", "image/heic"};

    int32_t ret = SetCompatibleInfo(bundleName, true, mimeTypes);
    EXPECT_EQ(ret, E_SUCCESS);
    GetCompatibleInfoRespBody respBody;
    ret = GetCompatibleInfo(bundleName, respBody);
    EXPECT_EQ(ret, E_SUCCESS);
    EXPECT_EQ(respBody.supportedMimeTypes[0], "image/heic");
    MEDIA_INFO_LOG("end GetCompatibleInfo_004");
}

HWTEST_F(CompatibleInfoTest, GetCompatibleInfo_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetCompatibleInfo_005: result MIME empty - can support compatible duplicate");
    string bundleName = "com.test.get005";

    GetCompatibleInfoRespBody respBody;
    int32_t ret = GetCompatibleInfo(bundleName, respBody);
    if (ret == E_SUCCESS) {
        EXPECT_GE(respBody.supportedMimeTypes.size(), 1);
    }
    MEDIA_INFO_LOG("end GetCompatibleInfo_005");
}

HWTEST_F(CompatibleInfoTest, GetCompatibleInfo_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetCompatibleInfo_006: result MIME empty - cannot support compatible duplicate");
    string bundleName = "com.test.get006";

    GetCompatibleInfoRespBody respBody;
    int32_t ret = GetCompatibleInfo(bundleName, respBody);
    if (ret == E_SUCCESS) {
        EXPECT_GE(respBody.supportedMimeTypes.size(), 1);
    }
    MEDIA_INFO_LOG("end GetCompatibleInfo_006");
}

HWTEST_F(CompatibleInfoTest, GetCompatibleInfo_007, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetCompatibleInfo_007: highResolution already set");
    string bundleName = "com.test.get007";
    bool highResolution = true;

    int32_t ret = SetCompatibleInfo(bundleName, highResolution, {"image/heic"});
    EXPECT_EQ(ret, E_SUCCESS);

    GetCompatibleInfoRespBody respBody;
    ret = GetCompatibleInfo(bundleName, respBody);
    EXPECT_EQ(ret, E_SUCCESS);
    EXPECT_EQ(respBody.supportedHighResolution, highResolution);
    MEDIA_INFO_LOG("end GetCompatibleInfo_007");
}

HWTEST_F(CompatibleInfoTest, GetTranscodeCheckInfo_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetTranscodeCheckInfo_001: normal get full transcode info");
    string bundleName = "com.test.transcode001";

    int32_t ret = SetCompatibleInfo(bundleName, true, {"image/heic"});
    EXPECT_EQ(ret, E_SUCCESS);

    GetTranscodeCheckInfoRespBody respBody;
    ret = GetTranscodeCheckInfo(bundleName, respBody);
    EXPECT_EQ(ret, E_SUCCESS);
    EXPECT_EQ(respBody.bundleName, bundleName);
    EXPECT_TRUE(respBody.supportedHighResolution);
    EXPECT_GE(respBody.supportedMimeTypes.size(), 0);
    MEDIA_INFO_LOG("preferredCompatibleMode: %{public}d", respBody.preferredCompatibleMode);
    MEDIA_INFO_LOG("end GetTranscodeCheckInfo_001");
}

HWTEST_F(CompatibleInfoTest, GetTranscodeCheckInfo_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetTranscodeCheckInfo_002: GetCompatibleInfo failed scenario");
    string bundleName = "com.test.transcode002";

    GetTranscodeCheckInfoRespBody respBody;
    int32_t ret = GetTranscodeCheckInfo(bundleName, respBody);
    EXPECT_EQ(ret, E_SUCCESS);
    MEDIA_INFO_LOG("end GetTranscodeCheckInfo_002");
}

HWTEST_F(CompatibleInfoTest, GetTranscodeCheckInfo_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetTranscodeCheckInfo_004: empty bundleName");
    string bundleName = "";

    MessageParcel data;
    MessageParcel reply;
    GetTranscodeCheckInfoReqBody reqBody;
    reqBody.bundleName = bundleName;
    reqBody.Marshalling(data);

    auto service = make_shared<MediaAssetsControllerService>();
    service->GetTranscodeCheckInfo(data, reply);

    MediaRespVo<GetTranscodeCheckInfoRespBody> resp;
    ASSERT_EQ(resp.Unmarshalling(reply), true);
    EXPECT_LT(resp.GetErrCode(), 0);
    MEDIA_INFO_LOG("end GetTranscodeCheckInfo_004");
}

HWTEST_F(CompatibleInfoTest, GetTranscodeCheckInfo_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetTranscodeCheckInfo_005: verify preferredCompatibleMode value");
    string bundleName = "com.test.transcode005";

    int32_t ret = SetCompatibleInfo(bundleName, true, {"image/heic"});
    EXPECT_EQ(ret, E_SUCCESS);

    GetTranscodeCheckInfoRespBody respBody;
    ret = GetTranscodeCheckInfo(bundleName, respBody);
    EXPECT_EQ(ret, E_SUCCESS);
    MEDIA_INFO_LOG("preferredCompatibleMode: %{public}d", respBody.preferredCompatibleMode);
    EXPECT_GE(respBody.preferredCompatibleMode, 0);
    MEDIA_INFO_LOG("end GetTranscodeCheckInfo_005");
}

HWTEST_F(CompatibleInfoTest, GetTranscodeCheckInfo_007, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetTranscodeCheckInfo_007: highResolution + compatible mode combination");
    string bundleName = "com.test.transcode007";
    bool highResolution = false;
    vector<string> mimeTypes = {"image/jpeg"};

    int32_t ret = SetCompatibleInfo(bundleName, highResolution, mimeTypes);
    EXPECT_EQ(ret, E_SUCCESS);

    GetTranscodeCheckInfoRespBody respBody;
    ret = GetTranscodeCheckInfo(bundleName, respBody);
    EXPECT_EQ(ret, E_SUCCESS);
    EXPECT_EQ(respBody.supportedHighResolution, highResolution);
    EXPECT_GE(respBody.supportedMimeTypes.size(), 0);
    MEDIA_INFO_LOG("end GetTranscodeCheckInfo_007");
}
}  // namespace OHOS::Media
