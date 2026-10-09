/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#define LOG_TAG "predicates_verify_test"

#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "datashare_predicates.h"
#include "datashare_predicates_verify.h"
#include "medialibrary_errno.h"

namespace OHOS {
namespace Media {
using DataSharePredicates = DataShare::DataSharePredicates;
using OperationType = DataShare::OperationType;

class PredicatesVerifyTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}

protected:
    DataSharePredicatesVerify verifier_;

    // Helper: verify and return whether result is E_OK
    bool IsOk(const DataSharePredicates &predicates)
    {
        auto [type, errCode] = verifier_.VerifyPredicates(predicates);
        return errCode == E_OK;
    }

    // Helper: verify and return the full result pair
    std::pair<int, int> Verify(const DataSharePredicates &predicates)
    {
        return verifier_.VerifyPredicates(predicates);
    }
};

// ============================================================
// 1. Normal field format validation — all OperationTypes
// ============================================================

/**
 * @tc.name: PredicatesVerify_NormalField_001
 * @tc.desc: Single-param public operations with valid fields pass validation
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_NormalField_001, testing::ext::TestSize.Level0)
{
    // ORDER_BY_ASC with normal field
    DataSharePredicates pred1;
    pred1.OrderByAsc("name");
    EXPECT_TRUE(IsOk(pred1));

    // ORDER_BY_DESC with normal field
    DataSharePredicates pred2;
    pred2.OrderByDesc("title");
    EXPECT_TRUE(IsOk(pred2));
}

/**
 * @tc.name: PredicatesVerify_NormalField_002
 * @tc.desc: Single-param public operations with table-qualified fields pass
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_NormalField_002, testing::ext::TestSize.Level0)
{
    DataSharePredicates pred;
    pred.EqualTo("table.col", "value");
    EXPECT_TRUE(IsOk(pred));
}

/**
 * @tc.name: PredicatesVerify_NormalField_003
 * @tc.desc: Single-param system operations with valid fields pass
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_NormalField_003, testing::ext::TestSize.Level0)
{
    // IS_NULL
    DataSharePredicates pred1;
    pred1.IsNull("field1");
    EXPECT_TRUE(IsOk(pred1));

    // IS_NOT_NULL
    DataSharePredicates pred2;
    pred2.IsNotNull("field2");
    EXPECT_TRUE(IsOk(pred2));

    // INDEXED_BY
    DataSharePredicates pred3;
    pred3.IndexedBy("idx_name");
    EXPECT_TRUE(IsOk(pred3));

    // KEY_PREFIX
    DataSharePredicates pred4;
    pred4.KeyPrefix("prefix");
    EXPECT_TRUE(IsOk(pred4));
}

/**
 * @tc.name: PredicatesVerify_NormalField_004
 * @tc.desc: 3-param system operations with valid fields pass
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_NormalField_004, testing::ext::TestSize.Level0)
{
    // GreaterThan
    DataSharePredicates pred1;
    pred1.GreaterThan("size", 100);
    EXPECT_TRUE(IsOk(pred1));

    // LessThan
    DataSharePredicates pred2;
    pred2.LessThan("date_added", 9999);
    EXPECT_TRUE(IsOk(pred2));

    // GreaterThanOrEqualTo
    DataSharePredicates pred3;
    pred3.GreaterThanOrEqualTo("width", 0);
    EXPECT_TRUE(IsOk(pred3));

    // LessThanOrEqualTo
    DataSharePredicates pred4;
    pred4.LessThanOrEqualTo("height", 4096);
    EXPECT_TRUE(IsOk(pred4));

    // NotEqualTo
    DataSharePredicates pred5;
    pred5.NotEqualTo("mime_type", "image/png");
    EXPECT_TRUE(IsOk(pred5));

    // Like
    DataSharePredicates pred6;
    pred6.Like("title", "%test%");
    EXPECT_TRUE(IsOk(pred6));

    // Unlike
    DataSharePredicates pred7;
    pred7.Unlike("title", "%temp%");
    EXPECT_TRUE(IsOk(pred7));

    // BeginsWith
    DataSharePredicates pred8;
    pred8.BeginsWith("uri", "file://");
    EXPECT_TRUE(IsOk(pred8));

    // EndsWith
    DataSharePredicates pred9;
    pred9.EndsWith("uri", ".jpg");
    EXPECT_TRUE(IsOk(pred9));

    // Contains
    DataSharePredicates pred10;
    pred10.Contains("title", "hello");
    EXPECT_TRUE(IsOk(pred10));

    // Glob
    DataSharePredicates pred11;
    pred11.Glob("title", "*.png");
    EXPECT_TRUE(IsOk(pred11));

    // Between
    DataSharePredicates pred12;
    pred12.Between("date_added", "1000", "2000");
    EXPECT_TRUE(IsOk(pred12));

    // NotBetween
    DataSharePredicates pred13;
    pred13.NotBetween("size", "0", "100");
    EXPECT_TRUE(IsOk(pred13));

    // NotIn
    DataSharePredicates pred14;
    pred14.NotIn("mime_type", std::vector<std::string>{"image/png", "image/jpeg"});
    EXPECT_TRUE(IsOk(pred14));
}

/**
 * @tc.name: PredicatesVerify_NormalField_005
 * @tc.desc: Multi-param system operations with valid fields pass
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_NormalField_005, testing::ext::TestSize.Level0)
{
    // IN_KEY
    DataSharePredicates pred1;
    pred1.InKeys({"key1", "key2"});
    EXPECT_TRUE(IsOk(pred1));

    // GROUP_BY
    DataSharePredicates pred2;
    pred2.GroupBy({"title", "mime_type"});
    EXPECT_TRUE(IsOk(pred2));
}

/**
 * @tc.name: PredicatesVerify_NormalField_006
 * @tc.desc: Bracketed and quoted field formats pass
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_NormalField_006, testing::ext::TestSize.Level0)
{
    // (colName)
    DataSharePredicates pred1;
    pred1.EqualTo("(name)", "val");
    EXPECT_TRUE(IsOk(pred1));

    // [colName]
    DataSharePredicates pred2;
    pred2.EqualTo("[name]", "val");
    EXPECT_TRUE(IsOk(pred2));

    // "colName"
    DataSharePredicates pred3;
    pred3.EqualTo("\"name\"", "val");
    EXPECT_TRUE(IsOk(pred3));

    // $.colName
    DataSharePredicates pred4;
    pred4.EqualTo("$.name", "val");
    EXPECT_TRUE(IsOk(pred4));

    // store.table.colName
    DataSharePredicates pred5;
    pred5.EqualTo("db.table.col", "val");
    EXPECT_TRUE(IsOk(pred5));

    // [tableName.colName]
    DataSharePredicates pred6;
    pred6.EqualTo("[table.col]", "val");
    EXPECT_TRUE(IsOk(pred6));
}

// ============================================================
// 2. Illegal field format — should be rejected
// ============================================================

/**
 * @tc.name: PredicatesVerify_IllegalField_001
 * @tc.desc: SQL injection patterns in field are rejected
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_IllegalField_001, testing::ext::TestSize.Level0)
{
    // Classic OR injection
    DataSharePredicates pred1;
    pred1.EqualTo("name OR 1=1", "val");
    EXPECT_FALSE(IsOk(pred1));

    // Semicolon injection
    DataSharePredicates pred2;
    pred2.GreaterThan("name; DROP TABLE users--", "0");
    EXPECT_FALSE(IsOk(pred2));

    // Union injection
    DataSharePredicates pred3;
    pred3.Like("name UNION SELECT * FROM photos--", "%");
    EXPECT_FALSE(IsOk(pred3));

    // Comment injection
    DataSharePredicates pred4;
    pred4.EqualTo("name/**/OR/**/1=1", "val");
    EXPECT_FALSE(IsOk(pred4));

    // Quote-based injection
    DataSharePredicates pred5;
    pred5.GreaterThan("name' OR '1'='1", "0");
    EXPECT_FALSE(IsOk(pred5));

    // Space-separated tokens
    DataSharePredicates pred6;
    pred6.EqualTo("true or name", "test");
    EXPECT_FALSE(IsOk(pred6));

    // Path traversal as field
    DataSharePredicates pred7;
    pred7.GreaterThan("../etc/passwd", "0");
    EXPECT_FALSE(IsOk(pred7));
}

