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

using namespace std;
using namespace OHOS::NativeRdb;
using namespace OHOS::RdbDataShareAdapter;

namespace OHOS {
namespace Media {
static const int32_t MAX_UPDATE_RETRY_TIMES = 5;

std::shared_ptr<ResultSet> EnhancementDatabaseOperations::Query(MediaLibraryCommand &cmd,
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

std::shared_ptr<ResultSet> EnhancementDatabaseOperations::BatchQuery(MediaLibraryCommand &cmd,
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

int32_t EnhancementDatabaseOperations::Update(ValuesBucket &rdbValues, AbsRdbPredicates &predicates)
{
    int32_t changedRows = -1;
    for (int32_t i = 0; i < MAX_UPDATE_RETRY_TIMES; i++) {
        changedRows = MediaLibraryRdbStore::UpdateWithDateTime(rdbValues, predicates);
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

static void HandleDateAdded(const int64_t dateAdded, const MediaType type, ValuesBucket &outValues)
{
    outValues.PutLong(MediaColumn::MEDIA_DATE_ADDED, dateAdded);
    if (type != MEDIA_TYPE_PHOTO) {
        return;
    }
    outValues.PutString(PhotoColumn::PHOTO_DATE_YEAR,
        MediaFileUtils::StrCreateTimeByMilliseconds(PhotoColumn::PHOTO_DATE_YEAR_FORMAT, dateAdded));
    outValues.PutString(PhotoColumn::PHOTO_DATE_MONTH,
        MediaFileUtils::StrCreateTimeByMilliseconds(PhotoColumn::PHOTO_DATE_MONTH_FORMAT, dateAdded));
    outValues.PutString(PhotoColumn::PHOTO_DATE_DAY,
        MediaFileUtils::StrCreateTimeByMilliseconds(PhotoColumn::PHOTO_DATE_DAY_FORMAT, dateAdded));
    outValues.PutLong(MediaColumn::MEDIA_DATE_TAKEN, dateAdded);
}

static void SetOwnerAlbumId(ValuesBucket &assetInfo)
{
    RdbPredicates queryPredicates(PhotoAlbumColumns::TABLE);
    queryPredicates.EqualTo(PhotoAlbumColumns::ALBUM_TYPE, to_string(PhotoAlbumType::SYSTEM));
    queryPredicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::CLOUD_ENHANCEMENT));
    vector<string> columns = { PhotoAlbumColumns::ALBUM_ID };
    shared_ptr<ResultSet> resultSet = MediaLibraryRdbStore::QueryWithFilter(queryPredicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to query cloud enhancement album id!");
        return;
    }
    int32_t albumId = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSet);
    assetInfo.PutInt(PhotoColumn::PHOTO_OWNER_ALBUM_ID, albumId);
}

int32_t EnhancementDatabaseOperations::InsertCloudEnhancementImageInDb(MediaLibraryCommand &cmd,
    const FileAsset &fileAsset, int32_t sourceFileId, shared_ptr<CloudEnhancementFileInfo> info,
    std::shared_ptr<TransactionOperations> trans)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "get rdb store failed");
    CHECK_AND_RETURN_RET_LOG(fileAsset.GetPath().empty() || !MediaFileUtils::IsFileExists(fileAsset.GetPath()),
        E_FILE_EXIST, "file %{private}s exists now", fileAsset.GetPath().c_str());
    const string& displayName = fileAsset.GetDisplayName();
    int64_t nowTime = MediaFileUtils::UTCTimeMilliSeconds();
    ValuesBucket assetInfo;
    assetInfo.PutInt(MediaColumn::MEDIA_TYPE, fileAsset.GetMediaType());
    string extension = ScannerUtils::GetFileExtension(displayName);
    assetInfo.PutString(MediaColumn::MEDIA_MIME_TYPE,
        MimeTypeUtils::GetMimeTypeFromExtension(extension));
    assetInfo.PutString(MediaColumn::MEDIA_FILE_PATH, fileAsset.GetPath());
    assetInfo.PutLong(MediaColumn::MEDIA_TIME_PENDING, fileAsset.GetTimePending());
    assetInfo.PutString(MediaColumn::MEDIA_NAME, displayName);
    assetInfo.PutString(MediaColumn::MEDIA_TITLE, MediaFileUtils::GetTitleFromDisplayName(displayName));
    assetInfo.PutString(MediaColumn::MEDIA_DEVICE_NAME, cmd.GetDeviceName());
    HandleDateAdded(nowTime, MEDIA_TYPE_PHOTO, assetInfo);
    // Set subtype if source image is moving photo
    MEDIA_DEBUG_LOG("source file subtype: %{public}d, hidden: %{public}d", info->subtype, info->hidden);
    assetInfo.PutInt(MediaColumn::MEDIA_HIDDEN, info->hidden);
    if (info->subtype == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)) {
        assetInfo.PutInt(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
        RdbPredicates queryPredicates(PhotoColumn::PHOTOS_TABLE);
        queryPredicates.EqualTo(MediaColumn::MEDIA_ID, sourceFileId);
        vector<string> columns = { PhotoColumn::PHOTO_DIRTY };
        shared_ptr<ResultSet> resultSet = MediaLibraryRdbStore::QueryWithFilter(queryPredicates, columns);
        if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Failed to query dirty!");
        }
        int32_t dirty = GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet);
        if (dirty < 0) {
            assetInfo.PutInt(PhotoColumn::PHOTO_DIRTY, dirty);
        }
    }
    assetInfo.PutInt(PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(CloudEnhancementAvailableType::FINISH));
    assetInfo.PutInt(PhotoColumn::PHOTO_STRONG_ASSOCIATION,
        static_cast<int32_t>(StrongAssociationType::CLOUD_ENHANCEMENT));
    assetInfo.PutInt(PhotoColumn::PHOTO_ASSOCIATE_FILE_ID, sourceFileId);
    SetOwnerAlbumId(assetInfo);
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
} // namespace Media
} // namespace OHOS