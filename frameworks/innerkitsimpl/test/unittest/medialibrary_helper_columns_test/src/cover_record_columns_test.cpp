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

#include "medialibrary_helper_test.h"

#include "base_column.h"
#include "cover_record_columns.h"
#include "medialibrary_operation.h"
#include "photo_album_column.h"
#include "userfilemgr_uri.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
HWTEST_F(MediaLibraryHelperUnitTest, CoverRecordColumns_FieldNames_Test_001, TestSize.Level1)
{
    EXPECT_EQ(CoverRecordColumns::ALBUM_TYPE, "album_type");
    EXPECT_EQ(CoverRecordColumns::ALBUM_SUBTYPE, "album_subtype");
    EXPECT_EQ(CoverRecordColumns::ALBUM_LPATH, "lpath");
    EXPECT_EQ(CoverRecordColumns::COVER_ORDER_KEY, "cover_order_key");
    EXPECT_EQ(CoverRecordColumns::COVER_ORDER_SUBKEY, "cover_order_subkey");
    EXPECT_EQ(CoverRecordColumns::COVER_ORDER_TYPE, "cover_order_type");
    EXPECT_EQ(CoverRecordColumns::HIDDEN_COVER_ORDER_KEY, "hidden_cover_order_key");
    EXPECT_EQ(CoverRecordColumns::HIDDEN_COVER_ORDER_SUBKEY, "hidden_cover_order_subkey");
    EXPECT_EQ(CoverRecordColumns::HIDDEN_COVER_ORDER_TYPE, "hidden_cover_order_type");
}

HWTEST_F(MediaLibraryHelperUnitTest, CoverRecordColumns_TableAndIndexName_Test_001, TestSize.Level1)
{
    EXPECT_EQ(CoverRecordColumns::COVER_RECORD_TABLE, "tab_cover_record");
    EXPECT_EQ(CoverRecordColumns::ALBUM_LPATH_INDEX, "lpath_index");
}

HWTEST_F(MediaLibraryHelperUnitTest, CoverRecordColumns_CreateTableSql_Test_001, TestSize.Level1)
{
    const string &createSql = CoverRecordColumns::CREATE_COVER_RECORD_TABLE;
    EXPECT_FALSE(createSql.empty());
    EXPECT_EQ(createSql.find(BaseColumn::CreateTable()), 0);
    EXPECT_NE(createSql.find(CoverRecordColumns::COVER_RECORD_TABLE), string::npos);

    const string expectSql = BaseColumn::CreateTable() + "tab_cover_record (" +
        "album_type INT NOT NULL DEFAULT 0, " +
        "album_subtype INT NOT NULL DEFAULT 0, " +
        "lpath TEXT DEFAULT NULL COLLATE NOCASE, " +
        "cover_order_key TEXT DEFAULT NULL, " +
        "cover_order_subkey TEXT DEFAULT NULL, " +
        "cover_order_type INT NOT NULL DEFAULT 0, " +
        "hidden_cover_order_key TEXT DEFAULT NULL, " +
        "hidden_cover_order_subkey TEXT DEFAULT NULL, " +
        "hidden_cover_order_type INT NOT NULL DEFAULT 0" + ")";
    EXPECT_EQ(createSql, expectSql);
}

HWTEST_F(MediaLibraryHelperUnitTest, CoverRecordColumns_CreateTableSql_Test_002, TestSize.Level1)
{
    const string &createSql = CoverRecordColumns::CREATE_COVER_RECORD_TABLE;
    EXPECT_NE(createSql.find(CoverRecordColumns::ALBUM_TYPE), string::npos);
    EXPECT_NE(createSql.find(CoverRecordColumns::ALBUM_SUBTYPE), string::npos);
    EXPECT_NE(createSql.find(CoverRecordColumns::ALBUM_LPATH), string::npos);
    EXPECT_NE(createSql.find(CoverRecordColumns::COVER_ORDER_KEY), string::npos);
    EXPECT_NE(createSql.find(CoverRecordColumns::COVER_ORDER_SUBKEY), string::npos);
    EXPECT_NE(createSql.find(CoverRecordColumns::COVER_ORDER_TYPE), string::npos);
    EXPECT_NE(createSql.find(CoverRecordColumns::HIDDEN_COVER_ORDER_KEY), string::npos);
    EXPECT_NE(createSql.find(CoverRecordColumns::HIDDEN_COVER_ORDER_SUBKEY), string::npos);
    EXPECT_NE(createSql.find(CoverRecordColumns::HIDDEN_COVER_ORDER_TYPE), string::npos);
    EXPECT_EQ(createSql.substr(createSql.size() - 1), ")");
}

