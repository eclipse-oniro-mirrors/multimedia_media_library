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
#include "gallery_db_upgrade.h"

#include "rdb_store.h"
#include "album_plugin_table_event_handler.h"
#include "media_log.h"
#include "db_upgrade_utils.h"

namespace OHOS::Media {
namespace DataTransfer {
/**
 * @brief Upgrade the database, before data restore or clone.
 */
int32_t GalleryDbUpgrade::OnUpgrade(std::shared_ptr<NativeRdb::RdbStore> galleryRdbPtr)
{
    CHECK_AND_RETURN_RET_WARN_LOG(galleryRdbPtr != nullptr, -1,
        "galleryRdbPtr is nullptr, Maybe init failed, skip gallery db upgrade.");
    return this->OnUpgrade(*galleryRdbPtr);
}

/**
 * @brief Upgrade the database, before data restore or clone.
 */
int32_t GalleryDbUpgrade::OnUpgrade(NativeRdb::RdbStore &store)
{
    MEDIA_INFO_LOG("GalleryDbUpgrade::OnUpgrade start.");
    AlbumPluginTableEventHandler handler;
    int32_t ret = handler.OnUpgrade(store, 0, 0);
    MEDIA_INFO_LOG("GalleryDbUpgrade::OnUpgrade end, ret: %{public}d", ret);
    this->AddPhotoQualityOfGalleryMedia(store);
    this->AddResolutionOfGalleryMedia(store);
    this->AddRelativeBucketIdOfGalleryAlbum(store);
    this->GarbageAlbumUpgrade(store);
    this->AddIndexOfGalleryAlbum(store);
    this->AddIndexAlbumIdOfGalleryAlbum(store);
    this->AddIndexOfAlbumPlugin(store);
    this->AddStoryChosenOfGalleryMedia(store);
    this->CreateRelativeAlbumOfGalleryAlbum(store);
    this->AddRelativeBucketIdColumn(store);
    this->AddUserDisplayLevelIntoMergeTag(store);
    this->AddHdcUniqueIdIntoGalleryMedia(store);
    this->AddColumnsOfTOcrResult(store);
    this->AddUploadStatusIntoGalleryAlbum(store);
    this->AddHdcUploadStatusIntoGalleryAlbum(store);
    return NativeRdb::E_OK;
}

/**
 * @brief Add photo_quality of gallery_media table in gallery.db.
 */
int32_t GalleryDbUpgrade::AddPhotoQualityOfGalleryMedia(NativeRdb::RdbStore &store)
{
    bool cond = this->dbUpgradeUtils_.IsColumnExists(store, "gallery_media", "photo_quality");
    CHECK_AND_RETURN_RET(!cond, NativeRdb::E_OK);
    std::string sql = this->SQL_GALLERY_MEDIA_TABLE_ADD_PHOTO_QUALITY;
    int32_t ret = store.ExecuteSql(sql);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "Media_Restore: GalleryDbUpgrade::AddPhotoQualityOfGalleryMedia failed,"
        "ret=%{public}d, sql=%{public}s", ret, sql.c_str());
    MEDIA_INFO_LOG("Media_Restore: GalleryDbUpgrade::AddPhotoQualityOfGalleryMedia success");
    return ret;
}

/**
 * @brief Add resolution of gallery_media table in gallery.db.
 */
 int32_t GalleryDbUpgrade::AddResolutionOfGalleryMedia(NativeRdb::RdbStore &store)
{
    bool cond = this->dbUpgradeUtils_.IsColumnExists(store, "gallery_media", "resolution");
    CHECK_AND_RETURN_RET(!cond, NativeRdb::E_OK);
    std::string sql = this->SQL_GALLERY_MEDIA_TABLE_ADD_RESOLUTION;
    int32_t ret = store.ExecuteSql(sql);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "Media_Restore: GalleryDbUpgrade::AddResolutionOfGalleryMedia failed,"
         "ret=%{public}d, sql=%{public}s", ret, sql.c_str());
    MEDIA_INFO_LOG("Media_Restore: GalleryDbUpgrade::AddResolutionOfGalleryMedia success");
    return ret;
}

