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

#ifndef UNIT_TEST_MULTISTAGES_CAPTURE_NOTIFY_H
#define UNIT_TEST_MULTISTAGES_CAPTURE_NOTIFY_H

#include <gtest/gtest.h>

namespace OHOS {
namespace Media {
class MultiStagesCaptureNotifyTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

struct PhotoAsset {
    int64_t fileId{-1};
    std::string path;
    std::string displayName;
    int32_t mediaType;
};
} // namespace Media
} // namespace OHOS
#endif  // UNIT_TEST_MULTISTAGES_CAPTURE_NOTIFY_H