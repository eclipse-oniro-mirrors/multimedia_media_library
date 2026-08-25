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

#include "media_fileinterwork_column.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
HWTEST_F(MediaLibraryHelperUnitTest, MediaFileInterworkColumn_Coverage_001, TestSize.Level1)
{
    EXPECT_EQ(MediaFileInterworkColumn::OPT_TABLE_NAME, "tab_file_opt");
    EXPECT_FALSE(MediaFileInterworkColumn::CREATE_FILE_OPT_TABLE.empty());
    EXPECT_NE(MediaFileInterworkColumn::CREATE_FILE_OPT_TABLE.find("CREATE TABLE IF NOT EXISTS tab_file_opt"),
        std::string::npos);

    EXPECT_FALSE(MediaFileInterworkColumn::ID_COLUMN.empty());
    EXPECT_FALSE(MediaFileInterworkColumn::OPT_COLUMN.empty());
    EXPECT_FALSE(MediaFileInterworkColumn::BEFORE_PATH_COLUMN.empty());
    EXPECT_FALSE(MediaFileInterworkColumn::AFTER_PATH_COLUMN.empty());
    EXPECT_FALSE(MediaFileInterworkColumn::OPT_STATUS_COLUMN.empty());

    EXPECT_FALSE(MediaFileInterworkColumn::FILE_ROOT_DIR.empty());
    EXPECT_FALSE(MediaFileInterworkColumn::HO_DATA_DIR.empty());
    EXPECT_FALSE(MediaFileInterworkColumn::THUMBS_DIR.empty());
    EXPECT_FALSE(MediaFileInterworkColumn::RECENT_DIR.empty());
    EXPECT_FALSE(MediaFileInterworkColumn::BACKUP_DIR.empty());
    EXPECT_FALSE(MediaFileInterworkColumn::TRASH_DIR_DIR.empty());
    EXPECT_FALSE(MediaFileInterworkColumn::VM_DOCS_DIR.empty());
    EXPECT_FALSE(MediaFileInterworkColumn::OHPM_DIR.empty());
    EXPECT_FALSE(MediaFileInterworkColumn::PCE_ENGINE_DIR.empty());
    EXPECT_FALSE(MediaFileInterworkColumn::APPDATA_DIR.empty());
}
} // namespace Media
} // namespace OHOS
