/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef IVISION_SERVICE_CV
#define IVISION_SERVICE_CV

#include "iremote_broker.h"

namespace OHOS {
namespace Media {
class IMediaAnalysisService : public IRemoteBroker {
public:
    enum ActivateServiceType {
        START_SERVICE_OCR = 1,
        START_DELETE_INDEX = 31,
        START_UPDATE_INDEX = 32,
        START_BACKGROUND_TASK = 33,
        PORTRAIT_COVER_SELECTION = 34,
        HIGHLIGHT_COVER_GENERATE = 35,
        PARSE_GEO_INFO = 36,
        PARSE_GEO_INFO_LIST = 37,
        START_FOREGROUND_OCR = 38,
        START_FOREGROUND_INDEX = 39,
        PORTRAIT_DISPLAY_CHANGE = 40,
        START_FOREGROUND_INDEX_FULL = 43,
        ALBUM_UPDATE_INDEX_TASK = 44,
        UPDATE_MEDIA_INDEX_FOR_CLOUD = 45,
    };
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"Multimedia.MediaAnalyseService.API");
};
} //namespace MEDIA
}
#endif