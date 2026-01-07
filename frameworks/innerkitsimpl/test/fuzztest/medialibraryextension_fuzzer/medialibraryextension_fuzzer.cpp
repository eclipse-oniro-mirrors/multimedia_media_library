/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "medialibraryextension_fuzzer.h"

#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <fuzzer/FuzzedDataProvider.h>

#include "ability_context_impl.h"
#include "data_ability_observer_interface.h"
#include "datashare_business_error.h"
#include "datashare_helper.h"
#include "datashare_predicates.h"
#include "datashare_values_bucket.h"
#include "media_datashare_ext_ability.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_kvstore_manager.h"
#include "media_datashare_stub_impl.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "runtime.h"
#include "js_runtime.h"

namespace OHOS {
using namespace std;
using namespace AbilityRuntime;
using namespace DataShare;
static const int32_t NUM_BYTES = 1;
static const int32_t DEFAULT_URI_LISTS = 1;
static const int32_t DEFAULT_MEDIA_OPEN_MODES = 1;
FuzzedDataProvider *provider = nullptr;

static inline vector<int32_t> FuzzVectorInt32()
{
    return {provider->ConsumeIntegral<int32_t>()};
}

static inline vector<int64_t> FuzzVectorInt64()
{
    return {provider->ConsumeIntegral<int64_t>()};
}

static inline vector<double> FuzzVectorDouble()
{
    return {provider->ConsumeFloatingPoint<double>()};
}

static inline vector<string> FuzzVectorString()
{
    return {provider->ConsumeBytesAsString(NUM_BYTES)};
}

static inline vector<SingleValue::Type> FuzzVectorSingleValueType()
{
    return {provider->ConsumeIntegral<int64_t>()};
}

static inline vector<MutliValue::Type> FuzzVectorMultiValueType()
{
    return {
        FuzzVectorInt32(),
        FuzzVectorInt64(),
        FuzzVectorString(),
        FuzzVectorDouble()
    };
}

static inline OperationItem FuzzOperationItem()
{
    return {
        .operation = provider->ConsumeIntegral<int32_t>(),
        .singleParams = FuzzVectorSingleValueType(),
        .multiParams = FuzzVectorMultiValueType(),
    };
}

static inline vector<OperationItem> FuzzVectorOperationItem()
{
    return {FuzzOperationItem()};
}

static inline Uri FuzzUri()
{
    uint8_t data = provider->ConsumeIntegralInRange<uint8_t>(0,
        static_cast<uint8_t>(Media::EXTENSION_FUZZER_URI_LISTS.size() - DEFAULT_URI_LISTS));
    return Uri(Media::EXTENSION_FUZZER_URI_LISTS[data]);
}

static inline DataSharePredicates FuzzDataSharePredicates()
{
    DataSharePredicates predicates(FuzzVectorOperationItem());
    predicates.SetSettingMode(SettingMode::QUERY_LANGUAGE);
    predicates.SetWhereArgs(FuzzVectorString());
    return predicates;
}

static inline DataShareValuesBucket FuzzDataShareValuesBucket()
{
    return {};
}

static inline vector<DataShareValuesBucket> FuzzVectorDataShareValuesBucket()
{
    return {FuzzDataShareValuesBucket()};
}

static inline DatashareBusinessError FuzzDataShareBusinessError()
{
    DatashareBusinessError error;
    error.SetCode(provider->ConsumeIntegral<int32_t>());
    error.SetMessage(provider->ConsumeBytesAsString(NUM_BYTES));
    return error;
}

static inline string FuzzOpenMode()
{
    uint8_t data = provider->ConsumeIntegralInRange<int32_t>(0,
        Media::MEDIA_OPEN_MODES.size() - DEFAULT_MEDIA_OPEN_MODES);
    auto it = Media::MEDIA_OPEN_MODES.begin();
    std::advance(it, data);
    return *it;
}

static inline void GetFileTypesFuzzer(MediaDataShareExtAbility &extension)
{
    extension.GetFileTypes(FuzzUri(), provider->ConsumeBytesAsString(NUM_BYTES));
}

static inline void OpenFileFuzzer(MediaDataShareExtAbility &extension)
{
    extension.OpenFile(FuzzUri(), FuzzOpenMode());
}

static inline void OpenRawFileFuzzer(MediaDataShareExtAbility &extension)
{
    extension.OpenRawFile(FuzzUri(), FuzzOpenMode());
}

static inline void InsertFuzzer(MediaDataShareExtAbility &extension)
{
    extension.Insert(FuzzUri(), FuzzDataShareValuesBucket());
}

static inline void InsertExtFuzzer(MediaDataShareExtAbility &extension)
{
    string uri;
    extension.InsertExt(FuzzUri(), FuzzDataShareValuesBucket(), uri);
}

static inline void UpdateFuzzer(MediaDataShareExtAbility &extension)
{
    extension.Update(FuzzUri(), FuzzDataSharePredicates(), FuzzDataShareValuesBucket());
}

static inline void DeleteFuzzer(MediaDataShareExtAbility &extension)
{
    extension.Delete(FuzzUri(), FuzzDataSharePredicates());
}

static inline void QueryFuzzer(MediaDataShareExtAbility &extension)
{
    auto columns = FuzzVectorString();
    auto error = FuzzDataShareBusinessError();
    extension.Query(FuzzUri(), FuzzDataSharePredicates(), columns, error);
}

static inline void GetTypeFuzzer(MediaDataShareExtAbility &extension)
{
    extension.GetType(FuzzUri());
}

static inline void BatchInsertFuzzer(MediaDataShareExtAbility &extension)
{
    extension.BatchInsert(FuzzUri(), FuzzVectorDataShareValuesBucket());
}

static inline void RegisterObserverFuzzer(MediaDataShareExtAbility &extension)
{
    sptr<AAFwk::IDataAbilityObserver> dataObserver;
    extension.RegisterObserver(FuzzUri(), dataObserver);
}

static inline void UnregisterObserverFuzzer(MediaDataShareExtAbility &extension)
{
    sptr<AAFwk::IDataAbilityObserver> dataObserver;
    extension.UnregisterObserver(FuzzUri(), dataObserver);
}

static inline void NotifyChangeFuzzer(MediaDataShareExtAbility &extension)
{
    extension.NotifyChange(FuzzUri());
}

static inline void NormalizeUriFuzzer(MediaDataShareExtAbility &extension)
{
    extension.NormalizeUri(FuzzUri());
}

static inline void DenormalizeUriFuzzer(MediaDataShareExtAbility &extension)
{
    extension.DenormalizeUri(FuzzUri());
}

static int InitExtention(MediaDataShareExtAbility &extension)
{
    extension.InitPermissionHandler();
    auto stageContext = std::make_shared<AbilityRuntime::ContextImpl>();
    auto abilityContextImpl = std::make_shared<OHOS::AbilityRuntime::AbilityContextImpl>();
    abilityContextImpl->SetStageContext(stageContext);
    int32_t sceneCode = 0;
    return Media::MediaLibraryDataManager::GetInstance()->InitMediaLibraryMgr(abilityContextImpl, abilityContextImpl,
        sceneCode);
}

static inline MediaDataShareExtAbility Init()
{
    const std::unique_ptr<AbilityRuntime::Runtime> runtime;
    return {(*runtime)};
}

static inline void ClearKvStore()
{
    Media::MediaLibraryKvStoreManager::GetInstance().CloseAllKvStore();
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    FuzzedDataProvider fdp(data, size);
    OHOS::provider = &fdp;
    if (data == nullptr) {
        return 0;
    }
    auto extension = OHOS::Init();
    OHOS::InitExtention(extension);
    OHOS::GetFileTypesFuzzer(extension);
    OHOS::OpenFileFuzzer(extension);
    OHOS::OpenRawFileFuzzer(extension);
    OHOS::InsertFuzzer(extension);
    OHOS::InsertExtFuzzer(extension);
    OHOS::UpdateFuzzer(extension);
    OHOS::DeleteFuzzer(extension);
    OHOS::QueryFuzzer(extension);
    OHOS::GetTypeFuzzer(extension);
    OHOS::BatchInsertFuzzer(extension);
    OHOS::RegisterObserverFuzzer(extension);
    OHOS::UnregisterObserverFuzzer(extension);
    OHOS::NotifyChangeFuzzer(extension);
    OHOS::NormalizeUriFuzzer(extension);
    OHOS::DenormalizeUriFuzzer(extension);
    int sleepTime = 100;
    std::this_thread::sleep_for(std::chrono::microseconds(sleepTime));
    OHOS::ClearKvStore();
    return 0;
}