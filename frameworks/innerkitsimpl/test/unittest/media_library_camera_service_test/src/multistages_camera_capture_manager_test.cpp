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

#include "multistages_camera_capture_manager_test.h"

#include "camera_test_utils.h"
#include "media_uri_utils.h"
#define private public
#include "multistages_camera_capture_manager.h"
#undef private
#include "new_image_pipeline.h"
#include "userfilemgr_uri.h"

namespace OHOS {
namespace Media {
using namespace testing::ext;

static const int32_t FILE_ID_INPUT = 1;
static const std::string PHOTO_ID_INPUT = "1970_000000_0000_001";
static const std::string URI_INPUT = "file://media/Photo/1/IMG_000000_001/IMG_19700000_000001.jpg";

void MultistagesCameraCaptureManagerTest::SetUpTestCase(void) {}

void MultistagesCameraCaptureManagerTest::TearDownTestCase(void) {}

// SetUp:Execute before each test case
void MultistagesCameraCaptureManagerTest::SetUp()
{
    MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.clear();
    MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.clear();
}

void MultistagesCameraCaptureManagerTest::TearDown(void)
{
    MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.clear();
    MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.clear();
}

/**
 * @tc.name: [创建1个pipeline] InsertCaptureData_FileAsset_test01
 * @tc.desc: 创建一个pipeline, 需要首先确定子类的类型, 否则无法创建
 *           [1] 不支持 UNDEFINED 类型
 */
HWTEST_F(MultistagesCameraCaptureManagerTest, InsertCaptureData_FileAsset_test01, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter InsertCaptureData_FileAsset_test01");

    // UNDEFINED 类型
    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    std::string pipelineType = std::to_string(static_cast<int32_t>(CameraPipelineType::UNDEFINED));
    bool ret = CameraTestUtils::InsertPipelineForConfirmType(pipelineType, assetInfo);
    ASSERT_EQ(ret, false);
}

/**
 * @tc.name: [创建1个pipeline] InsertCaptureData_FileAsset_test02
 * @tc.desc: 创建一个pipeline, 需要首先确定子类的类型, 否则无法创建
 *           [1] 当前支持 NEW_IMAGE 类型
 */
HWTEST_F(MultistagesCameraCaptureManagerTest, InsertCaptureData_FileAsset_test02, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter InsertCaptureData_FileAsset_test02");

    // NEW_IMAGE 类型
    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    std::string pipelineType = std::to_string(static_cast<int32_t>(CameraPipelineType::NEW_IMAGE));
    bool ret = CameraTestUtils::InsertPipelineForConfirmType(pipelineType, assetInfo);
    ASSERT_EQ(ret, true);

    // 可以通过fileId获取
    CameraPipelineType typeByFileId = CameraPipelineType::UNDEFINED;
    auto pipeline1 = MultistagesCameraCaptureManager::GetInstance().GetPipelineByFileId(FILE_ID_INPUT, typeByFileId);
    ASSERT_NE(pipeline1, nullptr);
    ASSERT_EQ(typeByFileId, CameraPipelineType::NEW_IMAGE);
    ASSERT_EQ(pipeline1->GetAssetInfo().GetPhotoId(), PHOTO_ID_INPUT);

    // 可以通过photoId获取
    CameraPipelineType typeByPhotoId = CameraPipelineType::UNDEFINED;
    auto pipeline2 = MultistagesCameraCaptureManager::GetInstance().GetPipelineByPhotoId(PHOTO_ID_INPUT, typeByPhotoId);
    ASSERT_NE(pipeline2, nullptr);
    ASSERT_EQ(typeByPhotoId, CameraPipelineType::NEW_IMAGE);
    ASSERT_EQ(pipeline2->GetAssetInfo().GetFileId(), FILE_ID_INPUT);
}

/**
 * @tc.name: [创建1个pipeline] InsertCaptureData_FileAsset_test03
 * @tc.desc: 创建一个pipeline, 需要首先确定子类的类型, 否则无法创建
 *           [1] 当前支持 IMAGE 类型
 */
HWTEST_F(MultistagesCameraCaptureManagerTest, InsertCaptureData_FileAsset_test03, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter InsertCaptureData_FileAsset_test03");

    // IMAGE类型
    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    std::string pipelineType = std::to_string(static_cast<int32_t>(CameraPipelineType::IMAGE));
    bool ret = CameraTestUtils::InsertPipelineForConfirmType(pipelineType, assetInfo);
    ASSERT_EQ(ret, true);

    // 可以通过fileId获取
    CameraPipelineType typeByFileId = CameraPipelineType::UNDEFINED;
    auto pipeline1 = MultistagesCameraCaptureManager::GetInstance().GetPipelineByFileId(FILE_ID_INPUT, typeByFileId);
    ASSERT_NE(pipeline1, nullptr);
    ASSERT_EQ(typeByFileId, CameraPipelineType::IMAGE);
    ASSERT_EQ(pipeline1->GetAssetInfo().GetPhotoId(), PHOTO_ID_INPUT);

    // 可以通过photoId获取
    CameraPipelineType typeByPhotoId = CameraPipelineType::UNDEFINED;
    auto pipeline2 = MultistagesCameraCaptureManager::GetInstance().GetPipelineByPhotoId(PHOTO_ID_INPUT, typeByPhotoId);
    ASSERT_NE(pipeline2, nullptr);
    ASSERT_EQ(typeByPhotoId, CameraPipelineType::IMAGE);
    ASSERT_EQ(pipeline2->GetAssetInfo().GetFileId(), FILE_ID_INPUT);
}

/**
 * @tc.name: [创建1个pipeline] InsertCaptureData_FileAsset_test04
 * @tc.desc: 创建一个pipeline, 需要首先确定子类的类型, 否则无法创建
 *           [1] 当前支持 YUV 类型
 */
HWTEST_F(MultistagesCameraCaptureManagerTest, InsertCaptureData_FileAsset_test04, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter InsertCaptureData_FileAsset_test04");

    // YUV 类型
    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    std::string pipelineType = std::to_string(static_cast<int32_t>(CameraPipelineType::YUV));
    bool ret = CameraTestUtils::InsertPipelineForConfirmType(pipelineType, assetInfo);
    ASSERT_EQ(ret, true);

    // 可以通过fileId获取
    CameraPipelineType typeByFileId = CameraPipelineType::UNDEFINED;
    auto pipeline1 = MultistagesCameraCaptureManager::GetInstance().GetPipelineByFileId(FILE_ID_INPUT, typeByFileId);
    ASSERT_NE(pipeline1, nullptr);
    ASSERT_EQ(typeByFileId, CameraPipelineType::YUV);
    ASSERT_EQ(pipeline1->GetAssetInfo().GetPhotoId(), PHOTO_ID_INPUT);

    // 可以通过photoId获取
    CameraPipelineType typeByPhotoId = CameraPipelineType::UNDEFINED;
    auto pipeline2 = MultistagesCameraCaptureManager::GetInstance().GetPipelineByPhotoId(PHOTO_ID_INPUT, typeByPhotoId);
    ASSERT_NE(pipeline2, nullptr);
    ASSERT_EQ(typeByPhotoId, CameraPipelineType::YUV);
    ASSERT_EQ(pipeline2->GetAssetInfo().GetFileId(), FILE_ID_INPUT);
}

/**
 * @tc.name: [创建1个pipeline] InsertCaptureData_FileAsset_test05
 * @tc.desc: 创建一个pipeline, 需要首先确定子类的类型, 否则无法创建
 *           [1] 当前不支持 VIDEO 类型
 */
HWTEST_F(MultistagesCameraCaptureManagerTest, InsertCaptureData_FileAsset_test05, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter InsertCaptureData_FileAsset_test05");

    // VIDEO类型
    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    std::string pipelineType = std::to_string(static_cast<int32_t>(CameraPipelineType::VIDEO));
    bool ret = CameraTestUtils::InsertPipelineForConfirmType(pipelineType, assetInfo);
    ASSERT_EQ(ret, false);
}

/**
 * @tc.name: [创建1个pipeline] InsertCaptureData_FileAsset_test06
 * @tc.desc: 创建一个pipeline, 需要首先确定子类的类型, 否则无法创建
 *           [1] CameraPipelineType 必须是数字类型
 */
HWTEST_F(MultistagesCameraCaptureManagerTest, InsertCaptureData_FileAsset_test06, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter InsertCaptureData_FileAsset_test06");

    // "undefined"类型
    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    std::string pipelineType = "undefined";
    bool ret = CameraTestUtils::InsertPipelineForConfirmType(pipelineType, assetInfo);
    ASSERT_EQ(ret, false);
}

/**
 * @tc.name: [创建1个pipeline] InsertCaptureData_FileAsset_test07
 * @tc.desc: 如果 photoId = null, 则 pipeLinesMap_ 会以 fileId 作为 key
 */
HWTEST_F(MultistagesCameraCaptureManagerTest, InsertCaptureData_FileAsset_test07, TestSize.Level1)
{
    // NEW_IMAGE类型
    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
    };
    std::string pipelineType = std::to_string(static_cast<int32_t>(CameraPipelineType::NEW_IMAGE));
    bool ret = CameraTestUtils::InsertPipelineForConfirmType(pipelineType, assetInfo);
    ASSERT_EQ(ret, true);

