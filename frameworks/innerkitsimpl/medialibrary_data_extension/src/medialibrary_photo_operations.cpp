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

#include <grp.h>
#include <unordered_set>
#include "album_plugin_config.h"
#include "directory_ex.h"
#include "duplicate_photo_operation.h"
#include "file_utils.h"
#include "map_operation_flag.h"
#include "media_analysis_helper.h"
#include "image_packer.h"
#include "media_change_effect.h"
#include "media_exif.h"
#include "medialibrary_album_helper.h"
#include "medialibrary_album_fusion_utils.h"
#include "medialibrary_analysis_album_operations.h"
#include "medialibrary_bundle_manager.h"
#include "medialibrary_data_manager_utils.h"
#include "medialibrary_json_operation.h"
#include "medialibrary_notify.h"
#include "medialibrary_object_utils.h"
#include "medialibrary_tracer.h"
#include "mimetype_utils.h"
#include "multistages_capture_manager.h"
#include "multistages_moving_photo_capture_manager.h"
#ifdef MEDIALIBRARY_FEATURE_CLOUD_ENHANCEMENT
#include "enhancement_manager.h"
#endif
#include "parameters.h"
#include "permission_utils.h"
#include "photo_custom_restore_operation.h"
#include "result_set_utils.h"
#include "thumbnail_service.h"
#include "medialibrary_formmap_operations.h"
#include "medialibrary_vision_operations.h"
#include "dfx_manager.h"
#include "dfx_utils.h"
#include "moving_photo_file_utils.h"
#include "hi_audit.h"
#include "video_composition_callback_imp.h"
#include "medialibrary_data_manager.h"
#include "shooting_mode_column.h"
#include "refresh_business_name.h"
#include "medialibrary_bundle_manager.h"
#include "medialibrary_transcode_data_aging_operation.h"
#include "lake_file_operations.h"
#include "lake_file_utils.h"
#include "tlv_util.h"

#include "camera_character_types.h"
#include "multistages_capture_notify_info.h"
#include "medialibrary_notify_new.h"
#include "multistages_capture_notify.h"
#include "cloud_media_define.h"
#include "cloud_media_common.h"

using namespace OHOS::DataShare;
using namespace std;
using namespace OHOS::NativeRdb;
using namespace OHOS::RdbDataShareAdapter;
//LCOV_EXCL_START
namespace OHOS {
namespace Media {
static const string ANALYSIS_HAS_DATA = "1";
const string PHOTO_ALBUM_URI_PREFIX_V0 = "file://media/PhotoAlbum/";
const string FILE_MANAGER_BUNDLE_NAME = "com." + AlbumPlugin::BRAND_NAME + ".hmos.filemanager";
constexpr int SAVE_PHOTO_WAIT_MS = 300;
constexpr int TASK_NUMBER_MAX = 5;

constexpr int32_t CROSS_POLICY_ERR = 18;
constexpr int32_t ORIENTATION_0 = 1;
constexpr int32_t ORIENTATION_90 = 6;
constexpr int32_t ORIENTATION_180 = 3;
constexpr int32_t ORIENTATION_270 = 8;
constexpr int32_t OFFSET = 5;
constexpr int32_t ZERO_ASCII = '0';
constexpr int32_t ORIGIN_VIDEO = 0;
constexpr int32_t XT_ORIGIN_VIDEO = 1;
constexpr int32_t XT_EFFECT_VIDEO = 2;
constexpr int64_t FIXED_PADDING_BYTES = 1024;
const std::string SET_LOCATION_KEY = "set_location";
const std::string SET_LOCATION_VALUE = "1";
const std::string SPECIAL_EDIT_COMPATIBLE_FORMAT = "system";
const std::string SPECIAL_EDIT_FORMAT_VERSION = "1.0";
const std::string SPECIAL_EDIT_EDIT_DATA = "";
const std::string SPECIAL_EDIT_APP_ID = "com.ohos.photos";
static const std::string ORIGIN_VIDEO_STR = "1";
const bool PROCESS_TRANSCODE_SIZE = false;
static const std::string CONTAIN_ADD_RESOURCE_FALSE = "0";
static const std::string CONTAIN_ADD_RESOURCE_TRUE = "1";

enum ImageFileType : int32_t {
    JPEG = 1,
    HEIF = 2
};

const std::string MULTI_USER_URI_FLAG = "user=";

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

const std::vector<std::string> CAMERA_BUNDLE_NAMES = {
    "com.huawei.hmos.camera"
};

struct DeleteBehaviorParams {
    map<string, string> displayNames;
    map<string, string> albumNames;
    map<string, string> ownerAlbumIds;
};

class DeleteBehaviorTaskData : public AsyncTaskData {
public:
    DeleteBehaviorTaskData() = default;
    ~DeleteBehaviorTaskData() override = default;

    std::string bundleName_;
    vector<string> notifyUris_;
    int32_t updatedRows_;
};

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
    CHECK_AND_RETURN_RET(MediaFileUtils::StartsWith(uriString, PhotoColumn::PHOTO_CACHE_URI_PREFIX), E_INVALID_VALUES);

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
        CHECK_AND_RETURN_RET_LOG(MediaLibraryDataManagerUtils::IsNumber(albumId),
            E_INVALID_ARGUMENTS, "invalid album id: %{public}s", albumId.c_str());
        PhotoAlbumColumns::GetUserAlbumPredicates(stoi(albumId), predicates, false);
        return E_SUCCESS;
    }

    if (PhotoAlbum::IsSourceAlbum(type, subType)) {
        CHECK_AND_RETURN_RET_LOG(MediaLibraryDataManagerUtils::IsNumber(albumId),
            E_INVALID_ARGUMENTS, "invalid album id: %{public}s", albumId.c_str());
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
        PhotoColumn::PHOTOS_TABLE + " WHERE " + MediaColumn::MEDIA_ID + " = " + photoId + " AND " +
        MediaColumn::MEDIA_DATE_TRASHED + " = 0 AND " + MediaColumn::MEDIA_HIDDEN + " = 0 AND " +
        PhotoColumn::PHOTO_IS_TEMP + " = 0 AND " + MediaColumn::MEDIA_TIME_PENDING + " = 0 ) ";
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

shared_ptr<NativeRdb::ResultSet> MediaLibraryPhotoOperations::HandleIndexOfUri(
    MediaLibraryCommand &cmd, RdbPredicates &predicates, const string &photoId, const string &albumId)
{
    CHECK_AND_RETURN_RET(albumId.empty(), HandleAlbumIndexOfUri(cmd, photoId, albumId));
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

static int32_t QueryAnalysisAlbumById(const string &albumId, PhotoAlbumSubType &subType, string &albumName)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "rdbStore is nullptr!");
    RdbPredicates predicates(ANALYSIS_ALBUM_TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, albumId);
    vector<string> columns = { PhotoAlbumColumns::ALBUM_SUBTYPE, PhotoAlbumColumns::ALBUM_NAME };
    auto resultSet = rdbStore->QueryByStep(predicates, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Analysis album id %{public}s is not exist", albumId.c_str());
        return E_INVALID_ARGUMENTS;
    }
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK, E_INVALID_ARGUMENTS,
        "Analysis album id %{public}s is not exist", albumId.c_str());
    subType = static_cast<PhotoAlbumSubType>(GetInt32Val(PhotoAlbumColumns::ALBUM_SUBTYPE, resultSet));
    albumName = GetStringVal(PhotoAlbumColumns::ALBUM_NAME, resultSet);
    resultSet->Close();
    return E_SUCCESS;
}

shared_ptr<NativeRdb::ResultSet> MediaLibraryPhotoOperations::HandleAnalysisIndex(MediaLibraryCommand &cmd,
    const string &photoId, const string &albumId)
{
    string orderClause;
    CHECK_AND_RETURN_RET_LOG(GetValidOrderClause(cmd.GetDataSharePred(), orderClause), nullptr, "invalid orderby");
    CHECK_AND_RETURN_RET_LOG(albumId.size() > 0, nullptr, "null albumId");
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    PhotoAlbumSubType albumSubtype;
    string albumName;
    CHECK_AND_RETURN_RET_LOG(QueryAnalysisAlbumById(albumId, albumSubtype, albumName) == E_SUCCESS, nullptr,
        "invalid analysis album id: %{public}s", albumId.c_str());
    MediaLibraryAlbumHelper::GetAnalysisAlbumPredicates(atoi(albumId.c_str()), albumSubtype,
        albumName, predicates, false);
    if (albumSubtype == PhotoAlbumSubType::PORTRAIT) {
        predicates.GroupBy({MediaColumn::MEDIA_ID});
    }
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
        return MediaLibraryPhotoOperations::HandleIndexOfUri(cmd, predicates, photoId, albumId);
    }
    CHECK_AND_RETURN_RET(cmd.GetOprnType() != OperationType::FIND_DUPLICATE_ASSETS,
        DuplicatePhotoOperation::GetAllDuplicateAssets(predicates, columns));
    CHECK_AND_RETURN_RET(cmd.GetOprnType() != OperationType::FIND_DUPLICATE_ASSETS_TO_DELETE,
        DuplicatePhotoOperation::GetDuplicateAssetsToDelete(predicates, columns));
    CHECK_AND_RETURN_RET(cmd.GetOprnType() != OperationType::UPDATE_SEARCH_INDEX,
        MediaLibraryRdbStore::Query(predicates, columns));
    CHECK_AND_RETURN_RET(cmd.GetOprnType() != OperationType::EDIT_DATA_EXISTS,
        MediaLibraryRdbStore::QueryEditDataExists(predicates));
    CHECK_AND_RETURN_RET(cmd.GetOprnType() != OperationType::MOVING_PHOTO_VIDEO_READY,
        MediaLibraryRdbStore::QueryMovingPhotoVideoReady(predicates));
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
    MediaColumn::MEDIA_HIDDEN,
    MediaColumn::MEDIA_DATE_TRASHED,
    MediaColumn::MEDIA_SIZE,
    MediaColumn::MEDIA_NAME,
    PhotoColumn::PHOTO_EXIST_COMPATIBLE_DUPLICATE,
};

bool CheckOpenMovingPhoto(int32_t photoSubType, int32_t effectMode, const string& request)
{
    return photoSubType == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO) ||
        (effectMode == static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY) &&
        request == SOURCE_REQUEST);
}

static void RefreshLivePhotoCache(const string &movingPhotoImagePath, int64_t movingPhotoSize)
{
    string livePhotoCachePath = MovingPhotoFileUtils::GetLivePhotoCachePath(movingPhotoImagePath);
    if (!MediaFileUtils::IsFileExists(livePhotoCachePath)) {
        return;
    }
    size_t livePhotoSize = 0;
    if (!MediaFileUtils::GetFileSize(livePhotoCachePath, livePhotoSize) ||
        static_cast<int64_t>(livePhotoSize) != movingPhotoSize) {
        MEDIA_INFO_LOG("Live photo need update from %{public}ld to %{public}ld",
            static_cast<long>(livePhotoSize), static_cast<long>(movingPhotoSize));
        CHECK_AND_PRINT_LOG(MediaFileUtils::DeleteFile(livePhotoCachePath),
            "Failed to delete live photo cache, errno: %{public}d", errno);
    }
}

int32_t GetMovingPhotoVideoInputPath(MediaLibraryCommand &cmd, string &inputPath, const string &imagePath, bool isTemp)
{
    string videoTypeStr = cmd.GetQuerySetParam(VIDEO_TYPE_KEYWORD);
    int32_t videoTypeNum = 0;

    if (videoTypeStr != "") {
        videoTypeNum = std::stoul(videoTypeStr);
    }

    if (videoTypeNum == XT_ORIGIN_VIDEO) {
        inputPath = isTemp ? MediaFileUtils::GetTempOriMovingPhotoVideoPath(imagePath)
            : MediaFileUtils::GetOriMovingPhotoVideoPath(imagePath);
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateDirectory(
            MediaLibraryAssetOperations::GetEditDataDirPath(imagePath)), E_HAS_FS_ERROR,
                "Can not create dir %{private}s", inputPath.c_str());
    } else {
        inputPath = isTemp ? MediaFileUtils::GetTempMovingPhotoVideoPath(imagePath)
            : MediaFileUtils::GetMovingPhotoVideoPath(imagePath);
        MEDIA_INFO_LOG("GetMovingPhotoVideoInputPath inputPath %{public}s, videoType %{public}d", inputPath.c_str(),
            cmd.GetVideoType());
    }
    MEDIA_INFO_LOG("GetMovingPhotoVideoInputPath inputPath %{public}s, videoType %{public}d",
        inputPath.c_str(), videoTypeNum);
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::ProcessMovingPhotoOprnKey(MediaLibraryCommand& cmd,
    shared_ptr<FileAsset>& fileAsset, const string& id, bool& isMovingPhotoVideo)
{
    string movingPhotoOprnKey = cmd.GetQuerySetParam(MEDIA_MOVING_PHOTO_OPRN_KEYWORD);
    if (movingPhotoOprnKey == OPEN_MOVING_PHOTO_VIDEO || movingPhotoOprnKey == CREATE_MOVING_PHOTO_VIDEO ||
        movingPhotoOprnKey == OPEN_MOVING_PHOTO_VIDEO_CLOUD) {
        bool isTemp = movingPhotoOprnKey == CREATE_MOVING_PHOTO_VIDEO;
        CHECK_AND_RETURN_RET_LOG(CheckOpenMovingPhoto(fileAsset->GetPhotoSubType(),
            fileAsset->GetMovingPhotoEffectMode(), cmd.GetQuerySetParam(MEDIA_OPERN_KEYWORD)),
            E_INVALID_VALUES,
            "Non-moving photo is requesting moving photo operation, file id: %{public}s, actual subtype: %{public}d",
            id.c_str(), fileAsset->GetPhotoSubType());
        string imagePath = fileAsset->GetPath();
        string inputPath;
        CHECK_AND_RETURN_RET_LOG(GetMovingPhotoVideoInputPath(cmd, inputPath, imagePath, isTemp) == E_OK,
            E_HAS_FS_ERROR, "Can not create dir for xtstle origin photo");
        fileAsset->SetPath(inputPath);
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
        int64_t movingPhotoSize = static_cast<int64_t>(MovingPhotoFileUtils::GetMovingPhotoSize(fileAsset->GetPath()));
        if (fileAsset->GetSize() != movingPhotoSize) {
            MEDIA_WARN_LOG("size of moving photo need scan from %{public}ld to %{public}ld",
                static_cast<long>(fileAsset->GetSize()), static_cast<long>(movingPhotoSize));
            MediaLibraryAssetOperations::ScanFileWithoutAlbumUpdate(fileAsset->GetPath(), false, false, true);
        }
        RefreshLivePhotoCache(fileAsset->GetPath(), movingPhotoSize);
        string livePhotoPath;
        CHECK_AND_RETURN_RET_LOG(MovingPhotoFileUtils::ConvertToLivePhoto(fileAsset->GetPath(),
            fileAsset->GetCoverPosition(), livePhotoPath) == E_OK,
            E_INVALID_VALUES,
            "Failed convert to live photo");
        fileAsset->SetPath(livePhotoPath);
    } else if (movingPhotoOprnKey == OPEN_PRIVATE_MOVING_PHOTO_METADATA) {
        string extraDataPath = MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(fileAsset->GetPath());
        fileAsset->SetPath(extraDataPath);
    }
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::ProcessCinematicVideoOprnKey(MediaLibraryCommand& cmd,
    shared_ptr<FileAsset>& fileAsset, const string& id)
{
    string cinematicVideoOprnKey = cmd.GetQuerySetParam(MEDIA_CINEMATIC_VIDEO_OPRN_KEYWORD);
    bool isValid = (cinematicVideoOprnKey == CREATE_CINEMATIC_VIDEO);
    CHECK_AND_RETURN_RET_INFO_LOG(isValid, E_OK,
        "cinematicVideoOprnKey is not valid. cinematicVideoOprnKey: %{public}s", cinematicVideoOprnKey.c_str());

    string videoPath = fileAsset->GetPath();
    MEDIA_INFO_LOG("ProcessCinematicVideoOprnKey videoPath %{private}s", videoPath.c_str());
    string inputPath;
    if (cmd.GetQuerySetParam(VIDEO_TYPE_KEYWORD) == ORIGIN_VIDEO_STR) {
        inputPath = PhotoFileUtils::GetEditDataSourcePath(videoPath);
        string editDataDirPath = GetEditDataDirPath(videoPath);
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateDirectory(editDataDirPath), E_HAS_FS_ERROR,
            "Can not create dir %{private}s", editDataDirPath.c_str());
    } else {
        inputPath = videoPath;
    }
    MEDIA_INFO_LOG("ProcessCinematicVideoOprnKey inputPath %{private}s, videoType %{public}s",
        inputPath.c_str(), cmd.GetQuerySetParam(VIDEO_TYPE_KEYWORD).c_str());
    fileAsset->SetPath(inputPath);
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
    size_t pos = uri.find("type=");
    if (pos != uri.npos) {
        type = uri[pos + OFFSET] - ZERO_ASCII;
    }
}

static bool CheckPermissionToOpenFileAsset(const shared_ptr<FileAsset>& fileAsset)
{
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, false, "File asset is nullptr");

    if (PermissionUtils::IsRootShell()) {
        return true;
    }

    if (fileAsset->GetDateTrashed() > 0) {
        bool cond = (!(PermissionUtils::IsSystemApp() || PermissionUtils::IsNativeSAApp() ||
            (PermissionUtils::IsHdcShell() &&
            OHOS::system::GetBoolParameter("const.security.developermode.state", true))));

        CHECK_AND_RETURN_RET_LOG(!cond, false,
            "Non-system app is not allowed to open trashed photo, %{public}d, %{public}d, %{public}d, "
            "%{public}d", PermissionUtils::IsSystemApp(), PermissionUtils::IsNativeSAApp(),
            PermissionUtils::IsHdcShell(),
            OHOS::system::GetBoolParameter("const.security.developermode.state", true));
    }

    if (fileAsset->IsHidden() && fileAsset->GetDateTrashed() == 0) {
        CHECK_AND_RETURN_RET_LOG(PermissionUtils::CheckCallerPermission(PERM_MANAGE_PRIVATE_PHOTOS),
            false, "MANAGE_PRIVATE_PHOTOS permission is required to open hidden photo");
        bool cond = (!(PermissionUtils::IsSystemApp() || PermissionUtils::IsNativeSAApp() ||
            (PermissionUtils::IsHdcShell() &&
            OHOS::system::GetBoolParameter("const.security.developermode.state", true))));

        CHECK_AND_RETURN_RET_LOG(!cond, false, "Non-system app is not allowed to open hidden photo,"
            " %{public}d, %{public}d, %{public}d, %{public}d", PermissionUtils::IsSystemApp(),
            PermissionUtils::IsNativeSAApp(), PermissionUtils::IsHdcShell(),
            OHOS::system::GetBoolParameter("const.security.developermode.state", true));

        string bundleName = MediaLibraryBundleManager::GetInstance()->GetClientBundleName();
        CHECK_AND_RETURN_RET_LOG(!(bundleName == FILE_MANAGER_BUNDLE_NAME), false,
            "The filemanager application is not allowed to open hidden photo");
    }
    return true;
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
    bool isHeif = cmd.GetQuerySetParam(PHOTO_TRANSCODE_OPERATION) == OPRN_TRANSCODE_HEIF;
    string uriString = cmd.GetUriStringWithoutSegment();
    string id = MediaFileUtils::GetIdFromUri(uriString);
    CHECK_AND_RETURN_RET(!uriString.empty() && MediaLibraryDataManagerUtils::IsNumber(id), E_INVALID_URI);
    string pendingStatus = cmd.GetQuerySetParam(MediaColumn::MEDIA_TIME_PENDING);
    string cmdUri = cmd.GetUri().ToString();
    size_t pos = cmdUri.find(MULTI_USER_URI_FLAG);
    string userId = "";
    if (pos != std::string::npos) {
        userId = cmdUri.substr(pos + MULTI_USER_URI_FLAG.length());
        uriString = uriString + "?" + MULTI_USER_URI_FLAG + userId;
    }
    shared_ptr<FileAsset> fileAsset = GetFileAssetByUri(uriString, true, PHOTO_COLUMN_VECTOR, pendingStatus);
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_URI,
        "Get FileAsset From Uri Failed, uri:%{public}s", uriString.c_str());
    CHECK_AND_RETURN_RET_LOG(CheckPermissionToOpenFileAsset(fileAsset),
        E_PERMISSION_DENIED, "Open not allowed");

    if (fileAsset->GetPhotoSubType() == static_cast<int32_t>(PhotoSubType::CINEMATIC_VIDEO)) {
        errCode = ProcessCinematicVideoOprnKey(cmd, fileAsset, id);
        CHECK_AND_RETURN_RET(errCode == E_OK, errCode);
    }

    bool isMovingPhotoVideo = false;
    errCode = ProcessMovingPhotoOprnKey(cmd, fileAsset, id, isMovingPhotoVideo);
    CHECK_AND_RETURN_RET(errCode == E_OK, errCode);
    UpdateLastVisitTime(cmd, id);
    string uri = cmd.GetUri().ToString();
    int32_t type = -1;
    GetType(uri, type);
    MEDIA_DEBUG_LOG("After spliting, uri is %{public}s, type is %{public}d", uri.c_str(), type);
    int32_t err = MediaLibraryTranscodeDataAgingOperation::SetTranscodeUriToFileAsset(fileAsset, mode, isHeif);
    int32_t ret = E_ERR;
    if (uriString.find(PhotoColumn::PHOTO_URI_PREFIX) != string::npos) {
        ret = OpenAsset(fileAsset, mode, MediaLibraryApi::API_10, isMovingPhotoVideo, type);
    } else {
        ret = OpenAsset(fileAsset, mode, cmd.GetApi(), type);
    }
    if (err == 0 && ret >= 0) {
        MediaLibraryTranscodeDataAgingOperation::DoTranscodeDfx(ACCESS_MEDIALIB);
    }
    return ret;
}

int32_t MediaLibraryPhotoOperations::Close(MediaLibraryCommand &cmd)
{
    const ValuesBucket &values = cmd.GetValueBucket();
    string uriString;
    CHECK_AND_RETURN_RET(GetStringFromValuesBucket(values, MEDIA_DATA_DB_URI, uriString), E_INVALID_VALUES);
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
    auto assetRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>(trans);
    std::function<int(void)> func = [&]()->int {
        errCode = SetAssetPathInCreate(fileAsset, trans);
        CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode,
            "Failed to Solve FileAsset Path and Name, displayName=%{private}s", displayName.c_str());

        outRow = InsertAssetInDb(trans, cmd, fileAsset, assetRefresh);
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
    assetRefresh->RefreshAlbum();
    assetRefresh->Notify();
    return outRow;
}

void PhotosAddAsset(const int &albumId, const string &assetId, const string &extrUri)
{
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_LOG(watch != nullptr, "Can not get MediaLibraryNotify Instance");
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

int32_t MediaLibraryPhotoOperations::CreateV10(MediaLibraryCommand &cmd)
{
    FileAsset fileAsset;
    ValuesBucket &values = cmd.GetValueBucket();
    string displayName;
    string extention;
    bool isContains = false;
    bool isNeedGrant = false;
    if (GetStringFromValuesBucket(values, PhotoColumn::MEDIA_NAME, displayName)) {
        SetAssetDisplayName(displayName, fileAsset, isContains);
        fileAsset.SetTimePending(UNCREATE_FILE_TIMEPENDING);
    } else {
        CHECK_AND_RETURN_RET(GetStringFromValuesBucket(values, ASSET_EXTENTION, extention), E_HAS_DB_ERROR);
        isNeedGrant = true;
        fileAsset.SetTimePending(UNOPEN_FILE_COMPONENT_TIMEPENDING);
        string title;
        if (GetStringFromValuesBucket(values, PhotoColumn::MEDIA_TITLE, title)) {
            displayName = title + "." + extention;
            SetAssetDisplayName(displayName, fileAsset, isContains);
        }
    }
    int32_t mediaType = 0;
    CHECK_AND_RETURN_RET(GetInt32FromValuesBucket(values, PhotoColumn::MEDIA_TYPE, mediaType), E_HAS_DB_ERROR);
    int32_t fileResourceType = 0;
    GetInt32FromValuesBucket(values, PhotoColumn::PHOTO_FILE_SOURCE_TYPE, fileResourceType);
    fileAsset.SetFileResourceType(fileResourceType);
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
        errCode = isContains ? SetAssetPathInCreate(fileAsset, trans) : SetAssetPath(fileAsset, extention, trans);
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
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "CreateV10: trans retry fail!, ret:%{public}d", errCode);
    string fileUri = CreateExtUriForV10Asset(fileAsset);
    if (isNeedGrant) {
        bool isMovingPhoto = fileAsset.GetPhotoSubType() == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO);
        int32_t ret = GrantUriPermission(fileUri, cmd.GetBundleName(), fileAsset.GetPath(), isMovingPhoto);
        CHECK_AND_RETURN_RET(ret == E_OK, ret);
    }
    cmd.SetResult(fileUri);
    MediaLibraryObjectUtils::TryUpdateAnalysisProp(ANALYSIS_HAS_DATA);
    return outRow;
}

int32_t MediaLibraryPhotoOperations::DeletePhoto(const shared_ptr<FileAsset> &fileAsset, MediaLibraryApi api,
    shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh)
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
    int32_t deleteRows = DeleteAssetInDb(cmd, assetRefresh);
    CHECK_AND_RETURN_RET_LOG(deleteRows > 0, E_HAS_DB_ERROR,
        "Delete photo in database failed, errCode=%{public}d", deleteRows);

    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_RET_LOG(watch != nullptr, E_ERR, "Can not get MediaLibraryNotify Instance");
    string notifyDeleteUri =
        MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX, to_string(deleteRows),
        (api == MediaLibraryApi::API_10 ? MediaFileUtils::GetExtraUri(displayName, filePath) : ""));
    watch->Notify(notifyDeleteUri, NotifyType::NOTIFY_REMOVE);
    if (assetRefresh !=nullptr) {
        assetRefresh->Notify();
    }
    DeleteRevertMessage(filePath);
    return deleteRows;
}

