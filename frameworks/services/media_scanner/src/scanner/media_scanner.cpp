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
#include "mimetype_utils.h"
#include "post_event_utils.h"
#include "photo_map_column.h"
#include "photo_album_column.h"
#include "vision_column.h"

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
    }
    // when path is /Photo, it means update or clone scene
    skipPhoto_ = path.compare("/storage/cloud/files/Photo") != 0;
    stopFlag_ = make_shared<bool>(false);
}

MediaScannerObj::MediaScannerObj(MediaScannerObj::ScanType type) : type_(type)
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
    MEDIA_INFO_LOG("scan dir %{private}s", dir_.c_str());

    int32_t ret = ScanDirInternal();
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
    if (callback_ == nullptr) {
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

static string GetAlbumId(const shared_ptr<NativeRdb::RdbStore> &rdbStore,
    const string &albumName)
{
    RdbPredicates predicates(ANALYSIS_ALBUM_TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_NAME, albumName);
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdbStore nullptr");
        return "";
    }
    vector<string> columns = {PhotoAlbumColumns::ALBUM_ID};
    auto albumResult = rdbStore->Query(predicates, columns);
    if (albumResult == nullptr || albumResult->GoToNextRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("query fails, no target shootingmode");
        return "";
    }
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

static int32_t MaintainShootingModeMap(std::unique_ptr<Metadata> &data,
    shared_ptr<NativeRdb::RdbStore> &rdbStorePtr)
{
    string insertShootingModeSql = "INSERT OR IGNORE INTO " + ANALYSIS_PHOTO_MAP_TABLE +
        "(" + PhotoMap::ALBUM_ID + "," + PhotoMap::ASSET_ID + ") SELECT AnalysisAlbum.album_id, " +
        "Photos.file_id FROM " + PhotoColumn::PHOTOS_TABLE + " JOIN " +
        ANALYSIS_ALBUM_TABLE + " ON Photos.shooting_mode=AnalysisAlbum.album_name WHERE file_id = " +
        to_string(data->GetFileId()) + " AND AnalysisAlbum.album_subtype = " +
        to_string(PhotoAlbumSubType::SHOOTING_MODE);
    int32_t result = rdbStorePtr->ExecuteSql(insertShootingModeSql);
    return result;
}

static int32_t MaintainAlbumRelationship(std::unique_ptr<Metadata> &data)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStoreRaw();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdbstore is nullptr");
        return E_HAS_DB_ERROR;
    }
    auto rdbStorePtr = rdbStore->GetRaw();
    if (rdbStorePtr == nullptr) {
        MEDIA_ERR_LOG("obtain raw rdbstore fails");
        return E_HAS_DB_ERROR;
    }
    auto ret = MaintainShootingModeMap(data, rdbStorePtr);
    if (ret != E_OK) {
        return ret;
    }
    string album_id = GetAlbumId(rdbStorePtr, data->GetShootingMode());
    if (album_id.empty()) {
        MEDIA_ERR_LOG("Failed to query album_id,Album cover and count update fails");
        return E_ERR;
    }
    std::unordered_map<int32_t, int32_t> updateResult;
    MediaLibraryRdbUtils::UpdateAnalysisAlbumInternal(rdbStorePtr, updateResult, {album_id});
    return E_OK;
}

