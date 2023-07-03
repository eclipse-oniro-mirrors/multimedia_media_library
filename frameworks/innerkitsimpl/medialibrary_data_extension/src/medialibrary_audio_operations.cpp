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

#include "medialibrary_audio_operations.h"

#include "abs_shared_result_set.h"
#include "file_asset.h"
#include "media_column.h"
#include "media_file_uri.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_command.h"
#include "medialibrary_data_manager_utils.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "medialibrary_notify.h"
#include "medialibrary_object_utils.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_type_const.h"
#include "medialibrary_uripermission_operations.h"
#include "userfile_manager_types.h"
#include "value_object.h"

using namespace std;
using namespace OHOS::NativeRdb;
using namespace OHOS::RdbDataShareAdapter;

namespace OHOS {
namespace Media {
int32_t MediaLibraryAudioOperations::Create(MediaLibraryCommand &cmd)
{
    switch (cmd.GetApi()) {
        case MediaLibraryApi::API_10:
            return CreateV10(cmd);
        case MediaLibraryApi::API_OLD:
            return CreateV9(cmd);
        default:
            MEDIA_ERR_LOG("get api failed");
            return E_FAIL;
    }
}

int32_t MediaLibraryAudioOperations::Delete(MediaLibraryCommand& cmd)
{
    string fileId = cmd.GetOprnFileId();
    vector<string> columns = {
        AudioColumn::MEDIA_ID,
        AudioColumn::MEDIA_FILE_PATH,
        AudioColumn::MEDIA_RELATIVE_PATH,
        AudioColumn::MEDIA_TYPE
    };
    shared_ptr<FileAsset> fileAsset = GetFileAssetFromDb(AudioColumn::MEDIA_ID,
        fileId, cmd.GetOprnObject());
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_FILEID, "Get fileAsset failed, fileId: %{public}s",
        fileId.c_str());
    
    int32_t deleteRow = DeleteAudio(fileAsset);
    CHECK_AND_RETURN_RET_LOG(deleteRow >= 0, deleteRow, "delete audio failed, deleteRow=%{public}d", deleteRow);

