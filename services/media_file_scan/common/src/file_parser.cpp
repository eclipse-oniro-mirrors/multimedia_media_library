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
#define MLOG_TAG "FileParser"

#include "file_parser.h"

#include <regex>
#include <sys/stat.h>

#include "directory_ex.h"
#include "file_scan_utils.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_file_notify_info.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdb_transaction.h"
#include "medialibrary_unistore_manager.h"
#include "result_set_utils.h"
#include "thumbnail_service.h"
#include "media_string_utils.h"
#include "medialibrary_photo_operations.h"
#include "moving_photo_file_utils.h"
#include "photo_dao.h"

using namespace OHOS::NativeRdb;
namespace OHOS::Media {
std::mutex g_fileManagerScanFlagMutex;
std::unordered_map<int32_t, bool> g_fileManagerScanFlag;
const std::string PATH_HIDDEN_PREFIX = ".";
// LCOV_EXCL_START

bool FileParser::MetaStatus::IsChanged() const
{
    return isMediaTypeChanged || isSizeChanged || isDateModifiedChanged || isMimeTypeChanged || isStoragePathChanged ||
        isInvisible;
}

std::string FileParser::MetaStatus::ToString() const
{
    std::stringstream ss;
    ss << "MetaStatus["
        << "isMediaTypeChanged: " << isMediaTypeChanged << ", "
        << "isSizeChanged: " << isSizeChanged << ", "
        << "isDateModifiedChanged: " << isDateModifiedChanged << ", "
        << "isMimeTypeChanged: " << isMimeTypeChanged  << ", "
        << "isStoragePathChanged: " << isStoragePathChanged  << ", "
        << "isInvisible: " << isInvisible  << "]";
    return ss.str();
}

FileParser::FileParser(const std::string &path, const FileSourceType &sourceType, ScanMode scanMode)
    : sourceType_(sourceType), scanMode_(scanMode)
{
    path_ = path;
    mediaLibraryRdb_ = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
}

FileParser::FileParser(const MediaNotifyInfo &notifyInfo, const FileSourceType &sourceType, ScanMode scanMode)
    : notifyInfo_(notifyInfo), sourceType_(sourceType), scanMode_(scanMode)
{
    MEDIA_INFO_LOG("Get notifyInfo obj: %{public}d, opt: %{public}d, before: %{public}s, after: %{public}s",
        static_cast<int32_t>(notifyInfo.objType),
        static_cast<int32_t>(notifyInfo.optType),
        FileScanUtils::GarbleFilePath(notifyInfo.beforePath).c_str(),
        FileScanUtils::GarbleFilePath(notifyInfo.afterPath).c_str());
    path_ = notifyInfo.afterPath;
    mediaLibraryRdb_ = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
}

bool IsImageOrVideoType(int32_t fileType)
{
    return fileType == MediaType::MEDIA_TYPE_IMAGE ||
           fileType == MediaType::MEDIA_TYPE_VIDEO;
}

bool FileParser::CheckTypeValid()
{
    if (IsImageOrVideoType(fileInfo_.fileType)) {
        return true;
    }
    fileInfo_.fileType = MediaFileUtils::GetMediaTypeNotSupported(fileInfo_.displayName);
    return IsImageOrVideoType(fileInfo_.fileType);
}

bool FileParser::CheckSizeValid()
{
    return fileInfo_.fileSize > FILE_SIZE_1K;
}

bool FileParser::CheckIsNotHidden()
{
    return !MediaStringUtils::StartsWith(fileInfo_.displayName, PATH_HIDDEN_PREFIX);
}

bool FileParser::IsFileValidAsset()
{
    bool isTypeValid = CheckTypeValid();
    bool isSizeValid = CheckSizeValid();
    bool isNotHidden = CheckIsNotHidden();
    bool ret = isTypeValid && isSizeValid && isNotHidden && fileInfo_.needInsert;
    CHECK_AND_EXECUTE(ret, MEDIA_INFO_LOG("FileParser: isTypeValid: %{public}d, "
        "isSizeValid: %{public}d, isNotHidden: %{public}d, needInsert: %{public}d, "
        "fileInfo: %{public}s", isTypeValid, isSizeValid, isNotHidden, fileInfo_.needInsert, ToString().c_str()));
    return ret;
}

IsBurstType FileParser::CheckBurst(const std::string &displayName)
{
    size_t burstPos = displayName.find("_BURST");
    if (burstPos == std::string::npos) {
        return IsBurstType::OTHER_TYPE;
    }
    if (displayName.find("COVER") != std::string::npos) {
        return IsBurstType::BURST_COVER_TYPE;
    }
    return IsBurstType::BURST_MEMBER_TYPE;
}

std::string FileParser::PrintInfo(const InnerFileInfo& info)
{
    std::stringstream ss;
    ss << "InnerFileInfo {\n\t"
       << "inode: " << info.inode << ", "
       << "filePath: " << FileScanUtils::GarbleFilePath(info.filePath) << ", "
       << "fileSize: " << info.fileSize << ", "
       << "dateModified: " << info.dateModified << ", "
       << "dateAdded: " << info.dateAdded << ", "
       << "dateTaken: " << info.dateTaken << ", "
       << "height: " << info.height << ", "
       << "width: " << info.width << ", "
       << "duration: " << info.duration << ", "
       << "detailTime: " << info.detailTime << ", "
       << "fileType: " << info.fileType << ", "
       << "subtype: " << info.subtype << ", "
       << "mimeType: " << info.mimeType << ", "
       << "bundleName: " << info.bundleName << ", "
       << "burstKey: " << info.burstKey << ", "
       << "cloudPath: " << FileScanUtils::GarbleFilePath(info.cloudPath) << ", "
       << "dateDay: " << info.dateDay << ", "
       << "dateMonth: " << info.dateMonth << ", "
       << "dateYear: " << info.dateYear << ", "
       << "displayName: " << FileScanUtils::GarbleFile(info.displayName) << ", "
       << "fileSourceType: " << info.fileSourceType << ", "
       << "frontCamera: " << info.frontCamera << ", "
       << "mediaSuffix: " << info.mediaSuffix << ", "
       << "packageName: " << info.packageName << ", "
       << "shootingMode: " << info.shootingMode << ", "
       << "shootingModeTag: " << info.shootingModeTag << ", "
       << "title: " << FileScanUtils::GarbleFile(info.title) << ", "
       << "userComment: " << info.userComment << ", "
       << "dynamicRangeType: " << info.dynamicRangeType << ", "
       << "ownerAlbumId: " << info.ownerAlbumId << ", "
       << "strongAssociation: " << info.strongAssociation << ", "
       << "lastVisitTime: " << info.lastVisitTime << ", "
       << "latitude: " << info.latitude << ", "
       << "longitude: " << info.longitude << ", "
       << "needInsert: " << (info.needInsert ? "true" : "false") << ", "
       << "hdrMode: " << info.hdrMode << ", "
       << "videoMode: " << info.videoMode << ", "
       << "}";
    return ss.str();
}

void FileParser::ParseFileInfo()
{
    struct stat statInfo;
    if (lstat(path_.c_str(), &statInfo) == -1) {
        MEDIA_ERR_LOG("FileParser:Failed to get info of path %{public}s, errno: %{public}d",
            FileScanUtils::GarbleFilePath(path_).c_str(), errno);
        return;
    }
    fileInfo_.inode = std::to_string(statInfo.st_ino);
    fileInfo_.displayName = ExtractFileName(path_);
    fileInfo_.fileType = MediaFileUtils::GetMediaType(fileInfo_.displayName);
    fileInfo_.fileSize = statInfo.st_size;
    fileInfo_.localAssetSize = statInfo.st_size;
    fileInfo_.dateAdded = GetFileDateAdded(statInfo);
    fileInfo_.dateModified = MediaFileUtils::Timespec2Millisecond(statInfo.st_mtim);
    fileInfo_.filePath = path_;
    fileInfo_.title = FileScanUtils::GetFileTitle(fileInfo_.displayName);
    fileInfo_.isBurst = CheckBurst(fileInfo_.displayName);
    FileScanUtils::SetBurstKey(fileInfo_);
    fileInfo_.subtype = FileScanUtils::FindSubtype(fileInfo_);
    if (fileInfo_.fileType == MediaType::MEDIA_TYPE_IMAGE) {
        std::regex pattern(R"(.*_enhanced(\.[^.]+)$)");
        if (std::regex_match(path_, pattern)) {
            MEDIA_INFO_LOG("FileParser: %{public}s is an enhanced image!",
                FileScanUtils::GarbleFilePath(path_).c_str());
            fileInfo_.strongAssociation = STRONG_ASSOCIATION_ENABLE;
            fileInfo_.ceAvailable = CLOUD_ENHANCEMENT_PHOTO;
        }
    }
    std::unique_ptr<Metadata> data = make_unique<Metadata>();
    data->SetFilePath(fileInfo_.filePath);
    data->SetFileMediaType(fileInfo_.fileType);
    data->SetFileDateModified(fileInfo_.dateModified);
    data->SetFileName(fileInfo_.displayName);
    data->SetPhotoSubType(fileInfo_.subtype);
    if (FileScanUtils::FillMetadata(data) != E_OK) {
        fileInfo_.needInsert = false;
        MEDIA_ERR_LOG("FileParser: Failed to get data: %{public}s", ToString().c_str());
        return;
    }
    fileInfo_.mimeType = data->GetFileMimeType();
    fileInfo_.mediaSuffix = data->GetFileExtension();
    fileInfo_.dateModified = data->GetFileDateModified();
    fileInfo_.dateTaken = data->GetDateTaken();
    fileInfo_.detailTime = data->GetDetailTime();
    fileInfo_.dateYear = data->GetDateYear();
    fileInfo_.dateMonth = data->GetDateMonth();
    fileInfo_.dateDay = data->GetDateDay();
    fileInfo_.duration = data->GetFileDuration();
    fileInfo_.orientation = data->GetOrientation();
    fileInfo_.height = data->GetFileHeight();
    fileInfo_.width = data->GetFileWidth();
    fileInfo_.longitude = data->GetLongitude();
    fileInfo_.latitude = data->GetLatitude();
    fileInfo_.allExif = data->GetAllExif();
    fileInfo_.shootingMode = data->GetShootingMode();
    fileInfo_.shootingModeTag = data->GetShootingModeTag();
    fileInfo_.lastVisitTime = data->GetLastVisitTime();
    fileInfo_.frontCamera = data->GetFrontCamera();
    fileInfo_.dynamicRangeType = data->GetDynamicRangeType();
    fileInfo_.userComment = data->GetUserComment();
    fileInfo_.fileSourceType = sourceType_;
    fileInfo_.exifRotate = data->GetExifRotate();
    SetSubtypeFromMetadata(data);
    fileInfo_.hdrMode = data->GetHdrMode();
    fileInfo_.videoMode = data->GetVideoMode();
    fileInfo_.coverPosition = data->GetCoverPosition();
    fileInfo_.musicMasterMode = data->GetMusicMasterMode();
}

bool FileParser::HasChangePart(const FileParser::PhotosRowData &rowData)
{
    metaStatus_.isMediaTypeChanged = rowData.mediaType != fileInfo_.fileType;
    metaStatus_.isSizeChanged = rowData.size != fileInfo_.fileSize;
    metaStatus_.isDateModifiedChanged = rowData.dateModified != fileInfo_.dateModified;
    metaStatus_.isMimeTypeChanged = rowData.mimeType != fileInfo_.mimeType;
    metaStatus_.isStoragePathChanged = rowData.storagePath != fileInfo_.filePath;
    metaStatus_.isInvisible = rowData.syncStatus != static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE);
    bool scanFlag = true;
    {
        std::lock_guard<std::mutex> lock(g_fileManagerScanFlagMutex);
        if (g_fileManagerScanFlag.count(rowData.fileId) > 0 && g_fileManagerScanFlag[rowData.fileId]) {
            scanFlag = false;
        }
    }
    bool ret = metaStatus_.IsChanged() && scanFlag;
    CHECK_AND_EXECUTE(!ret, MEDIA_INFO_LOG("metaStatus: %{public}s, fileInfo: %{public}s",
        metaStatus_.ToString().c_str(), ToString().c_str()));
    return ret;
}

