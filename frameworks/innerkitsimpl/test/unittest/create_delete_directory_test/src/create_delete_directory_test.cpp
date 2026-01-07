/**
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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
#include "create_delete_directory_test.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>
#include <string>

#include "iservice_registry.h"
#include "media_log.h"
#include "media_volume.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "medialibrary_object_utils.h"
#include "medialibrary_unittest_utils.h"
#include "system_ability_definition.h"
#include "userfile_manager_types.h"

using namespace std;
using namespace testing::ext;
namespace OHOS {
namespace Media {
unsigned char g_fileContentJpg[] = {
    0x49, 0x44, 0x33, 0x03, 0x20, 0x20, 0x20, 0x0c, 0x24, 0x5d, 0x54, 0x45, 0x4e, 0x43, 0x20, 0x20, 0x20, 0x0b,
    0x20, 0x20, 0x20, 0x50
};

void CreateDeleteDirectory::SetUpTestCase()
{
    MEDIA_INFO_LOG("CreateDeleteDirectory::SetUpTestCase:: invoked");
    MediaLibraryUnitTestUtils::Init();
    MEDIA_INFO_LOG("CreateDeleteDirectory::SetUpTestCase:: Finish");
}

void CreateDeleteDirectory::TearDownTestCase() {}

void CreateDeleteDirectory::SetUp() {}
void CreateDeleteDirectory::TearDown(void) {}
static constexpr int32_t g_mediaDeleteRootDirError = -2010;
static constexpr const char *g_dataSharePath = "datashare:///media/file/";

static inline int32_t GetAlbumId(string &&relativePath)
{
    return MediaLibraryObjectUtils::GetIdByPathFromDb("/storage/cloud/files/" + relativePath);
}

string PathSplicing(string subpath, string path = MEDIA_DIROPRN)
{
    return (MEDIALIBRARY_DATA_URI + "/" + path + "/" + subpath);
}

static int32_t CreateDir(string &&relativePath)
{
    Uri createAssetUri(PathSplicing(MEDIA_DIROPRN_FMS_CREATEDIR));
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(MEDIA_DATA_DB_RELATIVE_PATH, relativePath);
    int32_t res = MediaLibraryDataManager::GetInstance()->Insert(createAssetUri, valuesBucket);
    if (res <= 0) {
        MEDIA_ERR_LOG("Failed to create directory, error: %{public}d", res);
    }
    return res;
}

static int32_t DeleteDir(string &relativePath)
{
    Uri deleteDirUri(PathSplicing(MEDIA_DIROPRN_FMS_DELETEDIR));
    DataShareValuesBucket deleteValuesBucket;
    deleteValuesBucket.Put(MEDIA_DATA_DB_URI, relativePath);
    int32_t res = MediaLibraryDataManager::GetInstance()->Insert(deleteDirUri, deleteValuesBucket);
    if (res <= 0) {
        MEDIA_ERR_LOG("Failed to delete directory, error: %{public}d", res);
    }
    return res;
}

static int32_t TrashDir(string &&testNum)
{
    DataShareValuesBucket valuesBucket;
    Uri createAssetUri(PathSplicing(MEDIA_FILEOPRN_CREATEASSET, MEDIA_FILEOPRN));
    valuesBucket.Put(MEDIA_DATA_DB_MEDIA_TYPE, MEDIA_TYPE_IMAGE);
    valuesBucket.Put(MEDIA_DATA_DB_NAME, testNum + ".jpg");
    string relativePath = "Pictures/" + testNum + "/";
    valuesBucket.Put(MEDIA_DATA_DB_RELATIVE_PATH, relativePath);
    int32_t index = MediaLibraryDataManager::GetInstance()->Insert(createAssetUri, valuesBucket);
    if (index <= 0) {
        MEDIA_ERR_LOG("Failed to Insert, error index: %{public}d", index);
        return E_FAIL;
    }

    Uri deleteDirUri(PathSplicing(MEDIA_DIROPRN_FMS_TRASHDIR));
    relativePath.pop_back();
    int32_t albumId = GetAlbumId("/storage/cloud/files/" + relativePath);
    if (albumId <= 0) {
        MEDIA_ERR_LOG("Failed to add album, error: %{public}d", albumId);
        return E_FAIL;
    }
    DataShareValuesBucket valuesBucket1;
    valuesBucket1.Put(MEDIA_DATA_DB_ID, albumId);
    int32_t res = MediaLibraryDataManager::GetInstance()->Insert(createAssetUri, valuesBucket1);
    if (res <= 0) {
        MEDIA_ERR_LOG("Failed to trash directory, error: %{public}d", res);
    }
    return res;
}

/**
 * @tc.number    : directory_test_001
 * @tc.name      : directory_test_001
 * @tc.desc      : 1. create directory: Pictures/test001/
 *                 2. delete directory: Pictures/testDelete
 */
