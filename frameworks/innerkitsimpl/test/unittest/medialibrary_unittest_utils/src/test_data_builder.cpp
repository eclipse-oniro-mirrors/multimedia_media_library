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

#define MLOG_TAG "TestDataBuilder"

#include "test_data_builder.h"

#include <cinttypes>
#include <fstream>
#include <sys/stat.h>

#include "rdb_predicates.h"
#include "media_log.h"
#include "medialibrary_errno.h"

namespace OHOS::Media {
namespace {
    constexpr int64_t DEFAULT_FILE_SIZE = 175258;
    constexpr int64_t DEFAULT_DATE_ADDED = 1501924205218;
    constexpr int64_t DEFAULT_DATE_TAKEN = 1501924205;
    constexpr int32_t DEFAULT_PHOTO_HEIGHT = 1280;
    constexpr int32_t DEFAULT_PHOTO_WIDTH = 960;
}

TestDataBuilder& TestDataBuilder::GetInstance()
{
    static TestDataBuilder instance;
    return instance;
}

void TestDataBuilder::Init(std::shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    rdbStore_ = rdbStore;
    nextAlbumId_ = USER_ALBUM_ID_START;
    nextAssetId_ = ASSET_ID_START;
}

int32_t TestDataBuilder::InsertAlbum(const TestAlbumData& albumData)
{
    if (rdbStore_ == nullptr) {
        MEDIA_ERR_LOG("rdbStore_ is nullptr");
        return E_HAS_DB_ERROR;
    }

    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutInt(PhotoAlbumColumns::ALBUM_TYPE, albumData.albumType);
    valuesBucket.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, albumData.albumSubType);
    valuesBucket.PutString(PhotoAlbumColumns::ALBUM_NAME, albumData.albumName);
    valuesBucket.PutString(PhotoAlbumColumns::ALBUM_LPATH, albumData.albumLpath);

    int64_t rowId = -1;
    int32_t ret = rdbStore_->Insert(rowId, PhotoAlbumColumns::TABLE, valuesBucket);
    if (ret != NativeRdb::E_OK || rowId <= 0) {
        MEDIA_ERR_LOG("Failed to insert album, ret: %{public}d, rowId: %{public}" PRId64, ret, rowId);
        return E_HAS_DB_ERROR;
    }

    return static_cast<int32_t>(rowId);
}

int32_t TestDataBuilder::CreateAlbum(TestAlbumType albumType, const std::string& albumName)
{
    TestAlbumData albumData;
    albumData.albumId = nextAlbumId_++;
    albumData.albumName = albumName;
    if (albumType == TestAlbumType::FILE_MANAGER_ALBUM) {
        std::string path = "/storage/media/local/files/Docs/" + albumName;
        std::system(("mkdir -p " + path).c_str());
    }
    switch (albumType) {
        case TestAlbumType::USER_ALBUM:
            albumData.albumType = static_cast<int32_t>(PhotoAlbumType::USER);
            albumData.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC);
            albumData.albumLpath = "/Pictures/" + albumName;
            break;
        case TestAlbumType::SOURCE_ALBUM:
            albumData.albumType = static_cast<int32_t>(PhotoAlbumType::SOURCE);
            albumData.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC);
            albumData.albumLpath = "/Pictures/" + albumName;
            break;
        case TestAlbumType::FILE_MANAGER_ALBUM:
            albumData.albumType = static_cast<int32_t>(PhotoAlbumType::SOURCE);
            albumData.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILE_MANAGER);
            albumData.albumLpath = "/FromDocs/" + albumName;
            break;
        default:
            MEDIA_ERR_LOG("Invalid album type");
            return E_INVALID_VALUES;
    }
    return InsertAlbum(albumData);
}

int32_t TestDataBuilder::InsertAsset(const TestAssetData& assetData)
{
    if (rdbStore_ == nullptr) {
        MEDIA_ERR_LOG("rdbStore_ is nullptr");
        return E_HAS_DB_ERROR;
    }

    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutString(MediaColumn::MEDIA_FILE_PATH, assetData.filePath);
    valuesBucket.PutLong(MediaColumn::MEDIA_SIZE, static_cast<int64_t>(DEFAULT_FILE_SIZE));
    valuesBucket.PutString(MediaColumn::MEDIA_TITLE, assetData.displayName);
    valuesBucket.PutString(MediaColumn::MEDIA_NAME, assetData.displayName + ".jpg");
    valuesBucket.PutInt(MediaColumn::MEDIA_TYPE, static_cast<int32_t>(1));
    valuesBucket.PutString(MediaColumn::MEDIA_OWNER_PACKAGE, "com.ohos.camera");
    valuesBucket.PutString(MediaColumn::MEDIA_PACKAGE_NAME, "camera");
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_ADDED, static_cast<int64_t>(DEFAULT_DATE_ADDED));
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_MODIFIED, static_cast<int64_t>(0));
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_TAKEN, static_cast<int64_t>(DEFAULT_DATE_TAKEN));
    valuesBucket.PutInt(MediaColumn::MEDIA_DURATION, static_cast<int32_t>(0));
    valuesBucket.PutInt(MediaColumn::MEDIA_IS_FAV, static_cast<int32_t>(0));
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_TRASHED, static_cast<int64_t>(0));
    valuesBucket.PutInt(MediaColumn::MEDIA_HIDDEN, static_cast<int32_t>(0));