bool FileParser::IsStoragePathChanged(const FileParser::PhotosRowData &rowData)
{
    bool ret = rowData.storagePath != fileInfo_.filePath;
    CHECK_AND_EXECUTE(!ret, MEDIA_INFO_LOG("storagePath before: %{public}s, after: %{public}s",
        FileScanUtils::GarbleFilePath(rowData.storagePath).c_str(),
        FileScanUtils::GarbleFilePath(fileInfo_.filePath).c_str()));
    return ret;
}

FileParser::PhotosRowData FileParser::FindSameFile()
{
    switch (notifyInfo_.optType) {
        case FileNotifyOperationType::ADD:
            return FindSameFileByOptAdd();
        case FileNotifyOperationType::MOD:
            return FindSameFileByOptMod();
        default:
            return FindSameFileByDefault();
    }
}

// 新增消息通知场景，按afterPath查找
FileParser::PhotosRowData FileParser::FindSameFileByOptAdd()
{
    MEDIA_INFO_LOG("FindSameFileByOptAdd %{public}s", FileScanUtils::GarbleFilePath(notifyInfo_.afterPath).c_str());
    return FindSameFileByStoragePath(notifyInfo_.afterPath);
}

// 更新消息通知场景，按beforePath和afterPath查找
//  1. beforePath和afterPath都不存在：按新增处理
//  2. beforePath和afterPath存在一个：按更新判断
//  3. beforePath和afterPath都存在：报错，按afterPath更新判断
FileParser::PhotosRowData FileParser::FindSameFileByOptMod()
{
    MEDIA_INFO_LOG("FindSameFileByOptMod %{public}s and %{public}s",
        FileScanUtils::GarbleFilePath(notifyInfo_.beforePath).c_str(),
        FileScanUtils::GarbleFilePath(notifyInfo_.afterPath).c_str());
    FileParser::PhotosRowData rowDataBefore = FindSameFileByStoragePath(notifyInfo_.beforePath);
    FileParser::PhotosRowData rowDataAfter = FindSameFileByStoragePath(notifyInfo_.afterPath);
    if (!rowDataBefore.IsExist() && !rowDataAfter.IsExist()) {
        return FileParser::PhotosRowData();
    }
    if (rowDataBefore.IsExist() && rowDataAfter.IsExist()) {
        MEDIA_ERR_LOG("[Error]Both beforePath and afterPath records are found, use afterPath %{public}s",
            FileScanUtils::GarbleFilePath(notifyInfo_.afterPath).c_str());
        return rowDataAfter;
    }
    return rowDataBefore.IsExist() ? rowDataBefore : rowDataAfter;
}

