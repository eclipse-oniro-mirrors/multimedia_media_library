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

#define MLOG_TAG "MediaCloudSync"

#include "cloud_media_dao_utils_extended_test.h"

#include "media_log.h"
#include "cloud_media_dao_utils.h"
#include "cloud_media_photos_dao.h"
#include "cloud_mdkrecord_photos_vo.h"
#include "photos_po.h"
#include "cloud_media_define.h"
#include "media_string_utils.h"

namespace OHOS::Media::CloudSync {
using namespace testing::ext;

void CloudMediaDaoUtilsExtendedTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("start CloudMediaDaoUtilsExtendedTest::SetUpTestCase");
}

void CloudMediaDaoUtilsExtendedTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("start CloudMediaDaoUtilsExtendedTest::TearDownTestCase");
}

void CloudMediaDaoUtilsExtendedTest::SetUp()
{
    MEDIA_INFO_LOG("setup");
}

void CloudMediaDaoUtilsExtendedTest::TearDown(void) {}

HWTEST_F(CloudMediaDaoUtilsExtendedTest, ToStringWithComma_PathWithBraces, TestSize.Level1)
{
    std::vector<std::string> fileIds;
    fileIds.emplace_back("/test{path}/file{name}");
    std::string result = MediaStringUtils::ToStringWithComma(fileIds);
    EXPECT_EQ(result, "/test{path}/file{name}");
}

HWTEST_F(CloudMediaDaoUtilsExtendedTest, FillParams_PathWithBraces, TestSize.Level1)
{
    std::string sql = "SELECT name FROM Stu WHERE path = {0};";
    std::vector<std::string> bindArgs = {"/test{path}/file{name}"};
    std::string result = MediaStringUtils::FillParams(sql, bindArgs);
    EXPECT_EQ(result, "SELECT name FROM Stu WHERE path = /test{path}/file{name};");
}
HWTEST_F(CloudMediaDaoUtilsExtendedTest, ToStringWithCommaAndQuote_PathWithGreaterThan, TestSize.Level1)
{
    std::vector<std::string> values;
    values.emplace_back("/test>path/file>name");
    std::string result = MediaStringUtils::ToStringWithCommaAndQuote(values);
    EXPECT_EQ(result, "'/test>path/file>name'");
}

HWTEST_F(CloudMediaDaoUtilsExtendedTest, ToStringWithComma_PathWithGreaterThan, TestSize.Level1)
{
    std::vector<std::string> fileIds;
    fileIds.emplace_back("/test>path/file>name");
    std::string result = MediaStringUtils::ToStringWithComma(fileIds);
    EXPECT_EQ(result, "/test>path/file>name");
}

HWTEST_F(CloudMediaDaoUtilsExtendedTest, FillParams_PathWithGreaterThan, TestSize.Level1)
{
    std::string sql = "SELECT name FROM Stu WHERE path = {0};";
    std::vector<std::string> bindArgs = {"/test>path/file>name"};
    std::string result = MediaStringUtils::FillParams(sql, bindArgs);
    EXPECT_EQ(result, "SELECT name FROM Stu WHERE path = /test>path/file>name;");
}
}  // namespace OHOS::Media::CloudSync
