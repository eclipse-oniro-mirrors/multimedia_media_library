/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "multistages_local_enhancement_manager_test.h"

#include "camera_test_utils.h"
#include "local_enhancement_manager.h"
#include "new_image_pipeline.h"
#include "yuv_pipeline.h"
#include "userfilemgr_uri.h"

namespace OHOS {
namespace Media {
using namespace testing::ext;

static const int32_t FILE_ID_INPUT = 1;
static const std::string PHOTO_ID_INPUT = "1970_000000_0000_001";

void MultistagesLocalEnhancementManagerTest::SetUpTestCase(void) {}

void MultistagesLocalEnhancementManagerTest::TearDownTestCase(void) {}

void MultistagesLocalEnhancementManagerTest::SetUp()
{
    LocalEnhancementManager::GetInstance().fileId2PhotoId_.clear();
    LocalEnhancementManager::GetInstance().localEnhancementMap_.clear();
}

void MultistagesLocalEnhancementManagerTest::TearDown(void)
{
    LocalEnhancementManager::GetInstance().fileId2PhotoId_.clear();
    LocalEnhancementManager::GetInstance().localEnhancementMap_.clear();
}

/**
 * @tc.name: InsertLocalEnhancementData_test01
 * @tc.desc: NEW_IMAGE 类型, 正常创建 pipeline
 */
HWTEST_F(MultistagesLocalEnhancementManagerTest, InsertLocalEnhancementData_test01, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter InsertLocalEnhancementData_test01");

    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    FileAsset fileAsset;
    CameraTestUtils::CreateFileAsset(assetInfo, fileAsset);
    CameraAssetInfo cameraAssetInfo(fileAsset);

    size_t count = LocalEnhancementManager::GetInstance().InsertLocalEnhancementData(
        CameraPipelineType::NEW_IMAGE, cameraAssetInfo);
    ASSERT_EQ(count, 1);

    // 可以通过 fileId 获取
    auto pipeline = LocalEnhancementManager::GetInstance().GetLocalEnhancementPipeline(FILE_ID_INPUT);
    ASSERT_NE(pipeline, nullptr);
    ASSERT_EQ(pipeline->GetPipelineType(), CameraPipelineType::NEW_IMAGE);
    ASSERT_EQ(pipeline->GetAssetInfo().GetPhotoId(), PHOTO_ID_INPUT);
}

/**
 * @tc.name: InsertLocalEnhancementData_test02
 * @tc.desc: YUV 类型, 正常创建 pipeline
 */
HWTEST_F(MultistagesLocalEnhancementManagerTest, InsertLocalEnhancementData_test02, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter InsertLocalEnhancementData_test02");

    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    FileAsset fileAsset;
    CameraTestUtils::CreateFileAsset(assetInfo, fileAsset);
    CameraAssetInfo cameraAssetInfo(fileAsset);

    size_t count = LocalEnhancementManager::GetInstance().InsertLocalEnhancementData(
        CameraPipelineType::YUV, cameraAssetInfo);
    ASSERT_EQ(count, 1);

    auto pipeline = LocalEnhancementManager::GetInstance().GetLocalEnhancementPipeline(FILE_ID_INPUT);
    ASSERT_NE(pipeline, nullptr);
    ASSERT_EQ(pipeline->GetPipelineType(), CameraPipelineType::YUV);
    ASSERT_EQ(pipeline->GetAssetInfo().GetPhotoId(), PHOTO_ID_INPUT);
}

/**
 * @tc.name: InsertLocalEnhancementData_test03
 * @tc.desc: UNDEFINED 类型, 不支持创建 pipeline
 */
HWTEST_F(MultistagesLocalEnhancementManagerTest, InsertLocalEnhancementData_test03, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter InsertLocalEnhancementData_test03");

    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    FileAsset fileAsset;
    CameraTestUtils::CreateFileAsset(assetInfo, fileAsset);
    CameraAssetInfo cameraAssetInfo(fileAsset);

    size_t count = LocalEnhancementManager::GetInstance().InsertLocalEnhancementData(
        CameraPipelineType::UNDEFINED, cameraAssetInfo);
    ASSERT_EQ(count, 0);
}

/**
 * @tc.name: InsertLocalEnhancementData_test04
 * @tc.desc: IMAGE 类型, 不支持创建 pipeline
 */
HWTEST_F(MultistagesLocalEnhancementManagerTest, InsertLocalEnhancementData_test04, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter InsertLocalEnhancementData_test04");

    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    FileAsset fileAsset;
    CameraTestUtils::CreateFileAsset(assetInfo, fileAsset);
    CameraAssetInfo cameraAssetInfo(fileAsset);

    size_t count = LocalEnhancementManager::GetInstance().InsertLocalEnhancementData(
        CameraPipelineType::IMAGE, cameraAssetInfo);
    ASSERT_EQ(count, 0);
}

/**
 * @tc.name: InsertLocalEnhancementData_test05
 * @tc.desc: fileId <= 0, 无效输入, 不允许插入
 */
HWTEST_F(MultistagesLocalEnhancementManagerTest, InsertLocalEnhancementData_test05, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter InsertLocalEnhancementData_test05");

    FileAssetInfo assetInfo = {
        .fileId = 0,
        .photoId = PHOTO_ID_INPUT,
    };
    FileAsset fileAsset;
    CameraTestUtils::CreateFileAsset(assetInfo, fileAsset);
    CameraAssetInfo cameraAssetInfo(fileAsset);

    size_t count = LocalEnhancementManager::GetInstance().InsertLocalEnhancementData(
        CameraPipelineType::NEW_IMAGE, cameraAssetInfo);
    ASSERT_EQ(count, 0);
}

/**
 * @tc.name: InsertLocalEnhancementData_test06
 * @tc.desc: photoId 为空, 无效输入, 不允许插入
 */
HWTEST_F(MultistagesLocalEnhancementManagerTest, InsertLocalEnhancementData_test06, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter InsertLocalEnhancementData_test06");

    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = "",
    };
    FileAsset fileAsset;
    CameraTestUtils::CreateFileAsset(assetInfo, fileAsset);
    CameraAssetInfo cameraAssetInfo(fileAsset);

