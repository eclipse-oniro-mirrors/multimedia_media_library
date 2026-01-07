/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "medialibrarymediaphotoassetproxy_fuzzer.h"

#include <cstdint>
#include <memory>
#include <fstream>
#include <string>
#include <fuzzer/FuzzedDataProvider.h>

#include "ability_context_impl.h"
#include "access_token.h"
#include "accesstoken_kit.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"
#include "datashare_predicates.h"
#include "iservice_registry.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_command.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_errno.h"
#include "medialibrary_unistore.h"
#include "medialibrary_unistore_manager.h"
#include "rdb_store.h"
#include "system_ability_definition.h"
#include "userfilemgr_uri.h"
#include "medialibrary_kvstore_manager.h"

namespace OHOS {
using namespace std;
using namespace DataShare;
using namespace Security::AccessToken;
static const int32_t NUM_BYTES = 10;
static const int32_t MIN_VIDEO_TYPE = 1;
static const int32_t MAX_VIDEO_TYPE = 2;
static const int32_t MAX_PHOTO_QUALITY_FUZZER_LISTS = 1;
static const int32_t MAX_CAMERA_SHOT_TYPE_FUZZER_LISTS = 3;
static const int32_t MAX_PHOTO_FORMAT_FUZZER_LISTS = 4;
static const int32_t MAX_SUB_TYPE = 6;
static const int32_t MAX_BYTE_VALUE = 256;
static const int32_t SEED_SIZE = 1024;
constexpr int FUZZ_STORAGE_MANAGER_MANAGER_ID = 5003;
std::shared_ptr<DataShare::DataShareHelper> sDataShareHelper_ = nullptr;
std::shared_ptr<Media::MediaLibraryRdbStore> g_rdbStore;
FuzzedDataProvider *provider = nullptr;

static inline Media::CameraShotType FuzzCameraShotType()
{
    uint8_t data = provider->ConsumeIntegralInRange<uint8_t>(0, MAX_CAMERA_SHOT_TYPE_FUZZER_LISTS);
    return Media::CameraShotType_FUZZER_LISTS[data];
}

static inline Media::PhotoFormat FuzzPhotoFormat()
{
    uint8_t data = provider->ConsumeIntegralInRange<uint8_t>(0, MAX_PHOTO_FORMAT_FUZZER_LISTS);
    return Media::PhotoFormat_FUZZER_LISTS[data];
}

static inline Media::PhotoQuality FuzzPhotoQuality()
{
    uint8_t data = provider->ConsumeIntegralInRange<uint8_t>(0, MAX_PHOTO_QUALITY_FUZZER_LISTS);
    return Media::PhotoQuality_FUZZER_LISTS[data];
}

static inline Media::VideoType FuzzVideoType()
{
    uint8_t data = provider->ConsumeIntegralInRange<uint8_t>(MIN_VIDEO_TYPE, MAX_VIDEO_TYPE);
    return static_cast<Media::VideoType>(data);
}

static inline Media::PhotoSubType FuzzPhotoSubType()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0, MAX_SUB_TYPE);
    return static_cast<Media::PhotoSubType>(value);
}

void CreateDataHelper(int32_t systemAbilityId)
{
    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (saManager == nullptr) {
        MEDIA_ERR_LOG("Get system ability mgr failed.");
        return;
    }
    auto remoteObj = saManager->GetSystemAbility(systemAbilityId);
    if (remoteObj == nullptr) {
        MEDIA_ERR_LOG("GetSystemAbility Service Failed.");
        return;
    }

    if (sDataShareHelper_ == nullptr) {
        const sptr<IRemoteObject> &token = remoteObj;
        sDataShareHelper_ = DataShare::DataShareHelper::Creator(token, Media::MEDIALIBRARY_DATA_URI);
    }
}

static shared_ptr<Media::PhotoAssetProxy> Init()
{
    Media::PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = provider->ConsumeIntegral<int32_t>(),
        .userId = provider->ConsumeIntegral<int32_t>(),
        .callingTokenId = provider->ConsumeIntegral<uint32_t>(),
    };
    shared_ptr<Media::PhotoAssetProxy> photoAssetProxy = make_shared<Media::PhotoAssetProxy>(
        sDataShareHelper_, callerInfo, FuzzCameraShotType(), 1);
    return photoAssetProxy;
}

static sptr<Media::PhotoProxyFuzzTest> FuzzPhotoAssetProxy()
{
    sptr<Media::PhotoProxyFuzzTest> photoProxyFuzzTest = new(std::nothrow) Media::PhotoProxyFuzzTest();
    if (photoProxyFuzzTest == nullptr) {
        return nullptr;
    }
    photoProxyFuzzTest->SetFormat(FuzzPhotoFormat());
    photoProxyFuzzTest->SetPhotoQuality(FuzzPhotoQuality());
    photoProxyFuzzTest->SetShootingMode(static_cast<int32_t>(FuzzCameraShotType()));
    photoProxyFuzzTest->SetBurstKey(provider->ConsumeBytesAsString(NUM_BYTES));

    return photoProxyFuzzTest;
}

