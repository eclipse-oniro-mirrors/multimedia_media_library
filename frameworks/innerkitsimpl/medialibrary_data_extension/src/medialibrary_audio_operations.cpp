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
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "medialibrary_notify.h"
#include "medialibrary_object_utils.h"
#include "medialibrary_rdb_transaction.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_type_const.h"
#include "medialibrary_uripermission_operations.h"
#include "thumbnail_const.h"
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
    shared_ptr<FileAsset> fileAsset = GetFileAssetFromDb(*(cmd.GetAbsRdbPredicates()),
        cmd.GetOprnObject(), columns);
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_FILEID, "Get fileAsset failed, fileId: %{private}s",
        fileId.c_str());

    int32_t deleteRow = DeleteAudio(fileAsset, cmd.GetApi());
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

const static vector<string> AUDIO_COLUMN_VECTOR = {
    AudioColumn::MEDIA_FILE_PATH,
    AudioColumn::MEDIA_TIME_PENDING
};

int32_t MediaLibraryAudioOperations::Open(MediaLibraryCommand &cmd, const string &mode)
{
    string uriString = cmd.GetUriStringWithoutSegment();
    string pendingStatus = cmd.GetQuerySetParam(MediaColumn::MEDIA_TIME_PENDING);

    shared_ptr<FileAsset> fileAsset = GetFileAssetByUri(uriString, false, AUDIO_COLUMN_VECTOR, pendingStatus);
    if (fileAsset == nullptr) {
        MEDIA_ERR_LOG("Get FileAsset From Uri Failed, uri:%{public}s", uriString.c_str());
        return E_URI_INVALID;
    }

    if (uriString.find(AudioColumn::AUDIO_URI_PREFIX) != string::npos) {
        return OpenAsset(fileAsset, mode, MediaLibraryApi::API_10);
    }
    return OpenAsset(fileAsset, mode, cmd.GetApi());
}