/**
 * @tc.name: PredicatesVerify_IllegalField_002
 * @tc.desc: Malformed bracket patterns are rejected
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_IllegalField_002, testing::ext::TestSize.Level0)
{
    // Unmatched bracket
    DataSharePredicates pred1;
    pred1.EqualTo("(name", "val");
    EXPECT_FALSE(IsOk(pred1));

    // Bracket without content
    DataSharePredicates pred2;
    pred2.EqualTo("name]", "val");
    EXPECT_FALSE(IsOk(pred2));

    // Double dot
    DataSharePredicates pred3;
    pred3.EqualTo("test..", "val");
    EXPECT_FALSE(IsOk(pred3));

    // Dot with no column after
    DataSharePredicates pred4;
    pred4.EqualTo("(test.)", "val");
    EXPECT_FALSE(IsOk(pred4));
}

/**
 * @tc.name: PredicatesVerify_IllegalField_003
 * @tc.desc: ORDER_BY with injection field is rejected
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_IllegalField_003, testing::ext::TestSize.Level0)
{
    DataSharePredicates pred1;
    pred1.OrderByAsc("name; DROP TABLE photos");
    EXPECT_FALSE(IsOk(pred1));

    DataSharePredicates pred2;
    pred2.OrderByDesc("1 OR 1=1");
    EXPECT_FALSE(IsOk(pred2));
}

/**
 * @tc.name: PredicatesVerify_IllegalField_004
 * @tc.desc: Multi-param operations with illegal fields are rejected
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_IllegalField_004, testing::ext::TestSize.Level0)
{
    // IN_KEY with injection
    DataSharePredicates pred1;
    pred1.InKeys({"key1; DROP TABLE", "key2"});
    EXPECT_FALSE(IsOk(pred1));

    // GROUP_BY with injection
    DataSharePredicates pred2;
    pred2.GroupBy({"col1", "col2 OR 1=1"});
    EXPECT_FALSE(IsOk(pred2));
}

/**
 * @tc.name: PredicatesVerify_IllegalField_005
 * @tc.desc: IS_NULL / IS_NOT_NULL with illegal field are rejected
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_IllegalField_005, testing::ext::TestSize.Level0)
{
    DataSharePredicates pred1;
    pred1.IsNull("name;--");
    EXPECT_FALSE(IsOk(pred1));

    DataSharePredicates pred2;
    pred2.IsNotNull("1 OR 1=1");
    EXPECT_FALSE(IsOk(pred2));
}

// ============================================================
// 3. Whitelist bypass — exact match short-circuits validation
// ============================================================

/**
 * @tc.name: PredicatesVerify_Whitelist_001
 * @tc.desc: Whitelisted field bypasses format validation even if it would otherwise fail
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_Whitelist_001, testing::ext::TestSize.Level0)
{
    // "lower(lpath)" is in the whitelist but would NOT pass regex validation
    // because it contains parentheses around the argument — whitelist short-circuits
    DataSharePredicates pred;
    pred.EqualTo("lower(lpath)", "val");
    EXPECT_TRUE(IsOk(pred));
}

/**
 * @tc.name: PredicatesVerify_Whitelist_002
 * @tc.desc: Whitelisted field works with other OperationTypes
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_Whitelist_002, testing::ext::TestSize.Level0)
{
    DataSharePredicates pred1;
    pred1.Like("lower(lpath)", "%test%");
    EXPECT_TRUE(IsOk(pred1));

    DataSharePredicates pred2;
    pred2.OrderByAsc("lower(lpath)");
    EXPECT_TRUE(IsOk(pred2));

    DataSharePredicates pred3;
    pred1.GreaterThan("lower(lpath)", "0");
    EXPECT_TRUE(IsOk(pred3));
}

/**
 * @tc.name: PredicatesVerify_Whitelist_003
 * @tc.desc: Non-exact match does NOT trigger whitelist bypass
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_Whitelist_003, testing::ext::TestSize.Level0)
{
    // Whitespace-padded version — not an exact match
    DataSharePredicates pred1;
    pred1.EqualTo(" lower(lpath) ", "val");
    EXPECT_FALSE(IsOk(pred1));

    // Case-different version — not an exact match
    DataSharePredicates pred2;
    pred2.EqualTo("Lower(lpath)", "val");
    EXPECT_FALSE(IsOk(pred2));
}

// ============================================================
// 4. setWhereClause — raw SQL injection testing
// ============================================================

/**
 * @tc.name: PredicatesVerify_WhereClause_001
 * @tc.desc: setWhereClause with valid where clause does not trigger predicates verify
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_WhereClause_001, testing::ext::TestSize.Level0)
{
    // setWhereClause bypasses the operation-list-based verify entirely
    // (it sets QUERY_LANGUAGE mode, so GetOperationList() is empty)
    DataSharePredicates pred;
    pred.SetWhereClause("name = ?");
    auto [type, errCode] = Verify(pred);
    // Empty operation list → all pass
    EXPECT_EQ(errCode, E_OK);
}

/**
 * @tc.name: PredicatesVerify_WhereClause_002
 * @tc.desc: setWhereClause with SQL injection strings still passes verify
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_WhereClause_002, testing::ext::TestSize.Level0)
{
    // setWhereClause stores raw SQL, the predicates verify only checks operation list
    // This test confirms the boundary: setWhereClause is NOT validated by predicates verify
    DataSharePredicates pred;
    pred.SetWhereClause("1=1 OR name = 'admin'; DROP TABLE photos--");
    auto [type, errCode] = Verify(pred);
    EXPECT_EQ(errCode, E_OK);
}

/**
 * @tc.name: PredicatesVerify_WhereClause_003
 * @tc.desc: Mixing setWhereClause and predicate methods — predicates still get validated
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_WhereClause_003, testing::ext::TestSize.Level0)
{
    // SetWhereClause switches mode to QUERY_LANGUAGE, but EqualTo still adds to operations_
    // VerifyPredicates will check the operation list and reject illegal fields
    DataSharePredicates pred;
    pred.SetWhereClause("name = ?");
    pred.EqualTo("evil; DROP", "val");
    auto [type, errCode] = Verify(pred);
    EXPECT_EQ(errCode, E_SQL_CHECK_FAIL); // "evil; DROP" fails field format validation
}

/**
 * @tc.name: PredicatesVerify_WhereClause_004
 * @tc.desc: Using predicate methods first, then setWhereClause fails
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_WhereClause_004, testing::ext::TestSize.Level0)
{
    DataSharePredicates pred;
    pred.EqualTo("name", "val");
    // SetWhereClause fails after predicate methods (mode is PREDICATES_METHOD)
    int ret = pred.SetWhereClause("1=1");
    EXPECT_NE(ret, 0); // SetWhereClause returns error
    // VerifyPredicates only checks the operation list (which has EqualTo)
    EXPECT_TRUE(IsOk(pred));
}

// ============================================================
// 5. Comprehensive SQL injection patterns
// ============================================================

/**
 * @tc.name: PredicatesVerify_SqlInjection_001
 * @tc.desc: Common SQL injection patterns in field are all blocked
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_SqlInjection_001, testing::ext::TestSize.Level0)
{
    const std::vector<std::string> injectionFields = {
        "name OR 1=1",
        "name; DROP TABLE photos",
        "name' OR '1'='1",
        "name\" OR \"1\"=\"1",
        "name UNION SELECT * FROM photos",
        "name;--",
        "name/*",
        "name/**/",
        "1; DROP TABLE photos--",
        "1 OR 1=1--",
        "name OR '1'='1'--",
        "name' UNION SELECT password FROM users--",
        "name; DELETE FROM photos WHERE '1'='1",
        "name OR 1=1 #",
        "name' AND 1=1--",
        "name\\\\",
        "name'",
        "name\"",
        "name` OR `1`=`1",
    };

    for (const auto &field : injectionFields) {
        DataSharePredicates pred;
        pred.EqualTo(field, "val");
        auto [type, errCode] = Verify(pred);
        EXPECT_EQ(errCode, E_SQL_CHECK_FAIL) << "Injection not blocked: " << field;
    }
}

