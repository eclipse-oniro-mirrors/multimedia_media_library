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

#ifndef OHOS_MEDIA_REVERSE_CLONE_KVSTORE_EXECUTOR_H
#define OHOS_MEDIA_REVERSE_CLONE_KVSTORE_EXECUTOR_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace OHOS::Media {
class MediaLibraryKvStore;
struct ReverseCloneKvStoreExecuteStats;

enum class ReverseCloneKvStoreWriteMode {
    FILL_IF_MISSING = 0,
    OVERWRITE,
};

struct ReverseCloneKvStoreTask {
    int32_t oldFileId {0};
    std::string oldDateKey;
    int32_t newFileId {0};
    std::string newDateKey;
    ReverseCloneKvStoreWriteMode writeMode {ReverseCloneKvStoreWriteMode::OVERWRITE};

    bool IsValid() const;
};

class ReverseCloneKvStoreExecutor {
public:
    ReverseCloneKvStoreExecutor() = default;
    ~ReverseCloneKvStoreExecutor();

    bool Init(const std::string &backupRoot);
    void Close();
    int32_t Execute(const std::vector<ReverseCloneKvStoreTask> &tasks);

private:
    int32_t ExecuteOne(const ReverseCloneKvStoreTask &task, ReverseCloneKvStoreExecuteStats &stats) const;
    bool IsReady() const;

    std::shared_ptr<MediaLibraryKvStore> oldMonthKvStorePtr_;
    std::shared_ptr<MediaLibraryKvStore> oldYearKvStorePtr_;
    std::shared_ptr<MediaLibraryKvStore> newMonthKvStorePtr_;
    std::shared_ptr<MediaLibraryKvStore> newYearKvStorePtr_;
};
} // namespace OHOS::Media

#endif // OHOS_MEDIA_REVERSE_CLONE_KVSTORE_EXECUTOR_H
