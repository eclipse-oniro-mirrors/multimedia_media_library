/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#define MLOG_TAG "MediaLibraryAssetHelperCapiTest"

#define private public
#include "media_library_asset_helper_capi_test.h"
#include "datashare_helper.h"
#include "fetch_result.h"
#include "file_asset.h"
#include "get_self_permissions.h"
#include "hilog/log.h"
#include "iservice_registry.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "media_asset_base_capi.h"
#include "media_access_helper_capi.h"
#include "media_asset_manager_capi.h"
#include "media_asset_types.h"
#include "oh_media_asset_change_request.h"
#include "media_asset_change_request_capi.h"
#include "media_asset_capi.h"
#include "system_ability_definition.h"
#include "oh_media_asset.h"
#include "media_asset.h"
#include "media_file_utils.h"
#include "media_library_manager.h"
#include "media_log.h"
#include "media_volume.h"
#include "scanner_utils.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {

const int SCAN_WAIT_TIME_1S = 1;
const std::string ROOT_TEST_MEDIA_DIR =
    "/data/app/el2/100/base/com.ohos.medialibrary.medialibrarydata/haps/";
MediaLibraryManager* mediaLibraryManager = MediaLibraryManager::GetMediaLibraryManager();

void MediaLibraryAssetHelperCapiTest::SetUpTestCase(void) {}

void MediaLibraryAssetHelperCapiTest::TearDownTestCase(void) {}

void MediaLibraryAssetHelperCapiTest::SetUp(void) {}

void MediaLibraryAssetHelperCapiTest::TearDown(void) {}