void MediaLibraryPhotoOperations::TrashPhotosSendNotify(const vector<string> &notifyUris,
    shared_ptr<AlbumData> albumData)
{
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_LOG(watch != nullptr, "Can not get MediaLibraryNotify Instance");
    int trashAlbumId = watch->GetAlbumIdBySubType(PhotoAlbumSubType::TRASH);
    CHECK_AND_RETURN_LOG(trashAlbumId > 0,
        "Skip to send trash photos notify, trashAlbumId=%{public}d", trashAlbumId);
    CHECK_AND_RETURN_LOG(!notifyUris.empty(), "Skip to send trash photos notify, notifyUris is empty.");
    if (notifyUris.size() == 1) {
        if (albumData != nullptr && albumData->isHidden.size() != 0) {
            watch->Notify(notifyUris[0], albumData->isHidden[0] ?
                NotifyType::NOTIFY_UPDATE : NotifyType::NOTIFY_REMOVE);
        } else {
            watch->Notify(notifyUris[0], NotifyType::NOTIFY_REMOVE);
        }
        watch->Notify(notifyUris[0], NotifyType::NOTIFY_ALBUM_REMOVE_ASSET);
        watch->Notify(notifyUris[0], NotifyType::NOTIFY_ALBUM_ADD_ASSET, trashAlbumId);
    } else {
        watch->Notify(PhotoColumn::PHOTO_URI_PREFIX, NotifyType::NOTIFY_UPDATE);
        watch->Notify(PhotoAlbumColumns::ALBUM_URI_PREFIX, NotifyType::NOTIFY_REMOVE);
    }

    if (MAP_OPERATION_FLAG) {
        MEDIA_INFO_LOG("map operation flag is true");
        vector<int64_t> formIds;
        MediaLibraryFormMapOperations::GetFormIdsByUris(notifyUris, formIds);
        if (!formIds.empty()) {
            MediaLibraryFormMapOperations::PublishedChange("", formIds, false);
        }
    }
}

static void GetPhotoHiddenStatus(std::shared_ptr<AlbumData> data, const string& assetId)
{
    if (data == nullptr) {
        return;
    }
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "Failed to get rdbStore.");
    const std::string queryAssetHiddenInfo = "SELECT hidden FROM Photos WHERE file_id = " + assetId;
    shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(queryAssetHiddenInfo);
    if (resultSet == nullptr) {
        return;
    }
    int32_t isHidden = 0;
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t isHiddenIndex = -1;
        resultSet->GetColumnIndex(MediaColumn::MEDIA_HIDDEN, isHiddenIndex);
        if (resultSet->GetInt(isHiddenIndex, isHidden) != NativeRdb::E_OK) {
            return;
        }
    }
    data->isHidden.push_back(isHidden);
}

void MediaLibraryPhotoOperations::UpdateSourcePath(const vector<string> &whereArgs)
{
    if (whereArgs.empty()) {
        MEDIA_WARN_LOG("whereArgs is empty");
        return;
    }

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "RdbStore is null");

    std::string inClause;
    for (size_t i = 0; i < whereArgs.size(); ++i) {
        if (i > 0) {
            inClause += ",";
        }
        inClause += whereArgs[i];
    }

    const std::string updateSql = "UPDATE Photos "
        "SET source_path = ("
            "SELECT "
                "CASE "
                    "WHEN COALESCE(PhotoAlbum.lpath, '') = '/' THEN "
                        "'/storage/emulated/0' || '/' || SubPhotos.display_name "
                    "WHEN COALESCE(PhotoAlbum.lpath, '') <> '/' AND "
                        "COALESCE(PhotoAlbum.lpath, '') <> '' THEN "
                        "'/storage/emulated/0' || PhotoAlbum.lpath || '/' || SubPhotos.display_name "
                    "WHEN COALESCE(PhotoAlbum.lpath, '') = '' AND "
                        "COALESCE(Photos.source_path, '') = '' THEN "
                        "'/storage/emulated/0/Pictures/其它/' || SubPhotos.display_name "
                    "ELSE "
                        "Photos.source_path "
                "END "
            "FROM Photos AS SubPhotos "
            "LEFT JOIN PhotoAlbum ON SubPhotos.owner_album_id = PhotoAlbum.album_id "
            "WHERE SubPhotos.file_id = Photos.file_id "
            "LIMIT 1 "
        ") "
        "WHERE file_id IN (" + inClause + ")";
    int32_t result = rdbStore->ExecuteSql(updateSql);
    CHECK_AND_PRINT_LOG(result == NativeRdb::E_OK, "Failed to update source path, error code: %{private}d", result);
}

static void GetAlbumNamesById(DeleteBehaviorParams &filesParams)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetAlbumNamesById");
    CHECK_AND_RETURN_LOG(!filesParams.ownerAlbumIds.empty(), "ownerAlbumIds is empty");
    vector<string> albumIdList;
    set<string> albumIdSet;
    for (const auto &fileId : filesParams.ownerAlbumIds) {
        albumIdSet.insert(fileId.second);
    }
    albumIdList.assign(albumIdSet.begin(), albumIdSet.end());
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(uniStore != nullptr, "rdbstore is nullptr");
    MediaLibraryCommand queryAlbumMapCmd(OperationObject::PAH_ALBUM, OperationType::QUERY);
    queryAlbumMapCmd.GetAbsRdbPredicates()->In(PhotoAlbumColumns::ALBUM_ID, albumIdList);
    auto resultSet = uniStore->Query(queryAlbumMapCmd, {PhotoAlbumColumns::ALBUM_ID, PhotoAlbumColumns::ALBUM_NAME});
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Failed to query resultSet");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        filesParams.albumNames.insert({
            to_string(get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_ID, resultSet,
                TYPE_INT32))),
            get<string>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_NAME, resultSet,
                TYPE_STRING))});
    }
    resultSet->Close();
}

static void GetFilesParams(const vector<string> &notifyUris, DeleteBehaviorParams &filesParams)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetFilesParams");
    vector<string> photoIdList;
    for (const auto &uri : notifyUris) {
        photoIdList.push_back(MediaFileUri::GetPhotoId(uri));
    }
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(uniStore != nullptr, "rdbstore is nullptr");
    MediaLibraryCommand queryAlbumMapCmd(OperationObject::PAH_PHOTO, OperationType::QUERY);
    queryAlbumMapCmd.GetAbsRdbPredicates()->In(MediaColumn::MEDIA_ID, photoIdList);
    auto resultSet = uniStore->Query(queryAlbumMapCmd, {MediaColumn::MEDIA_ID, MediaColumn::MEDIA_NAME,
        PhotoColumn::PHOTO_OWNER_ALBUM_ID});
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Failed to query resultSet");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        filesParams.displayNames.insert({
            to_string(get<int32_t>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_ID, resultSet, TYPE_INT32))),
            get<string>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_NAME, resultSet, TYPE_STRING))});
        filesParams.ownerAlbumIds.insert({
            to_string(get<int32_t>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_ID, resultSet, TYPE_INT32))),
            to_string(get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_OWNER_ALBUM_ID, resultSet,
                TYPE_INT32)))});
    }
    resultSet->Close();
}

static void DeleteBehaviorAsync(AsyncTaskData *data)
{
    MEDIA_DEBUG_LOG("DeleteBehaviorAsync start.");
    CHECK_AND_RETURN_LOG(data != nullptr, "task data is nullptr");
    auto *taskData = static_cast<DeleteBehaviorTaskData *>(data);
    CHECK_AND_RETURN_LOG(taskData != nullptr, "taskData is nullptr");
    DeleteBehaviorParams filesParams;
    GetFilesParams(taskData->notifyUris_, filesParams);
    GetAlbumNamesById(filesParams);
    DeleteBehaviorData dataInfo {filesParams.displayNames, filesParams.albumNames, filesParams.ownerAlbumIds};
    DfxManager::GetInstance()->HandleDeleteBehavior(DfxType::TRASH_PHOTO,
        taskData->updatedRows_, taskData->notifyUris_, taskData->bundleName_, dataInfo);
}

static void HandleQualityAndHiddenSingle(NativeRdb::AbsRdbPredicates predicates,  const vector<string> &fileIds,
    std::shared_ptr<AlbumData> albumData)
{
    vector<string> columns { MediaColumn::MEDIA_ID, MEDIA_DATA_DB_PHOTO_ID, MEDIA_DATA_DB_PHOTO_QUALITY,
        MEDIA_DATA_DB_MEDIA_TYPE, MEDIA_DATA_DB_STAGE_VIDEO_TASK_STATUS, MediaColumn::MEDIA_FILE_PATH,
        PhotoColumn::PHOTO_SUBTYPE, MediaColumn::MEDIA_HIDDEN };
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Result set is empty");
        if (albumData != nullptr && albumData->isHidden.size() == 0) {
            GetPhotoHiddenStatus(albumData, fileIds[0]);
        }
        return;
    }
    int32_t isHiddenIndex = -1;
    int32_t isHidden = 0;
    resultSet->GetColumnIndex(MediaColumn::MEDIA_HIDDEN, isHiddenIndex);
    if (resultSet->GetInt(isHiddenIndex, isHidden) == NativeRdb::E_OK) {
        albumData->isHidden.push_back(isHidden);
    }
    int32_t quality = GetInt32Val(PhotoColumn::PHOTO_QUALITY, resultSet);
    int32_t taskStatus = GetInt32Val(MEDIA_DATA_DB_STAGE_VIDEO_TASK_STATUS, resultSet);
    if (quality == static_cast<int32_t>(MultiStagesPhotoQuality::LOW)
        || taskStatus == static_cast<int32_t>(StageVideoTaskStatus::STAGE_TASK_DELIVERED)) {
        MultiStagesCaptureManager::RemovePhotosWithResultSet(resultSet, true);
    }
    resultSet->Close();
}

static void HandleQualityAndHiddenBatch(NativeRdb::AbsRdbPredicates predicates, const vector<string> &fileIds,
    std::shared_ptr<AlbumData> albumData)
{
    string where = predicates.GetWhereClause() + " AND (" + PhotoColumn::PHOTO_QUALITY + "=" +
        to_string(static_cast<int32_t>(MultiStagesPhotoQuality::LOW)) + " OR " +
        PhotoColumn::STAGE_VIDEO_TASK_STATUS + " = " +
        to_string(static_cast<int32_t>(StageVideoTaskStatus::STAGE_TASK_DELIVERED)) + ")";
    predicates.SetWhereClause(where);
    vector<string> columns { MediaColumn::MEDIA_ID, MEDIA_DATA_DB_PHOTO_ID, MEDIA_DATA_DB_PHOTO_QUALITY,
        MEDIA_DATA_DB_MEDIA_TYPE, MEDIA_DATA_DB_STAGE_VIDEO_TASK_STATUS, MediaColumn::MEDIA_FILE_PATH,
        PhotoColumn::PHOTO_SUBTYPE, MediaColumn::MEDIA_HIDDEN };
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Result set is empty");
        if (albumData != nullptr && albumData->isHidden.size() == 0) {
            GetPhotoHiddenStatus(albumData, fileIds[0]);
        }
        return;
    }
    int32_t isHiddenIndex = -1;
    int32_t isHidden = 0;
    resultSet->GetColumnIndex(MediaColumn::MEDIA_HIDDEN, isHiddenIndex);
    if (resultSet->GetInt(isHiddenIndex, isHidden) == NativeRdb::E_OK) {
        albumData->isHidden.push_back(isHidden);
    }
    do {
        MultiStagesCaptureManager::RemovePhotosWithResultSet(resultSet, true);
    } while (resultSet->GoToNextRow() == NativeRdb::E_OK);
    resultSet->Close();
}

static void HandleQualityAndHidden(NativeRdb::RdbPredicates predicates, const vector<string> &fileIds,
    std::shared_ptr<AlbumData> albumData)
{
    if (predicates.GetTableName() != PhotoColumn::PHOTOS_TABLE) {
        MEDIA_INFO_LOG("Invalid table name: %{public}s", predicates.GetTableName().c_str());
        return;
    }
    NativeRdb::AbsRdbPredicates predicatesNew(predicates.GetTableName());
    predicatesNew.SetWhereClause(predicates.GetWhereClause());
    predicatesNew.SetWhereArgs(predicates.GetWhereArgs());
    if (predicates.GetWhereArgs().size() > 1) {
        HandleQualityAndHiddenBatch(predicatesNew, fileIds, albumData);
    } else {
        HandleQualityAndHiddenSingle(predicatesNew, fileIds, albumData);
    }
}

static void GetBurstMemberIds(vector<string> &fileIds)
{
    CHECK_AND_RETURN_LOG(!fileIds.empty(), "GetBurstMemberIds fileIds is empty");
    string inClause = Media::CloudMediaCommon::ToStringWithComma(fileIds);
    /**
        SELECT p.file_id
        FROM photos p
        WHERE p.PHOTO_BURST_KEY IN (
            SELECT DISTINCT p1.PHOTO_BURST_KEY
            FROM photos p1
            WHERE p1.fileid IN (1, 2, 3)
            AND p1.PHOTO_BURST_COVER_LEVEL = 1
            AND p1.PHOTO_SUBTYPE = 4
        )
        AND p.PHOTO_BURST_COVER_LEVEL = 2
    */
    string sql = "SELECT p." + MediaColumn::MEDIA_ID
        + " FROM " + PhotoColumn::PHOTOS_TABLE + " p "
        + " WHERE p." + PhotoColumn::PHOTO_BURST_KEY + " IN ("
        + " SELECT DISTINCT p1." + PhotoColumn::PHOTO_BURST_KEY
        + " FROM " + PhotoColumn::PHOTOS_TABLE + " p1 "
        + " WHERE p1." + MediaColumn::MEDIA_ID + " IN (" + inClause + ") "
        + " AND p1." + PhotoColumn::PHOTO_BURST_COVER_LEVEL + " = "
        + std::to_string(static_cast<int32_t>(BurstCoverLevelType::COVER))
        + " AND p1." + PhotoColumn::PHOTO_SUBTYPE + " = "
        + std::to_string(static_cast<int32_t>(PhotoSubType::BURST))
        + " ) "
        + " AND p." + PhotoColumn::PHOTO_BURST_COVER_LEVEL + " = "
        + std::to_string(static_cast<int32_t>(BurstCoverLevelType::MEMBER));
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "get rdb store fail");
    auto resultSet = rdbStore->QuerySql(sql);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Failed to query selected files!");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        string burstMemberFileId = to_string(get<int32_t>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_ID,
            resultSet, TYPE_INT32)));
        fileIds.push_back(burstMemberFileId);
    }
    resultSet->Close();
}

int32_t MediaLibraryPhotoOperations::TrashPhotos(MediaLibraryCommand &cmd)
{
    AccurateRefresh::AssetAccurateRefresh assetRefresh(AccurateRefresh::TRASH_PHOTOS_BUSSINESS_NAME);
    NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(cmd.GetDataSharePred(),
        PhotoColumn::PHOTOS_TABLE);
    vector<string> notifyUris = rdbPredicate.GetWhereArgs();
    MediaLibraryRdbStore::ReplacePredicatesUriToId(rdbPredicate);
    std::shared_ptr<AlbumData> albumData = std::make_shared<AlbumData>();
    vector<string> fileIds = rdbPredicate.GetWhereArgs();
    MEDIA_INFO_LOG("Start trash %{public}zu photos", fileIds.size());
    HandleQualityAndHidden(rdbPredicate, fileIds, albumData);

    // 1、AssetRefresh -> Init(rdbPredicate)
    UpdateSourcePath(fileIds);
    ValuesBucket values;
    values.Put(MediaColumn::MEDIA_DATE_TRASHED, MediaFileUtils::UTCTimeMilliSeconds());
    cmd.SetValueBucket(values);
    // 2、AssetRefresh -> Update()
    // 删除适配连拍照片
    GetBurstMemberIds(fileIds);
    CHECK_AND_PRINT_LOG(!fileIds.empty(), "get burst member photo failed. fileIds is empty.");
    rdbPredicate.Clear();
    rdbPredicate.In(PhotoColumn::MEDIA_ID, fileIds);
    rdbPredicate.EqualTo(MediaColumn::MEDIA_DATE_TRASHED, to_string(0));
    int32_t updatedRows = assetRefresh.UpdateWithDateTime(values, rdbPredicate);
    // 3、AssetRefresh -> RefreshAlbums()
    assetRefresh.RefreshAlbum(NotifyAlbumType::SYS_ALBUM);
    CHECK_AND_RETURN_RET_LOG(updatedRows >= 0, E_HAS_DB_ERROR, "Trash photo failed. Result %{public}d.", updatedRows);
    int32_t ret = LakeFileOperations::MoveAssetsFromLake(rdbPredicate.GetWhereArgs());
    CHECK_AND_PRINT_LOG(ret == E_OK, "trash inner anco file error");
    // delete cloud enhanacement task
#ifdef MEDIALIBRARY_FEATURE_CLOUD_ENHANCEMENT
    vector<string> photoIds;
    EnhancementManager::GetInstance().CancelTasksInternal(fileIds, photoIds, CloudEnhancementAvailableType::TRASH);
#endif
    MediaAnalysisHelper::StartMediaAnalysisServiceAsync(
        static_cast<int32_t>(MediaAnalysisProxy::ActivateServiceType::START_UPDATE_INDEX), notifyUris);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "Failed to get rdbStore");
    MediaLibraryRdbUtils::UpdateAnalysisAlbumByUri(rdbStore, notifyUris);
    CHECK_AND_WARN_LOG(static_cast<size_t>(updatedRows) == notifyUris.size(),
        "Try to notify %{public}zu items, but only %{public}d items updated.", notifyUris.size(), updatedRows);
    // 4、AssetRefresh -> Notify()
    assetRefresh.Notify();
    TrashPhotosSendNotify(notifyUris, albumData);
    auto asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    CHECK_AND_RETURN_RET_LOG(asyncWorker != nullptr, updatedRows, "Failed to get async worker instance!");
    auto *taskData = new (std::nothrow) DeleteBehaviorTaskData();
    CHECK_AND_RETURN_RET_LOG(taskData != nullptr, updatedRows, "Failed to alloc async data!");
    taskData->notifyUris_ = move(notifyUris);
    taskData->updatedRows_ = updatedRows;
    taskData->bundleName_ = MediaLibraryBundleManager::GetInstance()->GetClientBundleName();
    auto asyncTask = std::make_shared<MediaLibraryAsyncTask>(DeleteBehaviorAsync, taskData);
    asyncWorker->AddTask(asyncTask, false);
    return updatedRows;
}

int32_t GetPhotoIdByFileId(int32_t fileId, std::string &photoId)
{
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, std::to_string(fileId));

    std::vector<std::string> columns = { PhotoColumn::PHOTO_ID };
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK,
        E_FILE_EXIST, "result set is empty");
    
    photoId = GetStringVal(PhotoColumn::PHOTO_ID, resultSet);
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::DiscardCameraPhoto(MediaLibraryCommand &cmd)
{
    std::string fileId = cmd.GetQuerySetParam(PhotoColumn::MEDIA_ID);
    MEDIA_INFO_LOG("MultistagesCapture, start discard fileId: %{public}s", fileId.c_str());
    bool isClearCachedPicture = false;
    std::string photoId;
    int32_t ret = E_ERR;
    if (!fileId.empty() && MediaLibraryDataManagerUtils::IsNumber(fileId)) {
        MEDIA_INFO_LOG("MultistagesCapture start discard fileId: %{public}s.", fileId.c_str());
        isClearCachedPicture = true;
        ret = GetPhotoIdByFileId(stoi(fileId), photoId);
        if (ret != E_OK || photoId.empty()) {
            MEDIA_WARN_LOG("MultistagesCapture Memory leak may occur, please check yuv picture.");
            isClearCachedPicture = false;
        }
    }

    if (isClearCachedPicture) {
        MEDIA_INFO_LOG("MultistagesCapture start clear cached picture, photoId: %{public}s.", photoId.c_str());
        auto pictureManagerThread = PictureManagerThread::GetInstance();
        if (pictureManagerThread != nullptr) {
            pictureManagerThread->DeleteDataWithImageId(photoId, LOW_QUALITY_PICTURE);
        }
        MultistagesCaptureNotify::NotifyLowQualityMemoryCount();
        MultiStagesPhotoCaptureManager::GetInstance().CancelProcessRequest(photoId);
    }

    NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(cmd.GetDataSharePred(),
        PhotoColumn::PHOTOS_TABLE);
    vector<string> notifyUris = rdbPredicate.GetWhereArgs();
    MediaLibraryRdbStore::ReplacePredicatesUriToId(rdbPredicate);
    ret = MediaLibraryAssetOperations::DeleteFromDisk(rdbPredicate, false, true);
    MEDIA_INFO_LOG("MultistagesCapture discard end, ret: %{public}d.", ret);
    return ret;
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
    predicates.EqualTo(PhotoColumn::PHOTO_QUALITY, to_string(static_cast<int32_t>(MultiStagesPhotoQuality::FULL)));
    predicates.NotEqualTo(PhotoColumn::PHOTO_SUBTYPE, to_string(static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)));
    ValuesBucket valuesBucketDirty;
    valuesBucketDirty.Put(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_NEW));
    return MediaLibraryRdbStore::UpdateWithDateTime(valuesBucketDirty, predicates);
}

static int32_t UpdateThirdPartyPhotoDirtyFlag(
    ValuesBucket &values, const RdbPredicates &predicates, AccurateRefresh::AssetAccurateRefresh &assetRefresh)
{
    values.Put(PhotoColumn::PHOTO_QUALITY, static_cast<int32_t>(MultiStagesPhotoQuality::FULL));
    values.Put(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_NEW));
    return assetRefresh.UpdateWithDateTime(values, predicates);
}

static void UpdateValuesBucketForExt(MediaLibraryCommand &cmd, ValuesBucket &values)
{
    ValuesBucket &cmdValues = cmd.GetValueBucket();
    int32_t supportedWatermarkType = 0;
    if (MediaLibraryAssetOperations::GetInt32FromValuesBucket(cmdValues,
        PhotoColumn::SUPPORTED_WATERMARK_TYPE, supportedWatermarkType)) {
        values.Put(PhotoColumn::SUPPORTED_WATERMARK_TYPE, supportedWatermarkType);
    }

    std::string cameraShotKey;
    if (MediaLibraryAssetOperations::GetStringFromValuesBucket(cmdValues,
        PhotoColumn::CAMERA_SHOT_KEY, cameraShotKey)) {
        values.Put(PhotoColumn::CAMERA_SHOT_KEY, cameraShotKey);
    }
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} "
        "MultistagesCapture, supportedWatermarkType: %{public}d, cameraShotKey: %{public}s",
        MLOG_TAG, __FUNCTION__, __LINE__,
        supportedWatermarkType, cameraShotKey.c_str());
}

static int32_t UpdateIsTempAndDirty(MediaLibraryCommand &cmd, const string &fileId,
    const string &fileType, int32_t &getPicRet, PhotoExtInfo &photoExtInfo)
{
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, fileId);
    ValuesBucket values;
    values.Put(PhotoColumn::PHOTO_IS_TEMP, false);
    UpdateValuesBucketForExt(cmd, values);

    CHECK_AND_RETURN_RET_LOG(MediaLibraryDataManagerUtils::IsNumber(fileId), 0, "MultistagesCapture, get fileId fail");
    getPicRet = MediaLibraryPhotoOperations::GetPicture(std::stoi(fileId.c_str()), photoExtInfo.picture, false,
        photoExtInfo.photoId, photoExtInfo.isHighQualityPicture);
    if (!fileType.empty() && getPicRet == E_OK) {
        auto ret = MediaLibraryPhotoOperations::UpdateExtension(std::stoi(fileId.c_str()), std::stoi(fileType.c_str()),
            photoExtInfo, values);
        CHECK_AND_PRINT_LOG(ret == E_OK, "execute UpdateExtension failed, fileId: %{public}s, ret: %{public}d.",
            fileId.c_str(), ret);
    }

    int32_t updateRows = 0;
    AccurateRefresh::AssetAccurateRefresh assetRefresh(AccurateRefresh::SAVE_CAMERA_PHOTO_BUSSINESS_NAME);
    do {
        if (cmd.GetQuerySetParam(PhotoColumn::PHOTO_DIRTY) == to_string(static_cast<int32_t>(DirtyType::TYPE_NEW))) {
            // Only third-party app save photo, it will bring dirty flag
            // The photo saved by third-party apps, whether of low or high quality, should set dirty to TYPE_NEW
            // Every subtype of photo saved by third-party apps, should set dirty to TYPE_NEW
            // Need to change the quality to high quality before updating
            updateRows = UpdateThirdPartyPhotoDirtyFlag(values, predicates, assetRefresh);
            CHECK_AND_RETURN_RET_LOG(updateRows >= 0, E_ERR,
                "update third party photo temp, dirty flag, watermark type and camera shot key fail.");
            break;
        }
 
        string subTypeStr = cmd.GetQuerySetParam(PhotoColumn::PHOTO_SUBTYPE);
        CHECK_AND_RETURN_RET_LOG(!subTypeStr.empty(), E_ERR, "get subType fail");
        int32_t subType = stoi(subTypeStr);
        if (subType == static_cast<int32_t>(PhotoSubType::BURST)) {
            predicates.EqualTo(PhotoColumn::PHOTO_QUALITY,
                to_string(static_cast<int32_t>(MultiStagesPhotoQuality::FULL)));
            predicates.EqualTo(PhotoColumn::PHOTO_SUBTYPE, to_string(static_cast<int32_t>(PhotoSubType::BURST)));
            values.Put(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_NEW));
            updateRows = assetRefresh.UpdateWithDateTime(values, predicates);
            CHECK_AND_RETURN_RET_LOG(updateRows >= 0, E_ERR,
                "burst photo update temp, dirty flag, watermark type and camera shot key fail.");
            break;
        }
 
        updateRows = assetRefresh.UpdateWithDateTime(values, predicates);
        CHECK_AND_RETURN_RET_LOG(updateRows >= 0, E_ERR, "update temp flag fail.");
        if (subType != static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)) {
            int32_t updateDirtyRows = UpdateDirtyWithoutIsTemp(predicates);
            CHECK_AND_RETURN_RET_LOG(updateDirtyRows >= 0, E_ERR, "update dirty flag fail.");
        }
    } while (0);
 
    assetRefresh.RefreshAlbum(static_cast<NotifyAlbumType>(SYS_ALBUM | USER_ALBUM | SOURCE_ALBUM));
    assetRefresh.Notify();
    return updateRows;
}

