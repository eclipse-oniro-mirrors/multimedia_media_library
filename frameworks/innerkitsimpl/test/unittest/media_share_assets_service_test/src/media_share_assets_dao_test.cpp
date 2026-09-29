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

#define MLOG_TAG "MediaShareAssetsDaoTest"

#include "media_share_assets_test_utils.h"

#include <string>
#include <vector>

#include "dao/media_share_assets_dao.h"
#include "media_column.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_type_const.h"
#include "photo_album_column.h"
#include "share_member_column.h"
#include "userfile_manager_types.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::Media::ORM;

namespace OHOS::Media {

static constexpr int32_t SHARE_ALBUM_ID = 2001;
static constexpr int32_t OTHER_SHARE_ALBUM_ID = 2002;
static constexpr int32_t USER_ALBUM_ID = 2003;
static constexpr int32_t SHARED_ASSET_FLAG = 1;
static constexpr int32_t NOT_SHARED_ASSET_FLAG = 0;

/**
 * @tc.name: HasShareAssetToMarkDeleted_NoRecord_ReturnsFalse
 * @tc.desc: 无共享资产时 HasShareAssetToMarkDeleted 返回 false 且 fileIds 为空(覆盖 false 分支)
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, HasShareAssetToMarkDeleted_NoRecord_ReturnsFalse, TestSize.Level1)
{
    MediaShareAssetsDao dao;
    std::vector<std::string> fileIds;
    bool hasRecords = dao.HasShareAssetToMarkDeleted(fileIds, "0");
    EXPECT_FALSE(hasRecords);
    EXPECT_TRUE(fileIds.empty());
}

/**
 * @tc.name: HasShareAssetToMarkDeleted_NonSharedAsset_ReturnsFalse
 * @tc.desc: 非共享资产不参与标记, 返回 false
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, HasShareAssetToMarkDeleted_NonSharedAsset_ReturnsFalse, TestSize.Level1)
{
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertPhoto(1, "IMG_001.jpg", SHARE_ALBUM_ID, NOT_SHARED_ASSET_FLAG), E_OK);

    MediaShareAssetsDao dao;
    std::vector<std::string> fileIds;
    bool hasRecords = dao.HasShareAssetToMarkDeleted(fileIds, "0");
    EXPECT_FALSE(hasRecords);
    EXPECT_TRUE(fileIds.empty());
}

/**
 * @tc.name: HasShareAssetToMarkDeleted_AlreadyMarked_ReturnsFalse
 * @tc.desc: 已标记为待删除的共享资产(display_name 已被改写)不会被重复查询出来
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, HasShareAssetToMarkDeleted_AlreadyMarked_ReturnsFalse, TestSize.Level1)
{
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertPhoto(1, MediaShareAssetsTestUtils::DELETED_DISPLAY_NAME,
        SHARE_ALBUM_ID, SHARED_ASSET_FLAG), E_OK);

    MediaShareAssetsDao dao;
    std::vector<std::string> fileIds;
    bool hasRecords = dao.HasShareAssetToMarkDeleted(fileIds, "0");
    EXPECT_FALSE(hasRecords);
    EXPECT_TRUE(fileIds.empty());
}

/**
 * @tc.name: HasShareAssetToMarkDeleted_WithRecord_ReturnsTrueAndFileIds
 * @tc.desc: 存在共享资产时返回 true, 并带回 fileId 列表(覆盖 true 分支)
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, HasShareAssetToMarkDeleted_WithRecord_ReturnsTrueAndFileIds, TestSize.Level1)
{
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertPhoto(11, "IMG_011.jpg", SHARE_ALBUM_ID, SHARED_ASSET_FLAG), E_OK);
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertPhoto(12, "IMG_012.jpg", SHARE_ALBUM_ID, SHARED_ASSET_FLAG), E_OK);

    MediaShareAssetsDao dao;
    std::vector<std::string> fileIds;
    bool hasRecords = dao.HasShareAssetToMarkDeleted(fileIds, "0");
    EXPECT_TRUE(hasRecords);
    ASSERT_EQ(fileIds.size(), 2u);
    EXPECT_EQ(fileIds[0], "11");
    EXPECT_EQ(fileIds[1], "12");
}

/**
 * @tc.name: HasShareAssetToMarkDeleted_LastFileIdFilter_SkipsProcessed
 * @tc.desc: lastFileId 用于翻页, 只返回大于该 fileId 的共享资产
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, HasShareAssetToMarkDeleted_LastFileIdFilter_SkipsProcessed, TestSize.Level1)
{
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertPhoto(21, "IMG_021.jpg", SHARE_ALBUM_ID, SHARED_ASSET_FLAG), E_OK);
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertPhoto(22, "IMG_022.jpg", SHARE_ALBUM_ID, SHARED_ASSET_FLAG), E_OK);

    MediaShareAssetsDao dao;
    std::vector<std::string> fileIds;
    bool hasRecords = dao.HasShareAssetToMarkDeleted(fileIds, "21");
    EXPECT_TRUE(hasRecords);
    ASSERT_EQ(fileIds.size(), 1u);
    EXPECT_EQ(fileIds[0], "22");
}

/**
 * @tc.name: HasShareAssetToMarkDeletedByAlbumId_MatchAlbum_ReturnsTrue
 * @tc.desc: 指定相册下存在共享资产时返回 true, 且只返回该相册的资产
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, HasShareAssetToMarkDeletedByAlbumId_MatchAlbum_ReturnsTrue, TestSize.Level1)
{
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertPhoto(31, "IMG_031.jpg", SHARE_ALBUM_ID, SHARED_ASSET_FLAG), E_OK);
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertPhoto(32, "IMG_032.jpg", OTHER_SHARE_ALBUM_ID,
        SHARED_ASSET_FLAG), E_OK);

    MediaShareAssetsDao dao;
    std::vector<std::string> fileIds;
    bool hasRecords = dao.HasShareAssetToMarkDeletedByAlbumId(SHARE_ALBUM_ID, fileIds, "0");
    EXPECT_TRUE(hasRecords);
    ASSERT_EQ(fileIds.size(), 1u);
    EXPECT_EQ(fileIds[0], "31");
}

/**
 * @tc.name: HasShareAssetToMarkDeletedByAlbumId_OtherAlbum_ReturnsFalse
 * @tc.desc: 查询其它相册时无匹配资产, 返回 false
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, HasShareAssetToMarkDeletedByAlbumId_OtherAlbum_ReturnsFalse, TestSize.Level1)
{
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertPhoto(41, "IMG_041.jpg", SHARE_ALBUM_ID, SHARED_ASSET_FLAG), E_OK);

    MediaShareAssetsDao dao;
    std::vector<std::string> fileIds;
    bool hasRecords = dao.HasShareAssetToMarkDeletedByAlbumId(OTHER_SHARE_ALBUM_ID, fileIds, "0");
    EXPECT_FALSE(hasRecords);
    EXPECT_TRUE(fileIds.empty());
}

/**
 * @tc.name: MarkDeletedAndClearCloudInfo_EmptyIds_ReturnsErr
 * @tc.desc: 入参为空时直接返回 E_ERR(覆盖空入参分支)
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, MarkDeletedAndClearCloudInfo_EmptyIds_ReturnsErr, TestSize.Level1)
{
    MediaShareAssetsDao dao;
    std::vector<std::string> fileIds;
    EXPECT_EQ(dao.MarkDeletedAndClearCloudInfo(fileIds), E_ERR);
}

/**
 * @tc.name: MarkDeletedAndClearCloudInfo_ValidIds_UpdatesColumns
 * @tc.desc: 标记共享资产为待删除, 同时清理云侧信息(覆盖成功分支)
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, MarkDeletedAndClearCloudInfo_ValidIds_UpdatesColumns, TestSize.Level1)
{
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertPhoto(51, "IMG_051.jpg", SHARE_ALBUM_ID, SHARED_ASSET_FLAG), E_OK);

    MediaShareAssetsDao dao;
    std::vector<std::string> fileIds = { "51" };
    EXPECT_EQ(dao.MarkDeletedAndClearCloudInfo(fileIds), E_OK);

    EXPECT_EQ(MediaShareAssetsTestUtils::QueryPhotoString("51", MediaColumn::MEDIA_NAME),
        MediaShareAssetsTestUtils::DELETED_DISPLAY_NAME);
    EXPECT_EQ(MediaShareAssetsTestUtils::QueryPhotoInt("51", PhotoColumn::PHOTO_CLEAN_FLAG),
        static_cast<int32_t>(CleanType::TYPE_NEED_CLEAN));
    EXPECT_EQ(MediaShareAssetsTestUtils::QueryPhotoInt("51", PhotoColumn::PHOTO_DIRTY), -1);
    EXPECT_EQ(MediaShareAssetsTestUtils::QueryPhotoInt("51", PhotoColumn::PHOTO_CLOUD_VERSION), 0);
    EXPECT_EQ(MediaShareAssetsTestUtils::QueryPhotoInt("51", PhotoColumn::PHOTO_REAL_LCD_VISIT_TIME), 0);
    EXPECT_TRUE(MediaShareAssetsTestUtils::IsPhotoColumnNull("51", PhotoColumn::PHOTO_CLOUD_ID));
}

/**
 * @tc.name: MarkDeletedAndClearCloudInfo_NonSharedIds_ReturnsErr
 * @tc.desc: 非共享资产不会被标记, 更新行数为 0 时返回 E_ERR(覆盖失败分支)
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, MarkDeletedAndClearCloudInfo_NonSharedIds_ReturnsErr, TestSize.Level1)
{
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertPhoto(61, "IMG_061.jpg", SHARE_ALBUM_ID, NOT_SHARED_ASSET_FLAG), E_OK);

    MediaShareAssetsDao dao;
    std::vector<std::string> fileIds = { "61" };
    EXPECT_EQ(dao.MarkDeletedAndClearCloudInfo(fileIds), E_ERR);
    EXPECT_EQ(MediaShareAssetsTestUtils::QueryPhotoString("61", MediaColumn::MEDIA_NAME), "IMG_061.jpg");
}

/**
 * @tc.name: DeleteShareAssets_EmptyIds_ReturnsErr
 * @tc.desc: 入参为空时直接返回 E_ERR
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, DeleteShareAssets_EmptyIds_ReturnsErr, TestSize.Level1)
{
    MediaShareAssetsDao dao;
    std::vector<std::string> fileIds;
    EXPECT_EQ(dao.DeleteShareAssets(fileIds), E_ERR);
}

/**
 * @tc.name: DeleteShareAssets_ValidIds_DeletesRows
 * @tc.desc: 批量删除已清理的共享资产数据库记录(覆盖成功分支)
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, DeleteShareAssets_ValidIds_DeletesRows, TestSize.Level1)
{
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertPhoto(71, "IMG_071.jpg", SHARE_ALBUM_ID, SHARED_ASSET_FLAG), E_OK);
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertPhoto(72, "IMG_072.jpg", SHARE_ALBUM_ID, SHARED_ASSET_FLAG), E_OK);

    MediaShareAssetsDao dao;
    std::vector<std::string> fileIds = { "71", "72" };
    EXPECT_EQ(dao.DeleteShareAssets(fileIds), E_OK);
    EXPECT_EQ(MediaShareAssetsTestUtils::CountRows(PhotoColumn::PHOTOS_TABLE), 0);
}

/**
 * @tc.name: DeleteShareAssets_NotExistIds_ReturnsErr
 * @tc.desc: 删除不存在的记录时 deletedRows 为 0, 返回 E_ERR
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, DeleteShareAssets_NotExistIds_ReturnsErr, TestSize.Level1)
{
    MediaShareAssetsDao dao;
    std::vector<std::string> fileIds = { "999999" };
    EXPECT_EQ(dao.DeleteShareAssets(fileIds), E_ERR);
}

/**
 * @tc.name: DeleteShareAlbums_RemovesOnlyShareAlbums
 * @tc.desc: 全量删除共享相册, 普通相册不受影响(覆盖成功分支)
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, DeleteShareAlbums_RemovesOnlyShareAlbums, TestSize.Level1)
{
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertShareAlbum(SHARE_ALBUM_ID, "share_album_1"), E_OK);
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertUserAlbum(USER_ALBUM_ID, "user_album_1"), E_OK);

    MediaShareAssetsDao dao;
    EXPECT_EQ(dao.DeleteShareAlbums(), E_OK);
    EXPECT_EQ(MediaShareAssetsTestUtils::CountRows(PhotoAlbumColumns::TABLE), 1);
}

/**
 * @tc.name: DeleteShareAlbums_NoShareAlbum_ReturnsOk
 * @tc.desc: 没有共享相册时删除请求仍然返回 E_OK, 且不会误删普通相册(覆盖无数据分支)
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, DeleteShareAlbums_NoShareAlbum_ReturnsOk, TestSize.Level1)
{
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertUserAlbum(USER_ALBUM_ID, "user_album_1"), E_OK);

    MediaShareAssetsDao dao;
    EXPECT_EQ(dao.DeleteShareAlbums(), E_OK);

    // 普通相册不属于共享相册, 必须原样保留
    EXPECT_EQ(MediaShareAssetsTestUtils::CountRows(PhotoAlbumColumns::TABLE), 1);
}

/**
 * @tc.name: DeleteShareAlbumsByAlbumId_RemovesTargetAlbum
 * @tc.desc: 删除指定共享相册, 其它共享相册保留
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, DeleteShareAlbumsByAlbumId_RemovesTargetAlbum, TestSize.Level1)
{
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertShareAlbum(SHARE_ALBUM_ID, "share_album_1"), E_OK);
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertShareAlbum(OTHER_SHARE_ALBUM_ID, "share_album_2"), E_OK);

    MediaShareAssetsDao dao;
    EXPECT_EQ(dao.DeleteShareAlbumsByAlbumId(SHARE_ALBUM_ID), E_OK);
    EXPECT_EQ(MediaShareAssetsTestUtils::CountRows(PhotoAlbumColumns::TABLE), 1);
}

/**
 * @tc.name: GetShareAssetToRemove_MarkedAssets_ReturnsList
 * @tc.desc: 已标记待删除的共享资产会被查询出来, 供后续清理文件与数据库
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, GetShareAssetToRemove_MarkedAssets_ReturnsList, TestSize.Level1)
{
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertPhoto(81, "IMG_081.jpg", SHARE_ALBUM_ID, SHARED_ASSET_FLAG), E_OK);

    MediaShareAssetsDao dao;
    std::vector<std::string> markIds = { "81" };
    ASSERT_EQ(dao.MarkDeletedAndClearCloudInfo(markIds), E_OK);

    std::vector<PhotosPo> photoInfoList;
    EXPECT_EQ(dao.GetShareAssetToRemove(photoInfoList), E_OK);
    ASSERT_EQ(photoInfoList.size(), 1u);
    EXPECT_EQ(photoInfoList[0].fileId.value_or(-1), 81);
}

/**
 * @tc.name: GetShareAssetToRemove_NoMarkedAssets_ReturnsEmpty
 * @tc.desc: 没有待删除标记时查询结果为空列表(覆盖空结果分支)
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, GetShareAssetToRemove_NoMarkedAssets_ReturnsEmpty, TestSize.Level1)
{
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertPhoto(91, "IMG_091.jpg", SHARE_ALBUM_ID, SHARED_ASSET_FLAG), E_OK);

    MediaShareAssetsDao dao;
    std::vector<PhotosPo> photoInfoList;
    EXPECT_EQ(dao.GetShareAssetToRemove(photoInfoList), E_OK);
    EXPECT_TRUE(photoInfoList.empty());
}

/**
 * @tc.name: DeleteShareMemberInfo_RemovesAllMembers
 * @tc.desc: 账号退出/关闭开关时需要同时清理共享相册成员信息
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, DeleteShareMemberInfo_RemovesAllMembers, TestSize.Level1)
{
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertShareAlbum(SHARE_ALBUM_ID, "share_album_1"), E_OK);
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertShareMember(SHARE_ALBUM_ID, "member_1"), E_OK);
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertShareMember(SHARE_ALBUM_ID, "member_2"), E_OK);
    ASSERT_EQ(MediaShareAssetsTestUtils::CountRows(ShareMemberColumn::TABLE_NAME), 2);

    MediaShareAssetsDao dao;
    EXPECT_EQ(dao.DeleteShareMemberInfo(), E_OK);
    EXPECT_EQ(MediaShareAssetsTestUtils::CountRows(ShareMemberColumn::TABLE_NAME), 0);
}

} // namespace OHOS::Media