// 默认场景（非新增/更新消息通知，如首次加载），按当前路径查找
FileParser::PhotosRowData FileParser::FindSameFileByDefault()
{
    MEDIA_INFO_LOG("FindSameFileByDefault %{public}s", FileScanUtils::GarbleFilePath(path_).c_str());
    return FindSameFileByStoragePath(path_);
}

FileParser::PhotosRowData FileParser::FindSameFileByStoragePath(const std::string &storagePath)
{
    return PhotoDao().FindSameFileByStoragePath(storagePath, sourceType_);
}

int32_t FileParser::IsExistSameFileForCloneRestore(int32_t ownerAlbumId)
{
    int pictureFlag = fileInfo_.fileType == MediaType::MEDIA_TYPE_VIDEO ? 0 : 1;
    std::vector<NativeRdb::ValueObject> params = { ownerAlbumId, fileInfo_.displayName,
        fileInfo_.fileSize, pictureFlag, fileInfo_.orientation};
    return PhotoDao().IsExistSameFileForCloneRestore(params);
}

// 校验当前资产的通知是否合法
bool FileParser::IsNotifyInfoValid()
{
    switch (notifyInfo_.optType) {
        case FileNotifyOperationType::ADD:
        case FileNotifyOperationType::MOD:
            return !notifyInfo_.afterPath.empty();
        default:
            // 非新增/更新消息通知，如首次加载，默认合法
            return true;
    }
}

