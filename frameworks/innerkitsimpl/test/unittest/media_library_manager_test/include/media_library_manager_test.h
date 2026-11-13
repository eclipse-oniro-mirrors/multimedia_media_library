/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef MEDIA_LIBRARY_MANANGER_TEST_H
#define MEDIA_LIBRARY_MANANGER_TEST_H

#include <string>

#include "gtest/gtest.h"
#include "image_source.h"
#include "media_library_manager.h"

namespace OHOS {
namespace Media {
class MediaLibraryManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    MediaLibraryManager manager;
};

struct UriParams {
    std::string path;
    std::string fileUri;
    Size size;
    bool isAstc;
    DecodeDynamicRange dynamicRange;
    std::string user;
};
} // namespace Media
} // namespace OHOS
#endif // MEDIA_LIBRARY_MANANGER_TEST_H