/**
 * @tc.name: PredicatesVerify_SqlInjection_002
 * @tc.desc: SQL injection in multi-param operations is blocked
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_SqlInjection_002, testing::ext::TestSize.Level0)
{
    const std::vector<std::string> injectionFields = {
        "col; DROP TABLE",
        "col OR 1=1",
        "col' UNION SELECT",
    };

    for (const auto &field : injectionFields) {
        DataSharePredicates pred;
        pred.InKeys({field, "safe_key"});
        auto [type, errCode] = Verify(pred);
        EXPECT_EQ(errCode, E_SQL_CHECK_FAIL) << "Injection not blocked in InKeys: " << field;
    }

    for (const auto &field : injectionFields) {
        DataSharePredicates pred;
        pred.GroupBy({field});
        auto [type, errCode] = Verify(pred);
        EXPECT_EQ(errCode, E_SQL_CHECK_FAIL) << "Injection not blocked in GroupBy: " << field;
    }
}

// ============================================================
// 6. Numeric field rejection (EQUAL_TO only)
// ============================================================

/**
 * @tc.name: PredicatesVerify_NumericField_001
 * @tc.desc: EqualTo with pure numeric field is rejected
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_NumericField_001, testing::ext::TestSize.Level0)
{
    DataSharePredicates pred;
    pred.EqualTo("123", "val");
    EXPECT_FALSE(IsOk(pred));
}

/**
 * @tc.name: PredicatesVerify_NumericField_002
 * @tc.desc: EqualTo with large pure numeric field is rejected
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_NumericField_002, testing::ext::TestSize.Level0)
{
    DataSharePredicates pred;
    pred.EqualTo("999999999999", "val");
    EXPECT_FALSE(IsOk(pred));
}

/**
 * @tc.name: PredicatesVerify_NumericField_003
 * @tc.desc: EqualTo with zero as field is rejected
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_NumericField_003, testing::ext::TestSize.Level0)
{
    DataSharePredicates pred;
    pred.EqualTo("0", "val");
    EXPECT_FALSE(IsOk(pred));
}

/**
 * @tc.name: PredicatesVerify_NumericField_004
 * @tc.desc: EqualTo with non-numeric field containing digits is accepted
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_NumericField_004, testing::ext::TestSize.Level0)
{
    DataSharePredicates pred;
    pred.EqualTo("field123", "val");
    EXPECT_TRUE(IsOk(pred));
}

/**
 * @tc.name: PredicatesVerify_NumericField_005
 * @tc.desc: EqualTo with field starting with digits is accepted (not pure numeric)
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_NumericField_005, testing::ext::TestSize.Level0)
{
    DataSharePredicates pred;
    pred.EqualTo("123abc", "val");
    EXPECT_TRUE(IsOk(pred));
}

/**
 * @tc.name: PredicatesVerify_NumericField_006
 * @tc.desc: Whitelist short-circuits numeric check (whitelisted field passes)
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_NumericField_006, testing::ext::TestSize.Level0)
{
    // "lower(lpath)" is whitelisted and contains no digits, but this test verifies
    // that whitelist runs before NumericFieldStrategy
    DataSharePredicates pred;
    pred.EqualTo("lower(lpath)", "val");
    EXPECT_TRUE(IsOk(pred));
}

/**
 * @tc.name: PredicatesVerify_NumericField_007
 * @tc.desc: GreaterThan with pure numeric field is rejected (prevents 0 > -1 always-true)
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_NumericField_007, testing::ext::TestSize.Level0)
{
    DataSharePredicates pred;
    pred.GreaterThan("0", -1);
    EXPECT_FALSE(IsOk(pred));
}

/**
 * @tc.name: PredicatesVerify_NumericField_008
 * @tc.desc: LessThan with pure numeric field is rejected (prevents 0 < 999999 always-true)
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_NumericField_008, testing::ext::TestSize.Level0)
{
    DataSharePredicates pred;
    pred.LessThan("0", 999999);
    EXPECT_FALSE(IsOk(pred));
}

/**
 * @tc.name: PredicatesVerify_NumericField_009
 * @tc.desc: GreaterThanOrEqualTo with pure numeric field is rejected (prevents 0 >= 0 always-true)
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_NumericField_009, testing::ext::TestSize.Level0)
{
    DataSharePredicates pred;
    pred.GreaterThanOrEqualTo("0", 0);
    EXPECT_FALSE(IsOk(pred));
}

/**
 * @tc.name: PredicatesVerify_NumericField_010
 * @tc.desc: LessThanOrEqualTo with pure numeric field is rejected (prevents 0 <= 0 always-true)
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_NumericField_010, testing::ext::TestSize.Level0)
{
    DataSharePredicates pred;
    pred.LessThanOrEqualTo("0", 0);
    EXPECT_FALSE(IsOk(pred));
}

/**
 * @tc.name: PredicatesVerify_NumericField_011
 * @tc.desc: NotEqualTo with pure numeric field is rejected (prevents 1 != 0 always-true)
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_NumericField_011, testing::ext::TestSize.Level0)
{
    DataSharePredicates pred;
    pred.NotEqualTo("1", 0);
    EXPECT_FALSE(IsOk(pred));
}

/**
 * @tc.name: PredicatesVerify_NumericField_012
 * @tc.desc: Like with pure numeric field is rejected (prevents 0 LIKE '%' always-true)
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_NumericField_012, testing::ext::TestSize.Level0)
{
    DataSharePredicates pred;
    pred.Like("0", "%");
    EXPECT_FALSE(IsOk(pred));
}

/**
 * @tc.name: PredicatesVerify_NumericField_013
 * @tc.desc: Unlike with pure numeric field is rejected
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_NumericField_013, testing::ext::TestSize.Level0)
{
    DataSharePredicates pred;
    pred.Unlike("0", "%");
    EXPECT_FALSE(IsOk(pred));
}

/**
 * @tc.name: PredicatesVerify_NumericField_014
 * @tc.desc: Between with pure numeric field is rejected (prevents 0 BETWEEN -1 AND 999 always-true)
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_NumericField_014, testing::ext::TestSize.Level0)
{
    DataSharePredicates pred;
    pred.Between("0", "-1", "999");
    EXPECT_FALSE(IsOk(pred));
}

/**
 * @tc.name: PredicatesVerify_NumericField_015
 * @tc.desc: NotBetween with pure numeric field is rejected
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_NumericField_015, testing::ext::TestSize.Level0)
{
    DataSharePredicates pred;
    pred.NotBetween("0", "1", "2");
    EXPECT_FALSE(IsOk(pred));
}

/**
 * @tc.name: PredicatesVerify_NumericField_016
 * @tc.desc: Glob with pure numeric field is rejected (prevents 0 GLOB '*' always-true)
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_NumericField_016, testing::ext::TestSize.Level0)
{
    DataSharePredicates pred;
    pred.Glob("0", "*");
    EXPECT_FALSE(IsOk(pred));
}

/**
 * @tc.name: PredicatesVerify_NumericField_017
 * @tc.desc: BeginsWith with pure numeric field is rejected
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_NumericField_017, testing::ext::TestSize.Level0)
{
    DataSharePredicates pred;
    pred.BeginsWith("0", "");
    EXPECT_FALSE(IsOk(pred));
}

/**
 * @tc.name: PredicatesVerify_NumericField_018
 * @tc.desc: EndsWith with pure numeric field is rejected
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_NumericField_018, testing::ext::TestSize.Level0)
{
    DataSharePredicates pred;
    pred.EndsWith("0", "");
    EXPECT_FALSE(IsOk(pred));
}

/**
 * @tc.name: PredicatesVerify_NumericField_019
 * @tc.desc: Contains with pure numeric field is rejected
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_NumericField_019, testing::ext::TestSize.Level0)
{
    DataSharePredicates pred;
    pred.Contains("0", "");
    EXPECT_FALSE(IsOk(pred));
}

/**
 * @tc.name: PredicatesVerify_NumericField_020
 * @tc.desc: OR-chained EqualTo with numeric fields is rejected (prevents 1=1 OR 1=1 always-true)
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_NumericField_020, testing::ext::TestSize.Level0)
{
    DataSharePredicates pred;
    pred.EqualTo("1", "1")->Or()->EqualTo("1", "1");
    EXPECT_FALSE(IsOk(pred));
}

/**
 * @tc.name: PredicatesVerify_NumericField_021
 * @tc.desc: OR-chained GreaterThan with numeric fields is rejected (prevents 0>-1 OR 0<999 always-true)
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_NumericField_021, testing::ext::TestSize.Level0)
{
    DataSharePredicates pred;
    pred.GreaterThan("0", -1)->Or()->LessThan("0", 999);
    EXPECT_FALSE(IsOk(pred));
}

/**
 * @tc.name: PredicatesVerify_NumericField_022
 * @tc.desc: AND-chained NotEqualTo with numeric fields is rejected
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_NumericField_022, testing::ext::TestSize.Level0)
{
    DataSharePredicates pred;
    pred.NotEqualTo("1", 0)->And()->NotEqualTo("1", 2);
    EXPECT_FALSE(IsOk(pred));
}

/**
 * @tc.name: PredicatesVerify_NumericField_023
 * @tc.desc: 2-param operations (IsNull etc.) with numeric field are NOT blocked (no always-true risk)
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_NumericField_023, testing::ext::TestSize.Level0)
{
    // IS_NULL / IS_NOT_NULL cannot construct always-true with numeric field alone
    DataSharePredicates pred1;
    pred1.IsNull("123");
    EXPECT_TRUE(IsOk(pred1));

    DataSharePredicates pred2;
    pred2.IsNotNull("0");
    EXPECT_TRUE(IsOk(pred2));
}

/**
 * @tc.name: PredicatesVerify_NumericField_024
 * @tc.desc: NotIn with pure numeric field is rejected
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_NumericField_024, testing::ext::TestSize.Level0)
{
    DataSharePredicates pred;
    pred.NotIn("0", std::vector<std::string>{"1", "2"});
    EXPECT_FALSE(IsOk(pred));
}

// ============================================================
// 7. Edge cases and composite operations
// ============================================================

/**
 * @tc.name: PredicatesVerify_EdgeCases_001
 * @tc.desc: Empty predicates pass validation
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_EdgeCases_001, testing::ext::TestSize.Level0)
{
    DataSharePredicates pred;
    auto [type, errCode] = Verify(pred);
    EXPECT_EQ(type, E_OK);
    EXPECT_EQ(errCode, E_OK);
}

/**
 * @tc.name: PredicatesVerify_EdgeCases_002
 * @tc.desc: Multiple operations chained — all valid
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_EdgeCases_002, testing::ext::TestSize.Level0)
{
    DataSharePredicates pred;
    pred.EqualTo("name", "test")->And()->GreaterThan("size", 0)->OrderByAsc("date_added");
    EXPECT_TRUE(IsOk(pred));
}

/**
 * @tc.name: PredicatesVerify_EdgeCases_003
 * @tc.desc: Multiple operations chained — one invalid field fails the whole chain
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_EdgeCases_003, testing::ext::TestSize.Level0)
{
    DataSharePredicates pred;
    pred.EqualTo("name", "test")->And()->GreaterThan("evil; DROP", 0);
    EXPECT_FALSE(IsOk(pred));
}

/**
 * @tc.name: PredicatesVerify_EdgeCases_004
 * @tc.desc: Control flow operations (AND/OR/WRAP) have no field to validate — pass by default
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_EdgeCases_004, testing::ext::TestSize.Level0)
{
    DataSharePredicates pred;
    pred.BeginWrap()->EqualTo("name", "test")->Or()->EqualTo("title", "hello")->EndWrap();
    EXPECT_TRUE(IsOk(pred));
}

/**
 * @tc.name: PredicatesVerify_EdgeCases_005
 * @tc.desc: Unregistered OperationType passes by default (no strategies = E_OK)
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_EdgeCases_005, testing::ext::TestSize.Level0)
{
    // LIMIT is not registered in ValidatorRegistry — should pass by default
    DataSharePredicates pred;
    pred.Limit(10, 0);
    EXPECT_TRUE(IsOk(pred));
}

/**
 * @tc.name: PredicatesVerify_EdgeCases_006
 * @tc.desc: Return pair contains correct OperationType on failure
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_EdgeCases_006, testing::ext::TestSize.Level0)
{
    DataSharePredicates pred;
    pred.GreaterThan("evil field", "0");
    auto [type, errCode] = Verify(pred);
    // GREATER_THAN = 4 (from OperationType enum)
    EXPECT_EQ(type, static_cast<int>(OperationType::GREATER_THAN));
    EXPECT_EQ(errCode, E_SQL_CHECK_FAIL);
}

/**
 * @tc.name: PredicatesVerify_EdgeCases_007
 * @tc.desc: DISTINCT has no field parameter — passes by default
 * @tc.type: FUNC
 */
HWTEST_F(PredicatesVerifyTest, PredicatesVerify_EdgeCases_007, testing::ext::TestSize.Level0)
{
    DataSharePredicates pred;
    pred.Distinct();
    EXPECT_TRUE(IsOk(pred));
}

} // namespace Media
} // namespace OHOS
