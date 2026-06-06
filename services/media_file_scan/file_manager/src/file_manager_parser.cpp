/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
#define MLOG_TAG "FileManagerParser"

#include "file_manager_parser.h"
#include "file_scanner.h"

#include "asset_accurate_refresh.h"
#include "media_file_utils.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_notify.h"
#include "moving_photo_file_utils.h"
#include "userfile_manager_types.h"
#include "thumbnail_service.h"
#include "media_column.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "photo_album_column.h"

using namespace OHOS::NativeRdb;
namespace OHOS::Media {
// LCOV_EXCL_START
FileManagerParser::FileManagerParser(const std::string &path, ScanMode scanMode)
    : FileParser(path, FileSourceType::FILE_MANAGER, scanMode)
{
    ParseFileInfo();
    MEDIA_INFO_LOG("FileManagerParser: The info is %{public}s", PrintInfo(fileInfo_).c_str());
}

FileManagerParser::FileManagerParser(const MediaNotifyInfo &notifyInfo, ScanMode scanMode)
    : FileParser(notifyInfo, FileSourceType::FILE_MANAGER, scanMode)
{
    ParseFileInfo();
    MEDIA_INFO_LOG("FileManagerParser: The info is %{public}s", PrintInfo(fileInfo_).c_str());
}

bool FileManagerParser::IsFileValidAsset()
{
    bool isTypeValid = CheckTypeValid();
    bool isSizeValid = CheckSizeValid();
    bool ret = isTypeValid && isSizeValid&& fileInfo_.needInsert;
    CHECK_AND_EXECUTE(ret, MEDIA_INFO_LOG("FileParser: isTypeValid: %{public}d, "
        "isSizeValid: %{public}d, needInsert: %{public}d, fileInfo: %{public}s",
        isTypeValid, isSizeValid, fileInfo_.needInsert, ToString().c_str()));
    return ret;
}

// 返回当前数据是插入还是更新，还是不进行任何操作
// 包含判定是否有重复文件，以及判断重复文件是否有变更
FileUpdateType FileManagerParser::GetFileUpdateType()
{
    if (!IsNotifyInfoValid()) {
        MEDIA_ERR_LOG("Invalid lake notifyInfo obj: %{public}d, opt: %{public}d, isBeforePathEmpty: %{public}d, "
            "isAfterPathEmpty: %{public}d",
            static_cast<int32_t>(notifyInfo_.objType), static_cast<int32_t>(notifyInfo_.optType),
            notifyInfo_.beforePath.empty(), notifyInfo_.afterPath.empty());
        return FileUpdateType::NO_CHANGE;
    }
    if (notifyInfo_.beforePath.find(FILE_MANAGER_TRASH_PATH) == 0 ||
        notifyInfo_.afterPath.find(FILE_MANAGER_TRASH_PATH) == 0) {
        return GetTrashAssetUpdateType();
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
    MEDIA_INFO_LOG("file manager file updateType: %{public}d, fileInfo: %{public}s", static_cast<int32_t>(updateType_),
        ToString().c_str());
    return updateType_;
}

void FileManagerParser::SetCloudPath()
{
    if (!fileInfo_.cloudPath.empty()) {
        MEDIA_ERR_LOG("File [%{public}s] has exists cloudPath", fileInfo_.cloudPath.c_str());
        return;
    }
    std::string cloudPath;
    std::shared_ptr<TransactionOperations> trans = make_shared<TransactionOperations>(__func__);
    std::function<int(void)> tryCreatePath = [&]()->int {
        int32_t uniqueId = MediaLibraryAssetOperations::CreateAssetUniqueId(fileInfo_.fileType, trans);
        return MediaLibraryAssetOperations::CreateAssetPathById(uniqueId, fileInfo_.fileType,
            MediaFileUtils::GetExtensionFromPath(fileInfo_.displayName), cloudPath);
    };
    int32_t errCode = trans->RetryTrans(tryCreatePath);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("FileParser: File Manager CreateAssetPathById failed, errCode: %{public}d, fileInfo: %{public}s",
            errCode, ToString().c_str());
        return;
    }
    fileInfo_.cloudPath = cloudPath;
}

FileUpdateType FileManagerParser::GetTrashAssetUpdateType()
{
    FileParser::PhotosRowData rowDataBefore = FindSameFileByStoragePath(notifyInfo_.beforePath);
    FileParser::PhotosRowData rowDataAfter = FindSameFileByStoragePath(notifyInfo_.afterPath);
    bool isTrash = notifyInfo_.beforePath.find(FILE_MANAGER_TRASH_PATH) != 0 &&
        notifyInfo_.afterPath.find(FILE_MANAGER_TRASH_PATH) == 0;
    bool isRecover = notifyInfo_.beforePath.find(FILE_MANAGER_TRASH_PATH) == 0 &&
        notifyInfo_.afterPath.find(FILE_MANAGER_TRASH_PATH) != 0;
    if (isTrash && rowDataBefore.IsExist()) {
        SetByPhotosRowData(rowDataBefore);
        updateType_ = FileUpdateType::TRASH;
    } else if (isRecover && rowDataAfter.IsExist()) {
        SetByPhotosRowData(rowDataAfter);
        updateType_ = FileUpdateType::RECOVER;
    } else if (isRecover && !rowDataAfter.IsExist()) {
        updateType_ = FileUpdateType::INSERT;
    } else {
        updateType_ = FileUpdateType::NO_CHANGE;
    }
    MEDIA_INFO_LOG("file manager trash file isTrash: %{public}d, isRecover: %{public}d, "
        "updateType: %{public}d, fileInfo: %{public}s", isTrash, isRecover,
        static_cast<int32_t>(updateType_), ToString().c_str());
    return updateType_;
}
    
void FileManagerParser::SetSubtypeFromMetadata(std::unique_ptr<Metadata> &data)
{
    // FileManager 对 subtype 不做限制，保留所有类型
    if (fileInfo_.subtype != static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)) {
        fileInfo_.subtype = data->GetPhotoSubType();
    }
}