    // 可以通过fileId获取
    CameraPipelineType typeByFileId = CameraPipelineType::UNDEFINED;
    auto pipeline1 = MultistagesCameraCaptureManager::GetInstance().GetPipelineByFileId(FILE_ID_INPUT, typeByFileId);
    ASSERT_NE(pipeline1, nullptr);
    ASSERT_EQ(typeByFileId, CameraPipelineType::NEW_IMAGE);
    ASSERT_EQ(pipeline1->GetAssetInfo().GetPhotoId(), "");

    // photoId 为 null 时, pipeLinesMap_ 会以 fileId 作为 key
    std::string photoId = std::to_string(FILE_ID_INPUT);
    CameraPipelineType typeByPhotoId = CameraPipelineType::UNDEFINED;
    auto pipeline2 = MultistagesCameraCaptureManager::GetInstance().GetPipelineByPhotoId(photoId, typeByPhotoId);
    ASSERT_NE(pipeline2, nullptr);
    ASSERT_EQ(typeByPhotoId, CameraPipelineType::NEW_IMAGE);
    ASSERT_EQ(pipeline2->GetAssetInfo().GetFileId(), FILE_ID_INPUT);
}

/**
 * @tc.name: [创建1个pipeline] InsertCaptureData_FileAsset_test08
 * @tc.desc: 不能重复插入相同数据
 */
HWTEST_F(MultistagesCameraCaptureManagerTest, InsertCaptureData_FileAsset_test08, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter InsertCaptureData_FileAsset_test08");

    // 插入一条数据
    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    std::string pipelineType = std::to_string(static_cast<int32_t>(CameraPipelineType::NEW_IMAGE));
    bool ret = CameraTestUtils::InsertPipelineForConfirmType(pipelineType, assetInfo);
    ASSERT_EQ(ret, true);

    // fileId 重复
    auto initPipeline = std::make_shared<NewImagePipeline>();
    ASSERT_NE(initPipeline, nullptr);
    size_t count = MultistagesCameraCaptureManager::GetInstance().InsertCaptureData(FILE_ID_INPUT, "new", initPipeline);
    ASSERT_EQ(count, 1);

    // photoId 重复
    count = MultistagesCameraCaptureManager::GetInstance().InsertCaptureData(2, PHOTO_ID_INPUT, initPipeline);
    ASSERT_EQ(count, 1);
}

/**
 * @tc.name: [异常场景下, 恢复pipeline] RecoverForSessionSync_test01
 * @tc.desc: 恢复的pipeline, 默认为 NewImagePipeline
 */
HWTEST_F(MultistagesCameraCaptureManagerTest, RecoverForSessionSync_test01, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter RecoverForSessionSync_test01");

    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    FileAsset fileAsset;
    CameraTestUtils::CreateFileAsset(assetInfo, fileAsset);
    size_t count = MultistagesCameraCaptureManager::GetInstance().RecoverForSessionSync(fileAsset, false);
    ASSERT_EQ(count, 1);

    // 可以通过fileId获取
    CameraPipelineType typeByFileId = CameraPipelineType::UNDEFINED;
    auto pipeline1 = MultistagesCameraCaptureManager::GetInstance().GetPipelineByFileId(FILE_ID_INPUT, typeByFileId);
    ASSERT_NE(pipeline1, nullptr);
    ASSERT_EQ(typeByFileId, CameraPipelineType::NEW_IMAGE);
    ASSERT_EQ(pipeline1->GetAssetInfo().GetPhotoId(), PHOTO_ID_INPUT);
}

/**
 * @tc.name: [获取1个 pipeline] GetPipeline_test01
 * @tc.desc: 获取 pipeline 的同时会返回对应 type
 */
HWTEST_F(MultistagesCameraCaptureManagerTest, GetPipeline_test01, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter GetPipeline_test01");

    // NEW_IMAGE类型
    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    std::string pipelineType = std::to_string(static_cast<int32_t>(CameraPipelineType::NEW_IMAGE));
    bool ret = CameraTestUtils::InsertPipelineForConfirmType(pipelineType, assetInfo);
    ASSERT_EQ(ret, true);

    // 可以通过fileId获取
    CameraPipelineType typeByFileId = CameraPipelineType::UNDEFINED;
    auto pipeline1 = MultistagesCameraCaptureManager::GetInstance().GetPipelineByFileId(FILE_ID_INPUT, typeByFileId);
    ASSERT_NE(pipeline1, nullptr);
    ASSERT_EQ(typeByFileId, CameraPipelineType::NEW_IMAGE);
    ASSERT_EQ(pipeline1->GetAssetInfo().GetPhotoId(), PHOTO_ID_INPUT);

    // 可以通过photoId获取
    CameraPipelineType typeByPhotoId = CameraPipelineType::UNDEFINED;
    auto pipeline2 = MultistagesCameraCaptureManager::GetInstance().GetPipelineByPhotoId(PHOTO_ID_INPUT, typeByPhotoId);
    ASSERT_NE(pipeline2, nullptr);
    ASSERT_EQ(typeByPhotoId, CameraPipelineType::NEW_IMAGE);
    ASSERT_EQ(pipeline2->GetAssetInfo().GetFileId(), FILE_ID_INPUT);
}

/**
 * @tc.name: [获取1个 pipeline] GetPipeline_test02
 * @tc.desc: 如果内存中没有pipeline, 则无法获取
 */
HWTEST_F(MultistagesCameraCaptureManagerTest, GetPipeline_test02, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter GetPipeline_test02");

    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.size(), 0);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.size(), 0);

    // 基于fileId
    CameraPipelineType typeByFileId = CameraPipelineType::UNDEFINED;
    auto pipeline1 = MultistagesCameraCaptureManager::GetInstance().GetPipelineByFileId(FILE_ID_INPUT, typeByFileId);
    ASSERT_EQ(pipeline1, nullptr);

    // 基于photoId
    CameraPipelineType typeByPhotoId = CameraPipelineType::UNDEFINED;
    auto pipeline2 = MultistagesCameraCaptureManager::GetInstance().GetPipelineByPhotoId(PHOTO_ID_INPUT, typeByPhotoId);
    ASSERT_EQ(pipeline2, nullptr);
}

