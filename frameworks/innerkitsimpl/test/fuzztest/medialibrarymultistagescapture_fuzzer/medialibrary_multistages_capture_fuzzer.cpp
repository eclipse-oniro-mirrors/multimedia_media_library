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
#include "medialibrary_multistages_capture_fuzzer.h"

#include <cstdint>
#include <string>
#include <thread>
#include <fuzzer/FuzzedDataProvider.h>
#include "ability_context_impl.h"
#include "media_log.h"
#include "rdb_utils.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_unistore_manager.h"
#include "multistages_capture_manager.h"
#include "multistages_moving_photo_capture_manager.h"
#include "multistages_photo_capture_manager.h"
#include "multistages_video_capture_manager.h"
#include "multistages_capture_request_task_manager.h"

namespace OHOS {
using namespace std;
static const int32_t E_ERR = -1;
static const int32_t NUM_BYTES = 1;
static const string PHOTOS_TABLE = "Photos";
FuzzedDataProvider *provider = nullptr;
std::shared_ptr<Media::MediaLibraryRdbStore> g_rdbStore;

static inline Uri FuzzUri()
{
    return Uri(provider->ConsumeBytesAsString(NUM_BYTES));
}

static inline Media::MediaLibraryCommand FuzzMediaLibraryCmd()
{
    return Media::MediaLibraryCommand(FuzzUri());
}

static int32_t InsertAsset(string photoId)
{
    if (g_rdbStore == nullptr) {
        return E_ERR;
    }
    NativeRdb::ValuesBucket values;
    values.PutString(Media::PhotoColumn::PHOTO_ID, photoId);
    values.PutString(Media::MediaColumn::MEDIA_FILE_PATH, provider->ConsumeBytesAsString(NUM_BYTES));
    int64_t fileId = 0;
    g_rdbStore->Insert(fileId, PHOTOS_TABLE, values);
    return static_cast<int32_t>(fileId);
}

void SetTables()
{
    vector<string> createTableSqlList = { Media::PhotoColumn::CREATE_PHOTO_TABLE };
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
    int32_t sceneCode = 0;
    auto ret = Media::MediaLibraryDataManager::GetInstance()->InitMediaLibraryMgr(abilityContextImpl,
        abilityContextImpl, sceneCode);
    CHECK_AND_RETURN_LOG(ret == NativeRdb::E_OK, "InitMediaLibrary Mgr failed, ret: %{public}d.", ret);
    auto rdbStore = Media::MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdbStore is nullptr.");
        return;
    }
    g_rdbStore = rdbStore;
    SetTables();
}

static void MultistagesCaptureManagerTest()
{
    Media::MediaLibraryCommand cmd = FuzzMediaLibraryCmd();
    auto rdbStore = Media::MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "Failed to get rdb store.");
    std::string photoId = provider->ConsumeBytesAsString(NUM_BYTES);
    int32_t fileId = InsertAsset(photoId);
    MEDIA_DEBUG_LOG("fileId: %{public}d.", fileId);
    Media::MultiStagesCaptureRequestTaskManager::AddPhotoInProgress(fileId, photoId, false);
    NativeRdb::RdbPredicates rdbPredicate(PHOTOS_TABLE);
    rdbPredicate.EqualTo(Media::MediaColumn::MEDIA_ID, fileId);
    rdbPredicate.EqualTo(Media::PhotoColumn::PHOTO_ID, photoId);
    Media::MultiStagesCaptureManager::RemovePhotos(rdbPredicate, true);
    Media::MultiStagesCaptureManager::RestorePhotos(rdbPredicate);
    Media::MultiStagesCaptureManager::QuerySubType(photoId);
}

static void MultistagesMovingPhotoCaptureManagerTest()
{
    std::string photoId = provider->ConsumeBytesAsString(NUM_BYTES);
    Media::MultiStagesMovingPhotoCaptureManager::SaveMovingPhotoVideoFinished(photoId);
    Media::MultiStagesMovingPhotoCaptureManager::AddVideoFromMovingPhoto(photoId);
}

static void MultistagesPhotoCaptureManagerTest()
{
    Media::MediaLibraryCommand cmd = FuzzMediaLibraryCmd();
    Media::MultiStagesPhotoCaptureManager &instance =
        Media::MultiStagesPhotoCaptureManager::GetInstance();
    instance.UpdateDbInfo(cmd);
    std::string photoId = provider->ConsumeBytesAsString(NUM_BYTES);
    instance.CancelProcessRequest(photoId);
    int32_t fileId = InsertAsset(photoId);
    MEDIA_DEBUG_LOG("fileId: %{public}d.", fileId);
    int32_t deferredProcType = provider->ConsumeIntegral<int32_t>();
    instance.AddImage(fileId, photoId, deferredProcType);
    instance.IsPhotoDeleted(photoId);
}

static void MultistagesVideoCaptureManagerTest()
{
    std::string videoId = provider->ConsumeBytesAsString(NUM_BYTES);
    std::string filePath = provider->ConsumeBytesAsString(NUM_BYTES);
    Media::MultiStagesVideoCaptureManager &instance =
        Media::MultiStagesVideoCaptureManager::GetInstance();
    int32_t fileId = InsertAsset(videoId);
    instance.AddVideo(videoId, std::to_string(fileId), filePath);
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
    int sleepTime = 100;
    std::this_thread::sleep_for(std::chrono::microseconds(sleepTime));
    OHOS::MultistagesCaptureManagerTest();
    OHOS::MultistagesMovingPhotoCaptureManagerTest();
    OHOS::MultistagesPhotoCaptureManagerTest();
    OHOS::MultistagesVideoCaptureManagerTest();
    return 0;
}