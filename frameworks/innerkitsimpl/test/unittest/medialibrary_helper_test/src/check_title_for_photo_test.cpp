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

#include "medialibrary_helper_test.h"

#include "media_file_utils.h"
#include "medialibrary_errno.h"
#include "userfile_manager_types.h"

#include <string>

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {

//
// CheckTitleForPhoto 函数单元测试
// 函数签名: static int32_t CheckTitleForPhoto(const string& title)
// 逻辑:
//   1. 空标题返回 -EINVAL
//   2. 以"."开头的标题返回 -EINVAL (新增逻辑)
//   3. 包含非法字符 \/:*?"'`<>|{}[] 返回 -EINVAL
//   4. 其他合法标题返回 E_OK (允许中间包含.)
//
// ------------------------
// -------- 正向用例 --------
//
// 合法标题: 普通英文字符
HWTEST_F(MediaLibraryHelperUnitTest, CheckTitleForPhoto_NormalTitle_001, TestSize.Level1)
{
    string title = "photo001";
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), E_OK);
}

// 合法标题: 包含点号 (不以.开头)
HWTEST_F(MediaLibraryHelperUnitTest, CheckTitleForPhoto_DotInMiddle_001, TestSize.Level1)
{
    string title = "photo.test";
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), E_OK);
}

// 合法标题：包含多个点号
HWTEST_F(MediaLibraryHelperUnitTest, CheckTitleForPhoto_MultipleDots_001, TestSize.Level1)
{
    string title = "my.photo.2026";
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), E_OK);
}

// 合法标题：单字符
HWTEST_F(MediaLibraryHelperUnitTest, CheckTitleForPhoto_SingleChar_001, TestSize.Level1)
{
    string title = "a";
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), E_OK);
}

// 合法标题：中文
HWTEST_F(MediaLibraryHelperUnitTest, CheckTitleForPhoto_ChineseTitle_001, TestSize.Level1)
{
    string title = "我的照片";
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), E_OK);
}

// 合法标题：包含空格
HWTEST_F(MediaLibraryHelperUnitTest, CheckTitleForPhoto_WithSpace_001, TestSize.Level1)
{
    string title = "photo test";
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), E_OK);
}

// 合法标题：包含数字和字母混合
HWTEST_F(MediaLibraryHelperUnitTest, CheckTitleForPhoto_Alphanumeric_001, TestSize.Level1)
{
    string title = "IMG_20260101_001";
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), E_OK);
}

// -------- 负向用例 --------

// 非法标题：空串
HWTEST_F(MediaLibraryHelperUnitTest, CheckTitleForPhoto_Empty_001, TestSize.Level1)
{
    string title = "";
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), -EINVAL);
}

// 非法标题：以.开头
HWTEST_F(MediaLibraryHelperUnitTest, CheckTitleForPhoto_StartWithDot_001, TestSize.Level1)
{
    string title = ".hidden";
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), -EINVAL);
}

// 非法标题：以.开头且包含点号
HWTEST_F(MediaLibraryHelperUnitTest, CheckTitleForPhoto_StartWithDot_002, TestSize.Level1)
{
    string title = ".photo.test";
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), -EINVAL);
}

// 非法标题：仅一个点号
HWTEST_F(MediaLibraryHelperUnitTest, CheckTitleForPhoto_OnlyDot_001, TestSize.Level1)
{
    string title = ".";
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), -EINVAL);
}

// 非法标题：包含反斜杠
HWTEST_F(MediaLibraryHelperUnitTest, CheckTitleForPhoto_Backslash_001, TestSize.Level1)
{
    string title = "test\\file";
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), -EINVAL);
}

// 非法标题：包含正斜杠
HWTEST_F(MediaLibraryHelperUnitTest, CheckTitleForPhoto_Slash_001, TestSize.Level1)
{
    string title = "test/file";
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), -EINVAL);
}

// 非法标题：包含冒号
HWTEST_F(MediaLibraryHelperUnitTest, CheckTitleForPhoto_Colon_001, TestSize.Level1)
{
    string title = "test:file";
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), -EINVAL);
}

// 非法标题：包含星号
HWTEST_F(MediaLibraryHelperUnitTest, CheckTitleForPhoto_Asterisk_001, TestSize.Level1)
{
    string title = "test*file";
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), -EINVAL);
}

// 非法标题：包含问号
HWTEST_F(MediaLibraryHelperUnitTest, CheckTitleForPhoto_QuestionMark_001, TestSize.Level1)
{
    string title = "test?file";
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), -EINVAL);
}

