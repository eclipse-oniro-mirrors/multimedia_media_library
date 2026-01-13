/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
 
#ifndef FILE_PARSER_H
#define FILE_PARSER_H
#include <string>
 
#include "lake_const.h"
#include "media_lake_notify_info.h"
#include "medialibrary_unistore_manager.h"

namespace OHOS::Media {
class FileParser {
public:
    FileParser(const std::string &path, LakeScanMode scanMode = LakeScanMode::INCREMENT);
    FileParser(const MediaLakeNotifyInfo &info, LakeScanMode scanMode = LakeScanMode::INCREMENT);
    ~FileParser() = default;
    bool CheckTypeValid();
    bool CheckSizeValid();
    bool CheckIsNotHidden();
    bool IsFileValidAsset();
    int32_t GetUniqueId();
    void ParseFileInfo();
    FileUpdateType GetFileUpdateType();
    InnerFileInfo GetFileInfo();
    std::string PrintInfo(const InnerFileInfo& info);
    int32_t UpdateAssetInfo();
    int32_t UpdateAssetInfo(int32_t albumId, const std::string &bundleName, const std::string &albumName);
    NativeRdb::ValuesBucket TransFileInfoToBucket(int32_t albumId, const std::string &bundleName,
        const std::string &albumName);
    std::string GetFileAssetUri();
    std::string ToString();
    static std::vector<std::string> GenerateThumbnail(LakeScanMode scanMode, const std::vector<std::string> &inodes);
    static std::vector<std::string> GetFileUris(const std::vector<std::string> &inodes);
    static int32_t GenerateSingleThumbnail(const ThumbnailInfo &info);

private:
    struct PhotosRowData {
        int32_t fileId {0};
        int32_t mediaType {0};
        int32_t fileSourceType {0};
        int32_t ownerAlbumId {0};
        int64_t size {0};
        int64_t dateModified {0};
        int64_t dateTaken {0};
        std::string data;
        std::string inode;
        std::string mimeType;
        std::string storagePath;
        std::string ownerPackage;
        std::string packageName;
        bool IsExist();
        std::string ToString() const;
    };
    struct MetaStatus {
        bool isMediaTypeChanged {false};
        bool isSizeChanged {false};
        bool isDateModifiedChanged {false};
        bool isMimeTypeChanged {false};
        bool isStoragePathChanged {false};
        bool IsChanged() const;
        std::string ToString() const;
    };

    IsBurstType CheckBurst(const std::string &displayName);
    bool HasChangePart(const PhotosRowData &rowData);
    bool IsStoragePathChanged(const PhotosRowData &rowData);
    bool IsDateModifiedChanged();
    bool IsNotifyInfoValid();
    PhotosRowData FindSameFile();
    PhotosRowData FindSameFileByOptAdd();
    PhotosRowData FindSameFileByOptMod();
    PhotosRowData FindSameFileByDefault();
    PhotosRowData FindSameFileByStoragePath(const std::string &storagePath);
    PhotosRowData FindSameFileInDatabase(const std::string &querySql,
        const std::vector<NativeRdb::ValueObject> &params);

    void SetFileId(int32_t fileId);
    void SetAlbumInfo(int32_t albumId, const std::string &bundleName, const std::string &albumName);
    void SetCloudPath(int32_t uniqueId);
    void SetByPhotosRowData(const PhotosRowData &rowData);

    NativeRdb::ValuesBucket GetAssetInsertValues();
    NativeRdb::ValuesBucket GetAssetUpdateValues();
    NativeRdb::ValuesBucket GetAssetCommonValues();
    void SetAssetAlbumValues(NativeRdb::ValuesBucket &values);
    void SetAssetBurstValues(NativeRdb::ValuesBucket &values);
    void SetAssetCloudEnhancementValues(NativeRdb::ValuesBucket &values);
    void SetAssetLocationValues(NativeRdb::ValuesBucket &values);
    void PutStringVal(NativeRdb::ValuesBucket &values, const std::string &columnName, const std::string &columnVal);
    static std::string GetThumbnailUri(const ThumbnailInfo &info);
    int32_t UpdateAssetInDatabase();
    bool ShouldGenerateThumbnail();
    ThumbnailInfo GetThumbnailInfo();
 
private:
    static std::atomic<uint32_t> imageNumber_;
    static std::atomic<uint32_t> videoNumber_;
    std::shared_ptr<MediaLibraryRdbStore> mediaLibraryRdb_;
    std::string path_;
    FileUpdateType updateType_ {FileUpdateType::NO_CHANGE};
    InnerFileInfo fileInfo_;
    MetaStatus metaStatus_;
    MediaLakeNotifyInfo notifyInfo_;
    const LakeScanMode scanMode_;

    const std::string SQL_PHOTOS_FIND_SAME_FILE_BY_STORAGE_PATH = "\
        SELECT file_id, size, date_modified, mime_type, media_type, inode, storage_path, file_source_type, \
        owner_album_id, owner_package, package_name, date_taken, data \
        FROM Photos \
        WHERE LOWER(storage_path) = LOWER(?) AND \
        (file_source_type = ? OR (file_source_type = ? AND position IN (?, ?) AND date_trashed = ? AND hidden = ?)) \
        LIMIT 1;";
};
} // namespace OHOS::Media
#endif // FILE_PARSER_H