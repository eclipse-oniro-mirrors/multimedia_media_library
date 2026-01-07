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

#define MLOG_TAG "EnhancementDatabaseOperations"

#include "enhancement_database_operations.h"

#include "enhancement_manager.h"
#include "medialibrary_unistore_manager.h"
#include "media_file_uri.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "scanner_utils.h"
#include "mimetype_utils.h"
#include "result_set_utils.h"
#include "rdb_utils.h"
#include "photo_album_column.h"
#include "media_app_uri_permission_column.h"
#include "media_library_extend_manager.h"
#include "medialibrary_data_manager_utils.h"
#include "moving_photo_file_utils.h"

using namespace std;
using namespace OHOS::NativeRdb;
using namespace OHOS::RdbDataShareAdapter;

namespace OHOS {
namespace Media {
static const int32_t MAX_UPDATE_RETRY_TIMES = 5;
const size_t UPDATE_BATCH_SIZE = 200;

std::shared_ptr<NativeRdb::ResultSet> EnhancementDatabaseOperations::Query(MediaLibraryCommand &cmd,
    RdbPredicates &servicePredicates, const vector<string> &columns)
{
    RdbPredicates clientPredicates = RdbUtils::ToPredicates(cmd.GetDataSharePred(), PhotoColumn::PHOTOS_TABLE);
    const vector<string> &whereUriArgs = clientPredicates.GetWhereArgs();
    string uri = whereUriArgs.front();
    if (!MediaFileUtils::StartsWith(uri, PhotoColumn::PHOTO_URI_PREFIX)) {
        MEDIA_ERR_LOG("invalid URI: %{private}s", uri.c_str());
        return nullptr;
    }
    string fileId = MediaFileUri::GetPhotoId(uri);
    servicePredicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    return MediaLibraryRdbStore::QueryWithFilter(servicePredicates, columns);
}

std::shared_ptr<NativeRdb::ResultSet> EnhancementDatabaseOperations::BatchQuery(MediaLibraryCommand &cmd,
    const vector<string> &columns, unordered_map<int32_t, string> &fileId2Uri)
{
    RdbPredicates servicePredicates(PhotoColumn::PHOTOS_TABLE);
    RdbPredicates clientPredicates = RdbUtils::ToPredicates(cmd.GetDataSharePred(), PhotoColumn::PHOTOS_TABLE);
    const vector<string> &whereUriArgs = clientPredicates.GetWhereArgs();
    vector<string> whereIdArgs;
    whereIdArgs.reserve(whereUriArgs.size());
    for (const auto &arg : whereUriArgs) {
        if (!MediaFileUtils::StartsWith(arg, PhotoColumn::PHOTO_URI_PREFIX)) {
            MEDIA_ERR_LOG("invalid URI: %{private}s", arg.c_str());
            continue;
        }
        string fileId = MediaFileUri::GetPhotoId(arg);
        if (!MediaLibraryDataManagerUtils::IsNumber(fileId)) {
            MEDIA_ERR_LOG("invalid fileId: %{public}s", fileId.c_str());
            continue;
        }
        fileId2Uri.emplace(stoi(fileId), arg);
        whereIdArgs.push_back(fileId);
    }
    if (fileId2Uri.empty()) {
        MEDIA_ERR_LOG("submit tasks are invalid");
        return nullptr;
    }
    servicePredicates.In(MediaColumn::MEDIA_ID, whereIdArgs);
    return MediaLibraryRdbStore::QueryWithFilter(servicePredicates, columns);
}

int32_t EnhancementDatabaseOperations::Update(ValuesBucket &rdbValues, AbsRdbPredicates &predicates,
    shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh)
{
    int32_t changedRows = -1;
    for (int32_t i = 0; i < MAX_UPDATE_RETRY_TIMES; i++) {
        if (assetRefresh != nullptr) {
            changedRows = assetRefresh->UpdateWithDateTime(rdbValues, predicates);
        } else {
            changedRows = MediaLibraryRdbStore::UpdateWithDateTime(rdbValues, predicates);
        }
        if (changedRows >= 0) {
            break;
        }
        MEDIA_ERR_LOG("Update DB failed! changedRows: %{public}d times: %{public}d", changedRows, i);
    }
    if (changedRows <= 0) {
        MEDIA_INFO_LOG("Update DB failed! changedRows: %{public}d", changedRows);
        return E_HAS_DB_ERROR;
    }
    return E_OK;
}

static void HandleDateAdded(const MediaType type, ValuesBucket &outValues,
    shared_ptr<NativeRdb::ResultSet> resultSet)
{
    int64_t dateAdded = GetInt64Val(PhotoColumn::MEDIA_DATE_ADDED, resultSet);
    int64_t dateTaken = GetInt64Val(PhotoColumn::MEDIA_DATE_TAKEN, resultSet);
    outValues.PutLong(MediaColumn::MEDIA_DATE_ADDED, dateAdded);
    if (type != MEDIA_TYPE_PHOTO) {
        return;
    }
    outValues.PutString(PhotoColumn::PHOTO_DATE_YEAR,
        MediaFileUtils::StrCreateTimeByMilliseconds(PhotoColumn::PHOTO_DATE_YEAR_FORMAT, dateTaken));
    outValues.PutString(PhotoColumn::PHOTO_DATE_MONTH,
        MediaFileUtils::StrCreateTimeByMilliseconds(PhotoColumn::PHOTO_DATE_MONTH_FORMAT, dateTaken));
    outValues.PutString(PhotoColumn::PHOTO_DATE_DAY,
        MediaFileUtils::StrCreateTimeByMilliseconds(PhotoColumn::PHOTO_DATE_DAY_FORMAT, dateTaken));
    outValues.PutLong(MediaColumn::MEDIA_DATE_TAKEN, dateTaken);
}

static void SetOwnerAlbumId(ValuesBucket &assetInfo, shared_ptr<NativeRdb::ResultSet> resultSet)
{
    int32_t albumId = GetInt32Val(PhotoColumn::PHOTO_OWNER_ALBUM_ID, resultSet);
    string ownerPackage = GetStringVal(MediaColumn::MEDIA_OWNER_PACKAGE, resultSet);
    string ownerAppId = GetStringVal(MediaColumn::MEDIA_OWNER_APPID, resultSet);
    string packageName = GetStringVal(MediaColumn::MEDIA_PACKAGE_NAME, resultSet);
    assetInfo.PutInt(PhotoColumn::PHOTO_OWNER_ALBUM_ID, albumId);
    assetInfo.PutString(MediaColumn::MEDIA_OWNER_PACKAGE, ownerPackage);
    assetInfo.PutString(MediaColumn::MEDIA_OWNER_APPID, ownerAppId);
    assetInfo.PutString(MediaColumn::MEDIA_PACKAGE_NAME, packageName);
}

static void SetSupportedWatermarkType(int32_t sourceFileId, ValuesBucket &assetInfo,
    shared_ptr<NativeRdb::ResultSet> resultSet)
{
    int32_t supportedWatermarkType = GetInt32Val(PhotoColumn::SUPPORTED_WATERMARK_TYPE, resultSet);
    assetInfo.PutInt(PhotoColumn::SUPPORTED_WATERMARK_TYPE, supportedWatermarkType);
}

static void SetBasicAttributes(MediaLibraryCommand &cmd, const FileAsset &fileAsset,
    ValuesBucket &assetInfo, shared_ptr<NativeRdb::ResultSet> resultSet)
{
    const string& displayName = fileAsset.GetDisplayName();
    assetInfo.PutInt(MediaColumn::MEDIA_TYPE, fileAsset.GetMediaType());
    string extension = ScannerUtils::GetFileExtension(displayName);
    assetInfo.PutString(MediaColumn::MEDIA_MIME_TYPE,
        MimeTypeUtils::GetMimeTypeFromExtension(extension));
    assetInfo.PutString(PhotoColumn::PHOTO_MEDIA_SUFFIX, extension);
    assetInfo.PutString(MediaColumn::MEDIA_FILE_PATH, fileAsset.GetPath());
    assetInfo.PutLong(MediaColumn::MEDIA_TIME_PENDING, fileAsset.GetTimePending());
    assetInfo.PutString(MediaColumn::MEDIA_NAME, displayName);
    assetInfo.PutString(MediaColumn::MEDIA_TITLE, MediaFileUtils::GetTitleFromDisplayName(displayName));
    assetInfo.PutString(MediaColumn::MEDIA_DEVICE_NAME, cmd.GetDeviceName());
}

static void SetCloudEnhancementAttributes(int32_t sourceFileId, ValuesBucket &assetInfo)
{
    assetInfo.PutInt(PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(CloudEnhancementAvailableType::FINISH));
    assetInfo.PutInt(PhotoColumn::PHOTO_STRONG_ASSOCIATION,
        static_cast<int32_t>(StrongAssociationType::CLOUD_ENHANCEMENT));
    assetInfo.PutInt(PhotoColumn::PHOTO_ASSOCIATE_FILE_ID, sourceFileId);
}

static void SetSpecialAttributes(ValuesBucket &assetInfo, shared_ptr<CloudEnhancementFileInfo> info,
    shared_ptr<NativeRdb::ResultSet> resultSet)
{
    // Set hidden
    assetInfo.PutInt(MediaColumn::MEDIA_HIDDEN, info->hidden);
    // Set subtype if source image is moving photo
    int32_t effectMode = GetInt32Val(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, resultSet);
    int32_t originalSubtype = GetInt32Val(PhotoColumn::PHOTO_ORIGINAL_SUBTYPE, resultSet);
    if (MovingPhotoFileUtils::IsMovingPhoto(info->subtype, effectMode, originalSubtype)) {
        assetInfo.PutInt(PhotoColumn::PHOTO_SUBTYPE, info->subtype);
        assetInfo.PutInt(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, effectMode);
        assetInfo.PutInt(PhotoColumn::PHOTO_ORIGINAL_SUBTYPE, originalSubtype);
        MEDIA_INFO_LOG("subtype:%{public}d, moving_photo_effect_mode:%{public}d, original_subtype:%{public}d",
            info->subtype, effectMode, originalSubtype);
        int32_t dirty = GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet);
        if (dirty < 0) {
            assetInfo.PutInt(PhotoColumn::PHOTO_DIRTY, dirty);
        }
    }
}

int32_t EnhancementDatabaseOperations::InsertCloudEnhancementImageInDb(MediaLibraryCommand &cmd,
    const FileAsset &fileAsset, int32_t sourceFileId, shared_ptr<CloudEnhancementFileInfo> info,
    shared_ptr<NativeRdb::ResultSet> resultSet, std::shared_ptr<TransactionOperations> trans)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "get rdb store failed");
    CHECK_AND_RETURN_RET_LOG(fileAsset.GetPath().empty() || !MediaFileUtils::IsFileExists(fileAsset.GetPath()),
        E_FILE_EXIST, "file %{private}s exists now", fileAsset.GetPath().c_str());
    ValuesBucket assetInfo;
    SetBasicAttributes(cmd, fileAsset, assetInfo, resultSet);
    HandleDateAdded(MEDIA_TYPE_PHOTO, assetInfo, resultSet);
    SetSpecialAttributes(assetInfo, info, resultSet);
    SetCloudEnhancementAttributes(sourceFileId, assetInfo);
    SetOwnerAlbumId(assetInfo, resultSet);
    SetSupportedWatermarkType(sourceFileId, assetInfo, resultSet);
    cmd.SetValueBucket(assetInfo);
    cmd.SetTableName(PhotoColumn::PHOTOS_TABLE);
    int64_t outRowId = -1;
    int32_t errCode;
    if (trans == nullptr) {
        errCode = rdbStore->Insert(cmd, outRowId);
    } else {
        errCode = trans->Insert(cmd, outRowId);
    }
    CHECK_AND_RETURN_RET_LOG(errCode == NativeRdb::E_OK, E_HAS_DB_ERROR,
        "Insert into db failed, errCode = %{public}d", errCode);
    MEDIA_INFO_LOG("insert success, rowId = %{public}d", (int)outRowId);
    return static_cast<int32_t>(outRowId);
}