// 非法标题：包含双引号
HWTEST_F(MediaLibraryHelperUnitTest, CheckTitleForPhoto_DoubleQuote_001, TestSize.Level1)
{
    string title = "test\"file";
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), -EINVAL);
}

// 非法标题：包含单引号（单引号属于非法字符集，应拒绝）
HWTEST_F(MediaLibraryHelperUnitTest, CheckTitleForPhoto_SingleQuote_001, TestSize.Level1)
{
    string title = "test'file";
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), -EINVAL);
}

// 非法标题：包含反引号（反引号属于非法字符集，应拒绝）
HWTEST_F(MediaLibraryHelperUnitTest, CheckTitleForPhoto_Backtick_001, TestSize.Level1)
{
    string title = "test`file";
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), -EINVAL);
}

// 非法标题：包含小于号
HWTEST_F(MediaLibraryHelperUnitTest, CheckTitleForPhoto_LessThan_001, TestSize.Level1)
{
    string title = "test<file";
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), -EINVAL);
}

// 非法标题：包含大于号
HWTEST_F(MediaLibraryHelperUnitTest, CheckTitleForPhoto_GreaterThan_001, TestSize.Level1)
{
    string title = "test>file";
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), -EINVAL);
}

// 非法标题：包含竖线
HWTEST_F(MediaLibraryHelperUnitTest, CheckTitleForPhoto_Pipe_001, TestSize.Level1)
{
    string title = "test|file";
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), -EINVAL);
}

// 非法标题：包含左花括号
HWTEST_F(MediaLibraryHelperUnitTest, CheckTitleForPhoto_LeftBrace_001, TestSize.Level1)
{
    string title = "test{file";
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), -EINVAL);
}

// 非法标题：包含右花括号
HWTEST_F(MediaLibraryHelperUnitTest, CheckTitleForPhoto_RightBrace_001, TestSize.Level1)
{
    string title = "test}file";
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), -EINVAL);
}

// 非法标题：包含左方括号
HWTEST_F(MediaLibraryHelperUnitTest, CheckTitleForPhoto_LeftBracket_001, TestSize.Level1)
{
    string title = "test[file";
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), -EINVAL);
}

// 非法标题：包含右方括号
HWTEST_F(MediaLibraryHelperUnitTest, CheckTitleForPhoto_RightBracket_001, TestSize.Level1)
{
    string title = "test]file";
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), -EINVAL);
}

// 非法标题：包含多个非法字符
HWTEST_F(MediaLibraryHelperUnitTest, CheckTitleForPhoto_MultipleIllegal_001, TestSize.Level1)
{
    string title = "test:*?<>|";
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), -EINVAL);
}

// -------- 边界用例 --------

// 边界用例：单字符点号不再单独设用例——与 CheckTitleForPhoto_OnlyDot_001（上方）
// 输入参数值与断言逻辑完全相同（title="."），避免重复用例。

// 边界用例：两个字符以.开头
HWTEST_F(MediaLibraryHelperUnitTest, CheckTitleForPhoto_Boundary_TwoCharsDot_001, TestSize.Level1)
{
    string title = ".a";
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), -EINVAL);
}

// 边界用例：第二字符是点号 (不以.开头，合法)
HWTEST_F(MediaLibraryHelperUnitTest, CheckTitleForPhoto_Boundary_SecondCharDot_001, TestSize.Level1)
{
    string title = "a.";
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), E_OK);
}

// 边界用例：最大长度标题 (255字符)
HWTEST_F(MediaLibraryHelperUnitTest, CheckTitleForPhoto_Boundary_MaxLength_001, TestSize.Level1)
{
    string title(255, 'a');
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), E_OK);
}

// 边界用例：超过最大长度标题（256字符）- CheckTitleForPhoto 本身不做长度检查
HWTEST_F(MediaLibraryHelperUnitTest, CheckTitleForPhoto_Boundary_OverLength_001, TestSize.Level1)
{
    string title(256, 'a');
    // CheckTitleForPhoto 不做长度校验，只做空/点号/非法字符校验
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), E_OK);
}

// 边界用例：含点号且含非法字符（点号合法但非法字符应被拒绝）
HWTEST_F(MediaLibraryHelperUnitTest, CheckTitleForPhoto_Boundary_DotWithIllegal_001, TestSize.Level1)
{
    string title = "photo.test:file";
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), -EINVAL);
}