/**
 * @tc.name: [部分场景下, 重新修改pipeline] GetPipelineByFileIdWithExpected_test01
 * @tc.desc: 不支持预期值为 UNDEFINED 类型
 */
HWTEST_F(MultistagesCameraCaptureManagerTest, GetPipelineByFileIdWithExpected_test01, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter GetPipelineByFileIdWithExpected_test01");

    // 基于fileId
    auto pipelineByFileId = MultistagesCameraCaptureManager::GetInstance().GetPipelineByFileIdWithExpected(
        1, CameraPipelineType::UNDEFINED);
    ASSERT_EQ(pipelineByFileId, nullptr);

    // 基于photoId
    auto pipelineByPhotoId = MultistagesCameraCaptureManager::GetInstance().GetPipelineByPhotoIdWithExpected(
        "1", CameraPipelineType::UNDEFINED);
    ASSERT_EQ(pipelineByPhotoId, nullptr);
}

/**
 * @tc.name: [部分场景下, 重新修改pipeline] GetPipelineByFileIdWithExpected_test02
 * @tc.desc: 如果预期值(Expected type)和缓存(Pipeline type)一致, 则正常返回
 */
HWTEST_F(MultistagesCameraCaptureManagerTest, GetPipelineByFileIdWithExpected_test02, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter GetPipelineByFileIdWithExpected_test02");

    // 1.创建 NEW_IMAGE 类型
    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    std::string pipelineType = std::to_string(static_cast<int32_t>(CameraPipelineType::NEW_IMAGE));
    bool ret = CameraTestUtils::InsertPipelineForConfirmType(pipelineType, assetInfo);
    ASSERT_EQ(ret, true);

    // 2.可以通过 fileId 获取
    CameraPipelineType typeByFileId = CameraPipelineType::UNDEFINED;
    auto pipeline = MultistagesCameraCaptureManager::GetInstance().GetPipelineByFileId(FILE_ID_INPUT, typeByFileId);
    ASSERT_NE(pipeline, nullptr);
    ASSERT_EQ(typeByFileId, CameraPipelineType::NEW_IMAGE);
    ASSERT_EQ(pipeline->GetAssetInfo().GetPhotoId(), PHOTO_ID_INPUT);

    // 3.基于 fileId, 修正为 IMAGE 类型
    auto pipelineImproved = MultistagesCameraCaptureManager::GetInstance().GetPipelineByFileIdWithExpected(
        FILE_ID_INPUT, CameraPipelineType::NEW_IMAGE);
    ASSERT_NE(pipelineImproved, nullptr);
    ASSERT_EQ(pipelineImproved->GetPipelineType(), CameraPipelineType::NEW_IMAGE);
    ASSERT_EQ(pipelineImproved->GetAssetInfo().GetPhotoId(), PHOTO_ID_INPUT);
}

