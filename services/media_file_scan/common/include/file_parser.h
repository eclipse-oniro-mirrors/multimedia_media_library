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
 
#include "asset_accurate_refresh.h"
#include "file_const.h"
#include "media_file_notify_info.h"
#include "medialibrary_unistore_manager.h"
#include "metadata.h"

namespace OHOS::Media {
class FileParser {
public:
    FileParser(const std::string &path, const FileSourceType &sourceType,
        ScanMode scanMode = ScanMode::INCREMENT);
    FileParser(const MediaNotifyInfo &info, const FileSourceType &sourceType,
        ScanMode scanMode = ScanMode::INCREMENT);
    virtual ~FileParser() = default;
    bool CheckTypeValid();
    bool CheckSizeValid();
    bool CheckIsNotHidden();
    void ParseFileInfo();
    InnerFileInfo GetFileInfo();
    std::string PrintInfo(const InnerFileInfo& info);
    int32_t UpdateAssetInfo();
    int32_t UpdateAssetInfo(int32_t albumId, const std::string &bundleName, const std::string &albumName);
    NativeRdb::ValuesBucket TransFileInfoToBucket(int32_t albumId, const std::string &bundleName,
        const std::string &albumName);
    std::string GetFileAssetUri();
    std::string ToString();
    static std::vector<std::string> GenerateThumbnail(ScanMode scanMode, const std::vector<std::string> &inodes);
    static std::vector<std::string> GetFileUris(const std::vector<std::string> &inodes);
    static int32_t GenerateSingleThumbnail(const ThumbnailInfo &info);
    int32_t IsExistSameFileForCloneRestore(int32_t ownerAlbumId);
    static std::string GetThumbnailUri(const ThumbnailInfo &info);
    
    virtual bool IsFileValidAsset();
    virtual FileUpdateType GetFileUpdateType() = 0;

protected:
    // 公共查询逻辑：查询数据库获取 ThumbnailInfo 列表
    static int32_t QueryThumbnailInfos(const std::vector<std::string> &inodes,
        std::vector<ThumbnailInfo> &infos, std::vector<int32_t> &thumbnailVisibleList);

    struct PhotosRowData {
        int32_t fileId {0};
        int32_t mediaType {0};
        int32_t fileSourceType {0};
        int32_t ownerAlbumId {0};
        int32_t syncStatus {0};
        int64_t size {0};
        int64_t dateModified {0};
        int64_t dateTaken {0};
        int64_t editTime {0};
        std::string data;
        std::string inode;
        std::string mimeType;
        std::string storagePath;
        std::string ownerPackage;
        std::string packageName;
        std::string dateYear;
        std::string dateMonth;
        std::string dateDay;
        bool IsExist();
        std::string ToString() const;
        int32_t subtype {0};
        int32_t position {1};
    };
    struct MetaStatus {
        bool isMediaTypeChanged {false};
        bool isSizeChanged {false};
        bool isDateModifiedChanged {false};
        bool isMimeTypeChanged {false};
        bool isStoragePathChanged {false};
        bool isInvisible {false};
        bool IsChanged() const;
        std::string ToString() const;
    };

    bool IsNotifyInfoValid();
    bool HasChangePart(const PhotosRowData &rowData);
    bool IsStoragePathChanged(const PhotosRowData &rowData);
    PhotosRowData FindSameFile();
    void SetByPhotosRowData(const PhotosRowData &rowData);
    PhotosRowData FindSameFileByStoragePath(const std::string &storagePath);
    // 从 Metadata 设置 subtype，子类可重写以修改行为
    virtual void SetSubtypeFromMetadata(std::unique_ptr<Metadata> &data) = 0;
private:
    IsBurstType CheckBurst(const std::string &displayName);
    bool IsDateModifiedChanged();
    PhotosRowData FindSameFileByOptAdd();
    PhotosRowData FindSameFileByOptMod();
    PhotosRowData FindSameFileByDefault();
    PhotosRowData FindSameFileInDatabase(const std::string &querySql,
        const std::vector<NativeRdb::ValueObject> &params);

    void SetFileId(int32_t fileId);
    void SetAlbumInfo(int32_t albumId, const std::string &bundleName, const std::string &albumName);

    NativeRdb::ValuesBucket GetAssetInsertValues();
    NativeRdb::ValuesBucket GetAssetUpdateValues();
    NativeRdb::ValuesBucket GetAssetCommonValues();
    void SetAssetAlbumValues(NativeRdb::ValuesBucket &values);
    void SetAssetBurstValues(NativeRdb::ValuesBucket &values);
    void SetAssetCloudEnhancementValues(NativeRdb::ValuesBucket &values);
    void SetAssetLocationValues(NativeRdb::ValuesBucket &values);
    void SetAssetEditValues(NativeRdb::ValuesBucket &values);
    void PutStringVal(NativeRdb::ValuesBucket &values, const std::string &columnName, const std::string &columnVal);
    int32_t UpdateAssetInDatabase();
    bool ShouldGenerateThumbnail();
    ThumbnailInfo GetThumbnailInfo();
    int32_t SetAssetSubtypeValues(NativeRdb::ValuesBucket &values);

    int64_t GetFileDateAdded(const struct stat &statInfo);

    // 桶目录设置
    virtual void SetCloudPath() = 0;
 
private:
    std::shared_ptr<MediaLibraryRdbStore> mediaLibraryRdb_;
    std::string path_;
    MetaStatus metaStatus_;

    const std::string SQL_PHOTOS_FIND_SAME_FILE_BY_STORAGE_PATH = "\
        SELECT file_id, size, date_modified, mime_type, media_type, inode, storage_path, file_source_type, \
        owner_album_id, owner_package, package_name, date_taken, data, sync_status, edit_time, subtype, position, \
        date_year, date_month, date_day \
        FROM Photos \
        WHERE LOWER(storage_path) = LOWER(?) AND \
        (file_source_type = ? OR (file_source_type = ? AND position IN (?, ?) AND date_trashed = ? AND hidden = ?)) \
        LIMIT 1;";

    const std::string SQL_PHOTOS_FIND_SAME_FILE_FOR_CLONE_RESTORE = "\
        SELECT file_id \
        FROM Photos \
        WHERE owner_album_id = ? AND display_name = ? AND size = ? \
        AND (1 <> ? OR orientation = ?) \
        LIMIT 1;";

protected:
    MediaNotifyInfo notifyInfo_;
    const FileSourceType sourceType_;
    const ScanMode scanMode_;
    InnerFileInfo fileInfo_;
    FileUpdateType updateType_ {FileUpdateType::NO_CHANGE};
};
} // namespace OHOS::Media
#endif // FILE_PARSER_H