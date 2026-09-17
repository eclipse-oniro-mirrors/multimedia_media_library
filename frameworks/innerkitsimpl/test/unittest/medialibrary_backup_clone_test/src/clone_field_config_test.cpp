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

#include <gtest/gtest.h>
#include <unordered_map>

#include "field_config/clone_field_query.h"
#include "field_config/clone_field_registry.h"
#include "field_config/clone_field_writer.h"
#include "media_audio_column.h"
#include "media_column.h"
#include "photo_album_column.h"
#include "photo_map_column.h"

using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace OHOS::Media;

namespace OHOS {
namespace Media {

class CloneFieldConfigTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        CloneFieldRegistry::Instance().Init();
    }
    static void TearDownTestCase(void) {}
    void SetUp() {}
    void TearDown() {}
};

static std::unordered_map<std::string, std::string> BuildPhotosFakeColumns()
{
    return {
        { MediaColumn::MEDIA_ID, "INTEGER" },
        { MediaColumn::MEDIA_FILE_PATH, "TEXT" },
        { MediaColumn::MEDIA_SIZE, "BIGINT" },
        { PhotoColumn::PHOTO_LATITUDE, "REAL" },
        { PhotoColumn::PHOTO_LONGITUDE, "REAL" },
        { PhotoColumn::PHOTO_CLOUD_ID, "TEXT" },
        { PhotoColumn::PHOTO_POSITION, "INTEGER" },
        { PhotoColumn::UNIQUE_ID, "TEXT" },
        { PhotoColumn::PHOTO_ORIENTATION, "INTEGER" },
    };
}

HWTEST_F(CloneFieldConfigTest, Registry_Init_RegistersCoreTables, TestSize.Level0)
{
    EXPECT_NE(CloneFieldRegistry::Instance().GetTable(PhotoColumn::PHOTOS_TABLE), nullptr);
    EXPECT_NE(CloneFieldRegistry::Instance().GetTable(PhotoAlbumColumns::TABLE), nullptr);
    EXPECT_NE(CloneFieldRegistry::Instance().GetTable(PhotoMap::TABLE), nullptr);
    EXPECT_NE(CloneFieldRegistry::Instance().GetTable(std::string("AnalysisAlbum")), nullptr);
    EXPECT_NE(CloneFieldRegistry::Instance().GetTable(std::string("AnalysisPhotoMap")), nullptr);
    EXPECT_NE(CloneFieldRegistry::Instance().GetTable(AudioColumn::AUDIOS_TABLE), nullptr);
    EXPECT_EQ(CloneFieldRegistry::Instance().GetTable(std::string("NonExistentTable")), nullptr);
}

HWTEST_F(CloneFieldConfigTest, Query_IsNeeded_MapTablesHaveAlbumAndAsset, TestSize.Level0)
{
    EXPECT_TRUE(CloneFieldQuery::IsNeeded(PhotoMap::TABLE, PhotoMap::ALBUM_ID));
    EXPECT_TRUE(CloneFieldQuery::IsNeeded(PhotoMap::TABLE, PhotoMap::ASSET_ID));
    EXPECT_TRUE(CloneFieldQuery::IsNeeded(std::string("AnalysisPhotoMap"), PhotoMap::ALBUM_ID));
    EXPECT_TRUE(CloneFieldQuery::IsNeeded(std::string("AnalysisPhotoMap"), PhotoMap::ASSET_ID));
}

HWTEST_F(CloneFieldConfigTest, Query_GetType_ReturnsDeclaredType, TestSize.Level0)
{
    EXPECT_EQ(CloneFieldQuery::GetType(PhotoColumn::PHOTOS_TABLE, MediaColumn::MEDIA_ID),
        CloneFieldType::INT32);
    EXPECT_EQ(CloneFieldQuery::GetType(PhotoColumn::PHOTOS_TABLE, MediaColumn::MEDIA_FILE_PATH),
        CloneFieldType::STRING);
    EXPECT_EQ(CloneFieldQuery::GetType(PhotoColumn::PHOTOS_TABLE, MediaColumn::MEDIA_SIZE),
        CloneFieldType::INT64);
    EXPECT_EQ(CloneFieldQuery::GetType(PhotoColumn::PHOTOS_TABLE, PhotoColumn::PHOTO_LATITUDE),
        CloneFieldType::DOUBLE);
}