/**
 * @tc.name: [部分场景下, 重新修改pipeline] GetPipelineByFileIdWithExpected_test03
 * @tc.desc: [实际业务中, 不存在 UNDEFINED 的场景, 仅看护代码] 不允许 UNDEFINED 类型, 修改为 NEW_IMAGE 类型
 */
HWTEST_F(MultistagesCameraCaptureManagerTest, GetPipelineByFileIdWithExpected_test03, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter GetPipelineByFileIdWithExpected_test03");

    // 1.创建一个 UNDEFINED 类型的 pipeline
    FileAssetInfo newImageAssetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    bool ret = CameraTestUtils::PrepareMockPipeline(CameraPipelineType::UNDEFINED, newImageAssetInfo);
    ASSERT_EQ(ret, true);

    // 可以通过fileId获取
    CameraPipelineType typeByFileId = CameraPipelineType::UNDEFINED;
    auto pipeline = MultistagesCameraCaptureManager::GetInstance().GetPipelineByFileId(FILE_ID_INPUT, typeByFileId);
    ASSERT_NE(pipeline, nullptr);
    ASSERT_EQ(typeByFileId, CameraPipelineType::UNDEFINED);
    ASSERT_EQ(pipeline->GetAssetInfo().GetPhotoId(), PHOTO_ID_INPUT);

    // 2.基于photoId, 修正为 NEW_IMAGE 类型
    auto pipelineImproved = MultistagesCameraCaptureManager::GetInstance().GetPipelineByPhotoIdWithExpected(
        PHOTO_ID_INPUT, CameraPipelineType::NEW_IMAGE);
    ASSERT_NE(pipelineImproved, nullptr);
    ASSERT_EQ(pipelineImproved->GetPipelineType(), CameraPipelineType::UNDEFINED);
    ASSERT_EQ(pipelineImproved->GetAssetInfo().GetFileId(), FILE_ID_INPUT);
}

