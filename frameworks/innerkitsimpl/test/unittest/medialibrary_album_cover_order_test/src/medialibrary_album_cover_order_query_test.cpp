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

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "cover_record_columns.h"
#include "default_cover_order_info.h"
#include "media_column.h"
#include "medialibrary_album_operations.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdb_utils.h"
#include "photo_album_column.h"
#include "rdb_predicates.h"
#include "userfilemgr_uri.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {

class MediaLibraryAlbumCoverOrderQueryTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() override {}
    void TearDown() override {}
};

static UpdateAlbumData MakeAlbumData(int32_t subtype)
{
    UpdateAlbumData data;
    data.albumId = 1;
    data.albumSubtype = subtype;
    data.hiddenCount = 0;
    data.albumCount = 0;
    data.albumImageCount = 0;
    data.albumVideoCount = 0;
    data.isCoverSatisfied = 0;
    data.coverDateTime = 0;
    data.hiddenCoverDateTime = 0;
    data.coverOrderType = 0;
    data.hiddenCoverOrderType = 0;
    return data;
}

static vector<string> MakeDefaultColumns()
{
    return {
        CONST_MEDIA_COLUMN_COUNT_1, PhotoColumn::MEDIA_ID,
        PhotoColumn::MEDIA_FILE_PATH, PhotoColumn::MEDIA_NAME,
        PhotoColumn::PHOTO_HIDDEN_TIME,
        PhotoColumn::MEDIA_DATE_ADDED,
        PhotoColumn::MEDIA_DATE_TAKEN
    };
}

static bool HasWindowedCount(const vector<string> &columns)
{
    for (const auto &column : columns) {
        if (column == "count(1) over() as 'count(1)'") {
            return true;
        }
    }
    return false;
}

HWTEST_F(MediaLibraryAlbumCoverOrderQueryTest, DetermineQueryOrder_NoCoverOrder_KeepCount_Test_001, TestSize.Level1)
{
    UpdateAlbumData data = MakeAlbumData(static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC));
    vector<string> columns = MakeDefaultColumns();
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);

    MediaLibraryRdbUtils::DetermineQueryOrder(predicates, data, false, columns);

    EXPECT_FALSE(HasWindowedCount(columns));
    EXPECT_EQ(columns[0], CONST_MEDIA_COLUMN_COUNT_1);
    EXPECT_TRUE(predicates.GetOrder().empty());
}

HWTEST_F(MediaLibraryAlbumCoverOrderQueryTest, DetermineQueryOrder_CoverOrderKeyDesc_Test_001, TestSize.Level1)
{
    UpdateAlbumData data = MakeAlbumData(static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC));
    data.coverOrderKey = PhotoColumn::MEDIA_DATE_TAKEN;
    data.coverOrderType = 0;
    vector<string> columns = MakeDefaultColumns();
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);

    MediaLibraryRdbUtils::DetermineQueryOrder(predicates, data, false, columns);

    EXPECT_TRUE(HasWindowedCount(columns));
    string order = predicates.GetOrder();
    EXPECT_NE(order.find(PhotoColumn::MEDIA_DATE_TAKEN), string::npos);
    EXPECT_NE(order.find("DESC"), string::npos);
    EXPECT_NE(order.find(MediaColumn::MEDIA_ID), string::npos);
}

HWTEST_F(MediaLibraryAlbumCoverOrderQueryTest, DetermineQueryOrder_CoverOrderKeyAsc_Test_001, TestSize.Level1)
{
    UpdateAlbumData data = MakeAlbumData(static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC));
    data.coverOrderKey = PhotoColumn::MEDIA_DATE_TAKEN;
    data.coverOrderType = 1;
    vector<string> columns = MakeDefaultColumns();
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);

    MediaLibraryRdbUtils::DetermineQueryOrder(predicates, data, false, columns);

    EXPECT_TRUE(HasWindowedCount(columns));
    string order = predicates.GetOrder();
    EXPECT_NE(order.find(PhotoColumn::MEDIA_DATE_TAKEN), string::npos);
    EXPECT_NE(order.find("ASC"), string::npos);
    EXPECT_EQ(order.find("DESC"), string::npos);
}