int32_t MediaLibraryAudioOperations::Close(MediaLibraryCommand &cmd)
{
    const ValuesBucket &values = cmd.GetValueBucket();
    string uriString;
    if (!GetStringFromValuesBucket(values, MEDIA_DATA_DB_URI, uriString)) {
        return E_INVALID_VALUES;
    }
    string pendingStatus = cmd.GetQuerySetParam(MediaColumn::MEDIA_TIME_PENDING);

    shared_ptr<FileAsset> fileAsset = GetFileAssetByUri(uriString, false, AUDIO_COLUMN_VECTOR, pendingStatus);
    if (fileAsset == nullptr) {
        MEDIA_ERR_LOG("Get FileAsset From Uri Failed, uri:%{public}s", uriString.c_str());
        return E_INVALID_URI;
    }

    int32_t isSync = 0;
    int32_t errCode = 0;
    if (GetInt32FromValuesBucket(cmd.GetValueBucket(), CLOSE_CREATE_THUMB_STATUS, isSync) &&
        isSync == CREATE_THUMB_SYNC_STATUS) {
        errCode = CloseAsset(fileAsset, true);
    } else {
        errCode = CloseAsset(fileAsset, false);
    }
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

    TransactionOperations transactionOprn(MediaLibraryUnistoreManager::GetInstance().GetRdbStoreRaw()->GetRaw());
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
    bool isNeedGrant = false;
    if (GetStringFromValuesBucket(values, AudioColumn::MEDIA_NAME, displayName)) {
        fileAsset.SetDisplayName(displayName);
        fileAsset.SetTimePending(UNCREATE_FILE_TIMEPENDING);
        isContains = true;
    } else {
        CHECK_AND_RETURN_RET(GetStringFromValuesBucket(values, ASSET_EXTENTION, extention), E_HAS_DB_ERROR);
        isNeedGrant = true;
        fileAsset.SetTimePending(UNOPEN_FILE_COMPONENT_TIMEPENDING);
        if (GetStringFromValuesBucket(values, AudioColumn::MEDIA_TITLE, title)) {
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
    TransactionOperations transactionOprn(MediaLibraryUnistoreManager::GetInstance().GetRdbStoreRaw()->GetRaw());
    errCode = transactionOprn.Start();
    CHECK_AND_RETURN_RET(errCode == E_OK, errCode);
    errCode = isContains ? SetAssetPathInCreate(fileAsset) : SetAssetPath(fileAsset, extention);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode,
        "Failed to Solve FileAsset Path and Name, displayName=%{private}s", displayName.c_str());

    int32_t outRow = InsertAssetInDb(cmd, fileAsset);
    CHECK_AND_RETURN_RET_LOG(outRow > 0, E_HAS_DB_ERROR, "insert file in db failed, error = %{public}d", outRow);
    transactionOprn.Finish();
    fileAsset.SetId(outRow);
    string fileUri = CreateExtUriForV10Asset(fileAsset);
    if (isNeedGrant) {
        int32_t ret = GrantUriPermission(fileUri, cmd.GetBundleName(), fileAsset.GetPath());
        CHECK_AND_RETURN_RET(ret == E_OK, ret);
    }
    cmd.SetResult(fileUri);
    return outRow;
}

int32_t MediaLibraryAudioOperations::DeleteAudio(const shared_ptr<FileAsset> &fileAsset, MediaLibraryApi api)
{
    string filePath = fileAsset->GetPath();
    CHECK_AND_RETURN_RET_LOG(!filePath.empty(), E_INVALID_PATH, "get file path failed");
    bool res = MediaFileUtils::DeleteFile(filePath);
    CHECK_AND_RETURN_RET_LOG(res, E_HAS_FS_ERROR, "Delete audio file failed, errno: %{public}d", errno);

    // delete thumbnail
    int32_t fileId = fileAsset->GetId();
    InvalidateThumbnail(to_string(fileId), fileAsset->GetMediaType());

    TransactionOperations transactionOprn(MediaLibraryUnistoreManager::GetInstance().GetRdbStoreRaw()->GetRaw());
    int32_t errCode = transactionOprn.Start();
    if (errCode != E_OK) {
        return errCode;
    }
    string displayName = fileAsset->GetDisplayName();
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
    string notifyUri = MediaFileUtils::GetUriByExtrConditions(AudioColumn::AUDIO_URI_PREFIX, to_string(fileId),
        (api == MediaLibraryApi::API_10 ? MediaFileUtils::GetExtraUri(displayName, filePath) : ""));

    watch->Notify(notifyUri, NotifyType::NOTIFY_REMOVE);
    return deleteRows;
}

int32_t MediaLibraryAudioOperations::UpdateV10(MediaLibraryCommand &cmd)
{
    if (cmd.GetOprnType() == OperationType::UPDATE_PENDING) {
        return SetPendingStatus(cmd);
    }
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

    TransactionOperations transactionOprn(MediaLibraryUnistoreManager::GetInstance().GetRdbStoreRaw()->GetRaw());
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

    string extraUri = MediaFileUtils::GetExtraUri(fileAsset->GetDisplayName(), fileAsset->GetPath());
    errCode = SendTrashNotify(cmd, fileAsset->GetId(), extraUri);
    if (errCode == E_OK) {
        return rowId;
    }

    // Audio has no favorite album, do not send favorite notify
    auto watch = MediaLibraryNotify::GetInstance();
    watch->Notify(MediaFileUtils::GetUriByExtrConditions(AudioColumn::AUDIO_URI_PREFIX, to_string(fileAsset->GetId()),
        extraUri), NotifyType::NOTIFY_UPDATE);
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

    TransactionOperations transactionOprn(MediaLibraryUnistoreManager::GetInstance().GetRdbStoreRaw()->GetRaw());
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
    watch->Notify(MediaFileUtils::GetUriByExtrConditions(AudioColumn::AUDIO_URI_PREFIX, to_string(fileAsset->GetId())),
        NotifyType::NOTIFY_UPDATE);
    return rowId;
}

int32_t MediaLibraryAudioOperations::TrashAging(shared_ptr<int> countPtr)
{
    auto time = MediaFileUtils::UTCTimeMilliSeconds();
    RdbPredicates predicates(AudioColumn::AUDIOS_TABLE);
    predicates.GreaterThan(MediaColumn::MEDIA_DATE_TRASHED, to_string(0));
    predicates.And()->LessThanOrEqualTo(MediaColumn::MEDIA_DATE_TRASHED, to_string(time - AGING_TIME));
    int32_t deletedRows = DeleteFromDisk(predicates, true);
    if (deletedRows < 0) {
        return deletedRows;
    }
    if (countPtr != nullptr) {
        *countPtr = deletedRows;
    }
    return E_OK;
}
} // namespace Media
} // namespace OHOS