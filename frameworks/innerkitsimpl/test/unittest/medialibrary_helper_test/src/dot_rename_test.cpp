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
#include "file_const.h"

#include <string>
#include <filesystem>

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {

// ==========================================
// DOT_RENAME 释放/恢复机制单元测试
// ==========================================
// 被测逻辑说明:
//    IsDotRenameRelease: beforePath不以.开头 + afterPath以.开头 -> 释放本地
//    IsDotRenameRecover: beforePath以.开头 + afterPath不以.开头 -> 恢复重新入库
//
// IsDotRenameReleaseLogic / IsDotRenameRecoverLogic 为共享内联函数
// （定义于 file_const.h），FileManagerParser 与 LakeFileParser 的
// IsDotRenameRelease / IsDotRenameRecover 均委托该实现。
// 本文件直接测试共享函数，消除逻辑副本与生产代码的漂移风险。
//
// 生产方法为 private 且依赖 FileParser 基类（含 RDB 查询、MediaNotifyInfo 等），
// 无法在纯单元测试中直接实例化，因此经由共享函数覆盖以下场景:
//    1. beforePath/afterPath 文件名首字符判断
//    2. 空路径处理
//    3. 释放/恢复互斥性
// ==========================================

// ==========================================
// IsDotRenameRelease 逻辑测试
// ==========================================

// 正向：beforePath普通文件名 + afterPath以.开头 -> 释放本地
HWTEST_F(MediaLibraryHelperUnitTest, DotRenameRelease_Normal_001, TestSize.Level1)
{
    string beforePath = "/storage/media/local/files/Docs/photo.jpg";
    string afterPath = "/storage/media/local/files/Docs/.photo.jpg";
    EXPECT_TRUE(IsDotRenameReleaseLogic(beforePath, afterPath));
}

// 正向：不同目录下文件名以.开头
HWTEST_F(MediaLibraryHelperUnitTest, DotRenameRelease_DifferentDir_001, TestSize.Level1)
{
    string beforePath = "/storage/media/local/files/Docs/dir1/IMG_001.jpg";
    string afterPath = "/storage/media/local/files/Docs/dir2/.IMG_001.jpg";
    EXPECT_TRUE(IsDotRenameReleaseLogic(beforePath, afterPath));
}

// 负向：beforePath为空
HWTEST_F(MediaLibraryHelperUnitTest, DotRenameRelease_EmptyBefore_001, TestSize.Level1)
{
    string beforePath = "";
    string afterPath = "/storage/media/local/files/Docs/.photo.jpg";
    EXPECT_FALSE(IsDotRenameReleaseLogic(beforePath, afterPath));
}

// 负向：afterPath为空
HWTEST_F(MediaLibraryHelperUnitTest, DotRenameRelease_EmptyAfter_001, TestSize.Level1)
{
    string beforePath = "/storage/media/local/files/Docs/photo.jpg";
    string afterPath = "";
    EXPECT_FALSE(IsDotRenameReleaseLogic(beforePath, afterPath));
}

// 负向：两者均为空
HWTEST_F(MediaLibraryHelperUnitTest, DotRenameRelease_BothEmpty_001, TestSize.Level1)
{
    string beforePath = "";
    string afterPath = "";
    EXPECT_FALSE(IsDotRenameReleaseLogic(beforePath, afterPath));
}

// 负向：beforePath也以.开头（不是释放场景）
HWTEST_F(MediaLibraryHelperUnitTest, DotRenameRelease_BothStartWithDot_001, TestSize.Level1)
{
    string beforePath = "/storage/media/local/files/Docs/.photo.jpg";
    string afterPath = "/storage/media/local/files/Docs/.photo2.jpg";
    EXPECT_FALSE(IsDotRenameReleaseLogic(beforePath, afterPath));
}

// 负向：afterPath不以.开头（不是释放场景）
HWTEST_F(MediaLibraryHelperUnitTest, DotRenameRelease_AfterNotDot_001, TestSize.Level1)
{
    string beforePath = "/storage/media/local/files/Docs/photo.jpg";
    string afterPath = "/storage/media/local/files/Docs/photo2.jpg";
    EXPECT_FALSE(IsDotRenameReleaseLogic(beforePath, afterPath));
}

// 负向：beforePath以.开头 + afterPath不以.开头 (这是恢复场景，不是释放)
HWTEST_F(MediaLibraryHelperUnitTest, DotRenameRelease_ReverseCase_001, TestSize.Level1)
{
    string beforePath = "/storage/media/local/files/Docs/.photo.jpg";
    string afterPath = "/storage/media/local/files/Docs/photo.jpg";
    EXPECT_FALSE(IsDotRenameReleaseLogic(beforePath, afterPath));
}

// --------------------------------------------------
// IsDotRenameRecover 逻辑测试
// --------------------------------------------------

// 正向：beforePath以.开头 + afterPath不以.开头 -> 恢复重新入库
HWTEST_F(MediaLibraryHelperUnitTest, DotRenameRecover_Normal_001, TestSize.Level1)
{
    string beforePath = "/storage/media/local/files/Docs/.photo.jpg";
    string afterPath = "/storage/media/local/files/Docs/photo.jpg";
    EXPECT_TRUE(IsDotRenameRecoverLogic(beforePath, afterPath));
}

// 正向：不同目录下恢复
HWTEST_F(MediaLibraryHelperUnitTest, DotRenameRecover_DifferentDir_001, TestSize.Level1)
{
    string beforePath = "/storage/media/local/files/Docs/dir1/.IMG_001.jpg";
    string afterPath = "/storage/media/local/files/Docs/dir2/IMG_001.jpg";
    EXPECT_TRUE(IsDotRenameRecoverLogic(beforePath, afterPath));
}

// 负向：beforePath为空
HWTEST_F(MediaLibraryHelperUnitTest, DotRenameRecover_EmptyBefore_001, TestSize.Level1)
{
    string beforePath = "";
    string afterPath = "/storage/media/local/files/Docs/photo.jpg";
    EXPECT_FALSE(IsDotRenameRecoverLogic(beforePath, afterPath));
}

// 负向：afterPath为空
HWTEST_F(MediaLibraryHelperUnitTest, DotRenameRecover_EmptyAfter_001, TestSize.Level1)
{
    string beforePath = "/storage/media/local/files/Docs/.photo.jpg";
    string afterPath = "";
    EXPECT_FALSE(IsDotRenameRecoverLogic(beforePath, afterPath));
}

// 负向：两者均为空
HWTEST_F(MediaLibraryHelperUnitTest, DotRenameRecover_BothEmpty_001, TestSize.Level1)
{
    string beforePath = "";
    string afterPath = "";
    EXPECT_FALSE(IsDotRenameRecoverLogic(beforePath, afterPath));
}

// 负向：afterPath也以.开头（不是恢复场景）
HWTEST_F(MediaLibraryHelperUnitTest, DotRenameRecover_BothStartWithDot_001, TestSize.Level1)
{
    string beforePath = "/storage/media/local/files/Docs/.photo.jpg";
    string afterPath = "/storage/media/local/files/Docs/.photo2.jpg";
    EXPECT_FALSE(IsDotRenameRecoverLogic(beforePath, afterPath));
}

// 负向：beforePath不以.开头（不是恢复场景）
HWTEST_F(MediaLibraryHelperUnitTest, DotRenameRecover_BeforeNotDot_001, TestSize.Level1)
{
    string beforePath = "/storage/media/local/files/Docs/photo.jpg";
    string afterPath = "/storage/media/local/files/Docs/photo2.jpg";
    EXPECT_FALSE(IsDotRenameRecoverLogic(beforePath, afterPath));
}

// 负向：beforePath不以.开头 + afterPath以.开头（这是释放场景，不是恢复）
HWTEST_F(MediaLibraryHelperUnitTest, DotRenameRecover_ReverseCase_001, TestSize.Level1)
{
    string beforePath = "/storage/media/local/files/Docs/photo.jpg";
    string afterPath = "/storage/media/local/files/Docs/.photo.jpg";
    EXPECT_FALSE(IsDotRenameRecoverLogic(beforePath, afterPath));
}

// ==========================================
// 互斥性验证：Release 和 Recover 不会同时为 true
// ==========================================

// 互斥：正常释放场景，Recover 应为 false
HWTEST_F(MediaLibraryHelperUnitTest, DotRename_Mutex_Release_001, TestSize.Level1)
{
    string beforePath = "/storage/media/local/files/Docs/photo.jpg";
    string afterPath = "/storage/media/local/files/Docs/.photo.jpg";
    EXPECT_TRUE(IsDotRenameReleaseLogic(beforePath, afterPath));
    EXPECT_FALSE(IsDotRenameRecoverLogic(beforePath, afterPath));
}

// 互斥：正常恢复场景，Release 应为 false
HWTEST_F(MediaLibraryHelperUnitTest, DotRename_Mutex_Recover_001, TestSize.Level1)
{
    string beforePath = "/storage/media/local/files/Docs/.photo.jpg";
    string afterPath = "/storage/media/local/files/Docs/photo.jpg";
    EXPECT_FALSE(IsDotRenameReleaseLogic(beforePath, afterPath));
    EXPECT_TRUE(IsDotRenameRecoverLogic(beforePath, afterPath));
}

// 互斥：两者均不以.开头，Release 和 Recover 均为 false
HWTEST_F(MediaLibraryHelperUnitTest, DotRename_Mutex_Neither_001, TestSize.Level1)
{
    string beforePath = "/storage/media/local/files/Docs/photo1.jpg";
    string afterPath = "/storage/media/local/files/Docs/photo2.jpg";
    EXPECT_FALSE(IsDotRenameReleaseLogic(beforePath, afterPath));
    EXPECT_FALSE(IsDotRenameRecoverLogic(beforePath, afterPath));
}

// 互斥：两者均以.开头，Release 和 Recover 均为 false
HWTEST_F(MediaLibraryHelperUnitTest, DotRename_Mutex_BothDot_001, TestSize.Level1)
{
    string beforePath = "/storage/media/local/files/Docs/.photo1.jpg";
    string afterPath = "/storage/media/local/files/Docs/.photo2.jpg";
    EXPECT_FALSE(IsDotRenameReleaseLogic(beforePath, afterPath));
    EXPECT_FALSE(IsDotRenameRecoverLogic(beforePath, afterPath));
}

// ==========================================
// FileUpdateType 枚举互异性不再用运行期断言验证：
// 编译期由 file_const.h 内 static_assert 防护（成员互异为编译期属性，
// 运行期 EXPECT_NE 断言恒真，无回归价值）。
// ==========================================

// ==========================================
// CheckTitleForPhoto 与 DOT_RENAME 的关联验证
// DOT_RENAME 释放场景中，afterPath 以.开头的文件名提取的 title 应被拒绝
// ==========================================

// DOT_RENAME 释放场景：afterPath 文件名以.开头，提取 title 后 CheckTitleForPhoto 应拒绝
HWTEST_F(MediaLibraryHelperUnitTest, DotRename_Release_CheckTitleReject_001, TestSize.Level1)
{
    string afterPath = "/storage/media/local/files/Docs/.photo.jpg";
    string fileName = filesystem::path(afterPath).filename().string();
    // 模拟 GetTitleFromDisplayName 提取 title
    string::size_type pos = fileName.find_last_of('.');
    string title = (pos != string::npos) ? fileName.substr(0, pos) : "";
    // title 为 ".photo"，以.开头，CheckTitleForPhoto 应返回 -EINVAL
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), -EINVAL);
}

