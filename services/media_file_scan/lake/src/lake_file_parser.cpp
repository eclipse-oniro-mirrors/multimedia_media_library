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
#define MLOG_TAG "LakeFileParser"

#include "lake_file_parser.h"

#include <filesystem>

#include "file_scan_utils.h"
#include "file_scanner.h"
#include "media_file_utils.h"
#include "photo_album_column.h"
#include "medialibrary_notify.h"
#include "asset_accurate_refresh.h"
#include "cloud_sync_manager.h"
#include "media_column.h"
#include "dfx_utils.h"

using namespace OHOS::NativeRdb;
namespace OHOS::Media {

static const std::string DENTRY_INFO_FILE_TYPE_CONTENT = "CONTENT";

std::atomic<uint32_t> LakeFileParser::imageNumber_{0};
std::atomic<uint32_t> LakeFileParser::videoNumber_{0};

LakeFileParser::LakeFileParser(const std::string &path, ScanMode scanMode)
    : FileParser(path, FileSourceType::MEDIA_HO_LAKE, scanMode)
{
    ParseFileInfo();
    MEDIA_INFO_LOG("LakeFileParser: The info is %{public}s", PrintInfo(fileInfo_).c_str());
}

LakeFileParser::LakeFileParser(const MediaNotifyInfo &notifyInfo, ScanMode scanMode)
    : FileParser(notifyInfo, FileSourceType::MEDIA_HO_LAKE, scanMode)
{
    ParseFileInfo();
    MEDIA_INFO_LOG("LakeFileParser: The info is %{public}s", PrintInfo(fileInfo_).c_str());
}
// 返回当前数据是插入还是更新，还是不进行任何操作
// 包含判定是否有重复文件，以及判断重复文件是否有变更
FileUpdateType LakeFileParser::GetFileUpdateType()
{
    if (!IsNotifyInfoValid()) {
        MEDIA_ERR_LOG("Invalid lake notifyInfo obj: %{public}d, opt: %{public}d, isBeforePathEmpty: %{public}d, "
            "isAfterPathEmpty: %{public}d",
            static_cast<int32_t>(notifyInfo_.objType), static_cast<int32_t>(notifyInfo_.optType),
            notifyInfo_.beforePath.empty(), notifyInfo_.afterPath.empty());
        return FileUpdateType::NO_CHANGE;
    }
    // 检测点号重命名：文件名以.开头时释放本地，恢复为不以.开头时重新入库
    if (IsDotRenameRelease()) {
        FileParser::PhotosRowData rowDataBefore = FindSameFileByStoragePath(notifyInfo_.beforePath);
        if (rowDataBefore.IsExist()) {
            SetByPhotosRowData(rowDataBefore);
            updateType_ = FileUpdateType::DOT_RENAME_RELEASE;
            MEDIA_INFO_LOG("lake dot rename release detected, beforePath: %{public}s",
                DfxUtils::GetSafePath(notifyInfo_.beforePath).c_str());
            return updateType_;
        }
    }
    if (IsDotRenameRecover()) {
        FileParser::PhotosRowData rowDataAfter = FindSameFileByStoragePath(notifyInfo_.afterPath);
        if (rowDataAfter.IsExist()) {
            SetByPhotosRowData(rowDataAfter);
            updateType_ = FileUpdateType::DOT_RENAME_RECOVER;
            MEDIA_INFO_LOG("lake dot rename recover detected, afterPath: %{public}s",
                DfxUtils::GetSafePath(notifyInfo_.afterPath).c_str());
            return updateType_;
        }
        updateType_ = FileUpdateType::INSERT;
        MEDIA_INFO_LOG("lake dot rename recover as insert, afterPath: %{public}s",
            DfxUtils::GetSafePath(notifyInfo_.afterPath).c_str());
        return updateType_;
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
    MEDIA_INFO_LOG("lake file updateType: %{public}d, fileInfo: %{public}s", static_cast<int32_t>(updateType_),
        ToString().c_str());
    return updateType_;
}

bool LakeFileParser::IsDotRenameRelease() const
{
    // beforePath 不以.开头, afterPath 以.开头 -> 释放本地
    return IsDotRenameReleaseLogic(notifyInfo_.beforePath, notifyInfo_.afterPath);
}

bool LakeFileParser::IsDotRenameRecover() const
{
    // beforePath 以.开头, afterPath 不以.开头 -> 恢复名称重新入库
    return IsDotRenameRecoverLogic(notifyInfo_.beforePath, notifyInfo_.afterPath);
}

// 桶目录修改
void LakeFileParser::SetCloudPath()
{
    std::string cloudPath;
    int32_t uniqueId = GetUniqueId();
    int32_t errCode = FileScanUtils::CreateAssetPathById(
        uniqueId, fileInfo_.fileType, MediaFileUtils::GetExtensionFromPath(fileInfo_.displayName), cloudPath);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("FileParser: Lake CreateAssetPathById failed, errCode: %{public}d, fileInfo: %{public}s",
            errCode, ToString().c_str());
        fileInfo_.needInsert = false;
        return;
    }
    fileInfo_.cloudPath = cloudPath;
}

int32_t LakeFileParser::GetUniqueId()
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

void LakeFileParser::SetSubtypeFromMetadata(std::unique_ptr<Metadata> &data)
{
    if (data->GetPhotoSubType() == static_cast<int32_t>(PhotoSubType::SPATIAL_3DGS) ||
        data->GetPhotoSubType() == static_cast<int32_t>(PhotoSubType::SLOW_MOTION_VIDEO)) {
        fileInfo_.subtype = data->GetPhotoSubType();
    }
}

void LakeFileParser::UpdateTrashedAssetInfo()
{
    FileParser::PhotosRowData rowDataBefore = FindSameFileByStoragePath(notifyInfo_.beforePath);
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, rowDataBefore.fileId);