HWTEST_F(CloneFieldConfigTest, Query_IsNeeded_ReflectsNeededMap, TestSize.Level0)
{
    EXPECT_TRUE(CloneFieldQuery::IsNeeded(PhotoColumn::PHOTOS_TABLE, MediaColumn::MEDIA_ID));
    EXPECT_TRUE(CloneFieldQuery::IsNeeded(PhotoColumn::PHOTOS_TABLE, PhotoColumn::PHOTO_ORIENTATION));
    EXPECT_FALSE(CloneFieldQuery::IsNeeded(PhotoColumn::PHOTOS_TABLE, PhotoColumn::PHOTO_LATITUDE));
    EXPECT_FALSE(CloneFieldQuery::IsNeeded(PhotoColumn::PHOTOS_TABLE, PhotoColumn::PHOTO_CLOUD_ID));
}

HWTEST_F(CloneFieldConfigTest, Query_GetPolicy_PhotosForwardInsert, TestSize.Level0)
{
    EXPECT_EQ(CloneFieldQuery::GetPolicy(PhotoColumn::PHOTOS_TABLE, PhotoColumn::PHOTO_LATITUDE,
        CloneDirection::FORWARD, RecordPath::INSERT), FieldPolicy::INHERIT_SOURCE);
    EXPECT_EQ(CloneFieldQuery::GetPolicy(PhotoColumn::PHOTOS_TABLE, PhotoColumn::PHOTO_CLOUD_ID,
        CloneDirection::FORWARD, RecordPath::INSERT), FieldPolicy::SKIP);
    EXPECT_EQ(CloneFieldQuery::GetPolicy(PhotoColumn::PHOTOS_TABLE, PhotoColumn::UNIQUE_ID,
        CloneDirection::FORWARD, RecordPath::INSERT), FieldPolicy::SKIP);
    EXPECT_EQ(CloneFieldQuery::GetPolicy(PhotoColumn::PHOTOS_TABLE, PhotoColumn::PHOTO_CLOUD_ID,
        CloneDirection::REVERSE, RecordPath::INSERT), FieldPolicy::INHERIT_SOURCE);
    EXPECT_EQ(CloneFieldQuery::GetPolicy(PhotoColumn::PHOTOS_TABLE, PhotoColumn::UNIQUE_ID,
        CloneDirection::REVERSE, RecordPath::INSERT), FieldPolicy::SKIP);
}

HWTEST_F(CloneFieldConfigTest, Query_GetPolicy_PhotosMergeDefaultsKeepTarget, TestSize.Level0)
{
    EXPECT_EQ(CloneFieldQuery::GetPolicy(PhotoColumn::PHOTOS_TABLE, PhotoColumn::PHOTO_LATITUDE,
        CloneDirection::FORWARD, RecordPath::MERGE), FieldPolicy::KEEP_TARGET);
    EXPECT_EQ(CloneFieldQuery::GetPolicy(PhotoColumn::PHOTOS_TABLE, PhotoColumn::PHOTO_LATITUDE,
        CloneDirection::REVERSE, RecordPath::MERGE), FieldPolicy::KEEP_TARGET);
}

HWTEST_F(CloneFieldConfigTest, Query_GetWhereClause_PhotosPosition, TestSize.Level0)
{
    std::string noCloud = CloneFieldQuery::GetWhereClause(PhotoColumn::PHOTOS_TABLE,
        PhotoColumn::PHOTO_POSITION, false);
    EXPECT_NE(noCloud.find("IN (1, 3)"), std::string::npos);
    std::string withCloud = CloneFieldQuery::GetWhereClause(PhotoColumn::PHOTOS_TABLE,
        PhotoColumn::PHOTO_POSITION, true);
    EXPECT_NE(withCloud.find("IN (1, 2, 3)"), std::string::npos);
    EXPECT_EQ(CloneFieldQuery::GetWhereClause(PhotoColumn::PHOTOS_TABLE,
        PhotoColumn::PHOTO_CLOUD_ID, false), std::string(""));
}