HWTEST_F(MediaLibraryAlbumCoverOrderQueryTest, DetermineQueryOrder_CoverOrderSubKeyOnly_Test_001, TestSize.Level1)
{
    UpdateAlbumData data = MakeAlbumData(static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC));
    data.coverOrderSubKey = PhotoColumn::MEDIA_DATE_ADDED;
    data.coverOrderType = 1;
    vector<string> columns = MakeDefaultColumns();
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);

    MediaLibraryRdbUtils::DetermineQueryOrder(predicates, data, false, columns);

    EXPECT_TRUE(HasWindowedCount(columns));
    string order = predicates.GetOrder();
    EXPECT_NE(order.find(PhotoColumn::MEDIA_DATE_ADDED), string::npos);
    EXPECT_NE(order.find("ASC"), string::npos);
}

HWTEST_F(MediaLibraryAlbumCoverOrderQueryTest, DetermineQueryOrder_InvalidOrderType_Test_001, TestSize.Level1)
{
    UpdateAlbumData data = MakeAlbumData(static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC));
    data.coverOrderKey = PhotoColumn::MEDIA_DATE_TAKEN;
    data.coverOrderType = 2;
    vector<string> columns = MakeDefaultColumns();
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);

    MediaLibraryRdbUtils::DetermineQueryOrder(predicates, data, false, columns);

    EXPECT_TRUE(HasWindowedCount(columns));
    EXPECT_TRUE(predicates.GetOrder().empty());
}

HWTEST_F(MediaLibraryAlbumCoverOrderQueryTest, DetermineQueryOrder_HiddenStateUsesHiddenKey_001, TestSize.Level1)
{
    UpdateAlbumData data = MakeAlbumData(static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC));
    data.coverOrderKey = PhotoColumn::MEDIA_DATE_TAKEN;
    data.coverOrderType = 1;
    data.hiddenCoverOrderKey = PhotoColumn::PHOTO_HIDDEN_TIME;
    data.hiddenCoverOrderType = 0;
    vector<string> columns = MakeDefaultColumns();
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);

    MediaLibraryRdbUtils::DetermineQueryOrder(predicates, data, true, columns);

    string order = predicates.GetOrder();
    EXPECT_NE(order.find(PhotoColumn::PHOTO_HIDDEN_TIME), string::npos);
    EXPECT_EQ(order.find(PhotoColumn::MEDIA_DATE_TAKEN), string::npos);
    EXPECT_NE(order.find("DESC"), string::npos);
}

HWTEST_F(MediaLibraryAlbumCoverOrderQueryTest, DetermineQueryOrder_HiddenSubtypeUsesHiddenKey_001, TestSize.Level1)
{
    UpdateAlbumData data = MakeAlbumData(static_cast<int32_t>(PhotoAlbumSubType::HIDDEN));
    data.coverOrderKey = PhotoColumn::MEDIA_DATE_TAKEN;
    data.hiddenCoverOrderKey = PhotoColumn::PHOTO_HIDDEN_TIME;
    data.hiddenCoverOrderType = 1;
    vector<string> columns = MakeDefaultColumns();
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);

    MediaLibraryRdbUtils::DetermineQueryOrder(predicates, data, false, columns);

    string order = predicates.GetOrder();
    EXPECT_NE(order.find(PhotoColumn::PHOTO_HIDDEN_TIME), string::npos);
    EXPECT_EQ(order.find(PhotoColumn::MEDIA_DATE_TAKEN), string::npos);
}

HWTEST_F(MediaLibraryAlbumCoverOrderQueryTest, DetermineQueryOrder_HiddenNoOrderKey_Test_001, TestSize.Level1)
{
    UpdateAlbumData data = MakeAlbumData(static_cast<int32_t>(PhotoAlbumSubType::HIDDEN));
    vector<string> columns = MakeDefaultColumns();
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);

    MediaLibraryRdbUtils::DetermineQueryOrder(predicates, data, true, columns);

    EXPECT_FALSE(HasWindowedCount(columns));
    EXPECT_TRUE(predicates.GetOrder().empty());
}

