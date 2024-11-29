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

#include "medialibrary_photo_operations.h"

#include <memory>
#include <mutex>
#include <thread>
#include <nlohmann/json.hpp>

#include "abs_shared_result_set.h"
#include "file_asset.h"
#include "file_utils.h"
#include "image_source.h"
#include "media_analysis_helper.h"
#include "image_packer.h"
#include "iservice_registry.h"
#include "media_change_effect.h"
#include "media_column.h"
#include "media_exif.h"
#include "media_file_uri.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_album_fusion_utils.h"
#include "medialibrary_album_operations.h"
#include "medialibrary_analysis_album_operations.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_command.h"
#include "medialibrary_data_manager_utils.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "medialibrary_inotify.h"
#include "medialibrary_notify.h"
#include "medialibrary_object_utils.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_rdb_transaction.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_tracer.h"
#include "medialibrary_type_const.h"
#include "medialibrary_uripermission_operations.h"
#include "mimetype_utils.h"
#include "multistages_capture_manager.h"
#include "enhancement_manager.h"
#include "permission_utils.h"
#include "photo_album_column.h"
#include "photo_map_column.h"
#include "photo_map_operations.h"
#include "picture.h"
#include "image_type.h"
#include "picture_manager_thread.h"

#include "rdb_predicates.h"
#include "result_set_utils.h"
#include "thumbnail_const.h"
#include "thumbnail_service.h"
#include "uri.h"
#include "userfile_manager_types.h"
#include "value_object.h"
#include "values_bucket.h"
#include "medialibrary_formmap_operations.h"
#include "medialibrary_vision_operations.h"
#include "dfx_manager.h"
#include "moving_photo_file_utils.h"
#include "hi_audit.h"
#include "video_composition_callback_imp.h"

using namespace OHOS::DataShare;
using namespace std;
using namespace OHOS::NativeRdb;
using namespace OHOS::RdbDataShareAdapter;

namespace OHOS {
namespace Media {
static const string ANALYSIS_HAS_DATA = "1";
const string PHOTO_ALBUM_URI_PREFIX_V0 = "file://media/PhotoAlbum/";
constexpr int SAVE_PHOTO_WAIT_MS = 300;
constexpr int TASK_NUMBER_MAX = 5;

constexpr int32_t ORIENTATION_0 = 1;
constexpr int32_t ORIENTATION_90 = 6;
constexpr int32_t ORIENTATION_180 = 3;
constexpr int32_t ORIENTATION_270 = 8;
constexpr int32_t OFFSET = 5;
constexpr int32_t ZERO_ASCII = '0';

enum ImageFileType : int32_t {
    JPEG = 1,
    HEIF = 2
};

const std::string MIME_TYPE_JPEG = "image/jpeg";

const std::string MIME_TYPE_HEIF = "image/heif";

const std::string EXTENSION_JPEG = ".jpg";

const std::string EXTENSION_HEIF = ".heic";

const std::unordered_map<ImageFileType, std::string> IMAGE_FILE_TYPE_MAP = {
    {JPEG, MIME_TYPE_JPEG},
    {HEIF, MIME_TYPE_HEIF},
};

const std::unordered_map<ImageFileType, std::string> IMAGE_EXTENSION_MAP = {
    {JPEG, EXTENSION_JPEG},
    {HEIF, EXTENSION_HEIF},
};
shared_ptr<PhotoEditingRecord> PhotoEditingRecord::instance_ = nullptr;
mutex PhotoEditingRecord::mutex_;
std::mutex MediaLibraryPhotoOperations::saveCameraPhotoMutex_;
std::condition_variable MediaLibraryPhotoOperations::condition_;
std::string MediaLibraryPhotoOperations::lastPhotoId_ = "default";
int32_t MediaLibraryPhotoOperations::Create(MediaLibraryCommand &cmd)
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

int32_t MediaLibraryPhotoOperations::DeleteCache(MediaLibraryCommand& cmd)
{
    string uriString = cmd.GetUriStringWithoutSegment();
    if (!MediaFileUtils::StartsWith(uriString, PhotoColumn::PHOTO_CACHE_URI_PREFIX)) {
        return E_INVALID_VALUES;
    }

    string fileName = uriString.substr(PhotoColumn::PHOTO_CACHE_URI_PREFIX.size());
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CheckDisplayName(fileName) == E_OK, E_INVALID_URI,
        "Check fileName failed, fileName=%{private}s", fileName.c_str());
    string cacheDir = GetAssetCacheDir();
    string path = cacheDir + "/" + fileName;
    if (!MediaFileUtils::DeleteFile(path)) {
        MEDIA_ERR_LOG("Delete cache file failed, errno: %{public}d, uri: %{private}s", errno, uriString.c_str());
        return E_HAS_FS_ERROR;
    }
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::Delete(MediaLibraryCommand& cmd)
{
    // delete file in .cache
    if (MediaFileUtils::StartsWith(cmd.GetUri().ToString(), PhotoColumn::PHOTO_CACHE_URI_PREFIX)) {
        return DeleteCache(cmd);
    }

    string fileId = cmd.GetOprnFileId();
    vector<string> columns = {
        PhotoColumn::MEDIA_ID,
        PhotoColumn::MEDIA_FILE_PATH,
        PhotoColumn::MEDIA_RELATIVE_PATH,
        PhotoColumn::MEDIA_TYPE
    };
    shared_ptr<FileAsset> fileAsset = GetFileAssetFromDb(*(cmd.GetAbsRdbPredicates()),
        cmd.GetOprnObject(), columns);
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_FILEID, "Get fileAsset failed, fileId: %{private}s",
        fileId.c_str());
    int32_t deleteRow = DeletePhoto(fileAsset, cmd.GetApi());
    CHECK_AND_RETURN_RET_LOG(deleteRow >= 0, deleteRow, "delete photo failed, deleteRow=%{public}d", deleteRow);

    return deleteRow;
}

static int32_t GetAlbumTypeSubTypeById(const string &albumId, PhotoAlbumType &type, PhotoAlbumSubType &subType)
{
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, albumId);
    vector<string> columns = { PhotoAlbumColumns::ALBUM_TYPE, PhotoAlbumColumns::ALBUM_SUBTYPE };
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("album id %{private}s is not exist", albumId.c_str());
        return E_INVALID_ARGUMENTS;
    }
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK, E_INVALID_ARGUMENTS,
        "album id is not exist");
    type = static_cast<PhotoAlbumType>(GetInt32Val(PhotoAlbumColumns::ALBUM_TYPE, resultSet));
    subType = static_cast<PhotoAlbumSubType>(GetInt32Val(PhotoAlbumColumns::ALBUM_SUBTYPE, resultSet));
    resultSet->Close();
    return E_SUCCESS;
}

static int32_t GetPredicatesByAlbumId(const string &albumId, RdbPredicates &predicates)
{
    PhotoAlbumType type;
    PhotoAlbumSubType subType;
    CHECK_AND_RETURN_RET_LOG(GetAlbumTypeSubTypeById(albumId, type, subType) == E_SUCCESS, E_INVALID_ARGUMENTS,
        "invalid album uri");

    if ((!PhotoAlbum::CheckPhotoAlbumType(type)) || (!PhotoAlbum::CheckPhotoAlbumSubType(subType))) {
        MEDIA_ERR_LOG("album id %{private}s type:%d subtype:%d", albumId.c_str(), type, subType);
        return E_INVALID_ARGUMENTS;
    }

    if (PhotoAlbum::IsUserPhotoAlbum(type, subType)) {
        PhotoAlbumColumns::GetUserAlbumPredicates(stoi(albumId), predicates, false);
        return E_SUCCESS;
    }

    if (PhotoAlbum::IsSourceAlbum(type, subType)) {
        PhotoAlbumColumns::GetSourceAlbumPredicates(stoi(albumId), predicates, false);
        return E_SUCCESS;
    }

    if ((type != PhotoAlbumType::SYSTEM) || (subType == PhotoAlbumSubType::USER_GENERIC) ||
        (subType == PhotoAlbumSubType::ANY)) {
        MEDIA_ERR_LOG("album id %{private}s type:%d subtype:%d", albumId.c_str(), type, subType);
        return E_INVALID_ARGUMENTS;
    }
    PhotoAlbumColumns::GetSystemAlbumPredicates(subType, predicates, false);
    return E_SUCCESS;
}

static bool GetValidOrderClause(const DataSharePredicates &predicate, string &clause)
{
    constexpr int32_t FIELD_IDX = 0;
    vector<OperationItem> operations;
    const auto &items = predicate.GetOperationList();
    int32_t count = 0;
    clause = "ROW_NUMBER() OVER (ORDER BY ";
    for (const auto &item : items) {
        if (item.operation == ORDER_BY_ASC) {
            count++;
            clause += static_cast<string>(item.GetSingle(FIELD_IDX)) + " ASC) as " + PHOTO_INDEX;
        } else if (item.operation == ORDER_BY_DESC) {
            count++;
            clause += static_cast<string>(item.GetSingle(FIELD_IDX)) + " DESC) as " + PHOTO_INDEX;
        }
    }

    // only support orderby with one item
    return (count == 1);
}

static bool AppendValidOrderClause(MediaLibraryCommand &cmd, RdbPredicates &predicates, const string &photoId)
{
    constexpr int32_t FIELD_IDX = 0;
    const auto &items = cmd.GetDataSharePred().GetOperationList();
    int32_t count = 0;
    string dateType;
    string comparisonOperator;
    for (const auto &item : items) {
        if (item.operation == ORDER_BY_ASC) {
            count++;
            dateType = static_cast<string>(item.GetSingle(FIELD_IDX)) == MediaColumn::MEDIA_DATE_TAKEN ?
                MediaColumn::MEDIA_DATE_TAKEN : MediaColumn::MEDIA_DATE_ADDED;
            comparisonOperator = " <= ";
        } else if (item.operation == ORDER_BY_DESC) {
            count++;
            dateType = static_cast<string>(item.GetSingle(FIELD_IDX)) == MediaColumn::MEDIA_DATE_TAKEN ?
                MediaColumn::MEDIA_DATE_TAKEN : MediaColumn::MEDIA_DATE_ADDED;
            comparisonOperator = " >= ";
        }
    }
    string columnName = " (" + dateType + ", " + MediaColumn::MEDIA_ID + ") ";
    string value = " (SELECT " + dateType + ", " + MediaColumn::MEDIA_ID + " FROM " +
        PhotoColumn::PHOTOS_TABLE + " WHERE " + MediaColumn::MEDIA_ID + " = " + photoId + ") ";
    string whereClause = predicates.GetWhereClause();
    whereClause += " AND " + columnName + comparisonOperator + value;
    predicates.SetWhereClause(whereClause);

    // only support orderby with one item
    return (count == 1);
}

static shared_ptr<NativeRdb::ResultSet> HandleAlbumIndexOfUri(MediaLibraryCommand &cmd, const string &photoId,
    const string &albumId)
{
    string orderClause;
    CHECK_AND_RETURN_RET_LOG(GetValidOrderClause(cmd.GetDataSharePred(), orderClause), nullptr, "invalid orderby");
    vector<string> columns;
    columns.push_back(orderClause);
    columns.push_back(MediaColumn::MEDIA_ID);
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    CHECK_AND_RETURN_RET_LOG(GetPredicatesByAlbumId(albumId, predicates) == E_SUCCESS, nullptr, "invalid album uri");
    return MediaLibraryRdbStore::GetIndexOfUri(predicates, columns, photoId);
}

static shared_ptr<NativeRdb::ResultSet> HandleIndexOfUri(MediaLibraryCommand &cmd, RdbPredicates &predicates,
    const string &photoId, const string &albumId)
{
    if (!albumId.empty()) {
        return HandleAlbumIndexOfUri(cmd, photoId, albumId);
    }
    string indexClause = " COUNT(*) as " + PHOTO_INDEX;
    vector<string> columns;
    columns.push_back(indexClause);
    predicates.And()->EqualTo(
        PhotoColumn::PHOTO_SYNC_STATUS, std::to_string(static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE)));
    predicates.And()->EqualTo(
        PhotoColumn::PHOTO_CLEAN_FLAG, std::to_string(static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN)));
    CHECK_AND_RETURN_RET_LOG(AppendValidOrderClause(cmd, predicates, photoId), nullptr, "invalid orderby");
    return MediaLibraryRdbStore::GetIndexOfUriForPhotos(predicates, columns, photoId);
}

static shared_ptr<NativeRdb::ResultSet> HandleAnalysisIndex(MediaLibraryCommand &cmd,
    const string &photoId, const string &albumId)
{
    string orderClause;
    CHECK_AND_RETURN_RET_LOG(GetValidOrderClause(cmd.GetDataSharePred(), orderClause), nullptr, "invalid orderby");
    CHECK_AND_RETURN_RET_LOG(albumId.size() > 0, nullptr, "null albumId");
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    PhotoAlbumColumns::GetAnalysisAlbumPredicates(stoi(albumId), predicates, false);
    vector<string> columns;
    columns.push_back(orderClause);
    columns.push_back(MediaColumn::MEDIA_ID);
    return MediaLibraryRdbStore::GetIndexOfUri(predicates, columns, photoId);
}

shared_ptr<NativeRdb::ResultSet> MediaLibraryPhotoOperations::Query(
    MediaLibraryCommand &cmd, const vector<string> &columns)
{
    RdbPredicates predicates = RdbUtils::ToPredicates(cmd.GetDataSharePred(), PhotoColumn::PHOTOS_TABLE);
    if (cmd.GetOprnType() == OperationType::INDEX || cmd.GetOprnType() == OperationType::ANALYSIS_INDEX) {
        constexpr int32_t COLUMN_SIZE = 2;
        CHECK_AND_RETURN_RET_LOG(columns.size() >= COLUMN_SIZE, nullptr, "invalid id param");
        constexpr int32_t PHOTO_ID_INDEX = 0;
        constexpr int32_t ALBUM_ID_INDEX = 1;
        string photoId = columns[PHOTO_ID_INDEX];
        string albumId;
        if (!columns[ALBUM_ID_INDEX].empty()) {
            albumId = columns[ALBUM_ID_INDEX];
        }
        if (cmd.GetOprnType() == OperationType::ANALYSIS_INDEX) {
            return HandleAnalysisIndex(cmd, photoId, albumId);
        }
        return HandleIndexOfUri(cmd, predicates, photoId, albumId);
    }
    int limit = predicates.GetLimit();
    int offset = predicates.GetOffset();
    if (cmd.GetOprnType() == OperationType::ALL_DUPLICATE_ASSETS) {
        return MediaLibraryRdbStore::GetAllDuplicateAssets(columns, offset, limit);
    }
    if (cmd.GetOprnType() == OperationType::CAN_DEL_DUPLICATE_ASSETS) {
        return MediaLibraryRdbStore::GetCanDelDuplicateAssets(columns, offset, limit);
    }
    MediaLibraryRdbUtils::AddQueryIndex(predicates, columns);
    return MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
}

int32_t MediaLibraryPhotoOperations::Update(MediaLibraryCommand &cmd)
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

const static vector<string> PHOTO_COLUMN_VECTOR = {
    PhotoColumn::MEDIA_FILE_PATH,
    PhotoColumn::MEDIA_TYPE,
    PhotoColumn::MEDIA_TIME_PENDING,
    PhotoColumn::PHOTO_SUBTYPE,
    PhotoColumn::PHOTO_COVER_POSITION,
    PhotoColumn::MOVING_PHOTO_EFFECT_MODE,
    PhotoColumn::PHOTO_BURST_COVER_LEVEL,
    PhotoColumn::PHOTO_POSITION,
};

bool CheckOpenMovingPhoto(int32_t photoSubType, int32_t effectMode, const string& request)
{
    return photoSubType == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO) ||
        (effectMode == static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY) &&
        request == SOURCE_REQUEST);
}

static int32_t ProcessMovingPhotoOprnKey(MediaLibraryCommand& cmd, shared_ptr<FileAsset>& fileAsset, const string& id,
    bool& isMovingPhotoVideo)
{
    string movingPhotoOprnKey = cmd.GetQuerySetParam(MEDIA_MOVING_PHOTO_OPRN_KEYWORD);
    if (movingPhotoOprnKey == OPEN_MOVING_PHOTO_VIDEO ||
        movingPhotoOprnKey == OPEN_MOVING_PHOTO_VIDEO_CLOUD) {
        CHECK_AND_RETURN_RET_LOG(CheckOpenMovingPhoto(fileAsset->GetPhotoSubType(),
            fileAsset->GetMovingPhotoEffectMode(), cmd.GetQuerySetParam(MEDIA_OPERN_KEYWORD)),
            E_INVALID_VALUES,
            "Non-moving photo is requesting moving photo operation, file id: %{public}s, actual subtype: %{public}d",
            id.c_str(), fileAsset->GetPhotoSubType());
        string imagePath = fileAsset->GetPath();
        fileAsset->SetPath(MediaFileUtils::GetMovingPhotoVideoPath(imagePath));
        isMovingPhotoVideo = true;
        if (movingPhotoOprnKey == OPEN_MOVING_PHOTO_VIDEO_CLOUD && fileAsset->GetPosition() == POSITION_CLOUD) {
            fileAsset->SetPath(imagePath);
            isMovingPhotoVideo = false;
        }
    } else if (movingPhotoOprnKey == OPEN_PRIVATE_LIVE_PHOTO) {
        CHECK_AND_RETURN_RET_LOG(fileAsset->GetPhotoSubType() == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO),
            E_INVALID_VALUES,
            "Non-moving photo is requesting moving photo operation, file id: %{public}s, actual subtype: %{public}d",
            id.c_str(), fileAsset->GetPhotoSubType());
        string livePhotoPath;
        CHECK_AND_RETURN_RET_LOG(MovingPhotoFileUtils::ConvertToLivePhoto(fileAsset->GetPath(),
            fileAsset->GetCoverPosition(), livePhotoPath) == E_OK,
            E_INVALID_VALUES,
            "Failed convert to live photo");
        fileAsset->SetPath(livePhotoPath);
    }
    return E_OK;
}

static void UpdateLastVisitTime(MediaLibraryCommand &cmd, const string &id)
{
    if (cmd.GetTableName() != PhotoColumn::PHOTOS_TABLE) {
        return;
    }
    std::thread([=] {
        int32_t changedRows = MediaLibraryRdbStore::UpdateLastVisitTime(id);
        if (changedRows <= 0) {
            MEDIA_ERR_LOG("update lastVisitTime Failed, changedRows = %{public}d.", changedRows);
        }
    }).detach();
}

static void GetType(string &uri, int32_t &type)
{
    int pos = uri.find("type=");
    if (pos != uri.npos) {
        type = uri[pos + OFFSET] - ZERO_ASCII;
    }
}

int32_t MediaLibraryPhotoOperations::Open(MediaLibraryCommand &cmd, const string &mode)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryPhotoOperations::Open");

    bool isCacheOperation = false;
    int32_t errCode = OpenCache(cmd, mode, isCacheOperation);
    if (errCode != E_OK || isCacheOperation) {
        return errCode;
    }
    bool isSkipEdit = true;
    errCode = OpenEditOperation(cmd, isSkipEdit);
    if (errCode != E_OK || !isSkipEdit) {
        return errCode;
    }
    string uriString = cmd.GetUriStringWithoutSegment();
    string id = MediaFileUtils::GetIdFromUri(uriString);
    if (uriString.empty() || (!MediaLibraryDataManagerUtils::IsNumber(id))) {
        return E_INVALID_URI;
    }
    string pendingStatus = cmd.GetQuerySetParam(MediaColumn::MEDIA_TIME_PENDING);
    shared_ptr<FileAsset> fileAsset = GetFileAssetByUri(uriString, true,  PHOTO_COLUMN_VECTOR, pendingStatus);
    if (fileAsset == nullptr) {
        MEDIA_ERR_LOG("Get FileAsset From Uri Failed, uri:%{public}s", uriString.c_str());
        return E_INVALID_URI;
    }

    bool isMovingPhotoVideo = false;
    errCode = ProcessMovingPhotoOprnKey(cmd, fileAsset, id, isMovingPhotoVideo);
    if (errCode != E_OK) {
        return errCode;
    }
    UpdateLastVisitTime(cmd, id);
    string uri = cmd.GetUri().ToString();
    int32_t type = -1;
    GetType(uri, type);
    MEDIA_DEBUG_LOG("After spliting, uri is %{public}s", uri.c_str());
    MEDIA_DEBUG_LOG("After spliting, type is %{public}d", type);
    if (uriString.find(PhotoColumn::PHOTO_URI_PREFIX) != string::npos) {
        return OpenAsset(fileAsset, mode, MediaLibraryApi::API_10, isMovingPhotoVideo, type);
    }
    return OpenAsset(fileAsset, mode, cmd.GetApi(), type);
}

