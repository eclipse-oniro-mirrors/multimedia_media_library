/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#define MLOG_TAG "Scanner"

#include "media_scanner.h"

#include "directory_ex.h"
#include "hitrace_meter.h"
#include "ipc_skeleton.h"
#include "rdb_predicates.h"
#include "media_file_utils.h"
#include "media_file_uri.h"
#include "media_log.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_notify.h"
#include "medialibrary_vision_operations.h"
#include "mimetype_utils.h"
#include "post_event_utils.h"
#include "photo_map_column.h"
#include "photo_album_column.h"
#include "vision_column.h"
#include "moving_photo_file_utils.h"
#include "medialibrary_notify_new.h"
#include "asset_accurate_refresh.h"
#include "medialibrary_photo_operations.h"
#include "refresh_business_name.h"

namespace OHOS {
namespace Media {
using namespace std;
using namespace OHOS::AppExecFwk;
using namespace OHOS::DataShare;

MediaScannerObj::MediaScannerObj(const std::string &path, const std::shared_ptr<IMediaScannerCallback> &callback,
    MediaScannerObj::ScanType type, MediaLibraryApi api) : type_(type), callback_(callback), api_(api)
{
    if (type_ == DIRECTORY) {
        dir_ = path;
    } else if (type_ == FILE) {
        path_ = path;
    } else if (type_ == CAMERA_SHOT_MOVING_PHOTO) {
        path_ = path;
        isCameraShotMovingPhoto_ = true;
    }
    // when path is /Photo, it means update or clone scene
    skipPhoto_ = path.compare("/storage/cloud/files/Photo") != 0;
    stopFlag_ = make_shared<bool>(false);
}

MediaScannerObj::MediaScannerObj(MediaScannerObj::ScanType type, MediaLibraryApi api) : type_(type), api_(api)
{
}

void MediaScannerObj::SetStopFlag(std::shared_ptr<bool> &flag)
{
    stopFlag_ = flag;
}

void MediaScannerObj::SetErrorPath(const std::string &path)
{
    errorPath_ = path;
}

void MediaScannerObj::SetForceScan(bool isForceScan)
{
    isForceScan_ = isForceScan;
}

void MediaScannerObj::SetFileId(int32_t fileId)
{
    fileId_ = fileId;
}

void MediaScannerObj::SetIsSkipAlbumUpdate(bool isSkipAlbumUpdate)
{
    isSkipAlbumUpdate_ = isSkipAlbumUpdate;
}

int32_t MediaScannerObj::ScanFile()
{
    MEDIA_DEBUG_LOG("scan file %{private}s", path_.c_str());

    int32_t ret = ScanFileInternal();
    if (ret != E_OK) {
        MEDIA_ERR_LOG("ScanFileInternal err %{public}d", ret);
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, ret},
            {KEY_OPT_FILE, path_}, {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::FILE_OPT_ERR, map);
    }

    (void)InvokeCallback(ret);

    return ret;
}

int32_t MediaScannerObj::ScanDir()
{
    MEDIA_INFO_LOG("scan dir %{public}s", dir_.c_str());
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    int32_t ret = ScanDirInternal();
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("scan dir cost: %{public}ld", (long)(end - start));
    if (ret != E_OK) {
        MEDIA_ERR_LOG("ScanDirInternal err %{public}d", ret);
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, ret},
            {KEY_OPT_FILE, path_}, {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::FILE_OPT_ERR, map);
    }

    (void)InvokeCallback(ret);

    return ret;
}

void MediaScannerObj::Scan()
{
    switch (type_) {
        case FILE:
        case CAMERA_SHOT_MOVING_PHOTO:
            ScanFile();
            break;
        case DIRECTORY:
            ScanDir();
            break;
        case START:
            Start();
            break;
        case ERROR:
            ScanError();
            break;
        case SET_ERROR:
            SetError();
            break;
        default:
            break;
    }
}

int32_t MediaScannerObj::InvokeCallback(int32_t err)
{
    if (callback_ == nullptr || err == E_NO_THUMB) {
        return E_OK;
    }

    return callback_->OnScanFinished(err, uri_, path_);
}

