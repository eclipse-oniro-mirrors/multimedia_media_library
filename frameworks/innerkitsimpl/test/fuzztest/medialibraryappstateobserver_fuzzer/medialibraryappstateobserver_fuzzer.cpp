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

#include "medialibraryappstateobserver_fuzzer.h"

#include <cstddef>
#include <sstream>
#include <string>
#include <fuzzer/FuzzedDataProvider.h>

#include "medialibrary_appstate_observer.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "media_app_uri_permission_column.h"
#include "media_app_uri_sensitive_column.h"
#include "media_log.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_errno.h"
#include "system_ability_definition.h"
#include "ability_info.h"
#include "app_state_data.h"

namespace OHOS {
using namespace std;
using namespace DataShare;
using namespace OHOS::AppExecFwk;
using OHOS::AppExecFwk::AppStateData;
static const int32_t NUM_BYTES = 1;
static const int32_t MAX_ABILITY_TYPE = 30;
FuzzedDataProvider *FDP = nullptr;

static inline std::vector<int32_t> FuzzVector()
{
    return { FDP->ConsumeIntegral<int32_t>() };
}

static inline AppExecFwk::ExtensionAbilityType FuzzExtensionAbilityType()
{
    int32_t value = FDP->ConsumeIntegralInRange<int32_t>(0, MAX_ABILITY_TYPE);
    return static_cast<AppExecFwk::ExtensionAbilityType>(value);
}

static AppExecFwk::AppStateData FuzzAppStateData()
{
    AppExecFwk::AppStateData appStateData;
    appStateData.isFocused = FDP->ConsumeBool();
    appStateData.isSplitScreenMode = FDP->ConsumeBool();
    appStateData.isFloatingWindowMode = FDP->ConsumeBool();
    appStateData.isSpecifyTokenId = FDP->ConsumeBool();
    appStateData.isPreloadModule = FDP->ConsumeBool();
    appStateData.pid = FDP->ConsumeIntegral<int32_t>();
    appStateData.uid = FDP->ConsumeIntegral<int32_t>();
    appStateData.state = FDP->ConsumeIntegral<int32_t>();
    appStateData.appIndex = FDP->ConsumeIntegral<int32_t>();
    appStateData.accessTokenId = FDP->ConsumeIntegral<uint32_t>();
    appStateData.extensionType =  FuzzExtensionAbilityType();
    appStateData.renderPids = FuzzVector();
    appStateData.bundleName = FDP->ConsumeBytesAsString(NUM_BYTES);
    appStateData.callerBundleName = FDP->ConsumeBytesAsString(NUM_BYTES);
    return appStateData;
}

static void AppStateObserver()
{
    AppExecFwk::AppStateData appStateData = FuzzAppStateData();
    sptr<Media::MedialibraryAppStateObserver> appStateObserver =
        new (std::nothrow) Media::MedialibraryAppStateObserver();
    appStateObserver->OnAppStopped(appStateData);
}

} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    OHOS::FDP = &fdp;
    if (data == nullptr) {
        return 0;
    }
    OHOS::AppStateObserver();
    return 0;
}
