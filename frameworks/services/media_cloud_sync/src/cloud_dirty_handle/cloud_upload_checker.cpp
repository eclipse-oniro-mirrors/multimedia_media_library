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

#define MLOG_TAG "MediaLibraryCloudUploadChecker"

#include "cloud_upload_checker.h"

#include <sys/stat.h>

#include "exif_rotate_utils.h"
#include "media_file_utils.h"
#include "media_file_uri.h"
#include "medialibrary_unistore_manager.h"
#include "metadata_extractor.h"
#include "photo_album_column.h"
#include "thumbnail_const.h"
#include "media_column.h"
#include "preferences.h"
#include "preferences_helper.h"
#include "result_set_utils.h"
#include "thumbnail_const.h"
#include "medialibrary_object_utils.h"
#include "medialibrary_photo_operations.h"
#include "moving_photo_file_utils.h"
#include "medialibrary_subscriber.h"
#include "lake_const.h"

namespace OHOS {
namespace Media {
using namespace std;
using namespace NativeRdb;

const std::int32_t BATCH_SIZE = 500;
const int32_t NO_ORIGIN_NO_LCD = 101;

const int SCANLINE_DEFAULT_VERSION = 0;
const int SCANLINE_CURRENT_VERSION = 1;

const std::string TASK_PROGRESS_XML = "/data/storage/el2/base/preferences/task_progress.xml";
const std::string NO_ORIGIN_PHOTO_NUMBER = "no_origin_photo_number";
const std::string NO_ORIGIN_BUT_LCD_PHOTO_NUMBER = "no_origin_but_lcd_photo_number";
const std::string SCANLINE_VERSION = "scanline_version";

std::mutex CloudUploadChecker::mutex_;

const std::string SQL_PHOTOS_TABLE_QUERY_NO_ORIGIN_BUT_LCD_COUNT = "SELECT"
                                                                   " COUNT( * ) AS Count "
                                                                   "FROM"
                                                                   " Photos "
                                                                   "WHERE"
                                                                   " ( dirty = 100 OR dirty = 1 )"
                                                                   " AND thumbnail_ready >= 3"
                                                                   " AND lcd_visit_time >= 2"
                                                                   " AND file_id > ?;";

const std::string SQL_PHOTOS_TABLE_QUERY_NO_ORIGIN_BUT_LCD_PHOTO = "SELECT"
                                                                   " file_id,"
                                                                   " data,"
                                                                   " size,"
                                                                   " subtype,"
                                                                   " moving_photo_effect_mode, "
                                                                   " file_source_type "
                                                                   "FROM"
                                                                   " Photos "
                                                                   "WHERE"
                                                                   " ( dirty = 100 OR dirty = 1 )"
                                                                   " AND thumbnail_ready >= 3"
                                                                   " AND lcd_visit_time >= 2"
                                                                   " AND file_id > ?"
                                                                   " LIMIT ?;";

void CloudUploadChecker::HandleNoOriginPhoto()
{
    MEDIA_INFO_LOG("start handle no origin photo!");
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    int32_t errCode = E_OK;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(TASK_PROGRESS_XML, errCode);
    CHECK_AND_RETURN_LOG(prefs, "get preferences error: %{public}d", errCode);
    int32_t curFileId = prefs->GetInt(NO_ORIGIN_PHOTO_NUMBER, 0);
    MEDIA_INFO_LOG("start file id: %{public}d", curFileId);
    while (MedialibrarySubscriber::IsCurrentStatusOn() && QueryLcdPhotoCount(curFileId) > 0) {
        MEDIA_INFO_LOG("handle origin photo curFileId: %{public}d", curFileId);
        std::vector<CheckedPhotoInfo> photoInfos = QueryPhotoInfo(curFileId);
        HandlePhotoInfos(photoInfos, curFileId);
        prefs->PutInt(NO_ORIGIN_PHOTO_NUMBER, curFileId);
        prefs->FlushSync();
    }
    MEDIA_INFO_LOG(
        "end handle no origin photo! cost: %{public}" PRId64, MediaFileUtils::UTCTimeMilliSeconds() - startTime);
    return;
}

inline bool IsMovingPhoto(int32_t subtype, int32_t movingPhotoEffectMode)
{
    return subtype == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO) ||
           movingPhotoEffectMode == static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY);
}

int32_t GetPhotoRealSize(const bool isMovingPhoto, const std::string &path, int64_t &size)
{
    if (isMovingPhoto) {
        size = static_cast<int64_t>(MovingPhotoFileUtils::GetMovingPhotoSize(path));
        return E_OK;
    }
    struct stat st {};
    CHECK_AND_RETURN_RET_LOG(stat(path.c_str(), &st) == 0,
        E_ERR,
        "stat syscall failed, path=%{public}s, errno=%{public}d",
        path.c_str(),
        errno);

    size = static_cast<int64_t>(st.st_size);
    return E_OK;
}