int32_t MediaLibraryPhotoOperations::CalSingleEditDataSize(const std::string &fileId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdbStore is nullptr");
        return E_DB_FAIL;
    }
    if (MAP_OPERATION_FLAG) {
        MEDIA_INFO_LOG("map operation flag is true");
        std::string filePath;
        int ret = MediaLibraryPhotoOperations::GetFilePathById(rdbStore, fileId, filePath);
        if (ret != E_OK) {
            MEDIA_WARN_LOG("Skip invalid file ID: %{public}s (error code: %{public}d)",
                fileId.c_str(), ret);
            return ret;
        }
        return MediaLibraryRdbStore::UpdateEditDataSize(rdbStore, fileId, filePath);
    }

    return E_OK;
}

int32_t MediaLibraryPhotoOperations::Get500FileIdsAndPathS(const std::shared_ptr<MediaLibraryRdbStore> rdbStore,
    std::vector<std::string> &fileIds, std::vector<std::string> &filePaths, std::string startFileId, bool &hasMore)
{
    fileIds.clear();
    filePaths.clear();

    MediaLibraryCommand queryCmd(OperationObject::UFM_PHOTO, OperationType::QUERY);
    queryCmd.GetAbsRdbPredicates()
        ->IsNotNull(MediaColumn::MEDIA_ID)
        ->And()
        ->IsNotNull(MediaColumn::MEDIA_FILE_PATH);

    if ((!startFileId.empty()) && MediaLibraryDataManagerUtils::IsNumber(startFileId)) {
        queryCmd.GetAbsRdbPredicates()->Offset(std::stoi(startFileId));
    }
    // 一次查取500个
    queryCmd.GetAbsRdbPredicates()->Limit(500);
        
    std::vector<std::string> columns = {MediaColumn::MEDIA_ID, MediaColumn::MEDIA_FILE_PATH};

    auto result = rdbStore->Query(queryCmd, columns);
    if (!result || result->GoToFirstRow() != NativeRdb::E_OK) {
        hasMore = false;
        MEDIA_ERR_LOG("Query files failed");
        return E_GET_PRAMS_FAIL;
    }

    std::string fileId;
    std::string filePath;
    int count = 0;
    do {
        fileId = GetStringVal(MediaColumn::MEDIA_ID, result);
        filePath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, result);
        if (!fileId.empty() && !filePath.empty()) {
            fileIds.push_back(fileId);
            filePaths.push_back(filePath);
            count++;
        }
    } while (result->GoToNextRow() == NativeRdb::E_OK);

    // 没有取到500个，认为已经取完了所有的数据
    if (count < 500) {
        hasMore = false;
    }

    return E_OK;
}

static int32_t ConvertPhotoPathToEditDataDirPath(const std::string &sourcePath, std::string &editDataDir)
{
    if (sourcePath.empty()) {
        return E_INVALID_PATH;
    }

    editDataDir = MediaLibraryAssetOperations::GetEditDataDirPath(sourcePath);
    if (editDataDir.empty()) {
        MEDIA_ERR_LOG("Failed to convert path: %{public}s", sourcePath.c_str());
        return E_INVALID_PATH;
    }
    return E_OK;
}

static int32_t GetCloudFilePath(const shared_ptr<MediaLibraryRdbStore> rdbStore, const std::string &fileId,
    std::string &filePath)
{
    std::string sql = "SELECT " + MediaColumn::MEDIA_FILE_PATH +
                     " FROM " + PhotoColumn::PHOTOS_TABLE +
                     " WHERE " + MediaColumn::MEDIA_ID + " = ?";
    
    std::vector<NativeRdb::ValueObject> params = {fileId};
    auto result = rdbStore->QuerySql(sql, params);
    if (!result) {
        MEDIA_ERR_LOG("Query failed for fileId: %{public}s", fileId.c_str());
        return E_HAS_DB_ERROR;
    }

    if (result->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_WARN_LOG("File not found for ID: %{public}s", fileId.c_str());
        return E_INVALID_FILEID;
    }

    if (result->GetString(0, filePath) != NativeRdb::E_OK || filePath.empty()) {
        MEDIA_ERR_LOG("Failed to retrieve file path for ID: %{public}s", fileId.c_str());
        return E_INVALID_FILEID;
    }
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::ConvertPhotoCloudPathToLocalData(std::string retrievedPath, std::string &filePath)
{
    static const std::string OLD_PREFIX = "/storage/cloud/";
    static const std::string NEW_PREFIX = "/storage/media/local/";
    if (retrievedPath.compare(0, OLD_PREFIX.length(), OLD_PREFIX) == 0) {
        std::string editDataDir;
        int32_t ret = ConvertPhotoPathToEditDataDirPath(retrievedPath, editDataDir);
        if (ret != E_OK) {
            return ret;
        }
        filePath = NEW_PREFIX + editDataDir.substr(OLD_PREFIX.length());
        MEDIA_INFO_LOG("Converted to local path: %{private}s", filePath.c_str());
    } else {
        filePath = std::move(retrievedPath);
    }
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::GetFilePathById(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const std::string &fileId, std::string &filePath)
{
    std::string retrievedPath;
    if (GetCloudFilePath(rdbStore, fileId, retrievedPath) != E_OK) {
        MEDIA_ERR_LOG("Failed to retrieve file path for ID: %{public}s", fileId.c_str());
        return E_INVALID_FILEID;
    }

    if (ConvertPhotoCloudPathToLocalData(retrievedPath, filePath) != E_OK) {
        MEDIA_ERR_LOG("Failed to Convert cloue path: %{public}s to local path", retrievedPath.c_str());
        return E_INVALID_FILEID;
    }

    return E_OK;
}

void MediaLibraryPhotoOperations::HandleContainsAddResource(const std::string &fileId,
    const std::string containsAddResource)
{
    MEDIA_ERR_LOG("HandleContainsAddResource containsAddResource: %{public}s.", containsAddResource.c_str());
    if (containsAddResource == CONTAIN_ADD_RESOURCE_FALSE) {
        MEDIA_WARN_LOG("no need execute HandleContainsAddResource.");
        return;
    }

    const std::vector<std::string> columns = {
        PhotoColumn::PHOTO_ID,
    };
    std::shared_ptr<FileAsset> fileAsset = GetFileAssetFromDb(
        PhotoColumn::MEDIA_ID, fileId, OperationObject::FILESYSTEM_PHOTO, columns);
    CHECK_AND_RETURN_LOG(fileAsset != nullptr, "MultistagesCapture, get fileAsset fail");

    auto pictureManagerThread = PictureManagerThread::GetInstance();
    if (pictureManagerThread != nullptr) {
        pictureManagerThread->DeleteDataWithImageId(fileAsset->GetPhotoId(), LOW_QUALITY_PICTURE);
        pictureManagerThread->DeleteDataWithImageId(fileAsset->GetPhotoId(), HIGH_QUALITY_PICTURE);
    }
    MultistagesCaptureNotify::NotifyLowQualityMemoryCount();
}

void MediaLibraryPhotoOperations::HandleScanFile(const std::string &path, int32_t burstCoverLevel,
    std::shared_ptr<Media::Picture> &resultPicture, const std::string &fileId)
{
    if (!path.empty()) {
        MEDIA_INFO_LOG("MultistagesCapture, scan file start, fileId: %{public}s", fileId.c_str());
        if (burstCoverLevel == static_cast<int32_t>(BurstCoverLevelType::COVER)) {
            ScanFile(path, false, true, true, stoi(fileId), resultPicture);
        } else {
            resultPicture = nullptr;
            MediaLibraryAssetOperations::ScanFileWithoutAlbumUpdate(
                path, false, true, true, stoi(fileId));
        }
    }
}

int32_t MediaLibraryPhotoOperations::SaveCameraPhoto(MediaLibraryCommand &cmd)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryPhotoOperations::SaveCameraPhoto");
    string fileId = cmd.GetQuerySetParam(PhotoColumn::MEDIA_ID);
    bool cond = fileId.empty() && !MediaLibraryDataManagerUtils::IsNumber(fileId);
    CHECK_AND_RETURN_RET_LOG(!cond, 0, "MultistagesCapture, get fileId fail");

    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} MultistagesCapture, start save fileId: %{public}s",
        MLOG_TAG, __FUNCTION__, __LINE__, fileId.c_str());

    std::string containsAddResource = cmd.GetQuerySetParam(CONTAIN_ADD_RESOURCE);
    if (!containsAddResource.empty()) {
        HandleContainsAddResource(fileId, containsAddResource);
    }
    tracer.Start("MediaLibraryPhotoOperations::UpdateIsTempAndDirty");
    string fileType = cmd.GetQuerySetParam(IMAGE_FILE_TYPE);
    MEDIA_ERR_LOG("MultistagesCapture fileType: %{public}s", fileType.c_str());
    int32_t getPicRet = -1;
    PhotoExtInfo photoExtInfo = {"", MIME_TYPE_JPEG, "", "", nullptr};
    int32_t ret = UpdateIsTempAndDirty(cmd, fileId, fileType, getPicRet, photoExtInfo);
    tracer.Finish();
    CHECK_AND_RETURN_RET_LOG(ret >= 0, 0, "UpdateIsTempAndDirty failed, ret: %{public}d", ret);
    if (photoExtInfo.oldFilePath != "") {
        UpdateEditDataPath(photoExtInfo.oldFilePath, photoExtInfo.extension);
    }
    tracer.Start("MediaLibraryPhotoOperations::SavePicture");
    std::shared_ptr<Media::Picture> resultPicture = nullptr;
    if (!fileType.empty()) {
        SavePicture(stoi(fileType), stoi(fileId), getPicRet, photoExtInfo, resultPicture);
        MEDIA_DEBUG_LOG("MultistagesCapture, save picture end, fileId: %{public}s", fileId.c_str());
    }
    tracer.Finish();
    if (containsAddResource == CONTAIN_ADD_RESOURCE_TRUE) {
        MEDIA_ERR_LOG("MultistagesCapture Success, no need scanfile, fileId: %{public}s, ret: %{public}d",
            fileId.c_str(), ret);
        return ret;
    }

    string needScanStr = cmd.GetQuerySetParam(MEDIA_OPERN_KEYWORD);
    shared_ptr<FileAsset> fileAsset = GetFileAssetFromDb(PhotoColumn::MEDIA_ID, fileId,
        OperationObject::FILESYSTEM_PHOTO, PHOTO_COLUMN_VECTOR);
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, 0, "MultistagesCapture, get fileAsset fail");
    std::string path = fileAsset->GetPath();
    int32_t burstCoverLevel = fileAsset->GetBurstCoverLevel();
    tracer.Start("MediaLibraryPhotoOperations::Scan");
    HandleScanFile(path, burstCoverLevel, resultPicture, fileId);
    tracer.Finish();
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} "
        "MultistagesCapture Success, fileId: %{public}s, ret: %{public}d, needScanStr: %{public}s",
        MLOG_TAG, __FUNCTION__, __LINE__, fileId.c_str(), ret, needScanStr.c_str());
    return ret;
}

int32_t MediaLibraryPhotoOperations::SetVideoEnhancementAttr(MediaLibraryCommand &cmd)
{
    std::string videoId = cmd.GetQuerySetParam(PhotoColumn::PHOTO_ID);
    std::string fileId = cmd.GetQuerySetParam(MediaColumn::MEDIA_ID);
    std::string filePath = cmd.GetQuerySetParam(MediaColumn::MEDIA_FILE_PATH);
    int32_t fileIdNum = static_cast<int32_t>(std::strtoul(fileId.c_str(), nullptr, 10));
    VideoInfo videoInfo = {fileIdNum, VideoCount::SINGLE, filePath, "", ""};
    MultiStagesVideoCaptureManager::GetInstance().AddVideo(videoId, fileId, videoInfo);
    return E_OK;
}

static int32_t GetHiddenState(const ValuesBucket &values)
{
    ValueObject obj;
    auto ret = values.GetObject(MediaColumn::MEDIA_HIDDEN, obj);
    CHECK_AND_RETURN_RET(ret, E_INVALID_VALUES);
    int32_t hiddenState = 0;
    ret = obj.GetInt(hiddenState);
    CHECK_AND_RETURN_RET(ret == E_OK, E_INVALID_VALUES);
    return hiddenState == 0 ? 0 : 1;
}

static void SkipNotifyIfBurstMember(vector<string> &notifyUris)
{
    vector<string> tempUris;
    vector<string> ids;
    unordered_map<string, string> idUriMap;
    for (auto& uri : notifyUris) {
        string fileId = MediaLibraryDataManagerUtils::GetFileIdFromPhotoUri(uri);
        if (fileId.empty()) {
            return;
        }
        ids.push_back(fileId);
        idUriMap.insert({fileId, uri});
    }
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    vector<string> columns = {
        MediaColumn::MEDIA_ID,
    };
    NativeRdb::RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    rdbPredicates.In(MediaColumn::MEDIA_ID, ids);
    rdbPredicates.NotEqualTo(PhotoColumn::PHOTO_BURST_COVER_LEVEL, static_cast<int32_t>(BurstCoverLevelType::MEMBER));
    auto resultSet = uniStore->Query(rdbPredicates, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("IsAssetReadyById failed");
        return;
    }
    unordered_map<string, string>::iterator iter;
    while (resultSet->GoToNextRow() == E_OK) {
        int32_t fileId = get<int32_t>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_ID,
            resultSet, TYPE_INT32));
        if ((iter = idUriMap.find(to_string(fileId))) != idUriMap.end()) {
            tempUris.push_back(iter->second);
        }
    }
    resultSet->Close();
    notifyUris.swap(tempUris);
}

static void SendHideNotify(vector<string> &notifyUris, const int32_t hiddenState)
{
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_LOG(watch != nullptr, "notify watch is nullptr");
    int hiddenAlbumId = watch->GetAlbumIdBySubType(PhotoAlbumSubType::HIDDEN);
    CHECK_AND_RETURN_LOG(hiddenAlbumId > 0, "hiddenAlbumId is invalid");

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
    SkipNotifyIfBurstMember(notifyUris);
    for (auto &notifyUri : notifyUris) {
        watch->Notify(notifyUri, assetNotifyType);
        watch->Notify(notifyUri, albumNotifyType, 0);
        watch->Notify(notifyUri, hiddenAlbumNotifyType, hiddenAlbumId);
        if (!hiddenState) {
            watch->Notify(notifyUri, NotifyType::NOTIFY_THUMB_ADD);
        }
    }
    if (MAP_OPERATION_FLAG) {
        MEDIA_INFO_LOG("map operation flag is true");
        MediaLibraryFormMapOperations::GetFormIdsByUris(notifyUris, formIds);
        if (!formIds.empty()) {
            MediaLibraryFormMapOperations::PublishedChange("", formIds, false);
        }
    }
}

static int32_t HidePhotos(MediaLibraryCommand &cmd)
{
    AccurateRefresh::AssetAccurateRefresh assetRefresh(AccurateRefresh::HIDE_PHOTOS_BUSSINESS_NAME);
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryPhotoOperations::HidePhotos");

    int32_t hiddenState = GetHiddenState(cmd.GetValueBucket());
    CHECK_AND_RETURN_RET(hiddenState >= 0, hiddenState);

    RdbPredicates predicates = RdbUtils::ToPredicates(cmd.GetDataSharePred(), PhotoColumn::PHOTOS_TABLE);

    vector<string> notifyUris = predicates.GetWhereArgs();
    MEDIA_INFO_LOG("Hide %{public}zu Photos, hiddenState: %{public}d", notifyUris.size(), hiddenState);
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
    int32_t changedRows = assetRefresh.UpdateWithDateTime(values, predicates);
    CHECK_AND_RETURN_RET(changedRows >= 0, changedRows);
    if (hiddenState == 0) {
        int32_t ret = LakeFileOperations::MoveAssetsToLake(assetRefresh, predicates.GetWhereArgs());
        CHECK_AND_PRINT_LOG(ret == E_OK, "recover inner anco file error when cancel hide asset");
    } else {
        int32_t ret = LakeFileOperations::MoveAssetsFromLake(predicates.GetWhereArgs());
        CHECK_AND_PRINT_LOG(ret == E_OK, "hide photo inner anco file error");
    }
    MediaAnalysisHelper::StartMediaAnalysisServiceAsync(
        static_cast<int32_t>(MediaAnalysisProxy::ActivateServiceType::START_UPDATE_INDEX), notifyUris);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "Failed to get rdbStore");
    MediaLibraryRdbUtils::UpdateAnalysisAlbumByUri(rdbStore, notifyUris);
    assetRefresh.RefreshAlbum(NotifyAlbumType::SYS_ALBUM);
    SendHideNotify(notifyUris, hiddenState);
    assetRefresh.Notify();
    return changedRows;
}

static int32_t GetFavoriteState(const ValuesBucket& values)
{
    ValueObject obj;
    bool isValid = values.GetObject(PhotoColumn::MEDIA_IS_FAV, obj);
    CHECK_AND_RETURN_RET(isValid, E_INVALID_VALUES);

    int32_t favoriteState = -1;
    int ret = obj.GetInt(favoriteState);
    bool cond = (ret != E_OK || (favoriteState != 0 && favoriteState != 1));
    CHECK_AND_RETURN_RET(!cond, E_INVALID_VALUES);
    return favoriteState;
}

static bool CheckExistsHiddenPhoto(std::shared_ptr<MediaLibraryRdbStore> rdbStore, RdbPredicates predicates)
{
    string where = predicates.GetWhereClause() + " AND " + MediaColumn::MEDIA_HIDDEN + " > 0";
    NativeRdb::AbsRdbPredicates predicatesNew(predicates.GetTableName());
    predicatesNew.SetWhereClause(where);
    predicatesNew.SetWhereArgs(predicates.GetWhereArgs());
    vector<string> columns { MediaColumn::MEDIA_ID };
    shared_ptr<NativeRdb::ResultSet> resultSet = MediaLibraryRdbStore::QueryWithFilter(predicatesNew, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Result set is empty");
        return false;
    }
    int32_t rowCount = 0;
    int32_t ret = resultSet->GetRowCount(rowCount);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("result set get row count err %{public}d", ret);
        return false;
    }
    return rowCount > 0;
}

static int32_t BatchSetFavorite(MediaLibraryCommand& cmd)
{
    AccurateRefresh::AssetAccurateRefresh assetRefresh(AccurateRefresh::SET_ASSETS_FAVORITE_BUSSINESS_NAME);
    int32_t favoriteState = GetFavoriteState(cmd.GetValueBucket());
    CHECK_AND_RETURN_RET_LOG(favoriteState >= 0, favoriteState, "Failed to get favoriteState");

    RdbPredicates predicates = RdbUtils::ToPredicates(cmd.GetDataSharePred(), PhotoColumn::PHOTOS_TABLE);
    vector<string> notifyUris = predicates.GetWhereArgs();
    MediaLibraryRdbStore::ReplacePredicatesUriToId(predicates);
    ValuesBucket values;
    values.Put(PhotoColumn::MEDIA_IS_FAV, favoriteState);
    int32_t updatedRows = assetRefresh.UpdateWithDateTime(values, predicates);
    CHECK_AND_RETURN_RET_LOG(updatedRows >= 0, E_HAS_DB_ERROR, "Failed to set favorite, err: %{public}d", updatedRows);
    assetRefresh.RefreshAlbum();
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_RET_LOG(watch != nullptr, E_ERR, "Can not get MediaLibraryNotify Instance");
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

    assetRefresh.Notify();
    CHECK_AND_WARN_LOG(static_cast<size_t>(updatedRows) == notifyUris.size(),
        "Try to notify %{public}zu items, but only %{public}d items updated.", notifyUris.size(), updatedRows);
    return updatedRows;
}

int32_t MediaLibraryPhotoOperations::BatchSetUserComment(MediaLibraryCommand& cmd)
{
    AccurateRefresh::AssetAccurateRefresh assetRefresh(AccurateRefresh::SET_ASSETS_USER_COMMENT_BUSSINESS_NAME);
    vector<shared_ptr<FileAsset>> fileAssetVector;
    vector<string> columns = { PhotoColumn::MEDIA_ID, PhotoColumn::MEDIA_FILE_PATH,
        PhotoColumn::MEDIA_TYPE, PhotoColumn::MEDIA_NAME };
    MediaLibraryRdbStore::ReplacePredicatesUriToId(*(cmd.GetAbsRdbPredicates()));

    int32_t errCode = GetFileAssetVectorFromDb(*(cmd.GetAbsRdbPredicates()),
        OperationObject::FILESYSTEM_PHOTO, fileAssetVector, columns);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode,
        "Failed to query file asset vector from db, errCode=%{private}d", errCode);

    int32_t updateRows = UpdateFileInDb(cmd);
    CHECK_AND_RETURN_RET_LOG(updateRows >= 0, updateRows,
        "Update Photo in database failed, updateRows=%{public}d", updateRows);

    for (const auto& fileAsset : fileAssetVector) {
        errCode = SetUserComment(cmd, fileAsset);
        CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Failed to set user comment, errCode=%{private}d", errCode);
    }

    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_RET_LOG(watch != nullptr, E_ERR, "Can not get MediaLibraryNotify Instance");
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
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_VALUES, "fileAsset is null");
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
    CHECK_AND_RETURN_RET(values.GetObject(PhotoColumn::PHOTO_ORIENTATION, valueObject), E_OK);
    int32_t cmdOrientation;
    valueObject.GetInt(cmdOrientation);

    auto imageSourceOrientation = ORIENTATION_MAP.find(cmdOrientation);
    CHECK_AND_RETURN_RET_LOG(imageSourceOrientation != ORIENTATION_MAP.end(), E_INVALID_VALUES,
        "imageSourceOrientation value is invalid.");

    string exifStr = fileAsset->GetAllExif();
    if (!exifStr.empty() && nlohmann::json::accept(exifStr)) {
        nlohmann::json exifJson = nlohmann::json::parse(exifStr, nullptr, false);
        CHECK_AND_RETURN_RET_LOG(!exifJson.is_discarded(), E_INVALID_VALUES, "JSON parse error");
        exifJson["Orientation"] = imageSourceOrientation->second;
        exifStr = exifJson.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
        CHECK_AND_RETURN_RET_LOG(!exifStr.empty(), E_INVALID_VALUES,
            "JSON dump error, failed to convert JSON to string");
        values.PutString(PhotoColumn::PHOTO_ALL_EXIF, exifStr);
    }

    err = imageSource->ModifyImageProperty(0, PHOTO_DATA_IMAGE_ORIENTATION,
        std::to_string(imageSourceOrientation->second), fileAsset->GetFilePath());
    CHECK_AND_RETURN_RET_LOG(err == E_OK, E_INVALID_VALUES,
        "Modify image property all exif failed, err = %{public}d", err);
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::HandleLakeAssetRename(AccurateRefresh::AccurateRefreshBase &refresh,
    MediaLibraryCommand &cmd, const std::shared_ptr<FileAsset> &fileAsset)
{
    ValueObject valueDisplayName;
    string newDisplayName;
    if (cmd.GetValueBucket().GetObject(MediaColumn::MEDIA_NAME, valueDisplayName)) {
        valueDisplayName.GetString(newDisplayName);
    }
    if (!newDisplayName.empty() && newDisplayName != fileAsset->GetDisplayName() &&
        !fileAsset->GetStoragePath().empty()) {
        return LakeFileOperations::RenamePhoto(
            refresh, fileAsset->GetId(), newDisplayName, fileAsset->GetStoragePath(), fileAsset->GetFilePath());
    }
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::UpdateOrientationExif(MediaLibraryCommand &cmd,
    const shared_ptr<FileAsset> &fileAsset, bool &orientationUpdated, string &currentOrientation)
{
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_VALUES, "fileAsset is null");
    ValuesBucket &values = cmd.GetValueBucket();
    ValueObject valueObject;
    if (!values.GetObject(PhotoColumn::PHOTO_ORIENTATION, valueObject)) {
        MEDIA_INFO_LOG("No need update orientation.");
        return E_OK;
    }
    bool cond = ((fileAsset->GetMediaType() != MEDIA_TYPE_IMAGE) ||
        (fileAsset->GetPhotoSubType() == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)));
    CHECK_AND_RETURN_RET_LOG(!cond,
        E_INVALID_VALUES, "Only images support rotation.");
    int32_t errCode = UpdateOrientationAllExif(cmd, fileAsset, currentOrientation);
    if (errCode == E_OK) {
        orientationUpdated = true;
    }
    return errCode;
}

int32_t IsSystemAlbumMovement(MediaLibraryCommand &cmd, bool &isSystemAlbum)
{
    static vector<int32_t> systemAlbumIds;
    if (systemAlbumIds.empty()) {
        auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
        CHECK_AND_RETURN_RET_LOG(uniStore != nullptr, E_HAS_DB_ERROR, "rdbstore is nullptr");
        vector<string> columns = {PhotoAlbumColumns::ALBUM_ID};
        NativeRdb::RdbPredicates rdbPredicates(PhotoAlbumColumns::TABLE);
        rdbPredicates.EqualTo(PhotoAlbumColumns::ALBUM_TYPE, static_cast<int32_t>(PhotoAlbumType::SYSTEM));
        auto resultSet = uniStore->Query(rdbPredicates, columns);
        CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "Failed to query system albums");

        while (resultSet->GoToNextRow() == E_OK) {
            int32_t albumId =
                get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_ID, resultSet, TYPE_INT32));
            systemAlbumIds.push_back(albumId);
        }
        resultSet->Close();
    }

    auto whereClause = cmd.GetAbsRdbPredicates()->GetWhereClause();
    auto whereArgs = cmd.GetAbsRdbPredicates()->GetWhereArgs();
    int32_t oriAlbumId = MediaLibraryAssetOperations::GetAlbumIdByPredicates(whereClause, whereArgs);
    isSystemAlbum = std::find(systemAlbumIds.begin(), systemAlbumIds.end(), oriAlbumId) != systemAlbumIds.end();
    return E_OK;
}