valuesBucket.PutInt(PhotoColumn::PHOTO_HEIGHT, static_cast<int32_t>(DEFAULT_PHOTO_HEIGHT));
    valuesBucket.PutInt(PhotoColumn::PHOTO_WIDTH, static_cast<int32_t>(DEFAULT_PHOTO_WIDTH));
    valuesBucket.PutLong(PhotoColumn::PHOTO_EDIT_TIME, static_cast<int64_t>(0));
    valuesBucket.PutString(PhotoColumn::PHOTO_SHOOTING_MODE, "1");
    valuesBucket.PutInt(PhotoColumn::PHOTO_OWNER_ALBUM_ID, assetData.ownerAlbumId);
    valuesBucket.PutInt(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, assetData.fileSourceType);
    valuesBucket.PutString(PhotoColumn::PHOTO_STORAGE_PATH, assetData.storagePath);
    valuesBucket.PutInt(PhotoColumn::PHOTO_FILE_HIDDEN, static_cast<int32_t>(0));

    int64_t rowId = -1;
    int32_t ret = rdbStore_->Insert(rowId, PhotoColumn::PHOTOS_TABLE, valuesBucket);
    if (ret != NativeRdb::E_OK || rowId <= 0) {
        MEDIA_ERR_LOG("Failed to insert asset, ret: %{public}d, rowId: %{public}" PRId64, ret, rowId);
        return E_HAS_DB_ERROR;
    }

    return static_cast<int32_t>(rowId);
}

int32_t TestDataBuilder::CreateAsset(int32_t albumId, const std::string& displayName)
{
    return CreateAssetWithStoragePath(albumId, displayName, "");
}

int32_t TestDataBuilder::CreateAssetWithStoragePath(int32_t albumId, const std::string& displayName,
    const std::string& storagePath)
{
    TestAssetData assetData;
    assetData.assetId = nextAssetId_++;
    assetData.filePath = "/storage/media/local/files/Photo/" + std::to_string(assetData.assetId) +
        "_" + displayName + ".jpg";
    assetData.displayName = displayName;
    assetData.ownerAlbumId = albumId;
    assetData.fileSourceType = storagePath.empty() ?
        static_cast<int32_t>(FileSourceType::MEDIA) :
        static_cast<int32_t>(FileSourceType::FILE_MANAGER);
    assetData.storagePath = storagePath;
    
    return InsertAsset(assetData);
}

void TestDataBuilder::ClearPhotosTable()
{
    if (rdbStore_ == nullptr) {
        return;
    }
    
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    int32_t rows = 0;
    rdbStore_->Delete(rows, predicates);
}

void TestDataBuilder::ClearAlbumTable()
{
    if (rdbStore_ == nullptr) {
        return;
    }
    
    NativeRdb::RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    int32_t rows = 0;
    rdbStore_->Delete(rows, predicates);
}

void TestDataBuilder::ClearAllTables()
{
    ClearPhotosTable();
    ClearAlbumTable();
}

bool TestDataBuilder::CreatePhysicalFile(const std::string& filePath)
{
    std::ofstream file(filePath);
    if (!file.is_open()) {
        MEDIA_ERR_LOG("Failed to create file: %{public}s", filePath.c_str());
        return false;
    }
    file << "test content";
    file.close();
    return true;
}

bool TestDataBuilder::DeletePhysicalFile(const std::string& filePath)
{
    if (remove(filePath.c_str()) != 0) {
        MEDIA_ERR_LOG("Failed to delete file: %{public}s", filePath.c_str());
        return false;
    }
    return true;
}

bool TestDataBuilder::IsPhysicalFileExists(const std::string& filePath)
{
    struct stat buffer;
    return (stat(filePath.c_str(), &buffer) == 0);
}

} // namespace OHOS::Media
