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
#define MLOG_TAG "MediaLibraryRestore"

#include "acl.h"
#include "medialibrary_restore.h"
#include "dfx_utils.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_tracer.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "parameter.h"
#include "parameters.h"
#include "post_event_utils.h"
#ifdef CLOUD_SYNC_MANAGER
#include "cloud_sync_manager.h"
#endif

namespace OHOS {
namespace Media {
namespace {
    const std::string SWITCH_STATUS_KEY = "persist.multimedia.medialibrary.rdb_switch_status";
    const std::string CLOUD_STOP_FLAG_K = "persist.kernel.medialibrarydata.stopflag";
    const std::string CLOUD_STOP_FLAG_V = "1";
    constexpr int PARAMETER_E_OK        = 0;
    constexpr int WAIT_SECONDS          = 120;
    constexpr int SLEEP_SECONDS         = 60;
    constexpr int MAX_RETRY_TIMES       = 2;
    enum HA_SWITCH_STATUS : uint32_t {
        HA_SWITCH_READY = 0,
        HA_SWITCHING = 1
    };
} // namespace

MediaLibraryRestore &MediaLibraryRestore::GetInstance()
{
    static MediaLibraryRestore instance;
    return instance;
}

void MediaLibraryRestore::SaveHAModeSwitchStatusToPara(const int64_t &status)
{
    int ret = SetParameter(SWITCH_STATUS_KEY.c_str(), std::to_string(status).c_str());
    CHECK_AND_RETURN_LOG((ret == PARAMETER_E_OK), "MediaLibraryRestore SetParameter switch error");
}

void MediaLibraryRestore::CheckRestore(const int32_t &errCode)
{
    if (errCode != NativeRdb::E_SQLITE_CORRUPT) {
        return;
    }
    MEDIA_INFO_LOG("Restore is called");
    CHECK_AND_RETURN_LOG((!isRestoring_), "RdbStore is restoring");

    std::string date = DfxUtils::GetCurrentDateMillisecond();
    VariantMap map = {{KEY_DB_CORRUPT, std::move(date)}};
    PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_CORRUPT_ERR, map);

    isRestoring_ = true;
    std::thread([] {
        MEDIA_INFO_LOG("MediaLibraryRestore::Restore [start]");
        auto rdb = MediaLibraryDataManager::GetInstance()->rdbStore_;
        if (rdb == nullptr) {
            MediaLibraryRestore::GetInstance().isRestoring_ = false;
            MEDIA_ERR_LOG("Restore rdbStore is nullptr");
            return;
        }

        int retryTimes = MAX_RETRY_TIMES;
        int errCode = NativeRdb::E_OK;
        do {
            MediaLibraryTracer tracer;
            tracer.Start("MediaLibraryRestore::Restore");
            errCode = rdb->Restore("");
            MEDIA_INFO_LOG("MediaLibraryRestore::Restore errCode = %{public}d", errCode);
            if (errCode == NativeRdb::E_SQLITE_BUSY) {
                retryTimes--;
                continue;
            }
            break;
        } while (retryTimes > 0);
        MEDIA_INFO_LOG("MediaLibraryRestore::Restore [end]. errCode = %{public}d", errCode);
        MediaLibraryRestore::GetInstance().isRestoring_ = false;
    }).detach();
}

#ifdef CLOUD_SYNC_MANAGER
void MediaLibraryRestore::StopCloudSync()
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryRestore::StopCloudSync");
    FileManagement::CloudSync::CloudSyncManager::GetInstance().StopSync(BUNDLE_NAME, true);
    uint32_t times = 0;
    int ret = WaitParameter(CLOUD_STOP_FLAG_K.c_str(), CLOUD_STOP_FLAG_V.c_str(), WAIT_SECONDS);
    if (ret == PARAMETER_E_OK) {
        MEDIA_INFO_LOG("StopCloudSync success end");
        return;
    }
    isBackuping_ = false;
    MEDIA_INFO_LOG("StopCloudSync timeout error, set backup false");
    return;
}
#endif

void MediaLibraryRestore::CheckBackup()
{
    MEDIA_INFO_LOG("CheckBackup is called");
    CHECK_AND_RETURN_LOG((!isRestoring_), "CheckBackup: is restoring, return");

    auto rdb = MediaLibraryDataManager::GetInstance()->rdbStore_;
    CHECK_AND_RETURN_LOG((rdb != nullptr), "CheckBackup: rdbStore is nullptr");
    if (!rdb->IsSlaveDiffFromMaster()) {
        MEDIA_INFO_LOG("CheckBackup: isSlaveDiffFromMaster [false], return");
        return;
    }
    MEDIA_INFO_LOG("CheckBackup: isSlaveDiffFromMaster [true]");
    DoRdbBackup();
}