int32_t FileParser::UpdateAssetInfo()
{
    int32_t errCode = UpdateAssetInDatabase();
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode,
        "UpdateAssetInDatabase failed, ret: %{public}d, fileInfo: %{public}s", errCode, ToString().c_str());
    int32_t ret = MediaLibraryPhotoOperations::CalSingleEditDataSize(std::to_string(fileInfo_.fileId));
    CHECK_AND_PRINT_LOG(ret == E_OK,
        "CalSingleEditDataSize failed ID: %{public}d (ret code: %{public}d)", fileInfo_.fileId, ret);
    CHECK_AND_RETURN_RET_INFO_LOG(ShouldGenerateThumbnail(), E_OK,
        "No need to generate thumbnail, fileInfo: %{public}s", ToString().c_str());
    MEDIA_INFO_LOG("Start generate thumbnail of %{public}s", ToString().c_str());
    errCode = GenerateSingleThumbnail(GetThumbnailInfo());
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode,
        "GenerateThumbnail failed, ret: %{public}d, fileInfo: %{public}s", errCode, ToString().c_str());
    return E_OK;
}

int32_t FileParser::UpdateAssetInfo(int32_t albumId, const std::string &bundleName, const std::string &albumName)
{
    SetAlbumInfo(albumId, bundleName, albumName);
    return UpdateAssetInfo();
}

NativeRdb::ValuesBucket FileParser::TransFileInfoToBucket(int32_t albumId, const std::string &bundleName,
    const std::string &albumName)
{
    SetAlbumInfo(albumId, bundleName, albumName);
    SetCloudPath();
    return GetAssetInsertValues();
}

void FileParser::SetFileId(int32_t fileId)
{
    fileInfo_.fileId = fileId;
}

void FileParser::SetAlbumInfo(int32_t albumId, const std::string &bundleName, const std::string &albumName)
{
    fileInfo_.ownerAlbumId = albumId;
    fileInfo_.bundleName = bundleName;
    fileInfo_.packageName = albumName;
}

void FileParser::SetByPhotosRowData(const PhotosRowData &rowData)
{
    SetFileId(rowData.fileId);
    SetAlbumInfo(rowData.ownerAlbumId, rowData.ownerPackage, rowData.packageName);
    fileInfo_.cloudPath = rowData.data;
    fileInfo_.editTime = rowData.editTime;
    if (rowData.subtype == static_cast<int32_t>(PhotoSubType::SLOW_MOTION_VIDEO)) {
        fileInfo_.subtype = rowData.subtype;
    }
    if (rowData.dateTaken > 0 && rowData.dateTaken < fileInfo_.dateTaken) {
        SetDateTakenFields(rowData);
    }
}

void FileParser::SetDateTakenFields(const PhotosRowData &rowData)
{
    fileInfo_.dateTaken = rowData.dateTaken;
    fileInfo_.dateYear = rowData.dateYear;
    fileInfo_.dateMonth = rowData.dateMonth;
    fileInfo_.dateDay = rowData.dateDay;
    fileInfo_.detailTime = rowData.detailTime;
}

