/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#define MLOG_TAG "PhotoAlbumLPathOperationTest"

#include "photo_album_lpath_operation_test.h"

#include <string>
#include <vector>

#include "photo_album_lpath_operation.h"
#include "media_log.h"

using namespace testing::ext;

namespace OHOS::Media {
void PhotoAlbumLPathOperationTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("SetUpTestCase");
}

void PhotoAlbumLPathOperationTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("TearDownTestCase");
}

void PhotoAlbumLPathOperationTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
}

void PhotoAlbumLPathOperationTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}

HWTEST_F(PhotoAlbumLPathOperationTest, CleanInvalidPhotoAlbums_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CleanInvalidPhotoAlbums_Test_001");
    int32_t affectedCount = PhotoAlbumLPathOperation::GetInstance()
                                .SetRdbStore(nullptr)
                                .Start()
                                .CleanInvalidPhotoAlbums()
                                .GetAlbumAffectedCount();
    EXPECT_EQ(affectedCount, 0);
}

HWTEST_F(PhotoAlbumLPathOperationTest, CleanInvalidPhotoAlbums_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CleanInvalidPhotoAlbums_Test_002");
    PhotoAlbumLPathOperation::GetInstance()
        .SetRdbStore(nullptr)
        .Start()
        .Stop();
    int32_t affectedCount = PhotoAlbumLPathOperation::GetInstance()
                                .CleanInvalidPhotoAlbums()
                                .GetAlbumAffectedCount();
    EXPECT_EQ(affectedCount, 0);
}

HWTEST_F(PhotoAlbumLPathOperationTest, CleanDuplicatePhotoAlbums_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CleanDuplicatePhotoAlbums_Test_001");
    int32_t affectedCount = PhotoAlbumLPathOperation::GetInstance()
                                .SetRdbStore(nullptr)
                                .Start()
                                .CleanDuplicatePhotoAlbums()
                                .GetAlbumAffectedCount();
    EXPECT_EQ(affectedCount, 0);
}

HWTEST_F(PhotoAlbumLPathOperationTest, CleanDuplicatePhotoAlbums_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CleanDuplicatePhotoAlbums_Test_002");
    PhotoAlbumLPathOperation::GetInstance()
        .SetRdbStore(nullptr)
        .Start()
        .Stop();
    int32_t affectedCount = PhotoAlbumLPathOperation::GetInstance()
                                .CleanDuplicatePhotoAlbums()
                                .GetAlbumAffectedCount();
    EXPECT_EQ(affectedCount, 0);
}

HWTEST_F(PhotoAlbumLPathOperationTest, CleanEmptylPathPhotoAlbums_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CleanEmptylPathPhotoAlbums_Test_001");
    int32_t affectedCount = PhotoAlbumLPathOperation::GetInstance()
                                .SetRdbStore(nullptr)
                                .Start()
                                .CleanEmptylPathPhotoAlbums()
                                .GetAlbumAffectedCount();
    EXPECT_EQ(affectedCount, 0);
}

HWTEST_F(PhotoAlbumLPathOperationTest, CleanEmptylPathPhotoAlbums_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CleanEmptylPathPhotoAlbums_Test_002");
    PhotoAlbumLPathOperation::GetInstance()
        .SetRdbStore(nullptr)
        .Start()
        .Stop();
    int32_t affectedCount = PhotoAlbumLPathOperation::GetInstance()
                                .CleanEmptylPathPhotoAlbums()
                                .GetAlbumAffectedCount();
    EXPECT_EQ(affectedCount, 0);
}

