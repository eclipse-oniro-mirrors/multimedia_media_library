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

#define MLOG_TAG "Media_Cloud_Service"

#include "cloud_media_scan_service.h"


#include "media_file_utils.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_notify.h"
#include "metadata.h"
#include "metadata_extractor.h"
#include "mimetype_utils.h"
#include "photo_album_column.h"
#include "scanner_utils.h"
#include "shooting_mode_column.h"

namespace OHOS::Media::CloudSync {

static int32_t GetFileMetadata(std::unique_ptr<Metadata> &data)
{
    if (data == nullptr) {
        MEDIA_ERR_LOG("data is nullptr");
        return E_FAIL;
    }
    struct stat statInfo {};
    if (stat(data->GetFilePath().c_str(), &statInfo) != 0) {
        MEDIA_ERR_LOG("stat syscall err %{public}d", errno);
        return E_FAIL;
    }
    data->SetFileSize(statInfo.st_size);
    auto dateModified = static_cast<int64_t>(MediaFileUtils::Timespec2Millisecond(statInfo.st_mtim));
    if (dateModified == 0) {
        dateModified = MediaFileUtils::UTCTimeMilliSeconds();
        MEDIA_WARN_LOG("Invalid dateModified from st_mtim, use current time instead: %{public}lld",
            static_cast<long long>(dateModified));
    }
    if (dateModified != 0 && data->GetFileDateModified() == 0) {
        data->SetFileDateModified(dateModified);
    }
    string extension = ScannerUtils::GetFileExtension(data->GetFileName());
    string mimeType = MimeTypeUtils::GetMimeTypeFromExtension(extension);
    data->SetFileExtension(extension);
    data->SetFileMimeType(mimeType);
    return E_OK;
}

int32_t CloudMediaScanService::FillMetadata(std::unique_ptr<Metadata>& data)
{
    if (data == nullptr) {
        MEDIA_ERR_LOG("data is nullptr");
        return E_FAIL;
    }
    int32_t err = GetFileMetadata(data);
    if (err != E_OK) {
        MEDIA_ERR_LOG("failed to get file metadata");
        return err;
    }
    if (data->GetFileMediaType() == MEDIA_TYPE_IMAGE) {
        err = MetadataExtractor::ExtractImageMetadata(data);
    } else {
        err = MetadataExtractor::ExtractAVMetadata(data);
    }
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "failed to extension data");
    return E_OK;
}

int32_t CloudMediaScanService::ScanMetaData(const string& path, std::unique_ptr<Metadata>& data)
{
    if (data == nullptr) {
        MEDIA_ERR_LOG("data is nullptr");
        return E_FAIL;
    }
    const string fileName = MediaFileUtils::GetFileName(path);
    data->SetFilePath(path);
    data->SetFileName(fileName);
    data->SetFileMediaType(MediaFileUtils::GetMediaType(fileName));
    return FillMetadata(data);
}

int32_t CloudMediaScanService::ScanDownloadedFile(const string& path, CloudMediaScanService::ScanResult& result)
{
    std::unique_ptr<Metadata> data = make_unique<Metadata>();
    if (data == nullptr) {
        MEDIA_ERR_LOG("data is nullptr");
        return E_FAIL;
    }
    if (ScanMetaData(path, data) != E_OK) {
        MEDIA_ERR_LOG("failed to scan metadata");
        return E_FAIL;
    }
    result.shootingMode = data->GetShootingMode();
    result.shootingModeTag = data->GetShootingModeTag();
    result.frontCamera = data->GetFrontCamera();
    result.hdrMode = data->GetHdrMode();
    result.dateTaken = data->GetDateTaken();
    result.detailTime = data->GetDetailTime();
    result.dateYear = data->GetDateYear();
    result.dateMonth = data->GetDateMonth();
    result.dateDay = data->GetDateDay();
    result.subType = data->GetPhotoSubType();
    result.mediaType = data->GetFileMediaType();
    result.height = data->GetFileHeight();
    result.width = data->GetFileWidth();
    result.aspectRatio = data->GetFileAspectRatio();
    result.exifRotate = data->GetExifRotate();
    result.scanSuccess = true;
    MEDIA_INFO_LOG("ScanDownloadedFile, result: %{public}s", result.ToString().c_str());
    return E_OK;
}

static void NotifyAnalysisAlbum(const vector<string>& changedAlbumIds)
{
    if (changedAlbumIds.size() <= 0) {
        return;
    }
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_LOG(watch != nullptr, "Can not get MediaLibraryNotify Instance");
    for (const string& albumId : changedAlbumIds) {
        watch->Notify(MediaFileUtils::GetUriByExtrConditions(
            PhotoAlbumColumns::ANALYSIS_ALBUM_URI_PREFIX, albumId), NotifyType::NOTIFY_UPDATE);
    }
}

void CloudMediaScanService::UpdateAndNotifyShootingModeAlbumIfNeeded(
    const CloudMediaScanService::ScanResult& scanResult)
{
    CHECK_AND_RETURN(scanResult.scanSuccess);
    vector<ShootingModeAlbumType> albumTypes = ShootingModeAlbum::GetShootingModeAlbumOfAsset(
        scanResult.subType, "", -1, scanResult.frontCamera, scanResult.shootingMode);

    vector<string> albumIdsToUpdate;
    for (const auto& type : albumTypes) {
        int32_t albumId;
        if (MediaLibraryRdbUtils::QueryShootingModeAlbumIdByType(type, albumId)) {
            albumIdsToUpdate.push_back(to_string(albumId));
        }
    }

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "rdbstore is nullptr");

    if (albumIdsToUpdate.size() > 0) {
        MediaLibraryRdbUtils::UpdateAnalysisAlbumInternal(rdbStore, albumIdsToUpdate);
        NotifyAnalysisAlbum(albumIdsToUpdate);
    }
}

std::string CloudMediaScanService::ScanResult::ToString() const
{
    std::stringstream ss;
    ss << "scanSuccess:: " << scanSuccess << ", "
       << "shootingMode: " << shootingMode << ", "
       << "shootingModeTag" << shootingModeTag << ", "
       << "frontCamera" << frontCamera << ", "
       << "subType" << subType;
    return ss.str();
}
}  // namespace OHOS::Media::CloudSync