/**
 * @tc.name: [部分场景下, 重新修改pipeline] GetPipelineByFileIdWithExpected_test05
 * @tc.desc: 允许 NEW_IMAGE 类型, 修改为 IMAGE 类型
 */
HWTEST_F(MultistagesCameraCaptureManagerTest, GetPipelineByFileIdWithExpected_test05, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter GetPipelineByFileIdWithExpected_test05");

    // 1.创建一个 NEW_IMAGE 类型的 pipeline
    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    std::string pipelineType = std::to_string(static_cast<int32_t>(CameraPipelineType::NEW_IMAGE));
    bool ret = CameraTestUtils::InsertPipelineForConfirmType(pipelineType, assetInfo);
    ASSERT_EQ(ret, true);

    // 可以通过fileId获取
    CameraPipelineType typeByFileId = CameraPipelineType::UNDEFINED;
    auto pipeline = MultistagesCameraCaptureManager::GetInstance().GetPipelineByFileId(1, typeByFileId);
    ASSERT_NE(pipeline, nullptr);
    ASSERT_EQ(typeByFileId, CameraPipelineType::NEW_IMAGE);
    ASSERT_EQ(pipeline->GetAssetInfo().GetPhotoId(), PHOTO_ID_INPUT);

    // 2.基于fileId, 修正为 IMAGE 类型
    pipeline = MultistagesCameraCaptureManager::GetInstance().GetPipelineByFileIdWithExpected(
        FILE_ID_INPUT, CameraPipelineType::IMAGE);
    ASSERT_NE(pipeline, nullptr);
    ASSERT_EQ(pipeline->GetPipelineType(), CameraPipelineType::IMAGE);
    ASSERT_EQ(pipeline->GetAssetInfo().GetPhotoId(), PHOTO_ID_INPUT);
}

/**
 * @tc.name: [部分场景下, 重新修改pipeline] GetPipelineByFileIdWithExpected_test06
 * @tc.desc: IMAGE 类型不允许修改
 */
HWTEST_F(MultistagesCameraCaptureManagerTest, GetPipelineByFileIdWithExpected_test06, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter GetPipelineByFileIdWithExpected_test06");

    // 1.创建一个 IMAGE 类型的 pipeline
    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    std::string pipelineType = std::to_string(static_cast<int32_t>(CameraPipelineType::IMAGE));
    bool ret = CameraTestUtils::InsertPipelineForConfirmType(pipelineType, assetInfo);
    ASSERT_EQ(ret, true);

    // 可以通过fileId获取
    CameraPipelineType typeByFileId = CameraPipelineType::UNDEFINED;
    auto pipeline = MultistagesCameraCaptureManager::GetInstance().GetPipelineByFileId(1, typeByFileId);
    ASSERT_NE(pipeline, nullptr);
    ASSERT_EQ(typeByFileId, CameraPipelineType::IMAGE);
    ASSERT_EQ(pipeline->GetAssetInfo().GetPhotoId(), PHOTO_ID_INPUT);

    // 2.基于fileId, 修正为 IMAGE 类型
    pipeline = MultistagesCameraCaptureManager::GetInstance().GetPipelineByFileIdWithExpected(
        FILE_ID_INPUT, CameraPipelineType::NEW_IMAGE);
    ASSERT_NE(pipeline, nullptr);
    ASSERT_EQ(pipeline->GetPipelineType(), CameraPipelineType::IMAGE);
    ASSERT_EQ(pipeline->GetAssetInfo().GetPhotoId(), PHOTO_ID_INPUT);
}

/**
 * @tc.name: [删除1个pipeline] DeletePipelineWithFileId_test01
 * @tc.desc: pipeline 如果为一阶段生命周期, 一阶段结束以后, 可以清理
 */