int32_t MediaScannerObj::CommitTransaction()
{
    unordered_set<MediaType> mediaTypeSet = {};
    string uri;
    unique_ptr<Metadata> data;
    string tableName;

    // will begin a transaction in later pr
    for (uint32_t i = 0; i < dataBuffer_.size(); i++) {
        data = move(dataBuffer_[i]);
        if (data->GetFileId() != FILE_ID_DEFAULT) {
            MediaLibraryApi api = skipPhoto_ ? MediaLibraryApi::API_OLD : MediaLibraryApi::API_10;
            uri = mediaScannerDb_->UpdateMetadata(*data, tableName, api, skipPhoto_);
            scannedIds_.insert(make_pair(tableName, data->GetFileId()));
        } else {
            uri = mediaScannerDb_->InsertMetadata(*data, tableName);
            scannedIds_.insert(make_pair(tableName,
                stoi(MediaFileUtils::GetIdFromUri(uri))));
        }

        // set uri for callback
        uri_ = uri;
        mediaTypeSet.insert(data->GetFileMediaType());
    }

    dataBuffer_.clear();

    mediaScannerDb_->UpdateAlbumInfo();
    for (const MediaType &mediaType : mediaTypeSet) {
        mediaScannerDb_->NotifyDatabaseChange(mediaType);
    }

    return E_OK;
}

int32_t MediaScannerObj::AddToTransaction()
{
    dataBuffer_.emplace_back(move(data_));
    if (dataBuffer_.size() >= MAX_BATCH_SIZE) {
        return CommitTransaction();
    }

    return E_OK;
}

string GetUriWithoutSeg(const string &oldUri)
{
    size_t questionMaskPoint = oldUri.rfind('?');
    if (questionMaskPoint != string::npos) {
        return oldUri.substr(0, questionMaskPoint);
    }
    return oldUri;
}

static string GetAlbumId(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const string &albumName)
{
    NativeRdb::RdbPredicates predicates(ANALYSIS_ALBUM_TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_NAME, albumName);
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, "", "rdbStore nullptr");

    vector<string> columns = {PhotoAlbumColumns::ALBUM_ID};
    auto albumResult = rdbStore->Query(predicates, columns);
    bool cond = (albumResult == nullptr || albumResult->GoToNextRow() != NativeRdb::E_OK);
    CHECK_AND_RETURN_RET_LOG(!cond, "", "query fails, no target shootingmode");

    int32_t index = 0;
    if (albumResult->GetColumnIndex(PhotoAlbumColumns::ALBUM_ID, index) != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("get column index fails");
        return "";
    }
    string album_id = "";
    if (albumResult->GetString(index, album_id) != NativeRdb::E_OK) {
        return "";
    }
    return album_id;
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

static void UpdateAndNotifyShootingModeAlbumOfAsset(std::unique_ptr<Metadata>& data)
{
    vector<ShootingModeAlbumType> albumTypes = ShootingModeAlbum::GetShootingModeAlbumOfAsset(
        data->GetPhotoSubType(), data->GetFileMimeType(), data->GetMovingPhotoEffectMode(),
        data->GetFrontCamera(), data->GetShootingMode());

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

int32_t MediaScannerObj::Commit()
{
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    string tableName;
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_RET_LOG(watch != nullptr, E_ERR, "Can not get MediaLibraryNotify Instance");
    auto assetRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>(
        AccurateRefresh::SCAN_FILE_BUSSINESS_NAME);
    if (data_->GetFileId() != FILE_ID_DEFAULT) {
        uri_ = mediaScannerDb_->UpdateMetadata(*data_, tableName, api_, true, assetRefresh);
        if (!isSkipAlbumUpdate_) {
            assetRefresh->RefreshAlbum(static_cast<NotifyAlbumType>(NotifyAlbumType::SYS_ALBUM |
                NotifyAlbumType::USER_ALBUM | NotifyAlbumType::SOURCE_ALBUM));
            UpdateAndNotifyShootingModeAlbumOfAsset(data_);
        }
        if (watch != nullptr && data_->GetIsTemp() == FILE_IS_TEMP_DEFAULT && data_->GetBurstCoverLevel() == COVER
            && data_->GetFileSourceType() != static_cast<int32_t>(FileSourceTypes::TEMP_FILE_MANAGER)) {
            if (data_->GetForAdd()) {
                watch->Notify(GetUriWithoutSeg(uri_), NOTIFY_ADD);
            } else {
                watch->Notify(GetUriWithoutSeg(uri_), NOTIFY_UPDATE);
            }
        }
    } else {
        MEDIA_INFO_LOG("insert new file: %{public}s", data_->GetFilePath().c_str());
        uri_ = mediaScannerDb_->InsertMetadata(*data_, tableName, api_, assetRefresh);
        assetRefresh->RefreshAlbum(static_cast<NotifyAlbumType>(NotifyAlbumType::SYS_ALBUM |
            NotifyAlbumType::USER_ALBUM | NotifyAlbumType::SOURCE_ALBUM));
        UpdateAndNotifyShootingModeAlbumOfAsset(data_);
        if (watch != nullptr && data_->GetIsTemp() == FILE_IS_TEMP_DEFAULT
            && data_->GetFileSourceType() != static_cast<int32_t>(FileSourceTypes::TEMP_FILE_MANAGER)) {
            watch->Notify(GetUriWithoutSeg(uri_), NOTIFY_ADD);
        }
    }

    int32_t fileId = data_->GetFileId();
    int32_t ret = MediaLibraryPhotoOperations::CalSingleEditDataSize(std::to_string(fileId));
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CalSingleEditDataSize failed for ID: %{public}d (ret code: %{public}d)",
            fileId, ret);
    }

    assetRefresh->Notify();
    // notify change
    mediaScannerDb_->NotifyDatabaseChange(data_->GetFileMediaType());
    data_ = nullptr;
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t duration = endTime - startTime;
    // 在CPU占用率80%, 运行脚本一直执行连拍100次操作, 查看极端场景下用时大于400ms的频率
    if (duration > 400) {
        MEDIA_HILOG(HILOG_IMPL, LOG_INFO, "Process duration: %" PRId64 " milliseconds", duration);
    }

    return E_OK;
}