/**
 * @brief Add relativeBucketId of gallery_album table in gallery.db if not exists.
 */
int32_t GalleryDbUpgrade::AddRelativeBucketIdOfGalleryAlbum(NativeRdb::RdbStore &store)
{
    bool cond = this->dbUpgradeUtils_.IsColumnExists(store, "gallery_album", "relativeBucketId");
    CHECK_AND_RETURN_RET(!cond, NativeRdb::E_OK);
    std::string sql = this->SQL_GALLERY_ALBUM_TABLE_ADD_RELATIVE_BUCKET_ID;
    int32_t ret = store.ExecuteSql(sql);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK,
        "Media_Restore: GalleryDbUpgrade::AddRelativeBucketIdOfGalleryAlbum failed,"
        " ret=%{public}d, sql=%{public}s", ret, sql.c_str());
    MEDIA_INFO_LOG("Media_Restore: GalleryDbUpgrade::AddRelativeBucketIdOfGalleryAlbum success");
    return ret;
}

int32_t GalleryDbUpgrade::GarbageAlbumUpgrade(NativeRdb::RdbStore &store)
{
    this->GarbageAlbumCheckOrAddRelativeBucketId(store);
    this->GarbageAlbumCheckOrAddType(store);
    return NativeRdb::E_OK;
}

int32_t GalleryDbUpgrade::GarbageAlbumCheckOrAddRelativeBucketId(NativeRdb::RdbStore &store)
{
    bool cond = this->dbUpgradeUtils_.IsColumnExists(store, "garbage_album", "relative_bucket_id");
    CHECK_AND_RETURN_RET(!cond, NativeRdb::E_OK);
    std::string sql = this->SQL_GARBAGE_ALBUM_TABLE_ADD_RELATIVE_BUCKET_ID;
    int32_t ret = store.ExecuteSql(sql);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "Media_Restore: GarbageAlbumCheckOrAddRelativeBucketId failed,"
        " ret=%{public}d, sql=%{public}s", ret, sql.c_str());
    MEDIA_INFO_LOG("Media_Restore: GarbageAlbumCheckOrAddRelativeBucketId success");
    return ret;
}

int32_t GalleryDbUpgrade::GarbageAlbumCheckOrAddType(NativeRdb::RdbStore &store)
{
    bool cond = this->dbUpgradeUtils_.IsColumnExists(store, "garbage_album", "type");
    CHECK_AND_RETURN_RET(!cond, NativeRdb::E_OK);
    std::string sql = this->SQL_GARBAGE_ALBUM_TABLE_ADD_TYPE;
    int32_t ret = store.ExecuteSql(sql);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "Media_Restore: GarbageAlbumCheckOrAddType failed,"
        " ret=%{public}d, sql=%{public}s", ret, sql.c_str());
    MEDIA_INFO_LOG("Media_Restore: GarbageAlbumCheckOrAddType success");
    return ret;
}

int32_t GalleryDbUpgrade::AddIndexOfGalleryAlbum(NativeRdb::RdbStore &store)
{
    std::string sql = this->SQL_GALLERY_ALBUM_INDEX_RELATIVE_BUCKET_ID;
    int32_t ret = store.ExecuteSql(sql);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "Media_Restore: GalleryDbUpgrade::AddIndexOfGalleryAlbum failed,"
        " ret=%{public}d, sql=%{public}s", ret, sql.c_str());
    MEDIA_INFO_LOG("Media_Restore: GalleryDbUpgrade::AddIndexOfGalleryAlbum success");
    return ret;
}