NativeRdb::ValuesBucket FileParser::GetAssetInsertValues()
{
    NativeRdb::ValuesBucket values;
    if (!fileInfo_.needInsert) {
        return values;
    }
    values = GetAssetCommonValues();
    SetAssetBurstValues(values);
    SetAssetCloudEnhancementValues(values);
    values.Put(PhotoColumn::MEDIA_FILE_PATH, fileInfo_.cloudPath);
    values.Put(MediaColumn::MEDIA_DATE_ADDED, fileInfo_.dateAdded);
    values.Put(MediaColumn::MEDIA_DATE_TAKEN, fileInfo_.dateTaken);
    values.Put(PhotoColumn::PHOTO_DATE_YEAR, fileInfo_.dateYear);
    values.Put(PhotoColumn::PHOTO_DATE_MONTH, fileInfo_.dateMonth);
    values.Put(PhotoColumn::PHOTO_DATE_DAY, fileInfo_.dateDay);
    values.Put(PhotoColumn::PHOTO_DETAIL_TIME, fileInfo_.detailTime);
    return values;
}

NativeRdb::ValuesBucket FileParser::GetAssetUpdateValues()
{
    NativeRdb::ValuesBucket values = GetAssetCommonValues();
    SetAssetEditValues(values);
    values.Put(MediaColumn::MEDIA_DATE_TAKEN, fileInfo_.dateTaken);
    values.Put(PhotoColumn::PHOTO_DATE_YEAR, fileInfo_.dateYear);
    values.Put(PhotoColumn::PHOTO_DATE_MONTH, fileInfo_.dateMonth);
    values.Put(PhotoColumn::PHOTO_DATE_DAY, fileInfo_.dateDay);
    values.Put(PhotoColumn::PHOTO_DETAIL_TIME, fileInfo_.detailTime);
    return values;
}

int32_t FileParser::SetAssetSubtypeValues(NativeRdb::ValuesBucket &values)
{
    int32_t subtype = 0;
    int32_t effectMode = 0;
    bool found = false;
    int32_t ret = PhotoDao().QuerySubtypeAndEffectMode(fileInfo_.fileId, subtype, effectMode, found);
    CHECK_AND_RETURN_RET(ret == E_OK, E_ERR);
    CHECK_AND_RETURN_RET(found, E_OK);
    MEDIA_INFO_LOG("set asset subtype values, subtype:%{public}d, effectMode:%{public}d", subtype, effectMode);
    if (fileInfo_.subtype == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO) &&
        effectMode == static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY)) {
        values.Put(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(MovingPhotoEffectMode::DEFAULT));
        values.Put(MediaColumn::MEDIA_DURATION, 0);
    } else {
        values.Put(PhotoColumn::PHOTO_SUBTYPE, fileInfo_.subtype);
    }
    return E_OK;
}

// 电影模式V2资产的shooting_mode相关字段由相机侧写入（AddPhotoProxy/UpdatePhotoProxy），
// 扫描器解析视频元数据为空时会覆盖，因此对V2资产跳过这两个字段的更新
// 注意：values.Delete仅从本次UPDATE的SET子句移除该列，不修改数据库已有值
bool FileParser::IsCinematicVideoV2Asset()
{
    if (fileInfo_.fileType != MediaType::MEDIA_TYPE_VIDEO) {
        return false;
    }
    int32_t subtype = 0;
    bool found = false;
    int32_t ret = PhotoDao().QuerySubtype(fileInfo_.fileId, subtype, found);
    return ret == E_OK && found && subtype == static_cast<int32_t>(PhotoSubType::CINEMATIC_VIDEO_V2);
}

