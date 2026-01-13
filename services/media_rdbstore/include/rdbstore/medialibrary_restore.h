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
#ifndef OHOS_MEDIALIBRARY_RESTORE_H
#define OHOS_MEDIALIBRARY_RESTORE_H

#include <mutex>
#include <string>
#include <memory>
#include "result_set.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class MediaLibraryRestore {
public:
    MediaLibraryRestore() = default;
    virtual ~MediaLibraryRestore() = default;
    EXPORT static MediaLibraryRestore &GetInstance();

    EXPORT void CheckRestore(const int32_t &errCode);
    EXPORT bool IsRestoring() const;
    EXPORT bool IsBackuping() const;
    EXPORT bool IsRealBackuping() const;
    EXPORT void CheckBackup();
    EXPORT void InterruptBackup();
    EXPORT void CheckResultSet(const std::shared_ptr<NativeRdb::ResultSet> &resultSet);
    std::atomic<bool> isBackuping_{false};
    std::mutex mutex_;
    std::condition_variable cv_;
    bool isRestoring_{false};
    bool isDoingBackup_{false};
private:
#ifdef CLOUD_SYNC_MANAGER
    void StopCloudSync();
#endif
    void DoRdbBackup();
    void ResetHAModeSwitchStatus();
    void SaveHAModeSwitchStatusToPara(const int64_t &status);
};
} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIALIBRARY_RESTORE_H
