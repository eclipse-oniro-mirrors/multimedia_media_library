/*
 * Copyright (C) 2023-2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MediaLibraryBaseRestore"

#include "base_restore.h"

#include <cinttypes>
#include "application_context.h"
#include "background_task_mgr_helper.h"
#include "backup_database_utils.h"
#include "backup_dfx_utils.h"
#include "backup_file_utils.h"
#include "backup_log_utils.h"
#include "cloud_sync_manager.h"
#include "directory_ex.h"
#include "media_analysis_helper.h"
#include "media_image_framework_utils.h"
#include "media_scanner_manager.h"
#include "medialibrary_business_code.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_object_utils.h"
#include "moving_photo_file_utils.h"
#include "parameters.h"
#include "resource_type.h"
#include "user_define_ipc_client.h"
#include "medialibrary_notify.h"
#include "upgrade_restore_task_report.h"
#include "database_report.h"
#include "ohos_account_kits.h"
#include "medialibrary_photo_operations.h"
#include "restore_map_code_utils.h"
#include "preferences.h"
#include "preferences_helper.h"

namespace OHOS {
namespace Media {
const std::string DATABASE_PATH = "/data/storage/el2/database/rdb/media_library.db";
const std::string SINGLE_DIR_NAME = "A";
const std::string CLONE_FLAG = "multimedia.medialibrary.cloneFlag";
const std::string RESTORE_FLAG = "multimedia.medialibrary.restoreFlag";
const std::string BACKUP_FLAG = "multimedia.medialibrary.backupFlag";
// 同步服务模块使用: 时间戳|0，时间戳表征任务正在清理中，0表示无清理任务
const std::string CLOUDSYNC_SWITCH_STATUS_KEY = "persist.kernel.cloudsync.switch_status"; // ms
const std::string THM_SAVE_WITHOUT_ROTATE_PATH = "/THM_EX";
const std::string THUMB_EX_SUFFIX = "THM_EX/THM";
const std::string LCD_EX_SUFFIX = "THM_EX/LCD";
const int64_t THUMB_DENTRY_SIZE = 2 * 1024 * 1024;
const int32_t ORIETATION_ZERO = 0;
const int32_t MIGRATE_CLOUD_THM_TYPE = 0;
const int32_t MIGRATE_CLOUD_LCD_TYPE = 1;
const int32_t APP_MAIN_DATA_USER_ID = 0;
const int32_t APP_TWIN_DATA_USER_ID_START = 128;
const int32_t APP_TWIN_DATA_USER_ID_END = 147;
const int32_t SINGLE_LEN_EXTRADATA = 20;
const int32_t MAX_FILE_PATH_LENGTH = 256;
const double DOUBLE_EPSILON = 1e-15;

static constexpr int64_t RESTORE_OR_BACKUP_WAIT_FORCE_RETAIN_CLOUD_MEDIA_TIMEOUT_MILLISECOND = 60 * 60 * 1000;
static constexpr int64_t RESTORE_OR_BACKUP_WAIT_FORCE_RETAIN_CLOUD_MEDIA_SLEEP_TIME_MILLISECOND = 5000;
static const std::string MEDIA_LIBRARY_PREF_XML = "/data/storage/el2/base/preferences/media_library_preferences.xml";
static const std::string MEDIA_LIBRARY_RECOVERY_FLAG_KEY = "media_library_preferences_recovery_flag";

static int32_t GetRestoreModeFromRestoreInfo(const string &restoreInfo)
{
    int32_t restoreMode = RESTORE_MODE_PROC_ALL_DATA;
    nlohmann::json jsonObj = nlohmann::json::parse(restoreInfo, nullptr, false);
    CHECK_AND_RETURN_RET_LOG(!jsonObj.is_discarded(), restoreMode, "parse json failed");

    for (auto &obj : jsonObj) {
        bool cond = (!obj.contains("type") || obj.at("type") != "appTwinDataRestoreState" || !obj.contains("detail"));
        CHECK_AND_CONTINUE(!cond);
        CHECK_AND_CONTINUE(obj.at("detail").is_string());
        std::string curMode = obj.at("detail");
        if (curMode == "0" || curMode == "1" || curMode == "2" || curMode == "3") {
            restoreMode = std::stoi(curMode);
        } else {
            MEDIA_ERR_LOG("invalid restore mode:%{public}s", curMode.c_str());
        }
    }
    return restoreMode;
}

int32_t BaseRestore::GetRestoreMode()
{
    return restoreMode_;
}

void BaseRestore::GetAccountValid()
{
    if (sceneCode_ == UPGRADE_RESTORE_ID) {
        isAccountValid_ = true;
        return;
    }
    string oldId = "";
    string newId = "";
    nlohmann::json json_arr = nlohmann::json::parse(restoreInfo_, nullptr, false);
    CHECK_AND_RETURN_LOG(!json_arr.is_discarded(), "cloud account parse failed.");

    for (const auto& item : json_arr) {
        bool checkContain = item.contains("type") && item.contains("detail");
        CHECK_AND_CONTINUE(checkContain);
        bool checkType = item["type"].is_string() && item["detail"].is_string() && item["type"] == "dualAccountId";
        CHECK_AND_CONTINUE(checkType);
        oldId = item["detail"];
        MEDIA_INFO_LOG("the old is %{public}s", oldId.c_str());
        break;
    }
    std::pair<bool, OHOS::AccountSA::OhosAccountInfo> ret =
        OHOS::AccountSA::OhosAccountKits::GetInstance().QueryOhosAccountInfo();
    if (ret.first) {
        OHOS::AccountSA::OhosAccountInfo& resultInfo = ret.second;
        newId = resultInfo.uid_;
    } else {
        MEDIA_ERR_LOG("new account logins failed.");
        return;
    }
    MEDIA_INFO_LOG("the old id is %{public}s, new id is %{public}s",
        BackupFileUtils::GarbleFilePath(oldId, sceneCode_).c_str(),
        BackupFileUtils::GarbleFilePath(newId, sceneCode_).c_str());
    isAccountValid_ = ((oldId != "") && (oldId == newId));
}

void BaseRestore::GetSyncSwitchOn()
{
    syncSwitchType_ = BackupFileUtils::IsCloneCloudSyncSwitchOn(sceneCode_);
    isSyncSwitchOn_ = (syncSwitchType_ == CheckSwitchType::SUCCESS_ON ||
        syncSwitchType_ == CheckSwitchType::UPGRADE_FAILED_ON);
}

void BaseRestore::GetSourceDeviceInfo()
{
    nlohmann::json jsonArray = nlohmann::json::parse(restoreInfo_, nullptr, false);
    CHECK_AND_RETURN_LOG(!jsonArray.is_discarded(), "GetSourceDeviceInfo parse restoreInfo_ fail.");

    for (const auto& item : jsonArray) {
        bool cond = (!item.contains("type") || !item.contains("detail"));
        CHECK_AND_CONTINUE(!cond);
        CHECK_AND_CONTINUE(item["type"].is_string() && item["detail"].is_string());
        CHECK_AND_EXECUTE(item["type"] != "dualOdid", albumOdid_ = item["detail"]);
        if (item["type"] == "dualDeviceSoftName") {
            dualDeviceSoftName_ = item["detail"];
            MEDIA_INFO_LOG("get dualDeviceSoftName, %{public}s", dualDeviceSoftName_.c_str());
        }
    }
}

bool BaseRestore::IsRestorePhoto()
{
    if (sceneCode_ == UPGRADE_RESTORE_ID) {
        return true;
    }
    nlohmann::json jsonArray = nlohmann::json::parse(restoreInfo_, nullptr, false);
    CHECK_AND_RETURN_RET_LOG(!jsonArray.is_discarded(), true, "IsRestorePhoto parse restoreInfo_ fail.");

    for (const auto& item : jsonArray) {
        bool checkContain = item.contains("type") && item.contains("detail");
        CHECK_AND_CONTINUE(checkContain);
        bool checkType = item["type"].is_string() && item["detail"].is_string() && item["type"] == STAT_KEY_BACKUP_INFO;
        CHECK_AND_CONTINUE(checkType);
        for (const auto& backupInfo : item["detail"]) {
            bool conds = (backupInfo == STAT_TYPE_PHOTO || backupInfo == STAT_TYPE_VIDEO||
                backupInfo == STAT_TYPE_GALLERY_DATA);
            CHECK_AND_RETURN_RET(!conds, true);
        }
        MEDIA_INFO_LOG("not restore photo or video");
        return false;
    }
    return true;
}

void BaseRestore::StartRestore(const std::string &backupRetoreDir, const std::string &upgradePath)
{
    MEDIA_INFO_LOG("enter StartRestore");
    if (WaitSouthDeviceExitTimeout()) {
        MEDIA_ERR_LOG("baseRestore, wait south device cleaning data timeout.");
        SetErrorCode(RestoreError::RETAIN_FORCE_TIMEOUT);
        ErrorInfo errorInfo(RestoreError::RETAIN_FORCE_TIMEOUT, 0, "",
            "baseRestore, wait south device cleaning data timeout.");
        UpgradeRestoreTaskReport(sceneCode_, taskId_).ReportError(errorInfo);
        return;
    }
    MEDIA_INFO_LOG("StartRestore begin");

    backupRestoreDir_ = backupRetoreDir;
    upgradeRestoreDir_ = upgradePath;
    GetAccountValid();
    GetSyncSwitchOn();
    GetSourceDeviceInfo();
    int32_t errorCode = Init(backupRetoreDir, upgradePath, true);
    if (errorCode == E_OK) {
        RestorePhoto();
        RestoreAudio();
        MEDIA_INFO_LOG("migrate database number: %{public}lld, file number: %{public}lld (%{public}lld + "
            "%{public}lld), duplicate number: %{public}lld + %{public}lld, audio database number:%{public}lld, "
            "audio file number:%{public}lld, duplicate audio number: %{public}lld, map number: %{public}lld",
            (long long)migrateDatabaseNumber_, (long long)migrateFileNumber_,
            (long long)(migrateFileNumber_ - migrateVideoFileNumber_), (long long)migrateVideoFileNumber_,
            (long long)migratePhotoDuplicateNumber_, (long long)migrateVideoDuplicateNumber_,
            (long long)migrateAudioDatabaseNumber_, (long long)migrateAudioFileNumber_,
            (long long)migrateAudioDuplicateNumber_, (long long) migrateDatabaseMapNumber_);
        UpdateDatabase();
    } else {
        CHECK_AND_EXECUTE(errorCode == EXTERNAL_DB_NOT_EXIST, SetErrorCode(RestoreError::INIT_FAILED));
        ErrorInfo errorInfo(RestoreError::INIT_FAILED, 0, errorCode);
        UpgradeRestoreTaskReport().SetSceneCode(this->sceneCode_).SetTaskId(this->taskId_).ReportError(errorInfo);
    }
    HandleRestData();
}

int32_t BaseRestore::Init(void)
{
    if (mediaLibraryRdb_ != nullptr) {
        return E_OK;
    }
    auto context = AbilityRuntime::Context::GetApplicationContext();
    if (context == nullptr) {
        ErrorInfo errorInfo(RestoreError::INIT_FAILED, 0, "", "Failed to get context");
        UpgradeRestoreTaskReport().SetSceneCode(this->sceneCode_).SetTaskId(this->taskId_).ReportError(errorInfo);
        MEDIA_ERR_LOG("Failed to get context");
        return E_FAIL;
    }

    int32_t err = BackupDatabaseUtils::InitDb(mediaLibraryRdb_, MEDIA_DATA_ABILITY_DB_NAME, DATABASE_PATH, BUNDLE_NAME,
        true, context->GetArea());
    if (err != E_OK) {
        ErrorInfo errorInfo(RestoreError::INIT_FAILED, 0, "", "medialibrary rdb fail, err = " + std::to_string(err));
        UpgradeRestoreTaskReport().SetSceneCode(this->sceneCode_).SetTaskId(this->taskId_).ReportError(errorInfo);
        MEDIA_ERR_LOG("medialibrary rdb fail, err = %{public}d", err);
        return E_FAIL;
    }

    int32_t sceneCode = 0;
    int32_t errCode = MediaLibraryDataManager::GetInstance()->InitMediaLibraryMgr(context, nullptr, sceneCode, false);
    if (errCode != E_OK) {
        ErrorInfo errorInfo(RestoreError::INIT_FAILED, 0, "",
            "When restore, InitMediaLibraryMgr fail, errcode = " + std::to_string(errCode));
        UpgradeRestoreTaskReport().SetSceneCode(this->sceneCode_).SetTaskId(this->taskId_).ReportError(errorInfo);
        MEDIA_ERR_LOG("When restore, InitMediaLibraryMgr fail, errcode = %{public}d", errCode);
        return errCode;
    }

    migrateDatabaseNumber_ = 0;
    migrateFileNumber_ = 0;
    migrateVideoFileNumber_ = 0;
    migrateAudioDatabaseNumber_ = 0;
    migrateAudioFileNumber_ = 0;
    imageNumber_ = BackupDatabaseUtils::QueryUniqueNumber(mediaLibraryRdb_, IMAGE_ASSET_TYPE);
    videoNumber_ = BackupDatabaseUtils::QueryUniqueNumber(mediaLibraryRdb_, VIDEO_ASSET_TYPE);
    audioNumber_ = BackupDatabaseUtils::QueryUniqueNumber(mediaLibraryRdb_, AUDIO_ASSET_TYPE);
    MEDIA_INFO_LOG("imageNumber: %{public}d", (int)imageNumber_);
    MEDIA_INFO_LOG("videoNumber: %{public}d", (int)videoNumber_);
    MEDIA_INFO_LOG("audioNumber: %{public}d", (int)audioNumber_);
    photosDataHandler_.OnStart(sceneCode_, taskId_, mediaLibraryRdb_);
    photosDataHandler_.HandleDirtyFiles(isRestore_);
    return E_OK;
}

bool BaseRestore::ConvertPathToRealPath(const std::string &srcPath, const std::string &prefix,
    std::string &newPath, std::string &relativePath)
{
    size_t pos = 0;
    int32_t count = 0;
    constexpr int32_t prefixLevel = 4;
    for (size_t i = 0; i < srcPath.length(); i++) {
        if (srcPath[i] == '/') {
            count++;
            if (count == prefixLevel) {
                pos = i;
                break;
            }
        }
    }
    CHECK_AND_RETURN_RET(count >= prefixLevel, false);
    relativePath = srcPath.substr(pos);
    if (!dualDirName_.empty() && relativePath.find(dualDirName_) != string::npos) {
        std::size_t posStart = relativePath.find_first_of("/");
        std::size_t posEnd = relativePath.find_first_of("/", posStart + 1);
        if (posEnd != string::npos) {
            string temp = relativePath.substr(posStart + 1, posEnd - posStart -1);
            if (temp == dualDirName_) {
                relativePath.replace(relativePath.find(dualDirName_), dualDirName_.length(), SINGLE_DIR_NAME);
            }
        }
    }
    std::string extraPrefix = BackupFileUtils::GetExtraPrefixForRealPath(sceneCode_, srcPath);
    newPath = prefix + extraPrefix + relativePath;
    return true;
}

shared_ptr<NativeRdb::ResultSet> BaseRestore::QuerySql(const string &sql, const vector<string> &selectionArgs) const
{
    CHECK_AND_RETURN_RET_LOG(mediaLibraryRdb_ != nullptr, nullptr,
        "Pointer rdb_ is nullptr. Maybe it didn't init successfully.");
    return mediaLibraryRdb_->QuerySql(sql, selectionArgs);
}

int32_t BaseRestore::MoveFile(const std::string &srcFile, const std::string &dstFile) const
{
    if (MediaFileUtils::MoveFile(srcFile, dstFile)) {
        return E_OK;
    }
    if (this->CopyFile(srcFile, dstFile) != E_OK) {
        return E_FAIL;
    }
    (void)MediaFileUtils::DeleteFile(srcFile);
    return E_OK;
}

int32_t BaseRestore::CopyFile(const std::string &srcFile, const std::string &dstFile) const
{
    if (!MediaFileUtils::CopyFileUtil(srcFile, dstFile)) {
        MEDIA_ERR_LOG("CopyFile failed, filePath: %{private}s, errmsg: %{public}s", srcFile.c_str(),
            strerror(errno));
        return E_FAIL;
    }
    return E_OK;
}

int32_t BaseRestore::IsFileValid(FileInfo &fileInfo, const int32_t sceneCode)
{
    int32_t errCode = BackupFileUtils::IsFileValid(fileInfo.filePath, DUAL_FRAME_CLONE_RESTORE_ID,
        fileInfo.relativePath, hasLowQualityImage_);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("File is not valid: %{public}s, errno=%{public}d.",
            BackupFileUtils::GarbleFilePath(fileInfo.filePath, sceneCode).c_str(), errno);
        return errCode;
    }

    if (BackupFileUtils::IsLivePhoto(fileInfo)) {
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsFileValid(fileInfo.movingPhotoVideoPath), E_FAIL,
            "Moving photo video is not valid: %{public}s, errno=%{public}d.",
            BackupFileUtils::GarbleFilePath(fileInfo.movingPhotoVideoPath, sceneCode).c_str(), errno);

        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsFileValid(fileInfo.extraDataPath), E_FAIL,
            "Media extra data is not valid: %{public}s, errno=%{public}d.",
            BackupFileUtils::GarbleFilePath(fileInfo.extraDataPath, sceneCode).c_str(), errno);
    }
    return E_OK;
}

static void RemoveDuplicateDualCloneFiles(const FileInfo &fileInfo)
{
    (void)MediaFileUtils::DeleteFile(fileInfo.filePath);
    if (BackupFileUtils::IsLivePhoto(fileInfo)) {
        (void)MediaFileUtils::DeleteFile(fileInfo.movingPhotoVideoPath);
        (void)MediaFileUtils::DeleteFile(fileInfo.extraDataPath);
    }
}

static bool NeedReportError(int32_t restoreMode, int32_t userId)
{
    if (restoreMode == RESTORE_MODE_PROC_MAIN_DATA) {
        return userId == APP_MAIN_DATA_USER_ID;
    } else if (restoreMode == RESTORE_MODE_PROC_TWIN_DATA) {
        return userId >= APP_TWIN_DATA_USER_ID_START && userId <= APP_TWIN_DATA_USER_ID_END;
    }

    return true;
}

vector<NativeRdb::ValuesBucket> BaseRestore::GetInsertValues(const int32_t sceneCode, std::vector<FileInfo> &fileInfos,
    int32_t sourceType)
{
    vector<NativeRdb::ValuesBucket> values;
    int64_t prepareCost = 0;
    int64_t parseCost = 0;
    int64_t duplicateCost = 0;
    for (size_t i = 0; i < fileInfos.size(); i++) {
        int64_t startPrepare = MediaFileUtils::UTCTimeMilliSeconds();
        if (fileInfos[i].filePath.length() >= MAX_FILE_PATH_LENGTH) {
            MEDIA_WARN_LOG("abnormal filePath: %{private}s", fileInfos[i].filePath.c_str());
        }
        NativeRdb::ValuesBucket value;
        CHECK_AND_CONTINUE(PrepareInsertValue(sceneCode, fileInfos[i], sourceType, value));
        int64_t startMetaData = MediaFileUtils::UTCTimeMilliSeconds();
        SetValueFromMetaData(fileInfos[i], value);
        int64_t startDuplicate = MediaFileUtils::UTCTimeMilliSeconds();
        if ((sceneCode == DUAL_FRAME_CLONE_RESTORE_ID || sceneCode == OTHERS_PHONE_CLONE_RESTORE ||
            sceneCode == LITE_PHONE_CLONE_RESTORE || sceneCode == I_PHONE_CLONE_RESTORE ||
            sceneCode == UPGRADE_RESTORE_ID || sceneCode == CLOUD_BACKUP_RESTORE_ID) &&
            this->HasSameFileForDualClone(fileInfos[i])) {
            fileInfos[i].needMove = false;
            RemoveDuplicateDualCloneFiles(fileInfos[i]);
            MEDIA_WARN_LOG("File %{public}s already exists.",
                BackupFileUtils::GarbleFilePath(fileInfos[i].filePath, sceneCode).c_str());
            UpdateDuplicateNumber(fileInfos[i].fileType);
            continue;
        }
        CHECK_AND_EXECUTE(!fileInfos[i].isNew, values.emplace_back(value));
        int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
        prepareCost += startMetaData - startPrepare;
        parseCost += startDuplicate - startMetaData;
        duplicateCost += end - startDuplicate;
    }
    MEDIA_INFO_LOG("TimeCost: batch Prepare cost: %{public}" PRId64 ", SetValueFromMetaData cost: %{public}" PRId64
        ", DuplicateDualCloneFile cost: %{public}" PRId64, prepareCost, parseCost, duplicateCost);
    return values;
}

bool BaseRestore::PrepareInsertValue(const int32_t sceneCode, FileInfo &fileInfo, int32_t sourceType,
    NativeRdb::ValuesBucket &value)
{
    int32_t errCode = IsFileValid(fileInfo, sceneCode);
    if (errCode != E_OK) {
        fileInfo.needMove = false;
        if (!NeedReportError(restoreMode_, fileInfo.userId)) {
            MEDIA_WARN_LOG("file not found but no need report, file name:%{public}s",
                BackupFileUtils::GarbleFilePath(fileInfo.filePath, sceneCode).c_str());
            notFoundNumber_++;
            return false;
        }
        std::string fileDbCheckInfo = CheckInvalidFile(fileInfo, errCode);
        ErrorInfo errorInfo(RestoreError::FILE_INVALID, 1, std::to_string(errCode),
            BackupLogUtils::FileInfoToString(sceneCode, fileInfo, { fileDbCheckInfo }));
        UpgradeRestoreTaskReport().SetSceneCode(this->sceneCode_).SetTaskId(this->taskId_).ReportError(errorInfo);
        return false;
    }
    std::string cloudPath;
    int32_t uniqueId = GetUniqueId(fileInfo.fileType);
    errCode = BackupFileUtils::CreateAssetPathById(uniqueId, fileInfo.fileType,
        MediaFileUtils::GetExtensionFromPath(fileInfo.displayName), cloudPath);
    if (errCode != E_OK) {
        fileInfo.needMove = false;
        ErrorInfo errorInfo(RestoreError::CREATE_PATH_FAILED, 1, std::to_string(errCode),
            BackupLogUtils::FileInfoToString(sceneCode, fileInfo));
        UpgradeRestoreTaskReport().SetSceneCode(this->sceneCode_).SetTaskId(this->taskId_).ReportError(errorInfo);
        return false;
    }
    fileInfo.cloudPath = cloudPath;
    value = GetInsertValue(fileInfo, cloudPath, sourceType);
    return true;
}

static void InsertDateTaken(std::unique_ptr<Metadata> &metadata, FileInfo &fileInfo, NativeRdb::ValuesBucket &value)
{
    int64_t dateTaken = metadata->GetDateTaken();
    if (dateTaken > 0) {
        bool hasDateTaken = value.HasColumn(PhotoColumn::MEDIA_DATE_TAKEN);
        if (hasDateTaken) {
            value.Delete(PhotoColumn::MEDIA_DATE_TAKEN);
        }
        value.PutLong(MediaColumn::MEDIA_DATE_TAKEN, dateTaken);
        fileInfo.dateTaken = dateTaken;
        return;
    }

    int64_t dateAdded = metadata->GetFileDateAdded();
    if (dateAdded <= 0) {
        int64_t dateModified = metadata->GetFileDateModified();
        if (dateModified <= 0) {
            dateTaken = MediaFileUtils::UTCTimeMilliSeconds();
        } else {
            dateTaken = dateModified;
        }
    } else {
        dateTaken = dateAdded;
    }
    bool hasDateTaken = value.HasColumn(PhotoColumn::MEDIA_DATE_TAKEN);
    if (hasDateTaken) {
        value.Delete(PhotoColumn::MEDIA_DATE_TAKEN);
    }
    value.PutLong(MediaColumn::MEDIA_DATE_TAKEN, dateTaken);
    fileInfo.dateTaken = dateTaken;
}

vector<NativeRdb::ValuesBucket> BaseRestore::GetCloudInsertValues(const int32_t sceneCode,
    std::vector<FileInfo> &fileInfos, int32_t sourceType)
{
    MEDIA_INFO_LOG("START STEP 3 GET INSERT VALUES");
    int64_t prepareCost = 0;
    int64_t parseCost = 0;
    int64_t duplicateCost = 0;
    vector<NativeRdb::ValuesBucket> values;
    for (size_t i = 0; i < fileInfos.size(); i++) {
        int64_t startPrepare = MediaFileUtils::UTCTimeMilliSeconds();
        std::string cloudPath;
        int32_t uniqueId = GetUniqueId(fileInfos[i].fileType);
        int32_t errCode = BackupFileUtils::CreateAssetPathById(uniqueId, fileInfos[i].fileType,
            MediaFileUtils::GetExtensionFromPath(fileInfos[i].displayName), cloudPath);
        if (errCode != E_OK) {
            fileInfos[i].needMove = false;
            ErrorInfo errorInfo(RestoreError::CREATE_PATH_FAILED, 1, std::to_string(errCode),
                BackupLogUtils::FileInfoToString(sceneCode, fileInfos[i]));
            UpgradeRestoreTaskReport().SetSceneCode(this->sceneCode_).SetTaskId(this->taskId_).ReportError(errorInfo);
            continue;
        }
        fileInfos[i].cloudPath = cloudPath;
        NativeRdb::ValuesBucket value = GetInsertValue(fileInfos[i], cloudPath, sourceType);
        int64_t startMetaData = MediaFileUtils::UTCTimeMilliSeconds();
        SetValueFromMetaData(fileInfos[i], value);
        int64_t startDuplicate = MediaFileUtils::UTCTimeMilliSeconds();
        if ((sceneCode == DUAL_FRAME_CLONE_RESTORE_ID || sceneCode == UPGRADE_RESTORE_ID) &&
            this->HasSameFileForDualClone(fileInfos[i])) {
            fileInfos[i].needMove = false;
            RemoveDuplicateDualCloneFiles(fileInfos[i]);
            MEDIA_WARN_LOG("Cloud File %{public}s already exists.",
                BackupFileUtils::GarbleFilePath(fileInfos[i].filePath, sceneCode).c_str());
            UpdateDuplicateNumber(fileInfos[i].fileType);
            continue;
        }
        values.emplace_back(value);
        int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
        prepareCost += startMetaData - startPrepare;
        parseCost += startDuplicate - startMetaData;
        duplicateCost += end - startDuplicate;
    }
    MEDIA_DEBUG_LOG("END STEP 3 GET INSERT VALUES");
    MEDIA_INFO_LOG("TimeCost: cloud Prepare cost: %{public}" PRId64 ", SetValueFromMetaData cost: %{public}" PRId64
        ", DuplicateDualCloneFile cost: %{public}" PRId64, prepareCost, parseCost, duplicateCost);
    return values;
}

void BaseRestore::InsertVideoMode(std::unique_ptr<Metadata> &metadata, NativeRdb::ValuesBucket &value)
{
    MEDIA_INFO_LOG("BaseRestore::InsertVideoMode");
    int32_t videoMode = metadata->GetVideoMode();
    MEDIA_INFO_LOG("InsertVideoMode videoMode=%{public}d", videoMode);
    value.PutInt(PhotoColumn::PHOTO_VIDEO_MODE, videoMode);
}

static void InsertDateAdded(std::unique_ptr<Metadata> &metadata, NativeRdb::ValuesBucket &value)
{
    int64_t dateAdded = metadata->GetFileDateAdded();
    if (dateAdded != 0) {
        value.PutLong(MediaColumn::MEDIA_DATE_ADDED, dateAdded);
        return;
    }

    int64_t dateTaken = metadata->GetDateTaken();
    if (dateTaken == 0) {
        int64_t dateModified = metadata->GetFileDateModified();
        if (dateModified == 0) {
            dateAdded = MediaFileUtils::UTCTimeMilliSeconds();
        } else {
            dateAdded = dateModified;
        }
    } else {
        dateAdded = dateTaken;
    }
    value.PutLong(MediaColumn::MEDIA_DATE_ADDED, dateAdded);
}

static void InsertUserComment(std::unique_ptr<Metadata> &metadata, NativeRdb::ValuesBucket &value,
    FileInfo &fileInfo)
{
    if (fileInfo.userComment.empty()) {
        fileInfo.userComment = metadata->GetUserComment();
    }

    bool hasUserComment = value.HasColumn(PhotoColumn::PHOTO_USER_COMMENT);
    if (hasUserComment) {
        value.Delete(PhotoColumn::PHOTO_USER_COMMENT);
    }
    value.PutString(PhotoColumn::PHOTO_USER_COMMENT, fileInfo.userComment);
}

void BaseRestore::InsertDateTime(NativeRdb::ValuesBucket &value, FileInfo &fileInfo)
{
    const auto [dateTaken, detailTime] =
        PhotoFileUtils::ExtractTimeInfo(fileInfo.detailTime, PhotoColumn::PHOTO_DETAIL_TIME_FORMAT);
    if (dateTaken < MIN_MILSEC_TIMESTAMP || dateTaken > MAX_MILSEC_TIMESTAMP ||
        abs(fileInfo.dateTaken - dateTaken) > MAX_TIMESTAMP_DIFF) {
        MEDIA_ERR_LOG("invalid detailTime: %{public}s, dateTaken: %{public}lld",
            fileInfo.detailTime.c_str(),
            static_cast<long long>(fileInfo.dateTaken));
        fileInfo.detailTime =
            MediaFileUtils::StrCreateTimeByMilliseconds(PhotoColumn::PHOTO_DETAIL_TIME_FORMAT, fileInfo.dateTaken);
    } else {
        fileInfo.detailTime = detailTime;
    }

    value.Put(PhotoColumn::PHOTO_DETAIL_TIME, fileInfo.detailTime);

    const auto [dateYear, dateMonth, dateDay] = PhotoFileUtils::ExtractYearMonthDay(fileInfo.detailTime);
    value.Put(PhotoColumn::PHOTO_DATE_YEAR, dateYear);
    value.Put(PhotoColumn::PHOTO_DATE_MONTH, dateMonth);
    value.Put(PhotoColumn::PHOTO_DATE_DAY, dateDay);
}

void BaseRestore::CheckAndDelete(NativeRdb::ValuesBucket &value, const std::string &column)
{
    bool hasColumn = value.HasColumn(column);
    if (hasColumn) {
        value.Delete(column);
    }
}

void BaseRestore::InsertFileDuration(const std::unique_ptr<Metadata> &metadata, NativeRdb::ValuesBucket &value,
    FileInfo &fileInfo)
{
    int32_t duration = metadata->GetFileDuration();
    if (duration != 0) {
        CheckAndDelete(value, MediaColumn::MEDIA_DURATION);
    }
    value.PutInt(MediaColumn::MEDIA_DURATION, duration);
}

void BaseRestore::SetCoverPosition(const FileInfo &fileInfo, NativeRdb::ValuesBucket &value)
{
    uint64_t coverPosition = 0;
    if (BackupFileUtils::IsLivePhoto(fileInfo)) {
        uint32_t version = 0;
        uint32_t frameIndex = 0;
        bool hasCinemagraphInfo = false;
        string absExtraDataPath;
        if (!PathToRealPath(fileInfo.extraDataPath, absExtraDataPath)) {
            MEDIA_ERR_LOG("file is not real path: %{private}s, errno: %{public}d",
                fileInfo.extraDataPath.c_str(), errno);
            value.PutLong(PhotoColumn::PHOTO_COVER_POSITION, static_cast<int64_t>(coverPosition));
            return;
        }
        UniqueFd extraDataFd(open(absExtraDataPath.c_str(), O_RDONLY));
        (void)MovingPhotoFileUtils::GetVersionAndFrameNum(extraDataFd.Get(), version, frameIndex, hasCinemagraphInfo);
        (void)MovingPhotoFileUtils::GetCoverPosition(fileInfo.movingPhotoVideoPath,
            frameIndex, coverPosition, Scene::AV_META_SCENE_CLONE);
    }
    value.PutLong(PhotoColumn::PHOTO_COVER_POSITION, static_cast<int64_t>(coverPosition));
}

void BaseRestore::SetMetaDataValue(const FileInfo &fileInfo, std::unique_ptr<Metadata> &data)
{
    data->SetFilePath(fileInfo.filePath);
    data->SetFileMediaType(fileInfo.fileType);
    data->SetFileDateModified(fileInfo.dateModified);
    data->SetFileName(fileInfo.displayName);
    BackupFileUtils::FillMetadata(data);
}

double BaseRestore::GetDataLongitude(const FileInfo &fileInfo, std::unique_ptr<Metadata> &data)
{
    double longitude = data->GetLongitude();
    double latitude = data->GetLatitude();
    if ((sceneCode_ == UPGRADE_RESTORE_ID || sceneCode_ == DUAL_FRAME_CLONE_RESTORE_ID) &&
        std::fabs(longitude - DEFAULT_LONGITUDE) < DOUBLE_EPSILON &&
        std::fabs(latitude - DEFAULT_LATITUDE) < DOUBLE_EPSILON) {
        return fileInfo.longitude;
    }
    if (sceneCode_ == I_PHONE_CLONE_RESTORE && (!(std::fabs(fileInfo.longitude - DEFAULT_LONGITUDE) < DOUBLE_EPSILON)
        || !(std::fabs(fileInfo.latitude - DEFAULT_LATITUDE) < DOUBLE_EPSILON))) {
            return fileInfo.longitude;
    }
    return longitude;
}

double BaseRestore::GetDataLatitude(const FileInfo &fileInfo, std::unique_ptr<Metadata> &data)
{
    double longitude = data->GetLongitude();
    double latitude = data->GetLatitude();
    if ((sceneCode_ == UPGRADE_RESTORE_ID || sceneCode_ == DUAL_FRAME_CLONE_RESTORE_ID) &&
        std::fabs(longitude - DEFAULT_LONGITUDE) < DOUBLE_EPSILON &&
        std::fabs(latitude - DEFAULT_LATITUDE) < DOUBLE_EPSILON) {
        return fileInfo.latitude;
    }
    if (sceneCode_ == I_PHONE_CLONE_RESTORE && (!(std::fabs(fileInfo.longitude - DEFAULT_LONGITUDE) < DOUBLE_EPSILON)
        || !(std::fabs(fileInfo.latitude - DEFAULT_LATITUDE) < DOUBLE_EPSILON))) {
            return fileInfo.latitude;
    }
    return latitude;
}

void BaseRestore::Set3DgsSubtype(FileInfo &info, NativeRdb::ValuesBucket &value,
    std::unique_ptr<Metadata> &data)
{
    if (data->GetPhotoSubType() == static_cast<int32_t>(PhotoSubType::SPATIAL_3DGS)) {
        info.subtype = data->GetPhotoSubType();
        value.Delete(PhotoColumn::PHOTO_SUBTYPE);
        value.PutInt(PhotoColumn::PHOTO_SUBTYPE, info.subtype);
    }
}

void BaseRestore::SetValueFromMetaData(FileInfo &fileInfo, NativeRdb::ValuesBucket &value)
{
    std::unique_ptr<Metadata> data = make_unique<Metadata>();
    SetMetaDataValue(fileInfo, data);

    value.PutString(MediaColumn::MEDIA_FILE_PATH, data->GetFilePath());
    value.PutString(MediaColumn::MEDIA_MIME_TYPE, data->GetFileMimeType());
    value.PutString(PhotoColumn::PHOTO_MEDIA_SUFFIX, data->GetFileExtension());
    value.PutInt(MediaColumn::MEDIA_TYPE, data->GetFileMediaType());
    value.PutString(MediaColumn::MEDIA_TITLE, data->GetFileTitle());
    if (fileInfo.fileSize != 0) {
        value.PutLong(MediaColumn::MEDIA_SIZE, fileInfo.fileSize);
    } else {
        MEDIA_WARN_LOG("DB file size is zero!");
        value.PutLong(MediaColumn::MEDIA_SIZE, data->GetFileSize());
        fileInfo.fileSize = data->GetFileSize();
    }
    value.PutLong(MediaColumn::MEDIA_DATE_MODIFIED, data->GetFileDateModified());
    InsertFileDuration(data, value, fileInfo);
    if (fileInfo.dateTaken <= 0) {
        InsertDateTaken(data, fileInfo, value);
    }
    InsertDateTime(value, fileInfo);
    value.PutLong(MediaColumn::MEDIA_TIME_PENDING, 0);
    value.PutInt(PhotoColumn::PHOTO_HEIGHT, data->GetFileHeight());
    value.PutInt(PhotoColumn::PHOTO_WIDTH, data->GetFileWidth());
    value.PutDouble(PhotoColumn::PHOTO_ASPECT_RATIO, data->GetFileAspectRatio());
    value.PutDouble(PhotoColumn::PHOTO_LONGITUDE, GetDataLongitude(fileInfo, data));
    value.PutDouble(PhotoColumn::PHOTO_LATITUDE, GetDataLatitude(fileInfo, data));
    value.PutString(PhotoColumn::PHOTO_ALL_EXIF, data->GetAllExif());
    value.PutString(PhotoColumn::PHOTO_SHOOTING_MODE, data->GetShootingMode());
    value.PutString(PhotoColumn::PHOTO_SHOOTING_MODE_TAG, data->GetShootingModeTag());
    value.PutLong(PhotoColumn::PHOTO_LAST_VISIT_TIME, data->GetLastVisitTime());
    value.PutString(PhotoColumn::PHOTO_FRONT_CAMERA, data->GetFrontCamera());
    value.PutInt(PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE, data->GetDynamicRangeType());
    value.PutInt(PhotoColumn::PHOTO_HDR_MODE, data->GetHdrMode());
    InsertDateAdded(data, value);
    SetOrientationAndExifRotate(fileInfo, value, data);
    InsertUserComment(data, value, fileInfo);
    int64_t dateAdded = 0;
    NativeRdb::ValueObject valueObject;
    if (value.GetObject(MediaColumn::MEDIA_DATE_ADDED, valueObject)) {
        valueObject.GetLong(dateAdded);
    }
    fileInfo.dateAdded = dateAdded;
    SetCoverPosition(fileInfo, value);
    Set3DgsSubtype(fileInfo, value, data);
    InsertVideoMode(data, value);
}

void BaseRestore::CreateDir(std::string &dir)
{
    if (!MediaFileUtils::IsFileExists(dir)) {
        MediaFileUtils::CreateDirectory(dir);
    }
}

void BaseRestore::RecursiveCreateDir(std::string &relativePath, std::string &suffix)
{
    CreateDir(relativePath);
    size_t pos = suffix.find('/');
    CHECK_AND_RETURN_LOG(pos != std::string::npos, "Can not find slash");
    std::string prefix = suffix.substr(0, pos + 1);
    std::string suffixTmp = suffix.erase(0, prefix.length());
    prefix = relativePath + prefix;
    RecursiveCreateDir(prefix, suffixTmp);
}

void BaseRestore::InsertAudio(int32_t sceneCode, std::vector<FileInfo> &fileInfos)
{
    if (fileInfos.empty()) {
        MEDIA_ERR_LOG("fileInfos are empty");
        return;
    }
    int64_t startMove = MediaFileUtils::UTCTimeMilliSeconds();
    int32_t fileMoveCount = 0;
    for (size_t i = 0; i < fileInfos.size(); i++) {
        if (!MediaFileUtils::IsFileExists(fileInfos[i].filePath)) {
            MEDIA_ERR_LOG("File is not exist: filePath: %{public}s, size: %{public}lld",
                BackupFileUtils::GarbleFilePath(fileInfos[i].filePath, sceneCode).c_str(),
                (long long)fileInfos[i].fileSize);
            continue;
        }
        string relativePath0 = RESTORE_MUSIC_LOCAL_DIR;
        string relativePath1 = fileInfos[i].relativePath;
        if (relativePath1[0] == '/') {
            relativePath1.erase(0, 1);
        }
        string dstPath = RESTORE_MUSIC_LOCAL_DIR + relativePath1;
        RecursiveCreateDir(relativePath0, relativePath1);
        if (MediaFileUtils::IsFileExists(dstPath)) {
            MEDIA_INFO_LOG("dstPath %{public}s already exists.",
                BackupFileUtils::GarbleFilePath(fileInfos[i].filePath, sceneCode).c_str());
            UpdateDuplicateNumber(fileInfos[i].fileType);
            continue;
        }
        int32_t moveErrCode = BackupFileUtils::MoveFile(fileInfos[i].filePath, dstPath, sceneCode);
        if (moveErrCode != E_SUCCESS) {
            MEDIA_ERR_LOG("MoveFile failed, filePath: %{public}s, errCode: %{public}d, errno: %{public}d",
                BackupFileUtils::GarbleFilePath(fileInfos[i].filePath, sceneCode).c_str(), moveErrCode, errno);
            UpdateFailedFiles(fileInfos[i].fileType, fileInfos[i], RestoreError::MOVE_FAILED);
            continue;
        }
        MediaFileUtils::UpdateModifyTimeInMsec(dstPath, fileInfos[i].dateModified);
        fileMoveCount++;
    }
    migrateAudioFileNumber_ += fileMoveCount;
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("move %{public}ld file cost %{public}ld.", (long)fileMoveCount, (long)(end - startMove));
}

static bool MoveExtraData(const FileInfo &fileInfo, int32_t sceneCode)
{
    string localExtraDataDir = BackupFileUtils::GetReplacedPathByPrefixType(
        PrefixType::CLOUD, PrefixType::LOCAL, MovingPhotoFileUtils::GetMovingPhotoExtraDataDir(fileInfo.cloudPath));
    if (localExtraDataDir.empty()) {
        MEDIA_WARN_LOG("Failed to get local extra data dir");
        return false;
    }
    if (!MediaFileUtils::IsFileExists(localExtraDataDir) && !MediaFileUtils::CreateDirectory(localExtraDataDir)) {
        MEDIA_WARN_LOG("Failed to create local extra data dir, errno:%{public}d", errno);
        return false;
    }

    string localExtraDataPath = BackupFileUtils::GetReplacedPathByPrefixType(
        PrefixType::CLOUD, PrefixType::LOCAL, MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(fileInfo.cloudPath));
    if (localExtraDataPath.empty()) {
        MEDIA_WARN_LOG("Failed to get local extra data path");
        return false;
    }
    int32_t errCode = BackupFileUtils::MoveFile(fileInfo.extraDataPath, localExtraDataPath, sceneCode);
    if (errCode != E_OK) {
        MEDIA_WARN_LOG("MoveFile failed, src:%{public}s, dest:%{public}s, err:%{public}d, errno:%{public}d",
            BackupFileUtils::GarbleFilePath(fileInfo.extraDataPath, sceneCode).c_str(),
            BackupFileUtils::GarbleFilePath(localExtraDataPath, sceneCode).c_str(), errCode, errno);
        return false;
    }
    return true;
}

static bool SaveIosExtraData(const FileInfo &fileInfo, std::string movingPhotoVideoPath, int32_t sceneCode)
{
    string localExtraDataDir = BackupFileUtils::GetReplacedPathByPrefixType(
        PrefixType::CLOUD, PrefixType::LOCAL, MovingPhotoFileUtils::GetMovingPhotoExtraDataDir(fileInfo.cloudPath));
    if (localExtraDataDir.empty()) {
        MEDIA_WARN_LOG("Failed to get local extra data dir");
        return false;
    }
    if (!MediaFileUtils::IsFileExists(localExtraDataDir) && !MediaFileUtils::CreateDirectory(localExtraDataDir)) {
        MEDIA_WARN_LOG("Failed to create local extra data dir, errno:%{public}d", errno);
        return false;
    }

    string localExtraDataPath = BackupFileUtils::GetReplacedPathByPrefixType(
        PrefixType::CLOUD, PrefixType::LOCAL, MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(fileInfo.cloudPath));
    if (localExtraDataPath.empty()) {
        MEDIA_WARN_LOG("Failed to get local extra data path");
        return false;
    }

    size_t videoSize = 0;
    MediaFileUtils::GetFileSize(movingPhotoVideoPath, videoSize);

    std::string versionAndFrame = "v7_f0";
    versionAndFrame.resize(SINGLE_LEN_EXTRADATA, ' ');
    std::string startEndPosition = "0:0";
    startEndPosition.resize(SINGLE_LEN_EXTRADATA, ' ');
    std::string videoLengthStr = "LIVE_" + std::to_string(videoSize + SINGLE_LEN_EXTRADATA);
    videoLengthStr.resize(SINGLE_LEN_EXTRADATA, ' ');
    std::string extraData = versionAndFrame + startEndPosition + videoLengthStr;
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateFile(localExtraDataPath) &&
        MediaFileUtils::WriteStrToFile(localExtraDataPath, extraData), false,
        "Write to %{public}s failed", BackupFileUtils::GarbleFilePath(localExtraDataPath, sceneCode).c_str());
    return true;
}

static bool MoveAndModifyFile(const FileInfo &fileInfo, int32_t sceneCode)
{
    string tmpPath = fileInfo.cloudPath;
    string localPath = tmpPath.replace(0, RESTORE_CLOUD_DIR.length(), RESTORE_LOCAL_DIR);
    int32_t errCode = BackupFileUtils::MoveFile(fileInfo.filePath, localPath, sceneCode);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, false,
        "MoveFile failed, src:%{public}s, dest:%{public}s, err:%{public}d, errno:%{public}d",
        BackupFileUtils::GarbleFilePath(fileInfo.filePath, sceneCode).c_str(),
        BackupFileUtils::GarbleFilePath(localPath, sceneCode).c_str(), errCode, errno);
    MediaFileUtils::UpdateModifyTimeInMsec(localPath, fileInfo.dateModified);
    
    if (sceneCode == I_PHONE_CLONE_RESTORE && fileInfo.subtype == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)) {
        size_t destPos = fileInfo.filePath.find_last_of(".");
        CHECK_AND_RETURN_RET_LOG(destPos != std::string::npos, false, "fileInfo.filePath not contain '.'");
        string movSrcPath = fileInfo.filePath.substr(0, destPos) + ".MOV";

        destPos = localPath.find_last_of(".");
        CHECK_AND_RETURN_RET_LOG(destPos != std::string::npos, false, "localPath not contain '.'");
        string movTargetPath = localPath.substr(0, destPos) + ".mp4";
        
        errCode = BackupFileUtils::MoveFile(movSrcPath, movTargetPath, sceneCode);
        CHECK_AND_RETURN_RET_LOG(errCode == E_OK, false,
            "Move moving photo video failed, src:%{public}s, dest:%{public}s, err:%{public}d, errno:%{public}d",
            BackupFileUtils::GarbleFilePath(movSrcPath, sceneCode).c_str(),
            BackupFileUtils::GarbleFilePath(movTargetPath, sceneCode).c_str(), errCode, errno);
        MediaFileUtils::UpdateModifyTimeInMsec(movTargetPath, fileInfo.dateModified);
        
        CHECK_AND_RETURN_RET_LOG(SaveIosExtraData(fileInfo, movTargetPath, sceneCode), false,
            "Save the extraData for ios moving photo failed, src:%{public}s, dest:%{public}s",
            BackupFileUtils::GarbleFilePath(movSrcPath, sceneCode).c_str(),
            BackupFileUtils::GarbleFilePath(movTargetPath, sceneCode).c_str());
        return true;
    }

    if (BackupFileUtils::IsLivePhoto(fileInfo)) {
        string tmpVideoPath = MovingPhotoFileUtils::GetMovingPhotoVideoPath(fileInfo.cloudPath);
        string localVideoPath = tmpVideoPath.replace(0, RESTORE_CLOUD_DIR.length(), RESTORE_LOCAL_DIR);
        errCode = BackupFileUtils::MoveFile(fileInfo.movingPhotoVideoPath, localVideoPath, sceneCode);
        if (errCode != E_OK) {
            MEDIA_ERR_LOG(
                "MoveFile failed for mov video, src:%{public}s, dest:%{public}s, err:%{public}d, errno:%{public}d",
                BackupFileUtils::GarbleFilePath(fileInfo.movingPhotoVideoPath, sceneCode).c_str(),
                BackupFileUtils::GarbleFilePath(localVideoPath, sceneCode).c_str(), errCode, errno);
            (void)MediaFileUtils::DeleteFile(localPath);
            return false;
        }
        MediaFileUtils::UpdateModifyTimeInMsec(localVideoPath, fileInfo.dateModified);
        return MoveExtraData(fileInfo, sceneCode);
    }
    return true;
}

static std::string GetThumbnailLocalPath(const string &path)
{
    size_t cloudDirLength = RESTORE_FILES_CLOUD_DIR.length();
    bool cond = (path.length() <= cloudDirLength ||
        path.substr(0, cloudDirLength).compare(RESTORE_FILES_CLOUD_DIR) != 0);
    CHECK_AND_RETURN_RET(!cond, "");

    std::string suffixStr = path.substr(cloudDirLength);
    return RESTORE_FILES_LOCAL_DIR + ".thumbs/" + suffixStr;
}

int32_t BaseRestore::BatchCreateDentryFile(std::vector<FileInfo> &fileInfos, std::vector<std::string> &failCloudIds,
                                           std::string fileType)
{
    MEDIA_INFO_LOG("START STEP 4 CREATE DENTRY");
    bool cond = ((fileType != DENTRY_INFO_THM) && (fileType != DENTRY_INFO_LCD) && (fileType != DENTRY_INFO_ORIGIN));
    CHECK_AND_RETURN_RET_LOG(!cond, E_ERR, "Invalid fileType: %{public}s", fileType.c_str());
    std::vector<FileManagement::CloudSync::DentryFileInfo> dentryInfos;
    for (size_t i = 0; i < fileInfos.size(); i++) {
        CHECK_AND_CONTINUE(fileInfos[i].needMove);
        FileManagement::CloudSync::DentryFileInfo dentryInfo;
        dentryInfo.cloudId = fileInfos[i].uniqueId;
        dentryInfo.modifiedTime = fileInfos[i].dateModified;
        if (fileType == DENTRY_INFO_ORIGIN) {
            dentryInfo.fileType = fileType;
            dentryInfo.size = fileInfos[i].fileSize;
            dentryInfo.path = fileInfos[i].cloudPath;
            dentryInfo.fileName = fileInfos[i].displayName;
        } else {
            dentryInfo.size = THUMB_DENTRY_SIZE;
            if (fileType == DENTRY_INFO_LCD) {
                dentryInfo.fileType = (!HasExThumbnail(fileInfos[i])) ? fileType : LCD_EX_SUFFIX;
                dentryInfo.fileName = "LCD.jpg";
            } else {
                dentryInfo.fileType = (!HasExThumbnail(fileInfos[i])) ? fileType : THUMB_EX_SUFFIX;
                dentryInfo.fileName = "THM.jpg";
            }
        }
        dentryInfos.push_back(dentryInfo);
    }
    if (dentryInfos.empty()) {
        MEDIA_INFO_LOG("dentryInfos empty.");
        return E_OK;
    }
    int32_t ret = FileManagement::CloudSync::CloudSyncManager::GetInstance().BatchDentryFileInsert(
        dentryInfos, failCloudIds);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "BatchDentryFileInsert failed, ret: %{public}d.", ret);
    CHECK_AND_PRINT_LOG(failCloudIds.empty(), "failCloudIds size: %{public}zu, first one: %{public}s",
        failCloudIds.size(), failCloudIds[0].c_str());
    MEDIA_DEBUG_LOG("END STEP 4 CREATE DENTRY");
    return ret;
}

bool BaseRestore::RestoreLcdAndThumbFromCloud(const FileInfo &fileInfo, int32_t type, int32_t sceneCode)
{
    std::string srcPath = GetThumbFile(fileInfo, type, sceneCode);
    CHECK_AND_RETURN_RET(srcPath != "", false);
    std::string saveNoRotatePath = !HasExThumbnail(fileInfo) ? "" : THM_SAVE_WITHOUT_ROTATE_PATH;
    std::string dstDirPath = GetThumbnailLocalPath(fileInfo.cloudPath) + saveNoRotatePath;
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateDirectory(dstDirPath), false, "Prepare thumbnail dir path failed");

    std::string tmpTargetFileName = type == MIGRATE_CLOUD_LCD_TYPE ? "/LCD" : "/THM";
    std::string dstFilePath = dstDirPath + tmpTargetFileName + ".jpg";
    bool cond = (MediaFileUtils::IsFileExists(dstFilePath) && !MediaFileUtils::DeleteFile(dstFilePath));
    CHECK_AND_RETURN_RET_LOG(!cond, false, "Delete thumbnail new dir failed, errno: %{public}d", errno);

    int32_t errCode = MediaFileUtils::ModifyAsset(srcPath, dstFilePath);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, false,
        "MoveFile failed, src: %{public}s, dest: %{public}s, err: %{public}d, errno: %{public}d",
        BackupFileUtils::GarbleFilePath(srcPath, sceneCode).c_str(),
        BackupFileUtils::GarbleFilePath(dstFilePath, sceneCode).c_str(), errCode, errno);
    do {
        CHECK_AND_BREAK(HasExThumbnail(fileInfo));
        std::string rotateDirPath = dstDirPath.replace(dstDirPath.find(THM_SAVE_WITHOUT_ROTATE_PATH),
            THM_SAVE_WITHOUT_ROTATE_PATH.length(), "");
        if (type == MIGRATE_CLOUD_THM_TYPE &&
            MediaFileUtils::IsFileExists(rotateDirPath + tmpTargetFileName + ".jpg")) {
            rotateThmMigrateFileNumber_++;
            break;
        }
        if (fileInfo.fileType == MediaType::MEDIA_TYPE_IMAGE) {
            CHECK_AND_RETURN_RET_LOG(BackupFileUtils::HandleRotateImage(dstFilePath, rotateDirPath,
                fileInfo.exifRotate, type == MIGRATE_CLOUD_LCD_TYPE), false, "Rotate image fail!");
        } else {
            std::string rotateDirFile = rotateDirPath + tmpTargetFileName + ".jpg";
            CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CopyFileUtil(dstFilePath, rotateDirFile),
                false, "Copy file failed!");
        }
        type == MIGRATE_CLOUD_LCD_TYPE ? rotateLcdMigrateFileNumber_++ : rotateThmMigrateFileNumber_++;
    } while (0);
    if (fileInfo.localMediaId == -1) {
        type == MIGRATE_CLOUD_LCD_TYPE ? cloudLcdCount_++ : cloudThumbnailCount_++;
    } else {
        type == MIGRATE_CLOUD_LCD_TYPE ? localLcdCount_++ : localThumbnailCount_++;
    }
    return true;
}

std::string BaseRestore::GetThumbFile(const FileInfo &fileInfo, int32_t type, int32_t sceneCode)
{
    std::string thumbPath = type == MIGRATE_CLOUD_LCD_TYPE ? fileInfo.localBigThumbPath : fileInfo.localThumbPath;
    if (thumbPath != "") {
        std::string prefixStr = sceneCode == DUAL_FRAME_CLONE_RESTORE_ID ? backupRestoreDir_ : upgradeRestoreDir_;
        std::string srcPath = prefixStr + thumbPath;
        CHECK_AND_RETURN_RET(!MediaFileUtils::IsFileExists(srcPath), srcPath);
    }

    CHECK_AND_RETURN_RET(!fileInfo.albumId.empty(), "");
    CHECK_AND_RETURN_RET(!fileInfo.uniqueId.empty(), "");
    std::string newPrefixStr = sceneCode == DUAL_FRAME_CLONE_RESTORE_ID ?
        (backupRestoreDir_ + "/storage/emulated/0") : upgradeRestoreDir_;
    std::string tmpStr = type == MIGRATE_CLOUD_LCD_TYPE ? "lcd/" : "thumb/";
    std::string newSrcPath = newPrefixStr + "/.photoShare/thumb/" + tmpStr + fileInfo.albumId +
        "/" + fileInfo.uniqueId + ".jpg";
    CHECK_AND_RETURN_RET(!MediaFileUtils::IsFileExists(newSrcPath), newSrcPath);
    MEDIA_DEBUG_LOG("GetThumbFile fail: %{public}s, %{public}s, %{public}s, %{public}d",
        BackupFileUtils::GarbleFilePath(fileInfo.localBigThumbPath, sceneCode).c_str(),
        BackupFileUtils::GarbleFilePath(fileInfo.localThumbPath, sceneCode).c_str(),
        BackupFileUtils::GarbleFilePath(newSrcPath, sceneCode).c_str(),
        fileInfo.localMediaId);
    return "";
}

bool BaseRestore::RestoreLcdAndThumbFromKvdb(const FileInfo &fileInfo, int32_t type, int32_t sceneCode)
{
    return false;
}

bool NeedMoveLocalThumbnail(FileInfo &fileInfo)
{
    return fileInfo.fileType == MediaType::MEDIA_TYPE_IMAGE ||
        !ExifRotateUtils::IsExifRotateWithFlip(fileInfo.exifRotate);
}

void BaseRestore::MoveMigrateFile(std::vector<FileInfo> &fileInfos, int32_t &fileMoveCount, int32_t &videoFileMoveCount,
    int32_t sceneCode)
{
    vector<std::string> moveFailedData;
    int64_t startMoveAndModifyFile = MediaFileUtils::UTCTimeMilliSeconds();
    for (size_t i = 0; i < fileInfos.size(); i++) {
        CHECK_AND_CONTINUE(fileInfos[i].needMove);

        if (IsFileValid(fileInfos[i], sceneCode) != E_OK) {
            fileInfos[i].needVisible = false;
            fileInfos[i].needMove = false;
            MEDIA_ERR_LOG("File is not visible");
            continue;
        }

        if (!MoveAndModifyFile(fileInfos[i], sceneCode)) {
            fileInfos[i].needUpdate = false;
            fileInfos[i].needVisible = false;
            fileInfos[i].needMove = false;
            UpdateFailedFiles(fileInfos[i].fileType, fileInfos[i], RestoreError::MOVE_FAILED);
            ErrorInfo errorInfo(RestoreError::MOVE_FAILED, 1, "",
                BackupLogUtils::FileInfoToString(sceneCode, fileInfos[i]));
            UpgradeRestoreTaskReport().SetSceneCode(this->sceneCode_).SetTaskId(this->taskId_).ReportError(errorInfo);
            moveFailedData.push_back(fileInfos[i].cloudPath);
            continue;
        }

        if (NeedMoveLocalThumbnail(fileInfos[i])) {
            CHECK_AND_EXECUTE(RestoreLcdAndThumbFromCloud(fileInfos[i], MIGRATE_CLOUD_LCD_TYPE, sceneCode),
                RestoreLcdAndThumbFromKvdb(fileInfos[i], MIGRATE_CLOUD_LCD_TYPE, sceneCode));
            CHECK_AND_EXECUTE(RestoreLcdAndThumbFromCloud(fileInfos[i], MIGRATE_CLOUD_THM_TYPE, sceneCode),
                RestoreLcdAndThumbFromKvdb(fileInfos[i], MIGRATE_CLOUD_THM_TYPE, sceneCode));
        }

        MediaLibraryPhotoOperations::StoreThumbnailAndEditSize(to_string(fileInfos[i].fileIdNew),
            fileInfos[i].cloudPath);

        fileMoveCount++;
        videoFileMoveCount += fileInfos[i].fileType == MediaType::MEDIA_TYPE_VIDEO;
    }
    int64_t startDeleteMoveFailedData = MediaFileUtils::UTCTimeMilliSeconds();
    DeleteMoveFailedData(moveFailedData);
    int64_t startSetVisiblePhoto = MediaFileUtils::UTCTimeMilliSeconds();
    SetVisiblePhoto(fileInfos);
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    migrateFileNumber_ += fileMoveCount;
    migrateVideoFileNumber_ += videoFileMoveCount;
    MEDIA_INFO_LOG("TimeCost: MoveAndModifyFile cost: %{public}" PRId64
        ", DeleteMoveFailedData cost: %{public}" PRId64
        ", SetVisiblePhoto cost: %{public}" PRId64, startDeleteMoveFailedData - startMoveAndModifyFile,
        startSetVisiblePhoto - startDeleteMoveFailedData, end - startSetVisiblePhoto);
}

void BaseRestore::MoveMigrateCloudFile(std::vector<FileInfo> &fileInfos, int32_t &fileMoveCount,
    int32_t &videoFileMoveCount, int32_t sceneCode)
{
    MEDIA_INFO_LOG("START STEP 6 MOVE");
    vector<std::string> moveFailedData;
    std::vector<FileInfo> LCDNotFound;
    std::vector<FileInfo> THMNotFound;
    for (size_t i = 0; i < fileInfos.size(); i++) {
        CHECK_AND_CONTINUE(fileInfos[i].needMove);
        bool cond = ((!RestoreLcdAndThumbFromCloud(fileInfos[i], MIGRATE_CLOUD_LCD_TYPE, sceneCode)) &&
            (!RestoreLcdAndThumbFromKvdb(fileInfos[i], MIGRATE_CLOUD_LCD_TYPE, sceneCode)));
        CHECK_AND_EXECUTE(!cond, LCDNotFound.push_back(fileInfos[i]));

        cond = ((!RestoreLcdAndThumbFromCloud(fileInfos[i], MIGRATE_CLOUD_THM_TYPE, sceneCode)) &&
            (!RestoreLcdAndThumbFromKvdb(fileInfos[i], MIGRATE_CLOUD_THM_TYPE, sceneCode)));
        CHECK_AND_EXECUTE(!cond, THMNotFound.push_back(fileInfos[i]));
        videoFileMoveCount += fileInfos[i].fileType == MediaType::MEDIA_TYPE_VIDEO;
    }
    std::vector<std::string> dentryFailedLCD;
    std::vector<std::string> dentryFailedThumb;
    CHECK_AND_EXECUTE(BatchCreateDentryFile(LCDNotFound, dentryFailedLCD, DENTRY_INFO_LCD) != E_OK,
        HandleFailData(fileInfos, dentryFailedLCD, DENTRY_INFO_LCD));
    CHECK_AND_EXECUTE(BatchCreateDentryFile(THMNotFound, dentryFailedThumb, DENTRY_INFO_THM) != E_OK,
        HandleFailData(fileInfos, dentryFailedThumb, DENTRY_INFO_THM));
    fileMoveCount = SetVisiblePhoto(fileInfos);
    successCloudMetaNumber_ += fileMoveCount;
    migrateFileNumber_ += fileMoveCount;
    migrateVideoFileNumber_ += videoFileMoveCount;
    MEDIA_DEBUG_LOG("END STEP 6 MOVE");
}

void BaseRestore::UpdateLcdVisibleColumn(const FileInfo &fileInfo)
{
    NativeRdb::ValuesBucket updatePostBucket;
    updatePostBucket.Put(PhotoColumn::PHOTO_LCD_VISIT_TIME, RESTORE_LCD_VISIT_TIME_SUCCESS);
    std::unique_ptr<NativeRdb::AbsRdbPredicates> predicates =
        make_unique<NativeRdb::AbsRdbPredicates>(PhotoColumn::PHOTOS_TABLE);
    predicates->EqualTo(MediaColumn::MEDIA_NAME, fileInfo.displayName);
    int changeRows = 0;
    int32_t ret = BackupDatabaseUtils::Update(mediaLibraryRdb_, changeRows, updatePostBucket, predicates);
    bool cond = (changeRows < 0 || ret < 0);
    CHECK_AND_PRINT_LOG(!cond, "Failed to update LCD visible column, ret: %{public}d", ret);
}

void BaseRestore::HandleFailData(std::vector<FileInfo> &fileInfos, std::vector<std::string> &failCloudIds,
                                 std::string fileType)
{
    MEDIA_INFO_LOG("START STEP 5 HANDLE FAIL, fileInfos size: %{public}zu", fileInfos.size());
    CHECK_AND_RETURN_INFO_LOG(!failCloudIds.empty(), "HandleFailData failCloudIds empty.");
    vector<std::string> dentryFailedData;
    bool cond = ((fileType != DENTRY_INFO_ORIGIN) && (fileType != DENTRY_INFO_LCD) && (fileType != DENTRY_INFO_THM));
    CHECK_AND_RETURN_LOG(!cond, "Invalid fileType: %{public}s", fileType.c_str());

    auto iteration = fileInfos.begin();
    while (iteration != fileInfos.end()) {
        if (find(failCloudIds.begin(), failCloudIds.end(), iteration->uniqueId) != failCloudIds.end()) {
            dentryFailedData.push_back(iteration->cloudPath);
            iteration->needVisible = false;
            iteration->needMove = false;
            UpdateFailedFiles(iteration->fileType, *iteration, RestoreError::INSERT_FAILED);
            if (fileType == DENTRY_INFO_ORIGIN) {
                iteration = fileInfos.erase(iteration);
            } else if (fileType == DENTRY_INFO_LCD) {
                MediaFileUtils::DeleteFile(iteration->cloudPath);
                ++iteration;
            } else {
                MediaFileUtils::DeleteFile(iteration->cloudPath);

                std::string LCDFilePath = GetThumbnailLocalPath(iteration->cloudPath) + "/LCD.jpg";
                MediaFileUtils::DeleteFile(LCDFilePath);
                ++iteration;
            }
        } else {
            ++iteration;
        }
    }
    DeleteMoveFailedData(dentryFailedData);
    MEDIA_DEBUG_LOG("END STEP 5 HANDLE FAIL");
}

int BaseRestore::InsertPhoto(int32_t sceneCode, std::vector<FileInfo> &fileInfos, int32_t sourceType)
{
    MEDIA_INFO_LOG("Start insert %{public}zu photos", fileInfos.size());
    CHECK_AND_RETURN_RET_LOG(mediaLibraryRdb_ != nullptr, E_OK, "mediaLibraryRdb_ is null");
    CHECK_AND_RETURN_RET_LOG(!fileInfos.empty(), E_OK, "fileInfos are empty");

    int64_t startGenerate = MediaFileUtils::UTCTimeMilliSeconds();
    vector<NativeRdb::ValuesBucket> values = GetInsertValues(sceneCode, fileInfos, sourceType);
    int64_t startInsert = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t rowNum = 0;
    int32_t errCode = BatchInsertWithRetry(PhotoColumn::PHOTOS_TABLE, values, rowNum);
    if (errCode != E_OK) {
        if (needReportFailed_) {
            UpdateFailedFiles(fileInfos, RestoreError::INSERT_FAILED);
            ErrorInfo errorInfo(RestoreError::INSERT_FAILED, static_cast<int32_t>(fileInfos.size()), errCode);
            UpgradeRestoreTaskReport().SetSceneCode(this->sceneCode_).SetTaskId(this->taskId_).ReportError(errorInfo);
        }
        return errCode;
    }

    int64_t startInsertRelated = MediaFileUtils::UTCTimeMilliSeconds();
    InsertPhotoRelated(fileInfos, sourceType);
    BatchQueryPhoto(fileInfos);
    AddToPhotoInfoMap(fileInfos);

    int64_t startMove = MediaFileUtils::UTCTimeMilliSeconds();
    migrateDatabaseNumber_ += rowNum;
    int32_t fileMoveCount = 0;
    int32_t videoFileMoveCount = 0;
    MoveMigrateFile(fileInfos, fileMoveCount, videoFileMoveCount, sceneCode);
    int64_t startRestore = MediaFileUtils::UTCTimeMilliSeconds();
    this->tabOldPhotosRestore_.Restore(this->mediaLibraryRdb_, fileInfos);
    int64_t startUpdate = MediaFileUtils::UTCTimeMilliSeconds();
    UpdatePhotosByFileInfoMap(mediaLibraryRdb_, fileInfos);
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("generate values cost %{public}" PRId64 ", insert %{public}" PRId64 " assets cost %{public}" PRId64
        ", insert photo related cost %{public}" PRId64 ", move %{public}d files (%{public}d + %{public}d) cost "
        "%{public}" PRId64 ", old photos restore cost %{public}" PRId64 ", update cost %{public}" PRId64,
        startInsert - startGenerate, rowNum, startInsertRelated - startInsert,
        startMove - startInsertRelated, fileMoveCount, fileMoveCount - videoFileMoveCount, videoFileMoveCount,
        startRestore - startMove, startUpdate - startRestore, end - startUpdate);

    int32_t mapErrCode = RestoreMapCodeUtils::FileInfosToMapCode(mediaLibraryRdb_, fileInfos);
    MEDIA_INFO_LOG("BaseRestore::InsertPhoto FileInfosToMapCode mapErrCode %{public}d photoRowNum %{public}" PRId64,
        mapErrCode, rowNum);
    return E_OK;
}

int32_t BaseRestore::SetVisiblePhoto(std::vector<FileInfo> &fileInfos)
{
    MEDIA_INFO_LOG("START STEP 7 SET VISIBLE");
    std::vector<std::string> visibleIds;
    for (auto info : fileInfos) {
        if (info.needVisible && info.needMove) {
            visibleIds.push_back(info.cloudPath);
        }
    }
    if (visibleIds.empty()) {
        return 0;
    }

    NativeRdb::ValuesBucket updatePostBucket;
    updatePostBucket.Put(PhotoColumn::PHOTO_SYNC_STATUS, static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE));
    std::unique_ptr<NativeRdb::AbsRdbPredicates> predicates =
        make_unique<NativeRdb::AbsRdbPredicates>(PhotoColumn::PHOTOS_TABLE);
    predicates->In(MediaColumn::MEDIA_FILE_PATH, visibleIds);
    int32_t changeRows = 0;
    int32_t ret = BackupDatabaseUtils::Update(mediaLibraryRdb_, changeRows, updatePostBucket, predicates);
    if (changeRows < 0 || ret < 0) {
        MEDIA_ERR_LOG("Failed to update visible column, changeRows: %{public}d, ret: %{public}d", changeRows, ret);
        changeRows = 0;
    }
    MEDIA_DEBUG_LOG("END STEP 7 SET VISIBLE: visibleIds: %{public}zu, changeRows: %{public}d",
        visibleIds.size(), changeRows);
    return changeRows;
}

int BaseRestore::InsertCloudPhoto(int32_t sceneCode, std::vector<FileInfo> &fileInfos, int32_t sourceType)
{
    MEDIA_INFO_LOG("Start insert cloud %{public}zu photos", fileInfos.size());
    CHECK_AND_RETURN_RET_LOG(mediaLibraryRdb_ != nullptr, E_OK, "mediaLibraryRdb_ is null in cloud clone");
    CHECK_AND_RETURN_RET_LOG(!fileInfos.empty(), E_OK, "fileInfos are empty in cloud clone");

    int64_t startGenerate = MediaFileUtils::UTCTimeMilliSeconds();
    vector<NativeRdb::ValuesBucket> values = GetCloudInsertValues(sceneCode, fileInfos, sourceType);
    int64_t startInsert = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t rowNum = 0;
    int32_t errCode = BatchInsertWithRetry(PhotoColumn::PHOTOS_TABLE, values, rowNum);
    MEDIA_INFO_LOG("the insert result in insertCloudPhoto is %{public}d, the rowNum is %{public}" PRId64,
        errCode, rowNum);
    totalCloudMetaNumber_ += rowNum;
    if (errCode != E_OK) {
        if (needReportFailed_) {
            UpdateFailedFiles(fileInfos, RestoreError::INSERT_FAILED);
            ErrorInfo errorInfo(RestoreError::INSERT_FAILED, static_cast<int32_t>(fileInfos.size()), errCode);
            UpgradeRestoreTaskReport().SetSceneCode(this->sceneCode_).SetTaskId(this->taskId_).ReportError(errorInfo);
        }
        return errCode;
    }

    int64_t startInsertRelated = MediaFileUtils::UTCTimeMilliSeconds();
    InsertPhotoRelated(fileInfos, sourceType);
    BatchQueryPhoto(fileInfos);
    AddToPhotoInfoMap(fileInfos);

    // create dentry file for cloud origin, save failed cloud id
    std::vector<std::string> dentryFailedOrigin;
    CHECK_AND_EXECUTE(BatchCreateDentryFile(fileInfos, dentryFailedOrigin, DENTRY_INFO_ORIGIN) != E_OK,
        HandleFailData(fileInfos, dentryFailedOrigin, DENTRY_INFO_ORIGIN));

    int64_t startMove = MediaFileUtils::UTCTimeMilliSeconds();
    migrateDatabaseNumber_ += rowNum;
    int32_t fileMoveCount = 0;
    int32_t videoFileMoveCount = 0;
    MoveMigrateCloudFile(fileInfos, fileMoveCount, videoFileMoveCount, sceneCode);
    int64_t startRestore = MediaFileUtils::UTCTimeMilliSeconds();
    this->tabOldPhotosRestore_.Restore(this->mediaLibraryRdb_, fileInfos);
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("generate values cost %{public}" PRId64 ", insert %{public}" PRId64 " assets cost %{public}" PRId64
        ", insert photo related cost %{public}" PRId64 ", move %{public}d files (%{public}d + %{public}d) cost "
        "%{public}" PRId64 ", old photos restore cost %{public}" PRId64,
        startInsert - startGenerate, rowNum, startInsertRelated - startInsert,
        startMove - startInsertRelated, fileMoveCount, fileMoveCount - videoFileMoveCount, videoFileMoveCount,
        startRestore - startMove, end - startRestore);
    MEDIA_DEBUG_LOG("END STEP 2 INSERT CLOUD");
    return E_OK;
}

void BaseRestore::DeleteMoveFailedData(std::vector<std::string> &moveFailedData)
{
    CHECK_AND_RETURN_INFO_LOG(!moveFailedData.empty(), "No move failed");
    MEDIA_INFO_LOG("%{public}d file move failed", static_cast<int>(moveFailedData.size()));
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(MediaColumn::MEDIA_FILE_PATH, moveFailedData);
    int32_t changedRows = 0;
    int deleteRes = BackupDatabaseUtils::Delete(predicates, changedRows, mediaLibraryRdb_);
    MEDIA_INFO_LOG("changeRows:%{public}d, deleteRes:%{public}d", changedRows, deleteRes);
}

int32_t BaseRestore::BatchInsertWithRetry(const std::string &tableName, std::vector<NativeRdb::ValuesBucket> &values,
    int64_t &rowNum)
{
    if (values.empty()) {
        return 0;
    }

    int32_t errCode = E_ERR;
    TransactionOperations trans{ __func__ };
    trans.SetBackupRdbStore(mediaLibraryRdb_);
    std::function<int(void)> func = [&]()->int {
        errCode = trans.BatchInsert(rowNum, tableName, values);
        CHECK_AND_PRINT_LOG(errCode == E_OK,
            "InsertSql failed, errCode: %{public}d, rowNum: %{public}ld.", errCode, (long)rowNum);
        return errCode;
    };
    errCode = trans.RetryTrans(func, true);
    CHECK_AND_PRINT_LOG(errCode == E_OK, "BatchInsertWithRetry: tans finish fail!, ret:%{public}d", errCode);
    return errCode;
}

static bool IsTranscodeFile(const std::string &filePath)
{
    std::string displayName = filePath;
    size_t pos = filePath.find_last_of("/");
    if (pos != std::string::npos) {
        displayName = filePath.substr(pos + 1);
    }
    if (displayName == "transcode.jpg" && filePath.find(".editData") != std::string::npos) {
        MEDIA_INFO_LOG("displayname %{private}s, success", displayName.c_str());
        return true;
    }
    return false;
}

int32_t BaseRestore::MoveDirectory(const std::string &srcDir, const std::string &dstDir, bool deleteOriginalFile) const
{
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateDirectory(dstDir), E_FAIL,
        "Create dstDir %{public}s failed", BackupFileUtils::GarbleFilePath(dstDir, DEFAULT_RESTORE_ID).c_str());
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsFileExists(srcDir), E_OK,
        "%{public}s doesn't exist, skip.", srcDir.c_str());
    for (const auto &dirEntry : std::filesystem::directory_iterator{ srcDir }) {
        std::string srcFilePath = dirEntry.path();
        CHECK_AND_CONTINUE(!IsTranscodeFile(srcFilePath));
        std::string tmpFilePath = srcFilePath;
        std::string dstFilePath = tmpFilePath.replace(0, srcDir.length(), dstDir);
        int32_t opRet = E_FAIL;
        if (deleteOriginalFile) {
            opRet = this->MoveFile(srcFilePath, dstFilePath);
        } else {
            opRet = this->CopyFile(srcFilePath, dstFilePath);
        }
        CHECK_AND_RETURN_RET_LOG(opRet == E_OK, E_FAIL,
            "Move file from %{public}s to %{public}s failed, deleteOriginalFile=%{public}d",
            BackupFileUtils::GarbleFilePath(srcFilePath, sceneCode_).c_str(),
            BackupFileUtils::GarbleFilePath(dstFilePath, DEFAULT_RESTORE_ID).c_str(), deleteOriginalFile);
    }
    return E_OK;
}

bool BaseRestore::IsSameAudioFile(const std::shared_ptr<NativeRdb::RdbStore> &rdbStore, const std::string &tableName,
    FileInfo &fileInfo)
{
    string srcPath = fileInfo.filePath;
    string dstPath = BackupFileUtils::GetFullPathByPrefixType(PrefixType::LOCAL, fileInfo.relativePath);
    struct stat srcStatInfo {};
    struct stat dstStatInfo {};

    if (access(dstPath.c_str(), F_OK)) {
        return false;
    }
    if (stat(srcPath.c_str(), &srcStatInfo) != 0) {
        MEDIA_ERR_LOG("Failed to get file %{private}s StatInfo, err=%{public}d", srcPath.c_str(), errno);
        return false;
    }
    if (stat(dstPath.c_str(), &dstStatInfo) != 0) {
        MEDIA_ERR_LOG("Failed to get file %{private}s StatInfo, err=%{public}d", dstPath.c_str(), errno);
        return false;
    }
    if ((srcStatInfo.st_size != dstStatInfo.st_size || srcStatInfo.st_mtime != dstStatInfo.st_mtime) &&
        !HasSameAudioFile(rdbStore, tableName, fileInfo)) { /* file size & last modify time */
        MEDIA_INFO_LOG("Size (%{public}lld -> %{public}lld) or mtime (%{public}lld -> %{public}lld) differs",
            (long long)srcStatInfo.st_size, (long long)dstStatInfo.st_size, (long long)srcStatInfo.st_mtime,
            (long long)dstStatInfo.st_mtime);
        return false;
    }
    fileInfo.isNew = false;
    return true;
}