int32_t MediaScannerObj::GetMediaInfo()
{
    auto pos = data_->GetFileMimeType().find_first_of("/");
    string mimePrefix = data_->GetFileMimeType().substr(0, pos) + "/*";
    if (find(EXTRACTOR_SUPPORTED_MIME.begin(), EXTRACTOR_SUPPORTED_MIME.end(),
        mimePrefix) != EXTRACTOR_SUPPORTED_MIME.end()) {
        return MetadataExtractor::Extract(data_, isCameraShotMovingPhoto_);
    }

    return E_OK;
}

#ifdef MEDIALIBRARY_COMPATIBILITY
void MediaScannerObj::SetPhotoSubType(const string &parent)
{
    if ((data_->GetFileMediaType() != MediaType::MEDIA_TYPE_IMAGE) &&
        (data_->GetFileMediaType() != MediaType::MEDIA_TYPE_VIDEO)) {
        return;
    }

    string parentPath = parent + SLASH_CHAR;
    if (parentPath.find(ROOT_MEDIA_DIR) != 0) {
        return;
    }

    size_t len = ROOT_MEDIA_DIR.length();
    parentPath.erase(0, len);

    if (parentPath == CAMERA_PATH) {
        data_->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::CAMERA));
    } else if ((parentPath == SCREEN_RECORD_PATH) || (parentPath == SCREEN_SHOT_PATH)) {
        data_->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::SCREENSHOT));
    }
}
#endif

static bool SkipBucket(const string &albumPath)
{
    string path = albumPath;
    if (path.back() != SLASH_CHAR) {
        path += SLASH_CHAR;
    }

    if (path.find(ROOT_MEDIA_DIR + AUDIO_BUCKET + SLASH_CHAR) != string::npos) {
        return true;
    }

    if (path.find(ROOT_MEDIA_DIR + PHOTO_BUCKET + SLASH_CHAR) != string::npos) {
        return true;
    }
    return false;
}

int32_t MediaScannerObj::GetParentDirInfo(const string &parent, int32_t parentId)
{
    if (api_ == MediaLibraryApi::API_10 || SkipBucket(parent)) {
        return E_OK;
    }
    size_t len = ROOT_MEDIA_DIR.length();
    string parentPath = parent + SLASH_CHAR;

    if (parentPath.find(ROOT_MEDIA_DIR) != 0) {
        MEDIA_ERR_LOG("invaid path %{private}s, not managed by scanner", path_.c_str());
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_DATA},
            {KEY_OPT_FILE, path_}, {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::FILE_OPT_ERR, map);
        return E_DATA;
    }

    parentPath.erase(0, len);
    if (!parentPath.empty()) {
        data_->SetRelativePath(parentPath);
        parentPath = string("/") + parentPath.substr(0, parentPath.length() - 1);
        data_->SetAlbumName(ScannerUtils::GetFileNameFromUri(parentPath));
    }

    if (parentId == UNKNOWN_ID) {
        parentId = mediaScannerDb_->GetIdFromPath(parent);
        if (parentId == UNKNOWN_ID) {
            if (parent == ROOT_MEDIA_DIR) {
                parentId = 0;
            } else {
                MEDIA_ERR_LOG("failed to get parent id");
                VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_DATA},
                    {KEY_OPT_FILE, path_}, {KEY_OPT_TYPE, OptType::SCAN}};
                PostEventUtils::GetInstance().PostErrorProcess(ErrType::FILE_OPT_ERR, map);
                return E_DATA;
            }
        }
    }
    data_->SetParentId(parentId);

    return E_OK;
}

