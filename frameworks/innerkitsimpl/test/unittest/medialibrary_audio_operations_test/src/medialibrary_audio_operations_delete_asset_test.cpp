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

#define MLOG_TAG "audioOperationsTest"

#include "medialibrary_audio_operations_test.h"
#include "medialibrary_audio_operations_delete_asset_test.h"

#include <memory>

#include "file_asset.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_errno.h"
#include "media_log.h"
#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {
using namespace std;
using namespace testing::ext;

void MediaLibraryAudioOperationsDeleteAssetTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("start MediaLibraryAudioOperationsDeleteAssetTest::SetUpTestCase");
}

void MediaLibraryAudioOperationsDeleteAssetTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("end MediaLibraryAudioOperationsDeleteAssetTest::TearDownTestCase");
}

void MediaLibraryAudioOperationsDeleteAssetTest::SetUp(void)
{
    MEDIA_INFO_LOG("SetUp");
}

void MediaLibraryAudioOperationsDeleteAssetTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}

HWTEST_F(MediaLibraryAudioOperationsDeleteAssetTest, AssetOperations_DeleteNormalPhotoPermanently, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd AssetOperations_DeleteNormalPhotoPermanently_Branch");
    shared_ptr<FileAsset> fileAsset = make_shared<FileAsset>();
    fileAsset->SetPath("/data/test/tdd_delete_normal_photo_nonexist.jpg");
    fileAsset->SetMediaType(MediaType::MEDIA_TYPE_IMAGE);
    int32_t ret = MediaLibraryAssetOperations::DeleteNormalPhotoPermanently(fileAsset, nullptr);
    EXPECT_NE(ret, E_OK);
    MEDIA_INFO_LOG("end tdd AssetOperations_DeleteNormalPhotoPermanently_Branch");
}
}  // namespace Media
}  // namespace OHOS