bool BaseRestore::HasSameAudioFile(const std::shared_ptr<NativeRdb::RdbStore> &rdbStore, const std::string &tableName,
    FileInfo &fileInfo)
{
    string querySql = "SELECT " + MediaColumn::MEDIA_ID + ", " + MediaColumn::MEDIA_FILE_PATH + " FROM " +
        tableName + " WHERE " + MediaColumn::MEDIA_NAME + " = '" + fileInfo.displayName + "' AND " +
        MediaColumn::MEDIA_SIZE + " = " + to_string(fileInfo.fileSize) + " AND " +
        MediaColumn::MEDIA_DATE_MODIFIED + " = " + to_string(fileInfo.dateModified);
    querySql += " LIMIT 1";
    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(rdbStore, querySql);
    bool cond = (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK);
    CHECK_AND_RETURN_RET(resultSet != nullptr, false);
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return false;
    }
    int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
    string cloudPath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
    cond = (fileId <= 0 || cloudPath.empty());
    CHECK_AND_RETURN_RET(!cond, false);
    fileInfo.fileIdNew = fileId;
    fileInfo.cloudPath = cloudPath;
    resultSet->Close();
    return true;
}

void BaseRestore::InsertPhotoMap(std::vector<FileInfo> &fileInfos, int64_t &mapRowNum)
{
    BatchInsertMap(fileInfos, mapRowNum);
    migrateDatabaseMapNumber_ += mapRowNum;
}