// 边界用例：含点号且含花括号（点号合法但花括号非法）
HWTEST_F(MediaLibraryHelperUnitTest, CheckTitleForPhoto_Boundary_DotWithBraces_001, TestSize.Level1)
{
    string title = "photo{test}.file";
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), -EINVAL);
}

// 边界用例：含点号且含方括号
HWTEST_F(MediaLibraryHelperUnitTest, CheckTitleForPhoto_Boundary_DotWithBrackets_001, TestSize.Level1)
{
    string title = "photo[test].file";
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), -EINVAL);
}

// 边界用例：含点号且含反引号（反引号非法，应拒绝）
HWTEST_F(MediaLibraryHelperUnitTest, CheckTitleForPhoto_Boundary_DotWithBacktick_001, TestSize.Level1)
{
    string title = "photo`test.file";
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), -EINVAL);
}

// 边界用例：含点号且含单引号（单引号非法，应拒绝）
HWTEST_F(MediaLibraryHelperUnitTest, CheckTitleForPhoto_Boundary_DotWithSingleQuote_001, TestSize.Level1)
{
    string title = "photo'test.file";
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), -EINVAL);
}

// ==========================================
// CheckDisplayName 函数单元测试（统一点号规则）
// 函数签名: static int32_t CheckDisplayName(const string &displayName,
//     const bool compatibleCheckTitle = false, const bool useDotCompatibleRule = false)
// 逻辑:
//   1. 长度校验 (>255返回 -ENAMETOOLONG)
//   2. 以.开头返回 -EINVAL
//   3. displayName 为 "." 或 ".." 返回 -EINVAL
//   4. GetTitleFromDisplayName 提取 title，为空返回 -EINVAL
//   5. compatibleCheckTitle true/false 均走 CheckTitleForPhoto
// ==========================================

// -------- 正向用例 --------

// 合法displayName: 包含点号
// 注：compatibleCheckTitle true/false 两分支在 SUT 中均调 CheckTitleForPhoto，
// 参数变体（CompatibleTrue/CompatibleFalse/BothTrue）与 Normal_001 执行路径 100% 相同，不再重复设用例。
HWTEST_F(MediaLibraryHelperUnitTest, CheckDisplayName_DotRule_Normal_001, TestSize.Level1)
{
    string displayName = "photo.test.jpg";
    EXPECT_EQ(MediaFileUtils::CheckDisplayName(displayName), E_OK);
}

// 合法displayName: 包含多个点号
HWTEST_F(MediaLibraryHelperUnitTest, CheckDisplayName_DotRule_MultiDots_001, TestSize.Level1)
{
    string displayName = "my.photo.2026.jpg";
    EXPECT_EQ(MediaFileUtils::CheckDisplayName(displayName), E_OK);
}

// 合法displayName: compatibleCheckTitle=true 包含点号
HWTEST_F(MediaLibraryHelperUnitTest, CheckDisplayName_DotRule_CompatibleTrue_WithDot_001, TestSize.Level1)
{
    string displayName = "Com.hahaha.jpg";
    EXPECT_EQ(MediaFileUtils::CheckDisplayName(displayName, true, false), E_OK);
}

// -------- 负向用例 --------

// 非法displayName: 以.开头
// 注：以 '.' 开头的检查在 SUT 中先于 compatibleCheckTitle/useDotCompatibleRule 读取前返回，
// 参数变体（CompatibleTrue/TitleStartWithDot/UseDotCompat/BothTrue）与 StartWithDot_001
// 执行路径 100% 相同，不再重复设用例。
HWTEST_F(MediaLibraryHelperUnitTest, CheckDisplayName_DotRule_StartWithDot_001, TestSize.Level1)
{
    string displayName = ".hidden.jpg";
    // 以 '.' 开头应返回 -EINVAL，使用等式断言精准验证错误码
    EXPECT_EQ(MediaFileUtils::CheckDisplayName(displayName), -EINVAL);
}

// 非法displayName: 仅"."
HWTEST_F(MediaLibraryHelperUnitTest, CheckDisplayName_DotRule_OnlyDot_001, TestSize.Level1)
{
    string displayName = ".";
    // 以 '.' 开头应返回 -EINVAL，使用等式断言精准验证错误码
    EXPECT_EQ(MediaFileUtils::CheckDisplayName(displayName), -EINVAL);
}

