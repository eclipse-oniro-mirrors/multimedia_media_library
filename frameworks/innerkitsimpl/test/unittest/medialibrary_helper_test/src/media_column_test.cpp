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

#include "media_column_test.h"
#include "media_column.h"
#include "media_log.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {

void MediaColumnTest::SetUpTestCase(void) {}
void MediaColumnTest::TearDownTestCase(void) {}
void MediaColumnTest::SetUp(void) {}
void MediaColumnTest::TearDown(void) {}

/*
 * Feature : MediaColumnTest
 * Function : IsPhotoColumn
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : NA
 */
/**
 * @tc.number    : MediaColumn_Test_001
 * @tc.name      : MediaColumn_Test_001
 * @tc.desc      : 1. test IsPhotoColumn with empty string
 *                 2. test IsPhotoColumn with count(*)
 */
HWTEST_F(MediaColumnTest, MediaColumn_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaColumn_Test_001 start");
    std::string columnName = "";
    bool res = PhotoColumn::IsPhotoColumn(columnName);

    columnName = "count(*)";
    res = PhotoColumn::IsPhotoColumn(columnName);
    EXPECT_TRUE(res);
    MEDIA_INFO_LOG("MediaColumn_Test_001 end");
}

/**
 * @tc.number    : MediaColumn_Test_002
 * @tc.name      : MediaColumn_Test_002
 * @tc.desc      : 1. test IsPhotoColumn with orientation column
 */
HWTEST_F(MediaColumnTest, MediaColumn_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaColumn_Test_002 start");
    std::string columnName = "orientation";
    bool res = PhotoColumn::IsPhotoColumn(columnName);
    EXPECT_TRUE(res);
    MEDIA_INFO_LOG("MediaColumn_Test_002 end");
}

/**
 * @tc.number    : MediaColumn_Test_003
 * @tc.name      : MediaColumn_Test_003
 * @tc.desc      : 1. test IsPhotoColumn with data column
 */
HWTEST_F(MediaColumnTest, MediaColumn_Test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaColumn_Test_003 start");
    std::string columnName = "data";
    bool res = PhotoColumn::IsPhotoColumn(columnName);
    EXPECT_TRUE(res);
    MEDIA_INFO_LOG("MediaColumn_Test_003 end");
}

/**
 * @tc.number    : MediaColumn_Test_004
 * @tc.name      : MediaColumn_Test_004
 * @tc.desc      : 1. test IsPhotoColumn with width column
 */
HWTEST_F(MediaColumnTest, MediaColumn_Test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaColumn_Test_004 start");
    std::string columnName = "width";
    bool res = PhotoColumn::IsPhotoColumn(columnName);
    EXPECT_TRUE(res);
    MEDIA_INFO_LOG("MediaColumn_Test_004 end");
}

/**
 * @tc.number    : MediaColumn_Test_005
 * @tc.name      : MediaColumn_Test_005
 * @tc.desc      : 1. test IsPhotoColumn with height column
 */
HWTEST_F(MediaColumnTest, MediaColumn_Test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaColumn_Test_005 start");
    std::string columnName = "height";
    bool res = PhotoColumn::IsPhotoColumn(columnName);
    EXPECT_TRUE(res);
    MEDIA_INFO_LOG("MediaColumn_Test_005 end");
}

/**
 * @tc.number    : MediaColumn_Test_006
 * @tc.name      : MediaColumn_Test_006
 * @tc.desc      : 1. test IsPhotoColumn with date_added column
 */
HWTEST_F(MediaColumnTest, MediaColumn_Test_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaColumn_Test_006 start");
    std::string columnName = "date_added";
    bool res = PhotoColumn::IsPhotoColumn(columnName);
    EXPECT_TRUE(res);
    MEDIA_INFO_LOG("MediaColumn_Test_006 end");
}

/**
 * @tc.number    : MediaColumn_Test_007
 * @tc.name      : MediaColumn_Test_007
 * @tc.desc      : 1. test IsPhotoColumn with invalid column name
 */
HWTEST_F(MediaColumnTest, MediaColumn_Test_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaColumn_Test_007 start");
    std::string columnName = "invalid_column";
    bool res = PhotoColumn::IsPhotoColumn(columnName);
    EXPECT_FALSE(res);
    MEDIA_INFO_LOG("MediaColumn_Test_007 end");
}

/**
 * @tc.number    : MediaColumn_Test_008
 * @tc.name      : MediaColumn_Test_008
 * @tc.desc      : 1. test IsPhotoColumn with date_modified column
 */
HWTEST_F(MediaColumnTest, MediaColumn_Test_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaColumn_Test_008 start");
    std::string columnName = "date_modified";
    bool res = PhotoColumn::IsPhotoColumn(columnName);
    EXPECT_TRUE(res);
    MEDIA_INFO_LOG("MediaColumn_Test_008 end");
}

/**
 * @tc.number    : MediaColumn_Test_009
 * @tc.name      : MediaColumn_Test_009
 * @tc.desc      : 1. test IsPhotoColumn with date_taken column
 */
HWTEST_F(MediaColumnTest, MediaColumn_Test_009, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaColumn_Test_009 start");
    std::string columnName = "date_taken";
    bool res = PhotoColumn::IsPhotoColumn(columnName);
    EXPECT_TRUE(res);
    MEDIA_INFO_LOG("MediaColumn_Test_009 end");
}

/**
 * @tc.number    : MediaColumn_Test_010
 * @tc.name      : MediaColumn_Test_010
 * @tc.desc      : 1. test IsPhotoColumn with orientation column
 */