void BaseRestore::BatchQueryPhoto(vector<FileInfo> &fileInfos, bool isFull, const NeedQueryMap &needQueryMap)
{
    string querySql = "SELECT " + MediaColumn::MEDIA_ID + " , " + MediaColumn::MEDIA_FILE_PATH + " FROM " +
        PhotoColumn::PHOTOS_TABLE + " WHERE ";
    bool firstSql = false;
    std::vector<std::string> cloudPathArgs;
    for (auto &fileInfo : fileInfos) {
        if (!isFull && !NeedQuery(fileInfo, needQueryMap)) {
            continue;
        }
        if (firstSql) {
            querySql += " OR ";
        } else {
            firstSql = true;
        }
        querySql += MediaColumn::MEDIA_FILE_PATH + " = ? ";
        cloudPathArgs.push_back(fileInfo.cloudPath);
    }
    auto result = BackupDatabaseUtils::GetQueryResultSet(mediaLibraryRdb_, querySql, cloudPathArgs);
    if (result == nullptr) {
        MEDIA_ERR_LOG("Query resultSql is null.");
        return;
    }
    while (result->GoToNextRow() == NativeRdb::E_OK) {
        int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, result);
        std::string path = GetStringVal(MediaColumn::MEDIA_FILE_PATH, result);
        auto pathMatch = [path {path}](const auto &fileInfo) {
            return fileInfo.cloudPath == path;
        };
        auto it = std::find_if(fileInfos.begin(), fileInfos.end(), pathMatch);
        bool cond = (it != fileInfos.end() && fileId > 0);
        CHECK_AND_EXECUTE(!cond, it->fileIdNew = fileId);
    }
}