HWTEST_F(MediaLibraryHelperUnitTest, CoverRecordColumns_CreateIndexSql_Test_001, TestSize.Level1)
{
    const string &indexSql = CoverRecordColumns::CREATE_ALBUM_LPATH_INDEX;
    EXPECT_FALSE(indexSql.empty());
    EXPECT_EQ(indexSql.find(BaseColumn::CreateIndex()), 0);

    const string expectSql = BaseColumn::CreateIndex() + "lpath_index ON tab_cover_record " +
        "(album_type,album_subtype,lpath COLLATE NOCASE)";
    EXPECT_EQ(indexSql, expectSql);
}

HWTEST_F(MediaLibraryHelperUnitTest, CoverRecordColumns_CreateIndexSql_Test_002, TestSize.Level1)
{
    const string &indexSql = CoverRecordColumns::CREATE_ALBUM_LPATH_INDEX;
    EXPECT_NE(indexSql.find(CoverRecordColumns::ALBUM_LPATH_INDEX), string::npos);
    EXPECT_NE(indexSql.find(" ON " + CoverRecordColumns::COVER_RECORD_TABLE), string::npos);
    EXPECT_NE(indexSql.find("COLLATE NOCASE"), string::npos);
    EXPECT_LT(indexSql.find(CoverRecordColumns::ALBUM_TYPE), indexSql.find(CoverRecordColumns::ALBUM_SUBTYPE));
    EXPECT_LT(indexSql.find(CoverRecordColumns::ALBUM_SUBTYPE), indexSql.rfind(CoverRecordColumns::ALBUM_LPATH));
}

