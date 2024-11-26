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
    if (galleryRdbPtr == nullptr) {
        MEDIA_WARN_LOG("galleryRdbPtr is nullptr, Maybe init failed, skip gallery db upgrade.");
        return -1;
    }
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
    this->AddRelativeBucketIdOfGalleryAlbum(store);
    this->GarbageAlbumUpgrade(store);
    return NativeRdb::E_OK;
}

/**
 * @brief Add photo_quality of gallery_media table in gallery.db.
 */
int32_t GalleryDbUpgrade::AddPhotoQualityOfGalleryMedia(NativeRdb::RdbStore &store)
{
    if (this->dbUpgradeUtils_.IsColumnExists(store, "gallery_media", "photo_quality")) {
        return NativeRdb::E_OK;
    }
    std::string sql = this->SQL_GALLERY_MEDIA_TABLE_ADD_PHOTO_QUALITY;
    int32_t ret = store.ExecuteSql(sql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG(
            "Media_Restore: GalleryDbUpgrade::AddPhotoQualityOfGalleryMedia failed, ret=%{public}d, sql=%{public}s",
            ret,
            sql.c_str());
    }
    MEDIA_INFO_LOG("Media_Restore: GalleryDbUpgrade::AddPhotoQualityOfGalleryMedia success");
    return ret;
}

/**
 * @brief Add relativeBucketId of gallery_album table in gallery.db if not exists.
 */
int32_t GalleryDbUpgrade::AddRelativeBucketIdOfGalleryAlbum(NativeRdb::RdbStore &store)
{
    if (this->dbUpgradeUtils_.IsColumnExists(store, "gallery_album", "relativeBucketId")) {
        return NativeRdb::E_OK;
    }
    std::string sql = this->SQL_GALLERY_ALBUM_TABLE_ADD_RELATIVE_BUCKET_ID;
    int32_t ret = store.ExecuteSql(sql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG(
            "Media_Restore: GalleryDbUpgrade::AddRelativeBucketIdOfGalleryAlbum failed, ret=%{public}d, sql=%{public}s",
            ret,
            sql.c_str());
    }
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
    if (this->dbUpgradeUtils_.IsColumnExists(store, "garbage_album", "relative_bucket_id")) {
        return NativeRdb::E_OK;
    }
    std::string sql = this->SQL_GARBAGE_ALBUM_TABLE_ADD_RELATIVE_BUCKET_ID;
    int32_t ret = store.ExecuteSql(sql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Media_Restore: GarbageAlbumCheckOrAddRelativeBucketId failed, ret=%{public}d, sql=%{public}s",
            ret,
            sql.c_str());
    }
    MEDIA_INFO_LOG("Media_Restore: GarbageAlbumCheckOrAddRelativeBucketId success");
    return ret;
}

int32_t GalleryDbUpgrade::GarbageAlbumCheckOrAddType(NativeRdb::RdbStore &store)
{
    if (this->dbUpgradeUtils_.IsColumnExists(store, "garbage_album", "type")) {
        return NativeRdb::E_OK;
    }
    std::string sql = this->SQL_GARBAGE_ALBUM_TABLE_ADD_TYPE;
    int32_t ret = store.ExecuteSql(sql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG(
            "Media_Restore: GarbageAlbumCheckOrAddType failed, ret=%{public}d, sql=%{public}s", ret, sql.c_str());
    }
    MEDIA_INFO_LOG("Media_Restore: GarbageAlbumCheckOrAddType success");
    return ret;
}
}  // namespace DataTransfer
}  // namespace OHOS::Media