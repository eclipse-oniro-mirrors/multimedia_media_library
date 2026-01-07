/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "Media_Cloud_Dao"

#include "cloud_media_asset_retain_compare_dao.h"
#include "media_log.h"
#include "result_set_utils.h"
#include "cloud_media_file_utils.h"
#include "cloud_media_sync_utils.h"
#include "cloud_media_asset_manager.h"
#include "media_column.h"

namespace OHOS::Media::CloudSync {
DuplicatePhotoInfo CloudMediaAssetCompareDao::FindDuplicatePhoto(const CloudMediaPullDataDto &pullData,
    int32_t maxFileId)
{
    MEDIA_INFO_LOG("FindDuplicatePhoto for cloudId: %{public}s, maxFileId: %{public}d",
        pullData.cloudId.c_str(), maxFileId);

    DuplicatePhotoInfo duplicateInfo;
    CHECK_AND_RETURN_RET(maxFileId > 0, duplicateInfo);
    CHECK_AND_RETURN_RET(!pullData.cloudId.empty(), duplicateInfo);
    CHECK_AND_RETURN_RET(pullData.basicSize > 0, duplicateInfo);
    CHECK_AND_RETURN_RET(!pullData.basicFileName.empty(), duplicateInfo);

    duplicateInfo = FindSamePhotoInAlbum(pullData, maxFileId);
    if (duplicateInfo.isValid) {
        MEDIA_INFO_LOG("Found duplicate photo in album, fileId: %{public}d", duplicateInfo.fileId);
        return duplicateInfo;
    }

    duplicateInfo = FindSamePhotoWithoutAlbum(pullData, maxFileId);
    if (duplicateInfo.isValid) {
        MEDIA_INFO_LOG("Found duplicate photo without album, fileId: %{public}d", duplicateInfo.fileId);
        return duplicateInfo;
    }

    duplicateInfo = FindSamePhotoBySourcePath(pullData, maxFileId);
    if (duplicateInfo.isValid) {
        MEDIA_INFO_LOG("Found duplicate photo by source path, fileId: %{public}d", duplicateInfo.fileId);
        return duplicateInfo;
    }

    return duplicateInfo;
}

DuplicatePhotoInfo CloudMediaAssetCompareDao::FindSamePhotoInAlbum(const CloudMediaPullDataDto &pullData,
    int32_t maxFileId)
{
    DuplicatePhotoInfo duplicateInfo;
    CHECK_AND_RETURN_RET(maxFileId > 0, duplicateInfo);

    std::string albumPath = CloudMediaSyncUtils::GetLpath(pullData);
    if (albumPath.empty()) {
        MEDIA_INFO_LOG("Album path is empty for cloudId: %{public}s", pullData.cloudId.c_str());
        return duplicateInfo;
    }

    int32_t mediaType = GetMediaTypeFromPullData(pullData);
    int32_t pictureFlag = (mediaType == MEDIA_TYPE_VIDEO) ? 0 : 1;
    int32_t orientation = (pullData.propertiesRotate != -1) ? pullData.propertiesRotate : 0;

    const std::vector<NativeRdb::ValueObject> params = {
        albumPath, maxFileId, pullData.basicFileName, pullData.basicSize, pictureFlag, orientation};

    return ExecuteDuplicateQuery(SQL_PHOTOS_FIND_SAME_FILE_IN_ALBUM, params);
}

DuplicatePhotoInfo CloudMediaAssetCompareDao::FindSamePhotoWithoutAlbum(const CloudMediaPullDataDto &pullData,
    int32_t maxFileId)
{
    DuplicatePhotoInfo duplicateInfo;
    CHECK_AND_RETURN_RET(maxFileId > 0, duplicateInfo);

    int32_t mediaType = GetMediaTypeFromPullData(pullData);
    int32_t pictureFlag = (mediaType == MEDIA_TYPE_VIDEO) ? 0 : 1;
    int32_t orientation = (pullData.localOrientation != -1) ? pullData.localOrientation : 0;

    const std::vector<NativeRdb::ValueObject> params = {
        maxFileId, pullData.basicFileName, pullData.basicSize, pictureFlag, orientation};

    return ExecuteDuplicateQuery(SQL_PHOTOS_FIND_SAME_FILE_WITHOUT_ALBUM, params);
}

DuplicatePhotoInfo CloudMediaAssetCompareDao::FindSamePhotoBySourcePath(const CloudMediaPullDataDto &pullData,
    int32_t maxFileId)
{
    DuplicatePhotoInfo duplicateInfo;
    CHECK_AND_RETURN_RET(maxFileId > 0, duplicateInfo);

    std::string sourcePath = pullData.attributesStoragePath;
    if (sourcePath.empty()) {
        MEDIA_INFO_LOG("Source path is empty for cloudId: %{public}s", pullData.cloudId.c_str());
        return duplicateInfo;
    }

    int32_t mediaType = GetMediaTypeFromPullData(pullData);
    int32_t pictureFlag = (mediaType == MEDIA_TYPE_VIDEO) ? 0 : 1;
    int32_t orientation = (pullData.localOrientation != -1) ? pullData.localOrientation : 0;

    const std::vector<NativeRdb::ValueObject> params = {
        sourcePath, maxFileId, pullData.basicFileName, pullData.basicSize, pictureFlag, orientation};

    return ExecuteDuplicateQuery(SQL_PHOTOS_FIND_SAME_FILE_BY_SOURCE_PATH, params);
}

DuplicatePhotoInfo CloudMediaAssetCompareDao::ExecuteDuplicateQuery(const std::string &querySql,
    const std::vector<NativeRdb::ValueObject> &params)
{
    DuplicatePhotoInfo duplicateInfo;
    CHECK_AND_RETURN_RET(rdbStore_ != nullptr, duplicateInfo);

    auto resultSet = rdbStore_->QuerySql(querySql, params);
    CHECK_AND_RETURN_RET(resultSet != nullptr, duplicateInfo);

    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return duplicateInfo;
    }