int32_t MediaLibraryPhotoOperations::Close(MediaLibraryCommand &cmd)
{
    const ValuesBucket &values = cmd.GetValueBucket();
    string uriString;
    if (!GetStringFromValuesBucket(values, MEDIA_DATA_DB_URI, uriString)) {
        return E_INVALID_VALUES;
    }
    string pendingStatus = cmd.GetQuerySetParam(MediaColumn::MEDIA_TIME_PENDING);

    shared_ptr<FileAsset> fileAsset = GetFileAssetByUri(uriString, true, PHOTO_COLUMN_VECTOR, pendingStatus);
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

static inline void SetPhotoTypeByRelativePath(const string &relativePath, FileAsset &fileAsset)
{
    int32_t subType = static_cast<int32_t>(PhotoSubType::DEFAULT);
    if (relativePath.compare(CAMERA_PATH) == 0) {
        subType = static_cast<int32_t>(PhotoSubType::CAMERA);
    } else if (relativePath.compare(SCREEN_RECORD_PATH) == 0 ||
        relativePath.compare(SCREEN_SHOT_PATH) == 0) {
        subType = static_cast<int32_t>(PhotoSubType::SCREENSHOT);
    }

    fileAsset.SetPhotoSubType(subType);
}

static inline void SetPhotoSubTypeFromCmd(MediaLibraryCommand &cmd, FileAsset &fileAsset)
{
    int32_t subType = static_cast<int32_t>(PhotoSubType::DEFAULT);
    ValueObject value;
    if (cmd.GetValueBucket().GetObject(PhotoColumn::PHOTO_SUBTYPE, value)) {
        value.GetInt(subType);
    }
    fileAsset.SetPhotoSubType(subType);
}

static inline void SetCameraShotKeyFromCmd(MediaLibraryCommand &cmd, FileAsset &fileAsset)
{
    string cameraShotKey;
    ValueObject value;
    if (cmd.GetValueBucket().GetObject(PhotoColumn::CAMERA_SHOT_KEY, value)) {
        value.GetString(cameraShotKey);
    }
    fileAsset.SetCameraShotKey(cameraShotKey);
}

static inline int32_t GetCallingUid(MediaLibraryCommand &cmd)
{
    int32_t callingUid = 0;
    ValueObject value;
    if (cmd.GetValueBucket().GetObject(MEDIA_DATA_CALLING_UID, value)) {
        value.GetInt(callingUid);
    }
    return callingUid;
}

static inline void SetCallingPackageName(MediaLibraryCommand &cmd, FileAsset &fileAsset)
{
    int32_t callingUid = GetCallingUid(cmd);
    if (callingUid == 0) {
        return;
    }
    string bundleName;
    PermissionUtils::GetClientBundle(callingUid, bundleName);
    fileAsset.SetOwnerPackage(bundleName);
    string packageName;
    PermissionUtils::GetPackageName(callingUid, packageName);
    fileAsset.SetPackageName(packageName);
}

int32_t MediaLibraryPhotoOperations::CreateV9(MediaLibraryCommand& cmd)
{
    FileAsset fileAsset;
    ValuesBucket &values = cmd.GetValueBucket();

    string displayName;
    CHECK_AND_RETURN_RET(GetStringFromValuesBucket(values, PhotoColumn::MEDIA_NAME, displayName),
        E_HAS_DB_ERROR);
    fileAsset.SetDisplayName(displayName);

    string relativePath;
    CHECK_AND_RETURN_RET(GetStringFromValuesBucket(values, PhotoColumn::MEDIA_RELATIVE_PATH, relativePath),
        E_HAS_DB_ERROR);
    fileAsset.SetRelativePath(relativePath);
    MediaFileUtils::FormatRelativePath(relativePath);

    int32_t mediaType = 0;
    CHECK_AND_RETURN_RET(GetInt32FromValuesBucket(values, PhotoColumn::MEDIA_TYPE, mediaType),
        E_HAS_DB_ERROR);
    fileAsset.SetMediaType(static_cast<MediaType>(mediaType));

    int32_t errCode = CheckRelativePathWithType(relativePath, mediaType);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Failed to Check RelativePath and Extention, "
        "relativePath=%{private}s, mediaType=%{public}d", relativePath.c_str(), mediaType);
    SetPhotoTypeByRelativePath(relativePath, fileAsset);
    errCode = CheckDisplayNameWithType(displayName, mediaType);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Failed to Check Dir and Extention, "
        "displayName=%{private}s, mediaType=%{public}d", displayName.c_str(), mediaType);
    std::shared_ptr<TransactionOperations> trans = make_shared<TransactionOperations>(__func__);
    int32_t outRow = -1;
    std::function<int(void)> func = [&]()->int {
        errCode = SetAssetPathInCreate(fileAsset, trans);
        if (errCode != E_OK) {
            MEDIA_ERR_LOG("Failed to Solve FileAsset Path and Name, displayName=%{private}s", displayName.c_str());
            return errCode;
        }

        outRow = InsertAssetInDb(trans, cmd, fileAsset);
        if (outRow <= 0) {
            MEDIA_ERR_LOG("insert file in db failed, error = %{public}d", outRow);
            return E_HAS_DB_ERROR;
        }
        return errCode;
    };
    errCode = trans->RetryTrans(func);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("CreateV9: tans finish fail!, ret:%{public}d", errCode);
        return errCode;
    }
    return outRow;
}

void PhotosAddAsset(const int &albumId, const string &assetId, const string &extrUri)
{
    auto watch = MediaLibraryNotify::GetInstance();
    watch->Notify(MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX, assetId, extrUri),
        NotifyType::NOTIFY_ALBUM_ADD_ASSET, albumId);
}

void MediaLibraryPhotoOperations::SolvePhotoAlbumInCreate(MediaLibraryCommand &cmd, FileAsset &fileAsset)
{
    ValuesBucket &values = cmd.GetValueBucket();
    string albumUri;
    GetStringFromValuesBucket(values, MEDIA_DATA_DB_ALBUM_ID, albumUri);
    if (!albumUri.empty()) {
        PhotosAddAsset(stoi(MediaFileUtils::GetIdFromUri(albumUri)), to_string(fileAsset.GetId()),
            MediaFileUtils::GetExtraUri(fileAsset.GetDisplayName(), fileAsset.GetPath()));
    }
}

static void SetAssetDisplayName(const string &displayName, FileAsset &fileAsset, bool &isContains)
{
    fileAsset.SetDisplayName(displayName);
    isContains = true;
}

int32_t MediaLibraryPhotoOperations::CreateV10(MediaLibraryCommand& cmd)
{
    FileAsset fileAsset;
    ValuesBucket &values = cmd.GetValueBucket();
    string displayName;
    string extention;
    string title;
    bool isContains = false;
    bool isNeedGrant = false;
    if (GetStringFromValuesBucket(values, PhotoColumn::MEDIA_NAME, displayName)) {
        SetAssetDisplayName(displayName, fileAsset, isContains);
        fileAsset.SetTimePending(UNCREATE_FILE_TIMEPENDING);
    } else {
        CHECK_AND_RETURN_RET(GetStringFromValuesBucket(values, ASSET_EXTENTION, extention), E_HAS_DB_ERROR);
        isNeedGrant = true;
        fileAsset.SetTimePending(UNOPEN_FILE_COMPONENT_TIMEPENDING);
        if (GetStringFromValuesBucket(values, PhotoColumn::MEDIA_TITLE, title)) {
            displayName = title + "." + extention;
            SetAssetDisplayName(displayName, fileAsset, isContains);
        }
    }
    int32_t mediaType = 0;
    CHECK_AND_RETURN_RET(GetInt32FromValuesBucket(values, PhotoColumn::MEDIA_TYPE, mediaType), E_HAS_DB_ERROR);
    fileAsset.SetMediaType(static_cast<MediaType>(mediaType));
    SetPhotoSubTypeFromCmd(cmd, fileAsset);
    SetCameraShotKeyFromCmd(cmd, fileAsset);
    SetCallingPackageName(cmd, fileAsset);
    // Check rootdir and extention
    int32_t errCode = CheckWithType(isContains, displayName, extention, mediaType);
    CHECK_AND_RETURN_RET(errCode == E_OK, errCode);
    std::shared_ptr<TransactionOperations> trans = make_shared<TransactionOperations>(__func__);
    int32_t outRow = -1;
    std::function<int(void)> func = [&]()->int {
        errCode = isContains ? SetAssetPathInCreate(fileAsset, trans) :
            SetAssetPath(fileAsset, extention, trans);
        CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Failed to Set Path, Name=%{private}s", displayName.c_str());
        outRow = InsertAssetInDb(trans, cmd, fileAsset);
        AuditLog auditLog = { true, "USER BEHAVIOR", "ADD", "io", 1, "running", "ok" };
        HiAudit::GetInstance().Write(auditLog);
        CHECK_AND_RETURN_RET_LOG(outRow > 0, E_HAS_DB_ERROR, "insert file in db failed, error = %{public}d", outRow);
        fileAsset.SetId(outRow);
        SolvePhotoAlbumInCreate(cmd, fileAsset);
        return errCode;
    };
    errCode = trans->RetryTrans(func);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("CreateV10: trans retry fail!, ret:%{public}d", errCode);
        return errCode;
    }
    string fileUri = CreateExtUriForV10Asset(fileAsset);
    if (isNeedGrant) {
        bool isMovingPhoto = fileAsset.GetPhotoSubType() == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO);
        int32_t ret = GrantUriPermission(fileUri, cmd.GetBundleName(), fileAsset.GetPath(), isMovingPhoto);
        CHECK_AND_RETURN_RET(ret == E_OK, ret);
    }
    cmd.SetResult(fileUri);
    return outRow;
}

int32_t MediaLibraryPhotoOperations::DeletePhoto(const shared_ptr<FileAsset> &fileAsset, MediaLibraryApi api)
{
    string filePath = fileAsset->GetPath();
    CHECK_AND_RETURN_RET_LOG(!filePath.empty(), E_INVALID_PATH, "get file path failed");
    bool res = MediaFileUtils::DeleteFile(filePath);
    CHECK_AND_RETURN_RET_LOG(res, E_HAS_FS_ERROR, "Delete photo file failed, errno: %{public}d", errno);

    // delete thumbnail
    int32_t fileId = fileAsset->GetId();
    InvalidateThumbnail(to_string(fileId), fileAsset->GetMediaType());

    string displayName = fileAsset->GetDisplayName();
    // delete file in db
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::DELETE);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    int32_t deleteRows = DeleteAssetInDb(cmd);
    if (deleteRows <= 0) {
        MEDIA_ERR_LOG("Delete photo in database failed, errCode=%{public}d", deleteRows);
        return E_HAS_DB_ERROR;
    }

    auto watch = MediaLibraryNotify::GetInstance();
    string notifyDeleteUri =
        MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX, to_string(deleteRows),
        (api == MediaLibraryApi::API_10 ? MediaFileUtils::GetExtraUri(displayName, filePath) : ""));
    watch->Notify(notifyDeleteUri, NotifyType::NOTIFY_REMOVE);

    DeleteRevertMessage(filePath);
    return deleteRows;
}

void MediaLibraryPhotoOperations::TrashPhotosSendNotify(vector<string> &notifyUris)
{
    auto watch = MediaLibraryNotify::GetInstance();
    int trashAlbumId = watch->GetAlbumIdBySubType(PhotoAlbumSubType::TRASH);
    if (trashAlbumId <= 0) {
        MEDIA_ERR_LOG("Skip to send trash photos notify, trashAlbumId=%{public}d", trashAlbumId);
        return;
    }
    if (notifyUris.empty()) {
        MEDIA_ERR_LOG("Skip to send trash photos notify, notifyUris is empty.");
        return;
    }
    if (notifyUris.size() == 1) {
        watch->Notify(notifyUris[0], NotifyType::NOTIFY_REMOVE);
        watch->Notify(notifyUris[0], NotifyType::NOTIFY_ALBUM_REMOVE_ASSET);
        watch->Notify(notifyUris[0], NotifyType::NOTIFY_ALBUM_ADD_ASSET, trashAlbumId);
    } else {
        watch->Notify(PhotoColumn::PHOTO_URI_PREFIX, NotifyType::NOTIFY_REMOVE);
        watch->Notify(PhotoAlbumColumns::ALBUM_URI_PREFIX, NotifyType::NOTIFY_UPDATE);
    }
    vector<int64_t> formIds;
    for (const auto &notifyUri : notifyUris) {
        MediaLibraryFormMapOperations::GetFormMapFormId(notifyUri.c_str(), formIds);
    }
    if (!formIds.empty()) {
        MediaLibraryFormMapOperations::PublishedChange("", formIds, false);
    }
}

void MediaLibraryPhotoOperations::UpdateSourcePath(const vector<string> &whereArgs)
{
    if (whereArgs.empty()) {
        return;
    }

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("RdbStore is null");
        return;
    }

    std::string inClause;
    for (size_t i = 0; i < whereArgs.size(); ++i) {
        if (i > 0) {
            inClause += ",";
        }
        inClause += whereArgs[i];
    }

    const std::string updateSql = "UPDATE Photos"
        " SET source_path = ( SELECT"
        " '/storage/emulated/0' || COALESCE(PhotoAlbum.lpath, '/Pictures/其它') || '/' || SubPhotos.display_name "
        " FROM Photos AS SubPhotos "
        " LEFT JOIN PhotoAlbum ON SubPhotos.owner_album_id = PhotoAlbum.album_id "
        " WHERE SubPhotos.file_id = Photos.file_id"
        " LIMIT 1"
        ") "
        "WHERE file_id IN (" + inClause + ")";

    int32_t result = rdbStore->ExecuteSql(updateSql);
    if (result != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to update source path, error code: %{private}d", result);
    }
}

int32_t MediaLibraryPhotoOperations::TrashPhotos(MediaLibraryCommand &cmd)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("Failed to get rdb store.");
        return E_HAS_DB_ERROR;
    }

    NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(cmd.GetDataSharePred(),
        PhotoColumn::PHOTOS_TABLE);
    vector<string> notifyUris = rdbPredicate.GetWhereArgs();
    MediaLibraryRdbStore::ReplacePredicatesUriToId(rdbPredicate);

    MultiStagesCaptureManager::RemovePhotos(rdbPredicate, true);

    UpdateSourcePath(rdbPredicate.GetWhereArgs());
    ValuesBucket values;
    values.Put(MediaColumn::MEDIA_DATE_TRASHED, MediaFileUtils::UTCTimeMilliSeconds());
    cmd.SetValueBucket(values);
    int32_t updatedRows = rdbStore->UpdateWithDateTime(values, rdbPredicate);
    if (updatedRows < 0) {
        MEDIA_ERR_LOG("Trash photo failed. Result %{public}d.", updatedRows);
        return E_HAS_DB_ERROR;
    }
    // delete cloud enhanacement task
    vector<string> ids = rdbPredicate.GetWhereArgs();
    vector<string> photoIds;
    EnhancementManager::GetInstance().CancelTasksInternal(ids, photoIds, CloudEnhancementAvailableType::TRASH);
    for (string& photoId : photoIds) {
        CloudEnhancementGetCount::GetInstance().Report("DeleteCancellationType", photoId);
    }
    MediaAnalysisHelper::StartMediaAnalysisServiceAsync(
        static_cast<int32_t>(MediaAnalysisProxy::ActivateServiceType::START_UPDATE_INDEX), notifyUris);
    MediaLibraryRdbUtils::UpdateAllAlbums(rdbStore, notifyUris);
    if (static_cast<size_t>(updatedRows) != notifyUris.size()) {
        MEDIA_WARN_LOG("Try to notify %{public}zu items, but only %{public}d items updated.",
            notifyUris.size(), updatedRows);
    }
    TrashPhotosSendNotify(notifyUris);
    DfxManager::GetInstance()->HandleDeleteBehavior(DfxType::TRASH_PHOTO, updatedRows, notifyUris);
    return updatedRows;
}

static int32_t DiscardCameraPhoto(MediaLibraryCommand &cmd)
{
    NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(cmd.GetDataSharePred(),
        PhotoColumn::PHOTOS_TABLE);
    vector<string> notifyUris = rdbPredicate.GetWhereArgs();
    MediaLibraryRdbStore::ReplacePredicatesUriToId(rdbPredicate);
    return MediaLibraryAssetOperations::DeleteFromDisk(rdbPredicate, false, true);
}

static string GetUriWithoutSeg(const string &oldUri)
{
    size_t questionMaskPoint = oldUri.rfind('?');
    if (questionMaskPoint != string::npos) {
        return oldUri.substr(0, questionMaskPoint);
    }
    return oldUri;
}

static int32_t UpdateDirtyWithoutIsTemp(RdbPredicates &predicates)
{
    ValuesBucket valuesBucketDirty;
    valuesBucketDirty.Put(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_NEW));
    int32_t updateDirtyRows = MediaLibraryRdbStore::UpdateWithDateTime(valuesBucketDirty, predicates);
    return updateDirtyRows;
}

static int32_t UpdateIsTempAndDirty(MediaLibraryCommand &cmd, const string &fileId)
{
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, fileId);
    ValuesBucket values;
    values.Put(PhotoColumn::PHOTO_IS_TEMP, false);

    int32_t updateDirtyRows = 0;
    if (cmd.GetQuerySetParam(PhotoColumn::PHOTO_DIRTY) == to_string(static_cast<int32_t>(DirtyType::TYPE_NEW))) {
        // Only third-party app save photo, it will bring dirty flag
        // The photo saved by third-party apps, whether of low or high quality, should set dirty to TYPE_NEW
        // Every subtype of photo saved by third-party apps, should set dirty to TYPE_NEW
        // Need to change the quality to high quality before updating
        values.Put(PhotoColumn::PHOTO_QUALITY, static_cast<int32_t>(MultiStagesPhotoQuality::FULL));
        values.Put(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_NEW));
        updateDirtyRows = MediaLibraryRdbStore::UpdateWithDateTime(values, predicates);
        if (updateDirtyRows < 0) {
            MEDIA_ERR_LOG("update third party photo temp and dirty flag fail.");
            return E_ERR;
        }
        return updateDirtyRows;
    }

    string subTypeStr = cmd.GetQuerySetParam(PhotoColumn::PHOTO_SUBTYPE);
    if (subTypeStr.empty()) {
        MEDIA_ERR_LOG("get subType fail");
        return E_ERR;
    }
    int32_t subType = stoi(subTypeStr);
    if (subType == static_cast<int32_t>(PhotoSubType::BURST)) {
        predicates.EqualTo(PhotoColumn::PHOTO_QUALITY, to_string(static_cast<int32_t>(MultiStagesPhotoQuality::FULL)));
        predicates.EqualTo(PhotoColumn::PHOTO_SUBTYPE, to_string(static_cast<int32_t>(PhotoSubType::BURST)));
        values.Put(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_NEW));
        updateDirtyRows = MediaLibraryRdbStore::UpdateWithDateTime(values, predicates);
        if (updateDirtyRows < 0) {
            MEDIA_ERR_LOG("burst photo update temp and dirty flag fail.");
            return E_ERR;
        }
        return updateDirtyRows;
    }

    int32_t updateIsTempRows = MediaLibraryRdbStore::UpdateWithDateTime(values, predicates);
    if (updateIsTempRows < 0) {
        MEDIA_ERR_LOG("update temp flag fail.");
        return E_ERR;
    }
    if (subType != static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)) {
        predicates.EqualTo(PhotoColumn::PHOTO_QUALITY, to_string(static_cast<int32_t>(MultiStagesPhotoQuality::FULL)));
        predicates.NotEqualTo(PhotoColumn::PHOTO_SUBTYPE, to_string(static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)));
        updateDirtyRows = UpdateDirtyWithoutIsTemp(predicates);
        if (updateDirtyRows < 0) {
            MEDIA_ERR_LOG("update dirty flag fail.");
            return E_ERR;
        }
    }
    return updateDirtyRows;
}

int32_t MediaLibraryPhotoOperations::SaveCameraPhoto(MediaLibraryCommand &cmd)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryPhotoOperations::SaveCameraPhoto");
    string fileId = cmd.GetQuerySetParam(PhotoColumn::MEDIA_ID);
    if (fileId.empty()) {
        MEDIA_ERR_LOG("SaveCameraPhoto, get fileId fail");
        return 0;
    }
    MEDIA_INFO_LOG("start SaveCameraPhoto, fileId: %{public}s", fileId.c_str());
    int32_t ret = UpdateIsTempAndDirty(cmd, fileId);
    if (ret < 0) {
        return 0;
    }

    string fileType = cmd.GetQuerySetParam(IMAGE_FILE_TYPE);
    if (!fileType.empty()) {
        SavePicture(stoi(fileType), stoi(fileId));
    }

    string needScanStr = cmd.GetQuerySetParam(MEDIA_OPERN_KEYWORD);
    shared_ptr<FileAsset> fileAsset = GetFileAssetFromDb(PhotoColumn::MEDIA_ID, fileId,
                                                         OperationObject::FILESYSTEM_PHOTO, PHOTO_COLUMN_VECTOR);
    if (fileAsset == nullptr) {
        MEDIA_ERR_LOG("SaveCameraPhoto, get fileAsset fail");
        return 0;
    }
    string path = fileAsset->GetPath();
    int32_t burstCoverLevel = fileAsset->GetBurstCoverLevel();
    if (!path.empty()) {
        if (burstCoverLevel == static_cast<int32_t>(BurstCoverLevelType::COVER)) {
            ScanFile(path, false, true, true, stoi(fileId));
        } else {
            MediaLibraryAssetOperations::ScanFileWithoutAlbumUpdate(path, false, true, true, stoi(fileId));
        }
    }
    MEDIA_INFO_LOG("SaveCameraPhoto Success, ret: %{public}d, needScanStr: %{public}s", ret, needScanStr.c_str());
    return ret;
}

int32_t MediaLibraryPhotoOperations::SetVideoEnhancementAttr(MediaLibraryCommand &cmd)
{
    string videoId = cmd.GetQuerySetParam(PhotoColumn::PHOTO_ID);
    string fileId = cmd.GetQuerySetParam(MediaColumn::MEDIA_ID);
    string filePath = cmd.GetQuerySetParam(MediaColumn::MEDIA_FILE_PATH);
    MultiStagesVideoCaptureManager::GetInstance().AddVideo(videoId, fileId, filePath);
    return E_OK;
}

static int32_t GetHiddenState(const ValuesBucket &values)
{
    ValueObject obj;
    auto ret = values.GetObject(MediaColumn::MEDIA_HIDDEN, obj);
    if (!ret) {
        return E_INVALID_VALUES;
    }
    int32_t hiddenState = 0;
    ret = obj.GetInt(hiddenState);
    if (ret != E_OK) {
        return E_INVALID_VALUES;
    }
    return hiddenState == 0 ? 0 : 1;
}