// DOT_RENAME 恢复场景：afterPath 文件名不以.开头，提取 title 后 CheckTitleForPhoto 应通过
HWTEST_F(MediaLibraryHelperUnitTest, DotRename_Recover_CheckTitlePass_001, TestSize.Level1)
{
    string afterPath = "/storage/media/local/files/Docs/photo.jpg";
    string fileName = filesystem::path(afterPath).filename().string();
    // 模拟 GetTitleFromDisplayName 提取 title
    string::size_type pos = fileName.find_last_of('.');
    string title = (pos != string::npos) ? fileName.substr(0, pos) : "";
    // title 为 "photo"，不以.开头，CheckTitleForPhoto 应返回 E_OK
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), E_OK);
}

// DOT_RENAME 恢复场景：afterPath 文件名包含点号但不以.开头
HWTEST_F(MediaLibraryHelperUnitTest, DotRename_Recover_CheckTitlePass_WithDot_001, TestSize.Level1)
{
    string afterPath = "/storage/media/local/files/Docs/photo.test.jpg";
    string fileName = filesystem::path(afterPath).filename().string();
    string::size_type pos = fileName.find_last_of('.');
    string title = (pos != string::npos) ? fileName.substr(0, pos) : "";
    // title 为 "photo.test"，不以.开头且含点号，CheckTitleForPhoto 应返回 E_OK
    EXPECT_EQ(MediaFileUtils::CheckTitleForPhoto(title), E_OK);
}

// ==========================================
// PhotoPositionType 枚举验证不再用运行期断言：
// 成员为显式赋值（INVALID=-1, LOCAL=1, CLOUD=2, LOCAL_AND_CLOUD=3），
// 互异性由 userfile_manager_types.h 内 static_assert 编译期防护；
// DeleteFileByLakePath 的 position 分支处理逻辑由相关生产用例
// （media_file_monitor 测试）覆盖，此处运行期枚举自比较恒真，无回归价值。
// ==========================================

} // namespace Media
} // namespace OHOS