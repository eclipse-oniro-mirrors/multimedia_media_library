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

#include "cloud_sync_notify_handler.h"

#include <sys/stat.h>

#include "cloud_media_asset_manager.h"
#include "cloud_media_asset_types.h"
#include "cloud_sync_utils.h"
#include "medialibrary_album_fusion_utils.h"
#include "medialibrary_album_operations.h"
#include "notify_responsibility_chain_factory.h"
#include "result_set_utils.h"
#include "thumbnail_service.h"
#include "medialibrary_notify.h"
#include "medialibrary_photo_operations.h"
#include "medialibrary_rdb_utils.h"
#include "parameters.h"
#include "photo_album_column.h"
#include "media_log.h"

using namespace std;

namespace OHOS {
namespace Media {
using ChangeType = DataShare::DataShareObserver::ChangeType;

const std::string INVALID_ZERO_ID = "0";

static bool IsCloudInsertTaskPriorityHigh()
{
    int32_t cloudSyncStatus = static_cast<int32_t>(system::GetParameter(CLOUDSYNC_STATUS_KEY, "0").at(0) - '0');
    return cloudSyncStatus == CloudSyncStatus::FIRST_FIVE_HUNDRED ||
        cloudSyncStatus == CloudSyncStatus::INCREMENT_DOWNLOAD;
}

static inline bool IsCloudNotifyInfoValid(const string& cloudNotifyInfo)
{
    if (cloudNotifyInfo.empty()) {
        return false;
    }
    for (char const& ch : cloudNotifyInfo) {
        if (isdigit(ch) == 0) {
            return false;
        }
    }
    return true;
}

static void UpdateCloudAssetDownloadTask(const bool verifyFlag)
{
    CHECK_AND_RETURN_LOG(verifyFlag, "Current status is not suitable for task.");
    if (!CloudMediaAssetManager::GetInstance().SetIsThumbnailUpdate() && CloudSyncUtils::IsCloudSyncSwitchOn() &&
        CloudSyncUtils::IsCloudDataAgingPolicyOn()) {
        CloudMediaAssetManager::GetInstance().StartDownloadCloudAsset(CloudMediaDownloadType::DOWNLOAD_GENTLE);
    }
}

void CloudSyncNotifyHandler::HandleInsertEvent(const std::list<Uri> &uris)
{
    bool isCloudInsertTaskPriorityHigh = IsCloudInsertTaskPriorityHigh();
    if (!isCloudInsertTaskPriorityHigh && !ThumbnailService::GetInstance()->GetCurrentStatusForTask()) {
        MEDIA_INFO_LOG("current status is not suitable for task");
        return;
    }
    bool verifyFlag = false;
    for (auto &uri : uris) {
        string uriString = uri.ToString();
        auto pos = uriString.find_last_of('/');
        if (pos == string::npos) {
            continue;
        }
        string idString = uriString.substr(pos + 1);
        if (idString.compare(INVALID_ZERO_ID) == 0 || !IsCloudNotifyInfoValid(idString)) {
            MEDIA_WARN_LOG("cloud observer get no valid fileId and uri : %{public}s", uriString.c_str());
            continue;
        }
        if (!verifyFlag) {
            verifyFlag = true;
        }
        ThumbnailService::GetInstance()->CreateAstcCloudDownload(idString, isCloudInsertTaskPriorityHigh);
    }
    UpdateCloudAssetDownloadTask(verifyFlag);
}

void CloudSyncNotifyHandler::HandleDeleteEvent(const std::list<Uri> &uris)
{
    bool verifyFlag = false;
    for (auto &uri : uris) {
        string uriString = uri.ToString();
        auto dateTakenPos = uriString.rfind('/');
        if (dateTakenPos == string::npos) {
            continue;
        }
        auto fileIdPos = uriString.rfind('/', dateTakenPos - 1);
        if (fileIdPos == string::npos) {
            continue;
        }

        string dateTaken = uriString.substr(dateTakenPos + 1);
        string fileId = uriString.substr(fileIdPos + 1, dateTakenPos - fileIdPos - 1);
        if (!IsCloudNotifyInfoValid(dateTaken) || !IsCloudNotifyInfoValid(fileId)) {
            MEDIA_WARN_LOG("cloud observer get no valid uri : %{public}s", uriString.c_str());
            continue;
        }
        if (!verifyFlag) {
            verifyFlag = true;
        }

        ThumbnailService::GetInstance()->DeleteAstcWithFileIdAndDateTaken(fileId, dateTaken);
        MediaLibraryPhotoOperations::HasDroppedThumbnailSize(fileId);
    }
    if (verifyFlag) {
        CloudMediaAssetManager::GetInstance().SetIsThumbnailUpdate();
    }
}

void CloudSyncNotifyHandler::HandleTimeUpdateEvent(const std::list<Uri> &uris)
{
    for (auto &uri : uris) {
        string uriString = uri.ToString();
        auto newDateTakenPos = uriString.rfind('/');
        if (newDateTakenPos == string::npos) {
            continue;
        }
        auto formerDateTakenPos = uriString.rfind('/', newDateTakenPos - 1);
        if (formerDateTakenPos == string::npos) {
            continue;
        }
        auto fileIdPos = uriString.rfind('/', formerDateTakenPos - 1);
        if (fileIdPos == string::npos) {
            continue;
        }

        string newDateTaken = uriString.substr(newDateTakenPos + 1);
        string formerDateTaken = uriString.substr(formerDateTakenPos + 1, newDateTakenPos - formerDateTakenPos - 1);
        string fileId = uriString.substr(fileIdPos + 1, formerDateTakenPos - fileIdPos - 1);
        if (!IsCloudNotifyInfoValid(newDateTaken) || !IsCloudNotifyInfoValid(formerDateTaken) ||
            !IsCloudNotifyInfoValid(fileId)) {
            MEDIA_WARN_LOG("cloud observer get no valid uri : %{public}s", uriString.c_str());
            continue;
        }

        ThumbnailService::GetInstance()->UpdateAstcWithNewDateTaken(fileId, newDateTaken, formerDateTaken);
    }
}

void CloudSyncNotifyHandler::HandleExtraEvent(const std::list<Uri> &uris, const ChangeType &type)
{
    ExtraChangeType extraType = static_cast<ExtraChangeType>(type);
    if (extraType == ExtraChangeType::PHOTO_TIME_UPDATE) {
        HandleTimeUpdateEvent(uris);
        return;
    }
    MEDIA_DEBUG_LOG("change type is %{public}d, no need ThumbnailObserverOnChange", type);
}

void CloudSyncNotifyHandler::ThumbnailObserverOnChange(const list<Uri> &uris, const ChangeType &type)
{
    MediaLibraryRdbUtils::SetNeedRefreshAlbum(true);
    switch (type) {
        case ChangeType::INSERT:
            HandleInsertEvent(uris);
            break;
        case ChangeType::DELETE:
            HandleDeleteEvent(uris);
            break;
        default:
            HandleExtraEvent(uris, type);
            break;
    }
}

void CloudSyncNotifyHandler::HandleDirtyDataFix(const std::list<Uri> &uris, const CloudSyncErrType &errType)
{
    MediaLibraryRdbUtils::SetNeedRefreshAlbum(true);
    switch (errType) {
        case CloudSyncErrType::CONTENT_NOT_FOUND:
            HandleContentNotFound(uris);
            break;
        case CloudSyncErrType::THM_NOT_FOUND:
            HandleThumbnailNotFound(uris);
            break;
        case CloudSyncErrType::LCD_NOT_FOUND:
            HandleLCDNotFound(uris);
            break;
        case CloudSyncErrType::LCD_SIZE_IS_TOO_LARGE:
            HandleLCDSizeTooLarge(uris);
            break;
        case CloudSyncErrType::CONTENT_SIZE_IS_ZERO:
            HandleContentSizeIsZero(uris);
            break;
        case CloudSyncErrType::ALBUM_NOT_FOUND:
            HandleAlbumNotFound(uris);
            break;
        default:
            MEDIA_ERR_LOG("HandleDirtyDataFix, Unrecognized error type : %{public}d", errType);
        }
}

std::string CloudSyncNotifyHandler::GetfileIdFromPastDirtyDataFixUri(std::string uriString)
{
    auto fileIdPos = uriString.rfind('/');
    if (fileIdPos == string::npos) {
        return "";
    }
    std::string fileId = uriString.substr(fileIdPos + 1, uriString.size() - fileIdPos);
    return fileId;
}

void CloudSyncNotifyHandler::HandleContentNotFound(const std::list<Uri> &uris)
{
    for (auto &uri : uris) {
        std::string uriString = uri.ToString();
        std::string fileId = GetfileIdFromPastDirtyDataFixUri(uriString);
        if (fileId == "") {
            continue;
        }
        if (fileId.compare(INVALID_ZERO_ID) == 0 || !IsCloudNotifyInfoValid(fileId)) {
            MEDIA_WARN_LOG("cloud observer get no valid uri : %{public}s", uriString.c_str());
            continue;
        }
        MEDIA_INFO_LOG(
            "ContentNotFound, uri : %{public}s", uriString.c_str());
    }
}

void CloudSyncNotifyHandler::HandleThumbnailNotFound(const std::list<Uri> &uris)
{
    for (auto &uri : uris) {
        std::string uriString = uri.ToString();
        std::string fileId = GetfileIdFromPastDirtyDataFixUri(uriString);
        if (fileId == "") {
            continue;
        }
        if (fileId.compare(INVALID_ZERO_ID) == 0 || !IsCloudNotifyInfoValid(fileId)) {
            MEDIA_WARN_LOG("cloud observer get no valid uri : %{public}s", uriString.c_str());
            continue;
        }

        int32_t err = ThumbnailService::GetInstance()->CreateThumbnailPastDirtyDataFix(fileId);
        if (err != E_SUCCESS) {
            MEDIA_ERR_LOG("ThumbnailService CreateThumbnailPastDirtyDataFix failed : %{public}d", err);
            continue;
        }
        MEDIA_INFO_LOG("Generate thumbnail %{public}s, success ", uriString.c_str());
    }
}

void CloudSyncNotifyHandler::HandleLCDNotFound(const std::list<Uri> &uris)
{
    for (auto &uri : uris) {
        std::string uriString = uri.ToString();
        std::string fileId = GetfileIdFromPastDirtyDataFixUri(uriString);
        if (fileId == "") {
            continue;
        }
        if (fileId.compare(INVALID_ZERO_ID) == 0 || !IsCloudNotifyInfoValid(fileId)) {
            MEDIA_WARN_LOG("cloud observer get no valid uri : %{public}s", uriString.c_str());
            continue;
        }

        int32_t err = ThumbnailService::GetInstance()->CreateLcdPastDirtyDataFix(fileId);
        if (err != E_SUCCESS) {
            MEDIA_ERR_LOG("ThumbnailService CreateLCDPastDirtyDataFix failed : %{public}d", err);
            continue;
        }
        MEDIA_INFO_LOG("Generate Lcd %{public}s, success ", uriString.c_str());
    }
    return;
}

void CloudSyncNotifyHandler::HandleLCDSizeTooLarge(const std::list<Uri> &uris)
{
    for (auto &uri : uris) {
        std::string uriString = uri.ToString();
        std::string fileId = GetfileIdFromPastDirtyDataFixUri(uriString);
        if (fileId == "") {
            continue;
        }
        if (fileId.compare(INVALID_ZERO_ID) == 0 || !IsCloudNotifyInfoValid(fileId)) {
            MEDIA_WARN_LOG("cloud observer get no valid uri : %{public}s", uriString.c_str());
            continue;
        }

        int32_t err = ThumbnailService::GetInstance()->CreateLcdPastDirtyDataFix(fileId, THUMBNAIL_EIGHTY);
        if (err != E_SUCCESS) {
            MEDIA_ERR_LOG("ThumbnailService CreateLcdPastDirtyDataFix to eighty quality failed : %{public}d", err);
            continue;
        }
        MEDIA_INFO_LOG("Regenerate Lcd %{public}s to eighty quality, success ", uriString.c_str());
    }
    return;
}

void CloudSyncNotifyHandler::HandleContentSizeIsZero(const std::list<Uri> &uris)
{
    for (auto &uri : uris) {
        std::string uriString = uri.ToString();
        std::string fileId = GetfileIdFromPastDirtyDataFixUri(uriString);
        if (fileId == "") {
            continue;
        }
        if (fileId.compare(INVALID_ZERO_ID) == 0 || !IsCloudNotifyInfoValid(fileId)) {
            MEDIA_WARN_LOG("cloud observer get no valid uri : %{public}s", uriString.c_str());
            continue;
        }

        std::string filePath;
        auto err = QueryFilePathFromFileId(fileId, filePath);
        if (err != E_SUCCESS) {
            MEDIA_ERR_LOG("QueryFilePathFromFileId failed : %{public}d", err);
            continue;
        }
        auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
        int changeRows = 0;
        struct stat st;
        err = stat(filePath.c_str(), &st);
        if (err != E_SUCCESS) {
            MEDIA_ERR_LOG("stat failed : %{public}d", err);
            continue;
        }
        NativeRdb::ValuesBucket valuesNew;
        valuesNew.PutLong(PhotoColumn::MEDIA_SIZE, st.st_size);
        NativeRdb::RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
        rdbPredicates.EqualTo(PhotoColumn::MEDIA_ID, fileId);
        rdbStore->Update(changeRows, valuesNew, rdbPredicates);
        CHECK_AND_PRINT_LOG(changeRows >= 0, "Failed to update content size , ret = %{public}d", changeRows);
        MEDIA_INFO_LOG("refresh photo size field to : %{public}d , success", static_cast<int>(st.st_size));
    }
    return;
}

int32_t CloudSyncNotifyHandler::QueryFilePathFromFileId(const std::string &id, std::string &filePath)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_DB_FAIL, "QueryFilePathFromFileId failed. rdbStore is null");
    const string sqlQuery = "SELECT * From " + PhotoColumn::PHOTOS_TABLE +
                            " WHERE " + PhotoColumn::MEDIA_ID + " = " + id;
    auto resultSet = rdbStore->QuerySql(sqlQuery);
    CHECK_AND_RETURN_RET_LOG(
        resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK,
        E_DB_FAIL, "Query not matched data fails");

