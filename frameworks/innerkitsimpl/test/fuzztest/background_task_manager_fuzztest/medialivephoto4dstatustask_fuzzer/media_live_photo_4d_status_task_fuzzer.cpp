/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "media_live_photo_4d_status_task_fuzzer.h"
#include "medialibrary_rdbstore_utils_fuzzer.h"

#include <cstdint>
#include <string>
#include <fstream>
#include <fuzzer/FuzzedDataProvider.h>

#include "media_column.h"
#include "media_log.h"
#include "media_upgrade.h"
#include "medialibrary_errno.h"
#include "medialibrary_type_const.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_subscriber.h"
#include "media_live_photo_4d_status_task.h"

namespace OHOS::Media {
using namespace std;
static const int32_t MIN_PHOTO_POSITION_TYPE = -1;
static const int32_t MAX_PHOTO_POSITION_TYPE = 3;
static const int32_t MAX_BYTE_VALUE = 256;
static const int32_t SEED_SIZE = 1024;
static const string PHOTOS_TABLE = "Photos";
FuzzedDataProvider* provider = nullptr;
std::shared_ptr<Media::MediaLibraryRdbStore> g_rdbStore;

static inline PhotoPositionType FuzzPhotoPositionType()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(MIN_PHOTO_POSITION_TYPE, MAX_PHOTO_POSITION_TYPE);
    return static_cast<PhotoPositionType>(value);
}

static int32_t InsertPhotoAsset()
{
    if (g_rdbStore == nullptr) {
        return E_ERR;
    }
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    values.PutInt(PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_STATUS,
        static_cast<int32_t>(LivePhoto4dStatusType::TYPE_UNIDENTIFIED));
    values.PutInt(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(FuzzPhotoPositionType()));
    int64_t fileId = 0;
    int32_t ret = g_rdbStore->Insert(fileId, PHOTOS_TABLE, values);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_ERR, "g_rdbStore failed to insert ret=%{public}d", ret);
    return static_cast<int32_t>(fileId);
}

bool MedialibrarySubscriber::IsCurrentStatusOn()
{
    MEDIA_INFO_LOG("enter");
    return provider->ConsumeBool();
}

static void MediaLivePhoto4dStatusTaskTest()
{
    int32_t fileId = InsertPhotoAsset();
    CHECK_AND_RETURN_LOG(fileId > 0, "fileId is Invalid.");
    Background::MediaLivePhoto4dStatusTask task;
    task.Execute();
}

void SetTables()
{
    vector<string> createTableSqlList = {
        Media::PhotoUpgrade::CREATE_PHOTO_TABLE,
    };
    for (auto &createTableSql : createTableSqlList) {
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
    auto rdbStore = Media::MediaLibraryRdbStoreUtilsTest::InitMediaLibraryRdbStore(abilityContextImpl);
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdbStore is nullptr");
        return;
    }
    g_rdbStore = rdbStore;
    SetTables();
}

static int32_t AddSeed()
{
    std::unique_ptr<char[]> seedData = std::make_unique<char[]>(SEED_SIZE);
    for (int i = 0; i < SEED_SIZE; i++) {
        seedData[i] = static_cast<char>(i % MAX_BYTE_VALUE);
    }

    const char* filename = "corpus/seed.txt";
    std::ofstream file(filename, std::ios::binary | std::ios::trunc);
    if (!file) {
        MEDIA_ERR_LOG("Cannot open file filename:%{public}s", filename);
        return Media::E_ERR;
    }
    file.write(seedData.get(), SEED_SIZE);
    file.close();
    MEDIA_INFO_LOG("seedData has been successfully written to file filename:%{public}s", filename);
    return Media::E_OK;
}
} // namespace OHOS::Media

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    OHOS::Media::AddSeed();
    OHOS::Media::Init();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    CHECK_AND_RETURN_RET_LOG(data != nullptr, OHOS::Media::E_ERR, "data is nullptr");
    FuzzedDataProvider fdp(data, size);
    OHOS::Media::provider = &fdp;
    CHECK_AND_RETURN_RET_LOG(OHOS::Media::provider != nullptr, OHOS::Media::E_ERR, "provider is nullptr");

    OHOS::Media::MediaLivePhoto4dStatusTaskTest();
    return 0;
}