void BaseRestore::BatchQueryPhoto(std::vector<FileInfo> &fileInfos)
{
    std::stringstream querySql;
    querySql << "SELECT file_id, data FROM Photos WHERE data IN (";
    std::vector<NativeRdb::ValueObject> params;
    int32_t count = 0;
    for (const auto &fileInfo : fileInfos) {
        CHECK_AND_CONTINUE(fileInfo.fileIdNew <= 0);
        querySql << (count++ > 0 ? "," : "");
        querySql << "?";
        params.push_back(fileInfo.cloudPath);
    }
    querySql << ")";
    CHECK_AND_RETURN(!params.empty());
    auto resultSet = BackupDatabaseUtils::QuerySql(mediaLibraryRdb_, querySql.str(), params);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "resultSet is nullptr");

    std::vector<NativeRdb::ValuesBucket> values;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t fileId = GetInt32Val("file_id", resultSet);
        std::string data = GetStringVal("data", resultSet);
        auto it = std::find_if(fileInfos.begin(), fileInfos.end(),
            [data](const FileInfo &info) {
                return info.cloudPath == data;
            });
        CHECK_AND_CONTINUE(it != fileInfos.end());
        it->fileIdNew = fileId;
    }
    resultSet->Close();
}

void BaseRestore::BatchInsertMap(const vector<FileInfo> &fileInfos, int64_t &totalRowNum)
{
    vector<NativeRdb::ValuesBucket> values;
    for (const auto &fileInfo : fileInfos) {
        if (!fileInfo.packageName.empty()) {
            // add for trigger insert_photo_insert_source_album
            continue;
        }

        bool cond = (fileInfo.cloudPath.empty() || fileInfo.mediaAlbumId <= 0 || fileInfo.fileIdNew <= 0);
        CHECK_AND_CONTINUE_ERR_LOG(!cond, "AlbumMap error file name = %{public}s.", fileInfo.displayName.c_str());
        NativeRdb::ValuesBucket value;
        value.PutInt(PhotoMap::ASSET_ID, fileInfo.fileIdNew);
        value.PutInt(PhotoMap::ALBUM_ID, fileInfo.mediaAlbumId);
        values.emplace_back(value);
    }
    int64_t rowNum = 0;
    int32_t errCode = BatchInsertWithRetry(PhotoMap::TABLE, values, rowNum);
    CHECK_AND_PRINT_LOG(errCode == E_OK, "Batch insert map failed, errCode: %{public}d", errCode);
    totalRowNum += rowNum;
}

