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

#include <filesystem>

#include "file_scanner.h"

#include "asset_accurate_refresh.h"
#include "cloud_sync_manager.h"
#include "media_file_utils.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_notify.h"
#include "moving_photo_file_utils.h"
#include "userfile_manager_types.h"
#include "thumbnail_service.h"
#include "media_column.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_unistore_manager.h"
#include "photo_album_column.h"
#include "photo_dao.h"
#include "dfx_utils.h"

using namespace OHOS::NativeRdb;
namespace OHOS::Media {

const std::string DENTRY_INFO_FILE_TYPE_CONTENT = "CONTENT";

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
    bool isTypeValid = CheckTypeValidForFileManagerParser();
    bool isSizeValid = CheckSizeValid();
    // 与 lake 路径保持一致：以.开头的隐藏文件不入库（文管同步的.开头图片不允许入库）
    bool isNotHidden = CheckIsNotHidden();
    bool ret = isTypeValid && isSizeValid && isNotHidden && fileInfo_.needInsert;
    CHECK_AND_EXECUTE(ret, MEDIA_INFO_LOG("FileParser: isTypeValid: %{public}d, "
        "isSizeValid: %{public}d, isNotHidden: %{public}d, needInsert: %{public}d, fileInfo: %{public}s",
        isTypeValid, isSizeValid, isNotHidden, fileInfo_.needInsert, ToString().c_str()));
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
    // 检测点号重命名：文件名以.开头时释放本地，恢复为不以.开头时重新入库
    // 直接写入成员 updateType_，避免后续 SetAssetAlbumValues 基于成员状态判断时与返回的分类不一致
    if (GetDotRenameUpdateType(updateType_)) {
        return updateType_;
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

bool FileManagerParser::GetDotRenameUpdateType(FileUpdateType &updateType)
{
    if (IsDotRenameRelease()) {
        FileParser::PhotosRowData rowDataBefore = FindSameFileByStoragePath(notifyInfo_.beforePath);
        if (rowDataBefore.IsExist()) {
            SetByPhotosRowData(rowDataBefore);
            updateType = FileUpdateType::DOT_RENAME_RELEASE;
            MEDIA_INFO_LOG("dot rename release detected, beforePath: %{public}s",
                DfxUtils::GetSafePath(notifyInfo_.beforePath).c_str());
            return true;
        }
    }
    if (IsDotRenameRecover()) {
        FileParser::PhotosRowData rowDataAfter = FindSameFileByStoragePath(notifyInfo_.afterPath);
        if (rowDataAfter.IsExist()) {
            SetByPhotosRowData(rowDataAfter);
            updateType = FileUpdateType::DOT_RENAME_RECOVER;
            MEDIA_INFO_LOG("dot rename recover detected, afterPath: %{public}s",
                DfxUtils::GetSafePath(notifyInfo_.afterPath).c_str());
            return true;
        }
        // 纯云图恢复: afterPath 不以.开头, beforePath 以.开头, DB 中无记录但需重新入库融合
        updateType = FileUpdateType::INSERT;
        MEDIA_INFO_LOG("dot rename recover as insert, afterPath: %{public}s",
            DfxUtils::GetSafePath(notifyInfo_.afterPath).c_str());
        return true;
    }
    return false;
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

bool FileManagerParser::IsDotRenameRelease() const
{
    // beforePath 不以.开头, afterPath 以.开头 -> 释放本地
    return IsDotRenameReleaseLogic(notifyInfo_.beforePath, notifyInfo_.afterPath);
}

bool FileManagerParser::IsDotRenameRecover() const
{
    // beforePath 以.开头, afterPath 不以.开头 -> 恢复名称重新入库
    return IsDotRenameRecoverLogic(notifyInfo_.beforePath, notifyInfo_.afterPath);
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
    if (MovingPhotoFileUtils::IsLivePhoto(data->GetFilePath())) {
        fileInfo_.subtype = static_cast<int32_t>(PhotoSubType::MOVING_PHOTO);
        return;
    }
    fileInfo_.subtype = data->GetPhotoSubType();
}

int32_t FileManagerParser::HandleTrashedLocalAndCloudAsset(NativeRdb::AbsRdbPredicates &predicates)
{
    int32_t deletedRows = -1;
    AccurateRefresh::AssetAccurateRefresh assetRefresh;
    int32_t errCode = assetRefresh.Delete(deletedRows, predicates);
    CHECK_AND_RETURN_RET_LOG(
        errCode == NativeRdb::E_OK && deletedRows > 0, errCode == NativeRdb::E_OK ? E_FAIL : errCode,
        "HandleTrashedLocalAndCloudAsset failed, ret: %{public}d, changeRows: %{public}d, fileInfo: %{public}s",
        errCode, deletedRows, ToString().c_str());
    FileScanner::DeleteRelatedResource(fileInfo_);
    assetRefresh.RefreshAlbum();
    assetRefresh.Notify();
    return E_OK;
}

void FileManagerParser::UpdateTrashedAssetinfo()
{
    FileParser::PhotosRowData rowDataBefore = FindSameFileByStoragePath(notifyInfo_.beforePath);
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, rowDataBefore.fileId);

    auto watch = MediaLibraryNotify::GetInstance();
    bool assetChanged = false;

    if (rowDataBefore.position == static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD)) {
        // 处理端云合一图，创建dentry文件，置位position为CLOUD
        int32_t dentryRet = CreateDentryFileForCloudAsset(rowDataBefore);
        CHECK_AND_EXECUTE(dentryRet == E_OK,
            MEDIA_ERR_LOG("Create dentry for cloud asset failed, fileId: %{public}d, ret: %{public}d",
                rowDataBefore.fileId, dentryRet));
        assetChanged = (HandleUpdateCloudAsset(predicates, PhotoPositionType::CLOUD) == E_OK);
        CHECK_AND_EXECUTE(!assetChanged || watch == nullptr,
            watch->Notify(PhotoColumn::PHOTO_URI_PREFIX + to_string(rowDataBefore.fileId), NotifyType::NOTIFY_UPDATE));
    } else if (rowDataBefore.position == static_cast<int32_t>(PhotoPositionType::LOCAL)) {
        // 处理本地图
        assetChanged = (HandleTrashedLocalAndCloudAsset(predicates) == E_OK);
        CHECK_AND_EXECUTE(!assetChanged || watch == nullptr,
            watch->Notify(PhotoColumn::PHOTO_URI_PREFIX + to_string(rowDataBefore.fileId), NotifyType::NOTIFY_REMOVE));
    }

    std::string albumUri = PhotoAlbumColumns::ALBUM_URI_PREFIX + to_string(rowDataBefore.ownerAlbumId);
    CHECK_AND_EXECUTE(!assetChanged || watch == nullptr, watch->Notify(albumUri, NotifyType::NOTIFY_UPDATE));
}

void FileManagerParser::UpdateRecoverAssetinfo()
{
    FileParser::PhotosRowData rowDataAfter = FindSameFileByStoragePath(notifyInfo_.afterPath);
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, rowDataAfter.fileId);

