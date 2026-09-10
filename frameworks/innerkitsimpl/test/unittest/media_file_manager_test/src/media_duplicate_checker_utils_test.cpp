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

#define MLOG_TAG "MediaDuplicateCheckerUtilsTest"

#include "media_duplicate_checker_utils_test.h"

#include "media_duplicate_checker_utils.h"
#include "medialibrary_errno.h"
#include "photo_album_column.h"
#include "userfile_manager_types.h"
#include "media_log.h"
#include "values_bucket.h"

#include <unistd.h>

namespace OHOS {
namespace Media {
using namespace testing::ext;
using namespace OHOS::NativeRdb;

void MediaDuplicateCheckerUtilsTest::SetUpTestCase() {}
void MediaDuplicateCheckerUtilsTest::TearDownTestCase() {}
void MediaDuplicateCheckerUtilsTest::SetUp() {}
void MediaDuplicateCheckerUtilsTest::TearDown() {}

// ===================== checkNameValidForMediaLibrary =====================

HWTEST_F(MediaDuplicateCheckerUtilsTest, CheckNameValid_ValidName_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckNameValid_ValidName_001 start");
    MediaDuplicateCheckerUtils checker;
    int32_t ret = checker.checkNameValidForMediaLibrary("valid_album_name");
    EXPECT_EQ(ret, 0);
    MEDIA_INFO_LOG("CheckNameValid_ValidName_001 end");
}

HWTEST_F(MediaDuplicateCheckerUtilsTest, CheckNameValid_InvalidSlash_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckNameValid_InvalidSlash_002 start");
    MediaDuplicateCheckerUtils checker;
    int32_t ret = checker.checkNameValidForMediaLibrary("invalid/name");
    EXPECT_NE(ret, 0);
    MEDIA_INFO_LOG("CheckNameValid_InvalidSlash_002 end");
}

HWTEST_F(MediaDuplicateCheckerUtilsTest, CheckNameValid_InvalidDot_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckNameValid_InvalidDot_003 start");
    MediaDuplicateCheckerUtils checker;
    int32_t ret = checker.checkNameValidForMediaLibrary("invalid.name");
    EXPECT_NE(ret, 0);
    MEDIA_INFO_LOG("CheckNameValid_InvalidDot_003 end");
}

HWTEST_F(MediaDuplicateCheckerUtilsTest, CheckNameValid_InvalidSpecialChars_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckNameValid_InvalidSpecialChars_004 start");
    MediaDuplicateCheckerUtils checker;
    EXPECT_NE(checker.checkNameValidForMediaLibrary(R"(test\test)"), 0);
    EXPECT_NE(checker.checkNameValidForMediaLibrary("test*test"), 0);
    EXPECT_NE(checker.checkNameValidForMediaLibrary("test?test"), 0);
    EXPECT_NE(checker.checkNameValidForMediaLibrary("test\"test"), 0);
    EXPECT_NE(checker.checkNameValidForMediaLibrary("test<test"), 0);
    EXPECT_NE(checker.checkNameValidForMediaLibrary("test|test"), 0);
    EXPECT_NE(checker.checkNameValidForMediaLibrary("test{test}"), 0);
    EXPECT_NE(checker.checkNameValidForMediaLibrary("test[test]"), 0);
    MEDIA_INFO_LOG("CheckNameValid_InvalidSpecialChars_004 end");
}

// ===================== getAlbumLpathByAlbumId =====================

HWTEST_F(MediaDuplicateCheckerUtilsTest, GetAlbumLpath_EmptyAlbumId_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetAlbumLpath_EmptyAlbumId_001 start");
    string path;
    int32_t ret = MediaDuplicateCheckerUtils::getAlbumLpathByAlbumId("", path);
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("GetAlbumLpath_EmptyAlbumId_001 end");
}

HWTEST_F(MediaDuplicateCheckerUtilsTest, GetAlbumLpath_NotExistAlbum_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetAlbumLpath_NotExistAlbum_002 start");
    string path;
    int32_t ret = MediaDuplicateCheckerUtils::getAlbumLpathByAlbumId("99999", path);
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("GetAlbumLpath_NotExistAlbum_002 end");
}

// ===================== getAlbumActualPathByAlbumId =====================

HWTEST_F(MediaDuplicateCheckerUtilsTest, GetActualPath_EmptyAlbumId_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetActualPath_EmptyAlbumId_001 start");
    string actualPath;
    int32_t ret = MediaDuplicateCheckerUtils::getAlbumActualPathByAlbumId("", actualPath);
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("GetActualPath_EmptyAlbumId_001 end");
}

HWTEST_F(MediaDuplicateCheckerUtilsTest, GetActualPath_NotExistAlbum_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetActualPath_NotExistAlbum_002 start");
    string actualPath;
    int32_t ret = MediaDuplicateCheckerUtils::getAlbumActualPathByAlbumId("99999", actualPath);
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("GetActualPath_NotExistAlbum_002 end");
}

// ===================== checkAlbumNameDuplicateInDB =====================

HWTEST_F(MediaDuplicateCheckerUtilsTest, CheckAlbumNameInDB_NoDuplicate_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckAlbumNameInDB_NoDuplicate_001 start");
    int32_t ret = MediaDuplicateCheckerUtils::checkAlbumNameDuplicateInDB("UniqueAlbumName");
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("CheckAlbumNameInDB_NoDuplicate_001 end");
}

// ===================== checkAlbumNameDuplicate =====================

HWTEST_F(MediaDuplicateCheckerUtilsTest, CheckAlbumNameDup_EmptyAlbumId_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckAlbumNameDup_EmptyAlbumId_001 start");
    int32_t ret = MediaDuplicateCheckerUtils::checkAlbumNameDuplicate("", "NewName");
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("CheckAlbumNameDup_EmptyAlbumId_001 end");
}

HWTEST_F(MediaDuplicateCheckerUtilsTest, CheckAlbumNameDup_NotExistAlbum_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckAlbumNameDup_NotExistAlbum_002 start");
    int32_t ret = MediaDuplicateCheckerUtils::checkAlbumNameDuplicate("99999", "NewName");
    EXPECT_EQ(ret, false);
    MEDIA_INFO_LOG("CheckAlbumNameDup_NotExistAlbum_002 end");
}

// ===================== checkDirectoryNameConflict =====================

HWTEST_F(MediaDuplicateCheckerUtilsTest, CheckDirConflict_NotFileManagerAlbum_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckDirConflict_NotFileManagerAlbum_001 start");
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, PhotoAlbumSubType::USER_GENERIC);
    int32_t ret = MediaDuplicateCheckerUtils::checkDirectoryNameConflict(values);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("CheckDirConflict_NotFileManagerAlbum_001 end");
}

HWTEST_F(MediaDuplicateCheckerUtilsTest, CheckDirConflict_EmptyAlbumName_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckDirConflict_EmptyAlbumName_002 start");
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILE_MANAGER);
    values.PutString(PhotoAlbumColumns::ALBUM_LPATH, "/FromDocs");
    int32_t ret = MediaDuplicateCheckerUtils::checkDirectoryNameConflict(values);
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("CheckDirConflict_EmptyAlbumName_002 end");
}