int32_t GalleryDbUpgrade::AddIndexAlbumIdOfGalleryAlbum(NativeRdb::RdbStore &store)
{
    std::string sql = this->SQL_GALLERY_ALBUM_INDEX_ALBUM_ID;
    int32_t ret = store.ExecuteSql(sql);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "Media_Restore: GalleryDbUpgrade::AddIndexAlbumIdOfGalleryAlbum failed,"
        " ret=%{public}d, sql=%{public}s", ret, sql.c_str());
    MEDIA_INFO_LOG("Media_Restore: GalleryDbUpgrade::AddIndexAlbumIdOfGalleryAlbum success");
    return ret;
}

int32_t GalleryDbUpgrade::AddIndexOfAlbumPlugin(NativeRdb::RdbStore &store)
{
    std::string sql = this->SQL_ALBUM_PLUGIN_INDEX_ALBUM_NAME;
    int32_t ret = store.ExecuteSql(sql);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "Media_Restore: GalleryDbUpgrade::AddIndexOfAlbumPlugin failed,"
        " ret=%{public}d, sql=%{public}s", ret, sql.c_str());
    MEDIA_INFO_LOG("Media_Restore: GalleryDbUpgrade::AddIndexOfAlbumPlugin success");
    return ret;
}

/**
 * @brief Add story_chosen of gallery_media table in gallery.db.
 */
 int32_t GalleryDbUpgrade::AddStoryChosenOfGalleryMedia(NativeRdb::RdbStore &store)
{
    bool cond = this->dbUpgradeUtils_.IsColumnExists(store, "gallery_media", "story_chosen");
    CHECK_AND_RETURN_RET(!cond, NativeRdb::E_OK);
    std::string sql = this->SQL_GALLERY_MEDIA_TABLE_ADD_STORY_CHOSEN;
    int32_t ret = store.ExecuteSql(sql);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "Media_Restore: GalleryDbUpgrade::AddStoryChosenOfGalleryMedia failed,"
         "ret=%{public}d, sql=%{public}s", ret, sql.c_str());
    MEDIA_INFO_LOG("Media_Restore: GalleryDbUpgrade::AddStoryChosenOfGalleryMedia success");
    return ret;
}

/**
 * @brief Create relative_album table in gallery.db.
 */
int32_t GalleryDbUpgrade::CreateRelativeAlbumOfGalleryAlbum(NativeRdb::RdbStore &store)
{
    std::string sql = this->CREATE_RELATE_ALBUM_TBL_SQL;
    int32_t ret = store.ExecuteSql(sql);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "Add RelativeAlbum Of Gallery Album failed,"
        "ret=%{public}d, sql=%{public}s", ret, sql.c_str());

    std::string insertSql = this->INSERT_RELATE_ALBUM_TBL_SQL;
    ret = store.ExecuteSql(insertSql);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "Create RelativeAlbum Of Gallery Album failed,"
        "ret=%{public}d, sql=%{public}s", ret, sql.c_str());
    MEDIA_INFO_LOG("Create Relative Album Of Gallery Album success");
    return ret;
}

/**
 * @brief Add relative_bucket_id column for gallery_cover_cache table in gallery.db.
 */
