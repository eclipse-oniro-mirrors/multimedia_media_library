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
#include <mutex>
#include <unordered_map>
 
#include "asset_accurate_refresh.h"
#include "file_const.h"
#include "file_scan_utils.h"
#include "media_file_notify_info.h"
#include "medialibrary_unistore_manager.h"
#include "metadata.h"
#include "photo_dao.h"

namespace OHOS::Media {
extern std::mutex g_fileManagerScanFlagMutex;
extern std::unordered_map<int32_t, bool> g_fileManagerScanFlag;
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
    static void SetFileManagerScanFlagBySingle(const std::string &fileIdStr, bool stopScan);
    virtual bool IsFileValidAsset();
    virtual FileUpdateType GetFileUpdateType() = 0;

protected:
    using PhotosRowData = PhotoDao::PhotosRowData;
    struct MetaStatus {
        bool isMediaTypeChanged {false};
        bool isSizeChanged {false};
        bool isDateModifiedChanged {false};
        bool isMimeTypeChanged {false};
        bool isStoragePathChanged {false};
        bool isInvisible {false};
        bool IsChanged() const;
        std::string cloudId;
        std::string displayName;
        std::string ToString() const;
    };

    bool IsNotifyInfoValid();
    bool HasChangePart(const PhotosRowData &rowData);
    bool IsStoragePathChanged(const PhotosRowData &rowData);
    PhotosRowData FindSameFile();
    void SetByPhotosRowData(const PhotosRowData &rowData);
    void SetDateTakenFields(const PhotosRowData &rowData);
    PhotosRowData FindSameFileByStoragePath(const std::string &storagePath);
    // 从 Metadata 设置 subtype，子类可重写以修改行为
    virtual void SetSubtypeFromMetadata(std::unique_ptr<Metadata> &data) = 0;
private:
    IsBurstType CheckBurst(const std::string &displayName);
    bool IsDateModifiedChanged();
    PhotosRowData FindSameFileByOptAdd();
    PhotosRowData FindSameFileByOptMod();
    PhotosRowData FindSameFileByDefault();

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
    void SetAssetLivePhoto4dValues(NativeRdb::ValuesBucket &values);
    void PutStringVal(NativeRdb::ValuesBucket &values, const std::string &columnName, const std::string &columnVal);
    int32_t UpdateAssetInDatabase();
    bool IsCinematicVideoV2Asset();
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

protected:
    MediaNotifyInfo notifyInfo_;
    const FileSourceType sourceType_;
    const ScanMode scanMode_;
    InnerFileInfo fileInfo_;
    FileUpdateType updateType_ {FileUpdateType::NO_CHANGE};
};
} // namespace OHOS::Media
#endif // FILE_PARSER_H