HWTEST_F(MediaLibraryAlbumCoverOrderQueryTest, DetermineQueryOrder_BothKeyAndSubKey_Test_001, TestSize.Level1)
{
    UpdateAlbumData data = MakeAlbumData(static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC));
    data.coverOrderKey = PhotoColumn::MEDIA_DATE_TAKEN;
    data.coverOrderSubKey = PhotoColumn::MEDIA_DATE_ADDED;
    data.coverOrderType = 0;
    vector<string> columns = MakeDefaultColumns();
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);

    MediaLibraryRdbUtils::DetermineQueryOrder(predicates, data, false, columns);

    string order = predicates.GetOrder();
    EXPECT_LT(order.find(PhotoColumn::MEDIA_DATE_TAKEN), order.find(PhotoColumn::MEDIA_DATE_ADDED));
    EXPECT_LT(order.find(PhotoColumn::MEDIA_DATE_ADDED), order.rfind(MediaColumn::MEDIA_ID));
}

HWTEST_F(MediaLibraryAlbumCoverOrderQueryTest, DetermineQueryOrder_ReplaceCountOnlyOnce_Test_001, TestSize.Level1)
{
    UpdateAlbumData data = MakeAlbumData(static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC));
    data.coverOrderKey = PhotoColumn::MEDIA_DATE_TAKEN;
    vector<string> columns = { CONST_MEDIA_COLUMN_COUNT_1, CONST_MEDIA_COLUMN_COUNT_1, PhotoColumn::MEDIA_ID };
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);

    MediaLibraryRdbUtils::DetermineQueryOrder(predicates, data, false, columns);

    EXPECT_EQ(columns[0], "count(1) over() as 'count(1)'");
    EXPECT_EQ(columns[1], CONST_MEDIA_COLUMN_COUNT_1);
}

HWTEST_F(MediaLibraryAlbumCoverOrderQueryTest, DetermineQueryOrder_NoCountColumn_Test_001, TestSize.Level1)
{
    UpdateAlbumData data = MakeAlbumData(static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC));
    data.coverOrderKey = PhotoColumn::MEDIA_DATE_TAKEN;
    vector<string> columns = { PhotoColumn::MEDIA_ID, PhotoColumn::MEDIA_NAME };
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);

    MediaLibraryRdbUtils::DetermineQueryOrder(predicates, data, false, columns);

    EXPECT_EQ(columns.size(), 2);
    EXPECT_FALSE(HasWindowedCount(columns));
    EXPECT_FALSE(predicates.GetOrder().empty());
}

HWTEST_F(MediaLibraryAlbumCoverOrderQueryTest, DetermineQueryOrder_IndexedByVideo_Test_001, TestSize.Level1)
{
    UpdateAlbumData data = MakeAlbumData(static_cast<int32_t>(PhotoAlbumSubType::VIDEO));
    vector<string> columns = MakeDefaultColumns();
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);

    MediaLibraryRdbUtils::DetermineQueryOrder(predicates, data, false, columns);

    EXPECT_EQ(predicates.GetIndex(), PhotoColumn::PHOTO_SCHPT_MEDIA_TYPE_INDEX);
}

HWTEST_F(MediaLibraryAlbumCoverOrderQueryTest, DetermineQueryOrder_IndexedByHidden_Test_001, TestSize.Level1)
{
    UpdateAlbumData data = MakeAlbumData(static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC));
    data.coverOrderKey = PhotoColumn::MEDIA_DATE_TAKEN;
    vector<string> columns = MakeDefaultColumns();
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);

    MediaLibraryRdbUtils::DetermineQueryOrder(predicates, data, true, columns);

    EXPECT_EQ(predicates.GetIndex(), PhotoColumn::PHOTO_SCHPT_HIDDEN_TIME_INDEX);
}

HWTEST_F(MediaLibraryAlbumCoverOrderQueryTest, DetermineQueryOrder_IndexedByFavorite_Test_001, TestSize.Level1)
{
    UpdateAlbumData data = MakeAlbumData(static_cast<int32_t>(PhotoAlbumSubType::FAVORITE));
    vector<string> columns = MakeDefaultColumns();
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);

    MediaLibraryRdbUtils::DetermineQueryOrder(predicates, data, false, columns);

    EXPECT_EQ(predicates.GetIndex(), PhotoColumn::PHOTO_FAVORITE_INDEX);
}

