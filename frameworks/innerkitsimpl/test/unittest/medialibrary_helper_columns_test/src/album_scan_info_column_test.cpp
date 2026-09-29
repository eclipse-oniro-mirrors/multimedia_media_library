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

#include "medialibrary_helper_test.h"

#include "album_scan_info_column.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
HWTEST_F(MediaLibraryHelperUnitTest, AlbumScanInfoColumn_Coverage_001, TestSize.Level1)
{
    EXPECT_FALSE(AlbumScanInfoColumn::TABLE.empty());
    EXPECT_EQ(AlbumScanInfoColumn::TABLE, "AlbumScanInfo");

    EXPECT_FALSE(AlbumScanInfoColumn::ID.empty());
    EXPECT_FALSE(AlbumScanInfoColumn::ALBUM_ID.empty());
    EXPECT_FALSE(AlbumScanInfoColumn::STORAGE_PATH.empty());
    EXPECT_FALSE(AlbumScanInfoColumn::FOLDER_DATE_MODIFIED.empty());

    EXPECT_FALSE(AlbumScanInfoColumn::CREATE_TABLE.empty());
    EXPECT_NE(AlbumScanInfoColumn::CREATE_TABLE.find("CREATE TABLE IF NOT EXISTS AlbumScanInfo"),
        std::string::npos);

    EXPECT_FALSE(AlbumScanInfoColumn::CREATE_INDEX_ON_ALBUM_ID_STORAGE_PATH.empty());
    EXPECT_NE(AlbumScanInfoColumn::CREATE_INDEX_ON_ALBUM_ID_STORAGE_PATH.find(
        "CREATE UNIQUE INDEX IF NOT EXISTS idx_AlbumScanInfo_album_id_storage_path"),
        std::string::npos);
}
} // namespace Media
} // namespace OHOS