HWTEST_F(MediaColumnTest, MediaColumn_Test_010, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaColumn_Test_010 start");
    std::string columnName = "orientation";
    bool res = PhotoColumn::IsPhotoColumn(columnName);
    EXPECT_TRUE(res);
    MEDIA_INFO_LOG("MediaColumn_Test_010 end");
}

/**
 * @tc.number    : MediaColumn_Test_011
 * @tc.name      : MediaColumn_Test_011
 * @tc.desc      : 1. test IsPhotoColumn with title column
 */
HWTEST_F(MediaColumnTest, MediaColumn_Test_011, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaColumn_Test_011 start");
    std::string columnName = "title";
    bool res = PhotoColumn::IsPhotoColumn(columnName);
    EXPECT_TRUE(res);
    MEDIA_INFO_LOG("MediaColumn_Test_011 end");
}

/**
 * @tc.number    : MediaColumn_Test_012
 * @tc.name      : MediaColumn_Test_012
 * @tc.desc      : 1. test IsPhotoColumn with media_type column
 */
HWTEST_F(MediaColumnTest, MediaColumn_Test_012, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaColumn_Test_012 start");
    std::string columnName = "media_type";
    bool res = PhotoColumn::IsPhotoColumn(columnName);
    EXPECT_TRUE(res);
    MEDIA_INFO_LOG("MediaColumn_Test_012 end");
}

/**
 * @tc.number    : MediaColumn_Test_013
 * @tc.name      : MediaColumn_Test_013
 * @tc.desc      : 1. test IsPhotoColumn with size column
 */
HWTEST_F(MediaColumnTest, MediaColumn_Test_013, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaColumn_Test_013 start");
    std::string columnName = "size";
    bool res = PhotoColumn::IsPhotoColumn(columnName);
    EXPECT_TRUE(res);
    MEDIA_INFO_LOG("MediaColumn_Test_013 end");
}

/**
 * @tc.number    : MediaColumn_Test_014
 * @tc.name      : MediaColumn_Test_014
 * @tc.desc      : 1. test IsPhotoColumn with duration column
 */
HWTEST_F(MediaColumnTest, MediaColumn_Test_014, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaColumn_Test_014 start");
    std::string columnName = "duration";
    bool res = PhotoColumn::IsPhotoColumn(columnName);
    EXPECT_TRUE(res);
    MEDIA_INFO_LOG("MediaColumn_Test_014 end");
}

/**
 * @tc.number    : MediaColumn_Test_015
 * @tc.name      : MediaColumn_Test_015
 * @tc.desc      : 1. test IsPhotoColumn with relative_path column
 */
HWTEST_F(MediaColumnTest, MediaColumn_Test_015, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaColumn_Test_015 start");
    std::string columnName = "relative_path";
    bool res = PhotoColumn::IsPhotoColumn(columnName);
    EXPECT_TRUE(res);
    MEDIA_INFO_LOG("MediaColumn_Test_015 end");
}

/**
 * @tc.number    : MediaColumn_Test_016
 * @tc.name      : MediaColumn_Test_016
 * @tc.desc      : 1. test IsPhotoColumn with display_name column
 */
HWTEST_F(MediaColumnTest, MediaColumn_Test_016, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaColumn_Test_016 start");
    std::string columnName = "display_name";
    bool res = PhotoColumn::IsPhotoColumn(columnName);
    EXPECT_TRUE(res);
    MEDIA_INFO_LOG("MediaColumn_Test_016 end");
}

/**
 * @tc.number    : MediaColumn_Test_017
 * @tc.name      : MediaColumn_Test_017
 * @tc.desc      : 1. test IsPhotoColumn with file_id column
 */
HWTEST_F(MediaColumnTest, MediaColumn_Test_017, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaColumn_Test_017 start");
    std::string columnName = "file_id";
    bool res = PhotoColumn::IsPhotoColumn(columnName);
    EXPECT_TRUE(res);
    MEDIA_INFO_LOG("MediaColumn_Test_017 end");
}

/**
 * @tc.number    : MediaColumn_Test_018
 * @tc.name      : MediaColumn_Test_018
 * @tc.desc      : 1. test IsPhotoColumn with longitude column
 */
HWTEST_F(MediaColumnTest, MediaColumn_Test_018, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaColumn_Test_018 start");
    std::string columnName = "longitude";
    bool res = PhotoColumn::IsPhotoColumn(columnName);
    EXPECT_TRUE(res);
    MEDIA_INFO_LOG("MediaColumn_Test_018 end");
}

/**
 * @tc.number    : MediaColumn_Test_019
 * @tc.name      : MediaColumn_Test_019
 * @tc.desc      : 1. test IsPhotoColumn with date_added_year column
 */
HWTEST_F(MediaColumnTest, MediaColumn_Test_019, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaColumn_Test_019 start");
    std::string columnName = "date_added_year";
    bool res = PhotoColumn::IsPhotoColumn(columnName);
    EXPECT_TRUE(res);
    MEDIA_INFO_LOG("MediaColumn_Test_019 end");
}

/**
 * @tc.number    : MediaColumn_Test_020
 * @tc.name      : MediaColumn_Test_020
 * @tc.desc      : 1. test IsPhotoColumn with composite_display_status column
 */
HWTEST_F(MediaColumnTest, MediaColumn_Test_020, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaColumn_Test_020 start");
    std::string columnName = "composite_display_status";
    bool res = PhotoColumn::IsPhotoColumn(columnName);
    EXPECT_TRUE(res);
    MEDIA_INFO_LOG("MediaColumn_Test_020 end");
}
} // namespace Media
} // namespace OHOS