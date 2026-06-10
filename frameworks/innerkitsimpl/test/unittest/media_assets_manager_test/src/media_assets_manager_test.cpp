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

#define MLOG_TAG "MediaAssetsManagerTest"

#include "media_assets_manager_test.h"

#include "media_assets_service.h"
#include "media_assets_rdb_operations.h"
#include "medialibrary_errno.h"
#include "media_log.h"

// DTO headers
#include "form_info_dto.h"
#include "commit_edited_asset_dto.h"
#include "clone_asset_dto.h"
#include "revert_to_original_dto.h"
#include "cloud_enhancement_dto.h"
#include "create_asset_dto.h"
#include "grant_photo_uri_permission_dto.h"
#include "grant_photo_uris_permission_dto.h"
#include "grant_photo_uri_permission_inner_dto.h"
#include "cancel_photo_uri_permission_dto.h"
#include "cancel_photo_uri_permission_inner_dto.h"
#include "check_photo_uri_permission_inner_dto.h"
#include "save_camera_photo_dto.h"
#include "open_asset_compress_dto.h"
#include "create_tmp_compatible_dup_dto.h"
#include "set_location_dto.h"
#include "get_uris_by_old_uris_inner_dto.h"
#include "change_request_move_assets_to_dir_dto.h"
#include "change_request_move_assets_by_path_dto.h"
#include "query_cloud_enhancement_task_state_dto.h"
#include "start_thumbnail_creation_task_dto.h"
#include "stop_thumbnail_creation_task_dto.h"
#include "restore_dto.h"
#include "convert_format_dto.h"
#include "submit_cache_dto.h"
#include "add_image_dto.h"
#include "asset_cancel_task_dto.h"  // defines CancelTaskDto
#include "get_assets_dto.h"
#include "asset_change_create_asset_dto.h"
#include "is_edited_dto.h"
#include "request_edit_data_dto.h"
#include "get_edit_data_dto.h"
#include "start_asset_change_scan_dto.h"
#include "get_cloud_enhancement_pair_dto.h"

// VO headers
#include "check_photo_uris_read_permission_vo.h"
#include "compatible_info_vo.h"
#include "preferred_compatible_mode_check_utils.h"

