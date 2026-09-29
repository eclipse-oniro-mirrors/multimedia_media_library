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

#ifndef OHOS_MEDIA_SHARE_ASSETS_TEST_UTILS_H
#define OHOS_MEDIA_SHARE_ASSETS_TEST_UTILS_H

#include <memory>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "medialibrary_rdbstore.h"
#include "photos_po.h"

namespace OHOS::Media {

// 共享相册资产删除用例的公共测试环境工具
class MediaShareAssetsTestUtils {
public:
    MediaShareAssetsTestUtils() = delete;
    ~MediaShareAssetsTestUtils() = delete;

    static const std::string DELETED_DISPLAY_NAME;

    static std::shared_ptr<MediaLibraryRdbStore> &GetRdbStore();

    // 初始化 unistore 并创建 Photos / PhotoAlbum / share member 表
    static void InitEnvironment();
    // 释放 unistore
    static void ReleaseEnvironment();
    // 清空所有测试表数据
    static void CleanTables();
    // 创建 preferences 目录, 保证共享资产清理状态可以持久化
    static void PrepareShareRetainPreferences();

    // 插入一条资产记录, isShared 为 1 表示共享资产
    static int32_t InsertPhoto(int32_t fileId, const std::string &displayName, int32_t ownerAlbumId,
        int32_t isShared);
    // 拼接测试照片的物理路径, 与 InsertPhoto 写入 file_path 的值保持一致
    static std::string BuildPhotoPath(const std::string &displayName);
    // 判断测试照片的物理文件是否存在
    static bool IsPhotoFileExists(const std::string &displayName);
    // 插入一条共享相册记录
    static int32_t InsertShareAlbum(int32_t albumId, const std::string &albumName);
    // 插入一条非共享相册记录
    static int32_t InsertUserAlbum(int32_t albumId, const std::string &albumName);
    // 插入一条共享相册成员记录
    static int32_t InsertShareMember(int32_t albumId, const std::string &member);

    // 统计表内符合条件的记录数
    static int32_t CountRows(const std::string &table);
    // 统计已被标记为待删除(display_name 被改写)的共享资产数量
    static int32_t CountDeletedMarkedAssets();
    // 查询指定资产的某个字符串列
    static std::string QueryPhotoString(const std::string &fileId, const std::string &column);
    // 查询指定资产的某个整型列
    static int32_t QueryPhotoInt(const std::string &fileId, const std::string &column);
    // 查询指定资产的某个列是否为空
    static bool IsPhotoColumnNull(const std::string &fileId, const std::string &column);
};

// 统一用例 fixture, 所有分支用例共用该 SetUp/TearDown
class MediaShareAssetsServiceTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

} // namespace OHOS::Media

#endif // OHOS_MEDIA_SHARE_ASSETS_TEST_UTILS_H