void ParseLivePhoto(const std::string& path, const std::unique_ptr<Metadata>& data)
{
    if (!MediaFileUtils::IsMovingPhotoMimeType(data->GetFileMimeType())) {
        return;
    }
    if (!MovingPhotoFileUtils::IsLivePhoto(path)) {
        return;
    }

    string extraDataDir = MovingPhotoFileUtils::GetMovingPhotoExtraDataDir(path);
    if (extraDataDir.empty()) {
        MEDIA_ERR_LOG("failed to get local extra data dir");
        return;
    }
    if (!MediaFileUtils::IsFileExists(extraDataDir) && !MediaFileUtils::CreateDirectory(extraDataDir)) {
        MEDIA_ERR_LOG("Failed to create file, path:%{private}s", extraDataDir.c_str());
        return;
    }
    if (MovingPhotoFileUtils::ConvertToMovingPhoto(path, path,
            MovingPhotoFileUtils::GetMovingPhotoVideoPath(path),
            MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(path)) == E_OK) {
        size_t imageSize;
        if (MediaFileUtils::GetFileSize(path, imageSize)) {
            data->SetFileSize(static_cast<int64_t>(imageSize));
        }
        data->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    }
}

static void GetFileInfo(const string &path, int64_t &size, int64_t &dateModified)
{
    struct stat statInfo {};
    if (stat(path.c_str(), &statInfo) != E_OK) {
        MEDIA_ERR_LOG("stat error, path: %{public}s, errno: %{public}d", path.c_str(), errno);
        return;
    }
    size = statInfo.st_size;
    dateModified = MediaFileUtils::Timespec2Millisecond(statInfo.st_mtim);
}

static void GetMovingPhotoFileInfo(const string &imagePath, int64_t &movingPhotoSize,
    int64_t &movingPhotoDateModified)
{
    int64_t imageSize = 0;
    int64_t videoSize = 0;
    size_t extraDataSize = 0;
    int64_t imageDateModified = 0;
    int64_t videoDateModified = 0;
    string videoPath = MovingPhotoFileUtils::GetMovingPhotoVideoPath(imagePath);
    string extraDataPath = MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(imagePath);
    GetFileInfo(imagePath, imageSize, imageDateModified);
    GetFileInfo(videoPath, videoSize, videoDateModified);
    (void)MediaFileUtils::GetFileSize(extraDataPath, extraDataSize);
    movingPhotoSize = imageSize + videoSize + static_cast<int64_t>(extraDataSize);
    movingPhotoDateModified = imageDateModified >= videoDateModified ? imageDateModified : videoDateModified;
}

static bool IsFileNotChanged(const unique_ptr<Metadata> &data, const struct stat &statInfo)
{
    int64_t previousDateModified = data->GetFileDateModified();
    int64_t previousSize = data->GetFileSize();
    int64_t currentDateModified = MediaFileUtils::Timespec2Millisecond(statInfo.st_mtim);
    int64_t currentSize = statInfo.st_size;
    if (data->GetPhotoSubType() == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO) ||
        data->GetMovingPhotoEffectMode() == static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY)) {
        GetMovingPhotoFileInfo(data->GetFilePath(), currentSize, currentDateModified);
    }
    return previousDateModified == currentDateModified && previousSize == currentSize;
}