void GetSystemMoveAssets(AbsRdbPredicates &predicates)
{
    const vector<string> &whereUriArgs = predicates.GetWhereArgs();
    CHECK_AND_RETURN_LOG(whereUriArgs.size() > 1, "Move vector empty when move from system album");
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
    CHECK_AND_RETURN_RET_LOG(resultSetQuery != nullptr, E_INVALID_ARGUMENTS, "album id is not exist");
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
    CHECK_AND_RETURN_RET(PrepareUpdateArgs(cmd, ownerAlbumIds, assetString, predicates) == E_OK, E_INVALID_ARGUMENTS);

    ValueObject value;
    int32_t targetAlbumId = 0;
    CHECK_AND_RETURN_RET_LOG(cmd.GetValueBucket().GetObject(PhotoColumn::PHOTO_OWNER_ALBUM_ID, value),
        E_INVALID_ARGUMENTS, "get owner album id fail when move from system album");
    value.GetInt(targetAlbumId);
    ValuesBucket values;
    values.Put(PhotoColumn::PHOTO_OWNER_ALBUM_ID, to_string(targetAlbumId));

    AccurateRefresh::AssetAccurateRefresh assetRefresh(AccurateRefresh::UPDATE_SYSTEM_ASSET_BUSSINESS_NAME);
    int32_t changedRows = assetRefresh.UpdateWithDateTime(values, predicates);
    CHECK_AND_RETURN_RET_LOG(changedRows >= 0, changedRows, "Update owner album id fail when move from system album");
    int32_t ret = LakeFileOperations::MoveInnerLakeAssetsToNewAlbum(
        assetRefresh, cmd.GetAbsRdbPredicates()->GetWhereArgs(), targetAlbumId);
    CHECK_AND_PRINT_LOG(ret == E_OK, "move inner anco asset to system album error");
    CHECK_AND_EXECUTE(assetString.empty(), MediaAnalysisHelper::AsyncStartMediaAnalysisService(
        static_cast<int32_t>(MediaAnalysisProxy::ActivateServiceType::START_UPDATE_INDEX), assetString));
    
    assetRefresh.RefreshAlbum();
    for (auto it = ownerAlbumIds.begin(); it != ownerAlbumIds.end(); it++) {
        MEDIA_INFO_LOG("System album move assets target album id is: %{public}s", to_string(it->first).c_str());
        int32_t oriAlbumId = it->first;
        auto watch = MediaLibraryNotify::GetInstance();
        CHECK_AND_CONTINUE_ERR_LOG(watch != nullptr, "Can not get MediaLibraryNotify Instance");
        for (const auto &id : it->second) {
            watch->Notify(PhotoColumn::PHOTO_URI_PREFIX + to_string(id),
                NotifyType::NOTIFY_ALBUM_REMOVE_ASSET, oriAlbumId);
            watch->Notify(PhotoColumn::PHOTO_URI_PREFIX + to_string(id),
                NotifyType::NOTIFY_ALBUM_ADD_ASSET, targetAlbumId);
        }
    }
    assetRefresh.Notify();
    return changedRows;
}

int32_t MediaLibraryPhotoOperations::BatchSetOwnerAlbumId(MediaLibraryCommand &cmd)
{
    vector<shared_ptr<FileAsset>> fileAssetVector;
    vector<string> columns = { PhotoColumn::MEDIA_ID, PhotoColumn::MEDIA_FILE_PATH,
        PhotoColumn::MEDIA_TYPE, PhotoColumn::MEDIA_NAME };
    MediaLibraryRdbStore::ReplacePredicatesUriToId(*(cmd.GetAbsRdbPredicates()));

    // Check if move from system album
    bool isSystemAlbum = false;
    int32_t errCode = IsSystemAlbumMovement(cmd, isSystemAlbum);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode,
        "Failed to check whether move from system album, errCode=%{public}d", errCode);
    if (isSystemAlbum) {
        MEDIA_INFO_LOG("Move assets from system album");
        GetSystemMoveAssets(*(cmd.GetAbsRdbPredicates()));
        int32_t updateSysRows = UpdateSystemRows(cmd);
        return updateSysRows;
    }

    errCode = GetFileAssetVectorFromDb(*(cmd.GetAbsRdbPredicates()),
        OperationObject::FILESYSTEM_PHOTO, fileAssetVector, columns);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode,
        "Failed to query file asset vector from db, errCode=%{public}d, predicates=%{public}s",
        errCode, cmd.GetAbsRdbPredicates()->ToString().c_str());

    AccurateRefresh::AssetAccurateRefresh assetRefresh(AccurateRefresh::MOVE_ASSETS_BUSSINESS_NAME);
    int32_t updateRows = -1;
    assetRefresh.Update(cmd, updateRows);
    CHECK_AND_RETURN_RET_LOG(updateRows >= 0, updateRows,
        "Update Photo in database failed, updateRows=%{public}d", updateRows);
    int32_t targetAlbumId = 0;
    int32_t oriAlbumId = 0;
    vector<string> idsToUpdateIndex;
    UpdateOwnerAlbumIdOnMove(cmd, targetAlbumId, oriAlbumId);
    assetRefresh.RefreshAlbum();
    vector<string> idsToMove;
    for (auto &fileAsset : fileAssetVector) {
        idsToMove.push_back(to_string(fileAsset->GetId()));
    }
    if (!idsToMove.empty()) {
        int32_t ret = LakeFileOperations::MoveInnerLakeAssetsToNewAlbum(assetRefresh, idsToMove, targetAlbumId);
        CHECK_AND_PRINT_LOG(ret == E_OK, "move inner anco asset to new album error");
    }
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
    assetRefresh.Notify();
    if (!idsToUpdateIndex.empty()) {
        MediaAnalysisHelper::AsyncStartMediaAnalysisService(
            static_cast<int32_t>(MediaAnalysisProxy::ActivateServiceType::START_UPDATE_INDEX), idsToUpdateIndex);
    }
    return updateRows;
}

static int32_t GetRecentShowState(const ValuesBucket& values)
{
    ValueObject obj;
    bool isValid = values.GetObject(PhotoColumn::PHOTO_IS_RECENT_SHOW, obj);
    CHECK_AND_RETURN_RET(isValid, E_INVALID_VALUES);

    int32_t recentShowState = -1;
    int ret = obj.GetInt(recentShowState);
    bool cond = (ret != E_OK || (recentShowState != 0 && recentShowState != 1));
    CHECK_AND_RETURN_RET(!cond, E_INVALID_VALUES);
    return recentShowState;
}

int32_t MediaLibraryPhotoOperations::BatchSetRecentShow(MediaLibraryCommand &cmd)
{
    int32_t recentShowState = GetRecentShowState(cmd.GetValueBucket());
    CHECK_AND_RETURN_RET_LOG(recentShowState >= 0, recentShowState, "Failed to get recentShowState");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "Failed to get rdbStore");

    RdbPredicates predicates = RdbUtils::ToPredicates(cmd.GetDataSharePred(), PhotoColumn::PHOTOS_TABLE);
    vector<string> notifyUris = predicates.GetWhereArgs();
    MediaLibraryRdbStore::ReplacePredicatesUriToId(predicates);
    ValuesBucket values;
    values.Put(PhotoColumn::PHOTO_IS_RECENT_SHOW, recentShowState);
    int32_t updatedRows = rdbStore->UpdateWithDateTime(values, predicates);
    CHECK_AND_RETURN_RET_LOG(updatedRows >= 0, E_HAS_DB_ERROR, "Failed to set recentShow, err: %{public}d",
        updatedRows);
    return updatedRows;
}

static void RevertOrientation(const shared_ptr<FileAsset> &fileAsset, string &currentOrientation)
{
    CHECK_AND_RETURN_LOG(fileAsset != nullptr, "fileAsset is null");
    std::unique_ptr<ImageSource> imageSource;
    int32_t err = CreateImageSource(fileAsset, imageSource);
    CHECK_AND_RETURN_LOG(err == E_OK, "Failed to obtain image exif information, err = %{public}d", err);
    uint32_t ret = imageSource->ModifyImageProperty(
        0,
        PHOTO_DATA_IMAGE_ORIENTATION,
        currentOrientation.empty() ? ANALYSIS_HAS_DATA : currentOrientation,
        fileAsset->GetFilePath()
    );
    CHECK_AND_RETURN_LOG(ret == E_OK, "Rollback of exlf information failed, err = %{public}d", ret);
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
    CHECK_AND_RETURN_LOG(fileAsset != nullptr, "fileAsset is null");
    if (orientationUpdated) {
        auto watch = MediaLibraryNotify::GetInstance();
        CHECK_AND_RETURN_LOG(watch != nullptr, "Can not get MediaLibraryNotify Instance");
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

static int32_t UpdateAlbumDateModified(int32_t albumId)
{
    CHECK_AND_RETURN_RET_LOG(albumId > 0, E_ERR, "Update date modified albumId err. albumId=%{public}d.", albumId);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET(rdbStore != nullptr, E_HAS_DB_ERROR);
    MediaLibraryCommand updateCmd(OperationObject::PHOTO_ALBUM, OperationType::UPDATE);
    updateCmd.GetAbsRdbPredicates()->EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(albumId));
    ValuesBucket updateValues;
    updateValues.PutLong(PhotoAlbumColumns::ALBUM_DATE_MODIFIED, MediaFileUtils::UTCTimeMilliSeconds());
    updateCmd.SetValueBucket(updateValues);
    int32_t rowId = 0;
    int32_t result = rdbStore->Update(updateCmd, rowId);
    CHECK_AND_RETURN_RET_LOG(result == NativeRdb::E_OK && rowId > 0, E_HAS_DB_ERROR,
        "Update date modified failed. Result %{public}d.", result);
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::UpdateOrientation(MediaLibraryCommand &cmd,
    const shared_ptr<FileAsset>& fileAsset, bool &orientationUpdated,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &assetRefresh, int32_t &errCode)
{
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_VALUES, "fileAsset is null");
    string currentOrientation = "";
    errCode = UpdateOrientationExif(cmd, fileAsset, orientationUpdated, currentOrientation);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Update allexif failed, allexif=%{private}s",
        fileAsset->GetAllExif().c_str());

    int32_t rowId = -1;
    CHECK_AND_RETURN_RET_LOG(assetRefresh != nullptr, E_INVALID_VALUES, "assetRefresh is null");
    assetRefresh->Update(cmd, rowId);
    if (rowId < 0) {
        MEDIA_ERR_LOG("Update Photo In database failed, rowId=%{public}d", rowId);
        RevertOrientation(fileAsset, currentOrientation);
        return rowId;
    }
    if (orientationUpdated) {
        ValuesBucket &values = cmd.GetValueBucket();
        ValueObject valueObject;
        CHECK_AND_RETURN_RET(values.GetObject(PhotoColumn::PHOTO_ORIENTATION, valueObject), rowId);
        int32_t cmdOrientation;
        valueObject.GetInt(cmdOrientation);
        auto transCodeOrientation = ORIENTATION_MAP.find(cmdOrientation);
        CHECK_AND_RETURN_RET_LOG(transCodeOrientation != ORIENTATION_MAP.end(), rowId,
            "transCodeOrientation value is invalid.");
        TransCodeExifInfo transCodeExifInfo;
        transCodeExifInfo.orientation = std::to_string(transCodeOrientation->second);
        MediaLibraryTranscodeDataAgingOperation::ModifyTransCodeFileExif(ExifType::EXIF_ORIENTATION,
            fileAsset->GetFilePath(), transCodeExifInfo, __func__);
        AccurateRefresh::AlbumAccurateRefresh albumRefresh;
        albumRefresh.IsCoverContentChange(to_string(fileAsset->GetId()));
    }
    return rowId;
}

static void HandleAssetMoveFromLake(int32_t mediaId)
{
    MEDIA_INFO_LOG("HandleAssetMoveFromLake media: %{public}d", mediaId);
    int32_t ret = LakeFileOperations::MoveAssetsFromLake({ to_string(mediaId) });
    CHECK_AND_PRINT_LOG(ret == E_OK, "HandleAssetMoveFromLake");
}

int32_t MediaLibraryPhotoOperations::UpdateFileAsset(MediaLibraryCommand &cmd)
{
    shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh =
        make_shared<AccurateRefresh::AssetAccurateRefresh>(AccurateRefresh::UPDATE_FILE_ASSTE_BUSSINESS_NAME);
    vector<string> columns = { PhotoColumn::MEDIA_ID, PhotoColumn::MEDIA_FILE_PATH, PhotoColumn::MEDIA_TYPE,
        PhotoColumn::MEDIA_NAME, PhotoColumn::PHOTO_SUBTYPE, PhotoColumn::PHOTO_EDIT_TIME, MediaColumn::MEDIA_HIDDEN,
        PhotoColumn::MOVING_PHOTO_EFFECT_MODE, PhotoColumn::PHOTO_ORIENTATION, PhotoColumn::PHOTO_ALL_EXIF,
        PhotoColumn::PHOTO_OWNER_ALBUM_ID, PhotoColumn::PHOTO_EXIST_COMPATIBLE_DUPLICATE,
        PhotoColumn::PHOTO_STORAGE_PATH, PhotoColumn::PHOTO_FILE_SOURCE_TYPE };
    shared_ptr<FileAsset> fileAsset = GetFileAssetFromDb(*(cmd.GetAbsRdbPredicates()),
        OperationObject::FILESYSTEM_PHOTO, columns);
    CHECK_AND_RETURN_RET(fileAsset != nullptr, E_INVALID_VALUES);

    bool isNeedScan = false;
    int32_t errCode = RevertToOriginalEffectMode(cmd, fileAsset, isNeedScan);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Failed to revert original effect mode: %{public}d", errCode);
    // Update if FileAsset.title or FileAsset.displayName is modified
    bool isNameChanged = false;
    errCode = UpdateFileName(cmd, fileAsset, isNameChanged);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Update Photo Name failed, fileName=%{private}s",
        fileAsset->GetDisplayName().c_str());

    bool orientationUpdated = false;
    int32_t rowId = UpdateOrientation(cmd, fileAsset, orientationUpdated, assetRefresh, errCode);
    if (rowId < 0) {
        return rowId;
    }
    int64_t dateTrashed = 0;
    ValueObject dateTrashedValObj;
    bool hasDateTrashed = cmd.GetValueBucket().GetObject(PhotoColumn::MEDIA_DATE_TRASHED, dateTrashedValObj);
    dateTrashedValObj.GetLong(dateTrashed);
    AccurateRefresh::AccurateRefreshBase &baseRefresh = *assetRefresh;
    if (isNameChanged && !fileAsset->GetStoragePath().empty()) {
        HandleLakeAssetRename(baseRefresh, cmd, fileAsset);
    } else if (hasDateTrashed && dateTrashed != 0) {
        HandleAssetMoveFromLake(fileAsset->GetId());
    }
    CHECK_AND_EXECUTE(!isNameChanged, UpdateAlbumDateModified(fileAsset->GetOwnerAlbumId()));
    if (cmd.GetOprnType() == OperationType::SET_USER_COMMENT) {
        errCode = SetUserComment(cmd, fileAsset);
        CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Edit user comment errCode = %{private}d", errCode);
    }
    HandleUpdateIndex(cmd, to_string(fileAsset->GetId()));
    string extraUri = MediaFileUtils::GetExtraUri(fileAsset->GetDisplayName(), fileAsset->GetPath());
    errCode = SendTrashNotify(cmd, fileAsset->GetId(), extraUri, assetRefresh);
    CHECK_AND_RETURN_RET(errCode != E_OK, rowId);
    SendFavoriteNotify(cmd, fileAsset, extraUri, assetRefresh);
    SendModifyUserCommentNotify(cmd, fileAsset->GetId(), extraUri);

    CreateThumbnailFileScan(fileAsset, extraUri, orientationUpdated, isNeedScan);
    assetRefresh->Notify();
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_RET_LOG(watch != nullptr, E_ERR, "Can not get MediaLibraryNotify Instance");
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
        case OperationType::BATCH_UPDATE_RECENT_SHOW:
            return BatchSetRecentShow(cmd);
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
        case OperationType::UPDATE_SUPPORTED_WATERMARK_TYPE:
            return UpdateSupportedWatermarkType(cmd);
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
    CHECK_AND_RETURN_RET(fileAsset != nullptr, E_INVALID_VALUES);

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
    CHECK_AND_RETURN_RET(errCode != E_OK, rowId);
    SendFavoriteNotify(cmd, fileAsset);
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_RET_LOG(watch != nullptr, E_ERR, "Can not get MediaLibraryNotify Instance");
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
    MediaColumn::MEDIA_ID,
    MediaColumn::MEDIA_FILE_PATH,
    MediaColumn::MEDIA_NAME,
    MediaColumn::MEDIA_TYPE,
    PhotoColumn::PHOTO_EDIT_TIME,
    PhotoColumn::MEDIA_TIME_PENDING,
    PhotoColumn::MEDIA_DATE_TRASHED,
    PhotoColumn::PHOTO_SUBTYPE,
    PhotoColumn::MOVING_PHOTO_EFFECT_MODE,
    PhotoColumn::PHOTO_ORIGINAL_SUBTYPE,
    PhotoColumn::PHOTO_OWNER_ALBUM_ID,
    PhotoColumn::PHOTO_EXIST_COMPATIBLE_DUPLICATE,
    PhotoColumn::PHOTO_STORAGE_PATH,
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
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsFileExists(dataPath), E_HAS_FS_ERROR,
            "File %{public}s has edit at %{public}ld, but cannot get editdata", uriString.c_str(),
            (long)fileAsset->GetPhotoEditTime());
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
    bool isMovingPhotoMetadataRequest =
        cmd.GetQuerySetParam(MEDIA_MOVING_PHOTO_OPRN_KEYWORD) == OPEN_PRIVATE_MOVING_PHOTO_METADATA;
    CHECK_AND_RETURN_RET_LOG((!isMovingPhotoVideoRequest && !isMovingPhotoMetadataRequest) ||
        CheckOpenMovingPhoto(fileAsset->GetPhotoSubType(), fileAsset->GetMovingPhotoEffectMode(),
            cmd.GetQuerySetParam(MEDIA_OPERN_KEYWORD)),
        E_INVALID_VALUES,
        "Non-moving photo requesting moving photo operation, file id: %{public}s, actual subtype: %{public}d",
        id.c_str(), fileAsset->GetPhotoSubType());
    if (isMovingPhotoVideoRequest) {
        movingPhotoVideoPath = MediaFileUtils::GetMovingPhotoVideoPath(path);
    }

    if (isMovingPhotoMetadataRequest) {
        return OpenFileWithPrivacy(
            MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(path), MEDIA_FILEMODE_READONLY, id);
    }

    string sourcePath = isMovingPhotoVideoRequest ?
        MediaFileUtils::GetMovingPhotoVideoPath(GetEditDataSourcePath(path)) :
        GetEditDataSourcePath(path);
    if (sourcePath.empty() || !MediaFileUtils::IsFileExists(sourcePath)) {
        MEDIA_INFO_LOG("sourcePath does not exist: %{private}s", sourcePath.c_str());
        return OpenFileWithPrivacy(isMovingPhotoVideoRequest ? movingPhotoVideoPath : path, "r", id);
    }
    return OpenFileWithPrivacy(sourcePath, "r", id);
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
    CHECK_AND_RETURN_RET(PhotoEditingRecord::GetInstance()->StartCommitEdit(fileId), E_IS_IN_REVERT);
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
    CHECK_AND_RETURN_RET_LOG(!path.empty(), E_INVALID_URI, "Can not get file path");
    if (fileAsset->GetPhotoEditTime() == 0) {
        string sourceDirPath = GetEditDataDirPath(path);
        CHECK_AND_RETURN_RET_LOG(!sourceDirPath.empty(), E_INVALID_URI, "Can not get edit dir path");
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateDirectory(sourceDirPath), E_HAS_FS_ERROR,
            "Can not create dir %{private}s", sourceDirPath.c_str());
        string sourcePath = GetEditDataSourcePath(path);
        CHECK_AND_RETURN_RET_LOG(!sourcePath.empty(), E_INVALID_URI, "Can not get edit source path");
        if (!MediaFileUtils::IsFileExists(sourcePath)) {
            string realPath = LakeFileUtils::GetAssetRealPath(path);
            if (realPath.find("HO_DATA_EXT_MISC") != std::string::npos) {
                CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CopyFileUtil(realPath, sourcePath), E_HAS_FS_ERROR,
                    "Move file failed, srcPath:%{private}s, newPath:%{private}s", realPath.c_str(), sourcePath.c_str());
            } else {
                CHECK_AND_RETURN_RET_LOG(MediaFileUtils::ModifyAsset(path, sourcePath) == E_SUCCESS, E_HAS_FS_ERROR,
                    "Move file failed, srcPath:%{private}s, newPath:%{private}s", path.c_str(), sourcePath.c_str());
                CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateFile(path), E_HAS_FS_ERROR,
                    "Create file failed, path:%{private}s", path.c_str());
            }
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
    CHECK_AND_RETURN_RET(rdbStore != nullptr, E_HAS_DB_ERROR);
    MediaLibraryCommand updatePendingCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    updatePendingCmd.GetAbsRdbPredicates()->EqualTo(MediaColumn::MEDIA_ID, to_string(fileId));
    ValuesBucket updateValues;
    updateValues.PutLong(PhotoColumn::PHOTO_EDIT_TIME, time);
    updatePendingCmd.SetValueBucket(updateValues);
    int32_t rowId = 0;
    int32_t result = rdbStore->Update(updatePendingCmd, rowId);
    CHECK_AND_RETURN_RET_LOG(result == NativeRdb::E_OK && rowId > 0, E_HAS_DB_ERROR,
        "Update File pending failed. Result %{public}d.", result);
    return E_OK;
}

static int32_t RevertMetadata(int32_t fileId, int64_t time, int32_t effectMode, int32_t photoSubType)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET(rdbStore != nullptr, E_HAS_DB_ERROR);
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    cmd.GetAbsRdbPredicates()->EqualTo(MediaColumn::MEDIA_ID, to_string(fileId));
    ValuesBucket updateValues;
    updateValues.PutLong(PhotoColumn::PHOTO_EDIT_TIME, time);
    if (effectMode == static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY)) {
        updateValues.PutInt(PhotoColumn::MOVING_PHOTO_EFFECT_MODE,
            static_cast<int32_t>(MovingPhotoEffectMode::DEFAULT));
        updateValues.PutInt(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    } else if (effectMode != static_cast<int32_t>(MovingPhotoEffectMode::DEFAULT)) {
        updateValues.PutInt(PhotoColumn::MOVING_PHOTO_EFFECT_MODE,
            static_cast<int32_t>(MovingPhotoEffectMode::DEFAULT));
    }
    cmd.SetValueBucket(updateValues);
    int32_t rowId = 0;
    int32_t result = rdbStore->Update(cmd, rowId);
    CHECK_AND_RETURN_RET_LOG(result == NativeRdb::E_OK && rowId > 0, E_HAS_DB_ERROR,
        "Failed to revert metadata. Result %{public}d.", result);
    return E_OK;
}

static int32_t UpdateEffectMode(int32_t fileId, int32_t effectMode)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "Failed to get rdbStore when updating effect mode");

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
    CHECK_AND_RETURN_RET_LOG(errCode == NativeRdb::E_OK && updateRows >= 0, E_HAS_DB_ERROR,
        "Update effect mode failed. errCode:%{public}d, updateRows:%{public}d.", errCode, updateRows);
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
    CHECK_AND_RETURN_LOG(pos != string::npos, "Failed to parse the path");
    modifyFilePath = path.substr(0, pos) + extension;
}

static int32_t Move(const string& srcPath, const string& destPath)
{
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsFileExists(srcPath), E_NO_SUCH_FILE,
        "srcPath: %{private}s does not exist!", srcPath.c_str());

    CHECK_AND_RETURN_RET_LOG(!destPath.empty(), E_INVALID_VALUES, "Failed to check empty destPath");

    int32_t ret = rename(srcPath.c_str(), destPath.c_str());
    if (ret < 0) {
        MEDIA_ERR_LOG("Failed to rename, src: %{public}s, dest: %{public}s, ret: %{public}d, errno: %{public}d",
            srcPath.c_str(), destPath.c_str(), ret, errno);
        if (errno == CROSS_POLICY_ERR) {
            ret = MediaFileUtils::CopyFileAndDelSrc(srcPath, destPath) ? 0 : -1;
            MEDIA_INFO_LOG("sendfile result:%{public}d", ret);
        }
    }
    return ret;
}
 
int32_t MediaLibraryPhotoOperations::UpdateExtension(const int32_t &fileId, const int32_t &fileType,
    PhotoExtInfo &photoExtInfo, ValuesBucket &updateValues)
{
    ImageFileType type = static_cast<ImageFileType>(fileType);
    auto itr = IMAGE_FILE_TYPE_MAP.find(type);
    CHECK_AND_RETURN_RET_LOG(itr != IMAGE_FILE_TYPE_MAP.end(), E_INVALID_ARGUMENTS,
        "fileType : %{public} is not support", fileType);
    photoExtInfo.format = itr->second;
    auto extensionItr = IMAGE_EXTENSION_MAP.find(type);
    CHECK_AND_RETURN_RET_LOG(itr != IMAGE_EXTENSION_MAP.end(), E_INVALID_ARGUMENTS,
        "fileType : %{public} is not support", fileType);
    photoExtInfo.extension = extensionItr->second;

    shared_ptr<FileAsset> fileAsset = GetFileAssetFromDb(PhotoColumn::MEDIA_ID, to_string(fileId),
                                                         OperationObject::FILESYSTEM_PHOTO, EDITED_COLUMN_VECTOR);
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_VALUES, "fileAsset is nullptr");
    std::string filePath = fileAsset->GetFilePath();
    std::string displayName = fileAsset->GetDisplayName();
    std::string modifyFilePath;
    std::string modifyDisplayName;
    GetModityExtensionPath(filePath, modifyFilePath, photoExtInfo.extension);
    GetModityExtensionPath(displayName, modifyDisplayName, photoExtInfo.extension);
    MEDIA_DEBUG_LOG("modifyFilePath:%{public}s", modifyFilePath.c_str());
    MEDIA_DEBUG_LOG("modifyDisplayName:%{public}s", modifyDisplayName.c_str());
    bool cond = ((modifyFilePath == filePath) && (modifyDisplayName == displayName));
    CHECK_AND_RETURN_RET(!cond, E_OK);
    updateValues.PutString(MediaColumn::MEDIA_FILE_PATH, modifyFilePath);
    updateValues.PutString(MediaColumn::MEDIA_NAME, modifyDisplayName);
    updateValues.PutString(MediaColumn::MEDIA_MIME_TYPE, photoExtInfo.format);
    updateValues.PutString(PhotoColumn::PHOTO_MEDIA_SUFFIX, ScannerUtils::GetFileExtension(modifyDisplayName));
    photoExtInfo.oldFilePath = filePath;
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
        CHECK_AND_PRINT_LOG(deleteRow >= 0, "delete photo failed, deleteRow=%{public}d", deleteRow);
    }
}

