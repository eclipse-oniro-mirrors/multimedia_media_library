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

#define MLOG_TAG "MediaLibraryRestoreService"

#include <context.h>
#include <context_impl.h>

#include "backup_file_utils.h"
#include "backup_restore_service.h"
#include "media_log.h"
#include "clone_restore.h"
#include "upgrade_restore.h"
#include "others_clone_restore.h"

namespace OHOS {
namespace Media {
const int DUAL_FIRST_NUMBER = 65;
const int DUAL_SECOND_NUMBER = 110;
const int DUAL_THIRD_NUMBER = 100;
const int DUAL_FOURTH_NUMBER = 114;
const int DUAL_FIFTH_NUMBER = 111;
const int DUAL_SIXTH_NUMBER = 105;
const int DUAL_SEVENTH_NUMBER = 100;
BackupRestoreService &BackupRestoreService::GetInstance(void)
{
    static BackupRestoreService inst;
    return inst;
}

std::string GetDualDirName()
{
    int arr[] = { DUAL_FIRST_NUMBER, DUAL_SECOND_NUMBER, DUAL_THIRD_NUMBER, DUAL_FOURTH_NUMBER, DUAL_FIFTH_NUMBER,
        DUAL_SIXTH_NUMBER, DUAL_SEVENTH_NUMBER };
    int len = sizeof(arr) / sizeof(arr[0]);
    std::string dualDirName = "";
    for (int i = 0; i < len; i++) {
        dualDirName += char(arr[i]);
    }
    return dualDirName;
}

void BackupRestoreService::Init(const RestoreInfo &info)
{
    if (restoreService_ != nullptr) {
        return;
    }
    serviceBackupDir_ = info.backupDir;
    if (info.sceneCode == UPGRADE_RESTORE_ID) {
        restoreService_ = std::make_unique<UpgradeRestore>(info.galleryAppName, info.mediaAppName, UPGRADE_RESTORE_ID,
            GetDualDirName());
        serviceBackupDir_ = RESTORE_SANDBOX_DIR;
    } else if (info.sceneCode == DUAL_FRAME_CLONE_RESTORE_ID) {
        restoreService_ = std::make_unique<UpgradeRestore>(info.galleryAppName, info.mediaAppName,
            DUAL_FRAME_CLONE_RESTORE_ID);
    } else if (info.sceneCode == I_PHONE_CLONE_RESTORE) {
        restoreService_ = std::make_unique<OthersCloneRestore>(I_PHONE_CLONE_RESTORE, info.mediaAppName,
            info.bundleInfo);
    } else if (info.sceneCode == OTHERS_PHONE_CLONE_RESTORE) {
        restoreService_ = std::make_unique<OthersCloneRestore>(OTHERS_PHONE_CLONE_RESTORE, info.mediaAppName);
    } else {
        restoreService_ = std::make_unique<CloneRestore>();
    }
}

void BackupRestoreService::StartRestore(const std::shared_ptr<AbilityRuntime::Context> &context,
    const RestoreInfo &info)
{
    MEDIA_INFO_LOG("Start restore service: %{public}d", info.sceneCode);
    Init(info);
    if (restoreService_ == nullptr) {
        MEDIA_ERR_LOG("Create media restore service failed.");
        return;
    }
    if (context != nullptr) {
        BackupFileUtils::CreateDataShareHelper(context->GetToken());
    }
    restoreService_->StartRestore(serviceBackupDir_, UPGRADE_FILE_DIR);
}

void BackupRestoreService::StartRestoreEx(const std::shared_ptr<AbilityRuntime::Context> &context,
    const RestoreInfo &info, std::string &restoreExInfo)
{
    MEDIA_INFO_LOG("Start restoreEx service: %{public}d", info.sceneCode);
    Init(info);
    if (restoreService_ == nullptr) {
        MEDIA_ERR_LOG("Create media restore service failed.");
        restoreExInfo = "";
        return;
    }
    if (context != nullptr) {
        BackupFileUtils::CreateDataShareHelper(context->GetToken());
    }
    restoreService_->StartRestoreEx(serviceBackupDir_, UPGRADE_FILE_DIR, restoreExInfo);
}

void BackupRestoreService::GetBackupInfo(int32_t sceneCode, std::string &backupInfo)
{
    MEDIA_INFO_LOG("Start restore service: %{public}d", sceneCode);
    if (sceneCode != CLONE_RESTORE_ID) {
        MEDIA_ERR_LOG("StartRestoreEx current scene is not supported");
        backupInfo = "";
        return;
    }
    Init({CLONE_RESTORE_ID, "", "", "", ""});
    if (restoreService_ == nullptr) {
        MEDIA_ERR_LOG("Create media restore service failed.");
        backupInfo = "";
        return;
    }
    backupInfo = restoreService_->GetBackupInfo();
}

void BackupRestoreService::GetProgressInfo(std::string &progressInfo)
{
    MEDIA_INFO_LOG("Start get progressInfo");
    if (restoreService_ == nullptr) {
        MEDIA_WARN_LOG("Media restore service not created.");
        progressInfo = "";
        return;
    }
    progressInfo = restoreService_->GetProgressInfo();
}

void BackupRestoreService::StartBackup(int32_t sceneCode, const std::string &galleryAppName,
    const std::string &mediaAppName)
{
    MEDIA_INFO_LOG("Start backup service: %{public}d", sceneCode);
    if (sceneCode != CLONE_RESTORE_ID) {
        MEDIA_ERR_LOG("StartBackup current scene is not supported");
        return;
    }
    Init({CLONE_RESTORE_ID, galleryAppName, mediaAppName, "", ""});
    if (restoreService_ == nullptr) {
        MEDIA_ERR_LOG("Create media backup service failed.");
        return;
    }
    restoreService_->StartBackup();
}
} // namespace Media
} // namespace OHOS
