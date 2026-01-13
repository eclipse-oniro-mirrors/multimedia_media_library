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

#include "asset_accurate_refresh.h"
#include "directory_ex.h"
#include "lake_const.h"
#include "lake_file_utils.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_lake_notify_info.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdb_transaction.h"
#include "medialibrary_unistore_manager.h"
#include "metadata.h"
#include "result_set_utils.h"
#include "thumbnail_service.h"

using namespace OHOS::NativeRdb;
namespace OHOS::Media {
const std::string PATH_HIDDEN_PREFIX = ".";

std::atomic<uint32_t> FileParser::imageNumber_{0};
std::atomic<uint32_t> FileParser::videoNumber_{0};

bool FileParser::PhotosRowData::IsExist()
{
    return fileId > 0;
}

std::string FileParser::PhotosRowData::ToString() const
{
    std::stringstream ss;
    ss << "PhotosRowData["
        << "fileId: " << fileId << ", "
        << "mediaType: " << mediaType << ", "
        << "fileSourceType: " << fileSourceType << ", "
        << "size: " << size << ", "
        << "dateModified: " << dateModified << ", "
        << "dateTaken: " << dateTaken << ", "
        << "inode: " << inode << ", "
        << "mimeType: " << mimeType << ", "
        << "storagePath: " << LakeFileUtils::GarbleFilePath(storagePath) << ", "
        << "ownerAlbumId: " << ownerAlbumId << ", "
        << "ownerPackage: " << LakeFileUtils::GarbleFile(ownerPackage) << ", "
        << "packageName: " << LakeFileUtils::GarbleFile(packageName) << ", "
        << "data: " << LakeFileUtils::GarbleFilePath(data) << "]";
    return ss.str();
}

bool FileParser::MetaStatus::IsChanged() const
{
    return isMediaTypeChanged || isSizeChanged || isDateModifiedChanged || isMimeTypeChanged || isStoragePathChanged;
}

std::string FileParser::MetaStatus::ToString() const
{
    std::stringstream ss;
    ss << "MetaStatus["
        << "isMediaTypeChanged: " << isMediaTypeChanged << ", "
        << "isSizeChanged: " << isSizeChanged << ", "
        << "isDateModifiedChanged: " << isDateModifiedChanged << ", "
        << "isMimeTypeChanged: " << isMimeTypeChanged  << ", "
        << "isStoragePathChanged: " << isStoragePathChanged  << "]";
    return ss.str();
}

FileParser::FileParser(const std::string &path, LakeScanMode scanMode) : scanMode_(scanMode)
{
    path_ = path;
    mediaLibraryRdb_ = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ParseFileInfo();
    MEDIA_INFO_LOG("FileParser: The info is %{public}s", PrintInfo(fileInfo_).c_str());
}

FileParser::FileParser(const MediaLakeNotifyInfo &notifyInfo, LakeScanMode scanMode)
    : notifyInfo_(notifyInfo), scanMode_(scanMode)
{
    MEDIA_INFO_LOG("Get notifyInfo obj: %{public}d, opt: %{public}d, before: %{public}s, after: %{public}s",
        static_cast<int32_t>(notifyInfo.objType),
        static_cast<int32_t>(notifyInfo.optType),
        LakeFileUtils::GarbleFilePath(notifyInfo.beforePath).c_str(),
        LakeFileUtils::GarbleFilePath(notifyInfo.afterPath).c_str());
    path_ = notifyInfo.afterPath;
    mediaLibraryRdb_ = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ParseFileInfo();
    MEDIA_INFO_LOG("FileParser: The info is %{public}s", PrintInfo(fileInfo_).c_str());
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
    return !MediaFileUtils::StartsWith(fileInfo_.displayName, PATH_HIDDEN_PREFIX);
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

int32_t FileParser::GetUniqueId()
{
    int32_t uniqueId = 0;
    switch (fileInfo_.fileType) {
        case static_cast<int32_t>(OuterMediaType::PICTURE): {
            uniqueId = static_cast<int32_t>(imageNumber_.fetch_add(1));
            break;
        }
        case static_cast<int32_t>(OuterMediaType::VIDEO): {
            uniqueId = static_cast<int32_t>(videoNumber_.fetch_add(1));
            break;
        }
        default:
            MEDIA_ERR_LOG("FileParser: Unsupported file type: %{public}d", fileInfo_.fileType);
    }
    return uniqueId;
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
       << "filePath: " << LakeFileUtils::GarbleFilePath(info.filePath) << ", "
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
       << "cloudPath: " << LakeFileUtils::GarbleFilePath(info.cloudPath) << ", "
       << "dateDay: " << info.dateDay << ", "
       << "dateMonth: " << info.dateMonth << ", "
       << "dateYear: " << info.dateYear << ", "
       << "displayName: " << LakeFileUtils::GarbleFile(info.displayName) << ", "
       << "fileSourceType: " << info.fileSourceType << ", "
       << "frontCamera: " << info.frontCamera << ", "
       << "mediaSuffix: " << info.mediaSuffix << ", "
       << "packageName: " << info.packageName << ", "
       << "shootingMode: " << info.shootingMode << ", "
       << "shootingModeTag: " << info.shootingModeTag << ", "
       << "title: " << LakeFileUtils::GarbleFile(info.title) << ", "
       << "userComment: " << info.userComment << ", "
       << "dynamicRangeType: " << info.dynamicRangeType << ", "
       << "ownerAlbumId: " << info.ownerAlbumId << ", "
       << "strongAssociation: " << info.strongAssociation << ", "
       << "lastVisitTime: " << info.lastVisitTime << ", "
       << "latitude: " << info.latitude << ", "
       << "longitude: " << info.longitude << ", "
       << "needInsert: " << (info.needInsert ? "true" : "false")
       << "}";
    return ss.str();
}

void FileParser::ParseFileInfo()
{
    struct stat statInfo;
    if (lstat(path_.c_str(), &statInfo) == -1) {
        MEDIA_ERR_LOG("FileParser:Failed to get info of path %{public}s, errno: %{public}d",
            LakeFileUtils::GarbleFilePath(path_).c_str(), errno);
        return;
    }
    fileInfo_.inode = std::to_string(statInfo.st_ino);
    fileInfo_.displayName = ExtractFileName(path_);
    fileInfo_.fileType = MediaFileUtils::GetMediaType(fileInfo_.displayName);
    fileInfo_.fileSize = statInfo.st_size;
    fileInfo_.dateModified = MediaFileUtils::Timespec2Millisecond(statInfo.st_mtim);
    fileInfo_.filePath = path_;
    fileInfo_.title = LakeFileUtils::GetFileTitle(fileInfo_.displayName);
    fileInfo_.isBurst = CheckBurst(fileInfo_.displayName);
    LakeFileUtils::SetBurstKey(fileInfo_);
    fileInfo_.subtype = LakeFileUtils::FindSubtype(fileInfo_);
    if (fileInfo_.fileType == MediaType::MEDIA_TYPE_IMAGE) {
        std::regex pattern(R"(.*_enhanced(\.[^.]+)$)");
        if (std::regex_match(path_, pattern)) {
            MEDIA_INFO_LOG("FileParser: %{public}s is an enhanced image!",
                LakeFileUtils::GarbleFilePath(path_).c_str());
            fileInfo_.strongAssociation = STRONG_ASSOCIATION_ENABLE;
            fileInfo_.ceAvailable = CLOUD_ENHANCEMENT_PHOTO;
        }
    }
    std::unique_ptr<Metadata> data = make_unique<Metadata>();
    data->SetFilePath(fileInfo_.filePath);
    data->SetFileMediaType(fileInfo_.fileType);
    data->SetFileDateModified(fileInfo_.dateModified);
    data->SetFileName(fileInfo_.displayName);
    if (LakeFileUtils::FillMetadata(data) != E_OK) {
        fileInfo_.needInsert = false;
        MEDIA_ERR_LOG("FileParser: Failed to get data: %{public}s", ToString().c_str());
        return;
    }
    fileInfo_.mimeType = data->GetFileMimeType();
    fileInfo_.mediaSuffix = data->GetFileExtension();
    fileInfo_.dateModified = data->GetFileDateModified();
    fileInfo_.dateTaken = data->GetDateTaken();
    fileInfo_.dateAdded = fileInfo_.dateTaken;
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
    fileInfo_.fileSourceType = FileSourceType::MEDIA_HO_LAKE;
}

bool FileParser::HasChangePart(const FileParser::PhotosRowData &rowData)
{
    metaStatus_.isMediaTypeChanged = rowData.mediaType != fileInfo_.fileType;
    metaStatus_.isSizeChanged = rowData.size != fileInfo_.fileSize;
    metaStatus_.isDateModifiedChanged = rowData.dateModified != fileInfo_.dateModified;
    metaStatus_.isMimeTypeChanged = rowData.mimeType != fileInfo_.mimeType;
    metaStatus_.isStoragePathChanged = rowData.storagePath != fileInfo_.filePath;
    bool ret = metaStatus_.IsChanged();
    CHECK_AND_EXECUTE(!ret, MEDIA_INFO_LOG("metaStatus: %{public}s, fileInfo: %{public}s",
        metaStatus_.ToString().c_str(), ToString().c_str()));
    return ret;
}

bool FileParser::IsStoragePathChanged(const FileParser::PhotosRowData &rowData)
{
    bool ret = rowData.storagePath != fileInfo_.filePath;
    CHECK_AND_EXECUTE(!ret, MEDIA_INFO_LOG("storagePath before: %{public}s, after: %{public}s",
        LakeFileUtils::GarbleFilePath(rowData.storagePath).c_str(),
        LakeFileUtils::GarbleFilePath(fileInfo_.filePath).c_str()));
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
    MEDIA_INFO_LOG("FindSameFileByOptAdd %{public}s", LakeFileUtils::GarbleFilePath(notifyInfo_.afterPath).c_str());
    return FindSameFileByStoragePath(notifyInfo_.afterPath);
}

// 更新消息通知场景，按beforePath和afterPath查找
//  1. beforePath和afterPath都不存在：按新增处理
//  2. beforePath和afterPath存在一个：按更新判断
//  3. beforePath和afterPath都存在：报错，按afterPath更新判断
FileParser::PhotosRowData FileParser::FindSameFileByOptMod()
{
    MEDIA_INFO_LOG("FindSameFileByOptMod %{public}s and %{public}s",
        LakeFileUtils::GarbleFilePath(notifyInfo_.beforePath).c_str(),
        LakeFileUtils::GarbleFilePath(notifyInfo_.afterPath).c_str());
    FileParser::PhotosRowData rowDataBefore = FindSameFileByStoragePath(notifyInfo_.beforePath);
    FileParser::PhotosRowData rowDataAfter = FindSameFileByStoragePath(notifyInfo_.afterPath);
    if (!rowDataBefore.IsExist() && !rowDataAfter.IsExist()) {
        return FileParser::PhotosRowData();
    }
    if (rowDataBefore.IsExist() && rowDataAfter.IsExist()) {
        MEDIA_ERR_LOG("[Error]Both beforePath and afterPath records are found, use afterPath %{public}s",
            LakeFileUtils::GarbleFilePath(notifyInfo_.afterPath).c_str());
        return rowDataAfter;
    }
    return rowDataBefore.IsExist() ? rowDataBefore : rowDataAfter;
}

// 默认场景（非新增/更新消息通知，如首次加载），按当前路径查找
FileParser::PhotosRowData FileParser::FindSameFileByDefault()
{
    MEDIA_INFO_LOG("FindSameFileByDefault %{public}s", LakeFileUtils::GarbleFilePath(path_).c_str());
    return FindSameFileByStoragePath(path_);
}

FileParser::PhotosRowData FileParser::FindSameFileByStoragePath(const std::string &storagePath)
{
    const int32_t NOT_TRASHED = 0;
    const int32_t NOT_HIDDEN = 0;
    FileParser::PhotosRowData rowData;
    CHECK_AND_RETURN_RET_LOG(!storagePath.empty(), rowData, "storagePath is empty");
    std::vector<NativeRdb::ValueObject> params = { storagePath, FileSourceType::MEDIA_HO_LAKE, FileSourceType::MEDIA,
        static_cast<int32_t>(PhotoPositionType::LOCAL), static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD),
        NOT_TRASHED, NOT_HIDDEN };
    return FindSameFileInDatabase(SQL_PHOTOS_FIND_SAME_FILE_BY_STORAGE_PATH, params);
}

FileParser::PhotosRowData FileParser::FindSameFileInDatabase(const std::string &querySql,
    const std::vector<NativeRdb::ValueObject> &params)
{
    FileParser::PhotosRowData rowData;
    CHECK_AND_RETURN_RET_LOG(mediaLibraryRdb_ != nullptr, rowData, "mediaLibraryRdb_ is null.");
    auto resultSet = mediaLibraryRdb_->QuerySql(querySql, params);
    CHECK_AND_RETURN_RET(resultSet != nullptr, rowData);
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return rowData;
    }
    rowData.fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
    rowData.mediaType = GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet);
    rowData.fileSourceType = GetInt32Val(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, resultSet);
    rowData.size = GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet);
    rowData.dateModified = GetInt64Val(MediaColumn::MEDIA_DATE_MODIFIED, resultSet);
    rowData.inode = GetStringVal(PhotoColumn::PHOTO_FILE_INODE, resultSet);
    rowData.mimeType = GetStringVal(MediaColumn::MEDIA_MIME_TYPE, resultSet);
    rowData.storagePath = GetStringVal(PhotoColumn::PHOTO_STORAGE_PATH, resultSet);
    rowData.ownerAlbumId = GetInt32Val(PhotoColumn::PHOTO_OWNER_ALBUM_ID, resultSet);
    rowData.ownerPackage = GetStringVal(PhotoColumn::MEDIA_OWNER_PACKAGE, resultSet);
    rowData.packageName = GetStringVal(PhotoColumn::MEDIA_PACKAGE_NAME, resultSet);
    rowData.dateTaken = GetInt64Val(MediaColumn::MEDIA_DATE_TAKEN, resultSet);
    rowData.data = GetStringVal(PhotoColumn::MEDIA_FILE_PATH, resultSet);
    resultSet->Close();
    MEDIA_INFO_LOG("FileParser: rowData: %{public}s", rowData.ToString().c_str());
    return rowData;
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