void BaseRestore::StartRestoreEx(const std::string &backupRetoreDir, const std::string &upgradePath,
    std::string &restoreExInfo)
{
    UpgradeRestoreTaskReport()
        .SetSceneCode(this->sceneCode_)
        .SetTaskId(this->taskId_)
        .ReportProgress("start", std::to_string(MediaFileUtils::UTCTimeSeconds()));
    restoreMode_ = GetRestoreModeFromRestoreInfo(restoreInfo_);
    MEDIA_INFO_LOG("set restore mode to :%{public}d", restoreMode_);
    StartRestore(backupRetoreDir, upgradePath);
    DatabaseReport()
        .SetSceneCode(this->sceneCode_)
        .SetTaskId(this->taskId_)
        .ReportMedia(this->mediaLibraryRdb_, DatabaseReport::PERIOD_AFTER);
    DatabaseReport()
        .SetSceneCode(this->sceneCode_)
        .SetTaskId(this->taskId_)
        .ReportAudio(audioTotalNumber_);
    restoreExInfo = GetRestoreExInfo();
    UpgradeRestoreTaskReport()
        .SetSceneCode(this->sceneCode_)
        .SetTaskId(this->taskId_)
        .ReportTask(restoreExInfo)
        .ReportTotal(std::to_string(errorCode_), GetRestoreTotalInfo())
        .ReportTimeCost(migrateFileNumber_ + audioTotalNumber_, migratePhotoDuplicateNumber_, totalFailCount_)
        .ReportProgress("end", std::to_string(MediaFileUtils::UTCTimeSeconds()))
        .ReportUpgradeEnh(std::to_string(errorCode_), GetUpgradeEnhance());
}