HWTEST_F(MediaLibraryHelperUnitTest, CoverRecordColumns_SameKeyWithPhotoAlbum_Test_001, TestSize.Level1)
{
    EXPECT_EQ(CoverRecordColumns::COVER_ORDER_KEY, PhotoAlbumColumns::COVER_ORDER_KEY);
    EXPECT_EQ(CoverRecordColumns::COVER_ORDER_SUBKEY, PhotoAlbumColumns::COVER_ORDER_SUBKEY);
    EXPECT_EQ(CoverRecordColumns::COVER_ORDER_TYPE, PhotoAlbumColumns::COVER_ORDER_TYPE);
    EXPECT_EQ(CoverRecordColumns::HIDDEN_COVER_ORDER_KEY, PhotoAlbumColumns::HIDDEN_COVER_ORDER_KEY);
    EXPECT_EQ(CoverRecordColumns::HIDDEN_COVER_ORDER_SUBKEY, PhotoAlbumColumns::HIDDEN_COVER_ORDER_SUBKEY);
    EXPECT_EQ(CoverRecordColumns::HIDDEN_COVER_ORDER_TYPE, PhotoAlbumColumns::HIDDEN_COVER_ORDER_TYPE);
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoAlbumColumns_CoverOrderFieldNames_Test_001, TestSize.Level1)
{
    EXPECT_EQ(PhotoAlbumColumns::COVER_ORDER_KEY, "cover_order_key");
    EXPECT_EQ(PhotoAlbumColumns::COVER_ORDER_SUBKEY, "cover_order_subkey");
    EXPECT_EQ(PhotoAlbumColumns::COVER_ORDER_TYPE, "cover_order_type");
    EXPECT_EQ(PhotoAlbumColumns::HIDDEN_COVER_ORDER_KEY, "hidden_cover_order_key");
    EXPECT_EQ(PhotoAlbumColumns::HIDDEN_COVER_ORDER_SUBKEY, "hidden_cover_order_subkey");
    EXPECT_EQ(PhotoAlbumColumns::HIDDEN_COVER_ORDER_TYPE, "hidden_cover_order_type");
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoAlbumColumns_CoverOrderInCreateTable_Test_001, TestSize.Level1)
{
    const string &createSql = PhotoAlbumColumns::CREATE_TABLE;
    EXPECT_NE(createSql.find(PhotoAlbumColumns::COVER_ORDER_KEY + " TEXT DEFAULT NULL"), string::npos);
    EXPECT_NE(createSql.find(PhotoAlbumColumns::COVER_ORDER_SUBKEY + " TEXT DEFAULT NULL"), string::npos);
    EXPECT_NE(createSql.find(PhotoAlbumColumns::COVER_ORDER_TYPE + " INT NOT NULL DEFAULT 0"), string::npos);
    EXPECT_NE(createSql.find(PhotoAlbumColumns::HIDDEN_COVER_ORDER_KEY + " TEXT DEFAULT NULL"), string::npos);
    EXPECT_NE(createSql.find(PhotoAlbumColumns::HIDDEN_COVER_ORDER_SUBKEY + " TEXT DEFAULT NULL"), string::npos);
    EXPECT_NE(createSql.find(PhotoAlbumColumns::HIDDEN_COVER_ORDER_TYPE + " INT NOT NULL DEFAULT 0"), string::npos);
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoAlbumColumns_IsPhotoAlbumColumn_CoverOrder_Test_001, TestSize.Level1)
{
    EXPECT_TRUE(PhotoAlbumColumns::IsPhotoAlbumColumn(PhotoAlbumColumns::COVER_ORDER_KEY));
    EXPECT_TRUE(PhotoAlbumColumns::IsPhotoAlbumColumn(PhotoAlbumColumns::COVER_ORDER_SUBKEY));
    EXPECT_TRUE(PhotoAlbumColumns::IsPhotoAlbumColumn(PhotoAlbumColumns::COVER_ORDER_TYPE));
    EXPECT_TRUE(PhotoAlbumColumns::IsPhotoAlbumColumn(PhotoAlbumColumns::HIDDEN_COVER_ORDER_KEY));
    EXPECT_TRUE(PhotoAlbumColumns::IsPhotoAlbumColumn(PhotoAlbumColumns::HIDDEN_COVER_ORDER_SUBKEY));
    EXPECT_TRUE(PhotoAlbumColumns::IsPhotoAlbumColumn(PhotoAlbumColumns::HIDDEN_COVER_ORDER_TYPE));
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoAlbumColumns_IsPhotoAlbumColumn_Invalid_Test_001, TestSize.Level1)
{
    EXPECT_FALSE(PhotoAlbumColumns::IsPhotoAlbumColumn(""));
    EXPECT_FALSE(PhotoAlbumColumns::IsPhotoAlbumColumn("cover_order_key_not_exist"));
    EXPECT_FALSE(PhotoAlbumColumns::IsPhotoAlbumColumn("COVER_ORDER_KEY"));
    EXPECT_FALSE(PhotoAlbumColumns::IsPhotoAlbumColumn(CoverRecordColumns::ALBUM_LPATH_INDEX));
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaOperation_OprnObjMap_CoverRecord_Test_001, TestSize.Level1)
{
    const auto &oprnObjMap = MediaOperation::GetOprnObjMap();
    auto it = oprnObjMap.find(CONST_TAB_COVER_RECORD);
    ASSERT_NE(it, oprnObjMap.end());
    EXPECT_EQ(it->second, OperationObject::TAB_COVER_RECORD);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaOperation_OprnObjMap_CoverRecord_Test_002, TestSize.Level1)
{
    const auto &oprnObjMap = MediaOperation::GetOprnObjMap();
    EXPECT_EQ(oprnObjMap.count("tab_cover_record_operation"), 1);
    EXPECT_EQ(oprnObjMap.count("tab_cover_record"), 0);
    EXPECT_EQ(oprnObjMap.count(""), 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaOperation_TableNameMap_CoverRecord_Test_001, TestSize.Level1)
{
    const auto &tableNameMap = MediaOperation::GetTableNameMap();
    auto objIt = tableNameMap.find(OperationObject::TAB_COVER_RECORD);
    ASSERT_NE(objIt, tableNameMap.end());

    auto typeIt = objIt->second.find(OperationType::UNKNOWN_TYPE);
    ASSERT_NE(typeIt, objIt->second.end());
    EXPECT_EQ(typeIt->second, CoverRecordColumns::COVER_RECORD_TABLE);
    EXPECT_EQ(typeIt->second, "tab_cover_record");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaOperation_UriToTableName_CoverRecord_Test_001, TestSize.Level1)
{
    const auto &oprnObjMap = MediaOperation::GetOprnObjMap();
    const auto &tableNameMap = MediaOperation::GetTableNameMap();

    auto objIt = oprnObjMap.find(CONST_TAB_COVER_RECORD);
    ASSERT_NE(objIt, oprnObjMap.end());

    auto tableIt = tableNameMap.find(objIt->second);
    ASSERT_NE(tableIt, tableNameMap.end());

    auto typeIt = tableIt->second.find(OperationType::UNKNOWN_TYPE);
    ASSERT_NE(typeIt, tableIt->second.end());
    EXPECT_EQ(typeIt->second, CoverRecordColumns::COVER_RECORD_TABLE);
}
} // namespace Media
} // namespace OHOS