int64_t EnhancementDatabaseOperations::InsertCloudEnhancementPerm(int32_t sourceFileId,
    int32_t targetFileId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "get rdb store failed");
    RdbPredicates queryPredicates(AppUriPermissionColumn::APP_URI_PERMISSION_TABLE);
    queryPredicates.EqualTo(AppUriPermissionColumn::FILE_ID, sourceFileId);
    queryPredicates.And();
    queryPredicates.EqualTo(AppUriPermissionColumn::PERMISSION_TYPE,
        static_cast<int32_t>(PhotoPermissionType::PERSIST_READWRITE_IMAGEVIDEO));
    vector<string> columns = { AppUriPermissionColumn::APP_ID,
        AppUriPermissionColumn::URI_TYPE, AppUriPermissionColumn::DATE_MODIFIED,
        AppUriPermissionColumn::SOURCE_TOKENID, AppUriPermissionColumn::TARGET_TOKENID };
    auto resultSet = MediaLibraryRdbStore::StepQueryWithoutCheck(queryPredicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK,
        E_HAS_DB_ERROR, "cannot get permission from origin photo: %{public}d", sourceFileId);
    string appId = GetStringVal(AppUriPermissionColumn::APP_ID, resultSet);
    int32_t uriType = GetInt32Val(AppUriPermissionColumn::URI_TYPE, resultSet);
    int64_t dateModified = GetInt64Val(AppUriPermissionColumn::DATE_MODIFIED, resultSet);
    int64_t sourceTokenId = GetInt64Val(AppUriPermissionColumn::SOURCE_TOKENID, resultSet);
    int64_t targetTokenId = GetInt64Val(AppUriPermissionColumn::TARGET_TOKENID, resultSet);
    resultSet->Close();

    int64_t outRowId = -1;
    RdbPredicates checkPredicates(AppUriPermissionColumn::APP_URI_PERMISSION_TABLE);
    checkPredicates.EqualTo(AppUriPermissionColumn::FILE_ID, targetFileId);
    checkPredicates.EqualTo(AppUriPermissionColumn::URI_TYPE, uriType);
    checkPredicates.EqualTo(AppUriPermissionColumn::PERMISSION_TYPE,
        static_cast<int32_t>(PhotoPermissionType::PERSIST_READWRITE_IMAGEVIDEO));
    checkPredicates.EqualTo(AppUriPermissionColumn::DATE_MODIFIED, dateModified);
    checkPredicates.EqualTo(AppUriPermissionColumn::SOURCE_TOKENID, sourceTokenId);
    checkPredicates.EqualTo(AppUriPermissionColumn::TARGET_TOKENID, targetTokenId);
    resultSet = MediaLibraryRdbStore::StepQueryWithoutCheck(checkPredicates, columns);
    if (resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        MEDIA_INFO_LOG("cloud enhancement permission record has already exists: %{public}d", targetFileId);
        return outRowId;
    }

    ValuesBucket valueBucket;
    valueBucket.PutString(AppUriPermissionColumn::APP_ID, appId);
    valueBucket.PutInt(AppUriPermissionColumn::FILE_ID, targetFileId);
    valueBucket.PutInt(AppUriPermissionColumn::URI_TYPE, uriType);
    valueBucket.PutInt(AppUriPermissionColumn::PERMISSION_TYPE,
        static_cast<int32_t>(PhotoPermissionType::PERSIST_READWRITE_IMAGEVIDEO));
    valueBucket.PutLong(AppUriPermissionColumn::DATE_MODIFIED, dateModified);
    valueBucket.PutLong(AppUriPermissionColumn::SOURCE_TOKENID, sourceTokenId);
    valueBucket.PutLong(AppUriPermissionColumn::TARGET_TOKENID, targetTokenId);

    int32_t errCode = MediaLibraryRdbStore::InsertInternal(outRowId,
        AppUriPermissionColumn::APP_URI_PERMISSION_TABLE, valueBucket);
    CHECK_AND_PRINT_LOG(errCode == E_OK, "insert permission failed: %{public}d", errCode);
    MEDIA_INFO_LOG("Add permission for cloud enhancement photo success: %{public}d", targetFileId);
    return outRowId;
}