int32_t MediaScannerObj::BuildData(const struct stat &statInfo)
{
    data_ = make_unique<Metadata>();
    if (data_ == nullptr) {
        MEDIA_ERR_LOG("failed to make unique ptr for metadata");
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_DATA},
            {KEY_OPT_FILE, path_}, {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::FILE_OPT_ERR, map);
        return E_DATA;
    }

    if (S_ISDIR(statInfo.st_mode)) {
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_INVALID_ARGUMENTS},
            {KEY_OPT_FILE, path_}, {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::FILE_OPT_ERR, map);
        return E_INVALID_ARGUMENTS;
    }

    int32_t err = mediaScannerDb_->GetFileBasicInfo(path_, data_, api_, fileId_);
    if (err != E_OK) {
        MEDIA_ERR_LOG("failed to get file basic info");
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, err},
            {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return err;
    }
    if (data_->GetFileDateModified() == 0) {
        data_->SetForAdd(true);
    }

    // file path
    data_->SetFilePath(path_);
    // may need isPending here
    if (IsFileNotChanged(data_, statInfo) && !isForceScan_) {
        scannedIds_.insert(make_pair(data_->GetTableName(), data_->GetFileId()));
        if (path_.find(ROOT_MEDIA_DIR + PHOTO_BUCKET) != string::npos) {
            MEDIA_WARN_LOG("no need to scan, date_modified:%{public}ld, size:%{public}ld, pending:%{public}ld",
                static_cast<long>(data_->GetFileDateModified()), static_cast<long>(data_->GetFileSize()),
                static_cast<long>(data_->GetTimePending()));
        }
        return E_SCANNED;
    }

    if (data_->GetFileId() == FILE_ID_DEFAULT) {
        data_->SetFileName(ScannerUtils::GetFileNameFromUri(path_));
    }
    data_->SetFileTitle(ScannerUtils::GetFileTitle(data_->GetFileName()));

    // statinfo
    data_->SetFileSize(statInfo.st_size);
    // Temp file will not be notified. Do not set fileDateModified to keep GetForAdd true
    if (!data_->GetIsTemp()) {
        data_->SetFileDateModified(static_cast<int64_t>(MediaFileUtils::Timespec2Millisecond(statInfo.st_mtim)));
    }

    // extension and type
    string extension = ScannerUtils::GetFileExtension(path_);
    string mimeType = MimeTypeUtils::GetMimeTypeFromContent(path_);
    data_->SetFileExtension(extension);
    data_->SetFileMimeType(mimeType);
    data_->SetFileMediaType(MimeTypeUtils::GetMediaTypeFromMimeType(mimeType));
    ParseLivePhoto(path_, data_);
    return E_OK;
}

int32_t MediaScannerObj::GetFileMetadata()
{
    if (path_.empty()) {
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_INVALID_ARGUMENTS},
            {KEY_OPT_FILE, path_}, {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::FILE_OPT_ERR, map);
        return E_INVALID_ARGUMENTS;
    }
    struct stat statInfo = { 0 };
    if (stat(path_.c_str(), &statInfo) != 0) {
        MEDIA_ERR_LOG("stat syscall err %{public}d", errno);
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_SYSCALL},
            {KEY_OPT_FILE, path_}, {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::FILE_OPT_ERR, map);
        return E_SYSCALL;
    }
    if (statInfo.st_size == 0) {
        MEDIA_WARN_LOG("file size is 0, path: %{private}s", path_.c_str());
    }

    int errCode = BuildData(statInfo);
    if (errCode != E_OK) {
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, errCode},
            {KEY_OPT_FILE, path_}, {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::FILE_OPT_ERR, map);
        return errCode;
    }

    return E_OK;
}

int32_t MediaScannerObj::ScanFileInternal()
{
    if (ScannerUtils::IsFileHidden(path_)) {
        MEDIA_ERR_LOG("the file is hidden");
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_FILE_HIDDEN},
            {KEY_OPT_FILE, path_}, {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::FILE_OPT_ERR, map);
        return E_FILE_HIDDEN;
    }

    int32_t err = GetFileMetadata();
    if (err != E_OK) {
        if (err != E_SCANNED) {
            MEDIA_ERR_LOG("failed to get file metadata");
            VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, err},
                {KEY_OPT_FILE, path_}, {KEY_OPT_TYPE, OptType::SCAN}};
            PostEventUtils::GetInstance().PostErrorProcess(ErrType::FILE_OPT_ERR, map);
        }
        return err;
    }
    string parent = ScannerUtils::GetParentPath(path_);
    err = GetParentDirInfo(parent, UNKNOWN_ID);
    if (err != E_OK) {
        MEDIA_ERR_LOG("failed to get dir info");
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, err},
            {KEY_OPT_FILE, path_}, {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::FILE_OPT_ERR, map);
        return err;
    }

    err = GetMediaInfo();
    if (err != E_OK) {
        MEDIA_ERR_LOG("failed to get media info, error: %{public}d", err);
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, err},
            {KEY_OPT_FILE, path_}, {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::FILE_OPT_ERR, map);
        // no return here for fs metadata being updated or inserted
    }

    bool isShareScene = data_->GetForAdd() && data_->GetOwnerPackage() == "com.huawei.hmos.instantshare";
    err = Commit();
    if (err != E_OK) {
        MEDIA_ERR_LOG("failed to commit err %{public}d", err);
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, err},
            {KEY_OPT_FILE, path_}, {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::FILE_OPT_ERR, map);
        return err;
    }

    return isShareScene ? E_NO_THUMB : E_OK;
}