NativeRdb::ValuesBucket FileParser::GetAssetCommonValues()
{
    NativeRdb::ValuesBucket values;
    values.Put(MediaColumn::MEDIA_SIZE, fileInfo_.fileSize);
    values.Put(MediaColumn::MEDIA_TITLE, fileInfo_.title);
    values.Put(MediaColumn::MEDIA_NAME, fileInfo_.displayName);
    values.Put(MediaColumn::MEDIA_TYPE, fileInfo_.fileType);
    values.Put(MediaColumn::MEDIA_MIME_TYPE, fileInfo_.mimeType);
    values.Put(PhotoColumn::PHOTO_MEDIA_SUFFIX, fileInfo_.mediaSuffix);
    values.Put(PhotoColumn::PHOTO_SYNC_STATUS, static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE));

    values.Put(MediaColumn::MEDIA_DATE_MODIFIED, fileInfo_.dateModified);
    values.Put(PhotoColumn::PHOTO_META_DATE_MODIFIED, MediaFileUtils::UTCTimeMilliSeconds());

    values.Put(MediaColumn::MEDIA_DURATION, fileInfo_.duration);
    values.Put(PhotoColumn::PHOTO_ORIENTATION, fileInfo_.orientation);
    values.Put(PhotoColumn::PHOTO_HEIGHT, fileInfo_.height);
    values.Put(PhotoColumn::PHOTO_WIDTH, fileInfo_.width);
    double aspectRatio = MediaFileUtils::CalculateAspectRatio(fileInfo_.height, fileInfo_.width);
    values.PutDouble(PhotoColumn::PHOTO_ASPECT_RATIO, aspectRatio);
    PutStringVal(values, PhotoColumn::PHOTO_USER_COMMENT, fileInfo_.userComment);
    values.Put(PhotoColumn::PHOTO_ALL_EXIF, fileInfo_.allExif);
    values.Put(PhotoColumn::PHOTO_SHOOTING_MODE, fileInfo_.shootingMode);
    values.Put(PhotoColumn::PHOTO_SHOOTING_MODE_TAG, fileInfo_.shootingModeTag);
    values.Put(PhotoColumn::PHOTO_LAST_VISIT_TIME, fileInfo_.lastVisitTime);
    values.Put(PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE, fileInfo_.dynamicRangeType);
    values.Put(PhotoColumn::PHOTO_FRONT_CAMERA, fileInfo_.frontCamera);
    values.Put(PhotoColumn::PHOTO_EXIF_ROTATE, fileInfo_.exifRotate);

    values.Put(PhotoColumn::PHOTO_FILE_INODE, fileInfo_.inode);
    values.Put(PhotoColumn::PHOTO_STORAGE_PATH, fileInfo_.filePath);
    values.Put(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, fileInfo_.fileSourceType);
    values.Put(PhotoColumn::PHOTO_HDR_MODE, fileInfo_.hdrMode);
    values.Put(PhotoColumn::PHOTO_VIDEO_MODE, fileInfo_.videoMode);

    values.Put(PhotoColumn::LOCAL_ASSET_SIZE, fileInfo_.localAssetSize);
    values.Put(PhotoColumn::PHOTO_COVER_POSITION, fileInfo_.coverPosition);
    values.Put(PhotoColumn::MUSIC_MASTER_MODE, fileInfo_.musicMasterMode);

    SetAssetAlbumValues(values);
    SetAssetLocationValues(values);
    SetAssetSubtypeValues(values);
    CHECK_AND_EXECUTE(fileInfo_.subtype != static_cast<int32_t>(PhotoSubType::MOVING_PHOTO),
        SetAssetLivePhoto4dValues(values));

    return values;
}

void FileParser::SetAssetLivePhoto4dValues(NativeRdb::ValuesBucket &values)
{
    uint32_t version = 0;
    int32_t ret = MovingPhotoFileUtils::GetExtraDataVersion(fileInfo_.filePath, version);
    CHECK_AND_RETURN_LOG(ret == E_OK, "get extraData version failed, ret: %{public}d", ret);
    MEDIA_INFO_LOG("livePhoto4d: Live photo 4d version: %{public}u", version);
    if (fileInfo_.livePhoto4dStatus == static_cast<uint32_t>(MOVING_PHOTO_VERSION::MOVING_PHOTO_VERSION_8) ||
        version == LIVE_PHOTO_4D_VERSION) {
        values.Put(PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_STATUS,
            static_cast<int32_t>(LivePhoto4dStatusType::TYPE_LIVEPHOTO_4D));
        if (version != LIVE_PHOTO_4D_VERSION) {
            ret = MovingPhotoFileUtils::ModifyExtraDataVersion(fileInfo_.filePath,
                static_cast<uint32_t>(MOVING_PHOTO_VERSION::MOVING_PHOTO_VERSION_8));
            CHECK_AND_RETURN_LOG(ret == E_OK, "modify extraData version failed, ret: %{public}d", ret);
        }
    }
}

void FileParser::SetAssetAlbumValues(NativeRdb::ValuesBucket &values)
{
    if (updateType_ != FileUpdateType::INSERT && updateType_ != FileUpdateType::UPDATE_ALBUM) {
        return;
    }
    values.Put(PhotoColumn::MEDIA_OWNER_PACKAGE, fileInfo_.bundleName);
    values.Put(PhotoColumn::MEDIA_PACKAGE_NAME, fileInfo_.packageName);
    values.Put(PhotoColumn::PHOTO_OWNER_ALBUM_ID, fileInfo_.ownerAlbumId);
}

void FileParser::SetAssetBurstValues(NativeRdb::ValuesBucket &values)
{
    const std::unordered_map<IsBurstType, BurstCoverLevelType> BURST_COVER_LEVEL_MAP = {
        { IsBurstType::OTHER_TYPE, BurstCoverLevelType::COVER },
        { IsBurstType::BURST_COVER_TYPE, BurstCoverLevelType::COVER },
        { IsBurstType::BURST_MEMBER_TYPE, BurstCoverLevelType::MEMBER },
    };
    BurstCoverLevelType burstCoverLevelType = BurstCoverLevelType::COVER;
    auto iter = BURST_COVER_LEVEL_MAP.find(fileInfo_.isBurst);
    CHECK_AND_EXECUTE(iter == BURST_COVER_LEVEL_MAP.end(), burstCoverLevelType = iter->second);
    values.Put(PhotoColumn::PHOTO_BURST_COVER_LEVEL, static_cast<int32_t>(burstCoverLevelType));
    PutStringVal(values, PhotoColumn::PHOTO_BURST_KEY, fileInfo_.burstKey);
}