HWTEST_F(CreateDeleteDirectory, directory_test_001, TestSize.Level1)
{
    int32_t createRes = CreateDir("Pictures/test001/");
    ASSERT_GT(createRes, 0);
    int32_t albumId = GetAlbumId("Pictures/testDelete");
    string uri = g_dataSharePath + std::to_string(albumId);
    int32_t deleteRes = DeleteDir(uri);
    ASSERT_GT(deleteRes, 0);
    MEDIA_INFO_LOG("directory_test_001::End");
}
/**
 * @tc.number    : directory_test_002
 * @tc.name      : directory_test_002
 * @tc.desc      : 1. get directory :Pictures
 *                 2. delete directory: Pictures
 */
HWTEST_F(CreateDeleteDirectory, directory_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("directory_test_002::Start");
    int32_t albumId = GetAlbumId("Pictures");
    string uri = g_dataSharePath + std::to_string(albumId);
    int32_t resFirst = DeleteDir(uri);
    ASSERT_EQ(resFirst, g_mediaDeleteRootDirError);
    ASSERT_GT(resFirst, 0);
    MEDIA_INFO_LOG("directory_test_002::End");
}
/**
 * @tc.number    : directory_test_003
 * @tc.name      : directory_test_003
 * @tc.desc      : 1. get directory :Videos
 *                 2. delete directory: Videos
 */
HWTEST_F(CreateDeleteDirectory, directory_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("directory_test_003::Start");
    int32_t albumId = GetAlbumId("Videos");
    string uri = g_dataSharePath + std::to_string(albumId);
    int32_t resFirst = DeleteDir(uri);
    ASSERT_EQ(resFirst, g_mediaDeleteRootDirError);
    ASSERT_GT(resFirst, 0);
    MEDIA_INFO_LOG("directory_test_003::End");
}
/**
 * @tc.number    : directory_test_004
 * @tc.name      : directory_test_004
 * @tc.desc      : 1. get directory :Audios
 *                 2. delete directory: Audios
 */
HWTEST_F(CreateDeleteDirectory, directory_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("directory_test_004::Start");
    int32_t albumId = GetAlbumId("Audios");
    string uri = g_dataSharePath + std::to_string(albumId);
    int32_t resFirst = DeleteDir(uri);
    ASSERT_EQ(resFirst, g_mediaDeleteRootDirError);
    ASSERT_GT(resFirst, 0);
    MEDIA_INFO_LOG("directory_test_004::End");
}
/**
 * @tc.number    : directory_test_005
 * @tc.name      : directory_test_005
 * @tc.desc      : 1. get directory :Documents
 *                 2. delete directory: Documents
 */
HWTEST_F(CreateDeleteDirectory, directory_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("directory_test_005::Start");
    int32_t albumId = GetAlbumId("Documents");
    string uri = g_dataSharePath + std::to_string(albumId);
    int32_t resFirst = DeleteDir(uri);
    ASSERT_GT(resFirst, 0);
    ASSERT_EQ(resFirst, g_mediaDeleteRootDirError);
    MEDIA_INFO_LOG("directory_test_005::End");
}
/**
 * @tc.number    : directory_test_006
 * @tc.name      : directory_test_006
 * @tc.desc      : 1. get directory :Download
 *                 2. delete directory: Download
 */