HWTEST_F(CloneFieldConfigTest, Query_GetCommonColumns_FiltersExcludedAndNeeded, TestSize.Level0)
{
    auto cols = BuildPhotosFakeColumns();
    auto result = CloneFieldQuery::GetCommonColumns(PhotoColumn::PHOTOS_TABLE, cols, cols,
        CloneDirection::FORWARD, RecordPath::INSERT);
    EXPECT_EQ(result.count(PhotoColumn::PHOTO_CLOUD_ID), 0u);
    EXPECT_EQ(result.count(PhotoColumn::PHOTO_POSITION), 0u);
    EXPECT_EQ(result.count(PhotoColumn::UNIQUE_ID), 0u);
    EXPECT_EQ(result.count(MediaColumn::MEDIA_ID), 0u);
    EXPECT_EQ(result.count(PhotoColumn::PHOTO_ORIENTATION), 0u);
    EXPECT_EQ(result.count(PhotoColumn::PHOTO_LATITUDE), 1u);
    EXPECT_EQ(result.count(PhotoColumn::PHOTO_LONGITUDE), 1u);
    EXPECT_EQ(result.count(MediaColumn::MEDIA_SIZE), 1u);
}

HWTEST_F(CloneFieldConfigTest, Query_GetCommonColumns_ReverseAbsorbsExcluded, TestSize.Level0)
{
    auto cols = BuildPhotosFakeColumns();
    auto result = CloneFieldQuery::GetCommonColumns(PhotoColumn::PHOTOS_TABLE, cols, cols,
        CloneDirection::REVERSE, RecordPath::INSERT);
    EXPECT_EQ(result.count(PhotoColumn::PHOTO_CLOUD_ID), 1u);
    EXPECT_EQ(result.count(PhotoColumn::PHOTO_POSITION), 1u);
    EXPECT_EQ(result.count(PhotoColumn::UNIQUE_ID), 0u);
    EXPECT_EQ(result.count(PhotoColumn::PHOTO_LATITUDE), 1u);
}

HWTEST_F(CloneFieldConfigTest, Query_GetCommonColumns_DropsOnTypeMismatch, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> src = { { PhotoColumn::PHOTO_LATITUDE, "REAL" } };
    std::unordered_map<std::string, std::string> dst = { { PhotoColumn::PHOTO_LATITUDE, "INTEGER" } };
    auto result = CloneFieldQuery::GetCommonColumns(PhotoColumn::PHOTOS_TABLE, src, dst,
        CloneDirection::FORWARD, RecordPath::INSERT);
    EXPECT_EQ(result.count(PhotoColumn::PHOTO_LATITUDE), 0u);
}

HWTEST_F(CloneFieldConfigTest, Query_ApplyDefault_UniqueIdGeneratesValue, TestSize.Level0)
{
    ValuesBucket bucket;
    EXPECT_TRUE(CloneFieldQuery::ApplyDefault(bucket, PhotoColumn::PHOTOS_TABLE,
        PhotoColumn::UNIQUE_ID));
    EXPECT_FALSE(CloneFieldQuery::ApplyDefault(bucket, PhotoColumn::PHOTOS_TABLE,
        PhotoColumn::PHOTO_LATITUDE));
    EXPECT_FALSE(CloneFieldQuery::ApplyDefault(bucket, PhotoColumn::PHOTOS_TABLE,
        MediaColumn::MEDIA_ID));
}

HWTEST_F(CloneFieldConfigTest, Writer_PutIfPresent_NulloptIsNoop, TestSize.Level0)
{
    ValuesBucket bucket;
    std::optional<int32_t> empty;
    CloneFieldWriter::PutIfPresent(bucket, std::string("c"), empty);
    std::optional<int32_t> val(42);
    CloneFieldWriter::PutWithDefault<int32_t>(bucket, std::string("c"), val, 0);
    SUCCEED();
}

HWTEST_F(CloneFieldConfigTest, Writer_PutIfInIntersection_GatedByMembership, TestSize.Level0)
{
    ValuesBucket bucket;
    std::unordered_set<std::string> inter = { "a" };
    std::optional<int32_t> val(7);
    CloneFieldWriter::PutIfInIntersection(bucket, std::string("a"), val, inter);
    CloneFieldWriter::PutIfInIntersection(bucket, std::string("b"), val, inter);
    SUCCEED();
}
} // namespace Media
} // namespace OHOS