bool IsEditedTrashedHidden(const std::shared_ptr<NativeRdb::ResultSet> &ret)
{
    if (ret == nullptr) {
        MEDIA_ERR_LOG("resultset is null");
        return false;
    }
    if (ret->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("go to first row failed");
        return false;
    }
    int32_t colIndex = -1;
    double editTime = -1;
    double trashedTime = -1;
    double hiddenTime = -1;
    MEDIA_INFO_LOG("coIndex1 is %{public}d, editTime is %{public}d", colIndex, static_cast<int>(editTime));
    ret->GetColumnIndex(PhotoColumn::PHOTO_EDIT_TIME, colIndex);
    if (ret->GetDouble(colIndex, editTime) != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Fail to query DB ");
        MEDIA_INFO_LOG("coIndex2 is %{public}d, editTime is %{public}d", colIndex, static_cast<int>(editTime));
        return false;
    }
    ret->GetColumnIndex(MediaColumn::MEDIA_DATE_TRASHED, colIndex);
    if (ret->GetDouble(colIndex, trashedTime) != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Fail to query DB ");
        MEDIA_INFO_LOG("coIndex2 is %{public}d, trashedTime is %{public}d", colIndex, static_cast<int>(trashedTime));
        return false;
    }
    ret->GetColumnIndex(PhotoColumn::PHOTO_HIDDEN_TIME, colIndex);
    if (ret->GetDouble(colIndex, hiddenTime) != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Fail to query DB ");
        MEDIA_INFO_LOG("coIndex2 is %{public}d, hiddenTime is %{public}d", colIndex, static_cast<int>(hiddenTime));
        return false;
    }
    MEDIA_INFO_LOG("editTime is %{public}d, trashedTime is %{public}d, hiddenTime is %{public}d",
        static_cast<int>(editTime), static_cast<int>(trashedTime), static_cast<int>(hiddenTime));
    return (editTime == 0 && trashedTime == 0 && hiddenTime == 0) ? true : false;
}

