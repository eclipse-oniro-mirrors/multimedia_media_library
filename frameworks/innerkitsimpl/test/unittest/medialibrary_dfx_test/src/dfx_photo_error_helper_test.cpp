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
#include <algorithm>
#include <map>
#include <vector>

#include "dfx_photo_error_helper.h"
#include "media_log.h"

using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace {
void ExpectRoundTrip(int32_t fst, int32_t sd, int32_t pos, int32_t mt, PhotoErrorType err)
{
    PhotoErrorDimension dim{fst, sd, pos, mt, err};
    int32_t code = DfxPhotoErrorHelper::EncodePhotoErrorType(dim);
    int32_t normSd = DfxPhotoErrorHelper::NormalizeSouthDeviceType(sd);
    EXPECT_EQ(code / FILE_SOURCE_TYPE_BASE, fst);
    EXPECT_EQ((code / SOUTH_DEVICE_TYPE_BASE) % (FILE_SOURCE_TYPE_BASE / SOUTH_DEVICE_TYPE_BASE), normSd);
    EXPECT_EQ((code / POSITION_BASE) % (SOUTH_DEVICE_TYPE_BASE / POSITION_BASE), pos);
    EXPECT_EQ((code / MEDIA_TYPE_BASE) % (POSITION_BASE / MEDIA_TYPE_BASE), mt);
    EXPECT_EQ(code % MEDIA_TYPE_BASE, static_cast<int32_t>(err));
}
} // namespace

class DfxPhotoErrorHelperTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() {}
    void TearDown() {}
};

HWTEST_F(DfxPhotoErrorHelperTest, normalize_all_dims, TestSize.Level0)
{
    MEDIA_INFO_LOG("normalize: south_device_type visit to null");
    EXPECT_EQ(DfxPhotoErrorHelper::NormalizeSouthDeviceType(-1), 0);
    EXPECT_EQ(DfxPhotoErrorHelper::NormalizeSouthDeviceType(0), 0);
    EXPECT_EQ(DfxPhotoErrorHelper::NormalizeSouthDeviceType(1), 1);
    EXPECT_EQ(DfxPhotoErrorHelper::NormalizeSouthDeviceType(2), 2);
    MEDIA_INFO_LOG("normalize: other dims passthrough");
    EXPECT_EQ(DfxPhotoErrorHelper::NormalizeFileSourceType(3), 3);
    EXPECT_EQ(DfxPhotoErrorHelper::NormalizeMediaType(2), 2);
    EXPECT_EQ(DfxPhotoErrorHelper::NormalizePosition(1), 1);
}

HWTEST_F(DfxPhotoErrorHelperTest, encode_round_trip_all_dimensions, TestSize.Level0)
{
    MEDIA_INFO_LOG("encode round-trip: lake image local missing");
    ExpectRoundTrip(3, 0, 1, 1, PhotoErrorType::FILE_NOT_EXIST);
    MEDIA_INFO_LOG("encode round-trip: min domain");
    ExpectRoundTrip(0, 0, 1, 1, PhotoErrorType::FILE_NOT_EXIST);
    MEDIA_INFO_LOG("encode round-trip: max domain");
    ExpectRoundTrip(3, 2, 3, 2, PhotoErrorType::FILE_LARGER_THAN_DB);
    MEDIA_INFO_LOG("encode round-trip: sd normalization");
    ExpectRoundTrip(1, -1, 1, 2, PhotoErrorType::FILE_NOT_EXIST);
    MEDIA_INFO_LOG("encode round-trip: smaller subtype");
    ExpectRoundTrip(0, 1, 3, 1, PhotoErrorType::FILE_SMALLER_THAN_DB);

    EXPECT_GT(DfxPhotoErrorHelper::EncodePhotoErrorType({0, 0, 1, 1, PhotoErrorType::FILE_NOT_EXIST}), 0);
}

HWTEST_F(DfxPhotoErrorHelperTest, resolve_all_cases, TestSize.Level0)
{
    MEDIA_INFO_LOG("resolve: media uses data");
    EXPECT_EQ(DfxPhotoErrorHelper::ResolveRealPath(0, "/d/a.jpg", ""), "/d/a.jpg");
    EXPECT_EQ(DfxPhotoErrorHelper::ResolveRealPath(0, "/d/a.jpg", "/s/a.jpg"), "/d/a.jpg");
    MEDIA_INFO_LOG("resolve: file_manager and lake use storage_path");
    EXPECT_EQ(DfxPhotoErrorHelper::ResolveRealPath(1, "/d/a.jpg", "/s/a.jpg"), "/s/a.jpg");
    EXPECT_EQ(DfxPhotoErrorHelper::ResolveRealPath(3, "/d/a.jpg", "/s/a.jpg"), "/s/a.jpg");
    MEDIA_INFO_LOG("resolve: empty storage returns empty for file_manager");
    EXPECT_EQ(DfxPhotoErrorHelper::ResolveRealPath(1, "/d/a.jpg", ""), "");
    MEDIA_INFO_LOG("resolve: peripheral falls back to data");
    EXPECT_EQ(DfxPhotoErrorHelper::ResolveRealPath(2, "/d/a.jpg", ""), "/d/a.jpg");
}