static int32_t GetCriticalState(const ValuesBucket& values, bool &isCritical)
{
    ValueObject isCriticalValObj;
    bool hasIsCritical = values.GetObject(PhotoColumn::PHOTO_IS_CRITICAL, isCriticalValObj);
    if (hasIsCritical) {
        int32_t isCriticalInt = -1;
        int ret = isCriticalValObj.GetInt(isCriticalInt);
        bool cond = (ret != E_OK || (isCriticalInt != 0 && isCriticalInt != 1));
        CHECK_AND_RETURN_RET(!cond, E_INVALID_VALUES);
        isCritical = (isCriticalInt == 1);
    } else {
        // If is_critical is not provided, derive from photo_risk_status
        ValueObject photoRiskStatusValObj;
        bool hasPhotoRiskStatus = values.GetObject(PhotoColumn::PHOTO_RISK_STATUS, photoRiskStatusValObj);
        CHECK_AND_RETURN_RET(hasPhotoRiskStatus, E_INVALID_VALUES);
        int32_t riskStatus = -1;
        int ret = photoRiskStatusValObj.GetInt(riskStatus);
        CHECK_AND_RETURN_RET(ret == E_OK, E_INVALID_VALUES);
        // SUSPICIOUS (2) and REJECTED (3) are critical
        isCritical = (riskStatus == 2 || riskStatus == 3);
    }
    return E_OK;
}

// Safe Album: Set photo critical state (inner API)
int32_t MediaLibraryPhotoOperations::SetPhotoCritical(MediaLibraryCommand &cmd)
{
    AccurateRefresh::AssetAccurateRefresh assetRefresh(AccurateRefresh::UPDATE_FILE_ASSTE_BUSSINESS_NAME);
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryPhotoOperations::SetPhotoCritical");

    // Get critical state from ValuesBucket
    bool isCritical = false;
    int32_t ret = GetCriticalState(cmd.GetValueBucket(), isCritical);
    MEDIA_INFO_LOG("SetPhotoCritical isCritical:%{public}d", isCritical);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);

    // Convert isCritical to critical_type and is_critical values
    int32_t photoRiskStatus = isCritical ? 3 : 1; // REJECTED (3) or APPROVED (1)
    int32_t isCriticalInt = isCritical ? 1 : 0;

    RdbPredicates predicates = RdbUtils::ToPredicates(cmd.GetDataSharePred(), PhotoColumn::PHOTOS_TABLE);
    vector<string> notifyUris = predicates.GetWhereArgs();
    MEDIA_INFO_LOG("SetPhotoCritical %{public}zu Photos, isCritical: %{public}d", notifyUris.size(), isCriticalInt);
    MediaLibraryRdbStore::ReplacePredicatesUriToId(predicates);

    ValuesBucket values;
    values.Put(PhotoColumn::PHOTO_RISK_STATUS, photoRiskStatus);
    values.Put(PhotoColumn::PHOTO_IS_CRITICAL, isCriticalInt);

    int32_t changedRows = assetRefresh.UpdateWithDateTime(values, predicates);
    MEDIA_INFO_LOG("SetPhotoCritical changedRows:%{public}d", changedRows);
    CHECK_AND_RETURN_RET(changedRows >= 0, changedRows);

    // Send notification
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_RET_LOG(watch != nullptr, E_ERR, "Can not get MediaLibraryNotify Instance");
    for (const auto &uri : notifyUris) {
        watch->Notify(uri, NotifyType::NOTIFY_UPDATE);
    }
    assetRefresh.Notify();
    return E_OK;
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
    if (MAP_OPERATION_FLAG) {
        MEDIA_INFO_LOG("map operation flag is true");
        string uri = MediaLibraryFormMapOperations::GetUriByFileId(fileId, path);
        CHECK_AND_RETURN_LOG(!uri.empty(), "Failed to GetUriByFileId(%{public}d, %{private}s)", fileId, path.c_str());

        vector<int64_t> formIds;
        MediaLibraryFormMapOperations::GetFormMapFormId(uri.c_str(), formIds);
        if (!formIds.empty()) {
            MediaLibraryFormMapOperations::PublishedChange(uri, formIds, isSave);
        }
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

    errCode = UpdateEditTime(fileAsset->GetId(), MediaFileUtils::UTCTimeSeconds());
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Failed to update edit time, fileId:%{public}d",
        fileAsset->GetId());
    if (fileAsset->GetMediaType() == MEDIA_TYPE_IMAGE && fileAsset->GetExistCompatibleDuplicate() != 0) {
        MediaLibraryTranscodeDataAgingOperation::DeleteTransCodeInfo(fileAsset->GetFilePath(),
            to_string(fileAsset->GetId()), __func__);
    }
    UpdateAlbumDateModified(fileAsset->GetOwnerAlbumId());
    ScanFile(path, false, true, true);
    NotifyFormMap(fileAsset->GetId(), fileAsset->GetFilePath(), false);
    AccurateRefresh::AlbumAccurateRefresh albumRefresh;
    albumRefresh.IsCoverContentChange(to_string(fileAsset->GetId()));
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
    CHECK_AND_RETURN_RET(PhotoEditingRecord::GetInstance()->StartRevert(fileId), E_IS_IN_COMMIT);

    int32_t errCode = DoRevertEdit(fileAsset);
    PhotoEditingRecord::GetInstance()->EndRevert(fileId);
    return errCode;
}

int32_t MediaLibraryPhotoOperations::UpdateMovingPhotoSubtype(int32_t fileId, int32_t currentPhotoSubType)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "Failed to get rdbStore when updating subtype");

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
    CHECK_AND_RETURN_RET_LOG(errCode == NativeRdb::E_OK && updateRows >= 0, E_HAS_DB_ERROR,
        "Update subtype field failed. errCode:%{public}d, updateRows:%{public}d", errCode, updateRows);
    return E_OK;
}

void ResetOcrInfo(const int32_t &fileId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "MediaLibrary rdbStore is nullptr!");
    string sqlDeleteOcr = "DELETE FROM " + VISION_OCR_TABLE + " WHERE file_id = " + to_string(fileId) + ";" +
        " UPDATE " + VISION_TOTAL_TABLE + " SET ocr = 0 WHERE file_id = " + to_string(fileId) + ";";

    int ret = rdbStore->ExecuteSql(sqlDeleteOcr);
    CHECK_AND_RETURN_LOG(ret == NativeRdb::E_OK,
        "Update ocr info failed, ret = %{public}d, file id is %{public}d", ret, fileId);
}

static void RemoveMovingPhotoVideo(std::string &sourcePath, std::string &path)
{
    string sourceVideoPath = MediaFileUtils::GetMovingPhotoVideoPath(sourcePath);
    if (!MediaFileUtils::IsFileExists(sourceVideoPath)) {
        MEDIA_ERR_LOG("source video path not exists");
        return;
    }
    string videoPath = MediaFileUtils::GetMovingPhotoVideoPath(path);
    if (!MediaFileUtils::IsFileExists(videoPath)) {
        MEDIA_ERR_LOG("video path not exists");
        return;
    }
    CHECK_AND_PRINT_LOG(MediaFileUtils::DeleteFile(videoPath),
        "Failed to delete video file, errno: %{public}d", errno);
}

static void NotifyAnalysisAlbum(int32_t albumId)
{
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_LOG(watch != nullptr, "Can not get MediaLibraryNotify Instance");
    watch->Notify(MediaFileUtils::GetUriByExtrConditions(
        PhotoAlbumColumns::ANALYSIS_ALBUM_URI_PREFIX, to_string(albumId)), NotifyType::NOTIFY_UPDATE);
}

static void UpdateAndNotifyMovingPhotoAlbum()
{
    int32_t albumId;
    CHECK_AND_RETURN_LOG(
        MediaLibraryRdbUtils::QueryShootingModeAlbumIdByType(ShootingModeAlbumType::MOVING_PICTURE, albumId),
        "Failed to query albumId");

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "rdbstore is nullptr");

    MediaLibraryRdbUtils::UpdateAnalysisAlbumInternal(rdbStore, { to_string(albumId) });
    NotifyAnalysisAlbum(albumId);
}

int32_t MediaLibraryPhotoOperations::DoRevertEdit(const std::shared_ptr<FileAsset> &fileAsset)
{
    MEDIA_INFO_LOG("begin to do revertEdit");
    int32_t errCode = CheckFileAssetStatus(fileAsset);
    CHECK_AND_RETURN_RET(errCode == E_OK, errCode);
    int32_t fileId = fileAsset->GetId();
    CHECK_AND_RETURN_RET_INFO_LOG(fileAsset->GetPhotoEditTime() !=0, E_OK, "File %{public}d is not edit", fileId);

    string path = fileAsset->GetFilePath();
    CHECK_AND_RETURN_RET_LOG(!path.empty(), E_INVALID_URI, "Can not get file path, fileId=%{public}d", fileId);
    string sourcePath = GetEditDataSourcePath(path);
    CHECK_AND_RETURN_RET_LOG(!sourcePath.empty(), E_INVALID_URI, "Cannot get source path, id=%{public}d", fileId);
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsFileExists(sourcePath), E_NO_SUCH_FILE, "Can not get source file");

    int32_t subtype = static_cast<int32_t>(fileAsset->GetPhotoSubType());
    int32_t movingPhotoSubtype = static_cast<int32_t>(PhotoSubType::MOVING_PHOTO);
    bool revertMovingPhotoGraffiti = false;
    if (fileAsset->GetOriginalSubType() == movingPhotoSubtype &&
        subtype == static_cast<int32_t>(PhotoSubType::DEFAULT)) {
        errCode = UpdateMovingPhotoSubtype(fileAsset->GetId(), subtype);
        CHECK_AND_RETURN_RET_LOG(errCode == E_OK, E_HAS_DB_ERROR, "Failed to update movingPhoto subtype");
        revertMovingPhotoGraffiti = true;
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
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::DeleteFileWithRetry(path), E_HAS_FS_ERROR,
            "Failed to delete asset, path:%{private}s", path.c_str());
    }

    if (MovingPhotoFileUtils::IsMovingPhoto(subtype,
        fileAsset->GetMovingPhotoEffectMode(), fileAsset->GetOriginalSubType())) {
        RemoveMovingPhotoVideo(sourcePath, path);
    }
    CHECK_AND_RETURN_RET_LOG(DoRevertFilters(fileAsset, path, sourcePath) == E_OK, E_FAIL,
        "Failed to DoRevertFilters to photo");

    // revert cloud enhancement ce_available
#ifdef MEDIALIBRARY_FEATURE_CLOUD_ENHANCEMENT
    EnhancementManager::GetInstance().RevertEditUpdateInternal(fileId);
#endif
    string fileIdStr = to_string(fileId);
    ThumbnailService::GetInstance()->HasInvalidateThumbnail(fileIdStr, PhotoColumn::PHOTOS_TABLE);
    ScanFile(path, true, true, true);
    if (revertMovingPhotoGraffiti) {
        UpdateAndNotifyMovingPhotoAlbum();
    }
    AccurateRefresh::AlbumAccurateRefresh albumRefresh;
    albumRefresh.IsCoverContentChange(fileIdStr);
    NotifyFormMap(fileAsset->GetId(), fileAsset->GetFilePath(), false);
    MEDIA_INFO_LOG("end to do revertEdit");
    return E_OK;
}

// 与图库约定，图片一键还原失败时，将source文件作为效果图，并在editdata文件中写入EDITDATA，作为一种特殊的编辑保存
int32_t MediaLibraryPhotoOperations::DoRevertAfterAddFiltersFailed(const std::shared_ptr<FileAsset> &fileAsset,
    const std::string &path, const std::string &sourcePath)
{
    MEDIA_WARN_LOG("Failed to add filters to photo.");
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CopyFileUtil(sourcePath, path), E_HAS_FS_ERROR,
        "Failed to copy source file.");

    string editDataPath = GetEditDataPath(path);
    CHECK_AND_RETURN_RET_LOG(!editDataPath.empty(), E_INVALID_VALUES, "EditData path is empty");
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateFile(editDataPath), E_HAS_FS_ERROR,
        "Failed to create editdata file %{private}s", editDataPath.c_str());
    nlohmann::json editDataJson;
    editDataJson[COMPATIBLE_FORMAT] = SPECIAL_EDIT_COMPATIBLE_FORMAT;
    editDataJson[FORMAT_VERSION] = SPECIAL_EDIT_FORMAT_VERSION;
    editDataJson[EDIT_DATA] = SPECIAL_EDIT_EDIT_DATA;
    editDataJson[APP_ID] = SPECIAL_EDIT_APP_ID;
    string editData = editDataJson.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::WriteStrToFile(editDataPath, editData), E_HAS_FS_ERROR,
        "Failed to write editdata:%{private}s", editDataPath.c_str());
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_ERR, "fileAsset is nullptr");
    UpdateEditTime(fileAsset->GetId(), MediaFileUtils::UTCTimeSeconds());
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::DoRevertFilters(const std::shared_ptr<FileAsset> &fileAsset,
    std::string &path, std::string &sourcePath)
{
    string editDataCameraPath = GetEditDataCameraPath(path);
    int32_t subtype = static_cast<int32_t>(fileAsset->GetPhotoSubType());
    if (!MediaFileUtils::IsFileExists(editDataCameraPath)) {
        string storagePath = fileAsset->GetStoragePath();
        if (!storagePath.empty() && MediaFileUtils::IsFileExists(storagePath)) {
            CHECK_AND_RETURN_RET_LOG(LakeFileUtils::CoverLakeFile(sourcePath, storagePath), E_HAS_FS_ERROR,
                "Can not cover %{private}s to %{private}s", sourcePath.c_str(), storagePath.c_str());
        } else {
            CHECK_AND_RETURN_RET_LOG(MediaFileUtils::ModifyAsset(sourcePath, path) == E_OK, E_HAS_FS_ERROR,
                "Can not modify %{private}s to %{private}s", sourcePath.c_str(), path.c_str());
        }
        if (MovingPhotoFileUtils::IsMovingPhoto(subtype,
            fileAsset->GetMovingPhotoEffectMode(), fileAsset->GetOriginalSubType())) {
            string videoPath = MediaFileUtils::GetMovingPhotoVideoPath(path);
            string sourceVideoPath = MediaFileUtils::GetMovingPhotoVideoPath(sourcePath);
            CHECK_AND_RETURN_RET_LOG(Move(sourceVideoPath, videoPath) == E_OK, E_HAS_FS_ERROR,
                "Can not move %{private}s to %{private}s", sourceVideoPath.c_str(), videoPath.c_str());
        }
    } else {
        string editData;
        CHECK_AND_RETURN_RET_LOG(ReadEditdataFromFile(editDataCameraPath, editData) == E_OK, E_HAS_FS_ERROR,
            "Failed to read editdata, path=%{public}s", editDataCameraPath.c_str());
        if (AddFiltersToPhoto(sourcePath, path, editData) != E_OK) {
            CHECK_AND_RETURN_RET_LOG(DoRevertAfterAddFiltersFailed(fileAsset, path, sourcePath) == E_OK, E_HAS_FS_ERROR,
                "Failed to do revertAfterAddFiltersFailed");
        }
        if (MovingPhotoFileUtils::IsMovingPhoto(subtype,
            fileAsset->GetMovingPhotoEffectMode(), fileAsset->GetOriginalSubType())) {
            CHECK_AND_RETURN_RET_LOG(AddFiltersToVideoExecute(fileAsset->GetFilePath(), false, true) == E_OK, E_FAIL,
                "Failed to add filters to video");
        }
    }
    return E_OK;
}

void MediaLibraryPhotoOperations::DeleteRevertMessage(const string &path)
{
    CHECK_AND_RETURN_LOG(!path.empty(), "Input path is empty");

    string editDirPath = GetEditDataDirPath(path);
    CHECK_AND_RETURN_LOG(!editDirPath.empty(), "Can not get edit data dir path from path %{private}s", path.c_str());

    CHECK_AND_RETURN_LOG(MediaFileUtils::IsDirectory(editDirPath), "Edit dir path is not exist.");

    CHECK_AND_RETURN_LOG(MediaFileUtils::DeleteDir(editDirPath),
        "Failed to delete %{private}s dir, filePath is %{private}s", editDirPath.c_str(), path.c_str());
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
        return LakeFileUtils::MoveFileInEditScene(cachePath, destPath);
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
        int32_t errCode = LakeFileUtils::MoveFileInEditScene(cacheVideoPath, destVideoPath);
        CHECK_AND_RETURN_RET_LOG(errCode == E_OK, E_FILE_OPER_FAIL,
            "Failed to move moving photo video from %{private}s to %{private}s, errCode: %{public}d",
            cacheVideoPath.c_str(), destVideoPath.c_str(), errCode);
    }
    return LakeFileUtils::MoveFileInEditScene(cachePath, destPath);
}

bool MediaLibraryPhotoOperations::CheckCacheCmd(MediaLibraryCommand& cmd, int32_t subtype, const string& displayName)
{
    string cacheFileName;
    CHECK_AND_RETURN_RET_LOG(GetStringFromValuesBucket(cmd.GetValueBucket(), CACHE_FILE_NAME, cacheFileName), false,
        "Failed to get cache file name");
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
        CHECK_AND_RETURN_RET_LOG(containsVideo, false, "Failed to get video when creating moving photo");

        string videoExtension = MediaFileUtils::GetExtensionFromPath(movingPhotoVideoName);
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CheckMovingPhotoVideoExtension(videoExtension), false,
            "Failed to check video of moving photo, extension: %{public}s", videoExtension.c_str());
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
    CHECK_AND_RETURN_LOG(nlohmann::json::accept(editData),
        "Failed to verify the editData format, editData is: %{private}s", editData.c_str());
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

static bool ConvertPhotoPathToThumbnailDirPath(std::string& path)
{
    const std::string photoRelativePath = "/Photo/";
    const std::string thumbRelativePath = "/.thumbs/Photo/";
    size_t pos = path.find(photoRelativePath);
    CHECK_AND_RETURN_RET_LOG(pos != string::npos, false, "source file invalid! path is %{public}s", path.c_str());
    path.replace(pos, photoRelativePath.length(), thumbRelativePath);
    return true;
}

void MediaLibraryPhotoOperations::StoreThumbnailAndEditSize(const string& photoId, const string& photoPath)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "RdbStore is nullptr!");

    string thumbnailDir {photoPath};
    CHECK_AND_RETURN_LOG(ConvertPhotoPathToThumbnailDirPath(thumbnailDir),
        "Failed to get thumbnail dir path from photo path! file id: %{public}s", photoId.c_str());
    uint64_t photoThumbnailSize = GetFolderSize(thumbnailDir);

    uint64_t editDataSize = 0;
    string editDataDir;
    if (MediaLibraryPhotoOperations::ConvertPhotoCloudPathToLocalData(photoPath, editDataDir) != E_OK) {
        MEDIA_ERR_LOG("Failed to Convert cloue path: %{public}s to local path", photoPath.c_str());
        return;
    }
    size_t editDataSizeCast = static_cast<size_t>(editDataSize);
    MediaFileUtils::StatDirSize(editDataDir, editDataSizeCast);
    editDataSize = editDataSizeCast;

    string sql = "INSERT OR REPLACE INTO " + PhotoExtColumn::PHOTOS_EXT_TABLE + " (" +
                 PhotoExtColumn::PHOTO_ID + ", " +
                 PhotoExtColumn::THUMBNAIL_SIZE + ", " +
                 PhotoExtColumn::EDITDATA_SIZE + ") VALUES (?, ?, ?)";

    vector<NativeRdb::ValueObject> values = {
        NativeRdb::ValueObject(photoId),
        NativeRdb::ValueObject(to_string(photoThumbnailSize)),
        NativeRdb::ValueObject(to_string(editDataSize))
    };

    int32_t ret = rdbStore->ExecuteSql(sql, values);
    CHECK_AND_RETURN_LOG(ret == NativeRdb::E_OK,
        "Failed to store sizes for photoId: %{public}s", photoId.c_str());

    MEDIA_INFO_LOG("Successfully stored thumbnail size: %{public}" PRIu64, photoThumbnailSize);
    MEDIA_INFO_LOG("Edit data size: %{public}" PRIu64, editDataSize);
    MEDIA_INFO_LOG("For photo ID: %{public}s", photoId.c_str());
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

    string tmpPath = LakeFileUtils::GetAssetRealPath(assetPath);
    if (fileAsset->GetPhotoEditTime() == 0) { // the asset has not been edited before
        string editDataDirPath = GetEditDataDirPath(assetPath);
        CHECK_AND_RETURN_RET_LOG(!editDataDirPath.empty(), E_INVALID_URI, "Failed to get edit dir path");
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateDirectory(editDataDirPath), E_HAS_FS_ERROR,
            "Failed to create dir %{private}s", editDataDirPath.c_str());

        string sourcePath = GetEditDataSourcePath(assetPath);
        CHECK_AND_RETURN_RET_LOG(!sourcePath.empty(), E_INVALID_URI, "Failed to get source path");
        if (!MediaFileUtils::IsFileExists(sourcePath)) {
            string tmpPath = LakeFileUtils::GetAssetRealPath(assetPath);
            CHECK_AND_RETURN_RET_LOG(LakeFileUtils::MoveFileInEditScene(tmpPath, sourcePath) == E_SUCCESS,
                E_HAS_FS_ERROR, "Move file failed, srcPath:%{private}s, newPath:%{private}s",
                assetPath.c_str(), sourcePath.c_str());
        }

        if (!MediaFileUtils::IsFileExists(editDataPath)) {
            CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateFile(editDataPath), E_HAS_FS_ERROR,
                "Failed to create file %{private}s", editDataPath.c_str());
        }
    }

    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsFileExists(editDataPath), E_OK,
        "Failed to find editdata:%{private}s", editDataPath.c_str());
    if (!tmpPath.empty()) {
        MEDIA_INFO_LOG("NEW Edit lake assets need delete invalid thumbnails");
        ThumbnailService::GetInstance()->HasInvalidateThumbnail(to_string(fileAsset->GetId()),
            PhotoColumn::PHOTOS_TABLE);
    }
    std::string imageId = fileAsset->GetPhotoId();
    auto pictureManagerThread = PictureManagerThread::GetInstance();
    if (pictureManagerThread != nullptr) {
        pictureManagerThread->DeleteDataWithImageId(imageId, PictureType::HIGH_QUALITY_PICTURE);
        pictureManagerThread->DeleteDataWithImageId(imageId, PictureType::LOW_QUALITY_PICTURE);
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
        PhotoColumn::PHOTO_SUBTYPE, PhotoColumn::MEDIA_TIME_PENDING, PhotoColumn::MEDIA_DATE_TRASHED,
        PhotoColumn::PHOTO_EXIST_COMPATIBLE_DUPLICATE };
    shared_ptr<FileAsset> fileAsset = GetFileAssetFromDb(
        PhotoColumn::MEDIA_ID, to_string(id), OperationObject::FILESYSTEM_PHOTO, columns);
    return fileAsset;
}

int32_t MediaLibraryPhotoOperations::GetPicture(const int32_t &fileId, std::shared_ptr<Media::Picture> &picture,
    bool isCleanImmediately, std::string &photoId, bool &isHighQualityPicture)
{
    MEDIA_ERR_LOG("Enter GetPicture");
    int32_t ret = GetPhotoIdByFileId(fileId, photoId);
    if (ret != E_OK || photoId.empty()) {
        HILOG_COMM_ERROR("%{public}s:{%{public}s:%{public}d} photoId is emply fileId is: %{public}d",
            MLOG_TAG, __FUNCTION__, __LINE__, fileId);
        return E_FILE_EXIST;
    }

    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} photoId: %{public}s",
        MLOG_TAG, __FUNCTION__, __LINE__, photoId.c_str());
    auto pictureManagerThread = PictureManagerThread::GetInstance();
    bool isTakeEffect = false;
    CHECK_AND_EXECUTE(pictureManagerThread == nullptr,
        picture = pictureManagerThread->GetDataWithImageId(photoId,
        isHighQualityPicture, isTakeEffect, isCleanImmediately));
    CHECK_AND_RETURN_RET_LOG(picture != nullptr, E_FILE_EXIST, "picture is not exists!");
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} "
        "photoId: %{public}s, picture use: %{public}d, picture point to addr: %{public}s",
        MLOG_TAG, __FUNCTION__, __LINE__,
        photoId.c_str(), static_cast<int32_t>(picture.use_count()),
        std::to_string(reinterpret_cast<long long>(picture.get())).c_str());
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::GetTakeEffect(std::shared_ptr<Media::Picture> &picture, std::string &photoId)
{
    auto pictureManagerThread = PictureManagerThread::GetInstance();
    bool isHighQualityPicture = false;
    bool isTakeEffect = false;
    if (pictureManagerThread != nullptr) {
        picture = pictureManagerThread->GetDataWithImageId(photoId, isHighQualityPicture,
            isTakeEffect, false);
    }
    CHECK_AND_RETURN_RET_LOG(picture != nullptr, E_FILE_EXIST, "picture is not exists!");
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} get takeEffect: %{public}d",
        MLOG_TAG, __FUNCTION__, __LINE__, isTakeEffect);
    if (isTakeEffect) {
        return E_ERR;
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
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK, E_FILE_EXIST, "result set is empty");

    string photoId = GetStringVal(PhotoColumn::PHOTO_ID, resultSet);
    resultSet->Close();
    CHECK_AND_RETURN_RET_LOG(!photoId.empty(), E_FILE_EXIST, "photoId is emply fileId is: %{public}d", fileId);

    MEDIA_INFO_LOG("photoId: %{public}s", photoId.c_str());
    auto pictureManagerThread = PictureManagerThread::GetInstance();
    CHECK_AND_EXECUTE(pictureManagerThread == nullptr, pictureManagerThread->FinishAccessingPicture(photoId));
    return E_OK;
}

int64_t MediaLibraryPhotoOperations::CloneSingleAsset(MediaLibraryCommand &cmd)
{
    const ValuesBucket& values = cmd.GetValueBucket();
    int fileId = -1;
    ValueObject valueObject;
    CHECK_AND_RETURN_RET_LOG(values.GetObject(MediaColumn::MEDIA_ID, valueObject), fileId,
        "Failed to get media id from values bucket.");
    valueObject.GetInt(fileId);
    string title;
    GetStringFromValuesBucket(values, MediaColumn::MEDIA_TITLE, title);
    return MediaLibraryAlbumFusionUtils::CloneSingleAsset(static_cast<int64_t>(fileId), title);
}

