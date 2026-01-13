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
 
#ifndef INNER_API_ANALYSIS_DATA_MANAGER_H
#define INNER_API_ANALYSIS_DATA_MANAGER_H

#include "datashare_helper.h"
#include "analysis_data_visibility.h"
#include "iremote_object.h"
//LCOV_EXCL_START
namespace OHOS {
namespace Media::AnalysisData {
class AnalysisDataManager {
public:
    /**
     * @brief GetInstance.
     * A function used to create an AnalysisDataManager instance.
     */
    static API_EXPORT AnalysisDataManager &GetInstance();

    AnalysisDataManager(const AnalysisDataManager&) = delete;
    AnalysisDataManager& operator=(const AnalysisDataManager&) = delete;

private:
    AnalysisDataManager();
    sptr<IRemoteObject> InitToken();
    static std::shared_ptr<DataShare::DataShareHelper> sDataShareHelper_;
    static sptr<IRemoteObject> token_;
};
} // namespace Media::AnalysisData
} // namespace OHOS
#endif
//LCOV_EXCL_STOP