static void SendHideNotify(vector<string> &notifyUris, const int32_t hiddenState)
{
    auto watch = MediaLibraryNotify::GetInstance();
    if (watch == nullptr) {
        return;
    }
    int hiddenAlbumId = watch->GetAlbumIdBySubType(PhotoAlbumSubType::HIDDEN);
    if (hiddenAlbumId <= 0) {
        return;
    }

    NotifyType assetNotifyType;
    NotifyType albumNotifyType;
    NotifyType hiddenAlbumNotifyType;
    if (hiddenState > 0) {
        assetNotifyType = NotifyType::NOTIFY_REMOVE;
        albumNotifyType = NotifyType::NOTIFY_ALBUM_REMOVE_ASSET;
        hiddenAlbumNotifyType = NotifyType::NOTIFY_ALBUM_ADD_ASSET;
    } else {
        assetNotifyType = NotifyType::NOTIFY_ADD;
        albumNotifyType = NotifyType::NOTIFY_ALBUM_ADD_ASSET;
        hiddenAlbumNotifyType = NotifyType::NOTIFY_ALBUM_REMOVE_ASSET;
    }
    vector<int64_t> formIds;
    for (const auto &notifyUri : notifyUris) {
        watch->Notify(notifyUri, assetNotifyType);
        watch->Notify(notifyUri, albumNotifyType, 0, true);
        watch->Notify(notifyUri, hiddenAlbumNotifyType, hiddenAlbumId);
        MediaLibraryFormMapOperations::GetFormMapFormId(notifyUri.c_str(), formIds);
    }
    if (!formIds.empty()) {
        MediaLibraryFormMapOperations::PublishedChange("", formIds, false);
    }
}

static int32_t HidePhotos(MediaLibraryCommand &cmd)
{
    int32_t hiddenState = GetHiddenState(cmd.GetValueBucket());
    if (hiddenState < 0) {
        return hiddenState;
    }

    RdbPredicates predicates = RdbUtils::ToPredicates(cmd.GetDataSharePred(), PhotoColumn::PHOTOS_TABLE);
    vector<string> notifyUris = predicates.GetWhereArgs();
    MediaLibraryRdbStore::ReplacePredicatesUriToId(predicates);
    if (hiddenState != 0) {
        MediaLibraryPhotoOperations::UpdateSourcePath(predicates.GetWhereArgs());
    } else {
        MediaLibraryAlbumOperations::DealwithNoAlbumAssets(predicates.GetWhereArgs());
    }
    ValuesBucket values;
    values.Put(MediaColumn::MEDIA_HIDDEN, hiddenState);
    if (predicates.GetTableName() == PhotoColumn::PHOTOS_TABLE) {
        values.PutLong(PhotoColumn::PHOTO_HIDDEN_TIME,
            hiddenState ? MediaFileUtils::UTCTimeMilliSeconds() : 0);
    }
    int32_t changedRows = MediaLibraryRdbStore::UpdateWithDateTime(values, predicates);
    if (changedRows < 0) {
        return changedRows;
    }
    MediaAnalysisHelper::StartMediaAnalysisServiceAsync(
        static_cast<int32_t>(MediaAnalysisProxy::ActivateServiceType::START_UPDATE_INDEX), notifyUris);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    MediaLibraryRdbUtils::UpdateAllAlbums(rdbStore, notifyUris);
    SendHideNotify(notifyUris, hiddenState);
    return changedRows;
}

static int32_t GetFavoriteState(const ValuesBucket& values)
{
    ValueObject obj;
    bool isValid = values.GetObject(PhotoColumn::MEDIA_IS_FAV, obj);
    if (!isValid) {
        return E_INVALID_VALUES;
    }

    int32_t favoriteState = -1;
    int ret = obj.GetInt(favoriteState);
    if (ret != E_OK || (favoriteState != 0 && favoriteState != 1)) {
        return E_INVALID_VALUES;
    }
    return favoriteState;
}

static int32_t BatchSetFavorite(MediaLibraryCommand& cmd)
{
    int32_t favoriteState = GetFavoriteState(cmd.GetValueBucket());
    CHECK_AND_RETURN_RET_LOG(favoriteState >= 0, favoriteState, "Failed to get favoriteState");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "Failed to get rdbStore");

    RdbPredicates predicates = RdbUtils::ToPredicates(cmd.GetDataSharePred(), PhotoColumn::PHOTOS_TABLE);
    vector<string> notifyUris = predicates.GetWhereArgs();
    MediaLibraryRdbStore::ReplacePredicatesUriToId(predicates);
    ValuesBucket values;
    values.Put(PhotoColumn::MEDIA_IS_FAV, favoriteState);
    int32_t updatedRows = rdbStore->UpdateWithDateTime(values, predicates);
    CHECK_AND_RETURN_RET_LOG(updatedRows >= 0, E_HAS_DB_ERROR, "Failed to set favorite, err: %{public}d", updatedRows);

    MediaLibraryRdbUtils::UpdateSystemAlbumInternal(rdbStore, { to_string(PhotoAlbumSubType::FAVORITE) });
    MediaLibraryRdbUtils::UpdateSysAlbumHiddenState(rdbStore, { to_string(PhotoAlbumSubType::FAVORITE) });

    auto watch = MediaLibraryNotify::GetInstance();
    int favAlbumId = watch->GetAlbumIdBySubType(PhotoAlbumSubType::FAVORITE);
    if (favAlbumId > 0) {
        NotifyType type = favoriteState ? NotifyType::NOTIFY_ALBUM_ADD_ASSET : NotifyType::NOTIFY_ALBUM_REMOVE_ASSET;
        for (const string& notifyUri : notifyUris) {
            watch->Notify(notifyUri, type, favAlbumId);
        }
    } else {
        MEDIA_WARN_LOG("Favorite album not found, failed to notify favorite album.");
    }

    for (const string& notifyUri : notifyUris) {
        watch->Notify(notifyUri, NotifyType::NOTIFY_UPDATE);
    }

    if (static_cast<size_t>(updatedRows) != notifyUris.size()) {
        MEDIA_WARN_LOG(
            "Try to notify %{public}zu items, but only %{public}d items updated.", notifyUris.size(), updatedRows);
    }
    return updatedRows;
}

int32_t MediaLibraryPhotoOperations::BatchSetUserComment(MediaLibraryCommand& cmd)
{
    vector<shared_ptr<FileAsset>> fileAssetVector;
    vector<string> columns = { PhotoColumn::MEDIA_ID, PhotoColumn::MEDIA_FILE_PATH,
        PhotoColumn::MEDIA_TYPE, PhotoColumn::MEDIA_NAME };
    MediaLibraryRdbStore::ReplacePredicatesUriToId(*(cmd.GetAbsRdbPredicates()));
    int32_t errCode = GetFileAssetVectorFromDb(*(cmd.GetAbsRdbPredicates()),
        OperationObject::FILESYSTEM_PHOTO, fileAssetVector, columns);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode,
        "Failed to query file asset vector from db, errCode=%{private}d", errCode);

    for (const auto& fileAsset : fileAssetVector) {
        errCode = SetUserComment(cmd, fileAsset);
        CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Failed to set user comment, errCode=%{private}d", errCode);
    }

    int32_t updateRows = UpdateFileInDb(cmd);
    if (updateRows < 0) {
        MEDIA_ERR_LOG("Update Photo in database failed, updateRows=%{public}d", updateRows);
        return updateRows;
    }

    auto watch = MediaLibraryNotify::GetInstance();
    for (const auto& fileAsset : fileAssetVector) {
        string extraUri = MediaFileUtils::GetExtraUri(fileAsset->GetDisplayName(), fileAsset->GetPath());
        string assetUri = MediaFileUtils::GetUriByExtrConditions(
            PhotoColumn::PHOTO_URI_PREFIX, to_string(fileAsset->GetId()), extraUri);
        watch->Notify(assetUri, NotifyType::NOTIFY_UPDATE);
    }
    return updateRows;
}

static const std::unordered_map<int, int> ORIENTATION_MAP = {
    {0, ORIENTATION_0},
    {90, ORIENTATION_90},
    {180, ORIENTATION_180},
    {270, ORIENTATION_270}
};

static int32_t CreateImageSource(
    const std::shared_ptr<FileAsset> &fileAsset, std::unique_ptr<ImageSource> &imageSource)
{
    if (fileAsset == nullptr) {
        MEDIA_ERR_LOG("fileAsset is null");
        return E_INVALID_VALUES;
    }

    string filePath = fileAsset->GetFilePath();
    string extension = MediaFileUtils::GetExtensionFromPath(filePath);
    SourceOptions opts;
    opts.formatHint = "image/" + extension;
    uint32_t err = E_OK;
    imageSource = ImageSource::CreateImageSource(filePath, opts, err);
    if (err != E_OK || imageSource == nullptr) {
        MEDIA_ERR_LOG("Failed to obtain image exif, err = %{public}d", err);
        return E_INVALID_VALUES;
    }
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::UpdateOrientationAllExif(
    MediaLibraryCommand &cmd, const std::shared_ptr<FileAsset> &fileAsset, string &currentOrientation)
{
    std::unique_ptr<ImageSource> imageSource;
    uint32_t err = E_OK;
    if (CreateImageSource(fileAsset, imageSource) != E_OK) {
        return E_INVALID_VALUES;
    }

    err = imageSource->GetImagePropertyString(0, PHOTO_DATA_IMAGE_ORIENTATION, currentOrientation);
    if (err != E_OK) {
        currentOrientation = "";
        MEDIA_WARN_LOG("The rotation angle exlf of the image is empty");
    }

    MEDIA_INFO_LOG("Update image exif information, DisplayName=%{private}s, Orientation=%{private}d",
        fileAsset->GetDisplayName().c_str(), fileAsset->GetOrientation());

    ValuesBucket &values = cmd.GetValueBucket();
    ValueObject valueObject;
    if (!values.GetObject(PhotoColumn::PHOTO_ORIENTATION, valueObject)) {
        return E_OK;
    }
    int32_t cmdOrientation;
    valueObject.GetInt(cmdOrientation);

    auto imageSourceOrientation = ORIENTATION_MAP.find(cmdOrientation);
    if (imageSourceOrientation == ORIENTATION_MAP.end()) {
        MEDIA_ERR_LOG("imageSourceOrientation value is invalid.");
        return E_INVALID_VALUES;
    }

    string exifStr = fileAsset->GetAllExif();
    if (!exifStr.empty() && nlohmann::json::accept(exifStr)) {
        nlohmann::json exifJson = nlohmann::json::parse(exifStr, nullptr, false);
        exifJson["Orientation"] = imageSourceOrientation->second;
        exifStr = exifJson.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
        values.PutString(PhotoColumn::PHOTO_ALL_EXIF, exifStr);
    }

    err = imageSource->ModifyImageProperty(0, PHOTO_DATA_IMAGE_ORIENTATION,
        std::to_string(imageSourceOrientation->second), fileAsset->GetFilePath());
    if (err != E_OK) {
        MEDIA_ERR_LOG("Modify image property all exif failed, err = %{public}d", err);
        return E_INVALID_VALUES;
    }
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::UpdateOrientationExif(MediaLibraryCommand &cmd,
    const shared_ptr<FileAsset> &fileAsset, bool &orientationUpdated, string &currentOrientation)
{
    if (fileAsset == nullptr) {
        MEDIA_ERR_LOG("fileAsset is null");
        return E_INVALID_VALUES;
    }
    ValuesBucket &values = cmd.GetValueBucket();
    ValueObject valueObject;
    if (!values.GetObject(PhotoColumn::PHOTO_ORIENTATION, valueObject)) {
        MEDIA_INFO_LOG("No need update orientation.");
        return E_OK;
    }
    if ((fileAsset->GetMediaType() != MEDIA_TYPE_IMAGE) ||
        (fileAsset->GetPhotoSubType() == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO))) {
        MEDIA_ERR_LOG("Only images support rotation.");
        return E_INVALID_VALUES;
    }
    int32_t errCode = UpdateOrientationAllExif(cmd, fileAsset, currentOrientation);
    if (errCode == E_OK) {
        orientationUpdated = true;
    }
    return errCode;
}

void UpdateAlbumOnSystemMoveAssets(const int32_t &oriAlbumId, const int32_t &targetAlbumId)
{
    MediaLibraryRdbUtils::UpdateUserAlbumInternal(
        MediaLibraryUnistoreManager::GetInstance().GetRdbStore(),
        { to_string(oriAlbumId), to_string(targetAlbumId) });
    MediaLibraryRdbUtils::UpdateSourceAlbumInternal(
        MediaLibraryUnistoreManager::GetInstance().GetRdbStore(),
        { to_string(oriAlbumId), to_string(targetAlbumId) });
}

bool IsSystemAlbumMovement(MediaLibraryCommand &cmd)
{
    auto whereClause = cmd.GetAbsRdbPredicates()->GetWhereClause();
    auto whereArgs = cmd.GetAbsRdbPredicates()->GetWhereArgs();
    int32_t oriAlbumId = MediaLibraryAssetOperations::GetAlbumIdByPredicates(whereClause, whereArgs);
    PhotoAlbumType type;
    PhotoAlbumSubType subType;
    CHECK_AND_RETURN_RET_LOG(GetAlbumTypeSubTypeById(to_string(oriAlbumId), type, subType) == E_SUCCESS, false,
        "move assets invalid album uri");
    if ((!PhotoAlbum::CheckPhotoAlbumType(type)) || (!PhotoAlbum::CheckPhotoAlbumSubType(subType))) {
        MEDIA_ERR_LOG("album id %{private}s type:%d subtype:%d", to_string(oriAlbumId).c_str(), type, subType);
        return false;
    }

    return type == PhotoAlbumType::SYSTEM;
}

void GetSystemMoveAssets(AbsRdbPredicates &predicates)
{
    const vector<string> &whereUriArgs = predicates.GetWhereArgs();
    if (whereUriArgs.size() <= 1) {
        MEDIA_ERR_LOG("Move vector empty when move from system album");
        return;
    }
    vector<string> whereIdArgs;
    whereIdArgs.reserve(whereUriArgs.size() - 1);
    for (size_t i = 1; i < whereUriArgs.size(); ++i) {
        whereIdArgs.push_back(whereUriArgs[i]);
    }
    predicates.SetWhereArgs(whereIdArgs);
}

int32_t PrepareUpdateArgs(MediaLibraryCommand &cmd, std::map<int32_t, std::vector<int32_t>> &ownerAlbumIds,
    vector<string> &assetString, RdbPredicates &predicates)
{
    auto assetVector = cmd.GetAbsRdbPredicates()->GetWhereArgs();
    for (const auto &fileAsset : assetVector) {
        assetString.push_back(fileAsset);
    }

    predicates.And()->In(PhotoColumn::MEDIA_ID, assetString);
    vector<string> columns = { PhotoColumn::PHOTO_OWNER_ALBUM_ID, PhotoColumn::MEDIA_ID };
    auto resultSetQuery = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    if (resultSetQuery == nullptr) {
        MEDIA_ERR_LOG("album id is not exist");
        return E_INVALID_ARGUMENTS;
    }
    while (resultSetQuery->GoToNextRow() == NativeRdb::E_OK) {
        int32_t albumId = 0;
        albumId = GetInt32Val(PhotoColumn::PHOTO_OWNER_ALBUM_ID, resultSetQuery);
        ownerAlbumIds[albumId].push_back(GetInt32Val(MediaColumn::MEDIA_ID, resultSetQuery));
    }
    resultSetQuery->Close();
    return E_OK;
}

int32_t UpdateSystemRows(MediaLibraryCommand &cmd)
{
    std::map<int32_t, std::vector<int32_t>> ownerAlbumIds;
    vector<string> assetString;
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    if (PrepareUpdateArgs(cmd, ownerAlbumIds, assetString, predicates) != E_OK) {
        return E_INVALID_ARGUMENTS;
    }

    ValueObject value;
    int32_t targetAlbumId = 0;
    if (!cmd.GetValueBucket().GetObject(PhotoColumn::PHOTO_OWNER_ALBUM_ID, value)) {
        MEDIA_ERR_LOG("get owner album id fail when move from system album");
        return E_INVALID_ARGUMENTS;
    }
    value.GetInt(targetAlbumId);
    ValuesBucket values;
    values.Put(PhotoColumn::PHOTO_OWNER_ALBUM_ID, to_string(targetAlbumId));
    int32_t changedRows = MediaLibraryRdbStore::UpdateWithDateTime(values, predicates);
    if (changedRows < 0) {
        MEDIA_ERR_LOG("Update owner album id fail when move from system album");
        return changedRows;
    }
    if (!assetString.empty()) {
        MediaAnalysisHelper::AsyncStartMediaAnalysisService(
            static_cast<int32_t>(MediaAnalysisProxy::ActivateServiceType::START_UPDATE_INDEX), assetString);
    }
    for (auto it = ownerAlbumIds.begin(); it != ownerAlbumIds.end(); it++) {
        MEDIA_INFO_LOG("System album move assets target album id is: %{public}s", to_string(it->first).c_str());
        int32_t oriAlbumId = it->first;
        UpdateAlbumOnSystemMoveAssets(oriAlbumId, targetAlbumId);
        auto watch = MediaLibraryNotify::GetInstance();
        for (const auto &id : it->second) {
            watch->Notify(PhotoColumn::PHOTO_URI_PREFIX + to_string(id),
                NotifyType::NOTIFY_ALBUM_REMOVE_ASSET, oriAlbumId);
            watch->Notify(PhotoColumn::PHOTO_URI_PREFIX + to_string(id),
                NotifyType::NOTIFY_ALBUM_ADD_ASSET, targetAlbumId);
        }
    }
    return changedRows;
}

int32_t MediaLibraryPhotoOperations::BatchSetOwnerAlbumId(MediaLibraryCommand &cmd)
{
    vector<shared_ptr<FileAsset>> fileAssetVector;
    vector<string> columns = { PhotoColumn::MEDIA_ID, PhotoColumn::MEDIA_FILE_PATH,
        PhotoColumn::MEDIA_TYPE, PhotoColumn::MEDIA_NAME };
    MediaLibraryRdbStore::ReplacePredicatesUriToId(*(cmd.GetAbsRdbPredicates()));

    // Check if move from system album
    if (IsSystemAlbumMovement(cmd)) {
        MEDIA_INFO_LOG("Move assets from system album");
        GetSystemMoveAssets(*(cmd.GetAbsRdbPredicates()));
        int32_t updateSysRows = UpdateSystemRows(cmd);
        return updateSysRows;
    }

    int32_t errCode = GetFileAssetVectorFromDb(*(cmd.GetAbsRdbPredicates()),
        OperationObject::FILESYSTEM_PHOTO, fileAssetVector, columns);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode,
        "Failed to query file asset vector from db, errCode=%{private}d", errCode);

    int32_t updateRows = UpdateFileInDb(cmd);
    if (updateRows < 0) {
        MEDIA_ERR_LOG("Update Photo in database failed, updateRows=%{public}d", updateRows);
        return updateRows;
    }
    int32_t targetAlbumId = 0;
    int32_t oriAlbumId = 0;
    vector<string> idsToUpdateIndex;
    UpdateOwnerAlbumIdOnMove(cmd, targetAlbumId, oriAlbumId);
    auto watch =  MediaLibraryNotify::GetInstance();
    for (const auto &fileAsset : fileAssetVector) {
        idsToUpdateIndex.push_back(to_string(fileAsset->GetId()));
        string extraUri = MediaFileUtils::GetExtraUri(fileAsset->GetDisplayName(), fileAsset->GetPath());
        string assetUri = MediaFileUtils::GetUriByExtrConditions(
            PhotoColumn::PHOTO_URI_PREFIX, to_string(fileAsset->GetId()), extraUri);
        watch->Notify(assetUri, NotifyType::NOTIFY_UPDATE);
        watch->Notify(assetUri, NotifyType::NOTIFY_ALBUM_ADD_ASSET, targetAlbumId);
        watch->Notify(assetUri, NotifyType::NOTIFY_ALBUM_REMOVE_ASSET, oriAlbumId);
    }
    if (!idsToUpdateIndex.empty()) {
        MediaAnalysisHelper::AsyncStartMediaAnalysisService(
            static_cast<int32_t>(MediaAnalysisProxy::ActivateServiceType::START_UPDATE_INDEX), idsToUpdateIndex);
    }
    return updateRows;
}

static void RevertOrientation(const shared_ptr<FileAsset> &fileAsset, string &currentOrientation)
{
    if (fileAsset == nullptr) {
        MEDIA_ERR_LOG("fileAsset is null");
        return;
    }

    std::unique_ptr<ImageSource> imageSource;
    int32_t err = CreateImageSource(fileAsset, imageSource);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to obtain image exif information, err = %{public}d", err);
        return;
    }

    uint32_t ret = imageSource->ModifyImageProperty(
        0,
        PHOTO_DATA_IMAGE_ORIENTATION,
        currentOrientation.empty() ? ANALYSIS_HAS_DATA : currentOrientation,
        fileAsset->GetFilePath()
    );
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Rollback of exlf information failed, err = %{public}d", ret);
    }
}

static void CreateThumbnailFileScaned(const string &uri, const string &path, bool isSync)
{
    auto thumbnailService = ThumbnailService::GetInstance();
    if (thumbnailService == nullptr || uri.empty()) {
        return;
    }

    int32_t err = thumbnailService->CreateThumbnailFileScaned(uri, path, isSync);
    if (err != E_SUCCESS) {
        MEDIA_ERR_LOG("ThumbnailService CreateThumbnailFileScaned failed: %{public}d", err);
    }
}