    duplicateInfo.fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
    duplicateInfo.data = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
    duplicateInfo.cleanFlag = GetInt32Val(PhotoColumn::PHOTO_CLEAN_FLAG, resultSet);
    duplicateInfo.position = GetInt32Val(PhotoColumn::PHOTO_POSITION, resultSet);
    duplicateInfo.real_lcd_visit_time = GetInt64Val(PhotoColumn::PHOTO_REAL_LCD_VISIT_TIME, resultSet);
    duplicateInfo.isValid = true;

    resultSet->Close();

    MEDIA_INFO_LOG("Found duplicate photo: fileId=%{public}d, cleanFlag=%{public}d, data=%{public}s",
        duplicateInfo.fileId, duplicateInfo.cleanFlag, duplicateInfo.data.c_str());

    return duplicateInfo;
}

int32_t CloudMediaAssetCompareDao::GetMediaTypeFromPullData(const CloudMediaPullDataDto &pullData)
{
    if (pullData.basicFileType == FILE_TYPE_VIDEO) {
        return MEDIA_TYPE_VIDEO;
    } else {
        return MEDIA_TYPE_IMAGE;
    }
}

int32_t CloudMediaAssetCompareDao::GetPhotosMaxFileId()
{
    std::string querySql = this->SQL_PHOTOS_MAX_FILE_ID;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "GetPhotoMaxFileId Failed to get rdbStore.");
    auto resultSet = rdbStore->QuerySql(querySql);
    CHECK_AND_RETURN_RET_WARN_LOG(resultSet != nullptr, E_ERR, "Media_Restore: resultSet is null.");
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_WARN_LOG("Cloud_Restore: resultSet is null. querySql: %{public}s", querySql.c_str());
        resultSet->Close();
        return E_ERR;
    }
    int32_t maxFileId = GetInt32Val("max_file_id", resultSet);
    resultSet->Close();
    MEDIA_INFO_LOG("Cloud_Restore: max_file_id: %{public}d", maxFileId);
    return maxFileId;
}
} // namespace OHOS::Media::CloudSync