/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#include "medialibrary_helper_test.h"

#include "album_asset.h"
#include "file_asset.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "scanner_utils.h"
#include "userfile_manager_types.h"
#include "userfilemgr_uri.h"
#include "media_file_uri.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
HWTEST_F(MediaLibraryHelperUnitTest, FileAsset_SetGet_Test_001, TestSize.Level1)
{
    FileAsset fileAsset;

    const int32_t TEST_FILE_ID = 1;
    fileAsset.SetId(TEST_FILE_ID);
    EXPECT_EQ(fileAsset.GetId(), TEST_FILE_ID);

    const string TEST_URI = MEDIALIBRARY_DATA_URI + MEDIALIBRARY_TYPE_IMAGE_URI + "/" + to_string(TEST_FILE_ID);
    fileAsset.SetUri(TEST_URI);
    EXPECT_EQ(fileAsset.GetUri(), TEST_URI);

    const int32_t TEST_COUNT = 1;
    fileAsset.SetCount(TEST_COUNT);
    EXPECT_EQ(fileAsset.GetCount(), TEST_COUNT);

    const string TEST_DISPLAY_NAME = "test.jpg";
    const string TEST_RELATIVE_PATH = PIC_DIR_VALUES;
    const string TEST_PATH = TEST_RELATIVE_PATH + TEST_DISPLAY_NAME;
    fileAsset.SetPath(TEST_PATH);
    EXPECT_EQ(fileAsset.GetPath(), TEST_PATH);

    fileAsset.SetRelativePath(TEST_RELATIVE_PATH);
    EXPECT_EQ(fileAsset.GetRelativePath(), TEST_RELATIVE_PATH);

    const string TEST_MIME_TYPE = DEFAULT_IMAGE_MIME_TYPE;
    fileAsset.SetMimeType(TEST_MIME_TYPE);
    EXPECT_EQ(fileAsset.GetMimeType(), TEST_MIME_TYPE);

    fileAsset.SetDisplayName(TEST_DISPLAY_NAME);
    EXPECT_EQ(fileAsset.GetDisplayName(), TEST_DISPLAY_NAME);

    const int64_t TEST_SIZE = 1;
    fileAsset.SetSize(TEST_SIZE);
    EXPECT_EQ(fileAsset.GetSize(), TEST_SIZE);

    const int64_t TEST_DATE_ADDED = 1;
    fileAsset.SetDateAdded(TEST_DATE_ADDED);
    EXPECT_EQ(fileAsset.GetDateAdded(), TEST_DATE_ADDED);

    const int64_t TEST_DATE_MODIFIED = 1;
    fileAsset.SetDateModified(TEST_DATE_MODIFIED);
    EXPECT_EQ(fileAsset.GetDateModified(), TEST_DATE_MODIFIED);

    const string TEST_TITLE = "test";
    fileAsset.SetTitle(TEST_TITLE);
    EXPECT_EQ(fileAsset.GetTitle(), TEST_TITLE);
}