int32_t MediaLibraryPhotoOperations::AddFilters(MediaLibraryCommand& cmd)
{
    // moving photo video save and add filters
    const ValuesBucket& values = cmd.GetValueBucket();
    int32_t videoType = ORIGIN_VIDEO;
    GetInt32FromValuesBucket(values, VIDEO_TYPE_KEYWORD, videoType);
    string videoSaveFinishedUri;
    if (GetStringFromValuesBucket(values, NOTIFY_VIDEO_SAVE_FINISHED, videoSaveFinishedUri)) {
        int32_t id = -1;
        CHECK_AND_RETURN_RET_LOG(GetInt32FromValuesBucket(values, PhotoColumn::MEDIA_ID, id),
            E_INVALID_VALUES, "Failed to get fileId");
        vector<string> columns = { videoSaveFinishedUri };

        vector<string> fileAssetColumns = {PhotoColumn::MEDIA_ID, PhotoColumn::MEDIA_FILE_PATH,
            PhotoColumn::PHOTO_SUBTYPE, PhotoColumn::PHOTO_EDIT_TIME, PhotoColumn::PHOTO_ID,
            PhotoColumn::STAGE_VIDEO_TASK_STATUS };
        shared_ptr<FileAsset> fileAsset = GetFileAssetFromDb(
            PhotoColumn::MEDIA_ID, to_string(id), OperationObject::FILESYSTEM_PHOTO, fileAssetColumns);
        CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_VALUES,
            "Failed to GetFileAssetFromDb, fileId = %{public}d", id);
        int32_t errCode = AddFiltersToVideoExecute(fileAsset->GetFilePath(), true, true, videoType);
        if ((fileAsset->GetStageVideoTaskStatus() == static_cast<int32_t>(StageVideoTaskStatus::NEED_TO_STAGE)) &&
        (videoType != XT_EFFECT_VIDEO)) {
            MultiStagesMovingPhotoCaptureManager::SaveMovingPhotoVideoFinished(id);
        }
        return errCode;
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
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK,
        E_ERR, "result set is empty");
    if (GetInt32Val(PhotoColumn::PHOTO_IS_TEMP, resultSet) == 0) {
        return E_OK;
    }
    resultSet->Close();
    string uri = cmd.GetQuerySetParam("uri");
    std::shared_ptr<Media::Picture> resultPicture = nullptr;

    PhotoExtInfo photoExtInfo = {"", MIME_TYPE_JPEG, "", "", nullptr};
    string photoId;
    int32_t getPicRet = GetPicture(fileId, photoExtInfo.picture, false, photoId, photoExtInfo.isHighQualityPicture);
    SavePicture(fileType, fileId, getPicRet, photoExtInfo, resultPicture);
    string path = MediaFileUri::GetPathFromUri(uri, true);
    MediaLibraryAssetOperations::ScanFileWithoutAlbumUpdate(path, false, false, true, fileId, resultPicture);
    return E_OK;
}

int32_t UpdateQualityAndDirty(const int32_t &fileId)
{
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, fileId);
    ValuesBucket values;
    values.Put(PhotoColumn::PHOTO_QUALITY, static_cast<int32_t>(MultiStagesPhotoQuality::FULL));
    values.Put(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_NEW));
    int32_t updatedRows = MediaLibraryRdbStore::UpdateWithDateTime(values, predicates);
    return updatedRows;
}

int32_t MediaLibraryPhotoOperations::SavePicture(const int32_t &fileType, const int32_t &fileId,
    const int32_t getPicRet, PhotoExtInfo &photoExtInfo, std::shared_ptr<Media::Picture> &resultPicture)
{
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} savePicture fileType is: %{public}d, fileId is: %{public}d",
        MLOG_TAG, __FUNCTION__, __LINE__, fileType, fileId);
    CHECK_AND_RETURN_RET_LOG(getPicRet == E_OK && photoExtInfo.picture != nullptr, E_FILE_EXIST, "Failed get picture");
    auto fileAsset = GetFileAssetFromDb(PhotoColumn::MEDIA_ID, to_string(fileId),
                                        OperationObject::FILESYSTEM_PHOTO, EDITED_COLUMN_VECTOR);
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_VALUES, "fileAsset is nullptr");
    string assetPath = fileAsset->GetFilePath();
    CHECK_AND_RETURN_RET_LOG(!assetPath.empty(), E_INVALID_VALUES, "Failed to get asset path");
    string editData = "";
    string editDataCameraPath = GetEditDataCameraPath(assetPath);
    bool existEditData = (ReadEditdataFromFile(editDataCameraPath, editData) == E_OK);
    if (existEditData) {
        FileUtils::DealPicture(photoExtInfo.format, GetEditDataSourcePath(assetPath), photoExtInfo.picture,
            photoExtInfo.isHighQualityPicture);
    } else {
        FileUtils::DealPicture(photoExtInfo.format, assetPath, photoExtInfo.picture, photoExtInfo.isHighQualityPicture);
    }
    std::string photoId;
    std::shared_ptr<Media::Picture> picture;
    bool isHighQualityPicture = false;
    if (existEditData && GetPicture(fileId, picture, false, photoId, isHighQualityPicture) == E_OK &&
        GetTakeEffect(picture, photoId) == E_OK) {
        int32_t ret = MediaChangeEffect::TakeEffectForPicture(picture, editData);
        FileUtils::DealPicture(photoExtInfo.format, assetPath, picture, isHighQualityPicture);
    }
    isHighQualityPicture = (picture == nullptr) ? photoExtInfo.isHighQualityPicture : isHighQualityPicture;
    if (isHighQualityPicture) {
        int32_t updatedRows = UpdateQualityAndDirty(fileId);
        CHECK_AND_PRINT_LOG(updatedRows >= 0, "update photo quality fail.");
        MultistagesCaptureNotify::NotifyOnProcess(fileAsset, MultistagesCaptureNotifyType::ON_PROCESS_IMAGE_DONE);
    }
    resultPicture = (picture == nullptr) ? photoExtInfo.picture : picture;
    photoId = (picture == nullptr) ? photoExtInfo.photoId : photoId;
    auto pictureManagerThread = PictureManagerThread::GetInstance();
    if (pictureManagerThread != nullptr) {
        pictureManagerThread->FinishAccessingPicture(photoId);
        pictureManagerThread->DeleteDataWithImageId(lastPhotoId_, LOW_QUALITY_PICTURE);
    }
    lastPhotoId_ = photoId;
    MultistagesCaptureNotify::NotifyLowQualityMemoryCount();

    size_t size = -1;    // 删除已经存在的异常后缀的图片
    MediaFileUtils::GetFileSize(photoExtInfo.oldFilePath, size);
    bool cond = (photoExtInfo.oldFilePath != "" && size > 0);
    CHECK_AND_EXECUTE(!cond, DeleteAbnormalFile(assetPath, fileId, photoExtInfo.oldFilePath));
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
    CHECK_AND_RETURN_RET(GetPicture(fileId, picture, true, photoId, isHighQualityPicture) != E_OK, E_OK);
    return ret;
}

int32_t SaveTempMovingPhotoVideo(const string &assetPath, int32_t videoType)
{
    string assetTempPath = MediaFileUtils::GetTempMovingPhotoVideoPath(assetPath);
    string assetSavePath = MediaFileUtils::GetMovingPhotoVideoPath(assetPath);
    if (videoType == 1) {
        assetTempPath = MediaFileUtils::GetTempOriMovingPhotoVideoPath(assetPath);
        assetSavePath = MediaFileUtils::GetOriMovingPhotoVideoPath(assetPath);
    }
    MEDIA_INFO_LOG("SaveTempMovingPhotoVideo assetTempPath %{public}s", assetTempPath.c_str());
    MEDIA_INFO_LOG("SaveTempMovingPhotoVideo assetSavePath %{public}s", assetSavePath.c_str());
    if (!MediaFileUtils::IsFileExists(assetSavePath)) {
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::ModifyAsset(assetTempPath, assetSavePath) == E_SUCCESS,
            E_HAS_FS_ERROR, "Move video file failed, srcPath:%{private}s, newPath:%{private}s",
            assetTempPath.c_str(), assetSavePath.c_str());
        return E_OK;
    }
    MEDIA_ERR_LOG("File exists, assetSavePath: %{public}s.", assetSavePath.c_str());
    return E_ERR;
}

int32_t MediaLibraryPhotoOperations::CopyVideoFile(const string& assetPath, bool toSource)
{
    string sourceImagePath = PhotoFileUtils::GetEditDataSourcePath(assetPath);
    string videoPath = MediaFileUtils::GetMovingPhotoVideoPath(assetPath);
    string sourceVideoPath = MediaFileUtils::GetMovingPhotoVideoPath(sourceImagePath);
    if (toSource) {
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CopyFileSafe(videoPath, sourceVideoPath), E_HAS_FS_ERROR,
            "Copy videoPath to sourceVideoPath, path:%{private}s", videoPath.c_str());
    } else {
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CopyFileSafe(sourceVideoPath, videoPath), E_HAS_FS_ERROR,
            "Copy sourceVideoPath to videoPath, path:%{private}s", sourceVideoPath.c_str());
    }

    if (MediaFileUtils::IsFileExists(assetPath)) {
        MediaLibraryObjectUtils::ScanMovingPhotoVideoAsync(assetPath, true);
    }

    return E_OK;
}

int32_t MediaLibraryPhotoOperations::AddFiltersToVideoExecute(const std::string &assetPath,
    bool isSaveVideo, bool isNeedScan, int32_t videoType)
{
    string editDataCameraPath = MediaLibraryAssetOperations::GetEditDataCameraPath(assetPath);
    if ((MediaFileUtils::IsFileExists(editDataCameraPath)) && (videoType == ORIGIN_VIDEO)) {
        string editData;
        CHECK_AND_RETURN_RET_LOG(ReadEditdataFromFile(editDataCameraPath, editData) == E_OK, E_HAS_FS_ERROR,
            "Failed to read editData, path = %{public}s", editDataCameraPath.c_str());
        bool isFiltersFieldEmpty = false;
        // erase watermark tag and sticker field
        int32_t errCode = VideoCompositionCallbackImpl::EraseWatermarkTagAndStickerField(editData, isFiltersFieldEmpty);
        CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Failed to erase watermark tag and sticker field");
        if (isFiltersFieldEmpty && isSaveVideo) {
            HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} MovingPhoto video only supports filter now.",
                MLOG_TAG, __FUNCTION__, __LINE__);
            CHECK_AND_RETURN_RET_LOG(SaveTempMovingPhotoVideo(assetPath, videoType) == E_OK, E_HAS_FS_ERROR,
                "Failed to save temp movingphoto video, path = %{public}s", assetPath.c_str());
            return CopyVideoFile(assetPath, true);
        } else if (isFiltersFieldEmpty && !isSaveVideo) {
            return CopyVideoFile(assetPath, false);
        }
        HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} "
            "AddFiltersToVideoExecute after EraseStickerField, editData = %{public}s",
            MLOG_TAG, __FUNCTION__, __LINE__, editData.c_str());
        CHECK_AND_RETURN_RET_LOG(SaveSourceVideoFile(assetPath, true) == E_OK, E_HAS_FS_ERROR,
            "Failed to save source video, path = %{public}s", assetPath.c_str());
        VideoCompositionCallbackImpl::AddCompositionTask(assetPath, editData, isNeedScan);
    } else {
        int32_t ret = SaveTempMovingPhotoVideo(assetPath, videoType);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret,
            "Failed to save temp video, path = %{private}s", assetPath.c_str());
        MediaLibraryObjectUtils::ScanMovingPhotoVideoAsync(assetPath, true);
    }
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::AddFiltersForSourcePicture(std::shared_ptr<Media::Picture> &picture,
    const string& assetPath, const string& editDataCameraSourcePath, const string& mimeType, int32_t fileId)
{
    string editData;
    MediaFileUtils::ReadStrFromFile(editDataCameraSourcePath, editData);
    ParseCloudEnhancementEditData(editData);
    int32_t ret = AddFiltersToPicture(picture, assetPath, editData);
    if (ret != E_OK) {
        return ret;
    }
    FileUtils::DealPicture(mimeType, assetPath, picture, true);
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

    string editData;
    MediaFileUtils::ReadStrFromFile(editDataCameraSourcePath, editData);
    ParseCloudEnhancementEditData(editData);
    return AddFiltersToPhoto(sourcePath, assetPath, editData);
}

int32_t MediaLibraryPhotoOperations::SubmitEditCacheExecute(MediaLibraryCommand& cmd,
    const shared_ptr<FileAsset>& fileAsset, const string& cachePath, bool isWriteGpsAdvanced)
{
    string editData;
    int32_t id = fileAsset->GetId();
    int32_t errCode = ParseMediaAssetEditData(cmd, editData);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Failed to parse MediaAssetEditData");
    errCode = SaveSourceAndEditData(fileAsset, editData);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Failed to save source and editData");

    int32_t subtype = fileAsset->GetPhotoSubType();
    bool addMovingPhotoGraffiti = false;
    if (subtype == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO) ||
        fileAsset->GetMovingPhotoEffectMode() == static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY)) {
        errCode = SubmitEditMovingPhotoExecute(cmd, fileAsset, addMovingPhotoGraffiti);
        CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Failed to SubmitEditMovingPhotoExecute");
    }

    errCode = UpdateEditTime(id, MediaFileUtils::UTCTimeSeconds());
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Failed to update edit time, fileId:%{public}d", id);

    string assetPath = fileAsset->GetFilePath();
    errCode = MoveCacheFile(cmd, subtype, cachePath, assetPath);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, E_FILE_OPER_FAIL,
        "Failed to move %{private}s to %{private}s, errCode: %{public}d",
        cachePath.c_str(), assetPath.c_str(), errCode);
    UpdateAlbumDateModified(fileAsset->GetOwnerAlbumId());
    ResetOcrInfo(id);
    if (isWriteGpsAdvanced) {
        MultiStagesPhotoCaptureManager::UpdateLocation(cmd.GetValueBucket(), true, assetPath, id);
    }

    // delete cloud enhacement task
#ifdef MEDIALIBRARY_FEATURE_CLOUD_ENHANCEMENT
    vector<string> fileId;
    fileId.emplace_back(to_string(fileAsset->GetId()));
    vector<string> photoId;
    EnhancementManager::GetInstance().CancelTasksInternal(fileId, photoId, CloudEnhancementAvailableType::EDIT);
#endif
    ScanFile(assetPath, false, true, true);
    MediaLibraryAnalysisAlbumOperations::UpdatePortraitAlbumCoverSatisfied(id);
    if (addMovingPhotoGraffiti) {
        UpdateAndNotifyMovingPhotoAlbum();
    }
    string fileIdStr = to_string(fileAsset->GetId());
    AccurateRefresh::AlbumAccurateRefresh albumRefresh;
    albumRefresh.IsCoverContentChange(fileIdStr);
    NotifyFormMap(id, assetPath, false);
    MediaLibraryVisionOperations::EditCommitOperation(cmd);
    MEDIA_INFO_LOG("SubmitEditCacheExecute success, isWriteGpsAdvanced: %{public}d.", isWriteGpsAdvanced);
    return errCode;
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

    std::string val = cmd.GetQuerySetParam(SET_LOCATION_KEY);
    bool isWriteGpsAdvanced = val == SET_LOCATION_VALUE;

    if (isEdit) {
        CHECK_AND_RETURN_RET(PhotoEditingRecord::GetInstance()->StartCommitEdit(id), E_IS_IN_REVERT);
        int32_t errCode = SubmitEditCacheExecute(cmd, fileAsset, cachePath, isWriteGpsAdvanced);
        if (fileAsset->GetExistCompatibleDuplicate() != 0) {
            MediaLibraryTranscodeDataAgingOperation::DeleteTransCodeInfo(fileAsset->GetFilePath(),
                to_string(fileAsset->GetId()), __func__);
        }
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
    if (isWriteGpsAdvanced) {
        MultiStagesPhotoCaptureManager::UpdateLocation(cmd.GetValueBucket(), true, assetPath, id);
    }
    ScanFile(assetPath, false, true, true);
    MEDIA_INFO_LOG("SubmitCacheExecute success, isWriteGpsAdvanced: %{public}d.", isWriteGpsAdvanced);
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::SaveSourceVideoFile(const string& assetPath, const bool& isTemp)
{
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} "
        "Moving photo SaveSourceVideoFile begin, assetPath: %{public}s",
        MLOG_TAG, __FUNCTION__, __LINE__, DfxUtils::GetSafePath(assetPath).c_str());
    string sourceImagePath = GetEditDataSourcePath(assetPath);
    CHECK_AND_RETURN_RET_LOG(!sourceImagePath.empty(), E_INVALID_PATH, "Can not get source image path");
    string videoPath = isTemp ? MediaFileUtils::GetTempMovingPhotoVideoPath(assetPath)
        : MediaFileUtils::GetMovingPhotoVideoPath(assetPath);
    CHECK_AND_RETURN_RET_LOG(!videoPath.empty(), E_INVALID_PATH, "Can not get video path");
    string sourceVideoPath = MediaFileUtils::GetMovingPhotoVideoPath(sourceImagePath);
    CHECK_AND_RETURN_RET_LOG(!sourceVideoPath.empty(), E_INVALID_PATH, "Can not get source video path");
    if (!MediaFileUtils::IsFileExists(sourceVideoPath)) {
        CHECK_AND_RETURN_RET_LOG(Move(videoPath, sourceVideoPath) == E_SUCCESS, E_HAS_FS_ERROR,
            "Can not move %{private}s to %{private}s", videoPath.c_str(), sourceVideoPath.c_str());
    }
    return E_OK;
}

int32_t UpdateEffectModeWhenGraffiti(int32_t fileId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR,
        "Failed to get rdbStore when updating effect mode of graffiti");

    MediaLibraryCommand updateCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    updateCmd.GetAbsRdbPredicates()->EqualTo(MediaColumn::MEDIA_ID, to_string(fileId));
    ValuesBucket updateValues;
    int32_t updatedRows = -1;
    updateValues.PutInt(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, static_cast<int32_t>(MovingPhotoEffectMode::DEFAULT));
    updateValues.PutInt(PhotoColumn::PHOTO_ORIGINAL_SUBTYPE, static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    updateCmd.SetValueBucket(updateValues);
    int32_t errCode = rdbStore->Update(updateCmd, updatedRows);
    CHECK_AND_RETURN_RET_LOG(errCode == NativeRdb::E_OK && updatedRows >= 0, E_HAS_DB_ERROR,
        "Failed to update db, errCode:%{public}d, updatedRows:%{public}d", errCode, updatedRows);
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::SubmitEditMovingPhotoExecute(MediaLibraryCommand& cmd,
    const shared_ptr<FileAsset>& fileAsset, bool& isAddGraffiti)
{
    MEDIA_INFO_LOG("Moving photo SubmitEditMovingPhotoExecute begin, fileId:%{public}d", fileAsset->GetId());
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_VALUES, "fileAsset is nullptr");
    string assetPath = fileAsset->GetFilePath();
    CHECK_AND_RETURN_RET_LOG(!assetPath.empty(), E_INVALID_VALUES, "Failed to get asset path");
    int32_t errCode = E_OK;
    if (fileAsset->GetPhotoEditTime() == 0) { // the asset has not been edited before
        // Save video file in the photo direvtory to the .editdata directory
        errCode = SaveSourceVideoFile(assetPath, false);
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
        if (fileAsset->GetMovingPhotoEffectMode() != static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY)) {
            errCode = UpdateMovingPhotoSubtype(fileAsset->GetId(), fileAsset->GetPhotoSubType());
        } else {
            errCode = UpdateEffectModeWhenGraffiti(fileAsset->GetId());
        }
        isAddGraffiti = true;
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
                E_HAS_FS_ERROR, "Move file failed, srcPath:%{private}s, newPath:%{private}s",
                assetVideoPath.c_str(), sourceVideoPath.c_str());
        }
        if (!MediaFileUtils::IsFileExists(sourceImagePath)) {
            CHECK_AND_RETURN_RET_LOG(MediaFileUtils::ModifyAsset(assetPath, sourceImagePath) == E_SUCCESS,
                E_HAS_FS_ERROR, "Move file failed, srcPath:%{private}s, newPath:%{private}s",
                assetPath.c_str(), sourceImagePath.c_str());
        }
    }

    CHECK_AND_RETURN_RET_LOG(Move(imageCachePath, assetPath) == E_OK, E_HAS_FS_ERROR, "Failed to move image");
    CHECK_AND_RETURN_RET_LOG(Move(videoCachePath, assetVideoPath) == E_OK, E_HAS_FS_ERROR, "Failed to move video");
    CHECK_AND_RETURN_RET_LOG(UpdateEffectMode(id, effectMode) == E_OK, errCode, "Failed to update effect mode");
    ScanFile(assetPath, true, true, true);
    MEDIA_INFO_LOG("SubmitEffectModeExecute success.");
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
        PhotoColumn::PHOTO_EDIT_TIME, PhotoColumn::MOVING_PHOTO_EFFECT_MODE, PhotoColumn::PHOTO_OWNER_ALBUM_ID,
        PhotoColumn::PHOTO_EXIST_COMPATIBLE_DUPLICATE, PhotoColumn::PHOTO_ID };
    shared_ptr<FileAsset> fileAsset = GetFileAssetFromDb(
        PhotoColumn::MEDIA_ID, to_string(id), OperationObject::FILESYSTEM_PHOTO, columns);
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_VALUES,
        "Failed to getmapmanagerthread:: FileAsset, fileId=%{public}d", id);
    int32_t errCode = SubmitCacheExecute(cmd, fileAsset, cachePath);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Failed to submit cache, fileId=%{public}d", id);
    return id;
}

int32_t MediaLibraryPhotoOperations::ProcessMultistagesPhoto(const std::shared_ptr<FileAsset> &fileAsset,
    const uint8_t *addr, const long bytes)
{
    if (fileAsset == nullptr) {
        MEDIA_ERR_LOG("fileAsset is nullptr.");
        return E_ERR;
    }
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryPhotoOperations::ProcessMultistagesPhoto");
    std::string path = fileAsset->GetFilePath();
    string editDataSourcePath = GetEditDataSourcePath(path);
    string editDataCameraPath = GetEditDataCameraPath(path);

    bool isEdited = fileAsset->GetPhotoEditTime() > 0;
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
            CHECK_AND_RETURN_RET(ret == E_OK, ret);
            // (2) 生成高质量水印滤镜图片
            string editData;
            CHECK_AND_RETURN_RET_LOG(ReadEditdataFromFile(editDataCameraPath, editData) == E_OK, E_HAS_FS_ERROR,
                "Failed to read editdata, path=%{public}s", editDataCameraPath.c_str());
            const string HIGH_QUALITY_PHOTO_STATUS = "high";
            CHECK_AND_RETURN_RET_LOG(
                AddFiltersToPhoto(editDataSourcePath, path, editData, HIGH_QUALITY_PHOTO_STATUS) == E_OK,
                E_FAIL, "Failed to add filters to photo");
            MediaLibraryObjectUtils::ScanFileAsync(path, to_string(fileAsset->GetId()), MediaLibraryApi::API_10);
            return E_OK;
        }
    }
}

int32_t MediaLibraryPhotoOperations::ProcessMultistagesPhotoForPicture(const std::shared_ptr<FileAsset> &fileAsset,
    std::shared_ptr<Media::Picture> &picture, std::shared_ptr<Media::Picture> &resultPicture, bool &isTakeEffect)
{
    if (fileAsset == nullptr || picture == nullptr) {
        MEDIA_ERR_LOG("fileAsset or picture is nullptr");
        return E_ERR;
    }

    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryPhotoOperations::ProcessMultistagesPhoto");
    std::string path = fileAsset->GetFilePath();
    int32_t fileId = fileAsset->GetId();
    std::string imageId = fileAsset->GetPhotoId();
    std::string mime_type = fileAsset->GetMimeType();
    std::string editDataSourcePath = GetEditDataSourcePath(path);
    std::string editDataCameraPath = GetEditDataCameraPath(path);

    bool isEdited = fileAsset->GetPhotoEditTime() > 0;
    if (isEdited) {
        // 图片编辑过了只替换低质量裸图
        resultPicture = nullptr;
        return FileUtils::SavePicture(editDataSourcePath, picture, mime_type, isEdited);
    } else {
        if (!MediaFileUtils::IsFileExists(editDataCameraPath)) {
            CHECK_AND_PRINT_LOG(EnableYuvAndNotify(fileAsset, picture, isEdited, isTakeEffect,
                imageId, fileId) == E_OK, "Enable YUV failed.");
            // 图片没编辑过且没有editdata_camera，只落盘在Photo目录
            resultPicture = picture;
            return FileUtils::SavePicture(path, picture, mime_type, isEdited);
        } else {
            // 图片没编辑过且有editdata_camera
            MediaLibraryTracer tracer;
            tracer.Start("MediaLibraryPhotoOperations::ProcessMultistagesPhoto AddFiltersToPhoto");
            // (1) 先替换低质量裸图
            int ret = FileUtils::SavePicture(editDataSourcePath, picture, mime_type, isEdited);
            CHECK_AND_RETURN_RET(ret == E_OK, ret);

            // (2) 生成高质量水印滤镜图片
            string editData;
            CHECK_AND_RETURN_RET_LOG(ReadEditdataFromFile(editDataCameraPath, editData) == E_OK, E_HAS_FS_ERROR,
                "Failed to read editdata, path=%{public}s", editDataCameraPath.c_str());
            CHECK_AND_RETURN_RET_LOG(AddFiltersToPicture(picture, path, editData) == E_OK, E_FAIL,
                "Failed to add filters to photo");
            resultPicture = picture;
            isTakeEffect = true;
            CHECK_AND_PRINT_LOG(EnableYuvAndNotify(fileAsset, picture, isEdited, isTakeEffect,
                imageId, fileId) == E_OK, "Enable YUV failed.");
            FileUtils::DealPicture(mime_type, path, picture, true);
            return E_OK;
        }
    }
}

int32_t MediaLibraryPhotoOperations::EnableYuvAndNotify(
    const std::shared_ptr<FileAsset> &fileAsset, std::shared_ptr<Media::Picture> &picture,
    bool isEdited, bool isTakeEffect, const std::string imageId, const int32_t fileId)
{
    if ("" == imageId) {
        return E_ERR;
    }
    MultiStagesPhotoCaptureManager::GetInstance().DealHighQualityPicture(
        imageId, picture, isEdited, isTakeEffect);
    MultistagesCaptureNotify::NotifyOnProcess(fileAsset, MultistagesCaptureNotifyType::YUV_READY);
    auto assetRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>(
        AccurateRefresh::YUV_READY_BUSSINESS_NAME);
    return assetRefresh->NotifyYuvReady(fileId);
}