// 返回当前数据是插入还是更新，还是不进行任何操作
// 包含判定是否有重复文件，以及判断重复文件是否有变更
FileUpdateType FileParser::GetFileUpdateType()
{
    if (!IsNotifyInfoValid()) {
        MEDIA_ERR_LOG("Invalid notifyInfo obj: %{public}d, opt: %{public}d, isBeforePathEmpty: %{public}d, "
            "isAfterPathEmpty: %{public}d",
            static_cast<int32_t>(notifyInfo_.objType), static_cast<int32_t>(notifyInfo_.optType),
            notifyInfo_.beforePath.empty(), notifyInfo_.afterPath.empty());
        return FileUpdateType::NO_CHANGE;
    }
    FileParser::PhotosRowData rowData = FindSameFile();
    if (!rowData.IsExist()) {
        updateType_ = FileUpdateType::INSERT;
    } else {
        SetByPhotosRowData(rowData);
        if (HasChangePart(rowData)) {
            updateType_ = IsStoragePathChanged(rowData) ? FileUpdateType::UPDATE_ALBUM : FileUpdateType::UPDATE;
        } else {
            updateType_ = FileUpdateType::NO_CHANGE;
        }
    }
    MEDIA_INFO_LOG("updateType: %{public}d, fileInfo: %{public}s", static_cast<int32_t>(updateType_),
        ToString().c_str());
    return updateType_;
}