static void MediaLibraryMediaPhotoAssetProxyTest()
{
    MEDIA_INFO_LOG("MediaLibraryMediaPhotoAssetProxyTest start");
    if (sDataShareHelper_ == nullptr) {
        CreateDataHelper(FUZZ_STORAGE_MANAGER_MANAGER_ID);
        CHECK_AND_RETURN_LOG(sDataShareHelper_ != nullptr, "CreateDataHelper failed");
    }
    shared_ptr<Media::PhotoAssetProxy> photoAssetProxy = Init();
    if (photoAssetProxy == nullptr) {
        return;
    }
    sptr<Media::PhotoProxyFuzzTest> photoProxyFuzzTest = FuzzPhotoAssetProxy();
    photoAssetProxy->AddPhotoProxy((sptr<Media::PhotoProxy>&)photoProxyFuzzTest);
    photoAssetProxy->GetVideoFd(FuzzVideoType());
    photoAssetProxy->NotifyVideoSaveFinished(FuzzVideoType());
    photoAssetProxy->UpdatePhotoProxy((sptr<Media::PhotoProxy>&)photoProxyFuzzTest);

    int32_t fileId = provider->ConsumeIntegral<int32_t>();
    int32_t subType = static_cast<int32_t>(FuzzPhotoSubType());
    photoAssetProxy->SaveLowQualityPhoto(sDataShareHelper_, photoProxyFuzzTest, fileId, subType);
    
    uint8_t *data = new uint8_t();
    uint32_t size = sizeof(uint8_t);
    int fd = provider->ConsumeIntegral<int32_t>();
    photoAssetProxy->SetShootingModeAndGpsInfo(data, size, (sptr<Media::PhotoProxy>&)photoProxyFuzzTest, fd);
    delete data;
    data = nullptr;
    MEDIA_INFO_LOG("MediaLibraryMediaPhotoAssetProxyTest end");
}

void SetTables()
{
    vector<string> createTableSqlList = {
        Media::PhotoColumn::CREATE_PHOTO_TABLE,
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

static void RdbStoreInit()
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
    SetTables();
}

std::vector<OHOS::Security::AccessToken::PermissionStateFull> DefinePermissionStates()
{
    return {
        {
            .permissionName = "ohos.permission.SHORT_TERM_WRITE_IMAGEVIDEO",
            .isGeneral = true,
            .resDeviceID = { "local" },
            .grantStatus = { OHOS::Security::AccessToken::PermissionState::PERMISSION_GRANTED },
            .grantFlags = { 1 }
        },
        {
            .permissionName = "ohos.permission.READ_IMAGEVIDEO",
            .isGeneral = true,
            .resDeviceID = { "local" },
            .grantStatus = { OHOS::Security::AccessToken::PermissionState::PERMISSION_GRANTED },
            .grantFlags = { 1 }
        },
        {
            .permissionName = "ohos.permission.WRITE_IMAGEVIDEO",
            .isGeneral = true,
            .resDeviceID = { "local" },
            .grantStatus = { OHOS::Security::AccessToken::PermissionState::PERMISSION_GRANTED },
            .grantFlags = { 1 }
        },
        {
            .permissionName = "ohos.permission.READ_MEDIA",
            .isGeneral = true,
            .resDeviceID = { "local" },
            .grantStatus = { OHOS::Security::AccessToken::PermissionState::PERMISSION_GRANTED },
            .grantFlags = { 1 }
        },
        {
            .permissionName = "ohos.permission.WRITE_MEDIA",
            .isGeneral = true,
            .resDeviceID = { "local" },
            .grantStatus = { OHOS::Security::AccessToken::PermissionState::PERMISSION_GRANTED },
            .grantFlags = { 1 }
        },
        {
            .permissionName = "ohos.permission.GET_BUNDLE_INFO_PRIVILEGED",
            .isGeneral = true,
            .resDeviceID = { "local" },
            .grantStatus = { OHOS::Security::AccessToken::PermissionState::PERMISSION_GRANTED },
            .grantFlags = { 1 }
        }
    };
}

static void SetHapPermission()
{
    MEDIA_INFO_LOG("enter SetHapPermission");
    OHOS::Security::AccessToken::HapInfoParams info = {
        .userID = 100,
        .bundleName = "com.ohos.test.medialibrary",
        .instIndex = 0,
        .appIDDesc = "com.ohos.test.medialibrary",
        .isSystemApp = true
    };

    OHOS::Security::AccessToken::HapPolicyParams policy = {
        .apl = Security::AccessToken::APL_SYSTEM_BASIC,
        .domain = "test.domain.medialibrary",
        .permList = { },
        .permStateList = DefinePermissionStates()
    };
    OHOS::Security::AccessToken::AccessTokenIDEx tokenIdEx = { 0 };
    tokenIdEx = OHOS::Security::AccessToken::AccessTokenKit::AllocHapToken(info, policy);
    int ret = SetSelfTokenID(tokenIdEx.tokenIDEx);
    if (ret != 0) {
        MEDIA_INFO_LOG("Set hap token failed, err: %{public}d", ret);
    }
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
        seedData = nullptr;
        return Media::E_ERR;
    }
    file.write(seedData, OHOS::SEED_SIZE);
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
} // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    OHOS::SetHapPermission();
    OHOS::AddSeed();
    OHOS::RdbStoreInit();
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    FuzzedDataProvider fdp(data, size);
    OHOS::provider = &fdp;
    if (data == nullptr) {
        return 0;
    }
    OHOS::MediaLibraryMediaPhotoAssetProxyTest();
    OHOS::ClearKvStore();
    return 0;
}