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

#ifndef FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_GENERATION_POST_PROCESS_H
#define FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_GENERATION_POST_PROCESS_H

#include "thumbnail_data.h"
#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

class ThumbnailGenerationPostProcess {
public:
    ThumbnailGenerationPostProcess() = delete;
    virtual ~ThumbnailGenerationPostProcess() = delete;
    EXPORT static int32_t PostProcess(ThumbnailData& data, const ThumbRdbOpt& opts);

private:
    EXPORT static int32_t UpdateCachedRdbValue(ThumbnailData& data, const ThumbRdbOpt& opts);
    EXPORT static int32_t UpdateCachedRdbValueAndNotify(ThumbnailData& data, const ThumbRdbOpt& opts);
    EXPORT static bool HasGeneratedThumb(const ThumbnailData& data); // do not check lcd
    EXPORT static int32_t GetNotifyType(const ThumbnailData& data, const ThumbRdbOpt& opts, NotifyType& notifyType);
    EXPORT static int32_t Notify(const ThumbnailData& data, const NotifyType notifyType);
};
} // namespace Media
} // namespace OHOS

#endif //FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_GENERATION_POST_PROCESS_H