void MediaLibraryRestore::ResetHAModeSwitchStatus()
{
    auto switchStatus = HA_SWITCH_READY;
    SaveHAModeSwitchStatusToPara(std::move(switchStatus));
#ifdef CLOUD_SYNC_MANAGER
    auto ret = FileManagement::CloudSync::CloudSyncManager::GetInstance().StartSync(BUNDLE_NAME);
    MEDIA_INFO_LOG("ResetHAModeSwitchStatus::StartSync [%{public}d]", ret);
    ret = FileManagement::CloudSync::CloudSyncManager::GetInstance().DownloadThumb();
    MEDIA_INFO_LOG("ResetHAModeSwitchStatus::DownloadThumb [%{public}d]", ret);
#endif
}

void MediaLibraryRestore::DoRdbBackup()
{
    CHECK_AND_RETURN_LOG((!isBackuping_.load()), "DoRdbBackup: is backuping, return");
    isBackuping_ = true;

    std::thread([] {
        MEDIA_INFO_LOG("DoRdbBackup: Backup [start]");
        {
            MEDIA_INFO_LOG("DoRdbBackup: wait_for [start]");
            std::unique_lock<std::mutex> lock(MediaLibraryRestore::GetInstance().mutex_);
            MediaLibraryRestore::GetInstance().cv_.wait_for(lock, std::chrono::seconds(SLEEP_SECONDS),
                [] { return !MediaLibraryRestore::GetInstance().IsBackuping(); });
            MEDIA_INFO_LOG("DoRdbBackup: wait_for [end]");
        }
        CHECK_AND_RETURN_LOG((MediaLibraryRestore::GetInstance().IsBackuping()),
            "DoRdbBackup: after sleep isbackuping fasle, return");
        auto currentTime = MediaFileUtils::UTCTimeSeconds();
        MediaLibraryRestore::GetInstance().SaveHAModeSwitchStatusToPara(currentTime);
#ifdef CLOUD_SYNC_MANAGER
        MediaLibraryRestore::GetInstance().StopCloudSync();
#endif
        auto rdb = MediaLibraryDataManager::GetInstance()->rdbStore_;
        if (rdb == nullptr || !MediaLibraryRestore::GetInstance().IsBackuping()) {
            MediaLibraryRestore::GetInstance().ResetHAModeSwitchStatus();
            MediaLibraryRestore::GetInstance().isBackuping_ = false;
            MEDIA_ERR_LOG("DoRdbBackup: rdb is nullptr or interrupt or isbackuping false");
            return;
        }
        MediaLibraryTracer tracer;
        tracer.Start("MediaLibraryRestore::DoRdbBackup Backup");
        MediaLibraryRestore::GetInstance().isDoingBackup_ = true;
        if (!MediaLibraryRdbUtils::ExecuteDatabaseQuickCheck(rdb)
            || !MediaLibraryRestore::GetInstance().IsBackuping()) {
            MediaLibraryRestore::GetInstance().ResetHAModeSwitchStatus();
            MediaLibraryRestore::GetInstance().isBackuping_ = false;
            MEDIA_ERR_LOG("DoRdbBackup: QuickCheck fail");
            return;
        }
        MEDIA_INFO_LOG("DoRdbBackup: Backup [start]");
        int errCode = rdb->Backup("", false);
        MediaLibraryRestore::GetInstance().isDoingBackup_ = false;
        if (errCode == NativeRdb::E_OK) {
            Acl::AclSetSlaveDatabase();
        }
        MEDIA_INFO_LOG("DoRdbBackup: Backup [end]. errCode = %{public}d", errCode);
        MediaLibraryRestore::GetInstance().ResetHAModeSwitchStatus();
        if (errCode != NativeRdb::E_CANCEL) {
            MediaLibraryRestore::GetInstance().isBackuping_ = false;
        }
    }).detach();
}

void MediaLibraryRestore::InterruptBackup()
{
    MEDIA_INFO_LOG("InterruptBackup [start]");
    if (!isBackuping_.load()) {
        MEDIA_INFO_LOG("rdb is not backuping, no need to interrupt");
        return;
    }
    auto rdb = MediaLibraryDataManager::GetInstance()->rdbStore_;
    CHECK_AND_RETURN_LOG((rdb != nullptr), "[InterruptBackup] rdbStore is nullptr");
    int errCode = rdb->InterruptBackup();
    isBackuping_ = false;
    ResetHAModeSwitchStatus();
    cv_.notify_all();
    MEDIA_INFO_LOG("InterruptBackup [end]. errCode = %{public}d", errCode);
}

void MediaLibraryRestore::CheckResultSet(const std::shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    MediaLibraryTracer tracer;
    tracer.Start("CheckResultSet");
    if (resultSet == nullptr) {
        return;
    }
    int count;
    int errCode = resultSet->GetRowCount(count);
    CheckRestore(errCode);
}

bool MediaLibraryRestore::IsRestoring() const
{
    return isRestoring_;
}

bool MediaLibraryRestore::IsBackuping() const
{
    return isBackuping_.load();
}

bool MediaLibraryRestore::IsRealBackuping() const
{
    return isDoingBackup_;
}
} // namespace Media
} // namespace OHOS
