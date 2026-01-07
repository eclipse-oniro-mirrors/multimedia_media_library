/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIALIBRARY_VISION_OPERATIOINS_H
#define OHOS_MEDIALIBRARY_VISION_OPERATIOINS_H

#include <string>
#include <unordered_map>

#include "abs_shared_result_set.h"
#include "medialibrary_command.h"
#include "medialibrary_async_worker.h"
#include "datashare_predicates.h"
#include "foreground_analysis_meta.h"

namespace OHOS {
namespace Media {
class UpdateVisionAsyncTaskData : public AsyncTaskData {
public:
    UpdateVisionAsyncTaskData(int32_t fileId) : fileId_(fileId) {};
    virtual ~UpdateVisionAsyncTaskData() override = default;
    int32_t fileId_;
};

class MediaLibraryVisionOperations {
public:
    static int32_t InsertOperation(MediaLibraryCommand &cmd);
    static int32_t UpdateOperation(MediaLibraryCommand &cmd);
    static int32_t DeleteOperation(MediaLibraryCommand &cmd);
    EXPORT static int32_t EditCommitOperation(MediaLibraryCommand &cmd);
    static std::shared_ptr<NativeRdb::ResultSet> QueryOperation(MediaLibraryCommand &cmd,
        const std::vector<std::string> &columns);
    static std::shared_ptr<NativeRdb::ResultSet> HandleForegroundAnalysisOperation(MediaLibraryCommand &cmd);
    static int32_t PrepareDelayFgAnalysis(const std::string &assetUri, MediaType mediaType);

private:
    static int32_t InitForegroundAnalysisMeta(MediaLibraryCommand &cmd,
        std::shared_ptr<ForegroundAnalysisMeta> &infoPtr);
};
} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIALIBRARY_VISION_OPERATIOINS_H