std::shared_ptr<NativeRdb::ResultSet> EnhancementDatabaseOperations::GetPair(MediaLibraryCommand &cmd)
{
    RdbPredicates firstServicePredicates(PhotoColumn::PHOTOS_TABLE);
    RdbPredicates clientPredicates = RdbUtils::ToPredicates(cmd.GetDataSharePred(), PhotoColumn::PHOTOS_TABLE);
    const vector<string> &whereUriArgs = clientPredicates.GetWhereArgs();
    string UriArg = whereUriArgs.front();
    if (!MediaFileUtils::StartsWith(UriArg, PhotoColumn::PHOTO_URI_PREFIX)) {
        return nullptr;
    }
    string fileId = MediaFileUri::GetPhotoId(UriArg);
    MEDIA_INFO_LOG("GetPair_build fileId: %{public}s", fileId.c_str());
    vector<string> queryColumns = {PhotoColumn::PHOTO_EDIT_TIME, MediaColumn::MEDIA_DATE_TRASHED,
        PhotoColumn::PHOTO_HIDDEN_TIME};
    firstServicePredicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(firstServicePredicates, queryColumns);
    if (IsEditedTrashedHidden(resultSet)) {
        MEDIA_INFO_LOG("success into query stage after IsEditedTrashedHidden");
        RdbPredicates secondServicePredicates(PhotoColumn::PHOTOS_TABLE);
        secondServicePredicates.EqualTo(PhotoColumn::PHOTO_ASSOCIATE_FILE_ID, fileId);
        vector<string> columns = {};
        MEDIA_INFO_LOG("start query");
        auto resultSetCallback = MediaLibraryRdbStore::QueryWithFilter(secondServicePredicates, columns);
        if (IsEditedTrashedHidden(resultSetCallback)) {
            return resultSetCallback;
        }
    }
    MEDIA_INFO_LOG("PhotoAsset is edited or trashed or hidden");
    return nullptr;
}