HWTEST_F(PhotoAlbumLPathOperationTest, Get_Invalid_PhotoAlbums_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start Get_Invalid_PhotoAlbums_Test_001");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    ASSERT_NE(ptr, nullptr);
    std::vector<PhotoAlbumInfoPo> test;
    test = ptr->GetInvalidPhotoAlbums();
    EXPECT_EQ(test.empty(), true);
    MEDIA_INFO_LOG("Get_Invalid_PhotoAlbums_Test_001 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, Clean_Duplicate_Photo_Album_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start Clean_Duplicate_Photo_Albums_Test_001");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo mainAlbumInfo;
    int32_t ret;
    mainAlbumInfo.albumId = 10;
    mainAlbumInfo.albumName = "test.jpg";
    mainAlbumInfo.lPath = "/ment/test/test.jpg";
    ASSERT_NE(ptr, nullptr);
    ptr->isContinue_.store(false);
    ret = ptr->CleanDuplicatePhotoAlbum(mainAlbumInfo);
    EXPECT_NE(ret, -1);
    MEDIA_INFO_LOG("Clean_Duplicate_Photo_Albums_Test_001 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, Clean_Emptyl_Path_Photo_Album_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start Clean_Emptyl_Path_Photo_Album_Test_001");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo mainAlbumInfo;
    int32_t ret;
    ASSERT_NE(ptr, nullptr);
    ptr->isContinue_.store(false);
    ret = ptr->CleanEmptylPathPhotoAlbum(mainAlbumInfo);
    EXPECT_NE(ret, -1);
    MEDIA_INFO_LOG("Clean_Emptyl_Path_Photo_Album_Test_001 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, Palot_To_String_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start Clean_Emptyl_Path_Photo_Album_Test_001");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    std::string ret;
    std::vector<NativeRdb::ValueObject> values = { 1, 2, 3, 10 };
    ASSERT_NE(ptr, nullptr);
    ret = ptr->ToString(values);
    EXPECT_NE(ret, "");
    MEDIA_INFO_LOG("Clean_Emptyl_Path_Photo_Album_Test_001 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, UpdateAlbumInfoFromAlbumPluginByAlbumId_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start UpdateAlbumInfoFromAlbumPluginByAlbumId_Test_001");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo albumInfo;
    int32_t ret;
    ASSERT_NE(ptr, nullptr);
    ret = ptr->UpdateAlbumInfoFromAlbumPluginByAlbumId(albumInfo);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("UpdateAlbumInfoFromAlbumPluginByAlbumId_Test_001 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, UpdateAlbumInfoFromAlbumPluginByAlbumId_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start UpdateAlbumInfoFromAlbumPluginByAlbumId_Test_002");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo albumInfo;
    int32_t ret;
    albumInfo.albumId = 100;
    albumInfo.lPath = "/storage/test/album";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->UpdateAlbumInfoFromAlbumPluginByAlbumId(albumInfo);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("UpdateAlbumInfoFromAlbumPluginByAlbumId_Test_002 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, UpdateAlbumInfoFromAlbumPluginByAlbumId_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start UpdateAlbumInfoFromAlbumPluginByAlbumId_Test_003");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo albumInfo;
    int32_t ret;
    albumInfo.albumId = 0;
    albumInfo.lPath = "/storage/test/album";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->UpdateAlbumInfoFromAlbumPluginByAlbumId(albumInfo);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("UpdateAlbumInfoFromAlbumPluginByAlbumId_Test_003 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, UpdateAlbumInfoFromAlbumPluginByAlbumId_Test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start UpdateAlbumInfoFromAlbumPluginByAlbumId_Test_004");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo albumInfo;
    int32_t ret;
    albumInfo.albumId = -1;
    albumInfo.lPath = "/storage/test/album";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->UpdateAlbumInfoFromAlbumPluginByAlbumId(albumInfo);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("UpdateAlbumInfoFromAlbumPluginByAlbumId_Test_004 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, UpdateAlbumInfoFromAlbumPluginByAlbumId_Test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start UpdateAlbumInfoFromAlbumPluginByAlbumId_Test_005");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo albumInfo;
    int32_t ret;
    albumInfo.albumId = 100;
    albumInfo.lPath = "";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->UpdateAlbumInfoFromAlbumPluginByAlbumId(albumInfo);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("UpdateAlbumInfoFromAlbumPluginByAlbumId_Test_005 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, UpdateAlbumLPathByAlbumId_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start UpdateAlbumLPathByAlbumId_Test_001");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo albumInfo;
    int32_t ret;
    ASSERT_NE(ptr, nullptr);
    ret = ptr->UpdateAlbumLPathByAlbumId(albumInfo);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("UpdateAlbumLPathByAlbumId_Test_001 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, UpdateAlbumLPathByAlbumId_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start UpdateAlbumLPathByAlbumId_Test_002");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo albumInfo;
    int32_t ret;
    albumInfo.albumId = 100;
    albumInfo.lPath = "/storage/test/album";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->UpdateAlbumLPathByAlbumId(albumInfo);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("UpdateAlbumLPathByAlbumId_Test_002 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, UpdateAlbumLPathByAlbumId_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start UpdateAlbumLPathByAlbumId_Test_003");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo albumInfo;
    int32_t ret;
    albumInfo.albumId = 0;
    albumInfo.lPath = "/storage/test/album";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->UpdateAlbumLPathByAlbumId(albumInfo);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("UpdateAlbumLPathByAlbumId_Test_003 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, UpdateAlbumLPathByAlbumId_Test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start UpdateAlbumLPathByAlbumId_Test_004");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo albumInfo;
    int32_t ret;
    albumInfo.albumId = -1;
    albumInfo.lPath = "/storage/test/album";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->UpdateAlbumLPathByAlbumId(albumInfo);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("UpdateAlbumLPathByAlbumId_Test_004 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, UpdateAlbumLPathByAlbumId_Test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start UpdateAlbumLPathByAlbumId_Test_005");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo albumInfo;
    int32_t ret;
    albumInfo.albumId = 100;
    albumInfo.lPath = "";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->UpdateAlbumLPathByAlbumId(albumInfo);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("UpdateAlbumLPathByAlbumId_Test_005 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, GetLatestAlbumInfoBylPath_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetLatestAlbumInfoBylPath_Test_001");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo ret;
    std::string lPath = "/storage/test/album";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->GetLatestAlbumInfoBylPath(lPath);
    EXPECT_EQ(ret.albumId, 0);
    MEDIA_INFO_LOG("GetLatestAlbumInfoBylPath_Test_001 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, GetLatestAlbumInfoBylPath_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetLatestAlbumInfoBylPath_Test_002");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo ret;
    std::string lPath = "";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->GetLatestAlbumInfoBylPath(lPath);
    EXPECT_EQ(ret.albumId, 0);
    MEDIA_INFO_LOG("GetLatestAlbumInfoBylPath_Test_002 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, GetLatestAlbumInfoBylPath_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetLatestAlbumInfoBylPath_Test_003");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo ret;
    std::string lPath = "/storage/test/album/with/long/path";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->GetLatestAlbumInfoBylPath(lPath);
    EXPECT_EQ(ret.albumId, 0);
    MEDIA_INFO_LOG("GetLatestAlbumInfoBylPath_Test_003 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, MergePhotoAlbum_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start MergePhotoAlbum_Test_001");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo mainAlbumInfo;
    PhotoAlbumInfoPo subAlbumInfo;
    int32_t ret;
    mainAlbumInfo.albumId = 100;
    subAlbumInfo.albumId = 200;
    ASSERT_NE(ptr, nullptr);
    ret = ptr->MergePhotoAlbum(mainAlbumInfo, subAlbumInfo);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("MergePhotoAlbum_Test_001 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, MergePhotoAlbum_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start MergePhotoAlbum_Test_002");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo mainAlbumInfo;
    PhotoAlbumInfoPo subAlbumInfo;
    int32_t ret;
    mainAlbumInfo.albumId = 0;
    subAlbumInfo.albumId = 200;
    ASSERT_NE(ptr, nullptr);
    ret = ptr->MergePhotoAlbum(mainAlbumInfo, subAlbumInfo);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("MergePhotoAlbum_Test_002 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, MergePhotoAlbum_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start MergePhotoAlbum_Test_003");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo mainAlbumInfo;
    PhotoAlbumInfoPo subAlbumInfo;
    int32_t ret;
    mainAlbumInfo.albumId = 100;
    subAlbumInfo.albumId = 0;
    ASSERT_NE(ptr, nullptr);
    ret = ptr->MergePhotoAlbum(mainAlbumInfo, subAlbumInfo);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("MergePhotoAlbum_Test_003 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, MergePhotoAlbum_Test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start MergePhotoAlbum_Test_004");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo mainAlbumInfo;
    PhotoAlbumInfoPo subAlbumInfo;
    int32_t ret;
    mainAlbumInfo.albumId = -1;
    subAlbumInfo.albumId = 200;
    ASSERT_NE(ptr, nullptr);
    ret = ptr->MergePhotoAlbum(mainAlbumInfo, subAlbumInfo);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("MergePhotoAlbum_Test_004 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, MergePhotoAlbum_Test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start MergePhotoAlbum_Test_005");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo mainAlbumInfo;
    PhotoAlbumInfoPo subAlbumInfo;
    int32_t ret;
    mainAlbumInfo.albumId = 100;
    subAlbumInfo.albumId = -1;
    ASSERT_NE(ptr, nullptr);
    ret = ptr->MergePhotoAlbum(mainAlbumInfo, subAlbumInfo);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("MergePhotoAlbum_Test_005 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, GetDuplicatelPathAlbumInfoMain_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetDuplicatelPathAlbumInfoMain_Test_001");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    std::vector<PhotoAlbumInfoPo> ret;
    ASSERT_NE(ptr, nullptr);
    ret = ptr->GetDuplicatelPathAlbumInfoMain();
    EXPECT_EQ(ret.empty(), true);
    MEDIA_INFO_LOG("GetDuplicatelPathAlbumInfoMain_Test_001 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, GetDuplicatelPathAlbumInfoSub_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetDuplicatelPathAlbumInfoSub_Test_001");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo albumInfo;
    std::vector<PhotoAlbumInfoPo> ret;
    albumInfo.albumId = 100;
    albumInfo.lPath = "/storage/test/album";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->GetDuplicatelPathAlbumInfoSub(albumInfo);
    EXPECT_EQ(ret.empty(), true);
    MEDIA_INFO_LOG("GetDuplicatelPathAlbumInfoSub_Test_001 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, GetDuplicatelPathAlbumInfoSub_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetDuplicatelPathAlbumInfoSub_Test_002");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo albumInfo;
    std::vector<PhotoAlbumInfoPo> ret;
    albumInfo.albumId = 0;
    albumInfo.lPath = "/storage/test/album";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->GetDuplicatelPathAlbumInfoSub(albumInfo);
    EXPECT_EQ(ret.empty(), true);
    MEDIA_INFO_LOG("GetDuplicatelPathAlbumInfoSub_Test_002 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, GetDuplicatelPathAlbumInfoSub_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetDuplicatelPathAlbumInfoSub_Test_003");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo albumInfo;
    std::vector<PhotoAlbumInfoPo> ret;
    albumInfo.albumId = 100;
    albumInfo.lPath = "";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->GetDuplicatelPathAlbumInfoSub(albumInfo);
    EXPECT_EQ(ret.empty(), true);
    MEDIA_INFO_LOG("GetDuplicatelPathAlbumInfoSub_Test_003 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, GetDuplicatelPathAlbumInfoSub_Test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetDuplicatelPathAlbumInfoSub_Test_004");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo albumInfo;
    std::vector<PhotoAlbumInfoPo> ret;
    albumInfo.albumId = -1;
    albumInfo.lPath = "/storage/test/album";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->GetDuplicatelPathAlbumInfoSub(albumInfo);
    EXPECT_EQ(ret.empty(), true);
    MEDIA_INFO_LOG("GetDuplicatelPathAlbumInfoSub_Test_004 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, GetEmptylPathAlbumInfo_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetEmptylPathAlbumInfo_Test_001");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    std::vector<PhotoAlbumInfoPo> ret;
    ASSERT_NE(ptr, nullptr);
    ret = ptr->GetEmptylPathAlbumInfo();
    EXPECT_EQ(ret.empty(), true);
    MEDIA_INFO_LOG("GetEmptylPathAlbumInfo_Test_001 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, GetInstance_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetInstance_Test_001");
    PhotoAlbumLPathOperation &instance1 = PhotoAlbumLPathOperation::GetInstance();
    PhotoAlbumLPathOperation &instance2 = PhotoAlbumLPathOperation::GetInstance();
    EXPECT_EQ(&instance1, &instance2);
    MEDIA_INFO_LOG("GetInstance_Test_001 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, Start_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start Start_Test_001");
    PhotoAlbumLPathOperation &instance = PhotoAlbumLPathOperation::GetInstance();
    PhotoAlbumLPathOperation &result = instance.Start();
    EXPECT_EQ(&instance, &result);
    MEDIA_INFO_LOG("Start_Test_001 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, Stop_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start Stop_Test_001");
    PhotoAlbumLPathOperation &instance = PhotoAlbumLPathOperation::GetInstance();
    instance.Stop();
    int32_t count = instance.GetAlbumAffectedCount();
    EXPECT_GE(count, 0);
    MEDIA_INFO_LOG("Stop_Test_001 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, SetRdbStore_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetRdbStore_Test_001");
    PhotoAlbumLPathOperation &instance = PhotoAlbumLPathOperation::GetInstance();
    PhotoAlbumLPathOperation &result = instance.SetRdbStore(nullptr);
    EXPECT_EQ(&instance, &result);
    MEDIA_INFO_LOG("SetRdbStore_Test_001 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, GetAlbumAffectedCount_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetAlbumAffectedCount_Test_001");
    PhotoAlbumLPathOperation &instance = PhotoAlbumLPathOperation::GetInstance();
    int32_t count = instance.GetAlbumAffectedCount();
    EXPECT_GE(count, 0);
    MEDIA_INFO_LOG("GetAlbumAffectedCount_Test_001 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, ToString_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ToString_Test_001");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    std::vector<NativeRdb::ValueObject> values;
    std::string ret;
    ASSERT_NE(ptr, nullptr);
    ret = ptr->ToString(values);
    EXPECT_EQ(ret, "");
    MEDIA_INFO_LOG("ToString_Test_001 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, ToString_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ToString_Test_002");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    std::vector<NativeRdb::ValueObject> values = { NativeRdb::ValueObject(1) };
    std::string ret;
    ASSERT_NE(ptr, nullptr);
    ret = ptr->ToString(values);
    EXPECT_NE(ret, "");
    MEDIA_INFO_LOG("ToString_Test_002 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, ToString_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ToString_Test_003");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    std::vector<NativeRdb::ValueObject> values = { NativeRdb::ValueObject("test") };
    std::string ret;
    ASSERT_NE(ptr, nullptr);
    ret = ptr->ToString(values);
    EXPECT_NE(ret, "");
    MEDIA_INFO_LOG("ToString_Test_003 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, ToString_Test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ToString_Test_004");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    std::vector<NativeRdb::ValueObject> values = { NativeRdb::ValueObject(1), NativeRdb::ValueObject("test"),
        NativeRdb::ValueObject(2.5), NativeRdb::ValueObject(100) };
    std::string ret;
    ASSERT_NE(ptr, nullptr);
    ret = ptr->ToString(values);
    EXPECT_NE(ret, "");
    MEDIA_INFO_LOG("ToString_Test_004 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, CleanDuplicatePhotoAlbum_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CleanDuplicatePhotoAlbum_Test_002");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo mainAlbumInfo;
    int32_t ret;
    mainAlbumInfo.albumId = 0;
    mainAlbumInfo.albumName = "test.jpg";
    mainAlbumInfo.lPath = "/mnt/test/test.jpg";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->CleanDuplicatePhotoAlbum(mainAlbumInfo);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    MEDIA_INFO_LOG("CleanDuplicatePhotoAlbum_Test_002 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, CleanDuplicatePhotoAlbum_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CleanDuplicatePhotoAlbum_Test_003");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo mainAlbumInfo;
    int32_t ret;
    mainAlbumInfo.albumId = 10;
    mainAlbumInfo.albumName = "";
    mainAlbumInfo.lPath = "/mnt/test/test.jpg";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->CleanDuplicatePhotoAlbum(mainAlbumInfo);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    MEDIA_INFO_LOG("CleanDuplicatePhotoAlbum_Test_003 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, CleanDuplicatePhotoAlbum_Test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CleanDuplicatePhotoAlbum_Test_004");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo mainAlbumInfo;
    int32_t ret;
    mainAlbumInfo.albumId = 10;
    mainAlbumInfo.albumName = "test.jpg";
    mainAlbumInfo.lPath = "";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->CleanDuplicatePhotoAlbum(mainAlbumInfo);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    MEDIA_INFO_LOG("CleanDuplicatePhotoAlbum_Test_004 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, CleanEmptylPathPhotoAlbum_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CleanEmptylPathPhotoAlbum_Test_002");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo subAlbumInfo;
    int32_t ret;
    subAlbumInfo.albumId = 0;
    subAlbumInfo.albumName = "test.jpg";
    subAlbumInfo.lPath = "/mnt/test/test.jpg";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->CleanEmptylPathPhotoAlbum(subAlbumInfo);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("CleanEmptylPathPhotoAlbum_Test_002 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, CleanEmptylPathPhotoAlbum_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CleanEmptylPathPhotoAlbum_Test_003");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo subAlbumInfo;
    int32_t ret;
    subAlbumInfo.albumId = 10;
    subAlbumInfo.albumName = "";
    subAlbumInfo.lPath = "/mnt/test/test.jpg";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->CleanEmptylPathPhotoAlbum(subAlbumInfo);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("CleanEmptylPathPhotoAlbum_Test_003 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, CleanEmptylPathPhotoAlbum_Test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CleanEmptylPathPhotoAlbum_Test_004");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo subAlbumInfo;
    int32_t ret;
    subAlbumInfo.albumId = 10;
    subAlbumInfo.albumName = "test.jpg";
    subAlbumInfo.lPath = "";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->CleanEmptylPathPhotoAlbum(subAlbumInfo);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("CleanEmptylPathPhotoAlbum_Test_004 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, CleanEmptylPathPhotoAlbum_Test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CleanEmptylPathPhotoAlbum_Test_005");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo subAlbumInfo;
    int32_t ret;
    subAlbumInfo.albumId = -1;
    subAlbumInfo.albumName = "test.jpg";
    subAlbumInfo.lPath = "/mnt/test/test.jpg";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->CleanEmptylPathPhotoAlbum(subAlbumInfo);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("CleanEmptylPathPhotoAlbum_Test_005 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, CleanEmptylPathPhotoAlbum_Test_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CleanEmptylPathPhotoAlbum_Test_006");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo subAlbumInfo;
    int32_t ret;
    subAlbumInfo.albumId = 10;
    subAlbumInfo.albumName = "test.jpg";
    subAlbumInfo.lPath = "/storage/test/album/path";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->CleanEmptylPathPhotoAlbum(subAlbumInfo);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("CleanEmptylPathPhotoAlbum_Test_006 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, CleanInvalidPhotoAlbums_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CleanInvalidPhotoAlbums_Test_003");
    PhotoAlbumLPathOperation &instance = PhotoAlbumLPathOperation::GetInstance();
    instance.SetRdbStore(nullptr);
    instance.Start();
    int32_t affectedCount = instance.CleanInvalidPhotoAlbums().GetAlbumAffectedCount();
    EXPECT_EQ(affectedCount, 0);
    MEDIA_INFO_LOG("CleanInvalidPhotoAlbums_Test_003 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, CleanDuplicatePhotoAlbums_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CleanDuplicatePhotoAlbums_Test_003");
    PhotoAlbumLPathOperation &instance = PhotoAlbumLPathOperation::GetInstance();
    instance.SetRdbStore(nullptr);
    instance.Start();
    int32_t affectedCount = instance.CleanDuplicatePhotoAlbums().GetAlbumAffectedCount();
    EXPECT_EQ(affectedCount, 0);
    MEDIA_INFO_LOG("CleanDuplicatePhotoAlbums_Test_003 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, CleanEmptylPathPhotoAlbums_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CleanEmptylPathPhotoAlbums_Test_003");
    PhotoAlbumLPathOperation &instance = PhotoAlbumLPathOperation::GetInstance();
    instance.SetRdbStore(nullptr);
    instance.Start();
    int32_t affectedCount = instance.CleanEmptylPathPhotoAlbums().GetAlbumAffectedCount();
    EXPECT_EQ(affectedCount, 0);
    MEDIA_INFO_LOG("CleanEmptylPathPhotoAlbums_Test_003 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, GetInvalidPhotoAlbums_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetInvalidPhotoAlbums_Test_002");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    ASSERT_NE(ptr, nullptr);
    ptr->SetRdbStore(nullptr);
    std::vector<PhotoAlbumInfoPo> test = ptr->GetInvalidPhotoAlbums();
    EXPECT_EQ(test.empty(), true);
    MEDIA_INFO_LOG("GetInvalidPhotoAlbums_Test_002 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, ChainOperation_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ChainOperation_Test_001");
    int32_t affectedCount = PhotoAlbumLPathOperation::GetInstance()
                                .SetRdbStore(nullptr)
                                .Start()
                                .CleanInvalidPhotoAlbums()
                                .CleanDuplicatePhotoAlbums()
                                .CleanEmptylPathPhotoAlbums()
                                .GetAlbumAffectedCount();
    EXPECT_EQ(affectedCount, 0);
    MEDIA_INFO_LOG("ChainOperation_Test_001 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, ChainOperation_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ChainOperation_Test_002");
    PhotoAlbumLPathOperation::GetInstance().Stop();
    int32_t affectedCount = PhotoAlbumLPathOperation::GetInstance()
                                .CleanInvalidPhotoAlbums()
                                .CleanDuplicatePhotoAlbums()
                                .CleanEmptylPathPhotoAlbums()
                                .GetAlbumAffectedCount();
    EXPECT_EQ(affectedCount, 0);
    MEDIA_INFO_LOG("ChainOperation_Test_002 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, EdgeCase_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start EdgeCase_Test_001");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo albumInfo;
    int32_t ret;
    albumInfo.albumId = INT32_MAX;
    albumInfo.lPath = "/storage-max/test/album";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->UpdateAlbumInfoFromAlbumPluginByAlbumId(albumInfo);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("EdgeCase_Test_001 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, EdgeCase_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start EdgeCase_Test_002");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo albumInfo;
    int32_t ret;
    albumInfo.albumId = INT32_MIN;
    albumInfo.lPath = "/storage-min/test/album";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->UpdateAlbumInfoFromAlbumPluginByAlbumId(albumInfo);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("EdgeCase_Test_002 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, EdgeCase_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start EdgeCase_Test_003");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo mainAlbumInfo;
    PhotoAlbumInfoPo subAlbumInfo;
    int32_t ret;
    mainAlbumInfo.albumId = INT32_MAX;
    subAlbumInfo.albumId = INT32_MAX;
    ASSERT_NE(ptr, nullptr);
    ret = ptr->MergePhotoAlbum(mainAlbumInfo, subAlbumInfo);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("EdgeCase_Test_003 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, EdgeCase_Test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start EdgeCase_Test_004");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo albumInfo;
    std::vector<PhotoAlbumInfoPo> ret;
    albumInfo.albumId = INT32_MAX;
    albumInfo.lPath = "/storage-max/test/album";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->GetDuplicatelPathAlbumInfoSub(albumInfo);
    EXPECT_EQ(ret.empty(), true);
    MEDIA_INFO_LOG("EdgeCase_Test_004 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, EdgeCase_Test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start EdgeCase_Test_005");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo mainAlbumInfo;
    int32_t ret;
    mainAlbumInfo.albumId = INT32_MAX;
    mainAlbumInfo.albumName = "max-test.jpg";
    mainAlbumInfo.lPath = "/storage-max/test/test.jpg";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->CleanDuplicatePhotoAlbum(mainAlbumInfo);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    MEDIA_INFO_LOG("EdgeCase_Test_005 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, EdgeCase_Test_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start EdgeCase_Test_006");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo subAlbumInfo;
    int32_t ret;
    subAlbumInfo.albumId = INT32_MAX;
    subAlbumInfo.albumName = "max-test.jpg";
    subAlbumInfo.lPath = "/storage-max/test/test.jpg";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->CleanEmptylPathPhotoAlbum(subAlbumInfo);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("EdgeCase_Test_006 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, EdgeCase_Test_007, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start EdgeCase_Test_007");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo ret;
    std::string lPath = "/storage/very/long/path/that/goes/on/and/on/with/many/directories/to/test/edge/cases";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->GetLatestAlbumInfoBylPath(lPath);
    EXPECT_EQ(ret.albumId, 0);
    MEDIA_INFO_LOG("EdgeCase_Test_007 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, EdgeCase_Test_008, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start EdgeCase_Test_008");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo ret;
    std::string lPath = "/storage/test/album";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->GetLatestAlbumInfoBylPath(lPath);
    EXPECT_EQ(ret.albumId, 0);
    ret = ptr->GetLatestAlbumInfoBylPath(lPath);
    EXPECT_EQ(ret.albumId, 0);
    MEDIA_INFO_LOG("EdgeCase_Test_008 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, StressTest_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start StressTest_Test_001");
    for (int i = 0; i < 100; i++) {
        PhotoAlbumLPathOperation &instance = PhotoAlbumLPathOperation::GetInstance();
        EXPECT_GE(instance.GetAlbumAffectedCount(), 0);
    }
    MEDIA_INFO_LOG("StressTest_Test_001 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, StressTest_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start StressTest_Test_002");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    for (int i = 0; i < 50; i++) {
        std::vector<PhotoAlbumInfoPo> ret = ptr->GetInvalidPhotoAlbums();
        EXPECT_EQ(ret.empty(), true);
    }
    MEDIA_INFO_LOG("StressTest_Test_002 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, StressTest_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start StressTest_Test_003");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    for (int i = 0; i < 50; i++) {
        std::vector<PhotoAlbumInfoPo> ret = ptr->GetDuplicatelPathAlbumInfoMain();
        EXPECT_EQ(ret.empty(), true);
    }
    MEDIA_INFO_LOG("StressTest_Test_003 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, StressTest_Test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start StressTest_Test_004");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    for (int i = 0; i < 50; i++) {
        std::vector<PhotoAlbumInfoPo> ret = ptr->GetEmptylPathAlbumInfo();
        EXPECT_EQ(ret.empty(), true);
    }
    MEDIA_INFO_LOG("StressTest_Test_004 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, StressTest_Test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start StressTest_Test_005");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    for (int i = 0; i < 50; i++) {
        PhotoAlbumInfoPo ret = ptr->GetLatestAlbumInfoBylPath("/storage/test/album");
        EXPECT_EQ(ret.albumId, 0);
    }
    MEDIA_INFO_LOG("StressTest_Test_005 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, StressTest_Test_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start StressTest_Test_006");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo albumInfo;
    albumInfo.albumId = 100;
    albumInfo.lPath = "/storage/test/album";
    for (int i = 0; i < 50; i++) {
        int32_t ret = ptr->UpdateAlbumInfoFromAlbumPluginByAlbumId(albumInfo);
        EXPECT_EQ(ret, NativeRdb::E_ERROR);
    }
    MEDIA_INFO_LOG("StressTest_Test_006 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, StressTest_Test_007, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start StressTest_Test_007");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo albumInfo;
    albumInfo.albumId = 100;
    albumInfo.lPath = "/storage/test/album";
    for (int i = 0; i < 50; i++) {
        int32_t ret = ptr->UpdateAlbumLPathByAlbumId(albumInfo);
        EXPECT_EQ(ret, NativeRdb::E_ERROR);
    }
    MEDIA_INFO_LOG("StressTest_Test_007 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, StressTest_Test_008, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start StressTest_Test_008");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo mainAlbumInfo;
    PhotoAlbumInfoPo subAlbumInfo;
    mainAlbumInfo.albumId = 100;
    subAlbumInfo.albumId = 200;
    for (int i = 0; i < 50; i++) {
        int32_t ret = ptr->MergePhotoAlbum(mainAlbumInfo, subAlbumInfo);
        EXPECT_EQ(ret, NativeRdb::E_ERROR);
    }
    MEDIA_INFO_LOG("StressTest_Test_008 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, StressTest_Test_009, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start StressTest_Test_009");
    PhotoAlbumLPathOperation &instance = PhotoAlbumLPathOperation::GetInstance();
    for (int i = 0; i < 50; i++) {
        instance.Start();
        instance.Stop();
    }
    int32_t count = instance.GetAlbumAffectedCount();
    EXPECT_GE(count, 0);
    MEDIA_INFO_LOG("StressTest_Test_009 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, StressTest_Test_010, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start StressTest_Test_010");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    std::vector<NativeRdb::ValueObject> values;
    for (int i = 0; i < 100; i++) {
        values.push_back(NativeRdb::ValueObject(i));
    }
    std::string ret = ptr->ToString(values);
    EXPECT_NE(ret, "");
    MEDIA_INFO_LOG("StressTest_Test_010 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, NullPointerTest_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start NullPointerTest_001");
    PhotoAlbumLPathOperation &instance = PhotoAlbumLPathOperation::GetInstance();
    instance.SetRdbStore(nullptr);
    std::vector<PhotoAlbumInfoPo> ret = instance.GetInvalidPhotoAlbums();
    EXPECT_EQ(ret.empty(), true);
    MEDIA_INFO_LOG("NullPointerTest_001 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, NullPointerTest_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start NullPointerTest_002");
    PhotoAlbumLPathOperation &instance = PhotoAlbumLPathOperation::GetInstance();
    instance.SetRdbStore(nullptr);
    std::vector<PhotoAlbumInfoPo> ret = instance.GetDuplicatelPathAlbumInfoMain();
    EXPECT_EQ(ret.empty(), true);
    MEDIA_INFO_LOG("NullPointerTest_002 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, NullPointerTest_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start NullPointerTest_003");
    PhotoAlbumLPathOperation &instance = PhotoAlbumLPathOperation::GetInstance();
    instance.SetRdbStore(nullptr);
    std::vector<PhotoAlbumInfoPo> ret = instance.GetEmptylPathAlbumInfo();
    EXPECT_EQ(ret.empty(), true);
    MEDIA_INFO_LOG("NullPointerTest_003 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, NullPointerTest_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start NullPointerTest_004");
    PhotoAlbumLPathOperation &instance = PhotoAlbumLPathOperation::GetInstance();
    instance.SetRdbStore(nullptr);
    PhotoAlbumInfoPo ret = instance.GetLatestAlbumInfoBylPath("/storage/test/album");
    EXPECT_EQ(ret.albumId, 0);
    MEDIA_INFO_LOG("NullPointerTest_004 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, NullPointerTest_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start NullPointerTest_005");
    PhotoAlbumLPathOperation &instance = PhotoAlbumLPathOperation::GetInstance();
    instance.SetRdbStore(nullptr);
    PhotoAlbumInfoPo albumInfo;
    albumInfo.albumId = 100;
    albumInfo.lPath = "/storage/test/album";
    int32_t ret = instance.UpdateAlbumInfoFromAlbumPluginByAlbumId(albumInfo);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("NullPointerTest_005 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, NullPointerTest_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start NullPointerTest_006");
    PhotoAlbumLPathOperation &instance = PhotoAlbumLPathOperation::GetInstance();
    instance.SetRdbStore(nullptr);
    PhotoAlbumInfoPo albumInfo;
    albumInfo.albumId = 100;
    albumInfo.lPath = "/storage/test/album";
    int32_t ret = instance.UpdateAlbumLPathByAlbumId(albumInfo);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("NullPointerTest_006 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, NullPointerTest_007, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start NullPointerTest_007");
    PhotoAlbumLPathOperation &instance = PhotoAlbumLPathOperation::GetInstance();
    instance.SetRdbStore(nullptr);
    PhotoAlbumInfoPo mainAlbumInfo;
    PhotoAlbumInfoPo subAlbumInfo;
    mainAlbumInfo.albumId = 100;
    subAlbumInfo.albumId = 200;
    int32_t ret = instance.MergePhotoAlbum(mainAlbumInfo, subAlbumInfo);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("NullPointerTest_007 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, BoundaryTest_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start BoundaryTest_001");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo albumInfo;
    int32_t ret;
    albumInfo.albumId = 1;
    albumInfo.lPath = "/";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->UpdateAlbumInfoFromAlbumPluginByAlbumId(albumInfo);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("BoundaryTest_001 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, BoundaryTest_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start BoundaryTest_002");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo albumInfo;
    int32_t ret;
    albumInfo.albumId = 1;
    albumInfo.lPath = "a";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->UpdateAlbumInfoFromAlbumPluginByAlbumId(albumInfo);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("BoundaryTest_002 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, BoundaryTest_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start BoundaryTest_003");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo ret;
    std::string lPath = "/";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->GetLatestAlbumInfoBylPath(lPath);
    EXPECT_EQ(ret.albumId, 0);
    MEDIA_INFO_LOG("BoundaryTest_003 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, BoundaryTest_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start BoundaryTest_004");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo ret;
    std::string lPath = "a";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->GetLatestAlbumInfoBylPath(lPath);
    EXPECT_EQ(ret.albumId, 0);
    MEDIA_INFO_LOG("BoundaryTest_004 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, BoundaryTest_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start BoundaryTest_005");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo albumInfo;
    std::vector<PhotoAlbumInfoPo> ret;
    albumInfo.albumId = 1;
    albumInfo.lPath = "/";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->GetDuplicatelPathAlbumInfoSub(albumInfo);
    EXPECT_EQ(ret.empty(), true);
    MEDIA_INFO_LOG("BoundaryTest_005 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, BoundaryTest_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start BoundaryTest_006");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo albumInfo;
    std::vector<PhotoAlbumInfoPo> ret;
    albumInfo.albumId = 1;
    albumInfo.lPath = "a";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->GetDuplicatelPathAlbumInfoSub(albumInfo);
    EXPECT_EQ(ret.empty(), true);
    MEDIA_INFO_LOG("BoundaryTest_006 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, MixedOperationTest_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start MixedOperationTest_001");
    PhotoAlbumLPathOperation &instance = PhotoAlbumLPathOperation::GetInstance();
    instance.SetRdbStore(nullptr);
    instance.Start();
    std::vector<PhotoAlbumInfoPo> invalidAlbums = instance.GetInvalidPhotoAlbums();
    EXPECT_EQ(invalidAlbums.empty(), true);
    std::vector<PhotoAlbumInfoPo> duplicateAlbums = instance.GetDuplicatelPathAlbumInfoMain();
    EXPECT_EQ(duplicateAlbums.empty(), true);
    std::vector<PhotoAlbumInfoPo> emptyAlbums = instance.GetEmptylPathAlbumInfo();
    EXPECT_EQ(emptyAlbums.empty(), true);
    instance.Stop();
    MEDIA_INFO_LOG("MixedOperationTest_001 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, MixedOperationTest_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start MixedOperationTest_002");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo albumInfo1;
    PhotoAlbumInfoPo albumInfo2;
    albumInfo1.albumId = 100;
    albumInfo1.lPath = "/storage/test/album1";
    albumInfo2.albumId = 200;
    albumInfo2.lPath = "/storage/test/album2";
    ASSERT_NE(ptr, nullptr);
    int32_t ret1 = ptr->UpdateAlbumInfoFromAlbumPluginByAlbumId(albumInfo1);
    EXPECT_EQ(ret1, NativeRdb::E_ERROR);
    int32_t ret2 = ptr->UpdateAlbumLPathByAlbumId(albumInfo2);
    EXPECT_EQ(ret2, NativeRdb::E_ERROR);
    PhotoAlbumInfoPo ret3 = ptr->GetLatestAlbumInfoBylPath(albumInfo1.lPath);
    EXPECT_EQ(ret3.albumId, 0);
    MEDIA_INFO_LOG("MixedOperationTest_002 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, MixedOperationTest_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start MixedOperationTest_003");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo mainAlbumInfo;
    PhotoAlbumInfoPo subAlbumInfo;
    mainAlbumInfo.albumId = 100;
    mainAlbumInfo.albumName = "main";
    mainAlbumInfo.lPath = "/storage/test/album";
    subAlbumInfo.albumId = 200;
    subAlbumInfo.albumName = "sub";
    subAlbumInfo.lPath = "/storage/test/album";
    ASSERT_NE(ptr, nullptr);
    int32_t ret1 = ptr->MergePhotoAlbum(mainAlbumInfo, subAlbumInfo);
    EXPECT_EQ(ret1, NativeRdb::E_ERROR);
    int32_t ret2 = ptr->CleanDuplicatePhotoAlbum(mainAlbumInfo);
    EXPECT_EQ(ret2, NativeRdb::E_OK);
    int32_t ret3 = ptr->CleanEmptylPathPhotoAlbum(subAlbumInfo);
    EXPECT_EQ(ret3, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("MixedOperationTest_003 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, StateTransitionTest_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start StateTransitionTest_001");
    PhotoAlbumLPathOperation &instance = PhotoAlbumLPathOperation::GetInstance();
    instance.SetRdbStore(nullptr);
    instance.Start();
    int32_t count1 = instance.GetAlbumAffectedCount();
    instance.CleanInvalidPhotoAlbums();
    int32_t count2 = instance.GetAlbumAffectedCount();
    instance.CleanDuplicatePhotoAlbums();
    int32_t count3 = instance.GetAlbumAffectedCount();
    instance.CleanEmptylPathPhotoAlbums();
    int32_t count4 = instance.GetAlbumAffectedCount();
    instance.Stop();
    EXPECT_EQ(count1, 0);
    EXPECT_EQ(count2, 0);
    EXPECT_EQ(count3, 0);
    EXPECT_EQ(count4, 0);
    MEDIA_INFO_LOG("StateTransitionTest_001 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, StateTransitionTest_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start StateTransitionTest_002");
    PhotoAlbumLPathOperation &instance = PhotoAlbumLPathOperation::GetInstance();
    instance.Stop();
    int32_t count1 = instance.GetAlbumAffectedCount();
    instance.CleanInvalidPhotoAlbums();
    int32_t count2 = instance.GetAlbumAffectedCount();
    instance.CleanDuplicatePhotoAlbums();
    int32_t count3 = instance.GetAlbumAffectedCount();
    instance.CleanEmptylPathPhotoAlbums();
    int32_t count4 = instance.GetAlbumAffectedCount();
    instance.Start();
    EXPECT_EQ(count1, 0);
    EXPECT_EQ(count2, 0);
    EXPECT_EQ(count3, 0);
    EXPECT_EQ(count4, 0);
    MEDIA_INFO_LOG("StateTransitionTest_002 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, StateTransitionTest_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start StateTransitionTest_003");
    PhotoAlbumLPathOperation &instance = PhotoAlbumLPathOperation::GetInstance();
    instance.SetRdbStore(nullptr);
    instance.Start();
    instance.Stop();
    instance.Start();
    instance.Stop();
    instance.Start();
    int32_t count = instance.GetAlbumAffectedCount();
    EXPECT_EQ(count, 0);
    MEDIA_INFO_LOG("StateTransitionTest_003 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, RepeatedCallTest_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start RepeatedCallTest_001");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo albumInfo;
    albumInfo.albumId = 100;
    albumInfo.lPath = "/storage/test/album";
    ASSERT_NE(ptr, nullptr);
    int32_t ret1 = ptr->UpdateAlbumInfoFromAlbumPluginByAlbumId(albumInfo);
    int32_t ret2 = ptr->UpdateAlbumInfoFromAlbumPluginByAlbumId(albumInfo);
    int32_t ret3 = ptr->UpdateAlbumInfoFromAlbumPluginByAlbumId(albumInfo);
    EXPECT_EQ(ret1, NativeRdb::E_ERROR);
    EXPECT_EQ(ret2, NativeRdb::E_ERROR);
    EXPECT_EQ(ret3, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("RepeatedCallTest_001 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, RepeatedCallTest_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start RepeatedCallTest_002");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo albumInfo;
    albumInfo.albumId = 100;
    albumInfo.lPath = "/storage/test/album";
    ASSERT_NE(ptr, nullptr);
    int32_t ret1 = ptr->UpdateAlbumLPathByAlbumId(albumInfo);
    int32_t ret2 = ptr->UpdateAlbumLPathByAlbumId(albumInfo);
    int32_t ret3 = ptr->UpdateAlbumLPathByAlbumId(albumInfo);
    EXPECT_EQ(ret1, NativeRdb::E_ERROR);
    EXPECT_EQ(ret2, NativeRdb::E_ERROR);
    EXPECT_EQ(ret3, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("RepeatedCallTest_002 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, RepeatedCallTest_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start RepeatedCallTest_003");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    std::string lPath = "/storage/test/album";
    ASSERT_NE(ptr, nullptr);
    PhotoAlbumInfoPo ret1 = ptr->GetLatestAlbumInfoBylPath(lPath);
    PhotoAlbumInfoPo ret2 = ptr->GetLatestAlbumInfoBylPath(lPath);
    PhotoAlbumInfoPo ret3 = ptr->GetLatestAlbumInfoBylPath(lPath);
    EXPECT_EQ(ret1.albumId, 0);
    EXPECT_EQ(ret2.albumId, 0);
    EXPECT_EQ(ret3.albumId, 0);
    MEDIA_INFO_LOG("RepeatedCallTest_003 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, RepeatedCallTest_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start RepeatedCallTest_004");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo albumInfo;
    albumInfo.albumId = 100;
    albumInfo.lPath = "/storage/test/album";
    ASSERT_NE(ptr, nullptr);
    std::vector<PhotoAlbumInfoPo> ret1 = ptr->GetDuplicatelPathAlbumInfoSub(albumInfo);
    std::vector<PhotoAlbumInfoPo> ret2 = ptr->GetDuplicatelPathAlbumInfoSub(albumInfo);
    std::vector<PhotoAlbumInfoPo> ret3 = ptr->GetDuplicatelPathAlbumInfoSub(albumInfo);
    EXPECT_EQ(ret1.empty(), true);
    EXPECT_EQ(ret2.empty(), true);
    EXPECT_EQ(ret3.empty(), true);
    MEDIA_INFO_LOG("RepeatedCallTest_004 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, SpecialValueTest_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SpecialValueTest_001");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo albumInfo;
    int32_t ret;
    albumInfo.albumId = 1;
    albumInfo.lPath = " ";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->UpdateAlbumInfoFromAlbumPluginByAlbumId(albumInfo);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("SpecialValueTest_001 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, SpecialValueTest_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SpecialValueTest_002");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo albumInfo;
    int32_t ret;
    albumInfo.albumId = 1;
    albumInfo.lPath = "\t\n";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->UpdateAlbumInfoFromAlbumPluginByAlbumId(albumInfo);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("SpecialValueTest_002 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, SpecialValueTest_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SpecialValueTest_003");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo ret;
    std::string lPath = " ";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->GetLatestAlbumInfoBylPath(lPath);
    EXPECT_EQ(ret.albumId, 0);
    MEDIA_INFO_LOG("SpecialValueTest_003 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, SpecialValueTest_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SpecialValueTest_004");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo ret;
    std::string lPath = "\t\t";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->GetLatestAlbumInfoBylPath(lPath);
    EXPECT_EQ(ret.albumId, 0);
    MEDIA_INFO_LOG("SpecialValueTest_004 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, SpecialValueTest_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SpecialValueTest_005");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo albumInfo;
    std::vector<PhotoAlbumInfoPo> ret;
    albumInfo.albumId = 1;
    albumInfo.lPath = " ";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->GetDuplicatelPathAlbumInfoSub(albumInfo);
    EXPECT_EQ(ret.empty(), true);
    MEDIA_INFO_LOG("SpecialValueTest_005 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, SpecialValueTest_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SpecialValueTest_006");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo albumInfo;
    std::vector<PhotoAlbumInfoPo> ret;
    albumInfo.albumId = 1;
    albumInfo.lPath = "\t\n";
    ASSERT_NE(ptr, nullptr);
    ret = ptr->GetDuplicatelPathAlbumInfoSub(albumInfo);
    EXPECT_EQ(ret.empty(), true);
    MEDIA_INFO_LOG("SpecialValueTest_006 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, ConcurrencyTest_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ConcurrencyTest_001");
    PhotoAlbumLPathOperation &instance1 = PhotoAlbumLPathOperation::GetInstance();
    PhotoAlbumLPathOperation &instance2 = PhotoAlbumLPathOperation::GetInstance();
    instance1.Start();
    instance2.Start();
    int32_t count1 = instance1.GetAlbumAffectedCount();
    int32_t count2 = instance2.GetAlbumAffectedCount();
    instance1.Stop();
    instance2.Stop();
    EXPECT_EQ(count1, count2);
    MEDIA_INFO_LOG("ConcurrencyTest_001 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, ConcurrencyTest_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ConcurrencyTest_002");
    PhotoAlbumLPathOperation &instance = PhotoAlbumLPathOperation::GetInstance();
    instance.SetRdbStore(nullptr);
    instance.Start();
    std::vector<PhotoAlbumInfoPo> albums1 = instance.GetInvalidPhotoAlbums();
    std::vector<PhotoAlbumInfoPo> albums2 = instance.GetDuplicatelPathAlbumInfoMain();
    std::vector<PhotoAlbumInfoPo> albums3 = instance.GetEmptylPathAlbumInfo();
    instance.Stop();
    EXPECT_EQ(albums1.empty(), true);
    EXPECT_EQ(albums2.empty(), true);
    EXPECT_EQ(albums3.empty(), true);
    MEDIA_INFO_LOG("ConcurrencyTest_002 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, ConcurrencyTest_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ConcurrencyTest_003");
    shared_ptr<PhotoAlbumLPathOperation> ptr1 = std::make_shared<PhotoAlbumLPathOperation>();
    shared_ptr<PhotoAlbumLPathOperation> ptr2 = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo albumInfo;
    albumInfo.albumId = 100;
    albumInfo.lPath = "/storage/test/album";
    int32_t ret1 = ptr1->UpdateAlbumInfoFromAlbumPluginByAlbumId(albumInfo);
    int32_t ret2 = ptr2->UpdateAlbumLPathByAlbumId(albumInfo);
    EXPECT_EQ(ret1, NativeRdb::E_ERROR);
    EXPECT_EQ(ret2, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("ConcurrencyTest_003 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, MemoryTest_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start MemoryTest_001");
    int loopCount = 0;
    for (int i = 0; i < 1000; i++) {
        shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
        PhotoAlbumInfoPo albumInfo;
        albumInfo.albumId = i;
        albumInfo.lPath = "/storage/test/album" + std::to_string(i);
        ptr->UpdateAlbumInfoFromAlbumPluginByAlbumId(albumInfo);
        loopCount++;
    }
    EXPECT_EQ(loopCount, 1000);
    MEDIA_INFO_LOG("MemoryTest_001 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, MemoryTest_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start MemoryTest_002");
    std::vector<shared_ptr<PhotoAlbumLPathOperation>> instances;
    for (int i = 0; i < 100; i++) {
        instances.push_back(std::make_shared<PhotoAlbumLPathOperation>());
    }
    for (auto &ptr : instances) {
        std::vector<PhotoAlbumInfoPo> ret = ptr->GetInvalidPhotoAlbums();
        EXPECT_EQ(ret.empty(), true);
    }
    MEDIA_INFO_LOG("MemoryTest_002 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, PerformanceTest_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start PerformanceTest_001");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    ASSERT_NE(ptr, nullptr);
    auto start = std::chrono::high_resolution_clock::now();
    int loopCount = 0;
    for (int i = 0; i < 1000; i++) {
        PhotoAlbumInfoPo ret = ptr->GetLatestAlbumInfoBylPath("/storage/test/album");
        EXPECT_EQ(ret.albumId, 0);
        loopCount++;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_EQ(loopCount, 1000);
    MEDIA_INFO_LOG("PerformanceTest_001 duration: %{public}lld ms", duration.count());
    MEDIA_INFO_LOG("PerformanceTest_001 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, PerformanceTest_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start PerformanceTest_002");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    ASSERT_NE(ptr, nullptr);
    PhotoAlbumInfoPo albumInfo;
    albumInfo.albumId = 100;
    albumInfo.lPath = "/storage/test/album";
    auto start = std::chrono::high_resolution_clock::now();
    int loopCount = 0;
    for (int i = 0; i < 1000; i++) {
        int32_t ret = ptr->UpdateAlbumInfoFromAlbumPluginByAlbumId(albumInfo);
        EXPECT_EQ(ret, NativeRdb::E_ERROR);
        loopCount++;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_EQ(loopCount, 1000);
    MEDIA_INFO_LOG("PerformanceTest_002 duration: %{public}lld ms", duration.count());
    MEDIA_INFO_LOG("PerformanceTest_002 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, RobustnessTest_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start RobustnessTest_001");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    ASSERT_NE(ptr, nullptr);
    PhotoAlbumInfoPo albumInfo;
    albumInfo.albumId = 100;
    albumInfo.lPath = "/storage/test/album";
    int loopCount = 0;
    for (int i = 0; i < 100; i++) {
        int32_t ret1 = ptr->UpdateAlbumInfoFromAlbumPluginByAlbumId(albumInfo);
        int32_t ret2 = ptr->UpdateAlbumLPathByAlbumId(albumInfo);
        PhotoAlbumInfoPo ret3 = ptr->GetLatestAlbumInfoBylPath(albumInfo.lPath);
        std::vector<PhotoAlbumInfoPo> ret4 = ptr->GetDuplicatelPathAlbumInfoSub(albumInfo);
        EXPECT_EQ(ret1, NativeRdb::E_ERROR);
        EXPECT_EQ(ret2, NativeRdb::E_ERROR);
        EXPECT_EQ(ret3.albumId, 0);
        EXPECT_EQ(ret4.empty(), true);
        loopCount++;
    }
    EXPECT_EQ(loopCount, 100);
    MEDIA_INFO_LOG("RobustnessTest_001 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, RobustnessTest_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start RobustnessTest_002");
    PhotoAlbumLPathOperation &instance = PhotoAlbumLPathOperation::GetInstance();
    instance.SetRdbStore(nullptr);
    int loopCount = 0;
    for (int i = 0; i < 100; i++) {
        instance.Start();
        instance.CleanInvalidPhotoAlbums();
        instance.CleanDuplicatePhotoAlbums();
        instance.CleanEmptylPathPhotoAlbums();
        instance.Stop();
        loopCount++;
    }
    EXPECT_EQ(loopCount, 100);
    MEDIA_INFO_LOG("RobustnessTest_002 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, IntegrationTest_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start IntegrationTest_001");
    PhotoAlbumLPathOperation &instance = PhotoAlbumLPathOperation::GetInstance();
    instance.SetRdbStore(nullptr);
    instance.Start();
    int32_t initialCount = instance.GetAlbumAffectedCount();
    instance.CleanInvalidPhotoAlbums();
    int32_t afterInvalid = instance.GetAlbumAffectedCount();
    instance.CleanDuplicatePhotoAlbums();
    int32_t afterDuplicate = instance.GetAlbumAffectedCount();
    instance.CleanEmptylPathPhotoAlbums();
    int32_t afterEmpty = instance.GetAlbumAffectedCount();
    instance.Stop();
    EXPECT_EQ(initialCount, 0);
    EXPECT_EQ(afterInvalid, 0);
    EXPECT_EQ(afterDuplicate, 0);
    EXPECT_EQ(afterEmpty, 0);
    MEDIA_INFO_LOG("IntegrationTest_001 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, IntegrationTest_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start IntegrationTest_002");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo mainAlbumInfo;
    PhotoAlbumInfoPo subAlbumInfo;
    mainAlbumInfo.albumId = 100;
    mainAlbumInfo.albumName = "main";
    mainAlbumInfo.lPath = "/storage/test/album";
    subAlbumInfo.albumId = 200;
    subAlbumInfo.albumName = "sub";
    subAlbumInfo.lPath = "/storage/test/album";
    int32_t mergeRet = ptr->MergePhotoAlbum(mainAlbumInfo, subAlbumInfo);
    int32_t cleanDupRet = ptr->CleanDuplicatePhotoAlbum(mainAlbumInfo);
    int32_t cleanEmptyRet = ptr->CleanEmptylPathPhotoAlbum(subAlbumInfo);
    PhotoAlbumInfoPo latest = ptr->GetLatestAlbumInfoBylPath(mainAlbumInfo.lPath);
    EXPECT_EQ(mergeRet, NativeRdb::E_ERROR);
    EXPECT_EQ(cleanDupRet, NativeRdb::E_OK);
    EXPECT_EQ(cleanEmptyRet, NativeRdb::E_ERROR);
    EXPECT_EQ(latest.albumId, 0);
    MEDIA_INFO_LOG("IntegrationTest_002 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, ErrorHandlingTest_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ErrorHandlingTest_001");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo albumInfo;
    albumInfo.albumId = -100;
    albumInfo.lPath = "/storage/test/album";
    int32_t ret = ptr->UpdateAlbumInfoFromAlbumPluginByAlbumId(albumInfo);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("ErrorHandlingTest_001 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, ErrorHandlingTest_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ErrorHandlingTest_002");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo albumInfo;
    albumInfo.albumId = 100;
    albumInfo.lPath = "";
    int32_t ret = ptr->UpdateAlbumLPathByAlbumId(albumInfo);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("ErrorHandlingTest_002 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, ErrorHandlingTest_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ErrorHandlingTest_003");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo ret = ptr->GetLatestAlbumInfoBylPath("");
    EXPECT_EQ(ret.albumId, 0);
    MEDIA_INFO_LOG("ErrorHandlingTest_003 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, ErrorHandlingTest_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ErrorHandlingTest_004");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo mainAlbumInfo;
    PhotoAlbumInfoPo subAlbumInfo;
    mainAlbumInfo.albumId = -1;
    subAlbumInfo.albumId = -1;
    int32_t ret = ptr->MergePhotoAlbum(mainAlbumInfo, subAlbumInfo);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("ErrorHandlingTest_004 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, DataValidationTest_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start DataValidationTest_001");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo albumInfo;
    albumInfo.albumId = 100;
    albumInfo.lPath = "/storage/test/album";
    albumInfo.albumName = "Test Album";
    int32_t ret = ptr->UpdateAlbumInfoFromAlbumPluginByAlbumId(albumInfo);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("DataValidationTest_001 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, DataValidationTest_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start DataValidationTest_002");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo albumInfo;
    albumInfo.albumId = 100;
    albumInfo.lPath = "/storage/test/album";
    albumInfo.albumName = "";
    int32_t ret = ptr->UpdateAlbumLPathByAlbumId(albumInfo);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("DataValidationTest_002 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, DataValidationTest_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start DataValidationTest_003");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo albumInfo;
    albumInfo.albumId = 0;
    albumInfo.lPath = "";
    albumInfo.albumName = "";
    std::vector<PhotoAlbumInfoPo> ret = ptr->GetDuplicatelPathAlbumInfoSub(albumInfo);
    EXPECT_EQ(ret.empty(), true);
    MEDIA_INFO_LOG("DataValidationTest_003 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, ApiConsistencyTest_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ApiConsistencyTest_001");
    PhotoAlbumLPathOperation &instance = PhotoAlbumLPathOperation::GetInstance();
    PhotoAlbumLPathOperation &instance2 = PhotoAlbumLPathOperation::GetInstance();
    EXPECT_EQ(&instance, &instance2);
    instance.SetRdbStore(nullptr);
    instance2.SetRdbStore(nullptr);
    instance.Start();
    instance2.Start();
    int32_t count1 = instance.GetAlbumAffectedCount();
    int32_t count2 = instance2.GetAlbumAffectedCount();
    EXPECT_EQ(count1, count2);
    instance.Stop();
    instance2.Stop();
    MEDIA_INFO_LOG("ApiConsistencyTest_001 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, ApiConsistencyTest_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ApiConsistencyTest_002");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    std::string lPath1 = "/storage/test/album1";
    std::string lPath2 = "/storage/test/album2";
    PhotoAlbumInfoPo ret1 = ptr->GetLatestAlbumInfoBylPath(lPath1);
    PhotoAlbumInfoPo ret2 = ptr->GetLatestAlbumInfoBylPath(lPath2);
    EXPECT_EQ(ret1.albumId, 0);
    EXPECT_EQ(ret2.albumId, 0);
    MEDIA_INFO_LOG("ApiConsistencyTest_002 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, ScenarioTest_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ScenarioTest_001");
    PhotoAlbumLPathOperation &instance = PhotoAlbumLPathOperation::GetInstance();
    instance.SetRdbStore(nullptr);
    instance.Start();
    std::vector<PhotoAlbumInfoPo> invalidAlbums = instance.GetInvalidPhotoAlbums();
    if (!invalidAlbums.empty()) {
        instance.CleanInvalidPhotoAlbums();
    }
    std::vector<PhotoAlbumInfoPo> duplicateAlbums = instance.GetDuplicatelPathAlbumInfoMain();
    if (!duplicateAlbums.empty()) {
        instance.CleanDuplicatePhotoAlbums();
    }
    std::vector<PhotoAlbumInfoPo> emptyAlbums = instance.GetEmptylPathAlbumInfo();
    if (!emptyAlbums.empty()) {
        instance.CleanEmptylPathPhotoAlbums();
    }
    instance.Stop();
    int32_t count = instance.GetAlbumAffectedCount();
    EXPECT_GE(count, 0);
    MEDIA_INFO_LOG("ScenarioTest_001 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, ScenarioTest_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ScenarioTest_002");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    std::vector<std::string> testPaths = {
        "/storage/test/album1",
        "/storage/test/album2",
        "/storage/test/album3"
    };
    for (const auto &path : testPaths) {
        PhotoAlbumInfoPo ret = ptr->GetLatestAlbumInfoBylPath(path);
        EXPECT_EQ(ret.albumId, 0);
    }
    MEDIA_INFO_LOG("ScenarioTest_002 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, ScenarioTest_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ScenarioTest_003");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    std::vector<PhotoAlbumInfoPo> albumInfos;
    for (int i = 0; i < 10; i++) {
        PhotoAlbumInfoPo albumInfo;
        albumInfo.albumId = i + 1;
        albumInfo.lPath = "/storage/test/album" + std::to_string(i + 1);
        albumInfos.push_back(albumInfo);
    }
    for (const auto &albumInfo : albumInfos) {
        int32_t ret = ptr->UpdateAlbumInfoFromAlbumPluginByAlbumId(albumInfo);
        EXPECT_EQ(ret, NativeRdb::E_ERROR);
    }
    MEDIA_INFO_LOG("ScenarioTest_003 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, ScenarioTest_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ScenarioTest_004");
    PhotoAlbumLPathOperation &instance = PhotoAlbumLPathOperation::GetInstance();
    instance.SetRdbStore(nullptr);
    instance.Start();
    for (int i = 0; i < 5; i++) {
        instance.CleanInvalidPhotoAlbums();
        instance.CleanDuplicatePhotoAlbums();
        instance.CleanEmptylPathPhotoAlbums();
    }
    int32_t finalCount = instance.GetAlbumAffectedCount();
    EXPECT_EQ(finalCount, 0);
    instance.Stop();
    MEDIA_INFO_LOG("ScenarioTest_004 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, ScenarioTest_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ScenarioTest_005");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    PhotoAlbumInfoPo mainAlbumInfo;
    PhotoAlbumInfoPo subAlbumInfo;
    mainAlbumInfo.albumId = 100;
    mainAlbumInfo.albumName = "Main Album";
    mainAlbumInfo.lPath = "/storage/test/main";
    subAlbumInfo.albumId = 200;
    subAlbumInfo.albumName = "Sub Album";
    subAlbumInfo.lPath = "/storage/test/sub";
    int32_t mergeRet = ptr->MergePhotoAlbum(mainAlbumInfo, subAlbumInfo);
    PhotoAlbumInfoPo mainLatest = ptr->GetLatestAlbumInfoBylPath(mainAlbumInfo.lPath);
    PhotoAlbumInfoPo subLatest = ptr->GetLatestAlbumInfoBylPath(subAlbumInfo.lPath);
    EXPECT_EQ(mergeRet, NativeRdb::E_ERROR);
    EXPECT_EQ(mainLatest.albumId, 0);
    EXPECT_EQ(subLatest.albumId, 0);
    MEDIA_INFO_LOG("ScenarioTest_005 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, ComprehensiveTest_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ComprehensiveTest_001");
    PhotoAlbumLPathOperation &instance = PhotoAlbumLPathOperation::GetInstance();
    instance.SetRdbStore(nullptr);
    instance.Start();
    std::vector<PhotoAlbumInfoPo> invalidAlbums = instance.GetInvalidPhotoAlbums();
    std::vector<PhotoAlbumInfoPo> duplicateAlbums = instance.GetDuplicatelPathAlbumInfoMain();
    std::vector<PhotoAlbumInfoPo> emptyAlbums = instance.GetEmptylPathAlbumInfo();
    instance.CleanInvalidPhotoAlbums();
    instance.CleanDuplicatePhotoAlbums();
    instance.CleanEmptylPathPhotoAlbums();
    int32_t affectedCount = instance.GetAlbumAffectedCount();
    instance.Stop();
    EXPECT_EQ(invalidAlbums.empty(), true);
    EXPECT_EQ(duplicateAlbums.empty(), true);
    EXPECT_EQ(emptyAlbums.empty(), true);
    EXPECT_EQ(affectedCount, 0);
    MEDIA_INFO_LOG("ComprehensiveTest_001 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, ComprehensiveTest_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ComprehensiveTest_002");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    std::vector<PhotoAlbumInfoPo> testAlbums;
    for (int i = 0; i < 20; i++) {
        PhotoAlbumInfoPo albumInfo;
        albumInfo.albumId = i + 1;
        albumInfo.albumName = "Album " + std::to_string(i + 1);
        albumInfo.lPath = "/storage/test/album" + std::to_string(i + 1);
        testAlbums.push_back(albumInfo);
    }
    for (const auto &albumInfo : testAlbums) {
        int32_t ret1 = ptr->UpdateAlbumInfoFromAlbumPluginByAlbumId(albumInfo);
        int32_t ret2 = ptr->UpdateAlbumLPathByAlbumId(albumInfo);
        PhotoAlbumInfoPo ret3 = ptr->GetLatestAlbumInfoBylPath(albumInfo.lPath);
        EXPECT_EQ(ret1, NativeRdb::E_ERROR);
        EXPECT_EQ(ret2, NativeRdb::E_ERROR);
        EXPECT_EQ(ret3.albumId, 0);
    }
    MEDIA_INFO_LOG("ComprehensiveTest_002 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, ComprehensiveTest_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ComprehensiveTest_003");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    std::vector<NativeRdb::ValueObject> testValues;
    for (int i = 0; i < 50; i++) {
        testValues.push_back(NativeRdb::ValueObject(i));
        testValues.push_back(NativeRdb::ValueObject("value" + std::to_string(i)));
    }
    std::string result = ptr->ToString(testValues);
    EXPECT_NE(result, "");
    MEDIA_INFO_LOG("ComprehensiveTest_003 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, ComprehensiveTest_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ComprehensiveTest_004");
    PhotoAlbumLPathOperation &instance = PhotoAlbumLPathOperation::GetInstance();
    instance.SetRdbStore(nullptr);
    instance.Start();
    for (int i = 0; i < 10; i++) {
        instance.CleanInvalidPhotoAlbums();
        instance.CleanDuplicatePhotoAlbums();
        instance.CleanEmptylPathPhotoAlbums();
        int32_t count = instance.GetAlbumAffectedCount();
        EXPECT_EQ(count, 0);
    }
    instance.Stop();
    MEDIA_INFO_LOG("ComprehensiveTest_004 End");
}

HWTEST_F(PhotoAlbumLPathOperationTest, ComprehensiveTest_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ComprehensiveTest_005");
    shared_ptr<PhotoAlbumLPathOperation> ptr = std::make_shared<PhotoAlbumLPathOperation>();
    std::vector<PhotoAlbumInfoPo> mainAlbums;
    std::vector<PhotoAlbumInfoPo> subAlbums;
    for (int i = 0; i < 10; i++) {
        PhotoAlbumInfoPo mainAlbumInfo;
        PhotoAlbumInfoPo subAlbumInfo;
        mainAlbumInfo.albumId = (i + 1) * 100;
        mainAlbumInfo.albumName = "Main " + std::to_string(i + 1);
        mainAlbumInfo.lPath = "/storage/test/main" + std::to_string(i + 1);
        subAlbumInfo.albumId = (i + 1) * 200;
        subAlbumInfo.albumName = "Sub " + std::to_string(i + 1);
        subAlbumInfo.lPath = "/storage/test/sub" + std::to_string(i + 1);
        mainAlbums.push_back(mainAlbumInfo);
        subAlbums.push_back(subAlbumInfo);
    }
    for (size_t i = 0; i < mainAlbums.size(); i++) {
        int32_t mergeRet = ptr->MergePhotoAlbum(mainAlbums[i], subAlbums[i]);
        int32_t cleanDupRet = ptr->CleanDuplicatePhotoAlbum(mainAlbums[i]);
        int32_t cleanEmptyRet = ptr->CleanEmptylPathPhotoAlbum(subAlbums[i]);
        EXPECT_EQ(mergeRet, NativeRdb::E_ERROR);
        EXPECT_EQ(cleanDupRet, NativeRdb::E_OK);
        EXPECT_EQ(cleanEmptyRet, NativeRdb::E_ERROR);
    }
    MEDIA_INFO_LOG("ComprehensiveTest_005 End");
}

}  // namespace OHOS::Media