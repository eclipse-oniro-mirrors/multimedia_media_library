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

#include "cover_record_upgrade_test.h"

#include <string>
#include <vector>

#include "media_upgrade.h"
#include "value_object.h"
#include "photo_album_column.h"
#include "upgrade_album_sqls.h"
#include "upgrade_photos_sqls.h"

using namespace OHOS;
using namespace testing::ext;

namespace OHOS {
namespace Media {
void CoverRecordUpgradeTest::SetUpTestCase(void)
{
}

void CoverRecordUpgradeTest::TearDownTestCase(void)
{
}

void CoverRecordUpgradeTest::SetUp()
{
}

void CoverRecordUpgradeTest::TearDown()
{
}

HWTEST_F(CoverRecordUpgradeTest, CreateTabCoverRecordSqlHasAllColumns, TestSize.Level1)
{
    std::string sql = SQL_UPGRADE_CREATE_TAB_COVER_RECORD;
    EXPECT_NE(sql.find("album_type"), std::string::npos);
    EXPECT_NE(sql.find("album_subtype"), std::string::npos);
    EXPECT_NE(sql.find("lpath"), std::string::npos);
    EXPECT_NE(sql.find("cover_order_key"), std::string::npos);
    EXPECT_NE(sql.find("cover_order_subkey"), std::string::npos);
    EXPECT_NE(sql.find("cover_order_type"), std::string::npos);
    EXPECT_NE(sql.find("hidden_cover_order_key"), std::string::npos);
    EXPECT_NE(sql.find("hidden_cover_order_subkey"), std::string::npos);
    EXPECT_NE(sql.find("hidden_cover_order_type"), std::string::npos);
}

HWTEST_F(CoverRecordUpgradeTest, PhotoUpgradeIndexHasDisplayNameFileIdDesc, TestSize.Level1)
{
    EXPECT_NE(PhotoUpgrade::CREATE_SCHPT_HIDDEN_TIME_INDEX.find("display_name DESC"), std::string::npos);
    EXPECT_NE(PhotoUpgrade::CREATE_SCHPT_HIDDEN_TIME_INDEX.find("file_id DESC"), std::string::npos);
    EXPECT_NE(PhotoUpgrade::CREATE_PHOTO_FAVORITE_INDEX.find("display_name DESC"), std::string::npos);
    EXPECT_NE(PhotoUpgrade::CREATE_PHOTO_FAVORITE_INDEX.find("file_id DESC"), std::string::npos);
    EXPECT_NE(PhotoUpgrade::INDEX_SCTHP_PHOTO_DATEADDED.find("display_name DESC"), std::string::npos);
    EXPECT_NE(PhotoUpgrade::INDEX_SCTHP_PHOTO_DATEADDED.find("file_id DESC"), std::string::npos);
    EXPECT_NE(PhotoUpgrade::CREATE_PHOTO_SORT_MEDIA_TYPE_DATE_ADDED_INDEX.find("display_name DESC"),
        std::string::npos);
    EXPECT_NE(PhotoUpgrade::CREATE_PHOTO_SORT_MEDIA_TYPE_DATE_ADDED_INDEX.find("file_id DESC"),
        std::string::npos);
    EXPECT_NE(PhotoUpgrade::CREATE_PHOTO_SORT_IN_ALBUM_DATE_TAKEN_INDEX.find("display_name DESC"),
        std::string::npos);
    EXPECT_NE(PhotoUpgrade::CREATE_PHOTO_SORT_IN_ALBUM_DATE_TAKEN_INDEX.find("date_taken DESC"),
        std::string::npos);
}
} // namespace Media
} // namespace OHOS