std::string BaseRestore::GetRestoreExInfo()
{
    nlohmann::json restoreExInfoJson;
    restoreExInfoJson[STAT_KEY_RESULT_INFO] = { GetErrorInfoJson(), GetCountInfoJson(STAT_TYPES) };
    return restoreExInfoJson.dump();
}

nlohmann::json BaseRestore::GetErrorInfoJson()
{
    int32_t errorCode = errorCode_ == RestoreError::SUCCESS ? STAT_DEFAULT_ERROR_CODE_SUCCESS :
        STAT_DEFAULT_ERROR_CODE_FAILED;
    nlohmann::json errorInfoJson = {
        { STAT_KEY_TYPE, STAT_VALUE_ERROR_INFO },
        { STAT_KEY_ERROR_CODE, std::to_string(errorCode) },
        { STAT_KEY_ERROR_INFO, errorInfo_ }
    };
    return errorInfoJson;
}

nlohmann::json BaseRestore::GetCountInfoJson(const std::vector<std::string> &countInfoTypes)
{
    nlohmann::json countInfoJson;
    countInfoJson[STAT_KEY_TYPE] = STAT_VALUE_COUNT_INFO;
    size_t limit = MAX_FAILED_FILES_LIMIT;
    for (const auto &type : countInfoTypes) {
        SubCountInfo subCountInfo = GetSubCountInfo(type);
        totalFailCount_ += subCountInfo.failedFiles.size();
        MEDIA_INFO_LOG("SubCountInfo %{public}s success: %{public}lld, duplicate: %{public}lld, failed: %{public}zu",
            type.c_str(), (long long)subCountInfo.successCount, (long long)subCountInfo.duplicateCount,
            subCountInfo.failedFiles.size());
        countInfoJson[STAT_KEY_INFOS].push_back(GetSubCountInfoJson(type, subCountInfo, limit));
    }
    return countInfoJson;
}

SubCountInfo BaseRestore::GetSubCountInfo(const std::string &type)
{
    std::unordered_map<std::string, FailedFileInfo> failedFiles = GetFailedFiles(type);
    if (type == STAT_TYPE_PHOTO) {
        return SubCountInfo(migrateFileNumber_ - migrateVideoFileNumber_, migratePhotoDuplicateNumber_, failedFiles);
    }
    if (type == STAT_TYPE_VIDEO) {
        return SubCountInfo(migrateVideoFileNumber_, migrateVideoDuplicateNumber_, failedFiles);
    }
    return SubCountInfo(migrateAudioFileNumber_, migrateAudioDuplicateNumber_, failedFiles);
}

std::unordered_map<std::string, FailedFileInfo> BaseRestore::GetFailedFiles(const std::string &type)
{
    std::lock_guard<mutex> lock(failedFilesMutex_);
    std::unordered_map<std::string, FailedFileInfo> failedFiles;
    auto iter = failedFilesMap_.find(type);
    if (iter != failedFilesMap_.end()) {
        return iter->second;
    }
    return failedFiles;
}

nlohmann::json BaseRestore::GetSubCountInfoJson(const std::string &type, const SubCountInfo &subCountInfo,
    size_t &limit)
{
    nlohmann::json subCountInfoJson;
    subCountInfoJson[STAT_KEY_BACKUP_INFO] = type;
    subCountInfoJson[STAT_KEY_SUCCESS_COUNT] = subCountInfo.successCount;
    subCountInfoJson[STAT_KEY_DUPLICATE_COUNT] = subCountInfo.duplicateCount;
    subCountInfoJson[STAT_KEY_FAILED_COUNT] = subCountInfo.failedFiles.size();
    // update currentLimit = min(limit, failedFiles.size())
    size_t currentLimit = limit <= subCountInfo.failedFiles.size() ? limit : subCountInfo.failedFiles.size();
    std::string detailsPath;
    std::vector<std::string> failedFilesList;
    if (sceneCode_ == UPGRADE_RESTORE_ID) {
        detailsPath = BackupFileUtils::GetDetailsPath(DEFAULT_RESTORE_ID, type, subCountInfo.failedFiles, currentLimit);
        subCountInfoJson[STAT_KEY_DETAILS] = detailsPath;
    } else {
        failedFilesList = BackupFileUtils::GetFailedFilesList(DEFAULT_RESTORE_ID, subCountInfo.failedFiles,
            currentLimit);
        subCountInfoJson[STAT_KEY_DETAILS] = failedFilesList;
    }
    MEDIA_INFO_LOG("Get %{public}s details size: %{public}zu", type.c_str(), currentLimit);
    limit -= currentLimit; // update total limit
    return subCountInfoJson;
}

std::string BaseRestore::GetBackupInfo()
{
    return "";
}

void BaseRestore::SetErrorCode(int32_t errorCode)
{
    errorCode_ = errorCode;
    errorInfo_ = BackupLogUtils::RestoreErrorToString(errorCode);
}

void BaseRestore::UpdateFailedFileByFileType(int32_t fileType, const FileInfo &fileInfo, int32_t errorCode)
{
    std::lock_guard<mutex> lock(failedFilesMutex_);
    FailedFileInfo failedFileInfo(sceneCode_, fileInfo, errorCode);
    if (fileType == static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE)) {
        failedFilesMap_[STAT_TYPE_PHOTO].emplace(fileInfo.oldPath, failedFileInfo);
        return;
    }
    if (fileType == static_cast<int32_t>(MediaType::MEDIA_TYPE_VIDEO)) {
        failedFilesMap_[STAT_TYPE_VIDEO].emplace(fileInfo.oldPath, failedFileInfo);
        return;
    }
    if (fileType == static_cast<int32_t>(MediaType::MEDIA_TYPE_AUDIO)) {
        failedFilesMap_[STAT_TYPE_AUDIO].emplace(fileInfo.oldPath, failedFileInfo);
        return;
    }
    MEDIA_ERR_LOG("Unsupported file type: %{public}d", fileType);
}

void BaseRestore::UpdateFailedFiles(int32_t fileType, const FileInfo &fileInfo, int32_t errorCode)
{
    SetErrorCode(errorCode);
    UpdateFailedFileByFileType(fileType, fileInfo, errorCode);
}

void BaseRestore::UpdateFailedFiles(const std::vector<FileInfo> &fileInfos, int32_t errorCode)
{
    SetErrorCode(errorCode);
    for (const auto &fileInfo : fileInfos) {
        UpdateFailedFileByFileType(fileInfo.fileType, fileInfo, errorCode);
    }
}

void BaseRestore::UpdateDuplicateNumber(int32_t fileType)
{
    if (fileType == static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE)) {
        migratePhotoDuplicateNumber_++;
        return;
    }
    if (fileType == static_cast<int32_t>(MediaType::MEDIA_TYPE_VIDEO)) {
        migrateVideoDuplicateNumber_++;
        return;
    }
    if (fileType == static_cast<int32_t>(MediaType::MEDIA_TYPE_AUDIO)) {
        migrateAudioDuplicateNumber_++;
        return;
    }
    MEDIA_ERR_LOG("Unsupported file type: %{public}d", fileType);
}

void BaseRestore::SetParameterForClone()
{
    auto currentTime = to_string(MediaFileUtils::UTCTimeSeconds());
    MEDIA_INFO_LOG("SetParameterForClone currentTime:%{public}s", currentTime.c_str());
    bool retFlag = system::SetParameter(CLONE_FLAG, currentTime);
    CHECK_AND_PRINT_LOG(retFlag, "Failed to set parameter cloneFlag, retFlag:%{public}d", retFlag);
}

