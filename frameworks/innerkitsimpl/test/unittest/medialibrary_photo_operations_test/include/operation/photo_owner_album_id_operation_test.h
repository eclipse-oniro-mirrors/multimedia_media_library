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

#ifndef OHOS_MEDIA_PHOTO_OWNER_ALBUM_ID_OPERATIOIN_TEST_H
#define OHOS_MEDIA_PHOTO_OWNER_ALBUM_ID_OPERATIOIN_TEST_H

#include "gtest/gtest.h"

#include "photo_owner_album_id_operation.h"

namespace OHOS::Media {
class PhotoOwnerAlbumIdOperationTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

protected:
    PhotoOwnerAlbumIdOperation operation_;

private:
    const int32_t E_ERR = -1;
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_PHOTO_OWNER_ALBUM_ID_OPERATIOIN_TEST_H