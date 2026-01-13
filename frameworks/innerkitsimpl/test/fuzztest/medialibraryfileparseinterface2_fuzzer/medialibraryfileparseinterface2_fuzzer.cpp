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

#include "medialibraryfileparseinterface2_fuzzer.h"

#include <cstdint>
#include <memory>
#include <fstream>
#include <string>
#include <fuzzer/FuzzedDataProvider.h>

#define private public
#define protected public
#include "media_file_utils.h"
#include "media_library_manager.h"
#include "mimetype_utils.h"
#undef private
#undef protected

#include <ani.h>
#include "avmetadatahelper.h"
#include "ability_context_impl.h"
#include "datashare_predicates.h"
#include "iservice_registry.h"
#include "media_log.h"
#include "medialibrary_command.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_errno.h"
#include "medialibrary_unistore.h"
#include "medialibrary_unistore_manager.h"
#include "rdb_store.h"
#include "rdb_utils.h"
#include "userfile_manager_types.h"
#include "values_bucket.h"
#include "system_ability_definition.h"
#include "userfilemgr_uri.h"
#include "medialibrary_kvstore_manager.h"
#include "fetch_result.h"
#include "medialibrary_photo_operations.h"
#include "result_set_utils.h"
#include "media_library_extend_manager.h"

namespace OHOS {
namespace Media {
using namespace std;
using namespace DataShare;
static const int32_t NUM_BYTES = 1;
static const int32_t NUM_16 = 16;
static const int32_t MAX_BYTE_VALUE = 256;
static const int32_t MAX_DYNAMIC_RANGE = 2;
static const int32_t SEED_SIZE = 1024;
const string PHOTOS_TABLE = "Photos";
std::shared_ptr<Media::MediaLibraryRdbStore> g_rdbStore;
FuzzedDataProvider *provider = nullptr;

static void MimeTypeUtilsTest()
{
    MEDIA_INFO_LOG("MimeTypeUtilsTest start");
    string filePath = ROOT_MEDIA_DIR +
        to_string(provider->ConsumeIntegralInRange<int32_t>(1, NUM_16)) + "/" +
        provider->ConsumeBytesAsString(NUM_BYTES) + ".jpg";
    string res;
    MimeTypeUtils::GetImageMimetype(filePath, res);
    MimeTypeUtils::GetVideoMimetype(filePath, res);
    MEDIA_INFO_LOG("MimeTypeUtilsTest end");
}

static void MediaLibraryManagerTest()
{
    MEDIA_INFO_LOG("MediaLibraryManagerTest start");
    string filePath = ROOT_MEDIA_DIR + "test.jpg";
    MediaFileUtils::CreateFile(filePath);
    MediaFileUtils::WriteStrToFile(filePath, provider->ConsumeBytesAsString(NUM_BYTES));
    int32_t fd = MediaFileUtils::OpenFile(filePath, MEDIA_FILEMODE_READWRITE);
    UniqueFd uniqueFd(fd);
    Size size = { provider->ConsumeIntegralInRange<int32_t>(1, SEED_SIZE),
        provider->ConsumeIntegralInRange<int32_t>(1, SEED_SIZE) };
    DecodeDynamicRange dynamicRange = static_cast<DecodeDynamicRange>(
        provider->ConsumeIntegralInRange<int32_t>(0, MAX_DYNAMIC_RANGE));
    MediaLibraryManager::DecodeThumbnail(uniqueFd, size, dynamicRange);
    MediaLibraryManager::DecodeAstc(uniqueFd);
    MEDIA_INFO_LOG("MediaLibraryManagerTest end");
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
        return;
    }
    g_rdbStore = rdbStore;
}

static int32_t AddSeed()
{
    char *seedData = new char[SEED_SIZE];
    for (int i = 0; i < SEED_SIZE; i++) {
        seedData[i] = static_cast<char>(i % MAX_BYTE_VALUE);
    }

    const char* filename = "corpus/seed.txt";
    std::ofstream file(filename, std::ios::binary | std::ios::trunc);
    if (!file) {
        MEDIA_ERR_LOG("Cannot open file filename:%{public}s", filename);
        delete[] seedData;
        seedData = nullptr;
        return Media::E_ERR;
    }
    file.write(seedData, SEED_SIZE);
    file.close();
    delete[] seedData;
    seedData = nullptr;
    MEDIA_INFO_LOG("seedData has been successfully written to file filename:%{public}s", filename);
    return Media::E_OK;
}

static inline void ClearKvStore()
{
    Media::MediaLibraryKvStoreManager::GetInstance().CloseAllKvStore();
}
} // namespace Media
} // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    OHOS::Media::AddSeed();
    OHOS::Media::Init();
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    FuzzedDataProvider fdp(data, size);
    OHOS::Media::provider = &fdp;
    if (data == nullptr) {
        return 0;
    }
    OHOS::Media::MimeTypeUtilsTest();
    OHOS::Media::MediaLibraryManagerTest();
    OHOS::Media::ClearKvStore();
    return 0;
}