    filePath = get<std::string>(
        ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_FILE_PATH, resultSet, ResultSetDataType::TYPE_STRING));
    return E_OK;
}

int32_t CloudSyncNotifyHandler::QueryAlbumLpathFromFileId(const std::string &id, std::string &lpath)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(
        rdbStore != nullptr,
        E_DB_FAIL, "QueryAlbumLpathFromFileId failed. rdbStore is null");
    const string sqlQuery = "SELECT * From " + PhotoColumn::PHOTOS_TABLE + " WHERE " + PhotoColumn::MEDIA_ID +
                            " = " + id;
    auto resultSet = rdbStore->QuerySql(sqlQuery);
    CHECK_AND_RETURN_RET_LOG(
        resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK,
        E_DB_FAIL, "Query not matched data fails");

    auto sourcePath = get<std::string>(
        ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_SOURCE_PATH, resultSet, ResultSetDataType::TYPE_STRING));
    int32_t mediaType = GetInt32Val(PhotoColumn::MEDIA_TYPE, resultSet);
    int32_t err = MediaLibraryAlbumOperations::GetLPathFromSourcePath(sourcePath, lpath, mediaType);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, E_DB_FAIL, "GetLPathFromSourcePath fail : %{public}s ", lpath.c_str());
    MEDIA_INFO_LOG("QueryAlbumLpathFromFileId succcess, lpath is : %{public}s ", lpath.c_str());
    return E_OK;
}