void CloudUploadChecker::FillDbFileSize(const CheckedPhotoInfo &photoInfo, ValuesBucket &values)
{
    int64_t size = 0;
    bool isMovingPhoto = IsMovingPhoto(photoInfo.subtype, photoInfo.movingPhotoEffectMode);

    CHECK_AND_RETURN_LOG(GetPhotoRealSize(isMovingPhoto, photoInfo.path, size) == E_OK,
        "get photo real size failed, path=%{public}s", photoInfo.path.c_str());

    values.PutLong(MediaColumn::MEDIA_SIZE, size);
}

void CloudUploadChecker::FillDbHeightAndWidth(const CheckedPhotoInfo &photoInfo, ValuesBucket &values)
{
    std::unique_ptr<Metadata> data = std::make_unique<Metadata>();
    CHECK_AND_RETURN_LOG(data != nullptr, "Data is nullptr");
    data->SetFilePath(photoInfo.path);
    int32_t err = MetadataExtractor::ExtractImageMetadata(data);
    CHECK_AND_RETURN_LOG(err == E_OK, "Extract image metadata err: %{public}d", err);
    int32_t height = data->GetFileHeight();
    int32_t width = data->GetFileWidth();
    double aspectRatio = MediaFileUtils::CalculateAspectRatio(height, width);
    values.Put(PhotoColumn::PHOTO_ASPECT_RATIO, aspectRatio);
    values.PutInt(PhotoColumn::PHOTO_HEIGHT, height);
    values.PutInt(PhotoColumn::PHOTO_WIDTH, width);

    MEDIA_INFO_LOG("height: %{public}d, width: %{public}d, aspectRatio: %{public}lf", height, width, aspectRatio);
}

bool CloudUploadChecker::RepairPhotoByLcdUpdateDbColumn(const CheckedPhotoInfo &photoInfo)
{
    ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyTypes::TYPE_NEW));
    FillDbFileSize(photoInfo, values);
    FillDbHeightAndWidth(photoInfo, values);
    values.PutInt(PhotoColumn::PHOTO_ORIENTATION, 0);
    values.PutInt(PhotoColumn::PHOTO_EXIF_ROTATE, static_cast<int32_t>(ExifRotateType::TOP_LEFT));

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "rdbStore is nullptr");

    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, photoInfo.fileId);
    
    int32_t updateCount = 0;
    int32_t err = rdbStore->Update(updateCount, values, predicates);

    CHECK_AND_RETURN_RET_LOG(err == NativeRdb::E_OK, false, "Update rdb err: %{public}d, file id: %{public}d",
        err, photoInfo.fileId);
    return true;
}

void CloudUploadChecker::UpdateFileSize(const CheckedPhotoInfo &photoInfo, bool isMovingPhoto)
{
    int64_t size = 0;
    CHECK_AND_RETURN_LOG(GetPhotoRealSize(isMovingPhoto, photoInfo.path, size) == E_OK,
        "get photo real size failed, path=%{public}s",
        photoInfo.path.c_str());

    CHECK_AND_RETURN_INFO_LOG(photoInfo.size != size,
        "no need to update db file size, file_id=%{public}d, size=%{public}" PRId64,
        photoInfo.fileId,
        size);

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "rdbStore is nullptr");

    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, photoInfo.fileId);

    ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyTypes::TYPE_NEW));
    values.PutLong(MediaColumn::MEDIA_SIZE, size);

    int32_t updateCount = 0;
    int32_t err = rdbStore->Update(updateCount, values, predicates);

    CHECK_AND_RETURN_LOG(err == NativeRdb::E_OK,
        "update db file size failed, file_id=%{public}d, err=%{public}d",
        photoInfo.fileId,
        err);

    MEDIA_INFO_LOG("update db file size succeed, file_id=%{public}d, old size=%{public}" PRId64
                   ", new size=%{public}" PRId64,
        photoInfo.fileId,
        photoInfo.size,
        size);
}

void CloudUploadChecker::HandleMissingFile(
    const CheckedPhotoInfo &photoInfo, bool isMovingPhoto, std::vector<std::string> &noLcdList)
{
    std::string lcdPath = GetThumbnailPath(photoInfo.path, THUMBNAIL_LCD_SUFFIX);
    if (!MediaFileUtils::IsFileExists(lcdPath)) {
        MEDIA_WARN_LOG("lcd path does not exist, file_id: %{public}d", photoInfo.fileId);
        noLcdList.push_back(std::to_string(photoInfo.fileId));
        return;
    }

    CHECK_AND_RETURN_LOG(MediaFileUtils::CopyFileUtil(lcdPath, photoInfo.path),
        "copy lcd to origin photo failed, file_id: %{public}d",
        photoInfo.fileId);

    MEDIA_INFO_LOG("copy lcd to origin photo succeed, file_id: %{public}d", photoInfo.fileId);
    CHECK_AND_RETURN_LOG(RepairPhotoByLcdUpdateDbColumn(photoInfo), "Update column failed");
}