int32_t FileParser::UpdateAssetInfo()
{
    int32_t errCode = UpdateAssetInDatabase();
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode,
        "UpdateAssetInDatabase failed, ret: %{public}d, fileInfo: %{public}s", errCode, ToString().c_str());
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
    SetCloudPath(GetUniqueId());
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

void FileParser::SetCloudPath(int32_t uniqueId)
{
    std::string cloudPath;
    int32_t errCode = LakeFileUtils::CreateAssetPathById(
        uniqueId,
        fileInfo_.fileType,
        MediaFileUtils::GetExtensionFromPath(fileInfo_.displayName),
        cloudPath);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("FileParser: CreateAssetPathById failed, errCode: %{public}d, fileInfo: %{public}s",
            errCode, ToString().c_str());
        fileInfo_.needInsert = false;
        return;
    }
    fileInfo_.cloudPath = cloudPath;
}

void FileParser::SetByPhotosRowData(const PhotosRowData &rowData)
{
    SetFileId(rowData.fileId);
    SetAlbumInfo(rowData.ownerAlbumId, rowData.ownerPackage, rowData.packageName);
    fileInfo_.cloudPath = rowData.data;
    fileInfo_.dateTaken = rowData.dateTaken;
}

NativeRdb::ValuesBucket FileParser::GetAssetInsertValues()
{
    NativeRdb::ValuesBucket values;
    if (!fileInfo_.needInsert) {
        return values;
    }
    values = GetAssetCommonValues();
    SetAssetBurstValues(values);
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
    return values;
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

    values.Put(MediaColumn::MEDIA_DATE_MODIFIED, fileInfo_.dateModified);
    values.Put(PhotoColumn::PHOTO_META_DATE_MODIFIED, MediaFileUtils::UTCTimeMilliSeconds());

    values.Put(MediaColumn::MEDIA_DURATION, fileInfo_.duration);
    values.Put(PhotoColumn::PHOTO_ORIENTATION, fileInfo_.orientation);
    values.Put(PhotoColumn::PHOTO_HEIGHT, fileInfo_.height);
    values.Put(PhotoColumn::PHOTO_WIDTH, fileInfo_.width);
    double aspectRatio = MediaFileUtils::CalculateAspectRatio(fileInfo_.height, fileInfo_.width);
    values.PutDouble(PhotoColumn::PHOTO_ASPECT_RATIO, aspectRatio);
    values.Put(PhotoColumn::PHOTO_SUBTYPE, fileInfo_.subtype);
    PutStringVal(values, PhotoColumn::PHOTO_USER_COMMENT, fileInfo_.userComment);
    values.Put(PhotoColumn::PHOTO_ALL_EXIF, fileInfo_.allExif);
    values.Put(PhotoColumn::PHOTO_SHOOTING_MODE, fileInfo_.shootingMode);
    values.Put(PhotoColumn::PHOTO_SHOOTING_MODE_TAG, fileInfo_.shootingModeTag);
    values.Put(PhotoColumn::PHOTO_LAST_VISIT_TIME, fileInfo_.lastVisitTime);
    values.Put(PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE, fileInfo_.dynamicRangeType);
    values.Put(PhotoColumn::PHOTO_FRONT_CAMERA, fileInfo_.frontCamera);

    values.Put(PhotoColumn::PHOTO_FILE_INODE, fileInfo_.inode);
    values.Put(PhotoColumn::PHOTO_STORAGE_PATH, fileInfo_.filePath);
    values.Put(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, fileInfo_.fileSourceType);

    SetAssetAlbumValues(values);
    SetAssetCloudEnhancementValues(values);
    SetAssetLocationValues(values);

    return values;
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
        << "storagePath: " << LakeFileUtils::GarbleFilePath(fileInfo_.filePath) << ", "
        << "displayName: " << LakeFileUtils::GarbleFile(fileInfo_.displayName) << ", "
        << "size: " << fileInfo_.fileSize << ", "
        << "fileType: " << fileInfo_.fileType << ", "
        << "dateModified: " << fileInfo_.dateModified << ", "
        << "dateTaken: " << fileInfo_.dateTaken << ", "
        << "owner_album_id: " << fileInfo_.ownerAlbumId << ", "
        << "inode: " << fileInfo_.inode << "]";
    return ss.str();
}