int32_t MediaLibraryPhotoOperations::AddFiltersToPhoto(const std::string &inputPath,
    const std::string &outputPath, const std::string &editdata, const std::string &photoStatus)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryPhotoOperations::AddFiltersToPhoto");
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} "
        "MultistagesCapture inputPath: %{public}s, outputPath: %{public}s",
        MLOG_TAG, __FUNCTION__, __LINE__,
        MediaFileUtils::DesensitizePath(inputPath).c_str(), MediaFileUtils::DesensitizePath(outputPath).c_str());
    std::string info = editdata;
    size_t lastSlash = outputPath.rfind('/');
    CHECK_AND_RETURN_RET_LOG(lastSlash != string::npos && outputPath.size() > (lastSlash + 1), E_INVALID_VALUES,
        "Failed to check outputPath: %{public}s", outputPath.c_str());
    string tempOutputPath = outputPath.substr(0, lastSlash) +
        "/filters_" + photoStatus + outputPath.substr(lastSlash + 1);
    int32_t ret = MediaFileUtils::CreateAsset(tempOutputPath);
    CHECK_AND_RETURN_RET_LOG(ret == E_SUCCESS || ret == E_FILE_EXIST, E_HAS_FS_ERROR,
        "Failed to create temp filters file %{private}s", tempOutputPath.c_str());
    tracer.Start("MediaChangeEffect::TakeEffect");
    ret = MediaChangeEffect::TakeEffect(inputPath, tempOutputPath, info);
    tracer.Finish();
    if (ret != E_OK) {
        HILOG_COMM_ERROR("%{public}s:{%{public}s:%{public}d} MultistagesCapture, TakeEffect error. ret = %{public}d",
            MLOG_TAG, __FUNCTION__, __LINE__, ret);
        return E_ERR;
    }

    string editDataPath = GetEditDataPath(outputPath);
    if (MediaFileUtils::IsFileExists(editDataPath)) {
        HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} "
            "Editdata path: %{private}s exists, cannot add filters to photo",
            MLOG_TAG, __FUNCTION__, __LINE__, editDataPath.c_str());
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
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} MultistagesCapture finish",
        MLOG_TAG, __FUNCTION__, __LINE__);
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::AddFiltersToPicture(std::shared_ptr<Media::Picture> &inPicture,
    const std::string &outputPath, string &editdata)
{
    (inPicture != nullptr, E_ERR, "AddFiltersToPicture: picture is null");
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} "
        "AddFiltersToPicture outputPath: %{public}s, editdata: %{public}s",
        MLOG_TAG, __FUNCTION__, __LINE__, outputPath.c_str(), editdata.c_str());
    size_t lastSlash = outputPath.rfind('/');
    CHECK_AND_RETURN_RET_LOG(lastSlash != string::npos && outputPath.size() > (lastSlash + 1), E_INVALID_VALUES,
        "Failed to check outputPath: %{public}s", outputPath.c_str());
    int32_t ret = MediaChangeEffect::TakeEffectForPicture(inPicture, editdata);
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::ProcessMultistagesVideo(const std::shared_ptr<FileAsset> &fileAsset)
{
    if (fileAsset == nullptr) {
        MEDIA_ERR_LOG("fileAsset is nullptr.");
        return E_ERR;
    }
    bool isMovingPhoto =
        (fileAsset->GetStageVideoTaskStatus()) == static_cast<int32_t>(StageVideoTaskStatus::STAGE_TASK_DELIVERED);
    bool isEdited = (fileAsset->GetPhotoEditTime()) > 0;
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} "
        "ProcessMultistagesVideo path:%{public}s, isEdited: %{public}d, isMovingPhoto: %{public}d",
        MLOG_TAG, __FUNCTION__, __LINE__,
        DfxUtils::GetSafePath(fileAsset->GetFilePath()).c_str(), isEdited, isMovingPhoto);
    if (isMovingPhoto) {
        bool isMovingPhotoEffectMode = (fileAsset->GetMovingPhotoEffectMode()) > 0;
        return FileUtils::SaveMovingPhotoVideo(fileAsset->GetFilePath(), isEdited, isMovingPhotoEffectMode);
    }
    return FileUtils::SaveVideo(fileAsset->GetFilePath(), isEdited);
}

int32_t MediaLibraryPhotoOperations::RemoveTempVideo(const std::string &path)
{
    MEDIA_INFO_LOG("RemoveTempVideo path: %{public}s", DfxUtils::GetSafePath(path).c_str());
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
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "Medialibrary rdbStore is nullptr!");

    string thumbnailDir {photoPath};
    CHECK_AND_RETURN_LOG(ConvertPhotoPathToThumbnailDirPath(thumbnailDir),
        "Failed to get thumbnail dir path from photo path! file id: %{public}s", photoId.c_str());
    uint64_t photoThumbnailSize = GetFolderSize(thumbnailDir);
    string sql = "INSERT INTO " + PhotoExtColumn::PHOTOS_EXT_TABLE + " (" +
        PhotoExtColumn::PHOTO_ID + ", " + PhotoExtColumn::THUMBNAIL_SIZE +
        ") VALUES (" + photoId + ", " + to_string(photoThumbnailSize) + ")" +
        " ON CONFLICT(" + PhotoExtColumn::PHOTO_ID + ")" + " DO UPDATE SET " +
        PhotoExtColumn::THUMBNAIL_SIZE + " = " + to_string(photoThumbnailSize);
    int32_t ret = rdbStore->ExecuteSql(sql);
    CHECK_AND_RETURN_LOG(ret == NativeRdb::E_OK,
        "Failed to execute sql, photoId is %{public}s, error code is %{public}d", photoId.c_str(), ret);
}

bool MediaLibraryPhotoOperations::HasDroppedThumbnailSize(const string& photoId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false,
        "Medialibrary rdbStore is nullptr!");

    string sql = "UPDATE " + PhotoExtColumn::PHOTOS_EXT_TABLE + " SET " + PhotoExtColumn::THUMBNAIL_SIZE +
        " = 0 WHERE " + PhotoExtColumn::PHOTO_ID + " = " + photoId + ";";
    int32_t ret = rdbStore->ExecuteSql(sql);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, false,
        "Failed to execute sql, photoId is %{public}s, error code is %{public}d", photoId.c_str(), ret);
    return true;
}

bool DropThumbnailSize(const vector<string>& photoIds)
{
    if (photoIds.size() == 0) {
        return true;
    }

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "Medialibrary rdbStore is nullptr!");
    NativeRdb::RdbPredicates rdbPredicates(PhotoExtColumn::PHOTOS_EXT_TABLE);
    rdbPredicates.In(PhotoExtColumn::PHOTO_ID, photoIds);
    int32_t deletedRows = -1;
    auto ret = rdbStore->Delete(deletedRows, rdbPredicates);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK && deletedRows >= 0, false,
        "Delete thumbnail size failed, errCode = %{public}d, deletedRows = %{public}d",
        ret, deletedRows);
    MEDIA_INFO_LOG("Delete %{public}d rows in tab_photos_ext", deletedRows);
    return true;
}

bool MediaLibraryPhotoOperations::BatchDropThumbnailSize(const vector<string>& photoIds)
{
    if (photoIds.size() == 0) {
        return true;
    }

    constexpr int32_t ONE_BATCH_SIZE = 500;
    bool batchDropSuccess = true;
    for (size_t i = 0; i < photoIds.size(); i += ONE_BATCH_SIZE) {
        size_t endIndex = std::min(i + ONE_BATCH_SIZE, photoIds.size());
        vector<string> batchPhotoIds(photoIds.begin() + i, photoIds.begin() + endIndex);
        batchDropSuccess = DropThumbnailSize(batchPhotoIds) && batchDropSuccess;
    }
    return batchDropSuccess;
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
    MediaLibraryObjectUtils::ScanFileAsync(path, fileId, MediaLibraryApi::API_10, true);
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
    CHECK_AND_RETURN_RET(GetStringFromValuesBucket(values, MEDIA_DATA_DB_URI, uriString), E_INVALID_VALUES);

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
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_VALUES, "failed to query fileAsset");
    if (fileAsset->GetPhotoSubType() != static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)) {
        MEDIA_INFO_LOG("fileAsset is not moving photo");
        return E_OK;
    }
    if (fileAsset->GetPhotoEditTime() > 0) {
        MEDIA_INFO_LOG("moving photo is edited");
        return E_OK;
    }
    string videoPath = MediaFileUtils::GetTempMovingPhotoVideoPath(fileAsset->GetFilePath());
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
    CHECK_AND_RETURN_RET_LOG(updatedRows > 0, updatedRows,
        "Failed to update subtype, updatedRows=%{public}d", updatedRows);
    UpdateDirty(fileAsset->GetId());

    string extraUri = MediaFileUtils::GetExtraUri(fileAsset->GetDisplayName(), fileAsset->GetFilePath());
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_RET_LOG(watch != nullptr, E_ERR, "Can not get MediaLibraryNotify Instance");
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
    AccurateRefresh::AssetAccurateRefresh assetRefresh(AccurateRefresh::UPDATE_OWNER_ALBUMID_BUSSINESS_NAME);
    int32_t rowId = -1;
    assetRefresh.Update(cmd, rowId);
    CHECK_AND_RETURN_RET_LOG(rowId >= 0, rowId, "Update Photo In database failed, rowId=%{public}d", rowId);

    assetRefresh.RefreshAlbum();
    assetRefresh.Notify();
    return rowId;
}

int32_t MediaLibraryPhotoOperations::ProcessCustomRestore(MediaLibraryCommand& cmd)
{
    const ValuesBucket& values = cmd.GetValueBucket();
    string albumLpath;
    string keyPath;
    string isDeduplication;
    string bundleName;
    string appName;
    string appId;
    string dbPath;
    CHECK_AND_RETURN_RET_LOG(GetStringFromValuesBucket(values, "albumLpath", albumLpath),
        E_INVALID_VALUES, "Failed to get albumLpath: %{public}s", albumLpath.c_str());
    CHECK_AND_RETURN_RET_LOG(GetStringFromValuesBucket(values, "keyPath", keyPath),
        E_INVALID_VALUES, "Failed to get keyPath: %{public}s", keyPath.c_str());
    string dir = CUSTOM_RESTORE_DIR + "/" + keyPath;
    CHECK_AND_RETURN_RET_LOG(
        MediaFileUtils::IsFileExists(dir), E_NO_SUCH_FILE, "sourceDir: %{public}s does not exist!", dir.c_str());
    CHECK_AND_RETURN_RET_LOG(GetStringFromValuesBucket(values, "isDeduplication", isDeduplication),
        E_INVALID_VALUES, "Failed to get isDeduplication: %{public}s", isDeduplication.c_str());
    CHECK_AND_RETURN_RET_LOG(GetStringFromValuesBucket(values, "bundleName", bundleName),
        E_INVALID_VALUES, "Failed to get bundleName: %{public}s", bundleName.c_str());
    CHECK_AND_RETURN_RET_LOG(GetStringFromValuesBucket(values, "appName", appName),
        E_INVALID_VALUES, "Failed to get appName: %{public}s", appName.c_str());
    GetStringFromValuesBucket(values, "appId", appId);
    GetStringFromValuesBucket(values, "dbPath", dbPath);

    RestoreTaskInfo restoreTaskInfo = {.dbPath = dbPath,
        .albumLpath = albumLpath,
        .keyPath = keyPath,
        .isDeduplication = isDeduplication == "true",
        .bundleName = bundleName,
        .packageName = appName,
        .appId = appId,
        .sourceDir = dir};
    PhotoCustomRestoreOperation::GetInstance().AddTask(restoreTaskInfo).Start();
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::CancelCustomRestore(MediaLibraryCommand& cmd)
{
    const ValuesBucket& values = cmd.GetValueBucket();
    string keyPath;
    CHECK_AND_RETURN_RET_LOG(GetStringFromValuesBucket(values, "keyPath", keyPath),
        E_INVALID_VALUES, "Failed to get keyPath: %{public}s", keyPath.c_str());
    RestoreTaskInfo restoreTaskInfo = {.keyPath = keyPath};
    PhotoCustomRestoreOperation::GetInstance().CancelTask(restoreTaskInfo);
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::UpdateSupportedWatermarkType(MediaLibraryCommand &cmd)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "Failed to get rdbStore.");
    auto whereClause = cmd.GetAbsRdbPredicates()->GetWhereClause();
    auto args = cmd.GetAbsRdbPredicates()->GetWhereArgs();
    int32_t updateRows = -1;
    int32_t errCode = rdbStore->Update(updateRows, PhotoColumn::PHOTOS_TABLE, cmd.GetValueBucket(), whereClause, args);
    CHECK_AND_RETURN_RET_LOG(errCode == NativeRdb::E_OK, E_HAS_DB_ERROR,
        "Update subtype field failed. errCode:%{public}d,", errCode);
    return updateRows;
}

int32_t MediaLibraryPhotoOperations::UpdateAppLink(MediaLibraryCommand &cmd)
{
    auto rdbstore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbstore != nullptr, E_HAS_DB_ERROR, "Failed to get rdbStore.");
    auto whereClause = cmd.GetAbsRdbPredicates()->GetWhereClause();
    auto args = cmd.GetAbsRdbPredicates()->GetWhereArgs();
    int32_t updateRows = -1;
    int32_t errCode = rdbstore->Update(updateRows, PhotoColumn::PHOTOS_TABLE, cmd.GetValueBucket(), whereClause, args);
    CHECK_AND_RETURN_RET_LOG(errCode == NativeRdb::E_OK, E_HAS_DB_ERROR,
        "Update subtype field failed. errCode:%{public}d.", errCode);
    return E_OK;
}

struct LSOperationFileInfo {
    std::string permissions;
    int links;
    std::string owner;
    std::string group;
    long size;
    std::string modTime;
    std::string fileName;
};

static std::string GetFilePermissions(struct stat& fileStat)
{
    std::ostringstream ss;
    ss << (S_ISDIR(fileStat.st_mode) ? 'd' : '-');
    ss << ((fileStat.st_mode & S_IRUSR) ? 'r' : '-');
    ss << ((fileStat.st_mode & S_IWUSR) ? 'w' : '-');
    ss << ((fileStat.st_mode & S_IXUSR) ? 'x' : '-');
    ss << ((fileStat.st_mode & S_IRGRP) ? 'r' : '-');
    ss << ((fileStat.st_mode & S_IWGRP) ? 'w' : '-');
    ss << ((fileStat.st_mode & S_IXGRP) ? 'x' : '-');
    ss << ((fileStat.st_mode & S_IROTH) ? 'r' : '-');
    ss << ((fileStat.st_mode & S_IWOTH) ? 'w' : '-');
    ss << ((fileStat.st_mode & S_IXOTH) ? 'x' : '-');
    return ss.str();
}

static void GetFileOwnerAndGroup(struct stat& fileStat, std::string& owner, std::string& group)
{
    struct passwd pwbuf;
    struct group grbuf;
    char pwbuffer[1024];
    char grbuffer[1024];
    struct passwd* pw = nullptr;
    struct group* gr = nullptr;
    getpwuid_r(fileStat.st_uid, &pwbuf, pwbuffer, sizeof(pwbuffer), &pw);
    getgrgid_r(fileStat.st_gid, &grbuf, grbuffer, sizeof(grbuffer), &gr);
    owner = pw ? pw->pw_name : std::to_string(fileStat.st_uid);
    group = gr ? gr->gr_name : std::to_string(fileStat.st_gid);
}

static std::string GetFileModificationTime(struct stat& fileStat)
{
    char modTime[20];
    struct tm* timeInfo = localtime(&fileStat.st_mtime);
    if (timeInfo == nullptr) {
        return "Error";
    }
    strftime(modTime, sizeof(modTime), "%Y-%m-%d %H:%M", timeInfo);
    return std::string(modTime);
}

static bool QueryHiddenFilesList(set<string>& hiddenFiles)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "rdbStore is nullptr!");
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_HIDDEN, 1);
    vector<string> columns;
    columns.push_back(MediaColumn::MEDIA_FILE_PATH);
    columns.push_back(PhotoColumn::PHOTO_SUBTYPE);
    shared_ptr<NativeRdb::ResultSet> result = rdbStore->QueryByStep(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(result != nullptr, false, "Query hidden files failed!");
    hiddenFiles.clear();
    while (result->GoToNextRow() == NativeRdb::E_OK) {
        string filePath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, result);
        string fileName = MediaFileUtils::GetFileName(filePath);
        hiddenFiles.insert(fileName);
        int photoSubtype = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, result);
        if (photoSubtype == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)) {
            string movingPhotoVideoPath = MediaFileUtils::GetMovingPhotoVideoPath(filePath);
            string movingPhotoVideoName = MediaFileUtils::GetFileName(movingPhotoVideoPath);
            hiddenFiles.insert(movingPhotoVideoName);
        }
    }

    return true;
}

static int32_t ProcessFile(const string& filePath, const string fileName,
    std::vector<LSOperationFileInfo>& fileInfoList, bool excludeHiddenFiles, std::set<std::string>& hiddenFiles)
{
    struct stat fileStat;

    if (stat(filePath.c_str(), &fileStat) == -1) {
        MEDIA_ERR_LOG("stat failed. File path: %{public}s, err: %{public}s", filePath.c_str(), strerror(errno));
        return E_FAIL;
    }

    if (std::string(fileName) == "." || std::string(fileName) == "..") {
        return E_OK;
    }

    if (excludeHiddenFiles && hiddenFiles.find(fileName) != hiddenFiles.end()) {
        return E_OK;
    }

    LSOperationFileInfo fileInfo;
    fileInfo.permissions = GetFilePermissions(fileStat);
    fileInfo.links = fileStat.st_nlink;
    GetFileOwnerAndGroup(fileStat, fileInfo.owner, fileInfo.group);
    fileInfo.size = fileStat.st_size;
    fileInfo.modTime = GetFileModificationTime(fileStat);
    fileInfo.fileName = fileName;

    fileInfoList.push_back(fileInfo);

    return E_OK;
}

static int32_t ListPhotoPath(const std::string& path, std::vector<LSOperationFileInfo>& fileInfoList)
{
    bool excludeHiddenFiles = PermissionUtils::IsRootShell() ? false : true;
    set<string> hiddenFiles;
    if (excludeHiddenFiles && !QueryHiddenFilesList(hiddenFiles)) {
        MEDIA_ERR_LOG("Query hidden files failed. dir path: %{public}s", path.c_str());
        return E_FAIL;
    }

    struct stat pathStat;
    if (stat(path.c_str(), &pathStat) == -1) {
        MEDIA_ERR_LOG("stat failed. Path: %{public}s, err: %{public}s", path.c_str(), strerror(errno));
        return E_INVALID_PATH;
    }

    if (S_ISREG(pathStat.st_mode)) {
        std::string fileName = MediaFileUtils::GetFileName(path);
        return ProcessFile(path, fileName, fileInfoList, excludeHiddenFiles, hiddenFiles);
    }

    DIR* dp = opendir(path.c_str());
    if (dp == nullptr) {
        MEDIA_ERR_LOG("opendir failed. Dir path: %{public}s, err: %{public}s", path.c_str(), strerror(errno));
        return E_INVALID_PATH;
    }

    struct dirent* entry;
    while ((entry = readdir(dp)) != nullptr) {
        ProcessFile(path + "/" + string(entry->d_name), string(entry->d_name),
            fileInfoList, excludeHiddenFiles, hiddenFiles);
    }

    closedir(dp);
    return E_OK;
}

static string BuildLSResult(const std::vector<LSOperationFileInfo>& fileInfoList)
{
    nlohmann::json result;

    for (const auto& file : fileInfoList) {
        nlohmann::json fileJson;
        fileJson["permissions"] = file.permissions;
        fileJson["links"] = file.links;
        fileJson["owner"] = file.owner;
        fileJson["group"] = file.group;
        fileJson["size"] = file.size;
        fileJson["modTime"] = file.modTime;
        fileJson["fileName"] = file.fileName;

        result["files"].push_back(fileJson);
    }

    return result.dump();
}

int32_t MediaLibraryPhotoOperations::LSMediaFiles(MediaLibraryCommand& cmd)
{
    const ValuesBucket& values = cmd.GetValueBucket();
    string dirPath;
    CHECK_AND_RETURN_RET_LOG(GetStringFromValuesBucket(values, MediaColumn::MEDIA_FILE_PATH, dirPath),
        E_INVALID_VALUES, "Failed to get dirPath value");
    string realPath;
    CHECK_AND_RETURN_RET_LOG(PathToRealPath(dirPath, realPath),
        E_INVALID_PATH, "real path failed: %{public}s, errno: %{public}d", dirPath.c_str(), errno);
    if (!MediaFileUtils::StartsWith(realPath, "/storage/media/local/files/Photo")) {
        MEDIA_ERR_LOG("dirPath: %{public}s is not under local photo directory", dirPath.c_str());
        return E_INVALID_PATH;
    }

    std::vector<LSOperationFileInfo> fileInfoList;
    int32_t ret = ListPhotoPath(dirPath, fileInfoList);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to list photo directory, dirPath: %{public}s", dirPath.c_str());
    cmd.SetResult(BuildLSResult(fileInfoList));
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::WriteEditedDataToTlv(const std::string &assetPath, TlvFile tlv)
{
    MEDIA_INFO_LOG("WriteEditedDataToTlv start");
    CHECK_AND_RETURN_RET_LOG(!assetPath.empty(), E_ERR, "assetPath is empty.");
    std::string editedDataPath = MediaLibraryAssetOperations::GetEditDataPath(assetPath);
    if (MediaFileUtils::IsFileExists(editedDataPath)) {
        MEDIA_INFO_LOG("Edited data file exist");
        int32_t ret = TlvUtil::WriteEditDataToTlv(tlv, editedDataPath);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Write edited data to tlv failed for editedDataPath %{public}s",
            DfxUtils::GetSafePath(editedDataPath).c_str());
    }
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::WriteEditedDataCameraToTlv(const std::string &assetPath, TlvFile tlv)
{
    MEDIA_INFO_LOG("WriteEditedDataCameraToTlv start");
    CHECK_AND_RETURN_RET_LOG(!assetPath.empty(), E_ERR, "assetPath is empty.");
    std::string editedDataPath = MediaLibraryAssetOperations::GetEditDataCameraPath(assetPath);
    if (MediaFileUtils::IsFileExists(editedDataPath)) {
        MEDIA_INFO_LOG("Edited data camera file exist");
        int32_t ret = TlvUtil::WriteCameraDataToTlv(tlv, editedDataPath);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret,
            "Write edited data to tlv failed for editedDataCameraPath %{public}s",
            DfxUtils::GetSafePath(editedDataPath).c_str());
    }
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::OpenAssetCompress(MediaLibraryCommand &cmd, std::string &tlvPath,
    const int32_t &version, int32_t &fd)
{
    MEDIA_INFO_LOG("OpenAssetCompress start");
    string uriString = cmd.GetUriStringWithoutSegment();
    string id = MediaFileUtils::GetIdFromUri(uriString);
    CHECK_AND_RETURN_RET(!uriString.empty() && MediaLibraryDataManagerUtils::IsNumber(id), E_INVALID_URI);
    string pendingStatus = cmd.GetQuerySetParam(MediaColumn::MEDIA_TIME_PENDING);

    int32_t compressVersion = AssetCompressVersionManager::GetCompatibleCompressVersion(version);
    CHECK_AND_RETURN_RET_LOG(compressVersion != 0, E_INVALID_VALUES, "Invalid compress version: %{public}d", version);
    AssetCompressSpec compressSpec = AssetCompressVersionManager::GetAssetCompressSpec(compressVersion);
    std::vector<std::string> queryColumns = PHOTO_COLUMN_VECTOR;
    queryColumns.push_back(MediaColumn::MEDIA_ID);
    queryColumns.insert(queryColumns.end(), compressSpec.editedDataColumns.begin(),
        compressSpec.editedDataColumns.end());
    string cmdUri = cmd.GetUri().ToString();
    size_t pos = cmdUri.find(MULTI_USER_URI_FLAG);
    string userId = "";
    if (pos != std::string::npos) {
        userId = cmdUri.substr(pos + MULTI_USER_URI_FLAG.length());
        uriString = uriString + "?" + MULTI_USER_URI_FLAG + userId;
    }
    shared_ptr<FileAsset> fileAsset = GetFileAssetByUri(uriString, true, queryColumns, pendingStatus);
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_URI, "Get FileAsset From Uri Failed");
    CHECK_AND_RETURN_RET_LOG(fileAsset->GetMediaType() != MediaType::MEDIA_TYPE_VIDEO, E_INVALID_ARGUMENTS,
        "video type not support compress open");
    CHECK_AND_RETURN_RET_LOG(CheckPermissionToOpenFileAsset(fileAsset), E_PERMISSION_DENIED, "Open not allowed");

    UpdateLastVisitTime(cmd, id);

    int32_t ret = MediaLibraryPhotoOperations::HandleOpenAssetCompress(fileAsset, compressSpec, tlvPath, cmd, fd);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Handle open asset compress failed, ret: %{public}d", ret);
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::HandleOpenAssetCompress(const shared_ptr<FileAsset> &fileAsset,
    const AssetCompressSpec &compressSpec, std::string &tlvPath, MediaLibraryCommand &cmd, int32_t &fd)
{
    MEDIA_INFO_LOG("HandleOpenAssetCompress start");
    int32_t ret = E_OK;
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_VALUES, "fileAsset is nullptr");
    string assetPath = fileAsset->GetPath();
    tlvPath = MediaLibraryAssetOperations::GetAssetCompressCachePath(to_string(fileAsset->GetId()));
    MEDIA_DEBUG_LOG("HandleOpenAssetCompress tlvPath: %{public}s", DfxUtils::GetSafePath(tlvPath).c_str());
    CHECK_AND_RETURN_RET_LOG(!tlvPath.empty(), E_ERR, "Get tlv path failed for compress asset");
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    TlvFile tlv = TlvUtil::CreateTlvFile(tlvPath);
    UniqueFd tlvFdGuard(tlv);
    CHECK_AND_RETURN_RET_LOG(tlvFdGuard.Get() >= 0, E_ERR, "Create tlv file failed");
    bool isMovingPhoto = MovingPhotoFileUtils::IsMovingPhoto(fileAsset->GetPhotoSubType(),
        fileAsset->GetMovingPhotoEffectMode(), fileAsset->GetOriginalSubType());
    if (isMovingPhoto) {
        ret = MediaLibraryPhotoOperations::HandleMovingPhotoAsset(fileAsset, cmd, tlvFdGuard.Get());
    } else {
        ret = MediaLibraryPhotoOperations::HandleNormalPhotoAsset(fileAsset, cmd, tlvFdGuard.Get());
    }
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Handle asset failed");
    ret = MediaLibraryPhotoOperations::HandlePhotoEditData(fileAsset, compressSpec, cmd, tlvFdGuard.Get());
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Handle photo edit data failed");
    if (isMovingPhoto) {
        ret = MediaLibraryPhotoOperations::HandleMovingPhotoEditData(fileAsset, compressSpec, cmd, tlvFdGuard.Get());
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Handle moving photo edit data failed");
    }

    ret = TlvUtil::UpdateTlvHeadSize(tlvFdGuard.Get());
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Update tlv head size failed");
    int32_t tlvFd = open(tlvPath.c_str(), O_RDONLY);
    CHECK_AND_RETURN_RET_LOG(tlvFd >= 0, E_ERR, "Open tlv file failed, errno: %{public}d", errno);
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("Handle open tlv file success, time cost: %{public}ld ms", static_cast<long>(endTime - startTime));
    // ipc close fd after write to parcel
    fd = tlvFd;
    return ret;
}

