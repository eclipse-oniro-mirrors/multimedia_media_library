/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "Media_Service"

#include "cloud_media_file_info_service.h"

#include "media_log.h"
#include "cloud_media_sync_utils.h"
#include "medialibrary_db_const.h"
#include "media_string_utils.h"
#include "cloud_media_sync_const.h"
#include "media_file_utils.h"

namespace OHOS::Media::CloudSync {
/**
 * 根据跨端同步的数据特点，修正 fileSourceType 和 storagePath 字段的值
 * @return 修正后的 fileSourceType 和 storagePath，可直接新增入库；
 *
 * 数据场景（TO-BE）：
 *
 * | sourcePath          | hidden | trashed | fileSourceType       | storagePath |
 * |---------------------|--------|---------|----------------------|--------------------|
 * | lPath前缀 /FromDocs/ | 0      | 0       | 修正：FILE_MANAGER(1) | 修正：./Docs/{lPath}/{displayName} |
 * | lPath前缀 /FromDocs/ | 1      | 0       | 修正：MEDIA(0)        | 修正："" (文件不存储在storagePath路径) |
 * | lPath前缀 /FromDocs/ | 0      | 1       | 修正：MEDIA(0)        | 修正："" (文件不存储在storagePath路径) |
 * | lPath前缀 /FromDocs/ | 1      | 1       | 修正：MEDIA(0)        | 修正："" (文件不存储在storagePath路径) |
 * | 其他                 | 0 or 1 | 0 or 1  | 修正：MEDIA(0)        | 修正："" (重置为""，避免错误使用) |
 */
void CloudMediaFileInfoService::FixFileInfoWithCloudOnly(CloudMediaPullDataDto &pullData)
{
    // fileSourceType = MEDIA(0) 作为默认值，storagePath = "" 作为默认值
    pullData.attributesFileSourceType = static_cast<int32_t>(FileSourceType::MEDIA);
    pullData.attributesStoragePath = "";

    // 隐藏或回收站的文件，均视为媒体文件，且不存储在storagePath路径下；
    const bool isHidden = pullData.IsHiddenAsset();
    const bool isTrashed = pullData.basicRecycledTime != 0;
    const bool isHiddenOrTrashed = isHidden || isTrashed;
    CHECK_AND_RETURN(!isHiddenOrTrashed);

    // fileSourceType = FILE_MANAGER(1) 的修正规则：sourcePath的lPath前缀为 /FromDocs/（文管相册路径）
    const std::string lPath = CloudMediaSyncUtils::GetLpath(pullData);
    const bool isFromDocs = !lPath.empty() && CloudMediaSyncUtils::IsFileManagerAlbumPath(lPath);
    CHECK_AND_RETURN(isFromDocs);

    // 满足以下所有条件，修正 fileSourceType 为 FILE_MANAGER(1)，其他数据场景，均修正为 MEDIA(0)；
    // 1. sourcePath的lPath前缀为 /FromDocs/（文管相册路径），满足文管文件的特征；
    // 2. 文件未隐藏（hidden=0），且未被回收（dateTrashed=0）；
    pullData.attributesFileSourceType = static_cast<int32_t>(FileSourceType::FILE_MANAGER);

    // 仅当 fileSourceType = FILE_MANAGER(1) 时，才修正 storagePath 为文管文件的storagePath；
    // 其他数据场景，保持 storagePath = ""，文件不存储在storagePath路径；
    const std::string displayName = pullData.basicDisplayName;
    // 文管 storagePath 的格式为：/storage/media/local/files/Docs/{lPath}/{displayName}
    const std::string lPathWithoutPrefix = CloudMediaSyncUtils::GetLpathWithoutDocPrefix(lPath);
    std::string storagePath;
    if (lPathWithoutPrefix.empty()) {
        storagePath = MediaStringUtils::FillParams(DOCS_STORAGE_PATH_ROOT_PATTERN, {displayName});
    } else {
        storagePath = MediaStringUtils::FillParams(DOCS_STORAGE_PATH_NORMAL_PATTERN, {lPathWithoutPrefix, displayName});
    }
    pullData.attributesStoragePath = storagePath;

    MEDIA_INFO_LOG("cloudId: %{public}s, fileSourceType: %{public}d, storagePath: %{public}s",
                   pullData.cloudId.c_str(),
                   pullData.attributesFileSourceType,
                   MediaFileUtils::DesensitizePath(pullData.attributesStoragePath).c_str());
    return;
}

/**
 * 需结合本地信息，修正云资产的 fileSourceType 和 storagePath
 * @param pullData 云资产信息，包含云端下行的照片信息和关联的本地照片信息
 * 云资产数据场景（AS-IS）：
 *
 * | hidden | trashed | fileSourceType  | storagePath                  | 进一步修正 |
 * |--------|---------|-----------------|------------------------------| -------- |
 * | 0      | 0       | FILE_MANAGER(1) | ./Docs/{lPath}/{displayName} | N |
 * | 1      | 0       | MEDIA(0)，根据 sourcePath 的 /FromDocs/ 识别文管 | "" | N |
 * | 0      | 1       | MEDIA(0)，根据 sourcePath 的 /FromDocs/ 识别文管 | "" | N |
 * | 1      | 1       | MEDIA(0)，根据 sourcePath 的 /FromDocs/ 识别文管 | "" | N |
 * | 0      | 0       | MEDIA(0)        | ""                           | Y，需结合本地 storagePath，进行修正 |
 * | 1      | 0       | MEDIA(0)        | ""                           | Y，需结合本地 storagePath，进行修正 |
 * | 0      | 1       | MEDIA(0)        | ""                           | Y，需结合本地 storagePath，进行修正 |
 * | 1      | 1       | MEDIA(0)        | ""                           | Y，需结合本地 storagePath，进行修正 |
 *
 * @return 修正后的 fileSourceType 和 storagePath，可在维护文件存储位置之后，直接更新入库；
 *
 * 云资产数据场景（TO-BE）：
 *
 * | 本地storagePath | hidden | trashed | fileSourceType  | storagePath |
 * |----------------|--------|---------|-----------------|------------------|
 * | 任意，不做判断   | 0      | 0       | FILE_MANAGER(1) | ./Docs/{lPath}/{displayName} |
 * | 任意，不做判断   | 1      | 0       | MEDIA(0)，根据 sourcePath 的 /FromDocs/ 识别文管 | "" |
 * | 任意，不做判断   | 0      | 1       | MEDIA(0)，根据 sourcePath 的 /FromDocs/ 识别文管 | "" |
 * | 任意，不做判断   | 1      | 1       | MEDIA(0)，根据 sourcePath 的 /FromDocs/ 识别文管 | "" |
 * | 湖内路径        | 0      | 0       | 修正：LAKE(3)    | 修正： ./Docs/HO_DATA_EXT_MISC/{lPath}/{displayName} |
 * | 湖内路径        | 1      | 0       | MEDIA(0)        | 修正： ./Docs/HO_DATA_EXT_MISC/{lPath}/{displayName} |
 * | 湖内路径        | 0      | 1       | MEDIA(0)        | 修正： ./Docs/HO_DATA_EXT_MISC/{lPath}/{displayName} |
 * | 湖内路径        | 1      | 1       | MEDIA(0)        | 修正： ./Docs/HO_DATA_EXT_MISC/{lPath}/{displayName} |
 * | 否             | 0 or 1 | 0 or 1  | MEDIA(0)        | "" (本地资产非湖内文件，保持默认值) |
 */
void CloudMediaFileInfoService::FixFileInfoWithLocal(CloudMediaPullDataDto &pullData, const PhotosPo &photoInfo)
{
    // 仅修正fileSourceType为MEDIA的资产，其他fileSourceType的资产不做修改
    CHECK_AND_RETURN(pullData.attributesFileSourceType == static_cast<int32_t>(FileSourceType::MEDIA));

    std::string storagePath = photoInfo.storagePath.value_or("");
    // 只能根据本地资产storagePath判断是否为湖内文件，其他字段无法区分湖内文件和普通文件
    const bool isLakeFile = MediaStringUtils::StartsWith(storagePath, LAKE_STORAGE_PATH_PREFIX);
    CHECK_AND_RETURN(isLakeFile);

    // 结合本地资产信息，满足以下所有条件，才需要修正 fileSourceType 为 MEDIA_HO_LAKE：
    // 1. 本地资产的storagePath以湖内路径前缀开头，说明是湖内文件
    // 2. 文件未隐藏（hidden=0），且未被回收（dateTrashed=0），满足湖内文件的特征
    const bool isLakeFileSourceType = !pullData.IsHiddenAsset() && pullData.basicRecycledTime == 0;
    if (isLakeFileSourceType) {
        pullData.attributesFileSourceType = static_cast<int32_t>(FileSourceType::MEDIA_HO_LAKE);
    }

    // 背景：湖内资产文件信息不支持跨端，在云下行时被清理；
    // 处理措施：数据入库前，识别并修正 storagePath 为湖内文件的 storagePath，
    // 修正规则为：/storage/media/local/files/Docs/HO_DATA_EXT_MISC/{lPath}/{displayName}
    const std::string lPath = CloudMediaSyncUtils::GetLpath(pullData);
    const std::string displayName = pullData.basicDisplayName;
    const std::string lakePathPrefix = PATH_SEPARATOR;
    std::string lPathWithoutPrefix = lPath;
    if (MediaStringUtils::StartsWith(lPath, lakePathPrefix)) {
        lPathWithoutPrefix = lPath.substr(lakePathPrefix.length());
    }
    if (lPathWithoutPrefix.empty()) {
        storagePath = MediaStringUtils::FillParams(LAKE_STORAGE_PATH_ROOT_PATTERN, {displayName});
    } else {
        storagePath = MediaStringUtils::FillParams(LAKE_STORAGE_PATH_NORMAL_PATTERN, {lPathWithoutPrefix, displayName});
    }
    pullData.attributesStoragePath = storagePath;
    return;
}

/**
 * 定制化调整 云资产的 fileSourceType 和 storagePath 字段；
 * @return 修正后的 fileSourceType 和 storagePath，可在维护文件存储位置之后，直接更新入库；
 *
 * 前置条件：云资产的 fileSourceType 或 storagePath 与本地资产不一致时，才进行后续调整判断；
 * 调整策略：优先保留本地设备文件的存储位置信息，修正云资产的 fileSourceType 和 storagePath 字段值与本地资产保持一致；
 * 数据场景：
 * | nameChange | hiddenChange | trashedChange | albumOrSourcePathChange | 是否满足调整条件 |
 * |------------|--------------|---------------|-------------------------|---------------|
 * | 0          | 0            | 0             | 0                       | Y             |
 * | 0          | 0            | 0             | 1                       | N             |
 * | 0          | 0            | 1             | 0                       | N             |
 * | 0          | 0            | 1             | 1                       | N             |
 * | 0          | 1            | 0             | 0                       | N             |
 * | 0          | 1            | 0             | 1                       | N             |
 * | 0          | 1            | 1             | 0                       | N             |
 * | 0          | 1            | 1             | 1                       | N             |
 * | 1          | 0            | 0             | 0                       | N             |
 * | 1          | 0            | 1             | 0                       | N             |
 * | 1          | 0            | 0             | 1                       | N             |
 * | 1          | 0            | 1             | 1                       | N             |
 * | 1          | 1            | 0             | 0                       | N             |
 * | 1          | 1            | 0             | 1                       | N             |
 * | 1          | 1            | 1             | 0                       | N             |
 * | 1          | 1            | 1             | 1                       | N             |
 */
void CloudMediaFileInfoService::AdjustFileInfoWithLocal(CloudMediaPullDataDto &pullData, PhotosPo &photoInfo)
{
    // 无需调整场景：云资产和本地资产的 fileSourceType 和 storagePath 字段均相等；
    bool isNoNeedAdjust = pullData.attributesFileSourceType == photoInfo.fileSourceType.value_or(0);
    isNoNeedAdjust = isNoNeedAdjust && pullData.attributesStoragePath == photoInfo.storagePath.value_or("");
    CHECK_AND_RETURN(!isNoNeedAdjust);

    // 满足调整条件，无需移动文件，仅调整云资产的 fileSourceType 和 storagePath 字段值与本地资产保持一致；
    const bool isNameNotChanged = IsNameNotChanged(pullData, photoInfo);
    const bool isHiddenNotChanged = IsHiddenNotChanged(pullData, photoInfo);
    const bool isTrashedNotChanged = IsTrashedNotChanged(pullData, photoInfo);
    const bool isAlbumOrSourcePathNotChanged = IsAlbumOrSourcePathNotChanged(pullData, photoInfo);
    const bool isSameKeyData =
        isNameNotChanged && isHiddenNotChanged && isTrashedNotChanged && isAlbumOrSourcePathNotChanged;
    CHECK_AND_RETURN(isSameKeyData);

    pullData.attributesFileSourceType = photoInfo.fileSourceType.value_or(0);
    pullData.attributesStoragePath = photoInfo.storagePath.value_or("");
    MEDIA_INFO_LOG("No need relocate file, cloudId: %{public}s, fileSourceType: %{public}d, storagePath: %{public}s",
                   pullData.cloudId.c_str(),
                   pullData.attributesFileSourceType,
                   MediaFileUtils::DesensitizePath(pullData.attributesStoragePath).c_str());
    return;
}

bool CloudMediaFileInfoService::IsNameNotChanged(const CloudMediaPullDataDto &pullData, const PhotosPo &photoInfo) const
{
    const std::string localDisplayName = photoInfo.displayName.value_or("");
    return pullData.basicDisplayName == localDisplayName;
}

bool CloudMediaFileInfoService::IsHiddenNotChanged(const CloudMediaPullDataDto &pullData,
                                                   const PhotosPo &photoInfo) const
{
    const int32_t localHidden = photoInfo.hidden.value_or(0);
    const bool cloudIsHidden = pullData.IsHiddenAsset();
    return (cloudIsHidden && localHidden == 1) || (!cloudIsHidden && localHidden == 0);
}

bool CloudMediaFileInfoService::IsTrashedNotChanged(const CloudMediaPullDataDto &pullData,
                                                    const PhotosPo &photoInfo) const
{
    const int64_t localDateTrashed = photoInfo.dateTrashed.value_or(0);
    const int64_t cloudDateTrashed = pullData.basicRecycledTime;
    return (cloudDateTrashed > 0 && localDateTrashed > 0) || (cloudDateTrashed == 0 && localDateTrashed == 0);
}

bool CloudMediaFileInfoService::IsAlbumOrSourcePathNotChanged(CloudMediaPullDataDto &pullData,
                                                              PhotosPo &photoInfo) const
{
    const bool isCloudHidden = pullData.IsHiddenAsset();
    const bool isCloudTrashed = pullData.basicRecycledTime != 0;
    const bool isLocalHidden = photoInfo.hidden.value_or(0) == 1;
    const bool isLocalTrashed = photoInfo.dateTrashed.value_or(0) != 0;

    const std::string localSourcePath = photoInfo.sourcePath.value_or("");
    const bool isSourcePathValid = (isLocalHidden || isLocalTrashed) && !localSourcePath.empty();
    // 隐藏或回收站：检查sourcePath是否变更
    if (isSourcePathValid) {
        return pullData.propertiesSourcePath == localSourcePath;
    }
    // 检查lPath是否变更
    return IsPhotoAlbumNotChanged(pullData, photoInfo);
}

bool CloudMediaFileInfoService::IsPhotoAlbumNotChanged(const CloudMediaPullDataDto &pullData, PhotosPo &photoInfo) const
{
    CHECK_AND_EXECUTE(photoInfo.albumInfoOp.has_value(),
        this->commonDao_.QueryPhotoAlbumByAlbumId(photoInfo.ownerAlbumId.value_or(0), photoInfo.albumInfoOp));
    CHECK_AND_RETURN_RET_LOG(photoInfo.albumInfoOp.has_value(), false, "photoInfo.albumInfoOp has no value");

    PhotoAlbumPo localAlbumInfo = photoInfo.albumInfoOp.value();

    // 比较lPath是否一致
    const std::string cloudLPath = CloudMediaSyncUtils::GetLpath(pullData);
    const std::string localLPath = localAlbumInfo.lpath.value_or("");
    return cloudLPath == localLPath;
}

void CloudMediaFileInfoService::FixFileInfo(CloudMediaPullDataDto &pullData)
{
    const int32_t fileSourceTypeBefore = pullData.attributesFileSourceType;
    const std::string storagePathBefore = pullData.attributesStoragePath;

    this->FixFileInfoWithCloudOnly(pullData);

    // 关联不到本地资产，不满足调整条件，无需调整
    CHECK_AND_RETURN(pullData.localPhotosPoOp.has_value());
    PhotosPo photoInfo = pullData.localPhotosPoOp.value();

    this->FixFileInfoWithLocal(pullData, photoInfo);
    this->AdjustFileInfoWithLocal(pullData, photoInfo);

    const int32_t fileSourceTypeAfter = pullData.attributesFileSourceType;
    const std::string storagePathAfter = pullData.attributesStoragePath;

    MEDIA_INFO_LOG("FixFileInfo completed. cloudId: %{public}s, "
                   "fileSourceType: %{public}d -> %{public}d, storagePath: %{public}s -> %{public}s",
        pullData.cloudId.c_str(),
        fileSourceTypeBefore,
        fileSourceTypeAfter,
        MediaFileUtils::DesensitizePath(storagePathBefore).c_str(),
        MediaFileUtils::DesensitizePath(storagePathAfter).c_str());
}
}  // namespace OHOS::Media::CloudSync