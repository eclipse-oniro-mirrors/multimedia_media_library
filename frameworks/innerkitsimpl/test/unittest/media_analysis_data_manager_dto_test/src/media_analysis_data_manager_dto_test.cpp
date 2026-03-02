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

/*
 * NOTE:
 * - This suite focuses on DTOs in media_analysis_data_manager.
 * - Tests are designed to be "pure in-memory": no SA, no DB, no IPC calls.
 * - The goal is to cover concrete branches in DTO implementations (e.g. loops, separators),
 * and validate stability/format constraints for logging & debugging strings.
 */

// NOTE: This file is intended to compile standalone in the OHOS unit-test environment.
// It does not rely on an extra *_test.h header.

#include "gtest/gtest.h"
#include "gtest/hwext/gtest-ext.h"
#include "gtest/hwext/gtest-tag.h"

class MediaAnalysisDataManagerDtoTest : public testing::Test {
public:
    MediaAnalysisDataManagerDtoTest() = default;
    ~MediaAnalysisDataManagerDtoTest() override = default;
};


#include <algorithm>
#include <cctype>
#include <initializer_list>
#include <limits>
#include <map>
#include <numeric>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "dto/get_order_position_dto.h"
#include "dto/start_asset_analysis_dto.h"
#include "dto/change_request_set_order_position_dto.h"

using namespace OHOS::Media;

namespace {
// Tiny helpers to keep assertions readable.
[[maybe_unused]] constexpr const char *K_COMMA_SPACE = ", ";
[[maybe_unused]] constexpr const char *K_JSON_KEY_ALBUM_ID = R"("albumId": )";
[[maybe_unused]] constexpr const char *K_JSON_KEY_ASSET_ID_ARRAY = R"("assetIdArray": )";

static bool Contains(const std::string &s, const std::string &sub)
{
    return s.find(sub) != std::string::npos;
}

static size_t CountSubstr(const std::string &s, const std::string &sub)
{
    if (sub.empty()) {
        return 0U;
    }

    size_t count = 0U;
    for (size_t pos = 0U; (pos = s.find(sub, pos)) != std::string::npos; pos += sub.size()) {
        ++count;
    }
    return count;
}

// Extract the array payload printed by GetOrderPositionDto::ToString().
// Format in current impl: {"albumId": "X", "assetIdArray": " [<payload>]}
static std::string ExtractAssetArrayPayload(const std::string &s)
{
    const std::string kBegin = R"("assetIdArray": " [)";
    const auto begin = s.find(kBegin);
    if (begin == std::string::npos) {
        return "";
    }
    const auto start = begin + kBegin.size();
    // current ToString ends with "]}"
    const auto end = s.rfind("]}");
    if (end == std::string::npos || end < start) {
        return "";
    }
    return s.substr(start, end - start);
}

static std::vector<std::string> SplitByCommaSpace(const std::string &payload)
{
    std::vector<std::string> out;
    if (payload.empty()) {
        return out;
    }
    size_t pos = 0;
    size_t interval = 2;
    while (pos < payload.size()) {
        size_t next = payload.find(", ", pos);
        if (next == std::string::npos) {
            out.emplace_back(payload.substr(pos));
            break;
        }
        out.emplace_back(payload.substr(pos, next - pos));
        pos = next + interval;
    }
    return out;
}

static GetOrderPositionDto MakeDto(int32_t albumId, std::initializer_list<std::string> ids)
{
    GetOrderPositionDto dto;
    dto.albumId = albumId;
    dto.assetIdArray.assign(ids.begin(), ids.end());
    return dto;
}
} // namespace