void CloudSyncNotifyHandler::HandleAlbumNotFound(const std::list<Uri> &uris)
{
    for (auto &uri : uris) {
        std::string uriString = uri.ToString();
        std::string fileId = GetfileIdFromPastDirtyDataFixUri(uriString);
        if (fileId == "") {
            continue;
        }
        if (fileId.compare(INVALID_ZERO_ID) == 0 || !IsCloudNotifyInfoValid(fileId)) {
            MEDIA_WARN_LOG("cloud observer get no valid uri : %{public}s", uriString.c_str());
            continue;
        }

        std::string lpath;
        int64_t newAlbumId = -1;
        bool isUserAlbum = false;
        auto err = QueryAlbumLpathFromFileId(fileId, lpath);
        if (err != E_SUCCESS) {
            MEDIA_ERR_LOG("QueryAlbumLpathFromFileId failed : %{public}d", err);
            continue;
        }
        MediaLibraryAlbumOperations::RecoverAlbum(fileId, lpath, isUserAlbum, newAlbumId);
        if (newAlbumId == -1) {
            MEDIA_ERR_LOG("HandleAlbumNotFound Fail, Recover album fails");
            continue;
        }

        if (isUserAlbum) {
        MediaLibraryRdbUtils::UpdateUserAlbumInternal(
            MediaLibraryUnistoreManager::GetInstance().GetRdbStore(), {to_string(newAlbumId)});
        } else {
            MediaLibraryRdbUtils::UpdateSourceAlbumInternal(
                MediaLibraryUnistoreManager::GetInstance().GetRdbStore(), {to_string(newAlbumId)});
        }
        auto watch = MediaLibraryNotify::GetInstance();
        watch->Notify(MediaFileUtils::GetUriByExtrConditions(
            PhotoAlbumColumns::ALBUM_URI_PREFIX, to_string(newAlbumId)), NotifyType::NOTIFY_ADD);
    }
    return;
}

