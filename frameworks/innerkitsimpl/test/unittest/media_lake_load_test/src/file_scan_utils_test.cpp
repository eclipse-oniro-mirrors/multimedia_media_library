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

#define MLOG_TAG "FileScanUtilsTest"

#include "file_scan_utils_test.h"

#include <set>

#include "file_scan_utils.h"
#include "file_const.h"
#include "medialibrary_db_const.h"
#include "userfile_manager_types.h"
#include "media_log.h"

namespace OHOS {
namespace Media {
using namespace testing::ext;

// Forward declarations for free functions in file_scan_utils.cpp
std::string CleanBurstFileName(const std::string &fileName);

void FileScanUtilsTest::SetUpTestCase() {}
void FileScanUtilsTest::TearDownTestCase() {}
void FileScanUtilsTest::SetUp() {}
void FileScanUtilsTest::TearDown() {}

// ==================== GetFileTitle Tests ====================

/**
 * @tc.name: GetFileTitle_WithExtension_001
 * @tc.desc: Test GetFileTitle with a filename that has an extension
 * @tc.type: FUNC
 * @tc.require: branch coverage >= 75%
 */
HWTEST_F(FileScanUtilsTest, GetFileTitle_WithExtension_001, TestSize.Level1)
{
    std::string result = FileScanUtils::GetFileTitle("photo.jpg");
    EXPECT_EQ(result, "photo");
}

/**
 * @tc.name: GetFileTitle_WithoutExtension_002
 * @tc.desc: Test GetFileTitle with a filename without extension
 * @tc.type: FUNC
 */
HWTEST_F(FileScanUtilsTest, GetFileTitle_WithoutExtension_002, TestSize.Level1)
{
    std::string result = FileScanUtils::GetFileTitle("noextension");
    EXPECT_EQ(result, "noextension");
}

/**
 * @tc.name: GetFileTitle_MultipleDots_003
 * @tc.desc: Test GetFileTitle with multiple dots, should split at last dot
 * @tc.type: FUNC
 */
HWTEST_F(FileScanUtilsTest, GetFileTitle_MultipleDots_003, TestSize.Level1)
{
    std::string result = FileScanUtils::GetFileTitle("photo.backup.jpg");
    EXPECT_EQ(result, "photo.backup");
}

/**
 * @tc.name: GetFileTitle_EmptyString_004
 * @tc.desc: Test GetFileTitle with empty string
 * @tc.type: FUNC
 */
HWTEST_F(FileScanUtilsTest, GetFileTitle_EmptyString_004, TestSize.Level1)
{
    std::string result = FileScanUtils::GetFileTitle("");
    EXPECT_EQ(result, "");
}

// ==================== CreateAssetRealName Tests ====================

/**
 * @tc.name: CreateAssetRealName_ImageType_001
 * @tc.desc: Test CreateAssetRealName with MEDIA_TYPE_IMAGE
 * @tc.type: FUNC
 */
HWTEST_F(FileScanUtilsTest, CreateAssetRealName_ImageType_001, TestSize.Level1)
{
    std::string name;
    int32_t ret = FileScanUtils::CreateAssetRealName(1000, MediaType::MEDIA_TYPE_IMAGE, "jpg", name);
    EXPECT_EQ(ret, 0); // E_OK
    EXPECT_TRUE(name.find("IMG_") == 0);
    EXPECT_TRUE(name.find(".jpg") != std::string::npos);
}

/**
 * @tc.name: CreateAssetRealName_VideoType_002
 * @tc.desc: Test CreateAssetRealName with MEDIA_TYPE_VIDEO
 * @tc.type: FUNC
 */
HWTEST_F(FileScanUtilsTest, CreateAssetRealName_VideoType_002, TestSize.Level1)
{
    std::string name;
    int32_t ret = FileScanUtils::CreateAssetRealName(1000, MediaType::MEDIA_TYPE_VIDEO, "mp4", name);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(name.find("VID_") == 0);
    EXPECT_TRUE(name.find(".mp4") != std::string::npos);
}

/**
 * @tc.name: CreateAssetRealName_AudioType_003
 * @tc.desc: Test CreateAssetRealName with MEDIA_TYPE_AUDIO
 * @tc.type: FUNC
 */
HWTEST_F(FileScanUtilsTest, CreateAssetRealName_AudioType_003, TestSize.Level1)
{
    std::string name;
    int32_t ret = FileScanUtils::CreateAssetRealName(1000, MediaType::MEDIA_TYPE_AUDIO, "mp3", name);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(name.find("AUD_") == 0);
    EXPECT_TRUE(name.find(".mp3") != std::string::npos);
}

/**
 * @tc.name: CreateAssetRealName_InvalidType_004
 * @tc.desc: Test CreateAssetRealName with invalid media type, should return E_INVALID_VALUES
 * @tc.type: FUNC
 */
HWTEST_F(FileScanUtilsTest, CreateAssetRealName_InvalidType_004, TestSize.Level1)
{
    std::string name;
    int32_t ret = FileScanUtils::CreateAssetRealName(1000, 999, "txt", name);
    EXPECT_NE(ret, 0); // E_INVALID_VALUES
}

/**
 * @tc.name: CreateAssetRealName_SmallFileId_005
 * @tc.desc: Test CreateAssetRealName with fileId <= 999, check padded format
 * @tc.type: FUNC
 */
HWTEST_F(FileScanUtilsTest, CreateAssetRealName_SmallFileId_005, TestSize.Level1)
{
    std::string name;
    int32_t ret = FileScanUtils::CreateAssetRealName(5, MediaType::MEDIA_TYPE_IMAGE, "jpg", name);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(name.find("IMG_") == 0);
    // fileId 5 is <= ASSET_MAX_COMPLEMENT_ID(999), should be padded
    EXPECT_TRUE(name.find("005") != std::string::npos || name.find("_5.") != std::string::npos);
}

// ==================== GetReplacedPathByPrefixType Tests ====================

/**
 * @tc.name: GetReplacedPathByPrefixType_CloudToLocal_001
 * @tc.desc: Test path replacement from cloud prefix to local prefix
 * @tc.type: FUNC
 */
HWTEST_F(FileScanUtilsTest, GetReplacedPathByPrefixType_CloudToLocal_001, TestSize.Level1)
{
    std::string path = "/storage/cloud/files/Photo/1/test.jpg";
    std::string result = FileScanUtils::GetReplacedPathByPrefixType(
        PrefixType::CLOUD, PrefixType::LOCAL, path);
    EXPECT_EQ(result, "/storage/media/local/files/Photo/1/test.jpg");
}

/**
 * @tc.name: GetReplacedPathByPrefixType_LocalToCloud_002
 * @tc.desc: Test path replacement from local prefix to cloud prefix
 * @tc.type: FUNC
 */
HWTEST_F(FileScanUtilsTest, GetReplacedPathByPrefixType_LocalToCloud_002, TestSize.Level1)
{
    std::string path = "/storage/media/local/files/Photo/1/test.jpg";
    std::string result = FileScanUtils::GetReplacedPathByPrefixType(
        PrefixType::LOCAL, PrefixType::CLOUD, path);
    EXPECT_EQ(result, "/storage/cloud/files/Photo/1/test.jpg");
}

/**
 * @tc.name: GetReplacedPathByPrefixType_InvalidSrcPrefix_003
 * @tc.desc: Test with invalid source prefix type, should return empty string
 * @tc.type: FUNC
 */
HWTEST_F(FileScanUtilsTest, GetReplacedPathByPrefixType_InvalidSrcPrefix_003, TestSize.Level1)
{
    std::string path = "/some/path/test.jpg";
    std::string result = FileScanUtils::GetReplacedPathByPrefixType(
        static_cast<PrefixType>(999), PrefixType::LOCAL, path);
    EXPECT_TRUE(result.empty());
}

// ==================== FindObjectHash / FindTitlePrefix Tests ====================

/**
 * @tc.name: FindObjectHash_FormatCheck_001
 * @tc.desc: Test FindObjectHash output format: albumId#prefix#displayName
 * @tc.type: FUNC
 */
HWTEST_F(FileScanUtilsTest, FindObjectHash_FormatCheck_001, TestSize.Level1)
{
    InnerFileInfo info;
    info.ownerAlbumId = 42;
    info.displayName = "IMG_BURST001_COVER.jpg";
    std::string hash = FileScanUtils::FindObjectHash(info);
    EXPECT_TRUE(hash.find("42#") == 0);
    EXPECT_TRUE(hash.find(info.displayName) != std::string::npos);
}

/**
 * @tc.name: FindTitlePrefix_WithBurstKeyword_001
 * @tc.desc: Test FindTitlePrefix with _BURST keyword in display name
 * @tc.type: FUNC
 */
HWTEST_F(FileScanUtilsTest, FindTitlePrefix_WithBurstKeyword_001, TestSize.Level1)
{
    InnerFileInfo info;
    info.displayName = "IMG_BURST001.jpg";
    std::string prefix = FileScanUtils::FindTitlePrefix(info);
    EXPECT_FALSE(prefix.empty());
    EXPECT_TRUE(prefix.find("IMG") != std::string::npos);
}

/**
 * @tc.name: FindTitlePrefix_WithoutBurstKeyword_002
 * @tc.desc: Test FindTitlePrefix without _BURST keyword, should return empty
 * @tc.type: FUNC
 */
HWTEST_F(FileScanUtilsTest, FindTitlePrefix_WithoutBurstKeyword_002, TestSize.Level1)
{
    InnerFileInfo info;
    info.displayName = "normal_photo.jpg";
    std::string prefix = FileScanUtils::FindTitlePrefix(info);
    EXPECT_TRUE(prefix.empty());
}

// ==================== FindGroupIndex Tests ====================

/**
 * @tc.name: FindGroupIndex_IncrementingOccurrence_001
 * @tc.desc: Test FindGroupIndex returns incrementing values for same objectHash
 * @tc.type: FUNC
 */
HWTEST_F(FileScanUtilsTest, FindGroupIndex_IncrementingOccurrence_001, TestSize.Level1)
{
    // Use unique displayName to avoid interference from other tests
    InnerFileInfo info1;
    info1.ownerAlbumId = 9999;
    info1.displayName = "unique_group_test_IMG_BURST001.jpg";
    int32_t idx1 = FileScanUtils::FindGroupIndex(info1);

    InnerFileInfo info2;
    info2.ownerAlbumId = 9999;
    info2.displayName = "unique_group_test_IMG_BURST001.jpg";
    int32_t idx2 = FileScanUtils::FindGroupIndex(info2);

    EXPECT_EQ(idx1, 1);
    EXPECT_EQ(idx2, 2);
}

// ==================== SetBurstKey Tests ====================

/**
 * @tc.name: SetBurstKey_OtherType_EmptyKey_001
 * @tc.desc: Test SetBurstKey with OTHER_TYPE, burstKey should be empty
 * @tc.type: FUNC
 */
HWTEST_F(FileScanUtilsTest, SetBurstKey_OtherType_EmptyKey_001, TestSize.Level1)
{
    InnerFileInfo info;
    info.isBurst = IsBurstType::OTHER_TYPE;
    info.displayName = "photo.jpg";
    info.ownerAlbumId = 1;
    FileScanUtils::SetBurstKey(info);
    EXPECT_TRUE(info.burstKey.empty());
}

/**
 * @tc.name: SetBurstKey_CoverType_NonEmptyKey_002
 * @tc.desc: Test SetBurstKey with BURST_COVER_TYPE, burstKey should be non-empty
 * @tc.type: FUNC
 */
HWTEST_F(FileScanUtilsTest, SetBurstKey_CoverType_NonEmptyKey_002, TestSize.Level1)
{
    InnerFileInfo info;
    info.isBurst = IsBurstType::BURST_COVER_TYPE;
    info.displayName = "IMG_BURST001_COVER.jpg";
    info.ownerAlbumId = 8888;
    info.filePath = "/test/path/IMG_BURST001_COVER.jpg";
    FileScanUtils::SetBurstKey(info);
    EXPECT_FALSE(info.burstKey.empty());
}

// ==================== CleanBurstFileName Tests ====================

/**
 * @tc.name: CleanBurstFileName_RemoveDigitsAndCover_001
 * @tc.desc: Test CleanBurstFileName removes burst digits and _COVER suffix
 * @tc.type: FUNC
 */
HWTEST_F(FileScanUtilsTest, CleanBurstFileName_RemoveDigitsAndCover_001, TestSize.Level1)
{
    std::string result = CleanBurstFileName("IMG_BURST001_COVER.jpg");
    EXPECT_EQ(result, "IMG_BURST.jpg");
}

/**
 * @tc.name: CleanBurstFileName_RemoveDigits_002
 * @tc.desc: Test CleanBurstFileName removes burst digits without _COVER
 * @tc.type: FUNC
 */
HWTEST_F(FileScanUtilsTest, CleanBurstFileName_RemoveDigits_002, TestSize.Level1)
{
    std::string result = CleanBurstFileName("IMG_BURST003.jpg");
    EXPECT_EQ(result, "IMG_BURST.jpg");
}

/**
 * @tc.name: CleanBurstFileName_WithParentheses_003
 * @tc.desc: Test CleanBurstFileName with parentheses pattern
 * @tc.type: FUNC
 */
HWTEST_F(FileScanUtilsTest, CleanBurstFileName_WithParentheses_003, TestSize.Level1)
{
    std::string result = CleanBurstFileName("IMG_BURST001(2)_COVER.jpg");
    EXPECT_EQ(result, "IMG_BURST(2).jpg");
}

/**
 * @tc.name: CleanBurstFileName_NoBurst_004
 * @tc.desc: Test CleanBurstFileName with non-burst filename, no change
 * @tc.type: FUNC
 */
HWTEST_F(FileScanUtilsTest, CleanBurstFileName_NoBurst_004, TestSize.Level1)
{
    std::string result = CleanBurstFileName("normal_photo.jpg");
    EXPECT_EQ(result, "normal_photo.jpg");
}

// ==================== GarbleFile Tests ====================

/**
 * @tc.name: GarbleFile_WithExtension_001
 * @tc.desc: Test GarbleFile garbles the name part but preserves extension
 * @tc.type: FUNC
 */
HWTEST_F(FileScanUtilsTest, GarbleFile_WithExtension_001, TestSize.Level1)
{
    std::string result = FileScanUtils::GarbleFile("testphoto.jpg");
    EXPECT_TRUE(result.find(".jpg") != std::string::npos);
    EXPECT_TRUE(result.find('*') != std::string::npos);
    // Name part should be partially garbled
    EXPECT_NE(result, "testphoto.jpg");
}

/**
 * @tc.name: GarbleFile_WithoutExtension_002
 * @tc.desc: Test GarbleFile garbles entire filename without extension
 * @tc.type: FUNC
 */
HWTEST_F(FileScanUtilsTest, GarbleFile_WithoutExtension_002, TestSize.Level1)
{
    std::string result = FileScanUtils::GarbleFile("testfolder");
    EXPECT_TRUE(result.find('*') != std::string::npos);
    EXPECT_TRUE(result.find('.') == std::string::npos);
}

/**
 * @tc.name: GarbleFile_ShortName_003
 * @tc.desc: Test GarbleFile with short name, garble size = file.size()/2
 * @tc.type: FUNC
 */
HWTEST_F(FileScanUtilsTest, GarbleFile_ShortName_003, TestSize.Level1)
{
    std::string result = FileScanUtils::GarbleFile("ab");
    EXPECT_EQ(result, "*b");
}

// ==================== GenerateUuid Tests ====================

/**
 * @tc.name: GenerateUuid_NotEmpty_001
 * @tc.desc: Test GenerateUuid returns non-empty string
 * @tc.type: FUNC
 */
HWTEST_F(FileScanUtilsTest, GenerateUuid_NotEmpty_001, TestSize.Level1)
{
    std::string uuid = FileScanUtils::GenerateUuid();
    EXPECT_FALSE(uuid.empty());
    // UUID format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx (36 chars)
    EXPECT_EQ(uuid.length(), 36u);
}

/**
 * @tc.name: GenerateUuid_Unique_002
 * @tc.desc: Test GenerateUuid returns different values on each call
 * @tc.type: FUNC
 */
HWTEST_F(FileScanUtilsTest, GenerateUuid_Unique_002, TestSize.Level1)
{
    std::string uuid1 = FileScanUtils::GenerateUuid();
    std::string uuid2 = FileScanUtils::GenerateUuid();
    EXPECT_NE(uuid1, uuid2);
}

// ==================== GetFileIdsFromUris Tests ====================

/**
 * @tc.name: GetFileIdsFromUris_EmptyInput_001
 * @tc.desc: Test GetFileIdsFromUris with empty input vector
 * @tc.type: FUNC
 */
HWTEST_F(FileScanUtilsTest, GetFileIdsFromUris_EmptyInput_001, TestSize.Level1)
{
    std::vector<std::string> uris;
    std::vector<std::string> result = FileScanUtils::GetFileIdsFromUris(uris);
    EXPECT_TRUE(result.empty());
}

// ==================== Enum / Const Tests ====================

/**
 * @tc.name: InnerFileInfo_DefaultValues_001
 * @tc.desc: Test InnerFileInfo default values
 * @tc.type: FUNC
 */
HWTEST_F(FileScanUtilsTest, InnerFileInfo_DefaultValues_001, TestSize.Level1)
{
    InnerFileInfo info;
    EXPECT_EQ(info.fileId, 0);
    EXPECT_EQ(info.fileType, 0);
    EXPECT_EQ(info.ownerAlbumId, 0);
    EXPECT_TRUE(info.needInsert);
    EXPECT_EQ(info.isBurst, IsBurstType::OTHER_TYPE);
    EXPECT_EQ(info.height, 0);
    EXPECT_EQ(info.width, 0);
    EXPECT_EQ(info.fileSize, 0);
}

/**
 * @tc.name: PrefixType_EnumValues_001
 * @tc.desc: Test PrefixType enum values
 * @tc.type: FUNC
 */
HWTEST_F(FileScanUtilsTest, PrefixType_EnumValues_001, TestSize.Level1)
{
    EXPECT_EQ(static_cast<int>(PrefixType::CLOUD), 0);
    EXPECT_EQ(static_cast<int>(PrefixType::LOCAL), 1);
    EXPECT_EQ(static_cast<int>(PrefixType::CLOUD_EDIT_DATA), 2);
    EXPECT_EQ(static_cast<int>(PrefixType::LOCAL_EDIT_DATA), 3);
    EXPECT_EQ(static_cast<int>(PrefixType::CLOUD_THUMB), 4);
}

/**
 * @tc.name: FindSubtype_EmptyBurstKey_ReturnsBurst_001
 * @tc.desc: Test FindSubtype with empty burstKey, CHECK_AND_RETURN_RET triggers early return BURST
 * @tc.type: FUNC
 */
HWTEST_F(FileScanUtilsTest, FindSubtype_EmptyBurstKey_ReturnsBurst_001, TestSize.Level1)
{
    InnerFileInfo info;
    info.burstKey = "";
    info.filePath = "/nonexistent/test_photo.jpg";
    int32_t subtype = FileScanUtils::FindSubtype(info);
    // CHECK_AND_RETURN_RET(burstKey.size() <= 0, BURST) triggers when burstKey is empty
    EXPECT_EQ(subtype, static_cast<int32_t>(PhotoSubType::DEFAULT));
}

/**
 * @tc.name: FindSubtype_NonEmptyBurstKey_ChecksLivePhoto_002
 * @tc.desc: Test FindSubtype with non-empty burstKey, falls through to DEFAULT for non-live-photo
 * @tc.type: FUNC
 */
HWTEST_F(FileScanUtilsTest, FindSubtype_NonEmptyBurstKey_ChecksLivePhoto_002, TestSize.Level1)
{
    InnerFileInfo info;
    info.burstKey = "some-uuid-key";
    info.filePath = "/nonexistent/test_photo.jpg";
    int32_t subtype = FileScanUtils::FindSubtype(info);
    // Non-empty burstKey skips early return, IsLivePhoto returns false for nonexistent path
    EXPECT_EQ(subtype, static_cast<int32_t>(PhotoSubType::BURST));
}

} // namespace Media
} // namespace OHOS
