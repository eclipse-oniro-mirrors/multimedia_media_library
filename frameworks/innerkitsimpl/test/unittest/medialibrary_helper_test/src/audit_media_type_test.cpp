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

#include "audit_media_type_test.h"

#include "hi_audit.h"
#include "media_log.h"
#include "userfile_manager_types.h"

#include <string>

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {

void AuditMediaTypeTest::SetUpTestCase(void) {}
void AuditMediaTypeTest::TearDownTestCase(void) {}
void AuditMediaTypeTest::SetUp(void) {}
void AuditMediaTypeTest::TearDown(void) {}

/*
 * Feature : AuditMediaTypeTest
 * Function : MediaTypeToString
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : NA
 */
/**
 * @tc.number    : AuditMediaType_ToString_001
 * @tc.name      : AuditMediaType_ToString_001
 * @tc.desc      : 1. MediaTypeToString returns enum name without the MEDIA_TYPE_ prefix
 */
HWTEST_F(AuditMediaTypeTest, AuditMediaType_ToString_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("AuditMediaType_ToString_001 start");
    EXPECT_EQ(MediaTypeToString(static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE)), "IMAGE");
    EXPECT_EQ(MediaTypeToString(static_cast<int32_t>(MediaType::MEDIA_TYPE_VIDEO)), "VIDEO");
    EXPECT_EQ(MediaTypeToString(static_cast<int32_t>(MediaType::MEDIA_TYPE_AUDIO)), "AUDIO");
    EXPECT_EQ(MediaTypeToString(static_cast<int32_t>(MediaType::MEDIA_TYPE_FILE)), "FILE");
    MEDIA_INFO_LOG("AuditMediaType_ToString_001 end");
}

/**
 * @tc.number    : AuditMediaType_ToString_002
 * @tc.name      : AuditMediaType_ToString_002
 * @tc.desc      : 1. MEDIA_TYPE_PHOTO and MEDIA_TYPE_IMAGE map to distinct strings
 */
HWTEST_F(AuditMediaTypeTest, AuditMediaType_ToString_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("AuditMediaType_ToString_002 start");
    std::string photo = MediaTypeToString(static_cast<int32_t>(MediaType::MEDIA_TYPE_PHOTO));
    std::string image = MediaTypeToString(static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE));
    EXPECT_EQ(photo, "PHOTO");
    EXPECT_EQ(image, "IMAGE");
    EXPECT_NE(photo, image);
    MEDIA_INFO_LOG("AuditMediaType_ToString_002 end");
}

/**
 * @tc.number    : AuditMediaType_ToString_003
 * @tc.name      : AuditMediaType_ToString_003
 * @tc.desc      : 1. Out-of-range MediaType values become UNKNOWN
 */
HWTEST_F(AuditMediaTypeTest, AuditMediaType_ToString_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("AuditMediaType_ToString_003 start");
    EXPECT_EQ(MediaTypeToString(-1), "UNKNOWN");
    EXPECT_EQ(MediaTypeToString(99), "UNKNOWN");
    EXPECT_EQ(MediaTypeToString(INT32_MIN), "UNKNOWN");
    MEDIA_INFO_LOG("AuditMediaType_ToString_003 end");
}

/**
 * @tc.number    : AuditLog_MediaTypeColumn_001
 * @tc.name      : AuditLog_MediaTypeColumn_001
 * @tc.desc      : 1. TitleString exposes a mediaType column after albumName
 */
HWTEST_F(AuditMediaTypeTest, AuditLog_MediaTypeColumn_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("AuditLog_MediaTypeColumn_001 start");
    std::string title = AuditLog{}.TitleString();
    EXPECT_NE(title.find("albumName, mediaType"), std::string::npos);
    EXPECT_EQ(title.substr(title.find("mediaType")), std::string("mediaType\n"));
    MEDIA_INFO_LOG("AuditLog_MediaTypeColumn_001 end");
}

/**
 * @tc.number    : AuditLog_MediaTypeColumn_002
 * @tc.name      : AuditLog_MediaTypeColumn_002
 * @tc.desc      : 1. ToString appends the mediaType value at the end when set
 */
HWTEST_F(AuditMediaTypeTest, AuditLog_MediaTypeColumn_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("AuditLog_MediaTypeColumn_002 start");
    AuditLog auditLog = { true, "USER BEHAVIOR", "DELETE", "io", 1, "running", "ok" };
    auditLog.id = "cid";
    auditLog.type = static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE);
    auditLog.displayName = "pic.jpg";
    auditLog.mediaType = MediaTypeToString(auditLog.type);
    std::string row = auditLog.ToString();
    EXPECT_EQ(row.substr(row.rfind(", ")), std::string(", IMAGE"));
    MEDIA_INFO_LOG("AuditLog_MediaTypeColumn_002 end");
}

/**
 * @tc.number    : AuditLog_MediaTypeColumn_003
 * @tc.name      : AuditLog_MediaTypeColumn_003
 * @tc.desc      : 1. When mediaType is empty, ToString still ends with the trailing separator
 */
HWTEST_F(AuditMediaTypeTest, AuditLog_MediaTypeColumn_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("AuditLog_MediaTypeColumn_003 start");
    AuditLog auditLog = { true, "USER BEHAVIOR", "DELETE", "io", 1, "running", "ok" };
    auditLog.albumName = "Holiday";
    EXPECT_TRUE(auditLog.mediaType.empty());
    std::string row = auditLog.ToString();
    EXPECT_EQ(row.substr(row.rfind(", ")), std::string(", "));
    MEDIA_INFO_LOG("AuditLog_MediaTypeColumn_003 end");
}

} // namespace Media
} // namespace OHOS