namespace OHOS {
namespace Media {
using namespace testing::ext;

void MediaAssetsManagerTest::SetUpTestCase() {}
void MediaAssetsManagerTest::TearDownTestCase() {}
void MediaAssetsManagerTest::SetUp() {}
void MediaAssetsManagerTest::TearDown() {}

// ======================================================================
// RDB Operations - Data Structure Tests
// ======================================================================

HWTEST_F(MediaAssetsManagerTest, RdbOps_PhotoAssetReadState_Defaults_001, TestSize.Level1)
{
    PhotoAssetReadState state;
    EXPECT_FALSE(state.isHidden);
    EXPECT_FALSE(state.isTrashed);
}

HWTEST_F(MediaAssetsManagerTest, RdbOps_PhotoAssetReadState_SetValues_002, TestSize.Level1)
{
    PhotoAssetReadState state;
    state.isHidden = true;
    state.isTrashed = true;
    EXPECT_TRUE(state.isHidden);
    EXPECT_TRUE(state.isTrashed);
}

HWTEST_F(MediaAssetsManagerTest, RdbOps_Constants_Values_003, TestSize.Level1)
{
    constexpr int32_t PHOTO_HIDDEN_FLAG = 1;
    constexpr int32_t POSITION_CLOUD_FLAG = 2;
    constexpr int32_t CLOUD_COPY_DIRTY_FLAG = 7;
    EXPECT_EQ(PHOTO_HIDDEN_FLAG, 1);
    EXPECT_EQ(POSITION_CLOUD_FLAG, 2);
    EXPECT_EQ(CLOUD_COPY_DIRTY_FLAG, 7);
}

HWTEST_F(MediaAssetsManagerTest, RdbOps_QueryPhotoAssetsReadState_EmptyFileIds_004, TestSize.Level1)
{
    MediaAssetsRdbOperations ops;
    std::vector<std::string> emptyIds;
    std::vector<std::string> validFileIds;
    int32_t ret = ops.QueryPhotoAssetsReadState(emptyIds, validFileIds);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(validFileIds.empty());
}

HWTEST_F(MediaAssetsManagerTest, RdbOps_QueryPhotoAssetsReadState_ClearsOutput_005, TestSize.Level1)
{
    MediaAssetsRdbOperations ops;
    std::vector<std::string> emptyIds;
    std::vector<std::string> validFileIds = {"pre_existing"};
    int32_t ret = ops.QueryPhotoAssetsReadState(emptyIds, validFileIds);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(validFileIds.empty());
}

HWTEST_F(MediaAssetsManagerTest, RdbOps_QueryPhotoAssetsReadState_MultiplePreExisting_006, TestSize.Level1)
{
    MediaAssetsRdbOperations ops;
    std::vector<std::string> emptyIds;
    std::vector<std::string> validFileIds = {"a", "b", "c"};
    int32_t ret = ops.QueryPhotoAssetsReadState(emptyIds, validFileIds);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(validFileIds.empty());
}

// ======================================================================
// Service - Singleton Test
// ======================================================================

HWTEST_F(MediaAssetsManagerTest, Service_GetInstance_SameReference_001, TestSize.Level1)
{
    auto &inst1 = MediaAssetsService::GetInstance();
    auto &inst2 = MediaAssetsService::GetInstance();
    EXPECT_EQ(&inst1, &inst2);
}

// ======================================================================
// Service - Task Cancellation Tests
// ======================================================================

HWTEST_F(MediaAssetsManagerTest, Service_RegisterTaskCancelFlag_NewId_002, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    auto flag = std::make_shared<std::atomic<bool>>(false);
    bool ret = svc.RegisterTaskCancelFlag(90001, flag);
    EXPECT_TRUE(ret);
    svc.EarseTaskCancelFlag(90001);
}

HWTEST_F(MediaAssetsManagerTest, Service_RegisterTaskCancelFlag_DuplicateId_003, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    auto flag = std::make_shared<std::atomic<bool>>(false);
    EXPECT_TRUE(svc.RegisterTaskCancelFlag(90002, flag));
    EXPECT_FALSE(svc.RegisterTaskCancelFlag(90002, flag));
    svc.EarseTaskCancelFlag(90002);
}

HWTEST_F(MediaAssetsManagerTest, Service_EarseTaskCancelFlag_Existing_004, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    auto flag = std::make_shared<std::atomic<bool>>(false);
    svc.RegisterTaskCancelFlag(90003, flag);
    EXPECT_TRUE(svc.EarseTaskCancelFlag(90003));
}

HWTEST_F(MediaAssetsManagerTest, Service_EarseTaskCancelFlag_NonExisting_005, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    EXPECT_FALSE(svc.EarseTaskCancelFlag(99999));
}

HWTEST_F(MediaAssetsManagerTest, Service_CancelTask_ExistingFlag_006, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    auto flag = std::make_shared<std::atomic<bool>>(false);
    svc.RegisterTaskCancelFlag(90004, flag);
    int32_t ret = svc.CancelTask(90004);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(flag->load());
    svc.EarseTaskCancelFlag(90004);
}

HWTEST_F(MediaAssetsManagerTest, Service_CancelTask_NonExistingId_007, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    int32_t ret = svc.CancelTask(99998);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaAssetsManagerTest, Service_CancelTask_FlagNotChanged_008, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    auto flag = std::make_shared<std::atomic<bool>>(false);
    svc.RegisterTaskCancelFlag(90005, flag);
    svc.CancelTask(99997); // different id
    EXPECT_FALSE(flag->load());
    svc.EarseTaskCancelFlag(90005);
}

HWTEST_F(MediaAssetsManagerTest, Service_TaskCancel_RegisterEraseReRegister_009, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    auto flag = std::make_shared<std::atomic<bool>>(false);
    EXPECT_TRUE(svc.RegisterTaskCancelFlag(90010, flag));
    EXPECT_TRUE(svc.EarseTaskCancelFlag(90010));
    // After erase, should be able to re-register same id
    EXPECT_TRUE(svc.RegisterTaskCancelFlag(90010, flag));
    svc.EarseTaskCancelFlag(90010);
}

HWTEST_F(MediaAssetsManagerTest, Service_CancelTask_NullFlag_010, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    std::shared_ptr<std::atomic<bool>> nullFlag;
    EXPECT_TRUE(svc.RegisterTaskCancelFlag(90011, nullFlag));
    int32_t ret = svc.CancelTask(90011);
    EXPECT_EQ(ret, E_OK);
    svc.EarseTaskCancelFlag(90011);
}

HWTEST_F(MediaAssetsManagerTest, Service_EarseTaskCancelFlag_AlreadyErased_011, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    auto flag = std::make_shared<std::atomic<bool>>(false);
    svc.RegisterTaskCancelFlag(90012, flag);
    EXPECT_TRUE(svc.EarseTaskCancelFlag(90012));
    EXPECT_FALSE(svc.EarseTaskCancelFlag(90012));
}

// ======================================================================
// Service - Early Return Validation Tests
// ======================================================================

HWTEST_F(MediaAssetsManagerTest, Service_CheckPhotoUrisReadPermission_EmptyUris_012, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    CheckPhotoUrisReadPermissionReqBody req;
    CheckPhotoUrisReadPermissionRespBody resp;
    int32_t ret = svc.CheckPhotoUrisReadPermission(req, resp);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(resp.uriPermissionStateMap.empty());
}

HWTEST_F(MediaAssetsManagerTest, Service_CheckPhotoUrisReadPermission_ClearsPrevious_013, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    CheckPhotoUrisReadPermissionReqBody req;
    CheckPhotoUrisReadPermissionRespBody resp;
    resp.uriPermissionStateMap["test"] = 1;
    int32_t ret = svc.CheckPhotoUrisReadPermission(req, resp);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(resp.uriPermissionStateMap.empty());
}

HWTEST_F(MediaAssetsManagerTest, Service_AddAssetVisitCount_ReturnsOk_014, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    int32_t ret = svc.AddAssetVisitCount(1, 0);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaAssetsManagerTest, Service_CancelRequest_ReturnsOk_015, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    int32_t ret = svc.CancelRequest("photo_123", 0);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaAssetsManagerTest, Service_CheckMimeType_ZeroFileId_016, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    EXPECT_FALSE(svc.CheckMimeType(0));
}

HWTEST_F(MediaAssetsManagerTest, Service_CheckMimeType_NegativeFileId_017, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    EXPECT_FALSE(svc.CheckMimeType(-1));
}

HWTEST_F(MediaAssetsManagerTest, Service_CheckMimeType_PositiveFileId_018, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    // Positive fileId passes validation but DB not available, returns false
    EXPECT_FALSE(svc.CheckMimeType(100));
}

HWTEST_F(MediaAssetsManagerTest, Service_SetPreferredCompatibleMode_EmptyBundle_019, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    int32_t ret = svc.SetPreferredCompatibleMode("", 0);
    EXPECT_EQ(ret, E_INVALID_ARGUMENTS);
}

HWTEST_F(MediaAssetsManagerTest, Service_SetPreferredCompatibleMode_InvalidMode_020, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    int32_t ret = svc.SetPreferredCompatibleMode("com.test.app", 99);
    EXPECT_EQ(ret, E_INVALID_ARGUMENTS);
}

HWTEST_F(MediaAssetsManagerTest, Service_SetPreferredCompatibleMode_NegativeMode_021, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    int32_t ret = svc.SetPreferredCompatibleMode("com.test.app", -1);
    EXPECT_EQ(ret, E_INVALID_ARGUMENTS);
}

HWTEST_F(MediaAssetsManagerTest, Service_SetPreferredCompatibleMode_ValidDefault_022, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    int32_t ret = svc.SetPreferredCompatibleMode("com.test.app", 0);
    EXPECT_EQ(ret, E_SUCCESS);
}

HWTEST_F(MediaAssetsManagerTest, Service_SetPreferredCompatibleMode_ValidCurrent_023, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    int32_t ret = svc.SetPreferredCompatibleMode("com.test.app", 1);
    EXPECT_EQ(ret, E_SUCCESS);
}

HWTEST_F(MediaAssetsManagerTest, Service_SetPreferredCompatibleMode_ValidCompatible_024, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    int32_t ret = svc.SetPreferredCompatibleMode("com.test.app", 2);
    EXPECT_EQ(ret, E_SUCCESS);
}

HWTEST_F(MediaAssetsManagerTest, Service_GetPreferredCompatibleMode_EmptyBundle_025, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    int32_t mode = 0;
    int32_t ret = svc.GetPreferredCompatibleMode("", mode);
    EXPECT_EQ(ret, E_INVALID_ARGUMENTS);
}

HWTEST_F(MediaAssetsManagerTest, Service_GrantPhotoUriPermissionInner_SizeMismatch_026, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    GrantUriPermissionInnerDto dto;
    dto.fileIds = {"1", "2"};
    dto.uriTypes = {1};
    dto.permissionTypes = {1, 2};
    int32_t ret = svc.GrantPhotoUriPermissionInner(dto);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(MediaAssetsManagerTest, Service_GrantPhotoUriPermissionInner_AllEmpty_027, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    GrantUriPermissionInnerDto dto;
    int32_t ret = svc.GrantPhotoUriPermissionInner(dto);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaAssetsManagerTest, Service_GrantPhotoUriPermissionInner_AllMatchSize_028, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    GrantUriPermissionInnerDto dto;
    dto.fileIds = {"1", "2"};
    dto.uriTypes = {1, 2};
    dto.permissionTypes = {1, 2};
    int32_t ret = svc.GrantPhotoUriPermissionInner(dto);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaAssetsManagerTest, Service_CancelPhotoUriPermissionInner_SizeMismatch_029, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    CancelUriPermissionInnerDto dto;
    dto.fileIds = {"1"};
    dto.uriTypes = {1, 2};
    dto.permissionTypes = {{"read"}};
    int32_t ret = svc.CancelPhotoUriPermissionInner(dto);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(MediaAssetsManagerTest, Service_CancelPhotoUriPermissionInner_AllMatch_030, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    CancelUriPermissionInnerDto dto;
    dto.fileIds = {"1"};
    dto.uriTypes = {1};
    dto.permissionTypes = {{"read"}};
    int32_t ret = svc.CancelPhotoUriPermissionInner(dto);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaAssetsManagerTest, Service_QueryMediaDataStatus_InvalidKey_031, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    bool result = true;
    int32_t ret = svc.QueryMediaDataStatus("invalid_key", result);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(MediaAssetsManagerTest, Service_QueryMediaDataStatus_DateAddedYear_032, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    bool result = true;
    int32_t ret = svc.QueryMediaDataStatus("date_added_year", result);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(MediaAssetsManagerTest, Service_CheckSinglePhotoPermission_EmptyFileId_033, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    int32_t ret = svc.CheckSinglePhotoPermission("", 0);
    EXPECT_EQ(ret, E_INVALID_FILEID);
}

HWTEST_F(MediaAssetsManagerTest, Service_CheckSinglePhotoPermission_NonNumeric_034, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    int32_t ret = svc.CheckSinglePhotoPermission("abc", 0);
    EXPECT_EQ(ret, E_INVALID_FILEID);
}

HWTEST_F(MediaAssetsManagerTest, Service_CheckSinglePhotoPermission_PartialNumeric_035, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    int32_t ret = svc.CheckSinglePhotoPermission("123abc", 0);
    EXPECT_EQ(ret, E_INVALID_FILEID);
}

HWTEST_F(MediaAssetsManagerTest, Service_CheckSinglePhotoPermission_ValidNumeric_036, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    int32_t ret = svc.CheckSinglePhotoPermission("12345", 0);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaAssetsManagerTest, Service_CheckSinglePhotoPermission_Zero_037, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    int32_t ret = svc.CheckSinglePhotoPermission("0", 0);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaAssetsManagerTest, Service_CheckSinglePhotoPermission_Negative_038, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    int32_t ret = svc.CheckSinglePhotoPermission("-1", 0);
    EXPECT_EQ(ret, E_OK);
}

// --- CreateTmpCompatibleDup validation ---

HWTEST_F(MediaAssetsManagerTest, Service_CreateTmpCompatibleDup_ZeroFileId_039, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    CreateTmpCompatibleDupDto dto;
    dto.fileId = 0;
    dto.path = "/storage/test.jpg";
    int32_t ret = svc.CreateTmpCompatibleDup(dto);
    EXPECT_EQ(ret, E_INNER_FAIL);
}

HWTEST_F(MediaAssetsManagerTest, Service_CreateTmpCompatibleDup_EmptyPath_040, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    CreateTmpCompatibleDupDto dto;
    dto.fileId = 1;
    dto.path = "";
    int32_t ret = svc.CreateTmpCompatibleDup(dto);
    EXPECT_EQ(ret, E_INNER_FAIL);
}

HWTEST_F(MediaAssetsManagerTest, Service_CreateTmpCompatibleDup_NegativeFileId_041, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    CreateTmpCompatibleDupDto dto;
    dto.fileId = -1;
    dto.path = "/storage/test.jpg";
    int32_t ret = svc.CreateTmpCompatibleDup(dto);
    EXPECT_EQ(ret, E_INNER_FAIL);
}

HWTEST_F(MediaAssetsManagerTest, Service_CreateTmpCompatibleDup_Valid_042, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    CreateTmpCompatibleDupDto dto;
    dto.fileId = 1;
    dto.path = "/storage/test.jpg";
    int32_t ret = svc.CreateTmpCompatibleDup(dto);
    EXPECT_EQ(ret, E_OK);
}

// --- SetCompatibleInfo MIME type filtering ---

HWTEST_F(MediaAssetsManagerTest, Service_SetCompatibleInfo_EmptyEncodings_043, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    CompatibleInfo info;
    info.bundleName = "com.test";
    int32_t ret = svc.SetCompatibleInfo(info);
    EXPECT_EQ(ret, E_SUCCESS);
}

HWTEST_F(MediaAssetsManagerTest, Service_SetCompatibleInfo_ValidMimeTypes_044, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    CompatibleInfo info;
    info.bundleName = "com.test";
    info.encodings = {"image/heic", "image/jpeg"};
    int32_t ret = svc.SetCompatibleInfo(info);
    EXPECT_EQ(ret, E_SUCCESS);
}

HWTEST_F(MediaAssetsManagerTest, Service_SetCompatibleInfo_UnsupportedMimeTypes_045, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    CompatibleInfo info;
    info.bundleName = "com.test";
    info.encodings = {"image/png", "image/gif", "image/bmp"};
    // All filtered out, map is empty (0 <= 2)
    int32_t ret = svc.SetCompatibleInfo(info);
    EXPECT_EQ(ret, E_SUCCESS);
}

HWTEST_F(MediaAssetsManagerTest, Service_SetCompatibleInfo_DuplicateValidTypes_046, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    CompatibleInfo info;
    info.bundleName = "com.test";
    info.encodings = {"image/heic", "image/heic", "image/jpeg", "image/jpeg"};
    // Deduplicated to 2, which is <= MAX
    int32_t ret = svc.SetCompatibleInfo(info);
    EXPECT_EQ(ret, E_SUCCESS);
}

// --- FormInfo / RemoveForm / CommitEdit / Revert / Clone validation ---

HWTEST_F(MediaAssetsManagerTest, Service_SaveFormInfo_EmptyFormIds_047, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    FormInfoDto dto;
    int32_t ret = svc.SaveFormInfo(dto);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(MediaAssetsManagerTest, Service_RemoveFormInfo_EmptyFormId_048, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    int32_t ret = svc.RemoveFormInfo("");
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(MediaAssetsManagerTest, Service_RemoveFormInfo_ValidFormId_049, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    int32_t ret = svc.RemoveFormInfo("form_123");
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaAssetsManagerTest, Service_CommitEditedAsset_ZeroFileId_050, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    CommitEditedAssetDto dto;
    dto.fileId = 0;
    int32_t ret = svc.CommitEditedAsset(dto);
    EXPECT_EQ(ret, E_INVALID_VALUES);
}

HWTEST_F(MediaAssetsManagerTest, Service_CommitEditedAsset_ValidFileId_051, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    CommitEditedAssetDto dto;
    dto.fileId = 42;
    int32_t ret = svc.CommitEditedAsset(dto);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaAssetsManagerTest, Service_RevertToOriginal_ZeroFileId_052, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    RevertToOriginalDto dto;
    dto.fileId = 0;
    int32_t ret = svc.RevertToOriginal(dto);
    EXPECT_EQ(ret, E_INVALID_VALUES);
}

HWTEST_F(MediaAssetsManagerTest, Service_RevertToOriginal_ValidFileId_053, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    RevertToOriginalDto dto;
    dto.fileId = 10;
    int32_t ret = svc.RevertToOriginal(dto);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaAssetsManagerTest, Service_CloneAsset_ZeroFileId_054, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    CloneAssetDto dto;
    dto.fileId = 0;
    int32_t ret = svc.CloneAsset(dto);
    EXPECT_EQ(ret, E_INVALID_VALUES);
}

HWTEST_F(MediaAssetsManagerTest, Service_CloneAsset_ValidFileId_055, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    CloneAssetDto dto;
    dto.fileId = 5;
    dto.title = "clone_test";
    int32_t ret = svc.CloneAsset(dto);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaAssetsManagerTest, Service_SubmitCloudEnhancement_EmptyUris_056, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    CloudEnhancementDto dto;
    int32_t ret = svc.SubmitCloudEnhancementTasks(dto);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(MediaAssetsManagerTest, Service_CancelCloudEnhancement_EmptyUris_057, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    CloudEnhancementDto dto;
    int32_t ret = svc.CancelCloudEnhancementTasks(dto);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(MediaAssetsManagerTest, Service_CancelAllCloudEnhancement_ReturnsErr_058, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    int32_t ret = svc.CancelAllCloudEnhancementTasks();
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(MediaAssetsManagerTest, Service_SetAssetTitle_ZeroFileId_059, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    int32_t ret = svc.SetAssetTitle(0, "test");
    EXPECT_EQ(ret, E_INVALID_VALUES);
}

HWTEST_F(MediaAssetsManagerTest, Service_SetAssetTitle_ValidFileId_060, TestSize.Level1)
{
    auto &svc = MediaAssetsService::GetInstance();
    int32_t ret = svc.SetAssetTitle(1, "test");
    EXPECT_EQ(ret, E_OK);
}

// ======================================================================
// DTO Default Value Tests
// ======================================================================

HWTEST_F(MediaAssetsManagerTest, DTO_FormInfoDto_Defaults_061, TestSize.Level1)
{
    FormInfoDto dto;
    EXPECT_TRUE(dto.formIds.empty());
    EXPECT_TRUE(dto.fileUris.empty());
}

HWTEST_F(MediaAssetsManagerTest, DTO_CommitEditedAssetDto_Defaults_062, TestSize.Level1)
{
    CommitEditedAssetDto dto;
    EXPECT_TRUE(dto.editData.empty());
}

HWTEST_F(MediaAssetsManagerTest, DTO_CloneAssetDto_Defaults_063, TestSize.Level1)
{
    CloneAssetDto dto;
    EXPECT_TRUE(dto.title.empty());
}

HWTEST_F(MediaAssetsManagerTest, DTO_CloneAssetDto_ToString_064, TestSize.Level1)
{
    CloneAssetDto dto;
    dto.fileId = 42;
    dto.title = "test_clone";
    std::string s = dto.ToString();
    EXPECT_FALSE(s.empty());
    EXPECT_NE(s.find("42"), std::string::npos);
    EXPECT_NE(s.find("test_clone"), std::string::npos);
}

HWTEST_F(MediaAssetsManagerTest, DTO_RevertToOriginalDto_Defaults_065, TestSize.Level1)
{
    RevertToOriginalDto dto;
    EXPECT_TRUE(dto.fileUri.empty());
}

HWTEST_F(MediaAssetsManagerTest, DTO_RevertToOriginalDto_ToString_066, TestSize.Level1)
{
    RevertToOriginalDto dto;
    dto.fileId = 100;
    dto.fileUri = "file://test";
    std::string s = dto.ToString();
    EXPECT_NE(s.find("100"), std::string::npos);
    EXPECT_NE(s.find("file://test"), std::string::npos);
}

HWTEST_F(MediaAssetsManagerTest, DTO_CloudEnhancementDto_Defaults_067, TestSize.Level1)
{
    CloudEnhancementDto dto;
    EXPECT_EQ(dto.triggerMode, -1);
    EXPECT_TRUE(dto.fileUris.empty());
}

HWTEST_F(MediaAssetsManagerTest, DTO_CreateAssetDto_Defaults_068, TestSize.Level1)
{
    CreateAssetDto dto;
    EXPECT_EQ(dto.tokenId, 0);
    EXPECT_EQ(dto.mediaType, 0);
    EXPECT_EQ(dto.photoSubtype, 0);
    EXPECT_TRUE(dto.title.empty());
    EXPECT_TRUE(dto.extension.empty());
    EXPECT_TRUE(dto.displayName.empty());
    EXPECT_TRUE(dto.cameraShotKey.empty());
    EXPECT_TRUE(dto.bundleName.empty());
    EXPECT_TRUE(dto.packageName.empty());
    EXPECT_TRUE(dto.appId.empty());
    EXPECT_TRUE(dto.ownerAlbumId.empty());
    EXPECT_TRUE(dto.isRealTimeThumb);
    EXPECT_EQ(dto.fileId, 0);
    EXPECT_TRUE(dto.outUri.empty());
}

HWTEST_F(MediaAssetsManagerTest, DTO_GrantUriPermissionDto_Defaults_069, TestSize.Level1)
{
    GrantUriPermissionDto dto;
    EXPECT_EQ(dto.tokenId, 0);
    EXPECT_EQ(dto.srcTokenId, 0);
    EXPECT_EQ(dto.fileId, -1);
    EXPECT_EQ(dto.permissionType, -1);
    EXPECT_EQ(dto.hideSensitiveType, -1);
    EXPECT_EQ(dto.uriType, -1);
}

HWTEST_F(MediaAssetsManagerTest, DTO_GrantUrisPermissionDto_Defaults_070, TestSize.Level1)
{
    GrantUrisPermissionDto dto;
    EXPECT_EQ(dto.tokenId, 0);
    EXPECT_EQ(dto.srcTokenId, 0);
    EXPECT_TRUE(dto.fileIds.empty());
    EXPECT_EQ(dto.permissionType, -1);
    EXPECT_EQ(dto.hideSensitiveType, -1);
    EXPECT_EQ(dto.uriType, -1);
}

HWTEST_F(MediaAssetsManagerTest, DTO_CancelUriPermissionDto_Defaults_071, TestSize.Level1)
{
    CancelUriPermissionDto dto;
    EXPECT_EQ(dto.tokenId, 0);
    EXPECT_EQ(dto.srcTokenId, 0);
    EXPECT_EQ(dto.fileId, -1);
    EXPECT_EQ(dto.permissionType, -1);
    EXPECT_EQ(dto.uriType, -1);
}

HWTEST_F(MediaAssetsManagerTest, DTO_GrantUriPermissionInnerDto_Defaults_072, TestSize.Level1)
{
    GrantUriPermissionInnerDto dto;
    EXPECT_EQ(dto.tokenId, -1);
    EXPECT_EQ(dto.srcTokenId, -1);
    EXPECT_TRUE(dto.appId.empty());
    EXPECT_TRUE(dto.fileIds.empty());
    EXPECT_TRUE(dto.uriTypes.empty());
    EXPECT_TRUE(dto.permissionTypes.empty());
    EXPECT_EQ(dto.hideSensitiveType, -1);
}

HWTEST_F(MediaAssetsManagerTest, DTO_CancelUriPermissionInnerDto_Defaults_073, TestSize.Level1)
{
    CancelUriPermissionInnerDto dto;
    EXPECT_EQ(dto.targetTokenId, -1);
    EXPECT_EQ(dto.srcTokenId, -1);
    EXPECT_TRUE(dto.fileIds.empty());
    EXPECT_TRUE(dto.uriTypes.empty());
    EXPECT_TRUE(dto.permissionTypes.empty());
}

HWTEST_F(MediaAssetsManagerTest, DTO_CheckUriPermissionInnerDto_Defaults_074, TestSize.Level1)
{
    CheckUriPermissionInnerDto dto;
    EXPECT_EQ(dto.targetTokenId, -1);
    EXPECT_TRUE(dto.uriType.empty());
    EXPECT_TRUE(dto.inFileIds.empty());
    EXPECT_TRUE(dto.columns.empty());
    EXPECT_TRUE(dto.outFileIds.empty());
    EXPECT_TRUE(dto.permissionTypes.empty());
}

HWTEST_F(MediaAssetsManagerTest, DTO_SaveCameraPhotoDto_Defaults_075, TestSize.Level1)
{
    SaveCameraPhotoDto dto;
    EXPECT_EQ(dto.fileId, INT32_MIN);
    EXPECT_EQ(dto.photoSubType, INT32_MIN);
    EXPECT_EQ(dto.imageFileType, INT32_MIN);
    EXPECT_EQ(dto.supportedWatermarkType, INT32_MIN);
    EXPECT_FALSE(dto.discardHighQualityPhoto);
    EXPECT_FALSE(dto.needScan);
    EXPECT_TRUE(dto.path.empty());
    EXPECT_EQ(dto.cameraShotKey, "NotSet");
    EXPECT_FALSE(dto.containsAddResource);
}

HWTEST_F(MediaAssetsManagerTest, DTO_OpenAssetCompressDto_Defaults_076, TestSize.Level1)
{
    OpenAssetCompressDto dto;
    EXPECT_TRUE(dto.uri.empty());
    EXPECT_EQ(dto.version, -1);
    EXPECT_EQ(dto.type, -1);
}

HWTEST_F(MediaAssetsManagerTest, DTO_CreateTmpCompatibleDupDto_Defaults_077, TestSize.Level1)
{
    CreateTmpCompatibleDupDto dto;
    EXPECT_TRUE(dto.path.empty());
}

HWTEST_F(MediaAssetsManagerTest, DTO_CreateTmpCompatibleDupDto_ToString_078, TestSize.Level1)
{
    CreateTmpCompatibleDupDto dto;
    dto.fileId = 77;
    dto.path = "/storage/test.jpg";
    std::string s = dto.ToString();
    EXPECT_NE(s.find("77"), std::string::npos);
    EXPECT_NE(s.find("/storage/test.jpg"), std::string::npos);
}

HWTEST_F(MediaAssetsManagerTest, DTO_SetLocationDto_Defaults_079, TestSize.Level1)
{
    SetLocationDto dto;
    EXPECT_EQ(dto.fileId, 0);
    EXPECT_TRUE(dto.path.empty());
    EXPECT_DOUBLE_EQ(dto.latitude, 0.0);
    EXPECT_DOUBLE_EQ(dto.longitude, 0.0);
}

HWTEST_F(MediaAssetsManagerTest, DTO_GetUrisByOldUrisInnerDto_Defaults_080, TestSize.Level1)
{
    GetUrisByOldUrisInnerDto dto;
    EXPECT_TRUE(dto.uris.empty());
    EXPECT_TRUE(dto.columns.empty());
    EXPECT_TRUE(dto.fileIds.empty());
    EXPECT_TRUE(dto.datas.empty());
    EXPECT_TRUE(dto.displayNames.empty());
    EXPECT_TRUE(dto.oldFileIds.empty());
    EXPECT_TRUE(dto.oldDatas.empty());
}

HWTEST_F(MediaAssetsManagerTest, DTO_MoveAssetsToDirDto_Defaults_081, TestSize.Level1)
{
    ChangeRequestMoveAssetsToDirDto dto;
    EXPECT_TRUE(dto.assets.empty());
    EXPECT_EQ(dto.targetDir, "");
    EXPECT_EQ(dto.requestId, 0);
    EXPECT_EQ(dto.errCode, 0);
    EXPECT_TRUE(dto.resultList.empty());
    EXPECT_EQ(dto.mode, 0);
    EXPECT_EQ(dto.changeInfo, nullptr);
}

HWTEST_F(MediaAssetsManagerTest, DTO_MoveAssetsByPathDto_Defaults_082, TestSize.Level1)
{
    ChangeRequestMoveAssetsByPathDto dto;
    EXPECT_TRUE(dto.assetPaths.empty());
    EXPECT_EQ(dto.targetDir, "");
    EXPECT_EQ(dto.requestId, 0);
    EXPECT_TRUE(dto.resultList.empty());
    EXPECT_EQ(dto.errCode, 0);
    EXPECT_EQ(dto.mode, 0);
    EXPECT_EQ(dto.changeInfo, nullptr);
}

HWTEST_F(MediaAssetsManagerTest, DTO_QueryCloudEnhancementTaskStateDto_Defaults_083, TestSize.Level1)
{
    QueryCloudEnhancementTaskStateDto dto;
    EXPECT_TRUE(dto.photoUri.empty());
    EXPECT_EQ(dto.fileId, 0);
    EXPECT_TRUE(dto.photoId.empty());
    EXPECT_EQ(dto.ceAvailable, 0);
    EXPECT_EQ(dto.ceErrorCode, 0);
}

HWTEST_F(MediaAssetsManagerTest, DTO_RestoreDto_Defaults_084, TestSize.Level1)
{
    RestoreDto dto;
    EXPECT_TRUE(dto.dbPath.empty());
    EXPECT_TRUE(dto.albumLpath.empty());
    EXPECT_TRUE(dto.keyPath.empty());
    EXPECT_TRUE(dto.bundleName.empty());
    EXPECT_TRUE(dto.appName.empty());
    EXPECT_TRUE(dto.appId.empty());
}

HWTEST_F(MediaAssetsManagerTest, DTO_ConvertFormatDto_Defaults_085, TestSize.Level1)
{
    ConvertFormatDto dto;
    EXPECT_TRUE(dto.title.empty());
    EXPECT_TRUE(dto.extension.empty());
}

HWTEST_F(MediaAssetsManagerTest, DTO_SubmitCacheDto_Defaults_086, TestSize.Level1)
{
    SubmitCacheDto dto;
    EXPECT_EQ(dto.fileId, -1);
    EXPECT_TRUE(dto.outUri.empty());
}

HWTEST_F(MediaAssetsManagerTest, DTO_AddImageDto_Defaults_087, TestSize.Level1)
{
    AddImageDto dto;
    EXPECT_EQ(dto.fileId, 0);
    EXPECT_TRUE(dto.photoId.empty());
}

HWTEST_F(MediaAssetsManagerTest, DTO_CancelTaskDto_Defaults_088, TestSize.Level1)
{
    CancelTaskDto dto;
    EXPECT_EQ(dto.requestId, 0);
}

HWTEST_F(MediaAssetsManagerTest, DTO_StartThumbnailCreationTaskDto_Defaults_089, TestSize.Level1)
{
    StartThumbnailCreationTaskDto dto;
    EXPECT_EQ(dto.requestId, 0);
    EXPECT_EQ(dto.pid, 0);
}

HWTEST_F(MediaAssetsManagerTest, DTO_StopThumbnailCreationTaskDto_Defaults_090, TestSize.Level1)
{
    StopThumbnailCreationTaskDto dto;
    EXPECT_EQ(dto.requestId, 0);
    EXPECT_EQ(dto.pid, 0);
}

// ======================================================================
// VO Default Value Tests
// ======================================================================

HWTEST_F(MediaAssetsManagerTest, DTO_CheckPhotoUrisReadPermission_ReqBody_Defaults_091, TestSize.Level1)
{
    CheckPhotoUrisReadPermissionReqBody req;
    EXPECT_TRUE(req.uris.empty());
}

HWTEST_F(MediaAssetsManagerTest, DTO_CheckPhotoUrisReadPermission_RespBody_Defaults_092, TestSize.Level1)
{
    CheckPhotoUrisReadPermissionRespBody resp;
    EXPECT_TRUE(resp.uriPermissionStateMap.empty());
}

HWTEST_F(MediaAssetsManagerTest, DTO_CompatibleInfo_Defaults_093, TestSize.Level1)
{
    CompatibleInfo info;
    EXPECT_TRUE(info.bundleName.empty());
    EXPECT_EQ(info.highResolution, -1);
    EXPECT_TRUE(info.encodings.empty());
    EXPECT_EQ(info.preferredCompatibleMode, PreferredCompatibleMode::DEFAULT);
}

HWTEST_F(MediaAssetsManagerTest, DTO_PreferredCompatibleMode_EnumValues_094, TestSize.Level1)
{
    EXPECT_EQ(static_cast<int32_t>(PreferredCompatibleMode::DEFAULT), 0);
    EXPECT_EQ(static_cast<int32_t>(PreferredCompatibleMode::CURRENT), 1);
    EXPECT_EQ(static_cast<int32_t>(PreferredCompatibleMode::COMPATIBLE), 2);
}

HWTEST_F(MediaAssetsManagerTest, DTO_GetTranscodeCheckInfoRespBody_Defaults_095, TestSize.Level1)
{
    GetTranscodeCheckInfoRespBody resp;
    EXPECT_TRUE(resp.bundleName.empty());
    EXPECT_EQ(resp.preferredCompatibleMode, 0);
}

HWTEST_F(MediaAssetsManagerTest, DTO_GetCompatibleInfoRespBody_Defaults_096, TestSize.Level1)
{
    GetCompatibleInfoRespBody resp;
    EXPECT_TRUE(resp.bundleName.empty());
    EXPECT_TRUE(resp.supportedMimeTypes.empty());
}

HWTEST_F(MediaAssetsManagerTest, DTO_CompatibleInfo_SetEncodings_097, TestSize.Level1)
{
    CompatibleInfo info;
    info.encodings = {"image/heic", "image/jpeg"};
    EXPECT_EQ(info.encodings.size(), 2u);
    info.highResolution = 1;
    EXPECT_EQ(info.highResolution, 1);
    info.bundleName = "com.test.bundle";
    EXPECT_EQ(info.bundleName, "com.test.bundle");
}

HWTEST_F(MediaAssetsManagerTest, DTO_FormInfoDto_SetValues_098, TestSize.Level1)
{
    FormInfoDto dto;
    dto.formIds = {"form1", "form2"};
    dto.fileUris = {"uri1", "uri2"};
    EXPECT_EQ(dto.formIds.size(), 2u);
    EXPECT_EQ(dto.fileUris.size(), 2u);
}

HWTEST_F(MediaAssetsManagerTest, DTO_CloudEnhancementDto_SetValues_099, TestSize.Level1)
{
    CloudEnhancementDto dto;
    dto.hasCloudWatermark = true;
    dto.triggerMode = 1;
    dto.fileUris = {"uri1"};
    EXPECT_TRUE(dto.hasCloudWatermark);
    EXPECT_EQ(dto.triggerMode, 1);
    EXPECT_EQ(dto.fileUris.size(), 1u);
}

} // namespace Media
} // namespace OHOS
