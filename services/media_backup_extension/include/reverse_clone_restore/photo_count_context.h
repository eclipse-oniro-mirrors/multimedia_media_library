/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_PHOTO_COUNT_CONTEXT_H
#define OHOS_MEDIA_PHOTO_COUNT_CONTEXT_H

#include <memory>
#include <string>
#include "rdb_store.h"
#include "reverse_clone_restore.h"
#include "photo_count_strategy.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class PhotoCountContext {
public:
    EXPORT PhotoCountContext(std::shared_ptr<NativeRdb::RdbStore> mediaRdb,
        std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
        bool isCloudRestoreSatisfied, bool shouldAbsorbCloudFromSourceRdb,
        int32_t sceneCode, const std::string& taskId);
    ~PhotoCountContext() = default;

    EXPORT bool NeedReverseRestore(ReverseRestoreReportInfo& info);

private:
    std::shared_ptr<NativeRdb::RdbStore> mediaRdb_;
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb_;
    bool isCloudRestoreSatisfied_;
    bool shouldAbsorbCloudFromSourceRdb_;
    int32_t sceneCode_;
    std::string taskId_;
    std::unique_ptr<PhotoCountStrategy> countStrategy_;
};

} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIA_PHOTO_COUNT_CONTEXT_H