    return deleteRow;
}

std::shared_ptr<NativeRdb::ResultSet> MediaLibraryAudioOperations::Query(
    MediaLibraryCommand &cmd, const vector<string> &columns)
{
    return MediaLibraryRdbStore::Query(
        RdbUtils::ToPredicates(cmd.GetDataSharePred(), AudioColumn::AUDIOS_TABLE), columns);
}

int32_t MediaLibraryAudioOperations::Update(MediaLibraryCommand &cmd)
{
    switch (cmd.GetApi()) {
        case MediaLibraryApi::API_10:
            return UpdateV10(cmd);
        case MediaLibraryApi::API_OLD:
            return UpdateV9(cmd);
        default:
            MEDIA_ERR_LOG("get api failed");
            return E_FAIL;
    }

    return E_OK;
}

int32_t MediaLibraryAudioOperations::Open(MediaLibraryCommand &cmd, const string &mode)
{
    string uriString = cmd.GetUriStringWithoutSegment();
    string id = MediaFileUri(uriString).GetFileId();
    if (uriString.empty()) {
        return E_INVALID_URI;
    }

    vector<string> columns = {
        AudioColumn::MEDIA_ID,
        AudioColumn::MEDIA_FILE_PATH,
        AudioColumn::MEDIA_TYPE,
        AudioColumn::MEDIA_TIME_PENDING
    };
    auto fileAsset = GetFileAssetFromDb(AudioColumn::MEDIA_ID, id,
        OperationObject::FILESYSTEM_AUDIO, columns);
    fileAsset->SetUri(uriString);
    if (fileAsset == nullptr) {
        MEDIA_ERR_LOG("Failed to obtain path from Database, uri=%{private}s", uriString.c_str());
        return E_INVALID_URI;
    }

    if (uriString.find(AudioColumn::AUDIO_URI_PREFIX) != string::npos) {
        return OpenAsset(fileAsset, mode, MediaLibraryApi::API_10);
    }
    return OpenAsset(fileAsset, mode, cmd.GetApi());
}

int32_t MediaLibraryAudioOperations::Close(MediaLibraryCommand &cmd)
{
    string strFileId = cmd.GetOprnFileId();
    if (strFileId.empty()) {
        return E_INVALID_FILEID;
    }

    vector<string> columns = {
        AudioColumn::MEDIA_ID,
        AudioColumn::MEDIA_FILE_PATH,
        AudioColumn::MEDIA_TIME_PENDING,
        AudioColumn::MEDIA_TYPE,
        MediaColumn::MEDIA_DATE_MODIFIED,
        MediaColumn::MEDIA_DATE_ADDED
    };
    auto fileAsset = GetFileAssetFromDb(AudioColumn::MEDIA_ID, strFileId, cmd.GetOprnObject(), columns);
    if (fileAsset == nullptr) {
        MEDIA_ERR_LOG("Get FileAsset id %{public}s from database failed!", strFileId.c_str());
        return E_INVALID_FILEID;
    }

    int32_t errCode = CloseAsset(fileAsset);
    return errCode;
}

int32_t MediaLibraryAudioOperations::CreateV9(MediaLibraryCommand& cmd)
{
    FileAsset fileAsset;
    ValuesBucket &values = cmd.GetValueBucket();

    string displayName;
    CHECK_AND_RETURN_RET(GetStringFromValuesBucket(values, AudioColumn::MEDIA_NAME, displayName),
        E_HAS_DB_ERROR);
    fileAsset.SetDisplayName(displayName);

    string relativePath;
    CHECK_AND_RETURN_RET(GetStringFromValuesBucket(values, AudioColumn::MEDIA_RELATIVE_PATH, relativePath),
        E_HAS_DB_ERROR);
    fileAsset.SetRelativePath(relativePath);
    MediaFileUtils::FormatRelativePath(relativePath);

    int32_t mediaType = 0;
    CHECK_AND_RETURN_RET(GetInt32FromValuesBucket(values, AudioColumn::MEDIA_TYPE, mediaType),
        E_HAS_DB_ERROR);
    if (mediaType != MediaType::MEDIA_TYPE_AUDIO) {
        return E_CHECK_MEDIATYPE_FAIL;
    }
    fileAsset.SetMediaType(MediaType::MEDIA_TYPE_AUDIO);

    int32_t errCode = CheckRelativePathWithType(relativePath, MediaType::MEDIA_TYPE_AUDIO);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Failed to Check RelativePath and Extention, "
        "relativePath=%{private}s, mediaType=%{public}d", relativePath.c_str(), mediaType);
    errCode = CheckDisplayNameWithType(displayName, MediaType::MEDIA_TYPE_AUDIO);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Failed to Check Dir and Extention, "
        "displayName=%{private}s, mediaType=%{public}d", displayName.c_str(), mediaType);

    TransactionOperations transactionOprn;
    errCode = transactionOprn.Start();
    if (errCode != E_OK) {
        return errCode;
    }

    errCode = SetAssetPathInCreate(fileAsset);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("Failed to Solve FileAsset Path and Name, displayName=%{private}s", displayName.c_str());
        return errCode;
    }

    int32_t outRow = InsertAssetInDb(cmd, fileAsset);
    if (outRow <= 0) {
        MEDIA_ERR_LOG("insert file in db failed, error = %{public}d", outRow);
        return E_HAS_DB_ERROR;
    }
    transactionOprn.Finish();
    return outRow;
}