int32_t MediaLibraryPhotoOperations::HandleMovingPhotoAsset(const shared_ptr<FileAsset> &fileAsset,
    MediaLibraryCommand &cmd, TlvFile tlv)
{
    MEDIA_INFO_LOG("HandleMovingPhotoAsset start");
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_VALUES, "fileAsset is nullptr");
    string assetPath = fileAsset->GetPath();
    int64_t movingPhotoSize = static_cast<int64_t>(MovingPhotoFileUtils::GetMovingPhotoSize(fileAsset->GetPath()));
    if (fileAsset->GetSize() != movingPhotoSize) {
        MEDIA_WARN_LOG("size of moving photo need scan from %{public}ld to %{public}ld",
            static_cast<long>(fileAsset->GetSize()), static_cast<long>(movingPhotoSize));
        MediaLibraryAssetOperations::ScanFileWithoutAlbumUpdate(fileAsset->GetPath(), false, false, true);
    }
    int32_t ret = E_OK;
    ret = MediaLibraryPhotoOperations::HandleNormalPhotoAsset(fileAsset, cmd, tlv);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Handle moving photo image failed");
    ret = MediaLibraryPhotoOperations::HandleMovingPhotoVideoFile(fileAsset, cmd, tlv);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Handle moving photo video file failed");
    ret = MediaLibraryPhotoOperations::HandleMovingPhotoExtraData(fileAsset, cmd, tlv);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Handle moving photo extra data failed");
    return ret;
}

int32_t MediaLibraryPhotoOperations::HandleMovingPhotoEditData(const shared_ptr<FileAsset> &fileAsset,
    const AssetCompressSpec &compressSpec, MediaLibraryCommand &cmd, TlvFile tlv)
{
    MEDIA_INFO_LOG("HandleMovingPhotoEditData start");
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_VALUES, "fileAsset is nullptr");
    string assetPath = fileAsset->GetPath();
    int32_t ret = E_OK;
    for (const auto &fileType : compressSpec.editedDataFiles) {
        switch (fileType) {
            case EditedDataType::EDIT_DATA_SOURCE_BACK:
                ret = MediaLibraryPhotoOperations::HandleOpenMovingPhotoVideoSourceBackFile(fileAsset, cmd, tlv);
                break;
            case EditedDataType::EDIT_DATA_SOURCE:
                ret = MediaLibraryPhotoOperations::HandleOpenMovingPhotoVideoSourceFile(fileAsset, cmd, tlv);
                break;
            default:
                MEDIA_DEBUG_LOG("no need to process, type: %{public}d", static_cast<int32_t>(fileType));
                break;
        }
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret,
            "Handle moving photo edited data file failed, type %{public}d", static_cast<int32_t>(fileType));
    }
    return ret;
}

int32_t MediaLibraryPhotoOperations::HandleMovingPhotoVideoFile(const shared_ptr<FileAsset> &fileAsset,
    MediaLibraryCommand &cmd, TlvFile tlv)
{
    MEDIA_INFO_LOG("HandleMovingPhotoVideoFile start");
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_VALUES, "fileAsset is nullptr");
    string assetPath = fileAsset->GetPath();
    string videoPath = MovingPhotoFileUtils::GetMovingPhotoVideoPath(assetPath);
    int32_t ret = E_OK;
    if (MediaFileUtils::IsFileExists(videoPath)) {
        MEDIA_INFO_LOG("Moving photo video file exist");
        fileAsset->SetPath(videoPath);
        int32_t videoFd = MediaLibraryPhotoOperations::HandleOpenAsset(fileAsset, false, cmd);
        UniqueFd videoFdGuard(videoFd);
        CHECK_AND_RETURN_RET_LOG(videoFdGuard.Get() >= 0, E_ERR, "Open moving photo video file failed");
        ret = TlvUtil::WriteMovingPhotoVideoFileToTlv(tlv, videoFdGuard.Get());
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Write moving photo video file to tlv failed");
        fileAsset->SetPath(assetPath);
    }
    return ret;
}

int32_t MediaLibraryPhotoOperations::HandleMovingPhotoExtraData(const shared_ptr<FileAsset> &fileAsset,
    MediaLibraryCommand &cmd, TlvFile tlv)
{
    MEDIA_INFO_LOG("HandleMovingPhotoExtraData start");
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_VALUES, "fileAsset is nullptr");
    string assetPath = fileAsset->GetPath();
    string extraDataPath = MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(assetPath);
    int32_t ret = E_OK;
    if (MediaFileUtils::IsFileExists(extraDataPath)) {
        MEDIA_INFO_LOG("Moving photo extra file exist");
        ret = TlvUtil::WriteExtraDataFileToTlv(tlv, extraDataPath);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Write moving photo extra file to tlv failed");
    }
    return ret;
}

int32_t MediaLibraryPhotoOperations::HandlePhotoEditData(const shared_ptr<FileAsset> &fileAsset,
    const AssetCompressSpec &compressSpec, MediaLibraryCommand &cmd, TlvFile tlv)
{
    MEDIA_INFO_LOG("HandlePhotoEditData start");
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_VALUES, "fileAsset is nullptr");
    string assetPath = fileAsset->GetPath();
    int32_t ret = E_OK;
    for (const auto &fileType : compressSpec.editedDataFiles) {
        switch (fileType) {
            case EditedDataType::EDIT_DATA:
                ret = MediaLibraryPhotoOperations::WriteEditedDataToTlv(assetPath, tlv);
                break;
            case EditedDataType::EDIT_DATA_CAMERA:
                ret = MediaLibraryPhotoOperations::WriteEditedDataCameraToTlv(assetPath, tlv);
                break;
            case EditedDataType::EDIT_DATA_SOURCE_BACK:
                ret = MediaLibraryPhotoOperations::HandleOpenSourceBackFile(fileAsset, cmd, tlv);
                break;
            case EditedDataType::EDIT_DATA_SOURCE:
                ret = MediaLibraryPhotoOperations::HandleOpenSourceFile(fileAsset, cmd, tlv);
                break;
            case EditedDataType::EDIT_DATA_DB_JSON:
                ret = MediaLibraryPhotoOperations::HandleJsonFile(fileAsset, compressSpec.editedDataColumns, tlv);
                break;
            default:
                MEDIA_ERR_LOG("Unsupported edited data file type: %{public}d", static_cast<int32_t>(fileType));
                break;
        }
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret,
            "Handle edited data file type %{public}d failed", static_cast<int32_t>(fileType));
    }
    return ret;
}

int32_t MediaLibraryPhotoOperations::HandleOpenAsset(const shared_ptr<FileAsset> &fileAsset, bool isMovingPhoto,
    MediaLibraryCommand &cmd)
{
    MEDIA_INFO_LOG("HandleOpenAsset start");
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_VALUES, "fileAsset is nullptr");
    int32_t ret = E_ERR;
    string cmdUri = cmd.GetUri().ToString();
    int32_t type = -1;
    GetType(cmdUri, type);
    if (cmdUri.find(PhotoColumn::PHOTO_URI_PREFIX) != string::npos) {
        ret = OpenAsset(fileAsset, MEDIA_FILEMODE_READONLY, MediaLibraryApi::API_10, isMovingPhoto, type);
    } else {
        ret = OpenAsset(fileAsset, MEDIA_FILEMODE_READONLY, cmd.GetApi(), type);
    }
    return ret;
}

int32_t MediaLibraryPhotoOperations::HandleNormalPhotoAsset(const shared_ptr<FileAsset> &fileAsset,
    MediaLibraryCommand &cmd, TlvFile tlv)
{
    MEDIA_INFO_LOG("HandleNormalPhotoAsset start");
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_VALUES, "fileAsset is nullptr");
    string assetPath = fileAsset->GetPath();
    string realPath = LakeFileUtils::GetAssetRealPath(assetPath);
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsFileExists(realPath), E_ERR, "assetPath is not exist.");
    fileAsset->SetPath(realPath);
    MEDIA_INFO_LOG("Asset file exist");
    int32_t assetFd = MediaLibraryPhotoOperations::HandleOpenAsset(fileAsset, false, cmd);
    UniqueFd assetFdGuard(assetFd);
    CHECK_AND_RETURN_RET_LOG(assetFdGuard.Get() >= 0, E_ERR, "Open asset file failed");
    int32_t ret = TlvUtil::WriteOriginFileToTlv(tlv, realPath, assetFdGuard.Get());
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Write asset file to tlv failed");
    fileAsset->SetPath(assetPath);
    return ret;
}

int32_t MediaLibraryPhotoOperations::HandleOpenSourceFile(const shared_ptr<FileAsset> &fileAsset,
    MediaLibraryCommand &cmd, TlvFile tlv)
{
    MEDIA_INFO_LOG("HandleOpenSourceFile start");
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_VALUES, "fileAsset is nullptr");
    string assetPath = fileAsset->GetPath();
    string sourcePath = GetEditDataSourcePath(assetPath);
    int32_t ret = E_OK;
    if (MediaFileUtils::IsFileExists(sourcePath)) {
        MEDIA_INFO_LOG("source file exist");
        fileAsset->SetPath(sourcePath);
        int32_t sourceFd = MediaLibraryPhotoOperations::HandleOpenAsset(fileAsset, false, cmd);
        UniqueFd sourceFdGuard(sourceFd);
        CHECK_AND_RETURN_RET_LOG(sourceFdGuard.Get() > 0, E_ERR, "Open source file failed");
        ret = TlvUtil::WriteSourceFileToTlv(tlv, sourceFdGuard.Get());
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Write source file to tlv failed");
        fileAsset->SetPath(assetPath);
    }
    return ret;
}

int32_t MediaLibraryPhotoOperations::HandleOpenMovingPhotoVideoSourceFile(const shared_ptr<FileAsset> &fileAsset,
    MediaLibraryCommand &cmd, TlvFile tlv)
{
    MEDIA_INFO_LOG("HandleOpenMovingPhotoVideoSourceFile start");
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_VALUES, "fileAsset is nullptr");
    int32_t ret = E_OK;
    string assetPath = fileAsset->GetPath();
    string videoSourcePath = MovingPhotoFileUtils::GetSourceMovingPhotoVideoPath(assetPath);
    if (MediaFileUtils::IsFileExists(videoSourcePath)) {
        MEDIA_INFO_LOG("moving photo video source file exist");
        fileAsset->SetPath(videoSourcePath);
        int32_t videoSourceFd = MediaLibraryPhotoOperations::HandleOpenAsset(fileAsset, false, cmd);
        UniqueFd videoSourceFdGuard(videoSourceFd);
        CHECK_AND_RETURN_RET_LOG(videoSourceFdGuard.Get() >= 0, E_ERR, "Open moving photo video source file failed");
        ret = TlvUtil::WriteMovingPhotoVideoSourceFileToTlv(tlv, videoSourceFdGuard.Get());
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Write moving photo video source file to tlv failed");
        fileAsset->SetPath(assetPath);
    }
    return ret;
}

int32_t MediaLibraryPhotoOperations::HandleOpenSourceBackFile(const shared_ptr<FileAsset> &fileAsset,
    MediaLibraryCommand &cmd, TlvFile tlv)
{
    MEDIA_INFO_LOG("HandleOpenSourceBackFile start");
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_VALUES, "fileAsset is nullptr");
    string assetPath = fileAsset->GetPath();
    string sourceBackPath = GetEditDataSourceBackPath(assetPath);
    int32_t ret = E_OK;
    if (MediaFileUtils::IsFileExists(sourceBackPath)) {
        MEDIA_INFO_LOG("source back file exist");
        fileAsset->SetPath(sourceBackPath);
        int32_t sourceBackFd = MediaLibraryPhotoOperations::HandleOpenAsset(fileAsset, false, cmd);
        UniqueFd sourceBackFdGuard(sourceBackFd);
        CHECK_AND_RETURN_RET_LOG(sourceBackFdGuard.Get() >= 0, E_ERR, "Open source file failed");
        ret = TlvUtil::WriteSourceBackFileToTlv(tlv, sourceBackFdGuard.Get());
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Write source back file to tlv failed");
        fileAsset->SetPath(assetPath);
    }
    return ret;
}

int32_t MediaLibraryPhotoOperations::HandleOpenMovingPhotoVideoSourceBackFile(const shared_ptr<FileAsset> &fileAsset,
    MediaLibraryCommand &cmd, TlvFile tlv)
{
    MEDIA_INFO_LOG("HandleOpenMovingPhotoVideoSourceBackFile start");
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_VALUES, "fileAsset is nullptr");
    string assetPath = fileAsset->GetPath();
    int32_t ret = E_OK;
    string videoSourceBackPath = MovingPhotoFileUtils::GetSourceBackMovingPhotoVideoPath(assetPath);
    if (MediaFileUtils::IsFileExists(videoSourceBackPath)) {
        MEDIA_INFO_LOG("moving photo video source back file exist");
        fileAsset->SetPath(videoSourceBackPath);
        int32_t videoSourceBackFd = MediaLibraryPhotoOperations::HandleOpenAsset(fileAsset, false, cmd);
        UniqueFd videoSourceFdGuard(videoSourceBackFd);
        CHECK_AND_RETURN_RET_LOG(videoSourceFdGuard.Get() >= 0, E_ERR,
            "Open moving photo video source back file failed");
        ret = TlvUtil::WriteMovingPhotoVideoSourceBackFileToTlv(tlv, videoSourceFdGuard.Get());
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Write moving photo video source back file to tlv failed");
        fileAsset->SetPath(assetPath);
    }
    return ret;
}

int32_t MediaLibraryPhotoOperations::ProcessPhotoSubTypeForShare(const shared_ptr<FileAsset> &fileAsset)
{
    MEDIA_DEBUG_LOG("ProcessPhotoSubTypeForShare start");
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_VALUES, "fileAsset is nullptr");
    // not support burst photo
    int32_t subtype = fileAsset->GetPhotoSubType();
    if (subtype == static_cast<int32_t>(PhotoSubType::BURST)) {
        fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::DEFAULT));
    }
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::HandleJsonFile(const shared_ptr<FileAsset> &fileAsset,
    const EditedDataColumn &editedDataColumns, TlvFile tlv)
{
    MEDIA_INFO_LOG("HandleJsonFile start");
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_VALUES, "fileAsset is nullptr");
    CHECK_AND_RETURN_RET_LOG(!editedDataColumns.empty(), E_INVALID_VALUES,
        "editedDataColumns is empty for compress asset");
    std::string jsonFilePath = GetAssetCompressJsonPath(to_string(fileAsset->GetId()));
    int32_t ret = ProcessPhotoSubTypeForShare(fileAsset);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Process photo subtype for share failed");
    ret = MediaJsonOperation::MapToJsonFile(fileAsset->GetMemberMap(), editedDataColumns, jsonFilePath);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Generate json file for compress asset failed");
    ret = TlvUtil::WriteJsonDataToTlv(tlv, jsonFilePath);
    bool deleteFileRet = MediaFileUtils::DeleteFileOrFolder(jsonFilePath, true);
    CHECK_AND_PRINT_LOG(deleteFileRet, "Clean json of compress asset file failed");
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Write json data to tlv failed");
    return ret;
}

bool MediaLibraryPhotoOperations::SafeAccumulateSize(int64_t add, int64_t &acc)
{
    CHECK_AND_RETURN_RET_LOG(add <= INT64_MAX - acc, false, "File size overflow");
    acc += add;
    return true;
}

int32_t MediaLibraryPhotoOperations::ProcessFileSizeWithResultSet(const shared_ptr<NativeRdb::ResultSet> &resultSet,
    FileSizeResult &result)
{
    MEDIA_DEBUG_LOG("ProcessFileSizeWithResultSet start");
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "resultSet is null.");
    bool hasDuplicateIds = !result.duplicateIdMap.empty();
    int64_t fileSize = 0;
    result.transcodeTotalSize = 0;
    int64_t transcodeSize = 0;
    string extraDataPath;
    string fileId;
    bool needProcessMovingPhoto = false;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        fileSize = GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet);
        CHECK_AND_RETURN_RET_LOG(fileSize >= 0, E_ERR, "Invalid size value");
        CHECK_AND_RETURN_RET(SafeAccumulateSize(fileSize, result.size), E_ERR);
        fileId = to_string(GetInt32Val(MediaColumn::MEDIA_ID, resultSet));
        MEDIA_DEBUG_LOG("fileId: %{public}s, file size: %{public}" PRIu64 " bytes", fileId.c_str(), fileSize);
        if (GetInt32Val(PhotoColumn::MEDIA_TYPE, resultSet) == static_cast<int32_t>(MediaType::MEDIA_TYPE_VIDEO)) {
            result.validIds.erase(std::remove(result.validIds.begin(), result.validIds.end(), fileId),
                result.validIds.end());
        }
        transcodeSize = GetInt64Val(PhotoColumn::PHOTO_TRANS_CODE_FILE_SIZE, resultSet);
        CHECK_AND_RETURN_RET_LOG(transcodeSize >= 0, E_ERR, "Invalid transcode size value");
        CHECK_AND_RETURN_RET(SafeAccumulateSize(transcodeSize, result.transcodeTotalSize), E_ERR);
        needProcessMovingPhoto =
            GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet) == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO) ||
            GetInt32Val(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, resultSet) ==
            static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY);
        if (needProcessMovingPhoto) {
            extraDataPath = MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(
                GetStringVal(PhotoColumn::MEDIA_FILE_PATH, resultSet));
            result.movingPhotoExtraDataFiles.push_back(extraDataPath);
        }
        CHECK_AND_CONTINUE(hasDuplicateIds);
        CHECK_AND_CONTINUE(result.duplicateIdMap.count(fileId) > 0);
        for (int32_t i = 0; i < result.duplicateIdMap.at(fileId); ++i) {
            CHECK_AND_RETURN_RET(SafeAccumulateSize(fileSize, result.size), E_ERR);
            CHECK_AND_RETURN_RET(SafeAccumulateSize(transcodeSize, result.transcodeTotalSize), E_ERR);
            CHECK_AND_EXECUTE(!needProcessMovingPhoto, result.movingPhotoExtraDataFiles.push_back(extraDataPath));
        }
    }
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::GetFileSizeByIds(FileSizeResult &result)
{
    MEDIA_DEBUG_LOG("GetFileSizeByIds start");
    result.size = 0;
    CHECK_AND_RETURN_RET_LOG(!result.validIds.empty(), E_INVALID_VALUES, "validIds is empty");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "rdbStore is null");
    vector<string> queryColumns = { MediaColumn::MEDIA_SIZE, PhotoColumn::MEDIA_FILE_PATH, PhotoColumn::MEDIA_TYPE,
        PhotoColumn::PHOTO_SUBTYPE, PhotoColumn::MOVING_PHOTO_EFFECT_MODE, PhotoColumn::PHOTO_TRANS_CODE_FILE_SIZE,
        MediaColumn::MEDIA_ID };
    AbsRdbPredicates predicates = AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(MediaColumn::MEDIA_ID, result.validIds);
    auto resultSet = rdbStore->Query(predicates, queryColumns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "resultSet is null.");
    int32_t ret = ProcessFileSizeWithResultSet(resultSet, result);
    if (ret != E_OK) {
        resultSet->Close();
        return ret;
    }
    resultSet->Close();
    MEDIA_INFO_LOG("GetFileSizeByIds end, total size: %{public}" PRIu64 " bytes", result.size);
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::ProcessEditDataSizeWithResultSet(const shared_ptr<NativeRdb::ResultSet> &resultSet,
    FileSizeResult &result)
{
    MEDIA_DEBUG_LOG("ProcessEditDataSizeWithResultSet start");
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "resultSet is null.");
    bool hasDuplicateIds = !result.duplicateIdMap.empty();
    int64_t editdataSize = 0;
    string fileId;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        editdataSize = GetInt64Val(PhotoExtColumn::EDITDATA_SIZE, resultSet);
        CHECK_AND_RETURN_RET_LOG(editdataSize >= 0, E_ERR, "Invalid size value");
        CHECK_AND_RETURN_RET(SafeAccumulateSize(editdataSize, result.totalEditDataSize), E_ERR);
        CHECK_AND_CONTINUE(hasDuplicateIds);
        fileId = to_string(GetInt32Val(PhotoExtColumn::PHOTO_ID, resultSet));
        MEDIA_DEBUG_LOG("fileId: %{public}s, editdataSize: %{public}" PRIu64 " bytes", fileId.c_str(), editdataSize);
        CHECK_AND_CONTINUE(result.duplicateIdMap.count(fileId) > 0);
        for (int32_t i = 0; i < result.duplicateIdMap.at(fileId); ++i) {
            CHECK_AND_RETURN_RET(SafeAccumulateSize(editdataSize, result.totalEditDataSize), E_ERR);
        }
    }
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::GetEditDataSizeByIds(FileSizeResult &result)
{
    MEDIA_DEBUG_LOG("GetEditDataSizeByIds start");
    result.totalEditDataSize = 0;
    CHECK_AND_RETURN_RET_INFO_LOG(!result.validIds.empty(), E_OK, "validIds is empty");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "RdbStore is null");

    NativeRdb::AbsRdbPredicates predicates(PhotoExtColumn::PHOTOS_EXT_TABLE);
    predicates.In(PhotoExtColumn::PHOTO_ID, result.validIds);
    std::vector<std::string> columns = { PhotoExtColumn::EDITDATA_SIZE, PhotoExtColumn::PHOTO_ID };
    auto resultSet = rdbStore->Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "resultSet is null.");
    int32_t ret = ProcessEditDataSizeWithResultSet(resultSet, result);
    if (ret != E_OK) {
        resultSet->Close();
        return ret;
    }
    resultSet->Close();
    MEDIA_INFO_LOG("GetEditDataSizeByIds end, size: %{public}" PRIu64 " bytes", result.totalEditDataSize);
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::GetSizeByFiles(const std::vector<std::string> &filePaths, int64_t &size)
{
    MEDIA_DEBUG_LOG("GetSizeByFiles start");
    size = 0;
    CHECK_AND_RETURN_RET_LOG(!filePaths.empty(), E_INVALID_VALUES, "filePaths is empty");
    size_t fileSize = 0;
    for (const auto &filePath : filePaths) {
        CHECK_AND_CONTINUE_ERR_LOG(MediaFileUtils::IsFileExists(filePath), "File not exist: %{public}s",
            DfxUtils::GetSafePath(filePath).c_str());
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::GetFileSize(filePath, fileSize), E_ERR,
            "Get file size failed: %{public}s", DfxUtils::GetSafePath(filePath).c_str());
        CHECK_AND_RETURN_RET_LOG(fileSize <= INT64_MAX, E_ERR, "File size overflow");
        CHECK_AND_RETURN_RET(SafeAccumulateSize(static_cast<int64_t>(fileSize), size), E_ERR);
    }
    MEDIA_INFO_LOG("GetSizeByFiles end, total size: %{public}" PRIu64 " bytes", size);
    return E_OK;
}

int32_t MediaLibraryPhotoOperations::GetCompressAssetSize(const std::vector<std::string> &uris, int64_t &size)
{
    MEDIA_INFO_LOG("GetCompressAssetSize start");
    size = 0;
    CHECK_AND_RETURN_RET_LOG(!uris.empty(), E_INVALID_VALUES, "uris is empty");
    string id;
    FileSizeResult result;
    std::unordered_set<std::string> idSet;
    for (const auto &uri : uris) {
        id = MediaFileUtils::GetIdFromUri(uri);
        CHECK_AND_RETURN_RET_LOG(!uri.empty() && MediaLibraryDataManagerUtils::IsNumber(id), E_INVALID_URI,
            "Invalid uri: %{public}s", uri.c_str());
        result.validIds.push_back(id);
        CHECK_AND_EXECUTE(idSet.insert(id).second, result.duplicateIdMap[id]++);
    }
    int32_t ret = GetFileSizeByIds(result);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Get total file size by ids failed");

    ret = GetEditDataSizeByIds(result);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Get total edit data size by ids failed");

    // enable process transcode file size when it is included in editdata_size
    if (PROCESS_TRANSCODE_SIZE) {
        CHECK_AND_RETURN_RET_LOG(result.totalEditDataSize > result.transcodeTotalSize, E_ERR,
            "Invalid edit data size value");
        result.totalEditDataSize -= result.transcodeTotalSize;
    }
    CHECK_AND_RETURN_RET_LOG(result.validIds.size() <= UINT64_MAX / FIXED_PADDING_BYTES, E_ERR,
        "Padding size overflow");
    int64_t sumSize = FIXED_PADDING_BYTES * result.validIds.size();
    CHECK_AND_RETURN_RET(SafeAccumulateSize(result.totalEditDataSize, sumSize), E_ERR);
    CHECK_AND_RETURN_RET(SafeAccumulateSize(result.size, sumSize), E_ERR);
    size = sumSize;
    if (!result.movingPhotoExtraDataFiles.empty()) {
        int64_t totalExtraDataSize = 0;
        ret = GetSizeByFiles(result.movingPhotoExtraDataFiles, totalExtraDataSize);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Calculate moving photo extra data size failed");
        CHECK_AND_RETURN_RET_LOG(size > totalExtraDataSize, E_ERR, "Invalid size value");
        size -= totalExtraDataSize;
    }
    MEDIA_INFO_LOG("GetCompressAssetSize end, total size: %{public}" PRIu64 " bytes", size);
    return E_OK;
}
} // namespace Media
} // namespace OHOS
//LCOV_EXCL_STOP