HWTEST_F(MediaLibraryHelperUnitTest, FileAsset_SetGet_Test_002, TestSize.Level1)
{
    FileAsset fileAsset;

    const string TEST_ARTIST = "unknown";
    fileAsset.SetArtist(TEST_ARTIST);
    EXPECT_EQ(fileAsset.GetArtist(), TEST_ARTIST);

    const string TEST_ALBUM = "test";
    fileAsset.SetAlbum(TEST_ALBUM);
    EXPECT_EQ(fileAsset.GetAlbum(), TEST_ALBUM);

    const int32_t TEST_WIDTH = 1;
    fileAsset.SetWidth(TEST_WIDTH);
    EXPECT_EQ(fileAsset.GetWidth(), TEST_WIDTH);

    const int32_t TEST_HEIGHT = 1;
    fileAsset.SetHeight(TEST_HEIGHT);
    EXPECT_EQ(fileAsset.GetHeight(), TEST_HEIGHT);

    const int32_t TEST_DURATION = 1;
    fileAsset.SetDuration(TEST_DURATION);
    EXPECT_EQ(fileAsset.GetDuration(), TEST_DURATION);

    const int32_t TEST_ORIENTATION = 1;
    fileAsset.SetOrientation(TEST_ORIENTATION);
    EXPECT_EQ(fileAsset.GetOrientation(), TEST_ORIENTATION);

    const int32_t TEST_ALBUM_ID = 1;
    fileAsset.SetAlbumId(TEST_ALBUM_ID);
    EXPECT_EQ(fileAsset.GetAlbumId(), TEST_ALBUM_ID);

    const string TEST_ALBUM_NAME = "Pictures";
    fileAsset.SetAlbumName(TEST_ALBUM_NAME);
    EXPECT_EQ(fileAsset.GetAlbumName(), TEST_ALBUM_NAME);

    const string TEST_RECYCLE_PATH = "Pictures";
    fileAsset.SetRecyclePath(TEST_RECYCLE_PATH);
    EXPECT_EQ(fileAsset.GetRecyclePath(), TEST_RECYCLE_PATH);

    const ResultNapiType TEST_RESULT_NAPI_TYPE = ResultNapiType::TYPE_USERFILE_MGR;
    fileAsset.SetResultNapiType(TEST_RESULT_NAPI_TYPE);
    EXPECT_EQ(fileAsset.GetResultNapiType(), TEST_RESULT_NAPI_TYPE);
}

HWTEST_F(MediaLibraryHelperUnitTest, FileAsset_SetGet_Test_003, TestSize.Level1)
{
    FileAsset fileAsset;

    const int32_t TEST_PARENT = 1;
    fileAsset.SetParent(TEST_PARENT);
    EXPECT_EQ(fileAsset.GetParent(), TEST_PARENT);

    const string TEST_ALBUM_URI = MEDIALIBRARY_DATA_URI + MEDIALIBRARY_TYPE_FILE_URI + "/" + to_string(TEST_PARENT);
    fileAsset.SetAlbumUri(TEST_ALBUM_URI);
    EXPECT_EQ(fileAsset.GetAlbumUri(), TEST_ALBUM_URI);

    const int64_t TEST_DATE_TOKEN = 1;
    fileAsset.SetDateTaken(TEST_DATE_TOKEN);
    EXPECT_EQ(fileAsset.GetDateTaken(), TEST_DATE_TOKEN);

    fileAsset.SetFavorite(true);
    EXPECT_EQ(fileAsset.IsFavorite(), true);

    const int64_t TEST_TIME_PENDING = 1;
    fileAsset.SetTimePending(TEST_TIME_PENDING);
    EXPECT_EQ(fileAsset.GetTimePending(), TEST_TIME_PENDING);

    const int64_t TEST_DATE_TRASHED = 1;
    fileAsset.SetDateTrashed(TEST_DATE_TRASHED);
    EXPECT_EQ(fileAsset.GetDateTrashed(), TEST_DATE_TRASHED);

    const int32_t ASSET_ISTRASH = 1;
    fileAsset.SetIsTrash(ASSET_ISTRASH);
    EXPECT_EQ(fileAsset.GetIsTrash(), ASSET_ISTRASH);

    const string TEST_SELF_ID = "test";
    fileAsset.SetSelfId(TEST_SELF_ID);
    EXPECT_EQ(fileAsset.GetSelfId(), TEST_SELF_ID);

    fileAsset.SetMediaType(MEDIA_TYPE_IMAGE);
    EXPECT_EQ(fileAsset.GetMediaType(), MEDIA_TYPE_IMAGE);

    auto memberMap = fileAsset.GetMemberMap();
    EXPECT_EQ(memberMap.size() > 0, true);
}

HWTEST_F(MediaLibraryHelperUnitTest, FileAsset_GetMemberValue_Test_001, TestSize.Level1)
{
    FileAsset fileAsset;
    const int32_t TEST_FILE_ID = 1;
    fileAsset.SetId(TEST_FILE_ID);
    EXPECT_EQ(get<int32_t>(fileAsset.GetMemberValue(MEDIA_DATA_DB_ID)), TEST_FILE_ID);
}