// 非法displayName: 仅".."
HWTEST_F(MediaLibraryHelperUnitTest, CheckDisplayName_DotRule_OnlyDoubleDot_001, TestSize.Level1)
{
    string displayName = "..";
    // 以 '.' 开头应返回 -EINVAL，使用等式断言精准验证错误码
    EXPECT_EQ(MediaFileUtils::CheckDisplayName(displayName), -EINVAL);
}

// 非法displayName: 空串
HWTEST_F(MediaLibraryHelperUnitTest, CheckDisplayName_DotRule_Empty_001, TestSize.Level1)
{
    string displayName = "";
    // 空串由 CheckStringSize 拒绝，应返回 -EINVAL，使用等式断言精准验证错误码
    EXPECT_EQ(MediaFileUtils::CheckDisplayName(displayName), -EINVAL);
}

// 非法displayName: 无扩展名 (GetTitleFromDisplayName 返回空)
HWTEST_F(MediaLibraryHelperUnitTest, CheckDisplayName_DotRule_NoExtension_001, TestSize.Level1)
{
    string displayName = "testfile";
    // 无扩展名应返回 -EINVAL，使用等式断言精准验证错误码
    EXPECT_EQ(MediaFileUtils::CheckDisplayName(displayName), -EINVAL);
}

// 非法displayName: title 含非法字符 (CheckTitleForPhoto 校验)
HWTEST_F(MediaLibraryHelperUnitTest, CheckDisplayName_DotRule_TitleWithIllegal_001, TestSize.Level1)
{
    string displayName = "test*?.jpg";
    // title 含非法字符 '?'，CheckTitleForPhoto 应返回 -EINVAL，使用等式断言精准验证错误码
    EXPECT_EQ(MediaFileUtils::CheckDisplayName(displayName), -EINVAL);
}

// 合法displayName: 含点号 + useDotCompatibleRule=true (应通过)
HWTEST_F(MediaLibraryHelperUnitTest, CheckDisplayName_DotRule_WithDot_UseDotCompat_001, TestSize.Level1)
{
    string displayName = "test..file.jpg";
    EXPECT_EQ(MediaFileUtils::CheckDisplayName(displayName, false, true), E_OK);
}

// =================================================================
// CheckCreateAssetTitle / CheckSetAssetTitle 间接验证
// 这两个函数内部调用 CheckTitleForPhoto，通过 CheckDisplayName
// 间接验证点号规则生效
// =================================================================

// 验证 CheckDisplayName 统一走 CheckTitleForPhoto: 含反引号字符应被拒绝
HWTEST_F(MediaLibraryHelperUnitTest, CheckDisplayName_ViaCheckTitleForPhoto_Backtick_001, TestSize.Level1)
{
    string displayName = "test`file.jpg";
    // compatibleCheckTitle=true 也走 CheckTitleForPhoto，反引号属于非法字符集
    EXPECT_EQ(MediaFileUtils::CheckDisplayName(displayName, true, false), -EINVAL);
}

// 验证 CheckDisplayName 统一走 CheckTitleForPhoto: 含{}字符应被拒绝
HWTEST_F(MediaLibraryHelperUnitTest, CheckDisplayName_ViaCheckTitleForPhoto_Braces_001, TestSize.Level1)
{
    string displayName = "test{file}.jpg";
    // compatibleCheckTitle=true 也走 CheckTitleForPhoto，{}属于非法字符集
    EXPECT_EQ(MediaFileUtils::CheckDisplayName(displayName, true, false), -EINVAL);
}

// 验证 CheckDisplayName 统一走 CheckTitleForPhoto: 含[]字符应被拒绝
HWTEST_F(MediaLibraryHelperUnitTest, CheckDisplayName_ViaCheckTitleForPhoto_Brackets_001, TestSize.Level1)
{
    string displayName = "test[file].jpg";
    // compatibleCheckTitle=true 也走 CheckTitleForPhoto，[]属于非法字符集
    EXPECT_EQ(MediaFileUtils::CheckDisplayName(displayName, true, false), -EINVAL);
}

// 验证 compatibleCheckTitle=false 走 CheckTitle，单引号允许
HWTEST_F(MediaLibraryHelperUnitTest, CheckDisplayName_ViaCheckTitleForPhoto_Quote_False_001, TestSize.Level1)
{
    string displayName = "test'file.jpg";
    EXPECT_EQ(MediaFileUtils::CheckDisplayName(displayName, false, false), E_OK);
}

} // namespace Media
} // namespace OHOS