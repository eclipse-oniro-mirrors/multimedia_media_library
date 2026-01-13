/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include <fcntl.h>
#include <sys/stat.h>

#include "shooting_mode_album_operation.h"

#include "directory_ex.h"

#include "abs_rdb_predicates.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_subscriber.h"
#include "medialibrary_unistore_manager.h"
#include "metadata_extractor.h"
#include "result_set_utils.h"
#include "scanner_utils.h"
#include "values_bucket.h"
#include "medialibrary_type_const.h"
#include "photo_album_column.h"
#include "preferences.h"
#include "preferences_helper.h"
#include "medialibrary_notify.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_tracer.h"

using namespace OHOS::NativeRdb;

namespace OHOS::Media {
const std::string ORIGIN_SHOOTING_MODE_ASSETS_NUMBER = "origin_shooting_mode_assets_number";
const std::int32_t SHOOTING_MODE_SCAN_BATCH_SIZE = 50;
const std::string TASK_PROGRESS_XML = "/data/storage/el2/base/preferences/task_progress.xml";

const std::string SQL_PHOTOS_TABLE_QUERY_SHOOTING_ASSETS = "SELECT"
                                                           " file_id,"
                                                           " data,"
                                                           " media_type,"
                                                           " subtype,"
                                                           " front_camera,"
                                                           " shooting_mode,"
                                                           " shooting_mode_tag "
                                                           "FROM"
                                                           " Photos "
                                                           "WHERE"
                                                           " (COALESCE(shooting_mode, '') = ''"
                                                           " OR COALESCE(front_camera, '') = ''"
                                                           " OR (COALESCE(subtype, 0) = 0 AND mime_type = 'video/mp4'"
                                                           " AND media_type = 2))"
                                                           " AND sync_status = 0"
                                                           " AND clean_flag = 0"
                                                           " AND time_pending = 0"
                                                           " AND is_temp = 0"
                                                           " AND (position = 1 OR position = 3)"
                                                           " AND file_id > ?"
                                                           " AND file_id <= ?;";

std::atomic<bool> ShootingModeAlbumOperation::isContinue_{true};

void ShootingModeAlbumOperation::Stop()
{
    isContinue_.store(false);
}

static int QueryMaxFileId()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "get rdb store failed");
    string queryMaxSql = "SELECT Max(file_id) FROM " + PhotoColumn::PHOTOS_TABLE;
    auto resultSet = rdbStore->QuerySql(queryMaxSql);
    CHECK_AND_RETURN_RET_LOG(TryToGoToFirstRow(resultSet), E_ERR, "Query max file_id failed");
    int32_t maxFileId = -1;
    maxFileId = GetInt32Val("Max(file_id)", resultSet);
    return maxFileId;
}

void ShootingModeAlbumOperation::UpdateShootingModeAlbum()
{
    isContinue_.store(true);
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    int32_t errCode = E_OK;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(TASK_PROGRESS_XML, errCode);
    CHECK_AND_RETURN_LOG(prefs, "get preferences error: %{public}d", errCode);
    int32_t curFileId = prefs->GetInt(ORIGIN_SHOOTING_MODE_ASSETS_NUMBER, 0);
    int maxFileId = QueryMaxFileId();
    MEDIA_INFO_LOG("ShootingModeAlbumOperation::UpdateShootingModeAlbum start. "
        "curFileId: %{public}d, maxFileId: %{public}d", curFileId, maxFileId);
    CHECK_AND_RETURN_LOG(maxFileId > 0, "query max file id failed");
    while (curFileId < maxFileId && MedialibrarySubscriber::IsCurrentStatusOn() && isContinue_.load()) {
        int32_t endId = std::min(curFileId + SHOOTING_MODE_SCAN_BATCH_SIZE, maxFileId);
        std::vector<CheckedShootingAssetsInfo> photoInfos = QueryShootingAssetsInfo(curFileId, endId);
        HandleInfos(photoInfos);
        curFileId = endId;
    }
    prefs->PutInt(ORIGIN_SHOOTING_MODE_ASSETS_NUMBER, curFileId);
    prefs->FlushSync();
    MEDIA_INFO_LOG(
        "end handle no origin photo! curFileId: %{public}d, cost: %{public}" PRId64,
        curFileId, MediaFileUtils::UTCTimeMilliSeconds() - startTime);
    return;
}