HWTEST_F(MediaAnalysisDataManagerDtoTest, ToString_001, testing::ext::TestSize.Level0)
{
    auto dto = MakeDto(123, { "a", "b" });
    const std::string s = dto.ToString();

    ASSERT_TRUE(Contains(s, "{"));
    ASSERT_TRUE(Contains(s, R"("albumId": "123")"));
    ASSERT_TRUE(Contains(s, R"("assetIdArray": " [)"));
    ASSERT_TRUE(Contains(s, "]}"));
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, ToString_002, testing::ext::TestSize.Level0)
{
    auto dto = MakeDto(1, {});
    const std::string s = dto.ToString();
    const std::string payload = ExtractAssetArrayPayload(s);

    ASSERT_TRUE(payload.empty());
    ASSERT_EQ(CountSubstr(s, ", "), 1U) << "Only the albumId field should introduce \", \".";
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, ToString_003, testing::ext::TestSize.Level0)
{
    auto dto = MakeDto(2, { "only" });
    const std::string s = dto.ToString();
    const std::string payload = ExtractAssetArrayPayload(s);

    ASSERT_EQ(payload, "only");
    ASSERT_EQ(CountSubstr(payload, ", "), 0U);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, ToString_004, testing::ext::TestSize.Level0)
{
    auto dto = MakeDto(3, { "x", "y" });
    const std::string payload = ExtractAssetArrayPayload(dto.ToString());

    ASSERT_EQ(payload, "x, y");
    ASSERT_EQ(CountSubstr(payload, ", "), 1U);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, ToString_005, testing::ext::TestSize.Level0)
{
    GetOrderPositionDto dto;
    dto.albumId = 1000 + 0;
    dto.assetIdArray.clear();
    dto.assetIdArray.emplace_back("");
    dto.assetIdArray.emplace_back("a");
    dto.assetIdArray.emplace_back("A");
    dto.assetIdArray.emplace_back("0");
    dto.assetIdArray.emplace_back("001");
    const std::string out = dto.ToString();
    const std::string payload = ExtractAssetArrayPayload(out);
    // Split with the exact delimiter used by the implementation.
    const auto tokens = SplitByCommaSpace(payload);
    ASSERT_EQ(tokens.size(), dto.assetIdArray.size());
    for (size_t i = 0; i < tokens.size(); ++i) {
        ASSERT_EQ(tokens[i], dto.assetIdArray[i]) << "index=" << i << ", out=" << out;
    }
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, ToString_007, testing::ext::TestSize.Level0)
{
    GetOrderPositionDto dto;
    dto.albumId = 1000 + 10;
    dto.assetIdArray.clear();
    dto.assetIdArray.emplace_back("\t");
    dto.assetIdArray.emplace_back("\n");
    dto.assetIdArray.emplace_back("\"quote\"");
    dto.assetIdArray.emplace_back("slash/dir");
    dto.assetIdArray.emplace_back("emoji🙂");
    const std::string out = dto.ToString();
    const std::string payload = ExtractAssetArrayPayload(out);
    // Split with the exact delimiter used by the implementation.
    const auto tokens = SplitByCommaSpace(payload);
    ASSERT_EQ(tokens.size(), dto.assetIdArray.size());
    for (size_t i = 0; i < tokens.size(); ++i) {
        ASSERT_EQ(tokens[i], dto.assetIdArray[i]) << "index=" << i << ", out=" << out;
    }
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, ToString_008, testing::ext::TestSize.Level0)
{
    GetOrderPositionDto dto;
    dto.albumId = 1000 + 15;
    dto.assetIdArray.clear();
    dto.assetIdArray.emplace_back("中文");
    dto.assetIdArray.emplace_back("スペース");
    dto.assetIdArray.emplace_back("a\"b");
    dto.assetIdArray.emplace_back("a\\b");
    dto.assetIdArray.emplace_back("[]");
    const std::string out = dto.ToString();
    const std::string payload = ExtractAssetArrayPayload(out);
    // Split with the exact delimiter used by the implementation.
    const auto tokens = SplitByCommaSpace(payload);
    ASSERT_EQ(tokens.size(), dto.assetIdArray.size());
    for (size_t i = 0; i < tokens.size(); ++i) {
        ASSERT_EQ(tokens[i], dto.assetIdArray[i]) << "index=" << i << ", out=" << out;
    }
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, ToString_009, testing::ext::TestSize.Level0)
{
    GetOrderPositionDto dto;
    dto.albumId = 1000 + 20;
    dto.assetIdArray.clear();
    dto.assetIdArray.emplace_back("{}");
    dto.assetIdArray.emplace_back("very_long_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
        "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
        "xxxxxxxxxxxxxxxxxx");
    dto.assetIdArray.emplace_back("dJFCrnl2");
    dto.assetIdArray.emplace_back("dlB");
    dto.assetIdArray.emplace_back("dz1C5Jau2RJtBRn");
    const std::string out = dto.ToString();
    const std::string payload = ExtractAssetArrayPayload(out);
    // Split with the exact delimiter used by the implementation.
    const auto tokens = SplitByCommaSpace(payload);
    ASSERT_EQ(tokens.size(), dto.assetIdArray.size());
    for (size_t i = 0; i < tokens.size(); ++i) {
        ASSERT_EQ(tokens[i], dto.assetIdArray[i]) << "index=" << i << ", out=" << out;
    }
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, ToString_010, testing::ext::TestSize.Level0)
{
    GetOrderPositionDto dto;
    dto.albumId = 1000 + 25;
    dto.assetIdArray.clear();
    dto.assetIdArray.emplace_back("WmTSHf");
    dto.assetIdArray.emplace_back("pWkLUyifDLkDmWJ6UuVTAIjvFu7WIC");
    dto.assetIdArray.emplace_back("hDeOZIiBOB-Y6sHrFH2ZU");
    dto.assetIdArray.emplace_back("r-lgotu2iXW7Gbo");
    dto.assetIdArray.emplace_back("RoL3u6aHwnMztVuaP_");
    const std::string out = dto.ToString();
    const std::string payload = ExtractAssetArrayPayload(out);
    // Split with the exact delimiter used by the implementation.
    const auto tokens = SplitByCommaSpace(payload);
    ASSERT_EQ(tokens.size(), dto.assetIdArray.size());
    for (size_t i = 0; i < tokens.size(); ++i) {
        ASSERT_EQ(tokens[i], dto.assetIdArray[i]) << "index=" << i << ", out=" << out;
    }
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, ToString_011, testing::ext::TestSize.Level0)
{
    GetOrderPositionDto dto;
    dto.albumId = 1000 + 30;
    dto.assetIdArray.clear();
    dto.assetIdArray.emplace_back("oU");
    dto.assetIdArray.emplace_back("EhEkk_iqq8vH2BzNZV45");
    dto.assetIdArray.emplace_back("FCiRcDCa");
    dto.assetIdArray.emplace_back("hDieQ");
    dto.assetIdArray.emplace_back("EJ_Bq");
    const std::string out = dto.ToString();
    const std::string payload = ExtractAssetArrayPayload(out);
    // Split with the exact delimiter used by the implementation.
    const auto tokens = SplitByCommaSpace(payload);
    ASSERT_EQ(tokens.size(), dto.assetIdArray.size());
    for (size_t i = 0; i < tokens.size(); ++i) {
        ASSERT_EQ(tokens[i], dto.assetIdArray[i]) << "index=" << i << ", out=" << out;
    }
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, ToString_012, testing::ext::TestSize.Level0)
{
    GetOrderPositionDto dto;
    dto.albumId = 1000 + 35;
    dto.assetIdArray.clear();
    dto.assetIdArray.emplace_back("F80ymm3T207gmhZRnFyy5r2xJ7Fj4mg");
    dto.assetIdArray.emplace_back("l");
    dto.assetIdArray.emplace_back("v0_9BZhvWaXH6K2_");
    dto.assetIdArray.emplace_back("yLBhhOhg9u");
    dto.assetIdArray.emplace_back("kxii");
    const std::string out = dto.ToString();
    const std::string payload = ExtractAssetArrayPayload(out);
    // Split with the exact delimiter used by the implementation.
    const auto tokens = SplitByCommaSpace(payload);
    ASSERT_EQ(tokens.size(), dto.assetIdArray.size());
    for (size_t i = 0; i < tokens.size(); ++i) {
        ASSERT_EQ(tokens[i], dto.assetIdArray[i]) << "index=" << i << ", out=" << out;
    }
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, ToString_013, testing::ext::TestSize.Level0)
{
    GetOrderPositionDto dto;
    dto.albumId = 1000 + 40;
    dto.assetIdArray.clear();
    dto.assetIdArray.emplace_back("ZpFfk1OHAOEHYqM6");
    dto.assetIdArray.emplace_back("jb6mjBHqSiFVKu4MbMnrH");
    dto.assetIdArray.emplace_back("ntIKARAH");
    dto.assetIdArray.emplace_back("Ggl2JfaQqHu42bojteVs3qfNUfTAFnT0");
    dto.assetIdArray.emplace_back("Euw0dwQ0FI");
    const std::string out = dto.ToString();
    const std::string payload = ExtractAssetArrayPayload(out);
    // Split with the exact delimiter used by the implementation.
    const auto tokens = SplitByCommaSpace(payload);
    ASSERT_EQ(tokens.size(), dto.assetIdArray.size());
    for (size_t i = 0; i < tokens.size(); ++i) {
        ASSERT_EQ(tokens[i], dto.assetIdArray[i]) << "index=" << i << ", out=" << out;
    }
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, ToString_014, testing::ext::TestSize.Level0)
{
    GetOrderPositionDto dto;
    dto.albumId = 1000 + 45;
    dto.assetIdArray.clear();
    dto.assetIdArray.emplace_back("nWe8Cz6SNDC");
    dto.assetIdArray.emplace_back("yZ");
    dto.assetIdArray.emplace_back("JiJSZQdoHwHen3SO3oXyGf");
    dto.assetIdArray.emplace_back("azU3iQOpMN0PZLqy1WwMZaMKA3P7");
    dto.assetIdArray.emplace_back("4B8vkKQlENCzsdfF8j61yX-ZFsan2");
    const std::string out = dto.ToString();
    const std::string payload = ExtractAssetArrayPayload(out);
    // Split with the exact delimiter used by the implementation.
    const auto tokens = SplitByCommaSpace(payload);
    ASSERT_EQ(tokens.size(), dto.assetIdArray.size());
    for (size_t i = 0; i < tokens.size(); ++i) {
        ASSERT_EQ(tokens[i], dto.assetIdArray[i]) << "index=" << i << ", out=" << out;
    }
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, ToString_015, testing::ext::TestSize.Level0)
{
    GetOrderPositionDto dto;
    dto.albumId = 1000 + 50;
    dto.assetIdArray.clear();
    dto.assetIdArray.emplace_back("w7gFp6r7O425u85");
    dto.assetIdArray.emplace_back("FJ_EJ4jKEIQOkrtDX");
    dto.assetIdArray.emplace_back("Bi10Q71hA1");
    dto.assetIdArray.emplace_back("cW9aTMX1C_CI3_dXRZv7qdYdk");
    dto.assetIdArray.emplace_back("r7xgHWPB6PRWJ1Gk8cgSCifdFzct");
    const std::string out = dto.ToString();
    const std::string payload = ExtractAssetArrayPayload(out);
    // Split with the exact delimiter used by the implementation.
    const auto tokens = SplitByCommaSpace(payload);
    ASSERT_EQ(tokens.size(), dto.assetIdArray.size());
    for (size_t i = 0; i < tokens.size(); ++i) {
        ASSERT_EQ(tokens[i], dto.assetIdArray[i]) << "index=" << i << ", out=" << out;
    }
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, ToString_016, testing::ext::TestSize.Level0)
{
    GetOrderPositionDto dto;
    dto.albumId = 1000 + 55;
    dto.assetIdArray.clear();
    dto.assetIdArray.emplace_back("q8oB7GVvouNndNWY");
    dto.assetIdArray.emplace_back("jFnMpfS2ViRb1");
    dto.assetIdArray.emplace_back("n3U6t3wI973IPFlJ5F7WRd-Px_BTHRJJ");
    dto.assetIdArray.emplace_back("y");
    dto.assetIdArray.emplace_back("E0_E8_");
    const std::string out = dto.ToString();
    const std::string payload = ExtractAssetArrayPayload(out);
    // Split with the exact delimiter used by the implementation.
    const auto tokens = SplitByCommaSpace(payload);
    ASSERT_EQ(tokens.size(), dto.assetIdArray.size());
    for (size_t i = 0; i < tokens.size(); ++i) {
        ASSERT_EQ(tokens[i], dto.assetIdArray[i]) << "index=" << i << ", out=" << out;
    }
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, ToString_017, testing::ext::TestSize.Level0)
{
    GetOrderPositionDto dto;
    dto.albumId = 1000 + 60;
    dto.assetIdArray.clear();
    dto.assetIdArray.emplace_back("clLCZFNV8S2QT6INGDpyOpxyB9JKm");
    dto.assetIdArray.emplace_back("LDUwMbqJfgLq_");
    const std::string out = dto.ToString();
    const std::string payload = ExtractAssetArrayPayload(out);
    // Split with the exact delimiter used by the implementation.
    const auto tokens = SplitByCommaSpace(payload);
    ASSERT_EQ(tokens.size(), dto.assetIdArray.size());
    for (size_t i = 0; i < tokens.size(); ++i) {
        ASSERT_EQ(tokens[i], dto.assetIdArray[i]) << "index=" << i << ", out=" << out;
    }
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, ToString_086, testing::ext::TestSize.Level1)
{
    GetOrderPositionDto dto;
    dto.albumId = 9999;
    const size_t n = 200;
    dto.assetIdArray.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        dto.assetIdArray.emplace_back("id" + std::to_string(i));
    }

    const std::string s = dto.ToString();
    const std::string payload = ExtractAssetArrayPayload(s);

    // For N elements, delimiter count is N-1.
    ASSERT_EQ(CountSubstr(payload, ", "), n - 1);

    // Basic sanity: first and last should be present.
    ASSERT_TRUE(Contains(payload, "id0"));
    ASSERT_TRUE(Contains(payload, "id199"));
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, ToString_018, testing::ext::TestSize.Level0)
{
    auto dto = MakeDto(42, { "a", "b", "c" });

    const std::string s1 = dto.ToString();
    const std::string s2 = dto.ToString();
    const std::string s3 = dto.ToString();

    ASSERT_EQ(s1, s2);
    ASSERT_EQ(s2, s3);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, ToString_019, testing::ext::TestSize.Level0)
{
    GetOrderPositionDto dto;
    dto.albumId = 1;
    dto.assetIdArray = { "a" };

    const std::string s1 = dto.ToString();
    ASSERT_TRUE(Contains(s1, R"("albumId": "1")"));
    ASSERT_TRUE(Contains(s1, "a"));

    // mutate
    dto.albumId = -7;
    dto.assetIdArray.clear();
    dto.assetIdArray.emplace_back("x");
    dto.assetIdArray.emplace_back("y");

    const std::string s2 = dto.ToString();
    ASSERT_TRUE(Contains(s2, R"("albumId": "-7")"));
    ASSERT_TRUE(Contains(s2, "x, y"));
    ASSERT_FALSE(Contains(s2, R"("albumId": "1")"));
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, Dto_020, testing::ext::TestSize.Level0)
{
    auto dto = MakeDto(5, { "p", "q", "r" });
    GetOrderPositionDto copy(dto);

    ASSERT_EQ(copy.albumId, dto.albumId);
    ASSERT_EQ(copy.assetIdArray, dto.assetIdArray);
    ASSERT_EQ(copy.ToString(), dto.ToString());
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, Dto_021, testing::ext::TestSize.Level0)
{
    GetOrderPositionDto dto;
    dto.albumId = 8;
    dto.assetIdArray = { "m1", "m2" };

    GetOrderPositionDto moved(std::move(dto));
    ASSERT_EQ(moved.albumId, 8);
    ASSERT_EQ(moved.assetIdArray.size(), 2U);
    ASSERT_EQ(moved.assetIdArray[0], "m1");
    ASSERT_EQ(moved.assetIdArray[1], "m2");
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, GetOrderPositionDto_Assignments_WorkAsExpected, testing::ext::TestSize.Level0)
{
    GetOrderPositionDto a = MakeDto(1, { "a" });
    GetOrderPositionDto b = MakeDto(2, { "b1", "b2" });

    a = b;
    ASSERT_EQ(a.albumId, 2);
    ASSERT_EQ(a.assetIdArray, b.assetIdArray);

    GetOrderPositionDto c;
    c = std::move(b);
    ASSERT_EQ(c.albumId, 2);
    ASSERT_EQ(c.assetIdArray.size(), 2U);
}


HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_ValueInit_AndMove, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    ASSERT_TRUE(dto.uri.empty());

    StartAssetAnalysisDto dto2 = dto;
    ASSERT_TRUE(dto2.uri.empty());

    StartAssetAnalysisDto dto3 = std::move(dto2);
    ASSERT_TRUE(dto3.uri.empty());

    dto3.uri = "datashare://media/asset/1";
    StartAssetAnalysisDto dto4 = dto3;
    ASSERT_EQ(dto4.uri, "datashare://media/asset/1");
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, Dto_022, testing::ext::TestSize.Level0)
{
    ChangeRequestSetOrderPositionDto dto;
    ASSERT_EQ(dto.albumId, -1);
    ASSERT_TRUE(dto.orderString.empty());
    ASSERT_TRUE(dto.assetIds.empty());
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, Dto_023, testing::ext::TestSize.Level0)
{
    ChangeRequestSetOrderPositionDto dto;
    dto.albumId = 10;
    dto.orderString = "ASC";
    dto.assetIds = { "x", "y", "z" };

    ChangeRequestSetOrderPositionDto copy(dto);
    ASSERT_EQ(copy.albumId, 10);
    ASSERT_EQ(copy.orderString, "ASC");
    ASSERT_EQ(copy.assetIds.size(), 3U);

    ChangeRequestSetOrderPositionDto moved(std::move(copy));
    ASSERT_EQ(moved.albumId, 10);
    ASSERT_EQ(moved.orderString, "ASC");
    ASSERT_EQ(moved.assetIds[2], "z");
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, ToString_087, testing::ext::TestSize.Level1)
{
    // Validate that separators only appear between elements.
    GetOrderPositionDto dto;
    dto.albumId = 555;
    dto.assetIdArray = { "a", "b", "c", "d" };
    const std::string payload = ExtractAssetArrayPayload(dto.ToString());
    // There should be exactly 3 occurrences of ", ".
    ASSERT_EQ(CountSubstr(payload, ", "), 3U);
    // No leading delimiter.
    ASSERT_NE(payload.rfind(", ", 0), 0U);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, GetOrderPositionDto_ToString_AlbumId_Pos_0, testing::ext::TestSize.Level0)
{
    auto dto = MakeDto(0, { "id" });
    const std::string s = dto.ToString();
    ASSERT_TRUE(Contains(s, "\"albumId\": \"0\"")) << s;
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, GetOrderPositionDto_ToString_AlbumId_Pos_1, testing::ext::TestSize.Level0)
{
    auto dto = MakeDto(1, { "id" });
    const std::string s = dto.ToString();
    ASSERT_TRUE(Contains(s, "\"albumId\": \"1\"")) << s;
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, GetOrderPositionDto_ToString_AlbumId_Neg_1, testing::ext::TestSize.Level0)
{
    auto dto = MakeDto(-1, { "id" });
    const std::string s = dto.ToString();
    ASSERT_TRUE(Contains(s, "\"albumId\": \"-1\"")) << s;
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, ToString_024, testing::ext::TestSize.Level0)
{
    auto dto = MakeDto(2147483647, { "id" });
    const std::string s = dto.ToString();
    ASSERT_TRUE(Contains(s, "\"albumId\": \"2147483647\"")) << s;
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, ToString_025, testing::ext::TestSize.Level0)
{
    auto dto = MakeDto(-2147483648, { "id" });
    const std::string s = dto.ToString();
    ASSERT_TRUE(Contains(s, "\"albumId\": \"-2147483648\"")) << s;
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, Dto_026, testing::ext::TestSize.Level0)
{
    ChangeRequestSetOrderPositionDto dto;
    dto.albumId = 1;
    dto.orderString = "ORDER_1";
    for (int k = 0; k < 2; ++k) {
        dto.assetIds.emplace_back("asset_" + std::to_string(k));
    }

    ChangeRequestSetOrderPositionDto copy = dto;
    ASSERT_EQ(copy.albumId, 1);
    ASSERT_EQ(copy.orderString, "ORDER_1");
    ASSERT_EQ(copy.assetIds, dto.assetIds);

    // mutate original and ensure copy remains unchanged
    dto.assetIds.emplace_back("tail");
    ASSERT_NE(copy.assetIds.size(), dto.assetIds.size());
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, Dto_027, testing::ext::TestSize.Level0)
{
    ChangeRequestSetOrderPositionDto dto;
    dto.albumId = 2;
    dto.orderString = "ORDER_2";
    for (int k = 0; k < 3; ++k) {
        dto.assetIds.emplace_back("asset_" + std::to_string(k));
    }

    ChangeRequestSetOrderPositionDto copy = dto;
    ASSERT_EQ(copy.albumId, 2);
    ASSERT_EQ(copy.orderString, "ORDER_2");
    ASSERT_EQ(copy.assetIds, dto.assetIds);

    // mutate original and ensure copy remains unchanged
    dto.assetIds.emplace_back("tail");
    ASSERT_NE(copy.assetIds.size(), dto.assetIds.size());
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, Dto_028, testing::ext::TestSize.Level0)
{
    ChangeRequestSetOrderPositionDto dto;
    dto.albumId = 3;
    dto.orderString = "ORDER_3";
    for (int k = 0; k < 4; ++k) {
        dto.assetIds.emplace_back("asset_" + std::to_string(k));
    }

    ChangeRequestSetOrderPositionDto copy = dto;
    ASSERT_EQ(copy.albumId, 3);
    ASSERT_EQ(copy.orderString, "ORDER_3");
    ASSERT_EQ(copy.assetIds, dto.assetIds);

    // mutate original and ensure copy remains unchanged
    dto.assetIds.emplace_back("tail");
    ASSERT_NE(copy.assetIds.size(), dto.assetIds.size());
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, Dto_029, testing::ext::TestSize.Level0)
{
    ChangeRequestSetOrderPositionDto dto;
    dto.albumId = 4;
    dto.orderString = "ORDER_4";
    for (int k = 0; k < 5; ++k) {
        dto.assetIds.emplace_back("asset_" + std::to_string(k));
    }

    ChangeRequestSetOrderPositionDto copy = dto;
    ASSERT_EQ(copy.albumId, 4);
    ASSERT_EQ(copy.orderString, "ORDER_4");
    ASSERT_EQ(copy.assetIds, dto.assetIds);

    // mutate original and ensure copy remains unchanged
    dto.assetIds.emplace_back("tail");
    ASSERT_NE(copy.assetIds.size(), dto.assetIds.size());
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, Dto_030, testing::ext::TestSize.Level0)
{
    ChangeRequestSetOrderPositionDto dto;
    dto.albumId = 5;
    dto.orderString = "ORDER_5";
    for (int k = 0; k < 6; ++k) {
        dto.assetIds.emplace_back("asset_" + std::to_string(k));
    }

    ChangeRequestSetOrderPositionDto copy = dto;
    ASSERT_EQ(copy.albumId, 5);
    ASSERT_EQ(copy.orderString, "ORDER_5");
    ASSERT_EQ(copy.assetIds, dto.assetIds);

    // mutate original and ensure copy remains unchanged
    dto.assetIds.emplace_back("tail");
    ASSERT_NE(copy.assetIds.size(), dto.assetIds.size());
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, Dto_031, testing::ext::TestSize.Level0)
{
    ChangeRequestSetOrderPositionDto dto;
    dto.albumId = 6;
    dto.orderString = "ORDER_6";
    for (int k = 0; k < 7; ++k) {
        dto.assetIds.emplace_back("asset_" + std::to_string(k));
    }

    ChangeRequestSetOrderPositionDto copy = dto;
    ASSERT_EQ(copy.albumId, 6);
    ASSERT_EQ(copy.orderString, "ORDER_6");
    ASSERT_EQ(copy.assetIds, dto.assetIds);

    // mutate original and ensure copy remains unchanged
    dto.assetIds.emplace_back("tail");
    ASSERT_NE(copy.assetIds.size(), dto.assetIds.size());
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, Dto_032, testing::ext::TestSize.Level0)
{
    ChangeRequestSetOrderPositionDto dto;
    dto.albumId = 7;
    dto.orderString = "ORDER_7";
    for (int k = 0; k < 1; ++k) {
        dto.assetIds.emplace_back("asset_" + std::to_string(k));
    }

    ChangeRequestSetOrderPositionDto copy = dto;
    ASSERT_EQ(copy.albumId, 7);
    ASSERT_EQ(copy.orderString, "ORDER_7");
    ASSERT_EQ(copy.assetIds, dto.assetIds);

    // mutate original and ensure copy remains unchanged
    dto.assetIds.emplace_back("tail");
    ASSERT_NE(copy.assetIds.size(), dto.assetIds.size());
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, Dto_033, testing::ext::TestSize.Level0)
{
    ChangeRequestSetOrderPositionDto dto;
    dto.albumId = 8;
    dto.orderString = "ORDER_8";
    for (int k = 0; k < 2; ++k) {
        dto.assetIds.emplace_back("asset_" + std::to_string(k));
    }

    ChangeRequestSetOrderPositionDto copy = dto;
    ASSERT_EQ(copy.albumId, 8);
    ASSERT_EQ(copy.orderString, "ORDER_8");
    ASSERT_EQ(copy.assetIds, dto.assetIds);

    // mutate original and ensure copy remains unchanged
    dto.assetIds.emplace_back("tail");
    ASSERT_NE(copy.assetIds.size(), dto.assetIds.size());
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, Dto_034, testing::ext::TestSize.Level0)
{
    ChangeRequestSetOrderPositionDto dto;
    dto.albumId = 9;
    dto.orderString = "ORDER_9";
    for (int k = 0; k < 3; ++k) {
        dto.assetIds.emplace_back("asset_" + std::to_string(k));
    }

    ChangeRequestSetOrderPositionDto copy = dto;
    ASSERT_EQ(copy.albumId, 9);
    ASSERT_EQ(copy.orderString, "ORDER_9");
    ASSERT_EQ(copy.assetIds, dto.assetIds);

    // mutate original and ensure copy remains unchanged
    dto.assetIds.emplace_back("tail");
    ASSERT_NE(copy.assetIds.size(), dto.assetIds.size());
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, Dto_035, testing::ext::TestSize.Level0)
{
    ChangeRequestSetOrderPositionDto dto;
    dto.albumId = 10;
    dto.orderString = "ORDER_10";
    for (int k = 0; k < 4; ++k) {
        dto.assetIds.emplace_back("asset_" + std::to_string(k));
    }

    ChangeRequestSetOrderPositionDto copy = dto;
    ASSERT_EQ(copy.albumId, 10);
    ASSERT_EQ(copy.orderString, "ORDER_10");
    ASSERT_EQ(copy.assetIds, dto.assetIds);

    // mutate original and ensure copy remains unchanged
    dto.assetIds.emplace_back("tail");
    ASSERT_NE(copy.assetIds.size(), dto.assetIds.size());
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, Dto_036, testing::ext::TestSize.Level0)
{
    ChangeRequestSetOrderPositionDto dto;
    dto.albumId = 11;
    dto.orderString = "ORDER_11";
    for (int k = 0; k < 5; ++k) {
        dto.assetIds.emplace_back("asset_" + std::to_string(k));
    }

    ChangeRequestSetOrderPositionDto copy = dto;
    ASSERT_EQ(copy.albumId, 11);
    ASSERT_EQ(copy.orderString, "ORDER_11");
    ASSERT_EQ(copy.assetIds, dto.assetIds);

    // mutate original and ensure copy remains unchanged
    dto.assetIds.emplace_back("tail");
    ASSERT_NE(copy.assetIds.size(), dto.assetIds.size());
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, Dto_037, testing::ext::TestSize.Level0)
{
    ChangeRequestSetOrderPositionDto dto;
    dto.albumId = 12;
    dto.orderString = "ORDER_12";
    for (int k = 0; k < 6; ++k) {
        dto.assetIds.emplace_back("asset_" + std::to_string(k));
    }

    ChangeRequestSetOrderPositionDto copy = dto;
    ASSERT_EQ(copy.albumId, 12);
    ASSERT_EQ(copy.orderString, "ORDER_12");
    ASSERT_EQ(copy.assetIds, dto.assetIds);

    // mutate original and ensure copy remains unchanged
    dto.assetIds.emplace_back("tail");
    ASSERT_NE(copy.assetIds.size(), dto.assetIds.size());
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, Dto_038, testing::ext::TestSize.Level0)
{
    ChangeRequestSetOrderPositionDto dto;
    dto.albumId = 13;
    dto.orderString = "ORDER_13";
    for (int k = 0; k < 7; ++k) {
        dto.assetIds.emplace_back("asset_" + std::to_string(k));
    }

    ChangeRequestSetOrderPositionDto copy = dto;
    ASSERT_EQ(copy.albumId, 13);
    ASSERT_EQ(copy.orderString, "ORDER_13");
    ASSERT_EQ(copy.assetIds, dto.assetIds);

    // mutate original and ensure copy remains unchanged
    dto.assetIds.emplace_back("tail");
    ASSERT_NE(copy.assetIds.size(), dto.assetIds.size());
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, Dto_039, testing::ext::TestSize.Level0)
{
    ChangeRequestSetOrderPositionDto dto;
    dto.albumId = 14;
    dto.orderString = "ORDER_14";
    for (int k = 0; k < 1; ++k) {
        dto.assetIds.emplace_back("asset_" + std::to_string(k));
    }

    ChangeRequestSetOrderPositionDto copy = dto;
    ASSERT_EQ(copy.albumId, 14);
    ASSERT_EQ(copy.orderString, "ORDER_14");
    ASSERT_EQ(copy.assetIds, dto.assetIds);

    // mutate original and ensure copy remains unchanged
    dto.assetIds.emplace_back("tail");
    ASSERT_NE(copy.assetIds.size(), dto.assetIds.size());
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, Dto_040, testing::ext::TestSize.Level0)
{
    ChangeRequestSetOrderPositionDto dto;
    dto.albumId = 15;
    dto.orderString = "ORDER_15";
    for (int k = 0; k < 2; ++k) {
        dto.assetIds.emplace_back("asset_" + std::to_string(k));
    }

    ChangeRequestSetOrderPositionDto copy = dto;
    ASSERT_EQ(copy.albumId, 15);
    ASSERT_EQ(copy.orderString, "ORDER_15");
    ASSERT_EQ(copy.assetIds, dto.assetIds);

    // mutate original and ensure copy remains unchanged
    dto.assetIds.emplace_back("tail");
    ASSERT_NE(copy.assetIds.size(), dto.assetIds.size());
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, Dto_041, testing::ext::TestSize.Level0)
{
    ChangeRequestSetOrderPositionDto dto;
    dto.albumId = 16;
    dto.orderString = "ORDER_16";
    for (int k = 0; k < 3; ++k) {
        dto.assetIds.emplace_back("asset_" + std::to_string(k));
    }

    ChangeRequestSetOrderPositionDto copy = dto;
    ASSERT_EQ(copy.albumId, 16);
    ASSERT_EQ(copy.orderString, "ORDER_16");
    ASSERT_EQ(copy.assetIds, dto.assetIds);

    // mutate original and ensure copy remains unchanged
    dto.assetIds.emplace_back("tail");
    ASSERT_NE(copy.assetIds.size(), dto.assetIds.size());
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, Dto_042, testing::ext::TestSize.Level0)
{
    ChangeRequestSetOrderPositionDto dto;
    dto.albumId = 17;
    dto.orderString = "ORDER_17";
    for (int k = 0; k < 4; ++k) {
        dto.assetIds.emplace_back("asset_" + std::to_string(k));
    }

    ChangeRequestSetOrderPositionDto copy = dto;
    ASSERT_EQ(copy.albumId, 17);
    ASSERT_EQ(copy.orderString, "ORDER_17");
    ASSERT_EQ(copy.assetIds, dto.assetIds);

    // mutate original and ensure copy remains unchanged
    dto.assetIds.emplace_back("tail");
    ASSERT_NE(copy.assetIds.size(), dto.assetIds.size());
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, Dto_043, testing::ext::TestSize.Level0)
{
    ChangeRequestSetOrderPositionDto dto;
    dto.albumId = 18;
    dto.orderString = "ORDER_18";
    for (int k = 0; k < 5; ++k) {
        dto.assetIds.emplace_back("asset_" + std::to_string(k));
    }

    ChangeRequestSetOrderPositionDto copy = dto;
    ASSERT_EQ(copy.albumId, 18);
    ASSERT_EQ(copy.orderString, "ORDER_18");
    ASSERT_EQ(copy.assetIds, dto.assetIds);

    // mutate original and ensure copy remains unchanged
    dto.assetIds.emplace_back("tail");
    ASSERT_NE(copy.assetIds.size(), dto.assetIds.size());
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, Dto_044, testing::ext::TestSize.Level0)
{
    ChangeRequestSetOrderPositionDto dto;
    dto.albumId = 19;
    dto.orderString = "ORDER_19";
    for (int k = 0; k < 6; ++k) {
        dto.assetIds.emplace_back("asset_" + std::to_string(k));
    }

    ChangeRequestSetOrderPositionDto copy = dto;
    ASSERT_EQ(copy.albumId, 19);
    ASSERT_EQ(copy.orderString, "ORDER_19");
    ASSERT_EQ(copy.assetIds, dto.assetIds);

    // mutate original and ensure copy remains unchanged
    dto.assetIds.emplace_back("tail");
    ASSERT_NE(copy.assetIds.size(), dto.assetIds.size());
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, Dto_045, testing::ext::TestSize.Level0)
{
    ChangeRequestSetOrderPositionDto dto;
    dto.albumId = 20;
    dto.orderString = "ORDER_20";
    for (int k = 0; k < 7; ++k) {
        dto.assetIds.emplace_back("asset_" + std::to_string(k));
    }

    ChangeRequestSetOrderPositionDto copy = dto;
    ASSERT_EQ(copy.albumId, 20);
    ASSERT_EQ(copy.orderString, "ORDER_20");
    ASSERT_EQ(copy.assetIds, dto.assetIds);

    // mutate original and ensure copy remains unchanged
    dto.assetIds.emplace_back("tail");
    ASSERT_NE(copy.assetIds.size(), dto.assetIds.size());
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, Dto_046, testing::ext::TestSize.Level0)
{
    ChangeRequestSetOrderPositionDto dto;
    dto.albumId = 21;
    dto.orderString = "ORDER_21";
    for (int k = 0; k < 1; ++k) {
        dto.assetIds.emplace_back("asset_" + std::to_string(k));
    }

    ChangeRequestSetOrderPositionDto copy = dto;
    ASSERT_EQ(copy.albumId, 21);
    ASSERT_EQ(copy.orderString, "ORDER_21");
    ASSERT_EQ(copy.assetIds, dto.assetIds);

    // mutate original and ensure copy remains unchanged
    dto.assetIds.emplace_back("tail");
    ASSERT_NE(copy.assetIds.size(), dto.assetIds.size());
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, Dto_047, testing::ext::TestSize.Level0)
{
    ChangeRequestSetOrderPositionDto dto;
    dto.albumId = 22;
    dto.orderString = "ORDER_22";
    for (int k = 0; k < 2; ++k) {
        dto.assetIds.emplace_back("asset_" + std::to_string(k));
    }

    ChangeRequestSetOrderPositionDto copy = dto;
    ASSERT_EQ(copy.albumId, 22);
    ASSERT_EQ(copy.orderString, "ORDER_22");
    ASSERT_EQ(copy.assetIds, dto.assetIds);

    // mutate original and ensure copy remains unchanged
    dto.assetIds.emplace_back("tail");
    ASSERT_NE(copy.assetIds.size(), dto.assetIds.size());
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, Dto_048, testing::ext::TestSize.Level0)
{
    ChangeRequestSetOrderPositionDto dto;
    dto.albumId = 23;
    dto.orderString = "ORDER_23";
    for (int k = 0; k < 3; ++k) {
        dto.assetIds.emplace_back("asset_" + std::to_string(k));
    }

    ChangeRequestSetOrderPositionDto copy = dto;
    ASSERT_EQ(copy.albumId, 23);
    ASSERT_EQ(copy.orderString, "ORDER_23");
    ASSERT_EQ(copy.assetIds, dto.assetIds);

    // mutate original and ensure copy remains unchanged
    dto.assetIds.emplace_back("tail");
    ASSERT_NE(copy.assetIds.size(), dto.assetIds.size());
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, Dto_049, testing::ext::TestSize.Level0)
{
    ChangeRequestSetOrderPositionDto dto;
    dto.albumId = 24;
    dto.orderString = "ORDER_24";
    for (int k = 0; k < 4; ++k) {
        dto.assetIds.emplace_back("asset_" + std::to_string(k));
    }

    ChangeRequestSetOrderPositionDto copy = dto;
    ASSERT_EQ(copy.albumId, 24);
    ASSERT_EQ(copy.orderString, "ORDER_24");
    ASSERT_EQ(copy.assetIds, dto.assetIds);

    // mutate original and ensure copy remains unchanged
    dto.assetIds.emplace_back("tail");
    ASSERT_NE(copy.assetIds.size(), dto.assetIds.size());
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, Dto_050, testing::ext::TestSize.Level0)
{
    ChangeRequestSetOrderPositionDto dto;
    dto.albumId = 25;
    dto.orderString = "ORDER_25";
    for (int k = 0; k < 5; ++k) {
        dto.assetIds.emplace_back("asset_" + std::to_string(k));
    }

    ChangeRequestSetOrderPositionDto copy = dto;
    ASSERT_EQ(copy.albumId, 25);
    ASSERT_EQ(copy.orderString, "ORDER_25");
    ASSERT_EQ(copy.assetIds, dto.assetIds);

    // mutate original and ensure copy remains unchanged
    dto.assetIds.emplace_back("tail");
    ASSERT_NE(copy.assetIds.size(), dto.assetIds.size());
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, Dto_051, testing::ext::TestSize.Level0)
{
    ChangeRequestSetOrderPositionDto dto;
    dto.albumId = 26;
    dto.orderString = "ORDER_26";
    for (int k = 0; k < 6; ++k) {
        dto.assetIds.emplace_back("asset_" + std::to_string(k));
    }

    ChangeRequestSetOrderPositionDto copy = dto;
    ASSERT_EQ(copy.albumId, 26);
    ASSERT_EQ(copy.orderString, "ORDER_26");
    ASSERT_EQ(copy.assetIds, dto.assetIds);

    // mutate original and ensure copy remains unchanged
    dto.assetIds.emplace_back("tail");
    ASSERT_NE(copy.assetIds.size(), dto.assetIds.size());
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, Dto_052, testing::ext::TestSize.Level0)
{
    ChangeRequestSetOrderPositionDto dto;
    dto.albumId = 27;
    dto.orderString = "ORDER_27";
    for (int k = 0; k < 7; ++k) {
        dto.assetIds.emplace_back("asset_" + std::to_string(k));
    }

    ChangeRequestSetOrderPositionDto copy = dto;
    ASSERT_EQ(copy.albumId, 27);
    ASSERT_EQ(copy.orderString, "ORDER_27");
    ASSERT_EQ(copy.assetIds, dto.assetIds);

    // mutate original and ensure copy remains unchanged
    dto.assetIds.emplace_back("tail");
    ASSERT_NE(copy.assetIds.size(), dto.assetIds.size());
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, Dto_053, testing::ext::TestSize.Level0)
{
    ChangeRequestSetOrderPositionDto dto;
    dto.albumId = 28;
    dto.orderString = "ORDER_28";
    for (int k = 0; k < 1; ++k) {
        dto.assetIds.emplace_back("asset_" + std::to_string(k));
    }

    ChangeRequestSetOrderPositionDto copy = dto;
    ASSERT_EQ(copy.albumId, 28);
    ASSERT_EQ(copy.orderString, "ORDER_28");
    ASSERT_EQ(copy.assetIds, dto.assetIds);

    // mutate original and ensure copy remains unchanged
    dto.assetIds.emplace_back("tail");
    ASSERT_NE(copy.assetIds.size(), dto.assetIds.size());
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, Dto_054, testing::ext::TestSize.Level0)
{
    ChangeRequestSetOrderPositionDto dto;
    dto.albumId = 29;
    dto.orderString = "ORDER_29";
    for (int k = 0; k < 2; ++k) {
        dto.assetIds.emplace_back("asset_" + std::to_string(k));
    }

    ChangeRequestSetOrderPositionDto copy = dto;
    ASSERT_EQ(copy.albumId, 29);
    ASSERT_EQ(copy.orderString, "ORDER_29");
    ASSERT_EQ(copy.assetIds, dto.assetIds);

    // mutate original and ensure copy remains unchanged
    dto.assetIds.emplace_back("tail");
    ASSERT_NE(copy.assetIds.size(), dto.assetIds.size());
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, Dto_055, testing::ext::TestSize.Level0)
{
    ChangeRequestSetOrderPositionDto dto;
    dto.albumId = 30;
    dto.orderString = "ORDER_30";
    for (int k = 0; k < 3; ++k) {
        dto.assetIds.emplace_back("asset_" + std::to_string(k));
    }

    ChangeRequestSetOrderPositionDto copy = dto;
    ASSERT_EQ(copy.albumId, 30);
    ASSERT_EQ(copy.orderString, "ORDER_30");
    ASSERT_EQ(copy.assetIds, dto.assetIds);

    // mutate original and ensure copy remains unchanged
    dto.assetIds.emplace_back("tail");
    ASSERT_NE(copy.assetIds.size(), dto.assetIds.size());
}


HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriPatterns_01, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    dto.uri = "datashare://media/asset/" + std::to_string(1);
    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    // Move should preserve the uri in the destination.
    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriPatterns_02, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    dto.uri = "datashare://media/asset/" + std::to_string(2);
    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    // Move should preserve the uri in the destination.
    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriPatterns_03, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    dto.uri = "datashare://media/asset/" + std::to_string(3);
    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    // Move should preserve the uri in the destination.
    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriPatterns_04, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    dto.uri = "datashare://media/asset/" + std::to_string(4);
    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    // Move should preserve the uri in the destination.
    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriPatterns_05, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    dto.uri = "datashare://media/asset/" + std::to_string(5);
    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    // Move should preserve the uri in the destination.
    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriPatterns_06, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    dto.uri = "datashare://media/asset/" + std::to_string(6);
    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    // Move should preserve the uri in the destination.
    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriPatterns_07, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    dto.uri = "datashare://media/asset/" + std::to_string(7);
    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    // Move should preserve the uri in the destination.
    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriPatterns_08, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    dto.uri = "datashare://media/asset/" + std::to_string(8);
    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    // Move should preserve the uri in the destination.
    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriPatterns_09, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    dto.uri = "datashare://media/asset/" + std::to_string(9);
    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    // Move should preserve the uri in the destination.
    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriPatterns_10, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    dto.uri = "datashare://media/asset/" + std::to_string(10);
    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    // Move should preserve the uri in the destination.
    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriPatterns_11, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    dto.uri = "datashare://media/asset/" + std::to_string(11);
    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    // Move should preserve the uri in the destination.
    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriPatterns_12, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    dto.uri = "datashare://media/asset/" + std::to_string(12);
    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    // Move should preserve the uri in the destination.
    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriPatterns_13, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    dto.uri = "datashare://media/asset/" + std::to_string(13);
    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    // Move should preserve the uri in the destination.
    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriPatterns_14, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    dto.uri = "datashare://media/asset/" + std::to_string(14);
    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    // Move should preserve the uri in the destination.
    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriPatterns_15, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    dto.uri = "datashare://media/asset/" + std::to_string(15);
    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    // Move should preserve the uri in the destination.
    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriPatterns_16, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    dto.uri = "datashare://media/asset/" + std::to_string(16);
    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    // Move should preserve the uri in the destination.
    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriPatterns_17, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    dto.uri = "datashare://media/asset/" + std::to_string(17);
    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    // Move should preserve the uri in the destination.
    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriPatterns_18, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    dto.uri = "datashare://media/asset/" + std::to_string(18);
    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    // Move should preserve the uri in the destination.
    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriPatterns_19, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    dto.uri = "datashare://media/asset/" + std::to_string(19);
    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    // Move should preserve the uri in the destination.
    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriPatterns_20, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    dto.uri = "datashare://media/asset/" + std::to_string(20);
    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    // Move should preserve the uri in the destination.
    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

