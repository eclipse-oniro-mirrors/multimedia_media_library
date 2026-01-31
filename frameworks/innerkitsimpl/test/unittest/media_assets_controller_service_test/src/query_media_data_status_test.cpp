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

#define MLOG_TAG "MediaAssetsControllerServiceTest"

#include "query_media_data_status_test.h"

#include <string>
#include <vector>

#define private public
#define protected public
#include "media_assets_controller_service.h"
#undef private
#undef protected

#include "query_media_data_status_vo.h"

#include "user_define_ipc_client.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "preferences_helper.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;

void QueryMediaDataStatusTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("SetUpTestCase");
}

void QueryMediaDataStatusTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("TearDownTestCase");
}

void QueryMediaDataStatusTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
}

void QueryMediaDataStatusTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}

static int32_t QueryMediaDataStatus(const string &key, bool& result)
{
    QueryMediaDataStatusReqBody reqBody;
    reqBody.dataKey = key;
    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }
    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->QueryMediaDataStatus(data, reply);

    IPC::MediaRespVo<QueryMediaDataStatusRespBody> respVo;
    if (respVo.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return -1;
    }
    result = respVo.GetBody().result;
    return 0;
}

HWTEST_F(QueryMediaDataStatusTest, QueryMediaDataStatus_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("QueryMediaDataStatus_Test_001 Begin");
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(
            "/data/storage/el2/base/preferences/date_added_date_upgrade.xml", errCode);
    EXPECT_NE(prefs, nullptr);
    const string isFinishedKeyName = "is_task_finished";
    prefs->PutInt(isFinishedKeyName, 1); //task finished
    prefs->FlushSync();
    bool result = false;
    int32_t status = QueryMediaDataStatus("date_added_year", result);
    ASSERT_EQ(status, 0);
    ASSERT_TRUE(result);
    MEDIA_INFO_LOG("QueryMediaDataStatus_Test_001 End");
}
}  // namespace OHOS::Media