    size_t count = LocalEnhancementManager::GetInstance().InsertLocalEnhancementData(
        CameraPipelineType::NEW_IMAGE, cameraAssetInfo);
    ASSERT_EQ(count, 0);
}

/**
 * @tc.name: InsertLocalEnhancementData_test07
 * @tc.desc: 重复插入相同 fileId, 不允许
 */
HWTEST_F(MultistagesLocalEnhancementManagerTest, InsertLocalEnhancementData_test07, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter InsertLocalEnhancementData_test07");

    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    FileAsset fileAsset;
    CameraTestUtils::CreateFileAsset(assetInfo, fileAsset);
    CameraAssetInfo cameraAssetInfo(fileAsset);

    size_t count = LocalEnhancementManager::GetInstance().InsertLocalEnhancementData(
        CameraPipelineType::NEW_IMAGE, cameraAssetInfo);
    ASSERT_EQ(count, 1);

    // fileId 重复
    FileAssetInfo assetInfo2 = {
        .fileId = FILE_ID_INPUT,
        .photoId = "1970_000000_0000_002",
    };
    FileAsset fileAsset2;
    CameraTestUtils::CreateFileAsset(assetInfo2, fileAsset2);
    CameraAssetInfo cameraAssetInfo2(fileAsset2);

    count = LocalEnhancementManager::GetInstance().InsertLocalEnhancementData(
        CameraPipelineType::NEW_IMAGE, cameraAssetInfo2);
    ASSERT_EQ(count, 1);
}

/**
 * @tc.name: InsertLocalEnhancementData_test08
 * @tc.desc: 重复插入相同 photoId, 不允许
 */
HWTEST_F(MultistagesLocalEnhancementManagerTest, InsertLocalEnhancementData_test08, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter InsertLocalEnhancementData_test08");

    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    FileAsset fileAsset;
    CameraTestUtils::CreateFileAsset(assetInfo, fileAsset);
    CameraAssetInfo cameraAssetInfo(fileAsset);

    size_t count = LocalEnhancementManager::GetInstance().InsertLocalEnhancementData(
        CameraPipelineType::NEW_IMAGE, cameraAssetInfo);
    ASSERT_EQ(count, 1);

    // photoId 重复
    FileAssetInfo assetInfo2 = {
        .fileId = 2,
        .photoId = PHOTO_ID_INPUT,
    };
    FileAsset fileAsset2;
    CameraTestUtils::CreateFileAsset(assetInfo2, fileAsset2);
    CameraAssetInfo cameraAssetInfo2(fileAsset2);

    count = LocalEnhancementManager::GetInstance().InsertLocalEnhancementData(
        CameraPipelineType::NEW_IMAGE, cameraAssetInfo2);
    ASSERT_EQ(count, 1);
}

/**
 * @tc.name: GetLocalEnhancementPipelineByFileId_test01
 * @tc.desc: 正常通过 fileId 获取 pipeline
 */
HWTEST_F(MultistagesLocalEnhancementManagerTest, GetLocalEnhancementPipelineByFileId_test01, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter GetLocalEnhancementPipelineByFileId_test01");

    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    FileAsset fileAsset;
    CameraTestUtils::CreateFileAsset(assetInfo, fileAsset);
    CameraAssetInfo cameraAssetInfo(fileAsset);

    size_t count = LocalEnhancementManager::GetInstance().InsertLocalEnhancementData(
        CameraPipelineType::NEW_IMAGE, cameraAssetInfo);
    ASSERT_EQ(count, 1);

    auto pipeline = LocalEnhancementManager::GetInstance().GetLocalEnhancementPipeline(FILE_ID_INPUT);
    ASSERT_NE(pipeline, nullptr);
    ASSERT_EQ(pipeline->GetPipelineType(), CameraPipelineType::NEW_IMAGE);
    ASSERT_EQ(pipeline->GetAssetInfo().GetPhotoId(), PHOTO_ID_INPUT);
}

/**
 * @tc.name: GetLocalEnhancementPipelineByFileId_test02
 * @tc.desc: 内存中没有 pipeline, 无法获取
 */
HWTEST_F(MultistagesLocalEnhancementManagerTest, GetLocalEnhancementPipelineByFileId_test02, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter GetLocalEnhancementPipelineByFileId_test02");

    ASSERT_EQ(LocalEnhancementManager::GetInstance().fileId2PhotoId_.size(), 0);
    ASSERT_EQ(LocalEnhancementManager::GetInstance().localEnhancementMap_.size(), 0);

    auto pipeline = LocalEnhancementManager::GetInstance().GetLocalEnhancementPipeline(FILE_ID_INPUT);
    ASSERT_EQ(pipeline, nullptr);
}

/**
 * @tc.name: IsPhotoInProcessEnhancementByPhotoId_test01
 * @tc.desc: pipeline 存在时, 返回 true
 */
HWTEST_F(MultistagesLocalEnhancementManagerTest, IsPhotoInProcessEnhancementByPhotoId_test01, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter IsPhotoInProcessEnhancementByPhotoId_test01");

    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    FileAsset fileAsset;
    CameraTestUtils::CreateFileAsset(assetInfo, fileAsset);
    CameraAssetInfo cameraAssetInfo(fileAsset);

    size_t count = LocalEnhancementManager::GetInstance().InsertLocalEnhancementData(
        CameraPipelineType::NEW_IMAGE, cameraAssetInfo);
    ASSERT_EQ(count, 1);

    bool ret = LocalEnhancementManager::GetInstance().IsPhotoInProcessEnhancementByPhotoId(PHOTO_ID_INPUT);
    ASSERT_EQ(ret, true);
}

/**
 * @tc.name: IsPhotoInProcessEnhancementByPhotoId_test02
 * @tc.desc: pipeline 不存在时, 返回 false
 */
HWTEST_F(MultistagesLocalEnhancementManagerTest, IsPhotoInProcessEnhancementByPhotoId_test02, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter IsPhotoInProcessEnhancementByPhotoId_test02");

    bool ret = LocalEnhancementManager::GetInstance().IsPhotoInProcessEnhancementByPhotoId(PHOTO_ID_INPUT);
    ASSERT_EQ(ret, false);
}

/**
 * @tc.name: GetLocalEnhancementPipelineWithExpected_test01
 * @tc.desc: 不支持 UNDEFINED 类型
 */
HWTEST_F(MultistagesLocalEnhancementManagerTest, GetLocalEnhancementPipelineWithExpected_test01, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter GetLocalEnhancementPipelineWithExpected_test01");

    auto pipeline = LocalEnhancementManager::GetInstance().GetLocalEnhancementPipelineWithExpected(
        "1", CameraPipelineType::UNDEFINED);
    ASSERT_EQ(pipeline, nullptr);
}

/**
 * @tc.name: GetLocalEnhancementPipelineWithExpected_test02
 * @tc.desc: 不支持 IMAGE 类型
 */
HWTEST_F(MultistagesLocalEnhancementManagerTest, GetLocalEnhancementPipelineWithExpected_test02, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter GetLocalEnhancementPipelineWithExpected_test02");

    auto pipeline = LocalEnhancementManager::GetInstance().GetLocalEnhancementPipelineWithExpected(
        "1", CameraPipelineType::IMAGE);
    ASSERT_EQ(pipeline, nullptr);
}

/**
 * @tc.name: GetLocalEnhancementPipelineWithExpected_test03
 * @tc.desc: 预期值和缓存一致, 正常返回
 */
HWTEST_F(MultistagesLocalEnhancementManagerTest, GetLocalEnhancementPipelineWithExpected_test03, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter GetLocalEnhancementPipelineWithExpected_test03");

    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    FileAsset fileAsset;
    CameraTestUtils::CreateFileAsset(assetInfo, fileAsset);
    CameraAssetInfo cameraAssetInfo(fileAsset);

    size_t count = LocalEnhancementManager::GetInstance().InsertLocalEnhancementData(
        CameraPipelineType::NEW_IMAGE, cameraAssetInfo);
    ASSERT_EQ(count, 1);

    auto pipeline = LocalEnhancementManager::GetInstance().GetLocalEnhancementPipelineWithExpected(
        PHOTO_ID_INPUT, CameraPipelineType::NEW_IMAGE);
    ASSERT_NE(pipeline, nullptr);
    ASSERT_EQ(pipeline->GetPipelineType(), CameraPipelineType::NEW_IMAGE);
    ASSERT_EQ(pipeline->GetAssetInfo().GetPhotoId(), PHOTO_ID_INPUT);
}

/**
 * @tc.name: GetLocalEnhancementPipelineWithExpected_test04
 * @tc.desc: 允许 NEW_IMAGE 类型修改为 YUV 类型
 */
HWTEST_F(MultistagesLocalEnhancementManagerTest, GetLocalEnhancementPipelineWithExpected_test04, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter GetLocalEnhancementPipelineWithExpected_test04");

    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    FileAsset fileAsset;
    CameraTestUtils::CreateFileAsset(assetInfo, fileAsset);
    CameraAssetInfo cameraAssetInfo(fileAsset);

    size_t count = LocalEnhancementManager::GetInstance().InsertLocalEnhancementData(
        CameraPipelineType::NEW_IMAGE, cameraAssetInfo);
    ASSERT_EQ(count, 1);

    // 修正为 YUV 类型
    auto pipeline = LocalEnhancementManager::GetInstance().GetLocalEnhancementPipelineWithExpected(
        PHOTO_ID_INPUT, CameraPipelineType::YUV);
    ASSERT_NE(pipeline, nullptr);
    ASSERT_EQ(pipeline->GetPipelineType(), CameraPipelineType::YUV);
    ASSERT_EQ(pipeline->GetAssetInfo().GetPhotoId(), PHOTO_ID_INPUT);
}

/**
 * @tc.name: GetLocalEnhancementPipelineWithExpected_test05
 * @tc.desc: YUV 类型不允许修改
 */
HWTEST_F(MultistagesLocalEnhancementManagerTest, GetLocalEnhancementPipelineWithExpected_test05, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter GetLocalEnhancementPipelineWithExpected_test05");

    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    FileAsset fileAsset;
    CameraTestUtils::CreateFileAsset(assetInfo, fileAsset);
    CameraAssetInfo cameraAssetInfo(fileAsset);

    size_t count = LocalEnhancementManager::GetInstance().InsertLocalEnhancementData(
        CameraPipelineType::YUV, cameraAssetInfo);
    ASSERT_EQ(count, 1);

    // YUV 类型不允许修改
    auto pipeline = LocalEnhancementManager::GetInstance().GetLocalEnhancementPipelineWithExpected(
        PHOTO_ID_INPUT, CameraPipelineType::NEW_IMAGE);
    ASSERT_NE(pipeline, nullptr);
    ASSERT_EQ(pipeline->GetPipelineType(), CameraPipelineType::YUV);
    ASSERT_EQ(pipeline->GetAssetInfo().GetPhotoId(), PHOTO_ID_INPUT);
}

/**
 * @tc.name: DeleteLocalEnhancementPipeline_test01
 * @tc.desc: 正常删除 pipeline
 */
HWTEST_F(MultistagesLocalEnhancementManagerTest, DeleteLocalEnhancementPipeline_test01, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter DeleteLocalEnhancementPipeline_test01");

    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    FileAsset fileAsset;
    CameraTestUtils::CreateFileAsset(assetInfo, fileAsset);
    CameraAssetInfo cameraAssetInfo(fileAsset);

    size_t count = LocalEnhancementManager::GetInstance().InsertLocalEnhancementData(
        CameraPipelineType::NEW_IMAGE, cameraAssetInfo);
    ASSERT_EQ(count, 1);
    ASSERT_EQ(LocalEnhancementManager::GetInstance().fileId2PhotoId_.size(), 1);
    ASSERT_EQ(LocalEnhancementManager::GetInstance().localEnhancementMap_.size(), 1);

    // 删除
    count = LocalEnhancementManager::GetInstance().DeleteLocalEnhancementPipeline(PHOTO_ID_INPUT);
    ASSERT_EQ(count, 0);
    ASSERT_EQ(LocalEnhancementManager::GetInstance().fileId2PhotoId_.size(), 0);
    ASSERT_EQ(LocalEnhancementManager::GetInstance().localEnhancementMap_.size(), 0);
}

/**
 * @tc.name: DeleteLocalEnhancementPipeline_test02
 * @tc.desc: photoId 不存在于 localEnhancementMap_, 无法删除
 */
HWTEST_F(MultistagesLocalEnhancementManagerTest, DeleteLocalEnhancementPipeline_test02, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter DeleteLocalEnhancementPipeline_test02");

    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    FileAsset fileAsset;
    CameraTestUtils::CreateFileAsset(assetInfo, fileAsset);
    CameraAssetInfo cameraAssetInfo(fileAsset);

    size_t count = LocalEnhancementManager::GetInstance().InsertLocalEnhancementData(
        CameraPipelineType::NEW_IMAGE, cameraAssetInfo);
    ASSERT_EQ(count, 1);

    // photoId 不存在
    count = LocalEnhancementManager::GetInstance().DeleteLocalEnhancementPipeline("not_exist");
    ASSERT_EQ(count, 1);
    ASSERT_EQ(LocalEnhancementManager::GetInstance().fileId2PhotoId_.size(), 1);
    ASSERT_EQ(LocalEnhancementManager::GetInstance().localEnhancementMap_.size(), 1);
}

/**
 * @tc.name: DeleteLocalEnhancementPipeline_test03
 * @tc.desc: pipeline 存在但 pipeline = nullptr: 脏数据, 会清理
 */
HWTEST_F(MultistagesLocalEnhancementManagerTest, DeleteLocalEnhancementPipeline_test03, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter DeleteLocalEnhancementPipeline_test03");

    LocalEnhancementManager::GetInstance().fileId2PhotoId_.emplace(
        std::make_pair(FILE_ID_INPUT, PHOTO_ID_INPUT));
    LocalEnhancementManager::GetInstance().localEnhancementMap_.emplace(
        std::make_pair(PHOTO_ID_INPUT, nullptr));
    ASSERT_EQ(LocalEnhancementManager::GetInstance().fileId2PhotoId_.size(), 1);
    ASSERT_EQ(LocalEnhancementManager::GetInstance().localEnhancementMap_.size(), 1);

    size_t count = LocalEnhancementManager::GetInstance().DeleteLocalEnhancementPipeline(PHOTO_ID_INPUT);
    ASSERT_EQ(count, 0);
    ASSERT_EQ(LocalEnhancementManager::GetInstance().fileId2PhotoId_.size(), 1);
    ASSERT_EQ(LocalEnhancementManager::GetInstance().localEnhancementMap_.size(), 0);
}

/**
 * @tc.name: DeleteLocalEnhancementPipeline_test04
 * @tc.desc: fileId 不存在于 fileId2PhotoId_: 脏数据, 会清理 localEnhancementMap_
 */
HWTEST_F(MultistagesLocalEnhancementManagerTest, DeleteLocalEnhancementPipeline_test04, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter DeleteLocalEnhancementPipeline_test04");

    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    FileAsset fileAsset;
    CameraTestUtils::CreateFileAsset(assetInfo, fileAsset);
    CameraAssetInfo cameraAssetInfo(fileAsset);

    auto pipeline = std::make_shared<NewImagePipeline>(CameraBasePipelineType::CAMERA_ASSET_PIPELINE);
    ASSERT_NE(pipeline, nullptr);
    pipeline->Init(cameraAssetInfo);

    // 只插入 localEnhancementMap_, 不插入 fileId2PhotoId_ (模拟脏数据)
    LocalEnhancementManager::GetInstance().localEnhancementMap_.emplace(std::make_pair(PHOTO_ID_INPUT, pipeline));
    ASSERT_EQ(LocalEnhancementManager::GetInstance().localEnhancementMap_.size(), 1);

    size_t count = LocalEnhancementManager::GetInstance().DeleteLocalEnhancementPipeline(PHOTO_ID_INPUT);
    ASSERT_EQ(count, 0);
    ASSERT_EQ(LocalEnhancementManager::GetInstance().localEnhancementMap_.size(), 0);
}

/**
 * @tc.name: DeleteLocalEnhancementPipeline_test05
 * @tc.desc: 删除后, 通过 fileId 无法再获取 pipeline
 */
HWTEST_F(MultistagesLocalEnhancementManagerTest, DeleteLocalEnhancementPipeline_test05, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter DeleteLocalEnhancementPipeline_test05");

    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    FileAsset fileAsset;
    CameraTestUtils::CreateFileAsset(assetInfo, fileAsset);
    CameraAssetInfo cameraAssetInfo(fileAsset);

    size_t count = LocalEnhancementManager::GetInstance().InsertLocalEnhancementData(
        CameraPipelineType::NEW_IMAGE, cameraAssetInfo);
    ASSERT_EQ(count, 1);

    // 删除
    count = LocalEnhancementManager::GetInstance().DeleteLocalEnhancementPipeline(PHOTO_ID_INPUT);
    ASSERT_EQ(count, 0);

    // 通过 fileId 无法再获取
    auto pipeline = LocalEnhancementManager::GetInstance().GetLocalEnhancementPipeline(FILE_ID_INPUT);
    ASSERT_EQ(pipeline, nullptr);
}

/**
 * @tc.name: DeleteLocalEnhancementPipeline_test06
 * @tc.desc: 删除后, IsPhotoInProcessEnhancementByPhotoId 返回 false
 */
HWTEST_F(MultistagesLocalEnhancementManagerTest, DeleteLocalEnhancementPipeline_test06, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter DeleteLocalEnhancementPipeline_test06");

    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    FileAsset fileAsset;
    CameraTestUtils::CreateFileAsset(assetInfo, fileAsset);
    CameraAssetInfo cameraAssetInfo(fileAsset);

    size_t count = LocalEnhancementManager::GetInstance().InsertLocalEnhancementData(
        CameraPipelineType::NEW_IMAGE, cameraAssetInfo);
    ASSERT_EQ(count, 1);

    // 删除
    count = LocalEnhancementManager::GetInstance().DeleteLocalEnhancementPipeline(PHOTO_ID_INPUT);
    ASSERT_EQ(count, 0);

    // IsPhotoInProcessEnhancementByPhotoId 返回 false
    bool ret = LocalEnhancementManager::GetInstance().IsPhotoInProcessEnhancementByPhotoId(PHOTO_ID_INPUT);
    ASSERT_EQ(ret, false);
}
} // namespace Media
} // namespace OHOS