/* Added tests to replace removed GetAssetAnalysisDataDto cases (avoid DataShare dependency). */

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriEdgeCases_Extra_01, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    // Different URI shapes
    dto.uri = "datashare://media/asset/1?q=" + std::to_string(1 * 1);
    ASSERT_FALSE(dto.uri.empty());

    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriEdgeCases_Extra_02, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    // Different URI shapes
    dto.uri = "datashare://media/asset/2?q=" + std::to_string(2 * 2);
    ASSERT_FALSE(dto.uri.empty());

    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriEdgeCases_Extra_03, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    // Different URI shapes
    dto.uri = "datashare://media/asset/3?q=" + std::to_string(3 * 3);
    ASSERT_FALSE(dto.uri.empty());

    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriEdgeCases_Extra_04, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    // Different URI shapes
    dto.uri = "datashare://media/asset/4?q=" + std::to_string(4 * 4);
    ASSERT_FALSE(dto.uri.empty());

    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriEdgeCases_Extra_05, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    // Different URI shapes
    dto.uri = "datashare://media/asset/5?q=" + std::to_string(5 * 5);
    ASSERT_FALSE(dto.uri.empty());

    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriEdgeCases_Extra_06, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    // Different URI shapes
    dto.uri = "datashare://media/asset/6?q=" + std::to_string(6 * 6);
    ASSERT_FALSE(dto.uri.empty());

    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriEdgeCases_Extra_07, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    // Different URI shapes
    dto.uri = "datashare://media/asset/7?q=" + std::to_string(7 * 7);
    ASSERT_FALSE(dto.uri.empty());

    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriEdgeCases_Extra_08, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    // Different URI shapes
    dto.uri = "datashare://media/asset/8?q=" + std::to_string(8 * 8);
    ASSERT_FALSE(dto.uri.empty());

    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriEdgeCases_Extra_09, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    // Different URI shapes
    dto.uri = "datashare://media/asset/9?q=" + std::to_string(9 * 9);
    ASSERT_FALSE(dto.uri.empty());

    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriEdgeCases_Extra_10, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    // Different URI shapes
    dto.uri = "datashare://media/asset/10?q=" + std::to_string(10 * 10);
    ASSERT_FALSE(dto.uri.empty());

    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriEdgeCases_Extra_11, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    // Different URI shapes
    dto.uri = "datashare://media/asset/11?q=" + std::to_string(11 * 11);
    ASSERT_FALSE(dto.uri.empty());

    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriEdgeCases_Extra_12, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    // Different URI shapes
    dto.uri = "datashare://media/asset/12?q=" + std::to_string(12 * 12);
    ASSERT_FALSE(dto.uri.empty());

    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriEdgeCases_Extra_13, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    // Different URI shapes
    dto.uri = "datashare://media/asset/13?q=" + std::to_string(13 * 13);
    ASSERT_FALSE(dto.uri.empty());

    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriEdgeCases_Extra_14, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    // Different URI shapes
    dto.uri = "datashare://media/asset/14?q=" + std::to_string(14 * 14);
    ASSERT_FALSE(dto.uri.empty());

    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriEdgeCases_Extra_15, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    // Different URI shapes
    dto.uri = "datashare://media/asset/15?q=" + std::to_string(15 * 15);
    ASSERT_FALSE(dto.uri.empty());

    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriEdgeCases_Extra_16, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    // Different URI shapes
    dto.uri = "datashare://media/asset/16?q=" + std::to_string(16 * 16);
    ASSERT_FALSE(dto.uri.empty());

    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriEdgeCases_Extra_17, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    // Different URI shapes
    dto.uri = "datashare://media/asset/17?q=" + std::to_string(17 * 17);
    ASSERT_FALSE(dto.uri.empty());

    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriEdgeCases_Extra_18, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    // Different URI shapes
    dto.uri = "datashare://media/asset/18?q=" + std::to_string(18 * 18);
    ASSERT_FALSE(dto.uri.empty());

    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriEdgeCases_Extra_19, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    // Different URI shapes
    dto.uri = "datashare://media/asset/19?q=" + std::to_string(19 * 19);
    ASSERT_FALSE(dto.uri.empty());

    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriEdgeCases_Extra_20, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    // Different URI shapes
    dto.uri = "datashare://media/asset/20?q=" + std::to_string(20 * 20);
    ASSERT_FALSE(dto.uri.empty());

    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriEdgeCases_Extra_21, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    // Different URI shapes
    dto.uri = "datashare://media/asset/21?q=" + std::to_string(21 * 21);
    ASSERT_FALSE(dto.uri.empty());

    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriEdgeCases_Extra_22, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    // Different URI shapes
    dto.uri = "datashare://media/asset/22?q=" + std::to_string(22 * 22);
    ASSERT_FALSE(dto.uri.empty());

    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriEdgeCases_Extra_23, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    // Different URI shapes
    dto.uri = "datashare://media/asset/23?q=" + std::to_string(23 * 23);
    ASSERT_FALSE(dto.uri.empty());

    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriEdgeCases_Extra_24, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    // Different URI shapes
    dto.uri = "datashare://media/asset/24?q=" + std::to_string(24 * 24);
    ASSERT_FALSE(dto.uri.empty());

    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriEdgeCases_Extra_25, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    // Different URI shapes
    dto.uri = "datashare://media/asset/25?q=" + std::to_string(25 * 25);
    ASSERT_FALSE(dto.uri.empty());

    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriEdgeCases_Extra_26, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    // Different URI shapes
    dto.uri = "datashare://media/asset/26?q=" + std::to_string(26 * 26);
    ASSERT_FALSE(dto.uri.empty());

    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriEdgeCases_Extra_27, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    // Different URI shapes
    dto.uri = "datashare://media/asset/27?q=" + std::to_string(27 * 27);
    ASSERT_FALSE(dto.uri.empty());

    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriEdgeCases_Extra_28, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    // Different URI shapes
    dto.uri = "datashare://media/asset/28?q=" + std::to_string(28 * 28);
    ASSERT_FALSE(dto.uri.empty());

    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriEdgeCases_Extra_29, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    // Different URI shapes
    dto.uri = "datashare://media/asset/29?q=" + std::to_string(29 * 29);
    ASSERT_FALSE(dto.uri.empty());

    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, StartAssetAnalysisDto_UriEdgeCases_Extra_30, testing::ext::TestSize.Level0)
{
    StartAssetAnalysisDto dto{};
    // Different URI shapes
    dto.uri = "datashare://media/asset/30?q=" + std::to_string(30 * 30);
    ASSERT_FALSE(dto.uri.empty());

    StartAssetAnalysisDto copy = dto;
    ASSERT_EQ(copy.uri, dto.uri);

    StartAssetAnalysisDto moved = std::move(copy);
    ASSERT_EQ(moved.uri, dto.uri);
}

HWTEST_F(MediaAnalysisDataManagerDtoTest, Dto_CopyAndAssign_OrderPositionDto, testing::ext::TestSize.Level0)
{
    for (int i = 1; i <= 30; ++i) {
        ChangeRequestSetOrderPositionDto dto;
        dto.albumId = 100 + i;
        dto.orderString = (i % 3 == 0 ? "ASC" : (i % 3 == 1 ? "DESC" : "CUSTOM"));
        dto.assetIds.clear();
        for (int k = 0; k < (i % 9 + 1); ++k) {
            dto.assetIds.emplace_back("id_" + std::to_string(i) + "_" + std::to_string(k));
        }

        ChangeRequestSetOrderPositionDto copy = dto;
        EXPECT_EQ(copy.albumId, dto.albumId) << "i=" << i;
        EXPECT_EQ(copy.orderString, dto.orderString) << "i=" << i;
        EXPECT_EQ(copy.assetIds, dto.assetIds) << "i=" << i;
    }
}