void FileManagerParser::HandleTrashedLocalAndCloudAsset(NativeRdb::AbsRdbPredicates &predicates)
{
    int32_t deletedRows = -1;
    AccurateRefresh::AssetAccurateRefresh assetRefresh;
    int32_t errCode = assetRefresh.Delete(deletedRows, predicates);
    CHECK_AND_RETURN_LOG(errCode == NativeRdb::E_OK && deletedRows > 0,
        "HandleTrashedLocalAndCloudAsset failed, ret: %{public}d, changeRows: %{public}d, fileInfo: %{public}s",
        errCode, deletedRows, ToString().c_str());
    FileScanner::DeleteRelatedResource(fileInfo_);
    assetRefresh.RefreshAlbum();
    assetRefresh.Notify();
}

void FileManagerParser::UpdateTrashedAssetinfo()
{
    FileParser::PhotosRowData rowDataBefore = FindSameFileByStoragePath(notifyInfo_.beforePath);
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, rowDataBefore.fileId);

    auto watch = MediaLibraryNotify::GetInstance();

    if (rowDataBefore.position == static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD)) {
        // 处理端云合一图，置位为CLOUD
        HandleUpdateCloudAsset(predicates, PhotoPositionType::CLOUD);
        CHECK_AND_EXECUTE(watch == nullptr,
            watch->Notify(PhotoColumn::PHOTO_URI_PREFIX + to_string(rowDataBefore.fileId), NotifyType::NOTIFY_UPDATE));
    } else if (rowDataBefore.position == static_cast<int32_t>(PhotoPositionType::LOCAL)) {
        // 处理本地图
        HandleTrashedLocalAndCloudAsset(predicates);
        CHECK_AND_EXECUTE(watch == nullptr,
            watch->Notify(PhotoColumn::PHOTO_URI_PREFIX + to_string(rowDataBefore.fileId), NotifyType::NOTIFY_REMOVE));
    }

    std::string albumUri = PhotoAlbumColumns::ALBUM_URI_PREFIX + to_string(rowDataBefore.ownerAlbumId);
    CHECK_AND_EXECUTE(watch == nullptr, watch->Notify(albumUri, NotifyType::NOTIFY_UPDATE));
}