HWTEST_F(MediaLibraryHelperUnitTest, FileAsset_Test_001, TestSize.Level1)
{
    FileAsset fileAsset;
    int32_t fd = 0;
    int32_t openStatus = 0;
    fileAsset.openStatusMap_ = nullptr;
    fileAsset.SetOpenStatus(fd, openStatus);
    EXPECT_EQ(fileAsset.GetOpenStatus(fd), 0);
    EXPECT_EQ(fileAsset.GetOpenStatus(1), E_INVALID_VALUES);
}

HWTEST_F(MediaLibraryHelperUnitTest, FileAsset_Test_002, TestSize.Level1)
{
    FileAsset fileAsset;
    int32_t fd = 0;
    int32_t openStatus = 0;
    fileAsset.openStatusMap_ = make_shared<unordered_map<int32_t, int32_t>>();
    fileAsset.SetOpenStatus(fd, openStatus);
    EXPECT_EQ(fileAsset.GetOpenStatus(fd), 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, FileAsset_Test_003, TestSize.Level1)
{
    FileAsset fileAsset;
    string colName = "";
    ResultSetDataType type = TYPE_STRING;
    fileAsset.resultTypeMap_.insert(make_pair(colName, type));
    fileAsset.SetResultTypeMap(colName, type);
    EXPECT_FALSE(fileAsset.resultTypeMap_.empty());
}

HWTEST_F(MediaLibraryHelperUnitTest, FileAsset_Test_004, TestSize.Level1)
{
    FileAsset fileAsset;
    string colName = "";
    ResultSetDataType type = TYPE_STRING;
    fileAsset.SetResultTypeMap(colName, type);
    EXPECT_FALSE(fileAsset.resultTypeMap_.empty());
    EXPECT_FALSE(fileAsset.resultTypeMap_.empty());
}

HWTEST_F(MediaLibraryHelperUnitTest, FileAsset_SetGet_Test_004, TestSize.Level1)
{
    FileAsset fileAsset;

    const int32_t TEST_VISIT_COUNT = 1;
    fileAsset.SetVisitCount(TEST_VISIT_COUNT);
    EXPECT_EQ(fileAsset.GetVisitCount(), TEST_VISIT_COUNT);

    const int32_t TEST_PARENT = 1;
    fileAsset.SetLcdVisitCount(TEST_PARENT);
    EXPECT_EQ(fileAsset.GetLcdVisitCount(), TEST_PARENT);
}

HWTEST_F(MediaLibraryHelperUnitTest, FileAsset_SetGet_Test_005, TestSize.Level1)
{
    FileAsset fileAsset;
 
    const int32_t TEST_HAS_APPLINK = 1;
    fileAsset.SetHasAppLink(TEST_HAS_APPLINK);
    EXPECT_EQ(fileAsset.GetHasAppLink(), TEST_HAS_APPLINK);
 
    const string TEST_APPLINK = "https://item.taobao.com/item.htm?id=899930519163";
    fileAsset.SetAppLink(TEST_APPLINK);
    EXPECT_EQ(fileAsset.GetAppLink(), TEST_APPLINK);
}

HWTEST_F(MediaLibraryHelperUnitTest, FileAsset_SetGet_Test_006, TestSize.Level1)
{
    FileAsset fileAsset;

    const int32_t TEST_EXIST_COMPATIBLE_DUPLICATE = 1;
    fileAsset.SetExistCompatibleDuplicate(TEST_EXIST_COMPATIBLE_DUPLICATE);
    EXPECT_EQ(fileAsset.GetExistCompatibleDuplicate(), TEST_EXIST_COMPATIBLE_DUPLICATE);

    const int64_t CHANGE_TIME = 1;
    fileAsset.SetChangeTime(CHANGE_TIME);
    EXPECT_EQ(fileAsset.GetChangeTime(), CHANGE_TIME);
}

HWTEST_F(MediaLibraryHelperUnitTest, FileAsset_SetGet_Test_007, TestSize.Level1)
{
    FileAsset fileAsset;

    const int32_t TEST_FILE_SOURCE_TYPE = 0;
    fileAsset.SetFileResourceType(TEST_FILE_SOURCE_TYPE);
    EXPECT_EQ(fileAsset.GetFileResourceType(), TEST_FILE_SOURCE_TYPE);
}
} // namespace Media
} // namespace OHOS