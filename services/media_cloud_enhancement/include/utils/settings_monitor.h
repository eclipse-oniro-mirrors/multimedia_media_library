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

#ifndef FRAMEWORKS_SERVICES_MEDIA_CLOUD_ENHANCEMENT_INCLUDE_SETTINGS_MONITOR_H
#define FRAMEWORKS_SERVICES_MEDIA_CLOUD_ENHANCEMENT_INCLUDE_SETTINGS_MONITOR_H

#include "datashare_helper.h"
#include "datashare_predicates.h"
#include "datashare_result_set.h"
#include "datashare_values_bucket.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "uri.h"
#include "data_ability_observer_stub.h"

namespace OHOS {
namespace Media {
class SettingsMonitor {
public:
    static void RegisterSettingsObserver(const Uri &uri, const sptr<AAFwk::IDataAbilityObserver> &dataObserver);
    static void UnregisterSettingsObserver(const Uri &uri, const sptr<AAFwk::IDataAbilityObserver> &dataObserver);
    static std::string QueryPhotosAutoOption();
    static bool QueryPhotosWaterMark();
    static std::shared_ptr<DataShare::DataShareHelper> CreateNonBlockDataShareHelper();
    static std::shared_ptr<DataShare::DataShareHelper> CreateDataShareHelper();
    static int32_t Insert(Uri uri, const std::string &key, const std::string &value);
    static int32_t Query(Uri uri, const std::string &key, std::string &value);
};

class PhotosAutoOptionObserver : public AAFwk::DataAbilityObserverStub {
public:
    void OnChange() override;
};

class PhotosWaterMarkObserver : public AAFwk::DataAbilityObserverStub {
public:
    void OnChange() override;
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_CLOUD_ENHANCEMENT_INCLUDE_SETTINGS_MONITOR_H