int32_t MediaScannerObj::BuildFileInfo(const string &parent, int32_t parentId)
{
    int32_t err = GetParentDirInfo(parent, parentId);
    if (err != E_OK) {
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, err},
            {KEY_OPT_FILE, path_}, {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::FILE_OPT_ERR, map);
        MEDIA_ERR_LOG("failed to get dir info");
        return err;
    }

    err = GetMediaInfo();
    if (err != E_OK) {
        MEDIA_ERR_LOG("failed to get media info");
        // no return here for fs metadata being updated or inserted
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, err},
            {KEY_OPT_FILE, path_}, {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::FILE_OPT_ERR, map);
    }

    err = AddToTransaction();
    if (err != E_OK) {
        MEDIA_ERR_LOG("failed to add to transaction err %{public}d", err);
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, err},
            {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return err;
    }
    
    return E_OK;
}

int32_t MediaScannerObj::ScanFileInTraversal(const string &path, const string &parent, int32_t parentId)
{
    path_ = path;
    if (ScannerUtils::IsFileHidden(path_)) {
        MEDIA_ERR_LOG("the file is hidden");
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_FILE_HIDDEN},
            {KEY_OPT_FILE, path_}, {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::FILE_OPT_ERR, map);
        return E_FILE_HIDDEN;
    }
    int32_t err = GetFileMetadata();
    if (err != E_OK) {
        if (err != E_SCANNED) {
            MEDIA_ERR_LOG("failed to get file metadata");
            VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, err},
                {KEY_OPT_FILE, path_}, {KEY_OPT_TYPE, OptType::SCAN}};
            PostEventUtils::GetInstance().PostErrorProcess(ErrType::FILE_OPT_ERR, map);
        }
        return err;
    }

    if (data_->GetTimePending() != 0) {
        MEDIA_INFO_LOG("File %{private}s is pending", path.c_str());
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_IS_PENDING},
            {KEY_OPT_FILE, path_}, {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::FILE_OPT_ERR, map);
        return E_IS_PENDING;
    }

#ifdef MEDIALIBRARY_COMPATIBILITY
    SetPhotoSubType(parent);
#endif
    
    err = BuildFileInfo(parent, parentId);
    if (err != E_OK) {
        MEDIA_ERR_LOG("failed to get other file metadata");
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, err},
            {KEY_OPT_FILE, path_}, {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::FILE_OPT_ERR, map);
        return err;
    }
    
    return E_OK;
}

int32_t MediaScannerObj::InsertOrUpdateAlbumInfo(const string &albumPath, int32_t parentId,
    const string &albumName)
{
    struct stat statInfo;
    int32_t albumId = UNKNOWN_ID;
    bool update = false;

    if (stat(albumPath.c_str(), &statInfo)) {
        MEDIA_ERR_LOG("stat dir error %{public}d", errno);
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, -errno},
            {KEY_OPT_FILE, albumPath}, {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::FILE_OPT_ERR, map);
        return UNKNOWN_ID;
    }
    if (SkipBucket(albumPath)) {
        return FILE_ID_DEFAULT;
    }
    if (albumMap_.find(albumPath) != albumMap_.end()) {
        Metadata albumInfo = albumMap_.at(albumPath);
        albumId = albumInfo.GetFileId();

        if ((albumInfo.GetFileDateModified() / MSEC_TO_SEC) == statInfo.st_mtime) {
            scannedIds_.insert(make_pair(MEDIALIBRARY_TABLE, albumId));
            return albumId;
        } else {
            update = true;
        }
    }

    Metadata metadata;
    metadata.SetFilePath(albumPath);
    metadata.SetFileName(ScannerUtils::GetFileNameFromUri(albumPath));
    metadata.SetFileTitle(ScannerUtils::GetFileTitle(metadata.GetFileName()));
    metadata.SetFileMediaType(static_cast<MediaType>(MEDIA_TYPE_ALBUM));
    metadata.SetFileSize(statInfo.st_size);
    metadata.SetFileDateModified(MediaFileUtils::Timespec2Millisecond(statInfo.st_mtim));

    string relativePath = ScannerUtils::GetParentPath(albumPath) + SLASH_CHAR;
    metadata.SetRelativePath(relativePath.erase(0, ROOT_MEDIA_DIR.length()));
    metadata.SetParentId(parentId);
    metadata.SetAlbumName(albumName);

    if (update) {
        metadata.SetFileId(albumId);
        albumId = mediaScannerDb_->UpdateAlbum(metadata);
    } else {
        albumId = mediaScannerDb_->InsertAlbum(metadata);
    }
    scannedIds_.insert(make_pair(MEDIALIBRARY_TABLE, albumId));

    return albumId;
}