int32_t GalleryDbUpgrade::AddRelativeBucketIdColumn(NativeRdb::RdbStore &store)
{
    std::string sql = "ALTER TABLE gallery_cover_cache ADD COLUMN relative_bucket_id TEXT default ''";
    int32_t ret = store.ExecuteSql(sql);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "Add relative_bucket_id column for gallery_cover_cache table failed,"
        "ret=%{public}d, sql=%{public}s", ret, sql.c_str());

    std::string prefix = "/gallery/album/entity/preferential/*/%";
    size_t prefixLen = prefix.size();
    std::string updateSql = "UPDATE gallery_cover_cache SET relative_bucket_id = SUBSTR(column_value, " +
        std::to_string(prefixLen) + ") where column_name = 'relative_bucket_id' AND from_user = 1 AND "
        "column_value LIKE'" + prefix + "'";
    ret = store.ExecuteSql(updateSql);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "update gallery_cover_cache Album failed,"
        "ret=%{public}d, sql=%{public}s", ret, sql.c_str());
    
    prefix = "/gallery/album/entity/common/1/%";
    prefixLen = prefix.size();
    updateSql = "UPDATE gallery_cover_cache SET relative_bucket_id = SUBSTR(column_value, " +
        std::to_string(prefixLen) +
        ") where column_name = 'relative_bucket_id' AND from_user = 1 AND column_value LIKE'" + prefix + "'";
    ret = store.ExecuteSql(updateSql);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "update gallery_cover_cache Album failed,"
        "ret=%{public}d, sql=%{public}s", ret, sql.c_str());

    MEDIA_INFO_LOG("Add and update relative_bucket_id column for gallery_cover_cache table success");
    return ret;
}

/**
 * @brief Add user_display_level column to merge_tag table and set values based on group_tag query.
 */
int32_t GalleryDbUpgrade::AddUserDisplayLevelIntoMergeTag(NativeRdb::RdbStore &store)
{
    bool cond = this->dbUpgradeUtils_.IsColumnExists(store, "merge_tag", "user_display_level");
    CHECK_AND_RETURN_RET(!cond, NativeRdb::E_OK);

    std::string sqlAddColumn = "ALTER TABLE merge_tag ADD COLUMN user_display_level INTEGER DEFAULT -1;";
    int32_t ret = store.ExecuteSql(sqlAddColumn);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK,
        "Media_Restore: GalleryDbUpgrade::AddUserDisplayLevelIntoMergeTag add column failed,"
        "ret=%{public}d, sql=%{public}s",
        ret,
        sqlAddColumn.c_str());

    ret = store.ExecuteSql(this->UPDATE_USER_DISPLAY_LEVEL_SQL);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK,
        "Media_Restore: GalleryDbUpgrade::AddUserDisplayLevelIntoMergeTag update values failed,"
        "ret=%{public}d, sql=%{public}s",
        ret,
        this->UPDATE_USER_DISPLAY_LEVEL_SQL.c_str());

    MEDIA_INFO_LOG("Media_Restore: GalleryDbUpgrade::AddUserDisplayLevelIntoMergeTag success");
    return ret;
}

/**
 * @brief Upgrade gallery_media table in gallery.db.
 */
int32_t GalleryDbUpgrade::AddHdcUniqueIdIntoGalleryMedia(NativeRdb::RdbStore &store)
{
    bool cond = this->dbUpgradeUtils_.IsColumnExists(store, "gallery_media", "hdc_unique_id");
    CHECK_AND_RETURN_RET(!cond, NativeRdb::E_OK);
    std::string sql = this->SQL_GALLERY_MEDIA_TABLE_ADD_HDC_UNIQUE_ID_COLUMN;
    int32_t ret = store.ExecuteSql(sql);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK,
        "Media_Restore: GalleryDbUpgrade::AddHdcUniqueIdIntoGalleryMedia failed,"
        "ret=%{public}d, sql=%{public}s",
        ret,
        sql.c_str());

    MEDIA_INFO_LOG("Media_Restore: GalleryDbUpgrade::AddHdcUniqueIdIntoGalleryMedia success");
    return ret;
}

/**
 * @brief Add version_ocr, width, height of t_ocr_result table in gallery.db.
 */
