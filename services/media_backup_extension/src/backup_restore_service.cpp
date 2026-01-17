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
#include "ffrt_inner.h"
#include "media_log.h"
#include "cloud_backup_restore.h"
#include "clone_restore.h"
#include "upgrade_restore.h"
#include "others_clone_restore.h"
#include "rdb_sql_statistic.h"
#include "rdb_types.h"
#include "parameters.h"

namespace OHOS {
namespace Media {
class MediaLibraryBackupObserver : public DistributedRdb::SqlObserver {
public:
    virtual ~MediaLibraryBackupObserver() {}

    void OnStatistic(const SqlExecutionInfo &info) override
    {
        for (auto sql : info.sql_) {
            MEDIA_DEBUG_LOG("DEBUG_MediaLibraryBackup totaltime: %{public}lld waitTime: %{public}lld "
                "prepareTime: %{public}lld executeTime: %{public}lld sql: %{public}s",
                (long long) info.totalTime_, (long long) info.waitTime_, (long long) info.prepareTime_,
                (long long) info.executeTime_, sql.c_str());
        }
    };
};

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
    switch (info.sceneCode) {
        case UPGRADE_RESTORE_ID:
            restoreService_ = std::make_unique<UpgradeRestore>(info.galleryAppName, info.mediaAppName, info.sceneCode,
                GetDualDirName());
            serviceBackupDir_ = RESTORE_SANDBOX_DIR;
            break;
        case DUAL_FRAME_CLONE_RESTORE_ID:
            restoreService_ = std::make_unique<UpgradeRestore>(info.galleryAppName, info.mediaAppName, info.sceneCode);
            break;
        case CLOUD_BACKUP_RESTORE_ID:
            restoreService_ = std::make_unique<CloudBackupRestore>(info.galleryAppName, info.mediaAppName,
                info.sceneCode);
            break;
        case I_PHONE_CLONE_RESTORE:
            restoreService_ = std::make_unique<OthersCloneRestore>(info.sceneCode, info.mediaAppName, info.bundleInfo);
            break;
        case OTHERS_PHONE_CLONE_RESTORE:
        case LITE_PHONE_CLONE_RESTORE:
            restoreService_ = std::make_unique<OthersCloneRestore>(info.sceneCode, info.mediaAppName);
            break;
        default:
            restoreService_ = std::make_unique<CloneRestore>();
    }
    restoreService_->restoreInfo_ = info.bundleInfo;
}

void BackupRestoreService::StartRestore(const std::shared_ptr<AbilityRuntime::Context> &context,
    const RestoreInfo &info)
{
    MEDIA_INFO_LOG("Start restore service: %{public}d", info.sceneCode);
    Init(info);
    CHECK_AND_RETURN_LOG(restoreService_ != nullptr, "Create media restore service failed.");
    CHECK_AND_EXECUTE(context == nullptr, BackupFileUtils::CreateDataShareHelper(context->GetToken()));
    restoreService_->SetIsRestore(true);
    restoreService_->StartRestore(serviceBackupDir_, UPGRADE_FILE_DIR);
}

void BackupRestoreService::StartRestoreEx(const std::shared_ptr<AbilityRuntime::Context> &context,
    const RestoreInfo &info, std::string &restoreExInfo)
{
    shared_ptr<MediaLibraryBackupObserver> sqlPrintObserver = std::make_shared<MediaLibraryBackupObserver>();
    DistributedRdb::SqlStatistic::Subscribe(sqlPrintObserver);
    MEDIA_INFO_LOG("Start restoreEx service: %{public}d", info.sceneCode);
    Init(info);
    if (restoreService_ == nullptr) {
        MEDIA_ERR_LOG("Create media restore service failed.");
        restoreExInfo = "";
        return;
    }
    CHECK_AND_EXECUTE(context == nullptr, BackupFileUtils::CreateDataShareHelper(context->GetToken()));
    restoreService_->SetIsRestore(true);
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
    CHECK_AND_RETURN_LOG(restoreService_ != nullptr, "Create media backup service failed.");
    restoreService_->StartBackup();
}

void BackupRestoreService::StartBackupEx(int32_t sceneCode, const std::string &galleryAppName,
    const std::string &mediaAppName, const std::string& backupInfo, std::string& backupExInfo)
{
    MEDIA_INFO_LOG("Start backupEx service: %{public}d", sceneCode);
    if (sceneCode != CLONE_RESTORE_ID) {
        MEDIA_ERR_LOG("StartBackupEx current scene is not supported");
        backupExInfo = "";
        return;
    }
    Init({CLONE_RESTORE_ID, galleryAppName, mediaAppName, "", backupInfo});
    if (restoreService_ == nullptr) {
        MEDIA_ERR_LOG("Create media backup service failed.");
        backupExInfo = "";
        return;
    }
    restoreService_->StartBackupEx(backupExInfo);
}

void BackupRestoreService::Release(const std::shared_ptr<AbilityRuntime::Context> &context,
    int32_t sceneCode, int32_t releaseSceneInt)
{
    MEDIA_INFO_LOG("Start Release service, releaseScene:%{public}d", releaseSceneInt);
    CHECK_AND_RETURN_LOG(INT_RELEASE_SCENE_MAP.count(releaseSceneInt),
        "invalid releaseScene: %{public}d", releaseSceneInt);
    ReleaseScene releaseScene = INT_RELEASE_SCENE_MAP.at(releaseSceneInt);
    if (releaseScene == ReleaseScene::RESTORE) {
        std::string CLONE_FLAG = "multimedia.medialibrary.cloneFlag";
        std::string RESTORE_FLAG = "multimedia.medialibrary.restoreFlag";
        MEDIA_INFO_LOG("set Parameter to 0");
        bool retFlag = system::SetParameter(CLONE_FLAG, "0");
        CHECK_AND_PRINT_LOG(retFlag, "Failed to set stop parameter cloneFlag, retFlag:%{public}d", retFlag);
        retFlag = system::SetParameter(RESTORE_FLAG, "0");
        CHECK_AND_PRINT_LOG(retFlag, "Failed to set stop parameter restoreFlag, retFlag:%{public}d", retFlag);
        return;
    }
    CHECK_AND_RETURN_LOG(sceneCode == CLONE_RESTORE_ID && releaseScene == ReleaseScene::BACKUP,
        "current release scene is not supported, sceneCode: %{public}d releasescene: %{public}d",
        sceneCode, releaseSceneInt);
    Init({CLONE_RESTORE_ID, "", "", "", ""});
    CHECK_AND_RETURN_LOG(restoreService_ != nullptr, "Create media backup service failed.");
    CHECK_AND_EXECUTE(context == nullptr, BackupFileUtils::CreateDataShareHelper(context->GetToken()));
    restoreService_->Release(releaseScene);
}
} // namespace Media
} // namespace OHOS