    auto watch = MediaLibraryNotify::GetInstance();
    bool assetChanged = false;

    if (rowDataAfter.position == static_cast<int32_t>(PhotoPositionType::CLOUD)) {
        // 处理纯云图
        assetChanged = (HandleUpdateCloudAsset(predicates, PhotoPositionType::LOCAL_AND_CLOUD) == E_OK);
        CHECK_AND_EXECUTE(!assetChanged || watch == nullptr,
            watch->Notify(PhotoColumn::PHOTO_URI_PREFIX + to_string(rowDataAfter.fileId), NotifyType::NOTIFY_UPDATE));
    } else if (rowDataAfter.position == static_cast<int32_t>(PhotoPositionType::LOCAL)) {
        // 处理本地图
        MEDIA_WARN_LOG("Database has exists the local asset record, fileInfo: %{public}s",
            ToString().c_str());
    }

    std::string albumUri = PhotoAlbumColumns::ALBUM_URI_PREFIX + to_string(rowDataAfter.ownerAlbumId);
    CHECK_AND_EXECUTE(!assetChanged || watch == nullptr, watch->Notify(albumUri, NotifyType::NOTIFY_UPDATE));
}

int32_t FileManagerParser::HandleUpdateCloudAsset(NativeRdb::AbsRdbPredicates &predicates,
    const PhotoPositionType &positionType)
{
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_POSITION, static_cast<int>(positionType));
    int32_t changedRows = 0;
    AccurateRefresh::AssetAccurateRefresh assetRefresh;
    int32_t errCode = assetRefresh.Update(changedRows, values, predicates);
    CHECK_AND_RETURN_RET_LOG(errCode == NativeRdb::E_OK && changedRows > 0,
        errCode == NativeRdb::E_OK ? E_FAIL : errCode,
        "HandleUpdateCloudAsset failed, ret: %{public}d, changeRows: %{public}d", errCode, changedRows);
    assetRefresh.RefreshAlbum();
    assetRefresh.Notify();
    return E_OK;
}