HWTEST_F(MultistagesCameraCaptureManagerTest, DeletePipelineWithFileId_test01, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter DeletePipelineWithFileId_test01");

    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    auto pipeline = std::make_shared<NewImagePipeline>();
    ASSERT_NE(pipeline, nullptr);
    FileAsset fileAsset;
    CameraTestUtils::CreateFileAsset(assetInfo, fileAsset);
    CameraAssetInfo cameraAssetInfo(fileAsset);
    pipeline->Init(cameraAssetInfo);
    ASSERT_EQ(pipeline->GetAssetInfo().GetActiveType(), CameraInfoActiveType::FirstStage);

    MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.emplace(
        std::make_pair(FILE_ID_INPUT, PHOTO_ID_INPUT));
    MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.emplace(std::make_pair(PHOTO_ID_INPUT, pipeline));
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.size(), 1);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.size(), 1);

    // 1.预期无法清理
    size_t count = MultistagesCameraCaptureManager::GetInstance().DeletePipelineWithFileId(FILE_ID_INPUT, false);
    ASSERT_EQ(count, 1);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.size(), 1);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.size(), 1);

    // 设置一阶段结束
    pipeline->SaveCameraPhotoFinished();
    // value符合预期, 可以清理
    count = MultistagesCameraCaptureManager::GetInstance().DeletePipelineWithFileId(FILE_ID_INPUT, false);
    ASSERT_EQ(count, 0);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.size(), 0);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.size(), 0);
}

/**
 * @tc.name: [删除1个pipeline] DeletePipelineWithFileId_test02
 * @tc.desc: pipeline 如果为一阶段生命周期, 如果丢弃该图片, 则可以直接清理
 */
HWTEST_F(MultistagesCameraCaptureManagerTest, DeletePipelineWithFileId_test02, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter DeletePipelineWithFileId_test02");

    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };

    auto pipeline = std::make_shared<NewImagePipeline>();
    ASSERT_NE(pipeline, nullptr);
    FileAsset fileAsset;
    CameraTestUtils::CreateFileAsset(assetInfo, fileAsset);
    CameraAssetInfo cameraAssetInfo(fileAsset);
    pipeline->Init(cameraAssetInfo);
    ASSERT_EQ(pipeline->GetAssetInfo().GetActiveType(), CameraInfoActiveType::FirstStage);

    MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.emplace(
        std::make_pair(FILE_ID_INPUT, PHOTO_ID_INPUT));
    MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.emplace(std::make_pair(PHOTO_ID_INPUT, pipeline));
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.size(), 1);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.size(), 1);

    // value符合预期, 可以清理
    size_t count = MultistagesCameraCaptureManager::GetInstance().DeletePipelineWithFileId(FILE_ID_INPUT, true);
    ASSERT_EQ(count, 0);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.size(), 0);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.size(), 0);
}

/**
 * @tc.name: [删除1个pipeline] DeletePipelineWithFileId_test03
 * @tc.desc: pipeline 如果为二阶段生命周期, 需要2个阶段均结束, 才可以清理
 */
HWTEST_F(MultistagesCameraCaptureManagerTest, DeletePipelineWithFileId_test03, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter DeletePipelineWithFileId_test03");

    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    auto pipeline = std::make_shared<NewImagePipeline>();
    ASSERT_NE(pipeline, nullptr);
    FileAsset fileAsset;
    CameraTestUtils::CreateFileAsset(assetInfo, fileAsset);
    CameraAssetInfo cameraAssetInfo(fileAsset);
    pipeline->Init(cameraAssetInfo);
    pipeline->SetActiveType(CameraInfoActiveType::SecondStage);
    ASSERT_EQ(pipeline->GetAssetInfo().GetActiveType(), CameraInfoActiveType::SecondStage);

    MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.emplace(
        std::make_pair(FILE_ID_INPUT, PHOTO_ID_INPUT));
    MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.emplace(std::make_pair(PHOTO_ID_INPUT, pipeline));
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.size(), 1);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.size(), 1);

    // 1.预期无法清理
    size_t count = MultistagesCameraCaptureManager::GetInstance().DeletePipelineWithFileId(FILE_ID_INPUT, false);
    ASSERT_EQ(count, 1);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.size(), 1);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.size(), 1);

    // 设置一阶段结束
    pipeline->SaveCameraPhotoFinished();
    // 2.预期无法清理
    count = MultistagesCameraCaptureManager::GetInstance().DeletePipelineWithFileId(FILE_ID_INPUT, false);
    ASSERT_EQ(count, 1);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.size(), 1);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.size(), 1);

    // 设置二阶段结束
    pipeline->OnProcessFinished();
    // value符合预期, 可以清理
    count = MultistagesCameraCaptureManager::GetInstance().DeletePipelineWithFileId(FILE_ID_INPUT, false);
    ASSERT_EQ(count, 0);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.size(), 0);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.size(), 0);
}

/**
 * @tc.name: [删除1个pipeline] DeletePipelineWithFileId_test04
 * @tc.desc: pipeline 如果为二阶段生命周期, 如果丢弃该图片, 则可以直接清理
 */