int32_t MediaLibraryAudioOperations::CreateV10(MediaLibraryCommand& cmd)
{
    FileAsset fileAsset;
    ValuesBucket &values = cmd.GetValueBucket();
    string displayName;
    string extention;
    string title;
    bool isContains = false;
    if (GetStringFromValuesBucket(values, AudioColumn::MEDIA_NAME, displayName)) {
        fileAsset.SetDisplayName(displayName);
        isContains = true;
    } else {
        CHECK_AND_RETURN_RET(GetStringFromValuesBucket(values, ASSET_EXTENTION, extention), E_HAS_DB_ERROR);
        if (GetStringFromValuesBucket(values, PhotoColumn::MEDIA_TITLE, title)) {
            displayName = title + "." + extention;
            fileAsset.SetDisplayName(displayName);
            isContains = true;
        }
    }

    int32_t mediaType = 0;
    CHECK_AND_RETURN_RET(GetInt32FromValuesBucket(values, AudioColumn::MEDIA_TYPE, mediaType),
        E_HAS_DB_ERROR);
    CHECK_AND_RETURN_RET(mediaType == MediaType::MEDIA_TYPE_AUDIO, E_CHECK_MEDIATYPE_FAIL);
    fileAsset.SetMediaType(MediaType::MEDIA_TYPE_AUDIO);

    // Check rootdir and extention
    int32_t errCode = CheckWithType(isContains, displayName, extention, MediaType::MEDIA_TYPE_AUDIO);
    CHECK_AND_RETURN_RET(errCode == E_OK, errCode);
    TransactionOperations transactionOprn;
    errCode = transactionOprn.Start();
    CHECK_AND_RETURN_RET(errCode == E_OK, errCode);
    errCode = isContains ? SetAssetPathInCreate(fileAsset) : SetAssetPath(fileAsset, extention);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode,
        "Failed to Solve FileAsset Path and Name, displayName=%{private}s", displayName.c_str());

    int32_t outRow = InsertAssetInDb(cmd, fileAsset);
    CHECK_AND_RETURN_RET_LOG(outRow > 0, errCode, "insert file in db failed, error = %{public}d", outRow);
    string bdName = cmd.GetBundleName();
    if (!bdName.empty()) {
        errCode = UriPermissionOperations::InsertBundlePermission(outRow, bdName, MEDIA_FILEMODE_READWRITE,
            AudioColumn::AUDIOS_TABLE);
        CHECK_AND_RETURN_RET_LOG(errCode >= 0, errCode, "InsertBundlePermission failed, err=%{public}d", errCode);
    }
    transactionOprn.Finish();
    return outRow;
}

int32_t MediaLibraryAudioOperations::DeleteAudio(const shared_ptr<FileAsset> &fileAsset)
{
    string filePath = fileAsset->GetPath();
    CHECK_AND_RETURN_RET_LOG(!filePath.empty(), E_INVALID_PATH, "get file path failed");
    bool res = MediaFileUtils::DeleteFile(filePath);
    CHECK_AND_RETURN_RET_LOG(res, E_HAS_FS_ERROR, "Delete audio file failed, errno: %{public}d", errno);

    // delete thumbnail
    int32_t fileId = fileAsset->GetId();
    InvalidateThumbnail(to_string(fileId), fileAsset->GetMediaType());

    TransactionOperations transactionOprn;
    int32_t errCode = transactionOprn.Start();
    if (errCode != E_OK) {
        return errCode;
    }

    // delete file in db
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_AUDIO, OperationType::DELETE);
    cmd.GetAbsRdbPredicates()->EqualTo(AudioColumn::MEDIA_ID, to_string(fileId));
    int32_t deleteRows = DeleteAssetInDb(cmd);
    if (deleteRows <= 0) {
        MEDIA_ERR_LOG("Delete audio in database failed, errCode=%{public}d", deleteRows);
        return E_HAS_DB_ERROR;
    }
    transactionOprn.Finish();

    auto watch = MediaLibraryNotify::GetInstance();
    watch->Notify(AudioColumn::AUDIO_URI_PREFIX + to_string(fileId), NotifyType::NOTIFY_REMOVE);
    return deleteRows;
}