static void SetMediaLibraryRecoveryDone(bool flag)
{
    int32_t errCode;
    std::shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(MEDIA_LIBRARY_PREF_XML, errCode);
    if (prefs == nullptr) {
        MEDIA_ERR_LOG("get preferences error: %{public}d", errCode);
        return;
    }
    prefs->PutInt(MEDIA_LIBRARY_RECOVERY_FLAG_KEY, flag ? 1 : 0);
    prefs->FlushSync();
    MEDIA_INFO_LOG("SetMediaLibraryRecoveryDone: %{public}d", flag ? 1 : 0);
}

void BaseRestore::StopParameterForClone()
{
    MEDIA_INFO_LOG("StopParameterForClone set 0");
    bool retFlag = system::SetParameter(CLONE_FLAG, "0");
    CHECK_AND_PRINT_LOG(retFlag, "Failed to set stop parameter cloneFlag, retFlag:%{public}d", retFlag);
    SetMediaLibraryRecoveryDone(true);
    MEDIA_INFO_LOG("SetMediaLibraryRecoveryDone set true");
}

void BaseRestore::SetParameterForBackup()
{
    auto currentTime = to_string(MediaFileUtils::UTCTimeSeconds());
    MEDIA_INFO_LOG("SetParameterForBackup currentTime:%{public}s", currentTime.c_str());
    bool retFlag = system::SetParameter(BACKUP_FLAG, currentTime);
    CHECK_AND_PRINT_LOG(retFlag, "Failed to set parameter backupFlag, retFlag:%{public}d", retFlag);
}

void BaseRestore::StopParameterForBackup()
{
    MEDIA_INFO_LOG("StopParameterForBackup set 0");
    bool retFlag = system::SetParameter(BACKUP_FLAG, "0");
    CHECK_AND_PRINT_LOG(retFlag, "Failed to set stop parameter backupFlag, retFlag:%{public}d", retFlag);
}

void BaseRestore::SetParameterForRestore()
{
    auto currentTime = to_string(MediaFileUtils::UTCTimeSeconds());
    MEDIA_INFO_LOG("SetParameterForRestore currentTime:%{public}s", currentTime.c_str());
    bool retFlag = system::SetParameter(RESTORE_FLAG, currentTime);
    CHECK_AND_PRINT_LOG(retFlag, "Failed to set parameter restoreFlag, retFlag:%{public}d", retFlag);
}

void BaseRestore::StopParameterForRestore()
{
    MEDIA_INFO_LOG("StopParameterForRestore set 0");
    bool retFlag = system::SetParameter(RESTORE_FLAG, "0");
    CHECK_AND_PRINT_LOG(retFlag, "Failed to set stop parameter restoreFlag, retFlag:%{public}d", retFlag);
}

void BaseRestore::InsertPhotoRelated(std::vector<FileInfo> &fileInfos, int32_t sourceType)
{
    if (sourceType != SourceType::GALLERY) {
        return;
    }
    NeedQueryMap needQueryMap;
    CHECK_AND_RETURN_INFO_LOG(NeedBatchQueryPhoto(fileInfos, needQueryMap), "There is no need to batch query photo");
    int64_t startQuery = MediaFileUtils::UTCTimeMilliSeconds();
    BatchQueryPhoto(fileInfos, false, needQueryMap);
    int64_t startInsertMap = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t mapRowNum = 0;
    InsertPhotoMap(fileInfos, mapRowNum);
    int64_t startInsertPortrait = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t faceRowNum = 0;
    int64_t portraitMapRowNum = 0;
    int64_t portraitPhotoNum = 0;
    InsertFaceAnalysisData(fileInfos, needQueryMap, faceRowNum, portraitMapRowNum, portraitPhotoNum);
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("TimeCost: query cost %{public}ld, insert %{public}ld maps cost %{public}ld, "
        "insert face analysis data of %{public}ld photos (%{public}ld faces + %{public}ld maps) cost %{public}ld",
        (long)(startInsertMap - startQuery), (long)mapRowNum, (long)(startInsertPortrait - startInsertMap),
        (long)portraitPhotoNum, (long)faceRowNum, (long)portraitMapRowNum, (long)(end - startInsertPortrait));
}

bool BaseRestore::NeedBatchQueryPhoto(const std::vector<FileInfo> &fileInfos, NeedQueryMap &needQueryMap)
{
    return NeedBatchQueryPhotoForPhotoMap(fileInfos, needQueryMap) ||
        NeedBatchQueryPhotoForPortrait(fileInfos, needQueryMap);
}

bool BaseRestore::NeedBatchQueryPhotoForPhotoMap(const std::vector<FileInfo> &fileInfos, NeedQueryMap &needQueryMap)
{
    std::unordered_set<std::string> needQuerySet;
    for (const auto &fileInfo : fileInfos) {
        if (!fileInfo.packageName.empty()) {
            continue;
        }
        if (fileInfo.cloudPath.empty() || fileInfo.mediaAlbumId <= 0) {
            MEDIA_ERR_LOG("Album error file name = %{public}s.", fileInfo.displayName.c_str());
            continue;
        }
        needQuerySet.insert(fileInfo.cloudPath);
    }
    if (needQuerySet.empty()) {
        return false;
    }
    needQueryMap[PhotoRelatedType::PHOTO_MAP] = needQuerySet;
    return true;
}

bool BaseRestore::NeedBatchQueryPhotoForPortrait(const std::vector<FileInfo> &fileInfos, NeedQueryMap &needQueryMap)
{
    return false;
}

bool BaseRestore::NeedQuery(const FileInfo &fileInfo, const NeedQueryMap &needQueryMap)
{
    for (auto iter = needQueryMap.begin(); iter != needQueryMap.end(); ++iter) {
        if (NeedQueryByPhotoRelatedType(fileInfo, iter->first, iter->second)) {
            return true;
        }
    }
    return false;
}

bool BaseRestore::NeedQueryByPhotoRelatedType(const FileInfo &fileInfo, PhotoRelatedType photoRelatedType,
    const std::unordered_set<std::string> &needQuerySet)
{
    std::string searchPath;
    switch (photoRelatedType) {
        case PhotoRelatedType::PHOTO_MAP: {
            searchPath = fileInfo.cloudPath;
            break;
        }
        case PhotoRelatedType::PORTRAIT: {
            searchPath = fileInfo.hashCode;
            break;
        }
        default:
            MEDIA_ERR_LOG("Unsupported photo related type: %{public}d", static_cast<int32_t>(photoRelatedType));
    }
    return !searchPath.empty() && needQuerySet.count(searchPath) > 0;
}

void BaseRestore::InsertFaceAnalysisData(const std::vector<FileInfo> &fileInfos, const NeedQueryMap &needQueryMap,
    int64_t &faceRowNum, int64_t &mapRowNum, int64_t &photoNum)
{
    return;
}

void BaseRestore::ReportPortraitStat(int32_t sceneCode)
{
    MEDIA_INFO_LOG("PortraitStat: album %{public}zu, photo %{public}lld, face %{public}lld, cost %{public}lld",
        portraitAlbumIdMap_.size(), (long long)migratePortraitPhotoNumber_, (long long)migratePortraitFaceNumber_,
        (long long)migratePortraitTotalTimeCost_);
    BackupDfxUtils::PostPortraitStat(static_cast<uint32_t>(portraitAlbumIdMap_.size()), migratePortraitPhotoNumber_,
        migratePortraitFaceNumber_, migratePortraitTotalTimeCost_);
}

int32_t BaseRestore::GetUniqueId(int32_t fileType)
{
    int32_t uniqueId = -1;
    switch (fileType) {
        case MediaType::MEDIA_TYPE_IMAGE: {
            std::lock_guard<ffrt::mutex> lock(imageMutex_);
            uniqueId = static_cast<int32_t>(imageNumber_);
            imageNumber_++;
            break;
        }
        case MediaType::MEDIA_TYPE_VIDEO: {
            std::lock_guard<ffrt::mutex> lock(videoMutex_);
            uniqueId = static_cast<int32_t>(videoNumber_);
            videoNumber_++;
            break;
        }
        case MediaType::MEDIA_TYPE_AUDIO: {
            std::lock_guard<ffrt::mutex> lock(audioMutex_);
            uniqueId = static_cast<int32_t>(audioNumber_);
            audioNumber_++;
            break;
        }
        default:
            MEDIA_ERR_LOG("Unsupported file type: %{public}d", fileType);
    }
    return uniqueId;
}

std::string BaseRestore::GetProgressInfo()
{
    nlohmann::json progressInfoJson;
    for (const auto &type : STAT_PROGRESS_TYPES) {
        SubProcessInfo subProcessInfo = GetSubProcessInfo(type);
        progressInfoJson[STAT_KEY_PROGRESS_INFO].push_back(GetSubProcessInfoJson(type, subProcessInfo));
    }
    std::string progressInfo = progressInfoJson.dump();
    UpgradeRestoreTaskReport()
        .SetSceneCode(this->sceneCode_)
        .SetTaskId(this->taskId_)
        .ReportProgress("onProcess", progressInfo, ongoingTotalNumber_.load())
        .ReportTimeout(ongoingTotalNumber_.load());
    return progressInfo;
}

SubProcessInfo BaseRestore::GetSubProcessInfo(const std::string &type)
{
    uint64_t success = 0;
    uint64_t duplicate = 0;
    uint64_t failed = 0;
    uint64_t total = 0;
    if (type == STAT_TYPE_PHOTO_VIDEO) {
        success = migrateFileNumber_;
        duplicate = migratePhotoDuplicateNumber_ + migrateVideoDuplicateNumber_;
        failed = static_cast<uint64_t>(GetFailedFiles(STAT_TYPE_PHOTO).size() + GetFailedFiles(STAT_TYPE_VIDEO).size());
        total = totalNumber_;
        MEDIA_ERR_LOG("GetSubProcessInfo: %{public}s, migrateFileNumber: %{public}" PRIu64
        ",migratePhotoDuplicateNumber: %{public}" PRIu64 ",migrateVideoDuplicateNumber :%{public}" PRIu64
        "totalNumber :%{public}" PRIu64 ",failed :%{public}" PRIu64, type.c_str(), migrateFileNumber_.load(),
        migratePhotoDuplicateNumber_.load(), migrateVideoDuplicateNumber_.load(), totalNumber_.load(), failed);
    } else if (type == STAT_TYPE_AUDIO) {
        success = migrateAudioFileNumber_;
        duplicate = migrateAudioDuplicateNumber_;
        failed = static_cast<uint64_t>(GetFailedFiles(type).size());
        total = audioTotalNumber_;
    } else if (type == STAT_TYPE_UPDATE) {
        UpdateProcessedNumber(updateProcessStatus_, updateProcessedNumber_, updateTotalNumber_);
        success = updateProcessedNumber_;
        total = updateTotalNumber_;
    } else if (type == STAT_TYPE_THUMBNAIL) {
        success = thumbnailProcessedNumber_;
        total = thumbnailTotalNumber_;
    } else if (type == STAT_TYPE_OTHER) {
        UpdateProcessedNumber(otherProcessStatus_, otherProcessedNumber_, otherTotalNumber_);
        success = otherProcessedNumber_;
        total = otherTotalNumber_;
    } else {
        ongoingTotalNumber_++;
        success = ongoingTotalNumber_;
        total = ongoingTotalNumber_; // make sure progressInfo changes as process goes on
    }
    uint64_t processed = success + duplicate + failed;
    return SubProcessInfo(processed, total);
}

void BaseRestore::UpdateProcessedNumber(const std::atomic<int32_t> &processStatus,
    std::atomic<uint64_t> &processedNumber, const std::atomic<uint64_t> &totalNumber)
{
    if (processStatus == ProcessStatus::STOP) {
        processedNumber = totalNumber.load();
        return;
    }
    processedNumber += processedNumber < totalNumber ? 1 : 0;
}

nlohmann::json BaseRestore::GetSubProcessInfoJson(const std::string &type, const SubProcessInfo &subProcessInfo)
{
    nlohmann::json subProcessInfoJson;
    subProcessInfoJson[STAT_KEY_NAME] = type;
    subProcessInfoJson[STAT_KEY_PROCESSED] = subProcessInfo.processed;
    subProcessInfoJson[STAT_KEY_TOTAL] = subProcessInfo.total;
    subProcessInfoJson[STAT_KEY_IS_PERCENTAGE] = false;
    return subProcessInfoJson;
}

void BaseRestore::UpdateDatabase()
{
    int64_t startUpdateDatabase = MediaFileUtils::UTCTimeMilliSeconds();
    updateProcessStatus_ = ProcessStatus::START;
    GetUpdateTotalCount();
    MEDIA_INFO_LOG("Start update all albums");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    MediaLibraryRdbUtils::UpdateAllAlbums(rdbStore, {}, { NotifyAlbumType::NO_NOTIFY, true,
        AlbumOperationType::DEFAULT, false });
    MEDIA_INFO_LOG("Start update unique number");
    BackupDatabaseUtils::UpdateUniqueNumber(mediaLibraryRdb_, imageNumber_, IMAGE_ASSET_TYPE);
    BackupDatabaseUtils::UpdateUniqueNumber(mediaLibraryRdb_, videoNumber_, VIDEO_ASSET_TYPE);
    BackupDatabaseUtils::UpdateUniqueNumber(mediaLibraryRdb_, audioNumber_, AUDIO_ASSET_TYPE);
    MEDIA_INFO_LOG("Start notify");
    NotifyAlbum();
    updateProcessStatus_ = ProcessStatus::STOP;
    int64_t endUpdateDatabase = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("TimeCost: UpdateDatabase cost: %{public}" PRId64, endUpdateDatabase - startUpdateDatabase);
}

void BaseRestore::UpdatePhotoAlbumDateModified(const vector<string> &albumIds, const string &tableName)
{
    CHECK_AND_RETURN(!albumIds.empty());
    CHECK_AND_RETURN(tableName == PhotoAlbumColumns::TABLE);
    MEDIA_INFO_LOG("Start update photoAlbums date modified tableName:%{public}s, albumIds count:%{public}zu",
        tableName.c_str(), albumIds.size());
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    MediaLibraryRdbUtils::UpdateUserAlbumInternal(rdbStore, albumIds, false, true);
    MediaLibraryRdbUtils::UpdateSourceAlbumInternal(rdbStore, albumIds, false, true);
}

void BaseRestore::NotifyAlbum()
{
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_LOG(watch != nullptr, "Can not get MediaLibraryNotify Instance");

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::NOTIFY_FOR_RECHECK);
    int ret = IPC::UserDefineIPCClient().Call(businessCode);
    CHECK_AND_PRINT_LOG(ret == E_OK, "Notify check refresh failed");

    watch->Notify(PhotoColumn::DEFAULT_PHOTO_URI, NotifyType::NOTIFY_ADD);
    watch->Notify(PhotoAlbumColumns::ALBUM_URI_PREFIX, NotifyType::NOTIFY_ADD);
}

void BaseRestore::GetUpdateTotalCount()
{
    GetUpdateAllAlbumsCount();
    GetUpdateUniqueNumberCount();
}