HWTEST_F(CreateDeleteDirectory, directory_test_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("directory_test_006::Start");
    int32_t albumId = GetAlbumId("Download");
    string uri = g_dataSharePath + std::to_string(albumId);
    int32_t resFirst = DeleteDir(uri);
    ASSERT_GT(resFirst, 0);
    ASSERT_EQ(resFirst, g_mediaDeleteRootDirError);
    MEDIA_INFO_LOG("directory_test_006::End");
}
/**
 * @tc.number    : directory_test_007
 * @tc.name      : directory_test_007
 * @tc.desc      : 1. create directory: Pictures/test007/
 *                 2. create directory: Pictures/test007/ fail
 *                 3. delete directory: Pictures/test007/
 */
HWTEST_F(CreateDeleteDirectory, directory_test_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("directory_test_007::Start");
    int32_t resFirst = CreateDir("Pictures/test007/");
    int32_t resSecond = CreateDir("Pictures/test007/");
    ASSERT_GE(resFirst, 0);
    ASSERT_LT(resSecond, 0);
    // clean
    int32_t albumId = GetAlbumId("Pictures/test007");
    string uri = g_dataSharePath + std::to_string(albumId);
    int32_t resDelete = DeleteDir(uri);
    ASSERT_GE(resDelete, 0);
    MEDIA_INFO_LOG("directory_test_007::End");
}
/**
 * @tc.number    : directory_test_008
 * @tc.name      : directory_test_008
 * @tc.desc      : 1. create directory: Pictures/test008
 *                 2. delete directory :Pictures/test008
 *                 3. delete directory: Pictures/test008
 */
HWTEST_F(CreateDeleteDirectory, directory_test_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("directory_test_008::Start");
    int32_t resCreate = CreateDir("Pictures/test008/");
    ASSERT_GE(resCreate, 0);
    int32_t albumId = GetAlbumId("Pictures/test008");
    string uri = g_dataSharePath + std::to_string(albumId);
    int32_t resFirst = DeleteDir(uri);
    int32_t resSecond = DeleteDir(uri);
    ASSERT_GE(resFirst, 0);
    ASSERT_LT(resSecond, 0);
    MEDIA_INFO_LOG("directory_test_008::End");
}
/**
 * @tc.number    : directory_test_009
 * @tc.name      : directory_test_009
 * @tc.desc      : 1. create directory: Videos/test009/
 *                 2. create directory: Videos/test009/
 *                 3. delete directory: Videos/test009/
 */
HWTEST_F(CreateDeleteDirectory, directory_test_009, TestSize.Level1)
{
    MEDIA_INFO_LOG("directory_test_009::Start");
    int32_t resFirst = CreateDir("Videos/test009/");
    int32_t resSecond = CreateDir("Videos/test009/");
    ASSERT_GE(resFirst, 0);
    ASSERT_LT(resSecond, 0);
    // clean
    int32_t albumId = GetAlbumId("Videos/test009");
    string uri = g_dataSharePath + std::to_string(albumId);
    int32_t resDelete = DeleteDir(uri);
    ASSERT_GE(resDelete, 0);
    MEDIA_INFO_LOG("directory_test_009::End");
}
/**
 * @tc.number    : directory_test_010
 * @tc.name      : directory_test_010
 * @tc.desc      : 1. create directory: Videos/test010
 *                 2. delete directory :Videos/test010
 *                 3. delete directory: Videos/test010
 */