int32_t MediaScannerObj::CleanupDirectory()
{
    unordered_set<MediaType> mediaTypeSet;
    vector<string> scanTables = {
        MEDIALIBRARY_TABLE, PhotoColumn::PHOTOS_TABLE, AudioColumn::AUDIOS_TABLE
    };

    for (auto table : scanTables) {
        // clean up table
        vector<string> deleteIdList;
        unordered_map<int32_t, MediaType> prevIdMap;
        if (table == PhotoColumn::PHOTOS_TABLE) {
            prevIdMap = mediaScannerDb_->GetIdsFromFilePath(dir_, PhotoColumn::PHOTOS_TABLE,
                ROOT_MEDIA_DIR + PHOTO_BUCKET);
        } else if (table == AudioColumn::AUDIOS_TABLE) {
            prevIdMap = mediaScannerDb_->GetIdsFromFilePath(dir_, AudioColumn::AUDIOS_TABLE,
                ROOT_MEDIA_DIR + AUDIO_BUCKET);
        } else {
            prevIdMap = mediaScannerDb_->GetIdsFromFilePath(dir_, table);
        }

        for (auto itr : prevIdMap) {
            auto it = scannedIds_.find(make_pair(table, itr.first));
            if (it != scannedIds_.end()) {
                scannedIds_.erase(it);
            } else {
                deleteIdList.push_back(to_string(itr.first));
                mediaTypeSet.insert(itr.second);
            }
        }

        if (!deleteIdList.empty()) {
            mediaScannerDb_->DeleteMetadata(deleteIdList, table);
        }
    }

    for (const MediaType &mediaType : mediaTypeSet) {
        mediaScannerDb_->NotifyDatabaseChange(mediaType);
    }

    scannedIds_.clear();

    return E_OK;
}

static int32_t OpenFailedOperation(const string &path)
{
    MEDIA_ERR_LOG("Failed to opendir %{private}s, errno %{private}d", path.c_str(), errno);
    VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, -errno},
        {KEY_OPT_FILE, path}, {KEY_OPT_TYPE, OptType::SCAN}};
    PostEventUtils::GetInstance().PostErrorProcess(ErrType::FILE_OPT_ERR, map);
    return ERR_NOT_ACCESSIBLE;
}

int32_t MediaScannerObj::WalkFileTree(const string &path, int32_t parentId)
{
    int err = E_OK;
    DIR *dirPath = nullptr;
    struct dirent *ent = nullptr;
    size_t len = path.length();
    struct stat statInfo;

    if (len >= FILENAME_MAX - 1) {
        return ERR_INCORRECT_PATH;
    }

    auto fName = (char *)calloc(FILENAME_MAX, sizeof(char));
    if (fName == nullptr) {
        return ERR_MEM_ALLOC_FAIL;
    }

    if (strcpy_s(fName, FILENAME_MAX, path.c_str()) != ERR_SUCCESS) {
        FREE_MEMORY_AND_SET_NULL(fName);
        return ERR_MEM_ALLOC_FAIL;
    }
    fName[len++] = '/';
    if ((dirPath = opendir(path.c_str())) == nullptr) {
        FREE_MEMORY_AND_SET_NULL(fName);
        return OpenFailedOperation(path);
    }

    while ((ent = readdir(dirPath)) != nullptr) {
        if (*stopFlag_) {
            err = E_STOP;
            break;
        }

        if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
            continue;
        }

        if (strncpy_s(fName + len, FILENAME_MAX - len, ent->d_name, FILENAME_MAX - len)) {
            continue;
        }

        if (lstat(fName, &statInfo) == -1) {
            continue;
        }

        string currentPath = fName;
        if (S_ISDIR(statInfo.st_mode)) {
            if (ScannerUtils::IsDirHidden(currentPath, skipPhoto_)) {
                continue;
            }
            MEDIA_INFO_LOG("Walk dir %{public}s", currentPath.c_str());
            int32_t albumId = InsertOrUpdateAlbumInfo(currentPath, parentId, ent->d_name);
            if (albumId == UNKNOWN_ID) {
                err = E_DATA;
                // might break in later pr for a rescan
                continue;
            }

            (void)WalkFileTree(currentPath, albumId);
        } else {
            (void)ScanFileInTraversal(currentPath, path, parentId);
        }
    }

    closedir(dirPath);
    FREE_MEMORY_AND_SET_NULL(fName);

    return err;
}