std::vector<CheckedShootingAssetsInfo> ShootingModeAlbumOperation::QueryShootingAssetsInfo(int32_t startFileId,
    int32_t maxFileId)
{
    std::vector<CheckedShootingAssetsInfo> photoInfos;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, photoInfos, "Failed to get rdbstore!");

    const std::vector<NativeRdb::ValueObject> bindArgs = {startFileId, maxFileId};
    auto resultSet = rdbStore->QuerySql(SQL_PHOTOS_TABLE_QUERY_SHOOTING_ASSETS, bindArgs);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, photoInfos, "resultSet is null");
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return photoInfos;
    }

    do {
        std::string path =
            get<std::string>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_FILE_PATH, resultSet, TYPE_STRING));
        CHECK_AND_CONTINUE_ERR_LOG(!path.empty(), "Failed to get data path");
        CheckedShootingAssetsInfo photoInfo;
        photoInfo.fileId =
            get<int32_t>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_ID, resultSet, TYPE_INT32));
        photoInfo.path = path;
        photoInfo.mediaType =
            get<int32_t>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_TYPE, resultSet, TYPE_INT32));
        photoInfo.subtype =
            get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_SUBTYPE, resultSet, TYPE_INT32));
        photoInfo.frontCamera = get<std::string>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_FRONT_CAMERA,
            resultSet, TYPE_STRING));
        photoInfo.shootingMode = get<std::string>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_SHOOTING_MODE,
            resultSet, TYPE_STRING));
        photoInfo.shootingModeTag =
            get<std::string>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_SHOOTING_MODE_TAG,
            resultSet, TYPE_STRING));
        photoInfos.push_back(photoInfo);
    } while (resultSet->GoToNextRow() == NativeRdb::E_OK);
    resultSet->Close();
    return photoInfos;
}

static void NotifyAnalysisAlbum(const vector<string>& changedAlbumIds)
{
    if (changedAlbumIds.size() <= 0) {
        return;
    }
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_LOG(watch != nullptr, "Can not get MediaLibraryNotify Instance");
    for (const string& albumId : changedAlbumIds) {
        watch->Notify(MediaFileUtils::GetUriByExtrConditions(
            PhotoAlbumColumns::ANALYSIS_ALBUM_URI_PREFIX, albumId), NotifyType::NOTIFY_UPDATE);
    }
}

void ShootingModeAlbumOperation::HandleInfos(const std::vector<CheckedShootingAssetsInfo> &photoInfos)
{
    bool hasUpdateShootingAssets = false;
    std::unordered_set<string> albumIdsToUpdate;
    for (const CheckedShootingAssetsInfo &photoInfo : photoInfos) {
        if (ScanAndUpdateAssetShootingMode(photoInfo, albumIdsToUpdate)) {
            hasUpdateShootingAssets = true;
        }
    }

    if (hasUpdateShootingAssets) {
        auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
        CHECK_AND_RETURN_LOG(rdbStore != nullptr, "rdbStore is nullptr");
        vector<string> albumIdsVector(albumIdsToUpdate.begin(), albumIdsToUpdate.end());
        if (!albumIdsVector.empty()) {
            MediaLibraryRdbUtils::UpdateAnalysisAlbumInternal(rdbStore, albumIdsVector);
            NotifyAnalysisAlbum(albumIdsVector);
        }
    }
}