void MediaLibraryPhotoOperations::CreateThumbnailFileScan(const shared_ptr<FileAsset> &fileAsset, string &extraUri,
    bool orientationUpdated, bool isNeedScan)
{
    if (fileAsset == nullptr) {
        MEDIA_ERR_LOG("fileAsset is null");
        return;
    }
    if (orientationUpdated) {
        auto watch = MediaLibraryNotify::GetInstance();
        ScanFile(fileAsset->GetPath(), true, false, true);
        watch->Notify(MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX,
            to_string(fileAsset->GetId()), extraUri), NotifyType::NOTIFY_THUMB_UPDATE);
    }
    if (isNeedScan) {
        ScanFile(fileAsset->GetPath(), true, true, true);
        return;
    }
}

void HandleUpdateIndex(MediaLibraryCommand &cmd, string id)
{
    set<string> targetColumns = {"user_comment", "title"};
    bool needUpdate = false;

    map<string, ValueObject> valuesMap;
    cmd.GetValueBucket().GetAll(valuesMap);
    for (auto i : valuesMap) {
        if (targetColumns.find(i.first) != targetColumns.end()) {
            MEDIA_INFO_LOG("need update index");
            needUpdate = true;
            break;
        }
    }
    if (needUpdate) {
        MediaAnalysisHelper::AsyncStartMediaAnalysisService(
            static_cast<int32_t>(MediaAnalysisProxy::ActivateServiceType::START_UPDATE_INDEX), {id});
    }
}

int32_t MediaLibraryPhotoOperations::UpdateFileAsset(MediaLibraryCommand &cmd)
{
    vector<string> columns = { PhotoColumn::MEDIA_ID, PhotoColumn::MEDIA_FILE_PATH, PhotoColumn::MEDIA_TYPE,
        PhotoColumn::MEDIA_NAME, PhotoColumn::PHOTO_SUBTYPE, PhotoColumn::PHOTO_EDIT_TIME, MediaColumn::MEDIA_HIDDEN,
        PhotoColumn::MOVING_PHOTO_EFFECT_MODE, PhotoColumn::PHOTO_ORIENTATION, PhotoColumn::PHOTO_ALL_EXIF };
    shared_ptr<FileAsset> fileAsset = GetFileAssetFromDb(*(cmd.GetAbsRdbPredicates()),
        OperationObject::FILESYSTEM_PHOTO, columns);
    if (fileAsset == nullptr) {
        return E_INVALID_VALUES;
    }

    bool isNeedScan = false;
    int32_t errCode = RevertToOriginalEffectMode(cmd, fileAsset, isNeedScan);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Failed to revert original effect mode: %{public}d", errCode);
    if (cmd.GetOprnType() == OperationType::SET_USER_COMMENT) {
        errCode = SetUserComment(cmd, fileAsset);
        CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Edit user comment errCode = %{private}d", errCode);
    }

    // Update if FileAsset.title or FileAsset.displayName is modified
    bool isNameChanged = false;
    errCode = UpdateFileName(cmd, fileAsset, isNameChanged);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Update Photo Name failed, fileName=%{private}s",
        fileAsset->GetDisplayName().c_str());

    bool orientationUpdated = false;
    string currentOrientation = "";
    errCode = UpdateOrientationExif(cmd, fileAsset, orientationUpdated, currentOrientation);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Update allexif failed, allexif=%{private}s",
        fileAsset->GetAllExif().c_str());

    int32_t rowId = UpdateFileInDb(cmd);
    if (rowId < 0) {
        MEDIA_ERR_LOG("Update Photo In database failed, rowId=%{public}d", rowId);
        RevertOrientation(fileAsset, currentOrientation);
        return rowId;
    }
    HandleUpdateIndex(cmd, to_string(fileAsset->GetId()));
    string extraUri = MediaFileUtils::GetExtraUri(fileAsset->GetDisplayName(), fileAsset->GetPath());
    errCode = SendTrashNotify(cmd, fileAsset->GetId(), extraUri);
    if (errCode == E_OK) {
        return rowId;
    }
    SendFavoriteNotify(cmd, fileAsset, extraUri);
    SendModifyUserCommentNotify(cmd, fileAsset->GetId(), extraUri);

    CreateThumbnailFileScan(fileAsset, extraUri, orientationUpdated, isNeedScan);

    auto watch = MediaLibraryNotify::GetInstance();
    watch->Notify(MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX, to_string(fileAsset->GetId()),
        extraUri), NotifyType::NOTIFY_UPDATE);
    return rowId;
}

int32_t MediaLibraryPhotoOperations::UpdateV10(MediaLibraryCommand &cmd)
{
    switch (cmd.GetOprnType()) {
        case OperationType::TRASH_PHOTO:
            return TrashPhotos(cmd);
        case OperationType::UPDATE_PENDING:
            return SetPendingStatus(cmd);
        case OperationType::HIDE:
            return HidePhotos(cmd);
        case OperationType::BATCH_UPDATE_FAV:
            return BatchSetFavorite(cmd);
        case OperationType::BATCH_UPDATE_USER_COMMENT:
            return BatchSetUserComment(cmd);
        case OperationType::BATCH_UPDATE_OWNER_ALBUM_ID:
            return BatchSetOwnerAlbumId(cmd);
        case OperationType::DISCARD_CAMERA_PHOTO:
            return DiscardCameraPhoto(cmd);
        case OperationType::SAVE_CAMERA_PHOTO:
            return SaveCameraPhoto(cmd);
        case OperationType::SAVE_PICTURE:
            return ForceSavePicture(cmd);
        case OperationType::SET_VIDEO_ENHANCEMENT_ATTR:
            return SetVideoEnhancementAttr(cmd);
        case OperationType::DEGENERATE_MOVING_PHOTO:
            return DegenerateMovingPhoto(cmd);
        case OperationType::SET_OWNER_ALBUM_ID:
            return UpdateOwnerAlbumId(cmd);
        default:
            return UpdateFileAsset(cmd);
    }
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::UpdateV9(MediaLibraryCommand &cmd)
{
    vector<string> columns = {
        PhotoColumn::MEDIA_ID,
        PhotoColumn::MEDIA_FILE_PATH,
        PhotoColumn::MEDIA_TYPE,
        PhotoColumn::MEDIA_NAME,
        PhotoColumn::MEDIA_RELATIVE_PATH,
        MediaColumn::MEDIA_HIDDEN
    };
    shared_ptr<FileAsset> fileAsset = GetFileAssetFromDb(*(cmd.GetAbsRdbPredicates()),
        OperationObject::FILESYSTEM_PHOTO, columns);
    if (fileAsset == nullptr) {
        return E_INVALID_VALUES;
    }

    // Update if FileAsset.title or FileAsset.displayName is modified
    bool isNameChanged = false;
    int32_t errCode = UpdateFileName(cmd, fileAsset, isNameChanged);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Update Photo Name failed, fileName=%{private}s",
        fileAsset->GetDisplayName().c_str());
    errCode = UpdateRelativePath(cmd, fileAsset, isNameChanged);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Update Photo RelativePath failed, relativePath=%{private}s",
        fileAsset->GetRelativePath().c_str());
    if (isNameChanged) {
        UpdateVirtualPath(cmd, fileAsset);
    }

    int32_t rowId = UpdateFileInDb(cmd);
    if (rowId < 0) {
        MEDIA_ERR_LOG("Update Photo In database failed, rowId=%{public}d", rowId);
        return rowId;
    }

    errCode = SendTrashNotify(cmd, fileAsset->GetId());
    if (errCode == E_OK) {
        return rowId;
    }
    SendFavoriteNotify(cmd, fileAsset);
    auto watch = MediaLibraryNotify::GetInstance();
    watch->Notify(MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX, to_string(fileAsset->GetId())),
        NotifyType::NOTIFY_UPDATE);
    return rowId;
}

int32_t MediaLibraryPhotoOperations::OpenCache(MediaLibraryCommand& cmd, const string& mode, bool& isCacheOperation)
{
    isCacheOperation = false;
    string uriString = cmd.GetUriStringWithoutSegment();
    if (!MediaFileUtils::StartsWith(uriString, PhotoColumn::PHOTO_CACHE_URI_PREFIX)) {
        return E_OK;
    }

    isCacheOperation = true;
    string fileName = uriString.substr(PhotoColumn::PHOTO_CACHE_URI_PREFIX.size());
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CheckDisplayName(fileName) == E_OK, E_INVALID_URI,
        "Check fileName failed, fileName=%{private}s", fileName.c_str());
    MediaType mediaType = MediaFileUtils::GetMediaType(fileName);
    CHECK_AND_RETURN_RET_LOG(mediaType == MediaType::MEDIA_TYPE_IMAGE || mediaType == MediaType::MEDIA_TYPE_VIDEO,
        E_INVALID_URI, "Check mediaType failed, fileName=%{private}s, mediaType=%{public}d", fileName.c_str(),
        mediaType);

    string cacheDir = GetAssetCacheDir();
    string path = cacheDir + "/" + fileName;
    string fileId = MediaFileUtils::GetIdFromUri(uriString);

    if (mode == MEDIA_FILEMODE_READONLY) {
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsFileExists(path), E_HAS_FS_ERROR,
            "Cache file does not exist, path=%{private}s", path.c_str());
        return OpenFileWithPrivacy(path, mode, fileId);
    }
    CHECK_AND_RETURN_RET_LOG(
        MediaFileUtils::CreateDirectory(cacheDir), E_HAS_FS_ERROR, "Cannot create dir %{private}s", cacheDir.c_str());
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateAsset(path) == E_SUCCESS, E_HAS_FS_ERROR,
        "Create cache file failed, path=%{private}s", path.c_str());
    return OpenFileWithPrivacy(path, mode, fileId);
}

int32_t MediaLibraryPhotoOperations::OpenEditOperation(MediaLibraryCommand &cmd, bool &isSkip)
{
    isSkip = true;
    string operationKey = cmd.GetQuerySetParam(MEDIA_OPERN_KEYWORD);
    if (operationKey.empty()) {
        return E_OK;
    }
    if (operationKey == EDIT_DATA_REQUEST) {
        isSkip = false;
        return RequestEditData(cmd);
    } else if (operationKey == SOURCE_REQUEST) {
        isSkip = false;
        return RequestEditSource(cmd);
    } else if (operationKey == COMMIT_REQUEST) {
        isSkip = false;
        return CommitEditOpen(cmd);
    }

    return E_OK;
}

const static vector<string> EDITED_COLUMN_VECTOR = {
    PhotoColumn::MEDIA_FILE_PATH,
    MediaColumn::MEDIA_NAME,
    PhotoColumn::PHOTO_EDIT_TIME,
    PhotoColumn::MEDIA_TIME_PENDING,
    PhotoColumn::MEDIA_DATE_TRASHED,
    PhotoColumn::PHOTO_SUBTYPE,
    PhotoColumn::MOVING_PHOTO_EFFECT_MODE,
    PhotoColumn::PHOTO_ORIGINAL_SUBTYPE,
};

static int32_t CheckFileAssetStatus(const shared_ptr<FileAsset>& fileAsset, bool checkMovingPhoto = false)
{
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_URI, "FileAsset is nullptr");
    CHECK_AND_RETURN_RET_LOG(fileAsset->GetDateTrashed() == 0, E_IS_RECYCLED, "FileAsset is in recycle");
    CHECK_AND_RETURN_RET_LOG(fileAsset->GetTimePending() == 0, E_IS_PENDING_ERROR, "FileAsset is in pending");
    if (checkMovingPhoto) {
        CHECK_AND_RETURN_RET_LOG((fileAsset->GetPhotoSubType() == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO) ||
            fileAsset->GetMovingPhotoEffectMode() == static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY)),
            E_INVALID_VALUES, "FileAsset is not moving photo");
    }
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::RequestEditData(MediaLibraryCommand &cmd)
{
    string uriString = cmd.GetUriStringWithoutSegment();
    string id = MediaFileUtils::GetIdFromUri(uriString);
    if (uriString.empty() || (!MediaLibraryDataManagerUtils::IsNumber(id))) {
        return E_INVALID_URI;
    }

    shared_ptr<FileAsset> fileAsset = GetFileAssetFromDb(PhotoColumn::MEDIA_ID, id,
        OperationObject::FILESYSTEM_PHOTO, EDITED_COLUMN_VECTOR);
    int32_t err = CheckFileAssetStatus(fileAsset);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "Failed to check status of fileAsset: %{private}s", uriString.c_str());

    string path = fileAsset->GetFilePath();
    CHECK_AND_RETURN_RET_LOG(!path.empty(), E_INVALID_URI, "Can not get file path, uri=%{private}s",
        uriString.c_str());

    string editDataPath = GetEditDataPath(path);
    string dataPath = editDataPath;
    if (!MediaFileUtils::IsFileExists(dataPath)) {
        dataPath = GetEditDataCameraPath(path);
    }
    CHECK_AND_RETURN_RET_LOG(!dataPath.empty(), E_INVALID_PATH, "Get edit data path from path %{private}s failed",
        dataPath.c_str());
    if (fileAsset->GetPhotoEditTime() == 0 && !MediaFileUtils::IsFileExists(dataPath)) {
        MEDIA_INFO_LOG("File %{private}s does not have edit data", uriString.c_str());
        dataPath = editDataPath;
        string dataPathDir = GetEditDataDirPath(path);
        CHECK_AND_RETURN_RET_LOG(!dataPathDir.empty(), E_INVALID_PATH,
            "Get edit data dir path from path %{private}s failed", path.c_str());
        if (!MediaFileUtils::IsDirectory(dataPathDir)) {
            CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateDirectory(dataPathDir), E_HAS_FS_ERROR,
                "Failed to create dir %{private}s", dataPathDir.c_str());
        }
        err = MediaFileUtils::CreateAsset(dataPath);
        CHECK_AND_RETURN_RET_LOG(err == E_SUCCESS || err == E_FILE_EXIST, E_HAS_FS_ERROR,
            "Failed to create file %{private}s", dataPath.c_str());
    } else {
        if (!MediaFileUtils::IsFileExists(dataPath)) {
            MEDIA_INFO_LOG("File %{public}s has edit at %{public}ld, but cannot get editdata", uriString.c_str(),
                (long)fileAsset->GetPhotoEditTime());
            return E_HAS_FS_ERROR;
        }
    }

    return OpenFileWithPrivacy(dataPath, "r", id);
}

int32_t MediaLibraryPhotoOperations::RequestEditSource(MediaLibraryCommand &cmd)
{
    string uriString = cmd.GetUriStringWithoutSegment();
    string id = MediaFileUtils::GetIdFromUri(uriString);
    if (uriString.empty() || (!MediaLibraryDataManagerUtils::IsNumber(id))) {
        return E_INVALID_URI;
    }
    CHECK_AND_RETURN_RET_LOG(!PhotoEditingRecord::GetInstance()->IsInRevertOperation(stoi(id)),
        E_IS_IN_COMMIT, "File %{public}s is in revert, can not request source", id.c_str());
    shared_ptr<FileAsset> fileAsset = GetFileAssetFromDb(PhotoColumn::MEDIA_ID, id,
        OperationObject::FILESYSTEM_PHOTO, EDITED_COLUMN_VECTOR);
    int32_t err = CheckFileAssetStatus(fileAsset);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "Failed to check status of fileAsset: %{private}s", uriString.c_str());
    string path = fileAsset->GetFilePath();
    CHECK_AND_RETURN_RET_LOG(!path.empty(), E_INVALID_URI,
        "Can not get file path, uri=%{private}s", uriString.c_str());
    string movingPhotoVideoPath = "";
    bool isMovingPhotoVideoRequest =
        cmd.GetQuerySetParam(MEDIA_MOVING_PHOTO_OPRN_KEYWORD) == OPEN_MOVING_PHOTO_VIDEO;
    if (isMovingPhotoVideoRequest) {
        CHECK_AND_RETURN_RET_LOG(
            CheckOpenMovingPhoto(fileAsset->GetPhotoSubType(), fileAsset->GetMovingPhotoEffectMode(),
                cmd.GetQuerySetParam(MEDIA_OPERN_KEYWORD)),
            E_INVALID_VALUES,
            "Non-moving photo requesting moving photo operation, file id: %{public}s, actual subtype: %{public}d",
            id.c_str(), fileAsset->GetPhotoSubType());
        movingPhotoVideoPath = MediaFileUtils::GetMovingPhotoVideoPath(path);
    }

    string sourcePath = isMovingPhotoVideoRequest ?
        MediaFileUtils::GetMovingPhotoVideoPath(GetEditDataSourcePath(path)) :
        GetEditDataSourcePath(path);
    if (sourcePath.empty() || !MediaFileUtils::IsFileExists(sourcePath)) {
        MEDIA_INFO_LOG("sourcePath does not exist: %{private}s", sourcePath.c_str());
        return OpenFileWithPrivacy(isMovingPhotoVideoRequest ? movingPhotoVideoPath : path, "r", id);
    } else {
        return OpenFileWithPrivacy(sourcePath, "r", id);
    }
}

int32_t MediaLibraryPhotoOperations::CommitEditOpen(MediaLibraryCommand &cmd)
{
    string uriString = cmd.GetUriStringWithoutSegment();
    string id = MediaFileUtils::GetIdFromUri(uriString);
    if (uriString.empty() || (!MediaLibraryDataManagerUtils::IsNumber(id))) {
        return E_INVALID_URI;
    }

    shared_ptr<FileAsset> fileAsset = GetFileAssetFromDb(PhotoColumn::MEDIA_ID, id,
        OperationObject::FILESYSTEM_PHOTO, EDITED_COLUMN_VECTOR);
    if (fileAsset == nullptr) {
        MEDIA_ERR_LOG("Get FileAsset From Uri Failed, uri:%{public}s", uriString.c_str());
        return E_INVALID_URI;
    }
    int32_t fileId = stoi(id);  // after MediaLibraryDataManagerUtils::IsNumber, id must be number
    if (!PhotoEditingRecord::GetInstance()->StartCommitEdit(fileId)) {
        return E_IS_IN_REVERT;
    }
    int32_t fd = CommitEditOpenExecute(fileAsset);
    if (fd < 0) {
        PhotoEditingRecord::GetInstance()->EndCommitEdit(fileId);
    }
    return fd;
}

int32_t MediaLibraryPhotoOperations::CommitEditOpenExecute(const shared_ptr<FileAsset> &fileAsset)
{
    int32_t err = CheckFileAssetStatus(fileAsset);
    CHECK_AND_RETURN_RET(err == E_OK, err);
    string path = fileAsset->GetFilePath();
    if (path.empty()) {
        MEDIA_ERR_LOG("Can not get file path");
        return E_INVALID_URI;
    }
    if (fileAsset->GetPhotoEditTime() == 0) {
        string sourceDirPath = GetEditDataDirPath(path);
        CHECK_AND_RETURN_RET_LOG(!sourceDirPath.empty(), E_INVALID_URI, "Can not get edit dir path");
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateDirectory(sourceDirPath), E_HAS_FS_ERROR,
            "Can not create dir %{private}s", sourceDirPath.c_str());
        string sourcePath = GetEditDataSourcePath(path);
        CHECK_AND_RETURN_RET_LOG(!sourcePath.empty(), E_INVALID_URI, "Can not get edit source path");
        if (!MediaFileUtils::IsFileExists(sourcePath)) {
            CHECK_AND_RETURN_RET_LOG(MediaFileUtils::ModifyAsset(path, sourcePath) == E_SUCCESS, E_HAS_FS_ERROR,
                "Move file failed, srcPath:%{private}s, newPath:%{private}s", path.c_str(), sourcePath.c_str());
            CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateFile(path), E_HAS_FS_ERROR,
                "Create file failed, path:%{private}s", path.c_str());
        } else {
            MEDIA_WARN_LOG("Unexpected source file already exists for a not-edited asset, display name: %{private}s",
                fileAsset->GetDisplayName().c_str());
        }
    }

    string fileId = MediaFileUtils::GetIdFromUri(fileAsset->GetUri());
    return OpenFileWithPrivacy(path, "rw", fileId);
}

static int32_t UpdateEditTime(int32_t fileId, int64_t time)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        return E_HAS_DB_ERROR;
    }
    MediaLibraryCommand updatePendingCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    updatePendingCmd.GetAbsRdbPredicates()->EqualTo(MediaColumn::MEDIA_ID, to_string(fileId));
    ValuesBucket updateValues;
    updateValues.PutLong(PhotoColumn::PHOTO_EDIT_TIME, time);
    updatePendingCmd.SetValueBucket(updateValues);
    int32_t rowId = 0;
    int32_t result = rdbStore->Update(updatePendingCmd, rowId);
    if (result != NativeRdb::E_OK || rowId <= 0) {
        MEDIA_ERR_LOG("Update File pending failed. Result %{public}d.", result);
        return E_HAS_DB_ERROR;
    }

    return E_OK;
}

static int32_t RevertMetadata(int32_t fileId, int64_t time, int32_t effectMode, int32_t photoSubType)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        return E_HAS_DB_ERROR;
    }
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    cmd.GetAbsRdbPredicates()->EqualTo(MediaColumn::MEDIA_ID, to_string(fileId));
    ValuesBucket updateValues;
    updateValues.PutLong(PhotoColumn::PHOTO_EDIT_TIME, time);
    if (effectMode == static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY)) {
        updateValues.PutInt(PhotoColumn::MOVING_PHOTO_EFFECT_MODE,
            static_cast<int32_t>(MovingPhotoEffectMode::DEFAULT));
        updateValues.PutInt(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    } else if (photoSubType == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO) &&
        effectMode != static_cast<int32_t>(MovingPhotoEffectMode::DEFAULT)) {
        updateValues.PutInt(PhotoColumn::MOVING_PHOTO_EFFECT_MODE,
            static_cast<int32_t>(MovingPhotoEffectMode::DEFAULT));
    }
    cmd.SetValueBucket(updateValues);
    int32_t rowId = 0;
    int32_t result = rdbStore->Update(cmd, rowId);
    if (result != NativeRdb::E_OK || rowId <= 0) {
        MEDIA_ERR_LOG("Failed to revert metadata. Result %{public}d.", result);
        return E_HAS_DB_ERROR;
    }
    return E_OK;
}