int32_t MediaScannerObj::ScanDirInternal()
{
    if (ScannerUtils::IsDirHiddenRecursive(dir_, skipPhoto_)) {
        MEDIA_ERR_LOG("the dir %{private}s is hidden", dir_.c_str());
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_DIR_HIDDEN},
            {KEY_OPT_FILE, dir_}, {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::FILE_OPT_ERR, map);
        return E_DIR_HIDDEN;
    }

    /*
     * 1. may query albums in batch for the big data case
     * 2. postpone this operation might avoid some conflicts
     */
    int32_t err = mediaScannerDb_->ReadAlbums(dir_, albumMap_);
    if (err != E_OK) {
        MEDIA_ERR_LOG("read albums err %{public}d", err);
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, err},
            {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return err;
    }
    /* no further operation when stopped */
    err = WalkFileTree(dir_, NO_PARENT);
    if (err != E_OK) {
        MEDIA_ERR_LOG("walk file tree err %{public}d", err);
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, err},
            {KEY_OPT_FILE, dir_}, {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::FILE_OPT_ERR, map);
        return err;
    }
    err = CommitTransaction();
    if (err != E_OK) {
        MEDIA_ERR_LOG("commit transaction err %{public}d", err);
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, err},
            {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return err;
    }
    err = CleanupDirectory();
    if (err != E_OK) {
        MEDIA_ERR_LOG("clean up dir err %{public}d", err);
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, err},
            {KEY_OPT_FILE, dir_}, {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::FILE_OPT_ERR, map);
        return err;
    }

    return E_OK;
}

int32_t MediaScannerObj::Start()
{
    int32_t ret = ScanError(true);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "scann error fail %{public}d", ret);

    /*
     * primary key wouldn't be duplicate
     */
    ret = mediaScannerDb_->RecordError(ROOT_MEDIA_DIR);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("record err fail %{public}d", ret);
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, ret},
            {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return ret;
    }

    return E_OK;
}

int32_t MediaScannerObj::ScanError(bool isBoot)
{
    auto errSet = mediaScannerDb_->ReadError();
    for (auto &err : errSet) {
        string realPath;
        if (!PathToRealPath(err, realPath)) {
            MEDIA_ERR_LOG("failed to get real path %{private}s, errno %{public}d", err.c_str(), errno);
            VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, -errno},
                {KEY_OPT_FILE, err}, {KEY_OPT_TYPE, OptType::SCAN}};
            PostEventUtils::GetInstance().PostErrorProcess(ErrType::FILE_OPT_ERR, map);
            (void)mediaScannerDb_->DeleteError(err);
            continue;
        }

        callback_ = make_shared<ScanErrCallback>(err);

        /*
         * Scan full path only when boot; all other errors are processed in
         * broadcast receving context.
         */
        if (err == ROOT_MEDIA_DIR) {
            if (isBoot) {
                dir_ = move(realPath);
                (void)ScanDir();
                break;
            } else {
                continue;
            }
        }

        /* assume err paths are correct */
        if (ScannerUtils::IsDirectory(realPath)) {
            dir_ = move(realPath);
            (void)ScanDir();
        } else if (ScannerUtils::IsRegularFile(realPath)) {
            path_ = move(realPath);
            (void)ScanFile();
        }
    }

    return E_OK;
}

int32_t MediaScannerObj::SetError()
{
    int32_t ret = mediaScannerDb_->RecordError(errorPath_);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "record err fail %{public}d", ret);
    return E_OK;
}
} // namespace Media
} // namespace OHOS