    auto watch = MediaLibraryNotify::GetInstance();
    bool assetChanged = false;

    if (rowDataBefore.position == static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD)) {
        // 端云合一图：仅释放本地，创建 dentry 文件并置位 position 为 CLOUD
        int32_t dentryRet = CreateDentryFileForCloudAsset(rowDataBefore);
        CHECK_AND_EXECUTE(dentryRet == E_OK,
            MEDIA_ERR_LOG("Create dentry for cloud asset failed, fileId: %{public}d, ret: %{public}d",
                rowDataBefore.fileId, dentryRet));
        assetChanged = (HandleUpdateCloudAsset(predicates, PhotoPositionType::CLOUD) == E_OK);
        CHECK_AND_EXECUTE(!assetChanged || watch == nullptr,
            watch->Notify(PhotoColumn::PHOTO_URI_PREFIX + to_string(rowDataBefore.fileId), NotifyType::NOTIFY_UPDATE));
    } else if (rowDataBefore.position == static_cast<int32_t>(PhotoPositionType::LOCAL)) {
        // 仅本地图：删除记录（不删除东湖图片），恢复名称后可重新入库
        assetChanged = (HandleTrashedLocalAndCloudAsset(predicates) == E_OK);
        CHECK_AND_EXECUTE(!assetChanged || watch == nullptr,
            watch->Notify(PhotoColumn::PHOTO_URI_PREFIX + to_string(rowDataBefore.fileId), NotifyType::NOTIFY_REMOVE));
    }

    std::string albumUri = PhotoAlbumColumns::ALBUM_URI_PREFIX + to_string(rowDataBefore.ownerAlbumId);
    CHECK_AND_EXECUTE(!assetChanged || watch == nullptr, watch->Notify(albumUri, NotifyType::NOTIFY_UPDATE));
}

int32_t LakeFileParser::HandleTrashedLocalAndCloudAsset(NativeRdb::AbsRdbPredicates &predicates)
{
    int32_t deletedRows = -1;
    AccurateRefresh::AssetAccurateRefresh assetRefresh;
    int32_t errCode = assetRefresh.Delete(deletedRows, predicates);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK && deletedRows > 0, errCode == E_OK ? E_FAIL : errCode,
        "HandleTrashedLocalAndCloudAsset failed, ret: %{public}d, changeRows: %{public}d, fileInfo: %{public}s",
        errCode, deletedRows, ToString().c_str());
    FileScanner::DeleteRelatedResource(fileInfo_);
    assetRefresh.RefreshAlbum();
    assetRefresh.Notify();
    return E_OK;
}

int32_t LakeFileParser::CreateDentryFileForCloudAsset(const FileParser::PhotosRowData &rowData)
{
    CHECK_AND_RETURN_RET_LOG(!rowData.cloudId.empty(), E_FAIL, "cloudId is empty, fileId: %{public}d", rowData.fileId);

    FileManagement::CloudSync::DentryFileInfo dentryInfo;
    dentryInfo.cloudId = rowData.cloudId;
    dentryInfo.modifiedTime = rowData.dateModified;
    dentryInfo.fileType = DENTRY_INFO_FILE_TYPE_CONTENT;
    dentryInfo.size = rowData.size;
    dentryInfo.path = rowData.data;
    dentryInfo.fileName = rowData.displayName;

    std::vector<FileManagement::CloudSync::DentryFileInfo> dentryInfoList = { dentryInfo };
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

void LakeFileParser::UpdateRecoverAssetInfo()
{
    FileParser::PhotosRowData rowDataAfter = FindSameFileByStoragePath(notifyInfo_.afterPath);
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, rowDataAfter.fileId);

    auto watch = MediaLibraryNotify::GetInstance();
    bool assetChanged = false;

    if (rowDataAfter.position == static_cast<int32_t>(PhotoPositionType::CLOUD)) {
        assetChanged = (HandleUpdateCloudAsset(predicates, PhotoPositionType::LOCAL_AND_CLOUD) == E_OK);
        CHECK_AND_EXECUTE(!assetChanged || watch == nullptr,
            watch->Notify(PhotoColumn::PHOTO_URI_PREFIX + to_string(rowDataAfter.fileId), NotifyType::NOTIFY_UPDATE));
    } else if (rowDataAfter.position == static_cast<int32_t>(PhotoPositionType::LOCAL)) {
        MEDIA_WARN_LOG("Database has exists the local asset record, fileId: %{public}s", ToString().c_str());
    }

    std::string albumUri = PhotoAlbumColumns::ALBUM_URI_PREFIX + to_string(rowDataAfter.ownerAlbumId);
    CHECK_AND_EXECUTE(!assetChanged || watch == nullptr, watch->Notify(albumUri, NotifyType::NOTIFY_UPDATE));
}

int32_t LakeFileParser::HandleUpdateCloudAsset(NativeRdb::AbsRdbPredicates &predicates,
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
// LCOV_EXCL_STOP
}  // namespace OHOS::Media