static int32_t UpdateEffectMode(int32_t fileId, int32_t effectMode)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("Failed to get rdbStore when updating effect mode");
        return E_HAS_DB_ERROR;
    }

    MediaLibraryCommand updateCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    updateCmd.GetAbsRdbPredicates()->EqualTo(MediaColumn::MEDIA_ID, to_string(fileId));
    ValuesBucket updateValues;
    updateValues.PutInt(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, effectMode);
    if (effectMode == static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY)) {
        updateValues.PutInt(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::DEFAULT));
    } else {
        updateValues.PutInt(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    }
    updateCmd.SetValueBucket(updateValues);

    int32_t updateRows = -1;
    int32_t errCode = rdbStore->Update(updateCmd, updateRows);
    if (errCode != NativeRdb::E_OK || updateRows < 0) {
        MEDIA_ERR_LOG("Update effect mode failed. errCode:%{public}d, updateRows:%{public}d.", errCode, updateRows);
        return E_HAS_DB_ERROR;
    }
    return E_OK;
}

static int32_t UpdateMimeType(const int32_t &fileId, const std::string mimeType)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("Failed to get rdbStore when updating mime type");
        return E_HAS_DB_ERROR;
    }
    MediaLibraryCommand updateCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    updateCmd.GetAbsRdbPredicates()->EqualTo(MediaColumn::MEDIA_ID, to_string(fileId));
    ValuesBucket updateValues;

    updateCmd.SetValueBucket(updateValues);
    int32_t updateRows = -1;
    int32_t errCode = rdbStore->Update(updateCmd, updateRows);
    if (errCode != NativeRdb::E_OK || updateRows < 0) {
        MEDIA_ERR_LOG("Update mime type failed. errCode:%{public}d, updateRows:%{public}d.", errCode, updateRows);
        return E_HAS_DB_ERROR;
    }
    return E_OK;
}

static void GetModityExtensionPath(std::string &path, std::string &modifyFilePath, const std::string &extension)
{
    size_t pos = path.find_last_of('.');
    modifyFilePath = path.substr(0, pos) + extension;
}
 
int32_t MediaLibraryPhotoOperations::UpdateExtension(const int32_t &fileId, std::string &mimeType,
    const int32_t &fileType, std::string &oldFilePath)
{
    ImageFileType type = static_cast<ImageFileType>(fileType);
    auto itr = IMAGE_FILE_TYPE_MAP.find(type);
    if (itr == IMAGE_FILE_TYPE_MAP.end()) {
        MEDIA_ERR_LOG("fileType : %{public} is not support", fileType);
        return E_INVALID_ARGUMENTS;
    }
    mimeType = itr->second;
    auto extensionItr = IMAGE_EXTENSION_MAP.find(type);
    if (itr == IMAGE_EXTENSION_MAP.end()) {
        MEDIA_ERR_LOG("fileType : %{public} is not support", fileType);
        return E_INVALID_ARGUMENTS;
    }
    std::string extension = extensionItr->second;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("Failed to get rdbStore when updating mime type");
        return E_HAS_DB_ERROR;
    }
    shared_ptr<FileAsset> fileAsset = GetFileAssetFromDb(PhotoColumn::MEDIA_ID, to_string(fileId),
                                                         OperationObject::FILESYSTEM_PHOTO, EDITED_COLUMN_VECTOR);
    std::string filePath = fileAsset->GetFilePath();
    std::string displayName = fileAsset->GetDisplayName();
    std::string modifyFilePath;
    std::string modifyDisplayName;
    GetModityExtensionPath(filePath, modifyFilePath, extension);
    GetModityExtensionPath(displayName, modifyDisplayName, extension);
    MEDIA_DEBUG_LOG("modifyFilePath:%{public}s", modifyFilePath.c_str());
    MEDIA_DEBUG_LOG("modifyDisplayName:%{public}s", modifyDisplayName.c_str());
    if ((modifyFilePath == filePath) && (modifyDisplayName == displayName)) {
        return E_OK;
    }
    UpdateEditDataPath(filePath, extension);
    MediaLibraryCommand updateCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    updateCmd.GetAbsRdbPredicates()->EqualTo(MediaColumn::MEDIA_ID, to_string(fileId));
    ValuesBucket updateValues;
    updateValues.PutString(MediaColumn::MEDIA_FILE_PATH, modifyFilePath);
    updateValues.PutString(MediaColumn::MEDIA_NAME, modifyDisplayName);
    updateValues.PutString(MediaColumn::MEDIA_MIME_TYPE, mimeType);
    updateCmd.SetValueBucket(updateValues);
    int32_t updateRows = -1;
    int32_t errCode = rdbStore->Update(updateCmd, updateRows);
    if (errCode != NativeRdb::E_OK || updateRows < 0) {
        MEDIA_ERR_LOG("Update extension failed. errCode:%{public}d, updateRows:%{public}d.", errCode, updateRows);
        return E_HAS_DB_ERROR;
    }
    oldFilePath = filePath;
    return E_OK;
}

void MediaLibraryPhotoOperations::UpdateEditDataPath(std::string filePath, const std::string &extension)
{
    string editDataPath = GetEditDataDirPath(filePath);
    string tempOutputPath = editDataPath;
    size_t pos = tempOutputPath.find_last_of('.');
    if (pos != string::npos) {
        tempOutputPath.replace(pos, extension.length(), extension);
        rename(editDataPath.c_str(), tempOutputPath.c_str());
        MEDIA_DEBUG_LOG("rename, src: %{public}s, dest: %{public}s",
            editDataPath.c_str(), tempOutputPath.c_str());
    }
}

void MediaLibraryPhotoOperations::DeleteAbnormalFile(std::string &assetPath, const int32_t &fileId,
    const std::string &oldFilePath)
{
    MEDIA_INFO_LOG("DeleteAbnormalFile fileId:%{public}d, assetPath = %{public}s", fileId, assetPath.c_str());
    MediaLibraryObjectUtils::ScanFileAsync(assetPath, to_string(fileId), MediaLibraryApi::API_10);
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    vector<string> columns = { MediaColumn::MEDIA_ID, MediaColumn::MEDIA_FILE_PATH};
    string whereClause = MediaColumn::MEDIA_FILE_PATH + "= ?";
    vector<string> args = {oldFilePath};
    predicates.SetWhereClause(whereClause);
    predicates.SetWhereArgs(args);
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MediaFileUtils::DeleteFile(oldFilePath);
        DeleteRevertMessage(oldFilePath);
        return;
    }
    int32_t fileIdTemp = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
    string filePathTemp = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
    resultSet->Close();
    MEDIA_INFO_LOG("DeleteAbnormalFile fileId:%{public}d, filePathTemp = %{public}s", fileIdTemp, filePathTemp.c_str());
    auto oldFileAsset = GetFileAssetFromDb(PhotoColumn::MEDIA_ID, to_string(fileIdTemp),
                                           OperationObject::FILESYSTEM_PHOTO, EDITED_COLUMN_VECTOR);
    if (oldFileAsset != nullptr) {
        int32_t deleteRow = DeletePhoto(oldFileAsset, MediaLibraryApi::API_10);
        if (deleteRow < 0) {
            MEDIA_ERR_LOG("delete photo failed, deleteRow=%{public}d", deleteRow);
        }
    }
}

int32_t MediaLibraryPhotoOperations::CommitEditInsert(MediaLibraryCommand &cmd)
{
    const ValuesBucket &values = cmd.GetValueBucket();
    string editData;
    int32_t id = 0;
    if (!GetInt32FromValuesBucket(values, PhotoColumn::MEDIA_ID, id)) {
        MEDIA_ERR_LOG("Failed to get fileId");
        PhotoEditingRecord::GetInstance()->EndCommitEdit(id);
        return E_INVALID_VALUES;
    }
    if (!GetStringFromValuesBucket(values, EDIT_DATA, editData)) {
        MEDIA_ERR_LOG("Failed to get editdata");
        PhotoEditingRecord::GetInstance()->EndCommitEdit(id);
        return E_INVALID_VALUES;
    }

    shared_ptr<FileAsset> fileAsset = GetFileAssetFromDb(PhotoColumn::MEDIA_ID, to_string(id),
        OperationObject::FILESYSTEM_PHOTO, EDITED_COLUMN_VECTOR);
    if (fileAsset == nullptr) {
        MEDIA_ERR_LOG("Get FileAsset Failed, fileId=%{public}d", id);
        PhotoEditingRecord::GetInstance()->EndCommitEdit(id);
        return E_INVALID_VALUES;
    }
    fileAsset->SetId(id);
    int32_t ret = CommitEditInsertExecute(fileAsset, editData);
    PhotoEditingRecord::GetInstance()->EndCommitEdit(id);
    MEDIA_INFO_LOG("commit edit finished, fileId=%{public}d", id);
    return ret;
}

static void NotifyFormMap(const int32_t fileId, const string& path, const bool isSave)
{
    string uri = MediaLibraryFormMapOperations::GetUriByFileId(fileId, path);
    if (uri.empty()) {
        MEDIA_WARN_LOG("Failed to GetUriByFileId(%{public}d, %{private}s)", fileId, path.c_str());
        return;
    }

    vector<int64_t> formIds;
    MediaLibraryFormMapOperations::GetFormMapFormId(uri.c_str(), formIds);
    if (!formIds.empty()) {
        MediaLibraryFormMapOperations::PublishedChange(uri, formIds, isSave);
    }
}

int32_t MediaLibraryPhotoOperations::CommitEditInsertExecute(const shared_ptr<FileAsset> &fileAsset,
    const string &editData)
{
    int32_t errCode = CheckFileAssetStatus(fileAsset);
    CHECK_AND_RETURN_RET(errCode == E_OK, errCode);

    string path = fileAsset->GetPath();
    CHECK_AND_RETURN_RET_LOG(!path.empty(), E_INVALID_VALUES, "File path is empty");
    string editDataPath = GetEditDataPath(path);
    CHECK_AND_RETURN_RET_LOG(!editDataPath.empty(), E_INVALID_VALUES, "EditData path is empty");
    if (!MediaFileUtils::IsFileExists(editDataPath)) {
        string dataPathDir = GetEditDataDirPath(path);
        CHECK_AND_RETURN_RET_LOG(!dataPathDir.empty(), E_INVALID_PATH,
            "Get edit data dir path from path %{private}s failed", path.c_str());
        if (!MediaFileUtils::IsDirectory(dataPathDir)) {
            CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateDirectory(dataPathDir), E_HAS_FS_ERROR,
                "Failed to create dir %{private}s", dataPathDir.c_str());
        }
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateAsset(editDataPath) == E_SUCCESS, E_HAS_FS_ERROR,
            "Failed to create file %{private}s", editDataPath.c_str());
    }
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::WriteStrToFile(editDataPath, editData), E_HAS_FS_ERROR,
        "Failed to write editdata path:%{private}s", editDataPath.c_str());

    MediaLibraryRdbUtils::UpdateSystemAlbumInternal(
        MediaLibraryUnistoreManager::GetInstance().GetRdbStore(), {
        to_string(PhotoAlbumSubType::IMAGE),
        to_string(PhotoAlbumSubType::VIDEO),
        to_string(PhotoAlbumSubType::SCREENSHOT),
        to_string(PhotoAlbumSubType::FAVORITE),
        to_string(PhotoAlbumSubType::CLOUD_ENHANCEMENT),
    });
    errCode = UpdateEditTime(fileAsset->GetId(), MediaFileUtils::UTCTimeSeconds());
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Failed to update edit time, fileId:%{public}d",
        fileAsset->GetId());
    ScanFile(path, false, true, true);
    NotifyFormMap(fileAsset->GetId(), fileAsset->GetFilePath(), false);
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::RevertToOrigin(MediaLibraryCommand &cmd)
{
    const ValuesBucket &values = cmd.GetValueBucket();
    int32_t fileId = 0;
    CHECK_AND_RETURN_RET_LOG(GetInt32FromValuesBucket(values, PhotoColumn::MEDIA_ID, fileId), E_INVALID_VALUES,
        "Failed to get fileId");

    shared_ptr<FileAsset> fileAsset = GetFileAssetFromDb(PhotoColumn::MEDIA_ID, to_string(fileId),
        OperationObject::FILESYSTEM_PHOTO, EDITED_COLUMN_VECTOR);
    if (fileAsset == nullptr) {
        MEDIA_ERR_LOG("Get FileAsset From Uri Failed, fileId:%{public}d", fileId);
        return E_INVALID_VALUES;
    }
    fileAsset->SetId(fileId);
    if (!PhotoEditingRecord::GetInstance()->StartRevert(fileId)) {
        return E_IS_IN_COMMIT;
    }

    int32_t errCode = DoRevertEdit(fileAsset);
    PhotoEditingRecord::GetInstance()->EndRevert(fileId);
    return errCode;
}

int32_t MediaLibraryPhotoOperations::UpdateMovingPhotoSubtype(int32_t fileId, int32_t currentPhotoSubType)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("Failed to get rdbStore when updating subtype");
        return E_HAS_DB_ERROR;
    }

    int32_t updatePhotoSubType = currentPhotoSubType == static_cast<int32_t>(PhotoSubType::DEFAULT) ?
        static_cast<int32_t>(PhotoSubType::MOVING_PHOTO) : static_cast<int32_t>(PhotoSubType::DEFAULT);
    MediaLibraryCommand updateCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    updateCmd.GetAbsRdbPredicates()->EqualTo(MediaColumn::MEDIA_ID, to_string(fileId));
    ValuesBucket updateValues;
    updateValues.PutInt(PhotoColumn::PHOTO_SUBTYPE, updatePhotoSubType);
    if (currentPhotoSubType == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)) {
        updateValues.PutInt(PhotoColumn::PHOTO_ORIGINAL_SUBTYPE, static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    }
    updateCmd.SetValueBucket(updateValues);
    int32_t updateRows = -1;
    int32_t errCode = rdbStore->Update(updateCmd, updateRows);
    if (errCode != NativeRdb::E_OK || updateRows < 0) {
        MEDIA_ERR_LOG("Update subtype field failed. errCode:%{public}d, updateRows:%{public}d", errCode, updateRows);
        return E_HAS_DB_ERROR;
    }
    return E_OK;
}

void ResetOcrInfo(const int32_t &fileId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("MediaLibrary rdbStore is nullptr!");
        return;
    }
    string sqlDeleteOcr = "DELETE FROM " + VISION_OCR_TABLE + " WHERE file_id = " + to_string(fileId) + ";" +
        " UPDATE " + VISION_TOTAL_TABLE + " SET ocr = 0 WHERE file_id = " + to_string(fileId) + ";";

    int ret = rdbStore->ExecuteSql(sqlDeleteOcr);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Update ocr info failed, ret = %{public}d, file id is %{public}d", ret, fileId);
    }
}

int32_t MediaLibraryPhotoOperations::DoRevertEdit(const std::shared_ptr<FileAsset> &fileAsset)
{
    int32_t errCode = CheckFileAssetStatus(fileAsset);
    CHECK_AND_RETURN_RET(errCode == E_OK, errCode);
    int32_t fileId = fileAsset->GetId();
    if (fileAsset->GetPhotoEditTime() == 0) {
        MEDIA_INFO_LOG("File %{public}d is not edit", fileId);
        return E_OK;
    }

    string path = fileAsset->GetFilePath();
    CHECK_AND_RETURN_RET_LOG(!path.empty(), E_INVALID_URI, "Can not get file path, fileId=%{public}d", fileId);
    string sourcePath = GetEditDataSourcePath(path);
    CHECK_AND_RETURN_RET_LOG(!sourcePath.empty(), E_INVALID_URI, "Cannot get source path, id=%{public}d", fileId);
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsFileExists(sourcePath), E_NO_SUCH_FILE, "Can not get source file");

    int32_t subtype = static_cast<int32_t>(fileAsset->GetPhotoSubType());
    int32_t movingPhotoSubtype = static_cast<int32_t>(PhotoSubType::MOVING_PHOTO);
    if (fileAsset->GetOriginalSubType() == movingPhotoSubtype) {
        errCode = UpdateMovingPhotoSubtype(fileAsset->GetId(), subtype);
        CHECK_AND_RETURN_RET_LOG(errCode == E_OK, E_HAS_DB_ERROR, "Failed to update movingPhoto subtype");
    }

    string editDataPath = GetEditDataPath(path);
    CHECK_AND_RETURN_RET_LOG(!editDataPath.empty(), E_INVALID_URI, "Cannot get editdata path, id=%{public}d", fileId);

    errCode = RevertMetadata(fileId, 0, fileAsset->GetMovingPhotoEffectMode(), fileAsset->GetPhotoSubType());
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, E_HAS_DB_ERROR, "Failed to update data, fileId=%{public}d", fileId);

    ResetOcrInfo(fileId);
    if (MediaFileUtils::IsFileExists(editDataPath)) {
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::DeleteFile(editDataPath), E_HAS_FS_ERROR,
            "Failed to delete edit data, path:%{private}s", editDataPath.c_str());
    }

    if (MediaFileUtils::IsFileExists(path)) {
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::DeleteFile(path), E_HAS_FS_ERROR,
            "Failed to delete asset, path:%{private}s", path.c_str());
    }

    CHECK_AND_RETURN_RET_LOG(DoRevertFilters(fileAsset, path, sourcePath) == E_OK, E_FAIL,
        "Failed to DoRevertFilters to photo");

    ScanFile(path, true, true, true);
    // revert cloud enhancement ce_available
    EnhancementManager::GetInstance().RevertEditUpdateInternal(fileId);
    NotifyFormMap(fileAsset->GetId(), fileAsset->GetFilePath(), false);
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::DoRevertFilters(const std::shared_ptr<FileAsset> &fileAsset,
    std::string &path, std::string &sourcePath)
{
    string editDataCameraPath = GetEditDataCameraPath(path);
    int32_t subtype = static_cast<int32_t>(fileAsset->GetPhotoSubType());
    if (!MediaFileUtils::IsFileExists(editDataCameraPath)) {
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::ModifyAsset(sourcePath, path) == E_OK, E_HAS_FS_ERROR,
            "Can not modify %{private}s to %{private}s", sourcePath.c_str(), path.c_str());
        if (subtype == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)) {
            string videoPath = MediaFileUtils::GetMovingPhotoVideoPath(path);
            string sourceVideoPath = MediaFileUtils::GetMovingPhotoVideoPath(sourcePath);
            CHECK_AND_RETURN_RET_LOG(MediaFileUtils::ModifyAsset(sourceVideoPath, videoPath) == E_OK, E_HAS_FS_ERROR,
                "Can not modify %{private}s to %{private}s", sourceVideoPath.c_str(), videoPath.c_str());
        }
    } else {
        string editData;
        CHECK_AND_RETURN_RET_LOG(ReadEditdataFromFile(editDataCameraPath, editData) == E_OK, E_HAS_FS_ERROR,
            "Failed to read editdata, path=%{public}s", editDataCameraPath.c_str());
        CHECK_AND_RETURN_RET_LOG(AddFiltersToPhoto(sourcePath, path, editData) == E_OK, E_FAIL,
            "Failed to add filters to photo");
        if (subtype == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)) {
            CHECK_AND_RETURN_RET_LOG(AddFiltersToVideoExecute(fileAsset) == E_OK, E_FAIL,
                "Failed to add filters to video");
        }
    }
    return E_OK;
}

void MediaLibraryPhotoOperations::DeleteRevertMessage(const string &path)
{
    if (path.empty()) {
        MEDIA_ERR_LOG("Input path is empty");
        return;
    }

    string editDirPath = GetEditDataDirPath(path);
    if (editDirPath.empty()) {
        MEDIA_ERR_LOG("Can not get edit data dir path from path %{private}s", path.c_str());
        return;
    }

    if (!MediaFileUtils::IsDirectory(editDirPath)) {
        return;
    }
    if (!MediaFileUtils::DeleteDir(editDirPath)) {
        MEDIA_ERR_LOG("Failed to delete %{private}s dir, filePath is %{private}s",
            editDirPath.c_str(), path.c_str());
        return;
    }
    return;
}

static int32_t Move(const string& srcPath, const string& destPath)
{
    if (!MediaFileUtils::IsFileExists(srcPath)) {
        MEDIA_ERR_LOG("srcPath: %{private}s does not exist!", srcPath.c_str());
        return E_NO_SUCH_FILE;
    }

    if (destPath.empty()) {
        MEDIA_ERR_LOG("Failed to check empty destPath");
        return E_INVALID_VALUES;
    }

    int32_t ret = rename(srcPath.c_str(), destPath.c_str());
    if (ret < 0) {
        MEDIA_ERR_LOG("Failed to rename, src: %{public}s, dest: %{public}s, ret: %{public}d, errno: %{public}d",
            srcPath.c_str(), destPath.c_str(), ret, errno);
    }
    return ret;
}

bool MediaLibraryPhotoOperations::IsNeedRevertEffectMode(MediaLibraryCommand& cmd,
    const shared_ptr<FileAsset>& fileAsset, int32_t& effectMode)
{
    if (fileAsset->GetPhotoSubType() != static_cast<int32_t>(PhotoSubType::MOVING_PHOTO) &&
        fileAsset->GetMovingPhotoEffectMode() != static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY)) {
        return false;
    }
    if (fileAsset->GetPhotoEditTime() != 0) {
        ProcessEditedEffectMode(cmd);
        return false;
    }

    if (!GetInt32FromValuesBucket(cmd.GetValueBucket(), PhotoColumn::MOVING_PHOTO_EFFECT_MODE, effectMode) ||
        (effectMode != static_cast<int32_t>(MovingPhotoEffectMode::DEFAULT) &&
        effectMode != static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY))) {
        return false;
    }
    return true;
}