std::vector<std::string> FileParser::GenerateThumbnail(LakeScanMode scanMode, const std::vector<std::string> &inodes)
{
    MEDIA_INFO_LOG("generate thumbnail");
    std::vector<std::string> uris;
    if (scanMode == LakeScanMode::FULL) {
        MEDIA_INFO_LOG("no need to generate thumbnail");
        return GetFileUris(inodes);
    }
    // 查找对应的fileId
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(PhotoColumn::PHOTO_FILE_INODE, inodes);
    for (const auto &inode : inodes) {
        MEDIA_INFO_LOG("generate thumbnail inode: %{public}s", inode.c_str());
    }
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, uris, "rdbStore is null.");
    auto resultSet = rdbStore->Query(predicates, {PhotoColumn::PHOTO_FILE_INODE, MediaColumn::MEDIA_ID,
        MediaColumn::MEDIA_DATE_MODIFIED, MediaColumn::MEDIA_NAME, MediaColumn::MEDIA_DATE_TAKEN,
        PhotoColumn::MEDIA_FILE_PATH});
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, uris, "resultSet is nullptr!");

    while (resultSet->GoToNextRow() == E_OK) {
        ThumbnailInfo info;
        string inode = GetStringVal(PhotoColumn::PHOTO_FILE_INODE, resultSet);
        info.fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        info.displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
        info.path = GetStringVal(PhotoColumn::MEDIA_FILE_PATH, resultSet);
        info.dateTaken = GetInt64Val(MediaColumn::MEDIA_DATE_TAKEN, resultSet);
        info.dateModified = GetInt64Val(MediaColumn::MEDIA_DATE_MODIFIED, resultSet);
        MEDIA_INFO_LOG("info[%{public}s]:%{public}d, %{public}s, %{public}s, %{public}" PRId64 ", %{public}" PRId64,
            inode.c_str(), info.fileId, LakeFileUtils::GarbleFile(info.displayName).c_str(),
            LakeFileUtils::GarbleFilePath(info.path).c_str(), info.dateTaken, info.dateModified);
        // 调用触发缩略图生成接口
        int32_t err = GenerateSingleThumbnail(info);
        CHECK_AND_PRINT_LOG(err == E_OK, "create thumbnail fail");

        std::string uri = MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX,
            to_string(info.fileId), MediaFileUtils::GetExtraUri(info.displayName, info.path));
        uris.push_back(uri);
    }
    resultSet->Close();
    return uris;
}

std::vector<std::string> FileParser::GetFileUris(const std::vector<std::string> &inodes)
{
    // 查找对应的fileId
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(PhotoColumn::PHOTO_FILE_INODE, inodes);
    for (auto inode : inodes) {
        MEDIA_INFO_LOG("generate thumbnail inode: %{public}s", inode.c_str());
    }
    std::vector<std::string> uris;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, uris, "rdbStore is null.");
    auto resultSet = rdbStore->Query(predicates, {PhotoColumn::PHOTO_FILE_INODE, MediaColumn::MEDIA_ID,
        MediaColumn::MEDIA_NAME, PhotoColumn::MEDIA_FILE_PATH});
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, uris, "resultSet is nullptr!");

    while (resultSet->GoToNextRow() == E_OK) {
        int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        std::string inode = GetStringVal(PhotoColumn::PHOTO_FILE_INODE, resultSet);
        std::string displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
        std::string path = GetStringVal(PhotoColumn::MEDIA_FILE_PATH, resultSet);
        std::string uri = MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX,
            to_string(fileId), MediaFileUtils::GetExtraUri(displayName, path));
        uris.push_back(uri);
    }
    resultSet->Close();
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
}  // namespace OHOS::Media