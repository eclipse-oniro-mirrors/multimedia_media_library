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

#include "medialibrarydfx2_fuzzer.h"

#include <cstdint>
#include <string>
#include <fstream>
#include <fuzzer/FuzzedDataProvider.h>

#include "ability_context_impl.h"
#include "media_log.h"
#include "medialibrary_command.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_kvstore_manager.h"
#include "photo_album_column.h"
#include "photo_file_utils.h"

#define private public
#include "dfx_moving_photo.h"
#undef private
#include "media_upgrade.h"

namespace OHOS {
using namespace std;
using namespace AbilityRuntime;
using namespace OHOS::Media;
static const int32_t MIN_PHOTO_POSITION = 1;
static const int32_t MAX_PHOTO_POSITION = 3;
static const int32_t MAX_BYTE_VALUE = 256;
static const int32_t SEED_SIZE = 1024;
static const int64_t MIN_IMAGE_SIZE = 1 * 1000 * 1000;
static const int64_t MAX_IMAGE_SIZE = 10 * 1000 * 1000;
static const int32_t MIN_IMAGE_WIDTH = 1024;
static const int32_t MAX_IMAGE_WIDTH = 1920;
static const int32_t MIN_IMAGE_HEIGHT = 800;
static const int32_t MAX_IMAGE_HEIGHT = 1080;
static const int64_t SEC_TO_MSEC = 1e3;
static const std::string PHOTO_DIR = "/storage/cloud/files/photo/16/";
const string PHOTOS_TABLE = "Photos";
FuzzedDataProvider *provider = nullptr;
std::shared_ptr<Media::MediaLibraryRdbStore> g_rdbStore;
static std::atomic<int32_t> g_num{0};

static int64_t GetTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    ++g_num;
    return seconds.count() + g_num.load();
}

static string GetTitle(int64_t &timestamp)
{
    ++g_num;
    return "IMG_" + to_string(timestamp) + "_" + to_string(g_num.load());
}

static string InsertPhoto(const int32_t position)
{
    int64_t timestamp = GetTimestamp();
    int64_t timestampMilliSecond = timestamp * SEC_TO_MSEC;
    string title = GetTitle(timestamp);
    string displayName = title + ".jpg";
    string path = PHOTO_DIR + displayName;
    int64_t imageSize = provider->ConsumeIntegralInRange<int32_t>(MIN_IMAGE_SIZE, MAX_IMAGE_SIZE);
    int32_t imageWidth = provider->ConsumeIntegralInRange<int32_t>(MIN_IMAGE_WIDTH, MAX_IMAGE_WIDTH);
    int32_t imageHeight = provider->ConsumeIntegralInRange<int32_t>(MIN_IMAGE_HEIGHT, MAX_IMAGE_HEIGHT);
    string imageMimeType = "image/jpeg";
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutInt(MediaColumn::MEDIA_TYPE, MEDIA_TYPE_IMAGE);
    valuesBucket.PutInt(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    valuesBucket.PutInt(PhotoColumn::PHOTO_POSITION, position);
    valuesBucket.PutLong(MediaColumn::MEDIA_SIZE, imageSize);
    valuesBucket.PutInt(PhotoColumn::PHOTO_WIDTH, imageWidth);
    valuesBucket.PutInt(PhotoColumn::PHOTO_HEIGHT, imageHeight);
    valuesBucket.PutString(MediaColumn::MEDIA_MIME_TYPE, imageMimeType);
    valuesBucket.PutString(MediaColumn::MEDIA_FILE_PATH, path);
    valuesBucket.PutString(MediaColumn::MEDIA_TITLE, title);
    valuesBucket.PutString(MediaColumn::MEDIA_NAME, displayName);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_ADDED, timestampMilliSecond);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_MODIFIED, timestampMilliSecond);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_TAKEN, timestampMilliSecond);
    valuesBucket.PutLong(MediaColumn::MEDIA_TIME_PENDING, 0);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_TRASHED, 0);
    valuesBucket.PutInt(MediaColumn::MEDIA_HIDDEN, 0);
    valuesBucket.PutInt(MediaColumn::MEDIA_TIME_PENDING, 0);
    int64_t fileId = -1;
    int32_t ret = g_rdbStore->Insert(fileId, PhotoColumn::PHOTOS_TABLE, valuesBucket);
    MEDIA_INFO_LOG("ret: %{public}d, fileId: %{public}s", ret, to_string(fileId).c_str());
    std::system(("touch " + path).c_str());
    return path;
}