/**
 * @tc.name: media_library_capi_test_001
 * @tc.desc: OH_MediaAssetChangeRequest_Create entry parameter is MEDIA_TYPE_IMAGE
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_001, TestSize.Level0)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto fileAssetPtr = mediaAsset->mediaAsset_->GetFileAssetInstance();
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_002
 * @tc.desc: OH_MediaAssetChangeRequest_Create entry parameter is MEDIA_TYPE_VIDEO
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_002, TestSize.Level0)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_VIDEO);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto fileAssetPtr = mediaAsset->mediaAsset_->GetFileAssetInstance();
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_003
 * @tc.desc: OH_MediaAssetChangeRequest_Create entry parameter is TYPE_MEDIALIBRARY
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_003, TestSize.Level0)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_MEDIALIBRARY);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto fileAssetPtr = mediaAsset->mediaAsset_->GetFileAssetInstance();
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    EXPECT_EQ(changeRequest, nullptr);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_004
 * @tc.desc: OH_MediaAssetChangeRequest_AddResourceWithBuffer resourceType is MEDIA_LIBRARY_IMAGE_RESOURCE
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_004, TestSize.Level0)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    MediaLibrary_ResourceType resourceType = MediaLibrary_ResourceType::MEDIA_LIBRARY_IMAGE_RESOURCE;
    fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::CAMERA));
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto fileAssetPtr = mediaAsset->mediaAsset_->GetFileAssetInstance();
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    uint8_t buffer[] = {0x01, 0x02, 0x03, 0x04};
    uint32_t length = sizeof(buffer);
    uint32_t result = OH_MediaAssetChangeRequest_AddResourceWithBuffer(changeRequest, resourceType, buffer, length);
    EXPECT_EQ(result, MEDIA_LIBRARY_OK);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_005
 * @tc.desc: OH_MediaAssetChangeRequest_AddResourceWithBuffer length is zero
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_005, TestSize.Level0)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    MediaLibrary_ResourceType resourceType = MediaLibrary_ResourceType::MEDIA_LIBRARY_IMAGE_RESOURCE;
    fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto fileAssetPtr = mediaAsset->mediaAsset_->GetFileAssetInstance();
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    uint8_t buffer[] = {0x01, 0x02, 0x03, 0x04};
    uint32_t length = 0;
    uint32_t result = OH_MediaAssetChangeRequest_AddResourceWithBuffer(changeRequest, resourceType, buffer, length);
    EXPECT_EQ(result, MEDIA_LIBRARY_PARAMETER_ERROR);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_006
 * @tc.desc: OH_MediaAssetChangeRequest_AddResourceWithBuffer GetPhotoSubType is CAMERA
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_006, TestSize.Level0)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    MediaLibrary_ResourceType resourceType = MediaLibrary_ResourceType::MEDIA_LIBRARY_IMAGE_RESOURCE;
    fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::CAMERA));
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto fileAssetPtr = mediaAsset->mediaAsset_->GetFileAssetInstance();
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    uint8_t buffer[] = {0x01, 0x02, 0x03, 0x04};
    uint32_t length = 0;
    uint32_t result = OH_MediaAssetChangeRequest_AddResourceWithBuffer(changeRequest, resourceType, buffer, length);
    EXPECT_EQ(result, MEDIA_LIBRARY_PARAMETER_ERROR);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_007
 * @tc.desc: OH_MediaAssetChangeRequest_AddResourceWithBuffer resourceType is MEDIA_LIBRARY_VIDEO_RESOURCE
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_007, TestSize.Level0)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    MediaLibrary_ResourceType resourceType = MediaLibrary_ResourceType::MEDIA_LIBRARY_VIDEO_RESOURCE;
    fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto fileAssetPtr = mediaAsset->mediaAsset_->GetFileAssetInstance();
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    uint8_t buffer[] = {0x01, 0x02, 0x03, 0x04};
    uint32_t length = sizeof(buffer);
    uint32_t result = OH_MediaAssetChangeRequest_AddResourceWithBuffer(changeRequest, resourceType, buffer, length);
    EXPECT_EQ(result, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED);
    OH_MediaAccessHelper_ApplyChanges(changeRequest);
    uint32_t resultChange = OH_MediaAssetChangeRequest_AddResourceWithBuffer(changeRequest, resourceType, buffer,
        length);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_008
 * @tc.desc: OH_MediaAssetChangeRequest_SaveCameraPhoto imageFileType is SET_EDIT_DATA
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_008, TestSize.Level0)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto fileAssetPtr = mediaAsset->mediaAsset_->GetFileAssetInstance();
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    MediaLibrary_ImageFileType imageFileType = MEDIA_LIBRARY_IMAGE_JPEG;
    AssetChangeOperation changeOperation = AssetChangeOperation::SET_EDIT_DATA;
    changeRequest->request_->RecordChangeOperation(changeOperation);
    uint32_t result = OH_MediaAssetChangeRequest_SaveCameraPhoto(changeRequest, imageFileType);
    EXPECT_EQ(result, MEDIA_LIBRARY_OK);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_009
 * @tc.desc: OH_MediaAssetChangeRequest_SaveCameraPhoto changeOperation is ADD_FILTERS
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_009, TestSize.Level0)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto fileAssetPtr = mediaAsset->mediaAsset_->GetFileAssetInstance();
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    MediaLibrary_ImageFileType imageFileType = MEDIA_LIBRARY_IMAGE_JPEG;
    AssetChangeOperation changeOperation = AssetChangeOperation::ADD_FILTERS;
    changeRequest->request_->RecordChangeOperation(changeOperation);
    uint32_t result = OH_MediaAssetChangeRequest_SaveCameraPhoto(changeRequest, imageFileType);
    EXPECT_EQ(result, MEDIA_LIBRARY_OK);
    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_PARAMETER_ERROR);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_010
 * @tc.desc: OH_MediaAssetChangeRequest_SaveCameraPhoto changeOperation is CREATE_FROM_SCRATCH
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_010, TestSize.Level0)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto fileAssetPtr = mediaAsset->mediaAsset_->GetFileAssetInstance();
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    MediaLibrary_ImageFileType imageFileType = MEDIA_LIBRARY_IMAGE_JPEG;
    AssetChangeOperation changeOperation = AssetChangeOperation::CREATE_FROM_SCRATCH;
    changeRequest->request_->RecordChangeOperation(changeOperation);
    uint32_t result = OH_MediaAssetChangeRequest_SaveCameraPhoto(changeRequest, imageFileType);
    EXPECT_EQ(result, MEDIA_LIBRARY_OK);
    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_011
 * @tc.desc: OH_MediaAssetChangeRequest_SaveCameraPhoto changeOperation is GET_WRITE_CACHE_HANDLER
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_011, TestSize.Level0)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto fileAssetPtr = mediaAsset->mediaAsset_->GetFileAssetInstance();
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    MediaLibrary_ImageFileType imageFileType = MEDIA_LIBRARY_IMAGE_JPEG;
    AssetChangeOperation changeOperation = AssetChangeOperation::GET_WRITE_CACHE_HANDLER;
    changeRequest->request_->RecordChangeOperation(changeOperation);
    uint32_t result = OH_MediaAssetChangeRequest_SaveCameraPhoto(changeRequest, imageFileType);
    EXPECT_EQ(result, MEDIA_LIBRARY_OK);
    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_PARAMETER_ERROR);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_012
 * @tc.desc: OH_MediaAssetChangeRequest_SaveCameraPhoto changeOperation is ADD_RESOURCE
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_012, TestSize.Level0)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto fileAssetPtr = mediaAsset->mediaAsset_->GetFileAssetInstance();
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    MediaLibrary_ImageFileType imageFileType = MEDIA_LIBRARY_IMAGE_JPEG;
    AssetChangeOperation changeOperation = AssetChangeOperation::ADD_RESOURCE;
    changeRequest->request_->RecordChangeOperation(changeOperation);
    uint32_t result = OH_MediaAssetChangeRequest_SaveCameraPhoto(changeRequest, imageFileType);
    EXPECT_EQ(result, MEDIA_LIBRARY_OK);
    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_PARAMETER_ERROR);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_013
 * @tc.desc: OH_MediaAssetChangeRequest_DiscardCameraPhoto changeOperation is SET_EDIT_DATA
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_013, TestSize.Level0)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto fileAssetPtr = mediaAsset->mediaAsset_->GetFileAssetInstance();
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    AssetChangeOperation changeOperation = AssetChangeOperation::SET_EDIT_DATA;
    changeRequest->request_->RecordChangeOperation(changeOperation);
    uint32_t result = OH_MediaAssetChangeRequest_DiscardCameraPhoto(changeRequest);
    EXPECT_EQ(result, MEDIA_LIBRARY_OK);
    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_PARAMETER_ERROR);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_014
 * @tc.desc: OH_MediaAssetChangeRequest_DiscardCameraPhoto changeOperation is ADD_FILTERS
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_014, TestSize.Level0)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto fileAssetPtr = mediaAsset->mediaAsset_->GetFileAssetInstance();
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    AssetChangeOperation changeOperation = AssetChangeOperation::ADD_FILTERS;
    changeRequest->request_->RecordChangeOperation(changeOperation);
    uint32_t result = OH_MediaAssetChangeRequest_DiscardCameraPhoto(changeRequest);
    EXPECT_EQ(result, MEDIA_LIBRARY_OK);
    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_PARAMETER_ERROR);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_015
 * @tc.desc: OH_MediaAssetChangeRequest_DiscardCameraPhoto changeOperation is CREATE_FROM_SCRATCH
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_015, TestSize.Level0)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto fileAssetPtr = mediaAsset->mediaAsset_->GetFileAssetInstance();
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    AssetChangeOperation changeOperation = AssetChangeOperation::CREATE_FROM_SCRATCH;
    changeRequest->request_->RecordChangeOperation(changeOperation);
    uint32_t result = OH_MediaAssetChangeRequest_DiscardCameraPhoto(changeRequest);
    EXPECT_EQ(result, MEDIA_LIBRARY_OK);
    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_PARAMETER_ERROR);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_016
 * @tc.desc: OH_MediaAssetChangeRequest_DiscardCameraPhoto changeOperation is GET_WRITE_CACHE_HANDLER
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_016, TestSize.Level0)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto fileAssetPtr = mediaAsset->mediaAsset_->GetFileAssetInstance();
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    AssetChangeOperation changeOperation = AssetChangeOperation::GET_WRITE_CACHE_HANDLER;
    changeRequest->request_->RecordChangeOperation(changeOperation);
    uint32_t result = OH_MediaAssetChangeRequest_DiscardCameraPhoto(changeRequest);
    EXPECT_EQ(result, MEDIA_LIBRARY_OK);
    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_PARAMETER_ERROR);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_017
 * @tc.desc: OH_MediaAssetChangeRequest_DiscardCameraPhoto changeOperation is ADD_RESOURCE
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_017, TestSize.Level0)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto fileAssetPtr = mediaAsset->mediaAsset_->GetFileAssetInstance();
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    AssetChangeOperation changeOperation = AssetChangeOperation::ADD_RESOURCE;
    changeRequest->request_->RecordChangeOperation(changeOperation);
    uint32_t result = OH_MediaAssetChangeRequest_DiscardCameraPhoto(changeRequest);
    EXPECT_EQ(result, MEDIA_LIBRARY_OK);
    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_PARAMETER_ERROR);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_018
 * @tc.desc: OH_MediaAssetChangeRequest_DiscardCameraPhoto changeRequest is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_018, TestSize.Level0)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_MEDIALIBRARY);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto fileAssetPtr = mediaAsset->mediaAsset_->GetFileAssetInstance();
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    EXPECT_EQ(changeRequest, nullptr);

    uint32_t result = OH_MediaAssetChangeRequest_DiscardCameraPhoto(changeRequest);
    EXPECT_EQ(result, MEDIA_LIBRARY_PARAMETER_ERROR);
    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_PARAMETER_ERROR);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_019
 * @tc.desc: OH_MediaAccessHelper_ApplyChanges changeRequest is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_019, TestSize.Level0)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_MEDIALIBRARY);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto fileAssetPtr = mediaAsset->mediaAsset_->GetFileAssetInstance();
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    EXPECT_EQ(changeRequest, nullptr);

    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_PARAMETER_ERROR);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_020
 * @tc.desc: OH_MediaAccessHelper_ApplyChanges changeOperation is CREATE_FROM_SCRATCH
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_020, TestSize.Level0)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto fileAssetPtr = mediaAsset->mediaAsset_->GetFileAssetInstance();
    AssetChangeOperation changeOperation = AssetChangeOperation::CREATE_FROM_SCRATCH;
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    changeRequest->request_->RecordChangeOperation(changeOperation);
    ASSERT_NE(changeRequest, nullptr);

    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_PARAMETER_ERROR);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_021
 * @tc.desc: OH_MediaAccessHelper_ApplyChanges changeOperation is SET_EDIT_DATA
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_021, TestSize.Level0)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto fileAssetPtr = mediaAsset->mediaAsset_->GetFileAssetInstance();
    AssetChangeOperation changeOperation = AssetChangeOperation::SET_EDIT_DATA;
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    changeRequest->request_->RecordChangeOperation(changeOperation);
    ASSERT_NE(changeRequest, nullptr);

    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_PARAMETER_ERROR);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_022
 * @tc.desc: OH_MediaAccessHelper_ApplyChanges changeOperation is CREATE_FROM_URI
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_022, TestSize.Level0)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto fileAssetPtr = mediaAsset->mediaAsset_->GetFileAssetInstance();
    AssetChangeOperation changeOperation = AssetChangeOperation::CREATE_FROM_URI;
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    changeRequest->request_->RecordChangeOperation(changeOperation);
    ASSERT_NE(changeRequest, nullptr);

    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_023
 * @tc.desc: OH_MediaAssetChangeRequest_GetWriteCacheHandler changeOperation is CREATE_FROM_URI
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_023, TestSize.Level0)
{
    string srcDisplayName = "request_image_src.jpg";
    string destDisplayName = "request_image_dest.jpg";
    string srcuri = mediaLibraryManager->CreateAsset(srcDisplayName);
    int32_t srcFd = mediaLibraryManager->OpenAsset(srcuri, MEDIA_FILEMODE_READWRITE);
    mediaLibraryManager->CloseAsset(srcuri, srcFd);

    string destUri = ROOT_TEST_MEDIA_DIR + destDisplayName;
    EXPECT_NE(destUri, "");
    MEDIA_INFO_LOG("createFile uri: %{public}s", destUri.c_str());
    sleep(SCAN_WAIT_TIME_1S);

    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::CAMERA));
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto fileAssetPtr = mediaAsset->mediaAsset_->GetFileAssetInstance();
    AssetChangeOperation changeOperation = AssetChangeOperation::CREATE_FROM_URI;
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    changeRequest->request_->RecordChangeOperation(changeOperation);
    ASSERT_NE(changeRequest, nullptr);

    int32_t fd = 0;
    uint32_t resultChange = OH_MediaAssetChangeRequest_GetWriteCacheHandler(changeRequest, &fd);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED);
    resultChange = OH_MediaAssetChangeRequest_GetWriteCacheHandler(nullptr, &fd);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_PARAMETER_ERROR);
    changeRequest->request_ = nullptr;
    resultChange = OH_MediaAssetChangeRequest_GetWriteCacheHandler(changeRequest, &fd);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED);
    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}


/**
 * @tc.name: media_library_capi_test_024
 * @tc.desc: OH_MediaAssetChangeRequest_AddResourceWithUri resourceType is MEDIA_LIBRARY_VIDEO_RESOURCE
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_024, TestSize.Level0)
{
    string srcDisplayName = "request_image_src.jpg";
    string destDisplayName = "request_image_dest.jpg";
    string srcuri = mediaLibraryManager->CreateAsset(srcDisplayName);
    int32_t srcFd = mediaLibraryManager->OpenAsset(srcuri, MEDIA_FILEMODE_READWRITE);
    mediaLibraryManager->CloseAsset(srcuri, srcFd);

    string destUri = ROOT_TEST_MEDIA_DIR + destDisplayName;
    EXPECT_NE(destUri, "");
    MEDIA_INFO_LOG("createFile uri: %{public}s", destUri.c_str());
    sleep(SCAN_WAIT_TIME_1S);

    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    MediaLibrary_ResourceType resourceType = MediaLibrary_ResourceType::MEDIA_LIBRARY_VIDEO_RESOURCE;
    fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto fileAssetPtr = mediaAsset->mediaAsset_->GetFileAssetInstance();
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    char* fileUri = strdup(destUri.c_str());
    uint32_t result = OH_MediaAssetChangeRequest_AddResourceWithUri(changeRequest, resourceType, fileUri);
    EXPECT_EQ(result, MEDIA_LIBRARY_NO_SUCH_FILE);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_025
 * @tc.desc: OH_MediaAssetChangeRequest_AddResourceWithUri resourceType is MEDIA_LIBRARY_IMAGE_RESOURCE
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_025, TestSize.Level0)
{
    string srcDisplayName = "request_image_src.jpg";
    string destDisplayName = "request_image_dest.jpg";
    string srcuri = mediaLibraryManager->CreateAsset(srcDisplayName);
    int32_t srcFd = mediaLibraryManager->OpenAsset(srcuri, MEDIA_FILEMODE_READWRITE);
    mediaLibraryManager->CloseAsset(srcuri, srcFd);

    string destUri = ROOT_TEST_MEDIA_DIR + destDisplayName;
    EXPECT_NE(destUri, "");
    MEDIA_INFO_LOG("createFile uri: %{public}s", destUri.c_str());
    sleep(SCAN_WAIT_TIME_1S);

    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    MediaLibrary_ResourceType resourceType = MediaLibrary_ResourceType::MEDIA_LIBRARY_IMAGE_RESOURCE;
    fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto fileAssetPtr = mediaAsset->mediaAsset_->GetFileAssetInstance();
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    char* fileUri = strdup(destUri.c_str());
    uint32_t result = OH_MediaAssetChangeRequest_AddResourceWithUri(changeRequest, resourceType, fileUri);
    EXPECT_EQ(result, MEDIA_LIBRARY_NO_SUCH_FILE);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_026
 * @tc.desc: OH_MediaAssetChangeRequest_AddResourceWithUri invalid file type
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_026, TestSize.Level0)
{
    string srcDisplayName = "request_image_src.jpg";
    string destDisplayName = "request_image_dest.jpg";
    string srcuri = mediaLibraryManager->CreateAsset(srcDisplayName);
    int32_t srcFd = mediaLibraryManager->OpenAsset(srcuri, MEDIA_FILEMODE_READWRITE);
    mediaLibraryManager->CloseAsset(srcuri, srcFd);

    string destUri = ROOT_TEST_MEDIA_DIR + destDisplayName;
    EXPECT_NE(destUri, "");
    MEDIA_INFO_LOG("createFile uri: %{public}s", destUri.c_str());
    sleep(SCAN_WAIT_TIME_1S);

    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    MediaLibrary_ResourceType resourceType = MediaLibrary_ResourceType::MEDIA_LIBRARY_VIDEO_RESOURCE;
    fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_VIDEO);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto fileAssetPtr = mediaAsset->mediaAsset_->GetFileAssetInstance();
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    char* fileUri = strdup(destUri.c_str());
    uint32_t result = OH_MediaAssetChangeRequest_AddResourceWithUri(nullptr, resourceType, fileUri);
    EXPECT_EQ(result, MEDIA_LIBRARY_PARAMETER_ERROR);
    result = OH_MediaAssetChangeRequest_AddResourceWithUri(changeRequest, resourceType, nullptr);
    EXPECT_EQ(result, MEDIA_LIBRARY_PARAMETER_ERROR);
    changeRequest->request_ = nullptr;
    result = OH_MediaAssetChangeRequest_AddResourceWithUri(changeRequest, resourceType, fileUri);
    EXPECT_EQ(result, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_027
 * @tc.desc: OH_MediaAssetChangeRequest_AddResourceWithUri GetPhotoSubType is CAMERA
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_027, TestSize.Level0)
{
    string srcDisplayName = "request_image_src.jpg";
    string destDisplayName = "request_image_dest.jpg";
    string srcuri = mediaLibraryManager->CreateAsset(srcDisplayName);
    int32_t srcFd = mediaLibraryManager->OpenAsset(srcuri, MEDIA_FILEMODE_READWRITE);
    mediaLibraryManager->CloseAsset(srcuri, srcFd);

    string destUri = ROOT_TEST_MEDIA_DIR + destDisplayName;
    EXPECT_NE(destUri, "");
    MEDIA_INFO_LOG("createFile uri: %{public}s", destUri.c_str());
    sleep(SCAN_WAIT_TIME_1S);

    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    MediaLibrary_ResourceType resourceType = MediaLibrary_ResourceType::MEDIA_LIBRARY_IMAGE_RESOURCE;
    fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::CAMERA));
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto fileAssetPtr = mediaAsset->mediaAsset_->GetFileAssetInstance();
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    char* fileUri = strdup(destUri.c_str());
    uint32_t result = OH_MediaAssetChangeRequest_AddResourceWithUri(changeRequest, resourceType, fileUri);
    EXPECT_EQ(result, MEDIA_LIBRARY_NO_SUCH_FILE);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}
} // namespace Media
} // namespace OHOS