void FileParser::SetAssetCloudEnhancementValues(NativeRdb::ValuesBucket &values)
{
    values.Put(PhotoColumn::PHOTO_CE_AVAILABLE, fileInfo_.ceAvailable);
    values.Put(PhotoColumn::PHOTO_STRONG_ASSOCIATION, fileInfo_.strongAssociation);
}

void FileParser::SetAssetLocationValues(NativeRdb::ValuesBucket &values)
{
    constexpr double DOUBLE_EPSILON = 1e-15;
    if (fabs(fileInfo_.longitude) > DOUBLE_EPSILON || fabs(fileInfo_.latitude) > DOUBLE_EPSILON) {
        values.PutDouble(PhotoColumn::PHOTO_LONGITUDE, fileInfo_.longitude);
        values.PutDouble(PhotoColumn::PHOTO_LATITUDE, fileInfo_.latitude);
    } else {
        values.PutNull(PhotoColumn::PHOTO_LONGITUDE);
        values.PutNull(PhotoColumn::PHOTO_LATITUDE);
    }
}

void FileParser::SetAssetEditValues(NativeRdb::ValuesBucket &values)
{
    std::string editDataPath =
        FileScanUtils::GetReplacedPathByPrefixType(PrefixType::CLOUD, PrefixType::LOCAL_EDIT_DATA, fileInfo_.cloudPath);
    CHECK_AND_RETURN(fileInfo_.editTime > 0);
    bool isEditDataPathExist = MediaFileUtils::IsFileExists(editDataPath);
    bool isEditDataPathDir = MediaFileUtils::IsDirectory(editDataPath);
    bool isEditDataPathNotEmpty = !MediaFileUtils::IsDirEmpty(editDataPath);
    if (!(isEditDataPathExist && isEditDataPathDir && isEditDataPathNotEmpty)) {
        MEDIA_ERR_LOG("Reset edit_time = 0, editTime: %{public}" PRId64", isEditDataPathExist: %{public}d, "
            "isEditDataPathDir: %{public}d, isEditDataPathNotEmpty: %{public}d, fileInfo: %{public}s",
            fileInfo_.editTime, isEditDataPathExist, isEditDataPathDir, isEditDataPathNotEmpty, ToString().c_str());
        values.Put(PhotoColumn::PHOTO_EDIT_TIME, 0);
    }
}

void FileParser::PutStringVal(NativeRdb::ValuesBucket &values, const std::string &columnName,
    const std::string &columnVal)
{
    columnVal.empty() ? values.PutNull(columnName) : values.Put(columnName, columnVal);
}

InnerFileInfo FileParser::GetFileInfo()
{
    return fileInfo_;
}

std::string FileParser::GetFileAssetUri()
{
    return MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX,
        to_string(fileInfo_.fileId), MediaFileUtils::GetExtraUri(fileInfo_.displayName,
        fileInfo_.cloudPath));
}

std::string FileParser::ToString()
{
    std::stringstream ss;
    ss << "InnerFileInfo["
        << "fileId: " << fileInfo_.fileId << ", "
        << "storagePath: " << FileScanUtils::GarbleFilePath(fileInfo_.filePath) << ", "
        << "displayName: " << FileScanUtils::GarbleFile(fileInfo_.displayName) << ", "
        << "size: " << fileInfo_.fileSize << ", "
        << "fileType: " << fileInfo_.fileType << ", "
        << "dateModified: " << fileInfo_.dateModified << ", "
        << "dateTaken: " << fileInfo_.dateTaken << ", "
        << "owner_album_id: " << fileInfo_.ownerAlbumId << ", "
        << "inode: " << fileInfo_.inode << "]";
    return ss.str();
}

std::vector<std::string> FileParser::GenerateThumbnail(ScanMode scanMode, const std::vector<std::string> &inodes)
{
    MEDIA_INFO_LOG("generate thumbnail");
    std::vector<std::string> uris;
    if (scanMode == ScanMode::FULL) {
        MEDIA_INFO_LOG("no need to generate thumbnail");
        return GetFileUris(inodes);
    }

    // 查询数据库
    std::vector<ThumbnailInfo> infos;
    std::vector<int32_t> thumbnailVisibleList;
    int32_t ret = PhotoDao().QueryThumbnailInfos(inodes, infos, thumbnailVisibleList);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, uris, "QueryThumbnailInfos failed");

    // 生成缩略图（LakeFileScanner 不需要功耗管控）
    for (const auto& info : infos) {
        // 直接调用ThumbnailService生成缩略图
        std::string uri = GetThumbnailUri(info);
        int32_t err = ThumbnailService::GetInstance()->CreateThumbnailFileScaned(uri, info.path, false);
        CHECK_AND_PRINT_LOG(err == E_OK, "create thumbnail fail");

        uris.push_back(uri);
    }
    return uris;
}