void CloudSyncNotifyHandler::MakeResponsibilityChain()
{
    string uriString = notifyInfo_.uris.front().ToString();
    MEDIA_DEBUG_LOG("observer get first uri is : %{public}s", uriString.c_str());

    if (uriString.find("file://cloudsync/Photo/HeightError/") != string::npos) {
        return;
    }

    if (uriString.find("file://cloudsync/Photo/DownloadSuccessed/") != string::npos) {
        return;
    }

    if (uriString.find(PhotoColumn::PHOTO_CLOUD_URI_PREFIX) != string::npos) {
        ThumbnailObserverOnChange(notifyInfo_.uris, notifyInfo_.type);
    }

    if (uriString.find("file://cloudsync/Photo/RebuildCloudData/") != string::npos) {
        MEDIA_INFO_LOG("Get cloud rebuild cloud data notification : %{public}s",
            "file://cloudsync/Photo/RebuildCloudData/");
        MediaLibraryAlbumFusionUtils::CleanInvalidCloudAlbumAndData();
    }

    shared_ptr<BaseHandler> chain = nullptr;

    if (uriString.find(PhotoAlbumColumns::ALBUM_CLOUD_URI_PREFIX) != string::npos) {
        if (notifyInfo_.type == ChangeType::DELETE) {
            chain = NotifyResponsibilityChainFactory::CreateChain(ALBUM_DELETE);
        } else {
            chain = NotifyResponsibilityChainFactory::CreateChain(TRANSPARENT);
        }
    }

    if (uriString.find(PhotoColumn::PHOTO_CLOUD_URI_PREFIX) != string::npos) {
        if (notifyInfo_.type == ChangeType::UPDATE || notifyInfo_.type == ChangeType::OTHER) {
            chain = NotifyResponsibilityChainFactory::CreateChain(PHOTODELETE);
        } else {
            chain = NotifyResponsibilityChainFactory::CreateChain(TRANSPARENT);
        }
    }

    if (uriString.find(PhotoColumn::PHOTO_CLOUD_GALLERY_REBUILD_URI_PREFIX) != string::npos) {
        HandleDirtyDataFix(notifyInfo_.uris, static_cast<CloudSyncErrType>(notifyInfo_.type));
    }
    CloudSyncHandleData handleData;
    handleData.orgInfo = notifyInfo_;
    if (chain == nullptr) {
        MEDIA_ERR_LOG("uri OR type is Invalid");
        return;
    }
    chain->Handle(handleData);
}
} //namespace Media
} //namespace OHOS
