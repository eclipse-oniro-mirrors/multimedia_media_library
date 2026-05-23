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

#include "cloud_media_path_utils_test.h"

#include "cloud_media_path_utils.h"
#include "medialibrary_db_const.h"

using namespace testing::ext;

namespace OHOS::Media::CloudSync {
const int32_t FILE_SOURCE_TYPE_MEDIA = 0;
const int32_t FILE_SOURCE_TYPE_DOCS = 1;
const int32_t FILE_SOURCE_TYPE_LAKE = 3;
void CloudMediaPathUtilsTest::SetUpTestCase(void)
{}

void CloudMediaPathUtilsTest::TearDownTestCase(void)
{}

void CloudMediaPathUtilsTest::SetUp()
{}

void CloudMediaPathUtilsTest::TearDown()
{}

/**
 * @brief 测试目的：验证湖内资产路径转换正确性
 * @brief 测试场景：输入湖内存储路径，验证返回正确的HO_MEDIA路径格式
 */
HWTEST_F(CloudMediaPathUtilsTest, FindStoragePath_LakePath, TestSize.Level1)
{
    std::string storagePath = "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/test/file.jpg";
    int32_t userId = 100;
    std::string result = CloudMediaPathUtils::FindStoragePath(storagePath, userId);
    std::string expected = "/mnt/data/100/HO_MEDIA/test/file.jpg";
    EXPECT_EQ(result, expected);
}

/**
 * @brief 测试目的：验证文管资产路径转换正确性
 * @brief 测试场景：输入文管存储路径，验证返回正确的Docs路径格式
 */
HWTEST_F(CloudMediaPathUtilsTest, FindStoragePath_DocsPath, TestSize.Level1)
{
    std::string storagePath = "/storage/media/local/files/Docs/test/file.jpg";
    int32_t userId = 100;
    std::string result = CloudMediaPathUtils::FindStoragePath(storagePath, userId);
    std::string expected = "/storage/media/100/local/files/Docs/test/file.jpg";
    EXPECT_EQ(result, expected);
}

/**
 * @brief 测试目的：验证云端媒体资产路径转换正确性
 * @brief 测试场景：输入云端Photo路径，验证返回正确的hmdfs路径格式
 */
HWTEST_F(CloudMediaPathUtilsTest, FindStoragePath_CloudPath, TestSize.Level1)
{
    std::string storagePath = "/storage/cloud/files/Photo/4/IMG_1739459138_004.jpg";
    int32_t userId = 100;
    std::string result = CloudMediaPathUtils::FindStoragePath(storagePath, userId);
    std::string expected = "/data/service/el2/100/hmdfs/account/files/Photo/4/IMG_1739459138_004.jpg";
    EXPECT_EQ(result, expected);
}

/**
 * @brief 测试目的：验证未知路径的处理逻辑
 * @brief 测试场景：输入不匹配任何已知前缀的路径，验证返回空字符串
 */
HWTEST_F(CloudMediaPathUtilsTest, FindStoragePath_UnknownPath, TestSize.Level1)
{
    std::string storagePath = "/unknown/path/test/file.jpg";
    int32_t userId = 100;
    std::string result = CloudMediaPathUtils::FindStoragePath(storagePath, userId);
    EXPECT_TRUE(result.empty());
}

/**
 * @brief 测试目的：验证空路径的处理逻辑
 * @brief 测试场景：输入空字符串路径，验证返回空字符串
 */
HWTEST_F(CloudMediaPathUtilsTest, FindStoragePath_EmptyPath, TestSize.Level1)
{
    std::string storagePath = "";
    int32_t userId = 100;
    std::string result = CloudMediaPathUtils::FindStoragePath(storagePath, userId);
    EXPECT_EQ(result, storagePath);
}

/**
 * @brief 测试目的：验证userId为0时的路径转换
 * @brief 测试场景：湖内路径配合userId=0，验证路径中userId正确替换
 */
HWTEST_F(CloudMediaPathUtilsTest, FindStoragePath_LakePathZeroUserId, TestSize.Level1)
{
    std::string storagePath = "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/test/file.jpg";
    int32_t userId = 0;
    std::string result = CloudMediaPathUtils::FindStoragePath(storagePath, userId);
    std::string expected = "/mnt/data/0/HO_MEDIA/test/file.jpg";
    EXPECT_EQ(result, expected);
}

/**
 * @brief 测试目的：验证不同userId的路径转换
 * @brief 测试场景：文管路径配合userId=200，验证路径中userId正确替换
 */
HWTEST_F(CloudMediaPathUtilsTest, FindStoragePath_DocsPathDifferentUserId, TestSize.Level1)
{
    std::string storagePath = "/storage/media/local/files/Docs/test/file.jpg";
    int32_t userId = 200;
    std::string result = CloudMediaPathUtils::FindStoragePath(storagePath, userId);
    std::string expected = "/storage/media/200/local/files/Docs/test/file.jpg";
    EXPECT_EQ(result, expected);
}

/**
 * @brief 测试目的：验证负数userId的路径转换
 * @brief 测试场景：云端路径配合userId=-1，验证路径中userId正确替换
 */
HWTEST_F(CloudMediaPathUtilsTest, FindStoragePath_CloudPathNegativeUserId, TestSize.Level1)
{
    std::string storagePath = "/storage/cloud/files/Photo/test/file.jpg";
    int32_t userId = -1;
    std::string result = CloudMediaPathUtils::FindStoragePath(storagePath, userId);
    std::string expected = "/data/service/el2/-1/hmdfs/account/files/Photo/test/file.jpg";
    EXPECT_EQ(result, expected);
}

/**
 * @brief 测试目的：验证湖内复杂路径转换
 * @brief 测试场景：湖内路径包含多层子目录，验证相对路径正确提取
 */
HWTEST_F(CloudMediaPathUtilsTest, FindStoragePath_LakePathComplex, TestSize.Level1)
{
    std::string storagePath = "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/Photo/2025/03/06/test.jpg";
    int32_t userId = 100;
    std::string result = CloudMediaPathUtils::FindStoragePath(storagePath, userId);
    std::string expected = "/mnt/data/100/HO_MEDIA/Photo/2025/03/06/test.jpg";
    EXPECT_EQ(result, expected);
}

/**
 * @brief 测试目的：验证文管复杂路径转换
 * @brief 测试场景：文管路径包含多层子目录，验证相对路径正确提取
 */
HWTEST_F(CloudMediaPathUtilsTest, FindStoragePath_DocsPathComplex, TestSize.Level1)
{
    std::string storagePath = "/storage/media/local/files/Docs/Document/2025/test.pdf";
    int32_t userId = 100;
    std::string result = CloudMediaPathUtils::FindStoragePath(storagePath, userId);
    std::string expected = "/storage/media/100/local/files/Docs/Document/2025/test.pdf";
    EXPECT_EQ(result, expected);
}

/**
 * @brief 测试目的：验证LAKE类型资产的路径选择逻辑
 * @brief 测试场景：fileSourceType=LAKE(3)，验证返回storagePath转换结果
 */
HWTEST_F(CloudMediaPathUtilsTest, FindStoragePath_WithFileTypeLake, TestSize.Level1)
{
    int32_t fileSourceType = FILE_SOURCE_TYPE_LAKE;
    std::string cloudPath = "/storage/cloud/files/test.jpg";
    std::string storagePath = "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/test.jpg";
    int32_t userId = 100;
    std::string result = CloudMediaPathUtils::FindStoragePath(fileSourceType, cloudPath, storagePath, userId);
    std::string expected = "/mnt/data/100/HO_MEDIA/test.jpg";
    EXPECT_EQ(result, expected);
}

/**
 * @brief 测试目的：验证文管类型资产的路径选择逻辑
 * @brief 测试场景：fileSourceType=FILE_MANAGER(1)，验证返回storagePath转换结果
 */
HWTEST_F(CloudMediaPathUtilsTest, FindStoragePath_WithFileTypeFileManager, TestSize.Level1)
{
    int32_t fileSourceType = FILE_SOURCE_TYPE_DOCS;
    std::string cloudPath = "/storage/cloud/files/test.jpg";
    std::string storagePath = "/storage/media/local/files/Docs/test.jpg";
    int32_t userId = 100;
    std::string result = CloudMediaPathUtils::FindStoragePath(fileSourceType, cloudPath, storagePath, userId);
    std::string expected = "/storage/media/100/local/files/Docs/test.jpg";
    EXPECT_EQ(result, expected);
}

/**
 * @brief 测试目的：验证MEDIA类型资产的路径选择逻辑
 * @brief 测试场景：fileSourceType=MEDIA(0)，验证返回cloudPath转换结果
 */
HWTEST_F(CloudMediaPathUtilsTest, FindStoragePath_WithFileTypeMedia, TestSize.Level1)
{
    int32_t fileSourceType = FILE_SOURCE_TYPE_MEDIA;
    std::string cloudPath = "/storage/cloud/files/Photo/4/IMG_1739459138_004.jpg";
    std::string storagePath = "/storage/media/local/files/Docs/test.jpg";
    int32_t userId = 100;
    std::string result = CloudMediaPathUtils::FindStoragePath(fileSourceType, cloudPath, storagePath, userId);
    std::string expected = "/data/service/el2/100/hmdfs/account/files/Photo/4/IMG_1739459138_004.jpg";
    EXPECT_EQ(result, expected);
}

/**
 * @brief 测试目的：验证其他类型资产的路径选择逻辑
 * @brief 测试场景：fileSourceType=-1(其他)，验证返回cloudPath转换结果
 */
HWTEST_F(CloudMediaPathUtilsTest, FindStoragePath_WithFileTypeOther, TestSize.Level1)
{
    int32_t fileSourceType = -1;
    std::string cloudPath = "/storage/cloud/files/Photo/4/IMG_1739459138_004.jpg";
    std::string storagePath = "/storage/media/local/files/Docs/test.jpg";
    int32_t userId = 200;
    std::string result = CloudMediaPathUtils::FindStoragePath(fileSourceType, cloudPath, storagePath, userId);
    std::string expected = "/data/service/el2/200/hmdfs/account/files/Photo/4/IMG_1739459138_004.jpg";
    EXPECT_EQ(result, expected);
}

/**
 * @brief 测试目的：验证带尾部斜杠的湖内路径处理
 * @brief 测试场景：输入以斜杠结尾的目录路径，验证转换结果保留尾部斜杠
 */
HWTEST_F(CloudMediaPathUtilsTest, FindStoragePath_LakePathWithTrailingSlash, TestSize.Level1)
{
    std::string storagePath = "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/test/path/";
    int32_t userId = 100;
    std::string result = CloudMediaPathUtils::FindStoragePath(storagePath, userId);
    std::string expected = "/mnt/data/100/HO_MEDIA/test/path/";
    EXPECT_EQ(result, expected);
}

/**
 * @brief 测试目的：验证路径包含空格的处理
 * @brief 测试场景：云端路径包含空格字符，验证空格正确保留
 */
HWTEST_F(CloudMediaPathUtilsTest, FindStoragePath_CloudPathWithSpaces, TestSize.Level1)
{
    std::string storagePath = "/storage/cloud/files/Photo/test path/file name.jpg";
    int32_t userId = 100;
    std::string result = CloudMediaPathUtils::FindStoragePath(storagePath, userId);
    std::string expected = "/data/service/el2/100/hmdfs/account/files/Photo/test path/file name.jpg";
    EXPECT_EQ(result, expected);
}

/**
 * @brief 测试目的：验证路径包含特殊字符的处理
 * @brief 测试场景：文管路径包含点号等特殊字符，验证特殊字符正确保留
 */
HWTEST_F(CloudMediaPathUtilsTest, FindStoragePath_DocsPathWithSpecialChars, TestSize.Level1)
{
    std::string storagePath = "/storage/media/local/files/Docs/test.path/file.name.pdf";
    int32_t userId = 100;
    std::string result = CloudMediaPathUtils::FindStoragePath(storagePath, userId);
    std::string expected = "/storage/media/100/local/files/Docs/test.path/file.name.pdf";
    EXPECT_EQ(result, expected);
}
}  // namespace OHOS::Media::CloudSync