void FileManagerParser::UpdateRecoverAssetinfo()
{
    FileParser::PhotosRowData rowDataAfter = FindSameFileByStoragePath(notifyInfo_.afterPath);
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, rowDataAfter.fileId);

    auto watch = MediaLibraryNotify::GetInstance();

    if (rowDataAfter.position == static_cast<int32_t>(PhotoPositionType::CLOUD)) {
        // 处理纯云图
        HandleUpdateCloudAsset(predicates, PhotoPositionType::LOCAL_AND_CLOUD);
        CHECK_AND_EXECUTE(watch == nullptr,
            watch->Notify(PhotoColumn::PHOTO_URI_PREFIX + to_string(rowDataAfter.fileId), NotifyType::NOTIFY_UPDATE));
    } else if (rowDataAfter.position == static_cast<int32_t>(PhotoPositionType::LOCAL)) {
        // 处理本地图
        MEDIA_WARN_LOG("Database has exists the local asset record, fileInfo: %{public}s",
            ToString().c_str());
    }

    std::string albumUri = PhotoAlbumColumns::ALBUM_URI_PREFIX + to_string(rowDataAfter.ownerAlbumId);
    CHECK_AND_EXECUTE(watch == nullptr, watch->Notify(albumUri, NotifyType::NOTIFY_UPDATE));
}

void FileManagerParser::HandleUpdateCloudAsset(NativeRdb::AbsRdbPredicates &predicates,
    const PhotoPositionType &positionType)
{
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_POSITION, static_cast<int>(positionType));
    int32_t changedRows = 0;
    AccurateRefresh::AssetAccurateRefresh assetRefresh;
    int32_t errCode = assetRefresh.Update(changedRows, values, predicates);
    CHECK_AND_RETURN_LOG(errCode == NativeRdb::E_OK && changedRows > 0,
        "HandleUpdateCloudAsset failed, ret: %{public}d, changeRows: %{public}d", errCode, changedRows);
    assetRefresh.RefreshAlbum();
    assetRefresh.Notify();
}

// FileManager新增缩略图生成接口（调用ThumbnailService）
int32_t FileManagerParser::GenerateThumbnailForFileManager(const ThumbnailInfo &info)
{
    MEDIA_INFO_LOG("GenerateThumbnailForFileManager called, fileId: %{public}d", info.fileId);

    return ThumbnailService::GetInstance()->CreateThumbnailForFileManager(info);
}

// FileManager新增多文件缩略图生成接口（支持功耗管控）
std::vector<std::string> FileManagerParser::GenerateThumbnailWithPowerControl(ScanMode scanMode,
    const std::vector<std::string> &inodes)
{
    MEDIA_INFO_LOG("GenerateThumbnailWithPowerControl: generate thumbnail with power control");
    std::vector<std::string> uris;
    if (scanMode == ScanMode::FULL) {
        MEDIA_INFO_LOG("no need to generate thumbnail");
        return FileParser::GetFileUris(inodes);
    }

    // 查询数据库（复用父类的公共查询方法）
    std::vector<ThumbnailInfo> infos;
    std::vector<int32_t> thumbnailVisibleList;
    int32_t ret = FileParser::QueryThumbnailInfos(inodes, infos, thumbnailVisibleList);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, uris, "QueryThumbnailInfos failed");

    // 生成缩略图（支持功耗管控）
    for (size_t i = 0; i < infos.size(); ++i) {
        const auto& info = infos[i];
        int32_t thumbnailVisible = thumbnailVisibleList[i];

        MEDIA_INFO_LOG("info[%{public}zu]:%{public}d, thumbnailVisible: %{public}d",
            i, info.fileId, thumbnailVisible);

        // 检查thumbnail_visible
        if (thumbnailVisible == 1) {
            std::string uri = MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX,
                to_string(info.fileId), MediaFileUtils::GetExtraUri(info.displayName, info.path));
            uris.push_back(uri);
            continue;
        }

        // 调用支持功耗管控的缩略图生成接口
        int32_t err = GenerateThumbnailForFileManager(info);
        CHECK_AND_PRINT_LOG(err == E_OK, "create thumbnail with power control fail");

        std::string uri = MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX,
            to_string(info.fileId), MediaFileUtils::GetExtraUri(info.displayName, info.path));
        uris.push_back(uri);
    }
    return uris;
}
// LCOV_EXCL_STOP
}  // namespace OHOS::Media