void MediaLibraryPhotoOperations::ProcessEditedEffectMode(MediaLibraryCommand& cmd)
{
    int32_t effectMode = 0;
    if (!GetInt32FromValuesBucket(
        cmd.GetValueBucket(), PhotoColumn::MOVING_PHOTO_EFFECT_MODE, effectMode) ||
        effectMode != static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY)) {
        return;
    }
    ValuesBucket& updateValues = cmd.GetValueBucket();
    updateValues.PutInt(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, effectMode);
    updateValues.PutInt(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::DEFAULT));
}

int32_t MediaLibraryPhotoOperations::RevertToOriginalEffectMode(
    MediaLibraryCommand& cmd, const shared_ptr<FileAsset>& fileAsset, bool& isNeedScan)
{
    isNeedScan = false;
    int32_t effectMode{0};
    if (!IsNeedRevertEffectMode(cmd, fileAsset, effectMode)) {
        return E_OK;
    }
    isNeedScan = true;
    ValuesBucket& updateValues = cmd.GetValueBucket();
    updateValues.PutInt(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, effectMode);
    if (effectMode == static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY)) {
        updateValues.PutInt(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::DEFAULT));
    } else {
        updateValues.PutInt(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    }

    string imagePath = fileAsset->GetFilePath();
    string sourceImagePath = GetEditDataSourcePath(imagePath);
    CHECK_AND_RETURN_RET_LOG(!sourceImagePath.empty(), E_INVALID_PATH, "Cannot get source image path");
    string sourceVideoPath = MediaFileUtils::GetMovingPhotoVideoPath(sourceImagePath);
    CHECK_AND_RETURN_RET_LOG(!sourceVideoPath.empty(), E_INVALID_PATH, "Cannot get source video path");

    bool isSourceImageExist = MediaFileUtils::IsFileExists(sourceImagePath);
    bool isSourceVideoExist = MediaFileUtils::IsFileExists(sourceVideoPath);
    if (!isSourceImageExist || !isSourceVideoExist) {
        MEDIA_INFO_LOG("isSourceImageExist=%{public}d, isSourceVideoExist=%{public}d",
            isSourceImageExist, isSourceVideoExist);
        isNeedScan = false;
        return E_OK;
    }
    string videoPath = MediaFileUtils::GetMovingPhotoVideoPath(imagePath);
    int32_t errCode = Move(sourceVideoPath, videoPath);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, E_HAS_FS_ERROR, "Failed to move video from %{private}s to %{private}s",
        sourceVideoPath.c_str(), videoPath.c_str());
    string editDataCameraPath = GetEditDataCameraPath(imagePath);
    if (!MediaFileUtils::IsFileExists(editDataCameraPath)) {
        errCode = Move(sourceImagePath, imagePath);
        CHECK_AND_RETURN_RET_LOG(errCode == E_OK, E_HAS_FS_ERROR,
            "Failed to move image from %{private}s to %{private}s",
            sourceImagePath.c_str(), imagePath.c_str());
    } else {
        string editData;
        CHECK_AND_RETURN_RET_LOG(ReadEditdataFromFile(editDataCameraPath, editData) == E_OK, E_HAS_FS_ERROR,
            "Failed to read editdata, path=%{public}s", editDataCameraPath.c_str());
        CHECK_AND_RETURN_RET_LOG(AddFiltersToPhoto(sourceImagePath, imagePath, editData) == E_OK,
            E_FAIL, "Failed to add filters to photo");
    }
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::MoveCacheFile(
    MediaLibraryCommand& cmd, int32_t subtype, const string& cachePath, const string& destPath)
{
    if (subtype != static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)) {
        return Move(cachePath, destPath);
    }

    // write moving photo
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CheckMovingPhotoImage(cachePath), E_INVALID_MOVING_PHOTO,
        "Failed to check image of moving photo");
    string cacheMovingPhotoVideoName;
    if (GetStringFromValuesBucket(cmd.GetValueBucket(), CACHE_MOVING_PHOTO_VIDEO_NAME, cacheMovingPhotoVideoName) &&
            !cacheMovingPhotoVideoName.empty()) {
        string cacheVideoPath = GetAssetCacheDir() + "/" + cacheMovingPhotoVideoName;
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CheckMovingPhotoVideo(cacheVideoPath), E_INVALID_MOVING_PHOTO,
            "Failed to check video of moving photo");
        string destVideoPath = MediaFileUtils::GetMovingPhotoVideoPath(destPath);
        int32_t errCode = Move(cacheVideoPath, destVideoPath);
        CHECK_AND_RETURN_RET_LOG(errCode == E_OK, E_FILE_OPER_FAIL,
            "Failed to move moving photo video from %{private}s to %{private}s, errCode: %{public}d",
            cacheVideoPath.c_str(), destVideoPath.c_str(), errCode);
    }
    return Move(cachePath, destPath);
}

bool MediaLibraryPhotoOperations::CheckCacheCmd(MediaLibraryCommand& cmd, int32_t subtype, const string& displayName)
{
    string cacheFileName;
    if (!GetStringFromValuesBucket(cmd.GetValueBucket(), CACHE_FILE_NAME, cacheFileName)) {
        MEDIA_ERR_LOG("Failed to get cache file name");
        return false;
    }
    string cacheMimeType = MimeTypeUtils::GetMimeTypeFromExtension(MediaFileUtils::GetExtensionFromPath(cacheFileName));
    string assetMimeType = MimeTypeUtils::GetMimeTypeFromExtension(MediaFileUtils::GetExtensionFromPath(displayName));

    if (cacheMimeType.compare(assetMimeType) != 0) {
        MEDIA_ERR_LOG("cache mime type %{public}s mismatches the asset %{public}s",
            cacheMimeType.c_str(), assetMimeType.c_str());
        return false;
    }

    int32_t id = 0;
    bool isCreation = !GetInt32FromValuesBucket(cmd.GetValueBucket(), PhotoColumn::MEDIA_ID, id);
    if (subtype == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO) && isCreation) {
        string movingPhotoVideoName;
        bool containsVideo = GetStringFromValuesBucket(cmd.GetValueBucket(),
            CACHE_MOVING_PHOTO_VIDEO_NAME, movingPhotoVideoName);
        if (!containsVideo) {
            MEDIA_ERR_LOG("Failed to get video when creating moving photo");
            return false;
        }

        string videoExtension = MediaFileUtils::GetExtensionFromPath(movingPhotoVideoName);
        if (!MediaFileUtils::CheckMovingPhotoVideoExtension(videoExtension)) {
            MEDIA_ERR_LOG("Failed to check video of moving photo, extension: %{public}s", videoExtension.c_str());
            return false;
        }
    }
    return true;
}

int32_t MediaLibraryPhotoOperations::ParseMediaAssetEditData(MediaLibraryCommand& cmd, string& editData)
{
    string compatibleFormat;
    string formatVersion;
    string data;
    const ValuesBucket& values = cmd.GetValueBucket();
    bool containsCompatibleFormat = GetStringFromValuesBucket(values, COMPATIBLE_FORMAT, compatibleFormat);
    bool containsFormatVersion = GetStringFromValuesBucket(values, FORMAT_VERSION, formatVersion);
    bool containsData = GetStringFromValuesBucket(values, EDIT_DATA, data);
    bool notContainsEditData = !containsCompatibleFormat && !containsFormatVersion && !containsData;
    bool containsEditData = containsCompatibleFormat && containsFormatVersion && containsData;
    if (!containsEditData && !notContainsEditData) {
        MEDIA_ERR_LOG(
            "Failed to parse edit data, compatibleFormat: %{public}s, formatVersion: %{public}s, data: %{public}s",
            compatibleFormat.c_str(), formatVersion.c_str(), data.c_str());
        return E_INVALID_VALUES;
    }

    nlohmann::json editDataJson;
    string bundleName = cmd.GetBundleName();
    editDataJson[COMPATIBLE_FORMAT] = compatibleFormat.empty() ? bundleName : compatibleFormat;
    editDataJson[FORMAT_VERSION] = formatVersion;
    editDataJson[EDIT_DATA] = data;
    editDataJson[APP_ID] = bundleName;
    editData = editDataJson.dump();
    return E_OK;
}

void MediaLibraryPhotoOperations::ParseCloudEnhancementEditData(string& editData)
{
    if (!nlohmann::json::accept(editData)) {
        MEDIA_WARN_LOG("Failed to verify the editData format, editData is: %{private}s",
            editData.c_str());
        return;
    }
    string editDataJsonStr;
    nlohmann::json jsonObject = nlohmann::json::parse(editData);
    if (jsonObject.contains(EDIT_DATA)) {
        editDataJsonStr = jsonObject[EDIT_DATA];
        nlohmann::json editDataJson = nlohmann::json::parse(editDataJsonStr, nullptr, false);
        if (editDataJson.is_discarded()) {
            MEDIA_INFO_LOG("editDataJson parse failed");
            return;
        }
        string editDataStr = editDataJson.dump();
        editData = editDataStr;
    }
}

bool MediaLibraryPhotoOperations::IsSetEffectMode(MediaLibraryCommand &cmd)
{
    int32_t effectMode;
    return GetInt32FromValuesBucket(cmd.GetValueBucket(), PhotoColumn::MOVING_PHOTO_EFFECT_MODE, effectMode);
}

bool MediaLibraryPhotoOperations::IsContainsData(MediaLibraryCommand &cmd)
{
    string compatibleFormat;
    string formatVersion;
    string data;
    const ValuesBucket& values = cmd.GetValueBucket();
    bool containsCompatibleFormat = GetStringFromValuesBucket(values, COMPATIBLE_FORMAT, compatibleFormat);
    bool containsFormatVersion = GetStringFromValuesBucket(values, FORMAT_VERSION, formatVersion);
    bool containsData = GetStringFromValuesBucket(values, EDIT_DATA, data);
    return containsCompatibleFormat && containsFormatVersion && containsData;
}

bool MediaLibraryPhotoOperations::IsCameraEditData(MediaLibraryCommand &cmd)
{
    string bundleName = cmd.GetBundleName();
    return IsContainsData(cmd) && (count(CAMERA_BUNDLE_NAMES.begin(), CAMERA_BUNDLE_NAMES.end(), bundleName) > 0);
}

int32_t MediaLibraryPhotoOperations::ReadEditdataFromFile(const std::string &editDataPath, std::string &editData)
{
    string editDataStr;
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::ReadStrFromFile(editDataPath, editDataStr), E_HAS_FS_ERROR,
        "Can not read editdata from %{private}s", editDataPath.c_str());
    if (!nlohmann::json::accept(editDataStr)) {
        MEDIA_WARN_LOG("Failed to verify the editData format, editData is: %{private}s", editDataStr.c_str());
        editData = editDataStr;
        return E_OK;
    }
    nlohmann::json editdataJson = nlohmann::json::parse(editDataStr);
    if (editdataJson.contains(EDIT_DATA)) {
        editData = editdataJson[EDIT_DATA];
    } else {
        editData = editDataStr;
    }
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::SaveEditDataCamera(MediaLibraryCommand &cmd, const std::string &assetPath,
    std::string &editData)
{
    string editDataStr;
    string editDataCameraPath = GetEditDataCameraPath(assetPath);
    CHECK_AND_RETURN_RET_LOG(!editDataCameraPath.empty(), E_INVALID_VALUES, "Failed to get edit data path");
    if (!MediaFileUtils::IsFileExists(editDataCameraPath)) {
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateFile(editDataCameraPath), E_HAS_FS_ERROR,
            "Failed to create file %{private}s", editDataCameraPath.c_str());
    }
    int ret = ParseMediaAssetEditData(cmd, editDataStr);
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::WriteStrToFile(editDataCameraPath, editDataStr), E_HAS_FS_ERROR,
        "Failed to write editdata:%{private}s", editDataCameraPath.c_str());
    const ValuesBucket& values = cmd.GetValueBucket();
    GetStringFromValuesBucket(values, EDIT_DATA, editData);
    return ret;
}

int32_t MediaLibraryPhotoOperations::SaveSourceAndEditData(
    const shared_ptr<FileAsset>& fileAsset, const string& editData)
{
    CHECK_AND_RETURN_RET_LOG(!editData.empty(), E_INVALID_VALUES, "editData is empty");
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_VALUES, "fileAsset is nullptr");
    string assetPath = fileAsset->GetFilePath();
    CHECK_AND_RETURN_RET_LOG(!assetPath.empty(), E_INVALID_VALUES, "Failed to get asset path");
    string editDataPath = GetEditDataPath(assetPath);
    CHECK_AND_RETURN_RET_LOG(!editDataPath.empty(), E_INVALID_VALUES, "Failed to get edit data path");

    if (fileAsset->GetPhotoEditTime() == 0) { // the asset has not been edited before
        string editDataDirPath = GetEditDataDirPath(assetPath);
        CHECK_AND_RETURN_RET_LOG(!editDataDirPath.empty(), E_INVALID_URI, "Failed to get edit dir path");
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateDirectory(editDataDirPath), E_HAS_FS_ERROR,
            "Failed to create dir %{private}s", editDataDirPath.c_str());

        string sourcePath = GetEditDataSourcePath(assetPath);
        CHECK_AND_RETURN_RET_LOG(!sourcePath.empty(), E_INVALID_URI, "Failed to get source path");
        if (!MediaFileUtils::IsFileExists(sourcePath)) {
            CHECK_AND_RETURN_RET_LOG(MediaFileUtils::ModifyAsset(assetPath, sourcePath) == E_SUCCESS, E_HAS_FS_ERROR,
                "Move file failed, srcPath:%{private}s, newPath:%{private}s", assetPath.c_str(), sourcePath.c_str());
        }

        if (!MediaFileUtils::IsFileExists(editDataPath)) {
            CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateFile(editDataPath), E_HAS_FS_ERROR,
                "Failed to create file %{private}s", editDataPath.c_str());
        }
    }

    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::WriteStrToFile(editDataPath, editData), E_HAS_FS_ERROR,
        "Failed to write editdata:%{private}s", editDataPath.c_str());

    return E_OK;
}

std::shared_ptr<FileAsset> MediaLibraryPhotoOperations::GetFileAsset(MediaLibraryCommand& cmd)
{
    const ValuesBucket& values = cmd.GetValueBucket();
    int32_t id = 0;
    GetInt32FromValuesBucket(values, PhotoColumn::MEDIA_ID, id);
    vector<string> columns = { PhotoColumn::MEDIA_ID, PhotoColumn::MEDIA_FILE_PATH, PhotoColumn::MEDIA_NAME,
        PhotoColumn::PHOTO_SUBTYPE, PhotoColumn::MEDIA_TIME_PENDING, PhotoColumn::MEDIA_DATE_TRASHED };
    shared_ptr<FileAsset> fileAsset = GetFileAssetFromDb(
        PhotoColumn::MEDIA_ID, to_string(id), OperationObject::FILESYSTEM_PHOTO, columns);
    return fileAsset;
}

int32_t MediaLibraryPhotoOperations::GetPicture(const int32_t &fileId, std::shared_ptr<Media::Picture> &picture,
    bool isCleanImmediately, std::string &photoId, bool &isHighQualityPicture)
{
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, std::to_string(fileId));
    vector<string> columns = { PhotoColumn::PHOTO_ID };
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("result set is empty");
        return E_FILE_EXIST;
    }

    photoId = GetStringVal(PhotoColumn::PHOTO_ID, resultSet);
    resultSet->Close();
    if (photoId.empty()) {
        MEDIA_ERR_LOG("photoId is emply fileId is: %{public}d", fileId);
        return E_FILE_EXIST;
    }

    MEDIA_INFO_LOG("photoId: %{public}s", photoId.c_str());
    auto pictureManagerThread = PictureManagerThread::GetInstance();
    if (pictureManagerThread != nullptr) {
        picture = pictureManagerThread->GetDataWithImageId(photoId, isHighQualityPicture, isCleanImmediately);
    }
    if (picture == nullptr) {
        MEDIA_ERR_LOG("picture is not exists!");
        return E_FILE_EXIST;
    }
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::FinishRequestPicture(MediaLibraryCommand &cmd)
{
    const ValuesBucket& values = cmd.GetValueBucket();
    int32_t fileId = 0;
    GetInt32FromValuesBucket(values, PhotoColumn::MEDIA_ID, fileId);

    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, std::to_string(fileId));
    vector<string> columns = { PhotoColumn::PHOTO_ID };
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("result set is empty");
        return E_FILE_EXIST;
    }

    string photoId = GetStringVal(PhotoColumn::PHOTO_ID, resultSet);
    resultSet->Close();
    if (photoId.empty()) {
        MEDIA_ERR_LOG("photoId is emply fileId is: %{public}d", fileId);
        return E_FILE_EXIST;
    }

    MEDIA_INFO_LOG("photoId: %{public}s", photoId.c_str());
    auto pictureManagerThread = PictureManagerThread::GetInstance();
    if (pictureManagerThread != nullptr) {
        pictureManagerThread->FinishAccessingPicture(photoId);
    }
    return E_OK;
}

int64_t MediaLibraryPhotoOperations::CloneSingleAsset(MediaLibraryCommand &cmd)
{
    const ValuesBucket& values = cmd.GetValueBucket();
    int fileId = -1;
    ValueObject valueObject;
    if (values.GetObject(MediaColumn::MEDIA_ID, valueObject)) {
        valueObject.GetInt(fileId);
    } else {
        MEDIA_ERR_LOG("Failed to get media id from values bucket.");
        return fileId;
    }
    string title;
    GetStringFromValuesBucket(values, MediaColumn::MEDIA_TITLE, title);
    return MediaLibraryAlbumFusionUtils::CloneSingleAsset(static_cast<int64_t>(fileId), title);
}

int32_t MediaLibraryPhotoOperations::AddFilters(MediaLibraryCommand& cmd)
{
    // moving photo video save and add filters
    const ValuesBucket& values = cmd.GetValueBucket();
    string videoSaveFinishedUri;
    if (GetStringFromValuesBucket(values, NOTIFY_VIDEO_SAVE_FINISHED, videoSaveFinishedUri)) {
        int32_t id = -1;
        if (!GetInt32FromValuesBucket(values, PhotoColumn::MEDIA_ID, id)) {
            MEDIA_ERR_LOG("Failed to get fileId");
            return E_INVALID_VALUES;
        }
        vector<string> columns = { videoSaveFinishedUri };
        ScanMovingPhoto(cmd, columns);

        vector<string> fileAssetColumns = { PhotoColumn::MEDIA_ID, PhotoColumn::MEDIA_FILE_PATH,
            PhotoColumn::PHOTO_SUBTYPE, PhotoColumn::PHOTO_EDIT_TIME };
        shared_ptr<FileAsset> fileAsset = GetFileAssetFromDb(
            PhotoColumn::MEDIA_ID, to_string(id), OperationObject::FILESYSTEM_PHOTO, fileAssetColumns);
        CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_VALUES,
            "Failed to GetFileAssetFromDb, fileId = %{public}d", id);
        return AddFiltersToVideoExecute(fileAsset);
    }

    if (IsCameraEditData(cmd)) {
        shared_ptr<FileAsset> fileAsset = GetFileAsset(cmd);
        return AddFiltersExecute(cmd, fileAsset, "");
    }
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::ForceSavePicture(MediaLibraryCommand& cmd)
{
    MEDIA_DEBUG_LOG("ForceSavePicture");
    int fileType = std::atoi(cmd.GetQuerySetParam(IMAGE_FILE_TYPE).c_str());
    int fileId = std::atoi(cmd.GetQuerySetParam(PhotoColumn::MEDIA_ID).c_str());
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, std::to_string(fileId));
    vector<string> columns = { PhotoColumn::PHOTO_IS_TEMP };
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    if (resultSet ==nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("result set is empty");
        return E_ERR;
    }
    if (GetInt32Val(PhotoColumn::PHOTO_IS_TEMP, resultSet) == 0) {
        return E_OK;
    }
    resultSet->Close();
    string uri = cmd.GetQuerySetParam("uri");
    SavePicture(fileType, fileId);
    string path = MediaFileUri::GetPathFromUri(uri, true);
    MediaLibraryAssetOperations::ScanFileWithoutAlbumUpdate(path, false, false, true, fileId);
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::SavePicture(const int32_t &fileType, const int32_t &fileId)
{
    MEDIA_DEBUG_LOG("savePicture fileType is: %{public}d, fileId is: %{public}d", fileType, fileId);
    std::shared_ptr<Media::Picture> picture;
    std::string photoId;
    bool isHighQualityPicture = false;
    if (GetPicture(fileId, picture, false, photoId, isHighQualityPicture) != E_OK) {
        MEDIA_ERR_LOG("Failed to get picture");
        return E_FILE_EXIST;
    }

    std::string format = MIME_TYPE_JPEG;
    std::string oldFilePath = "";
    int32_t updateResult = UpdateExtension(fileId, format, fileType, oldFilePath);
    auto fileAsset = GetFileAssetFromDb(PhotoColumn::MEDIA_ID, to_string(fileId),
                                        OperationObject::FILESYSTEM_PHOTO, EDITED_COLUMN_VECTOR);
    string assetPath = fileAsset->GetFilePath();
    CHECK_AND_RETURN_RET_LOG(!assetPath.empty(), E_INVALID_VALUES, "Failed to get asset path");

    FileUtils::DealPicture(format, assetPath, picture);
    string editData = "";
    string editDataCameraPath = GetEditDataCameraPath(assetPath);
    if (ReadEditdataFromFile(editDataCameraPath, editData) == E_OK &&
        GetPicture(fileId, picture, false, photoId, isHighQualityPicture) == E_OK) {
        MediaFileUtils::CopyFileUtil(assetPath, GetEditDataSourcePath(assetPath));
        int32_t ret = MediaChangeEffect::TakeEffectForPicture(picture, editData);
        FileUtils::DealPicture(format, assetPath, picture);
    }
    if (isHighQualityPicture) {
        RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
        predicates.EqualTo(PhotoColumn::MEDIA_ID, fileId);
        ValuesBucket values;
        values.Put(PhotoColumn::PHOTO_QUALITY, static_cast<int32_t>(MultiStagesPhotoQuality::FULL));
        values.Put(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_NEW));
        int32_t updatedRows = MediaLibraryRdbStore::UpdateWithDateTime(values, predicates);
        if (updatedRows < 0) {
            MEDIA_ERR_LOG("update photo quality fail.");
        }
    }

    auto pictureManagerThread = PictureManagerThread::GetInstance();
    if (pictureManagerThread != nullptr) {
        pictureManagerThread->FinishAccessingPicture(photoId);
        pictureManagerThread->DeleteDataWithImageId(lastPhotoId_, LOW_QUALITY_PICTURE);
    }
    lastPhotoId_ = photoId;
    // 删除已经存在的异常后缀的图片
    size_t size = -1;
    MediaFileUtils::GetFileSize(oldFilePath, size);
    if (updateResult == E_OK && oldFilePath != "" && size > 0) {
        DeleteAbnormalFile(assetPath, fileId, oldFilePath);
    }
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::AddFiltersExecute(MediaLibraryCommand& cmd,
    const shared_ptr<FileAsset>& fileAsset, const string& cachePath)
{
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_VALUES, "fileAsset is nullptr");
    int32_t fileId = fileAsset->GetId();
    string assetPath = fileAsset->GetFilePath();
    CHECK_AND_RETURN_RET_LOG(!assetPath.empty(), E_INVALID_VALUES, "Failed to get asset path");
    string editDataDirPath = GetEditDataDirPath(assetPath);
    CHECK_AND_RETURN_RET_LOG(!editDataDirPath.empty(), E_INVALID_URI, "Can not get editdara dir path");
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateDirectory(editDataDirPath), E_HAS_FS_ERROR,
        "Can not create dir %{private}s", editDataDirPath.c_str());
    string sourcePath = GetEditDataSourcePath(assetPath);
    CHECK_AND_RETURN_RET_LOG(!sourcePath.empty(), E_INVALID_URI, "Can not get edit source path");

    if (cachePath.empty()) {
        // Photo目录照片复制到.editdata目录的source.jpg
        MediaFileUtils::CopyFileUtil(assetPath, sourcePath);
    } else {
        // cache移动到source.jpg
        int32_t subtype = fileAsset->GetPhotoSubType();
        MoveCacheFile(cmd, subtype, cachePath, sourcePath);
    }

    // 保存editdata_camera文件
    string editData;
    SaveEditDataCamera(cmd, assetPath, editData);
    // 生成水印
    int32_t ret = AddFiltersToPhoto(sourcePath, assetPath, editData);
    if (ret == E_OK) {
        MediaLibraryObjectUtils::ScanFileAsync(assetPath, to_string(fileAsset->GetId()), MediaLibraryApi::API_10);
    }
    std::shared_ptr<Media::Picture> picture;
    std::string photoId;
    bool isHighQualityPicture = false;
    if (GetPicture(fileId, picture, true, photoId, isHighQualityPicture) == E_OK) {
        return E_OK;
    }
    return ret;
}