void BaseRestore::GetUpdateAllAlbumsCount()
{
    const std::vector<std::string> ALBUM_TABLE_LIST = { "PhotoAlbum", "AnalysisAlbum" };
    int32_t albumTotalCount = 0;
    for (const auto &tableName : ALBUM_TABLE_LIST) {
        std::string querySql = "SELECT count(1) as count FROM " + tableName;
        albumTotalCount += BackupDatabaseUtils::QueryInt(mediaLibraryRdb_, querySql, CUSTOM_COUNT);
    }
    updateTotalNumber_ += static_cast<uint64_t>(albumTotalCount);
    MEDIA_INFO_LOG("onProcess Update updateTotalNumber_: %{public}lld", (long long)updateTotalNumber_);
}

void BaseRestore::GetUpdateUniqueNumberCount()
{
    updateTotalNumber_ += UNIQUE_NUMBER_NUM;
    MEDIA_INFO_LOG("onProcess Update updateTotalNumber_: %{public}lld", (long long)updateTotalNumber_);
}

uint64_t BaseRestore::GetNotFoundNumber()
{
    return notFoundNumber_;
}

void BaseRestore::RestoreThumbnail()
{
    // restore thumbnail for date fronted 2000 photos
    int32_t localNoAstcCount = BackupDatabaseUtils::QueryLocalNoAstcCount(mediaLibraryRdb_);
    CHECK_AND_RETURN_LOG(localNoAstcCount > 0,
        "No need to RestoreThumbnail, localNoAstcCount:%{public}d", localNoAstcCount);
    int32_t restoreAstcCount = localNoAstcCount < MAX_RESTORE_ASTC_NUM ? localNoAstcCount : MAX_RESTORE_ASTC_NUM;
    uint64_t thumbnailTotalNumber = static_cast<uint64_t>(restoreAstcCount);
    thumbnailTotalNumber_ = thumbnailTotalNumber;
    int32_t readyAstcCount = BackupDatabaseUtils::QueryReadyAstcCount(mediaLibraryRdb_);
    CHECK_AND_RETURN_LOG(readyAstcCount >= 0, "ReadyAstcCount:%{public}d is invalid", readyAstcCount);
    uint64_t waitAstcNum = sceneCode_ != UPGRADE_RESTORE_ID ? thumbnailTotalNumber :
        std::min(thumbnailTotalNumber, MAX_UPGRADE_WAIT_ASTC_NUM);

    MEDIA_INFO_LOG("Start RestoreThumbnail, readyAstcCount:%{public}d, restoreAstcCount:%{public}d, "
        "waitAstcNum:%{public}" PRIu64, readyAstcCount, restoreAstcCount, waitAstcNum);
    BackupFileUtils::GenerateThumbnailsAfterRestore(restoreAstcCount);
    uint64_t thumbnailProcessedNumber = 0;
    int32_t timeoutTimes = 0;
    int64_t startRestoreThumbnailTime = MediaFileUtils::UTCTimeMilliSeconds();
    bool isNeedWaitRestoreThumbnail = true;
    while (thumbnailProcessedNumber < waitAstcNum && isNeedWaitRestoreThumbnail) {
        thumbnailProcessedNumber_ = thumbnailProcessedNumber;
        std::this_thread::sleep_for(std::chrono::milliseconds(THUMBNAIL_QUERY_INTERVAL));
        int32_t newReadyAstcCount = BackupDatabaseUtils::QueryReadyAstcCount(mediaLibraryRdb_);
        CHECK_AND_RETURN_LOG(newReadyAstcCount >= 0, "NewReadyAstcCount:%{public}d is invalid", newReadyAstcCount);
        if (newReadyAstcCount <= readyAstcCount) {
            MEDIA_WARN_LOG("Astc is not added, oldReadyAstcCount:%{public}d, newReadyAstcCount:%{public}d, "
                "timeoutTimes:%{public}d", readyAstcCount, newReadyAstcCount, timeoutTimes);
            readyAstcCount = newReadyAstcCount;
            ++timeoutTimes;
            isNeedWaitRestoreThumbnail = timeoutTimes <= MAX_RESTORE_THUMBNAIL_TIMEOUT_TIMES ||
                MediaFileUtils::UTCTimeMilliSeconds() - startRestoreThumbnailTime <= MIN_RESTORE_THUMBNAIL_TIME;
            continue;
        }
        thumbnailProcessedNumber += static_cast<uint64_t>(newReadyAstcCount - readyAstcCount);
        readyAstcCount = newReadyAstcCount;
        timeoutTimes = 0;
        isNeedWaitRestoreThumbnail = true;
    }
    thumbnailProcessedNumber_ = thumbnailProcessedNumber < thumbnailTotalNumber ?
        thumbnailProcessedNumber : thumbnailTotalNumber;
    MEDIA_INFO_LOG("Finish RestoreThumbnail, readyAstcCount:%{public}d, restoreAstcCount:%{public}d",
        readyAstcCount, restoreAstcCount);
}

void BaseRestore::StartBackup()
{}

std::string BaseRestore::CheckInvalidFile(const FileInfo &fileInfo, int32_t errCode)
{
    return "";
}

std::string BaseRestore::GetRestoreTotalInfo()
{
    std::stringstream restoreTotalInfo;
    uint64_t failed = 0;
    uint64_t error = 0;
    SetRestoreFailedAndErrorCount(failed, error);
    restoreTotalInfo << failed;
    restoreTotalInfo << ";" << error;
    restoreTotalInfo << ";" << GetNoNeedMigrateCount();
    return restoreTotalInfo.str();
}

void BaseRestore::SetRestoreFailedAndErrorCount(uint64_t &failed, uint64_t &error)
{
    uint64_t success = migrateFileNumber_;
    uint64_t duplicate = migratePhotoDuplicateNumber_ + migrateVideoDuplicateNumber_;
    failed = static_cast<uint64_t>(GetFailedFiles(STAT_TYPE_PHOTO).size() +
        GetFailedFiles(STAT_TYPE_VIDEO).size());
    error = totalNumber_ - success - duplicate - failed - notFoundNumber_;
}

int32_t BaseRestore::GetNoNeedMigrateCount()
{
    return 0;
}

bool BaseRestore::ExtraCheckForCloneSameFile(FileInfo &fileInfo, PhotosDao::PhotosRowData &rowData)
{
    fileInfo.isNew = false;
    fileInfo.fileIdNew = rowData.fileId;
    fileInfo.cloudPath = rowData.data;
    bool isInCloud = rowData.cleanFlag == 1 && rowData.position == static_cast<int32_t>(PhotoPositionType::CLOUD);
    // If the file was in cloud previously with clean_flag = 1, reset it's position to local
    if (rowData.fileId > 0 && isInCloud) {
        fileInfo.needUpdate = true;
        return false;
    }
    fileInfo.needMove = false;
    return true;
}

void BaseRestore::UpdatePhotosByFileInfoMap(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
    std::vector<FileInfo>& fileInfos)
{
    std::vector<std::string> inColumn;
    for (FileInfo &fileInfo : fileInfos) {
        if (fileInfo.fileIdNew <= 0 || fileInfo.isNew || !fileInfo.needUpdate) {
            continue;
        }
        if (MediaFileUtils::IsFileExists(fileInfo.cloudPath)) {
            int32_t ret = RemoveDentryFileWithConflict(fileInfo);
            if (ret != E_OK) {
                fileInfo.needUpdate = false;
                MEDIA_ERR_LOG("Failed to remove dentry file %{public}s", fileInfo.cloudPath.c_str());
                continue;
            }
            inColumn.push_back(to_string(fileInfo.fileIdNew));
        }
    }
    BackupDatabaseUtils::BatchUpdatePhotosToLocal(mediaLibraryRdb, inColumn);
}

int32_t BaseRestore::RemoveDentryFileWithConflict(const FileInfo &fileInfo)
{
    // Delete dentry file
    string dummyPath = fileInfo.cloudPath + ".tmp";
    int32_t moveToRet = MoveFile(fileInfo.cloudPath, dummyPath);
    CHECK_AND_RETURN_RET_LOG(moveToRet == E_OK, moveToRet,
        "Move file from %{public}s to %{public}s failed, ret=%{public}d", fileInfo.cloudPath.c_str(),
        dummyPath.c_str(), moveToRet);

    bool dentryDeleted = false;
    if (MediaFileUtils::IsFileExists(fileInfo.cloudPath)) {
        (void)MediaFileUtils::DeleteFile(fileInfo.cloudPath);
        dentryDeleted = true;
    }

    int32_t moveBackRet = MoveFile(dummyPath, fileInfo.cloudPath);
    CHECK_AND_RETURN_RET_LOG(moveBackRet == E_OK, moveBackRet,
        "Move file from %{public}s to %{public}s failed, ret=%{public}d", dummyPath.c_str(),
        fileInfo.cloudPath.c_str(), moveBackRet);

    // Delete thumb dentry file
    string tmpPathThumb = fileInfo.cloudPath;
    string thumbPath = tmpPathThumb.replace(0, RESTORE_CLOUD_DIR.length(), RESTORE_THUMB_CLOUD_DIR);
    bool cond = (MediaFileUtils::IsFileExists(thumbPath) && dentryDeleted);
    CHECK_AND_EXECUTE(!cond, (void)MediaFileUtils::DeleteDir(thumbPath));
    return E_OK;
}

void BaseRestore::AddToPhotoInfoMap(std::vector<FileInfo> &fileInfos)
{
    std::lock_guard<ffrt::mutex> lock(photoInfoMutex_);
    for (auto fileInfo: fileInfos) {
        PhotoInfo photoInfo;
        photoInfo.fileIdNew = fileInfo.fileIdNew;
        photoInfo.fileType = fileInfo.fileType;
        photoInfo.displayName = fileInfo.displayName;
        photoInfo.cloudPath = fileInfo.cloudPath;
        photoInfoMap_.insert(std::make_pair(fileInfo.fileIdOld, photoInfo));
    }
}

std::string BaseRestore::GetUpgradeEnhance()
{
    std::stringstream upgradeEnhance;
    upgradeEnhance << localLcdCount_;
    upgradeEnhance << "," << localThumbnailCount_;
    upgradeEnhance << "," << cloudLcdCount_;
    upgradeEnhance << "," << cloudThumbnailCount_;
    upgradeEnhance << "," << totalCloudMetaNumber_;
    upgradeEnhance << "," << successCloudMetaNumber_;
    upgradeEnhance << "," << rotateLcdMigrateFileNumber_;
    upgradeEnhance << "," << rotateThmMigrateFileNumber_;
    return upgradeEnhance.str();
}

bool BaseRestore::IsCloudRestoreSatisfied()
{
    return isAccountValid_ && isSyncSwitchOn_;
}

void BaseRestore::ProcessBurstPhotos()
{
    int64_t startProcess = MediaFileUtils::UTCTimeMilliSeconds();
    BackupDatabaseUtils::UpdateBurstPhotos(mediaLibraryRdb_);
    int64_t endProcess = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("process burst photos end, cost: %{public}" PRId64, endProcess - startProcess);
}

void BaseRestore::SetOrientationAndExifRotate(FileInfo &info, NativeRdb::ValuesBucket &value,
    std::unique_ptr<Metadata> &data)
{
    CHECK_AND_RETURN_LOG(data != nullptr, "Metadata is nullptr");
    info.orientation = data->GetOrientation();
    value.PutInt(PhotoColumn::PHOTO_ORIENTATION, info.orientation);
    info.exifRotate = data->GetExifRotate();
    value.PutInt(PhotoColumn::PHOTO_EXIF_ROTATE, info.exifRotate);
}

bool BaseRestore::HasExThumbnail(const FileInfo &info)
{
    CHECK_AND_RETURN_RET(info.localMediaId != -1, BackupFileUtils::HasOrientationOrExifRotate(info));
    return info.fileType == MediaType::MEDIA_TYPE_IMAGE && BackupFileUtils::HasOrientationOrExifRotate(info);
}

inline int64_t GetSouthDeviceSwithStatusTimestamp()
{
    constexpr int64_t defaultValueTime = 0;
    return system::GetIntParameter(CLOUDSYNC_SWITCH_STATUS_KEY.c_str(), defaultValueTime);
}

bool BaseRestore::WaitSouthDeviceExitTimeout()
{
    constexpr int64_t defaultValueTime = 0;
    int64_t startTimeClean = GetSouthDeviceSwithStatusTimestamp();
    int64_t startTimeWait = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("Wait for the south device to exit. startTimeWait: %{public}" PRId64
        ", startTimeClean: %{public}" PRId64, startTimeWait, startTimeClean);
    while (startTimeClean > 0) {
        auto nowTime = MediaFileUtils::UTCTimeMilliSeconds();
        int64_t waitTimeout = RESTORE_OR_BACKUP_WAIT_FORCE_RETAIN_CLOUD_MEDIA_TIMEOUT_MILLISECOND; // unit: ms
        if ((nowTime - startTimeWait) > waitTimeout) {
            MEDIA_WARN_LOG("[Restore or Backup] timeout: now: %{public}" PRId64", startTimeWait: %{public}" PRId64
                ", startTimeClean: %{public}" PRId64, nowTime, startTimeWait, startTimeClean);
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(
            RESTORE_OR_BACKUP_WAIT_FORCE_RETAIN_CLOUD_MEDIA_SLEEP_TIME_MILLISECOND));
        startTimeClean = GetSouthDeviceSwithStatusTimestamp();
        MEDIA_DEBUG_LOG("[Restore or Backup] waiting: now: %{public}" PRId64", startTimeWait: %{public}" PRId64
            ", startTimeClean: %{public}" PRId64, nowTime, startTimeWait, startTimeClean);
    }
    MEDIA_INFO_LOG("the south device has exited. exitTime: %{public}" PRId64, MediaFileUtils::UTCTimeMilliSeconds());
    return false;
}

void BaseRestore::StartBackupEx(std::string& backupExInfo)
{
    StartBackup();
    backupExInfo = GetBackupExInfo();
}

std::string BaseRestore::GetBackupExInfo()
{
    return GetBackupErrorInfoJson().dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
}

nlohmann::json BaseRestore::GetBackupErrorInfoJson()
{
    int32_t errorCode = errorCode_ == RestoreError::SUCCESS ? STAT_DEFAULT_ERROR_CODE_SUCCESS :
        STAT_DEFAULT_ERROR_CODE_FAILED;
    nlohmann::json errorInfoJson = {
        { STAT_KEY_TYPE, STAT_VALUE_ERROR_INFO },
        { STAT_KEY_ERROR_CODE, std::to_string(errorCode) },
        { STAT_KEY_ERROR_INFO, errorInfo_ },
        { STAT_KEY_COMPATIBLE_DIR_MAPPING, dirMappingList_ }
    };
    return errorInfoJson;
}

void BaseRestore::Release(ReleaseScene releaseScene)
{
    if (releaseScene == ReleaseScene::BACKUP) {
        BackupRelease();
    } else if (releaseScene == ReleaseScene::RESTORE) {
        RestoreRelease();
    } else {
        MEDIA_ERR_LOG("Invalid ReleaseScene: %{public}d", static_cast<int>(releaseScene));
    }
}

void BaseRestore::BackupRelease()
{
}

void BaseRestore::RestoreRelease()
{
}

void BaseRestore::RestoreSearchIndex()
{
    int64_t doIndexStartTime = MediaFileUtils::UTCTimeMilliSeconds();
    std::vector<std::string> fileIds;
    MediaAnalysisHelper::StartMediaAnalysisServiceSync(
        IMediaAnalysisService::ActivateServiceType::START_FOREGROUND_INDEX_FULL, fileIds);
    int64_t doIndexEndTime = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("TimeCost: doIndex cost: %{public}" PRId64, doIndexEndTime - doIndexStartTime);
}

void BaseRestore::SetIsRestore(bool isRestore)
{
    isRestore_ = isRestore;
}
} // namespace Media
} // namespace OHOS