int32_t GalleryDbUpgrade::AddColumnsOfTOcrResult(NativeRdb::RdbStore &store)
{
    bool isVersionExists = this->dbUpgradeUtils_.IsColumnExists(store, "t_ocr_result", "version_ocr");
    if (!isVersionExists) {
        std::string sql = this->SQL_T_OCR_RESULT_TABLE_ADD_VERSION_OCR;
        int32_t ret = store.ExecuteSql(sql);
        CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "AddColumnsOfTOcrResult add version_ocr failed,"
            "ret=%{public}d, sql=%{public}s", ret, sql.c_str());
        MEDIA_INFO_LOG("AddColumnsOfTOcrResult add version_ocr end");
    }

    bool isWidthExists = this->dbUpgradeUtils_.IsColumnExists(store, "t_ocr_result", "width");
    if (!isWidthExists) {
        std::string sql = this->SQL_T_OCR_RESULT_TABLE_ADD_WIDTH;
        int32_t ret = store.ExecuteSql(sql);
        CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "AddColumnsOfTOcrResult add width failed,"
            "ret=%{public}d, sql=%{public}s", ret, sql.c_str());
        MEDIA_INFO_LOG("AddColumnsOfTOcrResult add width end");
    }

    bool isHeightExists = this->dbUpgradeUtils_.IsColumnExists(store, "t_ocr_result", "height");
    if (!isHeightExists) {
        std::string sql = this->SQL_T_OCR_RESULT_TABLE_ADD_HEIGHT;
        int32_t ret = store.ExecuteSql(sql);
        CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "AddColumnsOfTOcrResult add height failed,"
            "ret=%{public}d, sql=%{public}s", ret, sql.c_str());
        MEDIA_INFO_LOG("AddColumnsOfTOcrResult add height end");
    }

    if (!isWidthExists || !isHeightExists) {
        std::string sql = this->SQL_T_OCR_RESULT_TABLE_COPY_WIDTH_HEIGHT;
        int32_t ret = store.ExecuteSql(sql);
        CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "AddColumnsOfTOcrResult copy width height failed,"
            "ret=%{public}d, sql=%{public}s", ret, sql.c_str());
        MEDIA_INFO_LOG("AddColumnsOfTOcrResult copy width height end");
    }
    return NativeRdb::E_OK;
}

/**
 * @brief Upgrade gallery_album table in gallery.db.
 */
int32_t GalleryDbUpgrade::AddUploadStatusIntoGalleryAlbum(NativeRdb::RdbStore &store)
{
    bool cond = this->dbUpgradeUtils_.IsColumnExists(store, "gallery_album", "uploadStatus");
    CHECK_AND_RETURN_RET(!cond, NativeRdb::E_OK);
    std::string sql = this->SQL_GALLERY_ALBUM_TABLE_ADD_UPLOAD_STATUS_COLUMN;
    int32_t ret = store.ExecuteSql(sql);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK,
        "Media_Restore: GalleryDbUpgrade::AddUploadStatusIntoGalleryAlbum failed,"
        "ret=%{public}d, sql=%{public}s",
        ret,
        sql.c_str());
    MEDIA_INFO_LOG("Media_Restore: GalleryDbUpgrade::AddUploadStatusIntoGalleryAlbum success");
    return ret;
}

/**
 * @brief Upgrade gallery_album table in gallery.db.
 */
int32_t GalleryDbUpgrade::AddHdcUploadStatusIntoGalleryAlbum(NativeRdb::RdbStore &store)
{
    bool cond = this->dbUpgradeUtils_.IsColumnExists(store, "gallery_album", "hdcUploadStatus");
    CHECK_AND_RETURN_RET(!cond, NativeRdb::E_OK);
    std::string sql = this->SQL_GALLERY_ALBUM_TABLE_ADD_HDC_UPLOAD_STATUS_COLUMN;
    int32_t ret = store.ExecuteSql(sql);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK,
        "Media_Restore: GalleryDbUpgrade::AddHdcUploadStatusIntoGalleryAlbum failed,"
        "ret=%{public}d, sql=%{public}s",
        ret,
        sql.c_str());
    MEDIA_INFO_LOG("Media_Restore: GalleryDbUpgrade::AddHdcUploadStatusIntoGalleryAlbum success");
    return ret;
}
}  // namespace DataTransfer
}  // namespace OHOS::Media