static void PreparePhoto(const bool hasEditDataCamera, const bool hasEditData, const bool isCloud)
{
    int32_t position = isCloud ? 3 : 1;
    string path = InsertPhoto(position);
    if (hasEditDataCamera) {
        string editDataCameraPath = PhotoFileUtils::GetEditDataCameraPath(path);
        std::system(("mkdir -p " + editDataCameraPath).c_str());
    }
    if (hasEditData) {
        string editDataPath = PhotoFileUtils::GetEditDataPath(path);
        std::system(("mkdir -p " + editDataPath).c_str());
    }
}

static void DfxMovingPhotoFuzzer()
{
    std::system(("mkdir -p " + PHOTO_DIR).c_str());
    const int32_t position = provider->ConsumeIntegralInRange<int32_t>(MIN_PHOTO_POSITION, MAX_PHOTO_POSITION);
    InsertPhoto(position);
    const uint8_t stateUpperBound = 8;
    for (uint8_t state = 0; state < stateUpperBound; ++state) {
        PreparePhoto(state & 0b100, state & 0b010, state & 0b001);
    }
    DfxMovingPhoto::AbnormalMovingPhotoStatistics();
    std::system(("rm -rf " + PHOTO_DIR + "*").c_str());
}

void SetTables()
{
    vector<string> createTableSqlList = {
        Media::PhotoUpgrade::CREATE_PHOTO_TABLE,
    };
    for (auto &createTableSql : createTableSqlList) {
        CHECK_AND_RETURN_LOG(g_rdbStore != nullptr, "g_rdbStore is null");
        int32_t ret = g_rdbStore->ExecuteSql(createTableSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Execute sql %{private}s failed", createTableSql.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Execute sql %{private}s success", createTableSql.c_str());
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
    CHECK_AND_RETURN_LOG(ret == NativeRdb::E_OK, "InitMediaLibraryMgr failed, ret: %{public}d", ret);

    auto rdbStore = Media::MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdbStore is nullptr");
        return;
    }
    g_rdbStore = rdbStore;
    SetTables();
}

static int32_t AddSeed()
{
    char *seedData = new char[OHOS::SEED_SIZE];
    for (int i = 0; i < OHOS::SEED_SIZE; i++) {
        seedData[i] = static_cast<char>(i % MAX_BYTE_VALUE);
    }

    const char* filename = "corpus/seed.txt";
    std::ofstream file(filename, std::ios::binary | std::ios::trunc);
    if (!file) {
        MEDIA_ERR_LOG("Cannot open file filename:%{public}s", filename);
        delete[] seedData;
        return Media::E_ERR;
    }
    file.write(seedData, OHOS::SEED_SIZE);
    file.close();
    delete[] seedData;
    MEDIA_INFO_LOG("seedData has been successfully written to file filename:%{public}s", filename);
    return Media::E_OK;
}

static inline void ClearKvStore()
{
    Media::MediaLibraryKvStoreManager::GetInstance().CloseAllKvStore();
}
} // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    OHOS::AddSeed();
    OHOS::Init();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    OHOS::provider = &fdp;
    if (data == nullptr) {
        return 0;
    }
    OHOS::DfxMovingPhotoFuzzer();
    OHOS::ClearKvStore();
    return 0;
}