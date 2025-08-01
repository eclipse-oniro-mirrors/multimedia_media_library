/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef FRAMEWORKS_INNERKITSIMPL_TEST_UNITTEST_MEDIALIBRARY_HELPER_TEST_INCLUDE_MEDIALIBRARY_ALBUM_HELPER_TEST_H
#define FRAMEWORKS_INNERKITSIMPL_TEST_UNITTEST_MEDIALIBRARY_HELPER_TEST_INCLUDE_MEDIALIBRARY_ALBUM_HELPER_TEST_H

#include "gtest/gtest.h"

namespace OHOS {
namespace Media {
class MediaLibraryAlbumHelperUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};
} // namespace Media
} // namespace OHOS
#endif // FRAMEWORKS_INNERKITSIMPL_TEST_UNITTEST_MEDIALIBRARY_HELPER_TEST_INCLUDE_MEDIALIBRARY_ALBUM_HELPER_TEST_H
