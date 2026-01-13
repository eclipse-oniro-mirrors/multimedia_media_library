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

#define MLOG_TAG "MediaLibraryCloudBackupRestore"

#include "cloud_backup_restore.h"

#include "backup_const.h"
#include "backup_const_column.h"
#include "backup_file_utils.h"
#include "backup_log_utils.h"
#include "medialibrary_errno.h"
#include "photo_file_utils.h"
#include "result_set_utils.h"
#include "upgrade_restore_task_report.h"

namespace OHOS {
namespace Media {
int32_t CloudBackupRestore::Init(const std::string &backupRestoreDir, const std::string &upgradePath, bool isUpgrade)
{
    appDataPath_ = backupRestoreDir;
    filePath_ = backupRestoreDir;
    galleryDbPath_ = backupRestoreDir + "/" + GALLERY_DB_NAME;
    audioDbPath_ = backupRestoreDir + INTERNAL_PREFIX + "/0/" + AUDIO_DB_NAME;
    shouldIncludeSd_ = true;
    SetCloneParameterAndStopSync();

    int32_t errCode = InitDb(isUpgrade);
    CHECK_AND_RETURN_RET(errCode == E_OK, errCode);

    photoAlbumRestore_.OnStart(mediaLibraryRdb_, galleryRdb_);
    photosRestore_.OnStart(mediaLibraryRdb_, galleryRdb_);
    MEDIA_INFO_LOG("Init db succ.");
    return E_OK;
}

void CloudBackupRestore::GetAccountValid()
{
    isAccountValid_ = false;
}

void CloudBackupRestore::GetSyncSwitchOn()
{
    isSyncSwitchOn_ = false;
}

bool CloudBackupRestore::ParseResultSetFromGallery(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
    FileInfo &info)
{
    // [basic info]display_name, data, media_type, size, title
    CHECK_AND_RETURN_RET(IsBasicInfoValid(resultSet, info, GALLERY_DB_NAME), false);
    info.title = GetStringVal(GALLERY_TITLE, resultSet);
    info.fileIdOld = GetInt32Val(GALLERY_ID, resultSet);
    info.localMediaId = info.fileIdOld;

    // [album info]source_path, owner_album_id, package_name
    info.sourcePath = GetStringVal(GALLERY_MEDIA_SOURCE_PATH, resultSet);
    info.lPath = GetStringVal(GALLERY_ALBUM_IPATH, resultSet);
    info.lPath = photosRestore_.FindlPath(info);
    info.bundleName = photosRestore_.FindBundleName(info);
    info.packageName = photosRestore_.FindPackageName(info);

    // [special type]
    info.photoQuality = GetInt32Val(PhotoColumn::PHOTO_QUALITY, resultSet);
    info.photoQuality = photosRestore_.FindPhotoQuality(info);
    info.isLivePhoto = photosRestore_.FindIsLivePhoto(info);
    if (BackupFileUtils::IsLivePhoto(info) && !BackupFileUtils::ConvertToMovingPhoto(info)) {
        ErrorInfo errorInfo(RestoreError::MOVING_PHOTO_CONVERT_FAILED, 1, "",
            BackupLogUtils::FileInfoToString(sceneCode_, info));
        UpgradeRestoreTaskReport().SetSceneCode(this->sceneCode_).SetTaskId(this->taskId_).ReportError(errorInfo);
        return false;
    }

    // [time info]date_taken, date_modified, date_added, detail_time
    info.dateTaken = GetInt64Val(GALLERY_DATE_TAKEN, resultSet);
    info.dateModified = GetInt64Val(EXTERNAL_DATE_MODIFIED, resultSet) * MSEC_TO_SEC;
    info.firstUpdateTime = GetInt64Val(EXTERNAL_DATE_ADDED, resultSet) * MSEC_TO_SEC;
    info.detailTime = GetStringVal(GALLERY_DETAIL_TIME, resultSet);

    return true;
}

NativeRdb::ValuesBucket CloudBackupRestore::GetInsertValue(const FileInfo &fileInfo, const std::string &newPath,
    int32_t sourceType)
{
    NativeRdb::ValuesBucket values;
    // [basic info]display_name, data, media_type, title
    values.PutString(MediaColumn::MEDIA_NAME, fileInfo.displayName);
    values.PutString(MediaColumn::MEDIA_FILE_PATH, newPath);
    values.PutInt(MediaColumn::MEDIA_TYPE, fileInfo.fileType);
    values.PutString(MediaColumn::MEDIA_TITLE, fileInfo.title);

    // [album info]source_path, owner_album_id, package_name
    values.PutString(PhotoColumn::PHOTO_SOURCE_PATH, photosRestore_.FindSourcePath(fileInfo));
    values.PutInt(PhotoColumn::PHOTO_OWNER_ALBUM_ID, photosRestore_.FindAlbumId(fileInfo));
    CHECK_AND_EXECUTE(fileInfo.packageName.empty(),
        values.PutString(PhotoColumn::MEDIA_PACKAGE_NAME, fileInfo.packageName));

    // [special type]
    values.PutInt(PhotoColumn::PHOTO_QUALITY, fileInfo.photoQuality);
    values.PutInt(PhotoColumn::PHOTO_SUBTYPE, photosRestore_.FindSubtype(fileInfo));
    values.PutInt(PhotoColumn::PHOTO_BURST_COVER_LEVEL, photosRestore_.FindBurstCoverLevel(fileInfo));
    values.PutNull(PhotoColumn::PHOTO_BURST_KEY);
    values.PutInt(PhotoColumn::PHOTO_STRONG_ASSOCIATION, photosRestore_.FindStrongAssociationByDisplayName(fileInfo));
    values.PutInt(PhotoColumn::PHOTO_CE_AVAILABLE, photosRestore_.FindCeAvailableByDisplayName(fileInfo));

    values.PutInt(PhotoColumn::PHOTO_SYNC_STATUS, static_cast<int32_t>(SyncStatusType::TYPE_BACKUP));
    values.PutInt(PhotoColumn::PHOTO_DIRTY, photosRestore_.FindDirty(fileInfo));
    return values;
}

void CloudBackupRestore::SetValueFromMetaData(FileInfo &fileInfo, NativeRdb::ValuesBucket &value)
{
    std::unique_ptr<Metadata> data = make_unique<Metadata>();
    SetMetaDataValue(fileInfo, data);

    // [basic info]size
    SetSize(data, fileInfo, value);

    // [time info]date_taken, date_modified, date_added, detail_time, date_year, date_month, date_day
    SetTimeInfo(data, fileInfo, value);

    // [metadata info]
    value.PutString(MediaColumn::MEDIA_MIME_TYPE, data->GetFileMimeType());
    value.PutString(PhotoColumn::PHOTO_MEDIA_SUFFIX, data->GetFileExtension());
    value.PutInt(MediaColumn::MEDIA_DURATION, data->GetFileDuration());
    value.PutLong(MediaColumn::MEDIA_TIME_PENDING, 0);
    value.PutInt(PhotoColumn::PHOTO_HEIGHT, data->GetFileHeight());
    value.PutInt(PhotoColumn::PHOTO_WIDTH, data->GetFileWidth());
    value.PutDouble(PhotoColumn::PHOTO_ASPECT_RATIO, data->GetFileAspectRatio());
    value.PutDouble(PhotoColumn::PHOTO_LONGITUDE, data->GetLongitude());
    value.PutDouble(PhotoColumn::PHOTO_LATITUDE, data->GetLatitude());
    value.PutString(PhotoColumn::PHOTO_ALL_EXIF, data->GetAllExif());
    value.PutString(PhotoColumn::PHOTO_SHOOTING_MODE, data->GetShootingMode());
    value.PutString(PhotoColumn::PHOTO_SHOOTING_MODE_TAG, data->GetShootingModeTag());
    value.PutLong(PhotoColumn::PHOTO_LAST_VISIT_TIME, data->GetLastVisitTime());
    value.PutString(PhotoColumn::PHOTO_FRONT_CAMERA, data->GetFrontCamera());
    value.PutInt(PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE, data->GetDynamicRangeType());
    value.PutInt(PhotoColumn::PHOTO_HDR_MODE, data->GetHdrMode());
    value.PutString(PhotoColumn::PHOTO_USER_COMMENT, data->GetUserComment());
    BaseRestore::SetOrientationAndExifRotate(fileInfo, value, data);

    // [special type]live photo
    SetCoverPosition(fileInfo, value);
    Set3DgsSubtype(fileInfo, value, data);
    BaseRestore::InsertVideoMode(data, value);
}

void CloudBackupRestore::SetSize(const std::unique_ptr<Metadata> &data, FileInfo &info,
    NativeRdb::ValuesBucket &value)
{
    // [basic info]size
    info.fileSize = info.fileSize > 0 ? info.fileSize : data->GetFileSize();
    value.PutLong(MediaColumn::MEDIA_SIZE, info.fileSize);
}

void CloudBackupRestore::SetTimeInfo(const std::unique_ptr<Metadata> &data, FileInfo &info,
    NativeRdb::ValuesBucket &value)
{
    // [time info]date_taken, date_modified, date_added
    info.dateTaken = info.dateTaken > 0 ? info.dateTaken : data->GetDateTaken();
    info.dateModified = info.dateModified > 0 ? info.dateModified : data->GetFileDateModified();
    info.firstUpdateTime = info.firstUpdateTime > 0 ? info.firstUpdateTime : info.dateModified;

    info.firstUpdateTime =
        PhotoFileUtils::NormalizeTimestamp(info.firstUpdateTime, MediaFileUtils::UTCTimeMilliSeconds());
    info.dateModified = PhotoFileUtils::NormalizeTimestamp(info.dateModified, info.firstUpdateTime);
    info.dateTaken = PhotoFileUtils::NormalizeTimestamp(info.dateTaken, min(info.firstUpdateTime, info.dateModified));

    value.Put(MediaColumn::MEDIA_DATE_ADDED, info.firstUpdateTime);
    value.Put(MediaColumn::MEDIA_DATE_MODIFIED, info.dateModified);
    value.Put(MediaColumn::MEDIA_DATE_TAKEN, info.dateTaken);
    InsertDateTime(value, info);
}

void CloudBackupRestore::RestoreAnalysisAlbum()
{
    return;
}

void CloudBackupRestore::InsertPhotoRelated(std::vector<FileInfo> &fileInfos, int32_t sourceType)
{
    return;
}

bool CloudBackupRestore::ConvertPathToRealPath(const std::string &srcPath, const std::string &prefix,
    std::string &newPath, std::string &relativePath, FileInfo &fileInfo)
{
    if (MediaFileUtils::StartsWith(srcPath, INTERNAL_PREFIX)) {
        return UpgradeRestore::ConvertPathToRealPath(srcPath, prefix, newPath, relativePath);
    }
    size_t pos = 0;
    if (!BackupFileUtils::GetPathPosByPrefixLevel(sceneCode_, srcPath, SD_PREFIX_LEVEL, pos)) {
        return false;
    }
    newPath = prefix + srcPath;
    relativePath = srcPath.substr(pos);
    fileInfo.isInternal = false;
    return true;
}
} // namespace Media
} // namespace OHOS