HWTEST_F(MultistagesCameraCaptureManagerTest, DeletePipelineWithFileId_test04, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter DeletePipelineWithFileId_test04");

    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };

    auto pipeline = std::make_shared<NewImagePipeline>();
    ASSERT_NE(pipeline, nullptr);
    FileAsset fileAsset;
    CameraTestUtils::CreateFileAsset(assetInfo, fileAsset);
    CameraAssetInfo cameraAssetInfo(fileAsset);
    pipeline->Init(cameraAssetInfo);
    pipeline->SetActiveType(CameraInfoActiveType::SecondStage);
    ASSERT_EQ(pipeline->GetAssetInfo().GetActiveType(), CameraInfoActiveType::SecondStage);

    MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.emplace(
        std::make_pair(FILE_ID_INPUT, PHOTO_ID_INPUT));
    MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.emplace(std::make_pair(PHOTO_ID_INPUT, pipeline));
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.size(), 1);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.size(), 1);

    // value符合预期, 可以清理
    size_t count = MultistagesCameraCaptureManager::GetInstance().DeletePipelineWithFileId(FILE_ID_INPUT, true);
    ASSERT_EQ(count, 0);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.size(), 0);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.size(), 0);
}

/**
 * @tc.name: [删除1个pipeline] DeletePipelineWithFileId_test05
 * @tc.desc: fileId2PhotoId_中不存在, 无法删除
 */
HWTEST_F(MultistagesCameraCaptureManagerTest, DeletePipelineWithFileId_test05, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter DeletePipelineWithFileId_test05");

    // 预置数据
    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    auto pipeline = std::make_shared<NewImagePipeline>();
    ASSERT_NE(pipeline, nullptr);
    FileAsset fileAsset;
    CameraTestUtils::CreateFileAsset(assetInfo, fileAsset);
    CameraAssetInfo cameraAssetInfo(fileAsset);
    pipeline->Init(cameraAssetInfo);
    ASSERT_EQ(pipeline->GetAssetInfo().GetActiveType(), CameraInfoActiveType::FirstStage);

    MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.emplace(
        std::make_pair(FILE_ID_INPUT, PHOTO_ID_INPUT));
    MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.emplace(std::make_pair(PHOTO_ID_INPUT, pipeline));
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.size(), 1);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.size(), 1);

    // 执行
    size_t count = MultistagesCameraCaptureManager::GetInstance().DeletePipelineWithFileId(2, true);
    ASSERT_EQ(count, 1);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.size(), 1);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.size(), 1);
}

/**
 * @tc.name: [删除1个pipeline] DeletePipelineWithFileId_test06
 * @tc.desc: pipeLinesMap_中不存在: 表示 fileId2PhotoId_ 中存在脏数据, 会清理脏数据
 */
HWTEST_F(MultistagesCameraCaptureManagerTest, DeletePipelineWithFileId_test06, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter DeletePipelineWithFileId_test06");

    // 预置数据
    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    auto pipeline = std::make_shared<NewImagePipeline>();
    ASSERT_NE(pipeline, nullptr);
    FileAsset fileAsset;
    CameraTestUtils::CreateFileAsset(assetInfo, fileAsset);
    CameraAssetInfo cameraAssetInfo(fileAsset);
    pipeline->Init(cameraAssetInfo);
    ASSERT_EQ(pipeline->GetAssetInfo().GetActiveType(), CameraInfoActiveType::FirstStage);

    MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.emplace(
        std::make_pair(FILE_ID_INPUT, PHOTO_ID_INPUT));
    MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.emplace(std::make_pair("new", pipeline));
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.size(), 1);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.size(), 1);
    
    // 执行
    size_t count = MultistagesCameraCaptureManager::GetInstance().DeletePipelineWithFileId(FILE_ID_INPUT, true);
    ASSERT_EQ(count, 1);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.size(), 0);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.size(), 1);
}

/**
 * @tc.name: [删除1个pipeline] DeletePipelineWithFileId_test07
 * @tc.desc: fileId2PhotoId_ 和 pipeLinesMap_中均存在, 但pipeline = nullptr: 脏数据, 会清理
 */
HWTEST_F(MultistagesCameraCaptureManagerTest, DeletePipelineWithFileId_test07, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter DeletePipelineWithFileId_test07");

    MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.emplace(
        std::make_pair(FILE_ID_INPUT, PHOTO_ID_INPUT));
    MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.emplace(std::make_pair(PHOTO_ID_INPUT, nullptr));
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.size(), 1);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.size(), 1);

    // 执行
    size_t count = MultistagesCameraCaptureManager::GetInstance().DeletePipelineWithFileId(FILE_ID_INPUT, true);
    ASSERT_EQ(count, 0);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.size(), 0);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.size(), 0);
}

/**
 * @tc.name: [删除1个pipeline] DeletePipelineWithPhotoId_test01
 * @tc.desc: pipeline 如果为二阶段生命周期, 需要2个阶段均结束, 才可以清理
 */