int32_t MediaScannerObj::Commit()
{
    string tableName;
    auto watch = MediaLibraryNotify::GetInstance();
    if (data_->GetFileId() != FILE_ID_DEFAULT) {
        uri_ = mediaScannerDb_->UpdateMetadata(*data_, tableName, api_);
        mediaScannerDb_->UpdateAlbumInfoByMetaData(*data_);
        if (watch != nullptr) {
            if (data_->GetForAdd()) {
                watch->Notify(GetUriWithoutSeg(uri_), NOTIFY_ADD);
            } else {
                watch->Notify(GetUriWithoutSeg(uri_), NOTIFY_UPDATE);
            }
        }
    } else {
        uri_ = mediaScannerDb_->InsertMetadata(*data_, tableName, api_);
        mediaScannerDb_->UpdateAlbumInfoByMetaData(*data_);
        if (watch != nullptr) {
            watch->Notify(GetUriWithoutSeg(uri_), NOTIFY_ADD);
        }
    }

    if (!data_->GetShootingMode().empty()) {
        auto err = MaintainAlbumRelationship(data_);
        if (err != E_OK) {
            return err;
        }
    }

    // notify change
    mediaScannerDb_->NotifyDatabaseChange(data_->GetFileMediaType());
    data_ = nullptr;

    return E_OK;
}

int32_t MediaScannerObj::GetMediaInfo()
{
    auto pos = data_->GetFileMimeType().find_first_of("/");
    string mimePrefix = data_->GetFileMimeType().substr(0, pos) + "/*";
    if (find(EXTRACTOR_SUPPORTED_MIME.begin(), EXTRACTOR_SUPPORTED_MIME.end(),
        mimePrefix) != EXTRACTOR_SUPPORTED_MIME.end()) {
        return MetadataExtractor::Extract(data_);
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

    int32_t err = mediaScannerDb_->GetFileBasicInfo(path_, data_, api_);
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

    // may need isPending here
    if ((data_->GetFileDateModified() == MediaFileUtils::Timespec2Millisecond(statInfo.st_mtim)) &&
        (data_->GetFileSize() == statInfo.st_size) && (!isForceScan_)) {
        scannedIds_.insert(make_pair(data_->GetTableName(), data_->GetFileId()));
        if (path_.find(ROOT_MEDIA_DIR + PHOTO_BUCKET) != string::npos) {
            MEDIA_ERR_LOG("no need to scan, date_modified:%{public}ld, size:%{public}ld, time_pending:%{public}ld",
                static_cast<long>(data_->GetFileDateModified()), static_cast<long>(data_->GetFileSize()),
                static_cast<long>(data_->GetTimePending()));
        }
        return E_SCANNED;
    }

    // file path
    data_->SetFilePath(path_);
    if (data_->GetFileId() == FILE_ID_DEFAULT) {
        data_->SetFileName(ScannerUtils::GetFileNameFromUri(path_));
    }
    data_->SetFileTitle(ScannerUtils::GetFileTitle(data_->GetFileName()));

    // statinfo
    data_->SetFileSize(statInfo.st_size);
    data_->SetFileDateModified(static_cast<int64_t>(MediaFileUtils::Timespec2Millisecond(statInfo.st_mtim)));

    // extension and type
    string extension = ScannerUtils::GetFileExtension(path_);
    string mimeType = MimeTypeUtils::GetMimeTypeFromExtension(extension);
    data_->SetFileExtension(extension);
    data_->SetFileMimeType(mimeType);
    data_->SetFileMediaType(MimeTypeUtils::GetMediaTypeFromMimeType(mimeType));

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

    err = Commit();
    if (err != E_OK) {
        MEDIA_ERR_LOG("failed to commit err %{public}d", err);
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, err},
            {KEY_OPT_FILE, path_}, {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::FILE_OPT_ERR, map);
        return err;
    }

    return E_OK;
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
        MEDIA_ERR_LOG("Failed to opendir %{private}s, errno %{private}d", path.c_str(), errno);
        FREE_MEMORY_AND_SET_NULL(fName);
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, -errno},
            {KEY_OPT_FILE, path}, {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::FILE_OPT_ERR, map);
        return ERR_NOT_ACCESSIBLE;
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
    if (ret != E_OK) {
        MEDIA_ERR_LOG("scann error fail %{public}d", ret);
        return ret;
    }

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
    if (ret != E_OK) {
        MEDIA_ERR_LOG("record err fail %{public}d", ret);
        return ret;
    }

    return E_OK;
}
} // namespace Media
} // namespace OHOS
