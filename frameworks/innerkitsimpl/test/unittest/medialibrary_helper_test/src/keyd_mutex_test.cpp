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
#include "keyd_mutex.h"
#include <thread>

using namespace testing::ext;
namespace OHOS {
namespace Media {

HWTEST_F(MediaLibraryHelperUnitTest, KeydMutex_Get_SameKeyReturnsSameMutex_001, TestSize.Level1)
{
    KeydMutex<int> km;
    auto mtx1 = km.Get(1);
    auto mtx2 = km.Get(1);
    EXPECT_NE(mtx1, nullptr);
    EXPECT_EQ(mtx1.get(), mtx2.get());
}

HWTEST_F(MediaLibraryHelperUnitTest, KeydMutex_Get_DiffKeyReturnsDiffMutex_001, TestSize.Level1)
{
    KeydMutex<int> km;
    auto mtx1 = km.Get(1);
    auto mtx2 = km.Get(2);
    EXPECT_NE(mtx1.get(), mtx2.get());
}

HWTEST_F(MediaLibraryHelperUnitTest, KeydMutex_Get_CleanupOnRelease_001, TestSize.Level1)
{
    KeydMutex<int> km;
    std::mutex* mtxRaw1;
    {
        auto mtxSharedPtr1 = km.Get(1);
        mtxRaw1 = mtxSharedPtr1.get();
    }
    auto filler = km.Get(2);
    auto mtx2 = km.Get(1);
    EXPECT_NE(mtxRaw1, mtx2.get());
}

HWTEST_F(MediaLibraryHelperUnitTest, KeydMutex_Get_StringKey_001, TestSize.Level1)
{
    KeydMutex<std::string> km;
    auto mtx1 = km.Get("abc");
    auto mtx2 = km.Get("abc");
    EXPECT_EQ(mtx1.get(), mtx2.get());
}
} // Media
} // OHOS