int32_t MediaLibraryAudioOperations::UpdateV10(MediaLibraryCommand &cmd)
{
    vector<string> columns = {
        AudioColumn::MEDIA_ID,
        AudioColumn::MEDIA_FILE_PATH,
        AudioColumn::MEDIA_TYPE,
        AudioColumn::MEDIA_NAME
    };
    shared_ptr<FileAsset> fileAsset = GetFileAssetFromDb(*(cmd.GetAbsRdbPredicates()),
        OperationObject::FILESYSTEM_AUDIO, columns);
    if (fileAsset == nullptr) {
        return E_INVALID_VALUES;
    }

    // Update if FileAsset.title or FileAsset.displayName is modified
    bool isNameChanged = false;
    int32_t errCode = UpdateFileName(cmd, fileAsset, isNameChanged);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Update Audio Name failed, fileName=%{private}s",
        fileAsset->GetDisplayName().c_str());

    TransactionOperations transactionOprn;
    errCode = transactionOprn.Start();
    if (errCode != E_OK) {
        return errCode;
    }

    int32_t rowId = UpdateFileInDb(cmd);
    if (rowId < 0) {
        MEDIA_ERR_LOG("Update Audio In database failed, rowId=%{public}d", rowId);
        return rowId;
    }
    transactionOprn.Finish();

    errCode = SendTrashNotify(cmd, fileAsset->GetId());
    if (errCode == E_OK) {
        return rowId;
    }

    // Audio has no favorite album, do not send favorite notify
    auto watch = MediaLibraryNotify::GetInstance();
    watch->Notify(AudioColumn::AUDIO_URI_PREFIX + to_string(fileAsset->GetId()), NotifyType::NOTIFY_UPDATE);
    return rowId;
}

int32_t MediaLibraryAudioOperations::UpdateV9(MediaLibraryCommand &cmd)
{
    vector<string> columns = {
        AudioColumn::MEDIA_ID,
        AudioColumn::MEDIA_FILE_PATH,
        AudioColumn::MEDIA_TYPE,
        AudioColumn::MEDIA_NAME,
        AudioColumn::MEDIA_RELATIVE_PATH
    };
    shared_ptr<FileAsset> fileAsset = GetFileAssetFromDb(*(cmd.GetAbsRdbPredicates()),
        OperationObject::FILESYSTEM_AUDIO, columns);
    if (fileAsset == nullptr) {
        return E_INVALID_VALUES;
    }

    // Update if FileAsset.title or FileAsset.displayName is modified
    bool isNameChanged = false;
    int32_t errCode = UpdateFileName(cmd, fileAsset, isNameChanged);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Update Audio Name failed, fileName=%{private}s",
        fileAsset->GetDisplayName().c_str());
    errCode = UpdateRelativePath(cmd, fileAsset, isNameChanged);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Update Audio RelativePath failed, relativePath=%{private}s",
        fileAsset->GetRelativePath().c_str());
    if (isNameChanged) {
        UpdateVirtualPath(cmd, fileAsset);
    }

    TransactionOperations transactionOprn;
    errCode = transactionOprn.Start();
    if (errCode != E_OK) {
        return errCode;
    }

    int32_t rowId = UpdateFileInDb(cmd);
    if (rowId < 0) {
        MEDIA_ERR_LOG("Update Audio In database failed, rowId=%{public}d", rowId);
        return rowId;
    }
    transactionOprn.Finish();

    errCode = SendTrashNotify(cmd, fileAsset->GetId());
    if (errCode == E_OK) {
        return rowId;
    }

    // Audio has no favorite album, do not send favorite notify
    auto watch = MediaLibraryNotify::GetInstance();
    watch->Notify(AudioColumn::AUDIO_URI_PREFIX + to_string(fileAsset->GetId()), NotifyType::NOTIFY_UPDATE);
    return rowId;
}

int32_t MediaLibraryAudioOperations::TrashAging()
{
    auto time = MediaFileUtils::UTCTimeSeconds();
    RdbPredicates predicates(AudioColumn::AUDIOS_TABLE);
    predicates.GreaterThan(MediaColumn::MEDIA_DATE_TRASHED, to_string(0));
    predicates.And()->LessThanOrEqualTo(MediaColumn::MEDIA_DATE_TRASHED, to_string(time - AGING_TIME));
    int32_t deletedRows = MediaLibraryRdbStore::DeleteFromDisk(predicates);
    if (deletedRows < 0) {
        return deletedRows;
    }
    return E_OK;
}
} // namespace Media
} // namespace OHOS