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

#ifndef TEST_DATA_BUILDER_H
#define TEST_DATA_BUILDER_H

#include <string>
#include <memory>
#include "medialibrary_rdbstore.h"
#include "photo_album_column.h"
#include "media_column.h"

namespace OHOS::Media {

enum class TestAlbumType {
    USER_ALBUM,
    SOURCE_ALBUM,
    FILE_MANAGER_ALBUM,
    SHARE_ALBUM
};

struct TestAlbumData {
    int32_t albumId = 0;
    int32_t albumType = 0;
    int32_t albumSubType = 0;
    std::string albumName = "";
    std::string albumLpath = "";
};

struct TestAssetData {
    int32_t assetId = 0;
    std::string filePath;
    std::string displayName;
    int32_t ownerAlbumId = 0;
    int32_t fileSourceType = 0;
    std::string storagePath;
    int32_t isShared = 0;
};

class TestDataBuilder {
public:
    static TestDataBuilder& GetInstance();

    void Init(std::shared_ptr<MediaLibraryRdbStore> rdbStore);

    int32_t CreateAlbum(TestAlbumType albumType, const std::string& albumName);

    int32_t CreateAsset(int32_t albumId, const std::string& displayName);
    int32_t CreateAssetWithStoragePath(int32_t albumId, const std::string& displayName,
        const std::string& storagePath);
    int32_t CreateSharedAlbum(const std::string& albumName);
    int32_t CreateSharedAsset(int32_t albumId, const std::string& displayName);

    void ClearAllTables();
    void ClearPhotosTable();
    void ClearAlbumTable();

    bool CreatePhysicalFile(const std::string& filePath);
    bool DeletePhysicalFile(const std::string& filePath);
    bool IsPhysicalFileExists(const std::string& filePath);

    static constexpr int32_t USER_ALBUM_ID_START = 1000;
    static constexpr int32_t SOURCE_ALBUM_ID_START = 2000;
    static constexpr int32_t FILE_MANAGER_ALBUM_ID_START = 3000;
    static constexpr int32_t ASSET_ID_START = 10000;

private:
    TestDataBuilder() = default;
    ~TestDataBuilder() = default;

    int32_t InsertAlbum(const TestAlbumData& albumData);
    int32_t InsertAsset(const TestAssetData& assetData);

    std::shared_ptr<MediaLibraryRdbStore> rdbStore_;
    int32_t nextAlbumId_ = USER_ALBUM_ID_START;
    int32_t nextAssetId_ = ASSET_ID_START;
};

} // namespace OHOS::Media

#endif // TEST_DATA_BUILDER_H