std::vector<std::string> FileParser::GetFileUris(const std::vector<std::string> &inodes)
{
    // 查找对应的fileId
    std::vector<ThumbnailInfo> infos;
    std::vector<int32_t> thumbnailVisibleList;
    int32_t ret = PhotoDao().QueryThumbnailInfos(inodes, infos, thumbnailVisibleList);
    std::vector<std::string> uris;
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, uris, "QueryThumbnailInfos failed");
    for (const auto &info : infos) {
        std::string uri = MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX,
            to_string(info.fileId), MediaFileUtils::GetExtraUri(info.displayName, info.path));
        uris.push_back(uri);
    }
    return uris;
}

std::string FileParser::GetThumbnailUri(const ThumbnailInfo &info)
{
    return PhotoColumn::PHOTO_URI_PREFIX + to_string(info.fileId) + MediaFileUtils::GetExtraUri(info.displayName,
        info.path) + "?api_version=10&date_modified=" + to_string(info.dateModified) + "&date_taken=" +
        to_string(info.dateTaken);
}

int32_t FileParser::UpdateAssetInDatabase()
{
    CHECK_AND_RETURN_RET_LOG(mediaLibraryRdb_ != nullptr, E_HAS_DB_ERROR, "mediaLibraryRdb_ is null.");
    NativeRdb::ValuesBucket values = GetAssetUpdateValues();
    if (IsCinematicVideoV2Asset()) {
        values.Delete(PhotoColumn::PHOTO_SHOOTING_MODE);
        values.Delete(PhotoColumn::PHOTO_SHOOTING_MODE_TAG);
        MEDIA_INFO_LOG("skip update shooting mode fields for cinematic video v2, fileId: %{public}d",
            fileInfo_.fileId);
    }
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileInfo_.fileId);
    int32_t changedRows = 0;
    AccurateRefresh::AssetAccurateRefresh assetRefresh;
    int32_t errCode = assetRefresh.Update(changedRows, values, predicates);
    CHECK_AND_RETURN_RET_LOG(errCode == NativeRdb::E_OK && changedRows > 0, E_DB_FAIL,
        "UpdateAssetInfo failed, ret: %{public}d, changeRows: %{public}d, fileInfo: %{public}s",
        errCode, changedRows, ToString().c_str());
    assetRefresh.RefreshAlbum();
    assetRefresh.Notify();
    return E_OK;
}

int32_t FileParser::GenerateSingleThumbnail(const ThumbnailInfo &info)
{
    std::string uri = GetThumbnailUri(info);
    return ThumbnailService::GetInstance()->CreateThumbnailFileScaned(uri, info.path, false);
}

bool FileParser::ShouldGenerateThumbnail()
{
    return (updateType_ == FileUpdateType::UPDATE || updateType_ == FileUpdateType::UPDATE_ALBUM) &&
        metaStatus_.isDateModifiedChanged;
}

ThumbnailInfo FileParser::GetThumbnailInfo()
{
    ThumbnailInfo info;
    info.fileId = fileInfo_.fileId;
    info.displayName = fileInfo_.displayName;
    info.path = fileInfo_.cloudPath;
    info.dateTaken = fileInfo_.dateTaken;
    info.dateModified = fileInfo_.dateModified;
    return info;
}

int64_t FileParser::GetFileDateAdded(const struct stat &statInfo)
{
    // 首选Atime
    int64_t aTime = MediaFileUtils::Timespec2Millisecond(statInfo.st_atim);
    if (aTime != 0) {
        return aTime;
    }
    // 次选Mtime
    int64_t mTime = MediaFileUtils::Timespec2Millisecond(statInfo.st_mtim);
    if (mTime != 0) {
        return mTime;
    }
    return MediaFileUtils::UTCTimeMilliSeconds();
}

void FileParser::SetFileManagerScanFlagBySingle(const std::string &fileIdStr, bool stopScan)
{
    int32_t fileId;
    bool result = MediaStringUtils::ConvertToInt(fileIdStr, fileId);
    if (!result) {
        MEDIA_ERR_LOG("invalid fileuri %{private}s", fileIdStr.c_str());
        return;
    }
    std::lock_guard<std::mutex> lock(g_fileManagerScanFlagMutex);
    if (stopScan) {
        g_fileManagerScanFlag[fileId] = stopScan;
    } else {
        auto it = g_fileManagerScanFlag.find(fileId);
        if (it != g_fileManagerScanFlag.end()) {
            g_fileManagerScanFlag.erase(it);
        }
    }
}
// LCOV_EXCL_STOP
}  // namespace OHOS::Media