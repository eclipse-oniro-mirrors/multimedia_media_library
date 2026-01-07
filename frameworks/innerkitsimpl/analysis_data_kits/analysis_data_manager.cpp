/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License")
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

#include "analysis_data_manager.h"

#include "iservice_registry.h"
#include "media_log.h"
#include "user_inner_ipc_client.h"

using namespace std;

namespace OHOS {
namespace Media::AnalysisData {
sptr<IRemoteObject> AnalysisDataManager::token_ = nullptr;
std::shared_ptr<DataShare::DataShareHelper> AnalysisDataManager::sDataShareHelper_ = nullptr;
constexpr int STORAGE_MANAGER_MANAGER_ID = 5003;
const std::string MEDIALIBRARY_DATA_URI = "datashare:///media";

AnalysisDataManager &AnalysisDataManager::GetInstance()
{
    static AnalysisDataManager analysisMgr;
    return analysisMgr;
}

AnalysisDataManager::AnalysisDataManager()
{
    token_ = InitToken();
    if (sDataShareHelper_ == nullptr && token_ != nullptr) {
        sDataShareHelper_ = DataShare::DataShareHelper::Creator(token_, MEDIALIBRARY_DATA_URI);
    }
}
 
sptr<IRemoteObject> AnalysisDataManager::InitToken()
{
    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    CHECK_AND_RETURN_RET_LOG(saManager != nullptr, nullptr, "get system ability mgr failed.");
    auto remoteObj = saManager->GetSystemAbility(STORAGE_MANAGER_MANAGER_ID);
    CHECK_AND_RETURN_RET_LOG(remoteObj != nullptr, nullptr, "GetSystemAbility Service failed.");
    return remoteObj;
}
}
}