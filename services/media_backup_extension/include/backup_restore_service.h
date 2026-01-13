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

#ifndef OHOS_MEDIA_BACKUP_RESTORE_SERVICE_H
#define OHOS_MEDIA_BACKUP_RESTORE_SERVICE_H

#include <string>
#include <context.h>

#include "base_restore.h"

namespace OHOS {
namespace Media {

using RestoreInfo = struct {
    int32_t sceneCode;
    std::string galleryAppName;
    std::string mediaAppName;
    std::string backupDir;
    std::string bundleInfo;
};

class BackupRestoreService {
public:
    virtual ~BackupRestoreService() = default;
    static BackupRestoreService &GetInstance(void);
    void Init(const RestoreInfo &info);
    void StartRestore(const std::shared_ptr<AbilityRuntime::Context> &context, const RestoreInfo &info);
    void StartRestoreEx(const std::shared_ptr<AbilityRuntime::Context> &context, const RestoreInfo &info,
        std::string &restoreExInfo);

    void GetBackupInfo(int32_t sceneCode, std::string &backupInfo);
    void GetProgressInfo(std::string &progressInfo);
    void StartBackup(int32_t sceneCode, const std::string &galleryAppName = "", const std::string &mediaAppName = "");
    void StartBackupEx(int32_t sceneCode, const std::string &galleryAppName,
        const std::string &mediaAppName, const std::string& backupInfo, std::string& backupExInfo);
    void Release(const std::shared_ptr<AbilityRuntime::Context> &context,
        int32_t sceneCode, int32_t releaseScene);

private:
    BackupRestoreService() = default;
    std::string serviceBackupDir_;
    std::unique_ptr<BaseRestore> restoreService_;
};
} // namespace Media
} // namespace OHOS

#endif  // OHOS_MEDIA_BACKUP_RESTORE_SERVICE_H