HWTEST_F(CreateDeleteDirectory, directory_test_010, TestSize.Level1)
{
    MEDIA_INFO_LOG("directory_test_010::Start");
    int32_t resCreate = CreateDir("Videos/test010/");
    ASSERT_GE(resCreate, 0);
    int32_t albumId = GetAlbumId("Videos/test010");
    string uri = g_dataSharePath + std::to_string(albumId);
    int32_t resFirst = DeleteDir(uri);
    int32_t resSecond = DeleteDir(uri);
    ASSERT_GE(resFirst, 0);
    ASSERT_LT(resSecond, 0);
    MEDIA_INFO_LOG("directory_test_010::End");
}
/**
 * @tc.number    : directory_test_011
 * @tc.name      : directory_test_011
 * @tc.desc      : 1. create directory: Audios/test011/
 *                 2. create directory: Audios/test011/
 *                 3. delete directory: Audios/test011/
 */
HWTEST_F(CreateDeleteDirectory, directory_test_011, TestSize.Level1)
{
    MEDIA_INFO_LOG("directory_test_011::Start");
    int32_t resFirst = CreateDir("Audios/test011/");
    int32_t resSecond = CreateDir("Audios/test011/");
    ASSERT_GE(resFirst, 0);
    ASSERT_LT(resSecond, 0);
    int32_t albumId = GetAlbumId("Audios/test011");
    string uri = g_dataSharePath + std::to_string(albumId);
    int32_t resDelete = DeleteDir(uri);
    ASSERT_GE(resDelete, 0);
    MEDIA_INFO_LOG("directory_test_011::End");
}
/**
 * @tc.number    : directory_test_012
 * @tc.name      : directory_test_012
 * @tc.desc      : 1. create directory: Audios/test012
 *                 2. delete directory :Audios/test012
 *                 3. delete directory: Audios/test012
 */
HWTEST_F(CreateDeleteDirectory, directory_test_012, TestSize.Level1)
{
    MEDIA_INFO_LOG("directory_test_012::Start");
    int32_t resCreate = CreateDir("Audios/test012/");
    ASSERT_GE(resCreate, 0);
    int32_t albumId = GetAlbumId("Audios/test012");
    string uri = g_dataSharePath + std::to_string(albumId);
    int32_t resFirst = DeleteDir(uri);
    int32_t resSecond = DeleteDir(uri);
    ASSERT_GE(resFirst, 0);
    ASSERT_LT(resSecond, 0);
    MEDIA_INFO_LOG("directory_test_012::End");
}
/**
 * @tc.number    : directory_test_013
 * @tc.name      : directory_test_013
 * @tc.desc      : 1. create directory: Documents/test013/
 *                 2. create directory: Documents/test013/
 *                 3. delete directory: Documents/test013/
 */
HWTEST_F(CreateDeleteDirectory, directory_test_013, TestSize.Level1)
{
    MEDIA_INFO_LOG("directory_test_013::Start");
    int32_t resFirst = CreateDir("Documents/test013/");
    int32_t resSecond = CreateDir("Documents/test013/");
    ASSERT_GE(resFirst, 0);
    ASSERT_LT(resSecond, 0);
    // clean
    int32_t albumId = GetAlbumId("Documents/test013");
    string uri = g_dataSharePath + std::to_string(albumId);
    int32_t resDelete = DeleteDir(uri);
    ASSERT_GE(resDelete, 0);
    MEDIA_INFO_LOG("directory_test_013::End");
}
/**
 * @tc.number    : directory_test_014
 * @tc.name      : directory_test_014
 * @tc.desc      : 1. create directory: Documents/test014
 *                 2. delete directory :Documents/test014
 *                 3. delete directory: Documents/test014
 */
HWTEST_F(CreateDeleteDirectory, directory_test_014, TestSize.Level1)
{
    MEDIA_INFO_LOG("directory_test_014::Start");
    int32_t resCreate = CreateDir("Documents/test014/");
    ASSERT_GE(resCreate, 0);
    int32_t albumId = GetAlbumId("Documents/test014");
    string uri = g_dataSharePath + std::to_string(albumId);
    int32_t resFirst = DeleteDir(uri);
    int32_t resSecond = DeleteDir(uri);
    ASSERT_GE(resFirst, 0);
    ASSERT_LT(resSecond, 0);
    MEDIA_INFO_LOG("directory_test_014::End");
}
/**
 * @tc.number    : directory_test_015
 * @tc.name      : directory_test_015
 * @tc.desc      : 1. create directory: Download/test015/
 *                 2. create directory: Download/test015/
 *                 3. delete directory: Download/test015/
 */
