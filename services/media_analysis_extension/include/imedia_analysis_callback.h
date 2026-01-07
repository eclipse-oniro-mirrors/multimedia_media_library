/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OHOS_MEDIALIBRARY_IMEDIA_ANALYSIS_CALLBACK_H
#define OHOS_MEDIALIBRARY_IMEDIA_ANALYSIS_CALLBACK_H

#include <string>

#include "iremote_broker.h"

namespace OHOS {
namespace Media {
/**
 * @brief MediaAnalysis service callback remote request code for IPC.
 *
 */
enum MediaAnalysisCallbackInterfaceCode {
    PORTRAIT_COVER_SELECTION_COMPLETED_CALLBACK = 0
};
class IMediaAnalysisCallback : public IRemoteBroker {
public:
    virtual int32_t PortraitCoverSelectionCompleted(const std::string albumId) = 0;

    DECLARE_INTERFACE_DESCRIPTOR(u"IMediaAnalysisCallback");
};
} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIALIBRARY_IMEDIA_ANALYSIS_CALLBACK_H