HWTEST_F(MultistagesCameraCaptureManagerTest, DeletePipelineWithPhotoId_test01, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter DeletePipelineWithPhotoId_test01");

    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    auto pipeline = std::make_shared<NewImagePipeline>();
    ASSERT_NE(pipeline, nullptr);
    FileAsset fileAsset;
    CameraTestUtils::CreateFileAsset(assetInfo, fileAsset);
    CameraAssetInfo cameraAssetInfo(fileAsset);
    pipeline->Init(cameraAssetInfo);
    ASSERT_EQ(pipeline->GetAssetInfo().GetActiveType(), CameraInfoActiveType::FirstStage);

    MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.emplace(
        std::make_pair(FILE_ID_INPUT, PHOTO_ID_INPUT));
    MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.emplace(std::make_pair(PHOTO_ID_INPUT, pipeline));
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.size(), 1);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.size(), 1);

    // 1.预期无法清理
    size_t count = MultistagesCameraCaptureManager::GetInstance(). DeletePipelineWithPhotoId(PHOTO_ID_INPUT, false);
    ASSERT_EQ(count, 1);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.size(), 1);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.size(), 1);

    // 设置二阶段结束
    pipeline->OnProcessFinished();
    // 2.预期无法清理
    count = MultistagesCameraCaptureManager::GetInstance(). DeletePipelineWithPhotoId(PHOTO_ID_INPUT, false);
    ASSERT_EQ(count, 1);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.size(), 1);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.size(), 1);

    // 设置一阶段结束
    pipeline->SaveCameraPhotoFinished();
    count = MultistagesCameraCaptureManager::GetInstance(). DeletePipelineWithPhotoId(PHOTO_ID_INPUT, false);
    ASSERT_EQ(count, 0);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.size(), 0);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.size(), 0);
}

/**
 * @tc.name: [删除1个pipeline] DeletePipelineWithPhotoId_test02
 * @tc.desc: pipeLinesMap_中不存在, 无法删除
 */
HWTEST_F(MultistagesCameraCaptureManagerTest, DeletePipelineWithPhotoId_test02, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter DeletePipelineWithPhotoId_test02");

    // 预置数据
    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    auto pipeline = std::make_shared<NewImagePipeline>();
    ASSERT_NE(pipeline, nullptr);
    FileAsset fileAsset;
    CameraTestUtils::CreateFileAsset(assetInfo, fileAsset);
    CameraAssetInfo cameraAssetInfo(fileAsset);
    pipeline->Init(cameraAssetInfo);
    ASSERT_EQ(pipeline->GetAssetInfo().GetActiveType(), CameraInfoActiveType::FirstStage);

    MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.emplace(
        std::make_pair(FILE_ID_INPUT, PHOTO_ID_INPUT));
    MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.emplace(std::make_pair(PHOTO_ID_INPUT, pipeline));
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.size(), 1);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.size(), 1);

    // 执行
    size_t count = MultistagesCameraCaptureManager::GetInstance(). DeletePipelineWithPhotoId("new", true);
    ASSERT_EQ(count, 1);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.size(), 1);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.size(), 1);
}

/**
 * @tc.name: [删除1个pipeline] DeletePipelineWithPhotoId_test03
 * @tc.desc: pipeLinesMap_中存在, 但pipeline = nullptr: 脏数据, 会清理
 */
HWTEST_F(MultistagesCameraCaptureManagerTest, DeletePipelineWithPhotoId_test03, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter DeletePipelineWithPhotoId_test03");

    // 预置数据
    MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.emplace(
        std::make_pair(FILE_ID_INPUT, PHOTO_ID_INPUT));
    MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.emplace(std::make_pair(PHOTO_ID_INPUT, nullptr));
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.size(), 1);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.size(), 1);

    // 执行
    size_t count = MultistagesCameraCaptureManager::GetInstance(). DeletePipelineWithPhotoId(PHOTO_ID_INPUT, true);
    ASSERT_EQ(count, 0);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.size(), 1);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.size(), 0);
}

/**
 * @tc.name: [删除1个pipeline] DeletePipelineWithPhotoId_test04
 * @tc.desc: fileId2PhotoId_中不存在: 表示 pipeLinesMap_ 中存在脏数据, 会清理脏数据
 */
HWTEST_F(MultistagesCameraCaptureManagerTest, DeletePipelineWithPhotoId_test04, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter DeletePipelineWithPhotoId_test04");

    // 预置数据
    FileAssetInfo assetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    auto pipeline = std::make_shared<NewImagePipeline>();
    ASSERT_NE(pipeline, nullptr);
    FileAsset fileAsset;
    CameraTestUtils::CreateFileAsset(assetInfo, fileAsset);
    CameraAssetInfo cameraAssetInfo(fileAsset);
    pipeline->Init(cameraAssetInfo);
    ASSERT_EQ(pipeline->GetAssetInfo().GetActiveType(), CameraInfoActiveType::FirstStage);

    MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.emplace(
        std::make_pair(2, PHOTO_ID_INPUT));
    MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.emplace(std::make_pair(PHOTO_ID_INPUT, pipeline));
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.size(), 1);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.size(), 1);

    // 执行
    size_t count = MultistagesCameraCaptureManager::GetInstance(). DeletePipelineWithPhotoId(PHOTO_ID_INPUT, true);
    ASSERT_EQ(count, 0);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.size(), 1);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.size(), 0);
}
} // namespace Media
} // namespace OHOS