HWTEST_F(CreateDeleteDirectory, directory_test_015, TestSize.Level1)
{
    MEDIA_INFO_LOG("directory_test_015::Start");
    int32_t resFirst = CreateDir("Download/test015/");
    int32_t resSecond = CreateDir("Download/test015/");
    ASSERT_GE(resFirst, 0);
    ASSERT_LT(resSecond, 0);
    // clean
    int32_t albumId = GetAlbumId("Download/test015");
    string uri = g_dataSharePath + std::to_string(albumId);
    int32_t resDelete = DeleteDir(uri);
    ASSERT_GE(resDelete, 0);
    MEDIA_INFO_LOG("directory_test_015::End");
}
/**
 * @tc.number    : directory_test_016
 * @tc.name      : directory_test_016
 * @tc.desc      : 1. create directory: Download/test016
 *                 2. delete directory :Download/test016
 *                 3. delete directory: Download/test016
 */
HWTEST_F(CreateDeleteDirectory, directory_test_016, TestSize.Level1)
{
    MEDIA_INFO_LOG("directory_test_016::Start");
    int32_t resCreate = CreateDir("Download/test016/");
    ASSERT_GE(resCreate, 0);
    int32_t albumId = GetAlbumId("Download/test016");
    string uri = g_dataSharePath + std::to_string(albumId);
    int32_t resFirst = DeleteDir(uri);
    int32_t resSecond = DeleteDir(uri);
    ASSERT_GE(resFirst, 0);
    ASSERT_LT(resSecond, 0);
    MEDIA_INFO_LOG("directory_test_016::End");
}
/**
 * @tc.number    : directory_test_017
 * @tc.name      : directory_test_017
 * @tc.desc      : 1. create directory parameter is ""
 */
HWTEST_F(CreateDeleteDirectory, directory_test_017, TestSize.Level1)
{
    MEDIA_INFO_LOG("directory_test_017::Start");
    int32_t createRes = CreateDir("");
    ASSERT_LT(createRes, 0);
    MEDIA_INFO_LOG("directory_test_017::End");
}
/**
 * @tc.number    : directory_test_018
 * @tc.name      : directory_test_018
 * @tc.desc      : 1. create directory parameter illegal : test_018
 */
HWTEST_F(CreateDeleteDirectory, directory_test_018, TestSize.Level1)
{
    MEDIA_INFO_LOG("directory_test_018::Start");
    int32_t createRes = CreateDir("test_018");
    ASSERT_LT(createRes, 0);
    MEDIA_INFO_LOG("directory_test_018::End");
}
/**
 * @tc.number    : directory_test_019
 * @tc.name      : directory_test_019
 * @tc.desc      : 1. delete directory parameter dose not exist
 */
HWTEST_F(CreateDeleteDirectory, directory_test_019, TestSize.Level1)
{
    MEDIA_INFO_LOG("directory_test_019::Start");
    string uri = "not_exist";
    int32_t deleteRes = DeleteDir(uri);
    ASSERT_LT(deleteRes, 0);
    MEDIA_INFO_LOG("directory_test_019::End");
}
/**
 * @tc.number    : directory_test_020
 * @tc.name      : directory_test_020
 * @tc.desc      : 1. dir trash
 */
HWTEST_F(CreateDeleteDirectory, directory_test_020, TestSize.Level1)
{
    MEDIA_INFO_LOG("directory_test_020::Start");
    int32_t trashRes = TrashDir("directory_test_020");
    ASSERT_NE(trashRes, 0);
    MEDIA_INFO_LOG("directory_test_020::End");
}
} // namespace Media
} // namespace OHOS