static int32_t ExtractMetadata(std::unique_ptr<Metadata> &data, const CheckedShootingAssetsInfo &photoInfo)
{
    int32_t err = E_ERR;
    data->SetFilePath(photoInfo.path);
    data->SetFileName(MediaFileUtils::GetFileName(photoInfo.path));
    data->SetFileMediaType(photoInfo.mediaType);
    if (data->GetFileMediaType() == MediaType::MEDIA_TYPE_IMAGE) {
        err = MetadataExtractor::ExtractImageMetadata(data);
        data->SetVideoMode(0);
    } else {
        err = MetadataExtractor::ExtractAVMetadata(data);
    }
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "Failed to extract image metadata");
    return E_OK;
}

bool ShootingModeAlbumOperation::ScanAndUpdateAssetShootingMode(const CheckedShootingAssetsInfo &photoInfo,
    std::unordered_set<string> &albumIdsToUpdate)
{
    std::unique_ptr<Metadata> data = make_unique<Metadata>();
    int32_t ret = ExtractMetadata(data, photoInfo);
    CHECK_AND_RETURN_RET(ret == E_OK, false);
    CHECK_AND_RETURN_RET(!(data->GetShootingMode() == "" && data->GetFrontCamera() == "" &&
        data->GetPhotoSubType() == static_cast<int32_t>(PhotoSubType::DEFAULT)), false);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "rdbStore is nullptr");
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, photoInfo.fileId);

    ValuesBucket value;
    if (photoInfo.subtype == 0 && photoInfo.subtype != data->GetPhotoSubType()) {
        value.PutInt(PhotoColumn::PHOTO_SUBTYPE, data->GetPhotoSubType());
    }
    if (photoInfo.frontCamera.empty() && photoInfo.frontCamera != data->GetFrontCamera()) {
        value.PutString(PhotoColumn::PHOTO_FRONT_CAMERA, data->GetFrontCamera());
    }
    if (photoInfo.shootingMode.empty() && photoInfo.shootingMode != data->GetShootingMode()) {
        value.PutString(PhotoColumn::PHOTO_SHOOTING_MODE, data->GetShootingMode());
    }
    if (photoInfo.shootingModeTag.empty() && photoInfo.shootingModeTag != data->GetShootingModeTag()) {
        value.PutString(PhotoColumn::PHOTO_SHOOTING_MODE_TAG, data->GetShootingModeTag());
    }
    CHECK_AND_RETURN_RET(!value.IsEmpty(), false);
    value.PutLong(PhotoColumn::PHOTO_META_DATE_MODIFIED, MediaFileUtils::UTCTimeMilliSeconds());

    vector<ShootingModeAlbumType> albumTypes = ShootingModeAlbum::GetShootingModeAlbumOfAsset(
        data->GetPhotoSubType(), data->GetFileMimeType(), data->GetMovingPhotoEffectMode(),
        data->GetFrontCamera(), data->GetShootingMode());
    for (const auto& type : albumTypes) {
        int32_t albumId;
        if (MediaLibraryRdbUtils::QueryShootingModeAlbumIdByType(type, albumId)) {
            albumIdsToUpdate.insert(to_string(albumId));
        }
    }

    int32_t updateCount = 0;
    int32_t err = rdbStore->Update(updateCount, value, predicates);

    CHECK_AND_RETURN_RET_LOG(err == NativeRdb::E_OK, false,
        "Update shooting assets failed, file_id=%{public}d, err=%{public}d", photoInfo.fileId, err);

    MEDIA_INFO_LOG("Update shooting assets success, file_id=%{public}d, shooting_mode=%{public}s, "
        "shooting_mode_tag=%{public}s, front_camera=%{public}s, subtype=%{public}d",
        photoInfo.fileId, data->GetShootingMode().c_str(), data->GetShootingModeTag().c_str(),
        data->GetFrontCamera().c_str(), data->GetPhotoSubType());
    return updateCount > 0;
}
}  // namespace OHOS::Media