int32_t FileManagerParser::BatchCreateDentryFiles(
    const std::vector<FileManagement::CloudSync::DentryFileInfo> &dentryInfoList)
{
    CHECK_AND_RETURN_RET_LOG(!dentryInfoList.empty(), E_FAIL, "dentryInfoList is empty");

    std::vector<std::string> failCloudIdList;
    int32_t ret = FileManagement::CloudSync::CloudSyncManager::GetInstance().BatchDentryFileInsert(
        dentryInfoList, failCloudIdList);
    if (ret != E_OK || !failCloudIdList.empty()) {
        MEDIA_ERR_LOG("BatchDentryFileInsert failed, ret: %{public}d, count: %{public}zu, failSize: %{public}zu",
            ret, dentryInfoList.size(), failCloudIdList.size());
        for (const auto &cloudId : failCloudIdList) {
            MEDIA_ERR_LOG("BatchDentryFileInsert failed cloudId: %{private}s", cloudId.c_str());
        }
        return ret == E_OK ? E_FAIL : ret;
    }
    MEDIA_INFO_LOG("BatchDentryFileInsert ret: %{public}d, count: %{public}zu, failSize: %{public}zu",
        ret, dentryInfoList.size(), failCloudIdList.size());
    return E_OK;
}

int32_t FileManagerParser::CreateDentryFilesByFileIds(const std::vector<std::string> &fileIds)
{
    CHECK_AND_RETURN_RET_LOG(!fileIds.empty(), E_FAIL, "fileIds is empty");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_FAIL, "rdbStore is nullptr");

    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(MediaColumn::MEDIA_ID, fileIds);
    auto resultSet = rdbStore->Query(predicates,
        { PhotoColumn::PHOTO_CLOUD_ID, MediaColumn::MEDIA_DATE_MODIFIED,
          MediaColumn::MEDIA_SIZE, MediaColumn::MEDIA_FILE_PATH, MediaColumn::MEDIA_NAME });
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_FAIL, "resultSet is nullptr");

    std::vector<FileManagement::CloudSync::DentryFileInfo> dentryInfoList;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        FileManagement::CloudSync::DentryFileInfo dentryInfo;
        dentryInfo.cloudId = GetStringVal(PhotoColumn::PHOTO_CLOUD_ID, resultSet);
        dentryInfo.modifiedTime = GetInt64Val(MediaColumn::MEDIA_DATE_MODIFIED, resultSet);
        dentryInfo.size = GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet);
        dentryInfo.path = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
        dentryInfo.fileName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
        dentryInfo.fileType = DENTRY_INFO_FILE_TYPE_CONTENT;
        if (!dentryInfo.cloudId.empty()) {
            dentryInfoList.push_back(dentryInfo);
        }
    }
    resultSet->Close();
    int32_t ret = BatchCreateDentryFiles(dentryInfoList);
    CHECK_AND_EXECUTE(ret == E_OK, MEDIA_ERR_LOG("Create dentry files by fileIds failed, ret: %{public}d", ret));
    return ret;
}

int32_t FileManagerParser::CreateDentryFileForCloudAsset(
    const FileParser::PhotosRowData &rowData)
{
    CHECK_AND_RETURN_RET_LOG(!rowData.cloudId.empty(), E_FAIL, "cloudId is empty, fileId: %{public}d", rowData.fileId);

    FileManagement::CloudSync::DentryFileInfo dentryInfo;
    dentryInfo.cloudId = rowData.cloudId;
    dentryInfo.modifiedTime = rowData.dateModified;
    dentryInfo.fileType = DENTRY_INFO_FILE_TYPE_CONTENT;
    dentryInfo.size = rowData.size;
    dentryInfo.path = rowData.data;
    dentryInfo.fileName = rowData.displayName;

    return BatchCreateDentryFiles({ dentryInfo });
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

    // 查询数据库（复用 PhotoDao 公共查询方法）
    std::vector<ThumbnailInfo> infos;
    std::vector<int32_t> thumbnailVisibleList;
    int32_t ret = PhotoDao().QueryThumbnailInfos(inodes, infos, thumbnailVisibleList);
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

bool FileManagerParser::CheckTypeValidForFileManagerParser()
{
    return MediaFileUtils::IsSupportedMediaExtension(fileInfo_.displayName);
}
// LCOV_EXCL_STOP
}  // namespace OHOS::Media