int32_t MediaLibraryPhotoOperations::AddFiltersToVideoExecute(const shared_ptr<FileAsset>& fileAsset)
{
    string assetPath = fileAsset->GetFilePath();
    string editDataCameraPath = MediaLibraryAssetOperations::GetEditDataCameraPath(assetPath);
    if (MediaFileUtils::IsFileExists(editDataCameraPath)) {
        string editData;
        CHECK_AND_RETURN_RET_LOG(ReadEditdataFromFile(editDataCameraPath, editData) == E_OK, E_HAS_FS_ERROR,
            "Failed to read editData, path = %{public}s", editDataCameraPath.c_str());
        // erase sticker field
        auto index = editData.find(FRAME_STICKER);
        if (index != std::string::npos) {
            VideoCompositionCallbackImpl::EraseStickerField(editData, index);
        }
        index = editData.find(INPLACE_STICKER);
        if (index != std::string::npos) {
            VideoCompositionCallbackImpl::EraseStickerField(editData, index);
        }
        index = editData.find(FILTERS_FIELD);
        if (index == std::string::npos) {
            MEDIA_ERR_LOG("Can not find Video filters field.");
            return E_ERR;
        }
        if (editData[index + START_DISTANCE] == FILTERS_END) {
            MEDIA_INFO_LOG("MovingPhoto video only supports filter now.");
            return E_OK;
        }
        CHECK_AND_RETURN_RET_LOG(SaveSourceVideoFile(fileAsset, assetPath) == E_OK, E_HAS_FS_ERROR,
            "Failed to save source video, path = %{public}s", assetPath.c_str());
        VideoCompositionCallbackImpl::AddCompositionTask(assetPath, editData);
    }
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::AddFiltersForCloudEnhancementPhoto(int32_t fileId,
    const string& assetPath, const string& editDataCameraSourcePath, const string& mimeType)
{
    CHECK_AND_RETURN_RET_LOG(!assetPath.empty(), E_INVALID_VALUES, "Failed to get asset path");
    string editDataDirPath = GetEditDataDirPath(assetPath);
    CHECK_AND_RETURN_RET_LOG(!editDataDirPath.empty(), E_INVALID_URI, "Can not get editdata dir path");
    string sourcePath = GetEditDataSourcePath(assetPath);
    CHECK_AND_RETURN_RET_LOG(!sourcePath.empty(), E_INVALID_URI, "Can not get edit source path");

    // copy source.jpg
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateDirectory(editDataDirPath), E_HAS_FS_ERROR,
        "Can not create dir %{private}s, errno:%{public}d", editDataDirPath.c_str(), errno);
    bool copyResult = MediaFileUtils::CopyFileUtil(assetPath, sourcePath);
    if (!copyResult) {
        MEDIA_ERR_LOG("copy to source.jpg failed. errno=%{public}d, path: %{public}s", errno, assetPath.c_str());
    }
    string editData;
    MediaFileUtils::ReadStrFromFile(editDataCameraSourcePath, editData);
    ParseCloudEnhancementEditData(editData);
    string editDataCameraDestPath = PhotoFileUtils::GetEditDataCameraPath(assetPath);
    copyResult = MediaFileUtils::CopyFileUtil(editDataCameraSourcePath, editDataCameraDestPath);
    if (!copyResult) {
        MEDIA_ERR_LOG("copy editDataCamera failed. errno=%{public}d, path: %{public}s", errno,
            editDataCameraSourcePath.c_str());
    }
    // normal
    return AddFiltersToPhoto(sourcePath, assetPath, editData);
}

int32_t MediaLibraryPhotoOperations::SubmitEditCacheExecute(MediaLibraryCommand& cmd,
    const shared_ptr<FileAsset>& fileAsset, const string& cachePath)
{
    string editData;
    int32_t id = fileAsset->GetId();
    int32_t errCode = ParseMediaAssetEditData(cmd, editData);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Failed to parse MediaAssetEditData");
    errCode = SaveSourceAndEditData(fileAsset, editData);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Failed to save source and editData");

    int32_t subtype = fileAsset->GetPhotoSubType();
    if (subtype == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)) {
        errCode = SubmitEditMovingPhotoExecute(cmd, fileAsset);
        CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Failed to SubmitEditMovingPhotoExecute");
    }

    string assetPath = fileAsset->GetFilePath();
    errCode = MoveCacheFile(cmd, subtype, cachePath, assetPath);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, E_FILE_OPER_FAIL,
        "Failed to move %{private}s to %{private}s, errCode: %{public}d",
        cachePath.c_str(), assetPath.c_str(), errCode);

    errCode = UpdateEditTime(id, MediaFileUtils::UTCTimeSeconds());
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Failed to update edit time, fileId:%{public}d", id);

    ResetOcrInfo(id);
    ScanFile(assetPath, false, true, true);
    MediaLibraryAnalysisAlbumOperations::UpdatePortraitAlbumCoverSatisfied(id);
    // delete cloud enhacement task
    vector<string> fileId;
    fileId.emplace_back(to_string(fileAsset->GetId()));
    vector<string> photoId;
    EnhancementManager::GetInstance().CancelTasksInternal(fileId, photoId, CloudEnhancementAvailableType::EDIT);
    if (!photoId.empty()) {
        CloudEnhancementGetCount::GetInstance().Report("EditCancellationType", photoId.front());
    }
    NotifyFormMap(id, assetPath, false);
    MediaLibraryVisionOperations::EditCommitOperation(cmd);
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::SubmitCacheExecute(MediaLibraryCommand& cmd,
    const shared_ptr<FileAsset>& fileAsset, const string& cachePath)
{
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_VALUES, "fileAsset is nullptr");
    int32_t subtype = fileAsset->GetPhotoSubType();
    CHECK_AND_RETURN_RET_LOG(CheckCacheCmd(cmd, subtype, fileAsset->GetDisplayName()),
        E_INVALID_VALUES, "Failed to check cache cmd");
    CHECK_AND_RETURN_RET_LOG(fileAsset->GetDateTrashed() == 0, E_IS_RECYCLED, "FileAsset is in recycle");

    int64_t pending = fileAsset->GetTimePending();
    CHECK_AND_RETURN_RET_LOG(
        pending == 0 || pending == UNCREATE_FILE_TIMEPENDING || pending == UNOPEN_FILE_COMPONENT_TIMEPENDING,
        E_IS_PENDING_ERROR, "FileAsset is in pending: %{public}ld", static_cast<long>(pending));

    string assetPath = fileAsset->GetFilePath();
    CHECK_AND_RETURN_RET_LOG(!assetPath.empty(), E_INVALID_VALUES, "Failed to get asset path");

    int32_t id = fileAsset->GetId();
    bool isEdit = (pending == 0);

    if (isEdit) {
        if (!PhotoEditingRecord::GetInstance()->StartCommitEdit(id)) {
            return E_IS_IN_REVERT;
        }
        int32_t errCode = SubmitEditCacheExecute(cmd, fileAsset, cachePath);
        PhotoEditingRecord::GetInstance()->EndCommitEdit(id);
        return errCode;
    } else if (IsCameraEditData(cmd)) {
        AddFiltersExecute(cmd, fileAsset, cachePath);
    } else {
        int32_t errCode = MoveCacheFile(cmd, subtype, cachePath, assetPath);
        CHECK_AND_RETURN_RET_LOG(errCode == E_OK, E_FILE_OPER_FAIL,
            "Failed to move %{private}s to %{private}s, errCode: %{public}d",
            cachePath.c_str(), assetPath.c_str(), errCode);
    }
    ScanFile(assetPath, false, true, true);
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::SaveSourceVideoFile(const shared_ptr<FileAsset>& fileAsset,
    const string& assetPath)
{
    MEDIA_INFO_LOG("Moving photo SaveSourceVideoFile begin, fileId:%{public}d", fileAsset->GetId());
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_VALUES, "fileAsset is nullptr");
    string sourceImagePath = GetEditDataSourcePath(assetPath);
    CHECK_AND_RETURN_RET_LOG(!sourceImagePath.empty(), E_INVALID_PATH, "Can not get source image path");
    string videoPath = MediaFileUtils::GetMovingPhotoVideoPath(assetPath);
    CHECK_AND_RETURN_RET_LOG(!videoPath.empty(), E_INVALID_PATH, "Can not get video path");
    string sourceVideoPath = MediaFileUtils::GetMovingPhotoVideoPath(sourceImagePath);
    CHECK_AND_RETURN_RET_LOG(!sourceVideoPath.empty(), E_INVALID_PATH, "Can not get source video path");
    if (!MediaFileUtils::IsFileExists(sourceVideoPath)) {
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::ModifyAsset(videoPath, sourceVideoPath) == E_SUCCESS,
            E_HAS_FS_ERROR, "Move video file failed, srcPath:%{private}s, newPath:%{private}s",
            videoPath.c_str(), sourceVideoPath.c_str());
    }
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::SubmitEditMovingPhotoExecute(MediaLibraryCommand& cmd,
    const shared_ptr<FileAsset>& fileAsset)
{
    MEDIA_INFO_LOG("Moving photo SubmitEditMovingPhotoExecute begin, fileId:%{public}d", fileAsset->GetId());
    string assetPath = fileAsset->GetFilePath();
    CHECK_AND_RETURN_RET_LOG(!assetPath.empty(), E_INVALID_VALUES, "Failed to get asset path");
    int32_t errCode = E_OK;
    if (fileAsset->GetPhotoEditTime() == 0) { // the asset has not been edited before
        // Save video file in the photo direvtory to the .editdata directory
        errCode = SaveSourceVideoFile(fileAsset, assetPath);
        CHECK_AND_RETURN_RET_LOG(errCode == E_OK, E_FILE_OPER_FAIL,
            "Failed to save %{private}s to sourcePath, errCode: %{public}d", assetPath.c_str(), errCode);
    }

    string cacheMovingPhotoVideoName;
    GetStringFromValuesBucket(cmd.GetValueBucket(), CACHE_MOVING_PHOTO_VIDEO_NAME, cacheMovingPhotoVideoName);
    if (cacheMovingPhotoVideoName.empty()) {
        string videoPath = MediaFileUtils::GetMovingPhotoVideoPath(assetPath);
        CHECK_AND_RETURN_RET_LOG(!videoPath.empty(), E_INVALID_PATH, "Can not get video path");
        if (MediaFileUtils::IsFileExists(videoPath)) {
            MEDIA_INFO_LOG("Delete video file in photo directory, file is: %{private}s", videoPath.c_str());
            CHECK_AND_RETURN_RET_LOG(MediaFileUtils::DeleteFile(videoPath), E_HAS_FS_ERROR,
                "Failed to delete video file, path:%{private}s", videoPath.c_str());
        }
        errCode = UpdateMovingPhotoSubtype(fileAsset->GetId(), fileAsset->GetPhotoSubType());
        MEDIA_INFO_LOG("Moving photo graffiti editing, which becomes a normal photo, fileId:%{public}d",
            fileAsset->GetId());
    }
    return errCode;
}

int32_t MediaLibraryPhotoOperations::GetMovingPhotoCachePath(MediaLibraryCommand& cmd,
    const shared_ptr<FileAsset>& fileAsset, string& imageCachePath, string& videoCachePath)
{
    string imageCacheName;
    string videoCacheName;
    const ValuesBucket& values = cmd.GetValueBucket();
    GetStringFromValuesBucket(values, CACHE_FILE_NAME, imageCacheName);
    GetStringFromValuesBucket(values, CACHE_MOVING_PHOTO_VIDEO_NAME, videoCacheName);
    CHECK_AND_RETURN_RET_LOG(!imageCacheName.empty() || !videoCacheName.empty(),
        E_INVALID_VALUES, "Failed to check cache file of moving photo");

    string cacheDir = GetAssetCacheDir();
    if (!videoCacheName.empty()) {
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CheckMovingPhotoVideo(cacheDir + "/" + videoCacheName),
            E_INVALID_MOVING_PHOTO, "Failed to check cache video of moving photo");
    }

    string assetPath = fileAsset->GetPath();
    CHECK_AND_RETURN_RET_LOG(!assetPath.empty(), E_INVALID_PATH, "Failed to get image path of moving photo");
    string assetVideoPath = MediaFileUtils::GetMovingPhotoVideoPath(assetPath);
    CHECK_AND_RETURN_RET_LOG(!assetVideoPath.empty(), E_INVALID_PATH, "Failed to get video path of moving photo");

    if (imageCacheName.empty()) {
        imageCacheName = MediaFileUtils::GetTitleFromDisplayName(videoCacheName) + "_image." +
                         MediaFileUtils::GetExtensionFromPath(fileAsset->GetDisplayName());
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CopyFileUtil(assetPath, cacheDir + "/" + imageCacheName),
            E_HAS_FS_ERROR, "Failed to copy image to cache");
    }
    if (videoCacheName.empty()) {
        videoCacheName = MediaFileUtils::GetTitleFromDisplayName(imageCacheName) + "_video.mp4";
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CopyFileUtil(assetVideoPath, cacheDir + "/" + videoCacheName),
            E_HAS_FS_ERROR, "Failed to copy video to cache");
    }

    imageCachePath = cacheDir + "/" + imageCacheName;
    videoCachePath = cacheDir + "/" + videoCacheName;
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsFileExists(imageCachePath), E_NO_SUCH_FILE,
        "imageCachePath: %{private}s does not exist!", imageCachePath.c_str());
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsFileExists(videoCachePath), E_NO_SUCH_FILE,
        "videoCachePath: %{private}s does not exist!", videoCachePath.c_str());
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::SubmitEffectModeExecute(MediaLibraryCommand& cmd)
{
    int32_t id = -1;
    int32_t effectMode = -1;
    const ValuesBucket& values = cmd.GetValueBucket();
    CHECK_AND_RETURN_RET_LOG(GetInt32FromValuesBucket(values, PhotoColumn::MEDIA_ID, id) && id > 0,
        E_INVALID_VALUES, "Failed to get file id");
    CHECK_AND_RETURN_RET_LOG(GetInt32FromValuesBucket(values, PhotoColumn::MOVING_PHOTO_EFFECT_MODE, effectMode) &&
        MediaFileUtils::CheckMovingPhotoEffectMode(effectMode), E_INVALID_VALUES,
        "Failed to check effect mode: %{public}d", effectMode);
    vector<string> columns = { PhotoColumn::MEDIA_FILE_PATH, PhotoColumn::MEDIA_NAME, PhotoColumn::PHOTO_SUBTYPE,
        PhotoColumn::MEDIA_TIME_PENDING, PhotoColumn::MEDIA_DATE_TRASHED, PhotoColumn::PHOTO_EDIT_TIME,
        PhotoColumn::MOVING_PHOTO_EFFECT_MODE };
    shared_ptr<FileAsset> fileAsset = GetFileAssetFromDb(PhotoColumn::MEDIA_ID, to_string(id),
        OperationObject::FILESYSTEM_PHOTO, columns);
    int32_t errCode = CheckFileAssetStatus(fileAsset, true);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Failed to check status of fileAsset, id: %{public}d", id);

    string imageCachePath;
    string videoCachePath;
    errCode = GetMovingPhotoCachePath(cmd, fileAsset, imageCachePath, videoCachePath);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Failed to get cache file path of moving photo");

    string assetPath = fileAsset->GetPath();
    string assetVideoPath = MediaFileUtils::GetMovingPhotoVideoPath(assetPath);
    if (fileAsset->GetPhotoEditTime() == 0) { // save source moving photo
        string editDataDirPath = GetEditDataDirPath(assetPath);
        CHECK_AND_RETURN_RET_LOG(!editDataDirPath.empty(), E_INVALID_URI, "Failed to get edit dir path");
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateDirectory(editDataDirPath), E_HAS_FS_ERROR,
            "Failed to create dir %{private}s", editDataDirPath.c_str());

        string sourceImagePath = GetEditDataSourcePath(assetPath);
        CHECK_AND_RETURN_RET_LOG(!sourceImagePath.empty(), E_INVALID_PATH, "Cannot get source image path");
        string sourceVideoPath = MediaFileUtils::GetMovingPhotoVideoPath(sourceImagePath);
        CHECK_AND_RETURN_RET_LOG(!sourceVideoPath.empty(), E_INVALID_PATH, "Cannot get source video path");
        if (!MediaFileUtils::IsFileExists(sourceVideoPath)) {
            CHECK_AND_RETURN_RET_LOG(MediaFileUtils::ModifyAsset(assetVideoPath, sourceVideoPath) == E_SUCCESS,
                E_HAS_FS_ERROR, "Move file failed, srcPath:%{private}s, newPath:%{private}s", assetVideoPath.c_str(),
                sourceVideoPath.c_str());
        }
        if (!MediaFileUtils::IsFileExists(sourceImagePath)) {
            CHECK_AND_RETURN_RET_LOG(MediaFileUtils::ModifyAsset(assetPath, sourceImagePath) == E_SUCCESS,
                E_HAS_FS_ERROR, "Move file failed, srcPath:%{private}s, newPath:%{private}s", assetPath.c_str(),
                sourceImagePath.c_str());
        }
    }

    CHECK_AND_RETURN_RET_LOG(Move(imageCachePath, assetPath) == E_OK, E_HAS_FS_ERROR, "Failed to move image");
    CHECK_AND_RETURN_RET_LOG(Move(videoCachePath, assetVideoPath) == E_OK, E_HAS_FS_ERROR, "Failed to move video");
    CHECK_AND_RETURN_RET_LOG(UpdateEffectMode(id, effectMode) == E_OK, errCode, "Failed to update effect mode");
    ScanFile(assetPath, true, true, true);
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::SubmitCache(MediaLibraryCommand& cmd)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryPhotoOperations::SubmitCache");

    if (IsSetEffectMode(cmd)) {
        return SubmitEffectModeExecute(cmd);
    }

    const ValuesBucket& values = cmd.GetValueBucket();
    string fileName;
    CHECK_AND_RETURN_RET_LOG(GetStringFromValuesBucket(values, CACHE_FILE_NAME, fileName),
        E_INVALID_VALUES, "Failed to get fileName");
    string cacheDir = GetAssetCacheDir();
    string cachePath = cacheDir + "/" + fileName;
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsFileExists(cachePath), E_NO_SUCH_FILE,
        "cachePath: %{private}s does not exist!", cachePath.c_str());
    string movingPhotoVideoName;
    if (GetStringFromValuesBucket(values, CACHE_MOVING_PHOTO_VIDEO_NAME, movingPhotoVideoName)) {
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsFileExists(cacheDir + "/" + movingPhotoVideoName),
            E_NO_SUCH_FILE, "cahce moving video path: %{private}s does not exist!", cachePath.c_str());
    }

    int32_t id = 0;
    if (!GetInt32FromValuesBucket(values, PhotoColumn::MEDIA_ID, id)) {
        string displayName;
        CHECK_AND_RETURN_RET_LOG(GetStringFromValuesBucket(values, PhotoColumn::MEDIA_NAME, displayName),
            E_INVALID_VALUES, "Failed to get displayName");
        CHECK_AND_RETURN_RET_LOG(
            MediaFileUtils::GetExtensionFromPath(displayName) == MediaFileUtils::GetExtensionFromPath(fileName),
            E_INVALID_VALUES, "displayName mismatches extension of cache file name");
        ValuesBucket reservedValues = values;
        id = CreateV10(cmd);
        CHECK_AND_RETURN_RET_LOG(id > 0, E_FAIL, "Failed to create asset");
        cmd.SetValueBucket(reservedValues);
    }

    vector<string> columns = { PhotoColumn::MEDIA_ID, PhotoColumn::MEDIA_FILE_PATH, PhotoColumn::MEDIA_NAME,
        PhotoColumn::PHOTO_SUBTYPE, PhotoColumn::MEDIA_TIME_PENDING, PhotoColumn::MEDIA_DATE_TRASHED,
        PhotoColumn::PHOTO_EDIT_TIME };
    shared_ptr<FileAsset> fileAsset = GetFileAssetFromDb(
        PhotoColumn::MEDIA_ID, to_string(id), OperationObject::FILESYSTEM_PHOTO, columns);
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_VALUES,
        "Failed to getmapmanagerthread:: FileAsset, fileId=%{public}d", id);
    int32_t errCode = SubmitCacheExecute(cmd, fileAsset, cachePath);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Failed to submit cache, fileId=%{public}d", id);
    return id;
}

