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

#include "gtest/gtest.h"

#include "clone_restore.h"

using namespace testing::ext;
namespace OHOS {
namespace Media {
class CloneRestoreGalleryExecyteTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() {}
    void TearDown() {}
};

HWTEST_F(CloneRestoreGalleryExecyteTest, RestoreGalleryExecyte_NullRdb_NoCrash, TestSize.Level1)
{
    CloneRestore restore;
    restore.mediaRdb_ = nullptr;
    restore.mediaLibraryRdb_ = nullptr;
    EXPECT_NO_FATAL_FAILURE(restore.RestoreGalleryExecyte());
}

HWTEST_F(CloneRestoreGalleryExecyteTest, RestoreGalleryExecyte_DefaultCtor_NoCrash, TestSize.Level1)
{
    CloneRestore restore;
    EXPECT_NO_FATAL_FAILURE(restore.RestoreGalleryExecyte());
}
} // namespace Media
} // namespace OHOS