HWTEST_F(MediaDuplicateCheckerUtilsTest, CheckDirConflict_EmptyLPath_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckDirConflict_EmptyLPath_003 start");
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILE_MANAGER);
    values.PutString(PhotoAlbumColumns::ALBUM_NAME, "TestAlbum");
    int32_t ret = MediaDuplicateCheckerUtils::checkDirectoryNameConflict(values);
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("CheckDirConflict_EmptyLPath_003 end");
}

HWTEST_F(MediaDuplicateCheckerUtilsTest, CheckDirConflict_InvalidLPath_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckDirConflict_InvalidLPath_004 start");
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILE_MANAGER);
    values.PutString(PhotoAlbumColumns::ALBUM_NAME, "TestAlbum");
    values.PutString(PhotoAlbumColumns::ALBUM_LPATH, "/Pictures/invalid");
    int32_t ret = MediaDuplicateCheckerUtils::checkDirectoryNameConflict(values);
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("CheckDirConflict_InvalidLPath_004 end");
}

HWTEST_F(MediaDuplicateCheckerUtilsTest, CheckDirConflict_RootFromDocs_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckDirConflict_RootFromDocs_005 start");
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILE_MANAGER);
    values.PutString(PhotoAlbumColumns::ALBUM_NAME, "UniqueDirAlbum001");
    values.PutString(PhotoAlbumColumns::ALBUM_LPATH, "/FromDocs");
    int32_t ret = MediaDuplicateCheckerUtils::checkDirectoryNameConflict(values);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("CheckDirConflict_RootFromDocs_005 end");
}

HWTEST_F(MediaDuplicateCheckerUtilsTest, CheckDirConflict_RootLPath_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckDirConflict_RootLPath_006 start");
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILE_MANAGER);
    values.PutString(PhotoAlbumColumns::ALBUM_NAME, "UniqueDirAlbum002");
    values.PutString(PhotoAlbumColumns::ALBUM_LPATH, "/FromDocs/");
    int32_t ret = MediaDuplicateCheckerUtils::checkDirectoryNameConflict(values);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("CheckDirConflict_RootLPath_006 end");
}

HWTEST_F(MediaDuplicateCheckerUtilsTest, CheckDirConflict_SubLPath_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckDirConflict_SubLPath_007 start");
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILE_MANAGER);
    values.PutString(PhotoAlbumColumns::ALBUM_NAME, "UniqueDirAlbum003");
    values.PutString(PhotoAlbumColumns::ALBUM_LPATH, "/FromDocs/sub/deep");
    int32_t ret = MediaDuplicateCheckerUtils::checkDirectoryNameConflict(values);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("CheckDirConflict_SubLPath_007 end");
}

HWTEST_F(MediaDuplicateCheckerUtilsTest, CheckDirConflict_NoSubtype_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckDirConflict_NoSubtype_008 start");
    NativeRdb::ValuesBucket values;
    values.PutString(PhotoAlbumColumns::ALBUM_NAME, "TestAlbum");
    values.PutString(PhotoAlbumColumns::ALBUM_LPATH, "/FromDocs");
    int32_t ret = MediaDuplicateCheckerUtils::checkDirectoryNameConflict(values);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("CheckDirConflict_NoSubtype_008 end");
}

} // namespace Media
} // namespace OHOS