int32_t MediaLibraryPhotoOperations::ProcessMultistagesPhoto(bool isEdited, const std::string &path,
    const uint8_t *addr, const long bytes, int32_t fileId)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryPhotoOperations::ProcessMultistagesPhoto");
    string editDataSourcePath = GetEditDataSourcePath(path);
    string editDataCameraPath = GetEditDataCameraPath(path);

    if (isEdited) {
        // 图片编辑过了只替换低质量裸图
        return FileUtils::SaveImage(editDataSourcePath, (void*)addr, bytes);
    } else {
        if (!MediaFileUtils::IsFileExists(editDataCameraPath)) {
            // 图片没编辑过且没有editdata_camera，只落盘在Photo目录
            return FileUtils::SaveImage(path, (void*)addr, bytes);
        } else {
            // 图片没编辑过且有editdata_camera
            MediaLibraryTracer tracer;
            tracer.Start("MediaLibraryPhotoOperations::ProcessMultistagesPhoto AddFiltersToPhoto");
            // (1) 先替换低质量裸图
            int ret = FileUtils::SaveImage(editDataSourcePath, (void*)addr, bytes);
            if (ret != E_OK) {
                return ret;
            }
            // (2) 生成高质量水印滤镜图片
            string editData;
            CHECK_AND_RETURN_RET_LOG(ReadEditdataFromFile(editDataCameraPath, editData) == E_OK, E_HAS_FS_ERROR,
                "Failed to read editdata, path=%{public}s", editDataCameraPath.c_str());
            const string HIGH_QUALITY_PHOTO_STATUS = "high";
            CHECK_AND_RETURN_RET_LOG(
                AddFiltersToPhoto(editDataSourcePath, path, editData, HIGH_QUALITY_PHOTO_STATUS) == E_OK,
                E_FAIL, "Failed to add filters to photo");
            MediaLibraryObjectUtils::ScanFileAsync(path, to_string(fileId), MediaLibraryApi::API_10);
            return E_OK;
        }
    }
}

int32_t MediaLibraryPhotoOperations::ProcessMultistagesPhotoForPicture(bool isEdited, const std::string &path,
    std::shared_ptr<Media::Picture> &picture, int32_t fileId, const std::string &mime_type)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryPhotoOperations::ProcessMultistagesPhoto");
    string editDataSourcePath = GetEditDataSourcePath(path);
    string editDataCameraPath = GetEditDataCameraPath(path);

    if (isEdited) {
        // 图片编辑过了只替换低质量裸图
        return FileUtils::SavePicture(editDataSourcePath, picture, mime_type, isEdited);
    } else {
        if (!MediaFileUtils::IsFileExists(editDataCameraPath)) {
            // 图片没编辑过且没有editdata_camera，只落盘在Photo目录
            return FileUtils::SavePicture(path, picture, mime_type, isEdited);
        } else {
            // 图片没编辑过且有editdata_camera
            MediaLibraryTracer tracer;
            tracer.Start("MediaLibraryPhotoOperations::ProcessMultistagesPhoto AddFiltersToPhoto");
            // (1) 先替换低质量裸图
            int ret = FileUtils::SavePicture(editDataSourcePath, picture, mime_type, isEdited);
            if (ret != E_OK) {
                return ret;
            }
            // (2) 生成高质量水印滤镜图片
            string editData;
            CHECK_AND_RETURN_RET_LOG(ReadEditdataFromFile(editDataCameraPath, editData) == E_OK, E_HAS_FS_ERROR,
                "Failed to read editdata, path=%{public}s", editDataCameraPath.c_str());
            CHECK_AND_RETURN_RET_LOG(AddFiltersToPicture(picture, path, editData, mime_type) == E_OK, E_FAIL,
                "Failed to add filters to photo");
            return E_OK;
        }
    }
}

int32_t MediaLibraryPhotoOperations::AddFiltersToPhoto(const std::string &inputPath,
    const std::string &outputPath, const std::string &editdata, const std::string &photoStatus)
{
    MEDIA_INFO_LOG("AddFiltersToPhoto inputPath: %{public}s, outputPath: %{public}s, editdata: %{public}s",
        inputPath.c_str(), outputPath.c_str(), editdata.c_str());
    std::string info = editdata;
    size_t lastSlash = outputPath.rfind('/');
    CHECK_AND_RETURN_RET_LOG(lastSlash != string::npos && outputPath.size() > (lastSlash + 1), E_INVALID_VALUES,
        "Failed to check outputPath: %{public}s", outputPath.c_str());
    string tempOutputPath = outputPath.substr(0, lastSlash) +
        "/filters_" + photoStatus + outputPath.substr(lastSlash + 1);
    int32_t ret = MediaFileUtils::CreateAsset(tempOutputPath);
    CHECK_AND_RETURN_RET_LOG(ret == E_SUCCESS || ret == E_FILE_EXIST, E_HAS_FS_ERROR,
        "Failed to create temp filters file %{private}s", tempOutputPath.c_str());
    ret = MediaChangeEffect::TakeEffect(inputPath, tempOutputPath, info);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("MediaLibraryPhotoOperations: AddFiltersToPhoto: TakeEffect error. ret = %d", ret);
        return E_ERR;
    }

    string editDataPath = GetEditDataPath(outputPath);
    if (MediaFileUtils::IsFileExists(editDataPath)) {
        MEDIA_INFO_LOG("Editdata path: %{private}s exists, cannot add filters to photo", editDataPath.c_str());
        CHECK_AND_PRINT_LOG(MediaFileUtils::DeleteFile(tempOutputPath),
            "Failed to delete temp filters file, errno: %{public}d", errno);
        return E_OK;
    }

    ret = rename(tempOutputPath.c_str(), outputPath.c_str());
    if (ret < 0) {
        MEDIA_ERR_LOG("Failed to rename temp filters file, ret: %{public}d, errno: %{public}d", ret, errno);
        CHECK_AND_PRINT_LOG(MediaFileUtils::DeleteFile(tempOutputPath),
            "Failed to delete temp filters file, errno: %{public}d", errno);
        return ret;
    }
    MEDIA_INFO_LOG("AddFiltersToPhoto finish");
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::AddFiltersToPicture(std::shared_ptr<Media::Picture> &inPicture,
    const std::string &outputPath, string &editdata, const std::string &mime_type)
{
    if (inPicture == nullptr) {
        MEDIA_ERR_LOG("AddFiltersToPicture: picture is null");
        return E_ERR;
    }
    MEDIA_INFO_LOG("AddFiltersToPicture outputPath: %{public}s, editdata: %{public}s",
        outputPath.c_str(), editdata.c_str());
    size_t lastSlash = outputPath.rfind('/');
    CHECK_AND_RETURN_RET_LOG(lastSlash != string::npos && outputPath.size() > (lastSlash + 1), E_INVALID_VALUES,
        "Failed to check outputPath: %{public}s", outputPath.c_str());
    int32_t ret = MediaChangeEffect::TakeEffectForPicture(inPicture, editdata);
    FileUtils::DealPicture(mime_type, outputPath, inPicture);
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::ProcessMultistagesVideo(bool isEdited, const std::string &path)
{
    MEDIA_INFO_LOG("ProcessMultistagesVideo path:%{public}s, isEdited: %{public}d", path.c_str(), isEdited);
    return FileUtils::SaveVideo(path, isEdited);
}

int32_t MediaLibraryPhotoOperations::RemoveTempVideo(const std::string &path)
{
    MEDIA_INFO_LOG("RemoveTempVideo path: %{public}s", path.c_str());
    return FileUtils::DeleteTempVideoFile(path);
}

PhotoEditingRecord::PhotoEditingRecord()
{
}

shared_ptr<PhotoEditingRecord> PhotoEditingRecord::GetInstance()
{
    if (instance_ == nullptr) {
        lock_guard<mutex> lock(mutex_);
        if (instance_ == nullptr) {
            instance_ = make_shared<PhotoEditingRecord>();
        }
    }
    return instance_;
}

bool PhotoEditingRecord::StartCommitEdit(int32_t fileId)
{
    unique_lock<shared_mutex> lock(addMutex_);
    if (revertingPhotoSet_.count(fileId) > 0) {
        MEDIA_ERR_LOG("Photo %{public}d is reverting", fileId);
        return false;
    }
    editingPhotoSet_.insert(fileId);
    return true;
}

void PhotoEditingRecord::EndCommitEdit(int32_t fileId)
{
    unique_lock<shared_mutex> lock(addMutex_);
    editingPhotoSet_.erase(fileId);
}

bool PhotoEditingRecord::StartRevert(int32_t fileId)
{
    unique_lock<shared_mutex> lock(addMutex_);
    if (editingPhotoSet_.count(fileId) > 0) {
        MEDIA_ERR_LOG("Photo %{public}d is committing edit", fileId);
        return false;
    }
    revertingPhotoSet_.insert(fileId);
    return true;
}

void PhotoEditingRecord::EndRevert(int32_t fileId)
{
    unique_lock<shared_mutex> lock(addMutex_);
    revertingPhotoSet_.erase(fileId);
}

bool PhotoEditingRecord::IsInRevertOperation(int32_t fileId)
{
    shared_lock<shared_mutex> lock(addMutex_);
    return revertingPhotoSet_.count(fileId) > 0;
}

bool PhotoEditingRecord::IsInEditOperation(int32_t fileId)
{
    shared_lock<shared_mutex> lock(addMutex_);
    if (editingPhotoSet_.count(fileId) > 0 || revertingPhotoSet_.count(fileId) > 0) {
        return true;
    }
    return false;
}

void MediaLibraryPhotoOperations::StoreThumbnailSize(const string& photoId, const string& photoPath)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("Medialibrary rdbStore is nullptr!");
        return;
    }

    size_t LCDThumbnailSize = 0;
    size_t THMThumbnailSize = 0;
    size_t THMASTCThumbnailSize = 0;
    if (!MediaFileUtils::GetFileSize(GetThumbnailPath(photoPath, THUMBNAIL_LCD_SUFFIX), LCDThumbnailSize)) {
        MEDIA_WARN_LOG("Failed to get LCD thumbnail size for photo id %{public}s", photoId.c_str());
    }
    if (!MediaFileUtils::GetFileSize(GetThumbnailPath(photoPath, THUMBNAIL_THUMB_SUFFIX), THMThumbnailSize)) {
        MEDIA_WARN_LOG("Failed to get THM thumbnail size for photo id %{public}s", photoId.c_str());
    }
    if (!MediaFileUtils::GetFileSize(GetThumbnailPath(photoPath, THUMBNAIL_THUMBASTC_SUFFIX), THMASTCThumbnailSize)) {
        MEDIA_WARN_LOG("Failed to get THM_ASTC thumbnail size for photo id %{public}s", photoId.c_str());
    }
    size_t photoThumbnailSize = LCDThumbnailSize + THMThumbnailSize + THMASTCThumbnailSize;

    string sql = "INSERT OR REPLACE INTO " + PhotoExtColumn::PHOTOS_EXT_TABLE + " (" +
        PhotoExtColumn::PHOTO_ID + ", " + PhotoExtColumn::THUMBNAIL_SIZE +
        ") VALUES (" + photoId + ", " + to_string(photoThumbnailSize) + ")";

    int32_t ret = rdbStore->ExecuteSql(sql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to execute sql, photoId is %{public}s, error code is %{public}d", photoId.c_str(), ret);
        return;
    }
}

void MediaLibraryPhotoOperations::DropThumbnailSize(const string& photoId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("Medialibrary rdbStore is nullptr!");
        return;
    }

    string sql = "DELETE FROM " + PhotoExtColumn::PHOTOS_EXT_TABLE +
        " WHERE " + PhotoExtColumn::PHOTO_ID + " = " + photoId + ";";
    int32_t ret = rdbStore->ExecuteSql(sql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to execute sql, photoId is %{public}s, error code is %{public}d", photoId.c_str(), ret);
        return;
    }
}

shared_ptr<NativeRdb::ResultSet> MediaLibraryPhotoOperations::ScanMovingPhoto(MediaLibraryCommand &cmd,
    const vector<string> &columns)
{
    if (columns.empty()) {
        MEDIA_ERR_LOG("column is empty");
        return nullptr;
    }
    string uri = columns[0]; // 0 in columns predicates uri
    string path = MediaFileUri::GetPathFromUri(uri, true);
    string fileId = MediaFileUri::GetPhotoId(uri);
    MediaLibraryObjectUtils::ScanFileAsync(path, fileId, MediaLibraryApi::API_10);
    return nullptr;
}

int32_t MediaLibraryPhotoOperations::ScanFileWithoutAlbumUpdate(MediaLibraryCommand &cmd)
{
    if (!PermissionUtils::IsNativeSAApp()) {
        MEDIA_DEBUG_LOG("do not have permission");
        return E_VIOLATION_PARAMETERS;
    }
    const ValuesBucket &values = cmd.GetValueBucket();
    string uriString;
    if (!GetStringFromValuesBucket(values, MEDIA_DATA_DB_URI, uriString)) {
        return E_INVALID_VALUES;
    }
    string path = MediaFileUri::GetPathFromUri(uriString, true);
    string fileIdStr = MediaFileUri::GetPhotoId(uriString);
    int32_t fileId = 0;
    if (MediaLibraryDataManagerUtils::IsNumber(fileIdStr)) {
        fileId = atoi(fileIdStr.c_str());
    }
    MediaLibraryAssetOperations::ScanFileWithoutAlbumUpdate(path, false, false, true, fileId);

    return E_OK;
}

static void UpdateDirty(int32_t fileId)
{
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, fileId);
    predicates.EqualTo(PhotoColumn::PHOTO_QUALITY, static_cast<int32_t>(MultiStagesPhotoQuality::FULL));
    predicates.EqualTo(PhotoColumn::PHOTO_IS_TEMP, 0);
    predicates.EqualTo(PhotoColumn::PHOTO_DIRTY, -1);
    ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyTypes::TYPE_NEW));
    int32_t updateDirtyRows = MediaLibraryRdbStore::UpdateWithDateTime(values, predicates);
    MEDIA_INFO_LOG("update dirty to 1, file_id:%{public}d, changedRows:%{public}d", fileId, updateDirtyRows);
}

int32_t MediaLibraryPhotoOperations::DegenerateMovingPhoto(MediaLibraryCommand &cmd)
{
    vector<string> columns = { PhotoColumn::MEDIA_ID, PhotoColumn::MEDIA_FILE_PATH,
        PhotoColumn::MEDIA_NAME, PhotoColumn::PHOTO_SUBTYPE, PhotoColumn::PHOTO_EDIT_TIME };
    shared_ptr<FileAsset> fileAsset = GetFileAssetFromDb(*(cmd.GetAbsRdbPredicates()),
        OperationObject::FILESYSTEM_PHOTO, columns);
    if (fileAsset == nullptr) {
        MEDIA_ERR_LOG("failed to query fileAsset");
        return E_INVALID_VALUES;
    }

    if (fileAsset->GetPhotoSubType() != static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)) {
        MEDIA_INFO_LOG("fileAsset is not moving photo");
        return E_OK;
    }

    if (fileAsset->GetPhotoEditTime() > 0) {
        MEDIA_INFO_LOG("moving photo is edited");
        return E_OK;
    }

    string videoPath = MediaFileUtils::GetMovingPhotoVideoPath(fileAsset->GetFilePath());
    size_t videoSize = 0;
    if (MediaFileUtils::GetFileSize(videoPath, videoSize) && videoSize > 0) {
        MEDIA_INFO_LOG("no need to degenerate, video size:%{public}d", static_cast<int32_t>(videoSize));
        return E_OK;
    }

    if (MediaFileUtils::IsFileExists(videoPath)) {
        MEDIA_INFO_LOG("delete empty video file, size:%{public}d", static_cast<int32_t>(videoSize));
        (void)MediaFileUtils::DeleteFile(videoPath);
    }

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "Failed to get rdbStore");
    RdbPredicates predicates = RdbUtils::ToPredicates(cmd.GetDataSharePred(), PhotoColumn::PHOTOS_TABLE);
    MediaLibraryRdbStore::ReplacePredicatesUriToId(predicates);
    ValuesBucket values;
    values.Put(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::DEFAULT));
    int32_t updatedRows = rdbStore->UpdateWithDateTime(values, predicates);
    if (updatedRows <= 0) {
        MEDIA_WARN_LOG("Failed to update subtype, updatedRows=%{public}d", updatedRows);
        return updatedRows;
    }
    UpdateDirty(fileAsset->GetId());

    string extraUri = MediaFileUtils::GetExtraUri(fileAsset->GetDisplayName(), fileAsset->GetFilePath());
    auto watch = MediaLibraryNotify::GetInstance();
    watch->Notify(
        MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX, to_string(fileAsset->GetId()), extraUri),
        NotifyType::NOTIFY_UPDATE);
    return updatedRows;
}

int32_t MediaLibraryPhotoOperations::UpdateOwnerAlbumId(MediaLibraryCommand &cmd)
{
    const ValuesBucket &values = cmd.GetValueBucket();
    int32_t targetAlbumId = 0;
    CHECK_AND_RETURN_RET(
        GetInt32FromValuesBucket(values, PhotoColumn::PHOTO_OWNER_ALBUM_ID, targetAlbumId), E_HAS_DB_ERROR);

    vector<string> columns = { PhotoColumn::PHOTO_OWNER_ALBUM_ID, PhotoColumn::MEDIA_ID };
    auto predicates = cmd.GetAbsRdbPredicates();
    auto resultSetQuery = MediaLibraryRdbStore::QueryWithFilter(*predicates, columns);
    if (resultSetQuery == nullptr) {
        MEDIA_ERR_LOG("album id is not exist");
        return E_INVALID_ARGUMENTS;
    }
    if (resultSetQuery->GoToFirstRow() != NativeRdb::E_OK) {
        return E_HAS_DB_ERROR;
    }
    int32_t originalAlbumId = GetInt32Val(PhotoColumn::PHOTO_OWNER_ALBUM_ID, resultSetQuery);
    resultSetQuery->Close();

    int32_t rowId = UpdateFileInDb(cmd);
    if (rowId < 0) {
        MEDIA_ERR_LOG("Update Photo In database failed, rowId=%{public}d", rowId);
        return rowId;
    }
    auto watch = MediaLibraryNotify::GetInstance();
    MediaLibraryRdbUtils::UpdateSystemAlbumInternal(MediaLibraryUnistoreManager::GetInstance().GetRdbStore(),
        { to_string(PhotoAlbumSubType::IMAGE), to_string(PhotoAlbumSubType::VIDEO) });
    MediaLibraryRdbUtils::UpdateUserAlbumInternal(
        MediaLibraryUnistoreManager::GetInstance().GetRdbStore(), { to_string(originalAlbumId) });
    MediaLibraryRdbUtils::UpdateSourceAlbumInternal(
        MediaLibraryUnistoreManager::GetInstance().GetRdbStore(), { to_string(originalAlbumId) });
    MediaLibraryRdbUtils::UpdateUserAlbumInternal(
        MediaLibraryUnistoreManager::GetInstance().GetRdbStore(), { to_string(targetAlbumId) });
    MediaLibraryRdbUtils::UpdateSourceAlbumInternal(
        MediaLibraryUnistoreManager::GetInstance().GetRdbStore(), { to_string(targetAlbumId) });
    watch->Notify(PhotoColumn::PHOTO_URI_PREFIX + to_string(rowId),
        NotifyType::NOTIFY_ALBUM_REMOVE_ASSET, originalAlbumId);
    watch->Notify(PhotoColumn::PHOTO_URI_PREFIX + to_string(rowId),
        NotifyType::NOTIFY_ALBUM_ADD_ASSET, targetAlbumId);
    return rowId;
}
} // namespace Media
} // namespace OHOS