HWTEST_F(MediaLibraryAlbumCoverOrderQueryTest, DetermineQueryOrder_IndexedByUserGeneric_Test_001, TestSize.Level1)
{
    UpdateAlbumData data = MakeAlbumData(static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC));
    vector<string> columns = MakeDefaultColumns();
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);

    MediaLibraryRdbUtils::DetermineQueryOrder(predicates, data, false, columns);

    EXPECT_EQ(predicates.GetIndex(), PhotoColumn::PHOTO_SORT_IN_ALBUM_DATE_TAKEN_INDEX);
}

HWTEST_F(MediaLibraryAlbumCoverOrderQueryTest, DetermineQueryOrder_IndexedByDefault_Test_001, TestSize.Level1)
{
    UpdateAlbumData data = MakeAlbumData(static_cast<int32_t>(PhotoAlbumSubType::SCREENSHOT));
    vector<string> columns = MakeDefaultColumns();
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);

    MediaLibraryRdbUtils::DetermineQueryOrder(predicates, data, false, columns);

    EXPECT_EQ(predicates.GetIndex(), PhotoColumn::PHOTO_SCHPT_READY_INDEX);
}

HWTEST_F(MediaLibraryAlbumCoverOrderQueryTest, ModifyAlbumDefaultCoverOrder_EmptyInfos_Test_001, TestSize.Level1)
{
    vector<DefaultCoverOrderInfo> emptyInfos;

    EXPECT_EQ(MediaLibraryAlbumOperations::ModifyAlbumDefaultCoverOrder(emptyInfos, false, false), E_INVALID_ARGS);
    EXPECT_EQ(MediaLibraryAlbumOperations::ModifyAlbumDefaultCoverOrder(emptyInfos, true, false), E_INVALID_ARGS);
    EXPECT_EQ(MediaLibraryAlbumOperations::ModifyAlbumDefaultCoverOrder(emptyInfos, false, true), E_INVALID_ARGS);
}

HWTEST_F(MediaLibraryAlbumCoverOrderQueryTest, ModifyHiddenAlbumDefaultCoverOrder_EmptyInfos_001, TestSize.Level1)
{
    vector<DefaultCoverOrderInfo> emptyInfos;

    EXPECT_EQ(MediaLibraryAlbumOperations::ModifyHiddenAlbumDefaultCoverOrder(emptyInfos, false, false),
        E_INVALID_ARGS);
    EXPECT_EQ(MediaLibraryAlbumOperations::ModifyHiddenAlbumDefaultCoverOrder(emptyInfos, true, false),
        E_INVALID_ARGS);
}

HWTEST_F(MediaLibraryAlbumCoverOrderQueryTest, PrepareUserAlbum_CoverOrderNotSet_Test_001, TestSize.Level1)
{
    ValuesBucket values;
    MediaLibraryAlbumOperations::PrepareUserAlbum("cover_order_ut_album", values);

    ValueObject obj;
    EXPECT_TRUE(values.GetObject(PhotoAlbumColumns::ALBUM_NAME, obj));
    EXPECT_TRUE(values.GetObject(PhotoAlbumColumns::ALBUM_LPATH, obj));
    EXPECT_FALSE(values.GetObject(PhotoAlbumColumns::COVER_ORDER_KEY, obj));
    EXPECT_FALSE(values.GetObject(PhotoAlbumColumns::HIDDEN_COVER_ORDER_KEY, obj));
}

HWTEST_F(MediaLibraryAlbumCoverOrderQueryTest, PutGeneralPhotoAlbumValues_NoCoverRecord_Test_001, TestSize.Level1)
{
    ValuesBucket values;
    MediaLibraryAlbumOperations::PrepareUserAlbum("ut_album", values);

    ValueObject obj;
    EXPECT_TRUE(values.GetObject(PhotoAlbumColumns::ALBUM_LPATH, obj));
    EXPECT_FALSE(values.GetObject(PhotoAlbumColumns::COVER_ORDER_KEY, obj));
    EXPECT_FALSE(values.GetObject(PhotoAlbumColumns::COVER_ORDER_SUBKEY, obj));
    EXPECT_FALSE(values.GetObject(PhotoAlbumColumns::COVER_ORDER_TYPE, obj));
    EXPECT_FALSE(values.GetObject(PhotoAlbumColumns::HIDDEN_COVER_ORDER_SUBKEY, obj));
    EXPECT_FALSE(values.GetObject(PhotoAlbumColumns::HIDDEN_COVER_ORDER_TYPE, obj));
}
} // namespace Media
} // namespace OHOS
