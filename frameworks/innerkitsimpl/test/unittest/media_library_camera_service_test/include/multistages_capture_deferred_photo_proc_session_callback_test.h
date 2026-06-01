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

#ifndef MULTISTAGES_CAPTURE_DEFERRED_PHOTO_PROC_SESSION_CALLBACK_TEST_H
#define MULTISTAGES_CAPTURE_DEFERRED_PHOTO_PROC_SESSION_CALLBACK_TEST_H

#include <gtest/gtest.h>

#include "picture.h"
#include "picture_adapter.h"

namespace OHOS {
namespace Media {
class MultiStagesCaptureDeferredPhotoProcSessionCallbackTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MEDIALIBRARY_MULTISTAGES_CAPTURE_TEST_H