int32_t EnhancementDatabaseOperations::QueryAndUpdatePhotos(const std::vector<std::string> &photoIds)
{
    CHECK_AND_RETURN_RET_LOG(photoIds.size() > 0, NativeRdb::E_OK, "photoIds is emtpy");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "rdbStore is nullptr");

    std::string photoIdList =
        accumulate(photoIds.begin() + 1, photoIds.end(), photoIds[0], [](const string &a, const string &b) {
            return a + ", " + b;
        });
    std::string querySql = "SELECT file_id FROM Photos WHERE (ce_available = 0 AND photo_id IN (" + photoIdList +
                           ")) OR (ce_available = 1 AND photo_id NOT IN (" + photoIdList + "));";

    auto resultSet = rdbStore->QueryByStep(querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, NativeRdb::E_ERROR, "query by step failed");

    std::vector<std::string> needUpdateFileIds;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        needUpdateFileIds.push_back(GetStringVal(PhotoColumn::MEDIA_ID, resultSet));
    }
    resultSet->Close();
    auto needChange = needUpdateFileIds.size();
    CHECK_AND_RETURN_RET_LOG(needChange > 0, NativeRdb::E_OK, "needUpdateFileIds is emtpy");

    int32_t ret = NativeRdb::E_OK;
    int64_t totalChanged = 0;
    for (size_t start = 0; start < needChange; start += UPDATE_BATCH_SIZE) {
        size_t end = std::min(start + UPDATE_BATCH_SIZE, needChange);
        std::stringstream updateSql;
        updateSql << "UPDATE Photos SET ce_available = NOT ( ce_available ) WHERE file_id IN ( ";
        for (size_t i = start; i < end; ++i) {
            if (i != start) {
                updateSql << ", ";
            }
            updateSql << needUpdateFileIds[i];
        }
        updateSql << " );";
        int64_t changedRowCount = 0;
        auto errCode = rdbStore->ExecuteForChangedRowCount(changedRowCount, updateSql.str());
        if (errCode != NativeRdb::E_OK) {
            ret = errCode;
            MEDIA_ERR_LOG("execute for changed row count failed, errCode: %{public}d", errCode);
        } else {
            totalChanged += changedRowCount;
            MEDIA_INFO_LOG("changed row count: %{public}" PRId64, changedRowCount);
        }
    }

    MEDIA_INFO_LOG("need change: %{public}zu, total changed: %{public}" PRId64, needChange, totalChanged);
    return ret;
}
} // namespace Media
} // namespace OHOS