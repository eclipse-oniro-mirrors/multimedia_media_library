/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "medialibrary_multistages_deferred_capture_fuzzer.h"
#include "medialibrary_rdbstore_utils_fuzzer.h"

#include <cstdint>
#include <string>
#include <thread>
#include <vector>
#include <fuzzer/FuzzedDataProvider.h>
#include "ability_context_impl.h"
#include "media_log.h"
#include "rdb_utils.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_kvstore_manager.h"
#include "picture_adapter.h"
#define private public
#define protected public
#include "deferred_photo_proc_session.h"
#include "deferred_photo_proc_adapter.h"
#include "deferred_video_proc_adapter.h"
#undef private
#undef protected
#include "media_upgrade.h"

namespace OHOS {
using namespace std;
using namespace Media;
static const int32_t NUM_BYTES = 1;
static const string PHOTOS_TABLE = "Photos";
FuzzedDataProvider *provider = nullptr;
std::shared_ptr<Media::MediaLibraryRdbStore> g_rdbStore;

void SetTables()
{
    vector<string> createTableSqlList = { Media::PhotoUpgrade::CREATE_PHOTO_TABLE };
    for (auto &createTableSql : createTableSqlList) {
        CHECK_AND_RETURN_LOG(g_rdbStore != nullptr, "g_rdbStore is null.");
        int32_t ret = g_rdbStore->ExecuteSql(createTableSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Execute sql %{private}s failed.", createTableSql.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Execute sql %{private}s success.", createTableSql.c_str());
    }
}

static void Init()
{
    auto stageContext = std::make_shared<AbilityRuntime::ContextImpl>();
    auto abilityContextImpl = std::make_shared<OHOS::AbilityRuntime::AbilityContextImpl>();
    abilityContextImpl->SetStageContext(stageContext);
    auto rdbStore = Media::MediaLibraryRdbStoreUtilsTest::InitMediaLibraryRdbStore(abilityContextImpl);
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdbStore is nullptr.");
        return;
    }
    g_rdbStore = rdbStore;
    SetTables();
}

static void MultistagesCaptureDeferredPhotoProcAdapterTest()
{
    MEDIA_INFO_LOG("MultistagesCaptureDeferredPhotoProcAdapterTest start");
    shared_ptr<Media::DeferredPhotoProcessingAdapter> deferredProcSession =
        make_shared<Media::DeferredPhotoProcessingAdapter>();
    if (provider->ConsumeBool()) {
        deferredProcSession->deferredPhotoProcSession_ = nullptr;
    }
    deferredProcSession->BeginSynchronize();
    deferredProcSession->EndSynchronize();
    std::string photoId = provider->ConsumeBytesAsString(NUM_BYTES);
    std::string appName = provider->ConsumeBytesAsString(NUM_BYTES);
    deferredProcSession->RestoreImage(photoId);
    deferredProcSession->ProcessImage(appName, photoId);
    deferredProcSession->CancelProcessImage(photoId);
    MEDIA_INFO_LOG("MultistagesCaptureDeferredPhotoProcAdapterTest end");
}

static void MultistagesCaptureDeferredVideoProcAdapterTest()
{
    MEDIA_INFO_LOG("MultistagesCaptureDeferredVideoProcAdapterTest start");
    shared_ptr<Media::DeferredVideoProcessingAdapter> deferredProcSession =
        make_shared<Media::DeferredVideoProcessingAdapter>();
    if (provider->ConsumeBool()) {
        deferredProcSession->deferredVideoProcSession_ = nullptr;
    }
    deferredProcSession->BeginSynchronize();
    std::string videoId = provider->ConsumeBytesAsString(NUM_BYTES);
    std::string srcPath1 =
        "/data/service/el2/100/hmdfs/account/files/.editData/Photo/4/VID_1778744376_004.mp4/source.mp4";
    std::string srcPath2 = "/data/service/el2/100/hmdfs/account/files/Photo/4/VID_1778744376_004.mp4";
    std::vector<std::string> srcPath = {srcPath1, srcPath2};
    std::string tmp1Path = "/data/service/el2/100/hmdfs/account/files/cameraCache/temp/VID_1778744376_004_tmp1.mp4";
    std::string tmp2Path = "/data/service/el2/100/hmdfs/account/files/cameraCache/temp/VID_1778744376_004_tmp2.mp4";
    deferredProcSession->AddVideo(videoId, srcPath, tmp1Path, tmp2Path);
    deferredProcSession->RemoveVideo(videoId, provider->ConsumeBool());
    deferredProcSession->RestoreVideo(videoId);
    deferredProcSession->EndSynchronize();
    MEDIA_INFO_LOG("MultistagesCaptureDeferredVideoProcAdapterTest end");
}
} // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    OHOS::Init();
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    OHOS::provider = &fdp;
    if (data == nullptr) {
        return 0;
    }
    OHOS::MultistagesCaptureDeferredPhotoProcAdapterTest();
    OHOS::MultistagesCaptureDeferredVideoProcAdapterTest();
    return 0;
}