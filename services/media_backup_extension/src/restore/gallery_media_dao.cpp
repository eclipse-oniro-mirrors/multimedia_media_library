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
#include "gallery_media_dao.h"

#include <string>
#include <vector>

#include "rdb_store.h"
#include "result_set_utils.h"
#include "media_log.h"

namespace OHOS::Media {
void GalleryMediaDao::SetGalleryRdb(std::shared_ptr<NativeRdb::RdbStore> galleryRdb)
{
    this->galleryRdb_ = galleryRdb;
}

/**
 * @brief Get the gallery_media to restore to Photos.
 */
std::shared_ptr<NativeRdb::ResultSet> GalleryMediaDao::GetGalleryMedia(
    int32_t minId, int pageSize, bool shouldIncludeSd, bool hasLowQualityImage)
{
    int32_t shouldIncludeSdFlag = shouldIncludeSd ? 1 : 0;
    int32_t hasLowQualityImageFlag = hasLowQualityImage ? 1 : 0;
    std::vector<NativeRdb::ValueObject> params = {minId, hasLowQualityImageFlag, shouldIncludeSdFlag, pageSize};
    CHECK_AND_RETURN_RET_LOG(this->galleryRdb_ != nullptr, nullptr, "Media_Restore: galleryRdb_ is null.");
    return this->galleryRdb_->QuerySql(this->SQL_GALLERY_MEDIA_QUERY_FOR_RESTORE, params);
}

/**
 * @brief Get the gallery_media to restore cloud to Photos.
 */
std::shared_ptr<NativeRdb::ResultSet> GalleryMediaDao::GetCloudGalleryMedia(
    int32_t minId, int pageSize, bool shouldIncludeSd, bool hasLowQualityImage)
{
    CHECK_AND_RETURN_RET_LOG(this->galleryRdb_ != nullptr, nullptr, "Media_Restore: galleryRdb_ is null.");
    int32_t shouldIncludeSdFlag = shouldIncludeSd ? 1 : 0;
    int32_t hasLowQualityImageFlag = hasLowQualityImage ? 1 : 0;
    std::vector<NativeRdb::ValueObject> params = {minId, hasLowQualityImageFlag, shouldIncludeSdFlag, pageSize};
    return this->galleryRdb_->QuerySql(this->SQL_GALLERY_CLOUD_QUERY_FOR_RESTORE, params);
}

/**
 * @brief Get the gallery_media to restore hdc to Photos.
 */
std::shared_ptr<NativeRdb::ResultSet> GalleryMediaDao::GetHdcGalleryMedia(
    int32_t minId, int pageSize, bool shouldIncludeSd, bool hasLowQualityImage)
{
    CHECK_AND_RETURN_RET_LOG(this->galleryRdb_ != nullptr, nullptr, "Media_Restore: galleryRdb_ is null.");
    int32_t shouldIncludeSdFlag = shouldIncludeSd ? 1 : 0;
    int32_t hasLowQualityImageFlag = hasLowQualityImage ? 1 : 0;
    std::vector<NativeRdb::ValueObject> params = {minId, hasLowQualityImageFlag, shouldIncludeSdFlag, pageSize};
    return this->galleryRdb_->QuerySql(this->SQL_GALLERY_HDC_QUERY_FOR_RESTORE, params);
}

/**
 * @brief Get the row count of gallery_media.
 */
int32_t GalleryMediaDao::GetGalleryMediaCount(bool shouldIncludeSd, bool hasLowQualityImage)
{
    int32_t shouldIncludeSdFlag = shouldIncludeSd ? 1 : 0;
    int32_t hasLowQualityImageFlag = hasLowQualityImage ? 1 : 0;
    std::vector<NativeRdb::ValueObject> params = {hasLowQualityImageFlag, shouldIncludeSdFlag};
    CHECK_AND_RETURN_RET_LOG(this->galleryRdb_ != nullptr, 0, "Media_Restore: galleryRdb_ is null.");

    auto resultSet = this->galleryRdb_->QuerySql(this->SQL_GALLERY_MEDIA_QUERY_COUNT, params);
    CHECK_AND_RETURN_RET(resultSet != nullptr, 0);
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return 0;
    }
    int32_t count = GetInt32Val("count", resultSet);
    resultSet->Close();
    return count;
}

/**
 * @brief Get the row count of cloud_meta.
 */
int32_t GalleryMediaDao::GetCloudMetaCount(bool shouldIncludeSd, bool hasLowQualityImage)
{
    CHECK_AND_RETURN_RET_LOG(this->galleryRdb_ != nullptr, 0, "Cloud_meta_Restore: galleryRdb_ is null.");
    int32_t shouldIncludeSdFlag = shouldIncludeSd ? 1 : 0;
    int32_t hasLowQualityImageFlag = hasLowQualityImage ? 1 : 0;
    std::vector<NativeRdb::ValueObject> params = {hasLowQualityImageFlag, shouldIncludeSdFlag};
    auto resultSet = this->galleryRdb_->QuerySql(this->SQL_CLOUD_META_QUERY_COUNT, params);
    CHECK_AND_RETURN_RET(resultSet != nullptr, 0);
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return 0;
    }
    int32_t count = GetInt32Val("count", resultSet);
    resultSet->Close();
    return count;
}

/**
 * @brief Get the row count of hdc_meta.
 */
int32_t GalleryMediaDao::GetHdcMetaCount(bool shouldIncludeSd, bool hasLowQualityImage)
{
    CHECK_AND_RETURN_RET_LOG(this->galleryRdb_ != nullptr, 0, "Hdc_meta_Restore: galleryRdb_ is null.");
    int32_t shouldIncludeSdFlag = shouldIncludeSd ? 1 : 0;
    int32_t hasLowQualityImageFlag = hasLowQualityImage ? 1 : 0;
    std::vector<NativeRdb::ValueObject> params = {hasLowQualityImageFlag, shouldIncludeSdFlag};
    auto resultSet = this->galleryRdb_->QuerySql(this->SQL_HDC_META_QUERY_COUNT, params);
    CHECK_AND_RETURN_RET(resultSet != nullptr, 0);
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return 0;
    }
    int32_t count = GetInt32Val("count", resultSet);
    resultSet->Close();
    return count;
}


/**
 * @brief Get the row count of gallery_media no need migrate count.
 */
int32_t GalleryMediaDao::GetNoNeedMigrateCount(bool shouldIncludeSd)
{
    int32_t shouldIncludeSdFlag = shouldIncludeSd ? 1 : 0;
    std::vector<NativeRdb::ValueObject> params = {shouldIncludeSdFlag};
    CHECK_AND_RETURN_RET_LOG(this->galleryRdb_ != nullptr, 0, "Media_Restore: galleryRdb_ is null.");
    auto resultSet = this->galleryRdb_->QuerySql(this->SQL_GALLERY_MEDIA_QUERY_NO_NEED_MIGRATE_COUNT, params);
    CHECK_AND_RETURN_RET(resultSet != nullptr, 0);
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return 0;
    }
    int32_t count = GetInt32Val("count", resultSet);
    resultSet->Close();
    return count;
}
}  // namespace OHOS::Media