void CloudUploadChecker::HandlePhotoInfos(const std::vector<CheckedPhotoInfo> &photoInfos, int32_t &curFileId)
{
    std::vector<std::string> noLcdList;

    for (const CheckedPhotoInfo &photoInfo : photoInfos) {
        CHECK_AND_BREAK_INFO_LOG(MedialibrarySubscriber::IsCurrentStatusOn(), "current status is off, break");
        curFileId = photoInfo.fileId;
        if (photoInfo.fileSourceType == static_cast<int32_t>(FileSourceType::MEDIA_HO_LAKE)) {
            // do nothing in lake, delete when check;
            continue;
        }
        bool isMovingPhoto = IsMovingPhoto(photoInfo.subtype, photoInfo.movingPhotoEffectMode);
        if (MediaFileUtils::IsFileExists(photoInfo.path)) {
            UpdateFileSize(photoInfo, isMovingPhoto);
            continue;
        }
        HandleMissingFile(photoInfo, isMovingPhoto, noLcdList);
    }
    UpdateDirty(noLcdList, NO_ORIGIN_NO_LCD);
}

void CloudUploadChecker::UpdateDirty(const std::vector<std::string> &idList, int32_t dirtyType)
{
    CHECK_AND_RETURN_INFO_LOG(!idList.empty(), "idList is empty");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "Failed to get rdbstore!");
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(MediaColumn::MEDIA_ID, idList);
    ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_DIRTY, dirtyType);
    int32_t updateCount = 0;
    int32_t err = rdbStore->Update(updateCount, values, predicates);
    MEDIA_INFO_LOG("dirty: %{public}d, idList size: %{public}zu, update size: %{public}d, err: %{public}d",
        dirtyType,
        idList.size(),
        updateCount,
        err);
}

std::vector<CheckedPhotoInfo> CloudUploadChecker::QueryPhotoInfo(int32_t startFileId)
{
    std::vector<CheckedPhotoInfo> photoInfos;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, photoInfos, "Failed to get rdbstore!");

    const std::vector<NativeRdb::ValueObject> bindArgs = {startFileId, BATCH_SIZE};
    auto resultSet = rdbStore->QuerySql(SQL_PHOTOS_TABLE_QUERY_NO_ORIGIN_BUT_LCD_PHOTO, bindArgs);
    bool cond = resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK;
    CHECK_AND_RETURN_RET_LOG(cond, photoInfos, "resultSet is null or count is 0");

    do {
        std::string path =
            get<std::string>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_FILE_PATH, resultSet, TYPE_STRING));
        CHECK_AND_CONTINUE_ERR_LOG(!path.empty(), "Failed to get data path");
        CheckedPhotoInfo photoInfo;
        photoInfo.fileId = get<int32_t>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_ID, resultSet, TYPE_INT32));
        photoInfo.path = path;
        photoInfo.size = get<int64_t>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_SIZE, resultSet, TYPE_INT64));
        photoInfo.subtype =
            get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_SUBTYPE, resultSet, TYPE_INT32));
        photoInfo.movingPhotoEffectMode = get<int32_t>(
            ResultSetUtils::GetValFromColumn(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, resultSet, TYPE_INT32));
        photoInfo.fileSourceType = get<int32_t>(
            ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, resultSet, TYPE_INT32));
        photoInfos.push_back(photoInfo);
    } while (MedialibrarySubscriber::IsCurrentStatusOn() && resultSet->GoToNextRow() == NativeRdb::E_OK);
    return photoInfos;
}

int32_t CloudUploadChecker::QueryLcdPhotoCount(int32_t startFileId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "Failed to get rdbStore.");
    const std::vector<NativeRdb::ValueObject> bindArgs = {startFileId};
    auto resultSet = rdbStore->QuerySql(SQL_PHOTOS_TABLE_QUERY_NO_ORIGIN_BUT_LCD_COUNT, bindArgs);
    CHECK_AND_RETURN_RET_LOG(
        resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK, 0, "resultSet is null or count is 0");
    return get<int32_t>(ResultSetUtils::GetValFromColumn("Count", resultSet, TYPE_INT32));
}

void CloudUploadChecker::RepairNoOriginPhoto()
{
    std::unique_lock<std::mutex> lock(mutex_, std::defer_lock);
    CHECK_AND_RETURN_WARN_LOG(lock.try_lock(), "Repairing no origin photos has started, skipping this operation");
    MEDIA_INFO_LOG("start repair no origin photo!");
    int32_t errCode = E_OK;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(TASK_PROGRESS_XML, errCode);
    CHECK_AND_RETURN_LOG(prefs, "get preferences error: %{public}d", errCode);
    int scanlineVersion = prefs->GetInt(SCANLINE_VERSION, SCANLINE_DEFAULT_VERSION);
    MEDIA_INFO_LOG("scanline version: %{public}d", scanlineVersion);
    if (scanlineVersion < SCANLINE_CURRENT_VERSION) {
        prefs->PutInt(NO_ORIGIN_PHOTO_NUMBER, 0);
        prefs->PutInt(SCANLINE_VERSION, SCANLINE_CURRENT_VERSION);
        prefs->FlushSync();
    }
    HandleNoOriginPhoto();
    MEDIA_INFO_LOG("end repair no origin photo!");
}
}  // namespace Media
}  // namespace OHOS