HWTEST_F(DfxPhotoErrorHelperTest, classify_all_branches, TestSize.Level0)
{
    MEDIA_INFO_LOG("classify: size zero");
    EXPECT_EQ(DfxPhotoErrorHelper::ClassifyPhotoError(true, 0, 100), PhotoErrorType::FILE_SIZE_ZERO);
    EXPECT_EQ(DfxPhotoErrorHelper::ClassifyPhotoError(true, 0, 0), PhotoErrorType::FILE_SIZE_ZERO);
    MEDIA_INFO_LOG("classify: smaller and larger than db");
    EXPECT_EQ(DfxPhotoErrorHelper::ClassifyPhotoError(true, 50, 100), PhotoErrorType::FILE_SMALLER_THAN_DB);
    EXPECT_EQ(DfxPhotoErrorHelper::ClassifyPhotoError(true, 150, 100), PhotoErrorType::FILE_LARGER_THAN_DB);
    MEDIA_INFO_LOG("classify: missing file");
    EXPECT_EQ(DfxPhotoErrorHelper::ClassifyPhotoError(false, 0, 100), PhotoErrorType::FILE_NOT_EXIST);
    MEDIA_INFO_LOG("classify: consistent");
    EXPECT_EQ(DfxPhotoErrorHelper::ClassifyPhotoError(true, 100, 100), PhotoErrorType::CONSISTENT);
    MEDIA_INFO_LOG("classify: missing precedence over size_zero");
    EXPECT_EQ(DfxPhotoErrorHelper::ClassifyPhotoError(false, 0, 0), PhotoErrorType::FILE_NOT_EXIST);
}

HWTEST_F(DfxPhotoErrorHelperTest, pack_empty_and_single, TestSize.Level0)
{
    MEDIA_INFO_LOG("pack: empty is silent");
    std::map<int32_t, int32_t> empty;
    EXPECT_EQ(DfxPhotoErrorHelper::PackPhotoErrors(empty, 5).size(), 0u);
    MEDIA_INFO_LOG("pack: single entry one event");
    std::map<int32_t, int32_t> m{{10, 5}};
    auto batches = DfxPhotoErrorHelper::PackPhotoErrors(m, 5);
    ASSERT_EQ(batches.size(), 1u);
    ASSERT_EQ(batches[0].photoErrorTypes.size(), 1u);
    EXPECT_EQ(batches[0].photoErrorTypes[0], 10);
    EXPECT_EQ(batches[0].photoErrorCounts[0], 5);
}

HWTEST_F(DfxPhotoErrorHelperTest, pack_five_entries_one_event, TestSize.Level0)
{
    std::map<int32_t, int32_t> m{{10, 1}, {20, 2}, {30, 3}, {40, 4}, {50, 5}};
    auto batches = DfxPhotoErrorHelper::PackPhotoErrors(m, 5);
    ASSERT_EQ(batches.size(), 1u);
    EXPECT_EQ(batches[0].photoErrorTypes.size(), 5u);
    EXPECT_EQ(batches[0].photoErrorTypes[0], 10);
    EXPECT_EQ(batches[0].photoErrorTypes[4], 50);
}

HWTEST_F(DfxPhotoErrorHelperTest, pack_six_entries_two_events, TestSize.Level0)
{
    std::map<int32_t, int32_t> m{{10, 1}, {20, 2}, {30, 3}, {40, 4}, {50, 5}, {60, 6}};
    auto batches = DfxPhotoErrorHelper::PackPhotoErrors(m, 5);
    ASSERT_EQ(batches.size(), 2u);
    EXPECT_EQ(batches[0].photoErrorTypes.size(), 5u);
    EXPECT_EQ(batches[1].photoErrorTypes.size(), 1u);
    EXPECT_EQ(batches[1].photoErrorTypes[0], 60);
}

HWTEST_F(DfxPhotoErrorHelperTest, pack_preserves_ascending_order, TestSize.Level0)
{
    std::map<int32_t, int32_t> m{{300, 7}, {10, 1}, {200, 3}};
    auto batches = DfxPhotoErrorHelper::PackPhotoErrors(m, 5);
    ASSERT_EQ(batches.size(), 1u);
    EXPECT_EQ(batches[0].photoErrorTypes.size(), 3u);
    EXPECT_EQ(batches[0].photoErrorTypes[0], 10);
    EXPECT_EQ(batches[0].photoErrorTypes[1], 200);
    EXPECT_EQ(batches[0].photoErrorTypes[2], 300);
}

HWTEST_F(DfxPhotoErrorHelperTest, pack_respects_custom_batch_size, TestSize.Level0)
{
    std::map<int32_t, int32_t> m{{10, 1}, {20, 2}, {30, 3}};
    auto batches = DfxPhotoErrorHelper::PackPhotoErrors(m, 2);
    ASSERT_EQ(batches.size(), 2u);
    EXPECT_EQ(batches[0].photoErrorTypes.size(), 2u);
    EXPECT_EQ(batches[1].photoErrorTypes.size(), 1u);
}
} // namespace Media
} // namespace OHOS
